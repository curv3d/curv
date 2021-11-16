// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <iostream>
//#include <cmath>
//#include <cstdlib>
#include <chrono>
#include <thread>
#include <glm/geometric.hpp>

//#include <libcurv/io/compiled_shape.h>
#include <libcurv/io/mesh.h>
#include <libcurv/shape.h>
#include <libcurv/exception.h>
#include <libcurv/context.h>
//#include <libcurv/die.h>

struct Voxel_Config
{
    double cellsize;
    glm::ivec3 range_min, range_max;
    glm::ivec3 gridsize;
    int nvoxels;

    // Configure the voxel grid based on command line arguments.
    Voxel_Config(curv::BBox shape_bbox,
                 double vsize_opt,
                 const curv::Context& shape_cx)
    {
        glm::dvec3 shape_size = shape_bbox.max - shape_bbox.min;
        double volume = shape_size.x * shape_size.y * shape_size.z;
        if (std::isinf(volume)) {
            throw curv::Exception(shape_cx, "mesh export: shape is infinite");
        }

        if (vsize_opt > 0.0) {
            cellsize = vsize_opt;
        } else {
            cellsize = cbrt(volume / 100'000);
            if (cellsize < 0.1) cellsize = 0.1;
        }

        // This is the range of voxel coordinates.
        // For meshing to work, we need a thin band of voxels surrounding
        // the shape boundary. To provide a margin for error, I'll say
        // we need to populate voxels 2 units away from the surface.
        range_min = glm::ivec3(
            int(floor(shape_bbox.min.x/cellsize)) - 2,
            int(floor(shape_bbox.min.y/cellsize)) - 2,
            int(floor(shape_bbox.min.z/cellsize)) - 2);
        range_max = glm::ivec3(
            int(ceil(shape_bbox.max.x/cellsize)) + 2,
            int(ceil(shape_bbox.max.y/cellsize)) + 2,
            int(ceil(shape_bbox.max.z/cellsize)) + 2);

        gridsize = range_max - range_min + 1;
        nvoxels = gridsize.x * gridsize.y * gridsize.z;

        std::cerr
            << "vsize=" << cellsize << ": "
            << gridsize.x << "*"  << gridsize.y << "*" << gridsize.z
            << " voxels. Use '-O vsize=N' to change voxel size.\n";
        std::cerr.flush();
    }
};

struct Voxel_Timer
{
    const Voxel_Config& vox_;
    std::chrono::time_point<std::chrono::steady_clock> start_time_;

    Voxel_Timer(const Voxel_Config& vox) : vox_(vox)
    {
        start_time_ = std::chrono::steady_clock::now();
    }

    void print_stats()
    {
        std::chrono::time_point<std::chrono::steady_clock> end_time =
            std::chrono::steady_clock::now();
        std::chrono::duration<double> render_time = end_time - start_time_;
        std::cerr
            << "Rendered " << vox_.nvoxels
            << " voxels in " << render_time.count() << "s ("
            << int(vox_.nvoxels/render_time.count()) << " voxels/s).\n";
        std::cerr.flush();
    }
};

void print_mesh_stats(curv::io::Mesh_Stats& stats);

void libfive_mesher(
    const curv::Shape &shape,
    bool multithreaded,
    curv::io::Mesh_Export &opts,
    curv::At_Program &cx,
    curv::io::Mesh_Format format,
    std::ostream& out);

void vdb_mesher(
    const curv::Shape &shape,
    bool multithreaded,
    curv::io::Mesh_Export &opts,
    curv::At_Program &cx,
    curv::io::Mesh_Format format,
    std::ostream& out);

void tmc_mesher(
    const curv::Shape &shape,
    bool multithreaded,
    curv::io::Mesh_Export &opts,
    curv::At_Program &cx,
    curv::io::Mesh_Format format,
    std::ostream& out);
