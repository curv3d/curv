#include <dce/Solver.hpp>
#include <iostream>  // cerr for error output (quick-n-dirty)

namespace dce
{
	real determinant(
		real a, real b, real c,
		real d, real e, real f,
		real g, real h, real i )
	{
		return a * e * i 
		     + b * f * g
		     + c * d * h
		     - a * f * h
		     - b * d * i
		     - c * e * g;
	}


	/* Solves for x in  A*x = b.
	 'A' contains the matrix row-wise.
	 'b' and 'x' are column vectors.
	Uses cramers rule.
	*/
	Vec3 solve3x3(const real* A, const real b[3]) {
		auto det = determinant(
			A[0*3+0], A[0*3+1], A[0*3+2],
			A[1*3+0], A[1*3+1], A[1*3+2],
			A[2*3+0], A[2*3+1], A[2*3+2]);

		if (abs(det) <= 1e-12) {
			std::cerr << "Oh-oh - small determinant: " << det << std::endl;
			return Vec3(NAN);
		}

		return Vec3 {
			determinant(
				b[0],    A[0*3+1], A[0*3+2],
				b[1],    A[1*3+1], A[1*3+2],
				b[2],    A[2*3+1], A[2*3+2] ),

			determinant(
				A[0*3+0], b[0],    A[0*3+2],
				A[1*3+0], b[1],    A[1*3+2],
				A[2*3+0], b[2],    A[2*3+2] ),

			determinant(
				A[0*3+0], A[0*3+1], b[0]   ,
				A[1*3+0], A[1*3+1], b[1]   ,
				A[2*3+0], A[2*3+1], b[2]   )

		} / det;
	}

	/*
	Solves A*x = b for over-determined systems.

	Solves using  At*A*x = At*b   trick where At is the transponate of A
	*/
	Vec3 leastSquares(size_t N, const Vec3* A, const real* b)
	{
		if (N == 3) {
			const real A_mat[3*3] = {
				A[0].x, A[0].y, A[0].z,
				A[1].x, A[1].y, A[1].z,
				A[2].x, A[2].y, A[2].z,
			};
			return solve3x3(A_mat, b);
		}
		
		real At_A[3][3];
		real At_b[3];

		for (int i=0; i<3; ++i) {
			for (int j=0; j<3; ++j) {
				real sum = 0;
				for (size_t k=0; k<N; ++k) {
					sum += A[k][i] * A[k][j];
				}
				At_A[i][j] = sum;
			}
		}

		for (int i=0; i<3; ++i) {
			real sum = 0;

			for (size_t k=0; k<N; ++k) {
				sum += A[k][i] * b[k];
			}

			At_b[i] = sum;
		}
		
		
		/*
		// Improve conditioning:
		real offset = 0.0001;
		At_A[0][0] += offset;
		At_A[1][1] += offset;
		At_A[2][2] += offset;
		 */
		
		static_assert(sizeof(At_A) == 9*sizeof(real), "pack");

		return solve3x3(&At_A[0][0], At_b);
	}
}
