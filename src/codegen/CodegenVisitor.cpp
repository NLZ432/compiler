/**
 * @file CodegenVisitor.cpp
 * @author your name (you@domain.com)
 * @brief Implementation of the code generator visitor. 
 *  This only contains the visit() methods.
 *  The generating methods are in the CodegenGenerator.cpp file.
 * @version 0.1
 * @date 2022-08-06
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "CodegenVisitor.h"
#include <any>
#include <string>

// #define _TRACE_

using namespace llvm;

void trace(std::string message, Value *v = nullptr) {
#ifdef _TRACE_
  std::cout << message;
  if (v != nullptr) {
    std::cout.flush();
    v -> print(llvm::outs());
  }
  std::cout << std::endl;
#endif // _TRACE_
}

std::any CodegenVisitor::visitCompilationUnit(WPLParser::CompilationUnitContext *ctx) {
  // External functions
  auto printf_prototype = FunctionType::get(i8p, true);
  auto printf_fn = Function::Create(printf_prototype, Function::ExternalLinkage, "printf", module);

  FunctionCallee printExpr(printf_prototype, printf_fn);

  // Generate code for all expressions
  for (auto e : ctx->components) {
    // Generate code to output this expression
    Value* exprVal = std::any_cast<Value*>(e->accept(this));
  }

  return nullptr;
}

Type* CodegenVisitor::llvmTypeFromWPLType(WPLParser::TypeContext* tctx)
{
      if (tctx->BOOL()) return Int1Ty;
      if (tctx->INT()) return Int32Ty;
      if (tctx->STR()) return i8p;
      return VoidTy;
}

Type* CodegenVisitor::llvmTypeFromSymType(SymType t)
{
      if (t == SymType::BOOL) return Int1Ty;
      if (t == SymType::INT) return Int32Ty;
      if (t == SymType::STR) return i8p;
      return VoidTy;
}

std::any CodegenVisitor::visitFunction(WPLParser::FunctionContext *ctx) {
  Value *v;

  std::string funcName = ctx->fh->id->getText();
  Type* returntype = llvmTypeFromWPLType(ctx->fh->t);
  std::vector<Type*> argtypes;
  if (ctx->fh->p)
  {
    for (WPLParser::TypeContext* tctx : ctx->fh->p->types)
    {
      argtypes.push_back(llvmTypeFromWPLType(tctx));
    }
  }

  FunctionType *funcType = FunctionType::get(returntype, argtypes, false);
  Function *func = Function::Create(funcType, GlobalValue::ExternalLinkage, funcName, module);

  BasicBlock *bBlock = BasicBlock::Create(module->getContext(), "entry", func);
  builder->SetInsertPoint(bBlock);

  ctx->b->accept(this);

  return v;
}

std::any CodegenVisitor::visitProcedure(WPLParser::ProcedureContext *ctx) {
  Value *v;

  std::string procName = ctx->ph->id->getText();
  std::vector<Type*> argtypes;
  if (ctx->ph->p)
  {
    for (WPLParser::TypeContext* tctx : ctx->ph->p->types)
    {
      argtypes.push_back(llvmTypeFromWPLType(tctx));
    }
  }

  FunctionType *procType = FunctionType::get(VoidTy, argtypes, false);
  Function *proc = Function::Create(procType, GlobalValue::ExternalLinkage, procName, module);

  BasicBlock *bBlock = BasicBlock::Create(module->getContext(), "entry", proc);
  builder->SetInsertPoint(bBlock);

  ctx->b->accept(this);

  return v;
}

std::any CodegenVisitor::visitExternDeclaration(WPLParser::ExternDeclarationContext *ctx) { 
  Value* v = Int32Zero;
  std::string procName;
  std::vector<WPLParser::TypeContext*> parserargtypes;
  std::vector<Type*> llvmargtypes;
  Type* returntype;

  if (ctx->externProcHeader())
  {
    if (ctx->externProcHeader()->params()) parserargtypes = ctx->externProcHeader()->params()->types;
    procName = ctx->externProcHeader()->id->getText();
    returntype = VoidTy;
  }
  else if (ctx->externFuncHeader())
  {
    if (ctx->externFuncHeader()->params()) parserargtypes = ctx->externFuncHeader()->params()->types;
    procName = ctx->externFuncHeader()->id->getText();
    returntype = llvmTypeFromWPLType(ctx->externFuncHeader()->t);
  }

  for (WPLParser::TypeContext* tctx : parserargtypes)
  {
    llvmargtypes.push_back(llvmTypeFromWPLType(tctx));
  }

  auto exproc_prototype = FunctionType::get(returntype, llvmargtypes, false);
  auto exproc_fn = Function::Create(exproc_prototype, Function::ExternalLinkage, procName, module);

  FunctionCallee exExpr(exproc_prototype, exproc_fn);

  return v;
}

std::any CodegenVisitor::visitScalarDeclaration(WPLParser::ScalarDeclarationContext *ctx) {
  for (WPLParser::ScalarContext* sctx : ctx->scalars)
  {
    Symbol* symbol = props->getBinding(sctx);
    Type* type = llvmTypeFromSymType(symbol->type);
    Value* alloc = builder->CreateAlloca(type, 0, symbol->identifier);
    symbol->val = alloc;
    if (sctx->vi)
    {
      Value* v = std::any_cast<Value *>(sctx->vi->c->accept(this));
      builder->CreateStore(v, symbol->val); 
      symbol->defined = true;
    }
  }
  return Int32Zero;
}

std::any CodegenVisitor::visitAssignment(WPLParser::AssignmentContext *ctx) {
  Value* v = Int32Zero;
  for (int i = 0; i < ctx->exprs.size(); i++)
  {
    Symbol* symbol = props->getBinding(ctx->exprs[i]);
    Value* v = std::any_cast<Value *>(ctx->exprs[i]->accept(this));
    builder->CreateStore(v, symbol->val);
    symbol->defined = true;
  }
  return v;
}

std::any CodegenVisitor::visitIDExpr(WPLParser::IDExprContext *ctx) {
  Value* v = Int32Zero;
  Symbol* symbol = props->getBinding(ctx); 
  Type* type = llvmTypeFromSymType(symbol->type);
  if (!symbol->defined)
  {
    errors.addCodegenError(ctx->getStart(), "Symbol " + symbol->identifier + " has not been defined.");
    return v;
  }
  v = builder->CreateLoad(type, symbol->val, symbol->identifier);
  return v;
}

// std::any CodegenVisitor::visitArrayDeclaration(WPLParser::ArrayDeclarationContext *ctx) {
//   return SymType::UNDEFINED;
// }

std::any CodegenVisitor::visitCall(WPLParser::CallContext *ctx) {
  Value* v = Int32Zero;
  std::string id = ctx->id->getText();

  Function* called_func = module->getFunction(id);
  if (!called_func)
  {
    errors.addCodegenError(ctx->getStart(), "No definition found for function " + id);
    return v;
  }

  std::vector<Value *> args;
  if (ctx->arguments())
  {
    for (WPLParser::ArgContext* arg : ctx->arguments()->args)
    {
      args.push_back(std::any_cast<Value *>(arg->accept(this)));
    }
  }

  v = builder->CreateCall(called_func, args);
  return v;
}

std::any CodegenVisitor::visitFuncProcCallExpr(WPLParser::FuncProcCallExprContext *ctx) {
  Value* v = Int32Zero;
  std::string id = ctx->fpname->getText();

  Function* called_func = module->getFunction(id);
  if (!called_func)
  {
    errors.addCodegenError(ctx->getStart(), "No definition found for function " + id);
    return v;
  }

  std::vector<Value *> args;
  for (WPLParser::ExprContext* arg : ctx->args)
  {
    args.push_back(std::any_cast<Value *>(arg->accept(this)));
  }

  v = builder->CreateCall(called_func, args);
  return v;
}

std::any CodegenVisitor::visitReturn(WPLParser::ReturnContext *ctx) {
  Value* v = Int32Zero;
  if (ctx->expr())
  {
    v = std::any_cast<Value *>(ctx->expr()->accept(this)); 
  }
  return builder->CreateRet(v);
}

std::any CodegenVisitor::visitConstant(WPLParser::ConstantContext *ctx) {
  Value* v = Int32Zero;
  if (ctx->BOOLEAN())
  {
    if (ctx->getText() == "true")
    {
      v = builder->getInt32(1);
    }
    else
    {
      v = builder->getInt32(0);
    }
  }
  else if (ctx->INTEGER())
  {
    int i = stoi(ctx->getText());
    v = builder->getInt32(i);
  }
  else if (ctx->STRING())
  {
    std::string s = ctx->getText();
    v = builder->CreateGlobalStringPtr(s);
  }
  return v;
}

std::any CodegenVisitor::visitEqExpr(WPLParser::EqExprContext *ctx) {
  Value *lVal = std::any_cast<Value *>(ctx->left->accept(this));
  Value *rVal = std::any_cast<Value *>(ctx->right->accept(this));
  Value *v;
  if (ctx->EQUAL()) {
    v = builder->CreateICmpEQ(lVal, rVal);
  } else {
    v = builder->CreateICmpNE(lVal, rVal);
  }
  return v;
}

std::any CodegenVisitor::visitRelExpr(WPLParser::RelExprContext *ctx) {
  Value *lVal = std::any_cast<Value *>(ctx->left->accept(this));
  Value *rVal = std::any_cast<Value *>(ctx->right->accept(this));
  Value *v;
  if (ctx->LESS())
  {
    v = builder->CreateICmpSLT(lVal, rVal);
  }
  else if (ctx->LEQ())
  {
    v = builder->CreateICmpSLE(lVal, rVal);
  }
  else if (ctx->GTR())
  {
    v = builder->CreateICmpSGT(lVal, rVal);
  }
  else if (ctx->GEQ())
  {
    v = builder->CreateICmpSGE(lVal, rVal);
  }
  return v;
}

// std::any CodegenVisitor::visitMultExpr(WPLParser::MultExprContext *ctx) {
//   SymType leftt = std::any_cast<SymType>(ctx->left->accept(this));
//   SymType rightt = std::any_cast<SymType>(ctx->right->accept(this));
//   if (leftt != SymType::INT || rightt != SymType::INT)
//   {
//     errors.addSemanticError(ctx->getStart(), "cannot multiply " + Symbol::getSymTypeName(leftt) + "(" + ctx->left->getText() + ") with " + Symbol::getSymTypeName(rightt) + " (" + ctx->right->getText() + "). integers only.");
//   }
//   return SymType::INT;
// }

// std::any CodegenVisitor::visitAddExpr(WPLParser::AddExprContext *ctx) {
//   SymType leftt = std::any_cast<SymType>(ctx->left->accept(this));
//   SymType rightt = std::any_cast<SymType>(ctx->right->accept(this));
//   if (leftt != SymType::INT || rightt != SymType::INT)
//   {
//     errors.addSemanticError(ctx->getStart(), "cannot add " + Symbol::getSymTypeName(leftt) + "(" + ctx->left->getText() + ") with " + Symbol::getSymTypeName(rightt) + " (" + ctx->right->getText() + "). integers only.");
//   }
//   return SymType::INT;
// }

// std::any CodegenVisitor::visitAndExpr(WPLParser::AndExprContext *ctx) {
//   SymType leftt = std::any_cast<SymType>(ctx->left->accept(this));
//   SymType rightt = std::any_cast<SymType>(ctx->right->accept(this));
//   if (leftt != SymType::BOOL || rightt != SymType::BOOL)
//   {
//     errors.addSemanticError(ctx->getStart(), "cannot AND " + Symbol::getSymTypeName(leftt) + "(" + ctx->left->getText() + ") with " + Symbol::getSymTypeName(rightt) + " (" + ctx->right->getText() + "). booleans only.");
//   }
//   return SymType::BOOL;
// }

// std::any CodegenVisitor::visitOrExpr(WPLParser::OrExprContext *ctx) {
//   SymType leftt = std::any_cast<SymType>(ctx->left->accept(this));
//   SymType rightt = std::any_cast<SymType>(ctx->right->accept(this));
//   if (leftt != SymType::BOOL || rightt != SymType::BOOL)
//   {
//     errors.addSemanticError(ctx->getStart(), "cannot OR " + Symbol::getSymTypeName(leftt) + "(" + ctx->left->getText() + ") with " + Symbol::getSymTypeName(rightt) + " (" + ctx->right->getText() + "). booleans only.");
//   }
//   return SymType::BOOL;
// }

// std::any CodegenVisitor::visitNotExpr(WPLParser::NotExprContext *ctx) {
//   SymType e = std::any_cast<SymType>(ctx->e->accept(this));
//   if (e != SymType::BOOL)
//   {
//     errors.addSemanticError(ctx->getStart(), "expected boolean, got " + ctx->e->getText());
//   }
//   return SymType::BOOL;
// }

// std::any CodegenVisitor::visitSubscriptExpr(WPLParser::SubscriptExprContext *ctx) {
//   return SymType::UNDEFINED;
// }

// std::any CodegenVisitor::visitArrayLengthExpr(WPLParser::ArrayLengthExprContext *ctx) {
//   return SymType::UNDEFINED;
// }

// std::any CodegenVisitor::visitUMinusExpr(WPLParser::UMinusExprContext *ctx) {
//   SymType e = std::any_cast<SymType>(ctx->e->accept(this));
//   if (e != SymType::INT)
//   {
//     errors.addSemanticError(ctx->getStart(), "expected int, got " + ctx->e->getText());
//   }
//   return SymType::INT;
// }

// std::any CodegenVisitor::visitSelectAlt(WPLParser::SelectAltContext *ctx) {
//   SymType et = std::any_cast<SymType>(ctx->e->accept(this));
//   if (et != SymType::BOOL)
//   {
//     errors.addSemanticError(ctx->getStart(), "expected a boolean expression, got " + ctx->e->getText());
//   }
//   return SymType::UNDEFINED;
// }

// std::any CodegenVisitor::visitArrayIndex(WPLParser::ArrayIndexContext *ctx) {
//   return SymType::UNDEFINED;
// }


// std::any CodegenVisitor::visitLoop(WPLParser::LoopContext *ctx) {
//   SymType condt = std::any_cast<SymType>(ctx->e->accept(this));
//   if (condt != SymType::BOOL)
//   {
//     errors.addSemanticError(ctx->getStart(), "expected boolean expression for loop condition. got " + Symbol::getSymTypeName(condt));
//   }
  
//   ctx->block()->accept(this);
//   return SymType::UNDEFINED;
// }

// std::any CodegenVisitor::visitConditional(WPLParser::ConditionalContext *ctx) {
//   SymType condt = std::any_cast<SymType>(ctx->e->accept(this));
//   if (condt != SymType::BOOL)
//   {
//     errors.addSemanticError(ctx->getStart(), "expected boolean expression for 'if' condition. got " + Symbol::getSymTypeName(condt));
//   }
  
//   ctx->yesblock->accept(this);
//   if (ctx->noblock)
//   {
//     ctx->noblock->accept(this);
//   }

//   return SymType::UNDEFINED;
// }

std::any CodegenVisitor::visitParenExpr(WPLParser::ParenExprContext *ctx) {
  return std::any_cast<Value *>(ctx->expr()->accept(this));
}

// // /**
// //  * @brief Top-level visitor. Creates the beginning and end code. In between
// //  *  it visits all of the sub-expressions.
// //  * 
// //  * @param ctx 
// //  * @return std::any 
// //  */
// // std::any CodegenVisitor::visitProgram(CalculatorParser::ProgramContext *ctx) {
// //   // Generate code module header

