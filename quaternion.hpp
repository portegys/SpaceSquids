//***************************************************************************//
//* File Name: quaternion.hpp                                               *//
//* Author:    Tom Portegys, portegys@ilstu.edu                             *//
//* Date Made: 07/25/02                                                     *//
//* File Desc: Class declaration and implementation details                 *//
//*            representing a quaternion.                                   *//
//* Rev. Date:                                                              *//
//* Rev. Desc:                                                              *//
//*                                                                         *//
//***************************************************************************//

#ifndef __QUATERNION_HPP__
#define __QUATERNION_HPP__

#include <GL/gl.h>

class cQuaternion
{

    public:

        // Quaternion.
        GLfloat quat[4];

        // Constructor.
        cQuaternion(GLfloat q[4])
        {
            quat[0] = q[0];
            quat[1] = q[1];
            quat[2] = q[2];
            quat[3] = q[3];
        }

        // Vector operations.
        void vzero(GLfloat *);
        void vset(GLfloat *, GLfloat, GLfloat, GLfloat);
        void vsub(const GLfloat *, const GLfloat *, GLfloat *);
        void vcopy(const GLfloat *, GLfloat *);
        void vcross(const GLfloat *, const GLfloat *, GLfloat *);
        GLfloat vlength(const GLfloat *);
        void vscale(GLfloat *, GLfloat);
        void vnormal(GLfloat *);
        GLfloat vdot(const GLfloat *, const GLfloat *);
        void vadd(const GLfloat *, const GLfloat *, GLfloat *);

        // Clear.
        void clear()
        {
            quat[0] = 0.0;
            quat[1] = 0.0;
            quat[2] = 0.0;
            quat[3] = 1.0;
        }

        // Given an axis and angle, compute quaternion.
        void axis_to_quat(GLfloat a[3], GLfloat phi, GLfloat q[4]);

        // Add quaternions.
        void add_quats(GLfloat q1[4], GLfloat q2[4], GLfloat dest[4]);

        // Multiply quaternions.
        void mult_quats(GLfloat q1[4], GLfloat q2[4], GLfloat dest[4]);

        // Normalize a quaternion.
        void normalize_quat(GLfloat q[4]);

        // Build a rotation matrix, given a quaternion rotation.
        void build_rotmatrix(GLfloat m[4][4], GLfloat q[4]);

        // Load an axis-angle rotation into quaternion.
        void loadRotation(GLfloat angle, GLfloat *axis);

        // Merge an axis-angle rotation into quaternion.
        void mergeRotation(GLfloat angle, GLfloat *axis);

    private:

        int NormalCount;                          // Normalization counter.

};

void cQuaternion::vzero(GLfloat *v)
{
    v[0] = 0.0;
    v[1] = 0.0;
    v[2] = 0.0;
}


void cQuaternion::vset(GLfloat *v, GLfloat x, GLfloat y, GLfloat z)
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
}


void cQuaternion::vsub(const GLfloat *src1, const GLfloat *src2, GLfloat *dst)
{
    dst[0] = src1[0] - src2[0];
    dst[1] = src1[1] - src2[1];
    dst[2] = src1[2] - src2[2];
}


void cQuaternion::vcopy(const GLfloat *v1, GLfloat *v2)
{
    register int i;

    for (i = 0 ; i < 3 ; i++)
        v2[i] = v1[i];
}


void cQuaternion::vcross(const GLfloat *v1, const GLfloat *v2, GLfloat *cross)
{
    GLfloat temp[3];

    temp[0] = (v1[1] * v2[2]) - (v1[2] * v2[1]);
    temp[1] = (v1[2] * v2[0]) - (v1[0] * v2[2]);
    temp[2] = (v1[0] * v2[1]) - (v1[1] * v2[0]);

    vcopy(temp, cross);
}


GLfloat cQuaternion::vlength(const GLfloat *v)
{
    return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}


void cQuaternion::vscale(GLfloat *v, GLfloat div)
{
    v[0] *= div;
    v[1] *= div;
    v[2] *= div;
}


void cQuaternion::vnormal(GLfloat *v)
{
    vscale(v,1.0/vlength(v));
}


GLfloat cQuaternion::vdot(const GLfloat *v1, const GLfloat *v2)
{
    return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}


void cQuaternion::vadd(const GLfloat *src1, const GLfloat *src2, GLfloat *dst)
{
    dst[0] = src1[0] + src2[0];
    dst[1] = src1[1] + src2[1];
    dst[2] = src1[2] + src2[2];
}


/*
 *  Given an axis and angle, compute quaternion.
 */
void cQuaternion::axis_to_quat(GLfloat a[3], GLfloat phi, GLfloat q[4])
{
    vnormal(a);
    vcopy(a,q);
    vscale(q,sin(phi/2.0));
    q[3] = cos(phi/2.0);
}


/*
 * Add quaternions, normalizing periodically.
 */

#define NORMALFREQ 100

