//----------------------------------------------------------------------------------------------------//
/*
    Collision physics.
    Taken from:

    PHYSICS FOR GAME DEVELOPERS

    CHAPTER 16 EXAMPLE PROGRAM

    NAME:		Crash Test
    PURPOSE:	To demonstrate multi-rigid body simulation in 3D
    BY:			David Bourg
DATE:		07/20/01
COPYRIGHT:	Copyright 2000-2001 by David Bourg
*/
//----------------------------------------------------------------------------------------------------//
#ifndef UNIX
#include <windows.h>
#endif
#include "physics.h"
#include <iostream>
#include <memory.h>
#include <assert.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "glaux.h"

//------------------------------------------------------------------------//
// Global variables
//------------------------------------------------------------------------//

RigidBody   Bodies[MAX_BODIES + 1];
int         NumBodies = 0;
Collision   Collisions[(MAX_BODIES + 1) * 8];
int         NumCollisions = 0;

//------------------------------------------------------------------------//
// This function initializes the global data.
//------------------------------------------------------------------------//
void    InitializePhysics()
{
    for (int i = 0; i < MAX_BODIES + 1; i++) Bodies[i].valid = false;
}


//------------------------------------------------------------------------//
// This function sets the initial state of an object
//------------------------------------------------------------------------//
void    InitializeObject(int i, float size, int type, int group)
{
    if (i >= MAX_BODIES) return;
    if (i >= NumBodies) NumBodies = i + 1;

    InitializeObject(&Bodies[i], size, type, group);
}


void    InitializeObject(RigidBody *body, float size, int type, int group)
{
    float   iRoll, iPitch, iYaw;
    float   Ixx, Iyy, Izz;

    // This is a valid body.
    body->valid = true;

    // Set initial velocity
    body->vVelocity.x = 0.0f;
    body->vVelocity.y = 0.0f;
    body->vVelocity.z = 0.0f;
    body->fSpeed = 0.0f;

    // Set initial angular velocity
    body->vAngularVelocity.x = 0.0f;
    body->vAngularVelocity.y = 0.0f;
    body->vAngularVelocity.z = 0.0f;

    body->vAngularAcceleration.x = 0.0f;
    body->vAngularAcceleration.y = 0.0f;
    body->vAngularAcceleration.z = 0.0f;

    body->vAcceleration.x = 0.0f;
    body->vAcceleration.y = 0.0f;
    body->vAcceleration.z = 0.0f;

    // Set the initial thrust, forces and moments
    body->vForces.x = 0.0f;
    body->vForces.y = 0.0f;
    body->vForces.z = 0.0f;

    body->vMoments.x = 0.0f;
    body->vMoments.y = 0.0f;
    body->vMoments.z = 0.0f;

    // Zero the velocity in body space coordinates
    body->vVelocityBody.x = 0.0f;
    body->vVelocityBody.y = 0.0f;
    body->vVelocityBody.z = 0.0f;

    // Set initial position
    body->vPosition.x = 0.0;
    body->vPosition.y = 0.0;
    body->vPosition.z = 0.0;

    // Set the initial orientation
    iRoll = 0.0f;
    iPitch = 0.0f;
    iYaw = 0.0f;
    body->qOrientation = MakeQFromEulerAngles(iRoll, iPitch, iYaw);

    // Set the mass properties
    body->fMass = (size * size * size)/(-g);

    Ixx = Iyy = Izz = body->fMass/12.0f * (size*size + size*size);

    body->mInertia.e11 = Ixx;   body->mInertia.e12 = 0; body->mInertia.e13 = 0;
    body->mInertia.e21 = 0;     body->mInertia.e22 = Iyy;   body->mInertia.e23 = 0;
    body->mInertia.e31 = 0;     body->mInertia.e32 = 0; body->mInertia.e33 = Izz;

    body->mInertiaInverse = body->mInertia.Inverse();

                                                  // for bounding sphere check
    body->fRadius = size * (float)(sqrt(3.0) / 2.0);

    // bounding verteces relative to CG (assumed centered)
    body->vVertexList[0].x = size/2.0f;
    body->vVertexList[0].y = size/2.0f;
    body->vVertexList[0].z = -size/2.0f;

    body->vVertexList[1].x = size/2.0f;
    body->vVertexList[1].y = size/2.0f;
    body->vVertexList[1].z = size/2.0f;

    body->vVertexList[2].x = size/2.0f;
    body->vVertexList[2].y = -size/2.0f;
    body->vVertexList[2].z = size/2.0f;

    body->vVertexList[3].x = size/2.0f;
    body->vVertexList[3].y = -size/2.0f;
    body->vVertexList[3].z = -size/2.0f;

    body->vVertexList[4].x = -size/2.0f;
    body->vVertexList[4].y = size/2.0f;
    body->vVertexList[4].z = -size/2.0f;

    body->vVertexList[5].x = -size/2.0f;
    body->vVertexList[5].y = size/2.0f;
    body->vVertexList[5].z = size/2.0f;

    body->vVertexList[6].x = -size/2.0f;
    body->vVertexList[6].y = -size/2.0f;
    body->vVertexList[6].z = size/2.0f;

    body->vVertexList[7].x = -size/2.0f;
    body->vVertexList[7].y = -size/2.0f;
    body->vVertexList[7].z = -size/2.0f;

    body->type = type;
    body->group = group;
    body->exempt = -1;
}


