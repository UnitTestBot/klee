// This file is part of TestCov,
// a robust test executor with reliable coverage measurement:
// https://gitlab.com/sosy-lab/software/test-suite-validator/
//
// SPDX-FileCopyrightText: 2021 Dirk Beyer <https://www.sosy-lab.org>
//
// SPDX-License-Identifier: Apache-2.0

#include "Includes.hpp"

#ifndef LABELER_FRONTEND_ACTION_HPP
#define LABELER_FRONTEND_ACTION_HPP

// For each source-file being labeled, a new FrontendAction gets created
class LabelerFrontendAction : public ASTFrontendAction {
public:
  LabelerFrontendAction(LabelOptions labelOptions);
  void EndSourceFileAction() override;
  // A new Consumer for the AST, its job is calling the Visit...Stmt() Methods
  // of the Visitor
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override;

private:
  Rewriter labelAddRewriter;
  LabelOptions options;
};
#endif