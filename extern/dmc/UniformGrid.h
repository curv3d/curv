#pragma once
#pragma once

// Libs
#include <fstream>
#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>

// project
#include "Vector.h"

namespace dmc {

	class UniformGrid {
	public:
		using ushort = unsigned short;
		using Vertex = dmc::Vector;
		using Point = dmc::Vector;
		using Normal = dmc::Vector;
		using Index = std::array<int, 3>;
		using BBox = std::array<Point, 8>;
	public:
		void init(const std::string& filename);
		void init(const int nx, const int ny, const int nz);
		void init(const int nx, const int ny, const int nz, BBox& bb);
		void init(const int nx, const int ny, const int nz, BBox& bb, const double val);
		/// copy
		void copy(const UniformGrid& ug)
		{
			m_nx = ug.m_nx; //!< grid size in x-direction
			m_ny = ug.m_ny; //!< grid size in y-direction
			m_nz = ug.m_nz; //!< grid size in z-direction
			m_dx = ug.m_dx; //!< grid spacing in x-direction
			m_dy = ug.m_dy; //!< grid spacing in y-direction
			m_dz = ug.m_dz; //!< grid spacing in z-direction
			m_bbox = ug.m_bbox; //!< the bounding box of the ugrid.
			m_scalars = ug.m_scalars; //!< scalar values stored in the ugrid
			m_gradient = ug.m_gradient; //!< vector values stored in the ugrid
		}
		/// grid spacing in x-direction.
		double dx() { return m_dx; }
		double dx() const { return m_dx; }
		/// set grid spacing in x-direction
		void set_dx(const double d) { m_dx = d; }
		/// grid spacing in y-direction.
		double dy() { return m_dy; }
		double dy() const { return m_dy; }
		/// set grid spacing in y-direction
		void set_dy(const double d) { m_dy = d; }
		/// grid spacing in z-direction.
		double dz() { return m_dz; }
		double dz() const { return m_dz; }
		/// set grid spacing in z-direction
		void set_dz(const double d) { m_dz = d; }
		/// total number of grid points.
		int size() { return m_nx * m_ny * m_nz; }
		int size() const { return m_nx * m_ny * m_nz; }
		/// grid size in x-direction.
		int x_size() { return m_nx; }
		int x_size() const { return m_nx; }
		/// grid size in y-direction.
		int y_size() { return m_ny; }
		int y_size() const { return m_ny; }
		/// grid size in z-direction.
		int z_size() { return m_nz; }
		int z_size() const { return m_nz; }
		/// bounding box size
		std::array<Point, 8> bbox() { return m_bbox; }
		std::array<Point, 8> bbox() const { return m_bbox; }
		void bbox(const double xmin, const double xmax, const double ymin, const double ymax, const double zmin, const double zmax)
		{
			// v0
			m_bbox[0][0] = xmin;
			m_bbox[0][1] = ymin;
			m_bbox[0][2] = zmin;
			// v1
			m_bbox[1][0] = xmax;
			m_bbox[1][1] = ymin;
			m_bbox[1][2] = zmin;
			// v2
			m_bbox[2][0] = xmin;
			m_bbox[2][1] = ymax;
			m_bbox[2][2] = zmin;
			// v3
			m_bbox[3][0] = xmax;
			m_bbox[3][1] = ymax;
			m_bbox[3][2] = zmin;
			// v4
			m_bbox[4][0] = xmin;
			m_bbox[4][1] = ymin;
			m_bbox[4][2] = zmax;
			// v5
			m_bbox[5][0] = xmax;
			m_bbox[5][1] = ymin;
			m_bbox[5][2] = zmax;
			// v6
			m_bbox[6][0] = xmin;
			m_bbox[6][1] = ymax;
			m_bbox[6][2] = zmax;
			// v7
			m_bbox[7][0] = xmax;
			m_bbox[7][1] = ymax;
			m_bbox[7][2] = zmax;
		}
		double minX() { return m_bbox[0][0]; }
		double minX() const { return m_bbox[0][0]; }
		double minY() { return m_bbox[0][1]; }
		double minY() const { return m_bbox[0][1]; }
		double minZ() { return m_bbox[0][2]; }
		double minZ() const { return m_bbox[0][2]; }
		double maxX() { return m_bbox[7][0]; }
		double maxX() const { return m_bbox[7][0]; }
		double maxY() { return m_bbox[7][1]; }
		double maxY() const { return m_bbox[7][1]; }
		double maxZ() { return m_bbox[7][2]; }
		double maxZ() const { return m_bbox[7][2]; }

