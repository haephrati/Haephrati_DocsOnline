#pragma once

class msxml_exception: public exception
{
public:
  msxml_exception(const char *message, HRESULT hr): exception(message) { m_hr = hr; }
  HRESULT get_hr() { return m_hr; }
private:
  HRESULT m_hr;
};


class xml_element
{ 
public:
  class const_iterator;
  static const long npos = (long)(-1);

  xml_element() : m_element(NULL), m_nodeList(NULL), m_nodeCount(0) {}
  xml_element(const xml_element &other) : m_element(other.m_element), m_nodeList(other.m_nodeList), m_nodeCount(other.m_nodeCount) {}
  void assign(IXMLDOMElement *domElement);
  void operator=(const xml_element &other)
  {
    m_element = other.m_element;
    m_nodeList = other.m_nodeList; 
    m_nodeCount = other.m_nodeCount;
  }
  // the rest is const
  BOOL empty() const { return m_element == NULL; }
  wstring name() const;
  wstring attr(const wchar_t *name) const;
  wstring val() const;
  xml_element subnode(const wchar_t *name) const;
  wstring subval(const wchar_t *name) const;
  // iterators over sub nodes
  const_iterator begin() const;
  const_iterator end() const;

private:
  static const wstring m_emptyString;
  static const xml_element m_emptyElement;
  CComPtr<IXMLDOMElement> m_element;
  CComPtr<IXMLDOMNodeList> m_nodeList; 
  long m_nodeCount;
  //const_iterator m_endIterator;

  static const xml_element &emptyElement();
  static void MSXMLCall(HRESULT hr, const char *message);
  static const wstring &emptyString() { return m_emptyString; }
  xml_element(IXMLDOMElement *element) : m_element(element)
  { 
    getNodeList(); 
  }
  void getNodeList();
  const xml_element get_child(long pos) const; // for const_iterator
};

class xml_element::const_iterator
{
  friend class xml_element; // give access to the private constructor
public:
  // iterate over entries with "name", iterate over all entries if name is NULL
  const_iterator(const wchar_t *name = NULL): m_name(name), m_node(NULL), m_pos(npos), m_subNode() { }
  void operator=(const const_iterator &other)
  {
    m_node = other.m_node;
    m_pos = other.m_pos;
    m_subNode = other.m_subNode;
  }
  bool operator!=(const const_iterator &other) const { return m_pos != other.m_pos; }
  const_iterator operator++(int)
  {
    if (m_pos != npos)
    {
      m_pos++;
      m_subNode = m_node->get_child(m_pos);
    }
    return *this;
  }
  const xml_element *operator->() const { return &m_subNode; }

private:
  const wchar_t *m_name;
  const xml_element *m_node;
  long m_pos;
  xml_element m_subNode;

  const_iterator(const xml_element *node, long pos, const xml_element &subNode): m_node(node), m_pos(pos), m_subNode(subNode) { }
};

class xml_document
{
public:
  xml_document();
  xml_element &root() { return m_root; }
  BOOL load(const wchar_t *xml);
  BOOL load(const string &multiByteXml);

private:
  CComPtr<IXMLDOMDocument> m_doc;
  xml_element m_root;

  void provideDoc();
};

namespace myxml
{
  void MSXMLCall(HRESULT hr, const char *message = "");
  void CreateDocInstance(CComPtr<IXMLDOMDocument> &xmlDoc);
  void AppendNewNode(IXMLDOMNode **newNode, IXMLDOMDocument *xmlDoc, IXMLDOMNode *parentNode, const wchar_t *name);
  void AppendTextElement(IXMLDOMDocument *xmlDoc, IXMLDOMNode *parentNode, const wchar_t *name, const wchar_t *text);
  void SetElementAttribute(IXMLDOMElement *element, const wchar_t *name, const wchar_t *val);

  BOOL LoadXml(IXMLDOMElement **rootElement, IXMLDOMDocument *xmlDoc, const wchar_t *xml);
  BOOL SelectSubnode(IXMLDOMNode *node, const wchar_t *subnodeName, IXMLDOMNode **resultNode);
  BOOL GetSubnodeText(wstring &text, IXMLDOMNode *node, const wchar_t *subnodeName);
  void GetXml(BSTR *xmlString, IXMLDOMDocument *xmlDoc);
  wstring GetElementName(IXMLDOMElement *element);
}

