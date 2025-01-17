//===-- KModule.cpp -------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "KModule"

#include "klee/Module/KModule.h"

#include "ModuleHelper.h"
#include "Passes.h"

#include "klee/Core/Interpreter.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Module/LocationInfo.h"
#include "klee/Support/Debug.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/ModuleUtil.h"
#include "klee/Support/OptionCategories.h"

#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/Scalarizer.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <memory>
#include <utility>

using namespace llvm;
using namespace klee;

namespace klee {
cl::OptionCategory
    ModuleCat("Module-related options",
              "These options affect the compile-time processing of the code.");
}

namespace klee {
cl::opt<bool> OutputSource(
    "output-source",
    cl::desc(
        "Write the assembly for the final transformed source (default=true)"),
    cl::init(true), cl::cat(ModuleCat));

cl::opt<bool>
    OutputModule("output-module",
                 cl::desc("Write the bitcode for the final transformed module"),
                 cl::init(false), cl::cat(ModuleCat));

cl::opt<bool> DebugPrintEscapingFunctions(
    "debug-print-escaping-functions",
    cl::desc("Print functions whose address is taken (default=false)"),
    cl::cat(ModuleCat));

// For testing rounding mode only
cl::opt<bool> UseKleeFloatInternals(
    "float-internals",
    cl::desc("Use KLEE internal functions for floating-point"), cl::init(true));

// Don't run VerifierPass when checking module
cl::opt<bool>
    DontVerify("disable-verify",
               cl::desc("Do not verify the module integrity (default=false)"),
               cl::init(false), cl::cat(klee::ModuleCat));

cl::opt<bool> UseKleeFERoundInternals(
    "feround-internals",
    cl::desc("USE KLEE internal functions for passing rounding mode to "
             "external calls"),
    cl::init(true));

cl::opt<bool> OptimiseKLEECall("klee-call-optimisation",
                               cl::desc("Allow optimization of functions that "
                                        "contain KLEE calls (default=true)"),
                               cl::init(true), cl::cat(ModuleCat));

cl::opt<bool>
    SplitCalls("split-calls",
               cl::desc("Split each call in own basic block (default=true)"),
               cl::init(true), cl::cat(klee::ModuleCat));

cl::opt<bool>
    StripUnwantedCalls("strip-unwanted-calls",
                       cl::desc("Strip all unwanted calls (llvm.dbg.* stuff)"),
                       cl::init(false), cl::cat(klee::ModuleCat));

cl::opt<bool> SplitReturns(
    "split-returns",
    cl::desc("Split each return in own basic block (default=true)"),
    cl::init(true), cl::cat(klee::ModuleCat));

cl::opt<SwitchImplType> SwitchType(
    "switch-type",
    cl::desc("Select the implementation of switch (default=internal)"),
    cl::values(clEnumValN(SwitchImplType::eSwitchTypeSimple, "simple",
                          "lower to ordered branches"),
               clEnumValN(SwitchImplType::eSwitchTypeLLVM, "llvm",
                          "lower using LLVM"),
               clEnumValN(SwitchImplType::eSwitchTypeInternal, "internal",
                          "execute switch internally")),
    cl::init(SwitchImplType::eSwitchTypeInternal), cl::cat(ModuleCat));
} // namespace klee

/***/