		/// Returns a point with the euclidean position of the vertex (i,j,k).
		/** This methods does not check if vertex is within the uniform grid
		*  It returns the vertex position for an infinite grid.
		*  @param[in] i cell index along x-coordinate
		*  @param[in] i cell index along x-coordinate
		*  @param[in] i cell index along x-coordinate
		*  @return _Point_ with coordinates of vertex {i,j,k}
		*/
		Point point(const int i, const int j, const int k) { return { m_bbox[0][0] + i * m_dx, m_bbox[0][1] + j * m_dy, m_bbox[0][2] + k * m_dz }; }
		Point point(const int i, const int j, const int k) const { return { m_bbox[0][0] + i * m_dx, m_bbox[0][1] + j * m_dy, m_bbox[0][2] + k * m_dz }; }
		Point point(const Index i) { return { m_bbox[0][0] + i[0] * m_dx, m_bbox[0][1] + i[1] * m_dy, m_bbox[0][2] + i[2] * m_dz }; }
		Point point(const Index i) const { return { m_bbox[0][0] + i[0] * m_dx, m_bbox[0][1] + i[1] * m_dy, m_bbox[0][2] + i[2] * m_dz }; }
		Point point(const int gl_index)
		{
			Index i = local_index(gl_index);
			return { m_bbox[0][0] + i[0] * m_dx, m_bbox[0][1] + i[1] * m_dy, m_bbox[0][2] + i[2] * m_dz };
		}
		/// Set all scalar to an input value
		void setScalars(const double val) { std::fill(m_scalars.begin(), m_scalars.end(), val); }
		/// Set the scalar value at grid node using node's global index.
		void scalar(const int gindex, const double val) { m_scalars[gindex] = val; }
		/// set the scalar value at grid node specified by indices (i,j,k).
		void scalar(const int i, const int j, const int k, const double val) { m_scalars[global_index(i, j, k)] = val; }
		/// set the scalar value at grid node specified by Index i.
		void scalar(const Index i, const double val) { m_scalars[global_index(i)] = val; }
		/// returns scalar value at grid node using global node index.
		double scalar(const int gindex) { return m_scalars[gindex]; }
		double scalar(const int gindex) const { return m_scalars[gindex]; }
		/// returns scalar value at grid node specified by indices (i,k,j).
		double scalar(const int i, const int j, const int k) { return m_scalars[global_index(i, j, k)]; }
		double scalar(const int i, const int j, const int k) const { return m_scalars[global_index(i, j, k)]; }
		/// returns scalar value at grid node specified by index i.
		double scalar(const Index i) { return m_scalars[global_index(i)]; }
		double scalar(const Index i) const { return m_scalars[global_index(i)]; }
		/// compute scalar value at input point
		double scalar(Point& p)
		{
			int i0 = static_cast<int>((p[0] - m_bbox[0][0]) / m_dx);
			int j0 = static_cast<int>((p[1] - m_bbox[0][1]) / m_dy);
			int k0 = static_cast<int>((p[2] - m_bbox[0][2]) / m_dz);
			if (i0 >= (m_nx - 1)) {
				i0 = m_nx - 2;
			}
			if (j0 >= (m_ny - 1)) {
				j0 = m_ny - 2;
			}
			if (k0 >= (m_nz - 1)) {
				k0 = m_nz - 2;
			}
			const Point p0 = point(i0, j0, k0);
			//const Point p1 = point(i0+1, j0, k0);
			//const Point p2 = point(i0, j0 + 1, k0);
			//const Point p4 = point(i0, j0, k0 + 1);
			double f[8];
			f[0] = scalar(i0, j0, k0);
			f[1] = scalar(i0+1, j0, k0);
			f[2] = scalar(i0, j0+1, k0);
			f[3] = scalar(i0+1, j0+1, k0);
			f[4] = scalar(i0, j0, k0+1);
			f[5] = scalar(i0+1, j0, k0+1);
			f[6] = scalar(i0, j0+1, k0+1);
			f[7] = scalar(i0+1, j0+1, k0+1);
			const double u = (p[0] - p0[0]) / m_dx; // (p1[0] - p0[0]);
			const double v = (p[1] - p0[1]) / m_dy; // (p2[1] - p0[1]);
			const double w = (p[2] - p0[2]) / m_dz; // (p4[2] - p0[2]);

			return (1 - w) * ((1 - v) * ((1 - u) * f[0] + u * f[1]) + v * ((1 - u) * f[2] + u * f[3]))
				+ w * ((1 - v) * ((1 - u) * f[4] + u * f[5]) + v * ((1 - u) * f[6] + u * f[7]));
		}

