/**
 * @file SemanticVisitor.cpp
 * @author gpollice
 * @brief AST/Parse tree visitor that creates the symbol table
 *  and does the simple type checking for the Calculator application
 * @version 0.1
 * @date 2022-07-21
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "SemanticVisitor.h"
#include <any>

std::any SemanticVisitor::visitCompilationUnit(WPLParser::CompilationUnitContext *ctx) {
  stmgr->enterScope();    // initial scope (only one for this example)
  for (auto e : ctx->components) {
    e->accept(this);
  }
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitScalarDeclaration(WPLParser::ScalarDeclarationContext *ctx) {
  SymType t = std::any_cast<SymType>(ctx->scalars[0]->accept(this));
  if (ctx->t != nullptr)
  {
    SymType declaredtype = std::any_cast<SymType>(ctx->t->accept(this));
    std::string constant = ctx->scalars[0]->vi->c->getText();
    if (declaredtype != t)
    {
      errors.addSemanticError(ctx->getStart(), "scalar declaration type mismatch. expected type " + Symbol::getSymTypeName(declaredtype) + ", got type " + Symbol::getSymTypeName(t) + " (" + constant + ")");
    }
  }

  for (unsigned long i = 1; i < ctx->scalars.size(); i++)
  {
    SymType subscalartype = std::any_cast<SymType>(ctx->scalars[i]->accept(this));
    if (subscalartype != SymType::UNDEFINED && subscalartype != t)
    {
      std::string constant = ctx->scalars[i]->vi->c->getText();
      errors.addSemanticError(ctx->getStart(), "scalar declaration type mismatch. expected type " + Symbol::getSymTypeName(t) + ", got type " + Symbol::getSymTypeName(subscalartype) + " (" + constant + ")");
    }
  }

  return t;
}

std::any SemanticVisitor::visitScalar(WPLParser::ScalarContext *ctx) {
  SymType t;
  if (ctx->vi != nullptr)
  {
    t = std::any_cast<SymType>(ctx->vi->c->accept(this));
  }
  else
  {
    t = SymType::UNDEFINED;
  }

  std::string id = ctx->id->getText();
  Symbol *symbol = stmgr->findSymbol(id);
  if (symbol == nullptr) {
    symbol = stmgr->addSymbol(id, t);
    bindings->bind(ctx, symbol);
  } else {
    errors.addSemanticError(ctx->getStart(), "variable redeclaration: " + id);
  }
  return t;
}

std::any SemanticVisitor::visitArrayDeclaration(WPLParser::ArrayDeclarationContext *ctx) {
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitType(WPLParser::TypeContext *ctx) {
  SymType t = SymType::UNDEFINED;
  if (ctx->BOOL())
  {
    t = SymType::BOOL;
  }
  else if (ctx->INT())
  {
    t = SymType::INT;
  }
  else if (ctx->STR())
  {
    t = SymType::STR;
  }
  return t;
}

std::any SemanticVisitor::visitProcedure(WPLParser::ProcedureContext *ctx) {
  std::string id = ctx->ph->id->getText();

  stmgr->enterScope();
  if (ctx->ph->p)
  {
    for (unsigned long i = 0; i < ctx->ph->p->types.size(); i++)
    {
      std::string id = ctx->ph->p->ids[i]->getText();
      SymType t = std::any_cast<SymType>(ctx->ph->p->types[i]->accept(this));
      stmgr->addSymbol(id, t);
    }
  }
  visitChildren(ctx->b);
  stmgr->exitScope();

  Symbol *symbol = stmgr->findSymbol(id);
  if (symbol == nullptr) {
    symbol = stmgr->addSymbol(id, SymType::UNDEFINED);
    bindings->bind(ctx, symbol);
  } else {
    errors.addSemanticError(ctx->getStart(), "procedure redefinition: " + id);
  }
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitExternProcHeader(WPLParser::ExternProcHeaderContext *ctx) {
  std::string id = ctx->id->getText();
  Symbol *symbol = stmgr->findSymbol(id);
  if (symbol == nullptr) {
    symbol = stmgr->addSymbol(id, SymType::UNDEFINED);
    bindings->bind(ctx, symbol);
  } else {
    errors.addSemanticError(ctx->getStart(), "procedure redefinition: " + id);
  }
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitFunction(WPLParser::FunctionContext *ctx) {
  SymType t = std::any_cast<SymType>(ctx->fh->t->accept(this));
  std::string id = ctx->fh->id->getText();

  stmgr->enterScope();
  if (ctx->fh->p)
  {
    for (unsigned long i = 0; i < ctx->fh->p->types.size(); i++)
    {
      std::string id = ctx->fh->p->ids[i]->getText();
      SymType t = std::any_cast<SymType>(ctx->fh->p->types[i]->accept(this));
      stmgr->addSymbol(id, t);
    }
  }
  visitChildren(ctx->b);
  stmgr->exitScope();

  Symbol *symbol = stmgr->findSymbol(id);
  if (symbol == nullptr) {
    symbol = stmgr->addSymbol(id, t);
    bindings->bind(ctx, symbol);
  } else {
    errors.addSemanticError(ctx->getStart(), "function redefinition: " + id);
  }
  return t;
}

std::any SemanticVisitor::visitBlock(WPLParser::BlockContext *ctx) {
  stmgr->enterScope();
  visitChildren(ctx);
  stmgr->exitScope();
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitExternFuncHeader(WPLParser::ExternFuncHeaderContext *ctx) {
  SymType t = std::any_cast<SymType>(ctx->t->accept(this));
  std::string id = ctx->id->getText();

  Symbol *symbol = stmgr->findSymbol(id);
  if (symbol == nullptr) {
    symbol = stmgr->addSymbol(id, t);
    bindings->bind(ctx, symbol);
  } else {
    errors.addSemanticError(ctx->getStart(), "function redefinition: " + id);
  }
  return t;
}

std::any SemanticVisitor::visitSelect(WPLParser::SelectContext *ctx) {
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitSelectAlt(WPLParser::SelectAltContext *ctx) {
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitCall(WPLParser::CallContext *ctx) {
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitArguments(WPLParser::ArgumentsContext *ctx) {
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitArg(WPLParser::ArgContext *ctx) {
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitReturn(WPLParser::ReturnContext *ctx) {
  SymType t = SymType::UNDEFINED;
  if (ctx->expr()) {
    t = std::any_cast<SymType>(ctx->expr()->accept(this));
  }
  // TODO: Check that this matches parent function type
  return t;
}

std::any SemanticVisitor::visitConstant(WPLParser::ConstantContext *ctx) {
  SymType t = SymType::UNDEFINED;
  if (ctx->BOOLEAN())
  {
    t = SymType::BOOL;
  }
  else if (ctx->INTEGER())
  {
    t = SymType::INT;
  }
  else if (ctx->STRING())
  {
    t = SymType::STR;
  }
  return t;
}

std::any SemanticVisitor::visitAssignment(WPLParser::AssignmentContext *ctx) {
  SymType t = SymType::UNDEFINED;

  if (ctx->targets.size() != ctx->exprs.size())
  {
    errors.addSemanticError(ctx->getStart(), "Expected equal number of target/expression pairs in assignment expression.");
    return t;
  }

  for (unsigned long i = 0; i < ctx->targets.size(); i++)
  {
    std::string id = ctx->targets[i]->getText();
    Symbol *symbol = stmgr->findSymbol(id);
    if (symbol != nullptr)
    {
      bindings->bind(ctx, symbol);
    }
    else
    {
      errors.addSemanticError(ctx->getStart(), id + " undeclared.");
      return t;
    }

    t = std::any_cast<SymType>(ctx->exprs[i]->accept(this));
    if (symbol->type != t)
    {
      errors.addSemanticError(ctx->getStart(), id + "Type mismatch. Expected " + Symbol::getSymTypeName(symbol->type) + ", got " +  Symbol::getSymTypeName(t));
    }
  }
  return t;
}

std::any SemanticVisitor::visitArrayIndex(WPLParser::ArrayIndexContext *ctx) {
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitAndExpr(WPLParser::AndExprContext *ctx) {
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitIDExpr(WPLParser::IDExprContext *ctx) {
  std::string id = ctx->ID()->getText();
  Symbol *symbol = stmgr->findSymbol(id);
  SymType t = SymType::UNDEFINED;
  if (symbol == nullptr) {
    errors.addSemanticError(ctx->getStart(), id + " undeclared.");
  } else {
    t = symbol->type;
  } 
  return t;
}

std::any SemanticVisitor::visitSubscriptExpr(WPLParser::SubscriptExprContext *ctx) {
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitRelExpr(WPLParser::RelExprContext *ctx) {
  return SymType::BOOL;
}

std::any SemanticVisitor::visitMultExpr(WPLParser::MultExprContext *ctx) {
  SymType leftt = std::any_cast<SymType>(ctx->left->accept(this));
  SymType rightt = std::any_cast<SymType>(ctx->right->accept(this));
  if (leftt != SymType::INT || rightt != SymType::INT)
  {
    errors.addSemanticError(ctx->getStart(), "cannot multiply " + Symbol::getSymTypeName(leftt) + "(" + ctx->left->getText() + ") with " + Symbol::getSymTypeName(rightt) + " (" + ctx->right->getText() + "). integers only.");
  }
  return SymType::INT;
}

std::any SemanticVisitor::visitAddExpr(WPLParser::AddExprContext *ctx) {
  SymType leftt = std::any_cast<SymType>(ctx->left->accept(this));
  SymType rightt = std::any_cast<SymType>(ctx->right->accept(this));
  if (leftt != SymType::INT || rightt != SymType::INT)
  {
    errors.addSemanticError(ctx->getStart(), "cannot add " + Symbol::getSymTypeName(leftt) + "(" + ctx->left->getText() + ") with " + Symbol::getSymTypeName(rightt) + " (" + ctx->right->getText() + "). integers only.");
  }
  return SymType::INT;
}

std::any SemanticVisitor::visitArrayLengthExpr(WPLParser::ArrayLengthExprContext *ctx) {
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitUMinusExpr(WPLParser::UMinusExprContext *ctx) {
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitOrExpr(WPLParser::OrExprContext *ctx) {
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitEqExpr(WPLParser::EqExprContext *ctx) {
  SymType leftt = std::any_cast<SymType>(ctx->left->accept(this));
  SymType rightt = std::any_cast<SymType>(ctx->right->accept(this));
  if (leftt != rightt)
  {
    errors.addSemanticError(ctx->getStart(), "cannot compare " + Symbol::getSymTypeName(leftt) + "(" + ctx->left->getText() + ") with " + Symbol::getSymTypeName(rightt) + " (" + ctx->right->getText() + "). must be same type.");
  }
  return SymType::BOOL;
}

std::any SemanticVisitor::visitFuncProcCallExpr(WPLParser::FuncProcCallExprContext *ctx) {
  std::string id = ctx->fpname->getText();
  Symbol *symbol = stmgr->findSymbol(id);
  SymType t = SymType::UNDEFINED;
  if (symbol == nullptr) {
    errors.addSemanticError(ctx->getStart(), id + " undeclared.");
  } else {
    t = symbol->type;
  } 
  // TODO verify argument types
  return t;
}

std::any SemanticVisitor::visitNotExpr(WPLParser::NotExprContext *ctx) {
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitLoop(WPLParser::LoopContext *ctx) {
  SymType condt = std::any_cast<SymType>(ctx->e->accept(this));
  if (condt != SymType::BOOL)
  {
    errors.addSemanticError(ctx->getStart(), "expected boolean expression for loop condition. got " + Symbol::getSymTypeName(condt));
  }
  
  ctx->block()->accept(this);
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitConditional(WPLParser::ConditionalContext *ctx) {
  SymType condt = std::any_cast<SymType>(ctx->e->accept(this));
  if (condt != SymType::BOOL)
  {
    errors.addSemanticError(ctx->getStart(), "expected boolean expression for 'if' condition. got " + Symbol::getSymTypeName(condt));
  }
  
  ctx->yesblock->accept(this);
  if (ctx->noblock)
  {
    ctx->noblock->accept(this);
  }

  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitParenExpr(WPLParser::ParenExprContext *ctx) {
  return std::any_cast<SymType>(ctx->expr()->accept(this));
}

// /*
// /**
//  * @brief booleanConstant.type = BOOL
//  */
// std::any SemanticVisitor::visitBooleanConstant(CalculatorParser::BooleanConstantContext *ctx) {
//   return SymType::BOOL;
// }

