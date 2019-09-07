//***************************************************************************//
//* File Name: frustum.hpp                                                  *//
//* Author:    Tom Portegys, portegys@ilstu.edu                             *//
//* Date Made: 07/25/02                                                     *//
//* File Desc: Class declaration and implementation details                 *//
//*            representing the planes of a camera frustum.                 *//
//* Rev. Date:                                                              *//
//* Rev. Desc:                                                              *//
//*                                                                         *//
//***************************************************************************//

#ifndef __FRUSTUM_HPP__
#define __FRUSTUM_HPP__

#ifndef UNIX
#include <windows.h>
#endif
#include <GL/gl.h>
#include "math_etc.h"

class Frustum
{
    public:

        // Constructor.
        Frustum()
        {
            ExtractFrustum();
        }

        // Box/frustum bounding planes.
        struct Plane
        {
            GLfloat a,b,c,d;                      // Parameters.
            boolean inside;                       // Is box center "inside" plane?
        } planes[6];

        // Plane to point distance.
        GLfloat planeToPoint(struct Plane, Vector);

    private:

        // Extract frustum.
        void ExtractFrustum();

};

// Extracts The Current View Frustum Plane Equations
// Code from: www.markmorley.com/opengl/frustumculling.html
void
Frustum::ExtractFrustum()
{
    float   proj[16];                             // For Grabbing The PROJECTION Matrix
    float   modl[16];                             // For Grabbing The MODELVIEW Matrix
    float   clip[16];                             // Result Of Concatenating PROJECTION and MODELVIEW
    float   t;                                    // Temporary Work Variable

    glGetFloatv( GL_PROJECTION_MATRIX, proj );    // Grab The Current PROJECTION Matrix
    glGetFloatv( GL_MODELVIEW_MATRIX, modl );     // Grab The Current MODELVIEW Matrix

    // Concatenate (Multiply) The Two Matricies
    clip[ 0] = modl[ 0] * proj[ 0] + modl[ 1] * proj[ 4] + modl[ 2] * proj[ 8] + modl[ 3] * proj[12];
    clip[ 1] = modl[ 0] * proj[ 1] + modl[ 1] * proj[ 5] + modl[ 2] * proj[ 9] + modl[ 3] * proj[13];
    clip[ 2] = modl[ 0] * proj[ 2] + modl[ 1] * proj[ 6] + modl[ 2] * proj[10] + modl[ 3] * proj[14];
    clip[ 3] = modl[ 0] * proj[ 3] + modl[ 1] * proj[ 7] + modl[ 2] * proj[11] + modl[ 3] * proj[15];

    clip[ 4] = modl[ 4] * proj[ 0] + modl[ 5] * proj[ 4] + modl[ 6] * proj[ 8] + modl[ 7] * proj[12];
    clip[ 5] = modl[ 4] * proj[ 1] + modl[ 5] * proj[ 5] + modl[ 6] * proj[ 9] + modl[ 7] * proj[13];
    clip[ 6] = modl[ 4] * proj[ 2] + modl[ 5] * proj[ 6] + modl[ 6] * proj[10] + modl[ 7] * proj[14];
    clip[ 7] = modl[ 4] * proj[ 3] + modl[ 5] * proj[ 7] + modl[ 6] * proj[11] + modl[ 7] * proj[15];

    clip[ 8] = modl[ 8] * proj[ 0] + modl[ 9] * proj[ 4] + modl[10] * proj[ 8] + modl[11] * proj[12];
    clip[ 9] = modl[ 8] * proj[ 1] + modl[ 9] * proj[ 5] + modl[10] * proj[ 9] + modl[11] * proj[13];
    clip[10] = modl[ 8] * proj[ 2] + modl[ 9] * proj[ 6] + modl[10] * proj[10] + modl[11] * proj[14];
    clip[11] = modl[ 8] * proj[ 3] + modl[ 9] * proj[ 7] + modl[10] * proj[11] + modl[11] * proj[15];

    clip[12] = modl[12] * proj[ 0] + modl[13] * proj[ 4] + modl[14] * proj[ 8] + modl[15] * proj[12];
    clip[13] = modl[12] * proj[ 1] + modl[13] * proj[ 5] + modl[14] * proj[ 9] + modl[15] * proj[13];
    clip[14] = modl[12] * proj[ 2] + modl[13] * proj[ 6] + modl[14] * proj[10] + modl[15] * proj[14];
    clip[15] = modl[12] * proj[ 3] + modl[13] * proj[ 7] + modl[14] * proj[11] + modl[15] * proj[15];

    // Extract the RIGHT clipping plane
    planes[0].a = clip[ 3] - clip[ 0];
    planes[0].b = clip[ 7] - clip[ 4];
    planes[0].c = clip[11] - clip[ 8];
    planes[0].d = clip[15] - clip[12];
    planes[0].inside = false;

    // Normalize it
    t = (float) sqrt( planes[0].a * planes[0].a + planes[0].b * planes[0].b + planes[0].c * planes[0].c );
    planes[0].a /= t;
    planes[0].b /= t;
    planes[0].c /= t;
    planes[0].d /= t;

    // Extract the LEFT clipping plane
    planes[1].a = clip[ 3] + clip[ 0];
    planes[1].b = clip[ 7] + clip[ 4];
    planes[1].c = clip[11] + clip[ 8];
    planes[1].d = clip[15] + clip[12];
    planes[1].inside = false;

    // Normalize it
    t = (float) sqrt( planes[1].a * planes[1].a + planes[1].b * planes[1].b + planes[1].c * planes[1].c );
    planes[1].a /= t;
    planes[1].b /= t;
    planes[1].c /= t;
    planes[1].d /= t;

    // Extract the BOTTOM clipping plane
    planes[2].a = clip[ 3] + clip[ 1];
    planes[2].b = clip[ 7] + clip[ 5];
    planes[2].c = clip[11] + clip[ 9];
    planes[2].d = clip[15] + clip[13];
    planes[2].inside = false;

    // Normalize it
    t = (float) sqrt( planes[2].a * planes[2].a + planes[2].b * planes[2].b + planes[2].c * planes[2].c );
    planes[2].a /= t;
    planes[2].b /= t;
    planes[2].c /= t;
    planes[2].d /= t;

    // Extract the TOP clipping plane
    planes[3].a = clip[ 3] - clip[ 1];
    planes[3].b = clip[ 7] - clip[ 5];
    planes[3].c = clip[11] - clip[ 9];
    planes[3].d = clip[15] - clip[13];
    planes[3].inside = false;

    // Normalize it
    t = (float) sqrt( planes[3].a * planes[3].a + planes[3].b * planes[3].b + planes[3].c * planes[3].c );
    planes[3].a /= t;
    planes[3].b /= t;
    planes[3].c /= t;
    planes[3].d /= t;

    // Extract the FAR clipping plane
    planes[4].a = clip[ 3] - clip[ 2];
    planes[4].b = clip[ 7] - clip[ 6];
    planes[4].c = clip[11] - clip[10];
    planes[4].d = clip[15] - clip[14];
    planes[4].inside = false;

    // Normalize it
    t = (float) sqrt( planes[4].a * planes[4].a + planes[4].b * planes[4].b + planes[4].c * planes[4].c );
    planes[4].a /= t;
    planes[4].b /= t;
    planes[4].c /= t;
    planes[4].d /= t;

    // Extract the NEAR clipping plane.  This is last on purpose (see pointinfrustum() for reason)
    planes[5].a = clip[ 3] + clip[ 2];
    planes[5].b = clip[ 7] + clip[ 6];
    planes[5].c = clip[11] + clip[10];
    planes[5].d = clip[15] + clip[14];
    planes[5].inside = false;

    // Normalize it
    t = (float) sqrt( planes[5].a * planes[5].a + planes[5].b * planes[5].b + planes[5].c * planes[5].c );
    planes[5].a /= t;
    planes[5].b /= t;
    planes[5].c /= t;
    planes[5].d /= t;
}


// Plane to point distance.
GLfloat Frustum::planeToPoint(struct Plane plane, Vector point)
{
    return((plane.a * point.x) + (plane.b * point.y) + (plane.c * point.z) + plane.d);
}
#endif