//------------------------------------------------------------------------//
// This function clears all of the forces and moments.
//------------------------------------------------------------------------//
void    ClearObjectForces(void)
{
    Vector  Fb, Mb;
    int     i;

    for(i=0; i<NumBodies; i++)
    {
        if (!Bodies[i].valid) continue;

        // reset forces and moments:
        Bodies[i].vForces.x = 0.0f;
        Bodies[i].vForces.y = 0.0f;
        Bodies[i].vForces.z = 0.0f;

        Bodies[i].vMoments.x = 0.0f;
        Bodies[i].vMoments.y = 0.0f;
        Bodies[i].vMoments.z = 0.0f;

        Fb.x = 0.0f;    Mb.x = 0.0f;
        Fb.y = 0.0f;    Mb.y = 0.0f;
        Fb.z = 0.0f;    Mb.z = 0.0f;

        // Convert forces from model space to earth space
        Bodies[i].vForces = QVRotate(Bodies[i].qOrientation, Fb);

        // Save the moments
        Bodies[i].vMoments += Mb;

        Bodies[i].vAcceleration = Bodies[i].vForces / Bodies[i].fMass;
        Bodies[i].vAngularAcceleration = Bodies[i].mInertiaInverse *
            (Bodies[i].vMoments -
            (Bodies[i].vAngularVelocity^
            (Bodies[i].mInertia * Bodies[i].vAngularVelocity)));
    }
}


