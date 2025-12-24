#include "vtassetspch.h"

#include "Volt-Assets/SourceAssetImporters/TextureCompression.h"


#include <RHIModule/Images/ImageUtility.h>

#include <CoreUtilities/Profiling/Profiling.h>

#include <DirectXTex.h>
#include <compressonator.h>

namespace Volt::TextureCompression
{
#if 1
	void Compress(uint32_t width, 
		uint32_t height, 
		RHI::PixelFormat srcFormat, 
		uint8_t* pixelData, 
		RHI::PixelFormat dstFormat, 
		bool generateMips, 
		bool convertToSRGBIfRequired, 
		Buffer& outPixelData,
		Vector<Volt::TextureSerializerCommon::TextureMip>& outTextureMips)
	{
		VT_PROFILE_FUNCTION();

		DXGI_FORMAT srcDXGIFormat = DXGI_FORMAT_UNKNOWN;
		if (srcFormat == RHI::PixelFormat::R8G8B8A8_UNORM)
		{
			srcDXGIFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else if (srcFormat == RHI::PixelFormat::R8G8B8A8_SRGB)
		{
			srcDXGIFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		}
		else if (srcFormat == RHI::PixelFormat::R16G16B16A16_SFLOAT)
		{
			srcDXGIFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		}
		else
		{
			VT_ENSURE(false);
		}

		const uint32_t formatTexelBlockSize = RHI::Utility::GetFormatTexelBlockSize(srcFormat);
		const uint32_t formatTexelsPerBlock = RHI::Utility::GetFormatTexelsPerBlock(srcFormat);

		DirectX::Image srcImage{};
		srcImage.width = width;
		srcImage.height = height;
		srcImage.format = srcDXGIFormat;
		srcImage.rowPitch = width * uint32_t(float(formatTexelBlockSize) / float(formatTexelsPerBlock));
		srcImage.slicePitch = srcImage.rowPitch * height;
		srcImage.pixels = pixelData;

		DirectX::ScratchImage* currentSrcScratch = nullptr;

		DirectX::ScratchImage src{};
		HRESULT hr = src.InitializeFromImage(srcImage);
		if (FAILED(hr))
		{
		}
		currentSrcScratch = &src;

		DirectX::ScratchImage srgbDst{};
		if (convertToSRGBIfRequired)
		{
			if (!RHI::Utility::IsSRGBFormat(srcFormat) && RHI::Utility::IsSRGBFormat(dstFormat))
			{
				VT_PROFILE_SCOPE("sRGB Conversion");

				// Since we R8G8B8A8_UNORM_SRGB is the only SRGB target we really support, we specify it as a constant here.
				constexpr DXGI_FORMAT DstSRGBFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
				srcDXGIFormat = DstSRGBFormat;
				hr = DirectX::Convert(src.GetImages(), src.GetImageCount(), src.GetMetadata(), DstSRGBFormat, DirectX::TEX_FILTER_DEFAULT, 0.f, srgbDst);
				if (FAILED(hr))
				{
				}

				currentSrcScratch = &srgbDst;
			}
		}

		DirectX::ScratchImage mipsDst{};
		if (generateMips)
		{
			VT_PROFILE_SCOPE("Generate Mips");
			hr = DirectX::GenerateMipMaps(currentSrcScratch->GetImages(), currentSrcScratch->GetImageCount(), currentSrcScratch->GetMetadata(), DirectX::TEX_FILTER_DEFAULT, 0, mipsDst);
			if (FAILED(hr))
			{

			}

			currentSrcScratch = &mipsDst;
		}

		DirectX::ScratchImage compressedDst;

		DXGI_FORMAT dstDXGIFormat = DXGI_FORMAT_UNKNOWN;
		switch (dstFormat)
		{
			case RHI::PixelFormat::BC7_SRGB_BLOCK: dstDXGIFormat = DXGI_FORMAT_BC7_UNORM_SRGB; break;
			case RHI::PixelFormat::BC7_UNORM_BLOCK: dstDXGIFormat = DXGI_FORMAT_BC7_UNORM; break;
			case RHI::PixelFormat::BC5_UNORM_BLOCK: dstDXGIFormat = DXGI_FORMAT_BC5_UNORM; break;
			case RHI::PixelFormat::BC3_SRGB_BLOCK: dstDXGIFormat = DXGI_FORMAT_BC3_UNORM_SRGB; break;
			case RHI::PixelFormat::BC3_UNORM_BLOCK: dstDXGIFormat = DXGI_FORMAT_BC3_UNORM; break;
			case RHI::PixelFormat::BC1_RGBA_SRGB_BLOCK: dstDXGIFormat = DXGI_FORMAT_BC1_UNORM_SRGB; break;
			case RHI::PixelFormat::BC1_RGBA_UNORM_BLOCK: dstDXGIFormat = DXGI_FORMAT_BC1_UNORM; break;
		
			default:
				VT_ENSURE(false);
		}

		{
			VT_PROFILE_SCOPE("Compress");
			DirectX::Compress(currentSrcScratch->GetImages(), currentSrcScratch->GetImageCount(), currentSrcScratch->GetMetadata(), dstDXGIFormat, DirectX::TEX_COMPRESS_DEFAULT, 1.f, compressedDst);
		}
		
		outPixelData.Resize(compressedDst.GetPixelsSize());
		outPixelData.Copy(compressedDst.GetPixels(), compressedDst.GetPixelsSize());

		size_t offset = 0;
		for (uint32_t i = 0; i < compressedDst.GetMetadata().mipLevels; ++i)
		{
			const DirectX::Image* mipImage = compressedDst.GetImage(i, 0, 0);
			Volt::TextureSerializerCommon::TextureMip& newMip = outTextureMips.emplace_back();
			newMip.width = static_cast<uint32_t>(mipImage->width);
			newMip.height = static_cast<uint32_t>(mipImage->height);
			newMip.dataOffset = offset;
			newMip.dataSize = mipImage->slicePitch;

			offset += mipImage->slicePitch;
		}
	}
#else
	CMP_FORMAT ToCMPFormat(RHI::PixelFormat format)
	{
		switch (format)
		{
			case RHI::PixelFormat::R8G8B8A8_UNORM:
			case RHI::PixelFormat::R8G8B8A8_SRGB:
				return CMP_FORMAT_RGBA_8888;
	
			case RHI::PixelFormat::R16G16B16A16_SFLOAT:
				return CMP_FORMAT_RGBA_16F;
	
			case RHI::PixelFormat::BC7_UNORM_BLOCK:
			case RHI::PixelFormat::BC7_SRGB_BLOCK:
				return CMP_FORMAT_BC7;
	
			case RHI::PixelFormat::BC5_UNORM_BLOCK:
				return CMP_FORMAT_BC5;
	
			case RHI::PixelFormat::BC3_UNORM_BLOCK:
			case RHI::PixelFormat::BC3_SRGB_BLOCK:
				return CMP_FORMAT_BC3;
	
			case RHI::PixelFormat::BC1_RGBA_UNORM_BLOCK:
			case RHI::PixelFormat::BC1_RGBA_SRGB_BLOCK:
				return CMP_FORMAT_BC1;
	
			default:
				VT_ENSURE_NO_ENTRY();
				return CMP_FORMAT_Unknown;
		}
	}

	ChannelFormat GetChannelFormatFromFormat(RHI::PixelFormat format)
	{
		switch (format)
		{
			case RHI::PixelFormat::R8G8B8A8_UNORM:
			case RHI::PixelFormat::R8G8B8A8_SRGB:
				return ChannelFormat::CF_8bit;

			case RHI::PixelFormat::R16G16B16A16_SFLOAT:
				return ChannelFormat::CF_Float16;
		}

		VT_ENSURE_NO_ENTRY();
		return ChannelFormat::CF_8bit;
	}

	void Compress(uint32_t width,
		uint32_t height,
		RHI::PixelFormat srcFormat,
		uint8_t* pixelData,
		RHI::PixelFormat dstFormat,
		bool generateMips,
		bool convertToSRGBIfRequired,
		uint32_t& outMipCount,
		Vector<uint8_t>& outPixelData)
	{
		VT_PROFILE_FUNCTION();

		CMP_InitFramework();

		const uint32_t formatTexelBlockSize = RHI::Utility::GetFormatTexelBlockSize(srcFormat);
		const uint32_t formatTexelsPerBlock = RHI::Utility::GetFormatTexelsPerBlock(srcFormat);



#if 0
		if (convertToSRGBIfRequired)
		{
			VT_PROFILE_SCOPE("sRGB Conversion");

			if (!RHI::Utility::IsSRGBFormat(srcFormat) && RHI::Utility::IsSRGBFormat(dstFormat))
			{
				CMP_ConvertMipTexture(&srcImage, &srcImage, )
			}
		}
#endif

		CMP_MipSet inputMipSet{};
		if (CMP_CreateMipSet(&inputMipSet, width, height, 1, GetChannelFormatFromFormat(srcFormat), TextureType::TT_2D) != CMP_OK)
		{
			VT_ENSURE_NO_ENTRY();
		}

		CMP_MipLevel* baseLevel = nullptr;
		CMP_GetMipLevel(&baseLevel, &inputMipSet, 0, 0);

		VT_ENSURE(baseLevel != nullptr);

		// Copy data to mip set.
		const size_t srcDataSize = height * width * uint32_t(float(formatTexelBlockSize) / float(formatTexelsPerBlock));
		memcpy_s(baseLevel->m_pbData, baseLevel->m_dwLinearSize, pixelData, srcDataSize);

		const uint32_t numMipLevels = CMP_CalcMaxMipLevel(inputMipSet.dwHeight, inputMipSet.dwWidth, true);

		if (generateMips)
		{
			VT_PROFILE_SCOPE("Generate Mips");

			const CMP_INT minSize = CMP_CalcMinMipSize(inputMipSet.dwHeight, inputMipSet.dwWidth, numMipLevels);
			CMP_GenerateMIPLevels(&inputMipSet, minSize);

			outMipCount = numMipLevels;
		}
		else
		{
			outMipCount = 1;
		}

		CMP_MipSet dstMipSet{};
		memset(&dstMipSet, 0, sizeof(CMP_MipSet));

		KernelOptions kernelOptions;
		memset(&kernelOptions, 0, sizeof(KernelOptions));

		kernelOptions.format = ToCMPFormat(dstFormat);
		kernelOptions.fquality = 0.8f;
		kernelOptions.threads = 0;
		kernelOptions.encodeWith = CMP_GPU_VLK;

		CMP_ERROR result = CMP_ProcessTexture(&inputMipSet, &dstMipSet, kernelOptions, nullptr);
		if (result != CMP_OK)
		{
			VT_ENSURE_NO_ENTRY();
		}

		CMP_FreeMipSet(&inputMipSet);
		CMP_FreeMipSet(&dstMipSet);
	}
#endif
}
