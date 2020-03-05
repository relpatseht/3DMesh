#pragma once

#include <vector>

namespace mesh
{
	struct half_edge
	{
		union Vert
		{
			struct
			{
				uint32_t realIndex : 24;
				uint32_t splitIndex : 8;
			};

			uint32_t id;
		};

		enum FaceType : uint32_t
		{
			REAL,
			BOUNDARY,
			FILLER,
			COUNT
		};

		struct FaceIndex
		{
			uint32_t index : 30;
			FaceType type : 2;
		};

		struct Topology
		{
			std::vector<unsigned> vertEdges;
			std::vector<Vert> verts;

			std::vector<unsigned> faceEdges[FaceType::COUNT];
			std::vector<unsigned> faces;

			std::vector<unsigned> halfEdgeVerts;
			std::vector<FaceIndex> halfEdgeFaces;
			std::vector<unsigned> halfEdgePairs;
			std::vector<unsigned> halfEdgeNexts;
		};


		// Assumptions: manifold (singularities allowed), no two verts are identical, no lines (triangles with 2 identical points)
		static bool Construct(const unsigned* indices, unsigned triCount, Topology* outMesh);
	};
}