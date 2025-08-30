#pragma once

#include <CoreUtilities/UUID.h>

#include <string>

typedef UUID64 ModalID;


typedef int ImGuiWindowFlags;       // -> enum ImGuiWindowFlags_

class Modal
{
public:
	// ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_AlwaysAutoResize
	Modal(const std::string& strId, ImGuiWindowFlags flags = (1 << 1) + (1 << 3) + (1 << 4) + (1 << 6));
	virtual ~Modal() = default;

	void Open();
	void Close();
	bool Update();

	inline const ModalID GetID() const { return m_id; }

protected:
	virtual void DrawModalContent() = 0;
	virtual void OnOpen() {};
	virtual void OnClose() {};

private:
	friend class ModalSystem;

	bool m_wasOpenLastFrame = false;

	ModalID m_id;
	std::string m_strId;
	ImGuiWindowFlags m_flags;
};
