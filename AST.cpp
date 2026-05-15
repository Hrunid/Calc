#include "AST.h"

#include <utility>

namespace calc {

AST::AST()
    : root_(NodeType::Program, {}, {}) {}

AST::AST(Node root)
    : root_(std::move(root)) {}

const AST::Node& AST::root() const noexcept {
    return root_;
}

AST::Node& AST::root() noexcept {
    return root_;
}

void AST::clear() {
    root_ = Node(NodeType::Program, {}, {});
}

} // namespace calc
