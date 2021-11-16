#pragma once

// Libs
#include <iostream>
#include <cmath>

namespace dmc {

	struct Vector {
		double p[3]{ 0,0,0 };
		/// normalize vector
		void normalize() {
			const double s_ = std::sqrt(this->p[0] * this->p[0] + this->p[1] * this->p[1] + this->p[2] * this->p[2]);
			this->p[0] /= s_;
			this->p[1] /= s_;
			this->p[2] /= s_;
		}

        /// access raw data
        auto data() { return p; }

		/// Data access with subscript operator
		double& operator[](size_t i) { return p[i]; }
		/// Data access with subscript operator for constant objects.
		const double& operator[](size_t i) const { return p[i]; }

		/// Compound assignment operator <b>+</b>
		Vector& operator+=(const Vector& rhs) {
			this->p[0] += rhs.p[0];
			this->p[1] += rhs.p[1];
			this->p[2] += rhs.p[2];
			return *this;
		}
		/// Compound assignment operator <b>-</b>
		Vector& operator-=(const Vector& rhs) {
			this->p[0] -= rhs.p[0];
			this->p[1] -= rhs.p[1];
			this->p[2] -= rhs.p[2];
			return *this;
		}

		/// Sum of two different vertocs
		friend Vector operator+(Vector lhs, const Vector& rhs) {
			lhs[0] += rhs[0];
			lhs[1] += rhs[1];
			lhs[2] += rhs[2];
			return lhs;
		}
		/// Difference of two different vertocs
		friend Vector operator-(Vector lhs, const Vector& rhs) {
			lhs[0] -= rhs[0];
			lhs[1] -= rhs[1];
			lhs[2] -= rhs[2];
			return lhs;
		}

		/// Compound component wise multiplication with a scalar
		Vector& operator*=(const double val) {
			this->p[0] *= val;
			this->p[1] *= val;
			this->p[2] *= val;
			return *this;
		}

		/// Compound component wise division with a scalar
		Vector& operator/=(const double val) {
			this->p[0] /= val;
			this->p[1] /= val;
			this->p[2] /= val;
			return *this;
		}

		/// Unary minus operator
		Vector operator-() {
			Vector v{ -this->p[0],-this->p[1],-this->p[2] };
			return v;
		}

		/// Invert vector direction, i.e. multiply by -1
		void flip() {
			this->p[0] = -this->p[0];
			this->p[1] = -this->p[1];
			this->p[2] = -this->p[2];
		}

		/// Overload the cout
		friend std::ostream& operator<<(std::ostream& os, const Vector& v)
		{
			os << "v[0] = " << v[0] << ", v[1] = " << v[1] << ", v[2] = " << v[2] << std::endl;
			return os;
		}
		/// check if vector values are inf
		bool isinf()
		{
			if (std::isinf(p[0]) || std::isinf(p[2]) || std::isinf(p[2]))
				return true;
			else
				return false;
		}
		/// check if vector values are inf
		bool isnan()
		{
			if (std::isnan(p[0]) || std::isnan(p[2]) || std::isnan(p[2]))
				return true;
			else
				return false;
		}
	};

	inline Vector operator*(const Vector& iv, const double val) {
		Vector r = iv;
		r[0] *= val;
		r[1] *= val;
		r[2] *= val;
		return r;
	}
	inline Vector operator*(const double val, const Vector& iv) {
		Vector r = iv;
		r[0] *= val;
		r[1] *= val;
		r[2] *= val;
		return r;
	}

	inline Vector operator+(const Vector& iv, const double val) {
		Vector r = iv;
		r[0] += val;
		r[1] += val;
		r[2] += val;
		return r;
	}
	inline Vector operator+(const double val, const Vector& iv) {
		Vector r = iv;
		r[0] += val;
		r[1] += val;
		r[2] += val;
		return r;
	}

	inline Vector operator-(const Vector& iv, const double val) {
		Vector r = iv;
		r[0] -= val;
		r[1] -= val;
		r[2] -= val;
		return r;
	}
	inline Vector operator-(const double val, const Vector& iv) {
		Vector r = iv;
		r[0] -= val;
		r[1] -= val;
		r[2] -= val;
		return r;
	}

	inline Vector operator/(const Vector& iv, const double val) {
		Vector r = iv;
		r[0] /= val;
		r[1] /= val;
		r[2] /= val;
		return r;
	}


	// helper functions
	inline double distance(const Vector& p1, const Vector& p2) {
		double d = (p1[0] - p2[0])*(p1[0] - p2[0]) + (p1[1] - p2[1])*(p1[1] - p2[1]) + (p1[2] - p2[2])*(p1[2] - p2[2]);
		return std::sqrt(d);
	}
	inline double norm(const Vector& v) {
		return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	}
	inline void normalize(Vector& v) {
		double s_ = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
		v[0] /= s_;
		v[1] /= s_;
		v[2] /= s_;
	}
	inline double dot(const Vector& v1, const Vector& v2) {
		return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
	}
	inline Vector cross(const Vector& v1, const Vector& v2) {
		Vector nv{ v1[1] * v2[2] - v1[2] * v2[1], v1[2] * v2[0] - v1[0] * v2[2], v1[0] * v2[1] - v1[1] * v2[0] };
		return nv;
	}

}// namespace homotopy
