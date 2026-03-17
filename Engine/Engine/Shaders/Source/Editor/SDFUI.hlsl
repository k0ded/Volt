#include "Common.hlsli"

#include "Utility/FullscreenTriangleVertex.hlsli"
#include "Utility/Packing.hlsli"
#include "Utility/2DSDF.hlsli"

#include "ResourceTable.hlsli"
#include "StaticSamplerStates.hlsli"

namespace UIPrimitiveType
{
	static const uint Circle = 0;
	static const uint Rect = 1;
	static const uint Line = 2;
	static const uint CircleSegment = 3;
	static const uint TextChar = 4;
}

struct UICommand
{
	uint type;
	int primitiveGroup;

	// Common
	float rotation;
	float scale;
	float2 position;

	float glowDistance;
	float glowStrength;

	float2 shadowOffset;
	float shadowStrength;
	float padding0;

	uint color;
	uint textureIndex;

	// Rounding
	float rounding;

	// Circle
	float radius;

	// Rect
	float2 halfSize;

	// Circle Segment
	float radiusInner;
	float angle;

	// Line
	float2 lineA;
	float2 lineB;

	// Text
	float4 minMaxUV;
	float4 minMaxPx;
};

float3 SDF_Glow(float sdf, float glowDistance, float glowStrength, float3 glowColor, float3 prevColor)
{
	const float glowMask = clamp(sdf / glowDistance, 0.f, 1.f);
	return lerp(prevColor, glowColor.rgb, (1.f - glowMask) * glowStrength);
}

struct SDFPrimitiveGroup
{
	float sdf;
	float sdfShadow;
	float3 groupColor;

	void Reset(float3 initialColor)
	{
		sdf = FLT_MAX;
		sdfShadow = FLT_MAX;
		groupColor = initialColor;
	}

	void AddCircle(in UICommand command, float2 pixelPos)
	{
		const float4 color = UnpackUIntToFloat4(command.color);
		const float2 position = SDF_Transform(command.position, command.scale, command.rotation, pixelPos);

		float tempSdf = SDF_Circle(position, command.radius) * command.scale;
		if (command.radiusInner > 0.f)
		{
			tempSdf = SDF_MakeHollow(tempSdf, command.radiusInner);
		}

		float tempSdfShadow = SDF_Circle(position + command.shadowOffset, command.radius) * command.scale;
		if (command.radiusInner > 0.f)
		{
			tempSdfShadow = SDF_MakeHollow(tempSdfShadow, command.radiusInner);
		}

		sdf = SDF_Merge(sdf, tempSdf);
		sdfShadow = SDF_Merge(sdfShadow, tempSdfShadow);

		groupColor = lerp(groupColor, color.rgb * color.a, clamp(1.f - tempSdf, 0.f, 1.f));
	}

	void AddRectangle(in UICommand command, float2 pixelPos)
	{
		const float4 color = UnpackUIntToFloat4(command.color);
		float2 position = SDF_Transform(command.position, command.scale, command.rotation, pixelPos);
		position = SDF_Translate(position, -command.halfSize);

		float tempSdf = SDF_Rectangle(position, command.halfSize, command.rounding) * command.scale;
		if (command.radiusInner > 0.f)
		{
			tempSdf = SDF_MakeHollow(tempSdf, command.radiusInner);
		}

		float tempSdfShadow = SDF_Rectangle(position + command.shadowOffset, command.halfSize, command.rounding) * command.scale;
		if (command.radiusInner > 0.f)
		{
			tempSdfShadow = SDF_MakeHollow(tempSdfShadow, command.radiusInner);
		}

		sdf = SDF_Merge(sdf, tempSdf);
		sdfShadow = SDF_Merge(sdfShadow, tempSdfShadow);

		groupColor = lerp(groupColor, color.rgb * color.a, clamp(1.f - tempSdf, 0.f, 1.f));
	}

	void AddLine(in UICommand command, float2 pixelPos)
	{
		const float4 color = UnpackUIntToFloat4(command.color);

		float tempSdf = SDF_Line(pixelPos, command.lineA, command.lineB, command.radius);
		if (command.radiusInner > 0.f)
		{
			tempSdf = SDF_MakeHollow(tempSdf, command.radiusInner);
		}

		float tempSdfShadow = SDF_Line(pixelPos, command.lineA + command.shadowOffset, command.lineB + command.shadowOffset, command.radius);
		if (command.radiusInner > 0.f)
		{
			tempSdfShadow = SDF_MakeHollow(tempSdfShadow, command.radiusInner);
		}

		sdf = SDF_Merge(sdf, tempSdf);
		sdfShadow = SDF_Merge(sdfShadow, tempSdfShadow);

		groupColor = lerp(groupColor, color.rgb * color.a, clamp(1.f - tempSdf, 0.f, 1.f));
	}

	void AddCircleSegment(UICommand command, float2 pixelPos)
	{
		const float4 color = UnpackUIntToFloat4(command.color);
		const float2 position = SDF_Transform(command.position, command.scale, command.rotation, pixelPos);

		const float tempSdf = SDF_CircleSegment(position, command.angle, command.radiusInner, command.radius) * command.scale;
		const float tempSdfShadow = SDF_CircleSegment(position + command.shadowOffset, command.angle, command.radiusInner, command.radius) * command.scale;
	
		sdf = SDF_Merge(sdf, tempSdf);
		sdfShadow = SDF_Merge(sdfShadow, tempSdfShadow);

		groupColor = lerp(groupColor, color.rgb * color.a, clamp(1.f - tempSdf, 0.f, 1.f));
	}

