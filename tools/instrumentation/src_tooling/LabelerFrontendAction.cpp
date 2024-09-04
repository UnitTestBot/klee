// This file is part of TestCov,
// a robust test executor with reliable coverage measurement:
// https://gitlab.com/sosy-lab/software/test-suite-validator/
//
// SPDX-FileCopyrightText: 2021 Dirk Beyer <https://www.sosy-lab.org>
//
// SPDX-License-Identifier: Apache-2.0

#include "LabelerFrontendAction.hpp"
#include "Includes.hpp"
#include "LabelerASTConsumer.hpp"

LabelerFrontendAction::LabelerFrontendAction(LabelOptions labelOptions)
    : options(labelOptions) {}
void LabelerFrontendAction::EndSourceFileAction() {
  SourceManager &SM = labelAddRewriter.getSourceMgr();
  if (!options.noInfo) {
    llvm::errs() << "** EndSourceFileAction for: "
                 << SM.getFileEntryForID(SM.getMainFileID())->getName() << "\n";
  }

  // Now emit the rewritten buffer.
  if (options.inPlace) {
    labelAddRewriter.overwriteChangedFiles();
  } else {
    labelAddRewriter.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
  }
  SM.getMainFileID();
}

std::unique_ptr<ASTConsumer>
LabelerFrontendAction::CreateASTConsumer(CompilerInstance &CI, StringRef file) {
  if (!options.noInfo) {
    llvm::errs() << "** Creating AST consumer for: " << file << "\n";
  }
  labelAddRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
  return std::make_unique<LabelerASTConsumer>(CI.getASTContext(),
                                              labelAddRewriter, options);
}
