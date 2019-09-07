//***************************************************************************//
//* File Name: plasmaBolt.hpp                                               *//
//* Author:    Tom Portegys, portegys@ilstu.edu                             *//
//* Date Made: 08/7/02                                                      *//
//* File Desc: Class declaration and implementation details                 *//
//*            representing a plasma bolt.                                  *//
//* Rev. Date:                                                              *//
//* Rev. Desc:                                                              *//
//*                                                                         *//
//***************************************************************************//

#ifndef __PLASMA_BOLT_HPP__
#define __PLASMA_BOLT_HPP__

#include <GL/gl.h>
#include <GL/glu.h>
#include "glut.h"
#include "quaternion.hpp"

// Pi
#define M_PI 3.14159265358979323846

class PlasmaBolt
{

    public:

        // Parameters.
        static const GLfloat PLASMA_BOLT_SPEED;
        static const GLfloat PLASMA_BOLT_RANGE;

        // Constructor.
        PlasmaBolt(GLfloat x, GLfloat y, GLfloat z, GLfloat s, GLfloat sf, GLfloat q[4]);

        // Destructor.
        ~PlasmaBolt();

        // Set speed.
        void setSpeed(GLfloat speed) { Speed = speed + PLASMA_BOLT_SPEED; }

        // Set speed factor.
        void setSpeedFactor(GLfloat speedFactor) { SpeedFactor = speedFactor; }

        // Go: move and draw.
        void Go() { move(); draw(); }

        // Draw.
        void draw();

        // Rotations.
        GLfloat Rotmatrix[4][4];

        // Move.
        void move();

        // Bolt is active?
        bool Active;

        // Bolt is near object of given radius and position?
        bool isNear(GLfloat *v, GLfloat r);

        // Get model transformation matrix.
        void getModelTransform(GLfloat *matrix)
        {
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glTranslatef(X, Y, Z);
            glMultMatrixf(&Rotmatrix[0][0]);
            glTranslatef(0.0, -0.15, 0.0);
            glRotatef(90.0, 1.0, 0.0, 0.0);
            glGetFloatv( GL_MODELVIEW_MATRIX, matrix );
            glPopMatrix();
        }

        // Get world coordinates from local.
        void localToWorld(GLfloat *local, GLfloat *world)
        {
            int i,j;
            GLfloat m[16];
            Matrix x(4,4),p(4,1),t(4,1);

            getModelTransform(m);
            for (i=0; i < 4; i++)
                for (j=0; j < 4; j++)
                    x(i,j) = m[(j*4)+i];
            p(0,0) = local[0];
            p(1,0) = local[1];
            p(2,0) = local[2];
            p(3,0) = 1.0;
            t = x * p;
            world[0] = t(0,0);
            world[1] = t(1,0);
            world[2] = t(2,0);
        }

        // Rotation.
        cQuaternion *Qcalc;

        // Translation.
        GLfloat Speed;
        GLfloat SpeedFactor;
        GLfloat X, Y, Z;

        // Distance.
        GLfloat Distance;
};

// Constructor.
PlasmaBolt::PlasmaBolt(GLfloat x, GLfloat y, GLfloat z, GLfloat s, GLfloat sf, GLfloat q[4])
{
    X = x;
    Y = y;
    Z = z;
    Speed = s + PLASMA_BOLT_SPEED;
    SpeedFactor = sf;
    Distance = 0.0;
    Active = true;
    Qcalc = new cQuaternion(q);
    Qcalc->build_rotmatrix(Rotmatrix, Qcalc->quat);
}


// Destructor.
PlasmaBolt::~PlasmaBolt()
{
    delete Qcalc;
}


// Draw plasma bolt.
void PlasmaBolt::draw()
{
    if (!Active) return;
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(X, Y, Z);
    glMultMatrixf(&Rotmatrix[0][0]);
    glTranslatef(0.0, -0.15, 0.0);
    glRotatef(90.0, 1.0, 0.0, 0.0);
    glColor3f(0.5, 1.0, 0.5);
    glutSolidTorus(0.01, 0.04, 10, 10);
    glPopMatrix();
}


// Move.
void PlasmaBolt::move()
{
    GLfloat v[3];
    GLfloat x,y,z;

    v[0] = Rotmatrix[1][0];
    v[1] = Rotmatrix[1][1];
    v[2] = Rotmatrix[1][2];
    cSpacial::normalize(v);
    x = (v[0] * Speed * SpeedFactor);
    y = (v[1] * Speed * SpeedFactor);
    z = (v[2] * Speed * SpeedFactor);
    Distance += sqrt((x*x) + (y*y) + (z*z));
    X -= x;
    Y -= y;
    Z -= z;
    if (Distance >= PLASMA_BOLT_RANGE)
    {
        Active = false;
    }

}


// Bolt is near object of given radius and position?
bool PlasmaBolt::isNear(GLfloat *v, GLfloat r)
{
    GLfloat local[3],world[3];
    GLfloat dx,dy,dz,d;

    local[0] = 0.0;
    local[1] = 0.0;
    local[2] = 0.0;
    localToWorld(local, world);
    dx = v[0] - world[0]; dx *= dx;
    dy = v[1] - world[1]; dy *= dy;
    dz = v[2] - world[2]; dz *= dz;
    d = sqrt(dx + dy + dz);
    if (d <= r) return(true);
    return(false);
}


// Parameters.
const GLfloat PlasmaBolt::PLASMA_BOLT_SPEED = 0.5;
const GLfloat PlasmaBolt::PLASMA_BOLT_RANGE = 20.0;
#endif
