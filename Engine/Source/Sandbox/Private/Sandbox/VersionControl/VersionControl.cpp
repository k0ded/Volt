#include "sbpch.h"
#include "VersionControl/VersionControl.h"

#include "Sandbox/VersionControl/P4/P4Implementation.h"

void VersionControl::Initialize(VersionControlSystem system)
{
	switch (system)
	{
		case VersionControlSystem::Perforce:
			s_implementation = CreateUnique<P4Implementation>();
			break;

		default:
			VT_LOG(Error, "Invalid version control selected!");
			return;
			break;
	}

	s_implementation->InitializeImpl();
}

void VersionControl::Shutdown()
{
	VT_ASSERT_MSG(s_implementation, "No implementation loaded!");
	s_implementation->ShutdownImpl();
	s_implementation = nullptr;
}

bool VersionControl::Connect(const String& server, const String& user, const String& password)
{
	VT_ASSERT_MSG(s_implementation, "No implementation loaded!");
	return s_implementation->ConnectImpl(server, user, password);
}

void VersionControl::Disconnect()
{
	VT_ASSERT_MSG(s_implementation, "No implementation loaded!");
	s_implementation->DisconnectImpl();
}

void VersionControl::Add(const Filesystem::Path& file)
{
	VT_ASSERT_MSG(s_implementation, "No implementation loaded!");
	s_implementation->AddImpl(file);
}

void VersionControl::Delete(const Filesystem::Path& file)
{
	VT_ASSERT_MSG(s_implementation, "No implementation loaded!");
	s_implementation->DeleteImpl(file);
}

void VersionControl::Edit(const Filesystem::Path& file)
{
	VT_ASSERT_MSG(s_implementation, "No implementation loaded!");
	s_implementation->EditImpl(file);
}

void VersionControl::Submit(const String& message)
{
	VT_ASSERT_MSG(s_implementation, "No implementation loaded!");
	s_implementation->SubmitImpl(message);
}

void VersionControl::Sync(const String& depo)
{
	VT_ASSERT_MSG(s_implementation, "No implementation loaded!");
	s_implementation->SyncImpl(depo);
}

void VersionControl::SwitchStream(const String& newStream)
{
	VT_ASSERT_MSG(s_implementation, "No implementation loaded!");
	s_implementation->SwitchStreamImpl(newStream);
}

void VersionControl::RefreshStreams()
{
	VT_ASSERT_MSG(s_implementation, "No implementation loaded!");
	s_implementation->RefreshStreamsImpl();
}

void VersionControl::SwitchWorkspace(const String& workspace)
{
	VT_ASSERT_MSG(s_implementation, "No implementation loaded!");
	s_implementation->SwitchWorkspaceImpl(workspace);
}

void VersionControl::RefreshWorkspaces()
{
	VT_ASSERT_MSG(s_implementation, "No implementation loaded!");
	s_implementation->RefreshWorkspacesImpl();
}

const Vector<String>& VersionControl::GetWorkspaces()
{
	VT_ASSERT_MSG(s_implementation, "No implementation loaded!");
	return s_implementation->GetWorkspacesImpl();
}

const Vector<String>& VersionControl::GetStreams()
{
	VT_ASSERT_MSG(s_implementation, "No implementation loaded!");
	return s_implementation->GetStreamsImpl();
}

bool VersionControl::IsConnected()
{
	VT_ASSERT_MSG(s_implementation, "No implementation loaded!");
	return s_implementation->IsConnectedImpl();
}
