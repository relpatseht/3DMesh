#pragma once

#include <vector>

namespace mesh
{
	namespace half_edge
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
			COUNT
		};

		struct FaceIndex
		{
			uint32_t index : 31;
			FaceType type : 1;
		};

		struct Topology
		{
			std::vector<unsigned> vertHalfEdges; // Half edge pointer for each vert
			std::vector<Vert> verts; // Vert list. Holds original and split id

			std::vector<unsigned> faceHalfEdges[FaceType::COUNT]; // First half edge pointer for each face. Each boundary only has 1 face

			std::vector<unsigned> halfEdgeVerts; // Vert pointer for each half edge
			std::vector<FaceIndex> halfEdgeFaces; // Face pointer for each half edge
			std::vector<unsigned> halfEdgeNexts; // Next pointer for each half edge

			// Note: halfEdge ^ 1 == pair
			// Note: halfEdge / 2 == edge
			// Note: edge * 2 == halfEdge
		};


		// Assumptions: manifold (singularities allowed), no lines (triangles with 2 identical points)
		bool Construct(const unsigned* indices, unsigned triCount, Topology* outMesh);
	};
}