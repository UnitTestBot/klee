// This file is part of TestCov,
// a robust test executor with reliable coverage measurement:
// https://gitlab.com/sosy-lab/software/test-suite-validator/
//
// SPDX-FileCopyrightText: 2021 Dirk Beyer <https://www.sosy-lab.org>
//
// SPDX-License-Identifier: Apache-2.0

#include "Includes.hpp"

#ifndef LABELER_AST_VISITOR_HPP
#define LABELER_AST_VISITOR_HPP

// Class where the actual magic of the refactoring is happening
// Statements get visited with the Visit..Stmt Methods and get refactored by the
// Rewriter.
class LabelerASTVisitor : public RecursiveASTVisitor<LabelerASTVisitor> {
public:
  LabelerASTVisitor(ASTContext &Context, Rewriter &R,
                    LabelOptions labelOptions);

  // Methods called, while the AST is traversed, need to be public, as they are
  // accessed from outside.

  // Traverses If-Statements and adds Labels to every if and else case.
  // If braces are missing, as the body of an if or else case is a one-liner
  // instead of a compound statement, braces are added. else-ifs get split.
  // Example:
  // if(a){}
  // else if(b){}
  // else{}
  // becomes
  // if(a){}
  // else{
  //   if(b){}
  //   else{}
  // }
  bool VisitIfStmt(IfStmt *S);
  // Traverse while-loop and add label to begin of body and after the
  // loop.
  bool VisitWhileStmt(WhileStmt *S);
  // Traverse for-loop and add label to begin of body and after the
  // loop.
  bool VisitForStmt(ForStmt *S);
  // Traverse Switch-Cases and add Labels to them
  bool VisitCaseStmt(CaseStmt *S);
  // Traverse Switch-Default-Caes and add Labels to them
  bool VisitDefaultStmt(DefaultStmt *S);
  // Traverse ternary statements and refactor them:
  // each decision is on separate line, starting with a goal label.
  // refactor them if so. Add Labels to the true and false casek.
  // a = (a<2) ? 1 : 2;
  // becomes
  // a = (a<2) ?
  //     Goal_1:; 1 :
  //     : Goal_2:; 2;
  bool VisitConditionalOperator(ConditionalOperator *f);
  // Traverse Functions, check if they are declarations or functions with
  // bodies. If they are functions with bodies, add Labels to them.
  bool VisitFunctionDecl(FunctionDecl *f);
  // Traverse do-while-loop and add label after the loop.
  bool VisitDoStmt(DoStmt *f);

private:
  ASTContext &context;
  // The Rewriter is storing our refactoring of the code
  Rewriter &labelAddRewriter;
  // A struct, where all options are stored in
  LabelOptions options;

  // A counter, for the value of the current label
  int goalCounter = 0;

  // Returns the next Label of the currently visited file, increases a counter
  // variable by one, so the next call of this method returns a label of higher
  // order.
  std::string getNextLabel();

  // The getEndLoc Method leaves us with the position of the first character
  // from the last token before the semicolon or normal colon of a Statement.
  // As it is currently implemented in Libtooling, the (semi)colon itself is
  // not being part of a statement. Might be source for failure,
  // if that gets changed in later LLVM releases. This Method returns the
  // SourceLocation, where the Statement really ends.
  SourceLocation GetTrueEndLocation(Stmt *fromStatement);

  // This Method transforms a Statement to a compound statement, if it is not
  // one already.
  void AddBracesAroundStatement(Stmt *processedStatement);

  void LabelStatement(Stmt *processedStatement);

  // This Method adds a Label at the begin of a statement, if beginLabel is set.
  // And a Label at the end if endLabel is set. It also transforms a statement,
  // to a compound statement, if it is a one-liner with missing braces.
  void AddBracesIfMissing(Stmt *processedStatement);

  // Refactors a ternary Statement to an if-statements and adds requested Labels
  // Currently in progress...
  void LabelTernaryStmt(ConditionalOperator *ternaryStatement,
                        std::string leftHandString);

  Optional<Token> getNextToken(SourceLocation fromLocation);

  bool isInFunction(Stmt *s);
  bool isInFunctionDyn(DynTypedNode n);
};

#endif