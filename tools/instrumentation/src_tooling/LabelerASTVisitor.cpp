// This file is part of TestCov,
// a robust test executor with reliable coverage measurement:
// https://gitlab.com/sosy-lab/software/test-suite-validator/
//
// SPDX-FileCopyrightText: 2021 Dirk Beyer <https://www.sosy-lab.org>
//
// SPDX-License-Identifier: Apache-2.0

#include "LabelerASTVisitor.hpp"
#include "Includes.hpp"

LabelerASTVisitor::LabelerASTVisitor(ASTContext &Context, Rewriter &R,
                                     LabelOptions labelOptions)
    : context(Context), labelAddRewriter(R), options(labelOptions) {}

std::string LabelerASTVisitor::getNextLabel() {
  goalCounter++;
  return "\nGoal_" + std::to_string(goalCounter) + ":;\n";
}

SourceLocation LabelerASTVisitor::GetTrueEndLocation(Stmt *fromStatement) {
  if (isa<NullStmt>(fromStatement)) {
    return fromStatement->getEndLoc().getLocWithOffset(1);
  }
  while (isa<IfStmt>(fromStatement) || isa<WhileStmt>(fromStatement) ||
         isa<ForStmt>(fromStatement) || isa<LabelStmt>(fromStatement)) {
    if (isa<IfStmt>(fromStatement)) {
      IfStmt *ifStmt = cast<IfStmt>(fromStatement);
      if (ifStmt->getElse()) {
        fromStatement = ifStmt->getElse();
      } else {
        fromStatement = ifStmt->getThen();
      }
    }
    if (isa<WhileStmt>(fromStatement)) {
      fromStatement = cast<WhileStmt>(fromStatement)->getBody();
    }
    if (isa<ForStmt>(fromStatement)) {
      fromStatement = cast<ForStmt>(fromStatement)->getBody();
    }
    if (isa<LabelStmt>(fromStatement)) {
      fromStatement = cast<LabelStmt>(fromStatement)->getSubStmt();
    }
  }
  if (isa<CompoundStmt>(fromStatement)) {
    // the final decision of the if-statement is a compound statement with
    // curly braces, so we return the location at its closing }. Example 1: if
    // (p) {
    //  x++;
    // }
    // ^ this is returned
    //
    // Example 2:
    // if (p) {
    //  x++;
    // } else {
    //  y++;
    // }
    // ^ this is returned
    Optional<Token> nextToken = getNextToken(fromStatement->getEndLoc());
    // assert(nextToken->is(tok::r_brace));
    return nextToken->getLocation();
  }
  Optional<Token> nextToken = getNextToken(fromStatement->getEndLoc());
  if (nextToken->is(tok::semi)) {
    return nextToken->getEndLoc();
  }
  return fromStatement->getEndLoc();
}

Optional<Token> LabelerASTVisitor::getNextToken(SourceLocation fromLocation) {
  return Lexer::findNextToken(fromLocation, labelAddRewriter.getSourceMgr(),
                              labelAddRewriter.getLangOpts());
}

void LabelerASTVisitor::LabelStatement(Stmt *processedStatement) {
  if (isa<NullStmt>(processedStatement)) {
    labelAddRewriter.RemoveText(SourceRange(processedStatement->getBeginLoc(),
                                            processedStatement->getEndLoc()));
    labelAddRewriter.InsertTextAfter(processedStatement->getBeginLoc(),
                                     getNextLabel());
    return;
  }
  SourceLocation beginPos;
  // Check if Braces are missing
  if (isa<CompoundStmt>(processedStatement)) {
    // If braces are already there, beginLoc leaves us with the position
    // before the brace, so we have to offset by 1
    // The reverse applies to the closing brace, so we offset by -1
    beginPos = processedStatement->getBeginLoc().getLocWithOffset(1);
  } else {
    beginPos = processedStatement->getBeginLoc();
  }
  labelAddRewriter.InsertTextAfter(beginPos, getNextLabel());
}

bool LabelerASTVisitor::VisitIfStmt(IfStmt *S) {
  if (!isInFunction(S)) {
    return false;
  }

  Stmt *thenStatement = S->getThen();
  if (options.ifLabel) {
    LabelStatement(thenStatement);
  }

  Stmt *elseStatement = S->getElse();
  if (elseStatement && options.elseLabel) {
    if (isa<IfStmt>(elseStatement) ||
        (isa<LabelStmt>(elseStatement) &&
         isa<IfStmt>(cast<LabelStmt>(elseStatement)->getSubStmt()))) {
      return true;
    }
    LabelStatement(elseStatement);
  } else if (options.elseLabel) {
    SourceLocation endLoc = GetTrueEndLocation(S);
    labelAddRewriter.InsertTextBefore(endLoc,
                                      " else { " + getNextLabel() + "}");
  }
  return true;
}

