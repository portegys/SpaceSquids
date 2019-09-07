//***************************************************************************//
//* File Name: spacial.hpp                                                  *//
//* Author:    Tom Portegys, portegys@ilstu.edu                             *//
//* Date Made: 07/25/02                                                     *//
//* File Desc: Class declaration and implementation details                 *//
//*            representing spacial properties: rotation, translation,      *//
//*            scale, and speed.                                            *//
//* Rev. Date:                                                              *//
//* Rev. Desc:                                                              *//
//*                                                                         *//
//***************************************************************************//

#ifndef __SPACIAL_HPP__
#define __SPACIAL_HPP__

#include "quaternion.hpp"
#include "matrix.h"
#include "math_etc.h"

#ifndef _NO_NAMESPACE
using namespace std;
using namespace math;
#define STD std
#else
#define STD
#endif

#ifndef _NO_TEMPLATE
typedef matrix<GLfloat> Matrix;
#else
typedef matrix Matrix;
#endif

// Pi and radians
#define M_PI 3.14159265358979323846
#define DIV_PI_180 .01745329251
#define DIV_180_PI 57.29577951

class cSpacial
{
    public:

        // Rotation.
        GLfloat pitch, yaw, roll;                 // Rotation rates.
        GLfloat rotmatrix[4][4];
        class cQuaternion *qcalc;

        // Translation.
        GLfloat x, y, z;

        // Scale.
        GLfloat scale;

        // Speed.
        GLfloat speed;
        GLfloat speedFactor;

        // Constructors.
        cSpacial()
        {
            GLfloat rotation[3];
            GLfloat translation[3];

            rotation[0] = rotation[1] = rotation[2] = 0;
            translation[0] = translation[1] = translation[2] = 0.0;
            initialize(rotation, translation, 1.0, 0.0);
        }

        cSpacial(GLfloat *rotation, GLfloat *translation, GLfloat scale, GLfloat speed)
        {
            initialize(rotation, translation, scale, speed);
        }

        void initialize(GLfloat *rotation, GLfloat *translation, GLfloat scale, GLfloat speed)
        {
            GLfloat v[4];

            v[0] = v[1] = v[2] = 0.0;
            v[3] = 1.0;
            qcalc = new cQuaternion(v);
            qcalc->build_rotmatrix(rotmatrix, qcalc->quat);
            pitch = rotation[0];
            yaw = rotation[1];
            roll = rotation[2];
            update();
            x = translation[0];
            y = translation[1];
            z = translation[2];
            this->scale = scale;
            this->speed = speed;
            speedFactor = 1.0;
        }

        // Destructor.
        ~cSpacial() { delete qcalc; }

        // Update rotation and translation state.
        void update();

        // Get direction vectors.
        void getRight(GLfloat *v)
        {
            v[0] = rotmatrix[0][0];
            v[1] = rotmatrix[0][1];
            v[2] = rotmatrix[0][2];
        }
        void getUp(GLfloat *v)
        {
            v[0] = rotmatrix[1][0];
            v[1] = rotmatrix[1][1];
            v[2] = rotmatrix[1][2];
        }
        void getForward(GLfloat *v)
        {
            v[0] = rotmatrix[2][0];
            v[1] = rotmatrix[2][1];
            v[2] = rotmatrix[2][2];
        }

