//***************************************************************************//
//* File Name: tentacle.hpp                                                 *//
//* Author:    Tom Portegys, portegys@ilstu.edu                             *//
//* Date Made: 09/17/02                                                     *//
//* File Desc: Class declaration and implementation details                 *//
//*            representing a squid tentacle.                               *//
//* Rev. Date:                                                              *//
//* Rev. Desc:                                                              *//
//*                                                                         *//
//***************************************************************************//

#ifndef __TENTACLE_HPP__
#define __TENTACLE_HPP__

#include "game_object.hpp"
#include "physics.h"

// Tentacle segments. See BuildSegmentTransforms() for hard-coded dependency.
#define NUM_TENTACLE_SEGMENTS 20

// Number of undulation states.
#define NUM_TENTACLE_UNDULATIONS 10

// Tentacle programmable displays.
#define NUM_TENTACLE_PROGRAMMABLE_DISPLAYS 10

// Tentacle static displays.
#define NUM_TENTACLE_STATIC_DISPLAYS (NUM_TENTACLE_UNDULATIONS + NUM_TENTACLE_PROGRAMMABLE_DISPLAYS)

struct TentacleSegmentTransform
{
    GLfloat pitch,yaw,roll;                       // Rotation.
    GLfloat x,y,z;                                // Translation.
    GLfloat scale;                                // Scale.
};

struct TentacleConfiguration
{
    GLint display;
    RigidBody segmentBoundingBox[NUM_TENTACLE_SEGMENTS];
};

// Tentacle model.
#include "tentacle_model.h"

class Tentacle : public cGameObject
{

    public:

        // Constructor.
        Tentacle(class Squid *squid)
        {
            int i,j,n;
            GLfloat m[16];
            GLubyte texture[2][2][3];

            m_type = TENTACLE;
            m_disposition = HOSTILE;
            mySquid = squid;

            // Initialize segment transforms.
            segmentSize = TentacleDimensions[2].delta / NUM_TENTACLE_SEGMENTS;
            for (i = 0; i < NUM_TENTACLE_SEGMENTS; i++)
            {
                segmentTransform[i].pitch = 0.0;
                segmentTransform[i].yaw = 0.0;
                segmentTransform[i].roll = 0.0;
                segmentTransform[i].x = 0.0;
                segmentTransform[i].y = 0.0;
                segmentTransform[i].z = 0.0;
                segmentTransform[i].scale = 1.0;
            }
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();
            glGetFloatv( GL_MODELVIEW_MATRIX, m );
            for (n = 0; n < NUM_TENTACLE_SEGMENTS; n++)
            {
                for (i = 0; i < 16; i++) segmentTransformMatrix[n][i] = m[i];
            }
            glPopMatrix();
            dynamicDisplay = -1;
            segmentDynamicTransformValid = false;

            // Create default segment bounding boxes.
            createSegmentBounds(segmentBoundingBox);

            // Show bounding boxes?
            showBounds = false;

            // Initialize static data.
            if (staticInit == false)
            {
                for (i = 0; i < NUM_TENTACLE_STATIC_DISPLAYS; i++)
                {
                    staticConfig[i].display = -1;
                }
                staticInit = true;

                // Create undulation displays.
                createUndulationDisplays();

                // Create texture.
                for (i = 0; i < 2; i++)
                {
                    for (j = 0; j < 2; j++)
                    {
                        texture[i][j][0] = 200;   // Red
                        texture[i][j][1] = 255;   // Green
                        texture[i][j][2] = 200;   // Blue
                    }
                }
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glGenTextures(1, &textureName);
                glBindTexture(GL_TEXTURE_2D, textureName);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexImage2D(GL_TEXTURE_2D, 0, 3, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &(texture[0][0][0]));
            }
        }

        // Go: update and draw.
        void Go() { Update(); Draw(); }

        // Update.
        void Update() { m_spacial->update(); }

        // Draw.
        void Draw();                              // Dynamic.
        void Draw(int);                           // Static.

        void Kill() { m_isAlive = false; }

        // Set kinematic segment transforms.
        void SetSegmentTransforms(struct TentacleSegmentTransform *transforms)
        {
            int i;

            segmentDynamicTransformValid = false;
            for (i = 0; i < NUM_TENTACLE_SEGMENTS; i++)
            {
                segmentTransform[i] = transforms[i];
            }
        }

