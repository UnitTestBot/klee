#include "TypeManager.h"

#include "klee/Module/KType.h"
#include "klee/Module/KModule.h"
#include "klee/Module/KInstruction.h"

#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/Casting.h"

#include <vector>
#include <unordered_set>
#include <unordered_map>

using namespace klee;


/**
 * Initializes type system with raw llvm types.
 */
TypeManager::TypeManager(KModule *parent) : parent(parent) {}


/**
 * Computes KType for given type, and cache it, if it was not
 * inititalized before. So, any two calls with the same argument
 * will return same KType's.
 */
KType *TypeManager::getWrappedType(llvm::Type *type) {
  if (typesMap.count(type) == 0) {
    types.emplace_back(new KType(type, this));
    typesMap.emplace(type, types.back().get());
  }
  return typesMap[type];
}


/** 
 * "Language-specific function", as calls in high level languages
 * can affect type system. By default, it does nothing.
 */
void TypeManager::handleFunctionCall(KFunction *function, std::vector<MemoryObject *> &args) {}


/**
 * Performs initialization for struct types, including inner types.
 * Note, that initialization for structs differs from initialization
 * for other types, as types from structs can create cyclic dependencies,
 * and that is why it cannot be done in constructor. 
 */
void TypeManager::initTypesFromStructs() {
  /*
   * To collect information about all inner types 
   * we will topologically sort dependencies between structures
   * (e.g. if struct A contains class B, we will make edge from A to B)
   * and pull types to top.
   */
  std::unordered_map<llvm::StructType*, std::vector<llvm::StructType*>> structTypesGraph;
  
  std::vector<llvm::StructType *> collectedStructTypes = parent->module->getIdentifiedStructTypes();
  for (auto &typesToOffsets : typesMap) {
    if (llvm::isa<llvm::StructType>(typesToOffsets.first)) {
      collectedStructTypes.emplace_back(llvm::cast<llvm::StructType>(typesToOffsets.first));
    }
  }

  for (auto structType :collectedStructTypes) {    
    getWrappedType(structType);

    if (structTypesGraph.count(structType) == 0) {
      structTypesGraph.emplace(structType, std::vector<llvm::StructType*>());
    }

    for (auto structTypeMember : structType->elements()) {
      if (structTypeMember->isStructTy()) {      
        structTypesGraph[structType].emplace_back(llvm::cast<llvm::StructType>(structTypeMember));
      }
      /* Note that we initialize all members anyway */
      getWrappedType(structTypeMember);
    }
  }

  std::vector<llvm::StructType*> sortedStructTypesGraph;
  std::unordered_set<llvm::Type *> visitedStructTypesGraph;

  std::function<void(llvm::StructType*)> dfs = [&structTypesGraph,
                                          &sortedStructTypesGraph, 
                                          &visitedStructTypesGraph,
                                          &dfs](llvm::StructType *type) {
    visitedStructTypesGraph.insert(type);
    
    for (auto typeTo : structTypesGraph[type]) {
      if (visitedStructTypesGraph.count(typeTo) == 0) {
        dfs(typeTo);
      }
    }

    sortedStructTypesGraph.push_back(type);
  }; 

  for (auto &typeToOffset : structTypesGraph) {
    dfs(typeToOffset.first);
  }

  for (auto structType : sortedStructTypesGraph) {    
    /* Here we make initializaion for inner types of given structure type */
    const llvm::StructLayout *structLayout = parent->targetData->getStructLayout(structType);
    for (unsigned idx = 0; idx < structType->getNumElements(); ++idx) {
      uint64_t offset = structLayout->getElementOffset(idx);
      llvm::Type *rawElementType = structType->getElementType(idx);
      typesMap[structType]->innerTypes[typesMap[rawElementType]].push_back(offset);

      /* Provide initialization from types in inner class */
      for (auto &innerStructMemberTypesToOffsets : typesMap[rawElementType]->innerTypes) {
        KType *innerStructMemberType = innerStructMemberTypesToOffsets.first;
        const std::vector<uint64_t> &innerTypeOffsets = innerStructMemberTypesToOffsets.second;
        
        /* Add offsets from inner class */
        for (uint64_t innerTypeOffset : innerTypeOffsets) {
          typesMap[structType]->innerTypes[innerStructMemberType].emplace_back(offset + innerTypeOffset);
        }

      }
    }
  }
}


/**
 * Performs type system initialization for global objects.
 */
void TypeManager::initTypesFromGlobals() {
  for (auto &global : parent->module->getGlobalList()) {
    getWrappedType(global.getType());
  }
}



/**
 * Performs type system initialization for all instructions in
 * this module. Takes into consideration return and argument types.
 */
void TypeManager::initTypesFromInstructions() {
  for (auto &function : *(parent->module)) {
    auto kf = parent->functionMap[&function];
    
    for (auto &BasicBlock : function) {
      unsigned numInstructions = kf->blockMap[&BasicBlock]->numInstructions;
      KBlock *kb = kf->blockMap[&BasicBlock];
      
      for (unsigned i=0; i<numInstructions; ++i) {
        llvm::Instruction* inst = kb->instructions[i]->inst;
        
        /* Register return type */
        getWrappedType(inst->getType());

        /* Register types for arguments */
        for (auto opb = inst->op_begin(), ope = inst->op_end(); opb != ope; ++opb) {
          getWrappedType((*opb)->getType());
        }

      }
    }
  }
}


void TypeManager::postInitModule() {}


/**
 * Method to initialize all types in given module.
 * Note, that it cannot be called in costructor 
 * as implementation of getWrappedType can be different
 * for high-level languages. Note, that struct types is 
 * called last, as it is required to know about all 
 * structure types in code.
 */
void TypeManager::initModule() {
  initTypesFromGlobals();
  initTypesFromInstructions();
  initTypesFromStructs();
  postInitModule();
}


TypeManager *TypeManager::getTypeManager(KModule *module) {
  TypeManager *manager = new TypeManager(module);
  manager->initModule();
  return manager;
}