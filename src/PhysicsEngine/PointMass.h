#ifndef POINT_MASS_H
#define POINT_MASS_H

#include "Maths.h"
#include "General.h"



// A point mass, is a way/idealization to abstract a body from its dimensions.
struct PointMass
{
    // Holds the linear position of thepoint mass in world space.
    v3 position;

    // Holds the linear velocity of the particle in world space.
    v3 velocity;

    /*
        Holds the acceleration of the point mass. 
        This value can be use to se acceleration due to gravity (its primary use)
        or any other constant acceleration.
    */
    v3 acceleration;

    /*
        Holds the amount of damping applied to linear motion.
        Damping is required to remove energy added 
        through numerical instability.
    */
    f32 damping;


    /*
        Holds the inver of th mass of the point mass. 
        It is more usefull to holde the inverse mass because
        integration is simpler and because in real-time simulation
        it is more useful to have objects with infinite mass (inmovable)
        than zero mass (completely unstable in numerical simulation).
        This conclusion born for prevent some physics calc based in newton laws
        e.g acceleration => a = 1/m * f, where f is forces and m is the mass
    */
    f32 inverseMass;


    /*
        Holds the accumulated force to be applied at the next
        simulation iteration only. This value is zeroed at each
        integration step.
    */
    v3 forceAccumlated;
};


inline void Integrate(PointMass &pointMass, f32 duration)
{
    ASSERT(duration > 0.0);

    /* 
    Update linear position 
        =>  position = p + v * t = position + (velocity * duration)
    */
    pointMass.position += pointMass.velocity * duration;

    /*
     Velocity Update 
        =>  Vu = v * d^t + a * t = velocity * (damping^duration) + acceleration * duration
     
     Acceleration
         => a = 1 / m * mg = inverseMass * force
    
     Velocity
        => v = p / t =  position / duration
    */
    {
        // Work out the acceleration from the force
        v3 resultingAcceleration =  pointMass.acceleration + (pointMass.forceAccumlated * pointMass.inverseMass);

        // Update linear velocity from the acceleration.
        pointMass.velocity += resultingAcceleration * duration;
    }

    // Impose drag
    pointMass.velocity *= powf(pointMass.damping, duration);
}

#endif