        // Build kinematic segment transforms.
        void BuildSegmentTransforms(int);

        // Build tentacle configuration to grasp target's bounding boxes.
        void BuildGrasp(int targetIndex);

        // Show bounding boxes.
        void showBoundingBoxes(bool b) { showBounds = b; }

    private:

        // My squid.
        class Squid *mySquid;

        // Create undulation displays.
        void createUndulationDisplays();

        // Segment transforms.
        bool segmentDynamicTransformValid;
        GLint dynamicDisplay;
        GLfloat segmentSize;
        struct TentacleSegmentTransform segmentTransform[NUM_TENTACLE_SEGMENTS];
        GLfloat segmentTransformMatrix[NUM_TENTACLE_SEGMENTS][16];
        RigidBody segmentBoundingBox[NUM_TENTACLE_SEGMENTS];
        void buildSegmentTransformMatrices();
        void createSegmentBounds(RigidBody *);
        bool showBounds;

        // Prepared tentacle configurations.
        static struct TentacleConfiguration staticConfig[NUM_TENTACLE_STATIC_DISPLAYS];
        static bool staticInit;

        // Texture.
        static GLuint textureName;
};

// Prepared tentacle configurations.
struct TentacleConfiguration Tentacle::staticConfig[NUM_TENTACLE_STATIC_DISPLAYS];
bool Tentacle::staticInit = false;

// Texture.
GLuint Tentacle::textureName;

// Create undulation displays.
void Tentacle::createUndulationDisplays()
{
    int i,j,u;
    GLfloat x,y,z,mz,dz,a,s,amp,freq;
    GLint lid;
    int mcount,mindex;

    mz = TentacleDimensions[2].min;
    dz = TentacleDimensions[2].delta;
    for (u = 0; u < NUM_TENTACLE_UNDULATIONS; u++)
    {
        // Create segment bounding boxes.
        for (i = 0; i < NUM_TENTACLE_SEGMENTS; i++)
        {
            staticConfig[u].segmentBoundingBox[i] = segmentBoundingBox[i];
        }

        lid=glGenLists(1);
        mcount=0;
        mindex=0;
        glNewList(lid, GL_COMPILE);

        glBegin (GL_TRIANGLES);
        for(i=0;i<sizeof(tentacle_face_indicies)/sizeof(tentacle_face_indicies[0]);i++)
        {
            if(!mcount)
            {
                tentacle_SelectMaterial(tentacle_material_ref[mindex][0]);
                mcount=tentacle_material_ref[mindex][1];
                mindex++;
            }
            mcount--;
            for(j=0;j<3;j++)
            {
                int vi=tentacle_face_indicies[i][j];
                                                  //Normal index
                int ni=tentacle_face_indicies[i][j+3];
                                                  //Texture index
                int ti=tentacle_face_indicies[i][j+6];
                glNormal3f (tentacle_normals[ni][0],tentacle_normals[ni][1],tentacle_normals[ni][2]);
                glTexCoord2f(tentacle_textures[ti][0],tentacle_textures[ti][1]);

                // Shift x and y according to z using a shifted sine wave.
                amp = 60.0;                       // amplitude of wave: larger = smaller wave.
                freq = 360.0;                     // frequency: greater = more "wiggles"
                x = tentacle_vertices[vi][0];
                y = tentacle_vertices[vi][1];
                z = tentacle_vertices[vi][2];
                a = (z + mz) * (freq / dz);
                a += (GLfloat)u * (freq / (GLfloat)NUM_TENTACLE_UNDULATIONS);
                s = (sin(a * (M_PI / 180.0)) + amp) / amp;
                x += (s - 1.0);
                y += (s - 1.0);
                glVertex3f (x, y, z);
            }
        }
        glEnd ();

        glEndList();
        staticConfig[u].display = lid;
    }
}


// Draw statically prepared tentacle.
void Tentacle::Draw(int n)
{
    // Dynamic display?
    if (n == -1)
    {
        Draw();
        return;
    }

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glTranslatef(m_spacial->x, m_spacial->y, m_spacial->z);
    glMultMatrixf(&m_spacial->rotmatrix[0][0]);
    glScalef(m_spacial->scale, m_spacial->scale, m_spacial->scale);

    // Set smooth texture mapping and lighting.
    glShadeModel(GL_SMOOTH);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_LIGHTING);

    // Draw tentacle.
    glBindTexture(GL_TEXTURE_2D, textureName);
    glCallList(staticConfig[n].display);

    // Draw segment bounding boxes?
    if (showBounds)
    {
        for (int i = 0; i < NUM_TENTACLE_SEGMENTS; i++)
        {
            DrawBody(&(staticConfig[n].segmentBoundingBox[i]));
        }
    }

    glPopMatrix();
}


