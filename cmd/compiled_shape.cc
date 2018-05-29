// Copyright 2016-2018 Doug Moen
// Licensed under the Apache Licence, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

#include "compiled_shape.h"
#include "tempfile.h"
#include <cstdlib>
#include <fstream>
#include <libcurv/exception.h>
#include <libcurv/context.h>
#include <libcurv/function.h>
extern "C" {
#include <dlfcn.h>
}

Compiled_Shape::Compiled_Shape(curv::Shape_Recognizer& rshape)
{
    is_2d_ = rshape.is_2d_;
    is_3d_ = rshape.is_3d_;
    bbox_ = rshape.bbox_;

    // convert shape to C++ source code
    //auto cppname = make_tempfile(".cpp");
    auto cppname = tempfile_name(".cpp");
    std::ofstream cpp(cppname.c_str());
    shape_to_cpp(rshape, cpp);
    cpp.close();

    // compile C++ to optimized object code
    auto cc_cmd = curv::stringify("c++ -fpic -O3 -c ", cppname.c_str());
    //auto cc_cmd = curv::stringify("c++ -fpic -c -g ", cppname.c_str());
    if (system(cc_cmd->c_str()) != 0)
        throw curv::Exception({}, "c++ compile failed");

    // create shared object
    auto obj_name = tempfile_name(".o");
    auto so_name = tempfile_name(".so");
    auto link_cmd = curv::stringify("c++ -shared -o ", so_name.c_str(), " ", obj_name.c_str());
    if (system(link_cmd->c_str()) != 0)
        throw curv::Exception({}, "c++ link failed");

    // load shared object
    // TODO: so_name should contain a / character to prevent PATH search.
    // On macOS with a code-signed curv executable, so_name may need to be an
    // absolute pathname.
    void* dll = dlopen(so_name.c_str(), RTLD_NOW|RTLD_LOCAL);
    if (dll == nullptr)
        throw curv::Exception({},
            curv::stringify("can't load shared object: ", dlerror()));

    dlerror(); // Clear previous error.
    void* dist_p = dlsym(dll, "dist");
    const char* dist_err = dlerror();
    if (dist_err != nullptr)
        throw curv::Exception({},
            curv::stringify("can't load dist function: ",dist_err));
    assert(dist_p != nullptr);
    dist_ = reinterpret_cast<double (*)(double,double,double,double)>(dist_p);

    void* colour_p = dlsym(dll, "colour");
    const char* colour_err = dlerror();
    if (colour_err != nullptr)
        throw curv::Exception({},
            curv::stringify("can't load colour function: ",colour_err));
    assert(colour_p != nullptr);
    colour_ = (void (*)(double,double,double,double,glm::vec3*))colour_p;
}

void
shape_to_cpp(curv::Shape_Recognizer& shape, std::ostream& out)
{
    curv::GL_Compiler gl(out, curv::GL_Target::cpp);
    out <<
        "#include <glm/vec2.hpp>\n"
        "#include <glm/vec3.hpp>\n"
        "#include <glm/vec4.hpp>\n"
        "#include <glm/detail/func_common.hpp>\n"
        "#include <glm/detail/func_geometric.hpp>\n"
        "#include <glm/detail/func_trigonometric.hpp>\n"
        "\n"
        "using namespace glm;\n"
        "\n";

    curv::GL_Value dist_param = gl.newvalue(curv::GL_Type::Vec4);
    out <<
        "extern \"C\" double dist(double x, double y, double z, double t)\n"
        "{\n"
        "  vec4 "<<dist_param<<" = vec4(x,y,z,t);\n";
    curv::GL_Value dist_result = shape.gl_dist(dist_param, gl);
    out <<
        "  return " << dist_result << ";\n"
        "}\n";

    curv::GL_Value colour_param = gl.newvalue(curv::GL_Type::Vec4);
    out <<
        "\n"
        "extern \"C\" void colour(double x,double y,double z,double t,vec3* result)\n"
        "{\n"
        "  vec4 "<<colour_param<<" = vec4(x,y,z,t);\n";
    curv::GL_Value colour_result = shape.gl_colour(colour_param, gl);
    out <<
        "  *result = "<<colour_result<<";\n"
        "}\n";
}
