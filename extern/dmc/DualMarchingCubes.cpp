#include "DualMarchingCubes.h"
#include "LookupTables.h"


void dmc::DualMarchingCubes::dualMC(const double i0, UGrid& ugrid,
	std::vector<Vertex>& v, std::vector<Normal>& n,
	std::vector<int>& tris, std::vector<int>& quads,
    const bool sFlag)
{
	// collect information about the uniform grid
	const int idim = ugrid.x_size();
	const int jdim = ugrid.y_size();
	const int kdim = ugrid.z_size();
	std::map<int, std::array<int, 5>> m_;
    std::cout << " ... compute iso-surface" << std::endl;
#pragma omp parallel for
    for (int k = 0; k < (kdim - 1); k++)
	{
		for (int j = 0; j < (jdim - 1); j++)
		{
			for (int i = 0; i < (idim - 1); i++)
			{
				double u[8];
				u[0] = ugrid.scalar(i, j, k);
				u[1] = ugrid.scalar(i + 1, j, k);
				u[2] = ugrid.scalar(i, j + 1, k);
				u[3] = ugrid.scalar(i + 1, j + 1, k);
				u[4] = ugrid.scalar(i, j, k + 1);
				u[5] = ugrid.scalar(i + 1, j, k + 1);
				u[6] = ugrid.scalar(i, j + 1, k + 1);
				u[7] = ugrid.scalar(i + 1, j + 1, k + 1);

				//
				uint i_case{ 0 };
				i_case = i_case + ((uint)(u[0] >= i0));
				i_case = i_case + ((uint)(u[1] >= i0)) * 2;
				i_case = i_case + ((uint)(u[2] >= i0)) * 4;
				i_case = i_case + ((uint)(u[3] >= i0)) * 8;
				i_case = i_case + ((uint)(u[4] >= i0)) * 16;
				i_case = i_case + ((uint)(u[5] >= i0)) * 32;
				i_case = i_case + ((uint)(u[6] >= i0)) * 64;
				i_case = i_case + ((uint)(u[7] >= i0)) * 128;

				if (i_case == 0 || i_case == 255)
					continue;
				else
					slice(i0, i_case, i, j, k, u, ugrid, v, n, m_);
			}
		}
	}
    std::cout << " ... projections failed level 1: " << nrProj1 << std::endl;
    std::cout << " ... projections failed level 2: " << nrProj2 << std::endl;
    // collect quadrilaterals
    std::vector<int> colors;
    std::cout << " ... collect quadrilaterals" << std::endl;
	collectQuadrilaterals(quads, colors, m_);
    // remove unused vertices, e.g. at boundaries
    removeUnusedVertices(v, n, quads);
    // count elements
    std::cout << " ... nr. of vertices: " << v.size() << std::endl;
    std::cout << " ... nr. of quads: " << (quads.size() / 4) << std::endl;
    if (sFlag)
    {
        // simplify 3X3Y
        std::cout << " ... mesh simplification - P3X3Y" << std::endl;
        simplify3X3Y(v, n, quads, colors);
        // count elements
        std::cout << " ... nr. of vertices: " << v.size() << std::endl;
        std::cout << " ... nr. of quads: " << (quads.size() / 4) << std::endl;
        // simplify 3333
        std::cout << " ... mesh simplification - P3333" << std::endl;
        simplify3333(v, n, quads);
        // count elements
        std::cout << " ... nr. of vertices: " << v.size() << std::endl;
        std::cout << " ... nr. of quads: " << (quads.size() / 4) << std::endl;
    }
    // compute final halfedge data structure and find out nr. of non-manifold elements
    std::cout << " ... compute halfedges" << std::endl;
    const int nr_v{ static_cast<int>(v.size()) };
    std::vector<std::array<int, 5>> he_; // mark manifold and non-manifold halfedges
    std::vector<int> v_;
    std::vector<int> f_;
    halfedges(nr_v, quads, he_, v_, f_);
	// compute triangles
    std::cout << " ... compute triangles" << std::endl;
	collectTriangles(tris, quads, v);
    std::cout << " ... done!" << std::endl;
}


//=====================================================================================================================
//=====================================================================================================================
//  Compute intersection of iso-surface with a cell
//
//=====================================================================================================================
//=====================================================================================================================
void dmc::DualMarchingCubes::slice(const double i0, const uint i_case, const int i_index, const int j_index, const int k_index,
    double f[8], UGrid& ugrid, std::vector<Vertex>& v, std::vector<Normal>& n, std::map<int, std::array<int, 5>>& m_)
{
    /// compute mc polygons
    unsigned long long c_ = 0xFFFFFFFFFFFF0000;
    uint cnt_{ 0 };
    if (t_ambig[i_case] == MC_AMBIGUOUS)
    {
        cnt_ = mc_polygon(i0, f, c_);
    }
    else {
        cnt_ = mc_polygon(i_case, c_);
    }
    const unsigned char l_edges_[12]{ 16, 49, 50, 32, 84, 117, 118, 100, 64, 81, 115, 98 };
    ushort e_{ 0 };
    // collect data
    std::vector<std::array<double, 2>> bboxU(4, { 0,0, });
    std::vector<std::array<double, 2>> bboxV(4, { 0,0, });
    std::vector<std::array<double, 2>> bboxW(4, { 0,0, });
    for (uint t = 0; t < cnt_; t++)
    {
        bboxU[t][0] = 1;
        bboxU[t][1] = 0;
        bboxV[t][0] = 1;
        bboxV[t][1] = 0;
        bboxW[t][0] = 1;
        bboxW[t][1] = 0;
        const int cnt_size = get_cnt_size(t, c_);
        for (int i = 0; i < cnt_size; i++)
        {
            const uint e = get_c(t, i, c_);
            Vertex ui;
            getLocalCoordinates(l_edges_[e], e, f, i0, ui);
            bboxU[t][0] = bboxU[t][0] > ui[0] ? ui[0] : bboxU[t][0];
            bboxU[t][1] = bboxU[t][1] < ui[0] ? ui[0] : bboxU[t][1];
            bboxV[t][0] = bboxV[t][0] > ui[1] ? ui[1] : bboxV[t][0];
            bboxV[t][1] = bboxV[t][1] < ui[1] ? ui[1] : bboxV[t][1];
            bboxW[t][0] = bboxW[t][0] > ui[2] ? ui[2] : bboxW[t][0];
            bboxW[t][1] = bboxW[t][1] < ui[2] ? ui[2] : bboxW[t][1];
            //set edge case to construct oriented quadrilateral
            if (f[(l_edges_[e] & 0xF)] < i0) e_ |= (1 << e);
        }
    }
    // compute representative and quad indices
    for (uint t = 0; t < cnt_; t++)
    {
        // compute first estimate of vertex representative
        const int cnt_size = get_cnt_size(t, c_);
        Vertex p;
        for (int i = 0; i < cnt_size; i++)
        {
            const int e = get_c(t, i, c_);
            Vertex ui;
            getLocalCoordinates(l_edges_[e], e, f, i0, ui);
            p += ui;
        }
        p /= cnt_size;
        uint nrProjectionsFailed1{ 0 };
        uint nrProjectionsFailed2{ 0 };
        representative(i0, f, cnt_, c_, t, p, bboxU, bboxV, bboxW, nrProjectionsFailed1, nrProjectionsFailed2);
        nrProj1 += nrProjectionsFailed1;
        nrProj2 += nrProjectionsFailed2;

        // compute normal at mesh vertex
        Normal ni;
        ugrid.normal(ni, i_index, j_index, k_index, p[0], p[1], p[2]);
        // compute euclidean coordinates of new vertex
        Point pi;
        ugrid.position(pi, i_index, j_index, k_index, p[0], p[1], p[2]);
        // add vertex and normal to list
#pragma omp critical
        {
            const int v_addr = static_cast<int>(v.size());
            v.push_back(pi);
            n.push_back(ni);
            //cnt_size = get_cnt_size(t, c_);
            for (int i = 0; i < cnt_size; i++)
            {
                const uint e = get_c(t, i, c_);
                // compute unique edges id
                const int e_glId = e_glIndex(e, i_index, j_index, k_index, ugrid);
                const int pos = get_vertex_pos(e, (e_ >> e) & 1);
                // compute color
                int color{ -1 };
                if (e == 0) color = 3 * ((i_index & 1) | (j_index & 1) << 1 | (k_index & 1) << 2);
                if (e == 3) color = 3 * ((i_index & 1) | (j_index & 1) << 1 | (k_index & 1) << 2) + 1;
                if (e == 8) color = 3 * ((i_index & 1) | (j_index & 1) << 1 | (k_index & 1) << 2) + 2;
                //add vertex to has table
                if (color == -1)
                    addVertex(e_glId, pos, v_addr, m_);
                else
                    addVertex(e_glId, pos, v_addr, color, m_);
            }
        }
    }
}

