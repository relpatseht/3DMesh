#include <algorithm>
#include <unordered_map>
#include <cassert>
#include "MeshProc/HalfEdge.h"
#include "BitSet.h"
#include "sanity.h"

namespace
{
	using namespace mesh::half_edge;

	static unsigned EdgeLoopLength(const Topology& mesh, unsigned halfEdge)
	{
		const unsigned heStart = halfEdge;
		unsigned loopLength = 0;

		do
		{
			sanity(halfEdge < mesh.halfEdgeNexts.size());

			++loopLength;
			halfEdge = mesh.halfEdgeNexts[halfEdge];
		} while (halfEdge != heStart);

		return loopLength;
	}

	namespace singularity
	{
		namespace singularity_internal
		{
			static unsigned NextSingularityStart(const Topology& mesh, BitSet* workBitSet, const unsigned startHalfEdge, unsigned halfEdge)
			{
				const unsigned boundaryFaceIndex = mesh.halfEdgeFaces[startHalfEdge].index;

				do
				{
					const unsigned halfEdgeVertIndex = mesh.halfEdgeVerts[halfEdge];

					sanity(mesh.halfEdgeFaces[halfEdge].type == FaceType::BOUNDARY);
					sanity(mesh.halfEdgeFaces[halfEdge].index == boundaryFaceIndex);

					if (BitSet_IsSet(*workBitSet, halfEdgeVertIndex))
					{
						break;
					}
					else
					{
						BitSet_Set(workBitSet, halfEdgeVertIndex);
					}

					halfEdge = mesh.halfEdgeNexts[halfEdge];
				} while (halfEdge != startHalfEdge);

				return halfEdge;
			}

			static unsigned SplitBoundarySingularities(Topology* inoutMesh, BitSet* workBitSet, unsigned firstHalfEdge)
			{
				for (unsigned loopStartHE = NextSingularityStart(*inoutMesh, workBitSet, firstHalfEdge, firstHalfEdge); loopStartHE != firstHalfEdge; loopStartHE = NextSingularityStart(*inoutMesh, workBitSet, firstHalfEdge, loopStartHE))
				{
					const FaceIndex oldBoundary = inoutMesh->halfEdgeFaces[loopStartHE];
					const FaceIndex newBoundary{ (unsigned)inoutMesh->faceHalfEdges[oldBoundary.type].size(), oldBoundary.type };
					const unsigned loopStartVertIndex = inoutMesh->halfEdgeVerts[loopStartHE];
					const unsigned newVertIndex = static_cast<unsigned>(inoutMesh->verts.size());
					Vert* const newVert = &inoutMesh->verts.emplace_back(inoutMesh->verts[loopStartVertIndex]);
					const unsigned newSplitIndex = newVert->splitIndex + 1;
					unsigned curHalfEdge = inoutMesh->halfEdgeNexts[loopStartHE];
					unsigned prevHalfEdge = loopStartHE;

					// Cut the into two boundaries at the singularity, tracking the edge pointing to the vert
					{
						sanity(newBoundary.index == inoutMesh->faceHalfEdges[oldBoundary.type].size() && "FaceIndex::type overflow");
						sanity(inoutMesh->halfEdgeVerts[curHalfEdge] != loopStartVertIndex);

						inoutMesh->faceHalfEdges[newBoundary.type].emplace_back(loopStartHE);

						do
						{
							inoutMesh->halfEdgeFaces[prevHalfEdge] = newBoundary;

							prevHalfEdge = curHalfEdge;
							curHalfEdge = inoutMesh->halfEdgeNexts[curHalfEdge];
						} while (inoutMesh->halfEdgeVerts[curHalfEdge] != loopStartVertIndex);

						inoutMesh->halfEdgeNexts[prevHalfEdge] = loopStartHE;
					}

					// Update the new verts split index and move all edges off of it to the new index
					{
						newVert->splitIndex = newSplitIndex;
						sanity(newVert->splitIndex == newSplitIndex && "mesh::half_edge::Vert::splitIndex overflow");

						curHalfEdge = prevHalfEdge ^ 1;
						do
						{
							curHalfEdge = inoutMesh->halfEdgeNexts[curHalfEdge];

							sanity(inoutMesh->verts[inoutMesh->halfEdgeVerts[curHalfEdge]].realIndex == newVert->realIndex);
							sanity(inoutMesh->halfEdgeFaces[curHalfEdge].type != FaceType::BOUNDARY);

							inoutMesh->halfEdgeVerts[curHalfEdge] = newVertIndex;
							curHalfEdge ^= 1;
						} while (curHalfEdge != loopStartHE);

						inoutMesh->halfEdgeVerts[loopStartHE] = newVertIndex;
					}
				}
			}
		}

