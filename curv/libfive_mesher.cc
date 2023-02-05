// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <glm/geometric.hpp>

#include <libfive/oracle/oracle_storage.hpp>
#include <libfive/oracle/oracle_clause.hpp>
#include <libfive/tree/tree.hpp>
#include <libfive/render/brep/mesh.hpp>
#include <libfive/render/brep/settings.hpp>
#include <libfive/render/brep/region.hpp>

#include "export.h"
#include "mesher.h"
#include <libcurv/io/mesh.h>
#include <libcurv/shape.h>
#include <libcurv/exception.h>
#include <libcurv/context.h>
#include <libcurv/die.h>

using namespace curv::io;

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

        out.push_back(
            Eigen::Vector3f(dx - centre, dy - centre, dz - centre)
            .normalized());
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

void libfive_mesher(
    const curv::Shape &shape,
    bool multithreaded,
    Mesh_Export &opts,
    curv::At_Program &cx,
    Mesh_Format format,
    std::ostream& out)
{
    Voxel_Config vox(shape.bbox_, opts.vsize_, opts.vcount_, cx);
    if (opts.eps_ == 0.0) opts.eps_ = vox.cellsize/10;
    libfive::Tree tree(std::unique_ptr<libfive::OracleClause>
        (new CurvOracleClause(shape, opts.eps_)));
    libfive::BRepSettings settings;
    if (multithreaded)
        settings.workers = std::thread::hardware_concurrency();
    else
        settings.workers = 1;
    settings.min_feature = vox.cellsize;
    switch (opts.mgen_) {
    case Mesh_Gen::sharp:
        settings.alg = libfive::DUAL_CONTOURING;
        break;
    case Mesh_Gen::iso:
        settings.alg = libfive::ISO_SIMPLEX;
        break;
    case Mesh_Gen::hybrid:
        settings.alg = libfive::HYBRID;
        break;
    default: break;
    }
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
}
