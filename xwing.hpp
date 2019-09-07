//***************************************************************************//
//* File Name: xwing.hpp                                                    *//
//* Author:    Tom Portegys, portegys@ilstu.edu                             *//
//* Date Made: 07/25/02                                                     *//
//* File Desc: Class declaration and implementation details                 *//
//*            representing an X-Wing spacecraft.                           *//
//* Rev. Date:                                                              *//
//* Rev. Desc:                                                              *//
//*                                                                         *//
//***************************************************************************//

#ifndef __XWING_HPP__
#define __XWING_HPP__

#include "game_object.hpp"
#include "plasmaBolt.hpp"

// Random number > -1.0 && < 1.0
#define RAND_UNIT ((GLfloat)(rand() - rand()) / RAND_MAX)

// Random number >= 0.0 && < 1.0
#define PRAND_UNIT ((GLfloat)rand() / RAND_MAX)

#define NUM_DRAWABLES 4                           // Number of drawable components.
#define NUM_EXPLODING_DRAWABLES 9                 // Number of exploding drawable components.
#define NUM_EXHAUSTS 100                          // Number of thruster exhaust streams.
#define ID_LENGTH 12                              // ID length.

// Exploding part.
struct XWING_EXPLODING_PART
{
    Vector position;
    Vector velocity;
    float  angle;
    Vector axis;
    float angularVelocity;
};

class Xwing : public cGameObject
{

    public:

        // States.
        static const int ALIVE;
        static const int EXPLODE;
        static const int DEAD;
        int state;

        // Constructors.
        Xwing()
        {
            state = ALIVE;
            m_type = XWING;
            m_disposition = FRIENDLY;
            ColorSeed = -1;
            memset(ID, 0, ID_LENGTH+1);
            ShowAxes = false;
            createModelDrawables();
        }
        Xwing(char *id)
        {
            state = ALIVE;
            m_type = XWING;
            ColorSeed = -1;
            memset(ID, 0, ID_LENGTH+1);
            setID(id);
            ShowAxes = false;
            createModelDrawables();
        }
        Xwing(int colorSeed)
        {
            state = ALIVE;
            m_type = XWING;
            ColorSeed = colorSeed;
            memset(ID, 0, ID_LENGTH+1);
            ShowAxes = false;
            createModelDrawables();
        }
        Xwing(char *id, int colorSeed)
        {
            state = ALIVE;
            m_type = XWING;
            ColorSeed = colorSeed;
            memset(ID, 0, ID_LENGTH+1);
            setID(id);
            ShowAxes = false;
            createModelDrawables();
        }

        // Go: update and draw.
        void Go() { Update(); Draw(); }

        // Update.
        void Update()
        {
            if (state == ALIVE) m_spacial->update();
            if (state == EXPLODE) ExplodeUpdate();
        }

        // Draw.
        void Draw();

        // Show axes?
        void showAxes(bool n) { ShowAxes = n; }

        // Fire plasma bolt.
        PlasmaBolt *fire()
        {
            PlasmaBolt *plasmaBolt;

            if (state != ALIVE) return(NULL);

            // Create plasma bolt and move it away from X-wing.
            plasmaBolt = new PlasmaBolt(m_spacial->x, m_spacial->y, m_spacial->z,
                0.5, 1.0, m_spacial->qcalc->quat);
            plasmaBolt->move();
            plasmaBolt->setSpeed(m_spacial->speed);
            plasmaBolt->setSpeedFactor(m_spacial->speedFactor);
            return(plasmaBolt);
        }

        // Paint ID on model.
        void setID(char *id, GLfloat r, GLfloat g, GLfloat b)
        {
            if (id != NULL) strncpy(ID, id, ID_LENGTH);
            IDred = r;
            IDgreen = g;
            IDblue = b;
        }
        void setID(char *id)
        {
            setID(id, 0.8, 0.0, 0.8);
        }

        // Kill and resurrect.
        void Kill() { state = DEAD; m_isAlive = false; }
        void Resurrect() { state = ALIVE; m_isAlive = true; }

        // Get nearest vertex to given point in world coordinates.
        void GetNearestVertex(GLfloat *point, GLfloat *vertex);

