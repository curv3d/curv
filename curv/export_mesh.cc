// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <openvdb/openvdb.h>
#include <openvdb/tools/VolumeToMesh.h>
#include <glm/geometric.hpp>

#include "export.h"
#include <libcurv/io/compiled_shape.h>
#include <libcurv/io/mesh.h>
#include <libcurv/shape.h>
#include <libcurv/exception.h>
#include <libcurv/context.h>
#include <libcurv/die.h>
#include <omp.h>

using openvdb::Vec3s;
using openvdb::Vec3d;
using openvdb::Vec3i;
using namespace curv::io;

void export_mesh(Mesh_Format, curv::Value value,
    curv::Program&,
    const Export_Params& params,
    std::ostream& out);

void export_stl(curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    curv::Output_File& ofile)
{
    ofile.open();
    export_mesh(Mesh_Format::stl, value, prog, params, ofile.ostream());
}

void export_obj(curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    curv::Output_File& ofile)
{
    ofile.open();
    export_mesh(Mesh_Format::obj, value, prog, params, ofile.ostream());
}

void export_x3d(curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    curv::Output_File& ofile)
{
    ofile.open();
    export_mesh(Mesh_Format::x3d, value, prog, params, ofile.ostream());
}

void export_gltf(curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    curv::Output_File& ofile)
{
    ofile.open();
    export_mesh(Mesh_Format::gltf, value, prog, params, ofile.ostream());
}

inline glm::vec3 V3(Vec3s v)
{
    return glm::vec3{v.x(), v.y(), v.z()};
}

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
        return V3(mesher_.pointList()[i]);
    }
};

void describe_mesh_opts(std::ostream& out)
{
    out <<
    "-O mgen=#vdb|#dce : Mesh generation algorithm (default #vdb).\n"
    "-O jit : Fast evaluation using JIT compiler (uses C++ compiler).\n"
    "-O vsize=<voxel size>\n"
    "-O adaptive=<0...1> : Deprecated. Use meshlab to simplify mesh.\n"
    ;
}
void describe_colour_mesh_opts(std::ostream& out)
{
    describe_mesh_opts(out);
    out <<
    "-O colouring=#face|#vertex (default #face)\n"
    ;
}

struct Voxel_Timer
{
    const Vec3i& voxelrange_min_;
    const Vec3i& voxelrange_max_;
    std::chrono::time_point<std::chrono::steady_clock> start_time_;

    Voxel_Timer(const Vec3i& vmin, const Vec3i& vmax)
    : voxelrange_min_(vmin), voxelrange_max_(vmax)
    {
        start_time_ = std::chrono::steady_clock::now();
    }

    void print_stats()
    {
        std::chrono::time_point<std::chrono::steady_clock> end_time =
            std::chrono::steady_clock::now();
        std::chrono::duration<double> render_time = end_time - start_time_;
        int nvoxels =
            (voxelrange_max_.x() - voxelrange_min_.x() + 1) *
            (voxelrange_max_.y() - voxelrange_min_.y() + 1) *
            (voxelrange_max_.z() - voxelrange_min_.z() + 1);
        std::cerr
            << "Rendered " << nvoxels
            << " voxels in " << render_time.count() << "s ("
            << int(nvoxels/render_time.count()) << " voxels/s).\n";
        std::cerr.flush();
    }
};

void print_mesh_stats(Mesh_Stats& stats)
{
    if (stats.ntri == 0 && stats.nquad == 0) {
        std::cerr << "WARNING: no mesh was created (no volumes were found).\n"
          << "Maybe you should try a smaller voxel size.\n";
    } else {
        if (stats.ntri > 0)
            std::cerr << stats.ntri << " triangles";
        if (stats.ntri > 0 && stats.nquad > 0)
            std::cerr << ", ";
        if (stats.nquad > 0)
            std::cerr << stats.nquad << " quads";
        std::cerr << ".\n";
    }
}

