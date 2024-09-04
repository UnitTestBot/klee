// This file is part of TestCov,
// a robust test executor with reliable coverage measurement:
// https://gitlab.com/sosy-lab/software/test-suite-validator/
//
// SPDX-FileCopyrightText: 2021 Dirk Beyer <https://www.sosy-lab.org>
//
// SPDX-License-Identifier: Apache-2.0

#include "Includes.hpp"
#include "LabelerASTVisitor.hpp"

#ifndef LABELER_AST_CONSUMER_HPP
#define LABELER_AST_CONSUMER_HPP

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class LabelerASTConsumer : public ASTConsumer {
public:
  LabelerASTConsumer(ASTContext &Context, Rewriter &R,
                     LabelOptions labelOptions);

  // Override the method that gets called for each parsed top-level
  // declaration.
  bool HandleTopLevelDecl(DeclGroupRef DR) override;

private:
  LabelerASTVisitor Visitor;
  // A struct, where all options are stored in
  LabelOptions options;
};

#endif