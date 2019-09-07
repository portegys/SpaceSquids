//***************************************************************************//
//* File Name: plasmaBoltSet.hpp                                            *//
//* Author:    Tom Portegys, portegys@ilstu.edu                             *//
//* Date Made: 08/7/02                                                      *//
//* File Desc: Class declaration and implementation details                 *//
//*            representing a plasma bolt set.                              *//
//* Rev. Date:                                                              *//
//* Rev. Desc:                                                              *//
//*                                                                         *//
//***************************************************************************//

#include "plasmaBolt.hpp"

#ifndef __PLASMA_BOLT_SET__
#define __PLASMA_BOLT_SET__

class PlasmaBoltSet
{

    public:

        // Set link.
        class Link
        {
            public:
                PlasmaBolt *p;
                Link *next;
        };

        // Constructor.
        PlasmaBoltSet();

        // Destructor.
        ~PlasmaBoltSet();

        // Add a bolt to the set.
        void add(PlasmaBolt *);

        // Go: update and draw.
        void Go() { update(); draw(); }

        // Move active and delete inactive bolts.
        void update();

        // Draw bolts.
        void draw();

        // A bolt is near object of given radius and position?
        bool isNear(float *v, float r);

        // A bolt collides with object of given radius and position?
        bool collision(float *v, float r);

        // Get number of bolts in set.
        int getSize() { return(size); }

        Link *Set;
        int size;
};

// Constructor.
PlasmaBoltSet::PlasmaBoltSet()
{
    Set = NULL;
    size = 0;
}


// Destructor.
PlasmaBoltSet::~PlasmaBoltSet()
{
    Link *l;

    while (Set != NULL)
    {
        l = Set;
        Set = Set->next;
        delete l->p;
        delete l;
    }
}


// Add a plasma bolt to the set.
void PlasmaBoltSet::add(PlasmaBolt *p)
{
    Link *l;

    if (p == NULL) return;
    l = new Link();
    l->p = p;
    l->next = Set;
    Set = l;
    size++;
}


// Move active and delete inactive plasma bolts.
void PlasmaBoltSet::update()
{
    Link *l,*l2;

    for (l = Set, l2 = NULL; l != NULL; )
    {
        if (l->p->Active)
        {
            l->p->move();
            l2 = l;
            l = l->next;
        }
        else
        {
            // Delete inactive plasma bolt.
            if (l2 == NULL)
            {
                Set = l->next;
                delete l->p;
                delete l;
                l = Set;
            }
            else
            {
                l2->next = l->next;
                delete l->p;
                delete l;
                l = l2->next;
            }
            size--;
        }
    }
}


// Draw plasma bolts.
void PlasmaBoltSet::draw()
{
    Link *l;

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    for (l = Set; l != NULL; l = l->next)
    {
        l->p->draw();
    }
}


// A bolt is near object of given radius and position?
bool PlasmaBoltSet::isNear(float *v, float r)
{
    Link *l;

    for (l = Set; l != NULL; l = l->next)
    {
        if (l->p->isNear(v, r)) return(true);
    }
    return(false);
}


// A bolt collides with object of given radius and position?
// Collision destroys plasma bolt.
bool PlasmaBoltSet::collision(float *v, float r)
{
    Link *l;

    for (l = Set; l != NULL; l = l->next)
    {
        if (l->p->isNear(v, r))
        {
            l->p->Active = false;                 // destroy bolt.
            return(true);
        }
    }
    return(false);
}
#endif
