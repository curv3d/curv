// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <iostream>
#include <cmath>
#include <openvdb/openvdb.h>
#include <openvdb/tools/VolumeToMesh.h>

#include "export.h"
#include <curv/shape.h>
#include <curv/exception.h>

using openvdb::Vec3s;
using openvdb::Vec3d;
using openvdb::Vec3i;

void put_triangle(std::ostream& out, Vec3s v0, Vec3s v1, Vec3s v2)
{
    out << "facet normal 0 0 0\n"
        << " outer loop\n"
        << "  vertex " << v0.x() << " " << v0.y() << " " << v0.z() << "\n"
        << "  vertex " << v1.x() << " " << v1.y() << " " << v1.z() << "\n"
        << "  vertex " << v2.x() << " " << v2.y() << " " << v2.z() << "\n"
        << " endloop\n"
        << "endfacet\n";
}

void export_stl(curv::Value value,
    curv::System& sys, const curv::Context& cx, const Export_Params& params,
    std::ostream& out)
{
    curv::Shape_Recognizer shape(cx, sys);
    if (!shape.recognize(value) && !shape.is_3d_)
        throw curv::Exception(cx, "not a 3D shape");

#if 0
    for (auto p : params) {
        std::cerr << p.first << "=" << p.second << "\n";
    }
#endif

    Vec3d size(
        shape.bbox_.xmax - shape.bbox_.xmin,
        shape.bbox_.ymax - shape.bbox_.ymin,
        shape.bbox_.zmax - shape.bbox_.zmin);
    double maxspan = fmax(size.x(), fmax(size.y(), size.z()));
    double voxelsize = maxspan / 20.0;

    auto res_p = params.find("res");
    if (res_p != params.end()) {
        double res = strtod(res_p->second.c_str(), (char**)nullptr);
        if (res <= 0.0 || res != res) {
            throw curv::Exception(cx, curv::stringify(
                "STL export: invalid parameter res=",res_p->second));
        }
        voxelsize = res;
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
        << (voxelrange_max.x() - voxelrange_min.x() + 1) << "×"
        << (voxelrange_max.y() - voxelrange_min.y() + 1) << "×"
        << (voxelrange_max.z() - voxelrange_min.z() + 1) << " ";
    std::cerr.flush();

    openvdb::initialize();

    // Create a FloatGrid and populate it with a signed distance field.

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
    for (int x = voxelrange_min.x(); x <= voxelrange_max.x(); ++x) {
        for (int y = voxelrange_min.y(); y <= voxelrange_max.y(); ++y) {
            for (int z = voxelrange_min.z(); z <= voxelrange_max.z(); ++z) {
                accessor.setValue(openvdb::Coord{x,y,z},
                    shape.dist(x*voxelsize, y*voxelsize, z*voxelsize, 0.0));
            }
        }
    }
    std::cerr << "voxels, ";
    std::cerr.flush();

    // convert grid to a mesh
    openvdb::tools::VolumeToMesh mesher;
    mesher(*grid);

    std::cerr << mesher.pointListSize() << " vertices\n";
    std::cerr.flush();

    // output an STL file
    out << "solid curv\n";
    for (int i=0; i<mesher.polygonPoolListSize(); ++i) {
        openvdb::tools::PolygonPool& pool = mesher.polygonPoolList()[i];
        for (int j=0; j<pool.numTriangles(); ++j) {
            // swap ordering of nodes to get outside-normals
            put_triangle(out,
                mesher.pointList()[ pool.triangle(j)[0] ],
                mesher.pointList()[ pool.triangle(j)[2] ],
                mesher.pointList()[ pool.triangle(j)[1] ]);
        }
        for (int j=0; j<pool.numQuads(); ++j) {
            // swap ordering of nodes to get outside-normals
            put_triangle(out,
                mesher.pointList()[ pool.quad(j)[0] ],
                mesher.pointList()[ pool.quad(j)[2] ],
                mesher.pointList()[ pool.quad(j)[1] ]);
            put_triangle(out,
                mesher.pointList()[ pool.quad(j)[0] ],
                mesher.pointList()[ pool.quad(j)[3] ],
                mesher.pointList()[ pool.quad(j)[2] ]);
        }
    }
    out << "endsolid curv\n";
}
