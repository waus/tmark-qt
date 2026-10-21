#include "tmark/codec.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

int main() {
    const std::filesystem::path fixtures = TMARK_FIXTURES;
    int failures = 0;
    if (!std::filesystem::exists(fixtures)) {
        std::cerr << "Missing test fixtures: " << fixtures << '\n';
        return 1;
    }
    for (const auto& entry : std::filesystem::directory_iterator(fixtures / "valid")) {
        if (entry.path().extension() != ".tmark") continue;
        std::ifstream file(entry.path(), std::ios::binary);
        const std::string wire(std::istreambuf_iterator<char>{file}, {});
        try {
            const bool soft = entry.path().filename().string().find("unknown") != std::string::npos;
            auto parsed = tmark::parse(wire, soft);
            auto encoded = tmark::serialize(parsed, soft);
            auto reparsed = tmark::parse(encoded, soft);
            if (auto again = tmark::serialize(reparsed, soft); again != encoded) {
                std::cerr << "first: " << encoded.substr(0, 350) << "\nsecond: " << again.substr(0, 350) << '\n';
                throw tmark::Error("round trip is not stable");
            }
        } catch (const std::exception& error) {
            std::cerr << entry.path().filename() << ": " << error.what() << '\n';
            ++failures;
        }
    }
    for (const auto& entry : std::filesystem::directory_iterator(fixtures / "invalid")) {
        std::ifstream file(entry.path(), std::ios::binary);
        const std::string wire(std::istreambuf_iterator<char>{file}, {});
        try {
            (void)tmark::parse(wire);
            std::cerr << entry.path().filename() << ": accepted invalid document\n";
            ++failures;
        } catch (const tmark::Error&) {
        }
    }
    const std::string semicolon = "{document;#url{abc;foo}#title{Semicolon}{p;Hi}}";
    try {
        auto parsed = tmark::parse(semicolon);
        if (tmark::serialize(parsed) != "{document;\n#url{abc;foo}\n#title{Semicolon}\n{p;Hi}\n}") ++failures;
        (void)tmark::parse("{document;\r\n#url{x}\r\n#title{T}\r\n}");
    } catch (const tmark::Error& error) {
        std::cerr << "scalar/CRLF: " << error.what() << '\n';
        ++failures;
    }
    for (const std::string bad : {"\xef\xbb\xbf{p;x}", "{p;\xff}", "{p;x\ry}", "{h;#s{7}bad}"}) {
        try { (void)tmark::parse(bad); ++failures; }
        catch (const tmark::Error&) {}
    }
    return failures ? 1 : 0;
}
