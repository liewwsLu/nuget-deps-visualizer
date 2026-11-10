#include "xml.h"

#include <cctype>
#include <deque>

namespace {

/// Имя без префикса пространства имён: «nuspec:dependency» -> «dependency».
std::string localName(const std::string& name) {
    std::size_t colon = name.rfind(':');
    return colon == std::string::npos ? name : name.substr(colon + 1);
}

/// Заменить ссылки &amp; &lt; &gt; &quot; &apos; на символы.
std::string decodeEntities(const std::string& text) {
    static const std::pair<const char*, char> entities[] = {
        {"&amp;", '&'}, {"&lt;", '<'}, {"&gt;", '>'}, {"&quot;", '"'}, {"&apos;", '\''},
    };
    std::string result;
    for (std::size_t i = 0; i < text.size(); ++i) {
        bool replaced = false;
        for (const auto& entity : entities) {
            std::string code = entity.first;
            if (text.compare(i, code.size(), code) == 0) {
                result += entity.second;
                i += code.size() - 1;
                replaced = true;
                break;
            }
        }
        if (!replaced) {
            result += text[i];
        }
    }
    return result;
}

/// Рекурсивный разбор XML: элемент -> атрибуты -> содержимое -> вложенные элементы.
class XmlParser {
public:
    explicit XmlParser(const std::string& text) : text_(text) {}

    XmlElement parseDocument() {
        skipMisc();
        if (pos_ >= text_.size() || text_[pos_] != '<') {
            fail("root element not found");
        }
        XmlElement root = parseElement();
        skipMisc();
        if (pos_ != text_.size()) {
            fail("unexpected text after root element");
        }
        return root;
    }

private:
    const std::string& text_;
    std::size_t pos_ = 0;

    [[noreturn]] void fail(const std::string& message) const {
        throw XmlError("XML error at position " + std::to_string(pos_) + ": " + message);
    }

    bool startsWith(const std::string& prefix) const {
        return text_.compare(pos_, prefix.size(), prefix) == 0;
    }

    void skipSpaces() {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }

    /// Пропустить всё до end включительно.
    void skipPast(const std::string& end) {
        std::size_t found = text_.find(end, pos_);
        if (found == std::string::npos) {
            fail("'" + end + "' not found");
        }
        pos_ = found + end.size();
    }

    /// Пропустить пробелы, объявление <?xml?>, комментарии и <!DOCTYPE>.
    void skipMisc() {
        while (true) {
            skipSpaces();
            if (startsWith("<?")) {
                skipPast("?>");
            } else if (startsWith("<!--")) {
                skipPast("-->");
            } else if (startsWith("<!")) {
                skipPast(">");
            } else {
                return;
            }
        }
    }

    std::string parseName() {
        std::size_t start = pos_;
        while (pos_ < text_.size() && (std::isalnum(static_cast<unsigned char>(text_[pos_])) ||
                                       std::string(":_-.").find(text_[pos_]) != std::string::npos)) {
            ++pos_;
        }
        if (pos_ == start) {
            fail("expected a name");
        }
        return text_.substr(start, pos_ - start);
    }

    void parseAttribute(XmlElement& element) {
        std::string key = localName(parseName());
        skipSpaces();
        if (pos_ >= text_.size() || text_[pos_] != '=') {
            fail("expected '=' after attribute " + key);
        }
        ++pos_;
        skipSpaces();
        char quote = pos_ < text_.size() ? text_[pos_] : '\0';
        if (quote != '"' && quote != '\'') {
            fail("attribute value must be quoted");
        }
        std::size_t end = text_.find(quote, pos_ + 1);
        if (end == std::string::npos) {
            fail("unterminated attribute value");
        }
        element.attributes[key] = decodeEntities(text_.substr(pos_ + 1, end - pos_ - 1));
        pos_ = end + 1;
    }

    /// Разобрать открывающий тег; вернуть true, если тег пустой (<.../>).
    bool parseStartTag(XmlElement& element, std::string& fullName) {
        ++pos_;
        fullName = parseName();
        element.name = localName(fullName);
        while (true) {
            skipSpaces();
            if (startsWith("/>")) {
                pos_ += 2;
                return true;
            }
            if (startsWith(">")) {
                ++pos_;
                return false;
            }
            if (pos_ >= text_.size()) {
                fail("unterminated tag <" + fullName + ">");
            }
            parseAttribute(element);
        }
    }

    void parseClosingTag(const std::string& fullName) {
        pos_ += 2;
        std::string closing = parseName();
        if (closing != fullName) {
            fail("closing tag </" + closing + "> does not match <" + fullName + ">");
        }
        skipSpaces();
        if (!startsWith(">")) {
            fail("expected '>'");
        }
        ++pos_;
    }

    XmlElement parseElement() {
        XmlElement element;
        std::string fullName;
        if (parseStartTag(element, fullName)) {
            return element;
        }
        std::string text;
        while (true) {
            if (pos_ >= text_.size()) {
                fail("element <" + fullName + "> is not closed");
            }
            if (startsWith("</")) {
                parseClosingTag(fullName);
                element.text = decodeEntities(text);
                return element;
            }
            if (startsWith("<!--")) {
                skipPast("-->");
            } else if (startsWith("<![CDATA[")) {
                std::size_t start = pos_ + std::string("<![CDATA[").size();
                skipPast("]]>");
                text += text_.substr(start, pos_ - start - std::string("]]>").size());
            } else if (startsWith("<")) {
                element.children.push_back(parseElement());
            } else {
                text += text_[pos_++];
            }
        }
    }
};

}  // namespace

std::string XmlElement::attribute(const std::string& key, const std::string& fallback) const {
    auto found = attributes.find(key);
    return found == attributes.end() ? fallback : found->second;
}

std::vector<const XmlElement*> XmlElement::childrenNamed(const std::string& childName) const {
    std::vector<const XmlElement*> result;
    for (const XmlElement& child : children) {
        if (child.name == childName) {
            result.push_back(&child);
        }
    }
    return result;
}

const XmlElement* XmlElement::findDeep(const std::string& searchName) const {
    std::deque<const XmlElement*> queue = {this};
    while (!queue.empty()) {
        const XmlElement* element = queue.front();
        queue.pop_front();
        if (element->name == searchName) {
            return element;
        }
        for (const XmlElement& child : element->children) {
            queue.push_back(&child);
        }
    }
    return nullptr;
}

XmlElement parseXml(const std::string& text) {
    return XmlParser(text).parseDocument();
}
