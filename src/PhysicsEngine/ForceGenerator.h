
#ifndef FORCE_GENERATOR_H
#define FORCE_GENERATOR_H

#include <vector>
#include "Maths.h"
#include "PointMass.h"
#include "ForceGenerator.h"

union ForceUpdaterArgs
{
    // Holds the acceleration due to gravity.
    v3 gravity;

    struct
    {
        // Holds the velocity drag coefficiente.
        f32 k1;
        // Holds the velocity drag coefficiente.
        f32 k2;
    };
};

// Keeps track of one force generator and the particle it applies to.
struct PointMassForceRegistration
{
    PointMass *pointMass;
    ForceUpdaterArgs args;
    void (*forceUpdater)(PointMass*, f32, ForceUpdaterArgs);
};

typedef std::vector<PointMassForceRegistration> Registry;

#endif // FORCE_GENERATOR_H