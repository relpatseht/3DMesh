#pragma once

#include <vector>

namespace mesh
{
	void Recenter(float* inoutVerts, unsigned vertCount, const float* optCenter = nullptr, float* optOutOffset = nullptr);
	float Normalize(float* inoutVerts, unsigned vertCount);

	
}