bool LabelerASTVisitor::VisitWhileStmt(WhileStmt *S) {
  if (!isInFunction(S)) {
    return false;
  }

  if (options.ifLabel) {
    LabelStatement(S->getBody());
  }
  SourceLocation afterLoop = GetTrueEndLocation(S->getBody());
  if (options.elseLabel) {
    labelAddRewriter.InsertTextBefore(afterLoop, getNextLabel());
  }
  return true;
}

bool LabelerASTVisitor::VisitDoStmt(DoStmt *S) {
  if (!isInFunction(S)) {
    return false;
  }

  SourceLocation afterLoop = GetTrueEndLocation(S);
  if (options.elseLabel) {
    labelAddRewriter.InsertTextBefore(afterLoop, getNextLabel());
  }
  return true;
}

bool LabelerASTVisitor::VisitForStmt(ForStmt *S) {
  if (!isInFunction(S)) {
    return false;
  }

  if (options.ifLabel) {
    LabelStatement(S->getBody());
  }
  SourceLocation afterLoop = GetTrueEndLocation(S->getBody());
  if (options.elseLabel) {
    labelAddRewriter.InsertTextBefore(afterLoop, getNextLabel());
  }
  return true;
}

bool LabelerASTVisitor::VisitCaseStmt(CaseStmt *S) {
  if (!isInFunction(S)) {
    return false;
  }

  if (options.caseLabel) {
    LabelStatement(S->getSubStmt());
  }
  return true;
}

bool LabelerASTVisitor::VisitConditionalOperator(ConditionalOperator *S) {
  if (!isInFunction(S)) {
    return false;
  }

  if (options.ternaryTrueLabel) {
    SourceLocation beginOfTrueExpr = S->getTrueExpr()->getBeginLoc();
    labelAddRewriter.InsertTextBefore(beginOfTrueExpr, "({" + getNextLabel());
    SourceLocation endOfTrueExpr = S->getTrueExpr()->getEndLoc();
    labelAddRewriter.InsertTextAfterToken(endOfTrueExpr, ";})");
  }
  if (options.ternaryFalseLabel) {
    SourceLocation beginOfFalseExpr = S->getFalseExpr()->getBeginLoc();
    labelAddRewriter.InsertTextBefore(beginOfFalseExpr, "({" + getNextLabel());
    SourceLocation endOfFalseExpr = S->getFalseExpr()->getEndLoc();
    labelAddRewriter.InsertTextAfterToken(endOfFalseExpr, ";})");
  }
  return true;
}

bool LabelerASTVisitor::VisitDefaultStmt(DefaultStmt *S) {
  if (!isInFunction(S)) {
    return false;
  }

  if (options.defaultLabel) {
    LabelStatement(S->getSubStmt());
  }

  return true;
}

bool LabelerASTVisitor::VisitFunctionDecl(FunctionDecl *f) {
  // at least in clang 11, we have to reset the parentMapContext at every new
  // function that we enter. according to the documentation, all parents are
  // computed on the first call to `getParents()`, but it seems it stays within
  // the same function. Without clearing the map, all nodes in functions but the
  // first will have 0 parents.
  context.getParentMapContext().clear();
  // Only function with bodies should get labeled, not declarations.
  // maybe we have to replace this check for hashBody() with a check for
  // isThisDeclarationADefinition() in the future. But this would mean that we
  // have to do more special handling for function definitions without a body.
  if (f->hasBody()) {
    bool labelFunctionStart = options.functionStartLabel;
    if (!options.functionCall.empty()) {
      labelFunctionStart |= options.functionCall == f->getName().str();
    }
    if (labelFunctionStart) {
      LabelStatement(f->getBody());
    }
  }

  return true;
}

bool LabelerASTVisitor::isInFunction(Stmt *s) {
  return isInFunctionDyn(DynTypedNode::create(*s));
}

bool LabelerASTVisitor::isInFunctionDyn(DynTypedNode n) {
  DynTypedNodeList parents = context.getParentMapContext().getParents(n);
  for (const DynTypedNode parent : parents) {
    const Stmt *parentStmt = parent.get<Stmt>();
    if (parentStmt == NULL) {
      const Decl *parentDecl = parent.get<Decl>();
      if (parentDecl == NULL) {
        continue;
      }
      if (isa<FunctionDecl>(parentDecl)) {
        return true;
      }
    }
    if (isInFunctionDyn(parent)) {
      return true;
    }
  }
  return false;
}