void dmc::DualMarchingCubes::representative(const double i0, const double f[8], uint cnt_, ulong& c_, int t, Vertex& p,
    std::vector<std::array<double, 2>>& bboxU,
    std::vector<std::array<double, 2>>& bboxV,
    std::vector<std::array<double, 2>>& bboxW,
    uint& count1, uint& count2)
{
    const uint w_proj = 16434824;
    const uint v_proj = 16362248;
    const uint u_proj = 16096528;
    const int nrSamples{ 9 };
    // compute case
    ProjectionDirection prj{ ProjectionDirection::W_PROJECTION };
    if (get_cnt_size(t, c_) >= 6)
    {
        Vector ni = gradient(f, p[0], p[1], p[2]);
        prj = lProjection(ni);
    }
    else
    {
        prj = minProjection(bboxU[t][1] - bboxU[t][0], bboxV[t][1] - bboxV[t][0], bboxW[t][1] - bboxW[t][0]);
    }
    Vertex pt;
    switch (prj) {
    case ProjectionDirection::W_PROJECTION:
    {
        pt[0] = p[0];
        pt[1] = p[1];
        pt[2] = p[2];
        if (!projection(w_proj, i0, f, cnt_, c_, t, pt, bboxU, bboxV, bboxW, nrSamples))
        {
            count1++;
            if (!projection(w_proj, i0, f, cnt_, c_, t, pt, bboxU, bboxV, bboxW, nrSamples * 3 + 1))
            {
                count2++;
            }
        }
        p[0] = pt[0];
        p[1] = pt[1];
        p[2] = pt[2];
        break;
    }
    case ProjectionDirection::V_PROJECTION:
    {
        pt[0] = p[0];
        pt[1] = p[2];
        pt[2] = p[1];
        if (!projection(v_proj, i0, f, cnt_, c_, t, pt, bboxU, bboxW, bboxV, nrSamples))
        {
            count1++;
            if (!projection(v_proj, i0, f, cnt_, c_, t, pt, bboxU, bboxW, bboxV, nrSamples * 3 + 1))
            {
                count2++;
            }
        }
        p[0] = pt[0];
        p[1] = pt[2];
        p[2] = pt[1];
        break;
    }
    case ProjectionDirection::U_PROJECTION:
    {
        pt[0] = p[1];
        pt[1] = p[2];
        pt[2] = p[0];
        if (!projection(u_proj, i0, f, cnt_, c_, t, pt, bboxV, bboxW, bboxU, nrSamples))
        {
            count1++;
            if (!projection(u_proj, i0, f, cnt_, c_, t, pt, bboxV, bboxW, bboxU, nrSamples * 3 + 1))
            {
                count2++;
            }
        }
        p[0] = pt[2];
        p[1] = pt[0];
        p[2] = pt[1];
        break;
    }
    }
}

bool dmc::DualMarchingCubes::projection(const uint r, const double i0, const double f[8], uint cnt_, ulong& c_, int t, Vertex& p,
    std::vector<std::array<double, 2>>& bboxU,
    std::vector<std::array<double, 2>>& bboxV,
    std::vector<std::array<double, 2>>& bboxW,
    const int nrSamples)
{
    const double umin{ bboxU[t][0] };
    const double umax{ bboxU[t][1] };
    const double vmin{ bboxV[t][0] };
    const double vmax{ bboxV[t][1] };
    const double du = (umax - umin) / (nrSamples - 1);
    const double dv = (vmax - vmin) / (nrSamples - 1);
    double minDistance = 100;
    Vertex pt{ 0,0,-1 };
    double wmin{ bboxW[t][0] };
    double wmax{ bboxW[t][1] };
    const double eps{ 1e-5 };
    // consider tiny bounding box
    if (std::fabs(wmax - wmin) < eps) {
        wmin -= eps;
        wmax += eps;
    }

    const int cnt_size = get_cnt_size(t, c_);
    for (int i = 1; i < (nrSamples - 1); i++) {
        const float u = umin + i * du;
        for (int j = 1; j < (nrSamples - 1); j++) {
            const float v = vmin + j * dv;
            const float g1 = (1 - v) * ((1 - u) * f[0] + u * f[(r >> 3) & 0x7]) + v * ((1 - u) * f[(r >> 6) & 0x7] + u * f[(r >> 9) & 0x7]);
            const float g2 = (1 - v) * ((1 - u) * f[(r >> 12) & 0x7] + u * f[(r >> 15) & 0x7]) + v * ((1 - u) * f[(r >> 18) & 0x7] + u * f[(r >> 21) & 0x7]);
            if (g1 == g2) continue;
            const float w = (i0 - g1) / (g2 - g1);
            if (wmin <= w && w <= wmax)
            {
                if (cnt_ == 1 || cnt_size == 3 || !isInNeighborBBox(u, v, w, cnt_, c_, t, bboxU,bboxV,bboxW))
                {
                    const float d = (p[0] - u) * (p[0] - u) + (p[1] - v) * (p[1] - v) + (p[2] - w) * (p[2] - w);
                    if (minDistance > d) {
                        minDistance = d;
                        pt[0] = u;
                        pt[1] = v;
                        pt[2] = w;
                    }
                }
            }
        }
    }
    if (pt[2] > -1) {
        p = pt;
        return true;
    }
    else return false;
}

