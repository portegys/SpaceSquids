//***************************************************************************//
//* File Name: simp_particle.hpp                                            *//
//*    Author: Chris McBride chris_a_mcbride@hotmail.com                    *//
//* Date Made: 04/06/02                                                     *//
//* File Desc: Class declaration & implementation details for a base        *//
//*            particle class for use in any particle system                *//
//* Rev. Date: xx/xx/02                                                     *//
//* Rev. Desc:                                                              *//
//*                                                                         *//
//***************************************************************************//
#ifndef __SIMP_PARTICLE_HPP__
#define __SIMP_PARTICLE_HPP__

#include "math_etc.h"

class cParticle
{
    public:
        void GetColors3f(float* colorArray) const;
        void SetColors3f(const float* colorArray);

        void GetColors4f(float* colorArray) const;
        void SetColors4f(const float* colorArray);

        void GetPosition(float posArray[3]);
        Vector& GetPosition() {return itsPosition;}

        void SetVelocity(Vector& vel) {itsVelocity = vel;}
        void SetVelocity(float x, float y, float z);

        void SetPosition(Vector& pos) {itsPosition = pos;}
        void SetPosition(float x, float y, float z);

        void UpdatePosition(Vector& grav);
        void UpdateVelocity(Vector& vel);
        void UpdateLife();

    protected:                                    // allow only derived classes direct access to protected members
        cParticle();
        virtual ~cParticle() {return;};

        Vector itsPosition;                       // Position of the particle
        Vector itsVelocity;                       // Initial x, y, and z movement
        float   r, g, b, a;                       // Color of the particle
        float   itsSize;                          // Size of the particle (may change over time)
        float   itsWeight;                        // Weight of particle (may change as size changes)
        float   itsLife;                          // Time for the particle to be 'alive'
        float   itsFade;                          // How fast the particle dies

        friend class cParticleEngine;
        friend class cExplosion;
        friend class cSnowStorm;
        friend class cSpray;
        friend class cLaser;
};

// Default particle constructor.
cParticle::cParticle()
{
    itsVelocity.x = 0.0;
    itsVelocity.y = 0.0;
    itsVelocity.z = 0.0;

    itsPosition.x = 0;
    itsPosition.y = 0;
    itsPosition.z = 0;

    itsSize   = 0.5;
    itsWeight = 1.0f;
    itsLife   = 1.0f;
    itsFade   = 0.01f;

    //r = rand()%1000 * 0.001;
    //g = rand()%1000 * 0.001;
    //b = rand()%1000 * 0.001;
    a = itsLife;

    return;
}


// Update the particles attributes
void cParticle::UpdatePosition(Vector& grav)
{
    itsVelocity += itsWeight * grav;
    itsPosition += itsVelocity;
    return;
}


void cParticle::UpdateLife()
{
    itsLife -= itsFade;                           // Should I be using alpha for both?
    a       -= itsFade;                           // Probably not.. if life fades, some
    // particles will not need blending
    return;
}


void cParticle::UpdateVelocity(Vector& vel)
{
    itsVelocity += vel;
    return;
}


// Return the position of the particle into an array
// (use for glVertex3fv calls)
inline void cParticle::GetPosition(float posArray[3])
{
    posArray[0] = itsPosition.x;
    posArray[1] = itsPosition.y;
    posArray[2] = itsPosition.z;

    return;
}


// Set the particles velocity by component
inline void cParticle::SetVelocity(float x, float y, float z)
{
    itsVelocity.x = x;
    itsVelocity.y = y;
    itsVelocity.z = z;

    return;
}


// Set the particles position by component
inline void cParticle::SetPosition(float x, float y, float z)
{
    itsPosition.x = x;
    itsPosition.y = y;
    itsPosition.z = z;

    return;
}


// Return an rgb color for the particle
void cParticle::GetColors3f (float* colorArray) const
{
    colorArray[0] = r;
    colorArray[1] = g;
    colorArray[2] = b;

    return;
}


// Set the particles rgb color
void cParticle::SetColors3f (const float* colorArray)
{
    r = colorArray[0];
    g = colorArray[1];
    b = colorArray[2];

    return;
}


// Return an rgba color for the particle
void cParticle::GetColors4f(float* colorArray) const
{
    colorArray[0] = r;
    colorArray[1] = g;
    colorArray[2] = b;
    colorArray[3] = a;

    return;
}


// Set the rgba color for the particle
void cParticle::SetColors4f (const float* colorArray)
{
    r = colorArray[0];
    g = colorArray[1];
    b = colorArray[2];
    a = colorArray[3];

    return;
}
#endif                                            // #ifndef __SIMP_PARTICLE_HPP__
