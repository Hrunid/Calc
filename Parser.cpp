#include "Parser.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace calc {
namespace {

bool isToken(const Token& token, TokenType type, std::string_view lexeme) {
    return token.type == type && token.lexeme == lexeme;
}

bool isLeftParen(const Token& token) {
    return isToken(token, TokenType::Parenthesis, "(");
}

bool isRightParen(const Token& token) {
    return isToken(token, TokenType::Parenthesis, ")");
}

bool isOperator(const Token& token, std::string_view op) {
    return isToken(token, TokenType::Operator, op);
}

bool isAnyOperator(const Token& token) {
    return token.type == TokenType::Operator;
}

bool isSeparator(const Token& token, std::string_view separator) {
    return isToken(token, TokenType::Separator, separator);
}

bool isExpressionAtomStarter(const Token& token) {
    return token.type == TokenType::Integer ||
           token.type == TokenType::Variable ||
           token.type == TokenType::Function ||
           isLeftParen(token) ||
           isOperator(token, "-");
}

bool isExpressionAtomEnder(const Token& token) {
    return token.type == TokenType::Integer ||
           token.type == TokenType::Variable ||
           isRightParen(token);
}

std::string quoted(const std::string& text) {
    return "'" + text + "'";
}

std::string tokenName(const Token& token) {
    return quoted(token.lexeme);
}

std::optional<std::size_t> findMatchingRightParen(const std::vector<Token>& tokens, std::size_t leftParenIndex) {
    if (leftParenIndex >= tokens.size() || !isLeftParen(tokens[leftParenIndex])) {
        return std::nullopt;
    }

    int depth = 0;
    for (std::size_t i = leftParenIndex; i < tokens.size(); ++i) {
        if (isLeftParen(tokens[i])) {
            ++depth;
        } else if (isRightParen(tokens[i])) {
            --depth;
            if (depth == 0) {
                return i;
            }
            if (depth < 0) {
                return std::nullopt;
            }
        }
    }

    return std::nullopt;
}

} // namespace

ParseResult Parser::parse(const std::vector<Token>& tokens) {
    clear();
    tokens_ = tokens;

    reportUnknownTokens(tokens_);

    Node root(NodeType::Program, tokens_, {});
    expandNode(root);
    ast_ = AST(std::move(root));

    return ParseResult{ast_, errors_};
}

const AST& Parser::ast() const noexcept {
    return ast_;
}

const std::vector<Token>& Parser::tokens() const noexcept {
    return tokens_;
}

const std::vector<Error>& Parser::errors() const noexcept {
    return errors_;
}

bool Parser::hasErrors() const noexcept {
    return !errors_.empty();
}

void Parser::clear() {
    ast_.clear();
    tokens_.clear();
    errors_.clear();
}

void Parser::addError(ErrorType type, const Token& token, std::string message) {
    addError(type, token.position, tokenLength(token), std::move(message));
}

void Parser::addError(ErrorType type, std::size_t position, std::size_t length, std::string message) {
    errors_.emplace_back(type, position, length == 0 ? std::size_t{1} : length, std::move(message));
}

void Parser::addSyntaxError(const Token& token, std::string message) {
    addError(ErrorType::SyntaxError, token, std::move(message));
}

void Parser::addSyntaxErrorAt(std::size_t position, std::string message) {
    addError(ErrorType::SyntaxError, position, 1, std::move(message));
}

void Parser::reportUnknownTokens(const std::vector<Token>& tokens) {
    for (const auto& token : tokens) {
        if (token.type == TokenType::Unkown) {
            addError(
                ErrorType::UnknownToken,
                token,
                "Unrecognized token " + tokenName(token) + ". This text is not part of the language."
            );
        }
    }
}

std::size_t Parser::tokenLength(const Token& token) const {
    return token.lexeme.empty() ? std::size_t{1} : token.lexeme.size();
}

std::size_t Parser::endPosition(const std::vector<Token>& tokens) const {
    if (tokens.empty()) {
        return 0;
    }
    const auto& last = tokens.back();
    return last.position + tokenLength(last);
}

