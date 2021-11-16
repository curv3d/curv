#pragma once

// Libs
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cmath>
#include <fstream>
#include <list>
#include <vector>
#include <array>
#include <tuple>
#include <stack>
#include <map>
#include <utility>
#include <bitset>
#include <algorithm>
#include <random>
#include <chrono>

// project
#include "Vector.h"
#include "UniformGrid.h"

namespace dmc {

	//
	//  DualMarchingCubes.h
	//
	//  Created by Roberto Grosso on 05.06.16.
	//  Copyright � 2019 Roberto Grosso. All rights reserved.
	//


		/** Computes a topologically correct and manifold iso-surface consisting of only quads from an uniform grid.
		 *  # Volume data
		 *  The implementation assumes the volume data is sorted in a uniform grid. The file containing the data
		 *  should be a binary file with the following format:
		 *  - three unsigned short indicating the dimensions in x, y and z
		 *  - three floats with the cell sizes in x, y and z
		 *  - the volume data as unsigned shorts.
		 *  # Element topology
		 *  The numbering of vertices is given by vertex coordinates of a unit reference hexahedron. Edges
		 *  are given by their end vertices. We include lists with face-vertex and face-edge correspondences.
		 *  ## Vertices
		 *  - v0 = (0,0,0)
		 *  - v1 = (1,0,0)
		 *  - v2 = (1,0,0)
		 *  - v3 = (1,0,0)
		 *  - v4 = (1,0,0)
		 *  - v5 = (1,0,0)
		 *  - v6 = (1,0,0)
		 *  - v7 = (1,0,0)
		 *
		 *  ## Edges
		 *  - e0 = {0,1}
		 *  - e1 = {1,3}
		 *  - e2 = {2,3}
		 *  - e3 = {0,2}
		 *  - e4 = {4,5}
		 *  - e5 = {5,7}
		 *  - e6 = {6,7}
		 *  - e7 = {4,6}
		 *  - e8 = {0,4}
		 *  - e9 = {1,5}
		 *  - e10 = {3,7}
		 *  - e11 = {2,6}
		 *
		 *  ## Face-Edge correspondence
		 *  The order of the entries in the list corresponds to the orientation of the face
		 *  - f0 = {0,1,2,3}
		 *  - f1 = {4,5,6,7}
		 *  - f2 = {0,9,4,8}
		 *  - f3 = {2,10,6,11}
		 *  - f4 = {3,11,7,8}
		 *  - f5 = {1,10,5,9}
		 *
		 *  ## Face-Vertex correspondence
		 *  - f0 = {0,1,2,3}
		 *  - f1 = {4,5,6,7}
		 *  - f2 = {0,1,4,5}
		 *  - f3 = {2,3,6,7}
		 *  - f4 = {0,2,4,6}
		 *  - f5 = {1,3,5,7}
		 *
		 *  ## Orientation of contours
		 *  In order to construct contour which are oriented such that positive vertices are outside
		 *  we describes faces as been seeing from outside the hexahedron. In this case, the numbering
		 *  used to collect vertices for a face is as follows:
		 *  ### Oriented numbering of Face-Vertex correspondence
		 *  This correspondence is used to compute the intersection of the iso-surface with the face
		 *  of the hexahedron and orient the segments such that positive vertices are outside the
		 *  contour
		 *  - f0 = {0,2,1,3}
		 *  - f1 = {5,7,4,6}
		 *  - f2 = {4,0,5,1}
		 *  - f3 = {2,6,3,7}
		 *  - f4 = {4,6,0,2}
		 *  - f5 = {1,3,5,7}
		 *
		 *   ### Oriented numbering of Face-Edge correspondence
		 *  - f0 = {3,2,1,0}
		 *  - f1 = {5,6,7,4}
		 *  - f2 = {8,0,0,4}
		 *  - f3 = {11,6,10,2}
		 *  - f4 = {7,11,3,8}
		 *  - f5 = {1,10,5,9}
		 *
		 *  # Remark
		 *  The function `slice()` implements a new version of the algorithm to intersect the iso-surface
		 *  with a cell. MC polygons or contours, i.e. the intersection of the iso-surface with the faces of the cell,
		 *  are computed in an oriented way such that positive vertices are outside the surface.
		 *
		 *  # Halfedges
		 *  he[0] = origin vertex
		 *  he[1] = face
         *  he[2] = next
         *  he[3] = tween
        *   he[4] = 0 - manifold, 1 non-manifold
		 */
		class DualMarchingCubes {
		public:
			using uchar = unsigned char;
			using ushort = unsigned short;
			using uint = unsigned int;
			using ulong = unsigned long long;
			using Scalar = double;

