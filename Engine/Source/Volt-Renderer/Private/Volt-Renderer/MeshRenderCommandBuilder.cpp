#include "vrpch.h"

#include "Volt-Renderer/MeshRenderCommandBuilder.h"
#include "Volt-Renderer/RenderScene.h"
#include "Volt-Renderer/RenderPrimitiveData.h"

#include "Volt-Renderer/RenderMaterial.h"
#include "Volt-Renderer/Mesh/Mesh.h"

#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/DefaultShaders.h>
#include <RenderCore/RenderGraph/RenderGraph.h>

#include <CoreUtilities/EnumUtils.h>
#include <CoreUtilities/Profiling/Profiling.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>

namespace Volt
{
	void MeshRenderCommandBuilder::Build(RenderScene& renderScene, RenderGraph& renderGraph)
	{
		VT_PROFILE_FUNCTION();

		m_renderCommands.clear();
		m_meshBatches.clear();

		struct RenderCommandExt : public RenderCommand
		{
			size_t vertexIndexBufferHash;
			size_t subMeshHash;
			size_t pixelShaderHash;

			uint32_t subMeshIndex;
			MeshBatch::VertexBufferVector vertexBuffers;
			RefPtr<RHI::StorageBuffer> indexBuffer;
			RefPtr<RHI::Shader> pixelShader;
			Weak<RenderMaterial> renderMaterial;
			Weak<Mesh> mesh;
		};

		Vector<RenderCommandExt> renderCommandExts;
		renderCommandExts.reserve(renderScene.GetNumRenderPrimitives());

		Vector<uint32_t> validPrimitiveIndices;
		validPrimitiveIndices.reserve(renderScene.GetNumRenderPrimitives() + 1);

		// Counter
		validPrimitiveIndices.push_back(0);

		// First get all commands, their info and the required hashes for sorting.
		{
			VT_PROFILE_SCOPE("Gather Commands");

			for (const RenderPrimitiveData& renderPrimitive : renderScene)
			{
				const SubMesh& subMesh = renderPrimitive.mesh->GetSubMeshes().at(renderPrimitive.subMeshIndex);

				auto& newCommand = renderCommandExts.emplace_back();
				newCommand.indexBuffer = renderPrimitive.mesh->GetIndexBuffer();
				newCommand.primitiveIndex = renderScene.GetPrimitiveIndexFromID(renderPrimitive.id);
				newCommand.mesh = renderPrimitive.mesh;
				newCommand.subMeshIndex = renderPrimitive.subMeshIndex;

				// Always bind all vertex buffers, just in case.
				newCommand.vertexBuffers.emplace_back(renderPrimitive.mesh->GetVertexPositionsBuffer());
				newCommand.vertexBuffers.emplace_back(renderPrimitive.mesh->GetVertexMaterialBuffer());
				//newCommand.vertexBuffers.emplace_back(renderPrimitive.mesh->GetVertexAnimationInfoBuffer()->GetResource());

				RefPtr<RHI::Shader> primitivePixelShader = renderPrimitive.material->GetPixelShader();
				if (!primitivePixelShader)
				{
					primitivePixelShader = ShaderMap::Get<OpaqueDefaultPixelPS>();
				}

				newCommand.renderMaterial = renderPrimitive.material;
				newCommand.pixelShader = primitivePixelShader;
				newCommand.vertexIndexBufferHash = newCommand.indexBuffer.GetHash();

				for (const auto& vertexBuffer : newCommand.vertexBuffers)
				{
					newCommand.vertexIndexBufferHash = Math::HashCombine(newCommand.vertexIndexBufferHash, vertexBuffer.GetHash());
				}

				newCommand.subMeshHash = subMesh.GetHash();
				newCommand.pixelShaderHash = primitivePixelShader->GetHash();

				validPrimitiveIndices[0]++;
				validPrimitiveIndices.push_back(newCommand.primitiveIndex);
			}
		}

		{
			VT_PROFILE_SCOPE("Sort Commands");

			// Now sort the commands to get the correct order
			std::sort(renderCommandExts.begin(), renderCommandExts.end(), [](const RenderCommandExt& lhs, const RenderCommandExt& rhs)
			{
				if (lhs.vertexIndexBufferHash == rhs.vertexIndexBufferHash)
				{
					if (lhs.subMeshHash == rhs.subMeshHash)
					{
						return lhs.pixelShaderHash < rhs.pixelShaderHash;
					}

					return lhs.subMeshHash < rhs.subMeshHash;
				}

				return lhs.vertexIndexBufferHash < rhs.vertexIndexBufferHash;
			});
		}

		{
			VT_PROFILE_SCOPE("Build Batches");

			struct DefaultInvalid
			{
				int32_t value = -1;
			};

			Vector<uint32_t> meshIds;
			Vector<size_t> pixelShaderHashes;
			Vector<DefaultInvalid> primitiveDrawCommandIndex;

			for (size_t i = 0; i < renderCommandExts.size(); ++i)
			{
				const RenderCommandExt& renderCommandExt = renderCommandExts.at(i);
				const uint32_t meshId = renderScene.GetMeshID(renderCommandExt.mesh, renderCommandExt.subMeshIndex);

				if (i == 0)
				{
					meshIds.emplace_back(meshId);
					pixelShaderHashes.emplace_back(renderCommandExt.pixelShaderHash);
				}
				else
				{
					if (meshId != meshIds.back() || (renderCommandExt.pixelShaderHash != pixelShaderHashes.back()))
					{
						meshIds.emplace_back(meshId);
						pixelShaderHashes.emplace_back(renderCommandExt.pixelShaderHash);
					}
				}

				primitiveDrawCommandIndex.resize(std::max(renderCommandExt.primitiveIndex + 1, static_cast<uint32_t>(primitiveDrawCommandIndex.size())));
				primitiveDrawCommandIndex[renderCommandExt.primitiveIndex].value = static_cast<int32_t>(meshIds.size() - 1);
			}

			m_numDrawCommands = static_cast<uint32_t>(meshIds.size());

			if (!meshIds.empty())
			{
				renderGraph.BeginMarker("RenderScene::UploadRenderCommandData");

				m_drawCommandsToCopyIndices = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(meshIds.size(), "MeshRenderer.DrawCommandsToCopyIndices", RHI::MemoryUsage::CPUToGPU));
				AddMappedBufferUpload(renderGraph, renderGraph.CreateUAV(m_drawCommandsToCopyIndices, RHI::PixelFormat::R32_UINT), meshIds.data(), meshIds.byte_size());

				m_primitiveIndexToDrawCommandIndex = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<int32_t>(primitiveDrawCommandIndex.size(), "MeshRenderer.PrimitiveIndexToDrawCommandIndex", RHI::MemoryUsage::CPUToGPU));
				AddMappedBufferUpload(renderGraph, renderGraph.CreateUAV(m_primitiveIndexToDrawCommandIndex, RHI::PixelFormat::R32_SINT), primitiveDrawCommandIndex.data(), primitiveDrawCommandIndex.byte_size());

				m_validPrimitiveDrawDataIndices = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(validPrimitiveIndices.size(), "MeshRenderer.ValidPrimitiveIndices", RHI::MemoryUsage::CPUToGPU));
				AddMappedBufferUpload(renderGraph, renderGraph.CreateUAV(m_validPrimitiveDrawDataIndices, RHI::PixelFormat::R32_UINT), validPrimitiveIndices.data(), validPrimitiveIndices.byte_size());
			
