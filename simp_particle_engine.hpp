//***************************************************************************//
//* File Name: simp_particle_engine.hpp                                     *//
//*    Author: Chris McBride chris_a_mcbride@hotmail.com                    *//
//* Date Made: 04/06/02                                                     *//
//* File Desc: Class declaration & implementation details for a base        *//
//*            particle engine                                              *//
//* Rev. Date: 04/16/02                                                     *//
//* Rev. Desc: Moved drawing & building code into base class, put in        *//
//*            billboarding, went to glDrawElements instead of glDrawArrays *//
//***************************************************************************//
#ifndef __SIMP_PARTICLE_ENGINE_HPP__
#define __SIMP_PARTICLE_ENGINE_HPP__

#include "simp_particle.hpp"

class cParticleEngine
{
    public:
        cParticleEngine();                        // Create and initialize particles for system
        virtual ~cParticleEngine()                // Free all memory for the system
        {
            return;
        }
        virtual void DrawParticles() = 0;         // Cycle through particle collection & draw
        virtual void UpdateParticlesSpurt();      // Cycle through particle collection & update
        virtual void UpdateParticlesContinuous(); // Cycle through particle collection & update
        virtual void CreateParticles();
                                                  // Build the particle from its position
        virtual void BuildParticle(float* vertexArray, int p, float* matrix);
        //virtual void BuildParticleColors(float* colorArray, int p);
        virtual void Go() {return;}
        virtual void Reset() {return;}

        int  GetNumLiveParticles() {return m_numLiveParticles;}
        void KillParticles() { m_numLiveParticles = 0;}
        void SetDeviation(float xDev, float yDev, float zDev);
        void SetDeviation(Vector& dev) {m_deviation = dev;}
        void SetGravity(Vector& grav) {gravity = grav;}
        void SetGravity(float xi, float yi, float zi);
        void SetLocation(Vector loc) {itsLocation = loc;}
        void SetLocation(float xi, float yi, float zi);
        void SetNumParticles(int num);
        void AllocateVertexArrays();
        void DeallocateVertexArrays();

        void ResetEngine();
    protected:
        int     numParticles;                     // Number of particles in system
        int     m_numLiveParticles;

        GLuint* pIndices;
        float*  pVertices;                        // Storage for particle vertices
        float*  pTexCoords;                       // Storage for particle texture coords
        //float*  pColors;              // Storage for particle color

        Vector m_deviation;                       // Allowed deviation from emitter position
        Vector gravity;                           // Force of the system on all particles
        Vector itsLocation;                       // Base location of the particles spawning
        cParticle **particles;                    // The collection of particles
};

cParticleEngine::cParticleEngine()
: numParticles(0),
m_numLiveParticles(0),
m_deviation(1, 1, 1),
pIndices(NULL),
pVertices(NULL),
pTexCoords(NULL),
particles(NULL)
{
    return;
}


// Allocate space for per-vertex particle info
void cParticleEngine::AllocateVertexArrays()
{
    pVertices  = new float[numParticles * 12];    // 3 components per vertex, 4 vertices per quad
    pTexCoords = new float[numParticles * 8];     // 2 tex coords per vertex, 4 vertices per quad
    pIndices   = new GLuint[numParticles * 4];    // 4 indices needed to draw 4 vertices from vertex array
    //pColors    = new float[numParticles*16];
    return;
}


// Deallocate all space for per-vertex particle info
void cParticleEngine::DeallocateVertexArrays()
{
    delete [] pIndices;
    delete [] pVertices;
    delete [] pTexCoords;
    //delete [] pColors;

    return;
}


// Create representation of all particles in the system
void cParticleEngine::CreateParticles()
{
    float mat[16] = {0};
    glGetFloatv(GL_MODELVIEW_MATRIX, mat);
    static int NextIndex = 0;
    m_numLiveParticles = numParticles;

    for(int k=0; k<numParticles; k++)
    {
        if(particles[k]->itsLife > 0.0f)
        {
            BuildParticle(pVertices+(k*12), k, mat);

            pIndices[NextIndex] = NextIndex;
            pIndices[NextIndex+1] = NextIndex+1;
            pIndices[NextIndex+2] = NextIndex+2;
            pIndices[NextIndex+3] = NextIndex+3;

            NextIndex+=4;
        }
        else
        {
            --m_numLiveParticles;
        }
    }

    NextIndex = 0;

    return;
}


// Use particles single position to build a quad
void cParticleEngine::BuildParticle(float* vertexArray, int p, float* mat)
{
    Vector right(mat[0], mat[4], mat[8]);
    Vector up(mat[1], mat[5], mat[9]);

    right.Normalize();
    up.Normalize();

    right *= particles[p]->itsSize;
    up    *= particles[p]->itsSize;

    // Bottom Left Corner
    Vector temp;
    temp = particles[p]->itsPosition + (-right - up);
    vertexArray[0] = temp.x;
    vertexArray[1] = temp.y;
    vertexArray[2] = temp.z;

    // Bottom Right Corner
    temp = particles[p]->itsPosition + (right - up);
    vertexArray[3] = temp.x;
    vertexArray[4] = temp.y;
    vertexArray[5] = temp.z;

    // Top Right Corner
    temp = particles[p]->itsPosition + (right + up);
    vertexArray[6] = temp.x;
    vertexArray[7] = temp.y;
    vertexArray[8] = temp.z;

    // Top Left Corner
    temp = particles[p]->itsPosition + (up - right);
    vertexArray[9]  = temp.x;
    vertexArray[10] = temp.y;
    vertexArray[11] = temp.z;

    return;
}


/*void cParticleEngine::BuildParticleColors(float* colorArray, int p)
{
  particles[p]->GetColors4f(colorArray);
  particles[p]->GetColors4f(colorArray+4);
  particles[p]->GetColors4f(colorArray+8);
  particles[p]->GetColors4f(colorArray+12);
}*/

void cParticleEngine::UpdateParticlesSpurt()     {return;}
void cParticleEngine::UpdateParticlesContinuous(){return;}
void cParticleEngine::SetNumParticles(int num)
{
    numParticles = m_numLiveParticles = num;
}


// Set the force acting upon the particles at the system level
void cParticleEngine::SetGravity(float xi, float yi, float zi)
{
    gravity.x = xi;
    gravity.x = yi;
    gravity.x = zi;

    return;
}


void cParticleEngine::SetLocation(float xi, float yi, float zi)
{
    itsLocation.x = xi;
    itsLocation.y = yi;
    itsLocation.z = zi;

    for(int x=0; x<numParticles; x++)
    {
        particles[x]->SetPosition(xi, yi, zi);
    }

    return;
}


void cParticleEngine::SetDeviation(float xDev, float yDev, float zDev)
{
    m_deviation.x = xDev;
    m_deviation.y = yDev;
    m_deviation.z = zDev;

    return;
}
#endif                                            // #define __SIMP_PARTICLE_ENGINE_HPP__
