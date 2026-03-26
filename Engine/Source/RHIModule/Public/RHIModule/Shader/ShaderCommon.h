#pragma once

#include "RHIModule/Core/Core.h"

#include <CoreUtilities/String/StringHash.h>

#include <cstdint>
#include <unordered_map>

namespace Volt::RHI
{
	enum class ShaderStage : uint32_t
	{
		None = 0,
		Vertex = 0x00000001,
		Pixel = 0x00000010,
		Hull = 0x00000002,
		Domain = 0x00000004,
		Geometry = 0x00000008,
		Compute = 0x00000020,

		RayGen = 0x00000100,
		AnyHit = 0x00000200,
		ClosestHit = 0x00000400,
		Miss = 0x00000800,
		Intersection = 0x00001000,
		Callable = 0x00002000,

		Amplification = 0x00000040,
		Mesh = 0x00000080,

		// Note: Update when adding more shader stages.
		Num = 14,
		NumBindable = 9,

		All = Vertex | Pixel | Hull | Domain | Geometry | Compute,
		Common = Vertex | Pixel | Geometry | Compute
	};
	VT_SETUP_ENUM_CLASS_OPERATORS(ShaderStage);

	inline constexpr uint32_t GetNumShaderStages()
	{
		constexpr uint32_t NumShaderStages = static_cast<uint32_t>(ShaderStage::Num);
		return NumShaderStages;
	}

	inline constexpr uint32_t GetNumBindableShaderStages()
	{
		constexpr uint32_t NumShaderStages = static_cast<uint32_t>(ShaderStage::NumBindable);
		return NumShaderStages;
	}

	inline constexpr uint32_t GetNumMaxBoundShaderStages()
	{
		return 4;
	}

	inline constexpr uint32_t GetShaderStageIndex(ShaderStage shaderStage)
	{
		switch (shaderStage)
		{
			case ShaderStage::Vertex: return 0;
			case ShaderStage::Amplification: return 1;
			case ShaderStage::Mesh: return 2;
			case ShaderStage::Pixel: return 3;
			case ShaderStage::Compute: return 4;
			case ShaderStage::RayGen: return 5;
			case ShaderStage::Hull: return 6;
			case ShaderStage::Domain: return 7;
			case ShaderStage::Geometry: return 8;
		}

		return 0;
	}

	enum class ShaderRegisterType : uint8_t
	{
		CBV = 0,
		UAV,
		SRV,
		Sampler,
		Max
	};

	enum class ShaderResourceType : uint8_t
	{
		UniformBuffer,
		StructuredBuffer,
		TexelBuffer,
		Texture,
		Sampler,
		AccelerationStructure
	};

	enum class ShaderUniformBaseType : uint8_t
	{
		Invalid,
		Bool,

		Short,
		UShort,

		UInt,
		Int,

		Int64,
		UInt64,

		Double,
		Float,
		Half,

		Buffer,
		RWBuffer,
		UniformBuffer,

		Texture2D,
		RWTexture2D,

		Texture2DArray,
		RWTexture2DArray,

		TextureCube,

		Texture3D,
		RWTexture3D,

		Sampler
	};

	struct VTRHI_API ShaderUniformType
	{
		ShaderUniformBaseType baseType = ShaderUniformBaseType::Invalid;
		uint32_t vecsize = 1;
		uint32_t columns = 1;

		inline const size_t GetSize() const
		{
			size_t size = 0;

			switch (baseType)
			{
				case ShaderUniformBaseType::Bool: size = 4; break; // HLSL Bool size
				case ShaderUniformBaseType::UInt: size = 4; break;
				case ShaderUniformBaseType::Int: size = 4; break;
				case ShaderUniformBaseType::Float: size = 4; break;
				case ShaderUniformBaseType::Half: size = 2; break;
				case ShaderUniformBaseType::Short: size = 2; break;
				case ShaderUniformBaseType::UShort: size = 2; break;
				case ShaderUniformBaseType::Double: size = 8; break;
				case ShaderUniformBaseType::Int64: size = 8; break;
				case ShaderUniformBaseType::UInt64: size = 8; break;

				case ShaderUniformBaseType::Buffer: size = 4; break;
				case ShaderUniformBaseType::UniformBuffer: size = 4; break;
				case ShaderUniformBaseType::Texture2D: size = 4; break;
				case ShaderUniformBaseType::RWTexture2D: size = 4; break;
				case ShaderUniformBaseType::Texture2DArray: size = 4; break;
				case ShaderUniformBaseType::RWTexture2DArray: size = 4; break;
				case ShaderUniformBaseType::TextureCube: size = 4; break;
				case ShaderUniformBaseType::Texture3D: size = 4; break;
				case ShaderUniformBaseType::RWTexture3D: size = 4; break;
				case ShaderUniformBaseType::RWBuffer: size = 4; break;
				case ShaderUniformBaseType::Sampler: size = 4; break;

				case ShaderUniformBaseType::Invalid:
					break;
			}

			size = size * vecsize * columns;

			return size;
		}

