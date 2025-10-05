#pragma once
#include "lexer.hpp"
#include "error_reporting.hpp"
#include <print>
#include <variant>
#include <vector>
#include <memory>
#include <string>
#include <algorithm>

using namespace std;

// forward declaration for recursive structure
struct ASTExpr;

// primary expression - literal or identifier
struct ASTPrimaryExpr {
  optional<Token> int_lit;    // literal like 42
  optional<Token> ident;      // variable like x
};

// binary operation - supports chaining
struct ASTBinaryExpr {
  ASTPrimaryExpr lhs;
  Token op;                        // operator (+, -, *, /)
  shared_ptr<ASTExpr> rhs;   // right side (can be another expression)
};

// expression can be primary or binary operation
struct ASTExpr {
  variant<ASTPrimaryExpr, ASTBinaryExpr> var;
};

// let statement: let x = expr;
struct ASTLetStmt {
  Token ident;
  ASTExpr expr;
};

// print statement: print(expr);
struct ASTPrintStmt {
  ASTExpr expr;
};

// expression statement: just an expression followed by ;
struct ASTExprStmt {
  ASTExpr expr;
};

// return statement: return expr;
struct ASTReturnStmt {
  ASTExpr expr;
};

// statement can be one of: let, print, expression, or return
struct ASTStmt {
  variant<ASTLetStmt, ASTPrintStmt, ASTExprStmt, ASTReturnStmt> var;
};

// function definition: fn name() { statements }
struct ASTFunction {
  Token name;
  vector<ASTStmt> body;
};

// program is a list of functions
struct ASTProgram {
  vector<ASTFunction> functions;
};

class Parser
{
public:
  inline explicit Parser(vector<Token> tokens, string source, string filename = "") 
    : m_tokens(move(tokens)), m_error_reporter(move(source), move(filename))
  {
  }

private:
  // helper to expect a specific token type
  auto expect_token(TokenType expected_type, string_view context) -> Expected<Token>
  {
    if (!peek().has_value()) {
      return llvm::createStringError(
        llvm::inconvertibleErrorCode(), 
        m_error_reporter.format_error(format("Expected {} at end of input", context))
      );
    }
    
    Token token = peek().value();
    if (token.type != expected_type) {
      return llvm::createStringError(
        llvm::inconvertibleErrorCode(),
        m_error_reporter.format_error(format("Expected {}, got {}", context, token_type_to_string(token.type)), token.line, token.column)
      );
    }
    
    return consume();
  }
  
  // helper to expect a token and consume it
  auto expect_and_consume(TokenType expected_type, string_view context) -> llvm::Error
  {
    auto result = expect_token(expected_type, context);
    if (!result) {
      return result.takeError();
    }
    return llvm::Error::success();
  }
  
  // helper to convert token type to string for error messages
  string token_type_to_string(TokenType type) const
  {
    switch (type) {
      case TokenType::open_paren: return "'('";
      case TokenType::close_paren: return "')'";
      case TokenType::open_curly: return "'{'";
      case TokenType::close_curly: return "'}'";
      case TokenType::ident: return "identifier";
      case TokenType::eq: return "'='";
      case TokenType::semi: return "';'";
      case TokenType::int_lit: return "integer literal";
      case TokenType::let: return "'let'";
      case TokenType::print: return "'print'";
      case TokenType::fn: return "'fn'";
      case TokenType::ret: return "'return'";
      case TokenType::plus: return "'+'";
      case TokenType::minus: return "'-'";
      case TokenType::star: return "'*'";
      case TokenType::fslash: return "'/'";
      default: return "unknown token";
    }
  }

public:
  auto parse_print() -> Expected<ASTPrintStmt>
  {
    // expect: print ( expression ) ;
    
    if (auto err = expect_and_consume(TokenType::open_paren, "(")) {
      return std::move(err);
    }
    
    auto expr = parse_expr();
    if (!expr) {
      return expr.takeError();
    }
    
    if (auto err = expect_and_consume(TokenType::close_paren, ")")) {
      return std::move(err);
    }
    
    return ASTPrintStmt{.expr = *expr};
  }

  auto parse_let() -> Expected<ASTLetStmt>
  {
    // expect: let <ident> = <expr> ;
    
    auto ident_result = expect_token(TokenType::ident, "identifier after 'let'");
    if (!ident_result) {
      return ident_result.takeError();
    }
    Token ident = *ident_result;
    
    if (auto err = expect_and_consume(TokenType::eq, "'=' after identifier")) {
      return std::move(err);
    }
    
    auto expr = parse_expr();
    if (!expr) {
      return expr.takeError();
    }
    
    return ASTLetStmt{.ident = ident, .expr = *expr};
  }

  // parse a primary expression (literal or identifier)
  optional<ASTPrimaryExpr> parse_primary()
  {
    if (peek().has_value() && peek().value().type == TokenType::int_lit)
    {
      return ASTPrimaryExpr{.int_lit = consume()};
    }
    else if (peek().has_value() && peek().value().type == TokenType::ident)
    {
      return ASTPrimaryExpr{.ident = consume()};
    }
    return {};
  }

  // helper to check if token is a binary operator
  bool is_binary_operator(TokenType type) const
  {
    switch (type) {
      case TokenType::plus:
      case TokenType::minus:
      case TokenType::star:
      case TokenType::fslash:
        return true;
      default:
        return false;
    }
  }

