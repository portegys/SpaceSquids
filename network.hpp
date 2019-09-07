//***************************************************************************//
//* File Name: network.hpp                                                  *//
//* Author:    Tom Portegys, portegys@ilstu.edu                             *//
//* Date Made: 4/5/03                                                       *//
//* File Desc: Networking functionality for multi-player game.              *//
//*            Uses UDP protocol.                                           *//
//* Rev. Date:                                                              *//
//* Rev. Desc:                                                              *//
//*                                                                         *//
//***************************************************************************//

#ifdef NETWORK
#ifndef __NETWORK_HPP__
#define __NETWORK_HPP__

#include "globals.h"
#ifdef UNIX
#include <socket.h>
#else
#include <winsock.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Network port.
#define GAME_PORT 4507

// Block payload size.
#define NUM_PAYLOAD_BLOCKS (NUM_BLOCKS + (NUM_XWINGS * NUM_XWING_BLOCKS) + \
    (NUM_SQUIDS * NUM_SQUID_BLOCKS))

// Maximum plasma bolt payload size.
#define MAX_BOLT_PAYLOAD 500

// Time-out for message (ms).
#define MSG_WAIT 5000
#define MSG_RETRY 10

// Exit delay.
#define EXIT_DELAY 1000

class Network
{
    public:

        // Exit status.
        typedef enum { WINNER, KILLED, QUIT }
        EXIT_STATUS;

        // Constructor.
        Network()
        {
            plasmaBoltUpdated = false;
            newPlasmaBolts = new PlasmaBoltSet();
            newMaster = false;
        }

        // Destructor.
        ~Network()
        {
            closesocket(mySocket);
            WSACleanup();
        }

        // Initialize.
        bool init(char *id, int colorSeed);

        // Synchronize state.
        bool getMaster();
        bool sendMaster();
        bool getSlave();
        bool sendSlave();

        // Is this my (local) address?
        bool isMyAddr(SOCKADDR_IN addr);

        // Player exit.
        bool exitNotify(EXIT_STATUS);

        // Plasma bolts.
        void setPlasmaBoltUpdated()
        {
            plasmaBoltUpdated = true;
        }
        void fire(PlasmaBolt *bolt)
        {
            newPlasmaBolts->add(bolt);
            plasmaBoltUpdated = true;
        }

    private:

        // Messaging functions.
        bool setupMyAddress();
        bool setupMasterAddress();
        bool sendMessage();
        bool getMessage(bool wait);

        // Plasma bolt synchronization.
        bool plasmaBoltUpdated;
        PlasmaBoltSet *newPlasmaBolts;

        // Network connections.
        SOCKADDR_IN playerAddrs[NUM_XWINGS];
        bool currentPlayers[NUM_XWINGS];
        SOCKADDR_IN myAddr,masterAddr;
        SOCKET mySocket;

        // Kludge to repeat master update after change of mastership.
        bool newMaster;

        // Structure to store info returned from WSAStartup.
        WSADATA wsda;

        // Message types.
        typedef enum
        {
            INIT, INIT_ACK, PLAYER_EXIT, MASTER_INFO, SLAVE_INFO, MARK, TIME_OUT
        } MESSAGE_TYPE;

        // Message address.
        SOCKADDR_IN messageAddress;

        // INIT message.
        struct INIT_MSG
        {
            char id[ID_LENGTH+1];
            int colorSeed;
        };

        // INIT_ACK message.
        typedef enum
        {
            ACCEPTED, REFUSED, NO_CAPACITY, REDIRECT
        } INIT_STATUS;
        struct INIT_ACK_MSG
        {
            INIT_STATUS status;
            int playerIndex;
            int masterIndex;
            char id[ID_LENGTH+1];
            int colorSeed;
            char redirectIP[IP_LENGTH+1];
        };

        // MARK message.
        struct MARK_MSG
        {
            int playerIndex;
            char id[ID_LENGTH+1];
            int colorSeed;
        };

        // PLAYER_EXIT message.
        struct PLAYER_EXIT_MSG
        {
            int status;
            int playerIndex;

            // If master is exiting, the following allows
            // remaining players to determine new master:
            // the lowest numbered remaining player.
            bool currentPlayers[NUM_XWINGS];
            SOCKADDR_IN addresses[NUM_XWINGS];
        };

        // BOLT_PAYLOAD.
        struct BOLT_PAYLOAD
        {
            GLfloat position[3];
            GLfloat speed;
            GLfloat speedFactor;
            GLfloat quaternion[4];
        };

        // MASTER_INFO message.
        struct MASTER_INFO_MSG
        {
            int masterIndex;

