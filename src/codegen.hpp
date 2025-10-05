#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include "parser.hpp"

class CodeGen
{
public:
  CodeGen()
    : context(std::make_unique<llvm::LLVMContext>()),
      module(std::make_unique<llvm::Module>("Husk", *context)),
      builder(std::make_unique<llvm::IRBuilder<>>(*context))
  {
  }

  void generate(const NodeExit& program)
  {
    // Create main function: i32 @main()
    llvm::FunctionType* mainType = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(*context),  // return type: i32
      false                               // not vararg
    );
    
    llvm::Function* mainFunc = llvm::Function::Create(
      mainType,
      llvm::Function::ExternalLinkage,
      "main",
      module.get()
    );
    
    // Create entry basic block
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context, "entry", mainFunc);
    builder->SetInsertPoint(entry);
    
    // Generate code for the expression
    llvm::Value* result = generateExpr(program.expr);
    
    // For now, print and return the result
    // We'll call printf to print the number
    createPrintFunction(result);
    
    // Return 0
    builder->CreateRet(llvm::ConstantInt::get(*context, llvm::APInt(32, 0)));
  }

  void dump() const
  {
    module->print(llvm::outs(), nullptr);
  }

  std::unique_ptr<llvm::Module> getModule()
  {
    return std::move(module);
  }

private:
  llvm::Value* generateExpr(const NodeExpr& expr)
  {
    // Get first number
    int val1 = std::stoi(expr.int_lit.value.value_or("0"));
    llvm::Value* v1 = llvm::ConstantInt::get(*context, llvm::APInt(32, val1));
    
    // Check if there's an addition
    if (expr.add_op.has_value())
    {
      int val2 = std::stoi(expr.int_lit2.value().value.value_or("0"));
      llvm::Value* v2 = llvm::ConstantInt::get(*context, llvm::APInt(32, val2));
      return builder->CreateAdd(v1, v2, "addtmp");
    }
    
    return v1;
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

  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::Module> module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
};

