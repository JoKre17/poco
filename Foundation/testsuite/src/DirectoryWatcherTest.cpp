//
// DirectoryWatcherTest.cpp
//
// Copyright (c) 2012, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "DirectoryWatcherTest.h"


#ifndef POCO_NO_INOTIFY


#include "CppUnit/TestCaller.h"
#include "CppUnit/TestSuite.h"
#include "Poco/DirectoryWatcher.h"
#include "Poco/Delegate.h"
#include "Poco/FileStream.h"


using Poco::DirectoryWatcher;


DirectoryWatcherTest::DirectoryWatcherTest(const std::string& name):
	CppUnit::TestCase(name),
	_error(false)
{
}


DirectoryWatcherTest::~DirectoryWatcherTest()
{
}


void DirectoryWatcherTest::testAdded()
{
	DirectoryWatcher dw(path().toString(), DirectoryWatcher::DW_FILTER_ENABLE_ALL, 2);

	dw.itemAdded += Poco::delegate(this, &DirectoryWatcherTest::onItemAdded);
	dw.itemRemoved += Poco::delegate(this, &DirectoryWatcherTest::onItemRemoved);
	dw.itemModified += Poco::delegate(this, &DirectoryWatcherTest::onItemModified);
	dw.itemMovedFrom += Poco::delegate(this, &DirectoryWatcherTest::onItemMovedFrom);
	dw.itemMovedTo += Poco::delegate(this, &DirectoryWatcherTest::onItemMovedTo);

	Poco::Thread::sleep(1000);

	Poco::Path p(path());
	p.setFileName("test.txt");
	Poco::FileOutputStream fos(p.toString());
	fos << "Hello, world!";
	fos.close();

	Poco::Thread::sleep(2000*dw.scanInterval());

	std::lock_guard<std::recursive_mutex> l(_mutex);
	assertTrue (_events.size() >= 1);
	assertTrue (_events[0].callback == "onItemAdded");
	assertTrue (Poco::Path(_events[0].path).getFileName() == "test.txt");
	assertTrue (_events[0].type == DirectoryWatcher::DW_ITEM_ADDED);
	assertTrue (!_error);
}


void DirectoryWatcherTest::testRemoved()
{
	Poco::Path p(path());
	p.setFileName("test.txt");
	Poco::FileOutputStream fos(p.toString());
	fos << "Hello, world!";
	fos.close();

	DirectoryWatcher dw(path().toString(), DirectoryWatcher::DW_FILTER_ENABLE_ALL, 2);

	dw.itemAdded += Poco::delegate(this, &DirectoryWatcherTest::onItemAdded);
	dw.itemRemoved += Poco::delegate(this, &DirectoryWatcherTest::onItemRemoved);
	dw.itemModified += Poco::delegate(this, &DirectoryWatcherTest::onItemModified);
	dw.itemMovedFrom += Poco::delegate(this, &DirectoryWatcherTest::onItemMovedFrom);
	dw.itemMovedTo += Poco::delegate(this, &DirectoryWatcherTest::onItemMovedTo);

	Poco::Thread::sleep(1000);

	Poco::File f(p.toString());
	f.remove();

	Poco::Thread::sleep(2000*dw.scanInterval());

	std::lock_guard<std::recursive_mutex> l(_mutex);
	assertTrue (_events.size() >= 1);
	assertTrue (_events[0].callback == "onItemRemoved");
	assertTrue (Poco::Path(_events[0].path).getFileName() == "test.txt");
	assertTrue (_events[0].type == DirectoryWatcher::DW_ITEM_REMOVED);
	assertTrue (!_error);
}