// /**
//  * @brief IConstExpr.type = INT
//  */
// std::any SemanticVisitor::visitIConstExpr(CalculatorParser::IConstExprContext *ctx) {
//   return SymType::INT
// }

// /**
//  * @brief ParenExpr.type = ex.type
//  */
// std::any SemanticVisitor::visitParenExpr(CalculatorParser::ParenExprContext *ctx) {
//   return ctx->ex->accept(this);
// }

// /**
//  * @brief UnaryMinusExpr.type = ex.type && ex.type == INT;
//  * @return SymTyp::INT if no error, SymType::UNDEFINED if an error.
//  */
// std::any SemanticVisitor::visitUnaryMinusExpr(CalculatorParser::UnaryMinusExprContext *ctx) {
//   auto t = std::any_cast<SymType>(ctx -> ex ->accept(this));
//   if (t != SymType::INT){ // Type mismatch
//     errors.addSemanticError(ctx->getStart(), "INT expression expected, but was " + Symbol::getSymTypeName(t));
//     t = SymType::UNDEFINED;
//   }
//   return t;
// }

// std::any SemanticVisitor::visitUnaryNotExpr(CalculatorParser::UnaryNotExprContext *ctx) {
//   auto t = std::any_cast<SymType>(ctx -> ex ->accept(this));
//   if (t != SymType::BOOL){ // Type mismatch
//     errors.addSemanticError(ctx->getStart(), "BOOL expression expected, but was " + Symbol::getSymTypeName(t));
//   }
//   return t;
// }

