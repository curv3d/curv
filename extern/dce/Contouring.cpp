#include <dce/Contouring.hpp>
#include <dce/Settings.hpp>
#include <dce/Solver.hpp>
#include <iostream> // cerr


namespace dce
{
	
#pragma clang diagnostic ignored "-Wmissing-braces"
	typedef std::array<int, 2> Edge;
	

	const int NumCorners = 8;
	const int NumEdges   = 3*4;

	const Vec3u Corners[NumCorners] = {
		{0,0,0}, {0,0,1}, {0,1,0}, {0,1,1},
		{1,0,0}, {1,0,1}, {1,1,0}, {1,1,1},
	};

	// Indices into Corners:
	const Edge Edges[NumEdges] = {
		{0,1}, {0,2}, {0,4},
		{1,3}, {1,5},
		{2,3}, {2,6},
		{3,7},
		{4,5}, {4,6},
		{5,7},
		{6,7}
	};
	
	
	struct VoxelInfo {
		int vertIx = -1;  // -1 == no vertex
	};
	
	
	void constructVertices(const Field& field,
								  TriMesh& mesh,
								  Array3D<VoxelInfo>& voxels)
	{
		// Counters:
		int numClamped = 0;
		int numDistant = 0;
				
		// Reused for speed (no dynamic allocations in inner loop):
		std::vector<Plane>  planes;
		std::vector<Vec3>   A;
		std::vector<real>   b;
		
		foreach3D(voxels.size(), [&](const Vec3u& p)
		{
			bool inside[NumCorners];
			int  numInside = 0;
			
			for (int ci=0; ci<NumCorners; ++ci) {
				inside[ci] = (field[p + Corners[ci]].dist <= 0);
				if (inside[ci]) {
					numInside += 1;
				}
			}
			
			if (numInside == 0 || numInside == NumCorners) {
				// Fully contained, or fully excluded voxel
				return;
			}
			
			bool crossingCorners[NumCorners] = { false };
			
			auto& vox = voxels[p];
			for (int ai=0; ai<NumEdges; ++ai) {
				auto&& e = Edges[ai];
				if (inside[e[0]] != inside[e[1]]) {
					crossingCorners[e[0]] = true;
					crossingCorners[e[1]] = true;
				}
			}
			
			planes.clear();
			A.clear();
			b.clear();
			
			
			/* Add candidate planes from all corners with a sign-changing edge:  */
			for (int ci=0; ci<NumCorners; ++ci) {
				const auto  p_n = p + Corners[ci];
				auto plane = field[p_n];
				if (!plane.valid()) {
					continue; // Broken field cell (bad input)
				}
				
				if (!crossingCorners[ci]) {
					// Corner has no edge with sign change
					continue;
				}
				
				if (std::fabs(plane.dist) > MaxCornerDist) {
					// Large distance change - produced by bad input
					// This will most likely cause a bad vertex, so skip this corner
					continue;
				}
				
				plane.dist = dot(plane.normal, Vec3(p_n)) - plane.dist;
				planes.push_back( plane );
			}
			
			/*
			 Add a weak 'push' towards the voxel center to improve conditioning.
			 This is needed for any surface which is flat in at least one dimension, including a cylinder.
			 
			 We could do only as needed (when lastSquared have failed once),
			 but the push is so weak that it makes little difference to the precision of the model.
			 */
			for (int ai=0; ai<3; ++ai) {
				Vec3 normal = CenterPush * Vec3::Axes[ai];
				Vec3 pos = Vec3(p) + Vec3(0.5);
				planes.push_back( Plane{dot(normal, pos), normal} );
			}
			
			for (auto&& p : planes) {
				A.push_back(p.normal);
				b.push_back(p.dist);
			}
			
			Vec3 vertex = leastSquares(A.size(), A.data(), b.data());
			
			auto voxelCenter = Vec3(p) + Vec3(0.5);
			
			if (!isFinite(vertex)) {
				std::cerr << "leastSquares failed " << std::endl;
				vertex = voxelCenter;
			}
			
			auto clamped = clamp(vertex, Vec3(p), Vec3(p) + Vec3(1));
			
			if (Clamp)
			{
				if (vertex != clamped)
				{
					vertex = clamped;
					++numClamped;
				}
			}
			else if (dist(voxelCenter, vertex) > FarAway) {
				vertex = clamped;
				++numDistant;
			}
			
			vox.vertIx = mesh.vecs.size();
			mesh.vecs.push_back( vertex );
		});
		
		
		if (numClamped > 0) {
			std::cerr << "Clamped " << numClamped << " vertices to voxels" << std::endl;
		}
		
		if (numDistant > 0) {
			std::cerr << "Warning: " << numDistant << " vertices far outside voxel. They where clamped." << std::endl;
		}
	}
	
	
	void constructFaces(const Field& field,
	                    TriMesh& mesh,
	                    const Array3D<VoxelInfo>& voxels)
	{
		// The edges leading to our far corner:
		const Edge FarEdges[3] = {{3,7}, {5,7}, {6,7}};
		
		
		foreach3D(voxels.size(), [&](const Vec3u& p)
		{			
			auto&& vox = voxels[p];
			auto v0 = vox.vertIx;
			
			if (v0 == -1) {
				// Voxel has no vertex (fully contained or excluded)
				return;
			}
			
			bool inside[NumCorners];
			
			for (int ci=0; ci<NumCorners; ++ci) {
				inside[ci] = (field[p + Corners[ci]].dist <= 0);
			}
			
			for (int ai=0; ai<3; ++ai) {
				auto&& e = FarEdges[ai];
				if (inside[e[0]] == inside[e[1]]) {
					continue;  // Not a crossing
				}
				
				int v1,v2,v3;
				
				if (ai == 0) {
					v1 = voxels[p + Vec3u(0,0,1)].vertIx;
					v2 = voxels[p + Vec3u(0,1,0)].vertIx;
					v3 = voxels[p + Vec3u(0,1,1)].vertIx;
				} else if (ai == 1) {
					v1 = voxels[p + Vec3u(0,0,1)].vertIx;
					v2 = voxels[p + Vec3u(1,0,0)].vertIx;
					v3 = voxels[p + Vec3u(1,0,1)].vertIx;
				} else {
					v1 = voxels[p + Vec3u(0,1,0)].vertIx;
					v2 = voxels[p + Vec3u(1,0,0)].vertIx;
					v3 = voxels[p + Vec3u(1,1,0)].vertIx;
				}
				
				if (v1 < 0 || v2 < 0 || v3 < 0)
				{
					// Shouldn't ever happen
					std::cerr << "Bad triangle: the model will contain holes! " << std::endl;
					continue;
				}
				
				typedef unsigned uint;
				
				Triangle t0 = { (uint)v0, (uint)v1, (uint)v3 };
				Triangle t1 = { (uint)v0, (uint)v3, (uint)v2 };
				
				// Get the normals right:
				if (inside[e[0]] != (ai == 1))  // xor
				{
					t0 = flip(t0);
					t1 = flip(t1);
				}
				
				mesh.triangles.push_back( t0 );
				mesh.triangles.push_back( t1 );
			}
		});
	}


	TriMesh dualContouring(const Field& field)
	{
		/* The field describes values at the vertices (voxel corners).
		The size of the voxel grid is therefore one less on each dimension
		*/

		Array3D<VoxelInfo> voxels(field.size() - Vec3u(1), VoxelInfo{});
		TriMesh mesh;

		constructVertices(field, mesh, voxels);
		constructFaces(field, mesh, voxels);

		
		return mesh;
	}
}
