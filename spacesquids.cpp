/*
 * This software is provided under the terms of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * Copyright (c) 2003 Tom Portegys, All Rights Reserved.
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.
 */

//***************************************************************************//
//* File Name: spacesquids.cpp                                              *//
//* Author:    Tom Portegys, portegys@ilstu.edu                             *//
//* Date Made: 09/18/02                                                     *//
//* File Desc: Class declaration and implementation details                 *//
//*            representing the game of Space Squids.                       *//
//* Rev. Date: 4/5/03                                                       *//
//* Rev. Desc: Multi-player networked version (compile with NETWORK)        *//
//*                                                                         *//
//* Objective: Shoot the squids before they eat you!                        *//
//* Options:   [-id "<X-wing ID>"]                                          *//
//*            [-color <X-wing random color seed>]                          *//
//*            [-fullscreen]                                                *//
//*            [-connect <master IP address (for networked version)>]       *//
//***************************************************************************//

// Remove console.
#ifndef UNIX
#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )
#endif

#ifdef UNIX
#define UNIX_STDIN 1                              // For UNIX: read keyboard from stdin instead of using glut
#endif

#ifndef UNIX
#include <windows.h>
#endif
#include "globals.h"
#include "frustum.hpp"
#include "explosion.hpp"
#include "frameRate.hpp"
#include "fmod.h"
#ifdef NETWORK
#include "network.hpp"
#endif
#ifdef UNIX_STDIN
#include "kbhit.h"
#endif

// Game name and usage.
#define NAME "Space Squids"
#ifdef NETWORK
char *Usage = "Usage: %s [-id \"<X-wing ID>\"] [-color <X-wing random color seed>] [-fullscreen] [-connect <Master IP address>]\n";
#else
char *Usage = "Usage: %s [-id \"<X-wing ID>\"] [-color <X-wing random color seed>] [-fullscreen]\n";
#endif

// Network and master player status.
#ifdef NETWORK
Network *network;
bool Master = true;
char MasterIP[IP_LENGTH+1];
int masterXwing = -1;
#endif

// Window dimensions.
#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 500
int WindowWidth = WINDOW_WIDTH;
int WindowHeight = WINDOW_HEIGHT;

// Control information.
char *ControlInfo[] =
{
    "           h : Control help. Twice to print help to standard output",
    "           w : Full screen",
    "           f : Faster",
    "           s : Slower",
    "  Left arrow : Yaw left",
    " Right arrow : Yaw right",
    "    Up arrow : Pitch up",
    "  Down arrow : Pitch down",
    "           1 : Roll right",
    "           3 : Roll left",
    "       Space : Fire plasma bolt",
    "           m : Toggle mute mode",
    "           o : Toggle spotlight",
    "           p : Paint objects",
    #ifndef NETWORK
    "           r : Show frame rate",
    #endif
    "           v : Toggle rear view",
    #ifdef NETWORK
    "           w : Who is playing",
    #endif
    "           q : Quit",
    NULL
};

// X-wing and motion controls.
struct XwingControls Xwings[NUM_XWINGS];
void moveXwing(int);
int myXwing = 0;                                  // Index of user's xwing.

// Plasma bolts.
class PlasmaBoltSet *plasmaBolts;

// Squids.
struct SquidControls Squids[NUM_SQUIDS];
void moveSquid(int);

// Explosion.
#define NUM_EXPLOSION_PARTICLES 100
cExplosion *explosion;

// Frustum and camera position.
#define FRUSTUM_ANGLE 15.0
#define FRUSTUM_ASPECT 1.0
#define FRUSTUM_NEAR 1.0
#define FRUSTUM_FAR 200.0
#define CAMERA_X 0.0
#define CAMERA_Y 0.0
#define CAMERA_Z 5.0
GLfloat CameraPosition[3] = { CAMERA_X, CAMERA_Y, CAMERA_Z };

// Camera relative position following X-wing.
#define CAMERA_BEHIND 1.1
#define CAMERA_ABOVE 0.15

// Camera frustum.
Frustum *frustum;
bool inFrustum(int);

// Delay for "spring-loaded" camera lag..
#define CAMERA_DELAY_SIZE 50
struct
{
    GLfloat f[3];
    GLfloat u[3];
} CameraDelay[CAMERA_DELAY_SIZE];
int CameraDelayIndex;

// Rear view switch.
bool RearView = false;

// Lights.
GLfloat ShipLightPosition[] = {1.0, 1.0, -1.0, 1.0};
GLfloat ShipLightColor[] = {1.0, 1.0, 1.0, 1.0};
bool ShipLightSwitch = true;
GLfloat SpotLightPosition[] = {0.0, 0.0, -1.0, 1.0};
GLfloat SpotLightDirection[] = {0.0, 0.0, -1.0};
GLfloat SpotLightColor[] = {1.0, 1.0, 1.0, 1.0};
bool SpotLightSwitch = true;

// Dimension of enclosing walls.
#define WALL_SIZE 100
#define WALL_GRID_RATIO 0.05

// Max block initialization tries.
#define BLOCK_INIT_TRIES 10

// Block texture and material.
#ifndef UNIX
#define HELLBOX 1                                 // "Hellraiser" box
#endif
#ifdef HELLBOX
#define HELLBOX_BMP1_IMAGE "Hell1.bmp"
#define HELLBOX_BMP2_IMAGE "Hell2.bmp"
#define HELLBOX_BMP3_IMAGE "Hell3.bmp"
#define HELLBOX_BMP4_IMAGE "Hell4.bmp"
#define HELLBOX_BMP5_IMAGE "Hell5.bmp"
#define HELLBOX_BMP6_IMAGE "Hell6.bmp"
GLuint BlockTextureName[6];
GLint FixedBlockDisplay[7];
GLint BlockDisplay[7];
#else
#define BLOCK_TGA_IMAGE "flake.tga"
GLuint BlockTextureName;
#endif
bool BlockTextureLoaded;
bool PaintBlocks = true;
struct
{
    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    GLfloat emission[4];
    GLfloat phExp;
}


BlockMaterial =
{
    {0.117647f,0.117647f,0.117647f,1.0f},
    {
        0.752941f,0.752941f,0.752941f,1.0f
    },
    {0.752941f,0.752941f,0.752941f,1.0f},
    {
        0.0f,0.0f,0.0f,1.0f
    }, 8.0f
};

// Block functions.
void createBlock(int);
void setBlockVertices(int, float, float, float, float, float, float);
bool positionBlock(int);
void buildWallDisplay(int),buildBlockDisplay(int);
#ifdef HELLBOX
void buildFixedBlockDisplays();
void buildBlockDisplays();
#endif
bool ShowBoundingBlocks = false;
bool DrawWalls = true;

// Star display.
#define NUM_STARS 1000
int StarDisplay;
void buildStarDisplay();

// Modes.
USERMODE UserMode;
void modeInfo(), introInfo(), optionInfo(), runInfo();
void helpInfo(), finiInfo(), messageInfo(), whoInfo();
void setInfoProjection(), resetInfoProjection();
void renderBitmapString(GLfloat, GLfloat, void *, char *);
bool WinPending = false;
bool LossPending = false;
#define WIN_PENDING_DELAY 100.0
#define LOSS_PENDING_DELAY 50.0
float EndPendingCounter = 0.0;
char UserMessage[USER_MESSAGE_LENGTH+1];

// Options entry.
enum { GET_ID, GET_COLOR, GET_SLAVE, GET_IP }
OptionMode;
char idOption[ID_LENGTH+1];
int colorSeedOption;
#define DATA_STRING_SIZE 50
char DataString[DATA_STRING_SIZE + 1];
int DataIndex;

// Flags to prevent option-entered characters from affecting run mode.
// ' '=32, 'a'=97, 'z'=122, 'A'=65, 'Z'=90
bool skipChars[123];

/*
    Available fonts:
    GLUT_BITMAP_8_BY_13
    GLUT_BITMAP_9_BY_15
    GLUT_BITMAP_TIMES_ROMAN_10
    GLUT_BITMAP_TIMES_ROMAN_24
    GLUT_BITMAP_HELVETICA_10
    GLUT_BITMAP_HELVETICA_12
    GLUT_BITMAP_HELVETICA_18
*/
#define FONT GLUT_BITMAP_9_BY_15
#define BIG_FONT GLUT_BITMAP_TIMES_ROMAN_24
#define LINE_SPACE 10

// Frame-rate independent movement.
#define TARGET_FRAME_RATE 35.0
#define BLOCKSPEED_TUNE 0.1
class FrameRate frameRate(TARGET_FRAME_RATE);
bool ShowFrameRate = false;

#ifndef UNIX
// Keyboard input repeat delay (ms).
#define KEY_INPUT_SHORT_DELAY 15
#define KEY_INPUT_LONG_DELAY 150
#endif

// Sound effects.
FSOUND_SAMPLE *plasmaBoltSound,*explosionSound,*bumpSound;
FSOUND_SAMPLE *shipEngineSound,*youWinSound,*youLoseSound;
int shipEngineChannel = -1;
bool shipEnginePaused = true;
bool finiSoundPlayed = false;
int finiChannel = -1;
bool muteMode = false;
void getSounds(), freeSounds();

// Debug mode.
bool debugMode = false;
void idle(void);

// Get world position of a point.
void localToWorld(GLfloat *local, GLfloat *world);

