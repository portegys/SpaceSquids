//***************************************************************************//
//* File Name: squid.hpp                                                    *//
//* Author:    Tom Portegys, portegys@ilstu.edu                             *//
//* Date Made: 09/18/02                                                     *//
//* File Desc: Class declaration and implementation details                 *//
//*            representing a space squid.                                  *//
//* Rev. Date:                                                              *//
//* Rev. Desc:                                                              *//
//*                                                                         *//
//***************************************************************************//

#ifndef __SQUID_HPP__
#define __SQUID_HPP__

#include "game_object.hpp"
#include "tentacle.hpp"

// Number of undulation states.
#define NUM_SQUID_UNDULATIONS NUM_TENTACLE_UNDULATIONS

// Number of tentacle extension positions.
#define NUM_TENTACLE_EXTENSIONS NUM_TENTACLE_PROGRAMMABLE_DISPLAYS

// Exploding part.
struct SQUID_EXPLODING_PART
{
    Vector position;
    Vector velocity;
    float  angle;
    Vector axis;
    float angularVelocity;
};

class Squid : public cGameObject
{

    public:

        // States.
        static const int IDLE;
        static const int ATTACK;
        static const int DESTROY;
        static const int EXPLODE;
        static const int DEAD;
        int state;

        // Constructor.
        Squid()
        {
            int i;

            m_type = SQUID;
            m_disposition = HOSTILE;

            // Create tentacles.
            tentacles[0] = new Tentacle(this);
            tentacles[1] = new Tentacle(this);
            tentacles[2] = new Tentacle(this);

            // Create display lists for body.
            createModelDrawables();

            // Set tentacle display states.
            for (i = 0; i < 3; i++) tentacleDisplayIndex[i] = 0;
            extensionCount = -1;
            moveCount = 0;
            undulateIndex = undulateCount = 0;

            // Set state.
            Idle();
        }

        // Go: update and draw.
        void Go() { Update(); Draw(); }

        // Update.
        void Update()
        {
            if (state == IDLE) IdleUpdate();
            if (state == ATTACK) AttackUpdate();
            if (state == DESTROY) DestroyUpdate();
            if (state == EXPLODE) ExplodeUpdate();
        }

        // Draw.
        void Draw();

        // Kill.
        void Kill() { state = DEAD; m_isAlive = false; }

        // Undulate?
        void Undulate(bool b) { undulate = b; }

        // Idle state.
        void Idle()
        {
            state = IDLE;
            target = NULL;
            oriented = false;
            undulate = true;
        }
        void IdleUpdate();

        // Is squid idle?
        bool IsIdle()
        {
            if (state == IDLE) return(true); else return(false);
        }

        // Attack a game object.
        // This causes the squid to move toward the object and wrap
        // its tentacles around its bounding objects.
        // In multi-squid attacks, a squid may have to wrap
        // over other squids' bodies.
        void Attack(cGameObject *target)
        {
            state = ATTACK;
            this->target = target;
            undulate = false;
        }
        void AttackUpdate();

        // Is squid attacking target?
        bool IsAttacking()
        {
            if (state == ATTACK) return(true); else return(false);
        }

        // Is squid oriented toward target?
        bool IsOriented() { return(oriented); }

        // Get target.
        cGameObject *GetTarget() { return(target); }

        // Destroy target.
        void Destroy(int targetIndex);
        void DestroyUpdate();

        // Is squid destroying target?
        bool IsDestroying()
        {
            if (state == DESTROY) return(true); else return(false);
        }

        // Exploding state.
        void Explode();
        void ExplodeUpdate();

        // Is squid exploding?
        bool IsExploding()
        {
            if (state == EXPLODE) return(true); else return(false);
        }

    private:

        // Tentacles.
        class Tentacle *tentacles[3];