            // X-wing states.
            struct XwingPayload
            {
                int state;
                GLfloat position[3];
                GLfloat speed;
                GLfloat quaternion[4];
            } xwingPayload[NUM_XWINGS];

            // Squid states.
            struct SquidPayload
            {
                int state;
                int target;
                GLfloat position[3];
                GLfloat speed;
                GLfloat quaternion[4];
            } squidPayload[NUM_SQUIDS];

            // Block states.
            struct BlockPayload
            {
                GLfloat position[3];
                GLfloat velocity[3];
                GLfloat angularVelocity[3];
                GLfloat quaternion[4];
            } blockPayload[NUM_PAYLOAD_BLOCKS];

            // If numBolts > 0, a BOLT_PAYLOAD message follows this.
            int numBolts;
        };
        struct MASTER_INFO_WITH_BOLTS_MSG
        {
            struct MASTER_INFO_MSG info;
            struct BOLT_PAYLOAD bolts[MAX_BOLT_PAYLOAD];
        };

        // SLAVE_INFO message.
        struct SLAVE_INFO_MSG
        {
            int playerIndex;
            GLfloat pitch, yaw, roll;
            GLfloat speed;
            bool invulnerable;

            // If numBolts > 0, a BOLT_PAYLOAD follows this.
            int numBolts;
        };
        struct SLAVE_INFO_WITH_BOLTS_MSG
        {
            struct SLAVE_INFO_MSG info;
            struct BOLT_PAYLOAD bolts[MAX_BOLT_PAYLOAD];
        };

        // From/to address.
        SOCKADDR_IN messageAddr;

        // Message buffer.
        struct MESSAGE_BUFFER
        {
            int type;
            union
            {
                struct INIT_MSG initMsg;
                struct INIT_ACK_MSG initAckMsg;
                struct MARK_MSG markMsg;
                struct PLAYER_EXIT_MSG exitMsg;
                struct MASTER_INFO_MSG masterMsg;
                struct MASTER_INFO_WITH_BOLTS_MSG masterBoltMsg;
                struct SLAVE_INFO_MSG slaveMsg;
                struct SLAVE_INFO_WITH_BOLTS_MSG slaveBoltMsg;
            };
        } message;
};

// Initialize.
bool Network::init(char *id, int colorSeed)
{
    int i;
    struct XwingControls xcontrols;

    // Load version 1.1 of Winsock
    WSAStartup(MAKEWORD(1,1), &wsda);

    // Set up my networking.
    if (!setupMyAddress()) return false;

    // Initialize current players.
    for (i = 0; i < NUM_XWINGS; i++) currentPlayers[i] = false;
    currentPlayers[myXwing] = true;

    // Master plays until slaves connect.
    if (Master)
    {
        masterXwing = myXwing;
        return true;
    }

    // Address master.
    if (!setupMasterAddress()) return false;

    // Cannot be master and slave simultaneously.
    if (isMyAddr(masterAddr))
    {
        strcpy(UserMessage, "cannot connect to self, continuing as master");
        masterXwing = myXwing;
        Master = true;
        UserMode = MESSAGE;
        return true;
    }
    else
    {
        if (UserMode == FATAL) return false;
    }

    // Connect to master.
    while (true)
    {
        messageAddr = masterAddr;
        message.type = INIT;
        strncpy(message.initMsg.id, id, ID_LENGTH);
        message.initMsg.colorSeed = colorSeed;
        if (!sendMessage()) return false;
        if (!getMessage(true)) return false;

        if (message.type == INIT_ACK)
        {
            // Redirect to true master?
            if (message.initAckMsg.status == REDIRECT)
            {
                strncpy(MasterIP, message.initAckMsg.redirectIP, IP_LENGTH);
                if (!setupMasterAddress()) return false;
                continue;
            }
            if (message.initAckMsg.status == ACCEPTED)
            {
                // Switch over to assigned player number.
                i = myXwing;
                myXwing = message.initAckMsg.playerIndex;
                if (myXwing != i)
                {
                    xcontrols.xwing = Xwings[i].xwing;
                    strncpy(xcontrols.id, Xwings[i].id, ID_LENGTH);
                    xcontrols.colorSeed = Xwings[i].colorSeed;
                    Xwings[i].xwing = Xwings[myXwing].xwing;
                    strncpy(Xwings[i].id, Xwings[myXwing].id, ID_LENGTH);
                    Xwings[i].colorSeed = Xwings[myXwing].colorSeed;
                    Xwings[myXwing].xwing = xcontrols.xwing;
                    strncpy(Xwings[myXwing].id, xcontrols.id, ID_LENGTH);
                    Xwings[myXwing].colorSeed = xcontrols.colorSeed;
                    resurrectXwing(myXwing);
                    currentPlayers[myXwing] = true;
                    currentPlayers[i] = false;
                }
                masterXwing = message.initAckMsg.masterIndex;
                strncpy(Xwings[masterXwing].id, message.initAckMsg.id, ID_LENGTH);
                Xwings[masterXwing].colorSeed = message.initAckMsg.colorSeed;
                delete Xwings[masterXwing].xwing;
                Xwings[masterXwing].xwing = new Xwing(Xwings[masterXwing].id, Xwings[masterXwing].colorSeed);
                resurrectXwing(masterXwing);
                return true;
            }
            if (message.initAckMsg.status == REFUSED)
            {
                strcpy(UserMessage, "connection refused, continuing as master");
                masterXwing = myXwing;
                Master = true;
                UserMode = MESSAGE;
                return true;
            }
            if (message.initAckMsg.status == NO_CAPACITY)
            {
                strcpy(UserMessage, "cannot add new player, continuing as master");
                masterXwing = myXwing;
                Master = true;
                UserMode = MESSAGE;
                return true;
            }
        }
        if (message.type == TIME_OUT)
        {
            strcpy(UserMessage, "connection attempt timed-out, continuing as master");
            masterXwing = myXwing;
            Master = true;
            UserMode = MESSAGE;
            return true;
        }
        sprintf(UserMessage, "unexpected message type=%d arrived, continuing as master");
        masterXwing = myXwing;
        currentPlayers[myXwing] = true;
        Master = true;
        UserMode = MESSAGE;
        return true;
    }
    return true;
}