// /**
//  * @brief BinaryArithExpr.type = INT && left.type == INT && right.type == INT
//  * 
//  * @return SymType::INT if there are no errors or SymType::UNDEFINED if there are errors.  
//  */
// std::any SemanticVisitor::visitBinaryArithExpr(CalculatorParser::BinaryArithExprContext *ctx) {
//   SymType type = INT;
//   auto left = std::any_cast<SymType>(ctx -> left ->accept(this));
//   if (left != SymType::INT){ // Type mismatch
//     errors.addSemanticError(ctx->getStart(), "INT left expression expected, but was " + Symbol::getSymTypeName(left));
//     type = SymType::UNDEFINED;;
//   }
//   auto right = std::any_cast<SymType>(ctx -> right ->accept(this));
//   if (right != SymType::INT){ // Type mismatch
//     errors.addSemanticError(ctx->getStart(), "INT right expression expected, but was " + Symbol::getSymTypeName(right));
//     type = SymType::UNDEFINED;
//   }
//   return type;
// }

// /**
//  * @brief BinaryRelExpr.type = BOOL && left.type == INT && right.type == INT
//  * 
//  * @return SymType::BOOL if there are no errors or SymType::UNDEFINED if there are errors. 
//  */
// std::any SemanticVisitor::visitBinaryRelExpr(CalculatorParser::BinaryRelExprContext *ctx) {
//   SymType type = BOOL;
//   auto left = std::any_cast<SymType>(ctx -> left ->accept(this));
//   if (left != SymType::INT){ // Type mismatch
//     errors.addSemanticError(ctx->getStart(), "BOOL left expression expected, but was " +        Symbol::getSymTypeName(left));
//     type = SymType::UNDEFINED;
//   }
//   auto right = std::any_cast<SymType>(ctx -> right ->accept(this));
//   if (right != SymType::INT){ // Type mismatch
//     errors.addSemanticError(ctx->getStart(), "BOOL right expression expected, but was " + Symbol::getSymTypeName(right));
//     type = SymType::UNDEFINED;
//   }
//   return type;
// }

