//
// LRUCache.h
//
// Library: Foundation
// Package: Cache
// Module:  LRUCache
//
// Definition of the LRUCache class.
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#ifndef Foundation_LRUCache_INCLUDED
#define Foundation_LRUCache_INCLUDED


#include "Poco/AbstractCache.h"
#include "Poco/LRUStrategy.h"


namespace Poco {


template <
	class TKey,
	class TValue,
	class TMutex = std::mutex,
	class TEventMutex = std::mutex
>
class LRUCache: public AbstractCache<TKey, TValue, LRUStrategy<TKey, TValue>, TMutex, TEventMutex>
	/// An LRUCache implements Least Recently Used caching. The default size for a cache is 1024 entries.
{
public:
	LRUCache(std::size_t size = 1024):
		AbstractCache<TKey, TValue, LRUStrategy<TKey, TValue>, TMutex, TEventMutex>(LRUStrategy<TKey, TValue>(size))
	{
	}

	~LRUCache()
	{
	}

private:
	LRUCache(const LRUCache& aCache);
	LRUCache& operator = (const LRUCache& aCache);
};


} // namespace Poco


#endif // Foundation_LRUCache_INCLUDED
