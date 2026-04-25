#pragma once

//#define VT_SPECIFY_FORMAT(format) [[vk::image_format(format)]]
#define VT_SPECIFY_FORMAT(format)

/*
	Inline parameter blocks allows faster binding of uniform state per pipeline.
*/

#define INLINE_PARAMETER_BLOCK(decl) \
	struct InlineParameterBlock \
	decl \
	;\
	[[vk::push_constant]] InlineParameterBlock InlineParameters