#include "vtassetspch.h"
#include "Volt-Assets/SourceAssetImporters/DDSTextureSourceImporter.h"
#include "Volt-Assets/SourceAssetImporters/ImportConfigs.h"

#include <Volt-Renderer/Texture/Texture2D.h>

#include <RenderCore/CommandBufferPool.h>

#include <RHIModule/Images/Image.h>
#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Buffers/CommandBufferUtility.h>
#include <RHIModule/Utility/ResourceUtility.h>

#include <AssetSystem/AssetManager_New.h>

#include <CoreUtilities/Profiling/Profiling.h>

#define TINYDDSLOADER_IMPLEMENTATION
#include <tinyddsloader.h>
#include <AssetSystem/AssetLocks.h>

namespace tdl = tinyddsloader;

VT_DEFINE_LOG_CATEGORY(LogDDSTextureSourceImporter);

namespace Volt
{
	VT_REGISTER_SOURCE_ASSET_IMPORTER(({ ".dds", ".DDS" }), DDSTextureSourceImporter);

	inline std::string GetDDSError(tdl::Result code, const std::filesystem::path& filepath)
	{
		switch (code)
		{
			case tinyddsloader::ErrorFileOpen: return std::format("Failed to open file {}", filepath);
			case tinyddsloader::ErrorRead: return std::format("Failed to read file {}", filepath);
			case tinyddsloader::ErrorMagicWord: return std::format("Failed to read magic word in file {}", filepath);
			case tinyddsloader::ErrorSize: return std::format("File {} is not large enough to have any DDS data", filepath);
			case tinyddsloader::ErrorVerify: return std::format("Failed to verify file {}", filepath);
			case tinyddsloader::ErrorNotSupported: return std::format("File {} is not supported by importer", filepath);
			case tinyddsloader::ErrorInvalidData: return std::format("File {} contains invalid data", filepath);
		}

		return "";
	}

	inline RHI::PixelFormat DDSToImageFormat(tdl::DDSFile::DXGIFormat format)
	{
		switch (format)
		{
			case tdl::DDSFile::DXGIFormat::R8_UNorm: return RHI::PixelFormat::R8_UNORM;
			case tdl::DDSFile::DXGIFormat::R8_SNorm: return RHI::PixelFormat::R8_SNORM;

			case tdl::DDSFile::DXGIFormat::R32G32B32A32_Float: return RHI::PixelFormat::R32G32B32A32_SFLOAT;
			case tdl::DDSFile::DXGIFormat::R16G16B16A16_Float: return RHI::PixelFormat::R16G16B16A16_SFLOAT;

			case tdl::DDSFile::DXGIFormat::R32G32_Float: return RHI::PixelFormat::R32G32_SFLOAT;
			case tdl::DDSFile::DXGIFormat::R16G16_Float: return RHI::PixelFormat::R16G16_SFLOAT;

			case tdl::DDSFile::DXGIFormat::R8G8B8A8_UNorm: return RHI::PixelFormat::R8G8B8A8_UNORM;
			case tdl::DDSFile::DXGIFormat::R8G8B8A8_UNorm_SRGB: return RHI::PixelFormat::R8G8B8A8_SRGB;

			case tdl::DDSFile::DXGIFormat::R32_Float: return RHI::PixelFormat::R32_SFLOAT;
			case tdl::DDSFile::DXGIFormat::R32_UInt: return RHI::PixelFormat::R32_UINT;
			case tdl::DDSFile::DXGIFormat::R32_SInt: return RHI::PixelFormat::R32_SINT;

			case tdl::DDSFile::DXGIFormat::BC1_UNorm: return RHI::PixelFormat::BC1_RGB_UNORM_BLOCK;
			case tdl::DDSFile::DXGIFormat::BC1_UNorm_SRGB: return RHI::PixelFormat::BC1_RGB_SRGB_BLOCK;

			case tdl::DDSFile::DXGIFormat::BC2_UNorm: return RHI::PixelFormat::BC2_UNORM_BLOCK;
			case tdl::DDSFile::DXGIFormat::BC2_UNorm_SRGB: return RHI::PixelFormat::BC2_SRGB_BLOCK;

			case tdl::DDSFile::DXGIFormat::BC3_UNorm: return RHI::PixelFormat::BC3_UNORM_BLOCK;
			case tdl::DDSFile::DXGIFormat::BC3_UNorm_SRGB: return RHI::PixelFormat::BC3_SRGB_BLOCK;

			case tdl::DDSFile::DXGIFormat::BC4_UNorm: return RHI::PixelFormat::BC4_UNORM_BLOCK;

			case tdl::DDSFile::DXGIFormat::BC5_UNorm: return RHI::PixelFormat::BC5_UNORM_BLOCK;

			case tdl::DDSFile::DXGIFormat::BC7_UNorm: return RHI::PixelFormat::BC7_UNORM_BLOCK;
			case tdl::DDSFile::DXGIFormat::BC7_UNorm_SRGB: return RHI::PixelFormat::BC7_SRGB_BLOCK;
		}

		return RHI::PixelFormat::R8G8B8A8_UNORM;
	}

