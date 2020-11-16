#include "ForceRegistry.h"

ForceRegistry::ForceRegistry()
{

}

ForceRegistry::~ForceRegistry()
{

}

void ForceRegistry::Add(PointMass *pointMass, IForceGenerator *forceGenerator)
{
    const PointMassForceRegistration registration = {pointMass, forceGenerator};
    registrations.push_back(registration);
}


void ForceRegistry::Clear()
{
    registrations.clear();
}   

void ForceRegistry::UpdateForces(f32 duration)
{
    for (PointMassForceRegistration &registry : registrations)
    {
        registry.forceGenerato->UpdateForce(registry.pointMass, duration);
    }
}

