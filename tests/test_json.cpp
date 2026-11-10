// Тесты разбора JSON.

#include "json.h"
#include "test.h"

TEST(json_object_and_array) {
    JsonValue value = parseJson(R"({"versions": ["1.0.0", "2.0.0"], "count": 2})");
    CHECK(value.type == JsonValue::Type::Object);
    const JsonValue* versions = value.find("versions");
    CHECK(versions != nullptr);
    CHECK_EQ(versions->items.size(), std::size_t(2));
    CHECK_EQ(versions->items[1].text, std::string("2.0.0"));
    CHECK_EQ(value.find("count")->text, std::string("2"));
}

TEST(json_missing_key) {
    JsonValue value = parseJson(R"({"a": 1})");
    CHECK(value.find("b") == nullptr);
}

TEST(json_literals) {
    JsonValue value = parseJson(R"([true, false, null, -1.5e3])");
    CHECK(value.items[0].type == JsonValue::Type::Bool);
    CHECK_EQ(value.items[0].boolean, true);
    CHECK_EQ(value.items[1].boolean, false);
    CHECK(value.items[2].type == JsonValue::Type::Null);
    CHECK(value.items[3].type == JsonValue::Type::Number);
    CHECK_EQ(value.items[3].text, std::string("-1.5e3"));
}

TEST(json_string_escapes) {
    JsonValue value = parseJson(R"("a\"b\\c\/d\n\u0041\u0436")");
    CHECK_EQ(value.text, std::string("a\"b\\c/d\nA\xD0\xB6"));
}

TEST(json_nested_objects) {
    JsonValue value = parseJson(R"({"resources": [{"@id": "x", "@type": "T"}]})");
    const JsonValue& resource = value.find("resources")->items[0];
    CHECK_EQ(resource.find("@type")->text, std::string("T"));
}

TEST(json_errors) {
    CHECK_THROWS(parseJson(""), JsonError);
    CHECK_THROWS(parseJson("{\"a\": }"), JsonError);
    CHECK_THROWS(parseJson("[1, 2"), JsonError);
    CHECK_THROWS(parseJson("\"unterminated"), JsonError);
    CHECK_THROWS(parseJson("tru"), JsonError);
    CHECK_THROWS(parseJson("\"\\uZZZZ\""), JsonError);
    CHECK_THROWS(parseJson("<html></html>"), JsonError);
    CHECK_THROWS(parseJson("{} extra"), JsonError);
}
