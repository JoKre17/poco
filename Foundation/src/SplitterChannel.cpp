//
// SplitterChannel.cpp
//
// Library: Foundation
// Package: Logging
// Module:  SplitterChannel
//
// Copyright (c) 2004-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "Poco/SplitterChannel.h"
#include "Poco/LoggingRegistry.h"
#include "Poco/StringTokenizer.h"


namespace Poco {


SplitterChannel::SplitterChannel()
{
}


SplitterChannel::~SplitterChannel()
{
	try
	{
		close();
	}
	catch (...)
	{
		poco_unexpected();
	}
}


void SplitterChannel::addChannel(Channel::Ptr pChannel)
{
	poco_check_ptr (pChannel);

	std::lock_guard<std::mutex> lock(_mutex);
	_channels.push_back(pChannel);
}


void SplitterChannel::removeChannel(Channel::Ptr pChannel)
{
	std::lock_guard<std::mutex> lock(_mutex);

	for (ChannelVec::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		if (*it == pChannel)
		{
			_channels.erase(it);
			break;
		}
	}
}


void SplitterChannel::setProperty(const std::string& name, const std::string& value)
{
	if (name.compare(0, 7, "channel") == 0)
	{
		StringTokenizer tokenizer(value, ",;", StringTokenizer::TOK_IGNORE_EMPTY | StringTokenizer::TOK_TRIM);
		for (StringTokenizer::Iterator it = tokenizer.begin(); it != tokenizer.end(); ++it)
		{
			addChannel(LoggingRegistry::defaultRegistry().channelForName(*it));
		}
	}
	else Channel::setProperty(name, value);
}


void SplitterChannel::log(const Message& msg)
{
	std::lock_guard<std::mutex> lock(_mutex);

	for (auto& p: _channels)
	{
		p->log(msg);
	}
}


void SplitterChannel::close()
{
	std::lock_guard<std::mutex> lock(_mutex);
	_channels.clear();
}


int SplitterChannel::count() const
{
	std::lock_guard<std::mutex> lock(_mutex);

	return (int) _channels.size();
}


} // namespace Poco