        // Create model display list.
        static GLint outerBodyDisplay, gutsDisplay;
        static GLint outerUndulatingBody[NUM_SQUID_UNDULATIONS];
        static bool displayInit;
        void createModelDrawables();

        // Textures.
        static GLuint outerTextureName;
        static GLuint gutsTextureName;

        // Tentacle display states.
        int tentacleDisplayIndex[3];

        // Idle controls.
        bool undulate;
        int undulateIndex;
        static const int undulateFreq;
        int undulateCount;
        static const GLfloat idleSpeed;
        static const int changeDirectionFreq;
        int moveCount;

        // Attack controls.
        static const GLfloat attackSpeed;
        static const GLfloat rotateSpeed;
        cGameObject *target;
        int extensionCount;
        bool oriented;

        // Explosion controls.
        float explosionCounter;
        static const float explosionDelay;
        static const float maxExplosionVelocity;
        static const float maxExplosionAngularVelocity;
                                                  // Outer body, guts, and three tentacles.
        struct SQUID_EXPLODING_PART explodingParts[5];
        void explosionTransform(int);
};

// States.
const int Squid::IDLE = 0;
const int Squid::ATTACK = 1;
const int Squid::DESTROY = 2;
const int Squid::EXPLODE = 3;
const int Squid::DEAD = 4;

// Displays.
GLint Squid::outerBodyDisplay;
GLint Squid::gutsDisplay;
GLint Squid::outerUndulatingBody[NUM_SQUID_UNDULATIONS];
bool Squid::displayInit = false;

// Textures.
GLuint Squid::outerTextureName;
GLuint Squid::gutsTextureName;

// Idle parameters.
const int Squid::undulateFreq = 10;
const GLfloat Squid::idleSpeed = -0.05;
const int Squid::changeDirectionFreq = 10;

// Attack parameters.
const GLfloat Squid::attackSpeed = 0.1;
const GLfloat Squid::rotateSpeed = 3.0;           // degrees

// Explosion parameters.
const float Squid::explosionDelay = 100.0;
const float Squid::maxExplosionVelocity = 0.05;
const float Squid::maxExplosionAngularVelocity = 3.0;

// Squid models.
#include "squid_outer_body.h"
#include "squid_guts.h"