// //   // External functions
// //     auto printf_prototype = FunctionType::get(i8p, true);
// //     auto printf_fn = Function::Create(printf_prototype, Function::ExternalLinkage, "printf", module);

// //   FunctionCallee printExpr(printf_prototype, printf_fn);

// //   // main(arg, **string) prototype
// //   FunctionType *mainFuncType = FunctionType::get(Int32Ty, {Int32Ty, Int8PtrPtrTy}, false);
// //   Function *mainFunc = Function::Create(mainFuncType,     GlobalValue::ExternalLinkage,
// //     "main", module);

// //   // Create the basic block and attach it to the builder
// //   BasicBlock *bBlock = BasicBlock::Create(module->getContext(), "entry", mainFunc);
// //   builder->SetInsertPoint(bBlock);

// //   // Generate code for all expressions
// //   for (auto e : ctx->exprs) {

// //     // Generate code to output this expression
// //     Value *exprResult = std::any_cast<Value *>(e->accept(this));  // OK

// //     auto et = e->getText(); // the text of the expression -- OK
// //     StringRef formatRef = "Expression %s evaluates to %d\n";
// //     auto gFormat = builder->CreateGlobalStringPtr(formatRef, "fmtStr");
// //     StringRef exprRef = et;
// //     auto exFormat = builder->CreateGlobalStringPtr(exprRef, "exprStr");
// //     builder->CreateCall(printf_fn, {gFormat, exFormat, exprResult});
// //   }

