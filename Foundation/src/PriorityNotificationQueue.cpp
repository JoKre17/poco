//
// PriorityNotificationQueue.cpp
//
// Library: Foundation
// Package: Notifications
// Module:  PriorityNotificationQueue
//
// Copyright (c) 2009, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "Poco/PriorityNotificationQueue.h"
#include "Poco/NotificationCenter.h"
#include "Poco/Notification.h"
#include "Poco/SingletonHolder.h"


namespace Poco {


PriorityNotificationQueue::PriorityNotificationQueue()
{
}


PriorityNotificationQueue::~PriorityNotificationQueue()
{
	try
	{
		clear();
	}
	catch (...)
	{
		poco_unexpected();
	}
}


void PriorityNotificationQueue::enqueueNotification(Notification::Ptr pNotification, int priority)
{
	poco_check_ptr (pNotification);
	std::lock_guard<std::mutex> lock(_mutex);
	if (_waitQueue.empty())
	{
		_nfQueue.insert(NfQueue::value_type(priority, pNotification));
	}
	else
	{
		poco_assert_dbg(_nfQueue.empty());
		WaitInfo* pWI = _waitQueue.front();
		_waitQueue.pop_front();
		pWI->pNf = pNotification;
		pWI->nfAvailable.set();
	}
}


Notification* PriorityNotificationQueue::dequeueNotification()
{
	std::lock_guard<std::mutex> lock(_mutex);
	return dequeueOne().duplicate();
}


Notification* PriorityNotificationQueue::waitDequeueNotification()
{
	Notification::Ptr pNf;
	WaitInfo* pWI = 0;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		pNf = dequeueOne();
		if (pNf) return pNf.duplicate();
		pWI = new WaitInfo;
		_waitQueue.push_back(pWI);
	}
	pWI->nfAvailable.wait();
	pNf = pWI->pNf;
	delete pWI;
	return pNf.duplicate();
}


Notification* PriorityNotificationQueue::waitDequeueNotification(long milliseconds)
{
	Notification::Ptr pNf;
	WaitInfo* pWI = 0;
	{
		std::lock_guard<std::mutex> lock(_mutex);
		pNf = dequeueOne();
		if (pNf) return pNf.duplicate();
		pWI = new WaitInfo;
		_waitQueue.push_back(pWI);
	}
	if (pWI->nfAvailable.tryWait(milliseconds))
	{
		pNf = pWI->pNf;
	}
	else
	{
		std::lock_guard<std::mutex> lock(_mutex);
		pNf = pWI->pNf;
		for (WaitQueue::iterator it = _waitQueue.begin(); it != _waitQueue.end(); ++it)
		{
			if (*it == pWI)
			{
				_waitQueue.erase(it);
				break;
			}
		}
	}
	delete pWI;
	return pNf.duplicate();
}


void PriorityNotificationQueue::dispatch(NotificationCenter& notificationCenter)
{
	std::lock_guard<std::mutex> lock(_mutex);
	Notification::Ptr pNf = dequeueOne();
	while (pNf)
	{
		notificationCenter.postNotification(pNf);
		pNf = dequeueOne();
	}
}


void PriorityNotificationQueue::wakeUpAll()
{
	std::lock_guard<std::mutex> lock(_mutex);
	for (auto p: _waitQueue)
	{
		p->nfAvailable.set();
	}
	_waitQueue.clear();
}


bool PriorityNotificationQueue::empty() const
{
	std::lock_guard<std::mutex> lock(_mutex);
	return _nfQueue.empty();
}


int PriorityNotificationQueue::size() const
{
	std::lock_guard<std::mutex> lock(_mutex);
	return static_cast<int>(_nfQueue.size());
}


void PriorityNotificationQueue::clear()
{
	std::lock_guard<std::mutex> lock(_mutex);
	_nfQueue.clear();
}


bool PriorityNotificationQueue::hasIdleThreads() const
{
	std::lock_guard<std::mutex> lock(_mutex);
	return !_waitQueue.empty();
}


Notification::Ptr PriorityNotificationQueue::dequeueOne()
{
	Notification::Ptr pNf;
	NfQueue::iterator it = _nfQueue.begin();
	if (it != _nfQueue.end())
	{
		pNf = it->second;
		_nfQueue.erase(it);
	}
	return pNf;
}


namespace
{
	static SingletonHolder<PriorityNotificationQueue> sh;
}


PriorityNotificationQueue& PriorityNotificationQueue::defaultQueue()
{
	return *sh.get();
}


} // namespace Poco
