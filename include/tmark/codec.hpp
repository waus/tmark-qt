#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace tmark {

struct Node;
using Part = std::variant<std::string, std::shared_ptr<const Node>>;

struct Field {
    std::string name;
    std::vector<Part> value;
};

struct Node {
    std::string tag;
    std::vector<Field> fields;
    std::vector<Part> content;
    std::string raw; // Preserved only for unknown tags in soft mode.
};

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Parses one tagged value. Unknown tags are preserved verbatim when soft is true.
Node parse(std::string_view source, bool soft = false);
std::string serialize(const Node& node, bool soft = false);

} // namespace tmark
