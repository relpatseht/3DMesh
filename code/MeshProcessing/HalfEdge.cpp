#include <algorithm>
#include <unordered_map>
#include <cassert>
#include "MeshProc/HalfEdge.h"

#define sanity(X) if(!(X)) __debugbreak()

namespace mesh
{
	void half_edge::Construct(const unsigned* indices, unsigned triCount, Topology* outMesh)
	{
		static constexpr unsigned HE_NONE = ~0u;
		const unsigned realHalfEdgeCount = triCount * 6;
		std::vector<unsigned> indexVertMap;
		std::vector<unsigned> outVertHEs;
		std::vector<Vert> outVerts;
		std::vector<unsigned> outFaceHEs(triCount);
		std::vector<unsigned> outFaces(triCount);
		std::vector<unsigned> outHEVerts(realHalfEdgeCount);
		std::vector<FaceIndex> outHEFaces(realHalfEdgeCount);
		std::vector<unsigned> outHENexts(realHalfEdgeCount);
		std::unordered_map<uint64_t, unsigned> halfEdgeMap;
		auto FindAddVert = [&indexVertMap, &outVerts, &outVertHEs](unsigned vertIndex)
		{
			if (vertIndex > indexVertMap.size())
				indexVertMap.resize(vertIndex + 1, HE_NONE);

			if (indexVertMap[vertIndex] == HE_NONE)
			{
				indexVertMap[vertIndex] = static_cast<unsigned>(outVerts.size());
				outVerts.emplace_back(Vert{ {vertIndex, 0} });
				outVertHEs.emplace_back(HE_NONE);

				assert(outVerts.back().realIndex == vertIndex && "Vert::realIndex overflow.");
			}

			return indexVertMap[vertIndex];
		};

		// Build real topology
		for (unsigned triIndex = 0; triIndex < triCount; ++triIndex)
		{
			const unsigned* const triIndices = indices + triIndex * 3;
			const unsigned verts[3] = { FindAddVert(triIndices[0]), FindAddVert(triIndices[1]), FindAddVert(triIndices[2]) };

			outFaces[triIndex] = triIndex;

			sanity(!(triIndices[0] == triIndices[1] || triIndices[1] == triIndices[2] || triIndices[2] == triIndices[0]) && "Degenerate tri detected");

			for (unsigned vertIndex = 0; vertIndex < 3; ++vertIndex)
			{
				const unsigned nextVertIndex = (vertIndex + 1) % 3;
				const unsigned vertA = verts[vertIndex];
				const unsigned vertB = verts[nextVertIndex];
				const uint64_t edgeId = (static_cast<uint64_t>(vertA) << 32) | vertB;
				auto edgeIt = halfEdgeMap.find(edgeId);

				if (edgeIt == halfEdgeMap.end())
				{
					const uint64_t reverseEdge = (static_cast<uint64_t>(vertB) << 32) | vertA;
					const unsigned newHalfEdge = static_cast<unsigned>(outHEVerts.size());

					outHEVerts.resize(outHEVerts.size() + 2, HE_NONE);
					outHEFaces.resize(outHEFaces.size() + 2, FaceIndex{ HE_NONE, FaceType::COUNT });
					outHENexts.resize(outHENexts.size() + 2, HE_NONE);

					sanity((newHalfEdge & 1) == 0);

					halfEdgeMap.emplace(reverseEdge, newHalfEdge + 1);
					edgeIt = halfEdgeMap.emplace(edgeId, newHalfEdge).first;
				}
				else
				{
					sanity((edgeIt->second & 1) == 1 && "Non-manifold edge detected");
				}

				const unsigned halfEdge = edgeIt->second;

				sanity(outHEVerts[halfEdge] == HE_NONE && "Non-manifold edge detected");
				sanity(outHEFaces[halfEdge].type == FaceType::COUNT && "Non-manifold edge detected");

				outHEVerts[halfEdge] = vertA;
				outHEFaces[halfEdge].index = triIndex;
				outHEFaces[halfEdge].type = FaceType::REAL;
				sanity(outHEFaces[halfEdge].index == triIndex && "FaceIndex::index overflow");

				outVertHEs[vertA] = halfEdge;
			}

			unsigned prevVert = verts[2];
			unsigned prevVertHE = outVertHEs[prevVert];
			for (unsigned vertIndex = 0; vertIndex < 3; ++vertIndex)
			{
				const unsigned curVert = verts[vertIndex];
				const unsigned curVertHE = outVertHEs[curVert];

				outHENexts[prevVertHE] = curVertHE;

				sanity(outHENexts[prevVertHE] == HE_NONE && "Non-manifold edge detected");

				prevVert = curVert;
				prevVertHE = curVertHE;
			}

			outFaceHEs[triIndex] = prevVertHE;
		}

		// Add imaginary boundary faces
		std::vector<unsigned> outBoundaryHEs;
		for (size_t edgeIndex = 0; edgeIndex < outHalfEdgePairs.size(); ++edgeIndex)
		{
			if (outHalfEdgePairs[edgeIndex] == EDGE_NONE)
			{
				const unsigned boundaryIndex = static_cast<unsigned>(outBoundaries.size());
				const FaceIndex boundaryFace{ boundaryIndex, FaceType::BOUNDARY };
				unsigned boundaryFlipIndex = edgeIndex;

				outBoundaries.emplace_back(boundaryIndex);

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