	Vector<AssetReference<Asset_New>> DDSTextureSourceImporter::ImportInternal(const std::filesystem::path& filepath, const void* config, const SourceAssetUserImportData& userData) const
	{
		VT_PROFILE_FUNCTION();
		const TextureSourceImportConfig& importConfig = *reinterpret_cast<const TextureSourceImportConfig*>(config);

		tdl::DDSFile ddsFile;
		auto returnCode = ddsFile.Load(filepath.string().c_str());
		if (returnCode != tdl::Result::Success)
		{
			const std::string error = std::format("Failed to import DDS file: {}!", GetDDSError(returnCode, filepath));

			VT_LOGC(Error, LogDDSTextureSourceImporter, error);
			userData.OnError(error);
			return {};
		}

		if (ddsFile.GetTextureDimension() != tdl::DDSFile::TextureDimension::Texture2D)
		{
			const std::string error = std::format("Failed to import DDS file {}: Only 2D textures are currently supported!", filepath);

			VT_LOGC(Error, LogDDSTextureSourceImporter, error);
			userData.OnError(error);
			return {};
		}

		auto imageData = ddsFile.GetImageData();

		const uint32_t width = imageData->m_width;
		const uint32_t height = imageData->m_height;

		RefPtr<RHI::Image> image;

		const uint32_t mipLevelCount = importConfig.importMipMaps ? ddsFile.GetMipCount() : 1u;

		// Create image
		{
			RHI::ImageDesc specification{};
			specification.format = DDSToImageFormat(ddsFile.GetFormat());
			specification.usage = RHI::ImageUsage::Texture;
			specification.width = width;
			specification.height = height;
			specification.mips = mipLevelCount;
			specification.generateMips = mipLevelCount == 1 && importConfig.generateMipMaps;
			specification.debugName = importConfig.destinationFilename;

			image = RHI::Image::Create(specification);
		}

		RHI::ImageCopyData copyData{};

		for (uint32_t i = 0; i < mipLevelCount; i++)
		{
			auto mipData = ddsFile.GetImageData(i);

			auto& subData = copyData.copySubData.emplace_back();
			subData.data = mipData->m_mem;
			subData.rowPitch = mipData->m_memPitch;
			subData.slicePitch = mipData->m_memSlicePitch;
			subData.width = mipData->m_width;
			subData.height = mipData->m_height;
			subData.depth = mipData->m_depth;
			subData.subResource.baseArrayLayer = 0;
			subData.subResource.baseMipLevel = i;
			subData.subResource.layerCount = 1;
			subData.subResource.levelCount = 1;
		}

		RHI::BufferDesc stagingDesc{};
		stagingDesc.count = 1;
		stagingDesc.elementSize = RHI::GraphicsContext::GetDevice()->GetMaxRequiredStagingBufferSizeForImage(image);
		stagingDesc.usage = RHI::BufferUsage::StorageBuffer | RHI::BufferUsage::TransferSrc;
		stagingDesc.memoryUsage = RHI::MemoryUsage::CPUToGPU;
		stagingDesc.debugName = "Staging Alloc";

		Handle<RHI::Allocation> stagingAlloc = RHI::GraphicsContext::GetDefaultAllocator()->CreateBuffer(stagingDesc);

		RefPtr<PooledCommandBuffer> pooledCommandBuffer = CommandBufferPool::GetCommandBuffer();
		RefPtr<RHI::CommandBuffer> commandBuffer = pooledCommandBuffer->Get();

		commandBuffer->Begin();
		commandBuffer->BeginMarker(std::format("Import Texture {}", filepath.string()), { 1.f, 1.f, 1.f, 1.f });

		{
			RHI::ResourceBarrierInfo barrier{};
			barrier.type = RHI::BarrierType::Image;
			RHI::ResourceUtility::InitializeBarrierSrcFromCurrentState(barrier.imageBarrier(), image);

			barrier.imageBarrier().dstStage = RHI::BarrierStage::Copy;
			barrier.imageBarrier().dstAccess = RHI::BarrierAccess::CopyDest;
			barrier.imageBarrier().dstLayout = RHI::ImageLayout::CopyDest;
			barrier.imageBarrier().resource = image;

			commandBuffer->ResourceBarrier({ barrier });
		}

		commandBuffer->UploadTextureData(image, stagingAlloc, copyData);

		{
			RHI::ResourceBarrierInfo barrier{};
			barrier.type = RHI::BarrierType::Image;
			RHI::ResourceUtility::InitializeBarrierSrcFromCurrentState(barrier.imageBarrier(), image);

			barrier.imageBarrier().dstStage = RHI::BarrierStage::PixelShader;
			barrier.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderRead;
			barrier.imageBarrier().dstLayout = RHI::ImageLayout::ShaderRead;
			barrier.imageBarrier().resource = image;

			commandBuffer->ResourceBarrier({ barrier });
		}

		commandBuffer->EndMarker();
		commandBuffer->End();

		RHI::CommandBufferUtils::ExecuteCommandBufferWithNewFence(commandBuffer);

		RHI::GraphicsContext::GetDefaultAllocator()->DestroyBuffer(stagingAlloc);

		AssetReference<Texture2D> voltTexture;

		if (importConfig.createAsMemoryAsset)
		{
			voltTexture = g_assetManager->CreateMemoryAsset<Texture2D>(importConfig.destinationFilename);
		}
		else
		{
			voltTexture = g_assetManager->CreateAsset<Texture2D>(importConfig.destinationFilename);
		}

		ScopedAssetReferenceLock textureLock{ voltTexture };
		voltTexture->SetImage(image);

		return { voltTexture };
	}

	SourceAssetFileInformation DDSTextureSourceImporter::GetSourceFileInformation(const std::filesystem::path& filepath) const
	{
		VT_ENSURE(false);
		return SourceAssetFileInformation();
	}
}
