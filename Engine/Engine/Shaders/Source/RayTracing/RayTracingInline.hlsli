#pragma once

#include "RayTracingCommon.hlsli"
#include "Barycentrics.hlsli"

struct RayTraceInlineResult
{
	uint instanceID;
	uint primitiveIndex;
	float hitT;
	uint isFrontFace;

	Barycentrics barycentrics;

	uint GetInstanceID()
	{
		return instanceID;
	}

	uint GetPrimitiveIndex()
	{
		return primitiveIndex;
	}

	float GetHitT()
	{
		return hitT;
	}

	bool IsFrontFace()
	{
		return isFrontFace != 0;
	}

	bool IsBackFace()
	{
		return isFrontFace == 0;
	}

	Barycentrics GetBarycentrics()
	{
		return barycentrics;
	}

	bool IsMiss()
	{
		return hitT < 0.f;
	}

	bool IsHit()
	{
		return !IsMiss();
	}
};

RayTraceInlineResult TraceInlineRay(RaytracingAccelerationStructure tlas, uint rayFlags, uint instanceInclusionMask, in RayDescription rayDesc)
{
	RayQuery<RAY_FLAG_NONE> query;
	query.TraceRayInline(tlas, rayFlags, instanceInclusionMask, rayDesc.GetNativeDesc());

	RayTraceInlineResult result = (RayTraceInlineResult)0;

	while (query.Proceed())
	{
		switch (query.CandidateType())
		{
			case CANDIDATE_NON_OPAQUE_TRIANGLE:
			{
				// #TODO_RayTracing: Implement.
				break;
			}
		}
	}

	switch (query.CommittedStatus())
	{
		case COMMITTED_TRIANGLE_HIT:
		{
			result.instanceID = query.CommittedInstanceID();
			result.primitiveIndex = query.CommittedPrimitiveIndex();
			result.hitT = query.CommittedRayT();
			result.isFrontFace = query.CommittedTriangleFrontFace();
			result.barycentrics.Initialize(query.CommittedTriangleBarycentrics());
			break;
		}
		case COMMITTED_NOTHING:
		{
			result.hitT = -1.f;
			break;
		}
	}

	return result;
}