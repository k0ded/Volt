#pragma once

#include "RHIModule/Core/RHIInterface.h"

#include "RHIModule/Shader/BufferLayout.h"
#include "RHIModule/Shader/ShaderCommon.h"
#include "RHIModule/Shader/ShaderPermutationConfig.h"
#include "RHIModule/Shader/ShaderParameterMap.h"

#include "RHIModule/Core/RHICommon.h"

#include <filesystem>

namespace Volt::RHI
{
	class ShaderCache;

	enum class ShaderCompilerFlags : uint32_t
	{
		None = 0,
		WarningsAsErrors = BIT(0),
	};

	VT_SETUP_ENUM_CLASS_OPERATORS(ShaderCompilerFlags);

	struct ShaderCompilerCreateInfo
	{
		Vector<std::filesystem::path> includeDirectories;
		Vector<std::string> initialMacros;
	
		RefPtr<ShaderCache> shaderCache;
		ShaderCompilerFlags flags = ShaderCompilerFlags::None;
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

		enum class OptimizationLevel : uint32_t
		{
			Disable,
			Release,
			Dist,
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
			Vector<std::filesystem::path> includeDependencies;

			VT_NODISCARD VT_INLINE bool IsValid() const { return !shaderBinary.empty(); }
		};

		struct Specification
		{
			ShaderSourceInfo shaderSourceInfo;
			ShaderPermutationConfig permutationConfig;
			OptimizationLevel optimizationLevel = OptimizationLevel::Dist;
			bool forceCompile;
		};

		virtual ~ShaderCompiler();

		VT_NODISCARD static CompilationResultData TryCompile(const Specification& specification);
		static void AddMacro(const std::string& macroName);
		static void RemoveMacro(std::string_view macroName);
		
		static RefPtr<ShaderCompiler> Create(const ShaderCompilerCreateInfo& createInfo);

	protected:
		ShaderCompiler();

		// Should compile shader using shader source files, result is stored in shaders internal storage
		virtual CompilationResultData TryCompileImpl(const Specification& specification) = 0;
		virtual void AddMacroImpl(const std::string& macroName) = 0;
		virtual void RemoveMacroImpl(std::string_view macroName) = 0;

	private:
		inline static ShaderCompiler* s_instance = nullptr;
	};
}
