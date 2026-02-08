#pragma once

#include <EntitySystem/EntityID.h>

enum class DebugRenderingLayer : uint8_t
{
	Foreground,
	ForegroundWorld,
	World
};

struct EditorDrawInterfaceUserData
{
	Volt::EntityID entityId;
	int32_t visProxyId;
	glm::vec4 color;
	DebugRenderingLayer layer;

	static glm::vec4 Pack(const EditorDrawInterfaceUserData& userData)
	{
		return
		{
			std::bit_cast<float>(userData.entityId),
			std::bit_cast<float>(userData.visProxyId),
			std::bit_cast<float>(glm::packUnorm4x8(userData.color)),
			std::bit_cast<float>(uint32_t(std::to_underlying(userData.layer)))
		};
	}

	static EditorDrawInterfaceUserData Unpack(const glm::vec4& userData)
	{
		return 
		{
			std::bit_cast<Volt::EntityID>(userData.x),
			std::bit_cast<int32_t>(userData.y),
			glm::unpackUnorm4x8(std::bit_cast<uint32_t>(userData.z)),
			static_cast<DebugRenderingLayer>(std::bit_cast<uint32_t>(userData.w))
		};
	}
};
