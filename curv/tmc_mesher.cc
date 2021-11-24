// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <extern/dmc/UniformGrid.h>
#include <extern/dmc/DualMarchingCubes.h>
#include <omp.h>

#include "mesher.h"

using namespace curv::io;

struct TMC_Mesh : public Mesh
{
    std::vector<dmc::UniformGrid::Vertex> v_;
    std::vector<dmc::UniformGrid::Normal> n_;
    std::vector<int> tri_; 
    std::vector<int> quad_; 
    TMC_Mesh(bool simplify, dmc::UniformGrid& grid)
    {
        dmc::DualMarchingCubes dmc;
        dmc.dualMC(0.0, grid, v_, n_, tri_, quad_, simplify);
    };
    virtual void each_triangle(std::function<void(const glm::ivec3& tri)> f)
    {
    }
    virtual void each_quad(std::function<void(const glm::ivec4& quad)> f)
    {
        int nquad = quad_.size() / 4;
        for (int i = 0; i < nquad; ++i) {
            f(glm::ivec4{
                quad_[4*i+0],
                quad_[4*i+1],
                quad_[4*i+2],
                quad_[4*i+3]});
        }
    }
    virtual void all_triangles(std::function<void(const glm::ivec3& tri)> f)
    {
        int ntri = tri_.size() / 3;
        for (int i = 0; i < ntri; ++i) {
            f(glm::ivec3{tri_[4*i+0], tri_[4*i+1], tri_[4*i+2]});
        }
    }
    virtual unsigned num_vertices()
    {
        return v_.size();
    }
    virtual glm::vec3 vertex(unsigned i)
    {
        auto& pt = v_[i];
        return glm::vec3{pt[0], pt[1], pt[2]};
    }
};

void tmc_mesher(
    const curv::Shape &shape,
    bool multithreaded,
    curv::io::Mesh_Export &opts,
    curv::At_Program &cx,
    curv::io::Mesh_Format format,
    std::ostream& out)
{
    Voxel_Config vox(shape.bbox_, opts.vsize_, opts.vcount_, cx);
    Voxel_Timer vtimer(vox);
    dmc::UniformGrid grid;
    const curv::BBox& b = shape.bbox_;
    dmc::UniformGrid::BBox bb = {
        dmc::Vector{ b.min.x, b.min.y, b.min.z },
        dmc::Vector{ b.max.x, b.min.y, b.min.z },
        dmc::Vector{ b.min.x, b.max.y, b.min.z },
        dmc::Vector{ b.max.x, b.max.y, b.min.z },
        dmc::Vector{ b.min.x, b.min.y, b.max.z },
        dmc::Vector{ b.max.x, b.min.y, b.max.z },
        dmc::Vector{ b.min.x, b.max.y, b.max.z },
        dmc::Vector{ b.max.x, b.max.y, b.max.z },
    };
    grid.init(vox.gridsize.x, vox.gridsize.y, vox.gridsize.z, bb);
    if (multithreaded) {
        #pragma omp parallel for
        for (int x = vox.range_min.x; x <= vox.range_max.x; ++x) {
            for (int y = vox.range_min.y; y <= vox.range_max.y; ++y) {
                for (int z = vox.range_min.z; z <= vox.range_max.z; ++z) {
                    grid.scalar(
                        x - vox.range_min.x,
                        y - vox.range_min.y,
                        z - vox.range_min.z,
                        shape.dist(
                            x*vox.cellsize,
                            y*vox.cellsize,
                            z*vox.cellsize,
                            0.0));
                }
            }
        }
    } else {
        for (int x = vox.range_min.x; x <= vox.range_max.x; ++x) {
            for (int y = vox.range_min.y; y <= vox.range_max.y; ++y) {
                for (int z = vox.range_min.z; z <= vox.range_max.z; ++z) {
                    grid.scalar(
                        x - vox.range_min.x,
                        y - vox.range_min.y,
                        z - vox.range_min.z,
                        shape.dist(
                            x*vox.cellsize,
                            y*vox.cellsize,
                            z*vox.cellsize,
                            0.0));
                }
            }
        }
    }
    grid.estimateGradient();
    grid.flip_gradient();
    vtimer.print_stats();

    TMC_Mesh mesh(true, grid);
    auto tnow = std::chrono::steady_clock::now();
    std::chrono::duration<double> gen_time = tnow - vtimer.end_time_;
    unsigned nquads = mesh.quad_.size() / 4;
    std::cerr
        << "Generated " << nquads
        << " quads in " << gen_time.count() << "s ("
        << int(nquads/gen_time.count()) << " quads/s).\n";
    std::cerr.flush();

    auto stats = write_mesh(format, mesh, shape, opts, out);
    //print_mesh_stats(stats);
}
