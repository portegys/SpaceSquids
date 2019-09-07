//***************************************************************************//
//* File Name: explosion.hpp                                                *//
//*    Author: Chris McBride chris_a_mcbride@hotmail.com                    *//
//* Date Made: 04/06/02                                                     *//
//* File Desc: Class declaration & implementation for particle system class *//
//*            used to simulate explosions                                  *//
//* Rev. Date: 04/21/02                                                     *//
//* Rev. Desc: random tweaks to update and constructors                     *//
//*                                                                         *//
//***************************************************************************//
#ifndef __EXPOSION_HPP__
#define __EXPOSION_HPP__

#include "simp_particle_engine.hpp"
#include "texture.hpp"

#define FRAND   (((float)rand()-(float)rand())/RAND_MAX)
#define  RAND   (((float)rand())/(float)RAND_MAX)

class cExplosion : public cParticleEngine
{
    public:
        cExplosion(int particle_count);
        cExplosion(int particle_count, Vector& location);
        cExplosion(int particle_count, float x, float y, float z);
        ~cExplosion();

        void Go();
        void Reset();

    private:
        void SetupExplosion();
        void UpdateParticlesSpurt();
        void UpdateParticlesContinuous();
        void SetRandomVelocity(float& oldVelocity);
        void SetVelocityWithRange(float& oldVelocity, float& refValue);
        void DrawParticles();

        float  m_particleSize;
        TextureImage texture;
        bool textureLoaded;
};

// Default explosion constructor.  Explosion at origin.
cExplosion::cExplosion(int particle_count)
{
    numParticles = m_numLiveParticles = particle_count;
    AllocateVertexArrays();
    SetupExplosion();
    return;
}


// Constructor that spawns particles from a given location
cExplosion::cExplosion(int particle_count, Vector& location)
{
    itsLocation  = location;
    numParticles = m_numLiveParticles = particle_count;
    AllocateVertexArrays();
    SetupExplosion();
    return;
}


// Another constructor that spawns particles from a given location
cExplosion::cExplosion(int particle_count, float x, float y, float z)
{
    Vector temp(x, y, z);
    itsLocation  = temp;
    numParticles = m_numLiveParticles = particle_count;
    AllocateVertexArrays();
    SetupExplosion();
    return;
}


void cExplosion::SetupExplosion()
{
    m_particleSize = 0.1f;

    cParticle* shard;
    particles = new cParticle*[numParticles];
    for(int i=0; i<numParticles; i++)
    {
        shard = new cParticle;

        SetRandomVelocity(shard->itsVelocity.x);
        SetRandomVelocity(shard->itsVelocity.y);
        SetRandomVelocity(shard->itsVelocity.z);

        shard->SetPosition(itsLocation);
        shard->itsSize = m_particleSize;
        shard->itsLife = RAND;
        shard->itsFade = 0.04f;
        particles[i] = shard;
    }

    // Load explosion texture.
    if (LoadTGA(&texture, "explosion.tga"))
    {
        textureLoaded = true;
    }
    else
    {
        textureLoaded = false;
    }

    // Set up texture coords
    float tTexCoords[8] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
    for(int x=0; x<numParticles*8; x+=8)
    {
        memcpy(pTexCoords+x, tTexCoords, sizeof(float) * 8);
    }

    return;
}


// Assign completely random velocity to particle
void cExplosion::SetRandomVelocity(float& oldVelocity)
{
    oldVelocity = FRAND * (rand()%1000 * .0005);
    return;
}


// Assign random velocity with some random offset from base velocity
void cExplosion::SetVelocityWithRange(float& oldVelocity, float& refValue)
{
    oldVelocity = FRAND + refValue;

    return;
}


// Update only live particles in the system
void cExplosion::UpdateParticlesSpurt()
{
    for(int i=0; i<numParticles; i++)
    {
        if(particles[i]->itsLife > 0.0f)
        {
            particles[i]->UpdatePosition(gravity);
            particles[i]->UpdateLife();
        }
    }

    return;
}


void cExplosion::UpdateParticlesContinuous()
{
    for(int i=0; i<numParticles; i++)
    {
        if(particles[i]->itsLife <= 0.0f)
        {
            particles[i]->SetPosition(itsLocation);
            particles[i]->itsLife = 1.0f;

            SetRandomVelocity(particles[i]->itsVelocity.x);
            SetRandomVelocity(particles[i]->itsVelocity.y);
            SetRandomVelocity(particles[i]->itsVelocity.z);
        }
        else
        {
            particles[i]->UpdatePosition(gravity);
            particles[i]->UpdateLife();
        }
    }

    return;
}


// Draw all particles in the system as necessary
void cExplosion::DrawParticles()
{
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_BLEND);

    // Use textured particles?
    if (textureLoaded)
    {
        glShadeModel(GL_FLAT);
        glEnable(GL_TEXTURE_2D);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
        glBindTexture(GL_TEXTURE_2D, texture.texID);
        glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
        glDisable(GL_LIGHTING);
    }
    else
    {
        glColor4f(1.0f, 0.2f, 0.0f, 0.7f);
    }

    glVertexPointer(3, GL_FLOAT, 0, pVertices);
    glTexCoordPointer(2, GL_FLOAT, 0, pTexCoords);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);

    glDrawElements(GL_QUADS, m_numLiveParticles*4, GL_UNSIGNED_INT, pIndices);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}


// Make the particle system update its members and draw itself
void cExplosion::Go()
{
    UpdateParticlesSpurt();
    CreateParticles();
    DrawParticles();
}


// Function to reuse allocated memory for new particles
void cExplosion::Reset()
{
    for(int i=0; i<numParticles; i++)
    {
        particles[i]->SetPosition(itsLocation);
        particles[i]->itsSize = m_particleSize;
        particles[i]->itsLife = RAND;
        particles[i]->itsFade = 0.04f;

        SetRandomVelocity(particles[i]->itsVelocity.x);
        SetRandomVelocity(particles[i]->itsVelocity.y);
        SetRandomVelocity(particles[i]->itsVelocity.z);
    }
    m_numLiveParticles = numParticles;

    return;
}


// The destructor
cExplosion::~cExplosion()
{
    if (particles != NULL)
    {
        for(int i=0; i<numParticles; i++)
        {
            delete particles[i];
        }
        delete particles;
        particles = NULL;
    }

    DeallocateVertexArrays();

    return;
}
#endif                                            // #ifndef __EXPOSION_HPP__
