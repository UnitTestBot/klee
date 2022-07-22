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
TypeManager::TypeManager(KModule *parent) : parent(parent) {
  initTypesFromStructs();
  initTypesFromGlobals();
  initTypesFromInstructions();
}


/**
 * Computes KType for given type, and cache it, if it was not
 * inititalized before. So, any two calls with the same argument
 * will return same KType's.
 */
KType *TypeManager::getWrappedType(llvm::Type *type) {
  if (typesMap.count(type) == 0) {
    /// TODO: provide full initialization for inner types
    types.emplace_back(new KType(type, this));
    typesMap.emplace(type, types.back().get());
  }
  return typesMap[type];
}


/** 
 * "Language-specific function", as calls in high level languages
 * can affect type system. By default, it does nothing.
 */
void TypeManager::handleFunctionCall(KFunction *function, std::vector<MemoryObject *> &args) const {}


/**
 * Performs initialization for struct types, including inner types.
 */
void TypeManager::initTypesFromStructs() {
  /*
   * To collect information about all inner types 
   * we will topologically sort dependencies between structures
   * (e.g. if struct A contains class B, we will make edge from A to B)
   * and pull types to top.
   */
  std::unordered_map<llvm::Type*, std::vector<llvm::Type*>> typesGraph;

  for (auto structType : parent->module->getIdentifiedStructTypes()) {
    if (typesGraph.count(structType) == 0) {
      typesGraph.emplace(structType, std::vector<llvm::Type*>());
    }

    for (auto structMemberType : structType->elements()) {
      /* Note, that here we add not only struct types */
      typesGraph[structType].emplace_back(structMemberType);
    }
  }

  std::vector<llvm::Type*> sortedTypesGraph;
  std::unordered_set<llvm::Type *> visitedGraphTypes;

  std::function<void(llvm::Type*)> dfs = [&typesGraph,
                                          &sortedTypesGraph, 
                                          &visitedGraphTypes,
                                          &dfs](llvm::Type *type) {
    visitedGraphTypes.insert(type);
    
    for (auto typeTo : typesGraph[type]) {
      if (visitedGraphTypes.count(typeTo) == 0) {
        dfs(typeTo);
      }
    }

    sortedTypesGraph.push_back(type);
  }; 

  for (auto &typeToOffset : typesGraph) {
    dfs(typeToOffset.first);
  }

  for (auto type : sortedTypesGraph) {
    getWrappedType(type);
  }
}


/**
 * Performs type system initialization for global objects.
 */
void TypeManager::initTypesFromGlobals() {
  for (auto &global : parent->module->getGlobalList()) {
    if (!llvm::isa<llvm::StructType>(global.getType())) {
      getWrappedType(global.getType());
    }
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
        if (!llvm::isa<llvm::StructType>(inst->getType())) {
          getWrappedType(inst->getType());
        }

        /* Register types for arguments */
        for (auto opb = inst->op_begin(), ope = inst->op_end(); opb != ope; ++opb) {
          if (!llvm::isa<llvm::StructType>((*opb)->getType())) {
            getWrappedType((*opb)->getType());
          }
        }

      }
    }
  }
}