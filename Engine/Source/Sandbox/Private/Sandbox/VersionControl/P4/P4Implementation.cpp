#include "sbpch.h"
#include "VersionControl/P4/P4Implementation.h"

#include <filesystem>
#include <format>

void P4Implementation::InitializeImpl()
{
	Error e;
	P4Libraries::Initialize(P4LIBRARIES_INIT_ALL, &e);

	m_client.EnableExtensions(&e);
}

void P4Implementation::ShutdownImpl()
{
	Error e;
	P4Libraries::Shutdown(P4LIBRARIES_INIT_ALL, &e);
	P4CHECK(e);
}

void P4Implementation::DisconnectImpl()
{
	if (m_isConnected)
	{
		Error e;
		m_client.Final(&e);
		P4CHECK(e);
	}

	m_isConnected = false;
}

bool P4Implementation::ConnectImpl(const String& server, const String& user, const String& password)
{
	Error e;

	m_client.SetPort(server.c_str());
	m_client.SetUser(user.c_str());
	m_client.SetPassword(password.c_str());
	m_client.SetProtocol("tag", "");

	if (!m_isConnected)
	{
		m_client.Final(&e);
		if (e.Test())
		{
			StrBuf msg;
			e.Fmt(&msg);
			VT_LOG(Error, "VCS: {0}", msg.Text());
			return false;
		}
	}
	else
	{
		DisconnectImpl();
	}

	m_client.Init(&e);
	if (e.Test())
	{
		StrBuf msg;
		e.Fmt(&msg);
		VT_LOG(Error, "VCS: {0}", msg.Text());
		return false;
	}

	m_isConnected = true;
	return true;
}

void P4Implementation::AddImpl(const Filesystem::Path& file)
{
	String stringPath = file.ToString();
	char* argv[] = { stringPath.data() };

	m_client.SetArgv(1, argv);
	m_client.Run("add", &m_defaultUser);
}

void P4Implementation::DeleteImpl(const Filesystem::Path& file)
{
}

void P4Implementation::EditImpl(const Filesystem::Path& file)
{
	String stringPath = file.ToString();
	char* argv[] = { stringPath.data() };

	m_client.SetArgv(1, argv);
	m_client.Run("edit", &m_defaultUser);
}

void P4Implementation::SubmitImpl(const String& message)
{
}

void P4Implementation::SyncImpl(const String& depo)
{
	m_client.Run("sync", &m_defaultUser);
}

void P4Implementation::SwitchStreamImpl(const String& newStream)
{
	char* argv[] = { const_cast<char*>(newStream.c_str()) };

	m_client.SetArgv(1, argv);
	m_client.Run("switch", &m_defaultUser);
}

void P4Implementation::RefreshStreamsImpl()
{
	m_streamsCU.Clear();
	m_client.Run("streams", &m_streamsCU);
}

void P4Implementation::SwitchWorkspaceImpl(const String& newWorkspace)
{
	m_client.SetClient(newWorkspace.c_str());
}

void P4Implementation::RefreshWorkspacesImpl()
{
	m_workspacesCU.Clear();
	m_client.Run("clients", &m_workspacesCU);
}

const Vector<String>& P4Implementation::GetWorkspacesImpl()
{
	return m_workspacesCU.GetData();
}

const Vector<String>& P4Implementation::GetStreamsImpl()
{
	return m_streamsCU.GetData();
}

bool P4Implementation::IsConnectedImpl()
{
	return m_isConnected;
}
