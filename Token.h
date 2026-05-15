#ifndef CALC_TOKEN_H
#define CALC_TOKEN_H

#include <ostream>
#include <string>
#include <utility>

namespace calc {

enum TokenType {
    Operator,       // + - * / ^
    Integer,        // ex. 123 53453
    Function,       // sin cos tan ln sqrt abs
    Variable,       // ex. x1 a11 A B66
    Parenthesis,    // ( )
    Separator,      // ; .
    Assign,         // = :=
    Unkown,         // Any not recognized text. Name kept for compatibility with the original code.

    Count           // Special value representing number of token types.
};

inline std::ostream& operator<<(std::ostream& os, TokenType type) {
    switch (type) {
        case Operator:     return os << "Operator";
        case Integer:      return os << "Integer";
        case Function:     return os << "Function";
        case Variable:     return os << "Variable";
        case Parenthesis:  return os << "Parenthesis";
        case Separator:    return os << "Separator";
        case Assign:       return os << "Assign";
        case Unkown:       return os << "Unkown";
        case Count:        return os << "Count";
        default:           return os << "Unknown";
    }
}

struct Token {
    TokenType    type;
    std::string  lexeme;
    std::size_t  position;

    Token(TokenType _type, std::string _lexeme, std::size_t _position)
        : type(_type), lexeme(std::move(_lexeme)), position(_position) {}
};

inline std::ostream& operator<<(std::ostream& os, const Token& token) {
    return os << "Token{ type: " << token.type
              << ", lexeme: \"" << token.lexeme << "\""
              << ", position: " << token.position
              << " }";
}

} // namespace calc

#endif // CALC_TOKEN_H