//------------------------------------------------------------------------//
//  Using Euler's method
//------------------------------------------------------------------------//
void    StepSimulation(float dtime)
{

    Vector Ae;
    int     i,j;
    float   dt = dtime;

    // Clear all of the forces and moments.
    ClearObjectForces();

    // Integrate
    for(i=0; i<NumBodies; i++)
    {
        if (!Bodies[i].valid) continue;
        Bodies[i].collision = false;

        // calculate the acceleration of the object in earth space:
        Ae = Bodies[i].vForces / Bodies[i].fMass;
        Bodies[i].vAcceleration = Ae;

        // calculate the velocity of the object in earth space:
        Bodies[i].vVelocity += Ae * dt;
        if (Bodies[i].type == XWING_BLOCK_TYPE || Bodies[i].type == SQUID_BLOCK_TYPE)
        {
            if (Bodies[i].vVelocity.Magnitude() > MAX_OBJECT_VELOCITY)
            {
                Bodies[i].vVelocity.Normalize(MAX_OBJECT_VELOCITY);
            } else if (Bodies[i].vVelocity.Magnitude() < MIN_OBJECT_VELOCITY)
            {
                Bodies[i].vVelocity.Normalize(MIN_OBJECT_VELOCITY);
            }
        }
        else
        {
            if (Bodies[i].vVelocity.Magnitude() > MAX_VELOCITY)
            {
                Bodies[i].vVelocity.Normalize(MAX_VELOCITY);
            }
        }

        // calculate the position of the object in earth space:
        Bodies[i].vPosition += Bodies[i].vVelocity * dt;

        // Now handle the rotations:
        float       mag;

        Bodies[i].vAngularAcceleration = Bodies[i].mInertiaInverse *
            (Bodies[i].vMoments -
            (Bodies[i].vAngularVelocity^
            (Bodies[i].mInertia * Bodies[i].vAngularVelocity)));

        Bodies[i].vAngularVelocity += Bodies[i].vAngularAcceleration * dt;
        if (Bodies[i].type == XWING_BLOCK_TYPE || Bodies[i].type == SQUID_BLOCK_TYPE)
        {
            if (Bodies[i].vAngularVelocity.Magnitude() > MAX_OBJECT_ANGULAR_VELOCITY)
            {
                Bodies[i].vAngularVelocity.Normalize(MAX_OBJECT_ANGULAR_VELOCITY);
            } else if (Bodies[i].vAngularVelocity.Magnitude() < MIN_OBJECT_ANGULAR_VELOCITY)
            {
                Bodies[i].vAngularVelocity.Normalize(MIN_OBJECT_ANGULAR_VELOCITY);
            }
        }
        else
        {
            if (Bodies[i].vAngularVelocity.Magnitude() > MAX_ANGULAR_VELOCITY)
            {
                Bodies[i].vAngularVelocity.Normalize(MAX_ANGULAR_VELOCITY);
            }
        }

        // calculate the new rotation quaternion:
        Bodies[i].qOrientation +=   (Bodies[i].qOrientation * Bodies[i].vAngularVelocity) *
            (0.5f * dt);

        // now normalize the orientation quaternion:
        mag = Bodies[i].qOrientation.Magnitude();
        if (mag != 0)
            Bodies[i].qOrientation /= mag;

        // calculate the velocity in body space:
        Bodies[i].vVelocityBody = QVRotate(~Bodies[i].qOrientation, Bodies[i].vVelocity);

        // calculate the speed:
        Bodies[i].fSpeed = Bodies[i].vVelocity.Magnitude();

        // get the Euler angles for our information
        Vector u;

        u = MakeEulerAnglesFromQ(Bodies[i].qOrientation);
        Bodies[i].vEulerAngles.x = u.x;           // roll
        Bodies[i].vEulerAngles.y = u.y;           // pitch
        Bodies[i].vEulerAngles.z = u.z;           // yaw
    }

    // Move groups uniformly.
    for (i = 0; i < NumBodies;)
    {
        if (!Bodies[i].valid || Bodies[i].group == -1) { i++; continue; }

        for (j = i; Bodies[j].group == Bodies[i].group && j < NumBodies; j++)
        {
            Bodies[j].vAcceleration = Bodies[i].vAcceleration;
            Bodies[j].vVelocity = Bodies[i].vVelocity;
            Bodies[j].vPosition = Bodies[i].vPosition;
            Bodies[j].vAngularAcceleration = Bodies[i].vAngularAcceleration;
            Bodies[j].vAngularVelocity = Bodies[i].vAngularVelocity;
            Bodies[j].qOrientation = Bodies[i].qOrientation;
            Bodies[j].vVelocityBody = Bodies[i].vVelocityBody;
            Bodies[j].fSpeed = Bodies[i].fSpeed;
            Bodies[j].vEulerAngles.x = Bodies[i].vEulerAngles.x;
            Bodies[j].vEulerAngles.y = Bodies[i].vEulerAngles.y;
            Bodies[j].vEulerAngles.z = Bodies[i].vEulerAngles.z;
        }
        i = j;
    }

    // Handle Collisions
    if(CheckForCollisions() == COLLISION)
    {
        ResolveCollisions(dt);
    }
}


int CheckForCollisions(void)
{
    int status = NOCOLLISION;
    int i,j;
    Vector  d;
    pCollision  pCollisionData;
    int     check = NOCOLLISION;

    pCollisionData = Collisions;
    NumCollisions = 0;

    // check object collisions with each other
    for(i=0; i<NumBodies; i++)
    {
        if (!Bodies[i].valid) continue;
        if (Bodies[i].type == WALL_TYPE || Bodies[i].type == FIXED_BLOCK_TYPE) continue;
        for(j=0; j<NumBodies; j++)
        {
            if (!Bodies[j].valid) continue;
            if (i == j) continue;
            if (Bodies[i].group != -1 && Bodies[i].group == Bodies[j].group) continue;
            if (Bodies[i].exempt != -1 && Bodies[i].exempt == Bodies[j].group) continue;
            if (Bodies[j].exempt != -1 && Bodies[j].exempt == Bodies[i].group) continue;

            // do a bounding sphere check first
            d = Bodies[i].vPosition - Bodies[j].vPosition;
            if(d.Magnitude() < (Bodies[i].fRadius + Bodies[j].fRadius))
            {                                     // possible collision, do a vertex check
                check = CheckBoxCollision(pCollisionData, i, j, COLLISIONTOLERANCE);
                if(check == COLLISION)
                {
                    // flag collision
                    status = COLLISION;

                    // for X-wing, non-squid collisions take priority.
                    if (Bodies[i].type == XWING_BLOCK_TYPE)
                    {
                        if (Bodies[i].collision)
                        {
                            if (Bodies[j].type != SQUID_BLOCK_TYPE)
                            {
                                Bodies[i].withWho = j;
                            }
                        }
                        else
                        {
                            Bodies[i].collision = true;
                            Bodies[i].withWho = j;
                        }
                    }
                    else
                    {
                        Bodies[i].collision = true;
                        Bodies[i].withWho = j;
                    }
                    if (Bodies[j].type == XWING_BLOCK_TYPE)
                    {
                        if (Bodies[j].collision)
                        {
                            if (Bodies[i].type != SQUID_BLOCK_TYPE)
                            {
                                Bodies[j].withWho = i;
                            }
                        }
                        else
                        {
                            Bodies[j].collision = true;
                            Bodies[j].withWho = i;
                        }
                    }
                    else
                    {
                        Bodies[j].collision = true;
                        Bodies[j].withWho = i;
                    }
                }
            }
        }
    }

    return status;
}


