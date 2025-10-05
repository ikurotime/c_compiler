#pragma once

#include <format>
#include <string>
#include <vector>
#include <cctype>
#include <llvm/Support/Error.h>
#include "error_reporting.hpp"
#include "tokens.hpp"

template<typename T>
using Expected = llvm::Expected<T>;

using namespace std;

class Lexer
{
public:
  inline explicit Lexer(string src, string filename = "") 
    : m_src(src), m_error_reporter(move(src), move(filename))
  {
  }
  
  auto tokenize() -> Expected<vector<Token>>
  {
    vector<Token> tokens;
    size_t index = 0;
    size_t line = 1;
    size_t column = 1;
    
    while (index < m_src.length()) {
      // Skip whitespace
      if (skip_whitespace(index, line, column)) {
        continue;
      }
      
      // Try to lex a token using registered parsers
      auto token_result = parse_next_token(index, line, column);
      if (!token_result) {
        return token_result.takeError();
      }
      
      tokens.push_back(*token_result);
    }
    
    return tokens;
  }

private:
  // Skip whitespace and update line/column tracking
  bool skip_whitespace(size_t& index, size_t& line, size_t& column)
  {
    if (index >= m_src.length()) {
      return false;
    }
    
    char current = m_src[index];
    if (!isspace(current)) {
      return false;
    }
    
    if (current == '\n') {
      line++;
      column = 1;
    } else {
      column++;
    }
    
    index++;
    return true;
  }
  
  // Lex the next token using registered parsers
  auto parse_next_token(size_t& index, size_t line, size_t column) -> Expected<Token>
  {
    if (index >= m_src.length()) {
      return llvm::createStringError(
        llvm::inconvertibleErrorCode(),
        m_error_reporter.format_error("Unexpected end of input", line, column)
      );
    }
    
    // Try each registered parser in order
    const auto& parsers = TokenRegistry::get_parsers();
    for (const auto& parser : parsers) {
      if (parser->matches(m_src, index)) {
        auto result = parser->parse(m_src, index, line, column);
        if (result) {
          return result;
        }
        // If parsing failed, try next parser
      }
    }
    
    // No parser matched - unexpected character
    char current = m_src[index];
    return llvm::createStringError(
      llvm::inconvertibleErrorCode(),
      m_error_reporter.format_error(format("Unexpected character '{}'", current), line, column)
    );
  }

  const string m_src;
  ErrorReporter m_error_reporter;
};