unsigned int dmc::DualMarchingCubes::mc_polygon(const double i0, const double F[8], ulong& c_)
{
    // compute oriented contours
    // 1. build segments
    // 2. connect segments
    // build up segments
    // set segments map
    unsigned char segm_[12] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    for (int f = 0; f < 6; f++) {
        // classify face
        unsigned int f_case{ 0 };
        const int v0 = get_face_v(f, 0);
        const int v1 = get_face_v(f, 1);
        const int v2 = get_face_v(f, 2);
        const int v3 = get_face_v(f, 3);
        const int e0 = get_face_e(f, 0);
        const int e1 = get_face_e(f, 1);
        const int e2 = get_face_e(f, 2);
        const int e3 = get_face_e(f, 3);
        const double f0 = F[v0];
        const double f1 = F[v1];
        const double f2 = F[v2];
        const double f3 = F[v3];
        if (f0 >= i0)
            f_case |= BIT_1;
        if (f1 >= i0)
            f_case |= BIT_2;
        if (f2 >= i0)
            f_case |= BIT_3;
        if (f3 >= i0)
            f_case |= BIT_4;
        switch (f_case)
        {
        case 1:
            set_segm(e0, e3, segm_);
            break;
        case 2:
            set_segm(e1, e0, segm_);
            break;
        case 3:
            set_segm(e1, e3, segm_);
            break;
        case 4:
            set_segm(e3, e2, segm_);
            break;
        case 5:
            set_segm(e0, e2, segm_);
            break;
        case 6:
        {
            const double val = asymptotic_decider(f0, f1, f2, f3);
            if (val >= i0) {
                set_segm(e3, e0, segm_);
                set_segm(e1, e2, segm_);
            }
            else if (val < i0) {
                set_segm(e1, e0, segm_);
                set_segm(e3, e2, segm_);
            }
        }
        break;
        case 7:
            set_segm(e1, e2, segm_);
            break;
        case 8:
            set_segm(e2, e1, segm_);
            break;
        case 9:
        {
            const double val = asymptotic_decider(f0, f1, f2, f3);
            if (val >= i0) {
                set_segm(e0, e1, segm_);
                set_segm(e2, e3, segm_);
            }
            else if (val < i0) {
                set_segm(e0, e3, segm_);
                set_segm(e2, e1, segm_);
            }
        }
        break;
        case 10:
            set_segm(e2, e0, segm_);
            break;
        case 11:
            set_segm(e2, e3, segm_);
            break;
        case 12:
            set_segm(e3, e1, segm_);
            break;
        case 13:
            set_segm(e0, e1, segm_);
            break;
        case 14:
            set_segm(e3, e0, segm_);
            break;
        default:
            break;
        }
    }

    // connect oriented segments into oriented contours
    // closed contours are coded in 64 bit unsigned long long
    // 1) Each entry has 4 bits
    // 2) The first 4 entries are reserved for the size of the contours
    // 3) The next 12 entries are the indices of the edges constituting the contorus
    //    The indices are numbers from 0 to 12
    //unsigned long long c_ = 0xFFFFFFFFFFFF0000;
    // in the 4 first bits store size of contours
    // connect oriented contours
    int cnt_{ 0 };
    for (uint e = 0; e < 12; e++) {
        if (is_segm_set(e, segm_)) {
            uint eTo = get_segm(e, 0, segm_);
            uint eIn = get_segm(e, 1, segm_);
            uint eStart = e;
            uint pos = 0;
            set_c(cnt_, pos, eStart, c_);
            while (eTo != eStart) {
                pos = pos + 1;
                set_c(cnt_, pos, eTo, c_);
                eIn = eTo;
                eTo = get_segm(eIn, 0, segm_);
                unset_segm(eIn, segm_);
            }
            // set contour length
            set_cnt_size(cnt_, pos + 1, c_);
            // update number of contours
            cnt_ = cnt_ + 1;
        }
    }

    // return number of countures
    return cnt_;
}

unsigned int dmc::DualMarchingCubes::mc_polygon(const int i_case, ulong& c_)
{
    int cnt_{ 0 };
    const char* c_lt = &r_pattern[17 * i_case];
    unsigned char pos2 = c_lt[0] + 1;
    for (int c = 1; c <= c_lt[0]; c++) // loop over contours
    {
        // set polygon size
        set_cnt_size(cnt_, c_lt[c], c_);
        // for all this edges save the vertex
        for (int i = 0; i < c_lt[c]; i++)
        {
            const uint e = c_lt[pos2++];
            set_c(cnt_, i, e, c_);
        }
        cnt_++;
    }
    return cnt_;
}