// Create model display list.
void Squid::createModelDrawables()
{
    int i,j,u;
    GLfloat x,y,z,my,dy,a,s,amp,freq;
    GLubyte texture[2][2][3];
    struct TentacleSegmentTransform segmentTransform[NUM_TENTACLE_SEGMENTS];

    // Already created?
    if (displayInit == true) return;
    displayInit = true;

    // Create outer body texture.
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 2; j++)
        {
            texture[i][j][0] = 200;               // Red
            texture[i][j][1] = 255;               // Green
            texture[i][j][2] = 200;               // Blue
        }
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &outerTextureName);
    glBindTexture(GL_TEXTURE_2D, outerTextureName);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &(texture[0][0][0]));

    // Outer body.
    GLint lid=glGenLists(1);
    int mcount=0;
    int mindex=0;
    glNewList(lid, GL_COMPILE);

    glBegin (GL_TRIANGLES);
    for(i=0;i<sizeof(squid_outer_face_indicies)/sizeof(squid_outer_face_indicies[0]);i++)
    {
        if(!mcount)
        {
            squid_outer_SelectMaterial(squid_outer_material_ref[mindex][0]);
            mcount=squid_outer_material_ref[mindex][1];
            mindex++;
        }
        mcount--;
        for(j=0;j<3;j++)
        {
            int vi=squid_outer_face_indicies[i][j];
                                                  //Normal index
            int ni=squid_outer_face_indicies[i][j+3];
                                                  //Texture index
            int ti=squid_outer_face_indicies[i][j+6];
            glNormal3f (squid_outer_normals[ni][0],squid_outer_normals[ni][1],squid_outer_normals[ni][2]);
            glTexCoord2f(squid_outer_textures[ti][0],squid_outer_textures[ti][1]);
            glVertex3f (squid_outer_vertices[vi][0],squid_outer_vertices[vi][1],squid_outer_vertices[vi][2]);
        }
    }
    glEnd ();

    glEndList();
    outerBodyDisplay = lid;

    // Create undulating outer bodies.
    my = SquidOuterDimensions[1].min;
    dy = SquidOuterDimensions[1].delta;
    for (u = 0; u < NUM_SQUID_UNDULATIONS; u++)
    {
        lid=glGenLists(1);
        mcount=0;
        mindex=0;
        glNewList(lid, GL_COMPILE);

        glBegin (GL_TRIANGLES);
        for(i=0;i<sizeof(squid_outer_face_indicies)/sizeof(squid_outer_face_indicies[0]);i++)
        {
            if(!mcount)
            {
                squid_outer_SelectMaterial(squid_outer_material_ref[mindex][0]);
                mcount=squid_outer_material_ref[mindex][1];
                mindex++;
            }
            mcount--;
            for(j=0;j<3;j++)
            {
                int vi=squid_outer_face_indicies[i][j];
                                                  //Normal index
                int ni=squid_outer_face_indicies[i][j+3];
                                                  //Texture index
                int ti=squid_outer_face_indicies[i][j+6];
                glNormal3f (squid_outer_normals[ni][0],squid_outer_normals[ni][1],squid_outer_normals[ni][2]);
                glTexCoord2f(squid_outer_textures[ti][0],squid_outer_textures[ti][1]);

                // Scale x and z according to y using a shifted sine wave.
                amp = 10.0;                       // amplitude of wave: larger = smaller wave.
                freq = 360.0;                     // frequency: greater = more "wiggles"
                x = squid_outer_vertices[vi][0];
                y = squid_outer_vertices[vi][1];
                z = squid_outer_vertices[vi][2];
                a = (y + my) * (freq / dy);
                a += (GLfloat)u * (freq / (GLfloat)NUM_SQUID_UNDULATIONS);
                s = (sin(a * (M_PI / 180.0)) + amp) / amp;
                x *= s;
                z *= s;
                glVertex3f (x, y, z);
            }
        }
        glEnd ();

        glEndList();
        outerUndulatingBody[u] = lid;
    }

    // Create guts texture.
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 2; j++)
        {
            texture[i][j][0] = 100;               // Red
            texture[i][j][1] = 0;                 // Green
            texture[i][j][2] = 0;                 // Blue
        }
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &gutsTextureName);
    glBindTexture(GL_TEXTURE_2D, gutsTextureName);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &(texture[0][0][0]));

    // Guts.
    lid=glGenLists(1);
    mcount=0;
    mindex=0;
    glNewList(lid, GL_COMPILE);

    glBegin (GL_TRIANGLES);
    for(i=0;i<sizeof(squid_guts_face_indicies)/sizeof(squid_guts_face_indicies[0]);i++)
    {
        if(!mcount)
        {
            squid_guts_SelectMaterial(squid_guts_material_ref[mindex][0]);
            mcount=squid_guts_material_ref[mindex][1];
            mindex++;
        }
        mcount--;
        for(j=0;j<3;j++)
        {
            int vi=squid_guts_face_indicies[i][j];
                                                  //Normal index
            int ni=squid_guts_face_indicies[i][j+3];
                                                  //Texture index
            int ti=squid_guts_face_indicies[i][j+6];
            glNormal3f (squid_guts_normals[ni][0],squid_guts_normals[ni][1],squid_guts_normals[ni][2]);
            glTexCoord2f(squid_guts_textures[ti][0],squid_guts_textures[ti][1]);
            glVertex3f (squid_guts_vertices[vi][0],squid_guts_vertices[vi][1],squid_guts_vertices[vi][2]);
        }
    }
    glEnd ();

    glEndList();
    gutsDisplay = lid;

    // Create tentacle extensions.
    for (j = 0; j < NUM_TENTACLE_EXTENSIONS; j++)
    {
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
        segmentTransform[0].yaw = 3.0 * j;
        segmentTransform[7].yaw = 3.0 * j;
        segmentTransform[10].yaw = 3.0 * j;
        segmentTransform[15].yaw = -3.0 * j;
        tentacles[0]->SetSegmentTransforms(segmentTransform);
        tentacles[0]->BuildSegmentTransforms(NUM_SQUID_UNDULATIONS + j);
    }
}


