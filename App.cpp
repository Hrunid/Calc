#include "App.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>

namespace calc {

void App::run() {
    std::string input;

    std::cout << "Arithmetic language REPL. Type 'exit' or 'quit' to close.\n";

    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, input)) {
            std::cout << '\n';
            return;
        }

        if (isExitCommand(input)) {
            return;
        }

        if (isBlank(input)) {
            continue;
        }

        handleInput(input, std::cout);
    }
}

bool App::handleInput(const std::string& input, std::ostream& out) {
    const auto tokens = lexer_.tokenize(input);
    auto parseResult = parser_.parse(tokens);

    if (!parseResult.ok()) {
        printErrors(parseResult.errors, input, out);
        return false;
    }

    auto evalResult = evaluator_.evaluate(parseResult.ast, variables_);
    if (!evalResult.ok()) {
        printErrors(evalResult.errors, input, out);
        return false;
    }

    if (evalResult.hasValue) {
        out << evalResult.value << '\n';
    }

    return true;
}

const std::unordered_map<std::string, double>& App::variables() const noexcept {
    return variables_;
}

std::unordered_map<std::string, double>& App::variables() noexcept {
    return variables_;
}

void App::printErrors(const std::vector<Error>& errors,
                      const std::string& input,
                      std::ostream& out) const {
    for (const auto& error : errors) {
        out << error << '\n';

        if (!input.empty()) {
            out << input << '\n';
            const std::size_t caretPosition = std::min(error.position, input.size());
            const std::size_t caretLength = std::max<std::size_t>(1, error.length);
            out << std::string(caretPosition, ' ')
                << std::string(caretLength, '^')
                << '\n';
        }
    }
}

bool App::isExitCommand(const std::string& input) {
    auto begin = input.begin();
    auto end = input.end();

    while (begin != end && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }
    while (begin != end && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }

    const std::string trimmed(begin, end);
    return trimmed == "exit" || trimmed == "quit" || trimmed == ":q";
}

bool App::isBlank(const std::string& input) {
    return std::all_of(input.begin(), input.end(), [](unsigned char c) {
        return std::isspace(c);
    });
}

} // namespace calc