  // parse expression with support for chaining (left-associative)
  auto parse_expr() -> Expected<ASTExpr>
  {
    // Parse first operand
    optional<ASTPrimaryExpr> primary = parse_primary();
    if (!primary.has_value()) {
      if (peek().has_value()) {
        Token t = peek().value();
        return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected expression", t.line, t.column));
      }
      return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected expression"));
    }
    
    // check for operator
    if (peek().has_value() && is_binary_operator(peek().value().type)) {
      Token op = consume();
      
      // Recursively parse right side (supports chaining: x + y - z)
      Expected<ASTExpr> rhs = parse_expr();
      if (!rhs) {
        return rhs.takeError();
      }
      
      return ASTExpr{
        .var = ASTBinaryExpr{
          .lhs = primary.value(),
          .op = op,
          .rhs = make_shared<ASTExpr>(*rhs)
        }
      };
    }
    
    // no operator, just return primary
    return ASTExpr{.var = primary.value()};
  }

  // helper to expect semicolon after statement
  auto expect_semicolon(string_view context) -> llvm::Error
  {
    return expect_and_consume(TokenType::semi, format("semicolon after {}", context));
  }

  auto parse_statement() -> Expected<ASTStmt>
  {
    if (!peek().has_value()) {
      return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Unexpected end of input"));
    }
    
    Token token = peek().value();
    TokenType token_type = token.type;
    
    switch (token_type) {
      case TokenType::let: {
        consume();
        Expected<ASTLetStmt> let_stmt = parse_let();
        if (!let_stmt) {
          return let_stmt.takeError();
        }
        
        if (auto err = expect_semicolon("let")) {
          return std::move(err);
        }
        return ASTStmt{.var = *let_stmt};
      }
      
      case TokenType::print: {
        consume();
        Expected<ASTPrintStmt> print_stmt = parse_print();
        if (!print_stmt) {
          return print_stmt.takeError();
        }
        
        if (auto err = expect_semicolon("print")) {
          return std::move(err);
        }
        return ASTStmt{.var = *print_stmt};
      }
      
      case TokenType::ret: {
        consume();
        Expected<ASTExpr> expr = parse_expr();
        if (!expr) {
          return expr.takeError();
        }
        
        if (auto err = expect_semicolon("return")) {
          return std::move(err);
        }
        return ASTStmt{.var = ASTReturnStmt{.expr = *expr}};
      }
      
      default: {
        // expression statement
        Expected<ASTExpr> expr = parse_expr();
        if (!expr) {
          return expr.takeError();
        }
        
        if (auto err = expect_semicolon("expression")) {
          return std::move(err);
        }
        return ASTStmt{.var = ASTExprStmt{.expr = *expr}};
      }
    }
  }

  auto parse_function() -> Expected<ASTFunction>
  {
    // expect: fn name() { statements }
    
    auto name_result = expect_token(TokenType::ident, "function name after 'fn'");
    if (!name_result) {
      return name_result.takeError();
    }
    Token name = *name_result;
    
    if (auto err = expect_and_consume(TokenType::open_paren, "(")) {
      return std::move(err);
    }
    
    if (auto err = expect_and_consume(TokenType::close_paren, ") (parameters not yet supported)")) {
      return std::move(err);
    }
    
    if (auto err = expect_and_consume(TokenType::open_curly, "{ to start function body")) {
      return std::move(err);
    }
    
    vector<ASTStmt> body;
    while (peek().has_value() && peek().value().type != TokenType::close_curly) {
      auto stmt = parse_statement();
      if (!stmt) {
        return stmt.takeError();
      }
      body.push_back(*stmt);
    }
    
    if (auto err = expect_and_consume(TokenType::close_curly, "} to end function body")) {
      return std::move(err);
    }
    
    return ASTFunction{.name = name, .body = body};
  }

  // helper to check if program has a main function
  bool has_main_function(const ASTProgram& program) const
  {
    return any_of(program.functions.begin(), program.functions.end(),
                  [](const ASTFunction& func) { 
                    return func.name.value == "main"; 
                  });
  }

  auto parse() -> Expected<ASTProgram>
  {
    ASTProgram program;
    
    while (peek().has_value()) {
      if (peek().value().type == TokenType::fn) {
        consume();
        auto func = parse_function();
        if (!func) {
          return func.takeError();
        }
        program.functions.push_back(*func);
      } else {
        Token t = peek().value();
        return llvm::createStringError(
          llvm::inconvertibleErrorCode(), 
          m_error_reporter.format_error("Expected function definition (top-level statements not allowed)", t.line, t.column)
        );
      }
    }
    
    if (!has_main_function(program)) {
      return llvm::createStringError(
        llvm::inconvertibleErrorCode(), 
        m_error_reporter.format_error("Program must have a 'main' function")
      );
    }
    
    m_index = 0;
    return program;
  }

private:
  [[nodiscard]] inline optional<Token>
  peek(int ahead = 1) const
  {
    if (m_index + ahead > m_tokens.size())
    {
      return {};
    }
    else
    {
      return m_tokens.at(m_index);
    }
  }

  inline Token consume()
  {
    return m_tokens.at(m_index++);
  }

  const vector<Token> m_tokens;
  size_t m_index = 0;
  ErrorReporter m_error_reporter;
};