// Mesh simplification
//=================================================================================================================
// Mesh simplification
// Helper functions
//=================================================================================================================
void dmc::DualMarchingCubes::halfedges(const int nr_v, std::vector<int>& quads, std::vector<Halfedge>& he, std::vector<int>& he_v, std::vector<int>& he_f)
{
    bndVertices.clear();
    const int nr_q = static_cast<int>(quads.size()) / 4;
    //he_v.resize(nr_v);
    he_v.assign(nr_v, INVALID_INDEX);
    //he_f.resize(nr_q);
    he_f.assign(nr_q, INVALID_INDEX);
    he.resize(4 * nr_q);
    std::map<ulong, std::array<int, 5>> m_;
    auto setKey = [](const int v0, const int v1)
    {
        if (v0 < v1)
            return (static_cast<ulong>(v0) << 32) | (v1 & 0xffffffffL);
        else
            return (static_cast<ulong>(v1) << 32) | (v0 & 0xffffffffL);
    };
    auto addHalfedge = [this](const ulong key, const int he, std::map<ulong, std::array<int,5>>& m)
    {
        auto e = m.find(key);
        if (e != m.end())
        {
            // the element alread exist
            const int pos = e->second[4];
            e->second[pos] = he;
            e->second[4] = pos + 1;
        }
        else
        {
            // the element has to be created
            // 1. id of halfedge itself, i.e. index = 0
            // 2. - 4. are other halfedges sharing the same two vertices, i.e. indices 1 - 3
            // 5. pointer into array giving address to write next halfedge id, start at second slot, i.e. index = 1
            std::array<int, 5> h{ INVALID_INDEX,INVALID_INDEX,INVALID_INDEX,INVALID_INDEX, 1 };
            h[0] = he;
            m[key] = h;
        }
    };
#pragma omp parallel for
    for (int i = 0; i < nr_q; i++)
    {
        // quad vertices
        const int v0 = quads[4 * i];
        const int v1 = quads[4 * i + 1];
        const int v2 = quads[4 * i + 2];
        const int v3 = quads[4 * i + 3];
        // he[0] = origin vertex
        // he[1] = face
        // he[2] = next
        // he[3] = tween
        // he[4] = 0 - manifold, 1 non-manifold
        Halfedge e0_{ INVALID_INDEX, i, INVALID_INDEX, INVALID_INDEX, MANIFOLD };
        Halfedge e1_{ INVALID_INDEX, i, INVALID_INDEX, INVALID_INDEX, MANIFOLD };
        Halfedge e2_{ INVALID_INDEX, i, INVALID_INDEX, INVALID_INDEX, MANIFOLD };
        Halfedge e3_{ INVALID_INDEX, i, INVALID_INDEX, INVALID_INDEX, MANIFOLD };
        // origin vertex
        e0_[0] = v0;
        e1_[0] = v1;
        e2_[0] = v2;
        e3_[0] = v3;
        // next
        e0_[2] = 4 * i + 1;
        e1_[2] = 4 * i + 2;
        e2_[2] = 4 * i + 3;
        e3_[2] = 4 * i;
        // set
        he[4 * i] = e0_;
        he[4 * i + 1] = e1_;
        he[4 * i + 2] = e2_;
        he[4 * i + 3] = e3_;
        // collect faces
        he_f[i] = 4 * i; // this face is pointing to the "first" halfedge
        // collect vertices
        he_v[v0] = 4 * i;
        he_v[v1] = 4 * i + 1;
        he_v[v2] = 4 * i + 2;
        he_v[v3] = 4 * i + 3;
        // add halfedges to hash table
#pragma omp critical
        {
            addHalfedge(setKey(v0, v1), 4 * i, m_);
            addHalfedge(setKey(v1, v2), 4 * i + 1, m_);
            addHalfedge(setKey(v2, v3), 4 * i + 2, m_);
            addHalfedge(setKey(v3, v0), 4 * i + 3, m_);
        }
    }
    // connect halfedge twins
    int nr_nonmanifold{ 0 };
    for (auto e : m_)
    {
        const int c = e.second[4];
        switch (c)
        {
        case 1:
        {
            const int e0 = e.second[0];
            he[e0][3] = INVALID_INDEX;
            break;
        }
        case 2:
        {
            const int e0 = e.second[0];
            const int e1 = e.second[1];
            he[e0][3] = e1;
            he[e1][3] = e0;
            break;
        }
        case 3:
        {
            const int e0 = e.second[0];
            const int e1 = e.second[1];
            const int e2 = e.second[2];
            const int v0 = he[e0][0];
            const int v1 = he[e1][0];
            const int v2 = he[e2][0];
            he[e0][4] = BOUNDARY_MULTIPLE_EDGE;
            he[e1][4] = BOUNDARY_MULTIPLE_EDGE;
            he[e2][4] = BOUNDARY_MULTIPLE_EDGE;
            if (v0 != v1) {
                bndVertices.push_back(v0);
                bndVertices.push_back(v1);
                he[e0][3] = e1;
                he[e1][3] = e0;
                he[e2][3] = INVALID_INDEX;
                he_v[v2] = e2;
            }
            else if (v0 != v2) {
                bndVertices.push_back(v0);
                bndVertices.push_back(v2);
                he[e0][3] = e2;
                he[e2][3] = e0;
                he[e1][3] = INVALID_INDEX;
                he_v[v1] = e1;
            }
            else {
                bndVertices.push_back(v1);
                bndVertices.push_back(v2);
                he[e1][3] = e2;
                he[e2][3] = e1;
                he[e0][3] = INVALID_INDEX;
                he_v[v0] = e0;
            }
            nr_nonmanifold++;
            break;
        }
        case 4:
        {
            nr_nonmanifold++;
            const int e0 = e.second[0];
            const int e1 = e.second[1];
            const int e2 = e.second[2];
            const int e3 = e.second[3];
            // collect vertices
            const int v0 = he[e0][0];
            const int v1 = he[e1][0];
            const int v2 = he[e2][0];
            const int v3 = he[e3][0];
            if (v0 != v1)
            {
                he[e0][3] = e1;
                he[e1][3] = e0;
                he[e2][3] = e3;
                he[e3][3] = e2;
            }
            else
            {
                he[e0][3] = e2;
                he[e2][3] = e0;
                he[e1][3] = e3;
                he[e3][3] = e1;
            }
            he[e0][4] = NON_MANIFOLD;
            he[e1][4] = NON_MANIFOLD;
            he[e2][4] = NON_MANIFOLD;
            he[e3][4] = NON_MANIFOLD;
            break;
        }
        default:
            std::cout << " ... ERROR: wrong nr. of faces sharing an edge: " << c << std::endl;
            break;
        }
    }
    std::cout << " ... nr. of non-manifold edges: " << nr_nonmanifold << std::endl;
}

std::array<int, 4> dmc::DualMarchingCubes::collectNeighbors(const int quad, std::vector<Halfedge>& he, std::vector<int>& he_f)
{
    // he[0] = origin vertex
    // he[1] = face
    // he[2] = next
    // he[3] = tween
    // he[4] = 0 - manifold, 1 non-manifold
    // collect the four halfedges
    const int e0 = he_f[quad];
    const int e1 = he[e0][2];
    const int e2 = he[e1][2];
    const int e3 = he[e2][2];
    // collect neighbors
    const int t0 = he[e0][3];
    const int t1 = he[e1][3];
    const int t2 = he[e2][3];
    const int t3 = he[e3][3];
    // collect faces
    int f0{ -1 };
    int f1{ -1 };
    int f2{ -1 };
    int f3{ -1 };
    // set neighbors
    if (t0 != INVALID_INDEX) f0 = he[t0][1];
    if (t1 != INVALID_INDEX) f1 = he[t1][1];
    if (t2 != INVALID_INDEX) f2 = he[t2][1];
    if (t3 != INVALID_INDEX) f3 = he[t3][1];

    return { f0,f1,f2,f3 };

}

