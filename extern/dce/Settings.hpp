#ifndef DCE_SETTINGS_HPP
#define DCE_SETTINGS_HPP

#include <dce/Math.hpp>

/* Global settings for easy access */

namespace dce {

//------------------------------------------------------------------------------
/* Field generation: */


// Side of the cubic input field
const unsigned  FieldSize       =  128; // 512 -> 4 GB memory (using double)

/* If true, will subtract a sphere from the scene, else add it.
 * Subtracting it produces very acute angles which produces intersecting triangles */
const bool      SubtractSphere  =  true;

/* Ensure a closed mesh by giving all field periphery a positive distance. */
const bool      ClosedField     = true;

/* Emulate noisy input by perturbing the input field.
 * These are the standard deviation of the normal distribution of the jitter.
 * Dual contouring is VERY bad at handling noisy input,
 * even if we clamp and increate CenterPush. */
const bool  PerturbField  = false; // Main switch
const float DistJitter    = 0.05f;
const float NormalJitter  = 0.05f;


//------------------------------------------------------------------------------
/* Contouring: */

/*
 If true, will restrict the vertex to the voxel.
 Clamping is both necessary and sufficent to ensure no intersecting triangles.
 Clamping will, however, not always produce as good results.
 */
const bool Clamp = false;

/* A corner with an absolute distance greater than this will disregarded from vertex generation.
 * A distance larger than 1 should never occur with a perfect distance field as input.
 * Using this curoff we can produce much better results even for less-than-perfect inputs.
 * An infinite cutoff (none) can produce many "far away" vertices.
 */
const real MaxCornerDist  =  1.0;

/* When is a vertex "far" from the voxel center?
 * Far pixels are always clamped */
const real FarAway        =  2.5;

/* We add a weak push towards the center of the voxel.
   This controls how weak.
 */
const real CenterPush     = 0.01; // The actual push is the square of this

} // namespace
#endif
