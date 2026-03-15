#include "vtassetspch.h"

#include "Volt-Assets/FontCommon.h"

namespace Volt
{
	const GlyphGeometry* FontGeometry::GetGlyph(uint32_t character) const
	{
		auto it = m_glyphs.find(character);
		if (it != m_glyphs.end())
		{
			return &it->second;
		}

		return nullptr;
	}

	bool FontGeometry::GetAdvance(double& advance, uint32_t codepoint0, uint32_t codepoint1) const
	{
		const GlyphGeometry* glyph0 = GetGlyph(codepoint0);
		const GlyphGeometry* glyph1 = GetGlyph(codepoint1);

		if (!glyph0 || !glyph1)
		{
			return false;
		}

		advance = glyph0->GetAdvance();
		auto it = m_kerning.find(std::make_pair<int32_t, int32_t>(glyph0->GetIndex(), glyph1->GetIndex()));
		if (it != m_kerning.end())
		{
			advance += it->second;
		}

		return true;
	}

	Archive& operator<<(Archive& archive, FontGeometry& value)
	{
		archive << value.m_glyphs;
		archive << value.m_kerning;

		return archive;
	}

	Archive& operator<<(Archive& archive, GlyphGeometry& value)
	{
		archive << value.m_planeBounds;
		archive << value.m_atlasBounds;
		archive << value.m_advance;
		archive << value.m_index;

		return archive;
	}

	Archive& operator<<(Archive& archive, GlyphGeometry::Bounds& value)
	{
		archive << value.left;
		archive << value.bottom;
		archive << value.right;
		archive << value.top;

		return archive;
	}

	Archive& operator<<(Archive& archive, FontMetrics& value)
	{
		archive << value.emSize;
		archive << value.ascenderY;
		archive << value.descenderY;
		archive << value.lineHeight;
		archive << value.underlineY;
		archive << value.underlineThickness;

		return archive;
	}
}
