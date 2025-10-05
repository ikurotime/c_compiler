#pragma once

#include <format>
#include <print>
#include <map>
#include <string_view>
#include <algorithm>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include "ast.hpp"

using namespace std;

class CodeGen
{
public:
  CodeGen()
    : context(make_unique<llvm::LLVMContext>()),
      module(make_unique<llvm::Module>("Husk", *context)),
      builder(make_unique<llvm::IRBuilder<>>(*context))
  {
  }

private:
  // helper to create integer constant
  llvm::ConstantInt* createInt32(int value) const
  {
    return llvm::ConstantInt::get(*context, llvm::APInt(32, value));
  }
  
  // helper to get int32 type
  llvm::Type* getInt32Type() const
  {
    return llvm::Type::getInt32Ty(*context);
  }
  
  // helper to create alloca for variable
  llvm::AllocaInst* createVariableAlloca(const string& name)
  {
    return builder->CreateAlloca(getInt32Type(), nullptr, name.c_str());
  }
  
  // helper to generate binary operation
  auto generateBinaryOp(llvm::Value* lhs, llvm::Value* rhs, TokenType op) -> Expected<llvm::Value*>
  {
    switch (op) {
      case TokenType::plus:
        return builder->CreateAdd(lhs, rhs, "addtmp");
      case TokenType::minus:
        return builder->CreateSub(lhs, rhs, "subtmp");
      case TokenType::star:
        return builder->CreateMul(lhs, rhs, "multmp");
      case TokenType::fslash:
        return builder->CreateSDiv(lhs, rhs, "divtmp");
      default:
        return llvm::createStringError(llvm::inconvertibleErrorCode(), "Unsupported binary operator");
    }
  }
  
  // helper to check if variable exists
  bool variableExists(const string& name) const
  {
    return variables.contains(name);
  }

public:
  auto generate_statement(const ASTStmt& stmt) -> llvm::Error
  {
    return visit([this](auto&& arg) -> llvm::Error {
      using T = decay_t<decltype(arg)>;
      
      if constexpr (is_same_v<T, ASTLetStmt>) {
        return generateLetStatement(arg);
      }
      else if constexpr (is_same_v<T, ASTPrintStmt>) {
        return generatePrintStatement(arg);
      }
      else if constexpr (is_same_v<T, ASTExprStmt>) {
        return generateExpressionStatement(arg);
      }
      else if constexpr (is_same_v<T, ASTReturnStmt>) {
        return generateReturnStatement(arg);
      }
      
      return llvm::createStringError(llvm::inconvertibleErrorCode(), "Unknown statement type");
    }, stmt.var);
  }

private:
  // Generate let statement: let x = expr;
  auto generateLetStatement(const ASTLetStmt& stmt) -> llvm::Error
  {
    const auto varName = stmt.ident.value.value_or("");
    
    // Check if variable already exists in current scope
    if (variableExists(varName)) {
      return llvm::createStringError(
        llvm::inconvertibleErrorCode(), 
        format("Variable '{}' is already declared in this scope", varName)
      );
    }
    
    auto* alloca = createVariableAlloca(varName);
    variables[varName] = alloca;
    
    auto initValue = generateExpr(stmt.expr);
    if (!initValue) {
      return initValue.takeError();
    }
    
    builder->CreateStore(*initValue, alloca);
    return llvm::Error::success();
  }
  
  // Generate print statement: print(expr);
  auto generatePrintStatement(const ASTPrintStmt& stmt) -> llvm::Error
  {
    auto result = generateExpr(stmt.expr);
    if (!result) {
      return result.takeError();
    }
    
    createPrintFunction(*result);
    return llvm::Error::success();
  }
  
  // Generate expression statement: expr;
  auto generateExpressionStatement(const ASTExprStmt& stmt) -> llvm::Error
  {
    auto result = generateExpr(stmt.expr);
    if (!result) {
      return result.takeError();
    }
    return llvm::Error::success();
  }
  
  // Generate return statement: return expr;
  auto generateReturnStatement(const ASTReturnStmt& stmt) -> llvm::Error
  {
    auto result = generateExpr(stmt.expr);
    if (!result) {
      return result.takeError();
    }
    
    builder->CreateRet(*result);
    return llvm::Error::success();
  }

public:
  auto generate_function(const ASTFunction& func) -> llvm::Error
  {
    auto funcName = func.name.value.value_or("unknown");
    auto* llvmFunc = createFunction(funcName);
    
    setupFunctionBody(llvmFunc);
    
    auto result = generateFunctionBody(func.body, funcName);
    if (result) {
      return result;
    }
    
    addDefaultReturnIfNeeded(func.body);
    return llvm::Error::success();
  }

private:
  // create function declaration
  llvm::Function* createFunction(const string& name)
  {
    auto* funcType = llvm::FunctionType::get(getInt32Type(), false);
    return llvm::Function::Create(
      funcType,
      llvm::Function::ExternalLinkage,
      name.c_str(),
      module.get()
    );
  }
  