/// <summary>
/// Faces hinterit from uniform grid a coloring with 24 colors, the gird coloring is extended to an grid edge coloring
/// with 24 colors. The number of colors has to be reduced to 4, if possible. Proceed in two steps
/// 1. reduce the number of colors to 5: 0, 1, 2, 3, 4
/// 2. re-color as many faces as possible that have color 4 (the fifth color)
/// </summary>
/// <param name="he">halfedges</param>
/// <param name="he_f">halfedge faces</param>
/// <param name="colors">face colors</param>
void dmc::DualMarchingCubes::colorFaces(std::vector<Halfedge>& he, std::vector<int>& he_f, std::vector<int>& colors)
{
    const int nr_q{ static_cast<int>(he_f.size()) };
    // classify faces with colors larger than 5
    std::array<std::list<int>, 19> fC;
    std::list<int> fColor4;
    for (int f = 0; f < nr_q; f++)
    {
        if (colors[f] > 4) // color larger than fifth color
        {
            fC[colors[f] - 5].push_back(f);
        }
        else if (colors[f] == 4)
        {
            fColor4.push_back(f);
        }
    }
    // for each color simplify
    for (auto c : fC)
    {
        for (auto f : c)
        {
            // collect neighbors
            std::array<int, 4> n = collectNeighbors(f, he, he_f);
            // collect colors
            int c0{ INVALID_COLOR };
            int c1{ INVALID_COLOR };
            int c2{ INVALID_COLOR };
            int c3{ INVALID_COLOR };
            if (n[0] != INVALID_INDEX) c0 = colors[n[0]];
            if (n[1] != INVALID_INDEX) c1 = colors[n[1]];
            if (n[2] != INVALID_INDEX) c2 = colors[n[2]];
            if (n[3] != INVALID_INDEX) c3 = colors[n[3]];
            // this colors must be all larger than 4
            if (c0 != 0 && c1 != 0 && c2 != 0 && c3 != 0)
            {
                colors[f] = 0;
                continue;
            }
            if (c0 != 1 && c1 != 1 && c2 != 1 && c3 != 1)
            {
                colors[f] = 1;
                continue;
            }
            if (c0 != 2 && c1 != 2 && c2 != 2 && c3 != 2)
            {
                colors[f] = 2;
                continue;
            }
            if (c0 != 3 && c1 != 3 && c2 != 3 && c3 != 3)
            {
                colors[f] = 3;
                continue;
            }
            if (c0 != 4 && c1 != 4 && c2 != 4 && c3 != 4)
            {
                colors[f] = 4;
                continue;
            }
        }
    }
    // optimize colors, reduce the number of faces with color 4 as much as possible
    //for (int f = 0; f < nr_q; f++)
    for (auto f : fColor4)
    {
        // collect neighbors
        std::array<int, 4> n = collectNeighbors(f, he, he_f);
        // collect colors
        int c0{ INVALID_COLOR };
        int c1{ INVALID_COLOR };
        int c2{ INVALID_COLOR };
        int c3{ INVALID_COLOR };
        if (n[0] != INVALID_INDEX) c0 = colors[n[0]];
        if (n[1] != INVALID_INDEX) c1 = colors[n[1]];
        if (n[2] != INVALID_INDEX) c2 = colors[n[2]];
        if (n[3] != INVALID_INDEX) c3 = colors[n[3]];
        // this colors must be all larger than 4
        if (c0 != 0 && c1 != 0 && c2 != 0 && c3 != 0)
        {
            colors[f] = 0;
            continue;
        }
        if (c0 != 1 && c1 != 1 && c2 != 1 && c3 != 1)
        {
            colors[f] = 1;
            continue;
        }
        if (c0 != 2 && c1 != 2 && c2 != 2 && c3 != 2)
        {
            colors[f] = 2;
            continue;
        }
        if (c0 != 3 && c1 != 3 && c2 != 3 && c3 != 3)
        {
            colors[f] = 3;
            continue;
        }
    }
    /*
    // check which colors remain
    std::array<int, 6> c_{ 0,0,0,0,0,0 };
    for (auto c : colors)
    {
        switch (c)
        {
        case 0:
            c_[0]++;
            break;
        case 1:
            c_[1]++;
            break;
        case 2:
            c_[2]++;
            break;
        case 3:
            c_[3]++;
            break;
        case 4:
            c_[4]++;
            break;
        default:
            c_[5]++;
            break;
        }
    }
    for (auto c : c_)
    {
        std::cout << " ... colors: " << c << std::endl;
    }
    */
}


void dmc::DualMarchingCubes::vertexValence(const int nr_v, std::vector<Halfedge>& he_, std::vector<int>& vV)
{
    // he[0] = origin vertex
    // he[1] = face
    // he[2] = next
    // he[3] = tween
    // he[4] = 0 - manifold, 1 non-manifold
    const int nr_he{ static_cast<int>(he_.size()) };
    vV.resize(nr_v);
    std::fill(vV.begin(), vV.end(), 0);
    for (int e = 0; e < nr_he; e++)
    {
        vV[he_[e][0]]++;
        // check if boundary edge
        if (he_[e][3] == INVALID_INDEX)
        {
            const int ne{ he_[e][2] };
            vV[he_[ne][0]]++;
        }
    }
    const int minV = *std::min_element(vV.begin(), vV.end());
    if (minV == 0) {
        std::cout << "ERROR: there are elements with valence 0" << std::endl;
    }
}

bool dmc::DualMarchingCubes::isNonManifold(const int f, std::vector<Halfedge>& he, std::vector<int>& he_f)
{
    const int e0 = he_f[f];
    const int e1 = he[e0][2];
    const int e2 = he[e1][2];
    const int e3 = he[e2][2];
    if (he[e0][4] == NON_MANIFOLD || he[e1][4] == NON_MANIFOLD || he[e2][4] == NON_MANIFOLD || he[e3][4] == NON_MANIFOLD)
        return true;
    else
        return false;
}

//=================================================================================================================
// Mesh simplification
// Pattern 3X3Y
//=================================================================================================================
void dmc::DualMarchingCubes::mark3X3Y(std::vector<int>& quads, std::vector<int>& vV, std::vector<bool>& p3X3Y)
{
    const int nr_q{ static_cast<int>(quads.size()) / 4 };
    p3X3Y.resize(nr_q);
    std::fill(p3X3Y.begin(), p3X3Y.end(), false);
#pragma omp parallel for
    for (int f = 0; f < nr_q; f++)
    {
        const int v0{ quads[4 * f] };
        const int v1{ quads[4 * f + 1] };
        const int v2{ quads[4 * f + 2] };
        const int v3{ quads[4 * f + 3] };
        const int valence0 = vV[v0];
        const int valence1 = vV[v1];
        const int valence2 = vV[v2];
        const int valence3 = vV[v3];

        bool flag1 = (valence0 == 3 && valence1 >= 5 && valence2 == 3 && valence3 >= 5);
        bool flag2 = (valence0 >= 5 && valence1 == 3 && valence2 >= 5 && valence3 == 3);
        if (flag1 || flag2)
        {
            p3X3Y[f] = true;
        }
    }
}

