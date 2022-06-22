#ifndef KLEE_KTYPE_H
#define KLEE_KTYPE_H

#include <unordered_map>
#include <vector>
namespace llvm {
    class Type;
}

namespace klee {    
    class KModule;

    struct KType {
    public:
        /**
         * Wrapped type.
         */
        llvm::Type *type;
        KModule *parent;

        /**
         * Innner types. Maps type to their offsets in current
         * type. Must contains type itself and 
         * all types, that can be found in that object.
         * For example, if object of type A contains object 
         * of type B, then all types in B can be accessed via A. 
         */
        std::unordered_map<llvm::Type*, std::vector<uint64_t>> innerTypes;
        
        /**
         * Implements rules checking for type punning. Description
         * can be found on reinterpret_cast cppreference page. 
         * Return true, if type is accessible from another, and 
         * false otherwise.
         */
        bool isAccessableFrom(llvm::Type *anotherType) const;

        KType(llvm::Type *, KModule*);

    private:
        bool isTypesSimilar(llvm::Type*, llvm::Type*) const;
    };
}

#endif