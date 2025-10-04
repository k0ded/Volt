#pragma once

#include "Sandbox/Modals/Modal.h"

#include <CoreUtilities/UUID.h>

#include <unordered_map>

class ModalSystem
{
public:
	ModalSystem();
	~ModalSystem();

	static void Update();

	template<typename T> static T& AddModal(const std::string& strId);
	template<typename T> static [[nodiscard]] T& GetModal(const UUID64& modalId);

	static void RemoveModal(const UUID64& modalId);

private:
	inline static ModalSystem* s_instance = nullptr;

	Map<UUID64, Scope<Modal>> m_modals;
};

template<typename T>
T& ModalSystem::GetModal(const UUID64& modalId)
{
	if (!s_instance->m_modals.contains(modalId))
	{
		static T empty = T{ "" };
		return empty;
	}

	return reinterpret_cast<T&>(*s_instance->m_modals.at(modalId));
}

template<typename T>
inline T& ModalSystem::AddModal(const std::string& strId)
{
	UUID64 newUUID = {};
	Scope<T> newModal = CreateScope<T>(strId);

	newModal->m_id = newUUID;
	s_instance->m_modals[newUUID] = std::move(newModal);

	return reinterpret_cast<T&>(*s_instance->m_modals[newUUID]);
}
