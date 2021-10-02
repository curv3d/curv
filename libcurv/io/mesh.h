// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_IO_MESH_H
#define LIBCURV_IO_MESH_H

#include <functional>
#include <ostream>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <libcurv/shape.h>

namespace curv { namespace io {

// Mesh file formats
enum class Mesh_Format {
    stl,
    obj,
    x3d,
    gltf
};

// Mesh file export parameters
struct Mesh_Export
{
    bool jit_ = false;
    double vsize_ = 0.0;
    double adaptive_ = 0.0;
    enum {face_colour, vertex_colour} colouring_ = face_colour;
};

// Abstract interface for accessing an in-memory quad/triangle mesh,
// because each meshing library has its own data structure.
// Used for exporting to a mesh file.
struct Mesh
{
    //virtual unsigned num_triangles() const = 0;
    //virtual unsigned num_quads() const = 0;
    virtual void each_triangle(std::function<void(const glm::ivec3& tri)>) = 0;
    virtual void each_quad(std::function<void(const glm::ivec4& quad)>) = 0;
    virtual void all_triangles(std::function<void(const glm::ivec3& tri)>) = 0;
    virtual unsigned num_vertices() = 0;
    virtual glm::vec3 vertex(unsigned) = 0;
};

struct Mesh_Stats
{
    unsigned ntri = 0;
    unsigned nquad = 0;
};

Mesh_Stats write_mesh(
    Mesh_Format, Mesh&, const Shape&, const Mesh_Export&, std::ostream& out);

}} // namespace
#endif // header guard
