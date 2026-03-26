#pragma once

#include "Sandbox/VersionControl/VersionControl.h"

#include "ClientUsers.h"

#include <p4/clientapi.h>
#include <p4/p4libs.h>

#define P4CHECK(e) 	if (e.Test()) \
					{ \
						StrBuf msg; \
						e.Fmt(&msg); \
						VT_LOG(Error, "VCS: {0}", msg.Text()); \
						VT_ASSERT(false); \
					} \

class P4Implementation final : public VersionControl
{
public:
	P4Implementation() = default;

protected:
	void InitializeImpl() override;
	void ShutdownImpl() override;
	void DisconnectImpl() override;
	bool ConnectImpl(const String& server, const String& user, const String& password) override;

	void AddImpl(const Filesystem::Path& file) override;
	void DeleteImpl(const Filesystem::Path& file) override;
	void EditImpl(const Filesystem::Path& file) override;

	void SubmitImpl(const String& message) override;
	void SyncImpl(const String& depo = "") override;

	void SwitchStreamImpl(const String& newStream) override;
	void RefreshStreamsImpl() override;

	void SwitchWorkspaceImpl(const String& newStream) override;
	void RefreshWorkspacesImpl() override;

	const Vector<String>& GetWorkspacesImpl() override;
	const Vector<String>& GetStreamsImpl() override;
	bool IsConnectedImpl() override;

private:
	StreamsClientUser m_streamsCU;
	WorkspacesClientUser m_workspacesCU;

	ClientUser m_defaultUser;
	ClientApi m_client;

	bool m_isConnected = false;
};