		static unsigned SplitSingularities(Topology* inoutMesh)
		{
			const size_t numVerts = inoutMesh->verts.size();
			const size_t maxNewVerts = numVerts >> 2; // 25% verts created from splitting singularities should be unreasonably high
			const size_t maxVerts = numVerts + maxNewVerts;
			ScopedBitSet vertBitSet;

			BitSet_Create(&vertBitSet, maxVerts);

			for (unsigned boundaryHE : inoutMesh->faceHalfEdges[FaceType::BOUNDARY])
			{
				BitSet_ClearAll(&vertBitSet);
				singularity_internal::SplitBoundarySingularities(inoutMesh, &vertBitSet, boundaryHE);
			}
		}
	}

	namespace hole
	{
		namespace hole_internal
		{

		}

		static void FillHoles(Topology* inoutMesh)
		{
ToDo: This.
		}
	}


	namespace validate
	{
		namespace validate_internal
		{
			static bool HasIsolatedVertices(const Topology& mesh)
			{
				for (unsigned vertIndex : mesh.halfEdgeVerts)
				{
					if (vertIndex > mesh.verts.size())
						return true;
				}

				return false;
			}

			static bool HasSingularities(const Topology& mesh)
			{
				const size_t vertFaceCountSize = mesh.verts.size() * sizeof(uint8_t);
				uint8_t* const vertFaceCount = (uint8_t*)_malloca(vertFaceCountSize);

				memset(vertFaceCount, 0, vertFaceCountSize);

				for (unsigned type = 0; type < FaceType::COUNT; ++type)
				{
					for (unsigned faceHE : mesh.faceHalfEdges[type])
					{
						unsigned curHE = faceHE;

						do
						{
							const unsigned heVert = mesh.halfEdgeVerts[curHE];

							sanity(heVert < mesh.verts.size());
							++vertFaceCount[heVert];

							curHE = mesh.halfEdgeNexts[curHE];
						} while (curHE != faceHE);
					}
				}

				for (size_t vertIndex = 0; vertIndex < mesh.verts.size(); ++vertIndex)
				{
					const unsigned vertHE = mesh.vertHalfEdges[vertIndex];
					unsigned curHE = vertHE;
					unsigned vertDegree = 0;

					do
					{
						++vertDegree;
						curHE = mesh.halfEdgeNexts[curHE ^ 1];
					} while (curHE != vertHE);

					if (vertDegree != vertFaceCount[vertIndex])
						return true;
				}

				return false;
			}
		}

		static bool ValidateBoundaries(const Topology& mesh)
		{
			for (unsigned boundaryHE : mesh.faceHalfEdges[FaceType::BOUNDARY])
			{
				const unsigned boundaryLen = EdgeLoopLength(mesh, boundaryHE);

				if (boundaryLen < 3)
				{
					sanity(0 && "Mesh has overlapping faces");
					return false;
				}
			}

			return true;
		}

		static bool ValidateVertices(const Topology& mesh)
		{
			if (validate_internal::HasIsolatedVertices(mesh))
			{
				sanity(0 && "Mesh has verts attached to no edges");
				return false;
			}

			if (validate_internal::HasSingularities(mesh))
			{
				sanity(0 && "Mesh has vertices on multiple boundaries we failed to correct for");
				return false;
			}

			return true;
		}