// /**
//  * @brief BinaryRelExpr.type = SymType::BOOL && left.type == right.type
//  * 
//  * @return SymType::BOOL if there are no errors or SymType::UNDEFINED if there are errors.
//  */
// std::any SemanticVisitor::visitEqExpr(CalculatorParser::EqExprContext *ctx) {
//   SymType result = SymType::BOOL;
//   result = std::any_cast<SymType>(ctx -> right ->accept(this));
//   auto left = std::any_cast<SymType>(ctx -> left ->accept(this));
//   if (result != left) {
//     errors.addSemanticError(ctx->getStart(), "Both sides of '=' must have the same type");
//     result = SymType::UNDEFINED;
//   }
//   return result;
// }

// /**
//  * @brief assignExpression.defined = TRUE 
//  *        assignExpression.type = ex.type
//  * 
//  * @return the type of the expression to which the variable is assigned 
//  */
// std::any SemanticVisitor::visitAssignExpression(CalculatorParser::AssignExpressionContext *ctx) {
//   // Visit the expression
//   SymType result = std::any_cast<SymType>(ctx->ex->accept(this));
//   if (result == UNDEFINED) {
//     errors.addSemanticError(ctx->getStart(), "Expression evaluates to an UNDEFINED type");
//   }
//   std::string varId = ctx->v->getText(); 
//   Symbol *symbol = stmgr->findSymbol(varId);
//   if (symbol == nullptr) {
//     // Undefined: add it
//     symbol = stmgr->addSymbol(varId, result);
//   } else {
//     symbol->type = result;
//   }
//   // For the Calculator, we don't have to check the type of the variable since
//   // it would be redefined here.
//   bindings->bind(ctx, symbol);
//   return result;
// }

// /**
//  * @brief v.defined == TRUE && VariableExpr.type = v.type
//  * 
//  * @return The type of the variable as found in the symbol table or UNDEFINED if
//  *  it is not defined in the symbol table.
//  */
// std::any SemanticVisitor::visitVariableExpr(CalculatorParser::VariableExprContext *ctx) {
//   SymType result = UNDEFINED;
//   std::string varId = ctx->v->getText(); 
//   Symbol *symbol = stmgr->findSymbol(varId);
//   if (symbol == nullptr) {
//     // Undefined: error
//     errors.addSemanticError(ctx->getStart(), "Undefined variable in expression: " + varId);
//   } else {
//     // bind the symbol to this node
//     bindings->bind(ctx, symbol);
//     result = symbol->type;
//   }
//   return result;
// }