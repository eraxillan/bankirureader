/*
 * This file is part of Bitrix Forum Reader.
 *
 * Copyright (C) 2016-2020 Alexander Kamyshnikov <axill777@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/
#include "qtgumbodocument.h"

#include <common/logger.h>

#include <iostream>

bool QtGumboDocument::parse() {

	GumboOptions options = kGumboDefaultOptions;

	// Parse web page contents
	m_output = gumbo_parse_with_options(&options, m_rawHtmlData.constData(), m_rawHtmlData.length());
	Q_ASSERT(!!m_output);
	if (!m_output)
		return false;

	m_documentNode = QtGumboNodePool::globalInstance().getNode(m_output->document);
	m_rootNode = QtGumboNodePool::globalInstance().getNode(m_output->root);
	return true;
}

QtGumboDocument::QtGumboDocument() : m_output(nullptr)
{
}

QtGumboDocument::QtGumboDocument(const QString &rawData) {

	QTextCodec *htmlCodec = QTextCodec::codecForHtml(rawData.toLocal8Bit());

#ifdef BFR_PRINT_DEBUG_OUTPUT
	SystemLogger->info("HTML encoding/charset is '{}'", htmlCodec->name().toStdString());
#endif

	// Convert to UTF-8: Gumbo library understands only this encoding
#if defined(Q_OS_WIN)
	QString htmlFileString = htmlCodec->toUnicode(rawData.toLocal8Bit());
	m_rawHtmlData = htmlFileString.toUtf8();
#elif defined(Q_OS_UNIX) || defined(Q_OS_ANDROID)
	Q_UNUSED(htmlCodec);
	m_rawHtmlData = rawData.toUtf8();
#else
#error "Unsupported platform, needs testing"
#endif

	parse();
}

QtGumboDocument::~QtGumboDocument() {

	if (m_output)
		gumbo_destroy_output(&kGumboDefaultOptions, m_output);
}

QtGumboNodePtr QtGumboDocument::rootNode() const { return m_rootNode; }

QtGumboNodePtr QtGumboDocument::documentNode() const { return m_documentNode; }

// ---------------------------------------------------------------------------------------------------------------------------------------------------

static std::string nonbreaking_inline  = "|a|abbr|acronym|b|bdo|big|cite|code|dfn|em|font|i|img|kbd|nobr|s|small|span|strike|strong|sub|sup|tt|";
static std::string empty_tags          = "|area|base|basefont|bgsound|br|command|col|embed|event-source|frame|hr|image|img|input|keygen|link|menuitem|meta|param|source|spacer|track|wbr|";
static std::string preserve_whitespace = "|pre|textarea|script|style|";
static std::string special_handling    = "|html|body|";
static std::string no_entity_sub       = "|script|style|";
static std::string treat_like_inline   = "|p|";

static inline void rtrim(std::string &s) { s.erase(s.find_last_not_of(" \n\r\t") + 1); }

static inline void ltrim(std::string &s) { s.erase(0, s.find_first_not_of(" \n\r\t")); }

static void replace_all(std::string &s, const char *s1, const char *s2) {

	std::string t1(s1);
	size_t len = t1.length();
	size_t pos = s.find(t1);
	while (pos != std::string::npos) {
		s.replace(pos, len, s2);
		pos = s.find(t1, pos + len);
	}
}

static std::string substitute_xml_entities_into_text(const std::string &text) {

	std::string result = text;
	// replacing & must come first
	replace_all(result, "&", "&amp;");
	replace_all(result, "<", "&lt;");
	replace_all(result, ">", "&gt;");
	return result;
}

static std::string substitute_xml_entities_into_attributes(char quote, const std::string &text) {

	std::string result = substitute_xml_entities_into_text(text);
	if (quote == '"') {
		replace_all(result, "\"", "&quot;");
	} else if (quote == '\'') {
		replace_all(result, "'", "&apos;");
	}
	return result;
}

static std::string handle_unknown_tag(GumboStringPiece *text) {

	std::string tagname = "";
	if (text->data == NULL) {
		return tagname;
	}

	// work with copy GumboStringPiece to prevent asserts
	// if try to read same unknown tag name more than once
	GumboStringPiece gsp = *text;
	gumbo_tag_from_original_text(&gsp);
	tagname = std::string(gsp.data, gsp.length);

#ifdef BFR_PRINT_DEBUG_OUTPUT
	SystemLogger->info("UNKNOWN TAG: {}", tagname);
#endif
	return tagname;
}

static std::string get_tag_name(GumboNode *node) {

	std::string tagname;
	// work around lack of proper name for document node
	if (node->type == GUMBO_NODE_DOCUMENT) {
		tagname = "document";
	} else {
		tagname = gumbo_normalized_tagname(node->v.element.tag);
	}
	if (tagname.empty()) {
		tagname = handle_unknown_tag(&node->v.element.original_tag);
	}
	return tagname;
}

static std::string build_doctype(GumboNode *node) {

	std::string results = "";
	if (node->v.document.has_doctype) {
		results.append("<!DOCTYPE ");
		results.append(node->v.document.name);
		std::string pi(node->v.document.public_identifier);
		if ((node->v.document.public_identifier != NULL) && !pi.empty()) {
			results.append(" PUBLIC \"");
			results.append(node->v.document.public_identifier);
			results.append("\" \"");
			results.append(node->v.document.system_identifier);
			results.append("\"");
		}
		results.append(">\n");
	}
	return results;
}

static std::string build_attributes(GumboAttribute *at, bool no_entities) {

	std::string atts = "";
	atts.append(" ");
	atts.append(at->name);

	// how do we want to handle attributes with empty values
	// <input type="checkbox" checked />  or <input type="checkbox" checked="" />

	if ((!std::string(at->value).empty()) || (at->original_value.data[0] == '"')
		|| (at->original_value.data[0] == '\'')) {
		// determine original quote character used if it exists
		char quote = at->original_value.data[0];
		std::string qs = "";
		if (quote == '\'')
			qs = std::string("'");
		if (quote == '"')
			qs = std::string("\"");

		atts.append("=");

		atts.append(qs);

		if (no_entities) {
			atts.append(at->value);
		} else {
			atts.append(substitute_xml_entities_into_attributes(quote, std::string(at->value)));
		}

		atts.append(qs);
	}
	return atts;
}

// forward declaration
static std::string prettyprint(GumboNode*, int lvl, const std::string indent_chars);


// prettyprint children of a node
// may be invoked recursively
static std::string prettyprint_contents(GumboNode *node, int lvl, const std::string indent_chars) {

	std::string contents = "";
	std::string tagname = get_tag_name(node);
	std::string key = "|" + tagname + "|";
	bool no_entity_substitution = no_entity_sub.find(key) != std::string::npos;
	bool keep_whitespace = preserve_whitespace.find(key) != std::string::npos;
	bool is_inline = nonbreaking_inline.find(key) != std::string::npos;
	bool pp_okay = !is_inline && !keep_whitespace;

	GumboVector *children = &node->v.element.children;
	for (unsigned int i = 0; i < children->length; ++i) {
		GumboNode *child = static_cast<GumboNode *>(children->data[i]);

		if (child->type == GUMBO_NODE_TEXT) {
			std::string val;

			if (no_entity_substitution) {
				val = std::string(child->v.text.text);
			} else {
				val = substitute_xml_entities_into_text(std::string(child->v.text.text));
			}

			if (pp_okay)
				rtrim(val);

			if (pp_okay && (contents.length() == 0)) {
				// add required indentation
				char c = indent_chars.at(0);
				int n = indent_chars.length();
				contents.append(std::string((lvl - 1) * n, c));
			}

			contents.append(val);
		} else if ((child->type == GUMBO_NODE_ELEMENT) || (child->type == GUMBO_NODE_TEMPLATE)) {
			std::string val = prettyprint(child, lvl, indent_chars);

			// remove any indentation if this child is inline and not first child
			std::string childname = get_tag_name(child);
			std::string childkey = "|" + childname + "|";
			if ((nonbreaking_inline.find(childkey) != std::string::npos) && (contents.length() > 0)) {
				ltrim(val);
			}

			contents.append(val);
		} else if (child->type == GUMBO_NODE_WHITESPACE) {
			if (keep_whitespace || is_inline) {
				contents.append(std::string(child->v.text.text));
			}
		} else if (child->type != GUMBO_NODE_COMMENT) {
			// Does this actually exist: (child->type == GUMBO_NODE_CDATA)
			fprintf(stderr, "unknown element of type: %d\n", child->type);
		}
	}

	return contents;
}

// prettyprint a GumboNode back to html/xhtml
// may be invoked recursively

static std::string prettyprint(GumboNode *node, int lvl, const std::string indent_chars) {

	// special case the document node
	if (node->type == GUMBO_NODE_DOCUMENT) {
		std::string results = build_doctype(node);
		results.append(prettyprint_contents(node, lvl + 1, indent_chars));
		return results;
	}

	std::string close = "";
	std::string closeTag = "";
	std::string atts = "";
	std::string tagname = get_tag_name(node);
	std::string key = "|" + tagname + "|";
	bool need_special_handling = special_handling.find(key) != std::string::npos;
	bool is_empty_tag = empty_tags.find(key) != std::string::npos;
	bool no_entity_substitution = no_entity_sub.find(key) != std::string::npos;
	bool keep_whitespace = preserve_whitespace.find(key) != std::string::npos;
	bool is_inline = nonbreaking_inline.find(key) != std::string::npos;
	bool inline_like = treat_like_inline.find(key) != std::string::npos;
	bool pp_okay = !is_inline && !keep_whitespace;
	char c = indent_chars.at(0);
	int n = indent_chars.length();

	// build attr string
	const GumboVector *attribs = &node->v.element.attributes;
	for (unsigned int i = 0; i < attribs->length; ++i) {
		GumboAttribute *at = static_cast<GumboAttribute *>(attribs->data[i]);
		atts.append(build_attributes(at, no_entity_substitution));
	}

	// determine closing tag type
	if (is_empty_tag) {
		close = "/";
	} else {
		closeTag = "</" + tagname + ">";
	}

	std::string indent_space = std::string((lvl - 1) * n, c);

	// prettyprint your contents
	std::string contents = prettyprint_contents(node, lvl + 1, indent_chars);

	if (need_special_handling) {
		rtrim(contents);
		contents.append("\n");
	}

	char last_char = ' ';
	if (!contents.empty()) {
		last_char = contents.at(contents.length() - 1);
	}

	// build results
	std::string results;
	if (pp_okay) {
		results.append(indent_space);
	}
	results.append("<" + tagname + atts + close + ">");
	if (pp_okay && !inline_like) {
		results.append("\n");
	}
	if (inline_like) {
		ltrim(contents);
	}
	results.append(contents);
	if (pp_okay && !contents.empty() && (last_char != '\n') && (!inline_like)) {
		results.append("\n");
	}
	if (pp_okay && !inline_like && !closeTag.empty()) {
		results.append(indent_space);
	}
	results.append(closeTag);
	if (pp_okay && !closeTag.empty()) {
		results.append("\n");
	}

	return results;
}

void QtGumboDocument::prettify() const {

	std::string indent_chars = "  ";
	std::cout << prettyprint(m_output->document, 0, indent_chars) << std::endl;
}

QtGumboDocument &QtGumboDocument::operator=(QtGumboDocument other) {

	std::swap(m_rawHtmlData, other.m_rawHtmlData);
	std::swap(m_output, other.m_output);
	std::swap(m_documentNode, other.m_documentNode);
	std::swap(m_rootNode, other.m_rootNode);
	return *this;
}
