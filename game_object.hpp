//***************************************************************************//
//* File Name: game_object.hpp                                              *//
//* Author: Chris McBride chris_a_mcbride@hotmail.com                       *//
//* Date Made: 04/15/02                                                     *//
//* File Desc: Class declaration and implementation details for base class  *//
//*            representing a game object.                                  *//
//* Rev. Date: 07/25/02 (TEP)                                               *//
//* Rev. Desc: Added spacial properties and bounding object list.           *//
//*                                                                         *//
//***************************************************************************//
#ifndef __GAME_OBJECT_HPP__
#define __GAME_OBJECT_HPP__

#include <GL/gl.h>
#include <GL/glu.h>
#include "glut.h"
#include "spacial.hpp"

typedef unsigned int uint;

class cGameObject
{
    public:

        // VARIOUS OBJECT TYPES.
        static const int XWING;
        static const int SQUID;
        static const int TENTACLE;

        // DISPOSITION TOWARD PLAYER CHARACTER FOR OBJECTS
        static const int NEUTRAL;
        static const int FRIENDLY;
        static const int HOSTILE;

        // Constructor.
        cGameObject()
            : m_isAlive(true),
            m_numVertices(0),
            m_objId(0),
            m_healthPoints(100),
            m_dispList(0),
            m_type(0)
        {
            m_spacial = new cSpacial();
            m_pitch = m_yaw = m_roll = 0.0;
        }

        // Destructor.
        ~cGameObject()
        {
            delete m_spacial;
        }

        // Rotations.
        GLfloat GetPitch() { return(m_pitch); }
        GLfloat GetYaw()   { return(m_yaw); }
        GLfloat GetRoll()  { return(m_roll); }
        void SetPitch(GLfloat n)
        {
            m_spacial->pitch = (n - m_pitch);
            m_spacial->update();
            m_spacial->pitch = 0.0;
            m_pitch = n;
        }
        void SetYaw(GLfloat n)
        {
            m_spacial->yaw = (n - m_yaw);
            m_spacial->update();
            m_spacial->yaw = 0.0;
            m_yaw = n;
        }
        void SetRoll(GLfloat n)
        {
            m_spacial->roll = (n - m_roll);
            m_spacial->update();
            m_spacial->roll = 0.0;
            m_roll = n;
        }
        void AddPitch(GLfloat n)
        {
            m_spacial->pitch = n;
            m_spacial->update();
            m_spacial->pitch = 0.0;
            m_pitch = n + m_pitch;
        }
        void AddYaw(GLfloat n)
        {
            m_spacial->yaw = n;
            m_spacial->update();
            m_spacial->yaw = 0.0;
            m_yaw = n + m_yaw;
        }
        void AddRoll(GLfloat n)
        {
            m_spacial->roll = n;
            m_spacial->update();
            m_spacial->roll = 0.0;
            m_roll = n + m_roll;
        }

        // Get direction vectors.
        void GetRight(GLfloat *v)   { m_spacial->getRight(v); }
        void GetUp(GLfloat *v)      { m_spacial->getUp(v); }
        void GetForward(GLfloat *v) { m_spacial->getForward(v); }

        // Position.
        void GetPosition(GLfloat *v)
        {
            v[0] = m_spacial->x;
            v[1] = m_spacial->y;
            v[2] = m_spacial->z;
        }
        void SetPosition(GLfloat *v)
        {
            m_spacial->x = v[0];
            m_spacial->y = v[1];
            m_spacial->z = v[2];
            m_spacial->update();
        }

        // Scale.
        GLfloat GetScale() { return(m_spacial->scale); }
        void SetScale(GLfloat n) { m_spacial->scale = n; }

        // Speed.
        GLfloat GetSpeed() { return(m_spacial->speed); }
        void SetSpeed(GLfloat n)
        {
            m_spacial->speed = n;
        }
        GLfloat GetSpeedFactor() { return(m_spacial->speedFactor); }
        void SetSpeedFactor(GLfloat n)
        {
            m_spacial->speedFactor = n;
            if (m_spacial->speedFactor < 0.0) m_spacial->speedFactor = 0.0;
        }

        // Get model transformation matrix.
        void GetModelTransform(GLfloat *matrix)
        {
            m_spacial->getModelTransform(matrix);
        }

        // Get world coordinates from local.
        void LocalToWorld(GLfloat *local, GLfloat *world)
        {
            m_spacial->localToWorld(local, world);
        }

        // Transform local point.
        void TransformPoint(GLfloat *point)
        {
            m_spacial->transformPoint(point);
        }

        // Inverse transform local point.
        void InverseTransformPoint(GLfloat *point)
        {
            m_spacial->inverseTransformPoint(point);
        }

        // Get nearest vertex to given point in world coordinates.
        virtual void GetNearestVertex(GLfloat *point, GLfloat *vertex) { return; }

        // Expose vertices.
        virtual void ExposeVertices(GLfloat **vertices, int *size) { return; }

        // Exploding state.
        virtual void Explode() { return; }
        virtual bool IsExploding() { return(false); }

        // Get billboard rotation to given target point.
        void GetBillboard(GLfloat *target, GLfloat *rotation)
        {
            m_spacial->getBillboard(target, rotation);
        }

        // Get spacial properties.
        cSpacial *GetSpacial() { return(m_spacial); }

        void SetId(uint ident) {m_objId = ident;}
        uint GetId() {return m_objId;}

        bool IsAlive(){return m_isAlive;}

        virtual void Go() = 0;
        virtual void Update() = 0;
        virtual void Draw() = 0;
        virtual void Kill() = 0;

    protected:
        uint   *m_objIndices;
        GLfloat  *m_objColors;
        GLfloat  *m_objVertices;
        GLfloat  *m_objNormals;
        bool    m_isAlive;
        uint    m_numVertices;
        uint    m_numTriangles;
        uint    m_objId;
        int     m_type;
        int     m_disposition;
        uint    m_healthPoints;
        GLint   m_dispList;

        // Spacial properties.
        cSpacial *m_spacial;
        GLfloat m_pitch, m_yaw, m_roll;
};

// VARIOUS OBJECT TYPES.
const int cGameObject::XWING = 1;
const int cGameObject::SQUID = 2;
const int cGameObject::TENTACLE = 3;

// DISPOSITION TOWARD PLAYER CHARACTER FOR OBJECTS.
const int cGameObject::NEUTRAL  = 1;
const int cGameObject::FRIENDLY = 2;
const int cGameObject::HOSTILE  = 3;
#endif                                            // #ifndef __GAME_OBJECT_HPP__