// //   // Generate code module trailer
// //   builder->CreateRet(Int32Zero);
// //   return nullptr;
// // }

// // std::any CodegenVisitor::visitBooleanConstant(CalculatorParser::BooleanConstantContext *ctx) {
// //   Value *v;
// //   if (ctx->val->getType() == CalculatorParser::TRUE) {
// //     v = builder->getInt32(1);
// //   } else {
// //     v = builder->getInt32(0);
// //   }
// //   return v;
// // }

// // /**
// //  * @brief Return the Value object for this integer constant.
// //  * 
// //  * @param ctx 
// //  * @return std::any (llvm::Value*) 
// //  */
// // std::any CodegenVisitor::visitIConstExpr(CalculatorParser::IConstExprContext *ctx) {
// //   int i = std::stoi(ctx->i->getText());
// //   Value *v = builder->getInt32(i);
  
// //   // trace("visitIConstExpr: ", v);

// //   return v;
// // }

// // std::any CodegenVisitor::visitParenExpr(CalculatorParser::ParenExprContext *ctx) {
// //   return ctx->ex->accept(this);
// // }

// // std::any CodegenVisitor::visitUnaryMinusExpr(CalculatorParser::UnaryMinusExprContext *ctx) {
// //   Value *exVal = std::any_cast<Value *>(ctx->ex->accept(this));
// //   Value *v = builder->CreateNSWSub(builder->getInt32(0), exVal);
// //   return v;
// // }

