// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include "mesher.h"

using namespace curv::io;

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
