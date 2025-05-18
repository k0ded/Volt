#include "vrpch.h"
#include "Volt-Renderer/Texture/TextureSerializer.h"

#include <AssetSystem/AssetManager.h>

#include <Volt-Renderer/Texture/Texture2D.h>

#include <CoreUtilities/FileIO/BinaryStreamWriter.h>

#include <RHIModule/Images/Image.h>
#include <RHIModule/Images/ImageUtility.h>
#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Memory/Allocation.h>
#include <RHIModule/Utility/ResourceUtility.h>

namespace Volt
{
	struct TextureMip
	{
		uint32_t width;
		uint32_t height;
		size_t dataSize;
		size_t dataOffset;
	};

	struct TextureHeader
	{
		RHI::PixelFormat format; // Should be one of the BC formats
		Vector<TextureMip> mips;

		static void Serialize(BinaryStreamWriter& streamWriter, const TextureHeader& data)
		{
			streamWriter.Write(data.format);
			streamWriter.Write(data.mips);
		}

		static void Deserialize(BinaryStreamReader& streamReader, TextureHeader& outData)
		{
			streamReader.Read(outData.format);
			streamReader.Read(outData.mips);
		}
	};

	struct TextureData
	{
		struct Mip
		{
			uint32_t width;
			uint32_t height;
			size_t dataSize;
			size_t dataOffset;
			const void* dataPtr = nullptr;
		};

		void SetupMips(const TextureHeader& header, const Buffer& buffer)
		{
			for (const auto& mip : header.mips)
			{
				auto& newMip = mips.emplace_back();
				newMip.width = mip.width;
				newMip.height = mip.height;
				newMip.dataSize = mip.dataSize;
				newMip.dataOffset = mip.dataOffset;
				newMip.dataPtr = buffer.As<const void>(newMip.dataOffset);
			}
		}

		Vector<Mip> mips;
	};

	void TextureSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<Texture2D> texture = std::reinterpret_pointer_cast<Texture2D>(asset);
		RefPtr<RHI::Image> image = texture->GetImage();

		TextureHeader header{};
		header.format = image->GetFormat();

		Buffer dataBuffer{};

		//if (!Utility::IsEncodedFormat(texture->GetImage()->GetFormat()))
		//{
		//	// do encoding
		//}
		//else
		{
			const size_t maxSize = image->GetWidth() * image->GetHeight() * RHI::Utility::GetByteSizePerPixelFromFormat(image->GetFormat());

			RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();
			Handle<RHI::Allocation> stagingBuffer = RHI::GraphicsContext::GetDefaultAllocator()->CreateBuffer(maxSize, RHI::BufferUsage::TransferDst, RHI::MemoryUsage::GPUToCPU, "Staging Buffer");

			commandBuffer->Begin();

			{
				RHI::ResourceBarrierInfo barrier{};
				barrier.type = RHI::BarrierType::Image;
				barrier.imageBarrier().srcStage = RHI::BarrierStage::None;
				barrier.imageBarrier().srcAccess = RHI::BarrierAccess::None;
				barrier.imageBarrier().srcLayout = RHI::ImageLayout::ShaderRead;
				barrier.imageBarrier().dstStage = RHI::BarrierStage::Copy;
				barrier.imageBarrier().dstAccess = RHI::BarrierAccess::CopySource;
				barrier.imageBarrier().dstLayout = RHI::ImageLayout::CopySource;
				barrier.imageBarrier().resource = image;

				commandBuffer->ResourceBarrier({ barrier });
			}

			commandBuffer->End();
			commandBuffer->ExecuteAndWait();

			for (uint32_t i = 0; i < image->GetMipCount(); i++)
			{
				auto& newMip = header.mips.emplace_back();
				newMip.width = std::max(image->GetWidth() >> i, 1u);
				newMip.height = std::max(image->GetHeight() >> i, 1u);
				newMip.dataOffset = dataBuffer.GetSize();

				uint32_t sizeWidth = newMip.width;
				uint32_t sizeHeight = newMip.height;

				if (RHI::Utility::IsCompressedFormat(image->GetFormat()))
				{
					sizeWidth = RHI::Utility::NextPow2(sizeWidth);
					sizeHeight = RHI::Utility::NextPow2(sizeHeight);
				}

				const size_t mipSize = sizeWidth * sizeHeight * RHI::Utility::GetByteSizePerPixelFromFormat(image->GetFormat());
				newMip.dataSize = mipSize;

				commandBuffer->Begin();
				commandBuffer->CopyImageToBuffer(image, stagingBuffer, 0, newMip.width, newMip.height, 1, i);
				commandBuffer->End();
				commandBuffer->ExecuteAndWait();

				void* data = stagingBuffer->Map<void>();
				dataBuffer.Resize(newMip.dataOffset + mipSize);
				dataBuffer.Copy(data, mipSize, newMip.dataOffset);
				stagingBuffer->Unmap();
			}

			commandBuffer->Begin();

			{
				RHI::ResourceBarrierInfo barrier{};
				barrier.type = RHI::BarrierType::Image;
				barrier.imageBarrier().srcAccess = RHI::BarrierAccess::CopySource;
				barrier.imageBarrier().srcLayout = RHI::ImageLayout::CopySource;
				barrier.imageBarrier().srcStage = RHI::BarrierStage::Copy;
				barrier.imageBarrier().dstStage = RHI::BarrierStage::None;
				barrier.imageBarrier().dstAccess = RHI::BarrierAccess::None;
				barrier.imageBarrier().dstLayout = RHI::ImageLayout::ShaderRead;
				barrier.imageBarrier().resource = image;

				commandBuffer->ResourceBarrier({ barrier });
			}

			commandBuffer->End();
			commandBuffer->ExecuteAndWait();

			RHI::GraphicsContext::GetDefaultAllocator()->DestroyBuffer(stagingBuffer);
		}

