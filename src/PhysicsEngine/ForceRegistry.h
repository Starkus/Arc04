
#ifndef FORCE_REGISTRY_H
#define FORCE_REGISTRY_H

#include <vector>
#include "PointMass.h"
#include "ForceGenerator.h"

// Keeps track of one force generator and the particle it applies to.
struct PointMassForceRegistration
{
    PointMass *pointMass;
    IForceGenerator *forceGenerato; 
};

typedef std::vector<PointMassForceRegistration> Registry;

class ForceRegistry
{
public:
    ForceRegistry();
    ~ForceRegistry();

    // Registers the given force generator to apply to the  give particle.
    void Add(PointMass *pointMass, IForceGenerator *forceGenerator);

    /**
     * Clears all registrations from the registery. This will
     * not delete the point mass  or the force generators
     * themshelves, just the records of their connection.
     */
    void Clear();

    /**
     * Calls all the force generators to update the forces of
     * their corresponding particles.
     */
    void UpdateForces(f32 duration);

protected:
    // Holds the list of registration.
    Registry registrations;

};




#endif //  FORCE_REGISTRY_H