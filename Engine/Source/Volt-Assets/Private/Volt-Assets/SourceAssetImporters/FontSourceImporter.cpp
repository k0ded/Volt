#include "vtassetspch.h"

#include "Volt-Assets/SourceAssetImporters/FontSourceImporter.h"
#include "Volt-Assets/SourceAssetImporters/ImportConfigs.h"
#include "Volt-Assets/FontAsset.h"

#include <Volt-FileSystem/IOThreads/IORequest.h>
#include <Volt-FileSystem/IOThreads/IOThreads.h>

#include <AssetSystem/AssetManager.h>

#include <CoreUtilities/Profiling/Profiling.h>

#include <RHIModule/Images/Image.h>

#include <msdf-atlas-gen.h>

VT_DEFINE_LOG_CATEGORY(LogFontSourceImporter);

namespace Volt
{
	VT_REGISTER_SOURCE_ASSET_IMPORTER(({ ".ttf" }), FontSourceImporter);

	constexpr float DEFAULT_ANGLE_THRESHOLD = 3.f;
	constexpr float DEFAULT_MITER_LIMIT = 1.f;
	constexpr uint64_t LCG_MULTIPLIER = 6364136223846793005ull;
	constexpr uint64_t LCG_INCREMENT = 1442695040888963407ull;
	constexpr uint32_t THREADS = 8;

	constexpr Array<uint32_t, 9> CharsetRanges =
	{
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
		0x2DE0, 0x2DFF, // Cyrillic Extended-A
		0xA640, 0xA69F, // Cyrillic Extended-B
		0,
	};

	struct FontInput
	{
		std::string filename;
		std::string charsetFilename;
		std::string fontName;
		msdf_atlas::GlyphIdentifierType glyphType;
	};

	struct Configuration
	{
		msdf_atlas::ImageType imageType;
		msdf_atlas::ImageFormat imageFormat;
		msdf_atlas::YDirection yDirection;
		msdf_atlas::GeneratorAttributes generatorAttribs;
		int32_t width = 0;
		int32_t height = 0;
		float emSize = 0.f;
		float pxRange = 0.f;
		float angleThreshold = 0.f;
		float miterLimit = 0.f;

		bool expensiveColoring;
		uint64_t coloringSeed;
		void (*edgeColoring)(msdfgen::Shape&, double, unsigned long long);
	};

	struct FontLoader
	{
	public:
		FontLoader()
			: m_freeTypeHandle(msdfgen::initializeFreetype())
		{}

		~FontLoader()
		{
			if (m_freeTypeHandle)
			{
				if (m_fontHandle)
				{
					msdfgen::destroyFont(m_fontHandle);
				}

				msdfgen::deinitializeFreetype(m_freeTypeHandle);
			}
		}

		bool Load(const std::filesystem::path& filepath)
		{
			if (m_freeTypeHandle)
			{
				if (m_fontHandle = msdfgen::loadFont(m_freeTypeHandle, filepath.string().c_str()); m_fontHandle != nullptr)
				{
					return true;
				}
			}

			return false;
		}

		VT_INLINE msdfgen::FontHandle* GetHandle() { return m_fontHandle; }

	private:
		msdfgen::FreetypeHandle* m_freeTypeHandle = nullptr;
		msdfgen::FontHandle* m_fontHandle = nullptr;
	};

	class IORequestReadFont : public IORequest
	{
	public:
		using ResultType = FontLoader;

		IORequestReadFont(std::string_view name, const std::filesystem::path& filepath)
			: IORequest(name),
			m_filepath(filepath),
			m_resultCode(IORequestResultCode::Undefined)
		{}

		~IORequestReadFont() override = default;

		void Execute() override
		{
			bool result = m_fontLoader.Load(m_filepath);
			m_resultCode = result ? IORequestResultCode::Success : IORequestResultCode::Failure;
		}

		IORequestResultCode GetResultCode() const override { return m_resultCode; }
		FontLoader& GetResult() { return m_fontLoader; }

	private:
		FontLoader m_fontLoader;
		std::filesystem::path m_filepath;
		IORequestResultCode m_resultCode;
	};

	template<typename T, typename S, int32_t N, msdf_atlas::GeneratorFunction<S, N> GEN_FN>
	static RefPtr<RHI::Image> CreateAtlas(const std::vector<msdf_atlas::GlyphGeometry>& glyphs, const Configuration& config, RHI::PixelFormat format)
	{
		msdf_atlas::ImmediateAtlasGenerator<S, N, GEN_FN, msdf_atlas::BitmapAtlasStorage<T, N>> generator(config.width, config.height);
		generator.setAttributes(config.generatorAttribs);
		generator.setThreadCount(THREADS);
		generator.generate(glyphs.data(), static_cast<int32_t>(glyphs.size()));

		auto bitmap = (msdfgen::BitmapConstRef<T, N>)generator.atlasStorage();

		RHI::ImageDesc imageDesc{};
		imageDesc.width = bitmap.width;
		imageDesc.height = bitmap.height;
		imageDesc.usage = RHI::ImageUsage::Texture;
		imageDesc.format = format;

		RefPtr<RHI::Image> image = RHI::Image::Create(imageDesc, bitmap.pixels);
		return image;
	}