void cQuaternion::add_quats(GLfloat q1[4], GLfloat q2[4], GLfloat dest[4])
{
    GLfloat t1[4], t2[4], t3[4];
    GLfloat tf[4];

    vcopy(q1,t1);
    vscale(t1,q2[3]);
    vcopy(q2,t2);
    vscale(t2,q1[3]);
    vcross(q2,q1,t3);
    vadd(t1,t2,tf);
    vadd(t3,tf,tf);
    tf[3] = q1[3] * q2[3] - vdot(q1,q2);
    dest[0] = tf[0];
    dest[1] = tf[1];
    dest[2] = tf[2];
    dest[3] = tf[3];

    if (++NormalCount > NORMALFREQ)
    {
        NormalCount = 0;
        normalize_quat(dest);
    }
}


/*
 * Multiply quaternions, normalizing periodically.
 */

void cQuaternion::mult_quats(GLfloat q1[4], GLfloat q2[4], GLfloat dest[4])
{
    dest[ 3 ] =   q2[ 3 ] * q1[ 3 ]
        - q2[ 0 ] * q1[ 0 ]
        - q2[ 1 ] * q1[ 1 ]
        - q2[ 2 ] * q1[ 2 ];

    dest[ 0 ] =   q2[ 3 ] * q1[ 0 ]
        + q2[ 0 ] * q1[ 3 ]
        + q2[ 1 ] * q1[ 2 ]
        - q2[ 2 ] * q1[ 1 ];

    dest[ 1 ] =   q2[ 3 ] * q1[ 1 ]
        - q2[ 0 ] * q1[ 2 ]
        + q2[ 1 ] * q1[ 3 ]
        + q2[ 2 ] * q1[ 0 ];

    dest[ 2 ] =   q2[ 3 ] * q1[ 2 ]
        + q2[ 0 ] * q1[ 1 ]
        - q2[ 1 ] * q1[ 0 ]
        + q2[ 2 ] * q1[ 3 ];

    if (++NormalCount > NORMALFREQ)
    {
        NormalCount = 0;
        normalize_quat(dest);
    }
}


/*
 * Quaternions always obey:  a^2 + b^2 + c^2 + d^2 = 1.0
 * If they don't add up to 1.0, dividing by their magnitude will
 * renormalize them.
 *
 * Note: See the following for more information on quaternions:
 *
 * - Shoemake, K., Animating rotation with quaternion curves, Computer
 *   Graphics 19, No 3 (Proc. SIGGRAPH'85), 245-254, 1985.
 * - Pletinckx, D., Quaternion calculus as a basic tool in computer
 *   graphics, The Visual Computer 5, 2-13, 1989.
 */

void cQuaternion::normalize_quat(GLfloat q[4])
{
    int i;
    GLfloat mag;

    mag = (q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
    for (i = 0; i < 4; i++) q[i] /= mag;
}


/*
 * Build a rotation matrix, given a quaternion rotation.
 */

void cQuaternion::build_rotmatrix(GLfloat m[4][4], GLfloat q[4])
{
    m[0][0] = 1.0 - 2.0 * (q[1] * q[1] + q[2] * q[2]);
    m[0][1] = 2.0 * (q[0] * q[1] - q[2] * q[3]);
    m[0][2] = 2.0 * (q[2] * q[0] + q[1] * q[3]);
    m[0][3] = 0.0;

    m[1][0] = 2.0 * (q[0] * q[1] + q[2] * q[3]);
    m[1][1]= 1.0 - 2.0 * (q[2] * q[2] + q[0] * q[0]);
    m[1][2] = 2.0 * (q[1] * q[2] - q[0] * q[3]);
    m[1][3] = 0.0;

    m[2][0] = 2.0 * (q[2] * q[0] - q[1] * q[3]);
    m[2][1] = 2.0 * (q[1] * q[2] + q[0] * q[3]);
    m[2][2] = 1.0 - 2.0 * (q[1] * q[1] + q[0] * q[0]);
    m[2][3] = 0.0;

    m[3][0] = 0.0;
    m[3][1] = 0.0;
    m[3][2] = 0.0;
    m[3][3] = 1.0;
}


// Load axis-angle rotation into quaternion.
void cQuaternion::loadRotation(GLfloat angle, GLfloat *axis)
{
    quat[0] = axis[0] * sin(angle / 2.0);
    quat[1] = axis[1] * sin(angle / 2.0);
    quat[2] = axis[2] * sin(angle / 2.0);
    quat[3] = cos(angle / 2.0);
    normalize_quat(quat);
}


// Merge an axis-angle rotation into quaternion.
void cQuaternion::mergeRotation(GLfloat angle, GLfloat *axis)
{
    GLfloat q1[4],q2[4];

    q2[0] = axis[0] * sin(angle / 2.0);
    q2[1] = axis[1] * sin(angle / 2.0);
    q2[2] = axis[2] * sin(angle / 2.0);
    q2[3] = cos(angle / 2.0);
    normalize_quat(q2);
    q1[0] = quat[0];
    q1[1] = quat[1];
    q1[2] = quat[2];
    q1[3] = quat[3];
    mult_quats(q1, q2, quat);
    normalize_quat(quat);
}
#endif