// what a hack
static Function *getStubFunctionForCtorList(Module *m, GlobalVariable *gv,
                                            std::string name) {
  assert(!gv->isDeclaration() && !gv->hasInternalLinkage() &&
         "do not support old LLVM style constructor/destructor lists");

  std::vector<Type *> nullary;

  Function *fn = Function::Create(
      FunctionType::get(Type::getVoidTy(m->getContext()), nullary, false),
      GlobalVariable::InternalLinkage, name, m);
  BasicBlock *bb = BasicBlock::Create(m->getContext(), "entry", fn);
  llvm::IRBuilder<> Builder(bb);

  // From lli:
  // Should be an array of '{ int, void ()* }' structs.  The first value is
  // the init priority, which we ignore.
  auto arr = dyn_cast<ConstantArray>(gv->getInitializer());
  if (arr) {
    for (unsigned i = 0; i < arr->getNumOperands(); i++) {
      auto cs = cast<ConstantStruct>(arr->getOperand(i));
      // There is a third element in global_ctor elements (``i8 @data``).
      assert(cs->getNumOperands() == 3 &&
             "unexpected element in ctor initializer list");
      auto fp = cs->getOperand(1);
      if (!fp->isNullValue()) {
        if (auto ce = dyn_cast<llvm::ConstantExpr>(fp))
          fp = ce->getOperand(0);

        if (auto f = dyn_cast<Function>(fp)) {
          Builder.CreateCall(f);
        } else {
          assert(0 &&
                 "unable to get function pointer from ctor initializer list");
        }
      }
    }
  }

  Builder.CreateRetVoid();

  return fn;
}

void klee::injectStaticConstructorsAndDestructors(
    Module *m, llvm::StringRef entryFunction) {
  GlobalVariable *ctors = m->getNamedGlobal("llvm.global_ctors");
  GlobalVariable *dtors = m->getNamedGlobal("llvm.global_dtors");

  if ((!ctors && !dtors) || entryFunction.empty())
    return;

  Function *mainFn = m->getFunction(entryFunction);
  if (!mainFn)
    klee_error("Entry function '%s' not found in module.",
               entryFunction.str().c_str());

  if (ctors) {
    llvm::IRBuilder<> Builder(&*mainFn->begin()->begin());
    Builder.CreateCall(getStubFunctionForCtorList(m, ctors, "klee.ctor_stub"));
  }

  if (dtors) {
    Function *dtorStub = getStubFunctionForCtorList(m, dtors, "klee.dtor_stub");
    for (Function::iterator it = mainFn->begin(), ie = mainFn->end(); it != ie;
         ++it) {
      if (isa<ReturnInst>(it->getTerminator())) {
        llvm::IRBuilder<> Builder(it->getTerminator());
        Builder.CreateCall(dtorStub);
      }
    }
  }
}

void KModule::addInternalFunction(const char *functionName) {
  Function *internalFunction = module->getFunction(functionName);
  if (!internalFunction) {
    KLEE_DEBUG(klee_warning("Failed to add internal function %s. Not found.",
                            functionName));
    return;
  }
  KLEE_DEBUG(klee_message("Added function %s.", functionName));
  internalFunctions.insert(internalFunction);
}

bool KModule::link(std::vector<std::unique_ptr<llvm::Module>> &modules,
                   unsigned flags) {
  std::string error;
  if (!module) {
    module = std::move(modules.front());
  }
  if (!klee::linkModules(module.get(), modules, flags, error)) {
    klee_error("Could not link KLEE files %s", error.c_str());
    return false;
  }

  targetData = std::make_unique<llvm::DataLayout>(module.get());
  return true;
}

void KModule::instrument(const Interpreter::ModuleOptions &opts) {
  klee::instrument(opts.CheckDivZero, opts.CheckOvershift, opts.WithFPRuntime,
                   module.get());
}

void KModule::optimiseAndPrepare(
    const Interpreter::ModuleOptions &opts,
    llvm::ArrayRef<const char *> preservedFunctions) {
  // Add internal functions which are not used to check if instructions
  // have been already visited
  if (opts.CheckDivZero)
    addInternalFunction("klee_div_zero_check");
  if (opts.CheckOvershift)
    addInternalFunction("klee_overshift_check");

  klee::optimiseAndPrepare(OptimiseKLEECall, opts.Optimize, opts.Simplify,
                           opts.WithFPRuntime, SwitchType, opts.EntryPoint,
                           preservedFunctions, module.get());
}

class InstructionToLineAnnotator : public llvm::AssemblyAnnotationWriter {
private:
  std::unordered_map<uintptr_t, uint64_t> mapping = {};

public:
  void emitInstructionAnnot(const llvm::Instruction *i,
                            llvm::formatted_raw_ostream &os) override {
    os.flush();
    mapping.emplace(reinterpret_cast<std::uintptr_t>(i), os.getLine() + 1);
  }

