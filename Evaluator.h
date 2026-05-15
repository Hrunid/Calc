#ifndef CALC_EVALUATOR_H
#define CALC_EVALUATOR_H

#include "AST.h"
#include "Error.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace calc {

struct EvalResult {
    double value = 0.0;
    bool hasValue = false;
    std::vector<Error> errors;

    bool ok() const noexcept { return errors.empty(); }
};

class Evaluator {
public:
    EvalResult evaluate(const AST& ast, std::unordered_map<std::string, double>& variables);

    const std::vector<Error>& errors() const noexcept;
    bool hasErrors() const noexcept;
    void clearErrors();

private:
    using Node = AST::Node;
    using NodeType = AST::NodeType;

    std::vector<Error> errors_;
    std::unordered_map<std::string, double>* variables_ = nullptr;

    struct ValueResult {
        double value = 0.0;
        bool hasValue = false;
        bool ok = true;
    };

    ValueResult evalNode(const Node& node);
    ValueResult evalProgram(const Node& node);
    ValueResult evalInstructionList(const Node& node);
    ValueResult evalVariableDefinition(const Node& node);
    ValueResult evalExpression(const Node& node);
    ValueResult evalBinary(const Node& node);
    ValueResult evalUnary(const Node& node);
    ValueResult evalNumber(const Node& node);
    ValueResult evalVariableReference(const Node& node);
    ValueResult evalFunctionCall(const Node& node);

    void addError(ErrorType type, const Token& token, std::string message);
    void addError(ErrorType type, std::size_t position, std::size_t length, std::string message);
    std::size_t tokenLength(const Token& token) const;
    std::size_t nodePosition(const Node& node) const;
    std::size_t nodeLength(const Node& node) const;
    std::string nodeText(const Node& node) const;
};

} // namespace calc

#endif // CALC_EVALUATOR_H
