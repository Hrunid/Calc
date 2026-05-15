#ifndef CALC_ERROR_H
#define CALC_ERROR_H

#include <cstddef>
#include <ostream>
#include <string>
#include <utility>

namespace calc {

enum class ErrorType {
    UnknownToken,
    SyntaxError,
    UninitializedVariable,
    ValueError
};

inline const char* toString(ErrorType type) noexcept {
    switch (type) {
        case ErrorType::UnknownToken:          return "UnknownToken";
        case ErrorType::SyntaxError:           return "SyntaxError";
        case ErrorType::UninitializedVariable: return "UninitializedVariable";
        case ErrorType::ValueError:            return "ValueError";
        default:                               return "Error";
    }
}

inline std::ostream& operator<<(std::ostream& os, ErrorType type) {
    return os << toString(type);
}

struct Error {
    ErrorType   type;
    std::size_t position;
    std::size_t length;
    std::string message;

    Error(ErrorType _type, std::size_t _position, std::size_t _length, std::string _message)
        : type(_type),
          position(_position),
          length(_length == 0 ? std::size_t{1} : _length),
          message(std::move(_message)) {}
};

inline std::ostream& operator<<(std::ostream& os, const Error& error) {
    return os << error.type
              << " at position " << error.position
              << ": " << error.message;
}

} // namespace calc

#endif // CALC_ERROR_H
