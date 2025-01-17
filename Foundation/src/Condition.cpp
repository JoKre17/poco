//
// Condition.cpp
//
// Library: Foundation
// Package: Threading
// Module:  Condition
//
// Copyright (c) 2007, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "Poco/Condition.h"


namespace Poco {


Condition::Condition()
{
}

Condition::~Condition()
{
}


void Condition::signal()
{
	std::lock_guard<std::mutex> lock(_mutex);

	if (!_waitQueue.empty())
	{
		_waitQueue.front()->set();
		dequeue();
	}
}


void Condition::broadcast()
{
	std::lock_guard<std::mutex> lock(_mutex);

	for (auto p: _waitQueue)
	{
		p->set();
	}
	_waitQueue.clear();
}


void Condition::enqueue(Event& event)
{
	_waitQueue.push_back(&event);
}


void Condition::dequeue()
{
	_waitQueue.pop_front();
}


void Condition::dequeue(Event& event)
{
	for (WaitQueue::iterator it = _waitQueue.begin(); it != _waitQueue.end(); ++it)
	{
		if (*it == &event)
		{
			_waitQueue.erase(it);
			break;
		}
	}
}


} // namespace Poco
