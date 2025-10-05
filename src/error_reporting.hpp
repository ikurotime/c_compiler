#pragma once

#include <string>
#include <string_view>
#include <format>
#include <vector>
#include <sstream>

using namespace std;

// ANSI color codes
namespace Color {
  constexpr string_view RED = "\033[31m";
  constexpr string_view BOLD = "\033[1m";
  constexpr string_view RESET = "\033[0m";
}

class ErrorReporter {
public:
  explicit ErrorReporter(string source, string filename = "") 
    : m_source(move(source)), m_filename(move(filename)) {
    // Split source into lines for easy access
    stringstream ss(m_source);
    string line;
    while (getline(ss, line)) {
      m_lines.push_back(line);
    }
  }

  // Format error with source line and caret pointing to the error
  string format_error(string_view message, size_t line, size_t column) const {
    string result;
    
    // Error header with file location (if available)
    string location_info = m_filename.empty() ? "" : format(" in {}", m_filename);
    result += format("{}{}Error:{} {}{}\n", 
      Color::BOLD, Color::RED, Color::RESET, message, location_info);
    
    // Show context: previous line (if available) and the error line
    if (line > 0 && line <= m_lines.size()) {
      // Show previous line for context (if it exists)
      if (line > 1) {
        const auto& prev_line = m_lines[line - 2];
        result += format("  {} | {}\n", line - 1, prev_line);
      }
      
      // Show the line where the error occurred
      const auto& source_line = m_lines[line - 1];
      result += format("  {} | {}\n", line, source_line);
      
      // Add caret pointing to the column - ensure column is within bounds
      size_t actual_column = min(column, source_line.length() + 1);
      size_t prefix_len = format("  {} | ", line).length();
      string caret_line(prefix_len + actual_column - 1, ' ');
      caret_line += format("{}^{}", Color::RED, Color::RESET);
      result += caret_line;
    }
    
    return result;
  }

  // Format error without location info
  string format_error(string_view message) const {
    return format("{}{}Error:{} {}", 
      Color::BOLD, Color::RED, Color::RESET, message);
  }

private:
  string m_source;
  string m_filename;
  vector<string> m_lines;
};

