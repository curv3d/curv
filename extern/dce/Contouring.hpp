#ifndef DCE_CONTOURING_HPP
#define DCE_CONTOURING_HPP

#include <dce/Array3D.hpp>
#include <array>


namespace dce
{	
	/* For all p so that
	 dot(p, normal) == dist
	 */
	struct Plane {
		real dist;    // Signed distance to closest feature
		Vec3 normal;  // Unit-length normal of the feature
		
		bool valid() const { return normal != Zero; }
	};

	typedef Array3D<Plane> Field;
	
	typedef std::array<unsigned, 3> Triangle;
	
	// CW <-> CCW
	inline Triangle flip(Triangle t) {
		return {{ t[0], t[2], t[1] }};
	}
	

	struct TriMesh {
		std::vector<Vec3>  vecs;
		std::vector<Triangle>    triangles;  // Indices into vecs
	};


	/*
	Implementation of:

		Dual Contouring on Hermite Data
		Proceedings of ACM SIGGRAPH, 2002
		Tao Ju, Frank Losasso, Scott Schaefer and Joe Warren 
		http://www.cs.wustl.edu/~taoju/research/dualContour.pdf

	Will use the same resolution as the field for the dual contouring.

	No simplification.
	*/
	TriMesh dualContouring(const Field& field);
}

#endif