int CheckForSpecificCollision(int body1, int body2, float tolerance)
{
    pCollision  pCollisionData;

    pCollisionData = Collisions;
    NumCollisions = 0;

    if (CheckBoxCollision(pCollisionData, body1, body2, tolerance) == COLLISION)
    {
        return COLLISION;
    }
    else
    {
        return NOCOLLISION;
    }
}


void ResolveCollisions(float dt)
{
    int i,k;
    Vector pt1, pt2;
    float   j;
    float   fCr = COEFFICIENTOFRESTITUTION;
    int     b1, b2;
    float Vrt;
    float   mu = FRICTIONCOEFFICIENT;

    for(i=0; i<NumCollisions; i++)
    {
        b1 = Collisions[i].body1;
        b2 = Collisions[i].body2;

        pt1 = Collisions[i].vCollisionPoint - Bodies[b1].vPosition;
        pt2 = Collisions[i].vCollisionPoint - Bodies[b2].vPosition;

        // calculate impulse
        j = (-(1+fCr) * (Collisions[i].vRelativeVelocity*Collisions[i].vCollisionNormal)) /
            ( (1/Bodies[b1].fMass + 1/Bodies[b2].fMass) +
            (Collisions[i].vCollisionNormal * ( ( (pt1 ^ Collisions[i].vCollisionNormal)*Bodies[b1].mInertiaInverse )^pt1) ) +
            (Collisions[i].vCollisionNormal * ( ( (pt2 ^ Collisions[i].vCollisionNormal)*Bodies[b2].mInertiaInverse )^pt2) )
            );

        Vrt = Collisions[i].vRelativeVelocity * Collisions[i].vCollisionTangent;

        if(fabs(Vrt) > 0.0)
        {
            if (Bodies[b1].type != WALL_TYPE && Bodies[b1].type != FIXED_BLOCK_TYPE)
            {
                Bodies[b1].vVelocity += ( (j * Collisions[i].vCollisionNormal) + ((mu * j) * Collisions[i].vCollisionTangent) ) / Bodies[b1].fMass;
                Bodies[b1].vAngularVelocity += (pt1 ^ ((j * Collisions[i].vCollisionNormal) + ((mu * j) * Collisions[i].vCollisionTangent)))*Bodies[b1].mInertiaInverse;
            }
            if (Bodies[b2].type != WALL_TYPE && Bodies[b2].type != FIXED_BLOCK_TYPE)
            {
                Bodies[b2].vVelocity -= ((j * Collisions[i].vCollisionNormal) + ((mu * j) * Collisions[i].vCollisionTangent)) / Bodies[b2].fMass;
                Bodies[b2].vAngularVelocity -= (pt2 ^ ((j * Collisions[i].vCollisionNormal) + ((mu * j) * Collisions[i].vCollisionTangent)))*Bodies[b2].mInertiaInverse;
            }

        }
        else
        {
            if (Bodies[b1].type != WALL_TYPE && Bodies[b1].type != FIXED_BLOCK_TYPE)
            {
                // apply impulse
                Bodies[b1].vVelocity += (j * Collisions[i].vCollisionNormal) / Bodies[b1].fMass;
                Bodies[b1].vAngularVelocity += (pt1 ^ (j * Collisions[i].vCollisionNormal))*Bodies[b1].mInertiaInverse;
            }
            if (Bodies[b2].type != WALL_TYPE && Bodies[b2].type != FIXED_BLOCK_TYPE)
            {
                Bodies[b2].vVelocity -= (j * Collisions[i].vCollisionNormal) / Bodies[b2].fMass;
                Bodies[b2].vAngularVelocity -= (pt2 ^ (j * Collisions[i].vCollisionNormal))*Bodies[b2].mInertiaInverse;
            }
        }

        // All groups move as a whole.
        if (Bodies[b1].group != -1)
        {
            for (k = Bodies[b1].group; Bodies[k].group == Bodies[b1].group && k < NumBodies; k++)
            {
                Bodies[k].vVelocity = Bodies[b1].vVelocity;
                Bodies[k].vAngularVelocity = Bodies[b1].vAngularVelocity;
                Bodies[k].collision = true;
            }
        }
        if (Bodies[b2].group != -1)
        {
            for (k = Bodies[b2].group; Bodies[k].group == Bodies[b2].group && k < NumBodies; k++)
            {
                Bodies[k].vVelocity = Bodies[b2].vVelocity;
                Bodies[k].vAngularVelocity = Bodies[b2].vAngularVelocity;
                Bodies[k].collision = true;
            }
        }
    }
}


