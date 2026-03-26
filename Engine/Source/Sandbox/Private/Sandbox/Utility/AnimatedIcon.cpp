#include "sbpch.h"
#include "Sandbox/Utility/AnimatedIcon.h"

#include <Volt-Assets/SourceAssetImporters/ImportConfigs.h>

#include <Volt-Renderer/Texture/Texture2D.h>

#include <AssetSystem/SourceAssetManager.h>

#include <EventSystem/ApplicationEvents.h>

AnimatedIcon::AnimatedIcon(const Filesystem::Path& firstFrame, uint32_t frameCount, float animTime)
	: m_animationTime(animTime), m_frameCount(frameCount), m_perFrameTime(animTime / (float)frameCount)
{
	RegisterListener<Volt::AppUpdateEvent>(VT_BIND_EVENT_FN(AnimatedIcon::Animate), [this]() { return m_isEnabled; });

	String filename = firstFrame.Stem().ToString();
	const size_t numPos = filename.find_first_of("0123456789");
	if (numPos != String::npos)
	{
		filename = filename.substr(0, numPos);
	}

	const Filesystem::Path dirPath = firstFrame.ParentPath();

	Vector<Volt::JobFuture<Vector<AssetReference<Volt::Asset>>>> futures;

	for (uint32_t frame = 1; frame <= frameCount; frame++)
	{
		const Filesystem::Path path = dirPath / FormatString("{}{}{}", filename, frame, firstFrame.Extension());
		
		Volt::TextureSourceImportConfig importConfig{};
		importConfig.createAsMemoryAsset = true;
		importConfig.generateMipMaps = true;
		importConfig.importMipMaps = true;
		importConfig.destinationFilename = path.Stem().ToString();

		futures.emplace_back(Volt::SourceAssetManager::ImportSourceAsset(path, importConfig));
	}

	for (const auto& result : futures)
	{
		auto assets = result.Get();
		if (!assets.empty())
		{
			m_textures.emplace_back(assets.front().ConvertTo<Volt::Texture2D>());
		}
	}

	VT_ASSERT_MSG(!m_textures.empty(), "No frames found!");
	VT_ASSERT_MSG(m_textures.size() == frameCount, "Not all frames loaded!");
	m_currentTexture = m_textures[0];
}

bool AnimatedIcon::Animate(Volt::AppUpdateEvent& e)
{
	if (!m_isPlaying)
	{
		return false;
	}

	m_currentTime += e.GetTimestep();
	if (m_currentTime > m_perFrameTime)
	{
		m_currentFrame++;
		m_currentTime = 0.f;
		if (m_currentFrame >= m_frameCount)
		{
			m_currentFrame = 0;
		}

		m_currentTexture = m_textures.at(m_currentFrame);
	}

	return false;
}