        // Expose vertices.
        void ExposeVertices(GLfloat **vertices, int *size);

        // Exploding state.
        void Explode();
        void ExplodeUpdate();

        // Is squid exploding?
        bool IsExploding()
        {
            if (state == EXPLODE) return(true); else return(false);
        }

    private:

        // Create model display lists and color xwing_textures.
        static GLint display[NUM_DRAWABLES];
        static GLint explodeDisplay[NUM_EXPLODING_DRAWABLES];
        static GLint buttDisplay;
        static bool displayInit;
        GLuint textureName[NUM_DRAWABLES];
        GLuint explodeTextureName[NUM_EXPLODING_DRAWABLES];
        void createModelDrawables();
        void createDisplay(int, int, int);

        // Draw thruster exhaust.
        void drawExhaust();

        // ID.
        char ID[ID_LENGTH+1];
        GLfloat IDred, IDgreen, IDblue;
        void drawID();

        // Draw axes.
        bool ShowAxes;
        void drawAxes();

        // Random color seed.
        int ColorSeed;

        // Explosion controls.
        float explosionCounter;
        static const float explosionDelay;
        static const float maxExplosionVelocity;
        static const float maxExplosionAngularVelocity;
        struct XWING_EXPLODING_PART explodingParts[NUM_EXPLODING_DRAWABLES];
        void explosionTransform(int);
};

// States.
const int Xwing::ALIVE = 0;
const int Xwing::EXPLODE = 1;
const int Xwing::DEAD = 2;

// Explosion parameters.
const float Xwing::explosionDelay = 80.0;
const float Xwing::maxExplosionVelocity = 0.05;
const float Xwing::maxExplosionAngularVelocity = 3.0;

// Display lists.
GLint Xwing::display[NUM_DRAWABLES];
GLint Xwing::explodeDisplay[NUM_EXPLODING_DRAWABLES];
GLint Xwing::buttDisplay;
bool Xwing::displayInit = false;

// Optimized model.
#include "xmodelopt.h"

