#ifndef CALC_AST_H
#define CALC_AST_H

#include "Token.h"

#include <ostream>
#include <utility>
#include <vector>

namespace calc {

class AST {
public:
    enum class NodeType {
        Program,
        InstructionList,

        VariableDefinition,
        Expression,
        Add,
        Mult,
        Unary,
        Power,

        Number,
        VariableReference,
        FunctionCall,

        Error
    };

    struct Node {
        NodeType            type;
        std::vector<Token>  tokens;
        std::vector<Node>   children;

        Node(NodeType _type, std::vector<Token> _tokens, std::vector<Node> _children)
            : type(_type), tokens(std::move(_tokens)), children(std::move(_children)) {}
    };

    AST();
    explicit AST(Node root);

    AST(const AST&) = default;
    AST& operator=(const AST&) = default;
    AST(AST&&) noexcept = default;
    AST& operator=(AST&&) noexcept = default;
    ~AST() = default;

    const Node& root() const noexcept;
    Node& root() noexcept;
    void clear();

private:
    Node root_;
};

inline const char* toString(AST::NodeType type) noexcept {
    switch (type) {
        case AST::NodeType::Program:            return "Program";
        case AST::NodeType::InstructionList:    return "InstructionList";
        case AST::NodeType::VariableDefinition: return "VariableDefinition";
        case AST::NodeType::Expression:         return "Expression";
        case AST::NodeType::Add:                return "Add";
        case AST::NodeType::Mult:               return "Mult";
        case AST::NodeType::Unary:              return "Unary";
        case AST::NodeType::Power:              return "Power";
        case AST::NodeType::Number:             return "Number";
        case AST::NodeType::VariableReference:  return "VariableReference";
        case AST::NodeType::FunctionCall:       return "FunctionCall";
        case AST::NodeType::Error:              return "Error";
        default:                                return "UnknownNode";
    }
}

inline std::ostream& operator<<(std::ostream& os, AST::NodeType type) {
    return os << toString(type);
}

} // namespace calc

#endif // CALC_AST_H
