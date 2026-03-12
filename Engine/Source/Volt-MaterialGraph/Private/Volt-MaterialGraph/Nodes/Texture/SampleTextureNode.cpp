#include "vtmgpch.h"

#include "Volt-MaterialGraph/Nodes/Texture/SampleTextureNode.h"

#include <RHIModule/Images/Image.h>

#include <Mosaic/MosaicGraph.h>
#include <Mosaic/NodeRegistry.h>
#include <Mosaic/MosaicShaderWriter.h>

#include <CoreUtilities/Archive/ArchiveVersionRegistry.h>

namespace Volt::MosaicNodes
{
	struct SampleTextureNodeCustomVersion
	{
		enum Type
		{
			BaseVersion = 0,
			AddedTextureType = 1,

			VersionPlusOne,
			LatestVersion = VersionPlusOne - 1
		};

		inline static constexpr VoltGUID guid = "{DAE8ACDF-30A8-4AD6-B00B-3E441F823B9E}"_guid;
	private:
		SampleTextureNodeCustomVersion() = default;
	};
	ArchiveVersionRegistrar g_registerSampleTextureNodeCustomVersion(SampleTextureNodeCustomVersion::guid, SampleTextureNodeCustomVersion::LatestVersion, "SampleTextureNodeCustomVersion");

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

	void SampleTextureNode::SerializeCustom(Archive& archive)
	{
		archive.UseVersion(SampleTextureNodeCustomVersion::guid);

		archive << m_textureHandle;
	
		if (!archive.IsLoading() || archive.GetVersion(SampleTextureNodeCustomVersion::guid) >= SampleTextureNodeCustomVersion::AddedTextureType)
		{
			archive << m_textureType;
		}
	}

	const Mosaic::ResultInfo SampleTextureNode::Compile(const GraphNode<Ref<class Mosaic::MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const
	{
		constexpr const char* colorTypeNodeStr = "const float2 {} = {}; \n"
												 "const float4 {} = {}.Sample({}, {} * {}); \n";

		constexpr const char* normalTypeNodeStr = "const float2 {} = {}; \n"
												  "const float4 {} = {}.Sample({}, {} * {}) * 2.f - 1.f; \n";

		if (m_evaluated)
		{
			Mosaic::ResultInfo tempInfo = m_evaluatedResultInfo;
			GetCorrectedVariableName(tempInfo, outputIndex);

			return tempInfo;
		}

		const std::string texSamplerVarName = "StaticAnisotropicSampler"; //m_graph->GetNextVariableName();
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

		std::string result;
		if (m_textureType == TextureType::Color)
		{
			result = std::format(colorTypeNodeStr, tilingVarName, tilingParamString, valueVarName, textureVarName, texSamplerVarName, texCoordsVarName, tilingVarName);
		}
		else if (m_textureType == TextureType::Normal)
		{
			result = std::format(normalTypeNodeStr, tilingVarName, tilingParamString, valueVarName, textureVarName, texSamplerVarName, texCoordsVarName, tilingVarName);
		}

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