// Create model display lists and color texture maps.
void Xwing::createModelDrawables()
{
    int i,j;
    GLubyte texture[2][2][3];

    // Acquire static display lists.
    if (!displayInit)
    {
        i = glGenLists(NUM_DRAWABLES);
        for (j = 0; j < NUM_DRAWABLES; j++, i++) display[j] = i;
        i = glGenLists(NUM_EXPLODING_DRAWABLES);
        for (j = 0; j < NUM_EXPLODING_DRAWABLES; j++, i++) explodeDisplay[j] = i;
    }

    // Using random colors?
    if (ColorSeed != -1)
    {
        srand(ColorSeed);
        IDred = rand() % 256;
        IDgreen = rand() % 256;
        IDblue = rand() % 256;
    }

    // Cockpit and engines.
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 2; j++)
        {
            if (ColorSeed == -1)
            {
                texture[i][j][0] = 255;           // Red
                texture[i][j][1] = 255;           // Green
                texture[i][j][2] = 255;           // Blue
            }
            else
            {
                texture[i][j][0] = rand() % 256;  // Red
                texture[i][j][1] = rand() % 256;  // Green
                texture[i][j][2] = rand() % 256;  // Blue
            }
        }
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &(textureName[0]));
    glBindTexture(GL_TEXTURE_2D, textureName[0]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &(texture[0][0][0]));
    explodeTextureName[0] = textureName[0];
    explodeTextureName[1] = textureName[0];
    explodeTextureName[2] = textureName[0];

    if (!displayInit)
    {
        createDisplay(display[0], COCKPIT_START, ENGINES_END);
        createDisplay(explodeDisplay[0], COCKPIT_START, COCKPIT_END);
        createDisplay(explodeDisplay[1], ENGINE_ONE_START, ENGINE_ONE_END);
        createDisplay(explodeDisplay[2], ENGINE_TWO_START, ENGINE_TWO_END);
    }

    // Fuselage.
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 2; j++)
        {
            if (ColorSeed == -1)
            {
                texture[i][j][0] = 255;           // Red
                texture[i][j][1] = 200;           // Green
                texture[i][j][2] = 200;           // Blue
            }
            else
            {
                texture[i][j][0] = rand() % 256;  // Red
                texture[i][j][1] = rand() % 256;  // Green
                texture[i][j][2] = rand() % 256;  // Blue
            }
        }
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &(textureName[1]));
    glBindTexture(GL_TEXTURE_2D, textureName[1]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &(texture[0][0][0]));
    explodeTextureName[3] = textureName[1];

    if (!displayInit)
    {
        createDisplay(display[1], FUSELAGE_START, FUSELAGE_END);
        createDisplay(explodeDisplay[3], FUSELAGE_START, FUSELAGE_END);
    }

    // Use engine cylinder to repair fuselage butt lost during 3dsmax conversion.
    if (!displayInit)
    {
        buttDisplay = glGenLists(1);
        createDisplay(buttDisplay, CYLINDER01_OFFSET, TUBE07_OFFSET - 1);
    }

    // Wings.
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 2; j++)
        {
            if (ColorSeed == -1)
            {
                texture[i][j][0] = 200;           // Red
                texture[i][j][1] = 200;           // Green
                texture[i][j][2] = 255;           // Blue
            }
            else
            {
                texture[i][j][0] = rand() % 256;  // Red
                texture[i][j][1] = rand() % 256;  // Green
                texture[i][j][2] = rand() % 256;  // Blue
            }
        }
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &(textureName[2]));
    glBindTexture(GL_TEXTURE_2D, textureName[2]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &(texture[0][0][0]));
    explodeTextureName[4] = textureName[2];
    explodeTextureName[5] = textureName[2];
    explodeTextureName[6] = textureName[2];
    explodeTextureName[7] = textureName[2];

    if (!displayInit)
    {
        createDisplay(display[2], WINGS_START, WINGS_END);
        createDisplay(explodeDisplay[4], WING04_OFFSET, WING05_OFFSET - 1);
        createDisplay(explodeDisplay[5], WING05_OFFSET, WING01_OFFSET - 1);
        createDisplay(explodeDisplay[6], WING01_OFFSET, WING_OFFSET - 1);
        createDisplay(explodeDisplay[7], WING_OFFSET, WINGS_END);
    }

    // Logo.
    if (LOGO_START != NULL)
    {
        for (i = 0; i < 2; i++)
        {
            for (j = 0; j < 2; j++)
            {
                if (ColorSeed == -1)
                {
                    texture[i][j][0] = 255;       // Red
                    texture[i][j][1] = 0;         // Green
                    texture[i][j][2] = 0;         // Blue
                }
                else
                {
                                                  // Red
                    texture[i][j][0] = rand() % 256;
                                                  // Green
                    texture[i][j][1] = rand() % 256;
                                                  // Blue
                    texture[i][j][2] = rand() % 256;
                }
            }
        }
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glGenTextures(1, &(textureName[3]));
        glBindTexture(GL_TEXTURE_2D, textureName[3]);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, 3, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, &(texture[0][0][0]));
        explodeTextureName[8] = textureName[3];

        if (!displayInit)
        {
            createDisplay(display[3], LOGO_START, LOGO_END);
            explodeDisplay[8] = display[3];
        }
    }

    // Signify static initialization done.
    displayInit = true;
}


// Create a display.
void Xwing::createDisplay(int displayIndex, int startIndex, int endIndex)
{
    int i,j,vi,ni,ti;

    glNewList(displayIndex, GL_COMPILE);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushMatrix();
    xwing_SelectMaterial(xwing_material_ref[0][0]);
    glBegin(GL_TRIANGLES);
    for(i=startIndex;i<=endIndex;i++)
    {
        for(j=0;j<3;j++)
        {
            vi=xwing_face_indicies[i][j];
            ni=xwing_face_indicies[i][j+3];       //Normal index
            ti=xwing_face_indicies[i][j+6];       //Texture index
            glNormal3f(xwing_normals[ni][0],xwing_normals[ni][1],xwing_normals[ni][2]);
            glTexCoord2f(xwing_textures[ti][0],xwing_textures[ti][1]);
            glVertex3f(xwing_vertices[vi][0],xwing_vertices[vi][1],xwing_vertices[vi][2]);
        }
    }
    glEnd();
    glPopAttrib();
    glPopMatrix();
    glEndList();
}