		static bool ValidateMesh(const Topology& mesh)
		{
			return ValidateBoundaries(mesh) && ValidateVertices(mesh);
		}
	}

}

namespace mesh
{
	bool half_edge::Construct(const unsigned* indices, unsigned triCount, Topology* outMesh)
	{
		static constexpr unsigned HE_NONE = ~0u;
		const unsigned realHalfEdgeCount = triCount * 6;
		std::vector<unsigned> indexVertMap;
		std::vector<unsigned> outVertHEs;
		std::vector<Vert> outVerts;
		std::vector<unsigned> outFaceHEs(triCount);
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

		sanity(outHEFaces.size() == outHENexts.size() && outHEFaces.size() == outHEVerts.size());

		// Add imaginary boundary faces
		std::vector<unsigned> outBoundaryHEs;
		for (size_t heIndex = 0; heIndex < outHENexts.size(); ++heIndex)
		{
			if (outHENexts[heIndex] == HE_NONE)
			{
				const unsigned boundaryIndex = static_cast<unsigned>(outBoundaryHEs.size());
				const FaceIndex boundaryFace{ boundaryIndex, FaceType::BOUNDARY };
				unsigned boundaryHEIndex = heIndex;
				unsigned boundaryLoopLen = 0;

				outBoundaryHEs.emplace_back(boundaryHEIndex);

				// Create the new face by walking around the unpaired edges
				do
				{
					const unsigned boundaryHEFlipIndex = boundaryHEIndex ^ 1;
					unsigned boundaryNextIndex = outHENexts[boundaryHEFlipIndex] ^ 1;

					// Find next unpaired edge
					while (outHENexts[boundaryNextIndex] != HE_NONE)
						boundaryNextIndex = outHENexts[boundaryNextIndex] ^ 1;

					++boundaryLoopLen;

					outHEFaces[boundaryHEIndex] = boundaryFace;
					outHEVerts[boundaryHEIndex] = outHEVerts[boundaryHEFlipIndex];
					outHENexts[boundaryNextIndex] = boundaryHEIndex; // reverse order for boundary face
				} while (boundaryHEIndex != heIndex);

				sanity(boundaryLoopLen >= 3 && "Overlapping faces");
			}
		}

		sanity(outHEFaces.size() == outHENexts.size() && outHEFaces.size() == outHEVerts.size());

		for (unsigned halfEdge : outVertHEs)
			sanity(halfEdge < outHENexts.size());

		for (unsigned halfEdge : outFaceHEs)
			sanity(halfEdge < outHENexts.size());

		for (unsigned halfEdge : outBoundaryHEs)
			sanity(halfEdge < outHENexts.size());

		for(unsigned vertIndex : outHEVerts)
			sanity(vertIndex < outVerts.size());

		for (unsigned halfEdge : outHENexts)
			sanity(halfEdge < outHENexts.size());

		for (FaceIndex faceIndex : outHEFaces)
		{
			switch (faceIndex.type)
			{
			case FaceType::REAL:
				sanity(faceIndex.index < outFaceHEs.size());
				break;
			case FaceType::BOUNDARY:
				sanity(faceIndex.index < outBoundaryHEs.size());
				break;
			default:
				sanity(0 && "Unreachable");
			}
		}

		outMesh->vertHalfEdges = std::move(outVertHEs);
		outMesh->verts = std::move(outVerts);

		outMesh->faceHalfEdges[FaceType::REAL] = std::move(outFaceHEs);
		outMesh->faceHalfEdges[FaceType::BOUNDARY] = std::move(outBoundaryHEs);

		outMesh->halfEdgeVerts = std::move(outHEVerts);
		outMesh->halfEdgeFaces = std::move(outHEFaces);
		outMesh->halfEdgeNexts = std::move(outHENexts);

		singularity::SplitSingularities(outMesh);

		if (!validate::ValidateMesh(*outMesh))
			return false;

		hole::FillHoles(outMesh);

		return validate::ValidateVertices(*outMesh);
	}
}