// Get state of master.
bool Network::getMaster()
{
    register int i,j;
    register Xwing *xwing;
    register Squid *squid;

    if (!getMessage(true)) return false;
    switch(message.type)
    {
        case MASTER_INFO:
        {
            // Mastership might change.
            masterXwing = message.masterMsg.masterIndex;
            masterAddr = messageAddr;

            // Update X-wings.
            for (i = 0; i < NUM_XWINGS; i++)
            {
                // Update state.
                xwing = Xwings[i].xwing;
                switch(message.masterMsg.xwingPayload[i].state)
                {
                    case Xwing::ALIVE:
                        if (xwing->state != Xwing::ALIVE)
                        {
                            resurrectXwing(i);
                        }
                        else
                        {
                            xwing->Update();
                        }
                        break;
                    case Xwing::EXPLODE:
                        if (xwing->state != Xwing::EXPLODE && xwing->state != Xwing::DEAD)
                        {
                            explodeXwing(i);
                        }
                        else
                        {
                            xwing->Update();
                        }
                        break;
                    case Xwing::DEAD:
                        if (xwing->state != Xwing::DEAD)
                        {
                            killXwing(i);
                        }
                        else
                        {
                            xwing->Update();
                        }
                        break;
                }

                // Update position.
                xwing->SetPosition(message.masterMsg.xwingPayload[i].position);

                // Update speed.
                xwing->SetSpeed(message.masterMsg.xwingPayload[i].speed);

                // Update rotational state.
                for (j = 0; j < 4; j++)
                {
                    xwing->GetSpacial()->qcalc->quat[j] =
                        message.masterMsg.xwingPayload[i].quaternion[j];
                }
                xwing->GetSpacial()->build_rotmatrix();
            }

            // Update squids.
            for (i = 0; i < NUM_SQUIDS; i++)
            {
                // Update state.
                squid = Squids[i].squid;
                j = Squids[i].bodyGroup;
                switch(message.masterMsg.squidPayload[i].state)
                {
                    case Squid::IDLE:
                        if (squid->state != Squid::IDLE)
                        {
                            squid->Idle();
                            Bodies[j + 1].valid = true;
                        }
                        else
                        {
                            squid->Update();
                        }
                        break;
                    case Squid::ATTACK:
                        if (squid->state != Squid::ATTACK)
                        {
                            xwing = Xwings[message.masterMsg.squidPayload[i].target].xwing;
                            squid->Attack(xwing);
                            Bodies[j + 1].valid = false;
                        }
                        else
                        {
                            squid->Update();
                        }
                        break;
                    case Squid::DESTROY:
                        if (squid->state != Squid::DESTROY)
                        {
                            squid->Destroy(message.masterMsg.squidPayload[i].target);
                        }
                        else
                        {
                            squid->Update();
                        }
                        break;
                    case Squid::EXPLODE:
                        if (squid->state != Squid::EXPLODE && squid->state != Squid::DEAD)
                        {
                            explodeSquid(i);
                        }
                        else
                        {
                            squid->Update();
                        }
                        break;
                    case Squid::DEAD:
                        if (squid->state != Squid::DEAD)
                        {
                            squid->Kill();
                            Bodies[j].valid = false;
                            Bodies[j + 1].valid = false;
                        }
                        else
                        {
                            squid->Update();
                        }
                        break;
                }

                // Update position.
                squid->SetPosition(message.masterMsg.squidPayload[i].position);

                // Update speed.
                squid->SetSpeed(message.masterMsg.squidPayload[i].speed);

                // Update rotational state.
                for (j = 0; j < 4; j++)
                {
                    squid->GetSpacial()->qcalc->quat[j] =
                        message.masterMsg.squidPayload[i].quaternion[j];
                }
                squid->GetSpacial()->build_rotmatrix();
            }

            // Update blocks.
            // Only need velocity components if transfer of mastership happens.
            for (i = 0; i < NUM_PAYLOAD_BLOCKS; i++)
            {
                j = FIRST_BLOCK + i;
                Bodies[j].vPosition.x = message.masterMsg.blockPayload[i].position[0];
                Bodies[j].vPosition.y = message.masterMsg.blockPayload[i].position[1];
                Bodies[j].vPosition.z = message.masterMsg.blockPayload[i].position[2];
                Bodies[j].vVelocity.x = message.masterMsg.blockPayload[i].velocity[0];
                Bodies[j].vVelocity.y = message.masterMsg.blockPayload[i].velocity[1];
                Bodies[j].vVelocity.z = message.masterMsg.blockPayload[i].velocity[2];
                Bodies[j].vAngularVelocity.x = message.masterMsg.blockPayload[i].angularVelocity[0];
                Bodies[j].vAngularVelocity.y = message.masterMsg.blockPayload[i].angularVelocity[1];
                Bodies[j].vAngularVelocity.z = message.masterMsg.blockPayload[i].angularVelocity[2];
                Bodies[j].qOrientation.n = message.masterMsg.blockPayload[i].quaternion[0];
                Bodies[j].qOrientation.v.x = message.masterMsg.blockPayload[i].quaternion[1];
                Bodies[j].qOrientation.v.y = message.masterMsg.blockPayload[i].quaternion[2];
                Bodies[j].qOrientation.v.z = message.masterMsg.blockPayload[i].quaternion[3];
            }

            // If numBolts > 0, a BOLT_PAYLOAD message is included.
            if (message.masterMsg.numBolts == 0) break;

            // Replace plasma bolts.
            delete plasmaBolts;
            plasmaBolts = new PlasmaBoltSet();
            for (i = 0; i < message.masterMsg.numBolts; i++)
            {
                plasmaBolts->add(new PlasmaBolt(
                    message.masterBoltMsg.bolts[i].position[0],
                    message.masterBoltMsg.bolts[i].position[1],
                    message.masterBoltMsg.bolts[i].position[2],
                    0.5, 1.0, message.masterBoltMsg.bolts[i].quaternion));
            }
        }
        break;

        case INIT:
            // Redirect request to master.
            {
                message.type = INIT_ACK;
                message.initAckMsg.status = REDIRECT;
                strncpy(message.initAckMsg.redirectIP, inet_ntoa(masterAddr.sin_addr), IP_LENGTH);
                if (!sendMessage()) return false;

                // Continue waiting for master info.
                if (!getMaster()) return false;
            }
            break;

        case MARK:
            // Mark X-wing with player's id and colors.
            {
                i = message.markMsg.playerIndex;
                strncpy(Xwings[i].id, message.markMsg.id, ID_LENGTH);
                Xwings[i].colorSeed = message.markMsg.colorSeed;
                delete Xwings[i].xwing;
                Xwings[i].xwing = new Xwing(message.markMsg.id, message.markMsg.colorSeed);
                resurrectXwing(i);

                // Continue waiting for master info.
                if (!getMaster()) return false;
            }
            break;

        case PLAYER_EXIT:
            // Master assigning me as new master.
            {
                // Kill master.
                currentPlayers[masterXwing] = false;
                killXwing(masterXwing);

                // Store player addresses.
                for (i = 0; i < NUM_XWINGS; i++)
                {
                    playerAddrs[i] = message.exitMsg.addresses[i];
                    currentPlayers[i] = message.exitMsg.currentPlayers[i];
                }

                // Assume mastership.
                masterAddr = playerAddrs[myXwing];
                masterXwing = myXwing;
                Master = true;

                // Set flag to repeat first master message.
                newMaster = true;
            }
            break;

            // Assume message lost.
        case TIME_OUT:
        {
            masterXwing = myXwing;
            Master = true;
            currentPlayers[myXwing] = true;
            for (i = 0; i < NUM_XWINGS; i++)
            {
                if (i == myXwing) continue;
                killXwing(i);
                currentPlayers[i] = false;
            }
            strcpy(UserMessage, "connection timed-out, continuing as master");
            UserMode = MESSAGE;
        }
        break;
    }
    return true;
}