std::vector<Token> Parser::slice(const std::vector<Token>& tokens, std::size_t begin, std::size_t end) const {
    if (begin >= end || begin >= tokens.size()) {
        return {};
    }

    end = std::min(end, tokens.size());
    return std::vector<Token>(tokens.begin() + static_cast<std::ptrdiff_t>(begin),
                              tokens.begin() + static_cast<std::ptrdiff_t>(end));
}

AST::Node Parser::makeErrorNode(const std::vector<Token>& tokens) const {
    return Node(NodeType::Error, tokens, {});
}

AST::Node Parser::makeExpressionNode(const std::vector<Token>& tokens) {
    return Node(NodeType::Expression, tokens, {parseExpression(tokens)});
}

std::vector<std::size_t> Parser::findTopLevelSemicolons(const std::vector<Token>& tokens) const {
    std::vector<std::size_t> result;
    int depth = 0;

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (isLeftParen(tokens[i])) {
            ++depth;
        } else if (isRightParen(tokens[i])) {
            --depth;
        } else if (depth == 0 && isSeparator(tokens[i], ";")) {
            result.push_back(i);
        }
    }

    return result;
}

std::vector<std::size_t> Parser::findTopLevelAssignOperators(const std::vector<Token>& tokens) const {
    std::vector<std::size_t> result;
    int depth = 0;

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (isLeftParen(tokens[i])) {
            ++depth;
        } else if (isRightParen(tokens[i])) {
            --depth;
        } else if (depth == 0 && tokens[i].type == TokenType::Assign) {
            result.push_back(i);
        }
    }

    return result;
}

bool Parser::hasBalancedParentheses(const std::vector<Token>& tokens) {
    std::vector<Token> leftParens;
    bool balanced = true;

    for (const auto& token : tokens) {
        if (isLeftParen(token)) {
            leftParens.push_back(token);
        } else if (isRightParen(token)) {
            if (leftParens.empty()) {
                addSyntaxError(token, "Unexpected closing parenthesis ')'. There is no matching '('.");
                balanced = false;
            } else {
                leftParens.pop_back();
            }
        }
    }

    for (const auto& token : leftParens) {
        addSyntaxError(token, "Missing closing parenthesis ')' for this '('.");
        balanced = false;
    }

    return balanced;
}

bool Parser::isWrappedInParentheses(const std::vector<Token>& tokens) const {
    if (tokens.size() < 2 || !isLeftParen(tokens.front()) || !isRightParen(tokens.back())) {
        return false;
    }

    int depth = 0;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (isLeftParen(tokens[i])) {
            ++depth;
        } else if (isRightParen(tokens[i])) {
            --depth;
            if (depth == 0 && i != tokens.size() - 1) {
                return false;
            }
            if (depth < 0) {
                return false;
            }
        }
    }

    return depth == 0;
}

bool Parser::canBeLeftOperandBeforeBinaryOperator(const std::vector<Token>& tokens, std::size_t operatorIndex) const {
    if (operatorIndex == 0) {
        return false;
    }

    const Token& previous = tokens[operatorIndex - 1];
    if (previous.type == TokenType::Operator || previous.type == TokenType::Assign || isLeftParen(previous)) {
        return false;
    }

    return true;
}

std::size_t Parser::findRightmostTopLevelAddOperator(const std::vector<Token>& tokens) const {
    int depth = 0;

    for (std::size_t i = tokens.size(); i-- > 0;) {
        if (isRightParen(tokens[i])) {
            ++depth;
            continue;
        }
        if (isLeftParen(tokens[i])) {
            --depth;
            continue;
        }
        if (depth != 0) {
            continue;
        }

        if ((isOperator(tokens[i], "+") || isOperator(tokens[i], "-")) &&
            canBeLeftOperandBeforeBinaryOperator(tokens, i)) {
            return i;
        }
    }

    return tokens.size();
}