void DirectoryWatcherTest::testModified()
{
	Poco::Path p(path());
	p.setFileName("test.txt");
	Poco::FileOutputStream fos(p.toString());
	fos << "Hello, world!";
	fos.close();

	DirectoryWatcher dw(path().toString(), DirectoryWatcher::DW_FILTER_ENABLE_ALL, 2);

	dw.itemAdded += Poco::delegate(this, &DirectoryWatcherTest::onItemAdded);
	dw.itemRemoved += Poco::delegate(this, &DirectoryWatcherTest::onItemRemoved);
	dw.itemModified += Poco::delegate(this, &DirectoryWatcherTest::onItemModified);
	dw.itemMovedFrom += Poco::delegate(this, &DirectoryWatcherTest::onItemMovedFrom);
	dw.itemMovedTo += Poco::delegate(this, &DirectoryWatcherTest::onItemMovedTo);

	Poco::Thread::sleep(1000);

	Poco::FileOutputStream fos2(p.toString(), std::ios::app);
	fos2 << "Again!";
	fos2.close();

	Poco::Thread::sleep(2000*dw.scanInterval());

	std::lock_guard<std::recursive_mutex> l(_mutex);
	assertTrue (_events.size() >= 1);
	assertTrue (_events[0].callback == "onItemModified");
	assertTrue (Poco::Path(_events[0].path).getFileName() == "test.txt");
	assertTrue (_events[0].type == DirectoryWatcher::DW_ITEM_MODIFIED);
	assertTrue (!_error);
}


void DirectoryWatcherTest::testMoved()
{
	Poco::Path p(path());
	p.setFileName("test.txt");
	Poco::FileOutputStream fos(p.toString());
	fos << "Hello, world!";
	fos.close();

	DirectoryWatcher dw(path().toString(), DirectoryWatcher::DW_FILTER_ENABLE_ALL, 2);

	dw.itemAdded += Poco::delegate(this, &DirectoryWatcherTest::onItemAdded);
	dw.itemRemoved += Poco::delegate(this, &DirectoryWatcherTest::onItemRemoved);
	dw.itemModified += Poco::delegate(this, &DirectoryWatcherTest::onItemModified);
	dw.itemMovedFrom += Poco::delegate(this, &DirectoryWatcherTest::onItemMovedFrom);
	dw.itemMovedTo += Poco::delegate(this, &DirectoryWatcherTest::onItemMovedTo);

	Poco::Thread::sleep(1000);

	Poco::Path p2(path());
	p2.setFileName("test2.txt");
	Poco::File f(p.toString());
	f.renameTo(p2.toString());

	Poco::Thread::sleep(2000*dw.scanInterval());

	std::lock_guard<std::recursive_mutex> l(_mutex);
	if (dw.supportsMoveEvents())
	{
		assertTrue (_events.size() >= 2);
		assertTrue (
			(_events[0].callback == "onItemMovedFrom" && _events[1].callback == "onItemMovedTo") ||
			(_events[1].callback == "onItemMovedFrom" && _events[0].callback == "onItemMovedTo")
		);
		assertTrue (
			(Poco::Path(_events[0].path).getFileName() == "test.txt" && Poco::Path(_events[1].path).getFileName() == "test2.txt") ||
			(Poco::Path(_events[1].path).getFileName() == "test.txt" && Poco::Path(_events[0].path).getFileName() == "test2.txt")
		);
		assertTrue (
			(_events[0].type == DirectoryWatcher::DW_ITEM_MOVED_FROM && _events[1].type == DirectoryWatcher::DW_ITEM_MOVED_TO) ||
			(_events[1].type == DirectoryWatcher::DW_ITEM_MOVED_FROM && _events[0].type == DirectoryWatcher::DW_ITEM_MOVED_TO)
		);
	}
	else
	{
		assertTrue (_events.size() >= 2);
		assertTrue (
			(_events[0].callback == "onItemAdded" && _events[1].callback == "onItemRemoved") ||
			(_events[1].callback == "onItemAdded" && _events[0].callback == "onItemRemoved")
		);
		assertTrue (
			(Poco::Path(_events[0].path).getFileName() == "test.txt" && Poco::Path(_events[1].path).getFileName() == "test2.txt") ||
			(Poco::Path(_events[1].path).getFileName() == "test.txt" && Poco::Path(_events[0].path).getFileName() == "test2.txt")
		);
		assertTrue (
			(_events[0].type == DirectoryWatcher::DW_ITEM_ADDED && _events[1].type == DirectoryWatcher::DW_ITEM_REMOVED) ||
			(_events[1].type == DirectoryWatcher::DW_ITEM_ADDED && _events[0].type == DirectoryWatcher::DW_ITEM_REMOVED)
		);
	}
	assertTrue (!_error);
}


