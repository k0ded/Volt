#pragma once

#include "Sandbox/Window/MosaicEditor/MosaicNodeExtension.h"

class ColorNodeExtension final : public MosaicNodeExtension
{
public:
	~ColorNodeExtension() override = default;

	void Render(Ref<Mosaic::MosaicNode> node) override;
};

class SampleTextureNodeExtension final : public MosaicNodeExtension
{
public:
	~SampleTextureNodeExtension() override = default;

	void Render(Ref<Mosaic::MosaicNode> node) override;
};
