#include "sbpch.h"
#include "UISystems/ModalSystem.h"

#include "Sandbox/Modals/Modal.h"

#include <EventSystem/ApplicationEvents.h>

VT_REGISTER_SUBSYSTEM(ModalSystem, Default, Engine, 1);

ModalSystem::ModalSystem()
{
	VT_ENSURE(s_instance == nullptr);
	s_instance = this;
}

ModalSystem::~ModalSystem()
{
	s_instance = nullptr;
}

void ModalSystem::Initialize()
{
	RegisterEventListeners();
}

void ModalSystem::Shutdown()
{
}

void ModalSystem::RemoveModal(const UUID64& modalId)
{
	if (s_instance->m_modals.contains(modalId))
	{
		s_instance->m_modals.erase(modalId);
	}
}

void ModalSystem::RegisterEventListeners()
{
	auto isInitializedPred = [this]() { return s_instance != nullptr; };
	RegisterListener<Volt::AppImGuiUpdateEvent>(VT_BIND_EVENT_FN(ModalSystem::OnImGuiUpdate), isInitializedPred);
}

bool ModalSystem::OnImGuiUpdate(Volt::AppImGuiUpdateEvent& e)
{
	for (const auto& [modalId, modal] : s_instance->m_modals)
	{
		modal->Update();
	}

	return false;
}