float   CalcDistanceFromPointToPlane(Vector pt, Vector u, Vector v, Vector ptOnPlane)
{
    Vector n = u^v;
    Vector PQ = pt - ptOnPlane;

    n.Normalize();

    return PQ*n;
}


bool    IsPointOnFace(Vector pt, Vector f[4])
{
    Vector u, v, n;
    Vector vList[4];
    int i;
    Vector p;

    u = f[1] - f[0];
    v = f[3] - f[0];
    n = u^v;

    if((fabs(n.x) > fabs(n.y)) && (fabs(n.x) > fabs(n.z)))
    {                                             // flatten in yz plane
        for(i=0; i<4; i++)
        {
            vList[i].x = f[i].y;
            vList[i].y = f[i].z;
            vList[i].z = 0.0f;
        }
        p.x = pt.y;
        p.y = pt.z;
        p.z = 0.0;

        if(pnpoly(4, vList, p) == 1)
            return true;
    } else if((fabs(n.y) > fabs(n.x)) && (fabs(n.y) > fabs(n.z)))
    {                                             // flatten in xz plane
        for(i=0; i<4; i++)
        {
            vList[i].x = f[i].x;
            vList[i].y = f[i].z;
            vList[i].z = 0.0f;
        }
        p.x = pt.x;
        p.y = pt.z;
        p.z = 0.0;

        if(pnpoly(4, vList, p) == 1)
            return true;
    } else if((fabs(n.z) > fabs(n.x)) && (fabs(n.z) > fabs(n.y)))
    {                                             // flatten in xy plane
        for(i=0; i<4; i++)
        {
            vList[i].x = f[i].x;
            vList[i].y = f[i].y;
            vList[i].z = 0.0f;
        }
        p.x = pt.x;
        p.y = pt.y;
        p.z = 0.0;

        if(pnpoly(4, vList, p) == 1)
            return true;
    }

    return false;
}


