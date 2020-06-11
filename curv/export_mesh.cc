// Copyright 2016-2020 Doug Moen
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
#include <libcurv/geom/compiled_shape.h>
#include <libcurv/shape.h>
#include <libcurv/exception.h>
#include <libcurv/context.h>
#include <libcurv/die.h>

using openvdb::Vec3s;
using openvdb::Vec3d;
using openvdb::Vec3i;

enum Mesh_Format {
    stl_format,
    obj_format,
    x3d_format
};

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
    export_mesh(stl_format, value, prog, params, ofile.ostream());
}

void export_obj(curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    curv::Output_File& ofile)
{
    ofile.open();
    export_mesh(obj_format, value, prog, params, ofile.ostream());
}

void export_x3d(curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    curv::Output_File& ofile)
{
    ofile.open();
    export_mesh(x3d_format, value, prog, params, ofile.ostream());
}

void put_triangle(std::ostream& out, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2)
{
    glm::vec3 n = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    out << "facet normal " << n.x << " " << n.y << " " << n.z << "\n"
        << " outer loop\n"
        << "  vertex " << v0.x << " " << v0.y << " " << v0.z << "\n"
        << "  vertex " << v1.x << " " << v1.y << " " << v1.z << "\n"
        << "  vertex " << v2.x << " " << v2.y << " " << v2.z << "\n"
        << " endloop\n"
        << "endfacet\n";
}

curv::Vec3 linear_RGB_to_sRGB(curv::Vec3 c)
{
    constexpr double k = 0.4545;
    return curv::Vec3{pow(c.x, k), pow(c.y, k), pow(c.z, k)};
}

void put_face_colour(std::ostream& out, curv::Shape& shape,
    Vec3s v0, Vec3s v1, Vec3s v2)
{
    Vec3s centroid = (v0 + v1 + v2) / 3.0;
    curv::Vec3 c = shape.colour(centroid.x(), centroid.y(), centroid.z(), 0.0);
    c = linear_RGB_to_sRGB(c);
    out << " " << c.x << " " << c.y << " " << c.z;
}

void put_vertex_colour(std::ostream& out, curv::Shape& shape, Vec3s v)
{
    curv::Vec3 c = shape.colour(v.x(), v.y(), v.z(), 0.0);
    c = linear_RGB_to_sRGB(c);
    out << " " << c.x << " " << c.y << " " << c.z;
}

inline glm::vec3 V3(Vec3s v)
{
    return glm::vec3{v.x(), v.y(), v.z()};
}

