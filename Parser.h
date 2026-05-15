#ifndef CALC_PARSER_H
#define CALC_PARSER_H

#include "AST.h"
#include "Error.h"
#include "Token.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace calc {

struct ParseResult {
    AST ast;
    std::vector<Error> errors;

    bool ok() const noexcept { return errors.empty(); }
};

class Parser {
public:
    ParseResult parse(const std::vector<Token>& tokens);

    const AST& ast() const noexcept;
    const std::vector<Token>& tokens() const noexcept;
    const std::vector<Error>& errors() const noexcept;
    bool hasErrors() const noexcept;
    void clear();

private:
    using Node = AST::Node;
    using NodeType = AST::NodeType;

    AST ast_;
    std::vector<Token> tokens_;
    std::vector<Error> errors_;

    void expandNode(Node& node);

    void addError(ErrorType type, const Token& token, std::string message);
    void addError(ErrorType type, std::size_t position, std::size_t length, std::string message);
    void addSyntaxError(const Token& token, std::string message);
    void addSyntaxErrorAt(std::size_t position, std::string message);
    void reportUnknownTokens(const std::vector<Token>& tokens);

    std::size_t tokenLength(const Token& token) const;
    std::size_t endPosition(const std::vector<Token>& tokens) const;

    std::vector<Token> slice(const std::vector<Token>& tokens, std::size_t begin, std::size_t end) const;

    std::vector<std::size_t> findTopLevelSemicolons(const std::vector<Token>& tokens) const;
    std::vector<std::size_t> findTopLevelAssignOperators(const std::vector<Token>& tokens) const;
    std::size_t findRightmostTopLevelAddOperator(const std::vector<Token>& tokens) const;
    std::size_t findRightmostTopLevelMultOperator(const std::vector<Token>& tokens) const;
    std::size_t findLeftmostTopLevelPowerOperator(const std::vector<Token>& tokens) const;

    bool hasBalancedParentheses(const std::vector<Token>& tokens);
    bool isWrappedInParentheses(const std::vector<Token>& tokens) const;
    bool canBeLeftOperandBeforeBinaryOperator(const std::vector<Token>& tokens, std::size_t operatorIndex) const;
    bool isNumber(const std::vector<Token>& tokens) const;
    bool looksLikeMalformedNumber(const std::vector<Token>& tokens) const;
    bool containsTokenType(const std::vector<Token>& tokens, TokenType type) const;

    Node makeErrorNode(const std::vector<Token>& tokens) const;
    Node makeInstructionNode(const std::vector<Token>& tokens);
    Node makeExpressionNode(const std::vector<Token>& tokens);

    Node parseExpression(const std::vector<Token>& tokens);
    Node parseAdd(const std::vector<Token>& tokens);
    Node parseMult(const std::vector<Token>& tokens);
    Node parseUnary(const std::vector<Token>& tokens);
    Node parsePower(const std::vector<Token>& tokens);
    Node parsePrimary(const std::vector<Token>& tokens);
};

} // namespace calc

#endif // CALC_PARSER_H
