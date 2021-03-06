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
#include "forumthreadpool.h"

#include <website_backend/gumboparserimpl.h>

ForumThreadPool::ForumThreadPool(QObject *parent) : QObject(parent)
{
}

size_t ForumThreadPool::pageCountCacheSize() const {

	return static_cast<size_t>(m_threadPageCountCollection.size()) * (sizeof(ForumThreadUrlData) + sizeof(int));
}

namespace {
size_t postListSize(const bfr::PostList &posts) { return static_cast<size_t>(posts.size()) * (sizeof(bfr::PostPtr)); }
}

size_t ForumThreadPool::pagePostsCacheSize() const {

	size_t result = 0;
	const auto &allKeys = m_threadPagePostCollection.keys();
	for (const auto &urlKey : allKeys) {
		result += sizeof(ForumThreadUrlData);

		const auto &keys = m_threadPagePostCollection.value(urlKey).keys();
		for (const auto &pageNoKey : keys) {
			result += sizeof(int);
			result += postListSize(m_threadPagePostCollection.value(urlKey).value(pageNoKey));
		}
	}
	return result;
}

void ForumThreadPool::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
	
	emit downloadProgress(bytesReceived, bytesTotal);
}

ForumThreadPool &ForumThreadPool::globalInstance() {

	// Since it's a static variable, if the class has already been created, it won't be created again.
	// And it **is** thread-safe in C++11.
	static ForumThreadPool instance;
	return instance;
}

result_code::Type ForumThreadPool::getForumThreadPageCount(const ForumThreadUrlData &urlData, int &pageCount) {

	QScopedPointer<ForumThreadUrl> url(new ForumThreadUrl(urlData.m_sectionId, urlData.m_threadId));

	if (m_threadPageCountCollection.contains(urlData)) {
		pageCount = m_threadPageCountCollection.value(urlData);

		SystemLogger->debug(
			"Got forum thread '{}' page count ({}) from pagecount-cache", url->firstPageUrl(), pageCount);
		return result_code::Type::Ok;
	}

	// 1) Download the first forum web page
	SystemLogger->debug(
		"Forum thread '{}' was not parsed yet, no page count in the pagecount-cache", url->firstPageUrl());
	SystemLogger->debug("Downloading first page of forum thread '{}'...", url->firstPageUrl());
	QByteArray htmlRawData;
	BFR_RETURN_VALUE_IF(
		!FileDownloader::downloadUrl(url->firstPageUrl(), htmlRawData,
			std::bind(&ForumThreadPool::onDownloadProgress, this, std::placeholders::_1, std::placeholders::_2)),
		result_code::Type::NetworkError, "Unable to download first forum thread page");
	SystemLogger->debug("Forum thread '{}' first page has been downloaded", url->firstPageUrl());

	// 2) Parse the page HTML to get the page count
	bfr::ForumPageParser fpp;
	int pageCountTemp = -1;
	SystemLogger->debug("Parsing first page of forum thread '{}'...", url->firstPageUrl());
	result_code::Type result = fpp.getPageCount(htmlRawData, pageCountTemp);
	BFR_RETURN_VALUE_IF(result_code::failed(result), result, "Unable to parse first forum thread page");
	SystemLogger->debug("Forum thread '{}' first page has been parsed", url->firstPageUrl());

	// 4) Update cache
	pageCount = pageCountTemp;
	m_threadPageCountCollection.insert(urlData, pageCount);
	SystemLogger->debug(
		"Forum thread '{}' page count ({}) was added to pagecount-cache", url->firstPageUrl(), pageCount);
	SystemLogger->debug("New size of pagecount-cache: {} bytes", pageCountCacheSize());
	return result_code::Type::Ok;
}

result_code::Type ForumThreadPool::getForumPagePosts(const ForumThreadUrlData &urlData, const int pageNo, bfr::PostList &posts) {

	QScopedPointer<ForumThreadUrl> url(new ForumThreadUrl(urlData.m_sectionId, urlData.m_threadId));

	if (m_threadPagePostCollection.contains(urlData)) {
		if (m_threadPagePostCollection.value(urlData).contains(pageNo)) {
			posts = m_threadPagePostCollection.value(urlData).value(pageNo);

			SystemLogger->debug("Got forum thread '{}' page posts (size = {}) from pageposts-cache",
				url->pageUrl(pageNo), posts.size());
			return result_code::Type::Ok;
		}
	}

	// 1) Download the first forum web page
	SystemLogger->debug(
		"Forum thread '{}' was not parsed yet, no page posts in the pageposts-cache", url->pageUrl(pageNo));
	SystemLogger->debug("Downloading first page of forum thread '{}'...", url->pageUrl(pageNo));
	QByteArray htmlRawData;
	BFR_RETURN_VALUE_IF(
		!FileDownloader::downloadUrl(url->pageUrl(pageNo), htmlRawData,
			std::bind(&ForumThreadPool::onDownloadProgress, this, std::placeholders::_1, std::placeholders::_2)),
		result_code::Type::NetworkError, "Unable to download specified forum thread page");
	SystemLogger->debug("Forum thread '{}' specified page has been downloaded", url->pageUrl(pageNo));

	// 2) Parse the page HTML to get the page count
	bfr::ForumPageParser fpp;
	int pageCount = -1;
	SystemLogger->debug("Parsing specified page of forum thread '{}': page count...", url->pageUrl(pageNo));
	result_code::Type result = fpp.getPageCount(htmlRawData, pageCount);
	BFR_RETURN_VALUE_IF(result_code::failed(result), result, "Unable to parse specified forum thread page");
	SystemLogger->debug("Forum thread '{}' specified page has been parsed: page count", url->pageUrl(pageNo));

	// 3) Parse the page HTML to get the page user posts
	bfr::PostList postsTemp;
	SystemLogger->debug("Parsing specified page of forum thread '{}': page posts...", url->pageUrl(pageNo));
	result = fpp.getPagePosts(htmlRawData, postsTemp);
	BFR_RETURN_VALUE_IF(result_code::failed(result), result, "Unable to parse specified forum thread page");
	SystemLogger->debug("Forum thread '{}' specified page has been parsed: page posts", url->pageUrl(pageNo));

	// 4) Update cache
	m_threadPagePostCollection[urlData][pageNo] = postsTemp;
	posts.swap(postsTemp);
	SystemLogger->debug(
		"Forum thread '{}' page posts (count: {}) was added to pageposts-cache", url->pageUrl(pageNo), posts.size());
	SystemLogger->debug("New size of pageposts-cache: {} bytes", pagePostsCacheSize());
	return result_code::Type::Ok;
}

result_code::Type ForumThreadPool::getForumThreadPosts(const ForumThreadUrlData &urlData, bfr::PostList &posts) {

	QScopedPointer<ForumThreadUrl> url(new ForumThreadUrl(urlData.m_sectionId, urlData.m_threadId));

	// 1) Get thread page count
	int pageCount = -1;
	result_code::Type result = getForumThreadPageCount(urlData, pageCount);
	BFR_RETURN_VALUE_IF(result_code::failed(result), result, "Unable to get forum thread page count");

	// 2) Load and parse absent pages
	bfr::PostList postsTemp;
	for (int i = 1; i <= pageCount; i++) {
		result = getForumPagePosts(urlData, i, postsTemp);
		BFR_RETURN_VALUE_IF(result_code::failed(result), result, "Unable to get specified forum thread page posts");

		posts << postsTemp;
		postsTemp.clear();

		emit threadParseProgress(i, pageCount);
	}

	SystemLogger->debug("Forum thread '{}' posts has been parsed", url->firstPageUrl());
	return result_code::Type::Ok;
}
