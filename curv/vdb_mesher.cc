// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <openvdb/openvdb.h>
#include <openvdb/tools/VolumeToMesh.h>
#include <omp.h>

#include "mesher.h"

using namespace curv::io;

struct VDB_Mesh : public Mesh
{
    openvdb::tools::VolumeToMesh mesher_;
    VDB_Mesh(double a, openvdb::FloatGrid::Ptr& grid)
    : mesher_(0.0, a)
    {
        mesher_(*grid);
    };
    virtual void each_triangle(std::function<void(const glm::ivec3& tri)> f)
    {
        for (unsigned i=0; i<mesher_.polygonPoolListSize(); ++i) {
            auto& pool = mesher_.polygonPoolList()[i];
            for (unsigned j=0; j<pool.numTriangles(); ++j) {
                auto& tri = pool.triangle(j);
                // swap ordering of nodes to get outside-normals
                f(glm::ivec3(tri[0], tri[2], tri[1]));
            }
        }
    }
    virtual void each_quad(std::function<void(const glm::ivec4& quad)> f)
    {
        for (unsigned i=0; i<mesher_.polygonPoolListSize(); ++i) {
            auto& pool = mesher_.polygonPoolList()[i];
            for (unsigned j=0; j<pool.numQuads(); ++j) {
                auto& q = pool.quad(j);
                // swap ordering of nodes to get outside-normals
                f(glm::ivec4{q[0], q[3], q[2], q[1]});
            }
        }
    }
    virtual void all_triangles(std::function<void(const glm::ivec3& tri)> f)
    {
        for (unsigned i=0; i<mesher_.polygonPoolListSize(); ++i) {
            auto& pool = mesher_.polygonPoolList()[i];
            for (unsigned j=0; j<pool.numTriangles(); ++j) {
                auto& tri = pool.triangle(j);
                // swap ordering of nodes to get outside-normals
                f(glm::ivec3(tri[0], tri[2], tri[1]));
            }
            for (unsigned j=0; j<pool.numQuads(); ++j) {
                auto& quad = pool.quad(j);
                // swap ordering of nodes to get outside-normals
                f(glm::ivec3(quad[0], quad[2], quad[1]));
                f(glm::ivec3(quad[0], quad[3], quad[2]));
            }
        }
    }
    virtual unsigned num_vertices()
    {
        return mesher_.pointListSize();
    }
    virtual glm::vec3 vertex(unsigned i)
    {
        auto pt = mesher_.pointList()[i];
        return glm::vec3{pt.x(), pt.y(), pt.z()};
    }
};

void vdb_mesher(
    const curv::Shape &shape,
    bool multithreaded,
    curv::io::Mesh_Export &opts,
    curv::At_Program &cx,
    curv::io::Mesh_Format format,
    std::ostream& out)
{
    Voxel_Config vox(shape.bbox_, opts.vsize_, cx);
    openvdb::initialize();

    // Create a FloatGrid and populate it with a signed distance field.
    Voxel_Timer vtimer(vox);

    // 2.0 is the background (or default) distance value for this
    // sparse array of voxels. Each voxel is a `float`.
    openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create(2.0);

    // Attach a scaling transform that sets the voxel size in world space.
    grid->setTransform(
        openvdb::math::Transform::createLinearTransform(vox.cellsize));

    // Identify the grid as a signed distance field.
    grid->setGridClass(openvdb::GRID_LEVEL_SET);

    // Populate the grid.
    // I assume each distance value is in the centre of a voxel.
    auto accessor = grid->getAccessor();
    if (multithreaded) {
        auto voxels = std::make_unique<float[]>(vox.nvoxels);
        #pragma omp parallel for
        for (int x = vox.range_min.x; x <= vox.range_max.x; ++x) {
            for (int y = vox.range_min.y; y <= vox.range_max.y; ++y) {
                for (int z = vox.range_min.z; z <= vox.range_max.z; ++z) {
                    int i = (x - vox.range_min.x) * vox.gridsize.y * vox.gridsize.z
                        + (y - vox.range_min.y) * vox.gridsize.z
                        + (z - vox.range_min.z);
                    voxels[i] = shape.dist(x*vox.cellsize, y*vox.cellsize, z*vox.cellsize, 0.0);
                }
            }
        }
        for (int x = vox.range_min.x; x <= vox.range_max.x; ++x) {
            for (int y = vox.range_min.y; y <= vox.range_max.y; ++y) {
                for (int z = vox.range_min.z; z <= vox.range_max.z; ++z) {
                    int i = (x - vox.range_min.x) * vox.gridsize.y * vox.gridsize.z
                        + (y - vox.range_min.y) * vox.gridsize.z
                        + (z - vox.range_min.z);
                    accessor.setValue(openvdb::Coord{x,y,z}, voxels[i]);
                }
            }
        }
    } else {
        for (int x = vox.range_min.x; x <= vox.range_max.x; ++x) {
            for (int y = vox.range_min.y; y <= vox.range_max.y; ++y) {
                for (int z = vox.range_min.z; z <= vox.range_max.z; ++z) {
                    accessor.setValue(openvdb::Coord{x,y,z},
                        shape.dist(x*vox.cellsize, y*vox.cellsize, z*vox.cellsize, 0.0));
                }
            }
        }
    }
    vtimer.print_stats();
    VDB_Mesh mesh(opts.adaptive_, grid);
    auto stats = write_mesh(format, mesh, shape, opts, out);
    print_mesh_stats(stats);
}
