#ifndef _PHYSICS
#define _PHYSICS

#include "math_etc.h"

//------------------------------------------------------------------------//
// Notes:
//------------------------------------------------------------------------//
/*

    Earth coordinates:	x points North
                        y points West
                        z points up

                        Z   X
                        |  /
                        | /
                        |/
Y------+

Body coordinates:	x points to the front
y points to left
z points up

Units:	English system,
distance	--> feet
time		-->	seconds
mass		--> slugs
force		--> pounds
moment		--> foot-pounds
velocity	--> feet-per-second
acceleration-->	feet-per-second^2
density		--> slugs-per-feet^3

*/

//------------------------------------------------------------------------//
// Rigid body structure
//------------------------------------------------------------------------//
typedef struct _RigidBody
{

    bool        valid;                            // valid entry?
    float       fMass;                            // total mass (constant)
    Matrix3x3   mInertia;                         // mass moment of inertia in body coordinates (constant)
    Matrix3x3   mInertiaInverse;                  // inverse of mass moment of inertia matrix	(constant)

    Vector      vPosition;                        // position in earth coordinates
    Vector      vVelocity;                        // velocity in earth coordinates
    Vector      vVelocityBody;                    // velocity in body coordinates
    Vector      vAcceleration;                    // acceleration of cg in earth space
    Vector      vAngularAcceleration;             //angular acceleration in body coordinates
    Vector      vAngularVelocity;                 // angular velocity in body coordinates
    Vector      vEulerAngles;                     // Euler angles in body coordinates
    float       fSpeed;                           // speed (magnitude of the velocity)

    Quaternion  qOrientation;                     // orientation in earth coordinates

    Vector      vForces;                          // total force on body
    Vector      vMoments;                         // total moment (torque) on body

    Matrix3x3   mIeInverse;                       // inverse of moment of inertia in earth coordinates

    float       fRadius;
    Vector      vVertexList[8];
    int         type;                             // type of body.
    int         group;                            // index of first body in group.
    float       red,green,blue;                   // color
    int         display;                          // display list
    bool        collision;                        // collision?
    int         withWho;                          // with who: index.
    int         exempt;                           // no collisions with this group.

} RigidBody, *pRigidBody;

extern RigidBody Bodies[];
extern int NumBodies;

typedef struct  _Collision
{
    int             body1;
    int             body2;
    Vector          vCollisionNormal;             // outward from face of body2
    Vector          vCollisionPoint;              // in global coordinates
    Vector          vRelativeVelocity;
    Vector          vRelativeAcceleration;
    Vector          vCollisionTangent;
}   Collision, *pCollision;

#define     MAX_BODIES              200
#define     BLOCK_SIZE              2.0f
#define     FIXED_BLOCK_SIZE        5.0f

// Body types:
#define     WALL_TYPE               0
#define     BLOCK_TYPE              1
#define     FIXED_BLOCK_TYPE        2
#define     XWING_BLOCK_TYPE        3
#define     SQUID_BLOCK_TYPE        4

#define     MAX_VELOCITY            1.0f
#define     MAX_ANGULAR_VELOCITY    0.2f
#define     MIN_OBJECT_VELOCITY         0.2f
#define     MIN_OBJECT_ANGULAR_VELOCITY 0.2f
#define     MAX_OBJECT_VELOCITY         1.0f
#define     MAX_OBJECT_ANGULAR_VELOCITY 1.0f

#define     NOCOLLISION             0
#define     COLLISION               1
#define     PENETRATING             -1
#define     CONTACT                 2

#define     COLLISIONTOLERANCE      0.15f
#define     PENETRATIONTOLERANCE    0.1f
#define     COEFFICIENTOFRESTITUTION        0.5f
#define     FRICTIONCOEFFICIENT     0.9f
//------------------------------------------------------------------------//
// Function headers
//------------------------------------------------------------------------//
void    InitializePhysics();
void    InitializeObject(int index, float size, int type, int group);
void    InitializeObject(RigidBody *, float size, int type, int group);
void    ClearObjectForces(void);
void    StepSimulation(float dtime);              // step dt time in the simulation
int     CheckForCollisions(void);
int     CheckForSpecificCollision(int, int, float);
void    ResolveCollisions(float);
float   CalcDistanceFromPointToPlane(Vector pt, Vector u, Vector v, Vector ptOnPlane);
bool    IsPointOnFace(Vector pt, Vector f[4]);
int     CheckBoxCollision(pCollision CollisionData, int body1, int body2, float tolerance);

Vector  GetBodyZAxisVector(int index);
Vector  GetBodyXAxisVector(int index);
Matrix3x3   MakeAngularVelocityMatrix(Vector u);
int pnpoly(int  npol, Vector *vlist, Vector p);

void DrawBody(RigidBody *);
#endif
