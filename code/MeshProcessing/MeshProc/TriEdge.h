#pragma once

#include <vector>

namespace mesh
{
	namespace tri_edge
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

		union SharedEdge
		{
			static const uint32_t NONE = ~0u;
			static const uint32_t NO_TRIANGLE = (1u << 30) - 1;
			static const uint32_t NO_EDGE = 0x3;

			struct
			{

				uint32_t otherTriangle : 30;
				uint32_t otherEdge : 2;
			};

			uint32_t id;
		};

		struct Triangle
		{
			unsigned verts[3];
		};

		struct TriangleNeighbors
		{
			SharedEdge edge[3];
		};

		struct Topology
		{
			std::vector<Vert> verts;
			std::vector<TriangleNeighbors> triNeighbors;
			std::vector<Triangle> tris;
		};

		// Assumptions: manifold (singularities allowed), no lines (triangles with 2 identical points)
		bool Construct(const unsigned* indices, unsigned triCount, Topology* outMesh);
	};
}