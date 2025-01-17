//
// Task.cpp
//
// Library: Foundation
// Package: Tasks
// Module:  Tasks
//
// Copyright (c) 2004-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/Thread.h"
#include "Poco/Exception.h"


namespace Poco {


Task::Task(const std::string& name):
	_name(name),
	_pOwner(0),
	_progress(0),
	_state(TASK_IDLE),
	_cancelEvent(Event::EVENT_MANUALRESET)
{
}


Task::~Task()
{
}


void Task::cancel()
{
	_state = TASK_CANCELLING;
	_cancelEvent.set();
	if (_pOwner)
		_pOwner->taskCancelled(this);
}


void Task::reset()
{
	_progress = 0.0;
	_state    = TASK_IDLE;
	_cancelEvent.reset();
}


void Task::run()
{
	TaskManager* pOwner = getOwner();
	if (pOwner)
		pOwner->taskStarted(this);
	try
	{
		_state = TASK_RUNNING;
		runTask();
	}
	catch (Exception& exc)
	{
		if (pOwner)
			pOwner->taskFailed(this, exc);
	}
	catch (std::exception& exc)
	{
		if (pOwner)
			pOwner->taskFailed(this, SystemException(exc.what()));
	}
	catch (...)
	{
		if (pOwner)
			pOwner->taskFailed(this, SystemException("unknown exception"));
	}
	_state = TASK_FINISHED;
	if (pOwner) pOwner->taskFinished(this);
}


bool Task::sleep(long milliseconds)
{
	return _cancelEvent.tryWait(milliseconds);
}


bool Task::yield()
{
	Thread::yield();
	return isCancelled();
}


void Task::setProgress(float progress)
{
	std::lock_guard<std::mutex> lock(_mutex);

	if (_progress != progress)
	{
		_progress = progress;
		if (_pOwner)
			_pOwner->taskProgress(this, _progress);
	}
}


void Task::setOwner(TaskManager* pOwner)
{
	std::lock_guard<std::mutex> lock(_mutex);

	_pOwner = pOwner;
}


void Task::setState(TaskState state)
{
	_state = state;
}


void Task::postNotification(Notification* pNf)
{
	poco_check_ptr (pNf);

	std::lock_guard<std::mutex> lock(_mutex);

	if (_pOwner)
		_pOwner->postNotification(pNf);
	else if (pNf)
		pNf->release();
}


} // namespace Poco
