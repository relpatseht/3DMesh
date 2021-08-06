#include <algorithm>
#include <unordered_map>
#include <cassert>
#include "MeshProc/TriEdge.h"

namespace mesh
{
	bool tri_edge::Construct(const unsigned* indices, unsigned triCount, Topology* outMesh)
	{
		struct MappedEdge
		{
			uint32_t edgeIndex : 31;
			uint32_t occupied : 1;
		};

		static constexpr unsigned EDGE_NONE = 3;
		std::vector<unsigned> indexVertMap;
		std::vector<TriangleEdge> outVertEdges;
		std::vector<Vert> outVerts;
		std::vector<TriNeighbors> outTriNeighbors(triCount);
		std::vector<TriVerts> outTriVerts(triCount);
		std::vector<unsigned> outTris(triCount);
		std::unordered_map<uint64_t, MappedEdge> edgeMap;
		auto FindAddVert = [&indexVertMap, &outVerts, &outVertEdges](unsigned vertIndex)
		{
			static constexpr unsigned NO_VERT = ~0u;
			if (vertIndex > indexVertMap.size())
				indexVertMap.resize(vertIndex + 1, NO_VERT);

			if (indexVertMap[vertIndex] == NO_VERT)
			{
				indexVertMap[vertIndex] = static_cast<unsigned>(outVerts.size());
				outVerts.emplace_back(Vert{ {vertIndex, 0} });
				outVertEdges.emplace_back(NO_VERT);

				assert(outVerts.back().realIndex == vertIndex && "Vert::realIndex overflow.");
			}

			return indexVertMap[vertIndex];
		};

		// Build real topology
		for (unsigned triIndex = 0; triIndex < triCount; ++triIndex)
		{
			const unsigned firstTriIndex = triIndex * 3;
			const unsigned* const triIndices = indices + firstTriIndex;

			outTris[triIndex] = triIndex;

			if (triIndices[0] == triIndices[1] || triIndices[1] == triIndices[2] || triIndices[2] == triIndices[0])
				return false; // Invalid triangle

			for (unsigned edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
			{
				const unsigned thisEdgeIndex = firstTriIndex + edgeIndex;
				const unsigned nextEdgeIndex = (thisEdgeIndex + 1) % 3;
				const unsigned vertA = FindAddVert(indices[edgeIndex]);
				const unsigned vertB = FindAddVert(indices[nextEdgeIndex]);
				const uint64_t edgeId = (static_cast<uint64_t>(vertA) << 32) | vertB;
				auto [thisEdgeIt, newEdge] = edgeMap.emplace(edgeId, MappedEdge{ thisEdgeIndex });

				if (!newEdge)
					return false; // non-manifold edge
				else
				{
					const TriangleEdge thisEdge = TriangleEdge{ triIndex, edgeIndex, TriangleType::REAL };
					const uint64_t pairId = (static_cast<uint64_t>(vertB) << 32) | vertA;
					auto pairIt = edgeMap.find(pairId);

					outTriVerts[triIndex].verts[edgeIndex] = vertA;
					outVertEdges[vertA] = thisEdge;

					if (pairIt != edgeMap.end())
					{
						const MappedEdge pair = pairIt->second;

						if (pair.occupied)
							return false; // non-manifold edge
						else
						{
							const unsigned neighborTriIndex = pair.edgeIndex / 3;
							const unsigned neighborTriEdgeIndex = pair.edgeIndex % 3;

							outTriNeighbors[triIndex].edge[edgeIndex] = TriangleEdge{ neighborTriIndex, neighborTriEdgeIndex, TriangleType::REAL };
							outTriNeighbors[neighborTriIndex].edge[neighborTriEdgeIndex] = thisEdge;

							pairIt->second.occupied = 1;
							thisEdgeIt->second.occupied = 1;
						}
					}
					else
					{
						outTriNeighbors[triIndex].edge[edgeIndex].edge = EDGE_NONE;
					}
				}
			}
		}

		// Add imaginary boundary faces
		std::vector<TriNeighbors> outBoundaries;
		std::vector<TriVerts> outBoundaryVerts;
		for (size_t triIndex = 0; triIndex < outTris.size(); ++triIndex)
		{
			for (size_t edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
			{
				const TriangleEdge neighbor = outTriNeighbors[triIndex].edge[edgeIndex];

				if (neighbor.edge == EDGE_NONE)
				{
					do
					{

					}
					const unsigned boundaryIndex = static_cast<unsigned>(outBoundaries.size());

				}
			}
		}
		for (size_t edgeIndex = 0; edgeIndex < outHalfEdgePairs.size(); ++edgeIndex)
		{
			if (outHalfEdgePairs[edgeIndex] == EDGE_NONE)
			{
				const unsigned boundaryIndex = static_cast<unsigned>(outBoundaries.size());
				const FaceIndex boundaryFace{ boundaryIndex, FaceType::BOUNDARY };
				unsigned boundaryFlipIndex = edgeIndex;

				// Create the new face by walking around the unpaired edges
				do
				{
					const unsigned newHalfEdgeIndex = static_cast<unsigned>(outHalfEdgePairs.size());
					unsigned boundaryNextIndex = outHalfEdgeNexts[boundaryFlipIndex];

					// Find next unpaired edge
					while (outHalfEdgePairs[boundaryNextIndex] != EDGE_NONE)
						boundaryNextIndex = outHalfEdgeNexts[outHalfEdgePairs[boundaryNextIndex]];

					outHalfEdgeVerts.emplace_back(outHalfEdgeVerts[boundaryNextIndex]);
					outHalfEdgeFaces.emplace_back(boundaryFace);

					outHalfEdgePairs[boundaryFlipIndex] = newHalfEdgeIndex;
					outHalfEdgePairs.emplace_back(boundaryFlipIndex);

					outHalfEdgeNexts[boundaryFlipIndex] = newHalfEdgeIndex + 1;

					boundaryFlipIndex = boundaryNextIndex;
				} while (boundaryFlipIndex != edgeIndex);

				// Correct the last added next to point back to the start, as it's supposed to
				outHalfEdgeNexts.back() = outHalfEdgePairs[edgeIndex];
			}
		}

		// TODO: Split singularities

		outMesh->vertEdges = std::move(outVertEdges);
		outMesh->verts = std::move(outVerts);

		outMesh->faceEdges[FaceType::REAL] = std::move(outFaceEdges);
		outMesh->faceEdges[FaceType::BOUNDARY] = std::move(outBoundaries);
		outMesh->faces = std::move(outFaces);

		outMesh->halfEdgeVerts = std::move(outHalfEdgeVerts);
		outMesh->halfEdgeFaces = std::move(outHalfEdgeFaces);
		outMesh->halfEdgePairs = std::move(outHalfEdgePairs);
		outMesh->halfEdgeNexts = std::move(outHalfEdgeNexts);
	}
}