	Vector<AssetReference<Asset>> FontSourceImporter::ImportInternal(const std::filesystem::path& filepath, const void* config, const SourceAssetUserImportData& userData) const
	{
		VT_PROFILE_FUNCTION();
		const FontSourceImportConfig& importConfig = *reinterpret_cast<const FontSourceImportConfig*>(config);

		FontInput fontInput = {};
		Configuration msdfConfig = {};

		fontInput.glyphType = msdf_atlas::GlyphIdentifierType::UNICODE_CODEPOINT;
		fontInput.filename = filepath.string();

		msdfConfig.imageType = msdf_atlas::ImageType::MTSDF;
		msdfConfig.imageFormat = msdf_atlas::ImageFormat::BINARY_FLOAT;
		msdfConfig.yDirection = msdf_atlas::YDirection::BOTTOM_UP;
		msdfConfig.edgeColoring = msdfgen::edgeColoringInkTrap;
		msdfConfig.generatorAttribs.config.overlapSupport = true;
		msdfConfig.generatorAttribs.scanlinePass = true;
		msdfConfig.angleThreshold = DEFAULT_ANGLE_THRESHOLD;
		msdfConfig.miterLimit = DEFAULT_MITER_LIMIT;
		msdfConfig.emSize = 40.f;

		msdf_atlas::TightAtlasPacker::DimensionsConstraint atlasSizeConstraint = msdf_atlas::TightAtlasPacker::DimensionsConstraint::MULTIPLE_OF_FOUR_SQUARE;

		IORequestResult<IORequestReadFont> ioResult = IOThreads::SubmitRequest<IORequestReadFont>("Read Font File", filepath);

		if (ioResult.GetResultCode() == IORequestResultCode::Failure)
		{
			const std::string error = std::format("Failed to read font file {}!", filepath);
			VT_LOGC(Error, LogFontSourceImporter, error);
			userData.OnError(error);
			return {};
		}

		FontLoader& fontLoader = ioResult.GetResult();
	
		// Load charset
		msdf_atlas::Charset charset;
		for (size_t range = 0; range < CharsetRanges.size() - 1; range += 2)
		{
			for (uint32_t c = CharsetRanges[range]; c <= CharsetRanges[range + 1]; ++c)
			{
				charset.add(c);
			}
		}

		std::vector<msdf_atlas::GlyphGeometry> glyphs;
		msdf_atlas::FontGeometry fontGeometry(&glyphs);

		int32_t glyphsLoaded = -1;

		switch (fontInput.glyphType)
		{
			case msdf_atlas::GlyphIdentifierType::GLYPH_INDEX:
			{
				glyphsLoaded = fontGeometry.loadGlyphset(fontLoader.GetHandle(), importConfig.scale, charset);
				break;
			}

			case msdf_atlas::GlyphIdentifierType::UNICODE_CODEPOINT:
			{
				glyphsLoaded = fontGeometry.loadCharset(fontLoader.GetHandle(), importConfig.scale, charset);
				break;
			}
		}

		if (glyphsLoaded == -1)
		{
			const std::string error = std::format("Failed to load glyphs from font file {}!", filepath);
			VT_LOGC(Error, LogFontSourceImporter, error);
			userData.OnError(error);
			return {};
		}

		fontGeometry.setName(filepath.string().c_str());

		const float pxRange = 2.f;

		msdf_atlas::TightAtlasPacker atlasPacker;
		atlasPacker.setDimensionsConstraint(atlasSizeConstraint);

		if (importConfig.emSize > 0.f)
		{
			atlasPacker.setScale(importConfig.emSize);
		}
		else
		{
			atlasPacker.setMinimumScale(0.f);
		}

		atlasPacker.setPadding(msdfConfig.imageType == msdf_atlas::ImageType::MSDF || msdfConfig.imageType == msdf_atlas::ImageType::MTSDF ? 0 : -1);
		atlasPacker.setPixelRange(pxRange);
		atlasPacker.setMiterLimit(msdfConfig.miterLimit);

		if (int32_t remaining = atlasPacker.pack(glyphs.data(), static_cast<int32_t>(glyphs.size())))
		{
			if (remaining < 0)
			{
				if (remaining < 0)
				{
					VT_ASSERT_MSG(false, "Invalid number");
				}
				else
				{
					const std::string error = std::format("Could not fit {0} out of {1} glyphs in atlas!", remaining, static_cast<int32_t>(glyphs.size()));
					VT_LOGC(Error, LogFontSourceImporter, error);
					userData.OnError(error);
					return {};
				}
			}
		}

		atlasPacker.getDimensions(msdfConfig.width, msdfConfig.height);
		VT_ASSERT(msdfConfig.width > 0 && msdfConfig.height > 0);

		msdfConfig.emSize = static_cast<float>(atlasPacker.getScale());
		msdfConfig.pxRange = static_cast<float>(atlasPacker.getPixelRange());

		if (msdfConfig.imageType == msdf_atlas::ImageType::MSDF ||
			msdfConfig.imageType == msdf_atlas::ImageType::MTSDF)
		{
			if (msdfConfig.expensiveColoring)
			{
				msdf_atlas::Workload([&glyphs, &msdfConfig](int i, int) -> bool
				{
					uint64_t glyphSeed = (LCG_MULTIPLIER * (msdfConfig.coloringSeed ^ i) + LCG_INCREMENT) * !!msdfConfig.coloringSeed;
					glyphs[i].edgeColoring(msdfConfig.edgeColoring, msdfConfig.angleThreshold, glyphSeed);
					return true;
				}, static_cast<int32_t>(glyphs.size())).finish(THREADS);
			}
			else
			{
				uint64_t glyphSeed = msdfConfig.coloringSeed;
				for (msdf_atlas::GlyphGeometry& glyph : glyphs)
				{
					glyphSeed += LCG_MULTIPLIER;
					glyph.edgeColoring(msdfConfig.edgeColoring, msdfConfig.angleThreshold, glyphSeed);
				}
			}
		}

		constexpr bool floatingPointFormat = true;

		RefPtr<RHI::Image> atlas;

		switch (msdfConfig.imageType)
		{
			case msdf_atlas::ImageType::MSDF:
			{
				if (floatingPointFormat)
				{
					atlas = CreateAtlas<float, float, 3, msdf_atlas::msdfGenerator>(glyphs, msdfConfig, RHI::PixelFormat::R32G32B32_SFLOAT);
				}
				else
				{
					atlas = CreateAtlas<uint8_t, float, 3, msdf_atlas::msdfGenerator>(glyphs, msdfConfig, RHI::PixelFormat::R8G8B8_SNORM);
				}
				break;
			}

			case msdf_atlas::ImageType::MTSDF:
			{
				if (floatingPointFormat)
				{
					atlas = CreateAtlas<float, float, 4, msdf_atlas::mtsdfGenerator>(glyphs, msdfConfig, RHI::PixelFormat::R32G32B32A32_SFLOAT);
				}
				else
				{
					atlas = CreateAtlas<uint8_t, float, 4, msdf_atlas::mtsdfGenerator>(glyphs, msdfConfig, RHI::PixelFormat::R8G8B8A8_SNORM);
				}
				break;
			}
		}

		// Copy font data over
		AssetReference<FontAsset> fontAsset;
		if (importConfig.createAsMemoryAsset)
		{
			VT_CHECK_MSG(importConfig.targetAssetHandle == Asset::Null(), "Target asset handle with memory assets are not supported!");
			fontAsset = g_assetManager->CreateMemoryAsset<FontAsset>(importConfig.destinationFilename);
		}
		else
		{
			if (importConfig.targetAssetHandle != Asset::Null())
			{
				fontAsset = g_assetManager->CreateAssetWithAssetHandle<FontAsset>(importConfig.destinationFilename, importConfig.targetAssetHandle);
			}
			else
			{
				fontAsset = g_assetManager->CreateAsset<FontAsset>(importConfig.destinationFilename);
			}
		}

		fontAsset->m_atlas = atlas;

		// Metrics
		{
			const msdfgen::FontMetrics& metrics = fontGeometry.getMetrics();

			fontAsset->m_metrics.emSize = metrics.emSize;
			fontAsset->m_metrics.ascenderY = metrics.ascenderY;
			fontAsset->m_metrics.descenderY = metrics.descenderY;
			fontAsset->m_metrics.lineHeight = metrics.lineHeight;
			fontAsset->m_metrics.underlineY = metrics.underlineY;
			fontAsset->m_metrics.underlineThickness = metrics.underlineThickness;
		}

		// Font geometry
		{
			for (const msdf_atlas::GlyphGeometry& glyphGeom : fontGeometry.getGlyphs())
			{
				GlyphGeometry& newGlyph = fontAsset->m_fontGeometry.m_glyphs[glyphGeom.getCodepoint()];
				newGlyph.m_advance = glyphGeom.getAdvance();
				newGlyph.m_index = glyphGeom.getIndex();
			
				glyphGeom.getQuadPlaneBounds(
					newGlyph.m_planeBounds.left,
					newGlyph.m_planeBounds.bottom,
					newGlyph.m_planeBounds.right,
					newGlyph.m_planeBounds.top
				);

				glyphGeom.getQuadAtlasBounds(
					newGlyph.m_atlasBounds.left,
					newGlyph.m_atlasBounds.bottom,
					newGlyph.m_atlasBounds.right,
					newGlyph.m_atlasBounds.top
				);
			}

			for (const auto& [key, value] : fontGeometry.getKerning())
			{
				fontAsset->m_fontGeometry.m_kerning[key] = value;
			}
		}

		return { fontAsset };
	}

	SourceAssetFileInformation FontSourceImporter::GetSourceFileInformation(const std::filesystem::path& filepath) const
	{
		return {};
	}
}