int CheckBoxCollision(pCollision CollisionData, int body1, int body2, float tolerance)
{
    int     i;
    Vector  v1[8];
    Vector  v2[8];
    Vector  tmp;
    Vector  u, v;
    float   d;
    Vector  f[4];
    Vector  vel1, vel2;
    Vector  pt1, pt2;
    Vector  Vr;
    float   Vrn;
    Vector  n;
    int status = NOCOLLISION;

    //rotate bounding vertices and covert to global coordinates
    for(i=0; i<8; i++)
    {
        tmp = Bodies[body1].vVertexList[i];
        v1[i] = QVRotate(Bodies[body1].qOrientation, tmp);
        v1[i] += Bodies[body1].vPosition;

        tmp = Bodies[body2].vVertexList[i];
        v2[i] = QVRotate(Bodies[body2].qOrientation, tmp);
        v2[i] += Bodies[body2].vPosition;
    }

    //check each vertex of body i against each face of body j
    for(i=0; i<8; i++)
    {
        // Front face of body 2:
        u = v2[1]-v2[0];
        v = v2[3]-v2[0];
        d = CalcDistanceFromPointToPlane(v1[i], u, v, v2[0]);
        if(fabs(d) < tolerance)
        {
            f[0] = v2[0];
            f[1] = v2[1];
            f[2] = v2[2];
            f[3] = v2[3];
            if(IsPointOnFace(v1[i], f))
            {
                // calc relative velocity, if <0 collision
                pt1 = v1[i] - Bodies[body1].vPosition;
                pt2 = v1[i] - Bodies[body2].vPosition;

                vel1 = Bodies[body1].vVelocityBody + (Bodies[body1].vAngularVelocity^pt1);
                vel2 = Bodies[body2].vVelocityBody + (Bodies[body2].vAngularVelocity^pt2);

                vel1 = QVRotate(Bodies[body1].qOrientation, vel1);
                vel2 = QVRotate(Bodies[body2].qOrientation, vel2);

                n = u^v;
                n.Normalize();

                Vr = (vel1 - vel2);
                Vrn = Vr * n;

                if(Vrn < 0.0f)
                {
                    // have a collision, fill the data structure and return
                    assert(NumCollisions < (NumBodies*8));
                    if(NumCollisions < (NumBodies*8))
                    {
                        CollisionData->body1 = body1;
                        CollisionData->body2 = body2;
                        CollisionData->vCollisionNormal = n;
                        CollisionData->vCollisionPoint = v1[i];
                        CollisionData->vRelativeVelocity = Vr;
                        CollisionData->vCollisionTangent = -(Vr - ((Vr*n)*n));
                        CollisionData->vCollisionTangent.Normalize();
                        CollisionData->vCollisionTangent.Normalize();
                        CollisionData++;
                        NumCollisions++;
                        status = true;
                    }
                }
            }
        }

        // Aft face of body 2:
        u = v2[6]-v2[7];
        v = v2[4]-v2[7];
        d = CalcDistanceFromPointToPlane(v1[i], u, v, v2[7]);
        if(fabs(d) < tolerance)
        {
            f[0] = v2[7];
            f[1] = v2[6];
            f[2] = v2[5];
            f[3] = v2[4];
            if(IsPointOnFace(v1[i], f))
            {
                // calc relative velocity, if <0 collision
                pt1 = v1[i] - Bodies[body1].vPosition;
                pt2 = v1[i] - Bodies[body2].vPosition;

                vel1 = Bodies[body1].vVelocityBody + (Bodies[body1].vAngularVelocity^pt1);
                vel2 = Bodies[body2].vVelocityBody + (Bodies[body2].vAngularVelocity^pt2);

                vel1 = QVRotate(Bodies[body1].qOrientation, vel1);
                vel2 = QVRotate(Bodies[body2].qOrientation, vel2);

                n = u^v;
                n.Normalize();

                Vr = (vel1 - vel2);
                Vrn = Vr * n;

                if(Vrn < 0.0f)
                {
                    // have a collision, fill the data structure and return
                    assert(NumCollisions < (NumBodies*8));
                    if(NumCollisions < (NumBodies*8))
                    {
                        CollisionData->body1 = body1;
                        CollisionData->body2 = body2;
                        CollisionData->vCollisionNormal = n;
                        CollisionData->vCollisionPoint = v1[i];
                        CollisionData->vRelativeVelocity = Vr;
                        CollisionData->vCollisionTangent = -(Vr - ((Vr*n)*n));
                        CollisionData->vCollisionTangent.Normalize();
                        CollisionData->vCollisionTangent.Normalize();
                        CollisionData++;
                        NumCollisions++;
                        status = true;
                    }
                }
            }
        }

        // Top face of body 2:
        u = v2[2]-v2[6];
        v = v2[5]-v2[6];
        d = CalcDistanceFromPointToPlane(v1[i], u, v, v2[6]);
        if(fabs(d) < tolerance)
        {
            f[0] = v2[6];
            f[1] = v2[2];
            f[2] = v2[1];
            f[3] = v2[5];
            if(IsPointOnFace(v1[i], f))
            {
                // calc relative velocity, if <0 collision
                pt1 = v1[i] - Bodies[body1].vPosition;
                pt2 = v1[i] - Bodies[body2].vPosition;

                vel1 = Bodies[body1].vVelocityBody + (Bodies[body1].vAngularVelocity^pt1);
                vel2 = Bodies[body2].vVelocityBody + (Bodies[body2].vAngularVelocity^pt2);

                vel1 = QVRotate(Bodies[body1].qOrientation, vel1);
                vel2 = QVRotate(Bodies[body2].qOrientation, vel2);

                n = u^v;
                n.Normalize();

                Vr = (vel1 - vel2);
                Vrn = Vr * n;

                if(Vrn < 0.0f)
                {
                    // have a collision, fill the data structure and return
                    assert(NumCollisions < (NumBodies*8));
                    if(NumCollisions < (NumBodies*8))
                    {
                        CollisionData->body1 = body1;
                        CollisionData->body2 = body2;
                        CollisionData->vCollisionNormal = n;
                        CollisionData->vCollisionPoint = v1[i];
                        CollisionData->vRelativeVelocity = Vr;
                        CollisionData->vCollisionTangent = -(Vr - ((Vr*n)*n));
                        CollisionData->vCollisionTangent.Normalize();
                        CollisionData->vCollisionTangent.Normalize();
                        CollisionData++;
                        NumCollisions++;
                        status = true;
                    }
                }
            }
        }

        // Bottom face of body 2:
        u = v2[0]-v2[4];
        v = v2[7]-v2[4];
        d = CalcDistanceFromPointToPlane(v1[i], u, v, v2[4]);
        if(fabs(d) < tolerance)
        {
            f[0] = v2[4];
            f[1] = v2[0];
            f[2] = v2[3];
            f[3] = v2[7];
            if(IsPointOnFace(v1[i], f))
            {
                // calc relative velocity, if <0 collision
                pt1 = v1[i] - Bodies[body1].vPosition;
                pt2 = v1[i] - Bodies[body2].vPosition;

                vel1 = Bodies[body1].vVelocityBody + (Bodies[body1].vAngularVelocity^pt1);
                vel2 = Bodies[body2].vVelocityBody + (Bodies[body2].vAngularVelocity^pt2);

                vel1 = QVRotate(Bodies[body1].qOrientation, vel1);
                vel2 = QVRotate(Bodies[body2].qOrientation, vel2);

                n = u^v;
                n.Normalize();

                Vr = (vel1 - vel2);
                Vrn = Vr * n;

                if(Vrn < 0.0f)
                {
                    // have a collision, fill the data structure and return
                    assert(NumCollisions < (NumBodies*8));
                    if(NumCollisions < (NumBodies*8))
                    {
                        CollisionData->body1 = body1;
                        CollisionData->body2 = body2;
                        CollisionData->vCollisionNormal = n;
                        CollisionData->vCollisionPoint = v1[i];
                        CollisionData->vRelativeVelocity = Vr;
                        CollisionData->vCollisionTangent = -(Vr - ((Vr*n)*n));
                        CollisionData->vCollisionTangent.Normalize();
                        CollisionData->vCollisionTangent.Normalize();
                        CollisionData++;
                        NumCollisions++;
                        status = true;
                    }
                }
            }
        }

        // Left face of body 2:
        u = v2[5]-v2[4];
        v = v2[0]-v2[4];
        d = CalcDistanceFromPointToPlane(v1[i], u, v, v2[4]);
        if(fabs(d) < tolerance)
        {
            f[0] = v2[4];
            f[1] = v2[5];
            f[2] = v2[1];
            f[3] = v2[0];
            if(IsPointOnFace(v1[i], f))
            {
                // calc relative velocity, if <0 collision
                pt1 = v1[i] - Bodies[body1].vPosition;
                pt2 = v1[i] - Bodies[body2].vPosition;

                vel1 = Bodies[body1].vVelocityBody + (Bodies[body1].vAngularVelocity^pt1);
                vel2 = Bodies[body2].vVelocityBody + (Bodies[body2].vAngularVelocity^pt2);

                vel1 = QVRotate(Bodies[body1].qOrientation, vel1);
                vel2 = QVRotate(Bodies[body2].qOrientation, vel2);

                n = u^v;
                n.Normalize();

                Vr = (vel1 - vel2);
                Vrn = Vr * n;

                if(Vrn < 0.0f)
                {
                    // have a collision, fill the data structure and return
                    assert(NumCollisions < (NumBodies*8));
                    if(NumCollisions < (NumBodies*8))
                    {
                        CollisionData->body1 = body1;
                        CollisionData->body2 = body2;
                        CollisionData->vCollisionNormal = n;
                        CollisionData->vCollisionPoint = v1[i];
                        CollisionData->vRelativeVelocity = Vr;
                        CollisionData->vCollisionTangent = -(Vr - ((Vr*n)*n));
                        CollisionData->vCollisionTangent.Normalize();
                        CollisionData->vCollisionTangent.Normalize();
                        CollisionData++;
                        NumCollisions++;
                        status = true;
                    }
                }
            }
        }

        // Right face of body 2:
        u = v2[6]-v2[2];
        v = v2[3]-v2[2];
        d = CalcDistanceFromPointToPlane(v1[i], u, v, v2[2]);
        if(fabs(d) < tolerance)
        {
            f[0] = v2[2];
            f[1] = v2[6];
            f[2] = v2[7];
            f[3] = v2[3];
            if(IsPointOnFace(v1[i], f))
            {
                // calc relative velocity, if <0 collision
                pt1 = v1[i] - Bodies[body1].vPosition;
                pt2 = v1[i] - Bodies[body2].vPosition;

                vel1 = Bodies[body1].vVelocityBody + (Bodies[body1].vAngularVelocity^pt1);
                vel2 = Bodies[body2].vVelocityBody + (Bodies[body2].vAngularVelocity^pt2);

                vel1 = QVRotate(Bodies[body1].qOrientation, vel1);
                vel2 = QVRotate(Bodies[body2].qOrientation, vel2);

                n = u^v;
                n.Normalize();

                Vr = (vel1 - vel2);
                Vrn = Vr * n;

                if(Vrn < 0.0f)
                {
                    // have a collision, fill the data structure and return
                    assert(NumCollisions < (NumBodies*8));
                    if(NumCollisions < (NumBodies*8))
                    {
                        CollisionData->body1 = body1;
                        CollisionData->body2 = body2;
                        CollisionData->vCollisionNormal = n;
                        CollisionData->vCollisionPoint = v1[i];
                        CollisionData->vRelativeVelocity = Vr;
                        CollisionData->vCollisionTangent = -(Vr - ((Vr*n)*n));
                        CollisionData->vCollisionTangent.Normalize();
                        CollisionData->vCollisionTangent.Normalize();
                        CollisionData++;
                        NumCollisions++;
                        status = true;
                    }
                }
            }
        }

    }

    return status;
}


