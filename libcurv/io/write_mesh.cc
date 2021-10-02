// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/io/mesh.h>
#include <libcurv/die.h>
#include <libcurv/function.h>
#include <glm/geometric.hpp>
#include "encode.h"
#include <iostream>
#include <iomanip>

namespace curv { namespace io {

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

void put_face_colour(std::ostream& out, const curv::Shape& shape,
    glm::vec3 v0, glm::vec3 v1, glm::vec3 v2)
{
    glm::vec3 centroid = (v0 + v1 + v2) / 3.0f;
    curv::Vec3 c = shape.colour(centroid.x, centroid.y, centroid.z, 0.0);
    c = linear_RGB_to_sRGB(c);
    out << " " << c.x << " " << c.y << " " << c.z;
}

void put_vertex_colour(std::ostream& out, const curv::Shape& shape, glm::vec3 v)
{
    curv::Vec3 c = shape.colour(v.x, v.y, v.z, 0.0);
    c = linear_RGB_to_sRGB(c);
    out << " " << c.x << " " << c.y << " " << c.z;
}

inline size_t point_byte_size(Mesh& mesh)
{
    return mesh.num_vertices() * 3 * 4; // xyz (float) 4 bytes each
}

// padded buffer length with a given stride
inline size_t pad_buffer(size_t s, size_t stride)
{
    return (s/stride + 1) * stride;
}

void put_buffer(std::ostream& out, unsigned vertex_byte_size, Mesh& mesh)
{
    const size_t vs = pad_buffer(vertex_byte_size, 4);
    const size_t bs = vs + point_byte_size(mesh);
    auto buff = std::make_unique<unsigned char []>(bs);
    int offset = 0;

    unsigned short *vbuff = (unsigned short*)buff.get();
    mesh.all_triangles([&](const glm::vec3& tri)->void {
        vbuff[offset] = tri[0];
        vbuff[offset + 1] = tri[1];
        vbuff[offset + 2] = tri[2];
        offset += 3;
    });

    float *pbuff = (float*)(buff.get() + vs);
    for (unsigned i = 0; i < mesh.num_vertices(); ++i) {
        auto pt = mesh.vertex(i);
        offset = i * 3;
        pbuff[offset] = pt.x;
        pbuff[offset + 1] = pt.y;
        pbuff[offset + 2] = pt.z;
    }

    auto str = base64_encode(buff.get(), bs);
    out << str;
}

Mesh_Stats write_mesh(
    Mesh_Format format, Mesh& mesh, const curv::Shape& shape,
    const Mesh_Export& opts, std::ostream& out)
{
    Mesh_Stats stats;
    stats.ntri = 0;
    stats.nquad = 0;
    switch (format) {
    case Mesh_Format::stl:
        out << "solid curv\n";
        mesh.all_triangles([&](const glm::ivec3& tri) -> void {
            put_triangle(out,
                mesh.vertex( tri[0] ),
                mesh.vertex( tri[1] ),
                mesh.vertex( tri[2] ));
            ++stats.ntri;
        });
        out << "endsolid curv\n";
        break;
    case Mesh_Format::obj:
        for (unsigned int i = 0; i < mesh.num_vertices(); ++i) {
            auto pt = mesh.vertex(i);
            out << "v " << pt.x << " " << pt.y << " " << pt.z << "\n";
        }
        mesh.each_triangle([&](const glm::ivec3& tri) -> void {
            out << "f " << tri[0]+1 << " "
                        << tri[1]+1 << " "
                        << tri[2]+1 << "\n";
            ++stats.ntri;
        });
        mesh.each_quad([&](const glm::ivec4& quad) -> void {
            out << "f " << quad[0]+1 << " "
                        << quad[1]+1 << " "
                        << quad[2]+1 << " "
                        << quad[3]+1 << "\n";
            ++stats.nquad;
        });
        break;
    case Mesh_Format::x3d:
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
        out << (opts.colouring_ == Mesh_Export::vertex_colour ? "true" : "false");
        out << "\" coordIndex=\"";
        bool first = true;
        mesh.all_triangles([&](const glm::vec3& tri) -> void {
            if (!first) out << " ";
            first = false;
            out << tri[0] << " " << tri[1] << " " << tri[2] << " -1";
            ++stats.ntri;
        });
        out <<
        "\">\n"
        "    <Coordinate point=\"";
        first = true;
        for (unsigned i = 0; i < mesh.num_vertices(); ++i) {
            if (!first) out << " ";
            first = false;
            auto pt = mesh.vertex(i);
            out << pt.x << " " << pt.y << " " << pt.z;
        }
        out <<
        "\"/>\n"
        "    <Color color=\"";
        switch (opts.colouring_) {
        case Mesh_Export::face_colour:
            mesh.all_triangles([&](const glm::vec3& tri)->void {
                put_face_colour(out, shape,
                    mesh.vertex(tri[0]),
                    mesh.vertex(tri[1]),
                    mesh.vertex(tri[2]));
            });
            break;
        case Mesh_Export::vertex_colour:
            for (unsigned int i = 0; i < mesh.num_vertices(); ++i) {
                put_vertex_colour(out, shape, mesh.vertex(i));
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
    case Mesh_Format::gltf:
      {
        mesh.all_triangles([&](const glm::vec3)->void {
            ++stats.ntri;
        });
        unsigned vertex_byte_size = stats.ntri*6; /*3 vertices, 2 bytes per index*/
        float minx, miny, minz, maxx, maxy, maxz;
        minx = miny = minz = std::numeric_limits<float>::max();
        maxx = maxy = maxz = std::numeric_limits<float>::min();
        for (unsigned i=0; i < mesh.num_vertices(); ++i) {
            auto pt = mesh.vertex(i);
            minx = std::min(pt.x, minx);
            miny = std::min(pt.y, miny);
            minz = std::min(pt.z, minz);
            maxx = std::max(pt.x, maxx);
            maxy = std::max(pt.y, maxy);
            maxz = std::max(pt.z, maxz);
        }
        out << std::setprecision(16) <<
        "{\n"
        "  \"asset\": { \"version\": \"2.0\" },\n"
        "  \"scenes\": [{ \"nodes\": [0] }],\n"
        "  \"nodes\": [{ \"mesh\": 0 }],\n"
        "  \"meshes\": [{ \"primitives\": [{ \"indices\": 0, \"attributes\": { \"POSITION\": 1 } }] }],\n"
        "  \"bufferViews\": [\n"
        "    {\n"
        "      \"buffer\": 0,\n"
        "      \"byteOffset\": 0,\n"
        "      \"byteLength\": " << vertex_byte_size << ",\n"
        "      \"target\": 34963\n" // ELEMENT_ARRAY_BUFFER
        "    },\n"
        "    {\n"
        "      \"buffer\": 0,\n"
        "      \"byteOffset\": " << pad_buffer(vertex_byte_size, 4) << ",\n"
        "      \"byteLength\": " << point_byte_size(mesh) << " ,\n"
        "      \"target\": 34962\n" // ARRAY_BUFFER
        "    }\n"
        "  ],\n"
        "  \"accessors\": [\n"
        "    {\n"
        "      \"bufferView\": 0,\n"
        "      \"byteOffset\": 0,\n"
        "      \"componentType\": 5123,\n" // UNSIGNED_SHORT
        "      \"count\": " << stats.ntri * 3 << ",\n"
        "      \"type\": \"SCALAR\",\n"
        "      \"max\": [" << mesh.num_vertices() - 1 << "],\n"
        "      \"min\": [0]\n"
        "    },\n"
        "    {\n"
        "      \"bufferView\": 1,\n"
        "      \"byteOffset\": 0,\n"
        "      \"componentType\": 5126,\n" // FLOAT
        "      \"count\": " << mesh.num_vertices() << ",\n"
        "      \"type\": \"VEC3\",\n"
        "      \"max\": [" << maxx << ", " << maxy << ", " << maxz << "],\n"
        "      \"min\": [" << minx << ", " << miny << ", " << minz << "]\n"
        "    }\n"
        "  ],\n"
        "  \"buffers\": [\n"
        "    {\n"
        "      \"byteLength\": " << pad_buffer(vertex_byte_size, 4) + point_byte_size(mesh) << ",\n"
        "      \"uri\": \"data:application/octet-stream;base64,";
        put_buffer(out, vertex_byte_size, mesh);
        out << "\"\n"
        "    }\n"
        "  ]\n"
        "}";
        break;
      }
    default:
        curv::die("bad mesh format");
    }
    return stats;
}

}} // namespace