std::size_t Parser::findRightmostTopLevelMultOperator(const std::vector<Token>& tokens) const {
    int depth = 0;

    for (std::size_t i = tokens.size(); i-- > 0;) {
        if (isRightParen(tokens[i])) {
            ++depth;
            continue;
        }
        if (isLeftParen(tokens[i])) {
            --depth;
            continue;
        }
        if (depth != 0) {
            continue;
        }

        if ((isOperator(tokens[i], "*") || isOperator(tokens[i], "/")) &&
            canBeLeftOperandBeforeBinaryOperator(tokens, i)) {
            return i;
        }
    }

    return tokens.size();
}

std::size_t Parser::findLeftmostTopLevelPowerOperator(const std::vector<Token>& tokens) const {
    int depth = 0;

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (isLeftParen(tokens[i])) {
            ++depth;
            continue;
        }
        if (isRightParen(tokens[i])) {
            --depth;
            continue;
        }
        if (depth == 0 && isOperator(tokens[i], "^") &&
            canBeLeftOperandBeforeBinaryOperator(tokens, i)) {
            return i;
        }
    }

    return tokens.size();
}

bool Parser::isNumber(const std::vector<Token>& tokens) const {
    if (tokens.size() == 1 && tokens[0].type == TokenType::Integer) {
        return true;
    }

    return tokens.size() == 3 &&
           tokens[0].type == TokenType::Integer &&
           isSeparator(tokens[1], ".") &&
           tokens[2].type == TokenType::Integer;
}

bool Parser::looksLikeMalformedNumber(const std::vector<Token>& tokens) const {
    if (tokens.empty()) {
        return false;
    }

    bool hasDot = false;
    bool hasInteger = false;

    for (const auto& token : tokens) {
        if (token.type == TokenType::Integer) {
            hasInteger = true;
        } else if (isSeparator(token, ".")) {
            hasDot = true;
        } else {
            return false;
        }
    }

    return hasDot && hasInteger && !isNumber(tokens);
}

bool Parser::containsTokenType(const std::vector<Token>& tokens, TokenType type) const {
    return std::any_of(tokens.begin(), tokens.end(), [type](const Token& token) {
        return token.type == type;
    });
}

AST::Node Parser::makeInstructionNode(const std::vector<Token>& tokens) {
    if (tokens.empty()) {
        addSyntaxErrorAt(0, "Expected instruction, but found an empty instruction.");
        return makeErrorNode(tokens);
    }

    const auto assigns = findTopLevelAssignOperators(tokens);
    if (!assigns.empty()) {
        for (std::size_t i = 1; i < assigns.size(); ++i) {
            addSyntaxError(
                tokens[assigns[i]],
                "Only one assignment operator is allowed in one instruction. Use ';' to separate instructions."
            );
        }

        const std::size_t assignIndex = assigns.front();
        Node node(NodeType::VariableDefinition, tokens, {});

        if (assignIndex == 0) {
            addSyntaxError(tokens[assignIndex],
                           "Expected variable name before assignment operator " + tokenName(tokens[assignIndex]) + ".");
            node.children.push_back(makeErrorNode({}));
        } else if (assignIndex != 1 || tokens[0].type != TokenType::Variable) {
            addSyntaxError(tokens.front(),
                           "Left side of assignment must be exactly one variable, for example: x = expression.");
            node.children.push_back(makeErrorNode(slice(tokens, 0, assignIndex)));
        } else {
            node.children.push_back(Node(NodeType::VariableReference, {tokens[0]}, {}));
        }

        const auto valueTokens = slice(tokens, assignIndex + 1, tokens.size());
        if (valueTokens.empty()) {
            addSyntaxErrorAt(tokens[assignIndex].position + tokenLength(tokens[assignIndex]),
                             "Expected expression after assignment operator " + tokenName(tokens[assignIndex]) + ".");
            node.children.push_back(makeErrorNode(valueTokens));
        } else {
            node.children.push_back(makeExpressionNode(valueTokens));
        }

        return node;
    }

    return makeExpressionNode(tokens);
}

AST::Node Parser::parseExpression(const std::vector<Token>& tokens) {
    if (tokens.empty()) {
        addSyntaxErrorAt(0, "Expected expression, but found nothing.");
        return makeErrorNode(tokens);
    }

    if (containsTokenType(tokens, TokenType::Unkown)) {
        return makeErrorNode(tokens);
    }

    if (!hasBalancedParentheses(tokens)) {
        return makeErrorNode(tokens);
    }

    return parseAdd(tokens);
}