// Color based simplification of elements with vertex valence pattern 3X3Y
void dmc::DualMarchingCubes::mergeVertices3X3Y(std::vector<Vertex>& v, std::vector<Normal>& normals, std::vector<bool> p3X3Y,
    std::vector<int>& vV, std::vector<int>& colors, std::vector<Halfedge>& he, std::vector<int>& he_f,
    std::vector<std::pair<bool, int>>& vm_, std::vector<bool>& em_)
{
    const int nr_q{ static_cast<int>(he_f.size()) };
    const int nr_v{ static_cast<int>(v.size()) };
    vm_.resize(nr_v, std::make_pair(false, INVALID_INDEX));
    em_.resize(nr_q, false);
    // main loop
    for (int f = 0; f < nr_q; f++)
    {
        const int c = colors[f];
        if (!p3X3Y[f]) continue;

        // if one edge in non-manifold, do not remove
        // he[0] = origin vertex
        // he[1] = face
        // he[2] = next
        // he[3] = tween
        // he[4] = 0 - manifold, 1 non-manifold
        const int e0 = he_f[f];
        const int e1 = he[e0][2];
        const int e2 = he[e1][2];
        const int e3 = he[e2][2];
        //if (he[e0][4] == NON_MANIFOLD || he[e1][4] == NON_MANIFOLD || he[e2][4] == NON_MANIFOLD || he[e3][4] == NON_MANIFOLD) continue;
        // collect neighbors and check pattern and colors, be careful not to be at the boundary
        std::array<int, 4> n = collectNeighbors(f, he, he_f);
        bool nonManifoldNeighbor{ false };
        for (auto e : n)
        {
            if (e >= 0 && isNonManifold(e,he,he_f))
                nonManifoldNeighbor = true;
            if (e < 0)
                nonManifoldNeighbor = true;
        }
        if (nonManifoldNeighbor) continue;
        int n_color[4]{ INVALID_COLOR,INVALID_COLOR, INVALID_COLOR, INVALID_COLOR };
        bool n_pattern[4]{ false,false,false,false };
        for (int i = 0; i < 4; i++)
        {
            if (n[i] != INVALID_INDEX)
            {
                n_color[i] = colors[n[i]];
                n_pattern[i] = p3X3Y[n[i]];
            }
        }
        // check if element can be removed
        bool flag{ true };
        for (int i = 0; i < 4; i++)
        {
            if (n_pattern[i] && (n_color[i] <= c)) flag = false;
        }
        if (!flag) continue;
        // the element can be removed
        const int v0 = he[e0][0];
        const int v1 = he[e1][0];
        const int v2 = he[e2][0];
        const int v3 = he[e3][0];

        const int valence0 = vV[v0];
        const int valence1 = vV[v1];
        const int valence2 = vV[v2];
        const int valence3 = vV[v3];

        if (valence0 == 3 && valence1 >= 5 && valence2 == 3 && valence3 >= 5)
        {
            // compute new position and normals of v0
            v[v0] = v[v1] + 0.5 * (v[v3] - v[v1]);
            normals[v0] = normals[v1] + 0.5 * (normals[v3] - normals[v1]);
            normals[v0].normalize();

            // mark v2 to be removed
            vm_[v2].first = true;

            // set twins of v2 to be v0, to be able to remove element later
            vm_[v2].second = v0;

            // element has to be removed
            em_[f] = true;
        }
        else if (valence0 >= 5 && valence1 == 3 && valence2 >= 5 && valence3 == 3)
        {
            // compute new position and normal of v1
            v[v1] = v[v0] + 0.5 * (v[v2] - v[v0]);
            normals[v1] = normals[v0] + 0.5 * (normals[v2] - normals[v0]);
            normals[v1].normalize();

            // mark v3 to be removed
            vm_[v3].first = true;
            // set twins, remove v3, use addres of v1
            vm_[v3].second = v1;

            // element has to be removed
            em_[f] = true;
        }
    }
}
// Simplification of elements with vertex valence pattern 3X3Y, only consider neighbors
void dmc::DualMarchingCubes::mergeVertices3X3Y(std::vector<Vertex>& v, std::vector<Normal>& normals, std::vector<bool> p3X3Y,
    std::vector<int>& vV, std::vector<Halfedge>& he, std::vector<int>& he_f,
    std::vector<std::pair<bool, int>>& vm_, std::vector<bool>& em_)
{
    const int nr_q{ static_cast<int>(he_f.size()) };
    const int nr_v{ static_cast<int>(v.size()) };
    vm_.resize(nr_v);
    em_.resize(nr_q, false);
    std::fill(vm_.begin(), vm_.end(), std::make_pair(false, INVALID_INDEX));
    std::fill(em_.begin(), em_.end(), false);
    for (int f = 0; f < nr_q; f++)
    {
        if (!p3X3Y[f]) continue;

        // if one edge in non-manifold, do not remove
        // he[0] = origin vertex
        // he[1] = face
        // he[2] = next
        // he[3] = tween
        // he[4] = 0 - manifold, 1 non-manifold
        const int e0 = he_f[f];
        const int e1 = he[e0][2];
        const int e2 = he[e1][2];
        const int e3 = he[e2][2];
        // collect neghibors and check pattern and colors, be careful not to be at the boundary
        std::array<int, 4> n = collectNeighbors(f, he, he_f);
        if (n[0] != INVALID_INDEX && isNonManifold(n[0], he, he_f)) continue;
        if (n[1] != INVALID_INDEX && isNonManifold(n[1], he, he_f)) continue;
        if (n[2] != INVALID_INDEX && isNonManifold(n[2], he, he_f)) continue;
        if (n[3] != INVALID_INDEX && isNonManifold(n[3], he, he_f)) continue;

        // check if element has neighbor with same vertex valence pattern
        if (n[0] != INVALID_INDEX && p3X3Y[n[0]]) continue;
        if (n[1] != INVALID_INDEX && p3X3Y[n[1]]) continue;
        if (n[2] != INVALID_INDEX && p3X3Y[n[2]]) continue;
        if (n[3] != INVALID_INDEX && p3X3Y[n[3]]) continue;

        // the element can be removed
        const int v0 = he[e0][0];
        const int v1 = he[e1][0];
        const int v2 = he[e2][0];
        const int v3 = he[e3][0];

        const int valence0 = vV[v0];
        const int valence1 = vV[v1];
        const int valence2 = vV[v2];
        const int valence3 = vV[v3];

        if (valence0 == 3 && valence1 >= 5 && valence2 == 3 && valence3 >= 5)
        {
            // compute new position and normals of v0
            v[v0] = v[v1] + 0.5 * (v[v3] - v[v1]);
            normals[v0] = normals[v1] + 0.5 * (normals[v3] - normals[v1]);
            normals[v0].normalize();

            // mark v2 to be removed
            vm_[v2].first = true;

            // set twins of v2 to be v0, to be able to remove element later
            vm_[v2].second = v0;

            // element has to be removed
            em_[f] = true;
        }
        else if (valence0 >= 5 && valence1 == 3 && valence2 >= 5 && valence3 == 3)
        {
            // compute new position and normal of v1
            v[v1] = v[v0] + 0.5 * (v[v2] - v[v0]);
            normals[v1] = normals[v0] + 0.5 * (normals[v2] - normals[v0]);
            normals[v1].normalize();

            // mark v3 to be removed
            vm_[v3].first = true;
            // set twins, remove v3, use address of v1
            vm_[v3].second = v1;

            // element has to be removed
            em_[f] = true;
        }
    }
}

void dmc::DualMarchingCubes::removeVertices3X3Y(std::vector<Vertex>& v, std::vector<Normal>& n, std::vector<std::pair<bool, int>>& vm_,
    std::vector<Vertex>& nv, std::vector<Normal>& nn)
{
    const int nr_v{ static_cast<int>(v.size()) };
    nv.reserve(nr_v);
    nn.reserve(nr_v);
    for (int i = 0; i < nr_v; i++)
    {
        if (!vm_[i].first)
        {
            const int addr{ static_cast<int>(nv.size()) };
            nv.push_back(v[i]);
            nn.push_back(n[i]);
            vm_[i].second = addr;
        }
    }
}

