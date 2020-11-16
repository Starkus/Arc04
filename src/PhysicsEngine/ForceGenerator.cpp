#include "ForceGenerator.h"

GravityForce::GravityForce(const v3 &gravity)
{
    this->gravity = gravity;
}

GravityForce::~GravityForce()
{
}

void GravityForce::UpdateForce(PointMass &pointMass, f32 duration)
{
    // check that we do not have infinite mass.
    if (HasFiniteMass(pointMass)) return;

    // Apply the mass-scaled force to the point mass.
    pointMass.forceAccumlated += gravity * GetMass(pointMass);
}
// ---------------------------------------------------

DragForce::DragForce(const f32 k1, const f32 k2)
{
    this->k1 = k1;
    this->k1 = k2;
}

DragForce::~DragForce()
{
}

void DragForce::UpdateForce(PointMass &pointMass, f32 duration)
{
    /** drag force calc 
     * => fdrag = -Nv * (k1 * |Nv| + k2 * |Nv|^2)
     *    Nv = normalized velocity 
     */

    v3 force = pointMass.velocity;

    // Calculate the total drag coefficient.
    f32 dragCoefficient = V3Length(force);
    dragCoefficient = k1 * dragCoefficient + k2 * dragCoefficient * dragCoefficient;

    // Calculate the final force and apply it.
    V3Normalize(force);
    force *= -dragCoefficient;
    pointMass.forceAccumlated += force;
}