#include "qtgumbonode.h"

namespace {
const char* const ID_ATTRIBUTE 		= u8"id";
const char* const CLASS_ATTRIBUTE 	= u8"class";
}

QtGumboNode::QtGumboNode() : m_node(nullptr)
{
}

QtGumboNode::QtGumboNode(GumboNode *node) : m_node(node)
{
#ifdef QT_GUMBO_DEBUG
    m_type = getType();

    // FIXME: infinite loop
    //m_path = getPath();

    m_parent = getParent();
    m_parentIndex = getParentIndex();

    if (isElement())
    {
        //m_children = getChildren(false);
        m_tagName = getTagName();
        m_html = getHtml();
        m_idAttr = hasIdAttribute() ? getIdAttribute() : "";
        m_classAttr = hasClassAttribute() ? getClassAttribute() : "";
    }
#endif
}

bool QtGumboNode::isValid() const
{
    return (m_node != nullptr);
}

bool QtGumboNode::isWhitespace() const
{
    return (getType() == Type::Whitespace);
}

bool QtGumboNode::isElement() const
{
    Q_ASSERT(isValid()); if (!isValid()) return false;

    return (getType() == Type::Element);
}

bool QtGumboNode::isText() const
{
    Q_ASSERT(isValid()); if (!isValid()) return false;

    return (getType() == Type::Text);
}

bool QtGumboNode::isDocument() const
{
    Q_ASSERT(isValid()); if (!isValid()) return false;

    return (getType() == Type::Document);
}

QtGumboNode::Type QtGumboNode::getType() const
{
    Q_ASSERT(isValid()); if (!isValid()) return Type::Invalid;

    Type result = Type::Invalid;
    switch (m_node->type)
    {
    case GUMBO_NODE_DOCUMENT:       // v will be a GumboDocument
        result = Type::Document;
        break;
    case GUMBO_NODE_ELEMENT:        // v will be a GumboElement
        result = Type::Element;
        break;
    case GUMBO_NODE_TEXT:           // v will be a GumboText
        result = Type::Text;
        break;
    case GUMBO_NODE_CDATA:          // v will be a GumboText.
        result = Type::CDATA;
        break;
    case GUMBO_NODE_COMMENT:        // v will be a GumboText, excluding comment delimiters
        result = Type::Comment;
        break;
    case GUMBO_NODE_WHITESPACE:
        result = Type::Whitespace;  // v will be a GumboText
        break;
    case GUMBO_NODE_TEMPLATE:       // v will be a GumboElement
        result = Type::Template;
        break;
    default:
        Q_ASSERT_X(0, Q_FUNC_INFO, "Invalid node type");
        break;
    }
    return result;
}

QtGumboNodePtr QtGumboNode::getParent() const
{
    Q_ASSERT(isValid()); if (!isValid()) return QtGumboNodePtr();

    if (!m_node->parent)
        return QtGumboNodePtr();

    return QtGumboNodePtr(new QtGumboNode(m_node->parent));
    /*QtGumboNodePtr result(new QtGumboNode());
    result->m_node = m_node->parent;
    return result;*/
}

size_t QtGumboNode::getParentIndex() const
{
    Q_ASSERT(isValid()); if (!isValid()) return std::numeric_limits<size_t>::infinity();

    return m_node->index_within_parent;
}

QtGumboNode::NodePath QtGumboNode::getPath() const
{
    Q_ASSERT(isValid()); if (!isValid()) return NodePath();

    // FIXME: test
    if (!isElement())
        return NodePath();

    NodePath result;
    result << qMakePair(getTagName(), getParentIndex());

    auto parentNode = getParent();
    while (parentNode && parentNode->isValid() && !parentNode->isDocument())
    {
        if (parentNode->isElement())
            result << qMakePair(parentNode->getTagName(), parentNode->getParentIndex());

        parentNode = parentNode->getParent();
    }

    std::reverse(result.begin(), result.end());
    return result;
}

HtmlTag QtGumboNode::getTag() const
{
    if (isElement())
        return HtmlTag(m_node->v.element.tag);

    return HtmlTag::UNKNOWN;
}

QString QtGumboNode::getTagName() const
{
    Q_ASSERT(isElement()); if (!isElement()) return QString();

    return QString(gumbo_normalized_tagname(m_node->v.element.tag));
}