			using Vertex = dmc::Vector;
			using Point  = dmc::Vector;
			using Normal = dmc::Vector;
			using UGrid  = dmc::UniformGrid;
			using Index  = UGrid::Index;
		public:
			// constants
			const int MC_AMBIGUOUS{ 105 };
			const int INVALID_INDEX{ -1 };
			const int INVALID_COLOR{ -1 };
			const int MANIFOLD{ 0 };
			const int NON_MANIFOLD{ 1 };
			const int BOUNDARY_MULTIPLE_EDGE{ 2 };
			const int BIT_1{ 0x1 };
			const int BIT_2{ 0x2 };
			const int BIT_3{ 0x4 };
			const int BIT_4{ 0x8 };
			const int BIT_5{ 0x10 };
			const int BIT_6{ 0x20 };
			const int BIT_7{ 0x40 };
			const int BIT_8{ 0x80 };
			const int BIT_16{ 0x8000 };
		public:
			/// <summary>
			///  Computes the iso-surface with the dual marching cubes algorithm
			/// </summary>
			/// <param name="i0">iso-value</param>
			/// <param name="ugrid">uniform grid</param>
			/// <param name="vertices">output vertices</param>
			/// <param name="normals">output normals</param>
			/// <param name="triangles">output triangles</param>
			/// <param name="quads">output quadrilaterals</param>
			/// <param name="sFlag">if true, quad mesh will be simplified by removing vertices with given valence patterns</param>
			void dualMC(const double i0, UGrid& ugrid,
				std::vector<Vertex>& vertices, std::vector<Normal>& normals,
				std::vector<int>& triangles, std::vector<int>& quads, const bool sFlag);
			/// <summary>
			/// collect vertices at the boundary of the iso-surface
			/// </summary>
			/// <param name="bndV">output array containing the boundary vertices</param>
			/// <returns>true if there are boundary vertices</returns>
			bool getBndVertices(std::vector<int>& bndV) {
				if (bndVertices.size() > 0) {
					for (auto v : bndVertices) {
						bndV.push_back(v);
					}
					std::cout << "size of bndV: " << bndV.size() << std::endl;
					return true;
				}
				else {
					return false;
				}
			}
		private:
			std::vector<int> bndVertices;
			//std::list<std::list<Vertex>> pairs;
			uint nrProj1{ 0 };
			uint nrProj2{ 0 };

            /// <summary>
			/// Compute the intersection of the iso-surface with a cell
			/// </summary>
			/// <param name="i0">iso-value</param>
			/// <param name="i_case">MC case</param>
			/// <param name="i">i-index of cell</param>
			/// <param name="j">j-index of cell</param>
			/// <param name="k">k-index of cell</param>
			/// <param name="f">the eight values of the scalar at the cell vertices</param>
			/// <param name="ugrid">Uniform grid</param>
			/// <param name="v">vertices</param>
			/// <param name="n">normals</param>
			/// <param name="map">a hash map to reconstruct quadrilaterals</param>
			void slice(const double i0, const uint i_case, const int i, const int j, const int k, double f[8],
				UGrid& ugrid, std::vector<Vertex>& v, std::vector<Normal>& n, std::map<int, std::array<int, 5>>& map);
			//
			void representative(const double i0, const double f[8], uint cnt_, ulong& c_, int t, Vertex& p,
				std::vector<std::array<double,2>>& bboxU,
				std::vector<std::array<double, 2>>& bboxV,
				std::vector<std::array<double, 2>>& bboxW,
				uint& count1, uint& count2);
			bool projection(const uint r, const double i0, const double f[8], uint cnt_, ulong& c_, const int t, Vertex& p,
				std::vector<std::array<double, 2>>& bboxU,
				std::vector<std::array<double, 2>>& bboxV,
				std::vector<std::array<double, 2>>& bboxW,
				const int nrSamples);
			auto e_glIndex(const int e, const int i_idx, const int j_idx, const int k_idx, UGrid& ugrid)
			{
				const unsigned long long gei_pattern_ = 670526590282893600ull;
				const int i = i_idx + (int)((gei_pattern_ >> 5 * e) & 1); // global_edge_id[eg][0];
				const int j = j_idx + (int)((gei_pattern_ >> (5 * e + 1)) & 1); // global_edge_id[eg][1];
				const int k = k_idx + (int)((gei_pattern_ >> (5 * e + 2)) & 1); // global_edge_id[eg][2];
				const int offs = (int)((gei_pattern_ >> (5 * e + 3)) & 3);
				return (3 * ugrid.global_index(i, j, k) + offs);
			};
			// edge table unsigned long long e_table = 240177437832960;
			// input e and offset, which direction the normal has to point
			auto get_vertex_pos(const int e, const int offset)
			{
				const unsigned long long e_talbe = 240177437832960;
				return (e_talbe >> (4 * e + 2 * offset)) & 3;
			};