void DirectoryWatcherTest::testSuspend()
{
	Poco::Path p(path());
	p.setFileName("test.txt");
	Poco::FileOutputStream fos(p.toString());
	fos << "Hello, world!";
	fos.close();

	DirectoryWatcher dw(path().toString(), DirectoryWatcher::DW_FILTER_ENABLE_ALL, 2);

	dw.itemAdded += Poco::delegate(this, &DirectoryWatcherTest::onItemAdded);
	dw.itemRemoved += Poco::delegate(this, &DirectoryWatcherTest::onItemRemoved);
	dw.itemModified += Poco::delegate(this, &DirectoryWatcherTest::onItemModified);
	dw.itemMovedFrom += Poco::delegate(this, &DirectoryWatcherTest::onItemMovedFrom);
	dw.itemMovedTo += Poco::delegate(this, &DirectoryWatcherTest::onItemMovedTo);

	Poco::Thread::sleep(1000);

	dw.suspendEvents();

	Poco::FileOutputStream fos2(p.toString(), std::ios::app);
	fos2 << "Again!";
	fos2.close();

	Poco::Thread::sleep(2000*dw.scanInterval());

	std::lock_guard<std::recursive_mutex> l(_mutex);
	assertTrue (_events.size() == 0);
	assertTrue (!_error);
}


void DirectoryWatcherTest::testResume()
{
	Poco::Path p(path());
	p.setFileName("test.txt");
	Poco::FileOutputStream fos(p.toString());
	fos << "Hello, world!";
	fos.close();

	DirectoryWatcher dw(path().toString(), DirectoryWatcher::DW_FILTER_ENABLE_ALL, 2);

	dw.itemAdded += Poco::delegate(this, &DirectoryWatcherTest::onItemAdded);
	dw.itemRemoved += Poco::delegate(this, &DirectoryWatcherTest::onItemRemoved);
	dw.itemModified += Poco::delegate(this, &DirectoryWatcherTest::onItemModified);
	dw.itemMovedFrom += Poco::delegate(this, &DirectoryWatcherTest::onItemMovedFrom);
	dw.itemMovedTo += Poco::delegate(this, &DirectoryWatcherTest::onItemMovedTo);

	Poco::Thread::sleep(1000);

	dw.suspendEvents();

	Poco::FileOutputStream fos2(p.toString(), std::ios::app);
	fos2 << "Again!";
	fos2.close();

	{
		std::lock_guard<std::recursive_mutex> l(_mutex);
		assertTrue (_events.size() == 0);
		assertTrue (!_error);
	}

	dw.resumeEvents();

	Poco::FileOutputStream fos3(p.toString(), std::ios::app);
	fos3 << "Now it works!";
	fos3.close();

	Poco::Thread::sleep(2000*dw.scanInterval());

	std::lock_guard<std::recursive_mutex> l(_mutex);
	assertTrue (_events.size() >= 1);
	assertTrue (_events[0].callback == "onItemModified");
	assertTrue (Poco::Path(_events[0].path).getFileName() == "test.txt");
	assertTrue (_events[0].type == DirectoryWatcher::DW_ITEM_MODIFIED);
	assertTrue (!_error);
}


void DirectoryWatcherTest::testSuspendMultipleTimes()
{
	Poco::Path p(path());
	p.setFileName("test.txt");
	Poco::FileOutputStream fos(p.toString());
	fos << "Hello, world!";
	fos.close();

	DirectoryWatcher dw(path().toString(), DirectoryWatcher::DW_FILTER_ENABLE_ALL, 2);

	dw.itemAdded += Poco::delegate(this, &DirectoryWatcherTest::onItemAdded);
	dw.itemRemoved += Poco::delegate(this, &DirectoryWatcherTest::onItemRemoved);
	dw.itemModified += Poco::delegate(this, &DirectoryWatcherTest::onItemModified);
	dw.itemMovedFrom += Poco::delegate(this, &DirectoryWatcherTest::onItemMovedFrom);
	dw.itemMovedTo += Poco::delegate(this, &DirectoryWatcherTest::onItemMovedTo);

	Poco::Thread::sleep(1000);

	dw.suspendEvents();
	dw.suspendEvents();
	dw.suspendEvents();

	Poco::FileOutputStream fos2(p.toString(), std::ios::app);
	fos2 << "Not notified!";
	fos2.close();

	Poco::Thread::sleep(2000*dw.scanInterval());

	assertTrue (_events.size() == 0);
	assertTrue (!_error);

	dw.resumeEvents();

	Poco::FileOutputStream fos3(p.toString(), std::ios::app);
	fos3 << "Still not notified!";
	fos3.close();

	Poco::Thread::sleep(2000*dw.scanInterval());

	{
		std::lock_guard<std::recursive_mutex> l(_mutex);
		assertTrue (_events.size() == 0);
		assertTrue (!_error);
	}

	dw.resumeEvents();
	dw.resumeEvents();

	Poco::FileOutputStream fos4(p.toString(), std::ios::app);
	fos4 << "Now it works!";
	fos4.close();

	Poco::Thread::sleep(2000*dw.scanInterval());

	std::lock_guard<std::recursive_mutex> l(_mutex);
	assertTrue (_events.size() >= 1);
	assertTrue (_events[0].callback == "onItemModified");
	assertTrue (Poco::Path(_events[0].path).getFileName() == "test.txt");
	assertTrue (_events[0].type == DirectoryWatcher::DW_ITEM_MODIFIED);
	assertTrue (!_error);
}


