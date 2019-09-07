//***************************************************************************//
//* File Name: frameRate.hpp                                                *//
//*    Author: Chris McBride chris_a_mcbride@hotmail.com                    *//
//* Date Made: 04/06/02                                                     *//
//* File Desc: Frame rate counter for any project, windows specific.        *//
//* Rev. Date: 11/26/02                                                     *//
//* Rev. Desc: Added frame rate independence and UNIX functionality (TEP)   *//
//*                                                                         *//
//***************************************************************************//
#ifndef __FRAMERATE_HPP__
#define __FRAMERATE_HPP__

#include <time.h>

class FrameRate
{
    public:

        float targetFPS;                          // Target frames per second (FPS).
        float FPS;                                // Current FPS.
        float speedFactor;                        // Frame rate independence speed factor.
        static const float maxSpeedFactor;

        FrameRate(float targetFPS)
        {
            this->targetFPS = targetFPS;
            FPS = targetFPS;
            speedFactor = 1.0;
            frameCount = 0;
        #ifdef UNIX
            lastTime = time(NULL);
        #else
            lastTime = GetTickCount();
        #endif
        }

        // Update: call per frame.
        void update()
        {
            int currentTime,delta;

            // Count the frame.
            frameCount++;

            // Get the time delta.
        #ifdef UNIX
            currentTime = time(NULL);
            delta = currentTime - lastTime;
        #else
            currentTime = GetTickCount();
            delta = (currentTime - lastTime) / 1000;
        #endif

            // Has >= 1 second elapsed?
            if (delta >= 1)
            {
                // Calculate new values.
                FPS = (float)frameCount/(float)delta;
                speedFactor = targetFPS / FPS;
                if (speedFactor > maxSpeedFactor) speedFactor = maxSpeedFactor;
                frameCount = 0;
                lastTime = currentTime;
            }
        }

        // Reset.
        void reset()
        {
            FPS = targetFPS;
            speedFactor = 1.0;
            frameCount = 0;
        #ifdef UNIX
            lastTime = time(NULL);
        #else
            lastTime = GetTickCount();
        #endif
        }

    private:

        int frameCount;
        int lastTime;
};

// Maximum speed factor.
const float FrameRate::maxSpeedFactor = 5.0;
#endif                                            // #ifndef __FRAMERATE_HPP__
