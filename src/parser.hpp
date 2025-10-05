#pragma once
#include "tokenization.hpp"
#include "error_reporting.hpp"
#include <print>
#include <variant>
#include <vector>
#include <memory>
#include <string>

using namespace std;

// forward declaration for recursive structure
struct NodeExpr;

// primary expression - literal or identifier
struct NodeExprPrimary {
  optional<Token> int_lit;    // literal like 42
  optional<Token> ident;      // variable like x
};

// binary operation - supports chaining
struct NodeExprBin {
  NodeExprPrimary lhs;
  Token op;                        // operator (+, -, *, /)
  shared_ptr<NodeExpr> rhs;   // right side (can be another expression)
};

// expression can be primary or binary operation
struct NodeExpr {
  variant<NodeExprPrimary, NodeExprBin> var;
};

// let statement: let x = expr;
struct NodeStmtLet {
  Token ident;
  NodeExpr expr;
};

// print statement: print(expr);
struct NodeStmtPrint {
  NodeExpr expr;
};

// expression statement: just an expression followed by ;
struct NodeStmtExpr {
  NodeExpr expr;
};

// return statement: return expr;
struct NodeStmtReturn {
  NodeExpr expr;
};

// statement can be one of: let, print, expression, or return
struct NodeStmt {
  variant<NodeStmtLet, NodeStmtPrint, NodeStmtExpr, NodeStmtReturn> var;
};

// function definition: fn name() { statements }
struct NodeFunction {
  Token name;
  vector<NodeStmt> body;
};

// program is a list of functions
struct NodeProgram {
  vector<NodeFunction> functions;
};

class Parser
{
public:
  inline explicit Parser(vector<Token> tokens, string source, string filename = "") 
    : m_tokens(move(tokens)), m_error_reporter(move(source), move(filename))
  {
  }

  auto parse_print() -> Expected<NodeStmtPrint>
  {
    // expect: print ( expression ) ;
    
    if (!peek().has_value() || peek().value().type != TokenType::open_paren) {
      if (peek().has_value()) {
        Token t = peek().value();
        return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected '(' after print", t.line, t.column));
      }
      return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected '(' after print"));
    }
    consume();
    
    Expected<NodeExpr> expr = parse_expr();
    if (!expr) {
      return expr.takeError();
    }
    
    if (!peek().has_value() || peek().value().type != TokenType::close_paren) {
      if (peek().has_value()) {
        Token t = peek().value();
        return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected ')'", t.line, t.column));
      }
      return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected ')'"));
    }
    consume();
    
    return NodeStmtPrint{.expr = *expr};
  }

  auto parse_let() -> Expected<NodeStmtLet>
  {
    // expect: let <ident> = <expr> ;
    
    if (!peek().has_value() || peek().value().type != TokenType::ident) {
      if (peek().has_value()) {
        Token t = peek().value();
        return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected identifier after 'let'", t.line, t.column));
      }
      return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected identifier after 'let'"));
    }
    Token ident = consume();
    
    if (!peek().has_value() || peek().value().type != TokenType::eq) {
      if (peek().has_value()) {
        Token t = peek().value();
        return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected '=' after identifier", t.line, t.column));
      }
      return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected '=' after identifier"));
    }
    consume(); // eat '='
    
    Expected<NodeExpr> expr = parse_expr();
    if (!expr) {
      return expr.takeError();
    }
    
    return NodeStmtLet{.ident = ident, .expr = *expr};
  }

  // parse a primary expression (literal or identifier)
  optional<NodeExprPrimary> parse_primary()
  {
    if (peek().has_value() && peek().value().type == TokenType::int_lit)
    {
      return NodeExprPrimary{.int_lit = consume()};
    }
    else if (peek().has_value() && peek().value().type == TokenType::ident)
    {
      return NodeExprPrimary{.ident = consume()};
    }
    return {};
  }

  // parse expression with support for chaining (left-associative)
  auto parse_expr() -> Expected<NodeExpr>
  {
    // parse first operand
    optional<NodeExprPrimary> primary = parse_primary();
    if (!primary.has_value()) {
      if (peek().has_value()) {
        Token t = peek().value();
        return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected expression", t.line, t.column));
      }
      return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected expression"));
    }
    
    // check for operator
    if (peek().has_value() && 
        (peek().value().type == TokenType::plus || 
         peek().value().type == TokenType::minus ||
         peek().value().type == TokenType::star ||
         peek().value().type == TokenType::fslash)) {
      Token op = consume();
      
      // recursively parse right side (supports chaining: x + y - z)
      Expected<NodeExpr> rhs = parse_expr();
      if (!rhs) {
        return rhs.takeError();
      }
      
      return NodeExpr{
        .var = NodeExprBin{
          .lhs = primary.value(),
          .op = op,
          .rhs = make_shared<NodeExpr>(*rhs)
        }
      };
    }
    
    // no operator, just return primary
    return NodeExpr{.var = primary.value()};
  }

