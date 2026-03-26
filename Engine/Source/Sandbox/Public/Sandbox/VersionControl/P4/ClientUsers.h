#pragma once

#include <CoreUtilities/Containers/Vector.h>

#include <p4/clientapi.h>

#include <string>

class StreamsClientUser : public ClientUser
{
public:
	void OutputInfo(char level, const char* data) override;
	void Clear();

	inline const Vector<String>& GetData() const { return m_streams; }

private:
	Vector<String> m_streams;
};

class WorkspacesClientUser : public ClientUser
{
public:
	void OutputInfo(char level, const char* data) override;
	void Clear();

	inline const Vector<String>& GetData() const { return m_workspaces; }

private:
	Vector<String> m_workspaces;
};
