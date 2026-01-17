#pragma once

#include <Mosaic/MosaicNode.h>
#include <Mosaic/MosaicGraph.h>
#include <Mosaic/MosaicHelpers.h>
#include <Mosaic/MosaicShaderWriter.h>
#include <Mosaic/NodeRegistry.h>

#include <CoreUtilities/FormatterExtension.h>

namespace Volt::MosaicNodes
{
	template<typename ValueType, ValueType DEFAULT_VALUE, Mosaic::ValueBaseType BASE_TYPE, VoltGUID GUID>
	class MakeVec2Node : public Mosaic::MosaicNode
	{
	public:
		MakeVec2Node(Mosaic::MosaicGraph* ownerGraph)
			: Mosaic::MosaicNode(ownerGraph)
		{
			AddInputParameter("R", BASE_TYPE, 1, DEFAULT_VALUE, true);
			AddInputParameter("G", BASE_TYPE, 1, DEFAULT_VALUE, true);

			AddOutputParameter("Result", BASE_TYPE, 2, DEFAULT_VALUE, false);
		}

		MOSAIC_NODE_DECLARE_GUID(GUID);

		inline const std::string GetName() const override { return "Make " + Mosaic::Helpers::GetTypeNameFromTypeInfo(TYPE_INFO); }
		inline const std::string GetCategory() const override { return "Conversion"; }
		inline const glm::vec4 GetColor() const override { return 1.f; }

		inline const Mosaic::ResultInfo Compile(const GraphNode<Ref<class Mosaic::MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const override
		{
			constexpr const char* nodeStr = "const {} {} = {}({}, {});\n";

			std::string R = std::to_string(GetInputParameter(0).Get<ValueType>());
			std::string G = std::to_string(GetInputParameter(1).Get<ValueType>());

			for (const auto& edgeId : underlyingNode.GetInputEdges())
			{
				const auto edge = underlyingNode.GetEdgeFromID(edgeId);
				const uint32_t paramIndex = edge.metaDataType->GetParameterInputIndex();

				const auto& node = underlyingNode.GetNodeFromID(edge.startNode);
				const Mosaic::ResultInfo info = node.nodeData->Compile(node, edge.metaDataType->GetParameterOutputIndex(), shaderWriter);

				if (paramIndex == 0)
				{
					R = info.resultParamName;
				}
				else if (paramIndex == 1)
				{
					G = info.resultParamName;
				}
			}

			const std::string varName = m_graph->GetNextVariableName();

			const auto typeString = Mosaic::Helpers::GetTypeNameFromTypeInfo(TYPE_INFO);
			std::string result = std::format(nodeStr, typeString, varName, typeString, R, G);
			shaderWriter.AppendCodeBlock(result);

			Mosaic::ResultInfo resultInfo{};
			resultInfo.resultParamName = varName;
			resultInfo.resultType = TYPE_INFO;

			return resultInfo;
		}

	private:
		inline static constexpr Mosaic::TypeInfo TYPE_INFO{ BASE_TYPE, 2 };
	};

	template<typename ValueType, ValueType DEFAULT_VALUE, Mosaic::ValueBaseType BASE_TYPE, VoltGUID GUID>
	class MakeVec3Node : public Mosaic::MosaicNode
	{
	public:
		MakeVec3Node(Mosaic::MosaicGraph* ownerGraph)
			: Mosaic::MosaicNode(ownerGraph)
		{
			AddInputParameter("R", BASE_TYPE, 1, DEFAULT_VALUE, true);
			AddInputParameter("G", BASE_TYPE, 1, DEFAULT_VALUE, true);
			AddInputParameter("B", BASE_TYPE, 1, DEFAULT_VALUE, true);
		
			AddOutputParameter("Result", BASE_TYPE, 3, DEFAULT_VALUE, false);
		}

		MOSAIC_NODE_DECLARE_GUID(GUID);

		inline const std::string GetName() const override { return "Make " + Mosaic::Helpers::GetTypeNameFromTypeInfo(TYPE_INFO); }
		inline const std::string GetCategory() const override { return "Conversion"; }
		inline const glm::vec4 GetColor() const override { return 1.f; }

