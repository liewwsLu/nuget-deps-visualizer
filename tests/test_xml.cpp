// Тесты разбора XML.

#include "test.h"
#include "xml.h"

TEST(xml_elements_and_attributes) {
    XmlElement root = parseXml(R"(<?xml version="1.0"?>
<!-- comment -->
<package xmlns="urn:x"><metadata><id>Serilog</id><dependency id="A" version='1.0'/></metadata></package>)");
    CHECK_EQ(root.name, std::string("package"));
    const XmlElement* metadata = root.childrenNamed("metadata").front();
    CHECK_EQ(metadata->childrenNamed("id").front()->text, std::string("Serilog"));
    const XmlElement* dependency = metadata->childrenNamed("dependency").front();
    CHECK_EQ(dependency->attribute("id"), std::string("A"));
    CHECK_EQ(dependency->attribute("version"), std::string("1.0"));
    CHECK_EQ(dependency->attribute("missing", "none"), std::string("none"));
}

TEST(xml_namespace_prefix_removed) {
    XmlElement root = parseXml("<ns:package><ns:dependencies/></ns:package>");
    CHECK_EQ(root.name, std::string("package"));
    CHECK_EQ(root.children.front().name, std::string("dependencies"));
}

TEST(xml_entities_and_cdata) {
    XmlElement root = parseXml(R"(<a title="x &amp; y">1 &lt; 2<![CDATA[ <raw> ]]></a>)");
    CHECK_EQ(root.attribute("title"), std::string("x & y"));
    CHECK_EQ(root.text, std::string("1 < 2 <raw> "));
}

TEST(xml_find_deep) {
    XmlElement root = parseXml("<a><b><c id=\"1\"/></b><c id=\"2\"/></a>");
    CHECK_EQ(root.findDeep("c")->attribute("id"), std::string("2"));
    CHECK(root.findDeep("d") == nullptr);
}

TEST(xml_errors) {
    CHECK_THROWS(parseXml(""), XmlError);
    CHECK_THROWS(parseXml("<a><b></a>"), XmlError);
    CHECK_THROWS(parseXml("<a>"), XmlError);
    CHECK_THROWS(parseXml("<a b=c/>"), XmlError);
    CHECK_THROWS(parseXml("<a></a><b/>"), XmlError);
    CHECK_THROWS(parseXml("not xml"), XmlError);
}