void dmc::DualMarchingCubes::removeQuadrilaterals3X3Y(std::vector<int>& q, std::vector<bool>& em,
    std::vector<std::pair<bool, int>>& vm, std::vector<int>& nq)
{
    const int nr_q{ static_cast<int>(q.size()) / 4 };
    nq.reserve(nr_q);
    for (int f = 0; f < nr_q; f++)
    {
        if (!em[f])
        {
            const int v0 = q[4 * f];
            const int v1 = q[4 * f + 1];
            const int v2 = q[4 * f + 2];
            const int v3 = q[4 * f + 3];
            if (vm[v0].first) nq.push_back(vm[vm[v0].second].second);
            else nq.push_back(vm[v0].second);
            if (vm[v1].first) nq.push_back(vm[vm[v1].second].second);
            else nq.push_back(vm[v1].second);
            if (vm[v2].first) nq.push_back(vm[vm[v2].second].second);
            else nq.push_back(vm[v2].second);
            if (vm[v3].first) nq.push_back(vm[vm[v3].second].second);
            else nq.push_back(vm[v3].second);
        }
    }
}

void dmc::DualMarchingCubes::simplify3X3Y(std::vector<Vertex>& v, std::vector<Normal>& n, std::vector<int>& quads, std::vector<int>& colors)
{
    int nr_v = static_cast<int>(v.size());
    std::vector<std::array<int, 5>> he_; // mark manifold and non-manifold halfedges
    std::vector<int> v_;
    std::vector<int> f_;
    std::vector<int> vV_;
    std::vector<bool> p3X3Y;
    std::vector<std::pair<bool, int>> vm_;
    std::vector<bool> em_;
    std::vector<Vertex> nv;
    std::vector<Normal> nn;
    std::vector<int> nq;
    halfedges(nr_v, quads, he_, v_, f_);
    colorFaces(he_, f_, colors);
    vertexValence(nr_v, he_, vV_);
    mark3X3Y(quads, vV_, p3X3Y);
    mergeVertices3X3Y(v, n, p3X3Y, vV_, colors, he_, f_, vm_, em_);
    removeVertices3X3Y(v, n, vm_, nv, nn);
    removeQuadrilaterals3X3Y(quads, em_, vm_, nq);
    // copy elements back to input arrays
    v.resize(nv.size());
    std::copy(nv.begin(), nv.end(), v.begin());
    n.resize(nn.size());
    std::copy(nn.begin(), nn.end(), n.begin());
    quads.resize(nq.size());
    std::copy(nq.begin(), nq.end(), quads.begin());
    nv.clear();
    nn.clear();
    nq.clear();

    // Remove elements with vertex valence pattern 3X3Y if they do not have a
    // neighbor with the same vertex valence pattern
    nr_v = static_cast<int>(v.size());
    halfedges(nr_v, quads, he_, v_, f_);
    vertexValence(nr_v, he_, vV_);
    mark3X3Y(quads, vV_, p3X3Y);
    mergeVertices3X3Y(v, n, p3X3Y, vV_, he_, f_, vm_, em_);
    removeVertices3X3Y(v, n, vm_, nv, nn);
    removeQuadrilaterals3X3Y(quads, em_, vm_, nq);
    // copy elements back to input arrays
    v.assign(nv.begin(), nv.end());
    n.assign(nn.begin(), nn.end());
    quads.assign(nq.begin(), nq.end());
}