// // std::any CodegenVisitor::visitUnaryNotExpr(CalculatorParser::UnaryNotExprContext *ctx) {
// //   Value *v = std::any_cast<Value *>(ctx->ex->accept(this));
// //   v = builder->CreateZExtOrTrunc(v, CodegenVisitor::Int1Ty);
// //   v = builder->CreateXor(v, Int32One); 
// //   v = builder->CreateZExtOrTrunc(v, CodegenVisitor::Int32Ty);
// //   return v;
// // }

// // std::any CodegenVisitor::visitBinaryArithExpr(CalculatorParser::BinaryArithExprContext *ctx)
// // {
// //   Value *v = nullptr;
// //   Value *lVal = std::any_cast<Value *>(ctx->left->accept(this));
// //   Value *rVal = std::any_cast<Value *>(ctx->right->accept(this));
// //   auto opType = ctx->op->getType();
// //   switch (opType)
// //   {
// //   case CalculatorParser::PLUS:
// //     v = builder->CreateNSWAdd(lVal, rVal);
// //     break;
// //   case CalculatorParser::MINUS:
// //     v = builder->CreateNSWSub(lVal, rVal);
// //     break;
// //   case CalculatorParser::MULTIPLY:
// //     v = builder->CreateNSWMul(lVal, rVal);
// //     break;
// //   case CalculatorParser::DIVIDE:
// //     v = builder->CreateSDiv(lVal, rVal);
// //     break;
// //   }