AST::Node Parser::parseAdd(const std::vector<Token>& tokens) {
    const std::size_t operatorIndex = findRightmostTopLevelAddOperator(tokens);
    if (operatorIndex == tokens.size()) {
        return parseMult(tokens);
    }

    auto leftTokens = slice(tokens, 0, operatorIndex);
    auto rightTokens = slice(tokens, operatorIndex + 1, tokens.size());
    Node node(NodeType::Add, {tokens[operatorIndex]}, {});

    if (leftTokens.empty()) {
        addSyntaxError(tokens[operatorIndex],
                       "Expected expression before operator " + tokenName(tokens[operatorIndex]) + ".");
        node.children.push_back(makeErrorNode(leftTokens));
    } else {
        node.children.push_back(parseExpression(leftTokens));
    }

    if (rightTokens.empty()) {
        addSyntaxErrorAt(tokens[operatorIndex].position + tokenLength(tokens[operatorIndex]),
                         "Expected expression after operator " + tokenName(tokens[operatorIndex]) + ".");
        node.children.push_back(makeErrorNode(rightTokens));
    } else {
        node.children.push_back(parseMult(rightTokens));
    }

    return node;
}

AST::Node Parser::parseMult(const std::vector<Token>& tokens) {
    const std::size_t operatorIndex = findRightmostTopLevelMultOperator(tokens);
    if (operatorIndex == tokens.size()) {
        return parseUnary(tokens);
    }

    auto leftTokens = slice(tokens, 0, operatorIndex);
    auto rightTokens = slice(tokens, operatorIndex + 1, tokens.size());
    Node node(NodeType::Mult, {tokens[operatorIndex]}, {});

    if (leftTokens.empty()) {
        addSyntaxError(tokens[operatorIndex],
                       "Expected expression before operator " + tokenName(tokens[operatorIndex]) + ".");
        node.children.push_back(makeErrorNode(leftTokens));
    } else {
        node.children.push_back(parseExpression(leftTokens));
    }

    if (rightTokens.empty()) {
        addSyntaxErrorAt(tokens[operatorIndex].position + tokenLength(tokens[operatorIndex]),
                         "Expected expression after operator " + tokenName(tokens[operatorIndex]) + ".");
        node.children.push_back(makeErrorNode(rightTokens));
    } else {
        node.children.push_back(parseUnary(rightTokens));
    }

    return node;
}

AST::Node Parser::parseUnary(const std::vector<Token>& tokens) {
    if (tokens.empty()) {
        addSyntaxErrorAt(0, "Expected expression, but found nothing.");
        return makeErrorNode(tokens);
    }

    if (isOperator(tokens.front(), "-")) {
        auto operandTokens = slice(tokens, 1, tokens.size());
        Node node(NodeType::Unary, {tokens.front()}, {});

        if (operandTokens.empty()) {
            addSyntaxErrorAt(tokens.front().position + tokenLength(tokens.front()),
                             "Expected expression after unary operator '-'.");
            node.children.push_back(makeErrorNode(operandTokens));
        } else {
            node.children.push_back(parseUnary(operandTokens));
        }

        return node;
    }

    return parsePower(tokens);
}

AST::Node Parser::parsePower(const std::vector<Token>& tokens) {
    const std::size_t operatorIndex = findLeftmostTopLevelPowerOperator(tokens);
    if (operatorIndex == tokens.size()) {
        return parsePrimary(tokens);
    }

    auto baseTokens = slice(tokens, 0, operatorIndex);
    auto exponentTokens = slice(tokens, operatorIndex + 1, tokens.size());
    Node node(NodeType::Power, {tokens[operatorIndex]}, {});

    if (baseTokens.empty()) {
        addSyntaxError(tokens[operatorIndex], "Expected base expression before operator '^'.");
        node.children.push_back(makeErrorNode(baseTokens));
    } else {
        node.children.push_back(parsePrimary(baseTokens));
    }

    if (exponentTokens.empty()) {
        addSyntaxErrorAt(tokens[operatorIndex].position + tokenLength(tokens[operatorIndex]),
                         "Expected exponent expression after operator '^'.");
        node.children.push_back(makeErrorNode(exponentTokens));
    } else {
        node.children.push_back(parseUnary(exponentTokens));
    }

    return node;
}

