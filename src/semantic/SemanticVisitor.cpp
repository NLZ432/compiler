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
  SymType declaredtype = std::any_cast<SymType>(ctx->t->accept(this));
  for (WPLParser::ScalarContext* sctx : ctx->scalars)
  {
    // if assignment, check type
    if (sctx->vi)
    {
      SymType t = std::any_cast<SymType>(sctx->vi->c->accept(this));
      std::string constant = sctx->vi->c->getText();
      if (declaredtype != t)
      {
        errors.addSemanticError(ctx->getStart(), "scalar declaration type mismatch. expected type " + Symbol::getSymTypeName(declaredtype) + ", got type " + Symbol::getSymTypeName(t) + " (" + constant + ")");
      }
    }
    // create binding
    std::string id = sctx->id->getText();
    Symbol *symbol = stmgr->findSymbol(id);
    if (symbol == nullptr) {
      symbol = stmgr->addSymbol(id, declaredtype);
      bindings->bind(sctx, symbol);
    } else {
      errors.addSemanticError(ctx->getStart(), "variable redeclaration: " + id);
    }
  }

  return declaredtype;
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

std::any SemanticVisitor::visitSelectAlt(WPLParser::SelectAltContext *ctx) {
  SymType et = std::any_cast<SymType>(ctx->e->accept(this));
  if (et != SymType::BOOL)
  {
    errors.addSemanticError(ctx->getStart(), "expected a boolean expression, got " + ctx->e->getText());
  }
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitCall(WPLParser::CallContext *ctx) {
    std::string id = ctx->id->getText();
    Symbol *symbol = stmgr->findSymbol(id);
    if (symbol == nullptr)
    {
      errors.addSemanticError(ctx->getStart(), id + " undeclared.");
      return SymType::UNDEFINED;
    }
    // TODO make sure its actually a function
    // TODO check args
    return symbol->type;
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
      bindings->bind(ctx->exprs[i], symbol);
    }
    else
    {
      errors.addSemanticError(ctx->getStart(), id + " undeclared.");
      return t;
    }

    t = std::any_cast<SymType>(ctx->exprs[i]->accept(this));

    if (symbol->type == SymType::UNDEFINED)
    {
      symbol->type = t;
    }

    if (symbol->type != SymType::UNDEFINED && symbol->type != t)
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
  SymType leftt = std::any_cast<SymType>(ctx->left->accept(this));
  SymType rightt = std::any_cast<SymType>(ctx->right->accept(this));
  if (leftt != SymType::BOOL || rightt != SymType::BOOL)
  {
    errors.addSemanticError(ctx->getStart(), "cannot AND " + Symbol::getSymTypeName(leftt) + "(" + ctx->left->getText() + ") with " + Symbol::getSymTypeName(rightt) + " (" + ctx->right->getText() + "). booleans only.");
  }
  return SymType::BOOL;
}

std::any SemanticVisitor::visitIDExpr(WPLParser::IDExprContext *ctx) {
  std::string id = ctx->ID()->getText();
  Symbol *symbol = stmgr->findSymbol(id);
  SymType t = SymType::UNDEFINED;
  if (symbol == nullptr) {
    errors.addSemanticError(ctx->getStart(), id + " undeclared.");
  } else {
    t = symbol->type;
    bindings->bind(ctx, symbol);
  } 
  return t;
}

std::any SemanticVisitor::visitSubscriptExpr(WPLParser::SubscriptExprContext *ctx) {
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitRelExpr(WPLParser::RelExprContext *ctx) {
  SymType leftt = std::any_cast<SymType>(ctx->left->accept(this));
  SymType rightt = std::any_cast<SymType>(ctx->right->accept(this));
  if (leftt != SymType::INT || rightt != SymType::INT)
  {
    errors.addSemanticError(ctx->getStart(), "cannot compare " + Symbol::getSymTypeName(leftt) + "(" + ctx->left->getText() + ") with " + Symbol::getSymTypeName(rightt) + " (" + ctx->right->getText() + "). integers only.");
  }
  return SymType::BOOL;
}

std::any SemanticVisitor::visitMultExpr(WPLParser::MultExprContext *ctx) {
  SymType leftt = std::any_cast<SymType>(ctx->left->accept(this));
  SymType rightt = std::any_cast<SymType>(ctx->right->accept(this));
  if (leftt != SymType::INT || rightt != SymType::INT)
  {
    errors.addSemanticError(ctx->getStart(), "cannot multiply/divide " + Symbol::getSymTypeName(leftt) + "(" + ctx->left->getText() + ") with " + Symbol::getSymTypeName(rightt) + " (" + ctx->right->getText() + "). integers only.");
  }
  return SymType::INT;
}

std::any SemanticVisitor::visitAddExpr(WPLParser::AddExprContext *ctx) {
  SymType leftt = std::any_cast<SymType>(ctx->left->accept(this));
  SymType rightt = std::any_cast<SymType>(ctx->right->accept(this));
  if (leftt != SymType::INT || rightt != SymType::INT)
  {
    errors.addSemanticError(ctx->getStart(), "cannot add/subtract " + Symbol::getSymTypeName(leftt) + "(" + ctx->left->getText() + ") with " + Symbol::getSymTypeName(rightt) + " (" + ctx->right->getText() + "). integers only.");
  }
  return SymType::INT;
}

std::any SemanticVisitor::visitArrayLengthExpr(WPLParser::ArrayLengthExprContext *ctx) {
  return SymType::UNDEFINED;
}

std::any SemanticVisitor::visitUMinusExpr(WPLParser::UMinusExprContext *ctx) {
  SymType e = std::any_cast<SymType>(ctx->e->accept(this));
  if (e != SymType::INT)
  {
    errors.addSemanticError(ctx->getStart(), "expected int, got " + ctx->e->getText());
  }
  return SymType::INT;
}

std::any SemanticVisitor::visitOrExpr(WPLParser::OrExprContext *ctx) {
  SymType leftt = std::any_cast<SymType>(ctx->left->accept(this));
  SymType rightt = std::any_cast<SymType>(ctx->right->accept(this));
  if (leftt != SymType::BOOL || rightt != SymType::BOOL)
  {
    errors.addSemanticError(ctx->getStart(), "cannot OR " + Symbol::getSymTypeName(leftt) + "(" + ctx->left->getText() + ") with " + Symbol::getSymTypeName(rightt) + " (" + ctx->right->getText() + "). booleans only.");
  }
  return SymType::BOOL;
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
  SymType e = std::any_cast<SymType>(ctx->e->accept(this));
  if (e != SymType::BOOL)
  {
    errors.addSemanticError(ctx->getStart(), "expected boolean, got " + ctx->e->getText());
  }
  return SymType::BOOL;
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
