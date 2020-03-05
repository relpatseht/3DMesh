#include <cfloat>
#include <immintrin.h>
#include "MeshProc/Mesh.h"

namespace
{
	void MeshBounds(const float* verts, unsigned vertCount, __m128* outMins, __m128* outMaxs)
	{
		const __m128i vertMask = _mm_set_epi32(-1, -1, -1, 0);
		__m128 mins = _mm_set1_ps(FLT_MAX);
		__m128 maxs = _mm_set1_ps(-FLT_MAX);

		for (unsigned vertIndex = 0; vertIndex < vertCount; ++vertIndex)
		{
			const float* const vertPtr = verts + (vertIndex * 3);
			const __m128 vert = _mm_maskload_ps(vertPtr, vertMask);

			mins = _mm_min_ps(mins, vert);
			maxs = _mm_max_ps(maxs, vert);
		}

		*outMins = mins;
		*outMaxs = maxs;
	}
}

namespace mesh
{
	void Recenter(float* inoutVerts, unsigned vertCount, const float* optCenter, float* optOutOffset)
	{
		const __m128i vertMask = _mm_set_epi32(-1, -1, -1, 0);
		__m128 mins;
		__m128 maxs;

		MeshBounds(inoutVerts, vertCount, &mins, &maxs);

		const __m128 desiredCenter = optCenter ? _mm_maskload_ps(optCenter, vertMask) : _mm_set1_ps(0.0f);
		const __m128 center = _mm_mul_ps(_mm_add_ps(mins, maxs), _mm_set1_ps(0.5f));
		const __m128 offset = _mm_sub_ps(desiredCenter, center);

		for (unsigned vertIndex = 0; vertIndex < vertCount; ++vertIndex)
		{
			float* const vertPtr = inoutVerts + (vertIndex * 3);
			const __m128 vert = _mm_maskload_ps(vertPtr, vertMask);
			const __m128 offsetVert = _mm_add_ps(vert, offset);

			_mm_maskstore_ps(vertPtr, vertMask, offsetVert);
		}

		if (optOutOffset)
		{
			_mm_maskstore_ps(optOutOffset, vertMask, offset);
		}
	}

	float Normalize(float* inoutVerts, unsigned vertCount)
	{
		const __m128i vertMask = _mm_set_epi32(-1, -1, -1, 0);
		const __m128 posMask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFF));
		__m128 mins;
		__m128 maxs;

		MeshBounds(inoutVerts, vertCount, &mins, &maxs);

		const __m128 absMax = _mm_max_ps(_mm_or_ps(mins, posMask), _mm_or_ps(maxs, posMask));
		const float radius = std::max(absMax.m128_f32[0], std::max(absMax.m128_f32[1], absMax.m128_f32[2]));
		const __m128 invRadius = _mm_set1_ps(1.0f / radius);

		for (unsigned vertIndex = 0; vertIndex < vertCount; ++vertIndex)
		{
			float* const vertPtr = inoutVerts + (vertIndex * 3);
			const __m128 vert = _mm_maskload_ps(vertPtr, vertMask);
			const __m128 scaledVert = _mm_mul_ps(vert, invRadius);

			_mm_maskstore_ps(vertPtr, vertMask, scaledVert);
		}

		return radius;
	}

	
}
