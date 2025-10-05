#include <iostream>
#include <fstream>
#include <sstream>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include "tokenization.hpp"
#include "parser.hpp"
#include "codegen.hpp"

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    std::cerr << "Incorrect Usage. Correct usage is..." << std::endl;
    std::cerr << "husk <input.hsk>" << std::endl;
    return EXIT_FAILURE;
  }

  std::string contents;
  {
    std::stringstream contents_streams;
    std::fstream input(argv[1], std::ios::in);
    contents_streams << input.rdbuf();
    contents = contents_streams.str();
  }

  std::cout << contents << std::endl;

  // Tokenize
  Tokenizer tokenizer(std::move(contents));
  std::vector<Token> tokens = tokenizer.tokenize();
  std::cout << "Tokenized" << std::endl;
  
  // Parse
  Parser parser(std::move(tokens));
  std::optional<NodeExit> tree = parser.parse();
  std::cout << "Parsed" << std::endl;
  
  if (!tree.has_value())
  {
    std::cerr << "No valid program found" << std::endl;
    exit(EXIT_FAILURE);
  }

  // Initialize LLVM
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  // Generate LLVM IR
  CodeGen codegen;
  codegen.generate(tree.value());
  std::cout << "Generated LLVM IR" << std::endl;
  
  // Output LLVM IR to file
  std::error_code EC;
  llvm::raw_fd_ostream dest("out.ll", EC);
  if (EC)
  {
    std::cerr << "Could not open file: " << EC.message() << std::endl;
    return EXIT_FAILURE;
  }
  codegen.getModule()->print(dest, nullptr);
  dest.flush();
  
  std::cout << "\nLLVM IR saved to out.ll" << std::endl;
  std::cout << "Compile with: clang out.ll -o out" << std::endl;

  return EXIT_SUCCESS;
}