void export_mesh(Mesh_Format format, curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    std::ostream& out)
{
    curv::Shape_Program shape(prog);
    curv::At_Program cx(prog);
    if (!shape.recognize(value, nullptr) || !shape.is_3d_)
        throw curv::Exception(cx, "mesh export: not a 3D shape");

    Mesh_Export opts;
    for (auto& i : params.map_) {
        Param p{params, i};
        if (p.name_ == "mgen") {
            auto val = p.to_symbol();
            if (val == "vdb")
                opts.mgen_ = Mesh_Gen::vdb;
            else if (val == "dce")
                opts.mgen_ = Mesh_Gen::dce;
            else
                throw curv::Exception(p, "'mgen' must be #vdb or #dce");
        } else if (p.name_ == "jit") {
            opts.jit_ = p.to_bool();
        } else if (p.name_ == "vsize") {
            opts.vsize_ = p.to_double();
            if (opts.vsize_ <= 0.0) {
                throw curv::Exception(p, "'vsize' must be positive");
            }
        } else if (p.name_ == "adaptive") {
            opts.adaptive_ = p.to_double(1.0);
            if (opts.adaptive_ < 0.0 || opts.adaptive_ > 1.0) {
                throw curv::Exception(p, "'adaptive' must be in range 0...1");
            }
        } else if (format == Mesh_Format::x3d && p.name_ == "colouring") {
            auto val = p.to_symbol();
            if (val == "face")
                opts.colouring_ = Mesh_Export::face_colour;
            else if (val == "vertex")
                opts.colouring_ = Mesh_Export::vertex_colour;
            else {
                throw curv::Exception(p, "'colouring' must be #face or #vertex");
            }
        } else
            p.unknown_parameter();
    }

    std::unique_ptr<curv::io::Compiled_Shape> cshape = nullptr;
    if (opts.jit_) {
        //std::chrono::time_point<std::chrono::steady_clock> cstart_time, cend_time;
        auto cstart_time = std::chrono::steady_clock::now();
        cshape = std::make_unique<curv::io::Compiled_Shape>(shape);
        auto cend_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> compile_time = cend_time - cstart_time;
        std::cerr
            << "Compiled shape in " << compile_time.count() << "s\n";
        std::cerr.flush();
    } else {
        std::cerr <<
            "You are in SLOW MODE. Use '-O jit' to speed up rendering.\n";
    }

    Vec3d size(
        shape.bbox_.xmax - shape.bbox_.xmin,
        shape.bbox_.ymax - shape.bbox_.ymin,
        shape.bbox_.zmax - shape.bbox_.zmin);
    double volume = size.x() * size.y() * size.z();
    double infinity = 1.0/0.0;
    if (volume == infinity || volume == -infinity) {
        throw curv::Exception(cx, "mesh export: shape is infinite");
    }

    double voxelsize;
    if (opts.vsize_ > 0.0) {
        voxelsize = opts.vsize_;
    } else {
        voxelsize = cbrt(volume / 100'000);
        if (voxelsize < 0.1) voxelsize = 0.1;
    }

    // This is the range of voxel coordinates.
    // For meshing to work, we need to specify at least a thin band of voxels
    // surrounding the sphere boundary, both inside and outside. To provide a
    // margin for error, I'll say that we need to populate voxels 2 units away
    // from the surface.
    Vec3i voxelrange_min(
        int(floor(shape.bbox_.xmin/voxelsize)) - 2,
        int(floor(shape.bbox_.ymin/voxelsize)) - 2,
        int(floor(shape.bbox_.zmin/voxelsize)) - 2);
    Vec3i voxelrange_max(
        int(ceil(shape.bbox_.xmax/voxelsize)) + 2,
        int(ceil(shape.bbox_.ymax/voxelsize)) + 2,
        int(ceil(shape.bbox_.zmax/voxelsize)) + 2);

    int vx = voxelrange_max.x() - voxelrange_min.x() + 1;
    int vy = voxelrange_max.y() - voxelrange_min.y() + 1;
    int vz = voxelrange_max.z() - voxelrange_min.z() + 1;

    std::cerr
        << "vsize=" << voxelsize << ": "
        << vx << "*"  << vy << "*" << vz
        << " voxels. Use '-O vsize=N' to change voxel size.\n";
    std::cerr.flush();

    switch (opts.mgen_) {
    case Mesh_Gen::vdb:
      {
        openvdb::initialize();

        // Create a FloatGrid and populate it with a signed distance field.
        Voxel_Timer vtimer(voxelrange_min, voxelrange_max);

        // 2.0 is the background (or default) distance value for this
        // sparse array of voxels. Each voxel is a `float`.
        openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create(2.0);

        // Attach a scaling transform that sets the voxel size in world space.
        grid->setTransform(
            openvdb::math::Transform::createLinearTransform(voxelsize));

        // Identify the grid as a signed distance field.
        grid->setGridClass(openvdb::GRID_LEVEL_SET);

        // Populate the grid.
        // I assume each distance value is in the centre of a voxel.
        auto accessor = grid->getAccessor();
        if (cshape != nullptr) {
            auto voxels = std::make_unique<float[]>(vx * vy *vz);
            #pragma omp parallel for // uses multiple threads, since cshape->dist is thread safe.
            for (int x = voxelrange_min.x(); x <= voxelrange_max.x(); ++x) {
                for (int y = voxelrange_min.y(); y <= voxelrange_max.y(); ++y) {
                    for (int z = voxelrange_min.z(); z <= voxelrange_max.z(); ++z) {
                        int i = (x - voxelrange_min.x()) * vy * vz
                            + (y - voxelrange_min.y()) * vz
                            + (z - voxelrange_min.z());
                        voxels[i] = cshape->dist(x*voxelsize, y*voxelsize, z*voxelsize, 0.0);
                    }
                }
            }
            for (int x = voxelrange_min.x(); x <= voxelrange_max.x(); ++x) {
                for (int y = voxelrange_min.y(); y <= voxelrange_max.y(); ++y) {
                    for (int z = voxelrange_min.z(); z <= voxelrange_max.z(); ++z) {
                        int i = (x - voxelrange_min.x()) * vy * vz
                            + (y - voxelrange_min.y()) * vz
                            + (z - voxelrange_min.z());
                        accessor.setValue(openvdb::Coord{x,y,z}, voxels[i]);
                    }
                }
            }
        } else {
            for (int x = voxelrange_min.x(); x <= voxelrange_max.x(); ++x) {
                for (int y = voxelrange_min.y(); y <= voxelrange_max.y(); ++y) {
                    for (int z = voxelrange_min.z(); z <= voxelrange_max.z(); ++z) {
                        accessor.setValue(openvdb::Coord{x,y,z},
                            shape.dist(x*voxelsize, y*voxelsize, z*voxelsize, 0.0));
                    }
                }
            }
        }
        vtimer.print_stats();
        VDB_Mesh mesh(opts.adaptive_, grid);
        auto stats = write_mesh(format, mesh, shape, opts, out);
        print_mesh_stats(stats);
        break;
      }
    case Mesh_Gen::dce:
      {
        throw curv::Exception(cx, "mesh generator #dce not implemented");
      }
    default:
        throw curv::Exception(cx, "mesh export: unknown mesh generator");
    }
}