		/// set gradient at given position in array
		void gradient(const int i, const int j, const int k, const Normal& g) { m_gradient[global_index(i, j, k)] = g; }
		/// returns the normal vector at grid's node using node's global index.
		Vector gradient(const int gindex) { return m_gradient[gindex]; }
		const Vector gradient(const int gindex) const { return m_gradient[gindex]; }
		/// returns the normal vector at grid's node specified using i, j and k indices.
		Vector gradient(const int i, const int j, const int k) { return m_gradient[global_index(i, j, k)]; }
		const Vector gradient(const int i, const int j, const int k) const { return m_gradient[global_index(i, j, k)]; }
		Vector gradient(const Index i) { return m_gradient[global_index(i)]; }
		const Vector gradient(const Index i) const { return m_gradient[global_index(i)]; }
		/// invert normals
		void flip_gradient();
		/// check if a point is strictly inside the ugrid.
		bool in_bbox(const Point& p) {
			return  ((p[0] > m_bbox[0][0] && p[0] < m_bbox[1][0]) &&
				(p[1] > m_bbox[0][1] && p[1] < m_bbox[2][1]) &&
				(p[2] > m_bbox[0][2] && p[2] < m_bbox[4][2]));
		}
		/// compute index i,j,k of cell containing the point.
		Index cell_index(Point p)
		{
			Index id;
			id[0] = static_cast<int>((p[0] - m_bbox[0][0]) / m_dx);
			id[1] = static_cast<int>((p[1] - m_bbox[0][1]) / m_dy);
			id[2] = static_cast<int>((p[2] - m_bbox[0][2]) / m_dz);
			return id;
		}
		void cell(Point p, Point v[8])
		{
			const int i0 = static_cast<int>((p[0] - m_bbox[0][0]) / m_dx);
			const int j0 = static_cast<int>((p[1] - m_bbox[0][1]) / m_dy);
			const int k0 = static_cast<int>((p[2] - m_bbox[0][2]) / m_dz);
			v[0] = point(i0, j0, k0);
			v[1] = point(i0 + 1, j0, k0);
			v[2] = point(i0, j0 + 1, k0);
			v[3] = point(i0 + 1, j0 + 1, k0);
			v[4] = point(i0, j0, k0 + 1);
			v[5] = point(i0 + 1, j0, k0 + 1);
			v[6] = point(i0, j0 + 1, k0 + 1);
			v[7] = point(i0 + 1, j0 + 1, k0 + 1);
		}
		void cellVertices(const int i0, const int j0, const int k0, Point v[8])
		{
			v[0] = point(i0, j0, k0);
			v[1] = point(i0 + 1, j0, k0);
			v[2] = point(i0, j0 + 1, k0);
			v[3] = point(i0 + 1, j0 + 1, k0);
			v[4] = point(i0, j0, k0 + 1);
			v[5] = point(i0 + 1, j0, k0 + 1);
			v[6] = point(i0, j0 + 1, k0 + 1);
			v[7] = point(i0 + 1, j0 + 1, k0 + 1);
		}
		void cellGradients(const int i0, const int j0, const int k0, Normal n[8])
		{
			n[0] = m_gradient[global_index(i0, j0, k0)];
			n[1] = m_gradient[global_index(i0 + 1, j0, k0)];
			n[2] = m_gradient[global_index(i0, j0 + 1, k0)];
			n[3] = m_gradient[global_index(i0 + 1, j0 + 1, k0)];
			n[4] = m_gradient[global_index(i0, j0, k0 + 1)];
			n[5] = m_gradient[global_index(i0 + 1, j0, k0 + 1)];
			n[6] = m_gradient[global_index(i0, j0 + 1, k0 + 1)];
			n[7] = m_gradient[global_index(i0 + 1, j0 + 1, k0 + 1)];
		}
		/// compute global index.
		int global_index(const Point p)
		{
			const int i = static_cast<int>((p[0] - m_bbox[0][0]) / m_dx);
			const int j = static_cast<int>((p[1] - m_bbox[0][1]) / m_dy);
			const int k = static_cast<int>((p[2] - m_bbox[0][2]) / m_dz);
			return k * m_ny*m_nx + j * m_nx + i;
		}
		int global_index(const int i, const int j, const int k) { return (k*m_ny*m_nx + j * m_nx + i); }
		int global_index(const int i, const int j, const int k) const { return (k*m_ny*m_nx + j * m_nx + i); }