void describe_mesh_opts(std::ostream& out)
{
    out <<
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

void export_mesh(Mesh_Format format, curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    std::ostream& out)
{
    curv::Shape_Program shape(prog);
    curv::At_Program cx(prog);
    if (!shape.recognize(value, nullptr) || !shape.is_3d_)
        throw curv::Exception(cx, "mesh export: not a 3D shape");

    bool jit = false;
    double vsize = 0.0;
    double adaptive = 0.0;
    enum {face_colour, vertex_colour} colouring = face_colour;
    for (auto& i : params.map_) {
        Param p{params, i};
        if (p.name_ == "jit")
            jit = p.to_bool();
        else if (p.name_ == "vsize") {
            vsize = p.to_double();
            if (vsize <= 0.0) {
                throw curv::Exception(p, "'vsize' must be positive");
            }
        } else if (p.name_ == "adaptive") {
            adaptive = p.to_double(1.0);
            if (adaptive < 0.0 || adaptive > 1.0) {
                throw curv::Exception(p, "'adaptive' must be in range 0...1");
            }
        } else if (format == Mesh_Format::x3d_format && p.name_ == "colouring") {
            auto val = p.to_symbol();
            if (val == "face")
                colouring = face_colour;
            else if (val == "vertex")
                colouring = vertex_colour;
            else {
                throw curv::Exception(p, "'colouring' must be #face or #vertex");
            }
        } else
            p.unknown_parameter();
    }

    std::unique_ptr<curv::geom::Compiled_Shape> cshape = nullptr;
    if (jit) {
        //std::chrono::time_point<std::chrono::steady_clock> cstart_time, cend_time;
        auto cstart_time = std::chrono::steady_clock::now();
        cshape = std::make_unique<curv::geom::Compiled_Shape>(shape);
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
    if (vsize > 0.0) {
        voxelsize = vsize;
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

    std::cerr
        << "vsize="<<voxelsize<<": "
        << (voxelrange_max.x() - voxelrange_min.x() + 1) << "×"
        << (voxelrange_max.y() - voxelrange_min.y() + 1) << "×"
        << (voxelrange_max.z() - voxelrange_min.z() + 1)
        << " voxels. Use '-O vsize=N' to change voxel size.\n";
    std::cerr.flush();

    openvdb::initialize();

    // Create a FloatGrid and populate it with a signed distance field.
    std::chrono::time_point<std::chrono::steady_clock> start_time, end_time;
    start_time = std::chrono::steady_clock::now();

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
        // TODO: use multiple threads, since cshape->dist is thread safe.
        for (int x = voxelrange_min.x(); x <= voxelrange_max.x(); ++x) {
            for (int y = voxelrange_min.y(); y <= voxelrange_max.y(); ++y) {
                for (int z = voxelrange_min.z(); z <= voxelrange_max.z(); ++z) {
                    accessor.setValue(openvdb::Coord{x,y,z},
                        cshape->dist(x*voxelsize, y*voxelsize, z*voxelsize, 0.0));
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
    end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> render_time = end_time - start_time;
    int nvoxels =
        (voxelrange_max.x() - voxelrange_min.x() + 1) *
        (voxelrange_max.y() - voxelrange_min.y() + 1) *
        (voxelrange_max.z() - voxelrange_min.z() + 1);
    std::cerr
        << "Rendered " << nvoxels
        << " voxels in " << render_time.count() << "s ("
        << int(nvoxels/render_time.count()) << " voxels/s).\n";
    std::cerr.flush();

    // convert grid to a mesh
    openvdb::tools::VolumeToMesh mesher(0.0, adaptive);
    mesher(*grid);

    // output a mesh file
    int ntri = 0;
    int nquad = 0;
    switch (format) {
    case stl_format:
        out << "solid curv\n";
        for (unsigned int i=0; i<mesher.polygonPoolListSize(); ++i) {
            openvdb::tools::PolygonPool& pool = mesher.polygonPoolList()[i];
            for (unsigned int j=0; j<pool.numTriangles(); ++j) {
                // swap ordering of nodes to get outside-normals
                put_triangle(out,
                    V3(mesher.pointList()[ pool.triangle(j)[0] ]),
                    V3(mesher.pointList()[ pool.triangle(j)[2] ]),
                    V3(mesher.pointList()[ pool.triangle(j)[1] ]));
                ++ntri;
            }
            for (unsigned int j=0; j<pool.numQuads(); ++j) {
                // swap ordering of nodes to get outside-normals
                put_triangle(out,
                    V3(mesher.pointList()[ pool.quad(j)[0] ]),
                    V3(mesher.pointList()[ pool.quad(j)[2] ]),
                    V3(mesher.pointList()[ pool.quad(j)[1] ]));
                put_triangle(out,
                    V3(mesher.pointList()[ pool.quad(j)[0] ]),
                    V3(mesher.pointList()[ pool.quad(j)[3] ]),
                    V3(mesher.pointList()[ pool.quad(j)[2] ]));
                ntri += 2;
            }
        }
        out << "endsolid curv\n";
        break;
    case obj_format:
        for (unsigned int i = 0; i < mesher.pointListSize(); ++i) {
            auto& pt = mesher.pointList()[i];
            out << "v " << pt.x() << " " << pt.y() << " " << pt.z() << "\n";
        }
        for (unsigned int i=0; i<mesher.polygonPoolListSize(); ++i) {
            openvdb::tools::PolygonPool& pool = mesher.polygonPoolList()[i];
            for (unsigned int j=0; j<pool.numTriangles(); ++j) {
                // swap ordering of nodes to get outside-normals
                auto& tri = pool.triangle(j);
                out << "f " << tri[0]+1 << " "
                            << tri[2]+1 << " "
                            << tri[1]+1 << "\n";
                ++ntri;
            }
            for (unsigned int j=0; j<pool.numQuads(); ++j) {
                // swap ordering of nodes to get outside-normals
                auto& q = pool.quad(j);
                out << "f " << q[0]+1 << " "
                            << q[3]+1 << " "
                            << q[2]+1 << " "
                            << q[1]+1 << "\n";
                ++nquad;
            }
        }
        break;
    case x3d_format:
      {
        out <<
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE X3D PUBLIC \"ISO//Web3D//DTD X3D 3.1//EN\" \"http://www.web3d.org/specifications/x3d-3.1.dtd\">\n"
        "<X3D profile=\"Interchange\" version=\"3.1\" xsd:noNamespaceSchemaLocation=\"http://www.web3d.org/specifications/x3d-3.1.xsd\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
        " <head>\n"
        "  <meta content=\"Curv, https://github.com/doug-moen/curv\" name=\"generator\"/>\n"
        " </head>\n"
        " <Scene>\n"
        "  <Shape>\n"
        "   <IndexedFaceSet colorPerVertex=\"";
        out << (colouring == vertex_colour ? "true" : "false");
        out << "\" coordIndex=\"";
        bool first = true;
        for (unsigned int i=0; i<mesher.polygonPoolListSize(); ++i) {
            openvdb::tools::PolygonPool& pool = mesher.polygonPoolList()[i];
            for (unsigned int j=0; j<pool.numTriangles(); ++j) {
                if (!first) out << " ";
                first = false;
                auto& tri = pool.triangle(j);
                out << tri[0] << " " << tri[2] << " " << tri[1] << " -1";
                ++ntri;
            }
            for (unsigned int j=0; j<pool.numQuads(); ++j) {
                if (!first) out << " ";
                first = false;
                auto& q = pool.quad(j);
                out << q[0] << " " << q[2] << " " << q[1] << " -1 "
                    << q[0] << " " << q[3] << " " << q[2] << " -1";
                ntri += 2;
            }
        }
        out <<
        "\">\n"
        "    <Coordinate point=\"";
        first = true;
        for (unsigned int i = 0; i < mesher.pointListSize(); ++i) {
            if (!first) out << " ";
            first = false;
            auto& pt = mesher.pointList()[i];
            out << pt.x() << " " << pt.y() << " " << pt.z();
        }
        out <<
        "\"/>\n"
        "    <Color color=\"";
        switch (colouring) {
        case face_colour:
            for (unsigned int i=0; i<mesher.polygonPoolListSize(); ++i) {
                openvdb::tools::PolygonPool& pool = mesher.polygonPoolList()[i];
                for (unsigned int j=0; j<pool.numTriangles(); ++j) {
                    put_face_colour(out, shape,
                        mesher.pointList()[ pool.triangle(j)[0] ],
                        mesher.pointList()[ pool.triangle(j)[2] ],
                        mesher.pointList()[ pool.triangle(j)[1] ]);
                }
                for (unsigned int j=0; j<pool.numQuads(); ++j) {
                    put_face_colour(out, shape,
                        mesher.pointList()[ pool.quad(j)[0] ],
                        mesher.pointList()[ pool.quad(j)[2] ],
                        mesher.pointList()[ pool.quad(j)[1] ]);
                    put_face_colour(out, shape,
                        mesher.pointList()[ pool.quad(j)[0] ],
                        mesher.pointList()[ pool.quad(j)[3] ],
                        mesher.pointList()[ pool.quad(j)[2] ]);
                }
            }
            break;
        case vertex_colour:
            for (unsigned int i = 0; i < mesher.pointListSize(); ++i) {
                put_vertex_colour(out, shape, mesher.pointList()[i]);
            }
            break;
        }
        out <<
        "\"/>\n"
        "   </IndexedFaceSet>\n"
        "  </Shape>\n"
        " </Scene>\n"
        "</X3D>\n";
        break;
      }
    default:
        curv::die("bad mesh format");
    }

    if (ntri == 0 && nquad == 0) {
        std::cerr << "WARNING: no mesh was created (no volumes were found).\n"
          << "Maybe you should try a smaller voxel size.\n";
    } else {
        if (ntri > 0)
            std::cerr << ntri << " triangles";
        if (ntri > 0 && nquad > 0)
            std::cerr << ", ";
        if (nquad > 0)
            std::cerr << nquad << " quads";
        std::cerr << ".\n";
    }
}