  void emitFunctionAnnot(const llvm::Function *f,
                         llvm::formatted_raw_ostream &os) override {
    os.flush();
    mapping.emplace(reinterpret_cast<std::uintptr_t>(f), os.getLine() + 1);
  }

  std::unordered_map<uintptr_t, uint64_t> getMapping() const { return mapping; }
};

static std::unordered_map<uintptr_t, uint64_t>
buildInstructionToLineMap(const llvm::Module &m,
                          std::unique_ptr<llvm::raw_fd_ostream> assemblyFS) {

  InstructionToLineAnnotator a;

  m.print(*assemblyFS, &a);
  assemblyFS->flush();

  return a.getMapping();
}

void KModule::manifest(InterpreterHandler *ih, bool forceSourceOutput) {

  if (OutputModule) {
    std::unique_ptr<llvm::raw_fd_ostream> f(ih->openOutputFile("final.bc"));
    llvm::WriteBitcodeToFile(*module, *f);
  }

  {
    /* Build shadow structures */
    std::unique_ptr<llvm::raw_fd_ostream> assemblyFS;
    if (OutputSource || forceSourceOutput) {
      assemblyFS = ih->openOutputFile("assembly.ll");
      asmLineMap = buildInstructionToLineMap(*module, std::move(assemblyFS));
    }
  }

  std::vector<KFunction *> declarations;

  unsigned functionID = 0;
  maxGlobalIndex = 0;
  for (auto &Function : module->functions()) {
    auto kf = std::make_unique<KFunction>(&Function, this, maxGlobalIndex);

    if (Function.isDeclaration()) {
      declarations.push_back(kf.get());
    }

    kf->id = functionID;
    functionID++;
    functionNameMap.insert({kf->getName().str(), kf.get()});
    functionMap.insert(std::make_pair(&Function, kf.get()));
    functions.push_back(std::move(kf));
  }

  unsigned globalID = 0;
  for (auto &global : module->globals()) {
    globalMap.emplace(&global, new KGlobalVariable(&global, globalID++));
  }

  /* Compute various interesting properties */

  for (auto &kf : functions) {
    if (functionEscapes(kf->function())) {
      escapingFunctions.insert(kf.get());
    }
  }

  for (auto &declaration : declarations) {
    if (functionEscapes(declaration->function()))
      escapingFunctions.insert(declaration);
  }

  for (auto &kfp : functions) {
    for (auto kcb : kfp->kCallBlocks) {
      bool isInlineAsm = false;
      const CallBase &cs = cast<CallBase>(*kcb->kcallInstruction->inst());
      Value *fp = cs.getCalledOperand();
      Function *f = getTargetFunction(fp);
      if (f) {
        auto kf = functionMap.find(getTargetFunction(fp));
        if (kf != functionMap.end()) {
          kcb->calledFunctions.insert(kf->second);
        }
      }
      if (isa<InlineAsm>(cs.getCalledOperand())) {
        isInlineAsm = true;
      }
      if (kcb->calledFunctions.empty() && !isInlineAsm) {
        kcb->calledFunctions.insert(escapingFunctions.begin(),
                                    escapingFunctions.end());
      }
      for (auto calledFunction : kcb->calledFunctions) {
        callMap[calledFunction].insert(kfp.get());
      }
    }
  }

  if (DebugPrintEscapingFunctions && !escapingFunctions.empty()) {
    llvm::errs() << "KLEE: escaping functions: [";
    std::string delimiter = "";
    for (auto &Function : escapingFunctions) {
      llvm::errs() << delimiter << Function->getName();
      delimiter = ", ";
    }
    llvm::errs() << "]\n";
  }
}

std::optional<size_t> KModule::getAsmLine(const uintptr_t ref) const {
  if (!asmLineMap.empty()) {
    return asmLineMap.at(ref);
  }
  return std::nullopt;
}
std::optional<size_t> KModule::getAsmLine(const llvm::Function *func) const {
  return getAsmLine(reinterpret_cast<std::uintptr_t>(func));
}
std::optional<size_t> KModule::getAsmLine(const llvm::Instruction *inst) const {
  return getAsmLine(reinterpret_cast<std::uintptr_t>(inst));
}

