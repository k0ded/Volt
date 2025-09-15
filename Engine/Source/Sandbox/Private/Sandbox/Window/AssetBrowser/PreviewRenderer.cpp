#include "sbpch.h"
#include "Window/AssetBrowser/PreviewRenderer.h"

#include "Sandbox/Window/AssetBrowser/AssetItem.h"

#include <AssetSystem/AssetManager.h>

#include <Volt-Assets/MeshAsset.h>
#include <Volt-Assets/MaterialAsset.h>

#include <Volt-Renderer/Mesh/Mesh.h>

#include <Volt-Scene/Scene.h>

#include <Volt-Renderer/SceneRenderer.h>
#include <Volt-Renderer/Camera/Camera.h>

#include <Volt-CoreComponents/LightComponents.h>
#include <Volt-CoreComponents/RenderingComponents.h>

PreviewRenderer::PreviewRenderer()
{
	myCamera = CreateRef<Volt::Camera>(60.f, 1.f, 1.f, 100000.f);
	myPreviewScene = Volt::Scene::CreateDefaultScene("Preview", false, true);

	// Set HDRI
	{
		auto skylightEntities = myPreviewScene->GetAllEntitiesWith<Volt::SkylightComponent>();

		Volt::Entity ent = skylightEntities.front();
		//ent.GetComponent<Volt::SkylightComponent>().environmentHandle = Volt::AssetManager::GetAssetHandleFromFilePath("Engine/Textures/HDRIs/defaultHDRI.hdr");
	}

	myEntity = myPreviewScene->CreateEntity();
	myEntity.AddComponent<Volt::MeshComponent>();

	Volt::SceneRendererCreateInfo spec{};
	spec.initialResolution = { 256, 256 };
	spec.renderScene = myPreviewScene->GetRenderScene();

	//Volt::SceneRendererSettings settings{};
	//settings.enableSkybox = false;

	myPreviewRenderer = CreateRef<Volt::SceneRenderer>(spec);
}

PreviewRenderer::~PreviewRenderer()
{
	myPreviewRenderer = nullptr;
	myPreviewScene = nullptr;
	myCamera = nullptr;
}

void PreviewRenderer::RenderPreview(Weak<AssetBrowser::AssetItem> assetItem)
{
	if (assetItem)
	{
		return;
	}

	const auto assetType = Volt::AssetManager::GetAssetTypeFromHandle(assetItem->handle);

	const bool assetWasLoaded = Volt::AssetManager::Get().IsLoaded(assetItem->handle);

	if (assetType == AssetTypes::Mesh)
	{
		if (!RenderMeshPreview(assetItem))
		{
			return;
		}
	}
	else if (assetType == AssetTypes::Material)
	{
		if (!RenderMaterialPreview(assetItem))
		{
			return;
		}
	}

	//Volt::GraphicsContextVolt::GetDevice()->WaitForIdle();

	//Volt::ImageSpecification spec{};
	//spec = myPreviewRenderer->GetFinalImage()->GetSpecification();

	//itemPtr->previewImage = Volt::Image2D::Create(spec);

	//auto commandBuffer = Volt::GraphicsContextVolt::GetDevice()->GetSingleUseCommandBuffer(true);
	//itemPtr->previewImage->CopyFromImage(commandBuffer, myPreviewRenderer->GetFinalImage());
	//Volt::GraphicsContextVolt::GetDevice()->FlushSingleUseCommandBuffer(commandBuffer);

	if (!assetWasLoaded)
	{
		Volt::AssetManager::Get().UnloadAsset(assetItem->handle);
	}
}

bool PreviewRenderer::RenderMeshPreview(Weak<AssetBrowser::AssetItem> assetItem)
{
	Ref<Volt::MeshAsset> mesh = Volt::AssetManager::QueueAsset<Volt::MeshAsset>(assetItem->handle);
	if (!mesh || !mesh->IsValid())
	{
		return false;
	}

	myEntity.GetComponent<Volt::MeshComponent>().handle = assetItem->handle;
	//myEntity.GetComponent<Volt::MeshComponent>().material = Volt::Asset::Null();

	const glm::vec3 rotation = { glm::radians(30.f), glm::radians(135.f), 0.f };
	myCamera->SetRotation(rotation);

	const glm::vec3 position = mesh->GetMesh()->GetBoundingSphere().center - myCamera->GetForward() * mesh->GetMesh()->GetBoundingSphere().radius * 2.f;
	myCamera->SetPosition(position);

	//myPreviewRenderer->OnRenderEditor(myCamera);

	return true;
}

bool PreviewRenderer::RenderMaterialPreview(Weak<AssetBrowser::AssetItem> assetItem)
{
	Ref<Volt::MaterialAsset> material = Volt::AssetManager::QueueAsset<Volt::MaterialAsset>(assetItem->handle);
	if (!material || !material->IsValid())
	{
		return false;
	}

	auto meshAsset = Volt::AssetManager::QueueAsset<Volt::MeshAsset>(Volt::AssetManager::GetAssetHandleFromFilePath("Engine/Meshes/Primitives/SM_Sphere.vtasset"));
	if (!meshAsset || !meshAsset->IsValid())
	{
		return false;
	}

	myEntity.GetComponent<Volt::MeshComponent>().handle = meshAsset->handle;
	//myEntity.GetComponent<Volt::MeshComponent>().material = material->handle;

	myCamera->SetPosition({ 0.f, 0.f, -150.f });
	myCamera->SetRotation(0.f);

	//myPreviewRenderer->OnRenderEditor(myCamera);

	return true;
}