//------------------------------------------------------------------------//
//
//------------------------------------------------------------------------//
Vector  GetBodyZAxisVector(int index)
{

    Vector  v;

    v.x = 0.0f;
    v.y = 0.0f;
    v.z = 1.0f;

    return QVRotate(Bodies[index].qOrientation, v);
}


//------------------------------------------------------------------------//
//
//------------------------------------------------------------------------//
Vector  GetBodyXAxisVector(int index)
{

    Vector v;

    v.x = 1.0f;
    v.y = 0.0f;
    v.z = 0.0f;

    return QVRotate(Bodies[index].qOrientation, v);

}


//------------------------------------------------------------------------//
//
//------------------------------------------------------------------------//
Matrix3x3   MakeAngularVelocityMatrix(Vector u)
{
    return Matrix3x3(   0.0f, -u.z, u.y,
        u.z, 0.0f, -u.x,
        -u.y, u.x, 0.0f);
}


//------------------------------------------------------------------------//
// Adapted from Rourke's Computational Geometry FAQ
//------------------------------------------------------------------------//
int pnpoly(int  npol, Vector *vlist, Vector p)
{
    int i, j, c = 0;

    for (i = 0, j = npol-1; i<npol; j = i++)
    {
        if ((((vlist[i].y<=p.y) && (p.y<vlist[j].y)) ||
            ((vlist[j].y<=p.y) && (p.y<vlist[i].y))) &&
            (p.x < (vlist[j].x - vlist[i].x) * (p.y - vlist[i].y) / (vlist[j].y - vlist[i].y) + vlist[i].x))

            c = !c;
    }
    return c;
}


