#ifndef KLEE_KTYPE_H
#define KLEE_KTYPE_H

#include <unordered_map>
#include <vector>
namespace llvm {
    class Type;
}

namespace klee {
    struct KType {
    public:
        
        /**
         * Represents type entry in parent type: eiether
         * struct type or primitive type. Offset field 
         * contains offset in bytes from to reach field
         * of current type. Note, that object itself
         * can be exposed in this structure.
         */
        struct TypeEntry {
            const llvm::Type *parentType;
            const uint64_t offset;
            TypeEntry(llvm::Type *parentType, uint64_t offset) 
                    : parentType(parentType), offset(offset) {}
        };

        /**
         * Wrapped type.
         */
        const llvm::Type *type;

        /**
         * Innner types. Must contains type itself and 
         * all types, that can be found in that object.
         * For example, if object of type A contains object 
         * of type B, then all types in B can be accessed via A. 
         */
        std::unordered_map<llvm::Type*, std::vector<TypeEntry>> innerTypes;
        
        /**
         * Implements rules checking for type punning. Description
         * can be found on reinterpret_cast cppreference page. 
         * Return true, if type is accessible from another, and 
         * false otherwise.
         */
        bool isAccessableFrom(const llvm::Type *anotherType) const;
    
        KType(const llvm::Type *type) : type(type) {}

    private:
        static bool isTypesSimilar(const KType &firstType, const KType &secondType);
    };
}

#endif