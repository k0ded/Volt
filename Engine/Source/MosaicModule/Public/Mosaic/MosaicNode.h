#pragma once

#include "Mosaic/Parameter.h"
#include "Mosaic/Config.h"

#include <CoreUtilities/VoltGUID.h>
#include <CoreUtilities/Containers/Graph.h>
#include <CoreUtilities/Pointers/Ref.h>
#include <CoreUtilities/Archive/Archive.h>

#include <glm/glm.hpp>

#define MOSAIC_NODE_DECLARE_GUID(guid) \
	VT_INLINE static VoltGUID GetStaticGUID() { return guid; } \
	VT_INLINE const VoltGUID GetGUID() const override { return GetStaticGUID(); }

namespace Mosaic
{
	// #TODO_Ivar: Move to another file
	class MosaicEdge 
	{
	public:
		MosaicEdge(uint32_t parameterInputIndex, uint32_t parameterOutputIndex)
			: m_parameterInputIndex(parameterInputIndex), m_parameterOutputIndex(parameterOutputIndex)
		{ }

		inline const uint32_t GetParameterInputIndex() const { return m_parameterInputIndex; }
		inline const uint32_t GetParameterOutputIndex() const { return m_parameterOutputIndex; }

	private:
		uint32_t m_parameterInputIndex = 0;
		uint32_t m_parameterOutputIndex = 0;
	};

	class MosaicGraph;
	class MosaicShaderWriter;

	class VTMOSAIC_API MosaicNode
	{
	public:
		MosaicNode(MosaicGraph* ownerGraph);
		virtual ~MosaicNode() {}

		virtual const String GetName() const = 0;
		virtual const String GetCategory() const = 0;
		virtual const glm::vec4 GetColor() const = 0;
		virtual const VoltGUID GetGUID() const = 0;
		virtual void Reset() {}

		virtual void SerializeCustom(Archive& archive) {}

		virtual const ResultInfo Compile(const GraphNode<Ref<class MosaicNode>, Ref<MosaicEdge>>& underlyingNode, uint32_t outputIndex, MosaicShaderWriter& shaderWriter) const = 0;

		inline const Vector<Parameter>& GetInputParameters() const { return m_inputParameters; }
		inline const Vector<Parameter>& GetOutputParameters() const { return m_outputParameters; }

		inline Vector<Parameter>& GetInputParameters() { return m_inputParameters; }
		inline Vector<Parameter>& GetOutputParameters() { return m_outputParameters; }

		Parameter& GetInputParameter(uint32_t parameterIndex);
		Parameter& GetOutputParameter(uint32_t parameterIndex);

		const Parameter& GetInputParameter(uint32_t parameterIndex) const;
		const Parameter& GetOutputParameter(uint32_t parameterIndex) const;

		inline String& GetEditorState() { return m_editorState; }

	protected:
		void AddInputParameter(const String& name, ValueBaseType baseType, uint32_t vectorSize, bool showAttribute);
		void AddOutputParameter(const String& name, ValueBaseType baseType, uint32_t vectorSize, bool showAttribute);

		template<typename T>
		void AddInputParameter(const String& name, ValueBaseType baseType, uint32_t vectorSize, const T& defaultValue, bool showAttribute);

		template<typename T>
		void AddOutputParameter(const String& name, ValueBaseType baseType, uint32_t vectorSize, const T& defaultValue, bool showAttribute);

		MosaicGraph* m_graph = nullptr;

	private:
		Vector<Parameter> m_inputParameters;
		Vector<Parameter> m_outputParameters;

		String m_editorState;
	};

	template<typename T>
	inline void MosaicNode::AddInputParameter(const String& name, ValueBaseType baseType, uint32_t vectorSize, const T& defaultValue, bool showAttribute)
	{
		auto& param = m_inputParameters.emplace_back();
		param.name = name;
		param.typeInfo.baseType = baseType;
		param.typeInfo.vectorSize = vectorSize;
		param.direction = ParameterDirection::Input;
		param.index = static_cast<uint32_t>(m_inputParameters.size() - 1);
		param.showAttribute = showAttribute;
		param.Get<T>() = defaultValue;

		param.serializationFunc = [](Archive& archive, Parameter& parameter)
		{
			T& data = parameter.Get<T>();
			archive << data;
		};
	}

	template<typename T>
	inline void MosaicNode::AddOutputParameter(const String& name, ValueBaseType baseType, uint32_t vectorSize, const T& defaultValue, bool showAttribute)
	{
		auto& param = m_outputParameters.emplace_back();
		param.name = name;
		param.typeInfo.baseType = baseType;
		param.typeInfo.vectorSize = vectorSize;
		param.direction = ParameterDirection::Output;
		param.index = static_cast<uint32_t>(m_outputParameters.size() - 1);
		param.showAttribute = showAttribute;
		param.Get<T>() = defaultValue;

		param.serializationFunc = [](Archive& archive, Parameter& parameter)
		{
			T& data = parameter.Get<T>();
			archive << data;
		};
	}
}