// Draw body.
void DrawBody(RigidBody *body)
{
    GLfloat x,y,z;

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glLineWidth(1.0);
    glColor3f(1.0, 1.0, 1.0);

    x = body->vPosition.x;
    y = body->vPosition.y;
    z = body->vPosition.z;

    glBegin(GL_LINE_LOOP);
    glVertex3f(body->vVertexList[0].x + x, body->vVertexList[0].y + y, body->vVertexList[0].z + z);
    glVertex3f(body->vVertexList[1].x + x, body->vVertexList[1].y + y, body->vVertexList[1].z + z);
    glVertex3f(body->vVertexList[2].x + x, body->vVertexList[2].y + y, body->vVertexList[2].z + z);
    glVertex3f(body->vVertexList[3].x + x, body->vVertexList[3].y + y, body->vVertexList[3].z + z);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(body->vVertexList[3].x + x, body->vVertexList[3].y + y, body->vVertexList[3].z + z);
    glVertex3f(body->vVertexList[2].x + x, body->vVertexList[2].y + y, body->vVertexList[2].z + z);
    glVertex3f(body->vVertexList[6].x + x, body->vVertexList[6].y + y, body->vVertexList[6].z + z);
    glVertex3f(body->vVertexList[7].x + x, body->vVertexList[7].y + y, body->vVertexList[7].z + z);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(body->vVertexList[7].x + x, body->vVertexList[7].y + y, body->vVertexList[7].z + z);
    glVertex3f(body->vVertexList[6].x + x, body->vVertexList[6].y + y, body->vVertexList[6].z + z);
    glVertex3f(body->vVertexList[5].x + x, body->vVertexList[5].y + y, body->vVertexList[5].z + z);
    glVertex3f(body->vVertexList[4].x + x, body->vVertexList[4].y + y, body->vVertexList[4].z + z);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(body->vVertexList[0].x + x, body->vVertexList[0].y + y, body->vVertexList[0].z + z);
    glVertex3f(body->vVertexList[4].x + x, body->vVertexList[4].y + y, body->vVertexList[4].z + z);
    glVertex3f(body->vVertexList[5].x + x, body->vVertexList[5].y + y, body->vVertexList[5].z + z);
    glVertex3f(body->vVertexList[1].x + x, body->vVertexList[1].y + y, body->vVertexList[1].z + z);
    glEnd();
}
