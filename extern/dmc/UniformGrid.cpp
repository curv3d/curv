#include "UniformGrid.h"

void dmc::UniformGrid::init(const std::string& filename)
{
	std::ifstream ifile;
	ifile.open(filename, std::ios::binary);
	if (!ifile.is_open()) {
		exit(1);
	}

	ifile.read(reinterpret_cast<char*>(&m_nx), sizeof(int));
	ifile.read(reinterpret_cast<char*>(&m_ny), sizeof(int));
	ifile.read(reinterpret_cast<char*>(&m_nz), sizeof(int));
	ifile.read(reinterpret_cast<char*>(&m_dx), sizeof(double));
	ifile.read(reinterpret_cast<char*>(&m_dy), sizeof(double));
	ifile.read(reinterpret_cast<char*>(&m_dz), sizeof(double));
	double xmax = m_dx * (m_nx - 1.);
	double ymax = m_dy * (m_ny - 1.);
	double zmax = m_dz * (m_nz - 1.);
	std::array<Point, 8> bb;
	m_bbox[0] = Point{ 0, 0, 0 };
	m_bbox[1] = Point{ xmax, 0, 0 };
	m_bbox[2] = Point{ 0, ymax, 0 };
	m_bbox[3] = Point{ xmax, ymax, 0 };
	m_bbox[4] = Point{ 0, 0, zmax };
	m_bbox[5] = Point{ xmax, 0, zmax };
	m_bbox[6] = Point{ 0, ymax, zmax };
	m_bbox[7] = Point{ xmax, ymax, zmax };

	size_t size_ = static_cast<size_t>(m_nx) * static_cast<size_t>(m_ny) * static_cast<size_t>(m_nz);
	m_scalars.resize(size_);
	//ushort* t_buff = new ushort[size_];
	float* t_buff = new float[size_];
	//ifile.read(reinterpret_cast<char*>(t_buff), size_ * sizeof(ushort));
	ifile.read(reinterpret_cast<char*>(t_buff), size_ * sizeof(float));
	ifile.close();
	int pos = 0;
	for (auto& v : m_scalars)
	{
		v = static_cast<double>(*(t_buff + pos++));
	}
	delete[] t_buff;
	// compute gradient for normals
	estimateGradient();
	flip_gradient();
}

void dmc::UniformGrid::init(const int nx, const int ny, const int nz)
{
	// set grid size
	m_nx = nx;
	m_ny = ny;
	m_nz = nz;
	// compute spacing
	m_dx = 1. / (static_cast<double>(m_nx) - 1.0);
	m_dy = 1. / (static_cast<double>(m_ny) - 1.0);
	m_dz = 1. / (static_cast<double>(m_nz) - 1.0);
	// total number of grid points
	size_t tot_size = m_nx * m_ny * m_nz;
	// initialize scalar fields
	m_scalars.resize(tot_size, 0);
	m_gradient.resize(tot_size);
	for (auto& p : m_gradient) {
		p[0] = 0;
		p[1] = 0;
		p[2] = 0;
	}
	// create bounding box in [0,1]x[0,1]x[0,1]
	// fastest index is x, then y and slowest index is z
	for (int i = 0; i <= 1; i++) {
		for (int j = 0; j <= 1; j++) {
			for (int k = 0; k <= 1; k++) {
				int index = i * 4 + j * 2 + k;
				//m_bbox[index] = Point{ { double(k), double(j), double(i) } };
				m_bbox[index] = { double(k), double(j), double(i) };
			}
		}
	}
}