// Draw dynamically alterable tentacle.
void Tentacle::Draw()
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glTranslatef(m_spacial->x, m_spacial->y, m_spacial->z);
    glMultMatrixf(&m_spacial->rotmatrix[0][0]);
    glScalef(m_spacial->scale, m_spacial->scale, m_spacial->scale);

    // Build segment transform matrices?
    if (segmentDynamicTransformValid == false)
    {
        BuildSegmentTransforms(-1);
    }

    // Set smooth texture mapping and lighting.
    glShadeModel(GL_SMOOTH);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_LIGHTING);

    // Draw tentacle.
    glBindTexture(GL_TEXTURE_2D, textureName);
    glCallList(dynamicDisplay);

    // Draw segment bounding boxes?
    if (showBounds)
    {
        for (int i = 0; i < NUM_TENTACLE_SEGMENTS; i++)
        {
            DrawBody(&(segmentBoundingBox[i]));
        }
    }

    glPopMatrix();
}


// Build segment transforms.
void Tentacle::BuildSegmentTransforms(int index)
{
    int i,j,k,n,mcount,mindex,vi,ni,ti;
    GLfloat z;
    Matrix p(4,1),t(4,1);

    // Yuck: trouble constructing Matrix in an array.
    Matrix x0(4,4),x1(4,4),x2(4,4),x3(4,4),x4(4,4);
    Matrix x5(4,4),x6(4,4),x7(4,4),x8(4,4),x9(4,4);
    Matrix x10(4,4),x11(4,4),x12(4,4),x13(4,4),x14(4,4);
    Matrix x15(4,4),x16(4,4),x17(4,4),x18(4,4),x19(4,4);

    if (index < -1 || index >= NUM_TENTACLE_STATIC_DISPLAYS) return;

    // Build transform matrices.
    buildSegmentTransformMatrices();

    // Create segment bounding boxes.
    if (index == -1)
    {
        createSegmentBounds(segmentBoundingBox);
    }
    else
    {
        createSegmentBounds(staticConfig[index].segmentBoundingBox);
    }

    // Build matrices.
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x0(i,j) = segmentTransformMatrix[0][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x1(i,j) = segmentTransformMatrix[1][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x2(i,j) = segmentTransformMatrix[2][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x3(i,j) = segmentTransformMatrix[3][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x4(i,j) = segmentTransformMatrix[4][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x5(i,j) = segmentTransformMatrix[5][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x6(i,j) = segmentTransformMatrix[6][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x7(i,j) = segmentTransformMatrix[7][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x8(i,j) = segmentTransformMatrix[8][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x9(i,j) = segmentTransformMatrix[9][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x10(i,j) = segmentTransformMatrix[10][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x11(i,j) = segmentTransformMatrix[11][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x12(i,j) = segmentTransformMatrix[12][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x13(i,j) = segmentTransformMatrix[13][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x14(i,j) = segmentTransformMatrix[14][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x15(i,j) = segmentTransformMatrix[15][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x16(i,j) = segmentTransformMatrix[16][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x17(i,j) = segmentTransformMatrix[17][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x18(i,j) = segmentTransformMatrix[18][(j*4)+i];
    for (i=0; i < 4; i++) for (j=0; j < 4; j++) x19(i,j) = segmentTransformMatrix[19][(j*4)+i];

    // Transform segment vertices.
    for(i=0,k=sizeof(tentacle_face_indicies)/sizeof(tentacle_face_indicies[0]);i<k;i++)
    {
        for(j=0;j<3;j++)
        {
            vi=tentacle_face_indicies[i][j];
            z = tentacle_vertices[vi][2];
            for (n = 0; n < NUM_TENTACLE_SEGMENTS; n++)
            {
                if (z >= (TentacleDimensions[2].min + ((GLfloat)n * segmentSize)) &&
                    z < (TentacleDimensions[2].min + ((GLfloat)(n + 1) * segmentSize))) break;
            }
            if (n == NUM_TENTACLE_SEGMENTS) n--;
            n = NUM_TENTACLE_SEGMENTS - n - 1;
            p(0,0) = tentacle_vertices[vi][0];
            p(1,0) = tentacle_vertices[vi][1];
            p(2,0) = tentacle_vertices[vi][2];
            p(3,0) = 1.0;
            switch(n)
            {
                case 0: t = x0 * p; break;
                case 1: t = x1 * p; break;
                case 2: t = x2 * p; break;
                case 3: t = x3 * p; break;
                case 4: t = x4 * p; break;
                case 5: t = x5 * p; break;
                case 6: t = x6 * p; break;
                case 7: t = x7 * p; break;
                case 8: t = x8 * p; break;
                case 9: t = x9 * p; break;
                case 10: t = x10 * p; break;
                case 11: t = x11 * p; break;
                case 12: t = x12 * p; break;
                case 13: t = x13 * p; break;
                case 14: t = x14 * p; break;
                case 15: t = x15 * p; break;
                case 16: t = x16 * p; break;
                case 17: t = x17 * p; break;
                case 18: t = x18 * p; break;
                case 19: t = x19 * p; break;
            }
            xtentacle_vertices[vi][0] = t(0,0);
            xtentacle_vertices[vi][1] = t(1,0);
            xtentacle_vertices[vi][2] = t(2,0);
        }
    }

    // Remove old display list.
    if (index == -1)
    {
        if (dynamicDisplay != -1) glDeleteLists(dynamicDisplay, 1);
    }
    else
    {
        if (staticConfig[index].display != -1) glDeleteLists(staticConfig[index].display, 1);
    }

    // Build display list.
    if (index == -1)
    {
        dynamicDisplay=glGenLists(1);
        glNewList(dynamicDisplay, GL_COMPILE);
    }
    else
    {
        staticConfig[index].display=glGenLists(1);
        glNewList(staticConfig[index].display, GL_COMPILE);
    }
    mcount=0;
    mindex=0;
    glBegin (GL_TRIANGLES);
    for(i=0,k=sizeof(tentacle_face_indicies)/sizeof(tentacle_face_indicies[0]);i<k;i++)
    {
        if(!mcount)
        {
            tentacle_SelectMaterial(tentacle_material_ref[mindex][0]);
            mcount=tentacle_material_ref[mindex][1];
            mindex++;
        }
        mcount--;
        for(j=0;j<3;j++)
        {
            vi=tentacle_face_indicies[i][j];
            ni=tentacle_face_indicies[i][j+3];    //Normal index
            ti=tentacle_face_indicies[i][j+6];    //Texture index
            glNormal3f (tentacle_normals[ni][0],tentacle_normals[ni][1],tentacle_normals[ni][2]);
            glTexCoord2f(tentacle_textures[ti][0],tentacle_textures[ti][1]);
            glVertex3f(xtentacle_vertices[vi][0],xtentacle_vertices[vi][1],xtentacle_vertices[vi][2]);
        }
    }
    glEnd ();
    glEndList();

    // Dynamic transform now valid.
    if (index == -1) segmentDynamicTransformValid = true;
}


// Build segment transform matrices.
void Tentacle::buildSegmentTransformMatrices()
{
    int n;

    glMatrixMode(GL_MODELVIEW);
    for (n = 0; n < NUM_TENTACLE_SEGMENTS; n++)
    {
        glPushMatrix();
        if (n == 0) glLoadIdentity();
        glTranslatef(segmentTransform[n].x, segmentTransform[n].y, segmentTransform[n].z);
        glTranslatef(0.0, 0.0, (segmentSize * (GLfloat)(NUM_TENTACLE_SEGMENTS - (2 * n)))/2.0);
        glRotatef(segmentTransform[n].pitch, 1.0, 0.0, 0.0);
        glRotatef(segmentTransform[n].yaw, 0.0, 1.0, 0.0);
        glRotatef(segmentTransform[n].roll, 0.0, 0.0, 1.0);
        glTranslatef(0.0, 0.0, -(segmentSize * (GLfloat)(NUM_TENTACLE_SEGMENTS - (2 * n)))/2.0);
        glScalef(segmentTransform[n].scale, segmentTransform[n].scale, segmentTransform[n].scale);
        glGetFloatv( GL_MODELVIEW_MATRIX, segmentTransformMatrix[n] );
    }
    for (n = 0; n < NUM_TENTACLE_SEGMENTS; n++) glPopMatrix();
}


// Create tentacle segments as a string of bounding boxes.
void Tentacle::createSegmentBounds(RigidBody *boxes)
{
    int i,j,k;
    Matrix x(4,4),p(4,1),t(4,1),v(4,1);

    // Radius of each box is half segment size.
    p(0,0) = 0.0;
    p(1,0) = 0.0;
    p(2,0) = TentacleDimensions[2].max - (segmentSize * 0.5);
    p(3,0) = 1.0;

    // Create transformed boxes.
    for (i = 0; i < NUM_TENTACLE_SEGMENTS; i++, p(2,0) -= segmentSize)
    {
        for (k=0; k < 4; k++)
            for (j=0; j < 4; j++)
                x(k,j) = segmentTransformMatrix[i][(j*4)+k];
        InitializeObject(&boxes[i], segmentSize, BLOCK_TYPE, -1);
        for(j=0; j<8; j++)
        {
            v(0,0) = boxes[i].vVertexList[j].x;
            v(1,0) = boxes[i].vVertexList[j].y;
            v(2,0) = boxes[i].vVertexList[j].z + p(2,0);
            v(3,0) = 1.0;
            t = x * v;
            boxes[i].vVertexList[j].x = t(0,0);
            boxes[i].vVertexList[j].y = t(1,0);
            boxes[i].vVertexList[j].z = t(2,0);
        }
    }
}


// Build tentacle configuration to grasp target's bounding boxes.
void Tentacle::BuildGrasp(int targetIndex)
{
    int i,n,segment;
    GLfloat angle,range,tolerance,v[3];

    // Clear segment transforms.
    for (i = 0; i < NUM_TENTACLE_SEGMENTS; i++)
    {
        segmentTransform[i].pitch = 0.0;
        segmentTransform[i].yaw = 0.0;
        segmentTransform[i].roll = 0.0;
        segmentTransform[i].x = 0.0;
        segmentTransform[i].y = 0.0;
        segmentTransform[i].z = 0.0;
        segmentTransform[i].scale = 1.0;
    }
    segmentTransform[7].yaw = 20;
    segmentTransform[10].yaw = -20;
    segmentTransform[15].yaw = -20;

    /*
     * For each segment, bend tentacle down, proceeding from
     * +range to -range degrees until one of the segments collides
     * with a bounding box. This sets the grasping angle of the
     * current segment up to the colliding segment. Continue the
     * process with the next segment to effect a wrap-around
     * configuration.
     */
    range = 45.0;
    tolerance = 0.2;
    for (segment = 0; segment < NUM_TENTACLE_SEGMENTS;)
    {
        if (segment > 0) return;                  // For efficiency.
        for (angle = range; angle >= -range; angle -= 5)
        {
            // Create the bounding boxes for the segments.
            segmentTransform[segment].yaw = angle;
            buildSegmentTransformMatrices();
            createSegmentBounds(segmentBoundingBox);

            // Check for collisions with the target's bounding boxes.
            for (n = segment; n < NUM_TENTACLE_SEGMENTS; n++)
            {
                InitializeObject(&Bodies[MAX_BODIES], segmentSize, BLOCK_TYPE, -1);
                for (i = 0; i < 8; i++)
                {
                    // Set box vertices to segment world coordinates.
                    v[0] = segmentBoundingBox[n].vVertexList[i].x;
                    v[1] = segmentBoundingBox[n].vVertexList[i].y;
                    v[2] = segmentBoundingBox[n].vVertexList[i].z;
                    LocalToWorld(v, v);
                    Bodies[MAX_BODIES].vVertexList[i].x = v[0];
                    Bodies[MAX_BODIES].vVertexList[i].y = v[1];
                    Bodies[MAX_BODIES].vVertexList[i].z = v[2];
                }

                // Segment collides with target?
                for (i = targetIndex; Bodies[i].group == Bodies[targetIndex].group && i < NumBodies; i++)
                {
                    if (CheckForSpecificCollision(MAX_BODIES, i, tolerance) == COLLISION)
                    {
                        segment = n + 1;
                        n = NUM_TENTACLE_SEGMENTS;
                        break;
                    }
                }
            }
        }
        if (angle < -range) break;
    }
}
#endif