			/// <summary>
			/// Compute the MC polygon for the ambiguous cases
			/// </summary>
			/// <param name="i0">iso-values</param>
			/// <param name="f">scalar values at cell vertices</param>
			/// <param name="c_">mc polygon</param>
			/// <returns></returns>
			uint mc_polygon(const double i0, const double f[8], ulong& c_);
			/// <summary>
			/// Compute the MC polygon for the unambiguous case out of a modified MC lookup table.
			/// There might give up to four contours encoded in the unsigned long long c_
			/// </summary>
			/// <param name="i_case">MC case</param>
			/// <param name="c_">mc polygons</param>
			/// <returns></returns>
			uint mc_polygon(const int i_case, ulong& c_);
			/// <summary>
			/// For each edge in the reference unit cell set to which edge the MC polygon is going. The variable
			/// segm_ will at the end encode for each of the 12 edges the edge from which the MC polygon is coming
			/// and the edge towards the MC polygon is going. This way, the MC polygon can be reconstructed
			/// starting at any edge.
			/// </summary>
			/// <param name="ei">edge id where MC polygon starts at the face being processed</param>
			/// <param name="eo">edge id toward the MC polygon is moving at the face being processed</param>
			/// <param name="segm_">encodes for each edge the index of the edge from where the polygon is coming
			/// and the index of the edge towards the MC polygon is moving. </param>
			auto set_segm(const int ei, const int eo, unsigned char segm_[12]) {
				segm_[ei] &= 0xF0;
				segm_[ei] |= ((unsigned char)eo) & 0xF;
				segm_[eo] &= 0xF;
				segm_[eo] |= ((unsigned char)ei) << 4;
			}
			/// <summary>
			/// Get edge index of the edge toward or from which the MC polygon is going or coming.
			/// </summary>
			/// <param name="e">index of edge being processed</param>
			/// <param name="pos">pos=0 returns index of the edge toward the MC polygon moves, pos=1 the index of the
			/// edge from where the polygon is coming</param>
			/// <param name="segm_">encodes in and out for the given edge</param>
			/// <returns>edge index</returns>
			auto get_segm(const int e, const int pos, unsigned char segm_[12]) {
				if (pos == 0)
					return (int)(segm_[e] & 0xF);
				else
					return (int)((segm_[e] >> 4) & 0xF);
			}
			/// <summary>
			/// Checks if MC polygon intersects the edge
			/// </summary>
			/// <param name="e">edge index</param>
			/// <param name="segm_">array of unsigned chars encoding edges state</param>
			/// <returns>true if edge is set</returns>
			auto is_segm_set(const int e, unsigned char segm_[12]) {
				return (segm_[e] != 0xFF);
			}
			/// <summary>
			/// Initialize the entries for a given edge
			/// </summary>
			/// <param name="e">edge index</param>
			/// <param name="segm_">array of unsigned chars encoding the state of the edges</param>
			/// <returns></returns>
			auto unset_segm(const int e, unsigned char segm_[12]) {
				segm_[e] = 0xFF;
			}
			// In order to compute oriented segments, the hexahedron has to be flatten.
			// The insides of the faces of the hexahedron have to be all at the same
			// side of the flattened hexahedron. This requires changing the order of the
			// edges when reading from the faces
			// encode edges at face
			unsigned short e_face_[6]{ (ushort)291, (ushort)18277, (ushort)18696, (ushort)10859, (ushort)33719, (ushort)38305 };
			// encode vertices at face
			unsigned short v_face_[6]{ (ushort)12576, (ushort)25717, (ushort)5380, (ushort)29538, (ushort)8292, (ushort)30001 };

			/// <summary>
			/// Get edge index with respect to cell, where input is local edge index in face and face index.
			/// </summary>
			/// <param name="f">face index in cell</param>
			/// <param name="e">edge index with respect to face</param>
			/// <returns>index of edge in cell</returns>
			auto get_face_e(const int f, const int e) { return ((e_face_[f] >> (4 * e)) & 0xF); };
			/// <summary>
			/// Get vertex index in cell, where input is local vertex index in face and face index.
			/// </summary>
			/// <param name="f">face index in cell</param>
			/// <param name="e">vertex index in face</param>
			/// <returns>vertex index in cell</returns>
			auto get_face_v(const int f, const int e) { return ((v_face_[f] >> (4 * e)) & 0xF); };

			/// <summary>
			/// Computes the value of the asymptotic decider for a face of a hexahedron.
			/// Vertices are numbered according to the index convention used in this
			/// project.
			/// </summary>
			/// <param name="f0">scalar value at vertex 0</param>
			/// <param name="f1">scalar value at vertex 1</param>
			/// <param name="f2">scalar value at vertex 2</param>
			/// <param name="f3">scalar value at vertex 3</param>
			/// <returns></returns>
			auto asymptotic_decider(const double f0, const double f1, const double f2, const double f3) {
				return (f0 * f3 - f1 * f2) / (f0 + f3 - f1 - f2);
			};

