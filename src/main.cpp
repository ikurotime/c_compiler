#include <iostream>
#include <fstream>
#include <sstream>
#include "tokenization.hpp"
#include "parser.hpp"
#include "generation.hpp"

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    std::cerr << "Incorrect Usage Correct usage is..." << std::endl;
    std::cerr << "neon <input.n>Usage" << std::endl;
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

  Tokenizer tokenizer(std::move(contents));

  std::vector<Token>
      tokens = tokenizer.tokenize();
  std::cout << "Tokenized" << std::endl;
  Parser parser(std::move(tokens));

  std::optional<NodeExit> tree = parser.parse();

  std::cout << "Parsed" << std::endl;
  if (!tree.has_value())
  {
    std::cerr << "No exit statement found" << std::endl;
    exit(EXIT_FAILURE);
  }

  Generator generator(tree.value());
  {
    std::fstream file("out.asm", std::ios::out);
    file << generator.generate();
  }

  std::cout << "Generated" << std::endl;

  // system("clang -o out out.asm -e _start");

  return EXIT_SUCCESS;
}