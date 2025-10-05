#include <print>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string_view>
#include <filesystem>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include "tokenization.hpp"
#include "parser.hpp"
#include "codegen.hpp"

namespace fs = filesystem;

// Simple debug logger using C++23 println
class DebugLog {
  ofstream file;
public:
  explicit DebugLog(string_view filename) : file(filename.data()) {}
  
  template<typename... Args>
  void printline(format_string<Args...> fmt, Args&&... args) {
    println(file, fmt, forward<Args>(args)...);
  }
};

// Helper to read entire file into string
auto read_file(const fs::path& path) -> Expected<string> {
  if (auto file = ifstream(path); file) {
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
  }
  return llvm::createStringError(llvm::inconvertibleErrorCode(), format("Could not open file: {}", path.string()));
}

// Initialize LLVM targets
void init_llvm_targets() {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
}

// Write LLVM IR to file
auto write_llvm_ir(llvm::Module* module, string_view output_path) 
    -> llvm::Error {
  error_code ec;
  llvm::raw_fd_ostream dest(output_path.data(), ec);
  
  if (ec) {
    return llvm::createStringError(llvm::inconvertibleErrorCode(), format("Could not write IR: {}", ec.message()));
  }
  
  module->print(dest, nullptr);
  dest.flush();
  return llvm::Error::success();
}

int main(int argc, char* argv[]) try {
  auto log = DebugLog("DEBUG.txt");
  log.printline("Starting compiler...");
  
  // Validate arguments
  if (argc != 2) {
    println(cerr, "Usage: husk <input.hsk>");
    return EXIT_FAILURE;
  }
  log.printline("Args OK");

  // Read source file
  const auto input_path = fs::path(argv[1]);
  log.printline("Opening file: {}", input_path.string());
  
  auto contents_result = read_file(input_path);
  if (!contents_result) {
    println(cerr, "Error: {}", llvm::toString(contents_result.takeError()));
    return EXIT_FAILURE;
  }
  const auto& contents = *contents_result;
  
  log.printline("File size: {} bytes", size(contents));
  log.printline("File contents: [{}]", contents);

  // Tokenize
  auto tokens_result = Tokenizer(contents, input_path.filename().string()).tokenize();
  if (!tokens_result) {
    println(cerr, "{}", llvm::toString(tokens_result.takeError()));
    return EXIT_FAILURE;
  }
  auto tokens = *tokens_result;
  log.printline("Token count: {}", size(tokens));
  
  // Parse
  auto program_result = Parser(move(tokens), contents, input_path.filename().string()).parse();
  if (!program_result) {
    println(cerr, "{}", llvm::toString(program_result.takeError()));
    return EXIT_FAILURE;
  }
  auto program = *program_result;
  log.printline("Function count: {}", size(program.functions));

  // Generate LLVM IR
  init_llvm_targets();
  log.printline("Generating LLVM IR...");
  
  auto codegen = CodeGen();
  if (auto result = codegen.generate(program)) {
    println(cerr, "Code generation error: {}", llvm::toString(std::move(result)));
    return EXIT_FAILURE;
  }
  log.printline("Generated LLVM IR");
  
  // Write output
  if (auto result = write_llvm_ir(codegen.getModule().get(), "out.ll")) {
    println(cerr, "Error: {}", llvm::toString(std::move(result)));
    return EXIT_FAILURE;
  }
  log.printline("Done!");

  return EXIT_SUCCESS;
}
catch (const exception& e) {
  println(cerr, "Error: {}", e.what());
  return EXIT_FAILURE;
}
