#pragma once

#include "Circuit/Config.h"

#include <RHIModule/Descriptors/ResourceTable.h>

#include <WindowModule/WindowHandle.h>
#include <CoreUtilities/Core.h>
#include <CoreUtilities/Pointers/IntRef.h>

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
	class Window_New;
}

namespace Circuit
{
	class CIRCUIT_API CircuitRenderer
	{
	public:
		CircuitRenderer(CircuitWindow& targetCircuitWindow, IntRef<Volt::RHI::ResourceTable> resourceTable);
		~CircuitRenderer();

		void OnRender();

	private:
		void AddCircuitPrimitivesPass(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard);

		Circuit::CircuitWindow& m_targetCircuitWindow;
		Volt::Window_New& m_targetWindow;

		uint32_t m_width;
		uint32_t m_height;

		IntRef<Volt::RHI::Image> m_outputImage;
		IntRef<Volt::RHI::ResourceTable> m_resourceTable;

		std::atomic<uint64_t> m_frameTotalGPUAllocation;
	};
}
