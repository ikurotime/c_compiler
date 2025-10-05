#pragma once

#include <format>
#include <print>
#include <map>
#include <string_view>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include "parser.hpp"

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

  auto generate_statement(const NodeStmt& stmt) -> llvm::Error
  {
    return visit([this](auto&& arg) -> llvm::Error {
      using T = decay_t<decltype(arg)>;
      
      if constexpr (is_same_v<T, NodeStmtLet>) {
        // let statement: create variable
        const auto varName = arg.ident.value.value_or("");
        
        auto* alloca = builder->CreateAlloca(
          llvm::Type::getInt32Ty(*context),
          nullptr,
          varName.c_str()
        );
        
        variables[varName] = alloca;
        
        auto initValue = generateExpr(arg.expr);
        if (!initValue) {
          return initValue.takeError();
        }
        builder->CreateStore(*initValue, alloca);
        return llvm::Error::success();
      }
      else if constexpr (is_same_v<T, NodeStmtPrint>) {
        // print statement: evaluate and print
        auto result = generateExpr(arg.expr);
        if (!result) {
          return result.takeError();
        }
        createPrintFunction(*result);
        return llvm::Error::success();
      }
      else if constexpr (is_same_v<T, NodeStmtExpr>) {
        // expression statement: just evaluate (side effects only)
        auto result = generateExpr(arg.expr);
        if (!result) {
          return result.takeError();
        }
        return llvm::Error::success();
      }
      else if constexpr (is_same_v<T, NodeStmtReturn>) {
        // return statement: evaluate and return
        auto result = generateExpr(arg.expr);
        if (!result) {
          return result.takeError();
        }
        builder->CreateRet(*result);
        return llvm::Error::success();
      }
      
      return llvm::createStringError(llvm::inconvertibleErrorCode(), "Unknown statement type");
    }, stmt.var);
  }

  auto generate_function(const NodeFunction& func) -> llvm::Error
  {
    // Create function type: i32 @name()
    auto* funcType = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(*context),
      false
    );
    
    auto funcName = func.name.value.value_or("unknown");
    auto* llvmFunc = llvm::Function::Create(
      funcType,
      llvm::Function::ExternalLinkage,
      funcName.c_str(),
      module.get()
    );
    
    // Create entry basic block
    auto* entry = llvm::BasicBlock::Create(*context, "entry", llvmFunc);
    builder->SetInsertPoint(entry);
    
    // Clear variables for new function scope
    variables.clear();
    
    // Process each statement in function body
    bool has_return = false;
    for (const auto& stmt : func.body) {
      auto result = generate_statement(stmt);
      if (result) {
        return llvm::createStringError(llvm::inconvertibleErrorCode(), format("In function '{}': {}", funcName, toString(std::move(result))));
      }
      
      // Check if this was a return statement
      if (holds_alternative<NodeStmtReturn>(stmt.var)) {
        has_return = true;
      }
    }
    
    // If no explicit return, add default return 0
    if (!has_return) {
      builder->CreateRet(llvm::ConstantInt::get(*context, llvm::APInt(32, 0)));
    }
    
    return llvm::Error::success();
  }

  auto generate(const NodeProgram& program) -> llvm::Error
  {
    // Generate each function
    for (const auto& func : program.functions) {
      auto result = generate_function(func);
      if (result) {
        return result;
      }
    }
    
    return llvm::Error::success();
  }

  void dump() const
  {
    module->print(llvm::outs(), nullptr);
  }

  unique_ptr<llvm::Module> getModule()
  {
    return move(module);
  }

private:
  // generate code for primary expression
  auto generatePrimary(const NodeExprPrimary& primary) 
      -> Expected<llvm::Value*>
  {
    if (primary.int_lit.has_value()) {
      // literal: create constant
      const int val = stoi(primary.int_lit.value().value.value_or("0"));
      return llvm::ConstantInt::get(*context, llvm::APInt(32, val));
    }
    
    if (primary.ident.has_value()) {
      // variable: load from symbol table
      const auto varName = primary.ident.value().value.value_or("");
      
      if (!variables.contains(varName)) {
        return llvm::createStringError(llvm::inconvertibleErrorCode(), format("Undefined variable: {}", varName));
      }
      
      return builder->CreateLoad(
        llvm::Type::getInt32Ty(*context), 
        variables[varName], 
        varName.c_str()
      );
    }
    
    return llvm::createStringError(llvm::inconvertibleErrorCode(), "Invalid primary expression");
  }

  // generate code for expression (handles recursion)
  auto generateExpr(const NodeExpr& expr) 
      -> Expected<llvm::Value*>
  {
    return visit([this](auto&& arg) -> Expected<llvm::Value*> {
      using T = decay_t<decltype(arg)>;
      
      if constexpr (is_same_v<T, NodeExprPrimary>) {
        // simple primary expression
        return generatePrimary(arg);
      }
      else if constexpr (is_same_v<T, NodeExprBin>) {
        // binary operation: evaluate both sides and combine
        auto lhs = generatePrimary(arg.lhs);
        if (!lhs) {
          return lhs.takeError();
        }
        
        auto rhs = generateExpr(*arg.rhs);
        if (!rhs) {
          return rhs.takeError();
        }
        
        // Handle different operators
        switch (arg.op.type) {
          case TokenType::plus:
            return builder->CreateAdd(*lhs, *rhs, "addtmp");
          case TokenType::minus:
            return builder->CreateSub(*lhs, *rhs, "subtmp");
          case TokenType::star:
            return builder->CreateMul(*lhs, *rhs, "multmp");
          case TokenType::fslash:
            return builder->CreateSDiv(*lhs, *rhs, "divtmp");
          default:
            return llvm::createStringError(llvm::inconvertibleErrorCode(), "Unsupported binary operator");
        }
      }
      
      return llvm::createStringError(llvm::inconvertibleErrorCode(), "Invalid expression type");
    }, expr.var);
  }

  void createPrintFunction(llvm::Value* value)
  {
    // Declare printf: i32 @printf(ptr, ...)
    llvm::FunctionType* printfType = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(*context),
      llvm::PointerType::get(*context, 0),  // Use context overload
      true  // vararg
    );
    
    llvm::FunctionCallee printfFunc = module->getOrInsertFunction("printf", printfType);
    
    // Create format string "%d\n"
    llvm::Value* formatStr = builder->CreateGlobalString("%d\n");
    
    // Call printf
    builder->CreateCall(printfFunc, {formatStr, value});
  }

  unique_ptr<llvm::LLVMContext> context;
  unique_ptr<llvm::Module> module;
  unique_ptr<llvm::IRBuilder<>> builder;
  map<string, llvm::AllocaInst*> variables;  // Symbol table
};