			/// <summary>
			/// Set size of MC polygon encoded in an unsigned long long c_. There might give
			/// up to four contours encoded in c_
			/// </summary>
			/// <param name="cnt">mc polygon id</param>
			/// <param name="size">new size</param>
			/// <param name="c_">MC polygons</param>
			void set_cnt_size(const int cnt, const int size, unsigned long long& c_) {
				// unset contour size
				c_ &= ~(0xF << 4 * cnt);
				c_ |= (size << 4 * cnt);
			}
			/// <summary>
			/// Get size of contour encoded in c_ (unsigned long long)
			/// </summary>
			/// <param name="cnt">mc polygon id</param>
			/// <param name="c_">MC polygons</param>
			/// <returns></returns>
			int get_cnt_size(const int cnt, unsigned long long& c_) {
				return static_cast<int>((c_ & (0xF << 4 * cnt)) >> 4 * cnt);
			}
			/// <summary>
			/// Set edge building a MC polygon
			/// </summary>
			/// <param name="cnt">mc polygon id</param>
			/// <param name="pos">position</param>
			/// <param name="val">edge id</param>
			/// <param name="c_">MC polygons</param>
			void set_c(const int cnt, const int pos, const int val, unsigned long long& c_) {
				const uint mask[4] = { 0x0, 0xF, 0xFF, 0xFFF };
				const uint c_sz = c_ & mask[cnt];
				const uint e = 16 + 4 * ((c_sz & 0xF) + ((c_sz & 0xF0) >> 4) + ((c_sz & 0xF00) >> 8) + pos);
				c_ &= ~(((unsigned long long)0xF) << e);
				c_ |= (((unsigned long long)val) << e);
			}
			/// <summary>
			/// Read edge from MC polygon
			/// </summary>
			/// <param name="cnt">polygon id</param>
			/// <param name="pos">edge position in polygon</param>
			/// <param name="c_">MC polygons</param>
			/// <returns></returns>
			int get_c(const int cnt, const int pos, unsigned long long c_) {
				const uint mask[4] = { 0x0, 0xF, 0xFF, 0xFFF };
				const uint c_sz = (uint)(c_ & mask[cnt]);
				const uint e = 16 + 4 * ((c_sz & 0xF) + ((c_sz & 0xF0) >> 4) + ((c_sz & 0xF00) >> 8) + pos);
				return static_cast<int>((c_ >> e) & 0xF);
			}
			/// <summary>
			/// Compute the intersection of the iso-surface with a give edge of the cell and accumulate
			/// the values in order to compute a mean point within the cell
			/// </summary>
			/// <param name="l_edge">data structure containing the endpoints of the edges</param>
			/// <param name="e">edge being intersected</param>
			/// <param name="f">scalar values at the cell vertices</param>
			/// <param name="i0">iso-value</param>
			/// <param name="u">vector accumulating the coordinates of the intersection points</param>
			void getLocalCoordinates(const unsigned char l_edge, const int e, const double f[8], const double i0, Vertex& u)
			{
				const int v0 = (l_edge & 0xF);
				const int v1 = (l_edge >> 4) & 0xF;
				const float l = (i0 - f[v0]) / (f[v1] - f[v0]);
				unsigned long long int e_{ 75059404284193ULL };
				unsigned long long c_{ 38552806359568ULL };
				float val = (e_ & 1ULL << (4 * e)) >> (4 * e);
				float con = (c_ & 1ULL << (4 * e)) >> (4 * e);
				u[0] = l * val + con;
				val = (e_ & 1ULL << (4 * e + 1)) >> (4 * e + 1);
				con = (c_ & 1ULL << (4 * e + 1)) >> (4 * e + 1);
				u[1] = l * val + con;
				val = (e_ & 1ULL << (4 * e + 2)) >> (4 * e + 2);
				con = (c_ & 1ULL << (4 * e + 2)) >> (4 * e + 2);
				u[2] = l * val + con;
			}
			/// <summary>
			/// Compute the intersection of the iso-surface with the edge of a given cell
			/// </summary>
			/// <param name="l_edge">data structure containing the endpoints of the edges</param>
			/// <param name="e">edge being intersected</param>
			/// <param name="f">scalar values at the cell vertices</param>
			/// <param name="i0">iso-value</param>
			/// <param name="u">vector with the coordinates of the intersection point</param>
			void edgeIntersection(const unsigned char l_edge, const int e, const double f[8], const double i0, Vertex& u)
			{
				const int v0 = (l_edge & 0xF);
				const int v1 = (l_edge >> 4) & 0xF;
				const float l = (i0 - f[v0]) / (f[v1] - f[v0]);
				unsigned long long int e_{ 75059404284193ULL };
				unsigned long long c_{ 38552806359568ULL };
				float val = (e_ & 1ULL << (4 * e)) >> (4 * e);
				float con = (c_ & 1ULL << (4 * e)) >> (4 * e);
				u[0] = l * val + con;
				val = (e_ & 1ULL << (4 * e + 1)) >> (4 * e + 1);
				con = (c_ & 1ULL << (4 * e + 1)) >> (4 * e + 1);
				u[1] = l * val + con;
				val = (e_ & 1ULL << (4 * e + 2)) >> (4 * e + 2);
				con = (c_ & 1ULL << (4 * e + 2)) >> (4 * e + 2);
				u[2] = l * val + con;
			}
			/// <summary>
			/// Compares vertex coordinates with limits of intervals for each coordinate.
			/// </summary>
			/// <param name="ui">input vertex</param>
			/// <param name="umin">min value along u-coord.</param>
			/// <param name="umax">max value along u-coord.</param>
			/// <param name="vmin">min value along v-coord.</param>
			/// <param name="vmax">max value along v-coord.</param>
			/// <param name="wmin">min value along w-coord.</param>
			/// <param name="wmax">max value along w-coord.</param>
			void minmax(Vertex& ui, double& umin, double& umax, double& vmin, double& vmax, double& wmin, double& wmax)
			{
				umin = (ui[0] < umin) ? ui[0] : umin;
				umax = (ui[0] > umax) ? ui[0] : umax;
				vmin = (ui[1] < vmin) ? ui[1] : vmin;
				vmax = (ui[1] > vmax) ? ui[1] : vmax;
				wmin = (ui[2] < wmin) ? ui[2] : wmin;
				wmax = (ui[2] > wmax) ? ui[2] : wmax;
			}
			/// <summary>
			/// Compares coordinates of vertex with three interval
			/// to compute a bounding box.
			/// </summary>
			/// <param name="ui">input vertex</param>
			/// <param name="bbox">bounding box containing three intervals, one for each coordinate</param>
			void minmax(Vertex& ui, std::array<double, 6>& bbox)
			{
				bbox[0] = bbox[0] > ui[0] ? ui[0] : bbox[0];
				bbox[1] = bbox[1] < ui[0] ? ui[0] : bbox[1];
				bbox[2] = bbox[2] > ui[1] ? ui[1] : bbox[2];
				bbox[3] = bbox[3] < ui[1] ? ui[1] : bbox[3];
				bbox[4] = bbox[4] > ui[2] ? ui[2] : bbox[4];
				bbox[5] = bbox[5] < ui[2] ? ui[2] : bbox[5];
			}
			/// <summary>
			/// Check if point in within a bounding box. There might give up to four
			/// such bounding boxes corresponding to the four possible branches of
			/// the iso-surface which can intersect the cell.
			/// </summary>
			/// <param name="u">u-coord. of input point</param>
			/// <param name="v">v-coord. of input point</param>
			/// <param name="w">w-coord. of input point</param>
			/// <param name="t"></param>
			/// <param name="bbox"></param>
			/// <returns></returns>
			bool isInOwnBBox(const double u, const double v, const double w, const int t, std::vector<std::array<double, 6>>& bbox)
			{
				if (u < bbox[t][0] || bbox[t][1] < u) return false;
				if (v < bbox[t][2] || bbox[t][3] < v) return false;
				if (w < bbox[t][4] || bbox[t][5] < w) return false;
				return true;
			}
			/// <summary>
			/// Check if input point is contained in the bounding box of the other
			/// branches of the iso-surface intersecting the cell.
			/// </summary>
			/// <param name="r"></param>
			/// <param name="u"></param>
			/// <param name="v"></param>
			/// <param name="w"></param>
			/// <param name="cnt_"></param>
			/// <param name="c_"></param>
			/// <param name="t"></param>
			/// <param name="bboxU"></param>
			/// <param name="bboxV"></param>
			/// <param name="bboxW"></param>
			/// <returns></returns>
			bool isInNeighborBBox(const double u, const double v, const double w,
				const int cnt_, ulong& c_, const int t,
				std::vector<std::array<double, 2>>& bboxU,
				std::vector<std::array<double, 2>>& bboxV,
				std::vector<std::array<double, 2>>& bboxW)
			{
				for (int i = 0; i < cnt_; i++) {
					const int cnt_size = get_cnt_size(i, c_);
					if (cnt_size == 3 && t != i) {
						if (u < bboxU[i][0] || bboxU[i][1] < u)
							continue;
						if (v < bboxV[i][0] || bboxV[i][1] < v)
							continue;
						if (w < bboxW[i][0] || bboxW[i][1] < w)
							continue;
						return true;
					}
				}
				return false;
			}
			/// <summary>
			/// Projection plane
			/// </summary>
			enum class ProjectionDirection { W_PROJECTION = 1, V_PROJECTION = 2, U_PROJECTION = 3};
			/// <summary>
			/// Computes projection plane by comparing component of gradient/normal.
			/// </summary>
			/// <param name="ni">input normal direction</param>
			/// <returns>projection plane</returns>
			ProjectionDirection lProjection(Vector& ni)
			{
				const double dx = std::fabs(ni[0]);
				const double dy = std::fabs(ni[1]);
				const double dz = std::fabs(ni[2]);
				if (dz >= dx && dz >= dy) return ProjectionDirection::W_PROJECTION;
				if (dy >= dx && dy >= dz) return ProjectionDirection::V_PROJECTION;
				if (dx >= dy && dx >= dz) return ProjectionDirection::U_PROJECTION;
                return ProjectionDirection::W_PROJECTION;
			}
			/// <summary>
			/// Compute projection plane by interval sizes of bounding box.
			/// </summary>
			/// <param name="u"></param>
			/// <param name="v"></param>
			/// <param name="w"></param>
			/// <returns></returns>
			ProjectionDirection minProjection(const double u, const double v, const double w)
			{
				if (w <= u && w <= v) return ProjectionDirection::W_PROJECTION;
				if (v <= u && v <= w) return ProjectionDirection::V_PROJECTION;
				if (u <= v && u <= w) return ProjectionDirection::U_PROJECTION;
                return ProjectionDirection::W_PROJECTION;
			}
			/// <summary>
			/// Checks if point is within unit cell
			/// </summary>
			/// <param name="p">input point</param>
			/// <returns>true if point is within unit cell, otherwise it returns false</returns>
			bool isIn(Vertex& p)
			{
				if (p[0] < 0 || p[0] > 1)
					return false;
				if (p[1] < 0 || p[1] > 1)
					return false;
				if (p[2] < 0 || p[2] > 1)
					return false;
				return true;
			}
			/// <summary>
			/// Interpolate a scalar values at the given position.
			/// </summary>
			/// <param name="f">scalar values at cell vertices</param>
			/// <param name="u">u-coordinate</param>
			/// <param name="v">v-coordinate</param>
			/// <param name="w">w-coordinate</param>
			/// <returns>interpolated scalar value at (u,v,w) point</returns>
			double trilinear(const double f[8], const double u, const double v, const double w)
			{
				return (1 - w) * ((1 - v) * ((1 - u) * f[0] + u * f[1]) + v * ((1 - u) * f[2] + u * f[3]))
					+ w * ((1 - v) * ((1 - u) * f[4] + u * f[5]) + v * ((1 - u) * f[6] + u * f[7]));
			}
			/// <summary>
			/// Computes the gradient of the scalar field using the trilinear interpolant to compute
			/// the partial derivatives.
			/// </summary>
			/// <param name="f">scalar values at cell vertices</param>
			/// <param name="u">u-coordinate</param>
			/// <param name="v">v-coordinates</param>
			/// <param name="w">w-coordinates</param>
			/// <returns>gradient at input (u,v,w) point</returns>
			Vector gradient(const double f[8], const double u, const double v, const double w)
			{
				Vector g;
				g[0] = (1 - w) * ((1 - v) * (f[1] - f[0]) + v * (f[3] - f[2])) + w * ((1 - v) * (f[5] - f[4]) + v * (f[7] - f[6]));
				g[1] = (1 - w) * ((1 - u) * (f[2] - f[0]) + u * (f[3] - f[1])) + w * ((1 - u) * (f[6] - f[4]) + u * (f[7] - f[5]));
				g[2] = (1 - v) * ((1 - u) * (f[4] - f[0]) + u * (f[5] - f[1])) + v * ((1 - u) * (f[6] - f[2]) + u * (f[7] - f[3]));
				return g;
			}
			/// <summary>
			/// Add a vertex to the hash map. A hash map is used to assign to the vertex an unique index which is
			/// later used to build the quad only mesh.
			/// </summary>
			/// <param name="key">key to store vertex, it is the unique global edge index</param>
			/// <param name="pos">vertex position in the quadrilateral</param>
			/// <param name="v_addr">global vertex index</param>
			/// <param name="m_">hash map</param>
			/// <returns></returns>
			bool addVertex(const int key, const int pos, const int v_addr, std::map<int, std::array<int, 5>>& m_)
			{
				auto e = m_.find(key);
				if (e != m_.end())
				{
					// quadrilateral already exists
					e->second[pos] = v_addr;
				}
				else
				{
					// create quadrilateral
					std::array<int, 5> q{ INVALID_INDEX,INVALID_INDEX,INVALID_INDEX,INVALID_INDEX, INVALID_COLOR };
					q[pos] = v_addr;
					m_[key] = q;
				}
				return true;
			}
			/// <summary>
			/// Add a vertex to the hash map. A hash map is used to assign to the vertex an unique index which is
			/// later used to build the quad only mesh. This method adds also a color to the quadrilateral.
			/// </summary>
			/// <param name="key">key to store the vertex</param>
			/// <param name="pos">position of the vertex in the quadrilateral</param>
			/// <param name="v_addr">global vertex index</param>
			/// <param name="color">color of quadrilateral, corresponds to edge color/index</param>
			/// <param name="m_">hash map</param>
			/// <returns></returns>
			bool addVertex(const int key, const int pos, const int v_addr, const int color, std::map<int, std::array<int, 5>>& m_)
			{
				auto e = m_.find(key);
				if (e != m_.end())
				{
					// quadrilateral already exists
					e->second[pos] = v_addr;
					e->second[4] = color;
				}
				else
				{
					// create quadrilateral
					std::array<int, 5> q{ INVALID_INDEX,INVALID_INDEX,INVALID_INDEX,INVALID_INDEX, INVALID_COLOR };
					q[pos] = v_addr;
					q[4] = color;
					m_[key] = q;
				}
				return true;
			}
			/// <summary>
			/// During processing quadrilaterals are temporary saved in a hash map. This is due to the fact
			/// that a quadrilateral is obtained by connecting vertices from neighbor cells. At the end, the
			/// hash map is traversed and quadrilaterals are collected in an array, where four consecutive
			/// indices build a quadrilateral. In the case that not all of the indices are set, the quadrilateral
			/// is no complete and will not be stored in the array. This happens at boundary element.
			/// </summary>
			/// <param name="quads">array containing the quadrilaterals</param>
			/// <param name="colors">array containing the colors of the quadrilaterals</param>
			/// <param name="m_">hash map</param>
			void collectQuadrilaterals(std::vector<int>& quads, std::vector<int>& colors, std::map<int, std::array<int, 5>>& m_)
			{
				for (auto [k, q] : m_)
				{
					if (q[0] != INVALID_INDEX && q[1] != INVALID_INDEX && q[2] != INVALID_INDEX && q[3] != INVALID_INDEX)
					{
						quads.push_back(q[0]);
						quads.push_back(q[1]);
						quads.push_back(q[2]);
						quads.push_back(q[3]);
						colors.push_back(q[4]);
					}
				}
			}
			/// <summary>
			/// Some vertices might remain unused in the mesh due to boundary elements. These vertices
			/// are stored in the vertex array but are not part of any element and therefore should be
			/// removed.
			/// </summary>
			/// <param name="v">vertex list</param>
			/// <param name="n">normal list</param>
			/// <param name="quads">quad list</param>
			void removeUnusedVertices(std::vector<Vertex>& v, std::vector<Normal>& n, std::vector<int>& quads)
			{
				const int nr_v = static_cast<int>(v.size());
				const int nr_q = static_cast<int>(quads.size() / 4);
				std::vector<bool> flag(nr_v, false);
				for (int f = 0; f < nr_q; f++) {
					const int v0 = quads[4 * f];
					const int v1 = quads[4 * f + 1];
					const int v2 = quads[4 * f + 2];
					const int v3 = quads[4 * f + 3];
					flag[v0] = true;
					flag[v1] = true;
					flag[v2] = true;
					flag[v3] = true;
				}
				std::vector<int> m_(nr_v, INVALID_INDEX);
				std::vector<Vertex> nv;
				std::vector<Normal> nn;
				nv.reserve(nr_v);
				nn.reserve(nr_v);
				for (int i = 0; i < nr_v; i++) {
					if (flag[i]) {
						const int addr = static_cast<int>(nv.size());
						nv.push_back(v[i]);
						nn.push_back(n[i]);
						m_[i] = addr;
					}
				}
				// map quads indices
				for (int f = 0; f < nr_q; f++) {
					const int v0 = quads[4 * f];
					const int v1 = quads[4 * f + 1];
					const int v2 = quads[4 * f + 2];
					const int v3 = quads[4 * f + 3];
					quads[4 * f] = m_[v0];
					quads[4 * f + 1] = m_[v1];
					quads[4 * f + 2] = m_[v2];
					quads[4 * f + 3] = m_[v3];
				}
				// copy back
				const int nr_nv = static_cast<int>(nv.size());
				std::cout << " ... nr. of unused vertices removed: " << (nr_v - nr_nv) << std::endl;
				v.assign(nv.begin(), nv.end());
				n.assign(nn.begin(), nn.end());
			}
			/// <summary>
			/// Computes triangles from quad only mesh. The vertices are required to compute the
			/// triangle partition of a quad with the best quality elements.
			/// </summary>
			/// <param name="tris">Array of triangle indices, each three indices is a triangle</param>
			/// <param name="quads">Array of quad indices, each four indices is a quadrilateral</param>
			/// <param name="v">vertices in the mesh</param>
			void collectTriangles(std::vector<int>& tris, std::vector<int>& quads, std::vector<Vertex>& v);
			/// <summary>
			/// Minimum angle measure of quality of a triangle. Vertex index in triangle are
			/// counter clockwise.
			/// </summary>
			/// <param name="v0">vertex 0</param>
			/// <param name="v1">vertex 1</param>
			/// <param name="v2">vertex 2</param>
			/// <returns>MinAngle quality measure</returns>
			double minAngle(const Vertex v0, const Vertex v1, const Vertex v2)
			{
				const float a = distance(v0, v1); // std::sqrt((v1.x - v0.x) * (v1.x - v0.x) + (v1.y - v0.y) * (v1.y - v0.y) + (v1.z - v0.z) * (v1.z - v0.z));
				const float b = distance(v1, v2); // std::sqrt((v2.x - v1.x) * (v2.x - v1.x) + (v2.y - v1.y) * (v2.y - v1.y) + (v2.z - v1.z) * (v2.z - v1.z));
				const float c = distance(v2, v0); // std::sqrt((v0.x - v2.x) * (v0.x - v2.x) + (v0.y - v2.y) * (v0.y - v2.y) + (v0.z - v2.z) * (v0.z - v2.z));
				const float A = std::acos((b * b + c * c - a * a) / (2 * b * c));
				const float B = std::acos((a * a + c * c - b * b) / (2 * a * c));
				const float C = std::acos((b * b + a * a - c * c) / (2 * b * a));

				return std::min(std::min(A, B), C);
			}
			// Mesh simplification
			using Halfedge = std::array<int, 5>;
			void halfedges(const int nr_v, std::vector<int>& quads, std::vector<Halfedge>& he, std::vector<int>& he_v, std::vector<int>& he_f);
			std::array<int, 4> collectNeighbors(const int quad, std::vector<Halfedge>& he, std::vector<int>& he_f);
			void colorFaces(std::vector<Halfedge>& he, std::vector<int>& he_f, std::vector<int>& colors);
			void vertexValence(const int nr_v, std::vector<Halfedge>& he, std::vector<int>& vV_);
			bool isNonManifold(const int quad, std::vector<Halfedge>& he, std::vector<int>& he_f);
			void mark3X3Y(std::vector<int>& quads, std::vector<int>& vV, std::vector<bool>& p3X3Y);
			void mergeVertices3X3Y(std::vector<Vertex>& v, std::vector<Normal>& normals, std::vector<bool> p3X3Y, std::vector<int>& vV, std::vector<int>& colors,
				std::vector<Halfedge>& he, std::vector<int>& he_f, std::vector<std::pair<bool, int>>& vm_, std::vector<bool>& em_);
			void mergeVertices3X3Y(std::vector<Vertex>& v, std::vector<Normal>& normals, std::vector<bool> p3X3Y, std::vector<int>& vV,
				std::vector<Halfedge>& he, std::vector<int>& he_f, std::vector<std::pair<bool, int>>& vm_, std::vector<bool>& em_);
			void removeVertices3X3Y(std::vector<Vertex>& v, std::vector<Normal>& n, std::vector<std::pair<bool, int>>& vm_, std::vector<Vertex>& nv, std::vector<Normal>& nn);
			void removeQuadrilaterals3X3Y(std::vector<int>& q, std::vector<bool>& em, std::vector<std::pair<bool, int>>& vm, std::vector<int>& nq);
			void simplify3X3Y(std::vector<Vertex>& v, std::vector<Normal>& n, std::vector<int>& quads, std::vector<int>& colors);
			void mark3333(const int nr_v, std::vector<int>& q,
				std::vector < std::array<int, 5> >& he, std::vector<int>& he_f,
				std::vector<int>& vV, std::vector< std::tuple<bool, int, int> >& vm,
				std::vector<bool>& p3333, std::vector<bool>& rFlag);
			void removeVertices3333(std::vector<Vertex>& v, std::vector<Normal>& n,
				std::vector < std::tuple<bool, int, int> >& vm, std::vector<Vertex>& nv, std::vector<Normal>& nn);
			void removeQuadrilaterals3333(std::vector<int>& quads,
				std::vector<bool>& p3333, std::vector<bool>& rFlag,
				std::vector< std::tuple<bool, int, int> >& vm, std::vector<int>& nq);
			void simplify3333(std::vector<Vertex>& v, std::vector<Normal>& n, std::vector<int>& quads);
		};

} // namespace homotopy