bool QtGumboNode::hasAttribute(QString name) const
{
    Q_ASSERT(isElement()); if(!isElement()) return false;
    Q_ASSERT(!name.isEmpty()); if (name.isEmpty()) return false;

    GumboAttribute *attr = gumbo_get_attribute(&m_node->v.element.attributes, name.toUtf8().constData());
    return (attr != nullptr);
}

QString QtGumboNode::getAttribute(QString name) const
{
    Q_ASSERT(isElement()); if(!isElement()) return QString();
    Q_ASSERT(!name.isEmpty()); if (name.isEmpty()) return QString();

    QString result;
    GumboAttribute *attr = gumbo_get_attribute(&m_node->v.element.attributes, name.toUtf8().constData());
    if (attr)
        result = QString::fromUtf8(attr->value);
    return result;
}

bool QtGumboNode::hasIdAttribute() const
{
    return hasAttribute(ID_ATTRIBUTE);
}

bool QtGumboNode::hasClassAttribute() const
{
    return hasAttribute(CLASS_ATTRIBUTE);
}

QString QtGumboNode::getIdAttribute() const
{
    return getAttribute(ID_ATTRIBUTE);
}

QString QtGumboNode::getClassAttribute() const
{
    return getAttribute(CLASS_ATTRIBUTE);
}

size_t QtGumboNode::getAttributeCount() const
{
    Q_ASSERT(isElement()); if (!isElement()) return 0;

    return m_node->v.element.attributes.length;
}

QtGumboNodes QtGumboNode::getChildren(bool elementsOnly) const
{
    Q_ASSERT(isElement()); if (!isElement()) return QtGumboNodes();

    QtGumboNodes result;
    GumboVector* children = &m_node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i)
    {
        GumboNode *childNode = static_cast<GumboNode*>(children->data[i]);
        if (elementsOnly && (childNode->type != GUMBO_NODE_ELEMENT))
            continue;

        result << QtGumboNodePtr(new QtGumboNode(childNode));
    }
    return result;
}

int QtGumboNode::getChildElementCount(bool elementsOnly) const
{
    return getChildren(elementsOnly).size();
}

int QtGumboNode::getTextChildrenCount() const
{
    Q_ASSERT(isElement()); if (!isElement()) return 0;

    int textNodeCount = 0;
    GumboVector *nodeChildren = &m_node->v.element.children;
    for (unsigned int i = 0; i < nodeChildren->length; ++i)
    {
        GumboNode *childNode = static_cast<GumboNode*>(nodeChildren->data[i]);
        if (childNode->type == GUMBO_NODE_TEXT)
            textNodeCount++;
    }
    return textNodeCount;
}

QString QtGumboNode::getInnerText() const
{
    Q_ASSERT(isText()); if (!isText()) return QString();

    return QString::fromUtf8(m_node->v.text.text);
}

QString QtGumboNode::getChildrenInnerText()
{
    Q_ASSERT(isElement()); if (!isElement()) return QString();

    QString value;
    GumboVector *nodeChildren = &m_node->v.element.children;
    for (unsigned int i = 0; i < nodeChildren->length; ++i)
    {
        GumboNode *childNode = static_cast<GumboNode*>(nodeChildren->data[i]);
        if (childNode->type != GUMBO_NODE_TEXT)
            continue;

        value += QString::fromUtf8(childNode->v.text.text);
    }
    return value;
}

QtGumboNodePtr QtGumboNode::getElementByTag(std::pair< HtmlTag, int > tagDesc, int *foundPos)
{
    Q_ASSERT(isValid()); if (!isValid()) return QtGumboNodePtr();
    Q_ASSERT(tagDesc.second >= 0); if (tagDesc.second < 0) return QtGumboNodePtr();

    QtGumboNodes children = getChildren();
    // NOTE: not all chilren are elements, so the assert below can fail:
//  Q_ASSERT(startPos < children.size()); if (startPos >= children.size()) return QtGumboNode();
    int elementIndex = 0;
    for (auto iChild = children.begin(); iChild != children.end(); ++iChild)
    {
        if (((*iChild)->getTag() == tagDesc.first) && (elementIndex >= tagDesc.second))
        {
            if (foundPos) *foundPos = elementIndex;
            return *iChild;
        }
        else elementIndex++;
    }
    return QtGumboNodePtr();
}