// Draw squid.
void Squid::Draw()
{
    GLfloat f;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // Transform.
    glTranslatef(m_spacial->x, m_spacial->y, m_spacial->z);
    glMultMatrixf(&m_spacial->rotmatrix[0][0]);
    glScalef(m_spacial->scale, m_spacial->scale, m_spacial->scale);

    // Set smooth texture mapping and lighting.
    glShadeModel(GL_SMOOTH);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_LIGHTING);

    // Draw guts and outer body.
    glPushMatrix();
    glTranslatef(0.0, 0.225, 0.0);
    f = 0.75;
    glScalef(f, f, f);
    if (state == EXPLODE) explosionTransform(0);
    glBindTexture(GL_TEXTURE_2D, gutsTextureName);
    glCallList(gutsDisplay);
    glPopMatrix();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix();
    glTranslatef(0.0, 0.275, 0.0);
    glScalef(1.0, 0.8, 1.0);
    if (state == EXPLODE) explosionTransform(1);
    glBindTexture(GL_TEXTURE_2D, outerTextureName);
    if (!undulate)
    {
        glCallList(outerBodyDisplay);
    }
    else
    {
        // Draw undulating body.
        glCallList(outerUndulatingBody[undulateIndex]);
    }
    glPopMatrix();
    glDisable(GL_BLEND);

    // Draw tentacles.
    f = 1.5;
    glScalef(f, f, f);
    f = M_PI / 180.0;
    glPushMatrix();
    glTranslatef(cos(90.0 * f) * .05, -.5, sin(90.0 * f) * .05);
    glRotatef(90.0, 0.0, 1.0, 0.0);
    glRotatef(-90.0, 1.0, 0.0, 0.0);
    if (state == DESTROY)
    {
        // Draw dynamic grasping tentacle.
        tentacles[0]->Draw();
    }
    else
    {
        if (state == EXPLODE) explosionTransform(2);
        tentacles[0]->Draw(tentacleDisplayIndex[0]);
    }
    glPopMatrix();

    glPushMatrix();
    glTranslatef(cos(-30.0 * f) * .05, -.5, sin(-30.0 * f) * .05);
    glRotatef(-150.0, 0.0, 1.0, 0.0);
    glRotatef(-90.0, 1.0, 0.0, 0.0);
    if (state == DESTROY)
    {
        tentacles[1]->Draw();
    }
    else
    {
        if (state == EXPLODE) explosionTransform(3);
        tentacles[1]->Draw(tentacleDisplayIndex[1]);
    }
    glPopMatrix();

    glPushMatrix();
    glTranslatef(cos(-150.0 * f) * .05, -.5, sin(-150.0 * f) * .05);
    glRotatef(-30.0, 0.0, 1.0, 0.0);
    glRotatef(-90.0, 1.0, 0.0, 0.0);
    if (state == DESTROY)
    {
        tentacles[2]->Draw();
    }
    else
    {
        if (state == EXPLODE) explosionTransform(4);
        tentacles[2]->Draw(tentacleDisplayIndex[2]);
    }
    glPopMatrix();

    glPopMatrix();
}