		int global_index(const Index i) { return (i[2]*m_ny*m_nx + i[1] * m_nx + i[0]); }
		int global_index(const Index i) const { return (i[2] * m_ny*m_nx + i[1] * m_nx + i[0]); }
		Index local_index(const int g_index) { return Index{ g_index % m_nx,(g_index / m_nx) % m_ny, g_index / (m_nx*m_ny) }; }
        Index local_index(const int g_index) const { return Index{ g_index % m_nx,(g_index / m_nx) % m_ny, g_index / (m_nx*m_ny) }; }
        void local_index(const int g_index, int& i, int& j, int& k) {
            i = g_index%m_nx;
            j = (g_index / m_nx)%m_ny;
            k = g_index/(m_nx*m_ny);
        }
		/// interpolate scalar
		//double interpolate_scalar(const Point& p);
		/// interplate normal
		//bool interpolate_normal(const Point& p, Normal& n);
		/// returns maximum scalar value in the grid
		double max_scalar() { return *std::max_element(m_scalars.begin(), m_scalars.end()); }
		double max_scalar() const { return *std::max_element(m_scalars.begin(), m_scalars.end()); }
		/// compute minimum value of scalar data stored on the grid
		double min_scalar() { return *std::min_element(m_scalars.begin(), m_scalars.end()); }
		double min_scalar() const { return *std::min_element(m_scalars.begin(), m_scalars.end()); }
		void estimateGradient();
		void normal(Normal& n, const int i, const int j, const int k, const double u, const double v, const double w)
		{
			const Vector n0 = gradient(i, j, k);
			const Vector n1 = gradient(i+1, j, k);
			const Vector n2 = gradient(i, j+1, k);
			const Vector n3 = gradient(i+1, j+1, k);
			const Vector n4 = gradient(i, j, k+1);
			const Vector n5 = gradient(i+1, j, k+1);
			const Vector n6 = gradient(i, j+1, k+1);
			const Vector n7 = gradient(i+1, j+1, k+1);
			n = (1 - w) * ((1 - v) * ((1 - u) * n0 + u * n1) + v * ((1 - u) * n2 + u * n3))
				+ w * ((1 - v) * ((1 - u) * n4 + u * n5) + v * ((1 - u) * n6 + u * n7));
			n.normalize();
		}
		void position(Point& p, const int i, const int j, const int k, const double u, const double v, const double w)
		{
			p[0] = m_bbox[0][0] + (i + u) * m_dx;
			p[1] = m_bbox[0][1] + (j + v) * m_dy;
			p[2] = m_bbox[0][2] + (k + w) * m_dz;
		}
		void writeVolume(std::string filename)
		{
			const size_t bytes{ m_scalars.size() };
			auto myfile = std::fstream(filename, std::ios::out | std::ios::binary);
			myfile.write((char*)&m_nx, sizeof(int));
			myfile.write((char*)&m_ny, sizeof(int));
			myfile.write((char*)&m_nz, sizeof(int));
			myfile.write((char*)&m_scalars[0], sizeof(double) * bytes);
			myfile.close();
		}
		void clear()
		{
			m_nx = 0; //!< grid size in x-direction
			m_ny = 0; //!< grid size in y-direction
			m_nz = 0; //!< grid size in z-direction
			m_dx = 0; //!< grid spacing in x-direction
			m_dy = 0; //!< grid spacing in y-direction
			m_dz = 0; //!< grid spacing in z-direction
			for (int i = 0; i < 8; i++)
			{
				m_bbox[i][0] = 0;
				m_bbox[i][1] = 0;
				m_bbox[i][2] = 0;
			}
			m_scalars.clear(); //!< scalar values stored in the ugrid
			m_gradient.clear(); //!< estimated gradient of scalar field
		}
	private:
		int m_nx{ 0 }; //!< grid size in x-direction
		int m_ny{ 0 }; //!< grid size in y-direction
		int m_nz{ 0 }; //!< grid size in z-direction
		double m_dx{ 0 }; //!< grid spacing in x-direction
		double m_dy{ 0 }; //!< grid spacing in y-direction
		double m_dz{ 0 }; //!< grid spacing in z-direction
		std::array<Point, 8> m_bbox; //!< the bounding box of the ugrid.
		std::vector<double> m_scalars; //!< scalar values stored in the ugrid
		std::vector<Normal> m_gradient; //!< estimated gradient of scalar field

	public:
		/// print maximum scalar value stored in the ugrid.
		void print_max_scalar() { std::cout << "min scalar: " << *std::max_element(m_scalars.begin(), m_scalars.end()) << std::endl; }
		/// print minimum scalar valued stored in the ugrid.
		void print_min_scalar() { std::cout << "max scalar: " << *std::min_element(m_scalars.begin(), m_scalars.end()) << std::endl; }
		/// handle indices at grid boundaries using periodic conditions.
		int mod(int n, int m) { return (n >= 0) ? (n%m) : (m - (-n % m)) % m; }
		int mod(int n, int m) const { return (n >= 0) ? (n%m) : (m - (-n % m)) % m; }
		/// trilinar interpolation given local coordinates and vertex values
		//double trilinear(const double u, const double v, const double w, const std::array<double, 8>& F);
	};

} // homotopy
