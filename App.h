#ifndef CALC_APP_H
#define CALC_APP_H

#include "Error.h"
#include "Evaluator.h"
#include "Lexer.h"
#include "Parser.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace calc {

class App {
public:
    void run();
    bool handleInput(const std::string& input, std::ostream& out = std::cout);

    const std::unordered_map<std::string, double>& variables() const noexcept;
    std::unordered_map<std::string, double>& variables() noexcept;

private:
    Lexer lexer_;
    Parser parser_;
    Evaluator evaluator_;
    std::unordered_map<std::string, double> variables_;

    void printErrors(const std::vector<Error>& errors,const std::string& input, std::ostream& out) const;
    static bool isExitCommand(const std::string& input);
    static bool isBlank(const std::string& input);
};

} // namespace calc

#endif // CALC_APP_H
