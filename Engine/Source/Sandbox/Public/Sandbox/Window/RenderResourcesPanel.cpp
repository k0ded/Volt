#include "sbpch.h"
#include "Sandbox/Window/RenderResourcesPanel.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Memory/Allocation.h>

#include <CoreUtilities/String/StringUtility.h>

using namespace Volt;

struct AllocationInfo
{
	String name;
	uint64_t size;
};

RenderResourcesPanel::RenderResourcesPanel()
	: EditorWindow("Render Resources")
{
}

void RenderResourcesPanel::UpdateMainContent()
{
	auto defaultAllocator = RHI::GraphicsContext::GetDefaultAllocator();

	if (ImGui::CollapsingHeader("Images"))
	{
		auto activeImageAllocations = defaultAllocator->GetActiveImageAllocations();

		Vector<AllocationInfo> allocInfos;
		allocInfos.reserve(activeImageAllocations.size());

		for (const auto& allocation : activeImageAllocations)
		{
			auto& allocInfo = allocInfos.emplace_back();
			allocInfo.name = allocation->GetName();
			allocInfo.size = allocation->GetMemoryRequirements().size;
		}

		std::sort(allocInfos.begin(), allocInfos.end(), [](const auto& lhs, const auto& rhs) { return lhs.size > rhs.size; });

		for (const auto& allocation : allocInfos)
		{
			const String sizeString = ::Utility::ToStringWithMetricPrefixCharacterForBytes(allocation.size);
			ImGui::Text("%s: %s", allocation.name.c_str(), sizeString.c_str());
		}
	}

	if (ImGui::CollapsingHeader("Buffers"))
	{
		auto activeBufferAllocations = defaultAllocator->GetActiveBufferAllocations();

		Vector<AllocationInfo> allocInfos;
		allocInfos.reserve(activeBufferAllocations.size());

		for (const auto& allocation : activeBufferAllocations)
		{
			auto& allocInfo = allocInfos.emplace_back();
			allocInfo.name = allocation->GetName();
			allocInfo.size = allocation->GetMemoryRequirements().size;
		}

		std::sort(allocInfos.begin(), allocInfos.end(), [](const auto& lhs, const auto& rhs) { return lhs.size > rhs.size; });

		for (const auto& allocation : allocInfos)
		{
			const String sizeString = ::Utility::ToStringWithMetricPrefixCharacterForBytes(allocation.size);
			ImGui::Text("%s: %s", allocation.name.c_str(), sizeString.c_str());
		}
	}
}
