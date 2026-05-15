#include "Evaluator.h"

#include <cmath>
#include <cstddef>
#include <limits>
#include <sstream>
#include <string>
#include <utility>

namespace calc {

EvalResult Evaluator::evaluate(const AST& ast, std::unordered_map<std::string, double>& variables) {
    clearErrors();
    variables_ = &variables;

    const auto value = evalNode(ast.root());
    EvalResult result;
    result.value = value.value;
    result.hasValue = value.hasValue && value.ok && errors_.empty();
    result.errors = errors_;

    variables_ = nullptr;
    return result;
}

const std::vector<Error>& Evaluator::errors() const noexcept {
    return errors_;
}

bool Evaluator::hasErrors() const noexcept {
    return !errors_.empty();
}

void Evaluator::clearErrors() {
    errors_.clear();
}

void Evaluator::addError(ErrorType type, const Token& token, std::string message) {
    addError(type, token.position, tokenLength(token), std::move(message));
}

void Evaluator::addError(ErrorType type, std::size_t position, std::size_t length,std::string message) {
    errors_.emplace_back(type, position, length == 0 ? std::size_t{1} : length, std::move(message));
}

std::size_t Evaluator::tokenLength(const Token& token) const {
    return token.lexeme.empty() ? std::size_t{1} : token.lexeme.size();
}

std::size_t Evaluator::nodePosition(const Node& node) const {
    if (!node.tokens.empty()) {
        return node.tokens.front().position;
    }
    return 0;
}

std::size_t Evaluator::nodeLength(const Node& node) const {
    if (node.tokens.empty()) {
        return 1;
    }

    const auto& first = node.tokens.front();
    const auto& last = node.tokens.back();
    const std::size_t begin = first.position;
    const std::size_t end = last.position + tokenLength(last);
    return end > begin ? end - begin : std::size_t{1};
}

std::string Evaluator::nodeText(const Node& node) const {
    std::string result;
    for (const auto& token : node.tokens) {
        result += token.lexeme;
    }
    return result;
}

Evaluator::ValueResult Evaluator::evalNode(const Node& node) {
    switch (node.type) {
        case NodeType::Program:
            return evalProgram(node);
        case NodeType::InstructionList:
            return evalInstructionList(node);
        case NodeType::VariableDefinition:
            return evalVariableDefinition(node);
        case NodeType::Expression:
            return evalExpression(node);
        case NodeType::Add:
        case NodeType::Mult:
        case NodeType::Power:
            return evalBinary(node);
        case NodeType::Unary:
            return evalUnary(node);
        case NodeType::Number:
            return evalNumber(node);
        case NodeType::VariableReference:
            return evalVariableReference(node);
        case NodeType::FunctionCall:
            return evalFunctionCall(node);
        case NodeType::Error:
            addError(ErrorType::ValueError,
                     nodePosition(node),
                     nodeLength(node),
                     "Cannot evaluate an invalid AST node. Fix parser errors first.");
            return {0.0, false, false};
    }

    addError(ErrorType::ValueError, nodePosition(node), nodeLength(node), "Unknown AST node type.");
    return {0.0, false, false};
}

Evaluator::ValueResult Evaluator::evalProgram(const Node& node) {
    if (node.children.empty()) {
        return {0.0, false, true};
    }
    return evalNode(node.children.front());
}

Evaluator::ValueResult Evaluator::evalInstructionList(const Node& node) {
    ValueResult last{0.0, false, true};

    for (const auto& child : node.children) {
        last = evalNode(child);
        if (!last.ok || !errors_.empty()) {
            return {0.0, false, false};
        }
    }

    return last;
}

Evaluator::ValueResult Evaluator::evalVariableDefinition(const Node& node) {
    if (node.children.size() < 2 || node.children[0].tokens.empty()) {
        addError(ErrorType::ValueError, nodePosition(node), nodeLength(node), "Invalid assignment node.");
        return {0.0, false, false};
    }

    const std::string variableName = node.children[0].tokens[0].lexeme;
    const auto value = evalNode(node.children[1]);
    if (!value.ok || !errors_.empty()) {
        return {0.0, false, false};
    }

    (*variables_)[variableName] = value.value;
    return {value.value, true, true};
}

Evaluator::ValueResult Evaluator::evalExpression(const Node& node) {
    if (node.children.empty()) {
        addError(ErrorType::ValueError, nodePosition(node), nodeLength(node), "Expression node has no child to evaluate.");
        return {0.0, false, false};
    }
    return evalNode(node.children.front());
}

Evaluator::ValueResult Evaluator::evalBinary(const Node& node) {
    if (node.children.size() != 2 || node.tokens.empty()) {
        addError(ErrorType::ValueError, nodePosition(node), nodeLength(node), "Invalid binary expression node.");
        return {0.0, false, false};
    }

    const auto left = evalNode(node.children[0]);
    if (!left.ok || !errors_.empty()) {
        return {0.0, false, false};
    }

    const auto right = evalNode(node.children[1]);
    if (!right.ok || !errors_.empty()) {
        return {0.0, false, false};
    }

    const std::string& op = node.tokens[0].lexeme;

    if (op == "+") {
        return {left.value + right.value, true, true};
    }
    if (op == "-") {
        return {left.value - right.value, true, true};
    }
    if (op == "*") {
        return {left.value * right.value, true, true};
    }
    if (op == "/") {
        if (right.value == 0.0) {
            addError(ErrorType::ValueError, node.tokens[0], "Division by zero is not allowed.");
            return {0.0, false, false};
        }
        return {left.value / right.value, true, true};
    }
    if (op == "^") {
        const double value = std::pow(left.value, right.value);
        if (!std::isfinite(value)) {
            addError(ErrorType::ValueError,
                     node.tokens[0],
                     "Power operation produced an undefined or non-finite value.");
            return {0.0, false, false};
        }
        return {value, true, true};
    }

    addError(ErrorType::ValueError, node.tokens[0], "Unsupported binary operator '" + op + "'.");
    return {0.0, false, false};
}

Evaluator::ValueResult Evaluator::evalUnary(const Node& node) {
    if (node.children.size() != 1 || node.tokens.empty()) {
        addError(ErrorType::ValueError, nodePosition(node), nodeLength(node), "Invalid unary expression node.");
        return {0.0, false, false};
    }

    const auto operand = evalNode(node.children[0]);
    if (!operand.ok || !errors_.empty()) {
        return {0.0, false, false};
    }

    if (node.tokens[0].lexeme == "-") {
        return {-operand.value, true, true};
    }

    addError(ErrorType::ValueError, node.tokens[0], "Unsupported unary operator '" + node.tokens[0].lexeme + "'.");
    return {0.0, false, false};
}

Evaluator::ValueResult Evaluator::evalNumber(const Node& node) {
    const std::string text = nodeText(node);
    try {
        const double value = std::stod(text);
        return {value, true, true};
    } catch (...) {
        addError(ErrorType::ValueError, nodePosition(node), nodeLength(node), "Cannot convert '" + text + "' to a number.");
        return {0.0, false, false};
    }
}

Evaluator::ValueResult Evaluator::evalVariableReference(const Node& node) {
    if (node.tokens.empty()) {
        addError(ErrorType::ValueError, nodePosition(node), nodeLength(node), "Invalid variable reference.");
        return {0.0, false, false};
    }

    const std::string& name = node.tokens[0].lexeme;
    const auto it = variables_->find(name);
    if (it == variables_->end()) {
        addError(ErrorType::UninitializedVariable,
                 node.tokens[0],
                 "Variable '" + name + "' was used before assignment.");
        return {0.0, false, false};
    }

    return {it->second, true, true};
}

Evaluator::ValueResult Evaluator::evalFunctionCall(const Node& node) {
    if (node.tokens.empty() || node.children.size() != 1) {
        addError(ErrorType::ValueError, nodePosition(node), nodeLength(node), "Invalid function call node.");
        return {0.0, false, false};
    }

    const std::string& name = node.tokens[0].lexeme;
    const auto argument = evalNode(node.children[0]);
    if (!argument.ok || !errors_.empty()) {
        return {0.0, false, false};
    }

    if (name == "sin") {
        return {std::sin(argument.value), true, true};
    }
    if (name == "cos") {
        return {std::cos(argument.value), true, true};
    }
    if (name == "tan") {
        const double value = std::tan(argument.value);
        if (!std::isfinite(value)) {
            addError(ErrorType::ValueError, node.tokens[0], "Function 'tan' produced a non-finite value.");
            return {0.0, false, false};
        }
        return {value, true, true};
    }
    if (name == "sqrt") {
        if (argument.value < 0.0) {
            addError(ErrorType::ValueError, node.tokens[0], "Function 'sqrt' is not defined for negative values.");
            return {0.0, false, false};
        }
        return {std::sqrt(argument.value), true, true};
    }
    if (name == "ln") {
        if (argument.value <= 0.0) {
            addError(ErrorType::ValueError, node.tokens[0], "Function 'ln' is defined only for positive values.");
            return {0.0, false, false};
        }
        return {std::log(argument.value), true, true};
    }
    if (name == "abs") {
        return {std::fabs(argument.value), true, true};
    }

    addError(ErrorType::ValueError, node.tokens[0], "Unknown function '" + name + "'.");
    return {0.0, false, false};
}

} // namespace calc
