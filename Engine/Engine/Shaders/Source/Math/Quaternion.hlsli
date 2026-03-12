#pragma once

struct Quaternion
{
	float x;
	float y;
	float z;
	float w;

	Quaternion operator*(Quaternion other)
	{
		Quaternion result;
		result.w = w * other.w - x * other.x - y * other.y - z * other.z;
		result.x = w * other.x + x * other.w + y * other.z - z * other.y;
		result.y = w * other.y + y * other.w + z * other.x - x * other.z;
		result.z = w * other.z + z * other.w + x * other.y - y * other.x;
		return result;
	}

	float3 RotateVector(float3 v)
	{
		float3 q = float3(x, y, z);
		return v + 2.0f * cross(q, cross(q, v) + w * v);
	}

	static Quaternion Identity()
	{
		Quaternion result;
		result.x = 0.f;
		result.y = 0.f;
		result.z = 0.f;
		result.w = 1.f;

		return result;
	}
};