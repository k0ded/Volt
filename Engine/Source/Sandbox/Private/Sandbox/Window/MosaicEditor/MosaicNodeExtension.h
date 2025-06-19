#pragma once

#include <CoreUtilities/Core.h>

namespace Mosaic
{
	class MosaicNode;
}

class MosaicNodeExtension
{
public:
	virtual ~MosaicNodeExtension() = default;

	virtual void Render(Ref<Mosaic::MosaicNode> node) = 0;
};
