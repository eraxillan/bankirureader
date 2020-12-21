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
#ifndef __BFR_WEBSITEINTERFACE_FWD_H__
#define __BFR_WEBSITEINTERFACE_FWD_H__

#include <QtCore/QList>
#include <QtCore/QSharedPointer>

namespace bfr
{

struct IPostObject;
using IPostObjectPtr = QSharedPointer<IPostObject>;
using IPostObjectList = QList<IPostObjectPtr>;

struct PostSpoiler;
using PostSpoilerPtr = QSharedPointer<PostSpoiler>;

struct PostQuote;
using PostQuotePtr = QSharedPointer<PostQuote>;

struct PostImage;
using PostImagePtr = QSharedPointer<PostImage>;

struct PostLineBreak;
using PostLineBreakPtr = QSharedPointer<PostLineBreak>;

struct PostPlainText;
using PostPlainTextPtr = QSharedPointer<PostPlainText>;

struct PostRichText;
using PostRichTextPtr = QSharedPointer<PostRichText>;

struct PostVideo;
using PostVideoPtr = QSharedPointer<PostVideo>;

struct PostHyperlink;
using PostHyperlinkPtr = QSharedPointer<PostHyperlink>;

// ----------------------------------------------------------------------------------------------------------------

struct Post;
using PostPtr = QSharedPointer<Post>;
using PostList = QList<PostPtr>;

struct User;
using UserPtr = QSharedPointer<User>;
using UserList = QList<UserPtr>;

}  // namespace bfr

#endif // __BFR_WEBSITEINTERFACE_FWD_H__