		inline bool operator==(const ShaderUniformType& rhs) const
		{
			return baseType == rhs.baseType && vecsize == rhs.vecsize && columns == rhs.columns;
		}

		inline bool IsArithmeticType() const
		{
			switch (baseType)
			{
				case ShaderUniformBaseType::Bool:
				case ShaderUniformBaseType::Short:
				case ShaderUniformBaseType::UShort:
				case ShaderUniformBaseType::UInt:
				case ShaderUniformBaseType::Int:
				case ShaderUniformBaseType::Int64:
				case ShaderUniformBaseType::UInt64:
				case ShaderUniformBaseType::Double:
				case ShaderUniformBaseType::Float:
				case ShaderUniformBaseType::Half:
					return true;
			}

			return false;
		}

		friend Archive& operator<<(Archive& archive, ShaderUniformType& value)
		{
			archive << value.baseType;
			archive << value.vecsize;
			archive << value.columns;

			return archive;
		}
	};

	struct VTRHI_API ShaderUniform
	{
		ShaderUniform(ShaderUniformType type, size_t size, size_t offset);
		ShaderUniform() = default;
		~ShaderUniform() = default;

		ShaderUniformType type;

		size_t size = 0;
		size_t offset = 0;

		String name;

		friend Archive& operator<<(Archive& archive, ShaderUniform& value)
		{
			archive << value.type;
			archive << value.size;
			archive << value.offset;

			return archive;
		}
	};

	// Representation of shader types
	struct ShaderConstantBuffer
	{
		ShaderStage usageStages;
		size_t size = 0;
		uint32_t usageCount = 0;
	};

	struct ShaderStorageBuffer
	{
		ShaderStage usageStages;
		size_t size = 0;
		int32_t arraySize = 1; // -1 Means unsized array
		uint32_t usageCount = 0;
		bool isWrite = false;
	};

	struct ShaderStorageImage
	{
		ShaderStage usageStages;
		int32_t arraySize = 1; // -1 Means unsized array
		uint32_t usageCount = 0;
	};

	struct ShaderImage
	{
		ShaderStage usageStages;
		int32_t arraySize = 1; // -1 Means unsized array
		uint32_t usageCount = 0;
	};

	struct ShaderSampler
	{
		ShaderStage usageStages;
		uint32_t usageCount = 0;
	};
	/////////////////////////////////

	struct VTRHI_API ShaderResourceBinding
	{
		uint32_t set = std::numeric_limits<uint32_t>::max();
		uint32_t binding = std::numeric_limits<uint32_t>::max();
		uint32_t arraySize = 1;
		ShaderRegisterType registerType;
		ShaderResourceType resourceType;
		ShaderStage shaderStage;
		String name;

		inline const bool IsValid() const { return set != std::numeric_limits<uint32_t>::max() && binding != std::numeric_limits<uint32_t>::max(); }

		friend Archive& operator<<(Archive& archive, ShaderResourceBinding& value);
	};

	struct ShaderSourceEntry
	{
		String entryPoint = "main";
		RHI::ShaderStage shaderStage;
		Filesystem::Path filepath;
	};

	struct ShaderSourceInfo
	{
		ShaderSourceEntry sourceEntry;
		String source;
	};

	inline static uint32_t GetDescriptorSetIndexFromShaderStage(ShaderStage shaderStage)
	{
		switch (shaderStage)
		{
			case ShaderStage::Vertex: return 0;
			case ShaderStage::Amplification: return 1;
			case ShaderStage::Mesh: return 2;
			case ShaderStage::Pixel: return 3;
			case ShaderStage::Compute: return 4;
			case ShaderStage::RayGen: return 5;
			case ShaderStage::Hull: return 6;
			case ShaderStage::Domain: return 7;
			case ShaderStage::Geometry: return 8;
		}

		VT_ASSERT(false);
		return 0;
	}
}