  // setup function entry block
  void setupFunctionBody(llvm::Function* func)
  {
    auto* entry = llvm::BasicBlock::Create(*context, "entry", func);
    builder->SetInsertPoint(entry);
    variables.clear(); // Clear variables for new function scope
  }
  
  // generate function body statements
  auto generateFunctionBody(const vector<ASTStmt>& body, const string& funcName) -> llvm::Error
  {
    for (const auto& stmt : body) {
      auto result = generate_statement(stmt);
      if (result) {
        return llvm::createStringError(
          llvm::inconvertibleErrorCode(), 
          format("In function '{}': {}", funcName, toString(std::move(result)))
        );
      }
    }
    return llvm::Error::success();
  }
  
  // add default return 0 if no explicit return
  void addDefaultReturnIfNeeded(const vector<ASTStmt>& body)
  {
    bool has_return = any_of(body.begin(), body.end(),
                            [](const ASTStmt& stmt) {
                              return holds_alternative<ASTReturnStmt>(stmt.var);
                            });
    
    if (!has_return) {
      builder->CreateRet(createInt32(0));
    }
  }

public:
  auto generate(const ASTProgram& program) -> llvm::Error
  {
    // generate each function
    for (const auto& func : program.functions) {
      auto result = generate_function(func);
      if (result) {
        return result;
      }
    }
    
    return llvm::Error::success();
  }

  unique_ptr<llvm::Module> getModule()
  {
    return move(module);
  }

private:
  // generate code for primary expression
  auto generatePrimary(const ASTPrimaryExpr& primary) -> Expected<llvm::Value*>
  {
    if (primary.int_lit.has_value()) {
      return generateIntegerLiteral(primary.int_lit.value());
    }
    
    if (primary.ident.has_value()) {
      return generateVariableAccess(primary.ident.value());
    }
    
    return llvm::createStringError(llvm::inconvertibleErrorCode(), "Invalid primary expression");
  }
  
  // generate integer literal
  llvm::Value* generateIntegerLiteral(const Token& token)
  {
    const int val = stoi(token.value.value_or("0"));
    return createInt32(val);
  }
  
  // generate variable access
  auto generateVariableAccess(const Token& token) -> Expected<llvm::Value*>
  {
    const auto varName = token.value.value_or("");
    
    if (!variableExists(varName)) {
      return llvm::createStringError(llvm::inconvertibleErrorCode(), format("Undefined variable: {}", varName));
    }
    
    return builder->CreateLoad(getInt32Type(), variables[varName], varName.c_str());
  }

  // Generate code for expression (handles recursion)
  auto generateExpr(const ASTExpr& expr) -> Expected<llvm::Value*>
  {
    return visit([this](auto&& arg) -> Expected<llvm::Value*> {
      using T = decay_t<decltype(arg)>;
      
      if constexpr (is_same_v<T, ASTPrimaryExpr>) {
        return generatePrimary(arg);
      }
      else if constexpr (is_same_v<T, ASTBinaryExpr>) {
        return generateBinaryExpression(arg);
      }
      
      return llvm::createStringError(llvm::inconvertibleErrorCode(), "Invalid expression type");
    }, expr.var);
  }
  
  // Generate binary expression
  auto generateBinaryExpression(const ASTBinaryExpr& bin) -> Expected<llvm::Value*>
  {
    auto lhs = generatePrimary(bin.lhs);
    if (!lhs) {
      return lhs.takeError();
    }
    
    auto rhs = generateExpr(*bin.rhs);
    if (!rhs) {
      return rhs.takeError();
    }
    
    return generateBinaryOp(*lhs, *rhs, bin.op.type);
  }

  void createPrintFunction(llvm::Value* value)
  {
    llvm::FunctionCallee printfFunc = getOrCreatePrintfFunction();
    llvm::Value* formatStr = createFormatString();
    builder->CreateCall(printfFunc, {formatStr, value});
  }
  
  // Get or create printf function declaration
  llvm::FunctionCallee getOrCreatePrintfFunction()
  {
    auto* printfType = llvm::FunctionType::get(
      getInt32Type(),
      llvm::PointerType::get(*context, 0),
      true  // vararg
    );
    return module->getOrInsertFunction("printf", printfType);
  }
  
  // Create format string for printf
  llvm::Value* createFormatString()
  {
    return builder->CreateGlobalString("%d\n");
  }

  unique_ptr<llvm::LLVMContext> context;
  unique_ptr<llvm::Module> module;
  unique_ptr<llvm::IRBuilder<>> builder;
  map<string, llvm::AllocaInst*> variables;  // Symbol table
};
