// This file is part of TestCov,
// a robust test executor with reliable coverage measurement:
// https://gitlab.com/sosy-lab/software/test-suite-validator/
//
// SPDX-FileCopyrightText: 2021 Dirk Beyer <https://www.sosy-lab.org>
//
// SPDX-License-Identifier: Apache-2.0

#include "Includes.hpp"

#include "LabelerASTConsumer.hpp"
#include "LabelerASTVisitor.hpp"
#include "LabelerFrontendAction.hpp"
#include "LabelerFrontendActionFactory.hpp"

static llvm::cl::OptionCategory
    InstrumentationOptions("Instrumentation Options");
// If Options
static llvm::cl::opt<bool> NoBranchingLabel(
    "no-labels-branching",
    llvm::cl::desc("Do not add labels at the beginning of control-flow "
                   "branchings (if, else, while, for)."),
    llvm::cl::cat(InstrumentationOptions));
static llvm::cl::opt<bool> BranchingLabelOnly(
    "labels-branching-only",
    llvm::cl::desc("Do only add labels at the beginning of control-flow "
                   "branchings (if, else, while, for)."),
    llvm::cl::cat(InstrumentationOptions));
// Switch Options
static llvm::cl::opt<bool>
    NoSwitchLabel("no-labels-switch",
                  llvm::cl::desc("Do not add labels at the beginnig of switch "
                                 "cases (including the default case)"),
                  llvm::cl::cat(InstrumentationOptions));
static llvm::cl::opt<bool>
    SwitchLabelOnly("labels-switch-only",
                    llvm::cl::desc("Do only add labels at the beginning of "
                                   "switch cases (including the default case)"),
                    llvm::cl::cat(InstrumentationOptions));
// Function Options
static llvm::cl::opt<bool> NoFunctionStartLabel(
    "no-labels-function-start",
    llvm::cl::desc("Do not add labels at the begin of functions"),
    llvm::cl::cat(InstrumentationOptions));
static llvm::cl::opt<bool> FunctionStartLabelOnly(
    "labels-function-start-only",
    llvm::cl::desc("Do only add labels at the begin of functions"),
    llvm::cl::cat(InstrumentationOptions));
// Ternary Options
static llvm::cl::opt<bool> NoTernaryLabel(
    "no-labels-ternary",
    llvm::cl::desc("Do not add labels at the beginning of true- and "
                   "false-expressions of ternary expressions."),
    llvm::cl::cat(InstrumentationOptions));
static llvm::cl::opt<bool> TernaryLabelOnly(
    "labels-ternary-only",
    llvm::cl::desc("Do only add labels at the beginning of true- and "
                   "false-expressions of ternary expressions."),
    llvm::cl::cat(InstrumentationOptions));
static llvm::cl::opt<std::string> FunctionCall(
    "function-call", llvm::cl::desc("Add label to begin of the given function"),
    llvm::cl::value_desc("name"), llvm::cl::cat(InstrumentationOptions));
static llvm::cl::opt<bool> FunctionCallOnly(
    "function-call-only",
    llvm::cl::desc("Add label _only_ to begin of the given function"),
    llvm::cl::value_desc("name"), llvm::cl::cat(InstrumentationOptions));

// Output Options
static llvm::cl::opt<bool> InPlace("in-place",
                                   llvm::cl::desc("Overwrite files"),
                                   llvm::cl::cat(InstrumentationOptions));
static llvm::cl::opt<bool>
    DebugInfo("debug", llvm::cl::desc("Print debug information to stderr"),
              llvm::cl::cat(InstrumentationOptions));

LabelOptions generateLabelOptions() {
  LabelOptions labelOptions = {};

  if (BranchingLabelOnly) {
    labelOptions.ifLabel = true;
    labelOptions.elseLabel = true;
  }
  if (SwitchLabelOnly) {
    labelOptions.caseLabel = true;
    labelOptions.defaultLabel = true;
  }
  if (FunctionStartLabelOnly) {
    labelOptions.functionStartLabel = true;
  }
  if (TernaryLabelOnly) {
    labelOptions.ternaryTrueLabel = true;
    labelOptions.ternaryFalseLabel = true;
  }
  if (!(BranchingLabelOnly || SwitchLabelOnly || FunctionStartLabelOnly ||
        TernaryLabelOnly || FunctionCallOnly)) {
    labelOptions.ifLabel = !NoBranchingLabel.getValue();
    labelOptions.elseLabel = !NoBranchingLabel.getValue();

    labelOptions.caseLabel = !NoSwitchLabel.getValue();
    labelOptions.defaultLabel = !NoSwitchLabel.getValue();

    labelOptions.functionStartLabel = !NoFunctionStartLabel.getValue();

    labelOptions.ternaryTrueLabel = !NoTernaryLabel.getValue();
    labelOptions.ternaryFalseLabel = !NoTernaryLabel.getValue();
  }
  labelOptions.functionCall = FunctionCall.getValue();

  labelOptions.inPlace = InPlace.getValue();
  labelOptions.noInfo = !DebugInfo.getValue();

  return labelOptions;
}

int main(int argc, const char **argv) {
  auto ExpectedParser = CommonOptionsParser::create(
      argc, argv, InstrumentationOptions,
      llvm::cl::NumOccurrencesFlag(llvm::cl::OneOrMore), NULL);
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser &op = ExpectedParser.get();
  ClangTool Tool(op.getCompilations(), op.getSourcePathList());

  return Tool.run(
      newLabelerFrontendActionFactory(generateLabelOptions()).get());
}