// Idle update.
void Squid::IdleUpdate()
{
    int i,j;

    // Ramble around.
    SetSpeed(idleSpeed);
    moveCount++;
    if (moveCount >= changeDirectionFreq)
    {
        // Change direction.
        moveCount = 0;
        switch(rand() % 6)
        {
            case 0: AddPitch(1.0); break;
            case 1: AddPitch(-1.0); break;
            case 2: AddYaw(1.0); break;
            case 3: AddYaw(-1.0); break;
            case 4: AddRoll(1.0); break;
            case 5: AddRoll(-1.0); break;
        }
    }
    m_spacial->update();

    // Retract tentacles.
    if (extensionCount > 0)
    {
        extensionCount--;
        for (i = 0; i < 3; i++)
        {
            tentacleDisplayIndex[i] = extensionCount + NUM_SQUID_UNDULATIONS;
        }
        return;
    }
    extensionCount = -1;

    // Advance undulations.
    if (undulate)
    {
        undulateCount++;
        if (undulateCount < undulateFreq) return;
        undulateCount = 0;
        undulateIndex = (undulateIndex + 1) % NUM_SQUID_UNDULATIONS;
        j = undulateIndex;
    }
    else
    {
        j = 0;
    }
    for (i = 0; i < 3; i++) tentacleDisplayIndex[i] = j;
}


// Attack update.
void Squid::AttackUpdate()
{
    int i;
    GLfloat position[3],targetPosition[3],targetPoint[3],attackRotation[4],angle;

    // Get position of squid and target.
    GetPosition(position);
    target->GetPosition(targetPosition);

    // Find target point in local coordinates.
    targetPoint[0] = targetPosition[0] - position[0];
    targetPoint[1] = targetPosition[1] - position[1];
    targetPoint[2] = targetPosition[2] - position[2];

    // Get billboard rotation.
    m_spacial->build_rotmatrix();
    GetBillboard(targetPoint, attackRotation);
    angle = attackRotation[3];

    // Indicate not oriented toward target.
    oriented = false;

    // Turn squid to attack.
    if (angle != 0.0)
    {
        // Derive an exact billboard rotation.
        m_spacial->qcalc->clear();
        m_spacial->build_rotmatrix();
        GetBillboard(targetPoint, attackRotation);

        // Set incremental reduction from current rotation.
        angle = angle - DegreesToRadians(rotateSpeed);
        if (angle < 0.0 || (attackRotation[3] - angle) <= 0.0)
        {
            // When angle is close, use exact billboard rotation.
            angle = 0.0;
        }
        m_spacial->loadRotation(attackRotation[3] - angle, attackRotation);
        m_spacial->build_rotmatrix();
    }

    // Extend tentacles.
    if (extensionCount < (NUM_TENTACLE_EXTENSIONS - 1))
    {
        extensionCount++;
        for (i = 0; i < 3; i++)
        {
            tentacleDisplayIndex[i] = extensionCount + NUM_SQUID_UNDULATIONS;
        }
        return;
    }

    // Move toward target.
    if (RadiansToDegrees(angle) > 1.0)
    {
        SetSpeed(0.0);
    }
    else
    {
        SetSpeed(attackSpeed);
        oriented = true;
    }
    m_spacial->update();
}


// Destroy target.
void Squid::Destroy(int targetIndex)
{
    GLfloat f;

    state = DESTROY;
    SetSpeed(0.0);
    oriented = false;
    undulate = false;

    // Set squid transform state.
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(m_spacial->x, m_spacial->y, m_spacial->z);
    glMultMatrixf(&m_spacial->rotmatrix[0][0]);
    glScalef(m_spacial->scale, m_spacial->scale, m_spacial->scale);

    // Build tentacle configurations to grasp target.
    f = 1.5;
    glScalef(f, f, f);
    f = M_PI / 180.0;
    glPushMatrix();
    glTranslatef(cos(90.0 * f) * .05, -.5, sin(90.0 * f) * .05);
    glRotatef(90.0, 0.0, 1.0, 0.0);
    glRotatef(-90.0, 1.0, 0.0, 0.0);
    tentacles[0]->BuildGrasp(targetIndex);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(cos(-30.0 * f) * .05, -.5, sin(-30.0 * f) * .05);
    glRotatef(-150.0, 0.0, 1.0, 0.0);
    glRotatef(-90.0, 1.0, 0.0, 0.0);
    tentacles[1]->BuildGrasp(targetIndex);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(cos(-150.0 * f) * .05, -.5, sin(-150.0 * f) * .05);
    glRotatef(-30.0, 0.0, 1.0, 0.0);
    glRotatef(-90.0, 1.0, 0.0, 0.0);
    tentacles[2]->BuildGrasp(targetIndex);
    glPopMatrix();

    glPopMatrix();
}


