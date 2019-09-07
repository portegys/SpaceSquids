#ifndef _GLOBALS
#define _GLOBALS

#include "physics.h"
#include "xwing.hpp"
#include "plasmaBoltSet.hpp"
#include "squid.hpp"

// Random number > -1.0 && < 1.0
#define RAND_UNIT ((GLfloat)(rand() - rand()) / RAND_MAX)

// Random number >= 0.0 && < 1.0
#define PRAND_UNIT ((GLfloat)rand() / RAND_MAX)

// Network and master player status.
#ifdef NETWORK
#define PORT 5068
extern bool Master;
#define IP_LENGTH 50
extern char MasterIP[IP_LENGTH+1];
#define MAX_MESSAGE_PAYLOAD (1024*10)
extern int masterXwing;
#endif

// Quantities and ranges of various blocks.
#define FIRST_WALL_BLOCK 0
#define NUM_WALL_BLOCKS 6
#define FIRST_FIXED_BLOCK (FIRST_WALL_BLOCK + NUM_WALL_BLOCKS)
#define NUM_FIXED_BLOCKS 8
#define FIRST_BLOCK (FIRST_FIXED_BLOCK + NUM_FIXED_BLOCKS)
#define NUM_BLOCKS 20

// X-wing parameters and controls.
#define NUM_XWINGS 3
#define ROTATION_DELTA 0.5
#define MAX_SPEED 0.5
#define SPEED_DELTA 0.005
#define FIRST_XWING_BLOCK (FIRST_BLOCK + NUM_BLOCKS)
#define NUM_XWING_BLOCKS 4
#define XWING_COLLISION_STEPS 50
#define MAX_SHOTS 50
struct XwingControls
{
    Xwing *xwing;
    char id[ID_LENGTH+1];
    int colorSeed;
    GLfloat pitch, yaw, roll;
    GLfloat speed;
    int bodyGroup;
    int collisionSteps;
    bool firstBump;
    int shotCount;
    bool invulnerable;
};
extern struct XwingControls Xwings[NUM_XWINGS];
extern int myXwing;                               // Index of user's xwing.
extern int createXwing();
extern void resurrectXwing(int);
extern void explodeXwing(int);
extern void killXwing(int);

// Plasma bolts.
extern PlasmaBoltSet *plasmaBolts;

// Squid paramters and controls.
#define NUM_SQUIDS 5
#define FIRST_SQUID_BLOCK (FIRST_XWING_BLOCK + (NUM_XWING_BLOCKS * NUM_XWINGS))
#define NUM_SQUID_BLOCKS 2
#define SQUID_COLLISION_STEPS 50
#define SQUID_ATTACK_RANGE 15.0
#define SQUID_ATTACK_RANDOM_VARIANCE 5.0
#define SQUID_KILL_DISTANCE 1.0
#define SQUID_KILL_DELAY 200.0
struct SquidControls
{
    Squid *squid;
    int bodyGroup;
    int collisionSteps;
    GLfloat attackRange;
    GLfloat attachPoint[3];                       // Point of attachment to target.
    float killCounter;
};
extern struct SquidControls Squids[NUM_SQUIDS];
extern void explodeSquid(int);

typedef enum { INTRO, OPTIONS, RUN, HELP, WIN, LOSE, MESSAGE, FATAL, WHO }
USERMODE;
extern USERMODE UserMode;
#define USER_MESSAGE_LENGTH 50
extern char UserMessage[USER_MESSAGE_LENGTH + 1];
#endif
