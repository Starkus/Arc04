
#ifndef FORCE_GENERATOR_H
#define FORCE_GENERATOR_H

#include "PointMass.h"

class IForceGenerator
{

public:
    /*
        Overload this in implementations of the interfaces to calculate
        and update the force applied to the given particle.
    */
    virtual void UpdateForce(PointMass &pointMass, f32 duration) = 0;
};

class GravityForce: public IForceGenerator
{

public:
    // Creates the generator with the given acceleration.
    GravityForce(const v3 &gravity);
    ~GravityForce();

    // Applies the gravitational force to the given particle.
    void UpdateForce(PointMass &pointMass, f32 duration) override;

private:
    
    // Holds the acceleration due to gravity.
    v3 gravity;
};

/** A force generator that applie a drag force.
 * One instance can be used for multiple point mass
 */
class DragForce: public IForceGenerator
{
public:
    DragForce(const f32 k1,const f32 k2);
    ~DragForce();

    // Applies the drag force to the given point mass.
    void UpdateForce(PointMass &pointMass, f32 duration) override;

private:
    // Holds the velocity drag coefficiente.
    f32 k1;

    // Holds the velocity squared drag coefficient.
    f32 k2;
};






#endif // FORCE_GENERATOR_H