/*@ Docs Online - A Google Docs Backup Applicaiotn                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/* Written and Designed by Michael Haephrati                                */
/* COPYRIGHT ©2008 by Michael Haephrati    haephrati@gmail.com              */
/* All rights reserved.                                                     */
/* -------------------------------------------------------------------------*/
#include "stdafx.h"
#include "myxml.h"
#include "Utils.h"

void myxml::MSXMLCall(HRESULT hr, const char *message)
{
  if (FAILED(hr))
    throw msxml_exception(message, hr);
}

void myxml::CreateDocInstance(CComPtr<IXMLDOMDocument> &xmlDoc)
{
  MSXMLCall( xmlDoc.CoCreateInstance(__uuidof(DOMDocument)), 
    "IXMLDOMDocument::CoCreateInstance" );
}

// parentNode can be NULL to use xmlDoc as root
void myxml::AppendNewNode(IXMLDOMNode **newNode, IXMLDOMDocument *xmlDoc, IXMLDOMNode *parentNode, const wchar_t *name)
{
  CComPtr<IXMLDOMNode> node;

  MSXMLCall( xmlDoc->createNode(CComVariant(NODE_ELEMENT), CComBSTR(name), NULL, &node),
    "IXMLDOMDocument::createNode" );
  if (parentNode == NULL)
  {
    MSXMLCall( xmlDoc->appendChild(node, newNode),
      "IXMLDOMDocument::appendChild" );
  }
  else
  {
    MSXMLCall( parentNode->appendChild(node, newNode),
      "IXMLDOMNode::appendChild" );
  }
}

void myxml::AppendTextElement(IXMLDOMDocument *xmlDoc, IXMLDOMNode *parentNode, const wchar_t *name, const wchar_t *text)
{
  CComPtr<IXMLDOMNode> node;

  myxml::AppendNewNode(&node, xmlDoc, parentNode, name);
  CComQIPtr<IXMLDOMElement> textElement = node;
  MSXMLCall( textElement->put_text(CComBSTR(text)),
    "IXMLDOMElement::put_text");
}

void myxml::SetElementAttribute(IXMLDOMElement *element, const wchar_t *name, const wchar_t *val)
{
  ASSERT(element != NULL);
  MSXMLCall( element->setAttribute(CComBSTR(name), CComVariant(val)),
    "IXMLDOMElement::setAttribute" );
}

BOOL myxml::LoadXml(IXMLDOMElement **rootElement, IXMLDOMDocument *xmlDoc, const wchar_t *xml)
{
  VARIANT_BOOL successful;
  MSXMLCall( xmlDoc->loadXML(CComBSTR(xml), &successful),
    "IXMLDOMDocument::loadXML" );
  if (successful)
  {
    MSXMLCall( xmlDoc->get_documentElement(rootElement),
      "IXMLDOMDocument::get_documentElement" );
  }
  else
    *rootElement = NULL;
  return successful;
}

wstring myxml::GetElementName(IXMLDOMElement *element)
{
  CComBSTR bstr;

  MSXMLCall( element->get_tagName(&bstr),
    "IXMLDOMElement::get_tagName" );
  return wstring(bstr);
}

BOOL myxml::SelectSubnode(IXMLDOMNode *node, const wchar_t *subnodeName, IXMLDOMNode **resultNode) 
{
  myxml::MSXMLCall( node->selectSingleNode(CComBSTR(subnodeName), resultNode),
    "IXMLDOMNode::selectSingleNode" );
  return (*resultNode != NULL);
}

BOOL myxml::GetSubnodeText(wstring &text, IXMLDOMNode *node, const wchar_t *subnodeName)
{
  CComPtr<IXMLDOMNode> subNode;
  CComBSTR buffer;
  BOOL result;

  ASSERT(node != NULL);
  if (SelectSubnode(node, subnodeName, &subNode))
  {
    MSXMLCall( subNode->get_text(&buffer),
      "IXMLDOMNode::get_text" );
    text = buffer;
    result = TRUE;
  }
  else
  {
    text.clear();
    result = FALSE;
  }
  return result;
}

void myxml::GetXml(BSTR *xmlString, IXMLDOMDocument *xmlDoc)
{
  MSXMLCall( xmlDoc->get_xml(xmlString),
    "IXMLDOMDocument::get_xml" );
}

//////////////////////////////////////////////////////////////////////////

