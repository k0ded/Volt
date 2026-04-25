#pragma once

#include "RHIModule/Graphics/GraphicsContext.h"
#include "RHIModule/Shader/Shader.h"

#include <filesystem>
#include <fstream>

namespace Volt::RHI
{
	namespace Utility
	{
		inline ShaderStage GetShaderStageFromFilename(const String& filename)
		{
			if (filename.find("_fs.glsl") != String::npos || filename.find("_ps.hlsl") != String::npos)
			{
				return ShaderStage::Pixel;
			}
			else if (filename.find("_vs.glsl") != String::npos || filename.find("_vs.hlsl") != String::npos)
			{
				return ShaderStage::Vertex;
			}
			else if (filename.find("_cs.glsl") != String::npos || filename.find("_cs.hlsl") != String::npos)
			{
				return ShaderStage::Compute;
			}
			else if (filename.find("_gs.glsl") != String::npos || filename.find("_gs.hlsl") != String::npos)
			{
				return ShaderStage::Geometry;
			}
			else if (filename.find("_hs.glsl") != String::npos || filename.find("_hs.hlsl") != String::npos)
			{
				return ShaderStage::Hull;
			}
			else if (filename.find("_ds.glsl") != String::npos || filename.find("_ds.hlsl") != String::npos)
			{
				return ShaderStage::Domain;
			}
			else if (filename.find("_rgen.glsl") != String::npos || filename.find("_rgen.hlsl") != String::npos)
			{
				return ShaderStage::RayGen;
			}
			else if (filename.find("_rmiss.glsl") != String::npos || filename.find("_rmiss.hlsl") != String::npos)
			{
				return ShaderStage::Miss;
			}
			else if (filename.find("_rchit.glsl") != String::npos || filename.find("_rchit.hlsl") != String::npos)
			{
				return ShaderStage::ClosestHit;
			}
			else if (filename.find("_rahit.glsl") != String::npos || filename.find("_rahit.hlsl") != String::npos)
			{
				return ShaderStage::AnyHit;
			}
			else if (filename.find("_rinter.hlsl") != String::npos)
			{
				return ShaderStage::Intersection;
			}
			else if (filename.find("_as.hlsl") != String::npos)
			{
				return ShaderStage::Amplification;
			}
			else if (filename.find("_ms.hlsl") != String::npos)
			{
				return ShaderStage::Mesh;
			}

			return ShaderStage::None;
		}

		inline static const wchar_t* HLSLShaderProfile(const ShaderStage stage)
		{
			switch (stage)
			{
				case ShaderStage::Vertex:		return L"vs_6_6";
				case ShaderStage::Pixel:		return L"ps_6_6";
				case ShaderStage::Hull:			return L"hs_6_6";
				case ShaderStage::Domain:		return L"hs_6_6";
				case ShaderStage::Geometry:		return L"gs_6_6";
				case ShaderStage::Compute:		return L"cs_6_6";
					
				case ShaderStage::RayGen:		return L"lib_6_6";
				case ShaderStage::Miss:			return L"lib_6_6";
				case ShaderStage::ClosestHit:   return L"lib_6_6";
				case ShaderStage::AnyHit:		return L"lib_6_6";
				case ShaderStage::Intersection: return L"lib_6_6";
			
				case ShaderStage::Amplification: return L"as_6_6";
				case ShaderStage::Mesh:			 return L"ms_6_6";
			}

			VT_ASSERT(false);
			return L"";
		}

		inline String StageToString(ShaderStage stage)
		{
			switch (stage)
			{
				case ShaderStage::Vertex: return "Vertex";
				case ShaderStage::Hull: return "Hull";
				case ShaderStage::Domain: return "Domain";
				case ShaderStage::Geometry: return "Geometry";
				case ShaderStage::Pixel: return "Fragment";
				case ShaderStage::Compute: return "Compute";
				
				case ShaderStage::RayGen: return "Ray Generation";
				case ShaderStage::Miss: return "Miss";
				case ShaderStage::ClosestHit: return "Close Hit";
				case ShaderStage::AnyHit: return "Any Hit";
			
				case ShaderStage::Mesh: return "Mesh";
				case ShaderStage::Amplification: return "Amplification";
			}

			return "Unsupported";
		}
	}
}