        // Get model transformation matrix.
        void getModelTransform(GLfloat *matrix)
        {
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glTranslatef(x, y, z);
            qcalc->build_rotmatrix(rotmatrix, qcalc->quat);
            glMultMatrixf(&rotmatrix[0][0]);
            glScalef(scale, scale, scale);
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

        // Transform local point.
        void transformPoint(GLfloat *point)
        {
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();
            localToWorld(point, point);
            glPopMatrix();
        }

        // Inverse transform local point.
        void inverseTransformPoint(GLfloat *point)
        {
            int i,j;
            GLfloat m[16];
            Matrix x(4,4),y(4,4),p(4,1),t(4,1);

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();
            getModelTransform(m);
            for (i=0; i < 4; i++)
                for (j=0; j < 4; j++)
                    x(i,j) = m[(j*4)+i];
            y = !x;
            p(0,0) = point[0];
            p(1,0) = point[1];
            p(2,0) = point[2];
            p(3,0) = 1.0;
            t = y * p;
            point[0] = t(0,0);
            point[1] = t(1,0);
            point[2] = t(2,0);
            glPopMatrix();
        }

        // Normalize vector.
        inline static void normalize(GLfloat *v)
        {
            GLfloat d = (sqrt((v[0]*v[0]) + (v[1]*v[1]) + (v[2]*v[2])));
            v[0] = v[0] / d;
            v[1] = v[1] / d;
            v[2] = v[2] / d;
        }

        // Get billboard rotation to given target point.
        void getBillboard(GLfloat *target, GLfloat *rotation);

        // Load an axis-angle rotation into quaternion.
        void loadRotation(GLfloat angle, GLfloat *axis)
        {
            qcalc->loadRotation(angle, axis);
        }

        // Merge an axis-angle rotation into quaternion.
        void mergeRotation(GLfloat angle, GLfloat *axis)
        {
            qcalc->mergeRotation(angle, axis);
        }

        // Build rotation matrix from quaternion.
        void build_rotmatrix()
        {
            qcalc->build_rotmatrix(rotmatrix, qcalc->quat);
        }

        // Point-to-point distance.
        GLfloat pointDistance(GLfloat *p1, GLfloat *p2)
        {
            GLfloat dx = p1[0] - p2[0]; dx *= dx;
            GLfloat dy = p1[1] - p2[1]; dy *= dy;
            GLfloat dz = p1[2] - p2[2]; dz *= dz;
            return(sqrt(dx + dy + dz));
        }
};

// Update rotation and translation state.
void cSpacial::update()
{
    GLfloat xq[4], yq[4], zq[4], q1[4], q2[4];
    GLfloat v[3];

    v[0] = 1.0; v[1] = 0.0; v[2] = 0.0;
    qcalc->axis_to_quat(v, pitch * DIV_PI_180, xq);
    v[0] = 0.0; v[1] = 1.0; v[2] = 0.0;
    qcalc->axis_to_quat(v, roll * DIV_PI_180, yq);
    qcalc->mult_quats(xq, yq, q1);
    v[0] = 0.0; v[1] = 0.0; v[2] = 1.0;
    qcalc->axis_to_quat(v, yaw * DIV_PI_180, zq);
    qcalc->mult_quats(q1, zq, q2);
    q1[0] = qcalc->quat[0];
    q1[1] = qcalc->quat[1];
    q1[2] = qcalc->quat[2];
    q1[3] = qcalc->quat[3];
    qcalc->mult_quats(q1, q2, qcalc->quat);
    qcalc->build_rotmatrix(rotmatrix, qcalc->quat);
    v[0] = rotmatrix[1][0];
    v[1] = rotmatrix[1][1];
    v[2] = rotmatrix[1][2];
    normalize(v);
    x -= (v[0] * speed * speedFactor);
    y -= (v[1] * speed * speedFactor);
    z -= (v[2] * speed * speedFactor);
}


// Get billboard (face toward) rotation to target point given in local coordinates.
// Return axis and angle for rotation to accomplish billboard.
void cSpacial::getBillboard(GLfloat *target, GLfloat *rotation)
{
    int i;
    Vector v1,v2,v3;
    GLfloat v[3],d;

    // Check for coincidence.
    for (i = 0; i < 4; i++) rotation[i] = 0.0;
    if (target[0] == 0.0 && target[1] == 0.0 && target[2] == 0.0) return;

    // The axis to rotate about is the cross product
    // of the forward vector and the vector to the target.
    v1.x = target[0]; v1.y = target[1]; v1.z = target[2];
    v1.Normalize();
    getUp(v);
    v2.x = -v[0]; v2.y = -v[1]; v2.z = -v[2];
    v2.Normalize();
    v3 = v1 ^ v2;
    v3.Normalize();
    rotation[0] = v3.x; rotation[1] = v3.y; rotation[2] = v3.z;

    // The angle to rotate is the dot product of the vectors.
    d = v1 * v2;
    if (d > 0.9999)
    {
        rotation[3] = 0.0;
    }
    else
    {
        rotation[3] = acos(d);
    }
}
#endif                                            // #ifndef __SPACIAL_HPP__