void KModule::checkModule() { klee::checkModule(DontVerify, module.get()); }

KBlock *KModule::getKBlock(const llvm::BasicBlock *bb) {
  return functionMap[bb->getParent()]->blockMap[bb];
}

bool KModule::inMainModule(const llvm::Function &f) {
  return mainModuleFunctions.count(f.getName().str()) != 0;
}

bool KModule::inMainModule(const llvm::Instruction &i) {
  return inMainModule(*i.getParent()->getParent());
}

bool KModule::inMainModule(const GlobalVariable &v) {
  return mainModuleGlobals.count(v.getName().str()) != 0;
}

Function *llvm::getTargetFunction(Value *calledVal) {
  SmallPtrSet<const GlobalValue *, 3> Visited;

  Constant *c = dyn_cast<Constant>(calledVal);
  if (!c)
    return 0;

  while (true) {
    if (GlobalValue *gv = dyn_cast<GlobalValue>(c)) {
      if (!Visited.insert(gv).second)
        return 0;

      if (Function *f = dyn_cast<Function>(gv))
        return f;
      else if (GlobalAlias *ga = dyn_cast<GlobalAlias>(gv))
        c = ga->getAliasee();
      else
        return 0;
    } else if (llvm::ConstantExpr *ce = dyn_cast<llvm::ConstantExpr>(c)) {
      if (ce->getOpcode() == Instruction::BitCast)
        c = ce->getOperand(0);
      else
        return 0;
    } else
      return 0;
  }
}

KConstant *KModule::getKConstant(const Constant *c) {
  auto it = constantMap.find(c);
  if (it != constantMap.end())
    return it->second.get();
  return NULL;
}

unsigned KModule::getConstantID(Constant *c, KInstruction *ki) {
  if (KConstant *kc = getKConstant(c))
    return kc->id;

  unsigned id = constants.size();
  auto kc = std::unique_ptr<KConstant>(new KConstant(c, id, ki));
  constantMap.insert(std::make_pair(c, std::move(kc)));
  constants.push_back(c);
  return id;
}

unsigned KModule::getFunctionId(const llvm::Function *func) const {
  return functionMap.at(func)->id;
}
unsigned KModule::getGlobalIndex(const llvm::Function *func) const {
  return functionMap.at(func)->getGlobalIndex();
}
unsigned KModule::getGlobalIndex(const llvm::Instruction *inst) const {
  return functionMap.at(inst->getFunction())
      ->instructionMap.at(inst)
      ->getGlobalIndex();
}

/***/

KConstant::KConstant(llvm::Constant *_ct, unsigned _id, KInstruction *_ki)
    : KValue(_ct, Kind::CONSTANT) {
  id = _id;
  ki = _ki;
}

bool KConstant::operator<(const KValue &rhs) const {
  return getKind() == rhs.getKind() ? id < cast<KConstant>(rhs).id
                                    : getKind() < rhs.getKind();
}

unsigned KConstant::hash() const { return id; }

KGlobalVariable::KGlobalVariable(llvm::GlobalVariable *global, unsigned id)
    : KValue(global, KValue::Kind::GLOBAL_VARIABLE), id(id) {}

std::string KGlobalVariable::getSourceFilepath() const {
  return getLocationInfo(globalVariable())->file;
}
// Line number where the global variable is defined
size_t KGlobalVariable::getLine() const {
  return getLocationInfo(globalVariable())->line;
}

bool KGlobalVariable::operator<(const KValue &rhs) const {
  return getKind() == rhs.getKind() ? id < cast<KGlobalVariable>(rhs).id
                                    : getKind() < rhs.getKind();
}
unsigned KGlobalVariable::hash() const {
  // It is good enough value to use it as hash as ID of globals
  // different.
  return id;
}

