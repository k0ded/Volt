#include "sbpch.h"
#include "VersionControl/P4/ClientUsers.h"

void StreamsClientUser::OutputInfo(char level, const char* data)
{
	String dataStr(data);

	if (dataStr.find("Stream ") != String::npos)
	{
		String streamName = dataStr.substr(dataStr.find_first_of(' ') + 1);
		m_streams.emplace_back(streamName);
	}
}

void StreamsClientUser::Clear()
{
	m_streams.clear();
}

void WorkspacesClientUser::OutputInfo(char level, const char* data)
{
	String dataStr(data);

	if (dataStr.find("client ") != String::npos)
	{
		String clientName = dataStr.substr(dataStr.find_first_of(' ') + 1);
		m_workspaces.emplace_back(clientName);
	}
}

void WorkspacesClientUser::Clear()
{
	m_workspaces.clear();
}