const xml_element xml_element::m_emptyElement;
const wstring xml_element::m_emptyString;

void xml_element::MSXMLCall(HRESULT hr, const char *message)
{
  if (FAILED(hr))
    throw msxml_exception(message, hr);
}

void xml_element::assign(IXMLDOMElement *domElement)
{
  m_element = domElement;
  getNodeList();
}

wstring xml_element::name() const
{
  if (empty()) 
    return emptyString();
  CComBSTR bstr;
  MSXMLCall( m_element->get_tagName(&bstr),
    "IXMLDOMElement::get_tagName" );
  return wstring(bstr);
}

wstring xml_element::attr( const wchar_t *name ) const
{
  if (!empty())
  {
    CComBSTR bname(name);
    CComVariant val(VT_EMPTY);
    MSXMLCall( m_element->getAttribute(bname, &val),
      "IXMLDOMElement::getAttribute" );
    if (val.vt == VT_BSTR) 
      return val.bstrVal;
  }
  return emptyString();
}

wstring xml_element::val() const
{
  if (!empty())
  {
    CComVariant val(VT_EMPTY);
    MSXMLCall( m_element->get_nodeTypedValue(&val),
      "IXMLDOMElement::getAttribute" );
    if (val.vt == VT_BSTR) 
      return val.bstrVal;
  }
  return emptyString();
}

xml_element xml_element::subnode( const wchar_t *name ) const
{
  if (!empty()) 
  {
    CComQIPtr<IXMLDOMNode> n = m_element;
    CComPtr<IXMLDOMNode> resultNode;
    myxml::MSXMLCall( n->selectSingleNode(CComBSTR(name), &resultNode),
      "IXMLDOMNode::selectSingleNode" );
    if (resultNode != NULL)
    {
      CComQIPtr<IXMLDOMElement> resultElement(resultNode);
      return xml_element(resultElement);
    }
  }
  return xml_element();
}

const xml_element xml_element::get_child( long pos ) const
{
  if (pos >= 0 && pos < m_nodeCount)
  {
    CComPtr<IXMLDOMNode> node;
    MSXMLCall( m_nodeList->get_item(pos, &node),
      "IXMLDOMNodeList::get_item" );
    if (node != NULL) 
    {
      DOMNodeType type; 
      MSXMLCall( node->get_nodeType(&type),
        "IXMLDOMNode::get_nodeType" );
      if (type == NODE_ELEMENT) 
      {
        CComQIPtr<IXMLDOMElement> e(node);
        return xml_element(e);
      }
    }
  }
  else
  {
    ASSERT(pos == m_nodeCount);
  }
  return m_emptyElement;
}

wstring xml_element::subval( const wchar_t *name ) const
{
  if (empty()) 
    return emptyString();
  xml_element c = subnode(name);
  return c.val();
}

void xml_element::getNodeList()
{
  m_nodeCount = 0;
  if (!empty())
  { 
    MSXMLCall( m_element->get_childNodes(&m_nodeList),
      "IXMLDOMElement::get_childNodes" );
    if (m_nodeList != NULL)
    {
      MSXMLCall( m_nodeList->get_length(&m_nodeCount),
        "IXMLDOMNodeList::get_childNodes" );
    }
  }
}

const xml_element &xml_element::emptyElement()
{
  static const xml_element element;
  return element;
}

xml_element::const_iterator xml_element::begin() const
{
  return const_iterator(this, 0, get_child(0));
}

xml_element::const_iterator xml_element::end() const
{
  return const_iterator(this, m_nodeCount, m_emptyElement);
}

//////////////////////////////////////////////////////////////////////////

xml_document::xml_document()
{
}

void xml_document::provideDoc()
{
  if (m_doc == NULL)
    myxml::CreateDocInstance(m_doc);
}

BOOL xml_document::load( const wchar_t *xml )
{
  CComPtr<IXMLDOMElement> rootElement;

  provideDoc();
  if (myxml::LoadXml(&rootElement, m_doc, xml))
  {
    m_root.assign(rootElement); // else remains empty
    return TRUE;
  }
  return FALSE;
}

BOOL xml_document::load(const string &multiByteXml)
{
  wstring unicodeXml;
  str::MB2Unicode(unicodeXml, multiByteXml.c_str(), multiByteXml.length(), CP_UTF8);
  return load(unicodeXml.c_str());
}
