// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include "version.h"
#include <libcurv/version.h>
extern "C" {
#include <sys/utsname.h>
#include <stdlib.h>
}
#include <gl/opengl.h>

#define USE_X11 0
#if USE_X11
#include <X11/Xlib.h>
#include <GL/glx.h>
#endif

void
print_gpu(std::ostream& out)
{
    // GPU information: see `glxinfo` on Linux, but I don't want hundreds of
    // lines of gibberish. I want: GPU model, driver version, OpenGL version.
    // If X11, then 'direct rendering':' from glxinfo.

    static std::ostream* gout;

    out.flush(); // In case of crash while querying GPU.

    // This is a nice sanity test of Curv's ability to access the GPU.
    // If this doesn't work, there's no need to debug anything deep in the
    // Viewer code.

    gout = &out;
    glfwSetErrorCallback([](int err, const char* msg)->void {
        *gout << "GPU: "<<msg<<"\n";
    });
    if(!glfwInit()) {
        return;
    }

    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    GLFWwindow* window = glfwCreateWindow(100, 100, "version", NULL, NULL);
    if (window == nullptr) {
        return;
    }
    glfwMakeContextCurrent(window);

    const char* vendor = (const char*) glGetString(GL_VENDOR);
    if (vendor == nullptr) vendor = "???";
    const char* renderer = (const char*) glGetString(GL_RENDERER);
    if (renderer == nullptr) renderer = "???";
    const char* version = (const char*) glGetString(GL_VERSION);
    if (version == nullptr) version = "???";

    out << "GPU: " << vendor << ", " << renderer << "\n";
    out << "OpenGL: " << version << "\n";
    out.flush();

#if USE_X11
    char* display_name = getenv("DISPLAY");
    if (display_name != nullptr) {
        // X11 specific information, from glxinfo
        // To get correct information, I need to use the same display and ctx
        // that GLFW is using. This doesn't work.
        Display* display = XOpenDisplay(display_name);
        if (display == nullptr)
            return;
        GLXContext ctx = 0;
        out << "Direct Rendering: ";
        if (glXIsDirect(display, ctx)) {
            out << "Yes\n";
        }
        else if (getenv("LIBGL_ALWAYS_INDIRECT")) {
            out << "No (LIBGL_ALWAYS_INDIRECT set)\n";
        }
        else {
            out << "No (If you want to find out why, try setting "
                   "LIBGL_DEBUG=verbose)\n";
        }
        XCloseDisplay(display);
    }
#endif

    // In principle, this should free resources and leave no storage leaks.
    glfwDestroyWindow(window);
    glfwTerminate();
}

void
print_os(std::ostream& out)
{
    // TODO: print "OS" distro name and version.
    // On Linux (os.sysname=="Linux"), use /etc/os-release,
    //   see <https://www.freedesktop.org/software/systemd/man/os-release.html>
    // On macOS (os.sysname=="Darwin"), use
    //   /System/Library/CoreServices/SystemVersion.plist

    struct utsname os;
    uname(&os);
    out << "Kernel: " << os.sysname << " " << os.release
        << " " << os.machine << "\n";
}

void
print_cpu(std::ostream&)
{
    // CPU information: see `lscpu` on Linux.

    // TODO: cpu -- see `lscpu` on Linux. name, model, # of cores
    // TODO: vm -- see `lscpu` on Linux
    // TODO: The amount of RAM.
    // see https://github.com/dylanaraps/neofetch
}

void
print_compiler(std::ostream& out)
{
    #if defined __clang_version__
        out << "Compiler: clang " << __clang_version__ << "\n";
    #elif defined __GNUC__
        out << "Compiler: gcc " << __VERSION__ << "\n";
    #else
        (void) out;
    #endif
}

// Print version information useful for bug reports.
// Inspiration: openscad --info,
// and neofetch, which displays terse and readable system information:
//    see <https://github.com/dylanaraps/neofetch>
void
print_version(std::ostream& out)
{
    out << "Curv: " << CURV_VERSION << "\n";
    print_os(out);
    print_cpu(out);
    print_compiler(out);
    print_gpu(out); // Last info printed, due to crashiness.
}