// Send state of master to slaves.
bool Network::sendMaster()
{
    register int i,j;
    register Xwing *xwing;
    register Squid *squid;
    PlasmaBoltSet::Link *link;
    PlasmaBolt *bolt;

    // Store X-wings.
    for (i = 0; i < NUM_XWINGS; i++)
    {
        xwing = Xwings[i].xwing;
        message.masterMsg.xwingPayload[i].state = xwing->state;
        xwing->GetPosition(message.masterMsg.xwingPayload[i].position);
        message.masterMsg.xwingPayload[i].speed = xwing->GetSpeed();
        for (j = 0; j < 4; j++)
        {
            message.masterMsg.xwingPayload[i].quaternion[j] =
                xwing->GetSpacial()->qcalc->quat[j];
        }
    }

    // Store squids.
    for (i = 0; i < NUM_SQUIDS; i++)
    {
        squid = Squids[i].squid;
        message.masterMsg.squidPayload[i].state = squid->state;
        message.masterMsg.squidPayload[i].target = -1;
        if (squid->state == Squid::ATTACK || squid->state == Squid::DESTROY)
        {
            xwing = (Xwing *)squid->GetTarget();
            for (j = 0; j < NUM_XWINGS; j++)
            {
                if (xwing == Xwings[j].xwing) break;
            }
            if (j < NUM_XWINGS)
            {
                if (squid->state == Squid::ATTACK)
                {
                    message.masterMsg.squidPayload[i].target = j;
                }
                else
                {
                    message.masterMsg.squidPayload[i].target = Xwings[j].bodyGroup;
                }
            }
        }
        squid->GetPosition(message.masterMsg.squidPayload[i].position);
        message.masterMsg.squidPayload[i].speed = squid->GetSpeed();
        for (j = 0; j < 4; j++)
        {
            message.masterMsg.squidPayload[i].quaternion[j] =
                squid->GetSpacial()->qcalc->quat[j];
        }
    }

    // Update blocks.
    // Only need velocity components for transfer of mastership.
    for (i = 0; i < NUM_PAYLOAD_BLOCKS; i++)
    {
        j = FIRST_BLOCK + i;
        message.masterMsg.blockPayload[i].position[0] = Bodies[j].vPosition.x;
        message.masterMsg.blockPayload[i].position[1] = Bodies[j].vPosition.y;
        message.masterMsg.blockPayload[i].position[2] = Bodies[j].vPosition.z;
        message.masterMsg.blockPayload[i].velocity[0] = Bodies[j].vVelocity.x;
        message.masterMsg.blockPayload[i].velocity[1] = Bodies[j].vVelocity.y;
        message.masterMsg.blockPayload[i].velocity[2] = Bodies[j].vVelocity.z;
        message.masterMsg.blockPayload[i].angularVelocity[0] = Bodies[j].vAngularVelocity.x;
        message.masterMsg.blockPayload[i].angularVelocity[1] = Bodies[j].vAngularVelocity.y;
        message.masterMsg.blockPayload[i].angularVelocity[2] = Bodies[j].vAngularVelocity.z;
        message.masterMsg.blockPayload[i].quaternion[0] = Bodies[j].qOrientation.n;
        message.masterMsg.blockPayload[i].quaternion[1] = Bodies[j].qOrientation.v.x;
        message.masterMsg.blockPayload[i].quaternion[2] = Bodies[j].qOrientation.v.y;
        message.masterMsg.blockPayload[i].quaternion[3] = Bodies[j].qOrientation.v.z;
    }

    if (plasmaBoltUpdated)
    {
        message.masterMsg.numBolts = plasmaBolts->getSize();
        for (i = 0, link = plasmaBolts->Set; i < message.masterMsg.numBolts;
            i++, link = link->next)
        {
            bolt = link->p;
            message.masterBoltMsg.bolts[i].position[0] = bolt->X;
            message.masterBoltMsg.bolts[i].position[1] = bolt->Y;
            message.masterBoltMsg.bolts[i].position[2] = bolt->Z;
            message.masterBoltMsg.bolts[i].speed = bolt->Speed;
            message.masterBoltMsg.bolts[i].speedFactor = bolt->SpeedFactor;
            message.masterBoltMsg.bolts[i].quaternion[0] = bolt->Qcalc->quat[0];
            message.masterBoltMsg.bolts[i].quaternion[1] = bolt->Qcalc->quat[1];
            message.masterBoltMsg.bolts[i].quaternion[2] = bolt->Qcalc->quat[2];
            message.masterBoltMsg.bolts[i].quaternion[3] = bolt->Qcalc->quat[3];
        }
    }
    else
    {
        message.masterMsg.numBolts = 0;
    }
    plasmaBoltUpdated = false;

    // Send update to slaves.
    message.type = MASTER_INFO;
    message.masterMsg.masterIndex = myXwing;
    retry:
    for (i = 0; i < NUM_XWINGS; i++)
    {
        if (currentPlayers[i] && i != myXwing)
        {
            messageAddr = playerAddrs[i];
            if (!sendMessage()) return false;
        }
    }

    // If new master retry first message.
    if (newMaster)
    {
        newMaster = false;
        Sleep(1);
        goto retry;
    }
    return true;
}


