#include "ForceGenerator.h"

void UpdateForces(const Registry &registrations, f32 duration)
{
    PointMassForceRegistration t;

    for (PointMassForceRegistration registration : registrations)
    {
        registration.forceUpdater(registration.pointMass, duration, registration.args);
    }
}

void UpdateGravityForce(PointMass *pointMass, f32 duration, ForceUpdaterArgs args)
{
    // check that we do not have infinite mass.
    if (!HasFiniteMass(pointMass))
        return;

    // Apply the mass-scaled force to the point mass.
    pointMass->forceAccumlated += args.gravity * GetMass(pointMass);
}
// ---------------------------------------------------

void UpdateDragForce(PointMass *pointMass, f32 duration, ForceUpdaterArgs args)
{
    /** drag force calc 
     * => fdrag = -Nv * (k1 * |Nv| + k2 * |Nv|^2)
     *    Nv = normalized velocity 
     */

    v3 force = pointMass->velocity;

    // Calculate the total drag coefficient.
    f32 dragCoefficient = V3Length(force);
    dragCoefficient = args.k1 * dragCoefficient + args.k2 * dragCoefficient * dragCoefficient;

    // Calculate the final force and apply it.
    V3Normalize(force);
    force *= -dragCoefficient;
    pointMass->forceAccumlated += force;
}