// //   return v;
// // }

// // std::any CodegenVisitor::visitBinaryRelExpr(CalculatorParser::BinaryRelExprContext *ctx) {
// //   Value *v = nullptr;
// //   Value *lVal = std::any_cast<Value *>(ctx->left->accept(this));
// //   Value *rVal = std::any_cast<Value *>(ctx->right->accept(this));
// //   auto op = ctx->op->getType();
// //   Value *v1;
// //   if (op == CalculatorParser::LESS) {
// //     v1 = builder->CreateICmpSLT(lVal, rVal);
// //   } else {    // GREATER
// //     v1 = builder->CreateICmpSGT(lVal, rVal);
// //   }
// //   v = builder->CreateZExtOrTrunc(v1, CodegenVisitor::Int32Ty);
// //   return v;
// // }

// // std::any CodegenVisitor::visitEqExpr(CalculatorParser::EqExprContext *ctx) {
// //   Value *v = nullptr;
// //   Value *lVal = std::any_cast<Value *>(ctx->left->accept(this));
// //   Value *rVal = std::any_cast<Value *>(ctx->right->accept(this));
// //   auto op = ctx->op->getType();
// //   Value *v1;
// //   if (op == CalculatorParser::EQUAL) {
// //     v1 = builder->CreateICmpEQ(lVal, rVal);
// //   } else {    // UNEQUAL
// //     v1 = builder->CreateICmpNE(lVal, rVal);
// //   }
// //   v = builder->CreateZExtOrTrunc(v1, CodegenVisitor::Int32Ty);
// //   return v;
// // }

