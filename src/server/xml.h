
#ifndef _XML_H_
#define _XML_H_

#include <stdexcept>
#include <libxml/tree.h>

class XMLText {
public:
    XMLText()
        : m_node(0)
    {
    }

    XMLText(const std::string &content)
        : m_node(xmlNewTextLen(str_to_xml(content), content.size()))
    {
    }

    XMLText(xmlNodePtr _node)
        : m_node(_node)
    {
    }

    ~XMLText()
    {
        if(m_node != 0) {
            xmlFreeNode(m_node);
        }
    }

    xmlNodePtr get() const {
        return m_node;
    }

    xmlNodePtr release() {
        xmlNodePtr result = m_node;
        m_node = 0;
        return result;
    }

private:
    static const xmlChar *str_to_xml(const std::string &str) {
        return reinterpret_cast<const xmlChar*>(str.c_str());
    }

private:
    xmlNodePtr m_node;
};

class XMLNode {
public:
    XMLNode()
        : m_node(0)
    {
    }

    XMLNode(const std::string &name)
        : m_node(xmlNewNode(NULL, str_to_xml(name)))
    {
    }

    XMLNode(xmlNodePtr _node)
        : m_node(_node)
    {
    }

    ~XMLNode()
    {
        if(m_node != 0) {
            xmlFreeNode(m_node);
        }
    }

    void add(xmlNodePtr node)
    {
        xmlAddChild(m_node, node);
    }

    void add_text(const std::string &content)
    {
        XMLText text(content);
        this->add(text.get());
        text.release();
    }

    void set_attr(const std::string &name, const std::string &value)
    {
        xmlNewProp(m_node, str_to_xml(name), str_to_xml(value));
    }

    void set_attr(const std::string &name, int value)
    {
        std::ostringstream os;
        os << value;
        this->set_attr(name, os.str());
    }

    xmlNodePtr get() const {
        return m_node;
    }

    xmlNodePtr release() {
        xmlNodePtr result = m_node;
        m_node = 0;
        return result;
    }

private:
    static const xmlChar *str_to_xml(const std::string &str) {
        return reinterpret_cast<const xmlChar*>(str.c_str());
    }

private:
    xmlNodePtr m_node;
};

class XMLDoc {
public:
    XMLDoc(const std::string &rootname, const std::string &version = "1.0")
        : m_doc(xmlNewDoc(str_to_xml(version)))
    {
        XMLNode root(xmlNewNode(NULL, str_to_xml(rootname)));

        m_root = root.get();

        xmlDocSetRootElement(m_doc, m_root);

        root.release();
    }

    XMLDoc(const std::string &rootname, const std::string &rootns, const std::string &version = "1.0")
        : m_doc(xmlNewDoc(str_to_xml(version)))
    {
        XMLNode root(xmlNewNode(NULL, str_to_xml(rootname)));

        xmlNewNs(root.get(), str_to_xml(rootns), NULL);

        m_root = root.get();

        xmlDocSetRootElement(m_doc, m_root);

        root.release();
    }

    void add(xmlNodePtr node)
    {
        xmlAddChild(m_root, node);
    }

    void add_pi(const std::string &name, const std::string &content)
    {
        XMLNode PI(xmlNewDocPI(m_doc, str_to_xml(name), str_to_xml(content)));

        xmlAddPrevSibling(m_root, PI.get());

        PI.release();
    }

    ~XMLDoc() {
        xmlFreeDoc(m_doc);
    }

private:
    static const xmlChar *str_to_xml(const std::string &str) {
        return reinterpret_cast<const xmlChar*>(str.c_str());
    }

private:
    friend std::ostream &operator<<(std::ostream&, const XMLDoc&);
    xmlNodePtr m_root;
    xmlDocPtr m_doc;
};

inline std::ostream &operator<<(std::ostream &o, const XMLDoc &doc)
{
    xmlChar *mem; 
    int size;

    xmlDocDumpMemory(doc.m_doc, &mem, &size);

    if(mem == NULL) {
        throw std::runtime_error("cannot serialize document");
    }

    o.write(reinterpret_cast<const char*>(mem), size);

    xmlFree(mem);

    return o;
}

#endif
