#pragma once

#include <RHIModule/Images/Image.h>

class ComponentVisualizer
{
public:
	virtual ~ComponentVisualizer() = default;

	virtual RefPtr<Volt::RHI::Image> GetIcon() const { return nullptr; }

private:
};
