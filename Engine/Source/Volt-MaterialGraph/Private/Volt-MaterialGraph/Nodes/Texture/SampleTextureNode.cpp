#include "vtmgpch.h"

#include "Volt-MaterialGraph/Nodes/Texture/SampleTextureNode.h"
//#include "Volt/Utility/UIUtility.h"

#include <RHIModule/Images/Image.h>

#include <Mosaic/MosaicGraph.h>
#include <Mosaic/NodeRegistry.h>
#include <Mosaic/MosaicShaderWriter.h>

namespace Volt::MosaicNodes
{
	static void GetCorrectedVariableName(Mosaic::ResultInfo& resultInfo, uint32_t outputIndex)
	{
		if (outputIndex == 0)
		{
			resultInfo.resultParamName += ".rgb";
			resultInfo.resultType.vectorSize = 3;
		}
		else if (outputIndex == 1)
		{
			resultInfo.resultParamName += ".r";
		}
		else if (outputIndex == 2)
		{
			resultInfo.resultParamName += ".g";
		}
		else if (outputIndex == 3)
		{
			resultInfo.resultParamName += ".b";
		}
		else if (outputIndex == 4)
		{
			resultInfo.resultParamName += ".a";
		}
		else if (outputIndex == 5)
		{
			resultInfo.resultType.vectorSize = 4;
		}
	}

	SampleTextureNode::SampleTextureNode(Mosaic::MosaicGraph* ownerGraph)
		: Mosaic::MosaicNode(ownerGraph)
	{
		AddInputParameter("UV", Mosaic::ValueBaseType::Float, 2, false);
		AddInputParameter("Tiling", Mosaic::ValueBaseType::Float, 2, glm::vec2(1.f), false);
	
		AddOutputParameter("RGB", Mosaic::ValueBaseType::Float, 3, false);
		AddOutputParameter("R", Mosaic::ValueBaseType::Float, 1, false);
		AddOutputParameter("G", Mosaic::ValueBaseType::Float, 1, false);
		AddOutputParameter("B", Mosaic::ValueBaseType::Float, 1, false);
		AddOutputParameter("A", Mosaic::ValueBaseType::Float, 1, false);
		AddOutputParameter("RGBA", Mosaic::ValueBaseType::Float, 4, false);
	
		if (m_graph)
		{
			m_textureIndex = m_graph->GetNextTextureIndex();
		}
	}

	SampleTextureNode::~SampleTextureNode()
	{
		if (m_graph)
		{
			m_graph->ForfeitTextureIndex(m_textureIndex);
		}
	}

	void SampleTextureNode::Reset()
	{
		m_evaluated = false;
	}

	void SampleTextureNode::SerializeCustom(YAMLStreamWriter& streamWriter) const
	{
		streamWriter.SetKey("textureHandle", m_textureHandle);
	}

	void SampleTextureNode::DeserializeCustom(YAMLStreamReader& streamReader)
	{
		m_textureHandle = streamReader.ReadAtKey("textureHandle", Asset::Null());
	}

	const Mosaic::ResultInfo SampleTextureNode::Compile(const GraphNode<Ref<class Mosaic::MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const
	{
		constexpr const char* nodeStr = "const float2 {} = {}; \n"
										"const float4 {} = {}.Sample({}, {} * {}); \n";

		if (m_evaluated)
		{
			Mosaic::ResultInfo tempInfo = m_evaluatedResultInfo;
			GetCorrectedVariableName(tempInfo, outputIndex);

			return tempInfo;
		}

		const std::string texSamplerVarName = "TextureSamplerState"; //m_graph->GetNextVariableName();
		const std::string textureVarName = shaderWriter.AddTexture(m_textureIndex);
		const std::string valueVarName = m_graph->GetNextVariableName();
		const std::string tilingVarName = m_graph->GetNextVariableName();

		std::string texCoordsVarName = "evalData.texCoords";
		std::string tilingParamString = std::format("{}", GetInputParameter(1).Get<glm::vec2>());

		for (const auto& edgeId : underlyingNode.GetInputEdges())
		{
			const auto& edge = underlyingNode.GetEdgeFromID(edgeId);
			const uint32_t paramIndex = edge.metaDataType->GetParameterInputIndex();
			const auto& node = underlyingNode.GetNodeFromID(edge.startNode);

			const Mosaic::ResultInfo info = node.nodeData->Compile(node, edge.metaDataType->GetParameterOutputIndex(), shaderWriter);

			// UV
			if (paramIndex == 0)
			{
				texCoordsVarName = info.resultParamName;
			}
			// Tiling
			else if (paramIndex == 1)
			{
				tilingParamString = info.resultParamName;
			}
		}

		std::string result = std::format(nodeStr, tilingVarName, tilingParamString, valueVarName, textureVarName, texSamplerVarName, texCoordsVarName, tilingVarName);
		shaderWriter.AppendCodeBlock(result);

		Mosaic::ResultInfo resultInfo{};
		resultInfo.resultParamName = valueVarName;
		resultInfo.resultType = Mosaic::TypeInfo{ Mosaic::ValueBaseType::Float, 1 };

		m_evaluated = true;
		m_evaluatedResultInfo = resultInfo;

		GetCorrectedVariableName(resultInfo, outputIndex);

		return resultInfo;
	}

	const TextureInfo SampleTextureNode::GetTextureInfo() const
	{
		return { m_textureIndex, m_textureHandle };
	}

	REGISTER_NODE(SampleTextureNode);
}