KFunction::KFunction(llvm::Function *_function, KModule *_km,
                     unsigned &globalIndexInc)
    : KCallable(_function, Kind::FUNCTION), globalIndex(globalIndexInc++),
      parent(_km), entryKBlock(nullptr), numInstructions(0) {
  for (auto &BasicBlock : *function()) {
    numInstructions += BasicBlock.size();
  }
  instructions = new KInstruction *[numInstructions];
  std::unordered_map<Instruction *, unsigned> instructionToRegisterMap;
  // Assign unique instruction IDs to each basic block
  unsigned n = 0;
  // The first arg_size() registers are reserved for formals.
  unsigned rnum = getNumArgs();
  for (auto &bb : *function()) {
    for (auto &instr : bb) {
      instructionToRegisterMap[&instr] = rnum++;
    }
  }

  for (llvm::Function::iterator bbit = function()->begin(),
                                bbie = function()->end();
       bbit != bbie; ++bbit) {
    KBlock *kb;
    Instruction *fit = &bbit->front();
    Instruction *lit = &bbit->back();
    if (SplitCalls && (isa<CallInst>(fit) || isa<InvokeInst>(fit))) {
      auto *ckb = new KCallBlock(this, &*bbit, parent, instructionToRegisterMap,
                                 &instructions[n], globalIndexInc);
      kCallBlocks.push_back(ckb);
      kb = ckb;
    } else if (SplitReturns && isa<ReturnInst>(lit)) {
      kb = new KReturnBlock(this, &*bbit, parent, instructionToRegisterMap,
                            &instructions[n], globalIndexInc);
      returnKBlocks.push_back(kb);
    } else {
      kb = new KBasicBlock(this, &*bbit, parent, instructionToRegisterMap,
                           &instructions[n], globalIndexInc);
    }
    for (unsigned i = 0, ie = kb->getNumInstructions(); i < ie; i++, n++) {
      instructionMap[instructions[n]->inst()] = instructions[n];
    }
    blockMap[&*bbit] = kb;
    blocks.push_back(std::unique_ptr<KBlock>(kb));
  }

  if (blocks.size() > 0) {
    assert(function()->begin() != function()->end());
    entryKBlock = blockMap[&*function()->begin()];
  }
}

size_t KFunction::getLine() const {
  auto locationInfo = getLocationInfo(function());
  return locationInfo->line;
}

std::string KFunction::getSourceFilepath() const {
  auto locationInfo = getLocationInfo(function());
  return locationInfo->file;
}

KFunction::~KFunction() {
  for (unsigned i = 0; i < numInstructions; ++i)
    delete instructions[i];
  delete[] instructions;
}

bool KBlockCompare::operator()(const KBlock *a, const KBlock *b) const {
  return a->parent->getGlobalIndex() < b->parent->getGlobalIndex() ||
         (a->parent->getGlobalIndex() == b->parent->getGlobalIndex() &&
          a->getId() < b->getId());
}

bool KFunctionCompare::operator()(const KFunction *a,
                                  const KFunction *b) const {
  return a->getGlobalIndex() < b->getGlobalIndex();
}

KBlock::KBlock(
    KFunction *_kfunction, llvm::BasicBlock *block, KModule *km,
    const std::unordered_map<Instruction *, unsigned> &instructionToRegisterMap,
    KInstruction **instructionsKF, unsigned &globalIndexInc,
    KBlockType blockType)
    : KValue(block, KValue::Kind::BLOCK), parent(_kfunction),
      blockKind(blockType) {
  instructions = instructionsKF;

  for (auto &it : *block) {
    KInstruction *ki;

    switch (it.getOpcode()) {
    case Instruction::GetElementPtr:
    case Instruction::InsertValue:
    case Instruction::ExtractValue:
      ki = new KGEPInstruction(instructionToRegisterMap, &it, km, this,
                               globalIndexInc);
      break;
    default:
      ki = new KInstruction(instructionToRegisterMap, &it, km, this,
                            globalIndexInc);
      break;
    }
    instructions[ki->getIndex()] = ki;
  }
}