  auto parse_statement() -> Expected<NodeStmt>
  {
    if (!peek().has_value()) {
      return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Unexpected end of input"));
    }
    
    Token token = peek().value();
    TokenType token_type = token.type;
    
    // let statement
    if (token_type == TokenType::let) {
      consume();
      Expected<NodeStmtLet> let_stmt = parse_let();
      if (!let_stmt) {
        return let_stmt.takeError();
      }
      
      if (!peek().has_value() || peek().value().type != TokenType::semi) {
        if (peek().has_value()) {
          Token t = peek().value();
          return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected semicolon after let", t.line, t.column));
        }
        return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected semicolon after let"));
      }
      consume();
      return NodeStmt{.var = *let_stmt};
    }
    // print statement
    else if (token_type == TokenType::print) {
      consume();
      Expected<NodeStmtPrint> print_stmt = parse_print();
      if (!print_stmt) {
        return print_stmt.takeError();
      }
      
      if (!peek().has_value() || peek().value().type != TokenType::semi) {
        if (peek().has_value()) {
          Token t = peek().value();
          return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected semicolon after print", t.line, t.column));
        }
        return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected semicolon after print"));
      }
      consume();
      return NodeStmt{.var = *print_stmt};
    }
    // return statement
    else if (token_type == TokenType::ret) {
      consume();
      Expected<NodeExpr> expr = parse_expr();
      if (!expr) {
        return expr.takeError();
      }
      
      if (!peek().has_value() || peek().value().type != TokenType::semi) {
        if (peek().has_value()) {
          Token t = peek().value();
          return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected semicolon after return", t.line, t.column));
        }
        return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected semicolon after return"));
      }
      consume();
      return NodeStmt{.var = NodeStmtReturn{.expr = *expr}};
    }
    // expression statement
    else {
      Expected<NodeExpr> expr = parse_expr();
      if (!expr) {
        return expr.takeError();
      }
      
      if (!peek().has_value() || peek().value().type != TokenType::semi) {
        if (peek().has_value()) {
          Token t = peek().value();
          return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected semicolon", t.line, t.column));
        }
        return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected semicolon"));
      }
      consume();
      return NodeStmt{.var = NodeStmtExpr{.expr = *expr}};
    }
  }

  auto parse_function() -> Expected<NodeFunction>
  {
    // expect: fn name() { statements }
    
    if (!peek().has_value() || peek().value().type != TokenType::ident) {
      if (peek().has_value()) {
        Token t = peek().value();
        return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected function name after 'fn'", t.line, t.column));
      }
      return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected function name after 'fn'"));
    }
    Token name = consume();
    
    if (!peek().has_value() || peek().value().type != TokenType::open_paren) {
      if (peek().has_value()) {
        Token t = peek().value();
        return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected '(' after function name", t.line, t.column));
      }
      return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected '(' after function name"));
    }
    consume();
    
    if (!peek().has_value() || peek().value().type != TokenType::close_paren) {
      if (peek().has_value()) {
        Token t = peek().value();
        return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected ')' (parameters not yet supported)", t.line, t.column));
      }
      return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected ')' (parameters not yet supported)"));
    }
    consume();
    
    if (!peek().has_value() || peek().value().type != TokenType::open_curly) {
      if (peek().has_value()) {
        Token t = peek().value();
        return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected '{' to start function body", t.line, t.column));
      }
      return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected '{' to start function body"));
    }
    consume();
    
    vector<NodeStmt> body;
    while (peek().has_value() && peek().value().type != TokenType::close_curly) {
      Expected<NodeStmt> stmt = parse_statement();
      if (!stmt) {
        return stmt.takeError();
      }
      body.push_back(*stmt);
    }
    
    if (!peek().has_value() || peek().value().type != TokenType::close_curly) {
      if (peek().has_value()) {
        Token t = peek().value();
        return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected '}' to end function body", t.line, t.column));
      }
      return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected '}' to end function body"));
    }
    consume();
    
    return NodeFunction{.name = name, .body = body};
  }

  auto parse() -> Expected<NodeProgram>
  {
    NodeProgram program;
    
    while (peek().has_value()) {
      if (peek().value().type == TokenType::fn) {
        consume();
        Expected<NodeFunction> func = parse_function();
        if (!func) {
          return func.takeError();
        }
        program.functions.push_back(*func);
      } else {
        Token t = peek().value();
        return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Expected function definition (top-level statements not allowed)", t.line, t.column));
      }
    }
    
    // Check for main function
    bool has_main = false;
    for (const auto& func : program.functions) {
      if (func.name.value == "main") {
        has_main = true;
        break;
      }
    }
    
    if (!has_main) {
      return llvm::createStringError(llvm::inconvertibleErrorCode(), m_error_reporter.format_error("Program must have a 'main' function"));
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
