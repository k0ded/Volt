#pragma once

#include "Mosaic/Config.h"
#include "Mosaic/MosaicShaderWriter.h"

#include <CoreUtilities/FormatterExtension.h>
#include <CoreUtilities/Core.h>
#include <CoreUtilities/Containers/Graph.h>
#include <CoreUtilities/VoltGUID.h>
#include <CoreUtilities/Pointers/Ref.h>
#include <CoreUtilities/Pointers/Unique.h>

namespace Mosaic
{
	class MosaicNode;
	class MosaicEdge;

	class VTMOSAIC_API MosaicGraph
	{
	public:
		MosaicGraph();
		~MosaicGraph();

		void AddNode(const UUID64 uuid, const VoltGUID guid);
		void AddNode(const VoltGUID guid);

		uint32_t GetNextVariableIndex();
		uint32_t GetNextTextureIndex();
		const String GetNextVariableName();

		void ForfeitTextureIndex(uint32_t textureIndex);

		inline const uint32_t GetTextureCount() { return m_textureCount; }

		inline String& GetEditorState() { return m_editorState; }
		inline const String& GetEditorState() const { return m_editorState; }

		inline Graph<Ref<MosaicNode>, Ref<MosaicEdge>>& GetUnderlyingGraph() { return m_graph; }
		inline const Graph<Ref<MosaicNode>, Ref<MosaicEdge>>& GetUnderlyingGraph() const { return m_graph; }

		const MosaicShaderWriter Compile() const;
		void Clear();

		static Unique<MosaicGraph> CreateDefaultGraph();

		Graph<Ref<MosaicNode>, Ref<MosaicEdge>> m_graph;

	private:
		uint32_t m_currentVariableCount = 0;
		String m_editorState;

		Vector<uint32_t> m_availiableTextureIndices;

		uint32_t m_textureCount = 0;
		uint32_t m_currentTextureIndex = 0;
	};
}
