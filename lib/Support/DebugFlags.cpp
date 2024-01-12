#include "klee/Support/DebugFlags.h"
#include "llvm/Support/CommandLine.h"

namespace klee {
cl::bits<DebugPrint> debugPrints(
    "debug-log", cl::desc("What categories to debug-print"),
    cl::values(clEnumValN(DebugPrint::Forward, "forward", "Forward"),
               clEnumValN(DebugPrint::Backward, "backward", "Backward"),
               clEnumValN(DebugPrint::Init, "init", "Requested inits"),
               clEnumValN(DebugPrint::Reached, "reached",
                          "Reached isolated states"),
               clEnumValN(DebugPrint::Lemma, "lemma", "Lemmas"),
               clEnumValN(DebugPrint::RootPob, "rootpob", "Root pobs"),
               clEnumValN(DebugPrint::ClosePob, "closepob", "Close pob"),
               clEnumValN(DebugPrint::Conflict, "conflict", "Conflicts"),
               clEnumValN(DebugPrint::PathForest, "pathforest", "Path forest")),
    cl::CommaSeparated);

cl::bits<DebugPrint> debugConstraints(
    "debug-constraints",
    cl::desc("What categories to print constraints for in debug prints"),
    cl::values(clEnumValN(DebugPrint::Forward, "forward", "Forward"),
               clEnumValN(DebugPrint::Backward, "backward", "Backward"),
               clEnumValN(DebugPrint::Reached, "reached",
                          "Reached isolated states"),
               clEnumValN(DebugPrint::Lemma, "lemma", "Lemmas"),
               clEnumValN(DebugPrint::Conflict, "conflict", "Conflicts")),
    cl::CommaSeparated);
}; // namespace klee
