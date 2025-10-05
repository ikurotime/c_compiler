#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <cctype>
#include <optional>
#include <llvm/Support/Error.h>

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

struct Token
{
  TokenType type;
  std::optional<std::string> value{};
  size_t line = 1;
  size_t column = 1;
};

struct TokenInfo {
    std::string_view name;
    std::string_view description;
    bool has_value;
};

// Base token parser interface
struct TokenParser {
    virtual ~TokenParser() = default;
    virtual Expected<Token> parse(
        std::string_view source, 
        size_t& index, 
        size_t& line, 
        size_t& column
    ) const = 0;
    virtual bool matches(std::string_view source, size_t index) const = 0;
    virtual TokenInfo get_info() const = 0;
};

// Integer literal parser
struct IntLitParser : TokenParser {
    ~IntLitParser() override = default;
    TokenInfo info = {"int_lit", "Integer literal", true};
    
    Expected<Token> parse(
        std::string_view source, 
        size_t& index, 
        size_t& line, 
        size_t& column
    ) const override {
        std::string value;
        size_t start_col = column;
        
        // Consume all digits
        while (index < source.length() && std::isdigit(source[index])) {
            value.push_back(source[index]);
            index++;
            column++;
        }
        
        return Token{
            .type = TokenType::int_lit,
            .value = value,
            .line = line,
            .column = start_col
        };
    }
    
    bool matches(std::string_view source, size_t index) const override {
        return index < source.length() && std::isdigit(source[index]);
    }
    
    TokenInfo get_info() const override { return info; }
};

// Identifier parser
struct IdentifierParser : TokenParser {
    ~IdentifierParser() override = default;
    TokenInfo info = {"ident", "Identifier", true};
    
    Expected<Token> parse(
        std::string_view source, 
        size_t& index, 
        size_t& line, 
        size_t& column
    ) const override {
        std::string value;
        size_t start_col = column;
        
        // First character must be alpha
        if (index < source.length() && std::isalpha(source[index])) {
            value.push_back(source[index]);
            index++;
            column++;
        }
        
        // Subsequent characters can be alphanumeric
        while (index < source.length() && std::isalnum(source[index])) {
            value.push_back(source[index]);
            index++;
            column++;
        }
        
        return Token{
            .type = TokenType::ident,
            .value = value,
            .line = line,
            .column = start_col
        };
    }
    
    bool matches(std::string_view source, size_t index) const override {
        return index < source.length() && std::isalpha(source[index]);
    }
    
    TokenInfo get_info() const override { return info; }
};

// Keyword parser
struct KeywordParser : TokenParser {
    ~KeywordParser() override = default;
    std::string keyword;
    TokenType type;
    TokenInfo info;
    
    KeywordParser(std::string kw, TokenType t) 
        : keyword(std::move(kw)), type(t) {
        info = {keyword, keyword + " keyword", false};
    }
    
    Expected<Token> parse(
        std::string_view source, 
        size_t& index, 
        size_t& line, 
        size_t& column
    ) const override {
        size_t start_col = column;
        
        // Consume the keyword
        for (char c : keyword) {
            if (index >= source.length() || source[index] != c) {
                return llvm::createStringError(llvm::inconvertibleErrorCode(), "Keyword parsing failed");
            }
            index++;
            column++;
        }
        
        // Ensure we're not in the middle of an identifier
        if (index < source.length() && std::isalnum(source[index])) {
            return llvm::createStringError(llvm::inconvertibleErrorCode(), "Invalid keyword parsing");
        }
        
        return Token{
            .type = type,
            .value = std::nullopt,
            .line = line,
            .column = start_col
        };
    }
    
    bool matches(std::string_view source, size_t index) const override {
        if (index + keyword.length() > source.length()) {
            return false;
        }
        
        // Check if the keyword matches
        for (size_t i = 0; i < keyword.length(); ++i) {
            if (source[index + i] != keyword[i]) {
                return false;
            }
        }
        
        // Ensure we're not in the middle of an identifier
        if (index + keyword.length() < source.length() && 
            std::isalnum(source[index + keyword.length()])) {
            return false;
        }
        
        return true;
    }
    
    TokenInfo get_info() const override { return info; }
};

// Single character parser
struct SingleCharParser : TokenParser {
    ~SingleCharParser() override = default;
    char character;
    TokenType type;
    TokenInfo info;
    
    SingleCharParser(char c, TokenType t) : character(c), type(t) {
        std::string char_str = std::string(1, c);
        info = {char_str, char_str + " operator", false};
    }
    
    Expected<Token> parse(
        std::string_view source, 
        size_t& index, 
        size_t& line, 
        size_t& column
    ) const override {
        size_t start_col = column;
        
        if (index >= source.length() || source[index] != character) {
            return llvm::createStringError(llvm::inconvertibleErrorCode(), "Single character parsing failed");
        }
        
        index++;
        column++;
        
        return Token{
            .type = type,
            .value = std::nullopt,
            .line = line,
            .column = start_col
        };
    }
    
    bool matches(std::string_view source, size_t index) const override {
        return index < source.length() && source[index] == character;
    }
    
    TokenInfo get_info() const override { return info; }
};

// Token registry - centralized collection of all token parsers
class TokenRegistry {
public:
    static const std::vector<std::unique_ptr<TokenParser>>& get_parsers() {
        static std::vector<std::unique_ptr<TokenParser>> parsers = create_parsers();
        return parsers;
    }
    
private:
    static std::vector<std::unique_ptr<TokenParser>> create_parsers() {
        std::vector<std::unique_ptr<TokenParser>> parsers;
        
        // Keywords (must come before identifiers to match first)
        parsers.push_back(std::make_unique<KeywordParser>("return", TokenType::ret));
        parsers.push_back(std::make_unique<KeywordParser>("print", TokenType::print));
        parsers.push_back(std::make_unique<KeywordParser>("let", TokenType::let));
        parsers.push_back(std::make_unique<KeywordParser>("fn", TokenType::fn));
        
        // Single character operators
        parsers.push_back(std::make_unique<SingleCharParser>('(', TokenType::open_paren));
        parsers.push_back(std::make_unique<SingleCharParser>(')', TokenType::close_paren));
        parsers.push_back(std::make_unique<SingleCharParser>('{', TokenType::open_curly));
        parsers.push_back(std::make_unique<SingleCharParser>('}', TokenType::close_curly));
        parsers.push_back(std::make_unique<SingleCharParser>('+', TokenType::plus));
        parsers.push_back(std::make_unique<SingleCharParser>('-', TokenType::minus));
        parsers.push_back(std::make_unique<SingleCharParser>('*', TokenType::star));
        parsers.push_back(std::make_unique<SingleCharParser>('/', TokenType::fslash));
        parsers.push_back(std::make_unique<SingleCharParser>('=', TokenType::eq));
        parsers.push_back(std::make_unique<SingleCharParser>(';', TokenType::semi));
        
        // Complex parsers (must come last)
        parsers.push_back(std::make_unique<IntLitParser>());
        parsers.push_back(std::make_unique<IdentifierParser>());
        
        return parsers;
    }
};
