#pragma once

#include <CoreUtilities/Containers/Vector.h>

#include <msdf-atlas-gen.h>

namespace Volt
{
	struct MSDFData
	{
		msdf_atlas::FontGeometry fontGeometry;
		Vector<msdf_atlas::GlyphGeometry> glyphs;
	};
}
