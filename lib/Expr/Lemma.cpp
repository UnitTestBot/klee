// -*- C++ -*-
#include "klee/Expr/Lemma.h"
#include "klee/Expr/ArrayCache.h"
#include "klee/Expr/ExprBuilder.h"
#include "klee/Expr/ExprPPrinter.h"
#include "klee/Expr/Parser/Parser.h"
#include "klee/Module/KModule.h"
#include "klee/Support/DebugFlags.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/Support/OptionCategories.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

#include <fstream>
#include <memory>
#include <string>
#include <system_error>

using namespace llvm;

namespace klee {

cl::opt<std::string> SummaryFile("ksummary-file",
                                 cl::desc("File to use to read/write lemmas"),
                                 cl::init(""), cl::cat(ExecCat));

void Summary::addLemma(ref<Lemma> lemma) {
  if (!lemmas.count(lemma)) {
    lemmas.insert(lemma);

    if (debugPrints.isSet(DebugPrint::Lemma)) {
      llvm::errs() << "[lemma] New Lemma ------------------------\n";
      llvm::errs() << lemma->path.toString() << "\n";
      llvm::errs() << "Constraints [\n";
      for (auto i : lemma->constraints) {
        i->print(llvm::errs());
      }
      llvm::errs() << "]\n";
      llvm::errs() << "[lemma] New Lemma End --------------------\n";
    }

    std::error_code ec;
    raw_fd_ostream os(getFilename(), ec, sys::fs::CD_OpenAlways,
                      sys::fs::FA_Write, sys::fs::OF_Append);
    if (ec) {
      klee_error("Error while trying to write to .ksummary file.");
    }
    ExprPPrinter::printLemma(os, *lemma);
    dumped.insert(lemma);
    os << "\n";
  }
}

void Summary::dumpToFile(KModule *km) {
  std::error_code ec;
  raw_fd_ostream os(getFilename(), ec, sys::fs::CD_OpenAlways,
                    sys::fs::FA_Write, sys::fs::OF_Append);
  if (ec) {
    klee_error("Error while trying to write to .ksummary file.");
  }
  for (auto &l : lemmas) {
    if (!dumped.count(l)) {
      ExprPPrinter::printLemma(os, *l);
      dumped.insert(l);
      if (lemmas.size() != dumped.size()) {
        os << "\n";
      }
    }
  }
}

void Summary::readFromFile(KModule *km, ArrayCache *cache) {
  int fd;
  auto error = sys::fs::openFile(getFilename(), fd, sys::fs::CD_OpenAlways,
                                 sys::fs::FA_Read, sys::fs::OF_None);
  if (error) {
    klee_error("Could not open the .ksummary file.");
  }

  // what is fileSize?
  auto MBResult = MemoryBuffer::getOpenFile(fd, "", -1);
  if (!MBResult) {
    klee_error("Error during reading the .ksummary file.");
  }
  std::unique_ptr<MemoryBuffer> &MB = *MBResult;
  std::unique_ptr<ExprBuilder> Builder(createDefaultExprBuilder());
  expr::Parser *P = expr::Parser::Create("LemmaParser", MB.get(), Builder.get(),
                                         cache, km, true);
  while (auto parsed = P->ParseTopLevelDecl()) {
    if (auto lemmaDecl = dyn_cast<expr::LemmaCommand>(parsed)) {
      ref<Lemma> l(new Lemma(lemmaDecl->path, lemmaDecl->constraints));
      if (!lemmas.count(l)) {
        lemmas.insert(l);
        dumped.insert(l);
      }
    }
  }

  delete P;
}

std::string Summary::getFilename() {
  if (SummaryFile == "") {
    return ih->getOutputFilename("summary.ksummary");
  } else {
    return SummaryFile;
  }
}

}; // namespace klee
