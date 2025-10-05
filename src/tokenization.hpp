#pragma once

#include <format>
#include <string>
#include <vector>
#include <cctype>
#include <llvm/Support/Error.h>
#include "error_reporting.hpp"

template<typename T>
using Expected = llvm::Expected<T>;


enum class TokenType
{
  int_lit,
  semi,
  open_paren,
  close_paren,
  open_curly,
  close_curly,
  ident,
  let,
  eq,
  plus,
  star,
  minus,
  fslash,
  print,
  fn,
  ret
};

using namespace std;

struct Token
{
  TokenType type;
  optional<string> value{};
  size_t line = 1;
  size_t column = 1;
};

class Tokenizer
{
public:
  inline explicit Tokenizer(string src, string filename = "") 
    : m_src(src), m_error_reporter(move(src), move(filename))
  {
  }
  
private:
  size_t m_line = 1;
  size_t m_column = 1;
  
  Token make_token(TokenType type, optional<string> value = {}) {
    return Token{.type = type, .value = value, .line = m_line, .column = m_column};
  }

public:
  auto tokenize() -> Expected<vector<Token>>
  {
    vector<Token> tokens;
    string buf;
    
    while (peek().has_value()) {
      const char current = peek().value();
      
      // Identifier or keyword
      if (isalpha(current)) {
        buf.push_back(consume());
        while (peek().has_value() && isalnum(peek().value())) {
          buf.push_back(consume());
        }
        
        if (buf == "let") {
          tokens.push_back(make_token(TokenType::let));
        } else if (buf == "print") {
          tokens.push_back(make_token(TokenType::print));
        } else if (buf == "fn") {
          tokens.push_back(make_token(TokenType::fn));
        } else if (buf == "return") {
          tokens.push_back(make_token(TokenType::ret));
        } else {
          tokens.push_back(make_token(TokenType::ident, buf));
        }
        buf.clear();
      }
      // Integer literal
      else if (isdigit(current)) {
        buf.push_back(consume());
        while (peek().has_value() && isdigit(peek().value())) {
          buf.push_back(consume());
        }
        tokens.push_back(make_token(TokenType::int_lit, buf));
        buf.clear();
      }
      // Single character tokens
      else if (current == '(') {
        consume();
        tokens.push_back(make_token(TokenType::open_paren));
      }
      else if (current == ')') {
        consume();
        tokens.push_back(make_token(TokenType::close_paren));
      }
      else if (current == '{') {
        consume();
        tokens.push_back(make_token(TokenType::open_curly));
      }
      else if (current == '}') {
        consume();
        tokens.push_back(make_token(TokenType::close_curly));
      }
      else if (current == '+') {
        consume();
        tokens.push_back(make_token(TokenType::plus));
      }
      else if (current == '-') {
        consume();
        tokens.push_back(make_token(TokenType::minus));
      }
      else if (current == '*') {
        consume();
        tokens.push_back(make_token(TokenType::star));
      }
      else if (current == '=') {
        consume();
        tokens.push_back(make_token(TokenType::eq));
      }
      else if (current == ';') {
        consume();
        tokens.push_back(make_token(TokenType::semi));
      }
      // Skip whitespace
      else if (isspace(current)) {
        if (current == '\n') {
          m_line++;
          m_column = 1;
        } else {
          m_column++;
        }
        consume();
      }
      // Unrecognized character
      else {
        return llvm::createStringError(
          llvm::inconvertibleErrorCode(),
          m_error_reporter.format_error(format("Unexpected character '{}'", current), m_line, m_column)
        );
      }
    }
    
    m_index = 0;
    return tokens;
  }

private:
  [[nodiscard]] inline optional<char>
  peek(int ahead = 1) const
  {
    if (m_index + ahead > m_src.length())
    {
      return {};
    }
    else
    {
      return m_src.at(m_index);
    }
  }

  inline char consume()
  {
    m_column++;
    return m_src.at(m_index++);
  }

  const string m_src;
  size_t m_index = 0;
  ErrorReporter m_error_reporter;
};