AST::Node Parser::parsePrimary(const std::vector<Token>& tokens) {
    if (tokens.empty()) {
        addSyntaxErrorAt(0, "Expected primary expression: number, variable, function call or parenthesized expression.");
        return makeErrorNode(tokens);
    }

    if (tokens.size() == 1 && tokens[0].type == TokenType::Unkown) {
        return makeErrorNode(tokens);
    }

    if (isWrappedInParentheses(tokens)) {
        auto inner = slice(tokens, 1, tokens.size() - 1);
        if (inner.empty()) {
            addSyntaxError(tokens.front(), "Expected expression inside parentheses.");
            return makeErrorNode(tokens);
        }
        return parseExpression(inner);
    }

    if (isNumber(tokens)) {
        return Node(NodeType::Number, tokens, {});
    }

    if (looksLikeMalformedNumber(tokens)) {
        std::size_t dotCount = 0;
        std::optional<Token> secondDot;
        for (const auto& token : tokens) {
            if (isSeparator(token, ".")) {
                ++dotCount;
                if (dotCount == 2) {
                    secondDot = token;
                    break;
                }
            }
        }

        if (secondDot.has_value()) {
            addSyntaxError(*secondDot, "Unexpected decimal separator '.'. A number can contain only one decimal separator.");
        } else if (tokens.size() >= 2 && tokens[0].type == TokenType::Integer && isSeparator(tokens[1], ".") && tokens.size() == 2) {
            addSyntaxErrorAt(tokens[1].position + tokenLength(tokens[1]),
                             "Expected digits after decimal separator '.'.");
        } else if (!tokens.empty() && isSeparator(tokens.front(), ".")) {
            addSyntaxError(tokens.front(), "Expected digits before decimal separator '.'. Use form 0.5 instead of .5.");
        } else {
            addSyntaxError(tokens.front(), "Malformed number. A float must have the form Integer.Integer, for example 3.14.");
        }
        return makeErrorNode(tokens);
    }

    if (tokens.size() == 1 && tokens[0].type == TokenType::Variable) {
        return Node(NodeType::VariableReference, tokens, {});
    }

    if (tokens[0].type == TokenType::Function) {
        if (tokens.size() == 1) {
            addSyntaxError(tokens[0],
                           "Function " + tokenName(tokens[0]) + " must be called with parentheses, for example " + tokens[0].lexeme + "(x).");
            return makeErrorNode(tokens);
        }

        if (!isLeftParen(tokens[1])) {
            addSyntaxError(tokens[1], "Expected '(' after function name " + tokenName(tokens[0]) + ".");
            return makeErrorNode(tokens);
        }

        const auto closingParen = findMatchingRightParen(tokens, 1);
        if (!closingParen.has_value()) {
            return makeErrorNode(tokens);
        }

        if (*closingParen != tokens.size() - 1) {
            addSyntaxError(tokens[*closingParen + 1],
                           "Expected operator after function call, but found " + tokenName(tokens[*closingParen + 1]) + ".");
            return makeErrorNode(tokens);
        }

        auto argumentTokens = slice(tokens, 2, tokens.size() - 1);
        if (argumentTokens.empty()) {
            addSyntaxError(tokens[1], "Expected expression as argument of function " + tokenName(tokens[0]) + ".");
            return makeErrorNode(tokens);
        }

        return Node(NodeType::FunctionCall, {tokens[0]}, {parseExpression(argumentTokens)});
    }

    if (isLeftParen(tokens.front())) {
        const auto closingParen = findMatchingRightParen(tokens, 0);
        if (closingParen.has_value() && *closingParen != tokens.size() - 1) {
            addSyntaxError(tokens[*closingParen + 1],
                           "Expected operator after parenthesized expression, but found " + tokenName(tokens[*closingParen + 1]) + ".");
        } else {
            addSyntaxError(tokens.front(), "Expected complete parenthesized expression.");
        }
        return makeErrorNode(tokens);
    }

    if (isRightParen(tokens.front())) {
        addSyntaxError(tokens.front(), "Unexpected closing parenthesis ')'.");
        return makeErrorNode(tokens);
    }

    if (tokens.front().type == TokenType::Assign) {
        addSyntaxError(tokens.front(), "Assignment operator " + tokenName(tokens.front()) + " cannot appear inside an expression.");
        return makeErrorNode(tokens);
    }

    if (isSeparator(tokens.front(), ";")) {
        addSyntaxError(tokens.front(), "Unexpected ';' inside expression. Semicolon separates instructions, not expression parts.");
        return makeErrorNode(tokens);
    }

    if (isSeparator(tokens.front(), ".")) {
        addSyntaxError(tokens.front(), "Unexpected decimal separator '.'. Expected digits before '.'.");
        return makeErrorNode(tokens);
    }

    if (isAnyOperator(tokens.front())) {
        if (isOperator(tokens.front(), "+")) {
            addSyntaxError(tokens.front(), "Unexpected unary '+'. This grammar supports unary '-' only.");
        } else {
            addSyntaxError(tokens.front(), "Unexpected operator " + tokenName(tokens.front()) + ". Expected expression before this operator.");
        }
        return makeErrorNode(tokens);
    }

    if (tokens.size() >= 2 && isExpressionAtomEnder(tokens[0]) && isExpressionAtomStarter(tokens[1])) {
        addSyntaxError(tokens[1], "Expected operator between " + tokenName(tokens[0]) + " and " + tokenName(tokens[1]) + ".");
        return makeErrorNode(tokens);
    }

    if (tokens.size() >= 2) {
        addSyntaxError(tokens[1], "Unexpected token " + tokenName(tokens[1]) + ". Expected arithmetic operator or end of expression.");
        return makeErrorNode(tokens);
    }

    addSyntaxError(tokens.front(), "Unexpected token " + tokenName(tokens.front()) + ". Expected number, variable, function call or '('.");
    return makeErrorNode(tokens);
}

