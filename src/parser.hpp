#pragma once
#include "tokenization.hpp"

struct NodeExpr
{
  Token int_lit;
  std::optional<Token> add_op;
  std::optional<Token> int_lit2;  // Changed from NodeExpr to Token
};

struct NodeExit
{
  NodeExpr expr;
};

struct NodeStmtPrint{
  NodeExpr expr;
};

class Parser
{
public:
  inline explicit Parser(std::vector<Token> tokens) : m_tokens(std::move(tokens))
  {
  }

  std::optional<NodeStmtPrint> parse_print()
  {
    // expect: print ( expression ) ;
    
    if (peek().has_value() && peek().value().type == TokenType::open_paren)
    {
      consume();
      
      if (auto expr = parse_expr())
      {
        if (peek().has_value() && peek().value().type == TokenType::close_paren)
        {
          consume();
          return NodeStmtPrint{.expr = expr.value()};
        }
        else
        {
          std::cerr << "Expected ')'" << std::endl;
          exit(EXIT_FAILURE);
        }
      }
      else
      {
        std::cerr << "Expected expression in print()" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    else
    {
      std::cerr << "Expected '(' after print" << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  std::optional<NodeExpr> parse_expr()
  {
    if (peek().has_value() && peek().value().type == TokenType::int_lit)
    {
      auto int_lit = consume();  
      
      // check if there's a + after the first number
      if (peek().has_value() && peek().value().type == TokenType::plus)
      {
        auto plus_op = consume(); 
        
        if (peek().has_value() && peek().value().type == TokenType::int_lit)
        {
          auto int_lit2 = consume();  
          return NodeExpr{.int_lit = int_lit, .add_op = plus_op, .int_lit2 = int_lit2};
        }
        else
        {
          std::cerr << "Expected number after +" << std::endl;
          exit(EXIT_FAILURE);
        }
      }
      else
      {
        // simple number, no addition
        return NodeExpr{.int_lit = int_lit};
      }
    }
    else
    {
      return {};
    };
  }

  std::optional<NodeExit> parse()
  {
    std::optional<NodeExit> exit_node;
    while (peek().has_value())
    {
      // check if it's a print statement
      if (peek().value().type == TokenType::print)
      {
        consume(); 
        if (auto node_print = parse_print())
        {
          exit_node = NodeExit{.expr = node_print.value().expr};
          
          if (peek().has_value() && peek().value().type == TokenType::semi)
          {
            consume();
          }
          else
          {
            std::cerr << "Expected semicolon after print" << std::endl;
            exit(EXIT_FAILURE);
          }
        }
        else
        {
          std::cerr << "Invalid Print Statement" << std::endl;
          exit(EXIT_FAILURE);
        }
      }
      // parse as expression
      else if (auto node_expr = parse_expr())
      {
        exit_node = NodeExit{.expr = node_expr.value()};
        
        if (peek().has_value() && peek().value().type == TokenType::semi)
        {
          consume();
        }
        else
        {
          std::cerr << "Expected semicolon" << std::endl;
          exit(EXIT_FAILURE);
        }
      }
      else
      {
        std::cerr << "Invalid statement" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    m_index = 0;
    return exit_node;
  }

private:
  [[nodiscard]] inline std::optional<Token>
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

  const std::vector<Token> m_tokens;
  size_t m_index = 0;
};