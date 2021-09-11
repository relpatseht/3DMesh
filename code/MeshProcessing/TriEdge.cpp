#include <algorithm>
#include <unordered_map>
#include <cassert>
#include "MeshProc/TriEdge.h"
#include "sanity.h"

namespace mesh
{
	namespace tri_edge
	{
		bool Construct(const unsigned* indices, unsigned triCount, Topology* outMesh)
		{
			struct MappedEdge
			{
				uint32_t edgeIndex : 31;
				uint32_t occupied : 1;
			};

			std::vector<Vert> outVerts;
			std::vector<Triangle> outTris(triCount);
			std::vector<TriangleNeighbors> outNeighbors(triCount, TriangleNeighbors{ {SharedEdge::NONE, SharedEdge::NONE, SharedEdge::NONE} });
			std::unordered_map<uint64_t, SharedEdge> triEdges;
			std::unordered_map<unsigned, unsigned> vertMap;

			vertMap.reserve(triCount * 4);
			for (unsigned triIndex = 0; triIndex < triCount; ++triIndex)
			{
				Triangle* const outTri = outTris.data() + triIndex;
				const unsigned* const triIndices = indices + (triIndex * 3);

				sanity(triIndices[0] != triIndices[1] && triIndices[1] != triIndices[2] && triIndices[2] != triIndices[0] && "Degenerate triangle detected");

				for (unsigned vertIndex = 0; vertIndex < 3; ++vertIndex)
				{
					const unsigned realIndex = triIndices[vertIndex];
					auto vertIt = vertMap.find(realIndex);

					if (vertIt == vertMap.end())
					{
						const Vert vert{ realIndex, 0 };

						sanity(vert.realIndex == realIndex && "mesh::tri_edge::Vert::realIndex overflow. Input tri index out of bounds [0, 1<<24) supported.");

						vertIt = vertMap.emplace(realIndex, static_cast<unsigned>(outVerts.size())).first;
						outVerts.emplace_back(vert);
					}

					outTri->verts[vertIndex] = vertIt->second;
				}
			}


			triEdges.reserve(triCount * 3);
			for (unsigned triIndex = 0; triIndex < triCount; ++triIndex)
			{
				const unsigned* const triIndices = indices + (triIndex * 3);
				TriangleNeighbors* const outTriNeighbors = outNeighbors.data() + triIndex;

				for (unsigned edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
				{
					const uint32_t aIndex = triIndices[edgeIndex];
					const uint32_t bIndex = triIndices[(edgeIndex + 1) % 3];
					const uint64_t otherEdgeId = (static_cast<uint64_t>(bIndex) << 32) | aIndex;
					const auto otherEdgeIt = triEdges.find(otherEdgeId);
					const SharedEdge thisTriEdge{ triIndex, edgeIndex};

					if (otherEdgeIt != triEdges.end())
					{
						SharedEdge otherEdge = otherEdgeIt->second;
						TriangleNeighbors* const otherTriNeighbors = outNeighbors.data() + otherEdge.otherTriangle;
						SharedEdge* const otherTriNeighborEdge = otherTriNeighbors->edge + otherEdge.otherEdge;

						sanity(otherTriNeighborEdge->id == SharedEdge::NONE && "Non-manifold edge detected");

						outTriNeighbors->edge[edgeIndex] = otherEdge;
						*otherTriNeighborEdge = thisTriEdge;
					}
					else
					{
						const uint64_t edgeId = (static_cast<uint64_t>(aIndex) << 32) | bIndex;
						const bool inserted = triEdges.emplace(edgeId, thisTriEdge).second;

						sanity(inserted && "Non-manifold edge detected");
					}
				}
			}

			// TODO: Split singularities

			outMesh->verts = std::move(outVerts);
			outMesh->tris = std::move(outTris);
			outMesh->triNeighbors = std::move(outNeighbors);

			return true;
		}
	}
}
