#pragma once

#include "Circuit/Config.h"

#include <RHIModule/Descriptors/ResourceTable.h>

#include <WindowModule/WindowHandle.h>
#include <CoreUtilities/Core.h>
#include <CoreUtilities/Pointers/RefPtr.h>

namespace Circuit
{
	class CircuitWindow;
}

struct CircuitOutputData;
namespace Volt
{
	namespace RHI
	{
		class Image;
	}

	class RenderGraph;
	class RenderGraphBlackboard;
	class Window;
}

namespace Circuit
{
	class CIRCUIT_API CircuitRenderer
	{
	public:
		CircuitRenderer(CircuitWindow& targetCircuitWindow, RefPtr<Volt::RHI::ResourceTable> resourceTable);
		~CircuitRenderer();

		void OnRender();

	private:
		void AddCircuitPrimitivesPass(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard);

		Circuit::CircuitWindow& m_targetCircuitWindow;
		Volt::Window& m_targetWindow;

		uint32_t m_width;
		uint32_t m_height;

		RefPtr<Volt::RHI::Image> m_outputImage;
		RefPtr<Volt::RHI::ResourceTable> m_resourceTable;

		std::atomic<uint64_t> m_frameTotalGPUAllocation;
	};
}
