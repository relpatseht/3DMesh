#include <algorithm>
#include <unordered_map>
#include <cassert>
#include "MeshProc/HalfEdge.h"

namespace mesh
{
	bool half_edge::Construct(const unsigned* indices, unsigned triCount, Topology* outMesh)
	{
		struct MappedEdge
		{
			uint32_t edgeIndex : 31;
			uint32_t occupied : 1;
		};

		static constexpr unsigned EDGE_NONE = ~0u;
		const unsigned realHalfEdgeCount = triCount * 3;
		std::vector<unsigned> indexVertMap;
		std::vector<unsigned> outVertEdges;
		std::vector<Vert> outVerts;
		std::vector<unsigned> outFaceEdges(triCount);
		std::vector<unsigned> outFaces(triCount);
		std::vector<unsigned> outHalfEdgeVerts(realHalfEdgeCount);
		std::vector<FaceIndex> outHalfEdgeFaces(realHalfEdgeCount);
		std::vector<unsigned> outHalfEdgePairs(realHalfEdgeCount);
		std::vector<unsigned> outHalfEdgeNexts(realHalfEdgeCount);
		std::unordered_map<uint64_t, MappedEdge> halfEdgeMap;
		auto FindAddVert = [&indexVertMap, &outVerts, &outVertEdges](unsigned vertIndex)
		{
			if (vertIndex > indexVertMap.size())
				indexVertMap.resize(vertIndex + 1, EDGE_NONE);

			if (indexVertMap[vertIndex] == EDGE_NONE)
			{
				indexVertMap[vertIndex] = static_cast<unsigned>(outVerts.size());
				outVerts.emplace_back(Vert{ {vertIndex, 0} });
				outVertEdges.emplace_back(EDGE_NONE);

				assert(outVerts.back().realIndex == vertIndex && "Vert::realIndex overflow.");
			}

			return indexVertMap[vertIndex];
		};

		// Build real topology
		for (unsigned triIndex = 0; triIndex < triCount; ++triIndex)
		{
			const unsigned firstEdgeIndex = triIndex * 3;

			outFaces[triIndex] = triIndex;
			outFaceEdges[triIndex] = firstEdgeIndex;

			if (indices[firstEdgeIndex] == indices[firstEdgeIndex + 1] || indices[firstEdgeIndex + 1] == indices[firstEdgeIndex + 2] || indices[firstEdgeIndex + 2] == indices[firstEdgeIndex])
				return false; // Invalid triangle

			for (unsigned edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
			{
				const unsigned thisEdgeIndex = firstEdgeIndex + edgeIndex;
				const unsigned nextEdgeIndex = (thisEdgeIndex + 1) % 3;
				const unsigned vertA = FindAddVert(indices[edgeIndex]);
				const unsigned vertB = FindAddVert(indices[nextEdgeIndex]);
				const uint64_t edgeId = (static_cast<uint64_t>(vertA) << 32) | vertB;
				auto [thisEdgeIt, newEdge] = halfEdgeMap.emplace(edgeId, MappedEdge{ thisEdgeIndex });

				if (!newEdge)
					return false; // non-manifold edge
				else
				{
					const uint64_t pairId = (static_cast<uint64_t>(vertB) << 32) | vertA;
					auto pairIt = halfEdgeMap.find(pairId);

					outHalfEdgeVerts[thisEdgeIndex] = vertA;
					outHalfEdgeFaces[thisEdgeIndex].index = triIndex;
					outHalfEdgeNexts[thisEdgeIndex] = firstEdgeIndex + nextEdgeIndex;

					outVertEdges[vertA] = firstEdgeIndex + edgeIndex;

					if (pairIt != halfEdgeMap.end())
					{
						const MappedEdge pair = pairIt->second;

						if (pair.occupied)
							return false; // non-manifold edge
						else
						{
							outHalfEdgePairs[pair.edgeIndex] = thisEdgeIndex;
							outHalfEdgePairs[thisEdgeIndex] = pair.edgeIndex;

							pairIt->second.occupied = 1;
							thisEdgeIt->second.occupied = 1;
						}
					}
					else
					{
						outHalfEdgePairs[thisEdgeIndex] = EDGE_NONE;
					}
				}
			}
		}

		// Add imaginary boundary faces
		std::vector<unsigned> outBoundaries;
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

				// Correct the last added next to point back to the start, ad it's supposed to
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
