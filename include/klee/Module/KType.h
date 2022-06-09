#ifndef KLEE_KTYPE_H
#define KLEE_KTYPE_H

#include "llvm/Support/raw_ostream.h"

namespace llvm {
    class Type;
}

namespace klee {
    struct KType {
    public:
        const llvm::Type *type;
        
        KType(const llvm::Type *type) : type(type) {}

        bool isCompatatibleWith(const KType &anotherType) const;

    private:
        static bool isTypesSimilar(const KType &firstType, const KType &secondType);
    };
}

#endif