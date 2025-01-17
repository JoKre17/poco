//
// SharedLibrary_WIN32U.cpp
//
// Library: Foundation
// Package: SharedLibrary
// Module:  SharedLibrary
//
// Copyright (c) 2004-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "Poco/SharedLibrary_WIN32U.h"
#include "Poco/UnicodeConverter.h"
#include "Poco/Path.h"
#include "Poco/UnWindows.h"
#include "Poco/Error.h"
#include "Poco/Format.h"
#include "Poco/String.h"


namespace Poco {


std::mutex SharedLibraryImpl::_mutex;


SharedLibraryImpl::SharedLibraryImpl()
{
	_handle = 0;
}


SharedLibraryImpl::~SharedLibraryImpl()
{
}


void SharedLibraryImpl::loadImpl(const std::string& path, int /*flags*/)
{
	std::lock_guard<std::mutex> lock(_mutex);

	if (_handle) throw LibraryAlreadyLoadedException(_path);
	DWORD flags(0);
#if !defined(_WIN32_WCE)
	Path p(path);
	if (p.isAbsolute()) flags |= LOAD_WITH_ALTERED_SEARCH_PATH;
#endif
	std::wstring upath;
	UnicodeConverter::toUTF16(path, upath);
	_handle = LoadLibraryExW(upath.c_str(), 0, flags);
	if (!_handle)
	{
		DWORD errn = Error::last();
		std::string err;
		Poco::format(err, "Error %ul while loading [%s]: [%s]", errn, path, Poco::trim(Error::getMessage(errn)));
		throw LibraryLoadException(err);
	}
	_path = path;
}


void SharedLibraryImpl::unloadImpl()
{
	std::lock_guard<std::mutex> lock(_mutex);

	if (_handle)
	{
		FreeLibrary((HMODULE) _handle);
		_handle = 0;
	}
	_path.clear();
}


bool SharedLibraryImpl::isLoadedImpl() const
{
	std::lock_guard<std::mutex> lock(_mutex);
	return _handle != 0;
}


void* SharedLibraryImpl::findSymbolImpl(const std::string& name)
{
	std::lock_guard<std::mutex> lock(_mutex);

	if (_handle)
	{
#if defined(_WIN32_WCE)
		std::wstring uname;
		UnicodeConverter::toUTF16(name, uname);
		return (void*) GetProcAddressW((HMODULE) _handle, uname.c_str());
#else
		return (void*) GetProcAddress((HMODULE) _handle, name.c_str());
#endif
	}
	else return 0;
}


const std::string& SharedLibraryImpl::getPathImpl() const
{
	return _path;
}


std::string SharedLibraryImpl::suffixImpl()
{
#if defined(_DEBUG) && !defined(POCO_NO_SHARED_LIBRARY_DEBUG_SUFFIX)
	return "d.dll";
#else
	return ".dll";
#endif
}


bool SharedLibraryImpl::setSearchPathImpl(const std::string& path)
{
#if _WIN32_WINNT >= 0x0502
	std::wstring wpath;
	Poco::UnicodeConverter::toUTF16(path, wpath);
	return SetDllDirectoryW(wpath.c_str()) != 0;
#else
	return false;
#endif
}


} // namespace Poco