void DirectoryWatcherTest::setUp()
{
	_error = false;
	_events.clear();

	try
	{
		Poco::File d(path().toString());
		d.remove(true);
	}
	catch (...)
	{
	}

	Poco::File d(path().toString());
	d.createDirectories();
}


void DirectoryWatcherTest::tearDown()
{
	try
	{
		Poco::File d(path().toString());
		d.remove(true);
	}
	catch (...)
	{
	}
}


void DirectoryWatcherTest::onItemAdded(const Poco::DirectoryWatcher::DirectoryEvent& ev)
{
	DirEvent de;
	de.callback = "onItemAdded";
	de.path = ev.item.path();
	de.type = ev.event;

	std::lock_guard<std::recursive_mutex> l(_mutex);
	_events.push_back(de);
}


void DirectoryWatcherTest::onItemRemoved(const Poco::DirectoryWatcher::DirectoryEvent& ev)
{
	DirEvent de;
	de.callback = "onItemRemoved";
	de.path = ev.item.path();
	de.type = ev.event;

	std::lock_guard<std::recursive_mutex> l(_mutex);
	_events.push_back(de);
}


void DirectoryWatcherTest::onItemModified(const Poco::DirectoryWatcher::DirectoryEvent& ev)
{
	DirEvent de;
	de.callback = "onItemModified";
	de.path = ev.item.path();
	de.type = ev.event;

	std::lock_guard<std::recursive_mutex> l(_mutex);
	_events.push_back(de);
}


void DirectoryWatcherTest::onItemMovedFrom(const Poco::DirectoryWatcher::DirectoryEvent& ev)
{
	DirEvent de;
	de.callback = "onItemMovedFrom";
	de.path = ev.item.path();
	de.type = ev.event;

	std::lock_guard<std::recursive_mutex> l(_mutex);
	_events.push_back(de);
}


void DirectoryWatcherTest::onItemMovedTo(const Poco::DirectoryWatcher::DirectoryEvent& ev)
{
	DirEvent de;
	de.callback = "onItemMovedTo";
	de.path = ev.item.path();
	de.type = ev.event;

	std::lock_guard<std::recursive_mutex> l(_mutex);
	_events.push_back(de);
}


void DirectoryWatcherTest::onError(const Poco::Exception& exc)
{

	_error = true;
}


Poco::Path DirectoryWatcherTest::path() const
{
	Poco::Path p(Poco::Path::current());
	p.pushDirectory("DirectoryWatcherTest");
	return p;
}


CppUnit::Test* DirectoryWatcherTest::suite()
{
	CppUnit::TestSuite* pSuite = new CppUnit::TestSuite("DirectoryWatcherTest");

	CppUnit_addTest(pSuite, DirectoryWatcherTest, testAdded);
	CppUnit_addTest(pSuite, DirectoryWatcherTest, testRemoved);
	CppUnit_addTest(pSuite, DirectoryWatcherTest, testModified);
	CppUnit_addTest(pSuite, DirectoryWatcherTest, testMoved);
	CppUnit_addTest(pSuite, DirectoryWatcherTest, testSuspend);
	CppUnit_addTest(pSuite, DirectoryWatcherTest, testResume);
	CppUnit_addTest(pSuite, DirectoryWatcherTest, testSuspendMultipleTimes);

	return pSuite;
}


#endif // POCO_NO_INOTIFY