// Display function.
void
display(void)
{
    int i,j,k,si,xi,sb,xb;
    Vector axis;
    float angle;
    GLfloat e[3],p[3],f[3],u[3],b,a;
    GLfloat l[3],w[3];
    Xwing *xwing;
    Squid *squid;

    // Clear screen.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Clear transform matrix.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Not ready to display?
    if (UserMode == INTRO || UserMode == OPTIONS || UserMode == FATAL)
    {
        modeInfo();
        glutSwapBuffers();
        glFlush();
        return;
    }

    #ifdef NETWORK
    // Synchronize state.
    if (Master)
    {
        network->getSlave();
    }
    else
    {
        if (network->sendSlave())
            network->getMaster();
    }
    #endif

    #ifdef NETWORK
    // Fatal network error.
    if (UserMode == FATAL)
    {
        #else
        // Non-run mode.
        if (UserMode != RUN)
        {
            #endif
            modeInfo();
            glutSwapBuffers();
            glFlush();
            return;
        }

        // Camera follows above and behind X-wing in a spring-loaded fashion.
        xwing = Xwings[myXwing].xwing;
        xwing->GetPosition(p);
        CameraDelayIndex = (CameraDelayIndex + 1) % CAMERA_DELAY_SIZE;
        i = CameraDelayIndex;
        xwing->GetForward(CameraDelay[i].f);
        xwing->GetUp(CameraDelay[i].u);
        a = CAMERA_ABOVE;
        b = CAMERA_BEHIND;
        if (RearView) b = -b;
        j = i;
        i = (i + 1) % CAMERA_DELAY_SIZE;
        f[0] = CameraDelay[i].f[0];
        f[1] = CameraDelay[i].f[1];
        f[2] = CameraDelay[i].f[2];
        u[0] = CameraDelay[i].u[0];
        u[1] = CameraDelay[i].u[1];
        u[2] = CameraDelay[i].u[2];
        for (k = 1; k < CAMERA_DELAY_SIZE; k++)
        {
            i = (i + 1) % CAMERA_DELAY_SIZE;
            f[0] += CameraDelay[i].f[0];
            f[1] += CameraDelay[i].f[1];
            f[2] += CameraDelay[i].f[2];
            u[0] += CameraDelay[i].u[0];
            u[1] += CameraDelay[i].u[1];
            u[2] += CameraDelay[i].u[2];
        }
        f[0] /= (float)CAMERA_DELAY_SIZE;
        f[1] /= (float)CAMERA_DELAY_SIZE;
        f[2] /= (float)CAMERA_DELAY_SIZE;
        u[0] /= (float)CAMERA_DELAY_SIZE;
        u[1] /= (float)CAMERA_DELAY_SIZE;
        u[2] /= (float)CAMERA_DELAY_SIZE;
        e[0] = p[0] + (u[0] * (b + Xwings[myXwing].speed)) + (f[0] * a);
        e[1] = p[1] + (u[1] * (b + Xwings[myXwing].speed)) + (f[1] * a);
        e[2] = p[2] + (u[2] * (b + Xwings[myXwing].speed)) + (f[2] * a);
        gluLookAt(e[0], e[1], e[2], p[0], p[1], p[2], f[0], f[1], f[2]);

        // Get updated camera frustum.
        delete frustum;
        frustum = new Frustum();

        // Move the blocks and determine collisions.
        #ifdef NETWORK
        if (Master)
        #endif
            StepSimulation(frameRate.speedFactor * BLOCKSPEED_TUNE);

        // Update and draw plasma bolts.
        plasmaBolts->Go();

        // Process squids.
        for (si = 0; si < NUM_SQUIDS; si++)
        {
            squid = Squids[si].squid;
            sb = Squids[si].bodyGroup;

            // Move the squid.
            #ifdef NETWORK
            if (Master)
            #endif
                moveSquid(si);
            if (!squid->IsAlive()) continue;

            // Draw.
            if (inFrustum(sb) || inFrustum(sb + 1))
            {
                squid->Draw();
            }

            // Plasma bolt explodes squid when it hits body bounding block.
            for (i = sb; Bodies[i].group == sb && i < NumBodies; i++)
            {
                if (!ShowBoundingBlocks && i != sb) break;
                if (!Bodies[i].valid) break;

                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();

                // Transform block.
                glTranslatef(Bodies[i].vPosition.x, Bodies[i].vPosition.y, Bodies[i].vPosition.z);
                angle = QGetAngle(Bodies[i].qOrientation);
                angle = RadiansToDegrees(angle);
                angle = -angle;
                axis = QGetAxis(Bodies[i].qOrientation);
                glRotatef(angle, axis.x, axis.y, axis.z);

                // Draw bounding block?
                if (ShowBoundingBlocks)
                {
                    glDisable(GL_BLEND);
                    glDisable(GL_TEXTURE_2D);
                    glDisable(GL_LIGHTING);
                    glLineWidth(1.0);

                    // Draw block.
                    glColor3f(Bodies[i].red, Bodies[i].green, Bodies[i].blue);
                    glCallList(Bodies[i].display);
                }

                // Get world position of block.
                if (i == sb)
                {
                    l[0] = l[1] = l[2] = 0.0;
                    localToWorld(l, w);
                }

                glPopMatrix();

                // Plasma bolt hits squid?
                #ifdef NETWORK
                if (Master)
                {
                    #endif
                    if (squid->IsExploding()) continue;
                    if (i == sb)
                    {
                        if (plasmaBolts->collision(w, Bodies[i].fRadius))
                        {
                            #ifdef NETWORK
                            network->setPlasmaBoltUpdated();
                            #endif
                            // Explode squid.
                            explodeSquid(si);
                        }
                    }
                    #ifdef NETWORK
                }
                #endif
            }
        }

        // X-wings.
        for (xi = 0; xi < NUM_XWINGS; xi++)
        {
            xwing = Xwings[xi].xwing;
            xb = Xwings[xi].bodyGroup;

            // Move the X-wing.
            #ifdef NETWORK
            if (Master)
            #endif
                moveXwing(xi);
            if (!xwing->IsAlive()) continue;

            // Draw X-wing.
            xwing->Draw();

            // Plasma bolt explodes X-wing when it hits a bounding block.
            glDisable(GL_BLEND);
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_LIGHTING);
            glLineWidth(1.0);
            for (i = xb; Bodies[i].group == xb && i < NumBodies; i++)
            {
                if (!Bodies[i].valid) break;

                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();

                // Transform block.
                glTranslatef(Bodies[i].vPosition.x, Bodies[i].vPosition.y, Bodies[i].vPosition.z);
                angle = QGetAngle(Bodies[i].qOrientation);
                angle = RadiansToDegrees(angle);
                angle = -angle;
                axis = QGetAxis(Bodies[i].qOrientation);
                glRotatef(angle, axis.x, axis.y, axis.z);

                // Draw bounding block?
                if (ShowBoundingBlocks)
                {
                    glColor3f(Bodies[i].red, Bodies[i].green, Bodies[i].blue);
                    glCallList(Bodies[i].display);
                }

                // Get world position of block.
                l[0] = l[1] = l[2] = 0.0;
                localToWorld(l, w);

                glPopMatrix();

                // Plasma bolt hits X-wing?
                #ifdef NETWORK
                if (Master)
                {
                    #endif
                    if (xwing->IsExploding()) continue;
                    if (plasmaBolts->collision(w, Bodies[i].fRadius))
                    {
                        #ifdef NETWORK
                        network->setPlasmaBoltUpdated();
                        #endif
                        if (Xwings[xi].invulnerable) continue;

                        // Explode X-wing.
                        explodeXwing(xi);
                    }
                    #ifdef NETWORK
                }
                #endif
            }
        }

        glDisable(GL_BLEND);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glLineWidth(1.0);
        glColor3f(1.0, 1.0, 1.0);

        // Draw stars.
        glPointSize(3.0);
        glEnable(GL_POINT_SMOOTH);
        glCallList(StarDisplay);
        glPointSize(1.0);
        glDisable(GL_POINT_SMOOTH);

        // Draw wall grids?
        if (DrawWalls)
        {
            for (i = 0; i < 6; i++)
            {
                if (inFrustum(i)) glCallList(Bodies[i].display);
            }
        }

        // Draw blocks.
        if (PaintBlocks)
        {
            glShadeModel(GL_SMOOTH);
            glEnable(GL_TEXTURE_2D);
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            #ifndef HELLBOX
            glBindTexture(GL_TEXTURE_2D, BlockTextureName);
            #endif
            glEnable(GL_LIGHTING);
        }
        else
        {
            glDisable(GL_BLEND);
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_LIGHTING);
        }
        glLineWidth(1.0);
        for (i = 0; i < NumBodies; i++)
        {
            if (!Bodies[i].valid) continue;
            if (Bodies[i].type != BLOCK_TYPE && Bodies[i].type != FIXED_BLOCK_TYPE) continue;

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();

            // Transform block.
            glTranslatef(Bodies[i].vPosition.x, Bodies[i].vPosition.y, Bodies[i].vPosition.z);
            angle = QGetAngle(Bodies[i].qOrientation);
            angle = RadiansToDegrees(angle);
            axis = QGetAxis(Bodies[i].qOrientation);
            glRotatef(angle, axis.x, axis.y, axis.z);

            // Draw block if in camera view.
            if (inFrustum(i))
            {
                #ifdef HELLBOX
                if (!PaintBlocks)
                {
                    glColor3f(Bodies[i].red, Bodies[i].green, Bodies[i].blue);
                }
                for (j = 0; j < 6; j++)
                {
                    if (PaintBlocks)
                    {
                        switch(j)
                        {
                            case 0: k = 0; break;
                            case 2: k = 1; break;
                            case 1: k = 2; break;
                            case 3: k = 3; break;
                            case 4: k = 2; break;
                            case 5: k = 5; break;
                        }
                        glBindTexture(GL_TEXTURE_2D, BlockTextureName[k]);
                    }
                    if (Bodies[i].type == FIXED_BLOCK_TYPE)
                    {
                        glCallList(FixedBlockDisplay[j]);
                    }
                    else
                    {
                        glCallList(BlockDisplay[j]);
                    }
                }
                if (!PaintBlocks)
                {
                    if (Bodies[i].type == FIXED_BLOCK_TYPE)
                    {
                        glCallList(FixedBlockDisplay[6]);
                    }
                    else
                    {
                        glCallList(BlockDisplay[6]);
                    }
                }
                #else
                if (!PaintBlocks)
                {
                    glColor3f(Bodies[i].red, Bodies[i].green, Bodies[i].blue);
                }
                glCallList(Bodies[i].display);
                #endif
            }

            // Destroy plasma bolts hitting block.
            l[0] = l[1] = l[2] = 0.0;
            localToWorld(l, w);

            glPopMatrix();

            #ifdef NETWORK
            if (Master && plasmaBolts->collision(w, Bodies[i].fRadius))
                network->setPlasmaBoltUpdated();
            #else
            plasmaBolts->collision(w, Bodies[i].fRadius);
            #endif
        }

        // Explosion.
        if (explosion->GetNumLiveParticles() > 0)
        {
            explosion->Go();
        }

        // Check for and handle end of game.
        if (WinPending)
        {
            EndPendingCounter -= frameRate.speedFactor;
            if (EndPendingCounter <= 0.0) UserMode = WIN;
        }
        if (LossPending)
        {
            EndPendingCounter -= frameRate.speedFactor;
            if (EndPendingCounter <= 0.0)
            {
                #ifdef NETWORK
                UserMode = LOSE;
                #else
                // Can jump to next ship?
                for (i = (myXwing + 1) % NUM_XWINGS; i != myXwing; i = (i + 1) % NUM_XWINGS)
                {
                    if (Xwings[i].xwing->IsAlive()) break;
                }
                if (i == myXwing)
                {
                    // All dead - you lose.
                    UserMode = LOSE;
                }
                else
                {
                    LossPending = false;
                    myXwing = i;
                    if (shipEngineChannel != -1)
                    {
                        FSOUND_SetVolume(shipEngineChannel,
                            (int)(Xwings[myXwing].speed * 255.0 / (double)MAX_SPEED));
                    }
                }
                #endif
            }
        }
        if (!WinPending && !LossPending)
        {
            if (!Xwings[myXwing].xwing->IsAlive())
            {
                LossPending = true;
                EndPendingCounter = LOSS_PENDING_DELAY;
            }
            else
            {
                for (i = 0; i < NUM_SQUIDS; i++)
                {
                    if (Squids[i].squid->IsAlive()) break;
                }
                if (i == NUM_SQUIDS)
                {
                    WinPending = true;
                    EndPendingCounter = WIN_PENDING_DELAY;
                }
            }
        }

        #ifdef NETWORK
        // Send master state to slaves.
        if (Master) network->sendMaster();
        #endif

        // Run mode information.
        modeInfo();

        // Set frame-rate independence speed factor.
        frameRate.update();

        // Display new screen.
        glutSwapBuffers();
        glFlush();
    }

    // Get world position of a point.
    void localToWorld(GLfloat *local, GLfloat *world)
    {
        int i,j;
        GLfloat m[16];
        Matrix x(4,4),p(4,1),t(4,1);

        glGetFloatv( GL_MODELVIEW_MATRIX, m );
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

    // Move X-wing normally and during collisions.
    void
        moveXwing(int index)
    {
        int i,xb;
        Xwing *xwing;
        cSpacial *spacial;
        GLfloat v[3];
        Vector axis;
        float angle;

        // Access X-wing.
        xwing = Xwings[index].xwing;
        if (!xwing->IsAlive()) return;
        xb = Xwings[index].bodyGroup;
        spacial = xwing->GetSpacial();

        // Set speed factor.
        xwing->SetSpeedFactor(frameRate.speedFactor);

        // Check for new collision.
        if (Bodies[xb].collision) Xwings[index].collisionSteps = XWING_COLLISION_STEPS;

        // Normal movement.
        if (Xwings[index].collisionSteps == 0 || xwing->IsExploding())
        {
            Xwings[index].firstBump = true;
            xwing->Update();

            // If just died, invalidate bounding blocks.
            if (!xwing->IsAlive())
            {
                for (i = xb; Bodies[i].group == xb && i < NumBodies; i++)
                {
                    Bodies[i].valid = false;
                }
                return;
            }

            // Bounding blocks follow X-wing movement.
            for (i = xb; Bodies[i].group == xb && i < NumBodies; i++)
            {
                Bodies[i].vPosition.x = spacial->x;
                Bodies[i].vPosition.y = spacial->y;
                Bodies[i].vPosition.z = spacial->z;
                v[0] = spacial->rotmatrix[1][0];
                v[1] = spacial->rotmatrix[1][1];
                v[2] = spacial->rotmatrix[1][2];
                spacial->normalize(v);
                Bodies[i].vVelocity.x = -(v[0] * spacial->speed);
                Bodies[i].vVelocity.y = -(v[1] * spacial->speed);
                Bodies[i].vVelocity.z = -(v[2] * spacial->speed);
                Bodies[i].vAngularVelocity.x = 0.0;
                Bodies[i].vAngularVelocity.y = 0.0;
                Bodies[i].vAngularVelocity.z = 0.0;
                Bodies[i].qOrientation.n = spacial->qcalc->quat[3];
                Bodies[i].qOrientation.v.x = spacial->qcalc->quat[0];
                Bodies[i].qOrientation.v.y = spacial->qcalc->quat[1];
                Bodies[i].qOrientation.v.z = spacial->qcalc->quat[2];
            }
            return;
        }

        // Collision: bounding blocks in control of movement.
        Xwings[index].collisionSteps--;
        spacial->x = Bodies[xb].vPosition.x;
        spacial->y = Bodies[xb].vPosition.y;
        spacial->z = Bodies[xb].vPosition.z;
        angle = QGetAngle(Bodies[xb].qOrientation);
        axis = QGetAxis(Bodies[xb].qOrientation);
        v[0] = axis.x;
        v[1] = axis.y;
        v[2] = axis.z;
        spacial->qcalc->loadRotation(angle, v);
        spacial->qcalc->build_rotmatrix(spacial->rotmatrix, spacial->qcalc->quat);

        // Play bump sound?
        if (Xwings[index].firstBump == true)
        {
            Xwings[index].firstBump = false;
            if (index == myXwing && bumpSound && !muteMode)
            {
                FSOUND_PlaySound(FSOUND_FREE, bumpSound);
            }
        }
    }

    // Create an X-wing by finding a dead one and resurrecting it.
    // Return index or -1 for failure.
    int createXwing()
    {
        register int i;

        for (i = (myXwing + 1) % NUM_XWINGS; i != myXwing; i = (i + 1) % NUM_XWINGS)
        {
            if (!Xwings[i].xwing->IsAlive()) break;
        }
        if (i == myXwing) return(-1);
        resurrectXwing(i);
        return(i);
    }

    // Resurrect X-wing.
    void resurrectXwing(int i)
    {
        register int j;
        GLfloat p[3];

        Xwings[i].xwing->Resurrect();
        Xwings[i].pitch = 180.0;
        Xwings[i].yaw = 0.0;
        Xwings[i].roll = 0.0;
        Xwings[i].speed = 0.0;
        Xwings[i].xwing->SetPitch(Xwings[i].pitch);
        Xwings[i].xwing->SetYaw(Xwings[i].yaw);
        Xwings[i].xwing->SetRoll(Xwings[i].roll);
        Xwings[i].xwing->SetSpeed(Xwings[i].speed);
        Xwings[i].collisionSteps = 0;
        Xwings[i].firstBump = true;
        Xwings[i].shotCount = 0;
        Xwings[i].invulnerable = false;
        for (j = Xwings[i].bodyGroup; Bodies[j].group == Xwings[i].bodyGroup && j < NumBodies; j++)
        {
            Bodies[j].valid = true;
            if (j == Xwings[i].bodyGroup)
            {
                // Try to position non-overlapping block.
                if (positionBlock(j))
                {
                    // Set X-wing position.
                    p[0] = Bodies[j].vPosition.x;
                    p[1] = Bodies[j].vPosition.y;
                    p[2] = Bodies[j].vPosition.z;
                    Xwings[i].xwing->SetPosition(p);
                }
            }
            else
            {

                // Other blocks have same position.
                Bodies[j].vPosition = Bodies[Xwings[i].bodyGroup].vPosition;
            }
        }
    }

    // Explode X-wing.
    void explodeXwing(int index)
    {
        register Xwing *xwing = Xwings[index].xwing;
        register int xb = Xwings[index].bodyGroup;

        xwing->Explode();
        for (register int i = xb; Bodies[i].group == xb && i < NumBodies; i++)
        {
            Bodies[i].valid = false;
        }
        explosion->SetLocation(Bodies[xb].vPosition.x, Bodies[xb].vPosition.y, Bodies[xb].vPosition.z);
        explosion->Reset();

        // Play explosion sound.
        if (explosionSound && !muteMode) FSOUND_PlaySound(FSOUND_FREE, explosionSound);
    }

    // Kill X-wing.
    void killXwing(int index)
    {
        register Xwing *xwing = Xwings[index].xwing;
        register int xb = Xwings[index].bodyGroup;
        xwing->Kill();
        for (register int i = xb; Bodies[i].group == xb && i < NumBodies; i++)
        {
            Bodies[i].valid = false;
        }
    }

    // Move squid normally and during collisions.
    void
        moveSquid(int index)
    {
        int i,sb,xi,xb;
        Squid *squid;
        cSpacial *spacial;
        Xwing *xwing;
        GLfloat v[3];
        Vector x,v1,v2;
        float a,d,d1,d2;
        bool attack;

        // Access squid.
        squid = Squids[index].squid;
        if (!squid->IsAlive()) return;
        sb = Squids[index].bodyGroup;
        spacial = squid->GetSpacial();

        // Set speed factor.
        squid->SetSpeedFactor(frameRate.speedFactor);

        // Set squid to attack based on distance to and visibility of an X-wing.
        if (squid->IsIdle() || squid->IsAttacking())
        {
            attack = false;
            for (xi = 0; xi < NUM_XWINGS; xi++)
            {
                xwing = Xwings[xi].xwing;
                if (xwing->state == Xwing::EXPLODE || xwing->state == Xwing::DEAD) continue;
                xb = Xwings[xi].bodyGroup;

                d = Bodies[sb].vPosition.Distance(Bodies[xb].vPosition);
                if (d <= Squids[index].attackRange)
                {
                    // Within attack range, is X-wing visible?
                    // Check if obscured by a block, as defined by the block radius.
                    for (i = 0; i < NumBodies; i++)
                    {
                        if (!Bodies[i].valid) continue;
                        if (Bodies[i].type != BLOCK_TYPE && Bodies[i].type != FIXED_BLOCK_TYPE) continue;

                        // Make sure block is "between" X-wing and squid.
                        v1 = Bodies[sb].vPosition - Bodies[i].vPosition;
                        v1.Normalize();
                        v2 = Bodies[sb].vPosition - Bodies[xb].vPosition;
                        v2.Normalize();
                        if ((v2*v1) <= 0.0) continue;
                        v1 = Bodies[xb].vPosition - Bodies[i].vPosition;
                        d1 = v1.Magnitude();
                        v1.Normalize();
                        v2 = Bodies[xb].vPosition - Bodies[sb].vPosition;
                        v2.Normalize();
                        d2 = v2 * v1;             // dot product: cos of angle.
                        if (d2 <= 0.0) continue;

                        // Is distance from block to segment between X-wing and squid < block radius?
                        d = d2 * d1;
                        d = sqrt((d1 * d1) - (d * d));
                        if (d < Bodies[i].fRadius) break;
                    }
                    if (i == NumBodies)
                    {
                        attack = true;
                        break;
                    }
                }
            }
            if (attack)
            {
                squid->Attack(xwing);
                Bodies[sb + 1].valid = false;     // Disable tentacles' bounding box.
            }
            else
            {
                squid->Idle();
                Bodies[sb + 1].valid = true;
            }
        }

        // If destroying target, bind to it at attachment point.
        if (squid->IsDestroying())
        {
            xwing = (class Xwing *)squid->GetTarget();
            for (xi = 0; xi < NUM_XWINGS; xi++)
            {
                if (Xwings[xi].xwing == xwing) break;
            }
            xb = Xwings[xi].bodyGroup;

            // Return to idle if target is dead.
            if (!xwing->IsAlive())
            {
                squid->Idle();
                Bodies[sb + 1].valid = true;
                Bodies[sb].exempt = -1;
                Bodies[sb + 1].exempt = -1;
                return;
            }

            // Resume attacking invulnerable target.
            if (Xwings[xi].invulnerable)
            {
                squid->Attack(xwing);
                Bodies[sb].exempt = -1;
                Bodies[sb + 1].exempt = -1;
                return;
            }

            // Target is exploding?
            if (xwing->IsExploding()) return;

            // Bind to target.
            v[0] = Squids[index].attachPoint[0];
            v[1] = Squids[index].attachPoint[1];
            v[2] = Squids[index].attachPoint[2];
            xwing->TransformPoint(v);
            squid->SetPosition(v);
            squid->Update();

            // Move bounding boxes
            for (i = sb; Bodies[i].group == sb && i < NumBodies; i++)
            {
                Bodies[i].vPosition.x = spacial->x;
                Bodies[i].vPosition.y = spacial->y;
                Bodies[i].vPosition.z = spacial->z;
                v[0] = spacial->rotmatrix[1][0];
                v[1] = spacial->rotmatrix[1][1];
                v[2] = spacial->rotmatrix[1][2];
                spacial->normalize(v);
                Bodies[i].vVelocity.x = -(v[0] * spacial->speed);
                Bodies[i].vVelocity.y = -(v[1] * spacial->speed);
                Bodies[i].vVelocity.z = -(v[2] * spacial->speed);
                Bodies[i].vAngularVelocity.x = 0.0;
                Bodies[i].vAngularVelocity.y = 0.0;
                Bodies[i].vAngularVelocity.z = 0.0;
                Bodies[i].qOrientation.n = spacial->qcalc->quat[3];
                Bodies[i].qOrientation.v.x = spacial->qcalc->quat[0];
                Bodies[i].qOrientation.v.y = spacial->qcalc->quat[1];
                Bodies[i].qOrientation.v.z = spacial->qcalc->quat[2];
            }

            // Explode target?
            Squids[index].killCounter -= frameRate.speedFactor;
            if (Squids[index].killCounter <= 0.0)
            {
                explodeXwing(xi);
            }
            return;
        }

        // If not exploding, check for new collision.
        if (!squid->IsExploding())
        {
            if (Bodies[sb].collision)
            {
                // Collided with vulnerable target while oriented to attack?
                if ((xb = Bodies[Bodies[sb].withWho].group) != -1)
                {
                    for (xi = 0; xi < NUM_XWINGS; xi++)
                    {
                        if (xb == Xwings[xi].bodyGroup) break;
                    }
                    if (xi == NUM_XWINGS || Xwings[xi].invulnerable)
                    {
                        xb = -1;
                    }
                }
                if (xb != -1 && squid->IsOriented())
                {
                    // Commence destroying target.
                    squid->Destroy(xb);
                    Squids[index].killCounter = SQUID_KILL_DELAY;

                    // Prevent further collisions between target and squid.
                    Bodies[sb].exempt = xb;
                    Bodies[sb + 1].exempt = xb;

                    // Set attachment point in target coordinates.
                    squid->GetPosition(v);
                    xwing->InverseTransformPoint(v);
                    x.x = v[0];
                    x.y = v[1];
                    x.z = v[2];
                    if (x.Magnitude() > 0.5) x.Normalize(0.5);
                    Squids[index].attachPoint[0] = x.x;
                    Squids[index].attachPoint[1] = x.y;
                    Squids[index].attachPoint[2] = x.z;

                    Squids[index].collisionSteps = 0;
                    return;

                }
                else
                {
                    Squids[index].collisionSteps = SQUID_COLLISION_STEPS;
                }
            }
        }
        else
        {
            Squids[index].collisionSteps = 0;
        }

        // Normal movement.
        if (Squids[index].collisionSteps == 0)
        {
            squid->Update();

            for (i = sb; Bodies[i].group == sb && i < NumBodies; i++)
            {
                Bodies[i].vPosition.x = spacial->x;
                Bodies[i].vPosition.y = spacial->y;
                Bodies[i].vPosition.z = spacial->z;
                v[0] = spacial->rotmatrix[1][0];
                v[1] = spacial->rotmatrix[1][1];
                v[2] = spacial->rotmatrix[1][2];
                spacial->normalize(v);
                Bodies[i].vVelocity.x = -(v[0] * spacial->speed);
                Bodies[i].vVelocity.y = -(v[1] * spacial->speed);
                Bodies[i].vVelocity.z = -(v[2] * spacial->speed);
                Bodies[i].vAngularVelocity.x = 0.0;
                Bodies[i].vAngularVelocity.y = 0.0;
                Bodies[i].vAngularVelocity.z = 0.0;
                Bodies[i].qOrientation.n = spacial->qcalc->quat[3];
                Bodies[i].qOrientation.v.x = spacial->qcalc->quat[0];
                Bodies[i].qOrientation.v.y = spacial->qcalc->quat[1];
                Bodies[i].qOrientation.v.z = spacial->qcalc->quat[2];
            }
            return;
        }

        // Collision: bounding blocks in control of movement.
        Squids[index].collisionSteps--;
        spacial->x = Bodies[sb].vPosition.x;
        spacial->y = Bodies[sb].vPosition.y;
        spacial->z = Bodies[sb].vPosition.z;
        a = QGetAngle(Bodies[sb].qOrientation);
        x = QGetAxis(Bodies[sb].qOrientation);
        v[0] = x.x;
        v[1] = x.y;
        v[2] = x.z;
        spacial->qcalc->loadRotation(a, v);
        spacial->qcalc->build_rotmatrix(spacial->rotmatrix, spacial->qcalc->quat);
    }

    // Explode squid.
    void explodeSquid(int index)
    {
        register Squid *squid = Squids[index].squid;
        register int sb = Squids[index].bodyGroup;

        squid->Explode();
        Bodies[sb].valid = false;
        Bodies[sb + 1].valid = false;
        explosion->SetLocation(Bodies[sb].vPosition.x, Bodies[sb].vPosition.y, Bodies[sb].vPosition.z);
        explosion->Reset();

        // Play explosion sound.
        if (explosionSound && !muteMode) FSOUND_PlaySound(FSOUND_FREE, explosionSound);
    }

    // Is block in frustum?
    bool
        inFrustum(int index)
    {
        int i,j;
        Vector v[8],vtmp;
        float d;

        // Rotate bounding vertices and convert to global coordinates.
        for(i=0; i<8; i++)
        {
            vtmp = Bodies[index].vVertexList[i];
            v[i] = QVRotate(Bodies[index].qOrientation, vtmp);
            v[i] += Bodies[index].vPosition;
        }

        // Check for vertex intersecting frustum.
        for (i = 0; i < 6; i++)
        {
            for (j = 0; j < 8; j++)
            {
                d = frustum->planeToPoint(frustum->planes[i], v[j]);
                if (d > 0.0) break;
            }
            if (j == 8) return(false);
        }
        return(true);
    }

    // Window reshape.
    void
        reshape(int w, int h)
    {
        WindowWidth = w;
        WindowHeight = h;
        glViewport(0, 0, w, h);
    }

    // Keyboard input.
    #define RETURN_KEY 13
    #define BACKSPACE_KEY 8
    void
        keyInput(unsigned char key, int x, int y)
    {
        int i;
        Xwing *xwing;

        xwing = Xwings[myXwing].xwing;

        switch(UserMode)
        {

            // Introduction.
            case INTRO:
                if (key >= 0 && key < 123)
                {
                    i = key;
                    if (i >= 'A' && i <= 'Z') i = key - 'A' + 'a';
                    skipChars[i] = true;
                }
                switch(key)
                {
                    case 'h': UserMode = HELP; break;
                    case 'q':
                        freeSounds();
                #ifdef NETWORK
                        // Notify other game instances.
                        network->exitNotify(Network::QUIT);
                #endif
                        exit(0);
                    default:

                        // Need more options?
                        if (idOption[0] != '\0')
                        {
                            if (colorSeedOption != -1)
                            {
                        #ifdef NETWORK
                                if (!Master)
                                {
                            #endif
                                    // Ready to run.
                                    glutKeyboardFunc(NULL);
                                    UserMode = RUN;
                                    frameRate.reset();
                            #ifdef NETWORK
                                    network->init(Xwings[myXwing].id, Xwings[myXwing].colorSeed);
                                }
                                else
                                {
                                    OptionMode = GET_SLAVE;
                                    UserMode = OPTIONS;
                                }
                        #endif
                            }
                            else
                            {
                                OptionMode = GET_COLOR;
                                UserMode = OPTIONS;
                            }
                        }
                        else
                        {
                            OptionMode = GET_ID;
                            UserMode = OPTIONS;
                        }
                }
                DataIndex = 0;
                DataString[0] = '\0';
                break;

                // Options entry.
            case OPTIONS:
                if (key >= 0 && key < 123)
                {
                    i = key;
                    if (i >= 'A' && i <= 'Z') i = key - 'A' + 'a';
                    skipChars[i] = true;
                }
                if (key != BACKSPACE_KEY)
                {
                    if (DataIndex < DATA_STRING_SIZE && key != RETURN_KEY)
                    {
                        DataString[DataIndex] = key;
                        DataIndex++;
                        DataString[DataIndex] = '\0';
                    }
                }
                else
                {
                    if (DataIndex > 0) DataIndex--;
                    DataString[DataIndex] = '\0';
                }

                switch(OptionMode)
                {

                    case GET_ID:
                        if (key == RETURN_KEY)
                        {
                            if (DataString[0] == '\0')
                            {
                                idOption[0] = '\0';
                            }
                            else
                            {
                                strncpy(idOption, DataString, ID_LENGTH);
                            }
                            strcpy(Xwings[myXwing].id, idOption);
                            if (colorSeedOption == -1)
                            {
                                OptionMode = GET_COLOR;
                                DataIndex = 0;
                                DataString[0] = '\0';
                        #ifdef NETWORK
                            } else if (Master)
                            {
                                OptionMode = GET_SLAVE;
                                DataIndex = 0;
                                DataString[0] = '\0';
                        #endif
                            }
                            else
                            {
                                // Mark X-wing and run.
                                delete Xwings[myXwing].xwing;
                                Xwings[myXwing].xwing = new Xwing(Xwings[myXwing].id, Xwings[myXwing].colorSeed);
                                resurrectXwing(myXwing);
                                glutKeyboardFunc(NULL);
                                UserMode = RUN;
                                frameRate.reset();
                        #ifdef NETWORK
                                network->init(Xwings[myXwing].id, Xwings[myXwing].colorSeed);
                        #endif
                            }
                        }
                        break;

                    case GET_COLOR:
                        if (key == RETURN_KEY)
                        {
                            if (DataString[0] == '\0')
                            {
                                colorSeedOption = -1;
                            }
                            else
                            {
                                colorSeedOption = atoi(DataString);
                            }
                            Xwings[myXwing].colorSeed = colorSeedOption;
                    #ifdef NETWORK
                            if (Master)
                            {
                                OptionMode = GET_SLAVE;
                                DataIndex = 0;
                                DataString[0] = '\0';
                            }
                            else
                            {
                        #endif
                                // Mark X-wing and run.
                                delete Xwings[myXwing].xwing;
                                Xwings[myXwing].xwing = new Xwing(Xwings[myXwing].id, Xwings[myXwing].colorSeed);
                                resurrectXwing(myXwing);
                                glutKeyboardFunc(NULL);
                                UserMode = RUN;
                                frameRate.reset();
                        #ifdef NETWORK
                                network->init(Xwings[myXwing].id, Xwings[myXwing].colorSeed);
                            }
                    #endif
                        }
                        break;

                #ifdef NETWORK
                    case GET_SLAVE:
                        if (key == 'y' || key == 'Y')
                        {
                            Master = false;
                            OptionMode = GET_IP;
                            DataIndex = 0;
                            DataString[0] = '\0';
                        }
                        else
                        {
                            // Mark X-wing and run.
                            delete Xwings[myXwing].xwing;
                            Xwings[myXwing].xwing = new Xwing(Xwings[myXwing].id, Xwings[myXwing].colorSeed);
                            resurrectXwing(myXwing);
                            glutKeyboardFunc(NULL);
                            UserMode = RUN;
                            frameRate.reset();
                            network->init(Xwings[myXwing].id, Xwings[myXwing].colorSeed);
                        }
                        break;

                    case GET_IP:
                        if (key == RETURN_KEY)
                        {
                            if (DataString[0] != '\0')
                            {
                                strncpy(MasterIP, DataString, IP_LENGTH);

                                // Mark X-wing and run.
                                delete Xwings[myXwing].xwing;
                                Xwings[myXwing].xwing = new Xwing(Xwings[myXwing].id, Xwings[myXwing].colorSeed);
                                resurrectXwing(myXwing);
                                glutKeyboardFunc(NULL);
                                UserMode = RUN;
                                frameRate.reset();
                                network->init(Xwings[myXwing].id, Xwings[myXwing].colorSeed);
                            }
                        }
                        break;
                #endif
                }
                break;

                // Help.
            case HELP:
                if (key >= 0 && key < 123 && skipChars[key])
                {
                    skipChars[key] = false;
                    break;
                }
                if (key == 'h')                   // Twice prints control help.
                {
                    for (int i = 0; ControlInfo[i] != NULL; i++)
                    {
                        printf("%s\n", ControlInfo[i]);
                    }
                    break;
                }
                UserMode = RUN;
                frameRate.reset();
                break;

                // Message.
            case MESSAGE:
                if (key >= 0 && key < 123 && skipChars[key])
                {
                    skipChars[key] = false;
                    break;
                }
                switch(key)
                {
                    case 'h': UserMode = HELP; break;
                    case 'q':
                        freeSounds();
                #ifdef NETWORK
                        // Notify other game instances.
                        network->exitNotify(Network::QUIT);
                #endif
                        exit(0);
                    default:
                        UserMode = RUN;
                        frameRate.reset();
                        break;
                }
                break;

                // Fatal error.
            case FATAL:
                if (key >= 0 && key < 123 && skipChars[key])
                {
                    skipChars[key] = false;
                    break;
                }
                freeSounds();
            #ifdef NETWORK
                // Notify other game instances.
                network->exitNotify(Network::QUIT);
            #endif
                exit(0);

                // Who.
            case WHO:
                if (key >= 0 && key < 123 && skipChars[key])
                {
                    skipChars[key] = false;
                    break;
                }
                switch(key)
                {
                    case 'h': UserMode = HELP; break;
                    case 'q':
                        freeSounds();
                #ifdef NETWORK
                        // Notify other game instances.
                        network->exitNotify(Network::QUIT);
                #endif
                        exit(0);
                    default:
                        UserMode = RUN;
                        frameRate.reset();
                        break;
                }
                break;

                // Run mode.
            case RUN:
                if (key >= 0 && key < 123 && skipChars[key])
                {
                    skipChars[key] = false;
                    break;
                }
                switch(key)
                {
                    // X-wing user actions.
                    case 'f':
                        if (xwing->state == Xwing::EXPLODE || xwing->state == Xwing::DEAD) break;
                        Xwings[myXwing].speed += SPEED_DELTA;
                        if (Xwings[myXwing].speed > MAX_SPEED) Xwings[myXwing].speed = MAX_SPEED;
                        xwing->SetSpeed(Xwings[myXwing].speed);
                        if (shipEngineChannel != -1)
                        {
                            FSOUND_SetVolume(shipEngineChannel,
                                (int)(Xwings[myXwing].speed * 255.0 / (double)MAX_SPEED));
                        }
                        break;
                    case 's':
                        if (xwing->state == Xwing::EXPLODE || xwing->state == Xwing::DEAD) break;
                        Xwings[myXwing].speed -= SPEED_DELTA;
                        if (Xwings[myXwing].speed < 0.0) Xwings[myXwing].speed = 0.0;
                        xwing->SetSpeed(Xwings[myXwing].speed);
                        if (shipEngineChannel != -1)
                        {
                            FSOUND_SetVolume(shipEngineChannel,
                                (int)(Xwings[myXwing].speed * 255.0 / (double)MAX_SPEED));
                        }
                        break;
                    case ' ':
                        if (xwing->state == Xwing::EXPLODE || xwing->state == Xwing::DEAD) break;

                        // Fire plasma bolt.
                        if (Xwings[myXwing].shotCount < MAX_SHOTS)
                        {
                    #ifdef NETWORK
                            if (Master)
                            {
                                plasmaBolts->add(xwing->fire());
                                network->setPlasmaBoltUpdated();
                            }
                            else
                            {
                                network->fire(xwing->fire());
                            }
                    #else
                            plasmaBolts->add(xwing->fire());
                    #endif
                            Xwings[myXwing].shotCount++;

                            // Play shot sound.
                            if (plasmaBoltSound && !muteMode)
                                FSOUND_PlaySound(FSOUND_FREE, plasmaBoltSound);
                        }
                        break;
                    case '1':
                        if (xwing->state == Xwing::EXPLODE || xwing->state == Xwing::DEAD) break;
                        Xwings[myXwing].roll += ROTATION_DELTA;
                        xwing->SetRoll(Xwings[myXwing].roll);
                        break;
                    case '2':
                        if (xwing->state == Xwing::EXPLODE || xwing->state == Xwing::DEAD) break;
                        Xwings[myXwing].roll = 0.0;
                        xwing->SetRoll(Xwings[myXwing].roll);
                        break;
                    case '3':
                        if (xwing->state == Xwing::EXPLODE || xwing->state == Xwing::DEAD) break;
                        Xwings[myXwing].roll -= ROTATION_DELTA;
                        xwing->SetRoll(Xwings[myXwing].roll);
                        break;
                    case '4':
                        if (xwing->state == Xwing::EXPLODE || xwing->state == Xwing::DEAD) break;
                        Xwings[myXwing].yaw += ROTATION_DELTA;
                        xwing->SetYaw(Xwings[myXwing].yaw);
                        break;
                    case '5':
                        if (xwing->state == Xwing::EXPLODE || xwing->state == Xwing::DEAD) break;
                        Xwings[myXwing].yaw = 0.0;
                        xwing->SetYaw(Xwings[myXwing].yaw);
                        break;
                    case '6':
                        if (xwing->state == Xwing::EXPLODE || xwing->state == Xwing::DEAD) break;
                        Xwings[myXwing].yaw -= ROTATION_DELTA;
                        xwing->SetYaw(Xwings[myXwing].yaw);
                        break;
                    case '7':
                        if (xwing->state == Xwing::EXPLODE || xwing->state == Xwing::DEAD) break;
                        Xwings[myXwing].pitch += ROTATION_DELTA;
                        xwing->SetPitch(Xwings[myXwing].pitch);
                        break;
                    case '8':
                        if (xwing->state == Xwing::EXPLODE || xwing->state == Xwing::DEAD) break;
                        Xwings[myXwing].pitch = 0.0;
                        xwing->SetPitch(Xwings[myXwing].pitch);
                        break;
                    case '9':
                        if (xwing->state == Xwing::EXPLODE || xwing->state == Xwing::DEAD) break;
                        Xwings[myXwing].pitch -= ROTATION_DELTA;
                        xwing->SetPitch(Xwings[myXwing].pitch);
                        break;
                    case 'd':                     // Debug mode. Use 'n' to step.
                        debugMode = !debugMode;
                        break;
                    case 'g':                     // Toggle wall grid drawing.
                        DrawWalls = !DrawWalls;
                        break;
                    case 'm':                     // Mute mode.
                        muteMode = !muteMode;
                        break;
                    case 'o':
                        if (!xwing->IsAlive()) break;
                        SpotLightSwitch = !SpotLightSwitch;
                        if (SpotLightSwitch)
                        {
                            glEnable(GL_LIGHT1);
                        }
                        else
                        {
                            glDisable(GL_LIGHT1);
                        }
                        break;
                    case 'p':                     // Paint blocks.
                        PaintBlocks = !PaintBlocks;
                        if (!BlockTextureLoaded) PaintBlocks = false;
                        break;
                #ifndef NETWORK
                    case 'r':                     // Show frame rate.
                        ShowFrameRate = !ShowFrameRate;
                        break;
                #endif
                    case 'v':                     // Rear view.
                        RearView = !RearView;
                        break;
                    case 'w':                     // List who is playing.
                        UserMode = WHO;
                        break;
                    case 'h':                     // Control help.
                        UserMode = HELP;
                        break;
                    case 'q':                     // Quit.
                        freeSounds();
                #ifdef UNIX_STDIN
                        set_tty_cooked();         // restore normal TTY mode
                #endif
                #ifdef NETWORK
                        // Notify other game instances.
                        network->exitNotify(Network::QUIT);
                #endif
                        exit(0);
                    case 'b':
                        ShowBoundingBlocks = !ShowBoundingBlocks;
                        break;
                #ifndef NETWORK
                    case 'c':                     // Create another ship if possible.
                        createXwing();
                        break;
                    case 'j':                     // Jump to next ship.
                        for (i = (myXwing + 1) % NUM_XWINGS; i != myXwing; i = (i + 1) % NUM_XWINGS)
                        {
                            if (Xwings[i].xwing->IsAlive()) break;
                        }
                        myXwing = i;
                        if (shipEngineChannel != -1)
                        {
                            FSOUND_SetVolume(shipEngineChannel,
                                (int)(Xwings[myXwing].speed * 255.0 / (double)MAX_SPEED));
                        }
                        break;
                #endif
                    case 'x':
                        Xwings[myXwing].shotCount = 0;
                        break;
                    case 'y':
                        Xwings[myXwing].invulnerable = !Xwings[myXwing].invulnerable;
                    default: return;
                }
                break;

                // End game modes.
            case WIN:
            case LOSE:
                if (finiSoundPlayed)
                {
                    if (finiChannel != -1)
                    {
                        while (FSOUND_IsPlaying(finiChannel))
                    #ifdef UNIX
                            sleep(1);
                    #else
                        Sleep(1000);
                    #endif
                    }
                    freeSounds();
                #ifdef NETWORK
                    // Notify other game instances.
                    if (UserMode == WIN)
                    {
                        network->exitNotify(Network::WINNER);
                    }
                    else
                    {
                        network->exitNotify(Network::KILLED);
                    }
                #endif
                    exit(0);
                }
                break;
        }

        // Force re-display.
        glutPostRedisplay();
    }

    // Special keyboard input.
    void
        specialKeyInput(int key, int x, int y)
    {
        Xwing *xwing;

        xwing = Xwings[myXwing].xwing;

        if (xwing->state == Xwing::EXPLODE || xwing->state == Xwing::DEAD) return;
        switch(key)
        {
            case GLUT_KEY_UP:
                Xwings[myXwing].pitch += ROTATION_DELTA;
                xwing->SetPitch(Xwings[myXwing].pitch);
                break;
            case GLUT_KEY_DOWN:
                Xwings[myXwing].pitch -= ROTATION_DELTA;
                xwing->SetPitch(Xwings[myXwing].pitch);
                break;
            case GLUT_KEY_RIGHT:
                Xwings[myXwing].yaw += ROTATION_DELTA;
                xwing->SetYaw(Xwings[myXwing].yaw);
                break;
            case GLUT_KEY_LEFT:
                Xwings[myXwing].yaw -= ROTATION_DELTA;
                xwing->SetYaw(Xwings[myXwing].yaw);
                break;
            default: return;
        }

        // Force re-display.
        glutPostRedisplay();
    }

    // Poll for keyboard input.
    void
        pollKeys()
    {
        #ifdef UNIX_STDIN
        unsigned char key;

        // Get a character.
        if ((key = kb_getc()) == 0) return;

        switch(key)
        {
            case 0x41: specialKeyInput(GLUT_KEY_UP, 0, 0); break;
            case 0x42: specialKeyInput(GLUT_KEY_DOWN, 0, 0); break;
            case 0x43: specialKeyInput(GLUT_KEY_RIGHT, 0, 0); break;
            case 0x44: specialKeyInput(GLUT_KEY_LEFT, 0, 0); break;
            case 'n':  glutPostRedisplay(); break;// Step next for debugging.
            default:   keyInput(key, 0, 0);
        }

        #else

        static int lastShortTime = 0;
        static int lastLongTime = 0;
        int currentTime;

        // Short delay input.
        currentTime = GetTickCount();
        if ((currentTime - lastShortTime) >= KEY_INPUT_SHORT_DELAY)
        {
            lastShortTime = currentTime;

            if(GetAsyncKeyState('1')) keyInput('1', 0, 0);
            if(GetAsyncKeyState('3')) keyInput('3', 0, 0);
            if(GetAsyncKeyState('F')) keyInput('f', 0, 0);
            if(GetAsyncKeyState('S')) keyInput('s', 0, 0);
            if(GetAsyncKeyState(VK_UP)) specialKeyInput(GLUT_KEY_UP, 0, 0);
            if(GetAsyncKeyState(VK_DOWN)) specialKeyInput(GLUT_KEY_DOWN, 0, 0);
            if(GetAsyncKeyState(VK_RIGHT)) specialKeyInput(GLUT_KEY_RIGHT, 0, 0);
            if(GetAsyncKeyState(VK_LEFT)) specialKeyInput(GLUT_KEY_LEFT, 0, 0);
        }

        // Long delay input.
        if ((currentTime - lastLongTime) < KEY_INPUT_LONG_DELAY) return;
        lastLongTime = currentTime;

        if(GetAsyncKeyState('B')) keyInput('b', 0, 0);
        if(GetAsyncKeyState('C')) keyInput('c', 0, 0);
        if(GetAsyncKeyState('D')) keyInput('d', 0, 0);
        if(GetAsyncKeyState('G')) keyInput('g', 0, 0);
        if(GetAsyncKeyState('H')) keyInput('h', 0, 0);
        if(GetAsyncKeyState('J')) keyInput('j', 0, 0);
        if(GetAsyncKeyState('M')) keyInput('m', 0, 0);
                                                  // Step next for debugging.
        if(GetAsyncKeyState('N')) glutPostRedisplay();
        if(GetAsyncKeyState('O')) keyInput('o', 0, 0);
        if(GetAsyncKeyState('P')) keyInput('p', 0, 0);
        if(GetAsyncKeyState('Q')) keyInput('q', 0, 0);
        if(GetAsyncKeyState('R')) keyInput('r', 0, 0);
        if(GetAsyncKeyState('V')) keyInput('v', 0, 0);
        if(GetAsyncKeyState('W')) keyInput('w', 0, 0);
        if(GetAsyncKeyState('X')) keyInput('x', 0, 0);
        if(GetAsyncKeyState('Y')) keyInput('y', 0, 0);
        if(GetAsyncKeyState(VK_SPACE)) keyInput(' ', 0, 0);
        #endif
    }

    // Light controls.
    GLboolean lightZeroSwitch = GL_TRUE, lightOneSwitch = GL_TRUE;

    void
        controlLights(int value)
    {
        switch (value)
        {
            case 1:
                ShipLightSwitch = !ShipLightSwitch;
                if (ShipLightSwitch)
                {
                    glEnable(GL_LIGHT0);
                }
                else
                {
                    glDisable(GL_LIGHT0);
                }
                break;
            case 2:
                glutFullScreen();
                break;
            case 3:
                exit(0);
        }
        glutPostRedisplay();
    }

    // Idle function.
    void
        idle(void)
    {
        Xwing *xwing;

        xwing = Xwings[myXwing].xwing;

        // Poll for keyboard input.
        #if ( !UNIX || UNIX_STDIN )
        if (UserMode != INTRO && UserMode != OPTIONS) pollKeys();
        #endif

        if (UserMode != RUN)
        {
            if (shipEngineChannel != -1 && !shipEnginePaused)
            {
                FSOUND_SetPaused(shipEngineChannel, TRUE);
                shipEnginePaused = true;
            }
            if (UserMode == WIN)
            {
                if (!finiSoundPlayed)
                {
                    finiSoundPlayed = true;
                    if (youWinSound && !muteMode)
                        finiChannel = FSOUND_PlaySound(FSOUND_FREE, youWinSound);
                }
            }
            if (UserMode == LOSE)
            {
                if (!finiSoundPlayed)
                {
                    finiSoundPlayed = true;
                    if (youLoseSound && !muteMode)
                        finiChannel = FSOUND_PlaySound(FSOUND_FREE, youLoseSound);
                }
            }
        }
        else
        {
            if (shipEngineChannel != -1 && shipEnginePaused && !muteMode)
            {
                FSOUND_SetPaused(shipEngineChannel, FALSE);
                shipEnginePaused = false;
            }
            if (shipEngineChannel != -1 && !shipEnginePaused &&
                (muteMode || !xwing->IsAlive()))
            {
                FSOUND_SetPaused(shipEngineChannel, TRUE);
                shipEnginePaused = true;
            }
        }

        // Force re-display.
        if (!debugMode) glutPostRedisplay(); else frameRate.reset();
    }

    // Window visibility.
    void
        visible(int state)
    {
        // Reset frame rate counter.
        frameRate.reset();

        if (state == GLUT_VISIBLE)
        {
            // Restart movement.
            glutIdleFunc(idle);
        }
        else
        {

            // Idle game while not visible.
            glutIdleFunc(NULL);

            // Turn off engine sound.
            if (shipEngineChannel != -1 && !shipEnginePaused)
            {
                FSOUND_SetPaused(shipEngineChannel, TRUE);
                shipEnginePaused = true;
            }
        }
    }

    int
        main(int argc, char **argv)
    {
        int i,j;
        char id[ID_LENGTH+1];
        bool fullscreen = false;
        #ifndef HELLBOX
        TextureImage t;
        #endif
        class cSpacial *spacial;
        GLfloat v[3];
        Vector axis;
        float angle;

        // Initialize.
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
        glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
        glutCreateWindow(NAME);
        glutDisplayFunc(display);
        #ifndef NETWORK
        glutVisibilityFunc(visible);
        #endif
        glutReshapeFunc(reshape);
        glutIdleFunc(idle);
        glutKeyboardFunc(keyInput);
        #if ( UNIX && !UNIX_STDIN )
        glutSpecialFunc(specialKeyInput);
        #endif
        glutCreateMenu(controlLights);
        glutAddMenuEntry("Toggle light", 1);
        glutAddMenuEntry("Full screen", 2);
        glutAddMenuEntry("Exit", 3);
        glutAttachMenu(GLUT_RIGHT_BUTTON);

        #ifdef UNIX_STDIN
        // Set raw tty mode.
        set_tty_raw();
        #endif

        // Set initial mode.
        UserMode = INTRO;
        idOption[0] = '\0';
        colorSeedOption = -1;
        for (i = 0; i < 123; i++) skipChars[i] = false;

        // Display settings.
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Camera.
        glMatrixMode(GL_PROJECTION);
        gluPerspective(FRUSTUM_ANGLE, FRUSTUM_ASPECT, FRUSTUM_NEAR, FRUSTUM_FAR);
        gluLookAt(CAMERA_X, CAMERA_Y, CAMERA_Z, 0.0, 0.0, 0.0, 0.0, 1.0, 0.);

        // Get options.
        for (i = 1; i < argc;)
        {
            if (strcmp(argv[i], "-fullscreen") == 0)
            {
                fullscreen = true;
                i++;
                continue;
            }
            if (strcmp(argv[i], "-id") == 0)
            {
                i++;
                if (i < argc)
                {
                    strncpy(idOption, argv[i], ID_LENGTH);
                }
                else
                {
                    sprintf(UserMessage, Usage, argv[0]);
                    UserMode = FATAL;
                    break;
                }
                i++;
                continue;
            }
            if (strcmp(argv[i], "-color") == 0)
            {
                i++;
                if (i < argc)
                {
                    colorSeedOption = atoi(argv[i]);
                }
                else
                {
                    sprintf(UserMessage, Usage, argv[0]);
                    UserMode = FATAL;
                    break;
                }
                i++;
                continue;
            }
            #ifdef NETWORK
            // Connect to master game?
            if (strcmp(argv[i], "-connect") == 0)
            {
                i++;
                if (i < argc)
                {
                    strncpy(MasterIP, argv[i], IP_LENGTH);
                    Master = false;
                }
                else
                {
                    sprintf(UserMessage, Usage, argv[0]);
                    UserMode = FATAL;
                    break;
                }
                i++;
                continue;
            }
            #endif
            sprintf(UserMessage, Usage, argv[0]);
            UserMode = FATAL;
            break;
        }

        // Create X-wings.
        myXwing = 0;                              // User's X-wing.
        for (i = 0; i < NUM_XWINGS; i++)
        {
            sprintf(id, "Straw ratS %d", i);
            if (i == 0)
            {
                if (idOption[0] != '\0')
                {
                    Xwings[i].xwing = new Xwing(idOption, colorSeedOption);
                    strcpy(Xwings[i].id, idOption);
                }
                else
                {
                    Xwings[i].xwing = new Xwing(id, colorSeedOption);
                    strcpy(Xwings[i].id, id);
                }
                Xwings[i].colorSeed = colorSeedOption;
            }
            else
            {
                Xwings[i].xwing = new Xwing(id, colorSeedOption);
                strcpy(Xwings[i].id, id);
                Xwings[i].colorSeed = -1;
            }
            Xwings[i].pitch = 180.0;
            Xwings[i].yaw = 0.0;
            Xwings[i].roll = 0.0;
            Xwings[i].speed = 0.0;
            Xwings[i].xwing->SetPitch(Xwings[i].pitch);
            Xwings[i].bodyGroup = FIRST_XWING_BLOCK + (4 * i);
            Xwings[i].collisionSteps = 0;
            Xwings[i].firstBump = true;
            Xwings[i].shotCount = 0;
            Xwings[i].invulnerable = false;
        }

        // Set camera delay.
        Xwings[myXwing].xwing->GetForward(CameraDelay[0].f);
        Xwings[myXwing].xwing->GetUp(CameraDelay[0].u);
        for (i = 1; i < CAMERA_DELAY_SIZE; i++)
        {
            CameraDelay[i].f[0] = CameraDelay[0].f[0];
            CameraDelay[i].f[1] = CameraDelay[0].f[1];
            CameraDelay[i].f[2] = CameraDelay[0].f[2];
            CameraDelay[i].u[0] = CameraDelay[0].u[0];
            CameraDelay[i].u[1] = CameraDelay[0].u[1];
            CameraDelay[i].u[2] = CameraDelay[0].u[2];
        }
        CameraDelayIndex = 0;

        // Create plasma bolt set to contain fired bolts.
        plasmaBolts = new PlasmaBoltSet();

        // Create squids.
        srand(time(NULL));
        for (i = 0; i < NUM_SQUIDS; i++)
        {
            Squids[i].squid = new Squid();
            Squids[i].squid->SetScale(0.5);
            Squids[i].bodyGroup = FIRST_SQUID_BLOCK + (2 * i);
            Squids[i].collisionSteps = 0;
            if (rand()%2 == 1)
            {
                Squids[i].attackRange = SQUID_ATTACK_RANGE +
                    (float)(rand()%((int)(SQUID_ATTACK_RANDOM_VARIANCE * 100.0) + 1)) / 100.0;
            }
            else
            {
                Squids[i].attackRange = SQUID_ATTACK_RANGE -
                    (float)(rand()%((int)(SQUID_ATTACK_RANDOM_VARIANCE * 100.0) + 1)) / 100.0;
            }
            if (Squids[i].attackRange < 0.0) Squids[i].attackRange = 0.0;
        }

        // Create blocks.
        InitializePhysics();
        j = NUM_WALL_BLOCKS + NUM_FIXED_BLOCKS + NUM_BLOCKS + (NUM_XWINGS * NUM_XWING_BLOCKS);
        NumBodies = j + (NUM_SQUIDS * NUM_SQUID_BLOCKS);
        if (NumBodies >= MAX_BODIES)
        {
            sprintf(UserMessage, "Block overflow: Numbodies=%d, MAX_BODIES=%d\n", NumBodies, MAX_BODIES);
            UserMode = FATAL;
            NumBodies = MAX_BODIES - 1;
        }
        for (i = 0; i < j; i++)
        {
            createBlock(i);
        }
        for (i = j; i < NumBodies; i += 2)
        {
            createBlock(i);                       // Create squid bounding boxes in pairs.
        }

        // Co-locate squids with bounding blocks.
        for (i = 0; i < NUM_SQUIDS; i++)
        {
            if (!Bodies[Squids[i].bodyGroup].valid)
            {
                Squids[i].squid->Kill();
                continue;
            }
            spacial = Squids[i].squid->GetSpacial();
            spacial->x = Bodies[Squids[i].bodyGroup].vPosition.x;
            spacial->y = Bodies[Squids[i].bodyGroup].vPosition.y;
            spacial->z = Bodies[Squids[i].bodyGroup].vPosition.z;
            angle = QGetAngle(Bodies[Squids[i].bodyGroup].qOrientation);
            axis = QGetAxis(Bodies[Squids[i].bodyGroup].qOrientation);
            v[0] = axis.x;
            v[1] = axis.y;
            v[2] = axis.z;
            spacial->qcalc->loadRotation(angle, v);
            spacial->qcalc->build_rotmatrix(spacial->rotmatrix, spacial->qcalc->quat);
        }

        #ifdef HELLBOX
        // Create box display lists.
        buildFixedBlockDisplays();
        buildBlockDisplays();
        #endif

        // Load block textures.
        #ifdef HELLBOX
        BlockTextureLoaded = true;
        if (!CreateBmpTexture(HELLBOX_BMP1_IMAGE, &BlockTextureName[0]))
        {
            sprintf(UserMessage, "Cannot load texture %s\n", HELLBOX_BMP1_IMAGE);
            UserMode = FATAL;
            BlockTextureLoaded = false;
        }
        if (!CreateBmpTexture(HELLBOX_BMP2_IMAGE, &BlockTextureName[1]))
        {
            sprintf(UserMessage, "Cannot load texture %s\n", HELLBOX_BMP2_IMAGE);
            UserMode = FATAL;
            BlockTextureLoaded = false;
        }
        if (!CreateBmpTexture(HELLBOX_BMP3_IMAGE, &BlockTextureName[2]))
        {
            sprintf(UserMessage, "Cannot load texture %s\n", HELLBOX_BMP3_IMAGE);
            UserMode = FATAL;
            BlockTextureLoaded = false;
        }
        if (!CreateBmpTexture(HELLBOX_BMP4_IMAGE, &BlockTextureName[3]))
        {
            sprintf(UserMessage, "Cannot load texture %s\n", HELLBOX_BMP4_IMAGE);
            UserMode = FATAL;
            BlockTextureLoaded = false;
        }
        if (!CreateBmpTexture(HELLBOX_BMP5_IMAGE, &BlockTextureName[4]))
        {
            sprintf(UserMessage, "Cannot load texture %s\n", HELLBOX_BMP5_IMAGE);
            UserMode = FATAL;
            BlockTextureLoaded = false;
        }
        if (!CreateBmpTexture(HELLBOX_BMP6_IMAGE, &BlockTextureName[5]))
        {
            sprintf(UserMessage, "Cannot load texture %s\n", HELLBOX_BMP6_IMAGE);
            UserMode = FATAL;
            BlockTextureLoaded = false;
        }
        #else
        if (LoadTGA(&t, BLOCK_TGA_IMAGE))
        {
            BlockTextureName = t.texID;
            BlockTextureLoaded = true;
        }
        else
        {
            sprintf(UserMessage, "Cannot load texture %s\n", BLOCK_TGA_IMAGE);
            UserMode = FATAL;
            BlockTextureLoaded = false;
        }
        #endif
        if (!BlockTextureLoaded) PaintBlocks = false;

        // Build star field display.
        buildStarDisplay();

        // Create explosion.
        explosion = new cExplosion(NUM_EXPLOSION_PARTICLES);
        explosion->KillParticles();

        // Create lights.
        glEnable(GL_LIGHTING);
        glLightfv(GL_LIGHT0, GL_POSITION, ShipLightPosition);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, ShipLightColor);
        glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.0);
        glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.5);
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT1, GL_POSITION, SpotLightPosition);
        glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, SpotLightDirection);
        glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 7.5);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, SpotLightColor);
        glLightfv(GL_LIGHT1, GL_SPECULAR, SpotLightColor);
        glEnable(GL_LIGHT1);

        // Get camera frustum.
        frustum = new Frustum();

        // Use full screen?
        #ifndef UNIX
        if (!fullscreen)
        {
            if (MessageBox(NULL, "Full screen mode?", NAME, MB_YESNO) == IDYES)
            {
                fullscreen = true;
            }
        }
        #endif
        if (fullscreen) glutFullScreen();

        // Get sound effects.
        getSounds();

        // Kill extra X-wings: can re-animate with 'c' key.
        for (i = 1; i < NUM_XWINGS; i++)
        {
            Xwings[i].xwing->Kill();
            for (j = Xwings[i].bodyGroup; Bodies[j].group == Xwings[i].bodyGroup && j < NumBodies; j++)
            {
                Bodies[j].valid = false;
            }
        }

        #ifdef NETWORK
        network = new Network();
        #endif

        // Start up.
        glutMainLoop();
        return 0;
    }

    // Create randomized block.
    void
        createBlock(int index)
    {
        int i,k,xi,xb,si,sb;
        float f;
        GLfloat p[3];

        // Walls are six large cubes surrounding play space.
        if (index >= FIRST_WALL_BLOCK && index < (FIRST_WALL_BLOCK + NUM_WALL_BLOCKS))
        {
            // Default initialization.
            InitializeObject(index, WALL_SIZE, WALL_TYPE, -1);

            // Shift block to form a wall.
            switch(index - FIRST_WALL_BLOCK)
            {
                case 0:
                    // Top.
                    Bodies[index].vPosition.y += WALL_SIZE;
                    break;
                case 1:
                    // Right.
                    Bodies[index].vPosition.x += WALL_SIZE;
                    break;
                case 2:
                    // Back.
                    Bodies[index].vPosition.z += WALL_SIZE;
                    break;
                case 3:
                    // Bottom.
                    Bodies[index].vPosition.y -= WALL_SIZE;
                    break;
                case 4:
                    // Left.
                    Bodies[index].vPosition.x -= WALL_SIZE;
                    break;
                case 5:
                    // Front.
                    Bodies[index].vPosition.z -= WALL_SIZE;
                    break;
            }

            // Build display list.
            buildWallDisplay(index);
            return;
        }

        // Build X-wing bounding boxes.
        if (index >= FIRST_XWING_BLOCK && index < (FIRST_XWING_BLOCK + (NUM_XWING_BLOCKS * NUM_XWINGS)))
        {
            // Default initialization.
            xi = (index - FIRST_XWING_BLOCK) / NUM_XWING_BLOCKS;
            xb = Xwings[xi].bodyGroup;
            InitializeObject(index, 1.0, XWING_BLOCK_TYPE, xb);

            // Set vertex positions.
            k = (index - FIRST_XWING_BLOCK) % NUM_XWING_BLOCKS;
            switch(k)
            {
                case 0:
                    // Cockpit.
                    setBlockVertices(index,
                        COCKPIT_XMIN, COCKPIT_XMAX,
                        COCKPIT_YMIN, COCKPIT_YMAX,
                        COCKPIT_ZMIN, COCKPIT_ZMAX
                        );
                    break;
                case 1:
                    // Engines.
                    setBlockVertices(index,
                        ENGINES_XMIN, ENGINES_XMAX,
                        ENGINES_YMIN, ENGINES_YMAX,
                        ENGINES_ZMIN, ENGINES_ZMAX
                        );
                    break;
                case 2:
                    // Fuselage.
                    setBlockVertices(index,
                        FUSELAGE_XMIN, FUSELAGE_XMAX,
                        FUSELAGE_YMIN, FUSELAGE_YMAX,
                        FUSELAGE_ZMIN, FUSELAGE_ZMAX
                        );
                    break;
                case 3:
                    // Wings.
                    setBlockVertices(index,
                        WINGS_XMIN, WINGS_XMAX,
                        WINGS_YMIN, WINGS_YMAX,
                        WINGS_ZMIN, WINGS_ZMAX
                        );
                    break;
            }

            // Randomize first block position.
            if (k == 0)
            {
                // Try to position non-overlapping block.
                if (!positionBlock(index))
                {
                    Bodies[index].valid = false;
                    return;
                }

                // Set X-wing position.
                p[0] = Bodies[index].vPosition.x;
                p[1] = Bodies[index].vPosition.y;
                p[2] = Bodies[index].vPosition.z;
                Xwings[xi].xwing->SetPosition(p);

            }
            else
            {

                // Other blocks have same position.
                Bodies[index].vPosition = Bodies[xb].vPosition;
            }

            // Select a random color.
            Bodies[index].red = (float)(rand()%256) / 255.0;
            Bodies[index].green = (float)(rand()%256) / 255.0;
            Bodies[index].blue = (float)(rand()%256) / 255.0;

            // Build display list.
            if (index >= FIRST_XWING_BLOCK && index < (FIRST_XWING_BLOCK + NUM_XWING_BLOCKS))
            {
                buildBlockDisplay(index);
            }
            else
            {
                Bodies[index].display = Bodies[FIRST_XWING_BLOCK + k].display;
            }
            return;
        }

        // Create fixed blocks.
        if (index >= FIRST_FIXED_BLOCK && index < (FIRST_FIXED_BLOCK + NUM_FIXED_BLOCKS))
        {
            // Default initialization.
            InitializeObject(index, FIXED_BLOCK_SIZE, FIXED_BLOCK_TYPE, -1);

            // Position blocks near corners.
            f = WALL_SIZE / 6.0;
            switch(index - FIRST_FIXED_BLOCK)
            {
                case 0:
                    Bodies[index].vPosition.x = f;
                    Bodies[index].vPosition.y = f;
                    Bodies[index].vPosition.z = f;
                    break;
                case 1:
                    Bodies[index].vPosition.x = f;
                    Bodies[index].vPosition.y = f;
                    Bodies[index].vPosition.z = -f;
                    break;
                case 2:
                    Bodies[index].vPosition.x = f;
                    Bodies[index].vPosition.y = -f;
                    Bodies[index].vPosition.z = f;
                    break;
                case 3:
                    Bodies[index].vPosition.x = f;
                    Bodies[index].vPosition.y = -f;
                    Bodies[index].vPosition.z = -f;
                    break;
                case 4:
                    Bodies[index].vPosition.x = -f;
                    Bodies[index].vPosition.y = f;
                    Bodies[index].vPosition.z = f;
                    break;
                case 5:
                    Bodies[index].vPosition.x = -f;
                    Bodies[index].vPosition.y = f;
                    Bodies[index].vPosition.z = -f;
                    break;
                case 6:
                    Bodies[index].vPosition.x = -f;
                    Bodies[index].vPosition.y = -f;
                    Bodies[index].vPosition.z = f;
                    break;
                case 7:
                    Bodies[index].vPosition.x = -f;
                    Bodies[index].vPosition.y = -f;
                    Bodies[index].vPosition.z = -f;
                    break;
            }

            // Select a random color.
            Bodies[index].red = (float)(rand()%256) / 255.0;
            Bodies[index].green = (float)(rand()%256) / 255.0;
            Bodies[index].blue = (float)(rand()%256) / 255.0;

            #ifndef HELLBOX
            // Build display list.
            if (index == FIRST_FIXED_BLOCK)
            {
                buildBlockDisplay(index);
            }
            else
            {
                Bodies[index].display = Bodies[FIRST_FIXED_BLOCK].display;
            }
            #endif
            return;
        }

        // Create blocks.
        if (index >= FIRST_BLOCK && index < (FIRST_BLOCK + NUM_BLOCKS))
        {
            #ifdef HELLBOX
            InitializeObject(index, BLOCK_SIZE, BLOCK_TYPE, -1);
            #else
            // Randomized size.
            f = BLOCK_SIZE * ((float)(rand()%11) / 10.0);
            InitializeObject(index, BLOCK_SIZE + f, BLOCK_TYPE, -1);
            #endif

            // Try to position non-overlapping block.
            if (!positionBlock(index))
            {
                Bodies[index].valid = false;
                return;
            }

            // Randomize velocity.
            i = rand()%201 - 100;
            Bodies[index].vVelocity.x = MAX_VELOCITY * ((float)i / 100.0);
            i = rand()%201 - 100;
            Bodies[index].vVelocity.y = MAX_VELOCITY * ((float)i / 100.0);
            i = rand()%201 - 100;
            Bodies[index].vVelocity.z = MAX_VELOCITY * ((float)i / 100.0);
            if (Bodies[index].vVelocity.Magnitude() > MAX_VELOCITY)
            {
                Bodies[index].vVelocity.Normalize(MAX_VELOCITY);
            }
            i = rand()%201 - 100;
            Bodies[index].vAngularVelocity.x = MAX_ANGULAR_VELOCITY * ((float)i / 100.0);
            i = rand()%201 - 100;
            Bodies[index].vAngularVelocity.y = MAX_ANGULAR_VELOCITY * ((float)i / 100.0);
            i = rand()%201 - 100;
            Bodies[index].vAngularVelocity.z = MAX_ANGULAR_VELOCITY * ((float)i / 100.0);
            if (Bodies[index].vAngularVelocity.Magnitude() > MAX_ANGULAR_VELOCITY)
            {
                Bodies[index].vAngularVelocity.Normalize(MAX_ANGULAR_VELOCITY);
            }

            // Select a random color.
            Bodies[index].red = (float)(rand()%256) / 255.0;
            Bodies[index].green = (float)(rand()%256) / 255.0;
            Bodies[index].blue = (float)(rand()%256) / 255.0;

            #ifndef HELLBOX
            // Build display list.
            if (index == FIRST_BLOCK)
            {
                buildBlockDisplay(index);
            }
            else
            {
                Bodies[index].display = Bodies[FIRST_BLOCK].display;
            }
            #endif
            return;
        }

        // Create squid bounding boxes in pairs.
        if (index >= FIRST_SQUID_BLOCK && index < (FIRST_SQUID_BLOCK + (NUM_SQUIDS * NUM_SQUID_BLOCKS)))
        {
            // Default initialization.
            si = (index - FIRST_SQUID_BLOCK) / NUM_SQUID_BLOCKS;
            sb = Squids[si].bodyGroup;
            InitializeObject(index, 0.5, SQUID_BLOCK_TYPE, sb);
            for (i = 0; i < 8; i++)
            {
                Bodies[index].vVertexList[i].y += 0.1;
            }

            // Try to position non-overlapping block.
            if (!positionBlock(index))
            {
                Bodies[index].valid = false;
                return;
            }

            // Set squid position.
            p[0] = Bodies[index].vPosition.x;
            p[1] = Bodies[index].vPosition.y;
            p[2] = Bodies[index].vPosition.z;
            Squids[si].squid->SetPosition(p);

            // Create bounding box for tentacles.
            InitializeObject(index + 1, 0.5, SQUID_BLOCK_TYPE, sb);
            setBlockVertices(index + 1, -0.1, 0.1, -0.75, -0.25, -0.1, 0.1);
            Bodies[index + 1].vPosition = Bodies[index].vPosition;
            Bodies[index + 1].vVelocity = Bodies[index].vVelocity;
            Bodies[index + 1].vAngularVelocity = Bodies[index].vAngularVelocity;

            // Select a random color.
            Bodies[index].red = (float)(rand()%256) / 255.0;
            Bodies[index].green = (float)(rand()%256) / 255.0;
            Bodies[index].blue = (float)(rand()%256) / 255.0;
            Bodies[index + 1].red = (float)(rand()%256) / 255.0;
            Bodies[index + 1].green = (float)(rand()%256) / 255.0;
            Bodies[index + 1].blue = (float)(rand()%256) / 255.0;

            // Build display list.
            if (index == FIRST_SQUID_BLOCK)
            {
                buildBlockDisplay(index);
                buildBlockDisplay(index + 1);
            }
            else
            {
                Bodies[index].display = Bodies[FIRST_SQUID_BLOCK].display;
                Bodies[index + 1].display = Bodies[FIRST_SQUID_BLOCK + 1].display;
            }
            return;
        }
    }

    // Set block vertices.
    void
        setBlockVertices(int index, float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
    {
        Bodies[index].vVertexList[0].x = xmax;
        Bodies[index].vVertexList[0].y = ymax;
        Bodies[index].vVertexList[0].z = zmin;

        Bodies[index].vVertexList[1].x = xmax;
        Bodies[index].vVertexList[1].y = ymax;
        Bodies[index].vVertexList[1].z = zmax;

        Bodies[index].vVertexList[2].x = xmax;
        Bodies[index].vVertexList[2].y = ymin;
        Bodies[index].vVertexList[2].z = zmax;

        Bodies[index].vVertexList[3].x = xmax;
        Bodies[index].vVertexList[3].y = ymin;
        Bodies[index].vVertexList[3].z = zmin;

        Bodies[index].vVertexList[4].x = xmin;
        Bodies[index].vVertexList[4].y = ymax;
        Bodies[index].vVertexList[4].z = zmin;

        Bodies[index].vVertexList[5].x = xmin;
        Bodies[index].vVertexList[5].y = ymax;
        Bodies[index].vVertexList[5].z = zmax;

        Bodies[index].vVertexList[6].x = xmin;
        Bodies[index].vVertexList[6].y = ymin;
        Bodies[index].vVertexList[6].z = zmax;

        Bodies[index].vVertexList[7].x = xmin;
        Bodies[index].vVertexList[7].y = ymin;
        Bodies[index].vVertexList[7].z = zmin;
    }

    // Position a non-overlapping block.
    // Returns: true if block positioned, else false.
    bool
        positionBlock(int index)
    {
        int i,j;
        float d,f;

        // Try to initialize non-overlapping block.
        for (i = 0; i < BLOCK_INIT_TRIES; i++)
        {
            // Randomize block position.
            d = (float)WALL_SIZE - (Bodies[index].fRadius * 2.0);
            if (d < 0.0) continue;
            f = (float)(rand()%((int)(d * 100.0) + 1)) / 100.0;
            Bodies[index].vPosition.x = (f - (d / 2.0)) + Bodies[index].fRadius;
            f = (float)(rand()%((int)(d * 100.0) + 1)) / 100.0;
            Bodies[index].vPosition.y = (f - (d / 2.0)) + Bodies[index].fRadius;
            f = (float)(rand()%((int)(d * 100.0) + 1)) / 100.0;
            Bodies[index].vPosition.z = (f - (d / 2.0)) + Bodies[index].fRadius;

            // Stay away from other blocks.
            for (j = FIRST_FIXED_BLOCK; j < NumBodies; j++)
            {
                if (index == j || Bodies[j].valid == false) continue;
                if ((Bodies[index].fRadius + Bodies[j].fRadius) >
                    Bodies[index].vPosition.Distance(Bodies[j].vPosition)) break;
            }
            if (j == NumBodies) return(true);
        }
        return(false);
    }

    // Build wall grid display list.
    void
        buildWallDisplay(int index)
    {
        GLfloat s,d,r;

        s = WALL_SIZE / 2.0;
        r = (GLfloat)WALL_SIZE * WALL_GRID_RATIO;
        Bodies[index].display = glGenLists(1);
        glNewList(Bodies[index].display, GL_COMPILE);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();

        switch(index)
        {
            case 0:
                // Top.
                glBegin(GL_LINE_LOOP);
                glVertex3f(-s, s, -s);
                glVertex3f(-s, s, s);
                glVertex3f(s, s, s);
                glVertex3f(s, s, -s);
                glEnd();

                glBegin(GL_LINES);
                for (d = r; d < (GLfloat)WALL_SIZE; d += r)
                {
                    glVertex3f(-s, s, -s + d);
                    glVertex3f(s, s, -s + d);
                    glVertex3f(-s + d, s, -s);
                    glVertex3f(-s + d, s, s);
                }
                glEnd();
                break;
            case 1:
                // Right.
                glBegin(GL_LINE_LOOP);
                glVertex3f(s, -s, -s);
                glVertex3f(s, -s, s);
                glVertex3f(s, s, s);
                glVertex3f(s, s, -s);
                glEnd();

                glBegin(GL_LINES);
                for (d = r; d < (GLfloat)WALL_SIZE; d += r)
                {
                    glVertex3f(s, -s, -s + d);
                    glVertex3f(s, s, -s + d);
                    glVertex3f(s, -s + d, -s);
                    glVertex3f(s, -s + d, s);
                }
                glEnd();
                break;
            case 2:
                // Back;
                glBegin(GL_LINE_LOOP);
                glVertex3f(-s, s, s);
                glVertex3f(s, s, s);
                glVertex3f(s, -s, s);
                glVertex3f(-s, -s, s);
                glEnd();

                glBegin(GL_LINES);
                for (d = r; d < (GLfloat)WALL_SIZE; d += r)
                {
                    glVertex3f(-s, -s + d, s);
                    glVertex3f(s, -s + d, s);
                    glVertex3f(-s + d, -s, s);
                    glVertex3f(-s + d, s, s);
                }
                glEnd();
                break;
            case 3:
                // Bottom.
                glBegin(GL_LINE_LOOP);
                glVertex3f(-s, -s, -s);
                glVertex3f(-s, -s, s);
                glVertex3f(s, -s, s);
                glVertex3f(s, -s, -s);
                glEnd();

                glBegin(GL_LINES);
                for (d = r; d < (GLfloat)WALL_SIZE; d += r)
                {
                    glVertex3f(-s, -s, -s + d);
                    glVertex3f(s, -s, -s + d);
                    glVertex3f(-s + d, -s, -s);
                    glVertex3f(-s + d, -s, s);
                }
                glEnd();
                break;
            case 4:
                // Left.
                glBegin(GL_LINE_LOOP);
                glVertex3f(-s, -s, -s);
                glVertex3f(-s, -s, s);
                glVertex3f(-s, s, s);
                glVertex3f(-s, s, -s);
                glEnd();

                glBegin(GL_LINES);
                for (d = r; d < (GLfloat)WALL_SIZE; d += r)
                {
                    glVertex3f(-s, -s, -s + d);
                    glVertex3f(-s, s, -s + d);
                    glVertex3f(-s, -s + d, -s);
                    glVertex3f(-s, -s + d, s);
                }
                glEnd();
                break;
            case 5:
                // Front.
                glBegin(GL_LINE_LOOP);
                glVertex3f(-s, s, -s);
                glVertex3f(s, s, -s);
                glVertex3f(s, -s, -s);
                glVertex3f(-s, -s, -s);
                glEnd();

                glBegin(GL_LINES);
                for (d = r; d < (GLfloat)WALL_SIZE; d += r)
                {
                    glVertex3f(-s, -s + d, -s);
                    glVertex3f(s, -s + d, -s);
                    glVertex3f(-s + d, -s, -s);
                    glVertex3f(-s + d, s, -s);
                }
                glEnd();
                break;
        }
        glPopAttrib();
        glPopMatrix();
        glEndList();
    }

    // Build star field display.
    void buildStarDisplay()
    {
        int i;
        GLfloat r,r2,x,y,z;

        r = WALL_SIZE / 2.0;
        r = sqrt(3.0*r*r);
        StarDisplay = glGenLists(1);
        glNewList(StarDisplay, GL_COMPILE);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();

        // Generate stars on the surface of a bounding sphere.
        for (i = 0; i < NUM_STARS; i++)
        {
            x = PRAND_UNIT * r;
            r2 = sqrt((r*r) - (x*x));
            y = PRAND_UNIT * r2;
            z = sqrt((r2*r2) - (y*y));
            if (rand()%2) x = -x;
            if (rand()%2) y = -y;
            if (rand()%2) z = -z;
            glBegin(GL_POINTS);
            glVertex3f(x, y, z);
            glEnd();
        }

        glPopAttrib();
        glPopMatrix();
        glEndList();
    }

    #ifdef HELLBOX
    void
        buildFixedBlockDisplays()
    {
        int i,j,k,n;
        GLfloat x,y,z,tx,ty,d;

        if (NUM_FIXED_BLOCKS == 0) return;

        // Tile 1x1 quads on each side of block for better reflection.
        n = (int)(FIXED_BLOCK_SIZE / 1.0);
        d = 1.0 / (float)n;

        i = 0;
        FixedBlockDisplay[i] = glGenLists(1);
        glNewList(FixedBlockDisplay[i], GL_COMPILE);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, BlockMaterial.ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, BlockMaterial.diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, BlockMaterial.specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, BlockMaterial.emission);
        glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, BlockMaterial.phExp);

        glBegin(GL_QUADS);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[0].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[0].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[0].z;
        tx = ty = 1.0;
        for (j = 0; j < n; j++)
        {
            for (k = 0; k < n; k++)
            {
                glNormal3f(1.0, 0.0, 0.0);
                glTexCoord2f(tx, ty);
                glVertex3f(x, y, z);
                glNormal3f(1.0, 0.0, 0.0);
                glTexCoord2f(tx - d, ty);
                glVertex3f(x, y, z + 1.0);
                glNormal3f(1.0, 0.0, 0.0);
                glTexCoord2f(tx - d, ty - d);
                glVertex3f(x, y - 1.0, z + 1.0);
                glNormal3f(1.0, 0.0, 0.0);
                glTexCoord2f(tx, ty - d);
                glVertex3f(x, y - 1.0, z);
                z += 1.0;
                tx -= d;
            }
            z = Bodies[FIRST_FIXED_BLOCK].vVertexList[0].z;
            tx = 1.0;
            y -= 1.0;
            ty -= d;
        }
        glEnd();

        glPopAttrib();
        glPopMatrix();
        glEndList();

        i = 1;
        FixedBlockDisplay[i] = glGenLists(1);
        glNewList(FixedBlockDisplay[i], GL_COMPILE);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, BlockMaterial.ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, BlockMaterial.diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, BlockMaterial.specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, BlockMaterial.emission);
        glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, BlockMaterial.phExp);

        glBegin(GL_QUADS);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[2].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[2].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[2].z;
        tx = ty = 1.0;
        for (j = 0; j < n; j++)
        {
            for (k = 0; k < n; k++)
            {
                glNormal3f(0.0, -1.0, 0.0);
                glTexCoord2f(tx, ty);
                glVertex3f(x, y, z);
                glNormal3f(0.0, -1.0, 0.0);
                glTexCoord2f(tx - d, ty);
                glVertex3f(x - 1.0, y, z);
                glNormal3f(0.0, -1.0, 0.0);
                glTexCoord2f(tx - d, ty - d);
                glVertex3f(x - 1.0, y, z - 1.0);
                glNormal3f(0.0, -1.0, 0.0);
                glTexCoord2f(tx, ty - d);
                glVertex3f(x, y, z - 1.0);
                x -= 1.0;
                tx -= d;
            }
            x = Bodies[FIRST_FIXED_BLOCK].vVertexList[2].x;
            tx = 1.0;
            z -= 1.0;
            ty -= d;
        }
        glEnd();

        glPopAttrib();
        glPopMatrix();
        glEndList();

        i = 2;
        FixedBlockDisplay[i] = glGenLists(1);
        glNewList(FixedBlockDisplay[i], GL_COMPILE);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, BlockMaterial.ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, BlockMaterial.diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, BlockMaterial.specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, BlockMaterial.emission);
        glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, BlockMaterial.phExp);

        glBegin(GL_QUADS);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[5].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[5].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[5].z;
        tx = ty = 1.0;
        for (j = 0; j < n; j++)
        {
            for (k = 0; k < n; k++)
            {
                glNormal3f(-1.0, 0.0, 0.0);
                glTexCoord2f(tx, ty);
                glVertex3f(x, y, z);
                glNormal3f(-1.0, 0.0, 0.0);
                glTexCoord2f(tx - d, ty);
                glVertex3f(x, y, z - 1.0);
                glNormal3f(-1.0, 0.0, 0.0);
                glTexCoord2f(tx - d, ty - d);
                glVertex3f(x, y - 1.0, z - 1.0);
                glNormal3f(-1.0, 0.0, 0.0);
                glTexCoord2f(tx, ty - d);
                glVertex3f(x, y - 1.0, z);
                z -= 1.0;
                tx -= d;
            }
            z = Bodies[FIRST_FIXED_BLOCK].vVertexList[5].z;
            tx = 1.0;
            y -= 1.0;
            ty -= d;
        }
        glEnd();

        glPopAttrib();
        glPopMatrix();
        glEndList();

        i = 3;
        FixedBlockDisplay[i] = glGenLists(1);
        glNewList(FixedBlockDisplay[i], GL_COMPILE);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, BlockMaterial.ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, BlockMaterial.diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, BlockMaterial.specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, BlockMaterial.emission);
        glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, BlockMaterial.phExp);

        glBegin(GL_QUADS);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[0].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[0].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[0].z;
        tx = ty = 1.0;
        for (j = 0; j < n; j++)
        {
            for (k = 0; k < n; k++)
            {
                glNormal3f(0.0, 1.0, 0.0);
                glTexCoord2f(tx, ty);
                glVertex3f(x, y, z);
                glNormal3f(0.0, 1.0, 0.0);
                glTexCoord2f(tx - d, ty);
                glVertex3f(x - 1.0, y, z);
                glNormal3f(0.0, 1.0, 0.0);
                glTexCoord2f(tx - d, ty - d);
                glVertex3f(x - 1.0, y, z + 1.0);
                glNormal3f(0.0, 1.0, 0.0);
                glTexCoord2f(tx, ty - d);
                glVertex3f(x, y, z + 1.0);
                x -= 1.0;
                tx -= d;
            }
            x = Bodies[FIRST_FIXED_BLOCK].vVertexList[0].x;
            tx = 1.0;
            z += 1.0;
            ty -= d;
        }
        glEnd();

        glPopAttrib();
        glPopMatrix();
        glEndList();

        i = 4;
        FixedBlockDisplay[i] = glGenLists(1);
        glNewList(FixedBlockDisplay[i], GL_COMPILE);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, BlockMaterial.ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, BlockMaterial.diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, BlockMaterial.specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, BlockMaterial.emission);
        glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, BlockMaterial.phExp);

        glBegin(GL_QUADS);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[1].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[1].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[1].z;
        tx = ty = 1.0;
        for (j = 0; j < n; j++)
        {
            for (k = 0; k < n; k++)
            {
                glNormal3f(0.0, 0.0, 1.0);
                glTexCoord2f(tx, ty);
                glVertex3f(x, y, z);
                glNormal3f(0.0, 0.0, 1.0);
                glTexCoord2f(tx - d, ty);
                glVertex3f(x - 1.0, y, z);
                glNormal3f(0.0, 0.0, 1.0);
                glTexCoord2f(tx - d, ty - d);
                glVertex3f(x - 1.0, y - 1.0, z);
                glNormal3f(0.0, 0.0, 1.0);
                glTexCoord2f(tx, ty - d);
                glVertex3f(x, y - 1.0, z);
                x -= 1.0;
                tx -= d;
            }
            x = Bodies[FIRST_FIXED_BLOCK].vVertexList[1].x;
            tx = 1.0;
            y -= 1.0;
            ty -= d;
        }
        glEnd();

        glPopAttrib();
        glPopMatrix();
        glEndList();

        i = 5;
        FixedBlockDisplay[i] = glGenLists(1);
        glNewList(FixedBlockDisplay[i], GL_COMPILE);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, BlockMaterial.ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, BlockMaterial.diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, BlockMaterial.specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, BlockMaterial.emission);
        glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, BlockMaterial.phExp);

        glBegin(GL_QUADS);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[4].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[4].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[4].z;
        tx = ty = 1.0;
        for (j = 0; j < n; j++)
        {
            for (k = 0; k < n; k++)
            {
                glNormal3f(0.0, 0.0, -1.0);
                glTexCoord2f(tx, ty);
                glVertex3f(x, y, z);
                glNormal3f(0.0, 0.0, -1.0);
                glTexCoord2f(tx - d, ty);
                glVertex3f(x + 1.0, y, z);
                glNormal3f(0.0, 0.0, -1.0);
                glTexCoord2f(tx - d, ty - d);
                glVertex3f(x + 1.0, y - 1.0, z);
                glNormal3f(0.0, 0.0, -1.0);
                glTexCoord2f(tx, ty - d);
                glVertex3f(x, y - 1.0, z);
                x += 1.0;
                tx -= d;
            }
            x = Bodies[FIRST_FIXED_BLOCK].vVertexList[4].x;
            tx = 1.0;
            y -= 1.0;
            ty -= d;
        }
        glEnd();

        glPopAttrib();
        glPopMatrix();
        glEndList();

        // Build outline.
        i = 6;
        FixedBlockDisplay[i] = glGenLists(1);
        glNewList(FixedBlockDisplay[i], GL_COMPILE);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();

        glColor3f(1.0, 1.0, 1.0);
        glBegin(GL_LINE_LOOP);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[0].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[0].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[0].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[1].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[1].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[1].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[2].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[2].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[2].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[3].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[3].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[3].z;
        glVertex3f(x, y, z);
        glEnd();

        glBegin(GL_LINE_LOOP);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[3].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[3].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[3].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[2].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[2].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[2].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[6].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[6].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[6].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[7].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[7].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[7].z;
        glVertex3f(x, y, z);
        glEnd();

        glBegin(GL_LINE_LOOP);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[7].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[7].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[7].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[6].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[6].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[6].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[5].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[5].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[5].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[4].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[4].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[4].z;
        glVertex3f(x, y, z);
        glEnd();

        glBegin(GL_LINE_LOOP);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[0].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[0].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[0].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[4].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[4].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[4].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[5].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[5].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[5].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_FIXED_BLOCK].vVertexList[1].x;
        y = Bodies[FIRST_FIXED_BLOCK].vVertexList[1].y;
        z = Bodies[FIRST_FIXED_BLOCK].vVertexList[1].z;
        glVertex3f(x, y, z);
        glEnd();

        glPopAttrib();
        glPopMatrix();
        glEndList();
    }

    void
        buildBlockDisplays()
    {
        int i,j,k,n;
        GLfloat x,y,z,tx,ty,d;

        if (NUM_BLOCKS == 0) return;

        // Tile 1x1 quads on each side of block for better reflection.
        n = (int)(BLOCK_SIZE / 1.0);
        d = 1.0 / (float)n;

        i = 0;
        BlockDisplay[i] = glGenLists(1);
        glNewList(BlockDisplay[i], GL_COMPILE);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, BlockMaterial.ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, BlockMaterial.diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, BlockMaterial.specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, BlockMaterial.emission);
        glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, BlockMaterial.phExp);

        glBegin(GL_QUADS);
        x = Bodies[FIRST_BLOCK].vVertexList[0].x;
        y = Bodies[FIRST_BLOCK].vVertexList[0].y;
        z = Bodies[FIRST_BLOCK].vVertexList[0].z;
        tx = ty = 1.0;
        for (j = 0; j < n; j++)
        {
            for (k = 0; k < n; k++)
            {
                glNormal3f(1.0, 0.0, 0.0);
                glTexCoord2f(tx, ty);
                glVertex3f(x, y, z);
                glNormal3f(1.0, 0.0, 0.0);
                glTexCoord2f(tx - d, ty);
                glVertex3f(x, y, z + 1.0);
                glNormal3f(1.0, 0.0, 0.0);
                glTexCoord2f(tx - d, ty - d);
                glVertex3f(x, y - 1.0, z + 1.0);
                glNormal3f(1.0, 0.0, 0.0);
                glTexCoord2f(tx, ty - d);
                glVertex3f(x, y - 1.0, z);
                z += 1.0;
                tx -= d;
            }
            z = Bodies[FIRST_BLOCK].vVertexList[0].z;
            tx = 1.0;
            y -= 1.0;
            ty -= d;
        }
        glEnd();

        glPopAttrib();
        glPopMatrix();
        glEndList();

        i = 1;
        BlockDisplay[i] = glGenLists(1);
        glNewList(BlockDisplay[i], GL_COMPILE);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, BlockMaterial.ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, BlockMaterial.diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, BlockMaterial.specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, BlockMaterial.emission);
        glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, BlockMaterial.phExp);

        glBegin(GL_QUADS);
        x = Bodies[FIRST_BLOCK].vVertexList[2].x;
        y = Bodies[FIRST_BLOCK].vVertexList[2].y;
        z = Bodies[FIRST_BLOCK].vVertexList[2].z;
        tx = ty = 1.0;
        for (j = 0; j < n; j++)
        {
            for (k = 0; k < n; k++)
            {
                glNormal3f(0.0, -1.0, 0.0);
                glTexCoord2f(tx, ty);
                glVertex3f(x, y, z);
                glNormal3f(0.0, -1.0, 0.0);
                glTexCoord2f(tx - d, ty);
                glVertex3f(x - 1.0, y, z);
                glNormal3f(0.0, -1.0, 0.0);
                glTexCoord2f(tx - d, ty - d);
                glVertex3f(x - 1.0, y, z - 1.0);
                glNormal3f(0.0, -1.0, 0.0);
                glTexCoord2f(tx, ty - d);
                glVertex3f(x, y, z - 1.0);
                x -= 1.0;
                tx -= d;
            }
            x = Bodies[FIRST_BLOCK].vVertexList[2].x;
            tx = 1.0;
            z -= 1.0;
            ty -= d;
        }
        glEnd();

        glPopAttrib();
        glPopMatrix();
        glEndList();

        i = 2;
        BlockDisplay[i] = glGenLists(1);
        glNewList(BlockDisplay[i], GL_COMPILE);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, BlockMaterial.ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, BlockMaterial.diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, BlockMaterial.specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, BlockMaterial.emission);
        glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, BlockMaterial.phExp);

        glBegin(GL_QUADS);
        x = Bodies[FIRST_BLOCK].vVertexList[5].x;
        y = Bodies[FIRST_BLOCK].vVertexList[5].y;
        z = Bodies[FIRST_BLOCK].vVertexList[5].z;
        tx = ty = 1.0;
        for (j = 0; j < n; j++)
        {
            for (k = 0; k < n; k++)
            {
                glNormal3f(-1.0, 0.0, 0.0);
                glTexCoord2f(tx, ty);
                glVertex3f(x, y, z);
                glNormal3f(-1.0, 0.0, 0.0);
                glTexCoord2f(tx - d, ty);
                glVertex3f(x, y, z - 1.0);
                glNormal3f(-1.0, 0.0, 0.0);
                glTexCoord2f(tx - d, ty - d);
                glVertex3f(x, y - 1.0, z - 1.0);
                glNormal3f(-1.0, 0.0, 0.0);
                glTexCoord2f(tx, ty - d);
                glVertex3f(x, y - 1.0, z);
                z -= 1.0;
                tx -= d;
            }
            z = Bodies[FIRST_BLOCK].vVertexList[5].z;
            tx = 1.0;
            y -= 1.0;
            ty -= d;
        }
        glEnd();

        glPopAttrib();
        glPopMatrix();
        glEndList();

        i = 3;
        BlockDisplay[i] = glGenLists(1);
        glNewList(BlockDisplay[i], GL_COMPILE);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, BlockMaterial.ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, BlockMaterial.diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, BlockMaterial.specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, BlockMaterial.emission);
        glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, BlockMaterial.phExp);

        glBegin(GL_QUADS);
        x = Bodies[FIRST_BLOCK].vVertexList[0].x;
        y = Bodies[FIRST_BLOCK].vVertexList[0].y;
        z = Bodies[FIRST_BLOCK].vVertexList[0].z;
        tx = ty = 1.0;
        for (j = 0; j < n; j++)
        {
            for (k = 0; k < n; k++)
            {
                glNormal3f(0.0, 1.0, 0.0);
                glTexCoord2f(tx, ty);
                glVertex3f(x, y, z);
                glNormal3f(0.0, 1.0, 0.0);
                glTexCoord2f(tx - d, ty);
                glVertex3f(x - 1.0, y, z);
                glNormal3f(0.0, 1.0, 0.0);
                glTexCoord2f(tx - d, ty - d);
                glVertex3f(x - 1.0, y, z + 1.0);
                glNormal3f(0.0, 1.0, 0.0);
                glTexCoord2f(tx, ty - d);
                glVertex3f(x, y, z + 1.0);
                x -= 1.0;
                tx -= d;
            }
            x = Bodies[FIRST_BLOCK].vVertexList[0].x;
            tx = 1.0;
            z += 1.0;
            ty -= d;
        }
        glEnd();

        glPopAttrib();
        glPopMatrix();
        glEndList();

        i = 4;
        BlockDisplay[i] = glGenLists(1);
        glNewList(BlockDisplay[i], GL_COMPILE);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, BlockMaterial.ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, BlockMaterial.diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, BlockMaterial.specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, BlockMaterial.emission);
        glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, BlockMaterial.phExp);

        glBegin(GL_QUADS);
        x = Bodies[FIRST_BLOCK].vVertexList[1].x;
        y = Bodies[FIRST_BLOCK].vVertexList[1].y;
        z = Bodies[FIRST_BLOCK].vVertexList[1].z;
        tx = ty = 1.0;
        for (j = 0; j < n; j++)
        {
            for (k = 0; k < n; k++)
            {
                glNormal3f(0.0, 0.0, 1.0);
                glTexCoord2f(tx, ty);
                glVertex3f(x, y, z);
                glNormal3f(0.0, 0.0, 1.0);
                glTexCoord2f(tx - d, ty);
                glVertex3f(x - 1.0, y, z);
                glNormal3f(0.0, 0.0, 1.0);
                glTexCoord2f(tx - d, ty - d);
                glVertex3f(x - 1.0, y - 1.0, z);
                glNormal3f(0.0, 0.0, 1.0);
                glTexCoord2f(tx, ty - d);
                glVertex3f(x, y - 1.0, z);
                x -= 1.0;
                tx -= d;
            }
            x = Bodies[FIRST_BLOCK].vVertexList[1].x;
            tx = 1.0;
            y -= 1.0;
            ty -= d;
        }
        glEnd();

        glPopAttrib();
        glPopMatrix();
        glEndList();

        i = 5;
        BlockDisplay[i] = glGenLists(1);
        glNewList(BlockDisplay[i], GL_COMPILE);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, BlockMaterial.ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, BlockMaterial.diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, BlockMaterial.specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, BlockMaterial.emission);
        glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, BlockMaterial.phExp);

        glBegin(GL_QUADS);
        x = Bodies[FIRST_BLOCK].vVertexList[4].x;
        y = Bodies[FIRST_BLOCK].vVertexList[4].y;
        z = Bodies[FIRST_BLOCK].vVertexList[4].z;
        tx = ty = 1.0;
        for (j = 0; j < n; j++)
        {
            for (k = 0; k < n; k++)
            {
                glNormal3f(0.0, 0.0, -1.0);
                glTexCoord2f(tx, ty);
                glVertex3f(x, y, z);
                glNormal3f(0.0, 0.0, -1.0);
                glTexCoord2f(tx - d, ty);
                glVertex3f(x + 1.0, y, z);
                glNormal3f(0.0, 0.0, -1.0);
                glTexCoord2f(tx - d, ty - d);
                glVertex3f(x + 1.0, y - 1.0, z);
                glNormal3f(0.0, 0.0, -1.0);
                glTexCoord2f(tx, ty - d);
                glVertex3f(x, y - 1.0, z);
                x += 1.0;
                tx -= d;
            }
            x = Bodies[FIRST_BLOCK].vVertexList[4].x;
            tx = 1.0;
            y -= 1.0;
            ty -= d;
        }
        glEnd();

        glPopAttrib();
        glPopMatrix();
        glEndList();

        // Build outline.
        i = 6;
        BlockDisplay[i] = glGenLists(1);
        glNewList(BlockDisplay[i], GL_COMPILE);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();

        glColor3f(1.0, 1.0, 1.0);
        glBegin(GL_LINE_LOOP);
        x = Bodies[FIRST_BLOCK].vVertexList[0].x;
        y = Bodies[FIRST_BLOCK].vVertexList[0].y;
        z = Bodies[FIRST_BLOCK].vVertexList[0].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_BLOCK].vVertexList[1].x;
        y = Bodies[FIRST_BLOCK].vVertexList[1].y;
        z = Bodies[FIRST_BLOCK].vVertexList[1].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_BLOCK].vVertexList[2].x;
        y = Bodies[FIRST_BLOCK].vVertexList[2].y;
        z = Bodies[FIRST_BLOCK].vVertexList[2].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_BLOCK].vVertexList[3].x;
        y = Bodies[FIRST_BLOCK].vVertexList[3].y;
        z = Bodies[FIRST_BLOCK].vVertexList[3].z;
        glVertex3f(x, y, z);
        glEnd();

        glBegin(GL_LINE_LOOP);
        x = Bodies[FIRST_BLOCK].vVertexList[3].x;
        y = Bodies[FIRST_BLOCK].vVertexList[3].y;
        z = Bodies[FIRST_BLOCK].vVertexList[3].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_BLOCK].vVertexList[2].x;
        y = Bodies[FIRST_BLOCK].vVertexList[2].y;
        z = Bodies[FIRST_BLOCK].vVertexList[2].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_BLOCK].vVertexList[6].x;
        y = Bodies[FIRST_BLOCK].vVertexList[6].y;
        z = Bodies[FIRST_BLOCK].vVertexList[6].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_BLOCK].vVertexList[7].x;
        y = Bodies[FIRST_BLOCK].vVertexList[7].y;
        z = Bodies[FIRST_BLOCK].vVertexList[7].z;
        glVertex3f(x, y, z);
        glEnd();

        glBegin(GL_LINE_LOOP);
        x = Bodies[FIRST_BLOCK].vVertexList[7].x;
        y = Bodies[FIRST_BLOCK].vVertexList[7].y;
        z = Bodies[FIRST_BLOCK].vVertexList[7].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_BLOCK].vVertexList[6].x;
        y = Bodies[FIRST_BLOCK].vVertexList[6].y;
        z = Bodies[FIRST_BLOCK].vVertexList[6].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_BLOCK].vVertexList[5].x;
        y = Bodies[FIRST_BLOCK].vVertexList[5].y;
        z = Bodies[FIRST_BLOCK].vVertexList[5].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_BLOCK].vVertexList[4].x;
        y = Bodies[FIRST_BLOCK].vVertexList[4].y;
        z = Bodies[FIRST_BLOCK].vVertexList[4].z;
        glVertex3f(x, y, z);
        glEnd();

        glBegin(GL_LINE_LOOP);
        x = Bodies[FIRST_BLOCK].vVertexList[0].x;
        y = Bodies[FIRST_BLOCK].vVertexList[0].y;
        z = Bodies[FIRST_BLOCK].vVertexList[0].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_BLOCK].vVertexList[4].x;
        y = Bodies[FIRST_BLOCK].vVertexList[4].y;
        z = Bodies[FIRST_BLOCK].vVertexList[4].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_BLOCK].vVertexList[5].x;
        y = Bodies[FIRST_BLOCK].vVertexList[5].y;
        z = Bodies[FIRST_BLOCK].vVertexList[5].z;
        glVertex3f(x, y, z);
        x = Bodies[FIRST_BLOCK].vVertexList[1].x;
        y = Bodies[FIRST_BLOCK].vVertexList[1].y;
        z = Bodies[FIRST_BLOCK].vVertexList[1].z;
        glVertex3f(x, y, z);
        glEnd();

        glPopAttrib();
        glPopMatrix();
        glEndList();
    }
    #endif

    // Build block display.
    void
        buildBlockDisplay(int index)
    {
        int i;
        GLfloat x,y,z;

        Bodies[index].display = glGenLists(1);
        glNewList(Bodies[index].display, GL_COMPILE);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, BlockMaterial.ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, BlockMaterial.diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, BlockMaterial.specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, BlockMaterial.emission);
        glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, BlockMaterial.phExp);

        i = index;
        glBegin(GL_QUADS);

        x = Bodies[i].vVertexList[0].x;
        y = Bodies[i].vVertexList[0].y;
        z = Bodies[i].vVertexList[0].z;
        glNormal3f(1.0, 0.0, 0.0);
        glTexCoord2f(0.0, 0.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[1].x;
        y = Bodies[i].vVertexList[1].y;
        z = Bodies[i].vVertexList[1].z;
        glNormal3f(1.0, 0.0, 0.0);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[2].x;
        y = Bodies[i].vVertexList[2].y;
        z = Bodies[i].vVertexList[2].z;
        glNormal3f(1.0, 0.0, 0.0);
        glTexCoord2f(1.0, 1.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[3].x;
        y = Bodies[i].vVertexList[3].y;
        z = Bodies[i].vVertexList[3].z;
        glNormal3f(1.0, 0.0, 0.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(x, y, z);

        x = Bodies[i].vVertexList[3].x;
        y = Bodies[i].vVertexList[3].y;
        z = Bodies[i].vVertexList[3].z;
        glNormal3f(0.0, -1.0, 0.0);
        glTexCoord2f(0.0, 0.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[2].x;
        y = Bodies[i].vVertexList[2].y;
        z = Bodies[i].vVertexList[2].z;
        glNormal3f(0.0, -1.0, 0.0);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[6].x;
        y = Bodies[i].vVertexList[6].y;
        z = Bodies[i].vVertexList[6].z;
        glNormal3f(0.0, -1.0, 0.0);
        glTexCoord2f(1.0, 1.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[7].x;
        y = Bodies[i].vVertexList[7].y;
        z = Bodies[i].vVertexList[7].z;
        glNormal3f(0.0, -1.0, 0.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(x, y, z);

        x = Bodies[i].vVertexList[7].x;
        y = Bodies[i].vVertexList[7].y;
        z = Bodies[i].vVertexList[7].z;
        glNormal3f(-1.0, 0.0, 0.0);
        glTexCoord2f(0.0, 0.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[6].x;
        y = Bodies[i].vVertexList[6].y;
        z = Bodies[i].vVertexList[6].z;
        glNormal3f(-1.0, 0.0, 0.0);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[5].x;
        y = Bodies[i].vVertexList[5].y;
        z = Bodies[i].vVertexList[5].z;
        glNormal3f(-1.0, 0.0, 0.0);
        glTexCoord2f(1.0, 1.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[4].x;
        y = Bodies[i].vVertexList[4].y;
        z = Bodies[i].vVertexList[4].z;
        glNormal3f(-1.0, 0.0, 0.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(x, y, z);

        x = Bodies[i].vVertexList[0].x;
        y = Bodies[i].vVertexList[0].y;
        z = Bodies[i].vVertexList[0].z;
        glNormal3f(0.0, 1.0, 0.0);
        glTexCoord2f(0.0, 0.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[4].x;
        y = Bodies[i].vVertexList[4].y;
        z = Bodies[i].vVertexList[4].z;
        glNormal3f(0.0, 1.0, 0.0);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[5].x;
        y = Bodies[i].vVertexList[5].y;
        z = Bodies[i].vVertexList[5].z;
        glNormal3f(0.0, 1.0, 0.0);
        glTexCoord2f(1.0, 1.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[1].x;
        y = Bodies[i].vVertexList[1].y;
        z = Bodies[i].vVertexList[1].z;
        glNormal3f(0.0, 1.0, 0.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(x, y, z);

        x = Bodies[i].vVertexList[2].x;
        y = Bodies[i].vVertexList[2].y;
        z = Bodies[i].vVertexList[2].z;
        glNormal3f(0.0, 0.0, 1.0);
        glTexCoord2f(0.0, 0.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[1].x;
        y = Bodies[i].vVertexList[1].y;
        z = Bodies[i].vVertexList[1].z;
        glNormal3f(0.0, 0.0, 1.0);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[5].x;
        y = Bodies[i].vVertexList[5].y;
        z = Bodies[i].vVertexList[5].z;
        glNormal3f(0.0, 0.0, 1.0);
        glTexCoord2f(1.0, 1.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[6].x;
        y = Bodies[i].vVertexList[6].y;
        z = Bodies[i].vVertexList[6].z;
        glNormal3f(0.0, 0.0, 1.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(x, y, z);

        x = Bodies[i].vVertexList[7].x;
        y = Bodies[i].vVertexList[7].y;
        z = Bodies[i].vVertexList[7].z;
        glNormal3f(0.0, 0.0, -1.0);
        glTexCoord2f(0.0, 0.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[4].x;
        y = Bodies[i].vVertexList[4].y;
        z = Bodies[i].vVertexList[4].z;
        glNormal3f(0.0, 0.0, -1.0);
        glTexCoord2f(0.0, 1.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[0].x;
        y = Bodies[i].vVertexList[0].y;
        z = Bodies[i].vVertexList[0].z;
        glNormal3f(0.0, 0.0, -1.0);
        glTexCoord2f(1.0, 1.0);
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[3].x;
        y = Bodies[i].vVertexList[3].y;
        z = Bodies[i].vVertexList[3].z;
        glNormal3f(0.0, 0.0, -1.0);
        glTexCoord2f(1.0, 0.0);
        glVertex3f(x, y, z);

        glEnd();

        glColor3f(1.0, 1.0, 1.0);
        glBegin(GL_LINE_LOOP);
        x = Bodies[i].vVertexList[0].x;
        y = Bodies[i].vVertexList[0].y;
        z = Bodies[i].vVertexList[0].z;
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[1].x;
        y = Bodies[i].vVertexList[1].y;
        z = Bodies[i].vVertexList[1].z;
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[2].x;
        y = Bodies[i].vVertexList[2].y;
        z = Bodies[i].vVertexList[2].z;
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[3].x;
        y = Bodies[i].vVertexList[3].y;
        z = Bodies[i].vVertexList[3].z;
        glVertex3f(x, y, z);
        glEnd();

        glBegin(GL_LINE_LOOP);
        x = Bodies[i].vVertexList[3].x;
        y = Bodies[i].vVertexList[3].y;
        z = Bodies[i].vVertexList[3].z;
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[2].x;
        y = Bodies[i].vVertexList[2].y;
        z = Bodies[i].vVertexList[2].z;
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[6].x;
        y = Bodies[i].vVertexList[6].y;
        z = Bodies[i].vVertexList[6].z;
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[7].x;
        y = Bodies[i].vVertexList[7].y;
        z = Bodies[i].vVertexList[7].z;
        glVertex3f(x, y, z);
        glEnd();

        glBegin(GL_LINE_LOOP);
        x = Bodies[i].vVertexList[7].x;
        y = Bodies[i].vVertexList[7].y;
        z = Bodies[i].vVertexList[7].z;
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[6].x;
        y = Bodies[i].vVertexList[6].y;
        z = Bodies[i].vVertexList[6].z;
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[5].x;
        y = Bodies[i].vVertexList[5].y;
        z = Bodies[i].vVertexList[5].z;
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[4].x;
        y = Bodies[i].vVertexList[4].y;
        z = Bodies[i].vVertexList[4].z;
        glVertex3f(x, y, z);
        glEnd();

        glBegin(GL_LINE_LOOP);
        x = Bodies[i].vVertexList[0].x;
        y = Bodies[i].vVertexList[0].y;
        z = Bodies[i].vVertexList[0].z;
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[4].x;
        y = Bodies[i].vVertexList[4].y;
        z = Bodies[i].vVertexList[4].z;
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[5].x;
        y = Bodies[i].vVertexList[5].y;
        z = Bodies[i].vVertexList[5].z;
        glVertex3f(x, y, z);
        x = Bodies[i].vVertexList[1].x;
        y = Bodies[i].vVertexList[1].y;
        z = Bodies[i].vVertexList[1].z;
        glVertex3f(x, y, z);
        glEnd();

        glPopAttrib();
        glPopMatrix();
        glEndList();
    }

    // Display mode information.
    void modeInfo()
    {
        glColor3f(1.0, 1.0, 1.0);
        glDisable(GL_BLEND);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glLineWidth(2.0);

        setInfoProjection();

        switch(UserMode)
        {
            case INTRO: introInfo(); break;
            case OPTIONS: optionInfo(); break;
            case RUN: runInfo(); break;
            case HELP: helpInfo(); break;
            case WIN: finiInfo(); break;
            case LOSE: finiInfo(); break;
            case MESSAGE: messageInfo(); break;
            case FATAL: messageInfo(); break;
            case WHO: whoInfo(); break;
        }

        resetInfoProjection();
    }

    // Introductory message.
    void introInfo()
    {
        // Clear the screen.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderBitmapString((WINDOW_WIDTH/2) - 25, WINDOW_HEIGHT/2, BIG_FONT, NAME);
        renderBitmapString((WINDOW_WIDTH/2) - 70, (WINDOW_HEIGHT/2) + 20, FONT, "Shoot the squids before they eat you");
        renderBitmapString((WINDOW_WIDTH/2) - 60, (WINDOW_HEIGHT/2) + 30, FONT, "h for help or space key to start");
    }

    // Option entry information.
    void optionInfo()
    {
        char buf[100];

        switch(OptionMode)
        {
            case GET_ID:
                sprintf(buf,"Enter ship id: %s", DataString);
                break;
            case GET_COLOR:
                sprintf(buf, "Enter ship color scheme random number: %s", DataString);
                break;
            case GET_SLAVE:
                sprintf(buf,"Connect to remote game instance? (y|n): %s", DataString);
                break;
            case GET_IP:
                sprintf(buf,"Enter remote IP address: %s", DataString);
                break;
        }
        renderBitmapString(5, 10, FONT, buf);
    }

    // Run information.
    void runInfo()
    {
        int i,j;
        char buf[50];
        #ifdef NETWORK
        Xwing *xwing;
        #endif

        renderBitmapString(5, 10, FONT, "h for help");

        if (muteMode)
        {
            renderBitmapString((WINDOW_WIDTH/2) - 10, 10, FONT, "Mute");
        }

        #ifdef NETWORK
        for (i = j = 0; i < NUM_XWINGS; i++)
        {
            xwing = Xwings[i].xwing;
            if (xwing->IsAlive()) j++;
        }
        sprintf(buf, "Ships: %d", j);
        renderBitmapString(WINDOW_WIDTH - 50, 10, FONT, buf);
        #else
        if (ShowFrameRate)
        {
            sprintf(buf, "FPS = %d", static_cast<int>(frameRate.FPS));
            renderBitmapString(WINDOW_WIDTH - 50, 10, FONT, buf);
        }
        #endif

        sprintf(buf,"Shots: %d ", MAX_SHOTS - Xwings[myXwing].shotCount);
        renderBitmapString(5, WINDOW_HEIGHT - 20, FONT, buf);

        if (RearView)
        {
            renderBitmapString((WINDOW_WIDTH/2) - 20, WINDOW_HEIGHT - 20, FONT, "Rear View");
        }

        for (i = j = 0; i < NUM_SQUIDS; i++) if (Squids[i].squid->IsAlive()) j++;
        sprintf(buf,"Squids: %d ", j);
        renderBitmapString(WINDOW_WIDTH - 50, WINDOW_HEIGHT - 20, FONT, buf);
    }

    // Help for controls.
    void helpInfo()
    {
        int i,v;

        // Clear the screen.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        v = 10;
        renderBitmapString(5, v, FONT, "Controls:"); v += (2 * LINE_SPACE);
        for (i = 0; ControlInfo[i] != NULL; i++)
        {
            renderBitmapString(5, v, FONT, ControlInfo[i]);
            v += LINE_SPACE;
        }
    }

    // End game message.
    void finiInfo()
    {
        char buf[20];

        // Clear the screen.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (UserMode == WIN)
        {
            strcpy(buf, "You win");
        }
        else
        {
            strcpy(buf, "You lose");
        }
        renderBitmapString((WINDOW_WIDTH/2) - 25, WINDOW_HEIGHT/2, BIG_FONT, buf);
    }

    // Message.
    void messageInfo()
    {
        char buf[100];

        // Clear the screen.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (UserMode == FATAL)
        {
            sprintf(buf, "error: %s", UserMessage);
            renderBitmapString(5, 10, FONT, buf);
        }
        else
        {
            renderBitmapString(5, 10, FONT, UserMessage);
        }
    }

    // Who is playing.
    void whoInfo()
    {
        int i,v;
        char buf[50];

        // Clear the screen.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        v = 10;
        renderBitmapString(5, v, FONT, "Players:"); v += (2 * LINE_SPACE);
        for (i = 0; i < NUM_XWINGS; i++)
        {
            if (Xwings[i].xwing->IsAlive())
            {
                sprintf(buf,"%d %s", i, Xwings[i].id);
                renderBitmapString(5, v, FONT, buf);
                v += LINE_SPACE;
            }
        }
    }

    void setInfoProjection()
    {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);

        // Invert the y axis, down is positive.
        glScalef(1, -1, 1);

        // Move the origin from the bottom left corner to the upper left corner.
        glTranslatef(0, -WINDOW_HEIGHT, 0);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
    }

    void resetInfoProjection()
    {
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }

    // Print string on screen at specified location.
    void renderBitmapString(GLfloat x, GLfloat y, void *font, char *string)
    {
        char *c;
        glRasterPos2f(x, y);
        for (c=string; *c != '\0'; c++)
        {
            glutBitmapCharacter(font, *c);
        }
    }

    // Get sound effects.
    void
        getSounds()
    {
        FSOUND_Init(44100, 32, 0);

        plasmaBoltSound = FSOUND_Sample_Load(FSOUND_UNMANAGED, "plasmaBolt.wav", FSOUND_NORMAL, 0);
        if (!plasmaBoltSound)
        {
            sprintf(UserMessage, "cannot load sound plasmaBolt.wav");
            UserMode = FATAL;
        }
        explosionSound = FSOUND_Sample_Load(FSOUND_UNMANAGED, "explosion.wav", FSOUND_NORMAL, 0);
        if (!explosionSound)
        {
            sprintf(UserMessage, "cannot load sound explosion.wav");
            UserMode = FATAL;
        }
        bumpSound = FSOUND_Sample_Load(FSOUND_UNMANAGED, "bump.wav", FSOUND_NORMAL, 0);
        if (!bumpSound)
        {
            sprintf(UserMessage, "cannot load sound bump.wav");
            UserMode = FATAL;
        }
        shipEngineSound = FSOUND_Sample_Load(FSOUND_UNMANAGED, "shipEngine.wav", FSOUND_NORMAL, 0);
        if (!shipEngineSound)
        {
            sprintf(UserMessage, "cannot load sound shipEngine.wav");
            UserMode = FATAL;
        }
        else
        {
            if (!FSOUND_Sample_SetMode(shipEngineSound,  FSOUND_LOOP_NORMAL))
            {
                sprintf(UserMessage, "cannot loop sound shipEngine.wav");
                UserMode = FATAL;
            }
            else
            {
                // Start engine paused and on lowest volume.
                shipEngineChannel = FSOUND_PlaySoundEx(FSOUND_FREE, shipEngineSound, NULL, 'y');
                shipEnginePaused = true;
                if (shipEngineChannel != -1)
                {
                    FSOUND_SetVolume(shipEngineChannel, 0);
                }
            }
        }
        youWinSound = FSOUND_Sample_Load(FSOUND_UNMANAGED, "Applause.wav", FSOUND_NORMAL, 0);
        if (!youWinSound)
        {
            sprintf(UserMessage, "cannot load sound Applause.wav");
            UserMode = FATAL;
        }
        youLoseSound = FSOUND_Sample_Load(FSOUND_UNMANAGED, "WWONKA07.WAV", FSOUND_NORMAL, 0);
        if (!youLoseSound)
        {
            sprintf(UserMessage, "cannot load sound WWONKA07.WAV");
            UserMode = FATAL;
        }
    }

    // Free sounds.
    void
        freeSounds()
    {
        if (plasmaBoltSound)
        {
            FSOUND_Sample_Free(plasmaBoltSound);
            plasmaBoltSound = NULL;
        }
        if (explosionSound)
        {
            FSOUND_Sample_Free(explosionSound);
            explosionSound = NULL;
        }
        if (bumpSound)
        {
            FSOUND_Sample_Free(bumpSound);
            bumpSound = NULL;
        }
        if (shipEngineSound)
        {
            FSOUND_Sample_Free(shipEngineSound);
            shipEngineSound = NULL;
        }
        if (youWinSound)
        {
            FSOUND_Sample_Free(youWinSound);
            youWinSound = NULL;
        }
        if (youLoseSound)
        {
            FSOUND_Sample_Free(youLoseSound);
            youLoseSound = NULL;
        }
        FSOUND_Close();
    }