// Get state of slaves.
bool Network::getSlave()
{
    register int i,j,count;
    register Xwing *xwing;
    bool needInfo[NUM_XWINGS];
    bool gotinit;

    // How many slaves?
    for (i = count = 0; i < NUM_XWINGS; i++)
    {
        if (currentPlayers[i] && i != myXwing)
        {
            needInfo[i] = true;
            count++;
        }
        else
        {
            needInfo[i] = false;
        }
    }

    // If no slaves, check for new players.
    gotinit = false;
    if (count == 0)
    {
        if (!getMessage(false)) return false;
        if (message.type == INIT) gotinit = true;
    }

    while (gotinit || count > 0)
    {
        if (!gotinit) if (!getMessage(true)) return false;
        gotinit = false;
        switch(message.type)
        {
            case SLAVE_INFO:
            {
                i = message.slaveMsg.playerIndex;
                if (!currentPlayers[i]) break;
                needInfo[i] = false;
                count--;
                xwing = Xwings[i].xwing;
                xwing->SetPitch(message.slaveMsg.pitch);
                xwing->SetYaw(message.slaveMsg.yaw);
                xwing->SetRoll(message.slaveMsg.roll);
                xwing->SetSpeed(message.slaveMsg.speed);
                Xwings[i].invulnerable = message.slaveMsg.invulnerable;
                for (j = 0; j < message.slaveMsg.numBolts; j++)
                {
                    plasmaBolts->add(new PlasmaBolt(
                        message.slaveBoltMsg.bolts[j].position[0],
                        message.slaveBoltMsg.bolts[j].position[1],
                        message.slaveBoltMsg.bolts[j].position[2],
                        0.5, 1.0, message.slaveBoltMsg.bolts[j].quaternion));
                }
                if (message.slaveMsg.numBolts > 0) plasmaBoltUpdated = true;
            }
            break;

            case INIT:
                // New player request.
                {
                    message.type = INIT_ACK;
                    for (i = 0; i < NUM_XWINGS; i++)
                    {
                        if (!currentPlayers[i]) break;
                    }
                    if (i < NUM_XWINGS)
                    {
                        currentPlayers[i] = true;
                        playerAddrs[i] = messageAddr;
                        strncpy(Xwings[i].id, message.initMsg.id, ID_LENGTH);
                        Xwings[i].colorSeed = message.initMsg.colorSeed;
                        delete Xwings[i].xwing;
                        Xwings[i].xwing = new Xwing(Xwings[i].id, Xwings[i].colorSeed);
                        resurrectXwing(i);
                        message.initAckMsg.status = ACCEPTED;
                        message.initAckMsg.playerIndex = i;
                        message.initAckMsg.masterIndex = masterXwing;
                        strncpy(message.initAckMsg.id, Xwings[masterXwing].id, ID_LENGTH);
                        message.initAckMsg.colorSeed = Xwings[masterXwing].colorSeed;
                        if (!sendMessage()) return false;
                        needInfo[i] = true;
                        count++;

                        // Inform other players of new player identity.
                        message.type = MARK;
                        message.markMsg.playerIndex = i;
                        strncpy(message.markMsg.id, Xwings[i].id, ID_LENGTH);
                        message.markMsg.colorSeed = Xwings[i].colorSeed;
                        for (j = 0; j < NUM_XWINGS; j++)
                        {
                            if (j == i || j == masterXwing || !currentPlayers[j]) continue;
                            messageAddr = playerAddrs[j];
                            if (!sendMessage()) return false;
                        }

                        // Inform new player of other player identities.
                        message.type = MARK;
                        messageAddr = playerAddrs[i];
                        for (j = 0; j < NUM_XWINGS; j++)
                        {
                            if (j == i || j == masterXwing || !currentPlayers[j]) continue;
                            message.markMsg.playerIndex = j;
                            strncpy(message.markMsg.id, Xwings[j].id, ID_LENGTH);
                            message.markMsg.colorSeed = Xwings[j].colorSeed;
                            if (!sendMessage()) return false;
                        }
                    }
                    else
                    {
                        message.initAckMsg.status = NO_CAPACITY;
                        if (!sendMessage()) return false;
                    }
                }
                break;

            case PLAYER_EXIT:
            {
                // Player exiting.
                i = message.exitMsg.playerIndex;
                if (currentPlayers[i])
                {
                    currentPlayers[i] = false;
                    killXwing(i);
                    if (needInfo[i])
                    {
                        needInfo[i] = false;
                        count--;
                    }
                }
            }
            break;

            case TIME_OUT:
            {
                // Assume all non-responding players are gone.
                for (i = 0; i < NUM_XWINGS; i++)
                {
                    if (needInfo[i])
                    {
                        currentPlayers[i] = false;
                        killXwing(i);
                    }
                }
            }
            return true;
        }
    }
    return true;
}