void dmc::UniformGrid::init(const int nx, const int ny, const int nz, BBox & bb)
{
	// set grid size
	m_nx = nx;
	m_ny = ny;
	m_nz = nz;
	// total number of grid points
	size_t tot_size = static_cast<size_t>(m_nx * m_ny * m_nz);
	m_scalars.clear();
	m_scalars.resize(tot_size, 0);
	m_gradient.clear();
	m_gradient.resize(tot_size);
	for (auto& n : m_gradient) {
		n[0] = 0;
		n[1] = 0;
		n[2] = 0;
	}

	// create bounding box
	// fastest index is x, then y and slowest index is z
	for (int i = 0; i <= 1; i++) {
		for (int j = 0; j <= 1; j++) {
			for (int k = 0; k <= 1; k++) {
				int index = i * 4 + j * 2 + k;
				m_bbox[index] = bb[index];
			}
		}
	}
	// compute spacing
	double x_space = m_bbox[7][0] - m_bbox[0][0];
	double y_space = m_bbox[7][1] - m_bbox[0][1];
	double z_space = m_bbox[7][2] - m_bbox[0][2];
	m_dx = x_space / (static_cast<double>(m_nx) - 1.0);
	m_dy = y_space / (static_cast<double>(m_ny) - 1.0);
	m_dz = z_space / (static_cast<double>(m_nz) - 1.0);
}

void dmc::UniformGrid::init(const int nx, const int ny, const int nz, BBox & bb, const double val)
{
	// set grid size
	m_nx = nx;
	m_ny = ny;
	m_nz = nz;
	// total number of grid points
	size_t tot_size = m_nx * m_ny * m_nz;
	m_scalars.clear();
	m_scalars.resize(tot_size, val);
	m_gradient.clear();
	m_gradient.resize(tot_size);
	for (auto& n : m_gradient) {
		n[0] = 0;
		n[1] = 0;
		n[2] = 0;
	}

	// create bounding box
	// fastest index is x, then y and slowest index is z
	for (int i = 0; i <= 1; i++) {
		for (int j = 0; j <= 1; j++) {
			for (int k = 0; k <= 1; k++) {
				int index = i * 4 + j * 2 + k;
				m_bbox[index] = bb[index];
			}
		}
	}
	// compute spacing
	double x_space = m_bbox[7][0] - m_bbox[0][0];
	double y_space = m_bbox[7][1] - m_bbox[0][1];
	double z_space = m_bbox[7][2] - m_bbox[0][2];
	m_dx = x_space / (static_cast<double>(m_nx) - 1.0);
	m_dy = y_space / (static_cast<double>(m_ny) - 1.0);
	m_dz = z_space / (static_cast<double>(m_nz) - 1.0);
}


void dmc::UniformGrid::estimateGradient()
{
	auto index = [](const int i, const int max)
	{
		return (i < 0) ? 0 : (i >= max) ? max - i : i;
	};
	m_gradient.resize(m_scalars.size());
	const int nr = static_cast<int>(m_scalars.size());
#pragma omp parallel for
	for (int s = 0; s < nr; s++) {
		Index idx = local_index(s);
		const int i = idx[0];
		const int i0 = index(i - 1, m_nx);
		const int i1 = index(i + 1, m_nx);
		const int j = idx[1];
		const int j0 = index(j - 1, m_ny);
		const int j1 = index(j + 1, m_ny);
		const int k = idx[2];
		const int k0 = index(k - 1, m_nz);
		const int k1 = index(k + 1, m_nz);
		const double nx = (scalar(i1, j, k) - scalar(i0, j, k)) / (2*m_dx);
		const double ny = (scalar(i, j1, k) - scalar(i, j0, k)) / (2*m_dy);
		const double nz = (scalar(i, j, k1) - scalar(i, j, k0)) / (2*m_dz);

		m_gradient[s][0] = nx;
		m_gradient[s][1] = ny;
		m_gradient[s][2] = nz;
	}
}

void dmc::UniformGrid::flip_gradient()
{
	const double s = -1.;
	const int nr = static_cast<int>(m_gradient.size());
#pragma omp parallel for
	for (int i = 0; i < nr; i++)
	{
		m_gradient[i][0] *= s;
		m_gradient[i][1] *= s;
		m_gradient[i][2] *= s;
	}
}