				renderGraph.EndMarker();
			}

			MeshBatch* currentMeshBatch = nullptr;

			size_t lastSubMeshHash = 0;
			size_t lastVertexIndexBufferHash = 0;
			size_t lastPixelShaderHash = 0;

			for (size_t i = 0; i < renderCommandExts.size(); ++i)
			{
				const RenderCommandExt& renderCommandExt = renderCommandExts.at(i);

				if (i == 0)
				{
					currentMeshBatch = &m_meshBatches.emplace_back();
					currentMeshBatch->batchType = MeshBatchType::VertexIndexBuffer | MeshBatchType::RenderPipeline;
					currentMeshBatch->indexBuffer = renderCommandExt.indexBuffer;
					currentMeshBatch->vertexBuffers = renderCommandExt.vertexBuffers;
					currentMeshBatch->drawCommandOffset = 0;
					currentMeshBatch->renderMaterial = renderCommandExt.renderMaterial;
					currentMeshBatch->pixelShader = renderCommandExt.pixelShader;

					lastVertexIndexBufferHash = renderCommandExt.vertexIndexBufferHash;
					lastPixelShaderHash = renderCommandExt.pixelShaderHash;
					lastSubMeshHash = renderCommandExt.subMeshHash;
				}
				else
				{
					MeshBatchType batchType = MeshBatchType::None;

					if (renderCommandExt.vertexIndexBufferHash != lastVertexIndexBufferHash)
					{
						batchType |= MeshBatchType::VertexIndexBuffer;
						lastVertexIndexBufferHash = renderCommandExt.vertexIndexBufferHash;
					}

					if (renderCommandExt.pixelShaderHash != lastPixelShaderHash)
					{
						batchType |= MeshBatchType::RenderPipeline;
						lastPixelShaderHash = renderCommandExt.pixelShaderHash;
					}

					if (renderCommandExt.subMeshHash != lastSubMeshHash)
					{
						batchType |= MeshBatchType::SubMesh;
						lastSubMeshHash = renderCommandExt.subMeshHash;
					}

					if (batchType != MeshBatchType::None)
					{
						currentMeshBatch = &m_meshBatches.emplace_back();
						currentMeshBatch->batchType = batchType;
						currentMeshBatch->drawCommandOffset = primitiveDrawCommandIndex[renderCommandExt.primitiveIndex].value;

						if (EnumValueContainsFlag(batchType, MeshBatchType::VertexIndexBuffer))
						{
							currentMeshBatch->vertexBuffers = renderCommandExt.vertexBuffers;
							currentMeshBatch->indexBuffer = renderCommandExt.indexBuffer;
						}

						if (EnumValueContainsFlag(batchType, MeshBatchType::SubMesh))
						{
							currentMeshBatch->drawCommandOffset = primitiveDrawCommandIndex[renderCommandExt.primitiveIndex].value;
						}

						if (EnumValueContainsFlag(batchType, MeshBatchType::RenderPipeline))
						{
							currentMeshBatch->renderMaterial = renderCommandExt.renderMaterial;
							currentMeshBatch->pixelShader = renderCommandExt.pixelShader;
						}
					}
				}

				m_renderCommands.emplace_back(renderCommandExt);
			}
		}
	}
}
