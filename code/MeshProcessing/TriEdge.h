#pragma once

#include <vector>

namespace mesh
{
	struct tri_edge
	{
		struct Topology
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

			enum TriangleType : uint32_t
			{
				REAL,
				BOUNDARY,
				FILLER,
				COUNT
			};

			struct TriangleEdge
			{
				uint32_t triangle : 28;
				uint32_t edge : 2;
				TriangleType type: 2;
			};

			struct TriVerts
			{
				static constexpr uint32_t NONE = ~0u;

				uint32_t verts[3];
			};

			struct TriNeighbors
			{
				TriangleEdge edge[3];
			};

			std::vector<TriangleEdge> vertEdges;
			std::vector<Vert> verts;

			std::vector<TriNeighbors> triNeighbors[TriangleType::COUNT];
			std::vector<TriVerts> triVerts;
			std::vector<unsigned> tris;
		};


		// Assumptions: manifold (singularities allowed), no two verts are identical, no lines (triangles with 2 identical points)
		static bool Construct(const unsigned* indices, unsigned triCount, Topology* outMesh);
	};
}