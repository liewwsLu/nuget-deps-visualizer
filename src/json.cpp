#include "json.h"

#include <cctype>

namespace {

const int HEX_DIGITS = 4;
const int HEX_BASE = 16;
const unsigned ONE_BYTE_LIMIT = 0x80;
const unsigned TWO_BYTE_LIMIT = 0x800;

/// Разбор JSON: позиция в тексте и функции для каждого вида значения.
class JsonParser {
public:
    explicit JsonParser(const std::string& text) : text_(text) {}

    JsonValue parseDocument() {
        JsonValue value = parseValue();
        skipSpaces();
        if (pos_ != text_.size()) {
            fail("unexpected text after JSON value");
        }
        return value;
    }

private:
    const std::string& text_;
    std::size_t pos_ = 0;

    [[noreturn]] void fail(const std::string& message) const {
        throw JsonError("JSON error at position " + std::to_string(pos_) + ": " + message);
    }

    void skipSpaces() {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }

    char peek() {
        skipSpaces();
        if (pos_ >= text_.size()) {
            fail("unexpected end of text");
        }
        return text_[pos_];
    }

    void expect(char c) {
        if (peek() != c) {
            fail(std::string("expected '") + c + "'");
        }
        ++pos_;
    }

    bool consumeWord(const std::string& word) {
        if (text_.compare(pos_, word.size(), word) == 0) {
            pos_ += word.size();
            return true;
        }
        return false;
    }

    JsonValue parseValue() {
        char c = peek();
        if (c == '{') {
            return parseObject();
        }
        if (c == '[') {
            return parseArray();
        }
        JsonValue value;
        if (c == '"') {
            value.type = JsonValue::Type::String;
            value.text = parseString();
        } else if (consumeWord("true")) {
            value.type = JsonValue::Type::Bool;
            value.boolean = true;
        } else if (consumeWord("false")) {
            value.type = JsonValue::Type::Bool;
        } else if (consumeWord("null")) {
            value.type = JsonValue::Type::Null;
        } else {
            value.type = JsonValue::Type::Number;
            value.text = parseNumber();
        }
        return value;
    }

    JsonValue parseObject() {
        JsonValue value;
        value.type = JsonValue::Type::Object;
        expect('{');
        if (peek() == '}') {
            ++pos_;
            return value;
        }
        while (true) {
            if (peek() != '"') {
                fail("expected field name");
            }
            std::string key = parseString();
            expect(':');
            value.members.emplace_back(key, parseValue());
            if (peek() == '}') {
                ++pos_;
                return value;
            }
            expect(',');
        }
    }

    JsonValue parseArray() {
        JsonValue value;
        value.type = JsonValue::Type::Array;
        expect('[');
        if (peek() == ']') {
            ++pos_;
            return value;
        }
        while (true) {
            value.items.push_back(parseValue());
            if (peek() == ']') {
                ++pos_;
                return value;
            }
            expect(',');
        }
    }

    std::string parseNumber() {
        std::size_t start = pos_;
        while (pos_ < text_.size() && (std::isdigit(static_cast<unsigned char>(text_[pos_])) ||
                                       std::string("+-.eE").find(text_[pos_]) != std::string::npos)) {
            ++pos_;
        }
        if (pos_ == start) {
            fail("unexpected character");
        }
        return text_.substr(start, pos_ - start);
    }

    /// Добавить символ Юникода в строку в кодировке UTF-8.
    static void appendUtf8(std::string& out, unsigned code) {
        if (code < ONE_BYTE_LIMIT) {
            out += static_cast<char>(code);
        } else if (code < TWO_BYTE_LIMIT) {
            out += static_cast<char>(0xC0 | (code >> 6));
            out += static_cast<char>(0x80 | (code & 0x3F));
        } else {
            out += static_cast<char>(0xE0 | (code >> 12));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (code & 0x3F));
        }
    }

    unsigned parseHex() {
        if (pos_ + HEX_DIGITS > text_.size()) {
            fail("bad \\u escape");
        }
        std::string digits = text_.substr(pos_, HEX_DIGITS);
        for (char c : digits) {
            if (!std::isxdigit(static_cast<unsigned char>(c))) {
                fail("bad \\u escape");
            }
        }
        pos_ += HEX_DIGITS;
        return static_cast<unsigned>(std::stoul(digits, nullptr, HEX_BASE));
    }

    char escaped(char c) {
        switch (c) {
            case 'n': return '\n';
            case 't': return '\t';
            case 'r': return '\r';
            case 'b': return '\b';
            case 'f': return '\f';
            case '"': case '\\': case '/': return c;
            default: fail("bad escape sequence");
        }
    }

    std::string parseString() {
        expect('"');
        std::string result;
        while (pos_ < text_.size() && text_[pos_] != '"') {
            char c = text_[pos_++];
            if (c != '\\') {
                result += c;
            } else if (pos_ < text_.size() && text_[pos_] == 'u') {
                ++pos_;
                appendUtf8(result, parseHex());
            } else if (pos_ < text_.size()) {
                result += escaped(text_[pos_++]);
            }
        }
        if (pos_ >= text_.size()) {
            fail("unterminated string");
        }
        ++pos_;
        return result;
    }
};

}  // namespace

const JsonValue* JsonValue::find(const std::string& key) const {
    for (const auto& member : members) {
        if (member.first == key) {
            return &member.second;
        }
    }
    return nullptr;
}

JsonValue parseJson(const std::string& text) {
    return JsonParser(text).parseDocument();
}