QtGumboNodePtr QtGumboNode::getElementByTag(std::initializer_list< std::pair< HtmlTag, int> > tagDescsInitList, int *foundPos)
{
    Q_ASSERT(isElement()); if (!isElement()) return QtGumboNodePtr();
    Q_ASSERT(tagDescsInitList.size() > 0); if (!tagDescsInitList.size()) return QtGumboNodePtr();

    if (foundPos) *foundPos = 0;

    QtGumboNodePtr node(new QtGumboNode(m_node));
    for (auto iItem : tagDescsInitList)
    {
//        Q_ASSERT(iItem->first != HtmlTag::UNKNOWN);
        Q_ASSERT(iItem.second >= 0);

        QtGumboNodes nodeChildren = node->getChildren(false);
        if (iItem.second >= nodeChildren.size())
        {            
            node = QtGumboNodePtr();
            break;
        }

        if (foundPos) foundPos++;
        node = nodeChildren[iItem.second];
        Q_ASSERT(node->isElement()); if (!node->isElement()) return QtGumboNodePtr();

        if (node->getTag() != iItem.first)
        {
            node = QtGumboNodePtr();
            break;
        }
    }

    return node;
}

QtGumboNodePtr QtGumboNode::getElementByClass(QString className, HtmlTag childTag) const
{
    Q_ASSERT(isValid()); if (!isValid()) return QtGumboNodePtr();
    Q_ASSERT(!className.isEmpty()); if (className.isEmpty()) return QtGumboNodePtr();

    QtGumboNodes children = getChildren();
    for (QtGumboNodes::iterator iChild = children.begin(); iChild != children.end(); ++iChild)
    {
        if (((*iChild)->getTag() == childTag) && (QString::compare((*iChild)->getClassAttribute(), className, Qt::CaseInsensitive) == 0))
        {
            return *iChild;
        }
    }
    return QtGumboNodePtr();
}

QtGumboNodes QtGumboNode::getElementsByClass(QString className, HtmlTag childTag) const
{
    Q_ASSERT(isValid()); if(!isValid()) return QtGumboNodes();
    Q_ASSERT(!className.isEmpty()); if (className.isEmpty()) return QtGumboNodes();

    QtGumboNodes result;

    QtGumboNodes children = getChildren();
    for (QtGumboNodes::iterator iChild = children.begin(); iChild != children.end(); ++iChild)
    {
        if (((*iChild)->getTag() == childTag) && (QString::compare((*iChild)->getClassAttribute(), className, Qt::CaseInsensitive) == 0))
        {
            result << (*iChild);
        }
    }
    return result;
}

QtGumboNodes QtGumboNode::getElementsByClassRecursive(QString className, HtmlTag childTag) const
{
    Q_ASSERT(isValid()); if (!isValid()) return QtGumboNodes();
    Q_ASSERT(!className.isEmpty()); if (className.isEmpty()) return QtGumboNodes();

    QtGumboNodes result;

    QtGumboNodes children = getChildren();
    for (QtGumboNodes::iterator iChild = children.begin(); iChild != children.end(); ++iChild)
    {
        if (((*iChild)->getTag() == childTag) && (QString::compare((*iChild)->getClassAttribute(), className, Qt::CaseInsensitive) == 0))
        {
            result << *iChild;
        }

        QtGumboNodes childResult = (*iChild)->getElementsByClassRecursive(className, childTag);
        if (!childResult.empty())
            result << childResult;
    }

    return result;
}

size_t QtGumboNode::getTagLength() const
{
    Q_ASSERT(isElement()); if (!isElement()) return 0;

    return m_node->v.element.original_tag.length;
}

size_t QtGumboNode::getStartPos() const
{
    Q_ASSERT(isElement()); if (!isElement()) return 0;

    return m_node->v.element.start_pos.offset;
}

size_t QtGumboNode::getEndPos() const
{
    Q_ASSERT(isElement()); if (!isElement()) return 0;

    return m_node->v.element.end_pos.offset;
}

QString QtGumboNode::getHtml() const
{
    Q_ASSERT(isElement()); if (!isElement()) return QString();

    return QString::fromUtf8(m_node->v.element.original_tag.data);
}
