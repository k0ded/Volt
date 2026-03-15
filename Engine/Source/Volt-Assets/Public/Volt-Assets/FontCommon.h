#pragma once

namespace Volt
{
	// From msdfgen::FontMetrics
	struct FontMetrics
	{
		/// The size of one EM.
		double emSize;
		/// The vertical position of the ascender and descender relative to the baseline.
		double ascenderY, descenderY;
		/// The vertical difference between consecutive baselines.
		double lineHeight;
		/// The vertical position and thickness of the underline.
		double underlineY, underlineThickness;

		friend Archive& operator<<(Archive& archive, FontMetrics& value);
	};

	class GlyphGeometry
	{
	public:
		struct Bounds
		{
			double left;
			double bottom;
			double right;
			double top;

			friend Archive& operator<<(Archive& archive, Bounds& value);
		};

		VT_INLINE double GetAdvance() const { return m_advance; }
		VT_INLINE int32_t GetIndex() const { return m_index; }
		VT_INLINE const Bounds& GetPlaneBounds() const { return m_planeBounds; }
		VT_INLINE const Bounds& GetAtlasBounds() const { return m_atlasBounds; }

		friend Archive& operator<<(Archive& archive, GlyphGeometry& value);

	private:
		friend class FontSourceImporter;

		// Quad plane bounds
		Bounds m_planeBounds;
		Bounds m_atlasBounds;

		double m_advance;
		int32_t m_index;
	};

	class FontGeometry
	{
	public:
		const GlyphGeometry* GetGlyph(uint32_t codepoint) const;
		bool GetAdvance(double& advance, uint32_t codepoint0, uint32_t codepoint1) const;

		friend Archive& operator<<(Archive& archive, FontGeometry& value);

	private:
		friend class FontSourceImporter;

		Map<uint32_t, GlyphGeometry> m_glyphs;
		Map<std::pair<int32_t, int32_t>, double> m_kerning;
	};
}