// // /**
// //  * @brief Generate code for the variable. This will be on the stack.
// //  * @see https://mukulrathi.com/create-your-own-programming-language/llvm-ir-cpp-api-tutorial/#stack-allocation
// //  * 
// //  * @param ctx 
// //  * @return Value for the assignment, which is the variable.
// //  */
// // std::any CodegenVisitor::visitAssignExpression(CalculatorParser::AssignExpressionContext *ctx) {
// //   Value *v = nullptr;
// //   Value *exVal =  std::any_cast<Value *>(ctx->ex->accept(this));
// //   Symbol *varSymbol = props->getBinding(ctx);  // child variable symbol
// //   if (varSymbol == nullptr) {
// //     trace( "NULLPTR");
// //   // } else {
// //   //   trace(varSymbol->toString());
// //   }
// //   if (!(varSymbol->defined)) {
// //     // Define the symbol and allocate memory.
// //     v = builder->CreateAlloca(CodegenVisitor::Int32Ty, 0, varSymbol->identifier);
// //     varSymbol->defined = true;
// //     varSymbol->val = v;
// //   } else {
// //     v = varSymbol->val;
// //   }
// //   builder->CreateStore(exVal, v);

// //   return exVal;
// // }

// // std::any CodegenVisitor::visitVariableExpr(CalculatorParser::VariableExprContext *ctx) {
// //   std::string varId = ctx->v->getText(); 
// //   Symbol *symbol = props->getBinding(ctx);
// //   Value *v = nullptr;
// //   // We made sure that the variable is defined in the semantic analysis phase
// //   if (!(symbol->defined)) {
// //     errors.addCodegenError(ctx->getStart(), "Undefined variable in expression: " + varId);
// //   } else {
// //     v = builder->CreateLoad(CodegenVisitor::Int32Ty, symbol->val, varId);
// //   }
// //   // trace("VisitVariableExpr " + varId + ": ", v);
// //   return v;
// // }