// Send state of slave to master.
bool Network::sendSlave()
{
    int i;
    PlasmaBoltSet::Link *link;
    PlasmaBolt *bolt;

    messageAddr = masterAddr;
    message.type = SLAVE_INFO;
    message.slaveMsg.playerIndex = myXwing;
    message.slaveMsg.pitch = Xwings[myXwing].xwing->GetPitch();
    message.slaveMsg.yaw = Xwings[myXwing].xwing->GetYaw();
    message.slaveMsg.roll = Xwings[myXwing].xwing->GetRoll();
    message.slaveMsg.speed = Xwings[myXwing].xwing->GetSpeed();
    message.slaveMsg.invulnerable = Xwings[myXwing].invulnerable;
    message.slaveMsg.numBolts = newPlasmaBolts->getSize();
    for (i = 0, link = newPlasmaBolts->Set; i < message.slaveMsg.numBolts;
        i++, link = link->next)
    {
        bolt = link->p;
        message.slaveBoltMsg.bolts[i].position[0] = bolt->X;
        message.slaveBoltMsg.bolts[i].position[1] = bolt->Y;
        message.slaveBoltMsg.bolts[i].position[2] = bolt->Z;
        message.slaveBoltMsg.bolts[i].speed = bolt->Speed;
        message.slaveBoltMsg.bolts[i].speedFactor = bolt->SpeedFactor;
        message.slaveBoltMsg.bolts[i].quaternion[0] = bolt->Qcalc->quat[0];
        message.slaveBoltMsg.bolts[i].quaternion[1] = bolt->Qcalc->quat[1];
        message.slaveBoltMsg.bolts[i].quaternion[2] = bolt->Qcalc->quat[2];
        message.slaveBoltMsg.bolts[i].quaternion[3] = bolt->Qcalc->quat[3];
    }
    if (message.slaveMsg.numBolts > 0)
    {
        delete newPlasmaBolts;
        newPlasmaBolts = new PlasmaBoltSet();
    }
    if (!sendMessage()) return false;
    return true;
}


