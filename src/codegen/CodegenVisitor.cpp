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
  Function* func;

  std::string funcName = ctx->fh->id->getText();
  if (funcName == "program") //TODO: semantic check that this function exists
  {
    FunctionType *mainFuncType = FunctionType::get(Int32Ty, {Int32Ty, Int8PtrPtrTy}, false);
    func = Function::Create(mainFuncType,     GlobalValue::ExternalLinkage,
      "main", module);
  }
  else
  {
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
    func = Function::Create(funcType, GlobalValue::ExternalLinkage, funcName, module);
  }

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
  return (Value*) Int32Zero;
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
  if (!symbol)
  {
    errors.addCodegenError(ctx->getStart(), "No associated symbol for the given context.");
    return v;
  }
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
      std::any v = arg->accept(this);
      args.push_back(std::any_cast<Value *>(v));
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
  builder->CreateRet(v);
  return v;
}

std::any CodegenVisitor::visitConstant(WPLParser::ConstantContext *ctx) {
  Value* v = Int32Zero;
  if (ctx->BOOLEAN())
  {
    if (ctx->getText() == "true")
    {
      v = builder->getInt1(1);
    }
    else
    {
      v = builder->getInt1(0);
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
    // remove quotations added by getText()
    s.erase(s.length()-1, 1);
    s.erase(0, 1);
    
    // convert \n to newline characters
    for (int i = 0; i < s.length(); i++)
    {
      if (s.length() <= i) break; // this loop is shrinking the string
      if (s[i] == '\\' && s[i+1] == 'n')
      {
        s.erase(i, 2);
        s.insert(i, "\n");
      }
    }
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

std::any CodegenVisitor::visitAndExpr(WPLParser::AndExprContext *ctx) {
  Value *lVal = std::any_cast<Value *>(ctx->left->accept(this));
  Value *rVal = std::any_cast<Value *>(ctx->right->accept(this));
  Value *v = builder->CreateAnd(lVal, rVal);
  return v;
}

std::any CodegenVisitor::visitOrExpr(WPLParser::OrExprContext *ctx) {
  Value *lVal = std::any_cast<Value *>(ctx->left->accept(this));
  Value *rVal = std::any_cast<Value *>(ctx->right->accept(this));
  Value *v = builder->CreateOr(lVal, rVal);
  return v;
}

std::any CodegenVisitor::visitNotExpr(WPLParser::NotExprContext *ctx) {
  Value *e = std::any_cast<Value *>(ctx->e->accept(this));
  Value *v = builder->CreateNot(e);
  return v;
}

std::any CodegenVisitor::visitMultExpr(WPLParser::MultExprContext *ctx) {
  Value *lVal = std::any_cast<Value *>(ctx->left->accept(this));
  Value *rVal = std::any_cast<Value *>(ctx->right->accept(this));
  Value *v;
  if (ctx->MUL())
  {
    v = builder->CreateNSWMul(lVal, rVal);
  }
  if (ctx->DIV())
  {
    v = builder->CreateSDiv(lVal, rVal);
  }
  return v;
}

std::any CodegenVisitor::visitAddExpr(WPLParser::AddExprContext *ctx) {
  Value *lVal = std::any_cast<Value *>(ctx->left->accept(this));
  Value *rVal = std::any_cast<Value *>(ctx->right->accept(this));
  Value *v;
  if (ctx->PLUS())
  {
    v = builder->CreateNSWAdd(lVal, rVal);
  }
  if (ctx->MINUS())
  {
    v = builder->CreateNSWSub(lVal, rVal);
  }
  return v;
}

std::any CodegenVisitor::visitUMinusExpr(WPLParser::UMinusExprContext *ctx) {
  Value *e = std::any_cast<Value *>(ctx->e->accept(this));
  Value *v = builder->CreateNSWSub(Int32Zero, e);
  return v;
}

std::any CodegenVisitor::visitConditional(WPLParser::ConditionalContext *ctx) {
  Value* v = Int32Zero;
  
  Function* func = builder->GetInsertBlock()->getParent(); 
  
  // true block
  BasicBlock *trueblock = BasicBlock::Create(module->getContext(), "truebloc", func);
  // false block
  BasicBlock *falseblock = BasicBlock::Create(module->getContext(), "falsebloc", func);

  // continue block
  BasicBlock *continueblock = BasicBlock::Create(module->getContext(), "bContinue", func);
  Value* eresult = std::any_cast<Value*>(ctx->e->accept(this));
  builder->CreateCondBr(eresult, trueblock, falseblock);

  // true block code
  builder->SetInsertPoint(trueblock);
  Value *b1result = std::any_cast<Value*>(ctx->yesblock->accept(this));
  builder->CreateBr(continueblock);   // go to the continuation

  // false block code
  builder->SetInsertPoint(falseblock);
  if (ctx->noblock)
  {
    Value *b1result = std::any_cast<Value*>(ctx->noblock->accept(this));
  }
  builder->CreateBr(continueblock); // go to the continuation

  builder->SetInsertPoint(continueblock);

  return v;
}

std::any CodegenVisitor::visitSelect(WPLParser::SelectContext *ctx) {
  Value* v = Int32Zero;

  Function* func = builder->GetInsertBlock()->getParent(); 

  std::vector<BasicBlock*> yesblocs;
  std::vector<BasicBlock*> condblocs;

  for (int i = 0; i < ctx->selectAlt().size(); i++)
  {
    WPLParser::SelectAltContext* alt = ctx->selectAlt()[i];
    yesblocs.push_back(BasicBlock::Create(module->getContext(), "selectbloc", func));
    condblocs.push_back(BasicBlock::Create(module->getContext(), "condbloc", func));
    Value* eresult = std::any_cast<Value*>(alt->e->accept(this));

    builder->CreateCondBr(eresult, yesblocs[i], condblocs[i]);
    builder->SetInsertPoint(condblocs[i]);
  }

  // continue block
  BasicBlock *continueblock = BasicBlock::Create(module->getContext(), "continue", func);
  builder->CreateBr(continueblock); // last false case, go to continue block

  for (int i = 0; i < ctx->selectAlt().size(); i++)
  {
    WPLParser::SelectAltContext* alt = ctx->selectAlt()[i];
    builder->SetInsertPoint(yesblocs[i]);
    Value* blocResult = std::any_cast<Value*>(alt->s->accept(this));
    builder->CreateBr(continueblock);
  }

  builder->SetInsertPoint(continueblock);

  return v;
}

std::any CodegenVisitor::visitLoop(WPLParser::LoopContext *ctx) {
  Value* v = Int32Zero;

  Function* func = builder->GetInsertBlock()->getParent(); 
  
  BasicBlock *condblock = BasicBlock::Create(module->getContext(), "condbloc", func);
  BasicBlock *loopblock = BasicBlock::Create(module->getContext(), "loopbloc", func);
  BasicBlock *continueblock = BasicBlock::Create(module->getContext(), "continuebloc", func);

  builder->CreateBr(condblock);
  builder->SetInsertPoint(condblock);
  Value* eresult = std::any_cast<Value*>(ctx->e->accept(this));
  builder->CreateCondBr(eresult, loopblock, continueblock);

  // loop block code
  builder->SetInsertPoint(loopblock);
  Value *loopblocresult = std::any_cast<Value*>(ctx->b->accept(this));
  builder->CreateBr(condblock);   // go back to the condition

  builder->SetInsertPoint(continueblock);
  return v;
}

// std::any CodegenVisitor::visitSubscriptExpr(WPLParser::SubscriptExprContext *ctx) {
//   return SymType::UNDEFINED;
// }

// std::any CodegenVisitor::visitArrayLengthExpr(WPLParser::ArrayLengthExprContext *ctx) {
//   return SymType::UNDEFINED;
// }

// std::any CodegenVisitor::visitArrayIndex(WPLParser::ArrayIndexContext *ctx) {
//   return SymType::UNDEFINED;
// }

std::any CodegenVisitor::visitParenExpr(WPLParser::ParenExprContext *ctx) {
  return std::any_cast<Value *>(ctx->expr()->accept(this));
}

std::any CodegenVisitor::visitBlock(WPLParser::BlockContext *ctx) {
  Value* v = Int32Zero;
  for (WPLParser::StatementContext* sctx : ctx->statement())
  {
    v = std::any_cast<Value*>(sctx->accept(this));
  }
  return v;
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