//=================================================================================================================
// Mesh simplification
// Pattern 3333
//=================================================================================================================
void
dmc::DualMarchingCubes::mark3333(const int nr_v, std::vector<int>& quads,
    std::vector < std::array<int, 5> >& he, std::vector<int>& he_f,
    std::vector<int>& vV, std::vector< std::tuple<bool, int, int> >& vm,
    std::vector<bool>& p3333, std::vector<bool>& rFlag)
{
    // he[0] = origin vertex
    // he[1] = face
    // he[2] = next
    // he[3] = twin
    // he[4] = 0 - manifold, 1 non-manifold
    const int nr_q{ static_cast<int>(quads.size()) / 4 };
    p3333.assign(nr_q, false);
    rFlag.assign(nr_q, false);
    //vm.resize(nr_v);
    vm.assign(nr_v, std::make_tuple(false, -1, -1));
#pragma omp parallel for
    for (int f = 0; f < nr_q; f++)
    {
        if (isNonManifold(f, he, he_f)) continue;
        const int e0 = he_f[f];
        const int e1 = he[e0][2];
        const int e2 = he[e1][2];
        const int e3 = he[e2][2];

        const int v0{ he[e0][0] };
        const int v1{ he[e1][0] };
        const int v2{ he[e2][0] };
        const int v3{ he[e3][0] };

        const int valence0 = vV[v0];
        const int valence1 = vV[v1];
        const int valence2 = vV[v2];
        const int valence3 = vV[v3];
        if (valence0 != 3 || valence1 != 3 || valence2 != 3 || valence3 != 3) continue; // all four vertices must have valence 3
        // collect neighborhood
        int twin{ -1 };
        int next{ -1 };
        // f0
        twin = he[e0][3];
        if (twin == -1) continue; // boundary element
        const int f0 = he[twin][1];
        next = he[twin][2];
        next = he[next][2];
        const int v4 = he[next][0];
        // f1
        twin = he[e1][3];
        if (twin == -1) continue; // boundary element
        const int f1 = he[twin][1];
        next = he[twin][2];
        next = he[next][2];
        const int v5 = he[next][0];
        // f2
        twin = he[e2][3];
        if (twin == -1) continue; // boundary element
        const int f2 = he[twin][1];
        next = he[twin][2];
        next = he[next][2];
        const int v6 = he[next][0];
        // f3
        twin = he[e3][3];
        if (twin == -1) continue; // boundary element
        const int f3 = he[twin][1];
        next = he[twin][2];
        next = he[next][2];
        const int v7 = he[next][0];
        // check for manifoldness
        if (isNonManifold(f0, he, he_f)) continue;
        if (isNonManifold(f1, he, he_f)) continue;
        if (isNonManifold(f2, he, he_f)) continue;
        if (isNonManifold(f3, he, he_f)) continue;

        const int valence4 = vV[v4];
        const int valence5 = vV[v5];
        const int valence6 = vV[v6];
        const int valence7 = vV[v7];
        // check if neighor is of the sampe type
        bool flag0 = (valence4 == 3 && valence5 == 3);
        bool flag1 = (valence5 == 3 && valence6 == 3);
        bool flag2 = (valence6 == 3 && valence7 == 3);
        bool flag3 = (valence7 == 3 && valence4 == 3);
        if (flag0 || flag1 || flag2 || flag3) {
            // element can't be removed
            continue;
        }
        // valence will be reduced by one by neighbor vertices,
        // if one vertex has valence 3, simplification can't be done,
        // it will result in one vertex with valence 2
        if (valence4 == 3 || valence5 == 3 || valence6 == 3 || valence7 == 3) {
            // element can't be removed
            continue;
        }
        // check for special case, where removing element would
        // generate a non-manifold mesh
        if (valence4 == 4 && valence5 == 4 && valence6 == 4 && valence7 == 4) {
            continue;
        }
        // mark elements
        // frist element of tuple: vertex is of type 3333 element
        // second element of tuple: twin vertex or new element index
        // third element in tuple: index to map vertex index to. Set invalid index first
        vm[v0] = std::make_tuple(true, v4, -1);
        vm[v1] = std::make_tuple(true, v5, -1);
        vm[v2] = std::make_tuple(true, v6, -1);
        vm[v3] = std::make_tuple(true, v7, -1);
        // mark faces
        p3333[f] = true;
        rFlag[f0] = true;
        rFlag[f1] = true;
        rFlag[f2] = true;
        rFlag[f3] = true;
    }
}
void
dmc::DualMarchingCubes::removeVertices3333(std::vector<Vertex>& v, std::vector<Normal>& n,
    std::vector < std::tuple<bool, int, int> >& vm, std::vector<Vertex>& nv, std::vector<Normal>& nn)
{
    const int nr_v = static_cast<int>(v.size());
    nv.reserve(nr_v);
    nn.reserve(nr_v);
    for (int i = 0; i < nr_v; i++) {
        if (!std::get<0>(vm[i])) {
            // this vertex will not be removed
            const int addr = static_cast<int>(nv.size());
            std::get<2>(vm[i]) = addr;
            nv.push_back(v[i]);
            nn.push_back(n[i]);
        }
    }
}
void
dmc::DualMarchingCubes::removeQuadrilaterals3333(std::vector<int>& quads,
    std::vector<bool>& p3333, std::vector<bool>& rFlag,
    std::vector< std::tuple<bool, int, int> >& vm, std::vector<int>& nq)
{
    const int nr_q{ static_cast<int>(quads.size()) / 4 };
    nq.reserve(4 * nr_q);
    for (int f = 0; f < nr_q; f++) {
        if (!rFlag[f])
        {
            // compute new quadrilateral
            const int v0 = quads[4 * f];
            const int v1 = quads[4 * f + 1];
            const int v2 = quads[4 * f + 2];
            const int v3 = quads[4 * f + 3];
            int i0{ -1 };
            int i1{ -1 };
            int i2{ -1 };
            int i3{ -1 };
            if (p3333[f]) {
                const int twin0 = std::get<1>(vm[v0]);
                const int twin1 = std::get<1>(vm[v1]);
                const int twin2 = std::get<1>(vm[v2]);
                const int twin3 = std::get<1>(vm[v3]);
                i0 = std::get<2>(vm[twin0]);
                i1 = std::get<2>(vm[twin1]);
                i2 = std::get<2>(vm[twin2]);
                i3 = std::get<2>(vm[twin3]);
            }
            else {
                i0 = std::get<2>(vm[v0]);
                i1 = std::get<2>(vm[v1]);
                i2 = std::get<2>(vm[v2]);
                i3 = std::get<2>(vm[v3]);
            }
            nq.push_back(i0);
            nq.push_back(i1);
            nq.push_back(i2);
            nq.push_back(i3);
        }
    }
}

void dmc::DualMarchingCubes::simplify3333(std::vector<Vertex>& v, std::vector<Normal>& n, std::vector<int>& quads)
{
    int nr_v = static_cast<int>(v.size());
    std::vector<std::array<int, 5>> he_; // mark manifold and non-manifold halfedges
    std::vector<int> v_;
    std::vector<int> f_;
    std::vector<int> vV_;
    std::vector<bool> p3333;
    std::vector<bool> rFlag;
    std::vector<std::tuple<bool, int, int>> vm_;
    std::vector<bool> em_;
    std::vector<Vertex> nv;
    std::vector<Normal> nn;
    std::vector<int> nq;
    halfedges(nr_v, quads, he_, v_, f_);
    vertexValence(nr_v, he_, vV_);
    mark3333(nr_v, quads, he_, f_, vV_, vm_, p3333, rFlag);
    removeVertices3333(v, n, vm_, nv, nn);
    removeQuadrilaterals3333(quads, p3333, rFlag, vm_, nq);
    // copy elements back to input arrays
    v.assign(nv.begin(), nv.end());
    n.assign(nn.begin(), nn.end());
    quads.assign(nq.begin(), nq.end());
}

void dmc::DualMarchingCubes::collectTriangles(std::vector<int>& tris, std::vector<int>& quads, std::vector<Vertex>& v)
{
    const int nr_q = static_cast<int>(quads.size()) / 4;
#pragma omp parallel for
    for (int i = 0; i < nr_q; i++)
    {
        const int v0 = quads[4 * i];
        const int v1 = quads[4 * i + 1];
        const int v2 = quads[4 * i + 2];
        const int v3 = quads[4 * i + 3];
        /*tris.push_back(v0);
        tris.push_back(v1);
        tris.push_back(v2);
        // second triangle
        tris.push_back(v0);
        tris.push_back(v2);
        tris.push_back(v3);
        */
        double a1_ = minAngle(v[v0], v[v1], v[v2]);
        double a2_ = minAngle(v[v0], v[v2], v[v3]);
        const double b1_ = std::min(a1_, a2_);
        const double b2_ = std::max(a1_, a2_);
        a1_ = minAngle(v[v1], v[v3], v[v0]);
        a2_ = minAngle(v[v1], v[v2], v[v3]);
        const double c1_ = std::min(a1_, a2_);
        const double c2_ = std::max(a1_, a2_);
        std::array<int, 6> t;
        if (b1_ < c1_ || (b1_ == c1_ && b2_ <= c2_))
        {
            t[0] = v1;
            t[1] = v3;
            t[2] = v0;
            t[3] = v1;
            t[4] = v2;
            t[5] = v3;
            /*// first triangle
            tris.push_back(v1);
            tris.push_back(v3);
            tris.push_back(v0);
            // second triangle
            tris.push_back(v1);
            tris.push_back(v2);
            tris.push_back(v3);
            */
        }
        else
        {
            t[0] = v0;
            t[1] = v1;
            t[2] = v2;
            t[3] = v0;
            t[4] = v2;
            t[5] = v3;
            /*
            tris.push_back(v0);
            tris.push_back(v1);
            tris.push_back(v2);
            // second triangle
            tris.push_back(v0);
            tris.push_back(v2);
            tris.push_back(v3);
            */
        }
#pragma omp critical
        {
            tris.push_back(t[0]);
            tris.push_back(t[1]);
            tris.push_back(t[2]);
            tris.push_back(t[3]);
            tris.push_back(t[4]);
            tris.push_back(t[5]);
        }
    }
}
