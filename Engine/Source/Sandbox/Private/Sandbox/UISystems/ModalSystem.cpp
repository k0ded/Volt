#include "sbpch.h"
#include "UISystems/ModalSystem.h"

#include "Sandbox/Modals/Modal.h"

ModalSystem::ModalSystem()
{
	VT_ENSURE(s_instance == nullptr);
	s_instance = this;
}

ModalSystem::~ModalSystem()
{
	s_instance = nullptr;
}

void ModalSystem::Update()
{
	for (const auto& [modalId, modal] : s_instance->m_modals)
	{
		modal->Update();
	}
}

void ModalSystem::RemoveModal(const UUID64& modalId)
{
	if (s_instance->m_modals.contains(modalId))
	{
		s_instance->m_modals.erase(modalId);
	}
}