// Draw X-wing.
void Xwing::Draw()
{
    int i,j;

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

    // Draw components.
    if (state == EXPLODE)
        j = NUM_EXPLODING_DRAWABLES;
    else
        j = NUM_DRAWABLES;
    if (LOGO_START == NULL) j--;
    for (i = 0; i < j; i++)
    {
        if (state == EXPLODE)
        {
            // Add explosion transform to component.
            glBindTexture(GL_TEXTURE_2D, explodeTextureName[i]);
            glPushMatrix();
            explosionTransform(i);
            glCallList(explodeDisplay[i]);
            glPopMatrix();
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, textureName[i]);
            glCallList(display[i]);
        }

        // Draw cylinder to repair fuselage butt.
        if ((state == EXPLODE && i == 3) || (state != EXPLODE && i == 1))
        {
            glPushMatrix();
            glScalef(0.86, 0.86, 0.86);
            glTranslatef(-0.0675, 0.225, 0.015);
            if (state == EXPLODE)
            {
                glPushMatrix();
                explosionTransform(1);
                glCallList(buttDisplay);
                glPopMatrix();
            }
            else
            {
                glCallList(buttDisplay);
            }
            glPopMatrix();
        }
    }

    // Draw thruster exhaust, axes, and ID.
    if (state != EXPLODE)
    {
        drawExhaust();
        drawAxes();
        drawID();
    }

    glPopMatrix();
}


// Draw thruster exhaust streams.
void Xwing::drawExhaust()
{
    int i;
    GLfloat s,rx,ry,rz;

    // Stream color  depends on speed.
    if (m_spacial->speed == 0.0) return;
    if ((s = m_spacial->speed * 10.0) > 0.5) s = 0.5;
    glColor4f(2.0 * s, 2.0 * s, 0.75, 0.6);

    // Drawing options: blended streams.
    glEnable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glLineWidth(1.0);

    glBegin(GL_LINES);
    for (i = 0; i < NUM_EXHAUSTS; i++)
    {
        rx = RAND_UNIT;
        ry = (PRAND_UNIT / 2.0) + 0.5;
        rz = RAND_UNIT;
        glVertex3f(.075 + 0.03 * rx, 0.4, (0.03 * rz) - 0.01);
        glVertex3f(.075 + 0.01 * rx, 0.4 + (s * ry), (0.01 * rz) - 0.01);
    }
    for (i = 0; i < NUM_EXHAUSTS; i++)
    {
        rx = RAND_UNIT;
        ry = (PRAND_UNIT / 2.0) + 0.5;
        rz = RAND_UNIT;
        glVertex3f(-.075 + 0.03 * rx, 0.4, (0.03 * rz) - 0.01);
        glVertex3f(-.075 + 0.01 * rx, 0.4 + (s * ry), (0.01 * rz) - 0.01);
    }
    glEnd();
}


// Draw ID.
void Xwing::drawID()
{
    char *s;

    if (ID[0] == '\0') return;
    s = ID;

    // Transform to draw on wing.
    glPushMatrix();
    glTranslatef(-0.15, 0.26, 0.01);
    glRotatef(180.0, 0.0, 0.0, 1.0);
    glRotatef(-32.0, 0.0, 1.0, 0.0);
    glScalef(.0003, .0003, .0003);

    // Drawing options.
    glColor3f(IDred, IDgreen, IDblue);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glLineWidth(1.0);

    // Draw ID.
    while (*s)
    {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *s);
        s++;
    }
    glPopMatrix();
}


