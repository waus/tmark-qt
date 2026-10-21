#include "tmark/codec.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <set>
#include <string_view>

namespace tmark {
namespace {

constexpr int maxDepth = 15;

enum class Kind { text, number, integer, boolean, inlineNodes, blockNodes, listItems, rows, cells, attached, gallery, caption, none };
enum class Layout { compact, expanded, multiple };
struct FieldSpec { std::string_view name; Kind kind; bool required = false; };
struct Spec { std::string_view tag; Kind display; Layout layout; std::vector<FieldSpec> fields; Kind content; };

// The schema is the single source of truth for validation and wire layout.
const std::array<Spec, 39> specs {{
    {"caption", Kind::none, Layout::compact, {{"credit", Kind::inlineNodes}}, Kind::inlineNodes},
    {"a", Kind::inlineNodes, Layout::compact, {{"href", Kind::text, true}}, Kind::inlineNodes},
    {"anchor-link", Kind::inlineNodes, Layout::compact, {{"name", Kind::text, true}}, Kind::inlineNodes},
    {"ref", Kind::inlineNodes, Layout::compact, {{"name", Kind::text, true}}, Kind::inlineNodes},
    {"ref-link", Kind::inlineNodes, Layout::compact, {{"name", Kind::text, true}}, Kind::inlineNodes},
    {"b", Kind::inlineNodes, Layout::compact, {}, Kind::inlineNodes},
    {"i", Kind::inlineNodes, Layout::compact, {}, Kind::inlineNodes},
    {"m", Kind::inlineNodes, Layout::compact, {}, Kind::inlineNodes},
    {"u", Kind::inlineNodes, Layout::compact, {}, Kind::inlineNodes},
    {"s", Kind::inlineNodes, Layout::compact, {}, Kind::inlineNodes},
    {"spoiler", Kind::inlineNodes, Layout::compact, {}, Kind::inlineNodes},
    {"sub", Kind::inlineNodes, Layout::compact, {}, Kind::inlineNodes},
    {"sup", Kind::inlineNodes, Layout::compact, {}, Kind::inlineNodes},
    {"datetime", Kind::inlineNodes, Layout::compact, {{"timezone", Kind::text}}, Kind::number},
    {"code", Kind::inlineNodes, Layout::compact, {}, Kind::text},
    {"math", Kind::inlineNodes, Layout::compact, {}, Kind::text},
    {"icon", Kind::inlineNodes, Layout::compact, {{"alt", Kind::text}}, Kind::text},
    {"p", Kind::blockNodes, Layout::compact, {}, Kind::inlineNodes},
    {"h", Kind::blockNodes, Layout::compact, {{"s", Kind::integer, true}}, Kind::inlineNodes},
    {"pre", Kind::blockNodes, Layout::expanded, {{"language", Kind::text}}, Kind::text},
    {"math-block", Kind::blockNodes, Layout::expanded, {}, Kind::text},
    {"anchor", Kind::blockNodes, Layout::compact, {}, Kind::text},
    {"hr", Kind::blockNodes, Layout::compact, {}, Kind::none},
    {"q", Kind::blockNodes, Layout::multiple, {{"credit", Kind::inlineNodes}}, Kind::blockNodes},
    {"as", Kind::blockNodes, Layout::compact, {{"credit", Kind::inlineNodes}}, Kind::inlineNodes},
    {"collage", Kind::blockNodes, Layout::expanded, {{"caption", Kind::caption}}, Kind::gallery},
    {"list", Kind::blockNodes, Layout::expanded, {}, Kind::listItems},
    {"li", Kind::blockNodes, Layout::multiple, {{"type", Kind::text}, {"order", Kind::integer}, {"checked", Kind::boolean}}, Kind::blockNodes},
    {"map", Kind::blockNodes, Layout::compact, {{"lat", Kind::number, true}, {"lon", Kind::number, true}, {"zoom", Kind::integer}, {"caption", Kind::caption}}, Kind::none},
    {"img", Kind::blockNodes, Layout::compact, {{"caption", Kind::caption}, {"has_spoiler", Kind::boolean}}, Kind::text},
    {"video", Kind::blockNodes, Layout::compact, {{"caption", Kind::caption}, {"has_spoiler", Kind::boolean}, {"preview", Kind::text, true}, {"loop", Kind::boolean}}, Kind::text},
    {"audio", Kind::blockNodes, Layout::compact, {{"caption", Kind::caption}}, Kind::text},
    {"slideshow", Kind::blockNodes, Layout::expanded, {{"caption", Kind::caption}}, Kind::gallery},
    {"table", Kind::blockNodes, Layout::expanded, {{"caption", Kind::inlineNodes}, {"bordered", Kind::boolean}, {"striped", Kind::boolean}}, Kind::rows},
    {"tr", Kind::blockNodes, Layout::compact, {}, Kind::cells},
    {"td", Kind::blockNodes, Layout::compact, {{"header", Kind::boolean}, {"colspan", Kind::integer}, {"rowspan", Kind::integer}, {"align", Kind::text}, {"valign", Kind::text}}, Kind::inlineNodes},
    {"details", Kind::blockNodes, Layout::multiple, {{"summary", Kind::inlineNodes}, {"open", Kind::boolean}}, Kind::blockNodes},
    {"attached", Kind::none, Layout::expanded, {{"hash", Kind::text, true}}, Kind::blockNodes},
    {"document", Kind::none, Layout::expanded, {{"url", Kind::text, true}, {"title", Kind::text, true}, {"description", Kind::text}, {"author_name", Kind::text}, {"author_url", Kind::text}, {"image_url", Kind::text}, {"attached_media", Kind::attached}}, Kind::blockNodes},
}};

const Spec* schema(std::string_view tag) {
    const auto it = std::find_if(specs.begin(), specs.end(), [tag](const Spec& s) { return s.tag == tag; });
    return it == specs.end() ? nullptr : &*it;
}

bool nameChar(char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_'; }
bool validName(std::string_view name) {
    return !name.empty() && name.size() <= 32 && std::all_of(name.begin(), name.end(), nameChar);
}
bool validUtf8(std::string_view source) {
    for (size_t i = 0; i < source.size();) {
        const auto first = static_cast<unsigned char>(source[i]);
        if (first < 0x80) { ++i; continue; }
        const int count = first >= 0xf0 && first <= 0xf4 ? 4 : first >= 0xe0 && first <= 0xef ? 3 : first >= 0xc2 && first <= 0xdf ? 2 : 0;
        if (!count || i + count > source.size()) return false;
        for (int j = 1; j < count; ++j) if ((static_cast<unsigned char>(source[i + j]) & 0xc0) != 0x80) return false;
        const auto second = static_cast<unsigned char>(source[i + 1]);
        if ((first == 0xe0 && second < 0xa0) || (first == 0xed && second >= 0xa0) ||
            (first == 0xf0 && second < 0x90) || (first == 0xf4 && second >= 0x90)) return false;
        i += count;
    }
    return true;
}
bool newline(const Part& part) {
    const auto* s = std::get_if<std::string>(&part);
    return s && s->find('\n') != s->npos && std::all_of(s->begin(), s->end(), [](unsigned char c) { return std::isspace(c); });
}

class Parser {
public:
    explicit Parser(std::string_view source) : source_(source) {}
    Node root() {
        if (source_.starts_with("\xef\xbb\xbf")) throw Error("BOM is invalid");
        if (!validUtf8(source_)) throw Error("invalid UTF-8");
        for (size_t i = 0; i < source_.size(); ++i) {
            if (source_[i] == '\r' && (i + 1 == source_.size() || source_[i + 1] != '\n')) throw Error("standalone CR is invalid");
        }
        auto result = node(0);
        while (peek() == '\n' || peek() == '\r') ++pos_;
        if (pos_ != source_.size() || result.tag.empty()) throw Error("expected one tagged value");
        return result;
    }
private:
    std::string_view source_;
    size_t pos_ = 0;
    char peek() const { return pos_ < source_.size() ? source_[pos_] : '\0'; }
    std::string readName() {
        const auto start = pos_;
        while (nameChar(peek())) ++pos_;
        const auto name = source_.substr(start, pos_ - start);
        if (!validName(name)) throw Error("invalid tag or field name");
        return std::string(name);
    }
    Node node(int depth, bool allowTag = true) {
        if (depth > maxDepth) throw Error("maximum depth exceeded");
        const auto start = pos_;
        if (peek() != '{') throw Error("expected opening brace");
        ++pos_;
        Node out;
        const auto saved = pos_;
        if (allowTag && nameChar(peek())) {
            out.tag = readName();
            if (peek() == ';') ++pos_;
            else { out.tag.clear(); pos_ = saved; }
        }
        bool contentSeen = false;
        std::set<std::string> seen;
        while (peek() && peek() != '}') {
            if (peek() == '#') {
                if (contentSeen) throw Error("field after content");
                ++pos_;
                Field field;
                field.name = readName();
                if (!seen.insert(field.name).second) throw Error("duplicate field");
                if (peek() != '{') throw Error("expected field value");
                field.value.emplace_back(std::make_shared<Node>(node(depth + 1, false)));
                out.fields.push_back(std::move(field));
            } else if (peek() == '{') {
                out.content.emplace_back(std::make_shared<Node>(node(depth + 1)));
                contentSeen = true;
            } else {
                std::string value;
                while (peek() && peek() != '}' && peek() != '{' && peek() != '#') {
                    auto c = source_[pos_++];
                    if (c == '\\') {
                        if (!peek() || std::string_view("#{}\\").find(peek()) == std::string_view::npos) throw Error("invalid escape");
                        c = source_[pos_++];
                    }
                    if (c == '\r') { if (peek() == '\n') ++pos_; c = '\n'; }
                    value.push_back(c);
                }
                if (!value.empty()) {
                    out.content.emplace_back(std::move(value));
                    if (!newline(out.content.back())) contentSeen = true;
                    else if (!contentSeen && !out.tag.empty()) {
                        const auto* spec = schema(out.tag);
                        if (spec && spec->layout != Layout::compact) out.content.pop_back();
                    }
                }
            }
        }
        if (peek() != '}') throw Error("missing closing brace");
        ++pos_;
        out.raw = std::string(source_.substr(start, pos_ - start));
        return out;
    }
};

std::string scalar(const std::vector<Part>& parts, bool expanded = false) {
    std::string out;
    for (const auto& part : parts) {
        const auto* s = std::get_if<std::string>(&part);
        if (!s) throw Error("expected scalar");
        out += *s;
    }
    if (expanded) {
        if (out.starts_with('\n')) out.erase(0, 1);
        if (out.ends_with('\n')) out.pop_back();
    }
    return out;
}

bool validNumber(std::string_view s, bool integer) {
    if (s.empty()) return false;
    size_t i = s[0] == '-' ? 1 : 0;
    if (i >= s.size() || s[i] < '0' || s[i] > '9') return false;
    if (s[i] == '0') { ++i; if (i < s.size() && s[i] >= '0' && s[i] <= '9') return false; }
    else { while (i < s.size() && s[i] >= '0' && s[i] <= '9') ++i; }
    if (i == s.size()) return s != "-0";
    if (integer || s[i++] != '.' || i >= s.size()) return false;
    while (i < s.size() && s[i] >= '0' && s[i] <= '9') ++i;
    return i == s.size() && s.back() != '0';
}

void validate(const Node& node, Kind expected, int depth, bool soft);
std::string_view concreteTag(Kind kind) {
    switch (kind) {
    case Kind::caption: return "caption";
    case Kind::listItems: return "li";
    case Kind::rows: return "tr";
    case Kind::cells: return "td";
    case Kind::attached: return "attached";
    default: return {};
    }
}
void validateParts(const std::vector<Part>& parts, Kind kind, int depth, bool soft, bool expanded = false) {
    if (kind == Kind::none) { if (!parts.empty()) throw Error("unexpected content"); return; }
    if (kind == Kind::text || kind == Kind::number || kind == Kind::integer || kind == Kind::boolean) {
        const auto value = scalar(parts, expanded);
        if ((kind == Kind::number || kind == Kind::integer) && !validNumber(value, kind == Kind::integer)) throw Error("invalid number");
        if (kind == Kind::boolean && value != "t" && value != "f") throw Error("invalid boolean");
        return;
    }
    for (const auto& part : parts) {
        if (std::holds_alternative<std::string>(part)) {
            if (kind == Kind::inlineNodes || (expanded && newline(part))) continue;
            throw Error("unexpected text in node array");
        }
        const auto& child = std::get<std::shared_ptr<const Node>>(part);
        if (!child) throw Error("null node");
        validate(*child, kind, depth, soft);
    }
}
void validate(const Node& node, Kind expected, int depth, bool soft) {
    if (depth > maxDepth) throw Error("maximum depth exceeded");
    const auto tag = node.tag.empty() ? concreteTag(expected) : std::string_view(node.tag);
    if (!validName(tag)) throw Error("invalid tag");
    const Spec* spec = schema(tag);
    if (!spec) {
        if (soft && !node.tag.empty() && !node.raw.empty() && concreteTag(expected).empty()) {
            const auto parsed = Parser(node.raw).root();
            if (parsed.tag != node.tag) throw Error("unknown raw tag mismatch");
            return;
        }
        throw Error("unknown tag: " + node.tag);
    }
    if (expected == Kind::inlineNodes || expected == Kind::blockNodes) {
        if (spec->display != expected) throw Error("wrong display type: " + node.tag);
    } else if (expected == Kind::gallery) {
        if (node.tag != "img" && node.tag != "video") throw Error("gallery requires image or video");
    } else if (!concreteTag(expected).empty()) {
        if (tag != concreteTag(expected)) throw Error("wrong concrete element type");
    }
    std::set<std::string> seenFields;
    for (const auto& field : node.fields) {
        if (!validName(field.name) || !seenFields.insert(field.name).second) throw Error("invalid or duplicate field");
        const auto it = std::find_if(spec->fields.begin(), spec->fields.end(), [&](const FieldSpec& f) { return f.name == field.name; });
        if (it == spec->fields.end()) throw Error("unknown field: " + field.name);
        if (field.value.size() != 1 || !std::holds_alternative<std::shared_ptr<const Node>>(field.value.front())) throw Error("invalid field value");
        const auto& pointer = std::get<std::shared_ptr<const Node>>(field.value.front());
        if (!pointer) throw Error("null field value");
        const auto& wrapper = *pointer;
        if (!wrapper.tag.empty()) throw Error("field value must omit tag");
        if (it->kind == Kind::caption) validate(wrapper, Kind::caption, depth + 1, soft);
        else {
            if (!wrapper.fields.empty()) throw Error("unexpected nested fields");
            validateParts(wrapper.content, it->kind, depth + 1, soft);
        }
    }
    for (const auto& field : spec->fields) {
        if (field.required && std::none_of(node.fields.begin(), node.fields.end(), [&](const Field& f) { return f.name == field.name; })) throw Error("missing field: " + std::string(field.name));
    }
    validateParts(node.content, spec->content, depth + 1, soft, spec->layout == Layout::expanded || spec->layout == Layout::multiple);
    auto fieldValue = [&](std::string_view name) -> std::string {
        for (const auto& field : node.fields) if (field.name == name) return scalar(std::get<std::shared_ptr<const Node>>(field.value.front())->content);
        return {};
    };
    if (node.tag == "h") {
        const auto size = fieldValue("s");
        if (size.size() != 1 || size[0] < '1' || size[0] > '6') throw Error("header size must be 1..6");
    }
    if (node.tag == "td") {
        const auto align = fieldValue("align"), valign = fieldValue("valign");
        if (!align.empty() && align != "left" && align != "center" && align != "right") throw Error("invalid alignment");
        if (!valign.empty() && valign != "top" && valign != "middle" && valign != "bottom") throw Error("invalid vertical alignment");
    }
    if (node.tag == "li") {
        const auto type = fieldValue("type");
        if (!type.empty() && type != "a" && type != "A" && type != "i" && type != "I" && type != "1" && type != "checkbox") throw Error("invalid list type");
    }
}

std::string escape(std::string_view s) {
    std::string out;
    for (char c : s) { if (c == '#' || c == '{' || c == '}' || c == '\\') out += '\\'; out += c; }
    return out;
}
std::string writeNode(const Node& node, Kind expected);
std::string writeParts(const std::vector<Part>& parts, Kind kind, bool expanded) {
    std::string out;
    for (const auto& part : parts) {
        if (const auto* s = std::get_if<std::string>(&part)) {
            if (expanded && newline(part)) continue;
            out += escape(*s);
        } else {
            if (expanded) out += '\n';
            out += writeNode(**std::get_if<std::shared_ptr<const Node>>(&part), kind);
        }
    }
    return out;
}
std::string writeNode(const Node& node, Kind expected) {
    const auto tag = node.tag.empty() ? concreteTag(expected) : std::string_view(node.tag);
    const auto* spec = schema(tag);
    if (!spec) {
        std::string normalized;
        normalized.reserve(node.raw.size());
        for (size_t i = 0; i < node.raw.size(); ++i) {
            if (node.raw[i] == '\r' && i + 1 < node.raw.size() && node.raw[i + 1] == '\n') continue;
            normalized += node.raw[i];
        }
        return normalized;
    }
    const bool expanded = spec->layout == Layout::expanded || (spec->layout == Layout::multiple && std::count_if(node.content.begin(), node.content.end(), [](const Part& p) { return std::holds_alternative<std::shared_ptr<const Node>>(p); }) > 1);
    std::string out = "{";
    if (concreteTag(expected).empty()) out += std::string(tag) + ";";
    for (const auto& field : node.fields) {
        if (expanded) out += '\n';
        out += '#' + field.name + '{';
        const auto it = std::find_if(spec->fields.begin(), spec->fields.end(), [&](const FieldSpec& f) { return f.name == field.name; });
        const auto& wrapper = *std::get<std::shared_ptr<const Node>>(field.value.front());
        if (it->kind == Kind::caption) {
            auto encoded = writeNode(wrapper, Kind::caption);
            out += encoded.substr(1, encoded.size() - 2);
        } else out += writeParts(wrapper.content, it->kind, false);
        out += '}';
    }
    const bool expandedScalar = expanded && (spec->content == Kind::text || spec->content == Kind::number || spec->content == Kind::integer || spec->content == Kind::boolean);
    if (expandedScalar) {
        if (!node.content.empty()) out += '\n' + escape(scalar(node.content, true));
    } else out += writeParts(node.content, spec->content, expanded);
    if (expanded) out += '\n';
    return out + '}';
}

} // namespace

Node parse(std::string_view source, bool soft) {
    auto node = Parser(source).root();
    validate(node, Kind::none, 0, soft);
    return node;
}
std::string serialize(const Node& node, bool soft) {
    validate(node, Kind::none, 0, soft);
    auto result = writeNode(node, Kind::none);
    if (!validUtf8(result)) throw Error("invalid UTF-8");
    if (result.find('\r') != std::string::npos) throw Error("standalone CR is invalid");
    return result;
}

} // namespace tmark