// Notify players of exiting player.
bool Network::exitNotify(EXIT_STATUS status)
{
    register int i;

    message.type = PLAYER_EXIT;
    message.exitMsg.status = status;
    currentPlayers[myXwing] = false;

    if (Master)
    {
        for (i = 0; i < NUM_XWINGS; i++)
        {
            message.exitMsg.addresses[i] = playerAddrs[i];
            message.exitMsg.currentPlayers[i] = currentPlayers[i];
        }

        // Send player states to new master.
        for (i = 0; i < NUM_XWINGS; i++)
        {
            if (currentPlayers[i]) break;
        }
        if (i < NUM_XWINGS)
        {
            messageAddr = playerAddrs[i];
            if (!sendMessage()) return false;
        }
    }
    else
    {
        message.exitMsg.playerIndex = myXwing;
        messageAddr = masterAddr;
        if (!sendMessage()) return false;
    }

    // Wait for message to be sent to prevent receive error.
    Sleep(EXIT_DELAY);

    return true;
}


// Set up my address.
bool Network::setupMyAddress()
{
    unsigned long a[1];

    // Create UDP socket
    mySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (mySocket == SOCKET_ERROR)
    {
        sprintf(UserMessage, "socket call failed with: %d", WSAGetLastError());
        UserMode = FATAL;
        return false;
    }

    // Fill in the interface information.
    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons(GAME_PORT);
    myAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to port.
    if (bind(mySocket, (struct sockaddr *) &myAddr, sizeof(myAddr)) == SOCKET_ERROR)
    {
        sprintf(UserMessage, "bind call failed with: %d", WSAGetLastError());
        UserMode = FATAL;
        return false;
    }

    // Make socket non-blocking.
    a[0] = 1;
    if (ioctlsocket(mySocket, FIONBIO, a) == SOCKET_ERROR)
    {
        sprintf(UserMessage, "cannot make socket non-blocking");
        UserMode = FATAL;
        return false;
    }
    return true;
}