void Parser::expandNode(AST::Node& node) {
    switch (node.type) {
        case NodeType::Program: {
            node.children.clear();
            node.children.push_back(Node(NodeType::InstructionList, node.tokens, {}));
            expandNode(node.children.back());
            break;
        }

        case NodeType::InstructionList: {
            node.children.clear();

            if (node.tokens.empty()) {
                addSyntaxErrorAt(0, "Expected instruction, but input is empty.");
                break;
            }

            const auto separators = findTopLevelSemicolons(node.tokens);
            std::size_t begin = 0;

            for (const auto separatorIndex : separators) {
                if (separatorIndex == begin) {
                    addSyntaxError(node.tokens[separatorIndex], "Empty instruction before ';'.");
                    node.children.push_back(makeErrorNode({node.tokens[separatorIndex]}));
                } else {
                    node.children.push_back(makeInstructionNode(slice(node.tokens, begin, separatorIndex)));
                }
                begin = separatorIndex + 1;
            }

            if (begin < node.tokens.size()) {
                node.children.push_back(makeInstructionNode(slice(node.tokens, begin, node.tokens.size())));
            }
            break;
        }

        case NodeType::VariableDefinition:
            node = makeInstructionNode(node.tokens);
            break;

        case NodeType::Expression:
            node.children.clear();
            node.children.push_back(parseExpression(node.tokens));
            break;

        case NodeType::Add:
            node = parseAdd(node.tokens);
            break;

        case NodeType::Mult:
            node = parseMult(node.tokens);
            break;

        case NodeType::Unary:
            node = parseUnary(node.tokens);
            break;

        case NodeType::Power:
            node = parsePower(node.tokens);
            break;

        case NodeType::Number:
        case NodeType::VariableReference:
        case NodeType::FunctionCall:
        case NodeType::Error:
            break;
    }
}

} // namespace calc