		inline const Mosaic::ResultInfo Compile(const GraphNode<Ref<class Mosaic::MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const override
		{
			constexpr const char* nodeStr = "const {0} {1} = {2}({3}, {4}, {5});\n";
		
			std::string R = std::to_string(GetInputParameter(0).Get<ValueType>());
			std::string G = std::to_string(GetInputParameter(1).Get<ValueType>());
			std::string B = std::to_string(GetInputParameter(2).Get<ValueType>());

			for (const auto& edgeId : underlyingNode.GetInputEdges())
			{
				const auto edge = underlyingNode.GetEdgeFromID(edgeId);
				const uint32_t paramIndex = edge.metaDataType->GetParameterInputIndex();

				const auto& node = underlyingNode.GetNodeFromID(edge.startNode);
				const Mosaic::ResultInfo info = node.nodeData->Compile(node, edge.metaDataType->GetParameterOutputIndex(), shaderWriter);

				if (paramIndex == 0)
				{
					R = info.resultParamName;
				}
				else if (paramIndex == 1)
				{
					G = info.resultParamName;
				}
				else if (paramIndex == 2)
				{
					B = info.resultParamName;
				}
			}

			const std::string varName = m_graph->GetNextVariableName();
			
			const auto typeString = Mosaic::Helpers::GetTypeNameFromTypeInfo(TYPE_INFO);
			std::string result = std::format(nodeStr, typeString, varName, typeString, R, G, B);
			shaderWriter.AppendCodeBlock(result);

			Mosaic::ResultInfo resultInfo{};
			resultInfo.resultParamName = varName;
			resultInfo.resultType = TYPE_INFO;

			return resultInfo;
		}

	private:
		inline static constexpr Mosaic::TypeInfo TYPE_INFO{ BASE_TYPE, 3 };
	};

	template<typename ValueType, ValueType DEFAULT_VALUE, Mosaic::ValueBaseType BASE_TYPE, VoltGUID GUID>
	class MakeVec4Node : public Mosaic::MosaicNode
	{
	public:
		MakeVec4Node(Mosaic::MosaicGraph* ownerGraph)
			: Mosaic::MosaicNode(ownerGraph)
		{
			AddInputParameter("R", BASE_TYPE, 1, DEFAULT_VALUE, true);
			AddInputParameter("G", BASE_TYPE, 1, DEFAULT_VALUE, true);
			AddInputParameter("B", BASE_TYPE, 1, DEFAULT_VALUE, true);
			AddInputParameter("A", BASE_TYPE, 1, DEFAULT_VALUE, true);

			AddOutputParameter("Result", BASE_TYPE, 4, DEFAULT_VALUE, false);
		}

		MOSAIC_NODE_DECLARE_GUID(GUID);

		inline const std::string GetName() const override { return "Make " + Mosaic::Helpers::GetTypeNameFromTypeInfo(TYPE_INFO); }
		inline const std::string GetCategory() const override { return "Conversion"; }
		inline const glm::vec4 GetColor() const override { return 1.f; }

		inline const Mosaic::ResultInfo Compile(const GraphNode<Ref<class Mosaic::MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const override
		{
			constexpr const char* nodeStr = "const {0} {1} = {2}({3}, {4}, {5}, {6});\n";

			std::string R = std::to_string(GetInputParameter(0).Get<ValueType>());
			std::string G = std::to_string(GetInputParameter(1).Get<ValueType>());
			std::string B = std::to_string(GetInputParameter(2).Get<ValueType>());
			std::string A = std::to_string(GetInputParameter(3).Get<ValueType>());

			for (const auto& edgeId : underlyingNode.GetInputEdges())
			{
				const auto edge = underlyingNode.GetEdgeFromID(edgeId);
				const uint32_t paramIndex = edge.metaDataType->GetParameterInputIndex();

				const auto& node = underlyingNode.GetNodeFromID(edge.startNode);
				const Mosaic::ResultInfo info = node.nodeData->Compile(node, edge.metaDataType->GetParameterOutputIndex(), shaderWriter);

				if (paramIndex == 0)
				{
					R = info.resultParamName;
				}
				else if (paramIndex == 1)
				{
					G = info.resultParamName;
				}
				else if (paramIndex == 2)
				{
					B = info.resultParamName;
				}
				else if (paramIndex == 3)
				{
					A = info.resultParamName;
				}
			}

			const std::string varName = m_graph->GetNextVariableName();

			const auto typeString = Mosaic::Helpers::GetTypeNameFromTypeInfo(TYPE_INFO);
			std::string result = std::format(nodeStr, typeString, varName, typeString, R, G, B, A);
			shaderWriter.AppendCodeBlock(result);

			Mosaic::ResultInfo resultInfo{};
			resultInfo.resultParamName = varName;
			resultInfo.resultType = TYPE_INFO;

			return resultInfo;
		}

	private:
		inline static constexpr Mosaic::TypeInfo TYPE_INFO{ BASE_TYPE, 4 };
	};

	DECLARE_NODE_TEMPLATE(ConversionMakeFloat2, (MakeVec2Node<float, 0.f, Mosaic::ValueBaseType::Float, "{0205F058-8303-4854-9625-89D4D9C1BC0D}"_guid>));
	DECLARE_NODE_TEMPLATE(ConversionMakeFloat3, (MakeVec3Node<float, 0.f, Mosaic::ValueBaseType::Float, "{D7F668BB-AB49-431F-BD4A-A1CA4C623EF2}"_guid>));
	DECLARE_NODE_TEMPLATE(ConversionMakeFloat4, (MakeVec4Node<float, 0.f, Mosaic::ValueBaseType::Float, "{FC283A78-CF51-4651-9835-ACFE527FD07E}"_guid>));
}
