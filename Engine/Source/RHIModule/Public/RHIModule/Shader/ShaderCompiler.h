#pragma once

#include "RHIModule/Core/RHIInterface.h"

#include "RHIModule/Shader/BufferLayout.h"
#include "RHIModule/Shader/ShaderCommon.h"
#include "RHIModule/Shader/ShaderPermutationConfig.h"
#include "RHIModule/Shader/ShaderParameterMap.h"

#include "RHIModule/Core/RHICommon.h"

#include <CoreUtilities/Filesystem/Path.h>

namespace Volt::RHI
{
	class ShaderCache;

	enum class ShaderCompilerFlags : uint32_t
	{
		None = 0,
		WarningsAsErrors = BIT(0),
		OutputShaderDebugInfo = BIT(1)
	};

	VT_SETUP_ENUM_CLASS_OPERATORS(ShaderCompilerFlags);

	enum class ShaderOptimizationLevel : uint8_t
	{
		Disable = 0,
		Release,
		Dist,
	};

	struct ShaderCompilerCreateInfo
	{
		Vector<Filesystem::Path> includeDirectories;
		Vector<String> initialMacros;
	
		IntRef<ShaderCache> shaderCache;
		ShaderCompilerFlags flags = ShaderCompilerFlags::None;
		ShaderOptimizationLevel optimizationLevel = ShaderOptimizationLevel::Disable;
		Filesystem::Path shaderDebugInfoPath;
	};

	class VTRHI_API ShaderCompiler : public RHIInterface
	{
	public:
		enum class CompilationResult : uint32_t
		{
			Success = 0,
			PreprocessFailed,
			Failure
		};

		struct CompilationResultData
		{
			CompilationResult result = CompilationResult::Failure;
			Vector<uint32_t> shaderBinary;

			// Pixel Shader
			Vector<PixelFormat> outputFormats;

			// Vertex Shader
			BufferLayoutMap vertexLayout;
			BufferLayout instanceLayout;

			// Common
			ShaderParameterMap shaderParameterMap;
			Vector<Filesystem::Path> includeDependencies;

			VT_NODISCARD VT_INLINE bool IsValid() const { return !shaderBinary.empty(); }
		};

		struct Specification
		{
			ShaderSourceInfo shaderSourceInfo;
			ShaderPermutationConfig permutationConfig;
			ShaderOptimizationLevel optimizationLevel = ShaderOptimizationLevel::Disable;
			bool forceCompile;
		};

		virtual ~ShaderCompiler();

		VT_NODISCARD static CompilationResultData TryCompile(const Specification& specification);
		static void AddMacro(const String& macroName);
		static void RemoveMacro(StringView macroName);
		
		static IntRef<ShaderCompiler> Create(const ShaderCompilerCreateInfo& createInfo);

	protected:
		ShaderCompiler();

		// Should compile shader using shader source files, result is stored in shaders internal storage
		virtual CompilationResultData TryCompileImpl(const Specification& specification) = 0;
		virtual void AddMacroImpl(const String& macroName) = 0;
		virtual void RemoveMacroImpl(StringView macroName) = 0;

	private:
		inline static ShaderCompiler* s_instance = nullptr;
	};
}
