#ifndef CALC_LEXER_H
#define CALC_LEXER_H

#include "Token.h"

#include <array>
#include <cctype>
#include <regex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace calc {

class Lexer {
public:
    std::vector<Token> tokenize(const std::string& input) const {
        std::size_t position = 0;
        const std::size_t n = input.length();
        std::vector<Token> tokens;

        while (position < n) {
            auto wordRange = findNextWordRange(std::string_view(input), position);
            if (wordRange.first == std::string_view::npos) {
                break;
            }

            std::string_view word(input.data() + wordRange.first, wordRange.second - wordRange.first);
            const auto tokenType = classifyWord(word);
            tokens.emplace_back(tokenType, std::string(word.begin(), word.end()), wordRange.first);
            position = wordRange.second;
        }

        return tokens;
    }

private:
    struct TokenForm {
        TokenType type;
        std::regex pattern;
    };

    const std::array<TokenForm, TokenType::Count - 1> rules_ {{
        {TokenType::Integer,     std::regex(R"(\d+)")},
        {TokenType::Operator,    std::regex(R"([+\-*/^])")},
        {TokenType::Function,    std::regex(R"(sin|cos|tan|ln|sqrt|abs)")},
        {TokenType::Assign,      std::regex(R"(:?=)")},
        {TokenType::Separator,   std::regex(R"([.;])")},
        {TokenType::Parenthesis, std::regex(R"([()])")},
        {TokenType::Variable,    std::regex(R"([a-zA-Z]\d*)")}
    }};

    static bool isOperator(char c) noexcept {
        return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
    }

    static bool isParen(char c) noexcept {
        return c == '(' || c == ')';
    }

    static bool isSeparator(char c) noexcept {
        return c == '.' || c == ';';
    }

    static std::pair<std::size_t, std::size_t>
    findNextWordRange(std::string_view s, std::size_t pos) {
        const std::size_t n = s.length();

        while (pos < n && std::isspace(static_cast<unsigned char>(s[pos]))) {
            ++pos;
        }

        if (pos >= n) {
            return {std::string_view::npos, std::string_view::npos};
        }

        const std::size_t begin = pos;
        const char c = s[pos];

        if (std::isalpha(static_cast<unsigned char>(c))) {
            ++pos;
            while (pos < n && std::isalnum(static_cast<unsigned char>(s[pos]))) {
                ++pos;
            }
            return {begin, pos};
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            ++pos;
            while (pos < n && std::isdigit(static_cast<unsigned char>(s[pos]))) {
                ++pos;
            }
            return {begin, pos};
        }

        if (c == ':' && pos + 1 < n && s[pos + 1] == '=') {
            return {begin, begin + 2};
        }

        if (c == '=' || isOperator(c) || isParen(c) || isSeparator(c)) {
            return {begin, begin + 1};
        }

        return {begin, begin + 1};
    }

    TokenType classifyWord(std::string_view word) const {
        for (const auto& rule : rules_) {
            if (std::regex_match(word.begin(), word.end(), rule.pattern)) {
                return rule.type;
            }
        }
        return TokenType::Unkown;
    }
};

} // namespace calc

#endif // CALC_LEXER_H
