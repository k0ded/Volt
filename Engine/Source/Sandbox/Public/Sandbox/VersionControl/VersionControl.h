#pragma once

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Pointers/Unique.h>
#include <CoreUtilities/Filesystem/Path.h>

struct VersionControlSettings
{
	String server = "localhost:1666";
	String user;
	String password;

	String workspace;
	String stream;
};

enum class VersionControlSystem
{
	Perforce
};

class VersionControl
{
public:
	static void Initialize(VersionControlSystem system);
	static void Shutdown();

	static bool Connect(const String& server, const String& user, const String& password);
	static void Disconnect();

	static void Add(const Filesystem::Path& file);
	static void Delete(const Filesystem::Path& file);
	static void Edit(const Filesystem::Path& file);

	static void Submit(const String& message);
	static void Sync(const String& stream = "");

	static void SwitchStream(const String& newStream);
	static void RefreshStreams();

	static void SwitchWorkspace(const String& workspace);
	static void RefreshWorkspaces();

	static const Vector<String>& GetWorkspaces();
	static const Vector<String>& GetStreams();
	static bool IsConnected();

protected:
	virtual void InitializeImpl() = 0;
	virtual void ShutdownImpl() = 0;
	virtual void DisconnectImpl() = 0;
	virtual bool ConnectImpl(const String& server, const String& user, const String& password) = 0;

	virtual void AddImpl(const Filesystem::Path& file) = 0;
	virtual void DeleteImpl(const Filesystem::Path& file) = 0;
	virtual void EditImpl(const Filesystem::Path& file) = 0;

	virtual void SubmitImpl(const String& message) = 0;
	virtual void SyncImpl(const String& depo = "") = 0;

	virtual void SwitchWorkspaceImpl(const String& newStream) = 0;
	virtual void RefreshWorkspacesImpl() = 0;

	virtual void SwitchStreamImpl(const String& newStream) = 0;
	virtual void RefreshStreamsImpl() = 0;

	virtual const Vector<String>& GetWorkspacesImpl() = 0;
	virtual const Vector<String>& GetStreamsImpl() = 0;
	virtual bool IsConnectedImpl() = 0;

private:
	inline static Unique<VersionControl> s_implementation;
};
