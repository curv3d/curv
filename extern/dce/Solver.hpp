#ifndef DCE_SOLVER_HPP
#define DCE_SOLVER_HPP

#include <dce/Vec3.hpp>

namespace dce
{
	/*
	Various simple low-dimensional linear algebra solvers
	(I should use Eigen for this.... but I'm on a train.)
	*/


	/*
	 Solves for x in  A*x = b.
	 'A' contains the matrix row-wise.
	 'b' and 'x' are column vectors.
	 */
	Vec3 solve3x3(const real* A, const real b[3]);


	/*
	Solves A*x = b for over-determined systems.

	Solves using  At*A*x = At*b   trick where At is the transponate of A.

	Note that each Vec3 in A is a row in the A-matrix.

	N is the length of A and b.
	*/
	Vec3 leastSquares(size_t N, const Vec3* A, const real* b);
}

#endif
