// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <glm/geometric.hpp>
#include <omp.h>

#include <openvdb/openvdb.h>
#include <openvdb/tools/VolumeToMesh.h>
#include <libfive/oracle/oracle_storage.hpp>
#include <libfive/oracle/oracle_clause.hpp>
#include <libfive/tree/tree.hpp>
#include <libfive/render/brep/mesh.hpp>
#include <libfive/render/brep/settings.hpp>
#include <libfive/render/brep/region.hpp>

#include "export.h"
#include <libcurv/io/compiled_shape.h>
#include <libcurv/io/mesh.h>
#include <libcurv/shape.h>
#include <libcurv/exception.h>
#include <libcurv/context.h>
#include <libcurv/die.h>

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

void describe_mesh_opts(std::ostream& out)
{
    out <<
    "-O mgen=#vdb|#five : Mesh generation algorithm (default #vdb).\n"
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

struct CurvOracle : public libfive::OracleStorage<LIBFIVE_EVAL_ARRAY_SIZE>
{
    const curv::Shape& shape_;
    double epsilon_;
    CurvOracle(const curv::Shape& sh, double e) : shape_(sh), epsilon_(e) {}

    void evalInterval(libfive::Interval& out) override
    {
        // Since Curv currently doesn't do interval arithmetic,
        // return a large interval. Which magic number do I choose?
        //
        // mkeeter: If the top-level tree is just this CurvOracle, then
        // returning [-inf, inf] should be fine; however, if you're
        // transforming it further, then I agree that the math could get iffy.
        // You could also return [NaN, NaN], which should cause interval
        // subdivision to always subdivide (down to individual voxels).

        // This gives rounded edges for a large cube (`cube 9999999`).
        //out = {-10000.0, 10000.0};

        // This gives the same results (rounded edges) for a large cube.
        // I'm using infinity because it is "the least magic" alternative
        // and is not overly scale dependent like 10000 is.
        out = { -std::numeric_limits<double>::infinity(),
                 std::numeric_limits<double>::infinity() };

      #if 0
        // This gives the same results (rounded edges) for a large cube.
        out = { std::numeric_limits<double>::quiet_NaN(),
                std::numeric_limits<double>::quiet_NaN() };
      #endif
    }
    void evalPoint(float& out, size_t index=0) override
    {
        const auto pt = points.col(index);
        out = shape_.dist(pt.x(), pt.y(), pt.z(), 0.0);
    }
    void checkAmbiguous(Eigen::Block<
        Eigen::Array<bool, 1, LIBFIVE_EVAL_ARRAY_SIZE>,
        1, Eigen::Dynamic>) override
    {
        // Nothing to do here, because we can only find one derivative
        // per point. Points on sharp features may not be handled correctly.
    }
    void evalFeatures(boost::container::small_vector<libfive::Feature, 4>& out)
    override {
        // Find one derivative with partial differences.

        // TODO: scale-independent epsilon?
        float centre, dx, dy, dz;
        Eigen::Vector3f before = points.col(0);
        evalPoint(centre);

        points.col(0) = before + Eigen::Vector3f(epsilon_, 0.0, 0.0);
        evalPoint(dx);

        points.col(0) = before + Eigen::Vector3f(0.0, epsilon_, 0.0);
        evalPoint(dy);

        points.col(0) = before + Eigen::Vector3f(0.0, 0.0, epsilon_);
        evalPoint(dz);

        points.col(0) = before;

        // TODO: divide by magnitude (normalize the vector)
        out.push_back(Eigen::Vector3f(
            (dx - centre) / epsilon_,
            (dy - centre) / epsilon_,
            (dz - centre) / epsilon_));
    }
};
struct CurvOracleClause : public libfive::OracleClause
{
    const curv::Shape& shape_;
    double epsilon_;
    CurvOracleClause(const curv::Shape& sh, double e) : shape_(sh), epsilon_(e)
    {}
    std::unique_ptr<libfive::Oracle> getOracle() const override
    {
        return std::unique_ptr<libfive::Oracle>
            (new CurvOracle(shape_, epsilon_));
    }
    std::string name() const override { return "CurvOracleClause"; }
};

struct Libfive_Mesh : public Mesh
{
    std::unique_ptr<libfive::Mesh> mesh_;
    Libfive_Mesh(std::unique_ptr<libfive::Mesh> m) : mesh_(std::move(m)) {}
    virtual void each_triangle(std::function<void(const glm::ivec3& tri)> f)
    {
        for (const auto& tri : mesh_->branes)
            f(glm::ivec3(tri[0], tri[1], tri[2]));
    }
    virtual void each_quad(std::function<void(const glm::ivec4& quad)> f)
    {
    }
    virtual void all_triangles(std::function<void(const glm::ivec3& tri)> f)
    {
        for (const auto& tri : mesh_->branes)
            f(glm::ivec3(tri[0], tri[1], tri[2]));
    }
    virtual unsigned num_vertices()
    {
        return mesh_->verts.size();
    }
    virtual glm::vec3 vertex(unsigned i)
    {
        auto pt = mesh_->verts[i];
        return glm::vec3{pt.x(), pt.y(), pt.z()};
    }
};

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
            else if (val == "five")
                opts.mgen_ = Mesh_Gen::five;
            else
                throw curv::Exception(p, "'mgen' must be #vdb|#five");
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

    switch (opts.mgen_) {
    case Mesh_Gen::vdb:
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
        if (cshape != nullptr) {
            auto voxels = std::make_unique<float[]>(vox.nvoxels);
            #pragma omp parallel for // uses multiple threads, since cshape->dist is thread safe.
            for (int x = vox.range_min.x; x <= vox.range_max.x; ++x) {
                for (int y = vox.range_min.y; y <= vox.range_max.y; ++y) {
                    for (int z = vox.range_min.z; z <= vox.range_max.z; ++z) {
                        int i = (x - vox.range_min.x) * vox.gridsize.y * vox.gridsize.z
                            + (y - vox.range_min.y) * vox.gridsize.z
                            + (z - vox.range_min.z);
                        voxels[i] = cshape->dist(x*vox.cellsize, y*vox.cellsize, z*vox.cellsize, 0.0);
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
        break;
      }
    case Mesh_Gen::five:
      {
        Voxel_Config vox(shape.bbox_, opts.vsize_, cx);
        const curv::Shape* sh;
        if (cshape) sh = &*cshape; else sh = &shape;
        libfive::Tree tree(std::unique_ptr<libfive::OracleClause>
            (new CurvOracleClause(*sh, vox.cellsize/10)));
        libfive::BRepSettings settings;
        settings.workers = 1; /* crash if workers != 1 */
        settings.min_feature = vox.cellsize;
        libfive::Region<3> region
            ({shape.bbox_.min.x - vox.cellsize,
              shape.bbox_.min.y - vox.cellsize,
              shape.bbox_.min.z - vox.cellsize},
             {shape.bbox_.max.x + vox.cellsize,
              shape.bbox_.max.y + vox.cellsize,
              shape.bbox_.max.z + vox.cellsize});

        auto start_time = std::chrono::steady_clock::now();
        Libfive_Mesh mesh(libfive::Mesh::render(tree, region, settings));
        auto end_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> render_time = end_time - start_time;
        std::cerr
            << "Rendered " << mesh.mesh_->branes.size()
            << " triangles in " << render_time.count() << "s\n";

        (void) write_mesh(format, mesh, shape, opts, out);
        break;
      }
    default:
        throw curv::Exception(cx, "mesh export: unknown mesh generator");
    }
}
