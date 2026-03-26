#pragma once

#include <CoreUtilities/Pointers/Ref.h>

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