// Destroying target update.
void Squid::DestroyUpdate()
{
    GLfloat position[3],targetPosition[3],targetPoint[3],attackRotation[4];

    // Get position of squid and target.
    GetPosition(position);
    target->GetPosition(targetPosition);

    // Find target point in local coordinates.
    targetPoint[0] = targetPosition[0] - position[0];
    targetPoint[1] = targetPosition[1] - position[1];
    targetPoint[2] = targetPosition[2] - position[2];

    // Derive an exact billboard rotation.
    m_spacial->qcalc->clear();
    m_spacial->build_rotmatrix();
    GetBillboard(targetPoint, attackRotation);
    m_spacial->loadRotation(attackRotation[3], attackRotation);
    m_spacial->update();
}


// Explode squid.
void Squid::Explode()
{
    int i,j;

    state = EXPLODE;
    SetSpeed(0.0);
    target = NULL;
    oriented = false;
    undulate = true;

    // Initialize exploding parts.
    explosionCounter = explosionDelay;
    for (i = 0; i < 5; i++)
    {
        explodingParts[i].position.Zero();
        j= rand()%201 - 100;
        explodingParts[i].velocity.x = maxExplosionVelocity * ((float)j / 100.0);
        j= rand()%201 - 100;
        explodingParts[i].velocity.y = maxExplosionVelocity * ((float)j / 100.0);
        j= rand()%201 - 100;
        explodingParts[i].velocity.z = maxExplosionVelocity * ((float)j / 100.0);
        if (explodingParts[i].velocity.Magnitude() > maxExplosionVelocity)
        {
            explodingParts[i].velocity.Normalize(maxExplosionVelocity);
        }
        explodingParts[i].angle = 0.0;
        j= rand()%201 - 100;
        explodingParts[i].axis.x = (float)j / 100.0;
        j= rand()%201 - 100;
        explodingParts[i].axis.y = (float)j / 100.0;
        j= rand()%201 - 100;
        explodingParts[i].axis.z = (float)j / 100.0;
        explodingParts[i].axis.Normalize();
        if (explodingParts[i].axis.Magnitude() == 0.0)
        {
            explodingParts[i].axis.x = 1.0;
        }
        explodingParts[i].angularVelocity = maxExplosionAngularVelocity * (float)(rand()%101) / 100.0;
    }
}


// Exploding update.
void Squid::ExplodeUpdate()
{
    int i;
    float s = GetSpeedFactor();

    // Explosion finished?
    explosionCounter -= s;
    if (explosionCounter <= 0.0)
    {
        Kill();
        return;
    }

    // Update exploding parts.
    for (i = 0; i < 5; i++)
    {
        explodingParts[i].position += explodingParts[i].velocity * s;
        explodingParts[i].angle += explodingParts[i].angularVelocity;
    }
}


// Add explosion transform.
void Squid::explosionTransform(int i)
{
    glTranslatef(explodingParts[i].position.x,
        explodingParts[i].position.y,
        explodingParts[i].position.z);
    glRotatef(explodingParts[i].angle,
        explodingParts[i].axis.x,
        explodingParts[i].axis.y,
        explodingParts[i].axis.z);
}
#endif