unsigned KBlock::getGlobalIndex() const {
  return getFirstInstruction()->getGlobalIndex();
}

bool KBlock::operator<(const KValue &rhs) const {
  // Additional comparison on block types is redundant,
  // as getGlobalIndex defines the position of block.
  return getKind() == rhs.getKind()
             ? getGlobalIndex() < cast<KBlock>(rhs).getGlobalIndex()
             : getKind() < rhs.getKind();
}

unsigned KBlock::hash() const {
  // Use position of a block as a hash
  return getGlobalIndex();
}

KCallBlock::KCallBlock(
    KFunction *_kfunction, llvm::BasicBlock *block, KModule *km,
    const std::unordered_map<Instruction *, unsigned> &instructionToRegisterMap,
    KInstruction **instructionsKF, unsigned &globalIndexInc)
    : KBlock::KBlock(_kfunction, block, km, instructionToRegisterMap,
                     instructionsKF, globalIndexInc, KBlockType::Call),
      kcallInstruction(this->instructions[0]) {}

bool KCallBlock::intrinsic() const {
  if (calledFunctions.size() != 1) {
    return false;
  }
  KFunction *calledFunction = *calledFunctions.begin();
  if (calledFunction && calledFunction->kleeHandled) {
    return true;
  }
  return calledFunction->function()->getIntrinsicID() !=
         llvm::Intrinsic::not_intrinsic;
}

bool KCallBlock::internal() const {
  return calledFunctions.size() == 1 &&
         !(*calledFunctions.begin())->function()->isDeclaration();
}

KFunction *KCallBlock::getKFunction() const {
  return calledFunctions.size() == 1 ? *calledFunctions.begin() : nullptr;
}

KBasicBlock::KBasicBlock(KFunction *_kfunction, llvm::BasicBlock *block,
                         KModule *km,
                         const std::unordered_map<llvm::Instruction *, unsigned>
                             &instructionToRegisterMap,
                         KInstruction **instructionsKF,
                         unsigned &globalIndexInc)
    : KBlock::KBlock(_kfunction, block, km, instructionToRegisterMap,
                     instructionsKF, globalIndexInc, KBlockType::Base) {}

KReturnBlock::KReturnBlock(
    KFunction *_kfunction, llvm::BasicBlock *block, KModule *km,
    const std::unordered_map<Instruction *, unsigned> &instructionToRegisterMap,
    KInstruction **instructionsKF, unsigned &globalIndexInc)
    : KBlock::KBlock(_kfunction, block, km, instructionToRegisterMap,
                     instructionsKF, globalIndexInc, KBlockType::Return) {}

KBlockSet KBlock::successors() {
  KBlockSet result;
  for (auto bb : llvm::successors(basicBlock())) {
    result.insert(parent->blockMap[bb]);
  }
  return result;
}

KBlockSet KBlock::predecessors() {
  KBlockSet result;
  for (auto bb : llvm::predecessors(basicBlock())) {
    result.insert(parent->blockMap[bb]);
  }
  return result;
}

std::string KBlock::getLabel() const {
  std::string _label;
  llvm::raw_string_ostream label_stream(_label);
  basicBlock()->printAsOperand(label_stream, false);
  std::string label = label_stream.str();
  return label;
}

std::string KBlock::toString() const {
  return getLabel() + " in function " + parent->function()->getName().str();
}

uintptr_t KBlock::getId() const { return instructions - parent->instructions; }

KInstruction *KFunction::getInstructionByRegister(size_t reg) const {
  return instructions[reg - function()->arg_size()];
}

bool KFunction::operator<(const KValue &rhs) const {
  return getKind() == rhs.getKind()
             ? KFunctionCompare{}(this, cast<KFunction>(&rhs))
             : getKind() < rhs.getKind();
}

unsigned KFunction::hash() const {
  // It is good enough value to use it as
  // index is unique.
  return id;
}

size_t KFunction::getNumArgs() const { return function()->arg_size(); }
size_t KFunction::getNumRegisters() const {
  return function()->arg_size() + numInstructions;
}