// Set up master address.
bool Network::setupMasterAddress()
{
    struct hostent *host;

    // Fill in the host information
    masterAddr.sin_family = AF_INET;
    masterAddr.sin_port = htons(GAME_PORT);
    masterAddr.sin_addr.s_addr = inet_addr(MasterIP);

    // Resolve non-numeric address?
    if(masterAddr.sin_addr.s_addr == INADDR_NONE)
    {
        host = NULL;
        host = gethostbyname(MasterIP);
        if(host == NULL)
        {
            sprintf(UserMessage, "unknown host: %s", MasterIP);
            UserMode = FATAL;
            return false;
        }
        memcpy(&masterAddr.sin_addr, host->h_addr_list[0], host->h_length);
    }
    return true;
}


// Send message from message buffer.
bool Network::sendMessage()
{
    int ret,len;

    // Compute message length.
    len = sizeof(int);
    switch(message.type)
    {
        case INIT: len += sizeof(struct INIT_MSG); break;
        case INIT_ACK: len += sizeof(struct INIT_ACK_MSG); break;
        case MARK: len += sizeof(struct MARK_MSG); break;
        case PLAYER_EXIT: len += sizeof(struct PLAYER_EXIT_MSG); break;
        case MASTER_INFO:
        {
            len += sizeof(struct MASTER_INFO_MSG);
            len += sizeof(struct BOLT_PAYLOAD) * message.masterMsg.numBolts;
        }
        break;
        case SLAVE_INFO:
        {
            len += sizeof(struct SLAVE_INFO_MSG);
            len += sizeof(struct BOLT_PAYLOAD) * message.slaveMsg.numBolts;
        }
        break;
    }

    // Send message.
    ret = sendto(mySocket, (char *)&message, len, 0,
        (struct sockaddr *) &messageAddr, sizeof(messageAddr));
    if(ret == SOCKET_ERROR)
    {
        sprintf(UserMessage, "sendto call failed with: %d", WSAGetLastError());
        UserMode = FATAL;
        return false;
    }
    return true;
}


// Get a message into message buffer.
bool Network::getMessage(bool wait)
{
    int ret,addrLen;

    for (int timer = 0; timer < MSG_WAIT; timer += MSG_RETRY)
    {
        addrLen = sizeof(messageAddr);
        ret = recvfrom(mySocket, (char *)&message, sizeof(struct MESSAGE_BUFFER), 0,
            (struct sockaddr *) &messageAddr, &addrLen);

        if (ret == SOCKET_ERROR)
        {
            if (WSAGetLastError() == WSAEWOULDBLOCK)
            {
                if (!wait) break;
                Sleep(MSG_RETRY);
                continue;
            }
            sprintf(UserMessage, "recvfrom call failed with: %d", WSAGetLastError());
            UserMode = FATAL;
            return false;
        }

        // Got message.
        return true;
    }

    // Time-out.
    message.type = TIME_OUT;
    return true;
}


// Is this my (local) address?
bool Network::isMyAddr(SOCKADDR_IN testAddr)
{
    char ac[80];
    if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR)
    {
        sprintf(UserMessage, "error %d when getting local host name", WSAGetLastError());
        UserMode = FATAL;
        return false;
    }

    struct hostent *phe = gethostbyname(ac);
    if (phe == 0)
    {
        sprintf(UserMessage, "bad host lookup");
        UserMode = FATAL;
        return false;
    }

    // Check all my addresses.
    for (int i = 0; phe->h_addr_list[i] != 0; ++i)
    {
        struct in_addr iaddr;
        SOCKADDR_IN saddr;
        memcpy(&iaddr, phe->h_addr_list[i], sizeof(struct in_addr));
        saddr.sin_addr.s_addr = inet_addr(inet_ntoa(iaddr));
        if (testAddr.sin_addr.s_addr == saddr.sin_addr.s_addr) return true;
    }
    return false;
}
#endif                                            // #ifndef __NETWORK_HPP__
#endif                                            // #ifdef NETWORK