		BinaryStreamWriter streamWriter{};
		const size_t compressedDataOffset = AssetSerializer::WriteMetadata(metadata, asset->GetVersion(), streamWriter);

		streamWriter.Write(header);
		streamWriter.Write(dataBuffer);
		dataBuffer.Release();

		const auto filePath = AssetManager::GetFilesystemPath(metadata.filePath);
		streamWriter.WriteToDisk(filePath, true, compressedDataOffset);
	}

	bool TextureSerializer::Deserialize(const AssetMetadata& metadata, Ref<Asset> destinationAsset) const
	{
		const auto filePath = AssetManager::GetFilesystemPath(metadata.filePath);

		if (!std::filesystem::exists(filePath))
		{
			VT_LOG(Error, "File {0} not found!", metadata.filePath);
			destinationAsset->SetFlag(AssetFlag::Missing, true);
			return false;
		}

		BinaryStreamReader streamReader{ filePath };

		if (!streamReader.IsStreamValid())
		{
			VT_LOG(Error, "Failed to open file: {0}!", metadata.filePath);
			destinationAsset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		SerializedAssetMetadata serializedMetadata = AssetSerializer::ReadMetadata(streamReader);
		VT_ASSERT_MSG(serializedMetadata.version == destinationAsset->GetVersion(), "Incompatible version!");

		TextureHeader textureHeader{};
		streamReader.Read(textureHeader);

		Buffer textureDataBuffer{};
		streamReader.Read(textureDataBuffer);

		Ref<Texture2D> texture = std::reinterpret_pointer_cast<Texture2D>(destinationAsset);

		RefPtr<RHI::Image> image;
		RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();

		// Create image
		{
			RHI::ImageSpecification specification{};
			specification.format = textureHeader.format;
			specification.usage = RHI::ImageUsage::Texture;
			specification.width = textureHeader.mips.front().width;
			specification.height = textureHeader.mips.front().height;

			// #TODO_Ivar: Remove and implement proper way of generating mips
			//specification.mips = static_cast<uint32_t>(textureHeader.mips.size());
			specification.mips = RHI::Utility::CalculateMipCount(specification.width, specification.height);
			specification.generateMips = false;
			specification.debugName = filePath.stem().string();

			image = RHI::Image::Create(specification);
		}

		TextureData texData{};
		texData.SetupMips(textureHeader, textureDataBuffer);

		uint64_t stagingAllocSize = 0;

		RHI::ImageCopyData copyData{};
		for (uint32_t mipIndex = 0; const auto & mipData : texData.mips)
		{
			auto& subData = copyData.copySubData.emplace_back();
			subData.data = mipData.dataPtr;
			subData.rowPitch = mipData.width * RHI::Utility::GetByteSizePerPixelFromFormat(textureHeader.format);
			subData.slicePitch = static_cast<uint32_t>(mipData.dataSize);
			subData.width = mipData.width;
			subData.height = mipData.height;
			subData.depth = 1;
			subData.subResource.baseArrayLayer = 0;
			subData.subResource.baseMipLevel = mipIndex;
			subData.subResource.layerCount = 1;
			subData.subResource.levelCount = 1;

			stagingAllocSize += subData.slicePitch;

			mipIndex++;
		}

		Handle<RHI::Allocation> stagingAlloc = RHI::GraphicsContext::GetDefaultAllocator()->CreateBuffer(stagingAllocSize, RHI::BufferUsage::StorageBuffer | RHI::BufferUsage::TransferSrc, RHI::MemoryUsage::CPUToGPU, "Staging Alloc");

		commandBuffer->Begin();

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

		commandBuffer->End();
		commandBuffer->Execute();

		RHI::GraphicsContext::GetDefaultAllocator()->DestroyBuffer(stagingAlloc);

		image->GenerateMips();

		texture->SetImage(image);

		return true;
	}
}