	void ApplyGroup(in UICommand lastCommandInGroup, inout float3 resultColor)
	{
		const float clampedSdf = clamp(1.f - sdf, 0.f, 1.f);
		sdfShadow = clamp((1.f - sdfShadow) * lastCommandInGroup.shadowStrength, 0.f, 1.f);

		if (sdfShadow > 0.f)
		{
			resultColor = lerp(resultColor, 0.f, sdfShadow);
		}

		if (lastCommandInGroup.glowStrength > 0.f && lastCommandInGroup.glowDistance > 0.f)
		{
			const float4 color = UnpackUIntToFloat4(lastCommandInGroup.color);
			resultColor = SDF_Glow(sdf, lastCommandInGroup.glowDistance, lastCommandInGroup.glowStrength, color.rgb, resultColor);
		}

		if (clampedSdf > 0.f)
		{
			resultColor = lerp(resultColor, groupColor, clampedSdf);
		}
	}
};

StructuredBuffer<UICommand> Commands;

uint CommandCount;
uint2 RenderSize;

float ScreenPxRange(float2 msdfSize, float2 screenSize)
{
	const float pxRange = 2.f;
	float2 unitRange = pxRange / msdfSize;
	
	return max(0.5f * dot(unitRange, screenSize), 1.f);
}

float SDF_TextMedian(float r, float g, float b)
{
	return max(min(r, g), min(max(r, g), b));
}

float4 MainPS(FullscreenTriangleVertex input) : SV_Target0
{
	const float2 pixelPos = input.position.xy + 0.5f;
	
	float3 resultColor = 0.f;

	SDFPrimitiveGroup primitiveGroup;
	int activePrimitiveGroup = -1;

	for (uint i = 0; i < CommandCount; i++)
	{
		UICommand command = Commands[i];
		
		const float4 commandColor = UnpackUIntToFloat4(command.color);

		if (i == 0 || 
			activePrimitiveGroup != command.primitiveGroup || // Different group
			command.primitiveGroup == -1) // No group
		{
			activePrimitiveGroup = command.primitiveGroup;
			primitiveGroup.Reset(resultColor);	
		}

		switch (command.type)
		{
			case UIPrimitiveType::Circle:
			{
				primitiveGroup.AddCircle(command, pixelPos);
				break;
			}

			case UIPrimitiveType::Rect:
			{
				primitiveGroup.AddRectangle(command, pixelPos);
				break;
			}

			case UIPrimitiveType::Line:
			{
				primitiveGroup.AddLine(command, pixelPos);
				break;
			}

			case UIPrimitiveType::CircleSegment:
			{
				primitiveGroup.AddCircleSegment(command, pixelPos);
				break;
			}

			case UIPrimitiveType::TextChar:
			{
				if (pixelPos.x < command.minMaxPx.x || pixelPos.x > command.minMaxPx.z || pixelPos.y < command.minMaxPx.w || pixelPos.y > command.minMaxPx.y)
				{
					break;
				}

				// Calculate UV within character quad
				const float xPercent = (pixelPos.x - command.minMaxPx.x) / (command.minMaxPx.z - command.minMaxPx.x);
				const float yPercent = (pixelPos.y - command.minMaxPx.y) / (command.minMaxPx.w - command.minMaxPx.y);
				
				const float2 textTexUv = float2(lerp(command.minMaxUV.x, command.minMaxUV.z, xPercent), lerp(command.minMaxUV.y, command.minMaxUV.w, yPercent));
				
				// Sample UV
				Texture2D fontAtlas = ResourceTable::LoadTexture(command.textureIndex);
				const float3 msd = fontAtlas.Sample(StaticTrilinearSamplerClamp, textTexUv).rgb;
				
				uint2 msdfSize;
				fontAtlas.GetDimensions(msdfSize.x, msdfSize.y);
				
				const float4 color = UnpackUIntToFloat4(command.color);
				float4 fgColor = color;
				
				float sd = SDF_TextMedian(msd.x, msd.y, msd.z);
				float screenPxDistance = ScreenPxRange((float2) msdfSize, (float2)RenderSize) * (sd - 0.5f);
				float opacity = clamp(screenPxDistance + 0.5f, 0.f, 1.f);
				
				resultColor = lerp(resultColor, fgColor.rgb * fgColor.a, opacity);

				break;
			}
		}

		if (i < CommandCount - 1)
		{
			int nextPrimitiveGroup = Commands[i + 1].primitiveGroup;
		
			// Apply group
			if (activePrimitiveGroup != nextPrimitiveGroup || // Next will be a new group
				activePrimitiveGroup == -1) // Always apply no groups.
			{
				primitiveGroup.ApplyGroup(command, resultColor);
			}
		}
		// Last command, apply
		else if (i == CommandCount - 1)
		{
			primitiveGroup.ApplyGroup(command, resultColor);
		}
	}

	return float4(resultColor, 1.f);
}