// Draw axes.
void Xwing::drawAxes()
{
    int i,j;
    GLfloat f;

    if (!ShowAxes) return;

    j = 20;
    f = .05;

    // Set drawing options.
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glLineWidth(1.0);

    // Draw axes.
    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_LINES);
    glVertex3f(1.0, 0.0, 0.0);
    glVertex3f(-1.0, 0.0, 0.0);
    for (i = 0; i < j; i++)
    {
        glVertex3f(((GLfloat)i * f) + f, f, 0.0);
        glVertex3f(((GLfloat)i * f) + f, -f, 0.0);
        glVertex3f(((GLfloat)i * f) + f, 0.0, f);
        glVertex3f(((GLfloat)i * f) + f, 0.0, -f);
        glVertex3f(((GLfloat)i * -f) - f, f, 0.0);
        glVertex3f(((GLfloat)i * -f) - f, -f, 0.0);
        glVertex3f(((GLfloat)i * -f) - f, 0.0, f);
        glVertex3f(((GLfloat)i * -f) - f, 0.0, -f);
    }
    glEnd();

    glColor3f(0.0, 1.0, 0.0);
    glBegin(GL_LINES);
    glVertex3f(0.0, 1.0, 0.0);
    glVertex3f(0.0, -1.0, 0.0);
    for (i = 0; i < j; i++)
    {
        glVertex3f(f, ((GLfloat)i * f) + f, 0.0);
        glVertex3f(-f, ((GLfloat)i * f) + f, 0.0);
        glVertex3f(0.0, ((GLfloat)i * f) + f, f);
        glVertex3f(0.0, ((GLfloat)i * f) + f, -f);
        glVertex3f(f, ((GLfloat)i * -f) - f, 0.0);
        glVertex3f(-f, ((GLfloat)i * -f) - f, 0.0);
        glVertex3f(0.0, ((GLfloat)i * -f) - f, f);
        glVertex3f(0.0, ((GLfloat)i * -f) - f, -f);
    }
    glEnd();

    glColor3f(0.0, 0.0, 1.0);
    glBegin(GL_LINES);
    glVertex3f(0.0, 0.0, 1.0);
    glVertex3f(0.0, 0.0, -1.0);
    for (i = 0; i < j; i++)
    {
        glVertex3f(f, 0.0, ((GLfloat)i * f) + f);
        glVertex3f(-f, 0.0, ((GLfloat)i * f) + f);
        glVertex3f(0.0, f, ((GLfloat)i * f) + f);
        glVertex3f(0.0, -f, ((GLfloat)i * f) + f);
        glVertex3f(f, 0.0, ((GLfloat)i * -f) - f);
        glVertex3f(-f, 0.0, ((GLfloat)i * -f) - f);
        glVertex3f(0.0, f, ((GLfloat)i * -f) - f);
        glVertex3f(0.0, -f, ((GLfloat)i * -f) - f);
    }
    glEnd();
}


// Get nearest vertex to given point in world coordinates.
void Xwing::GetNearestVertex(GLfloat *point, GLfloat *vertex)
{
    int i;
    GLfloat d1,d2,v[3];
    Vector p1(point[0], point[1], point[2]),p2;

    for (i = 0; i < NUM_XWING_VERTICES; i++)
    {
        LocalToWorld(xwing_vertices[i], v);
        p2.x = v[0];
        p2.y = v[1];
        p2.z = v[2];

        d1= p1.SquareDistance(p2);
        if (i == 0 || d1 < d2)
        {
            d2 = d1;
            vertex[0] = p2.x;
            vertex[1] = p2.y;
            vertex[2] = p2.z;
        }
    }
}


// Expose vertices.
void Xwing::ExposeVertices(GLfloat **vertices, int *size)
{
    *vertices = (GLfloat *)xwing_vertices;
    *size = NUM_XWING_VERTICES;
}


// Explode X-wing.
void Xwing::Explode()
{
    int i,j;

    state = EXPLODE;
    SetSpeed(0.0);

    // Initialize exploding parts.
    explosionCounter = explosionDelay;
    for (i = 0; i < NUM_EXPLODING_DRAWABLES; i++)
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
void Xwing::ExplodeUpdate()
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
    for (i = 0; i < NUM_EXPLODING_DRAWABLES; i++)
    {
        explodingParts[i].position += explodingParts[i].velocity * s;
        explodingParts[i].angle += explodingParts[i].angularVelocity;
    }
}


// Add explosion transform.
void Xwing::explosionTransform(int i)
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
