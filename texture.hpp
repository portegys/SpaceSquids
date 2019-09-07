//***************************************************************************//
//* File Name: texture.hpp                                                  *//
//*    Author: Chris McBride chris_a_mcbride@hotmail.com                    *//
//* Date Made: 04/06/02                                                     *//
//* File Desc: Texture loading functions for use until formal texture       *//
//*            manager is created                                           *//
//* Rev. Date: xx/xx/02                                                     *//
//* Rev. Desc:                                                              *//
//*                                                                         *//
//***************************************************************************//
#ifndef __TEXTURE_HPP__
#define __TEXTURE_HPP__

#include <GL/gl.h>
#include <GL/glu.h>
#ifndef UNIX
#include "glaux.h"
#endif

// Targa Loader
typedef struct
{
    GLubyte *imageData;
    GLuint   bpp;
    GLuint   width;
    GLuint   height;
    GLuint   texID;
} TextureImage;

bool LoadTGA(TextureImage *texture, char* filename)
{
    GLubyte TGAheader[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    GLubyte TGAcompare[12];
    GLubyte header[6];
    GLuint  bytesPerPixel;
    GLuint  imageSize;
    GLuint  type = GL_RGBA;

    FILE *file = fopen(filename, "rb");

    if(file == NULL ||
        fread(TGAcompare, 1, sizeof(TGAcompare), file) != sizeof(TGAcompare) ||
        memcmp(TGAheader, TGAcompare, sizeof(TGAheader)) != 0 ||
        fread(header, 1, sizeof(header), file) != sizeof(header))
    {
        if(file == NULL)
        {
            return false;
        }
        else
        {
            fclose(file);
            return false;
        }
    }

    texture->width  = header[1] * 256 + header[0];
    texture->height = header[3] * 256 + header[2];

    if(texture->width  <= 0 ||
        texture->height <= 0 ||
        (header[4] != 24 && header[4] != 32))
    {
        fclose(file);
        return false;
    }

    texture->bpp  = header[4];
    bytesPerPixel = texture->bpp/8;
    imageSize     = texture->width * texture->height * bytesPerPixel;

    texture->imageData = new GLubyte[imageSize];

    if(texture->imageData == NULL ||
        fread(texture->imageData, 1, imageSize, file) != imageSize)
    {
        if(texture->imageData != NULL)
            delete [] texture->imageData;

        fclose(file);
        return false;
    }

    for(unsigned int k=0; k<imageSize; k+=bytesPerPixel)
    {
        texture->imageData[k] ^= texture->imageData[k+2] ^=
            texture->imageData[k] ^= texture->imageData[k+2];
    }

    fclose(file);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &texture[0].texID);

    glBindTexture(GL_TEXTURE_2D, texture[0].texID);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    if(texture[0].bpp == 24)
    {
        type = GL_RGB;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, type, texture[0].width, texture[0].height,0, type, GL_UNSIGNED_BYTE, texture[0].imageData);

    return true;
}


#ifndef UNIX
bool CreateBmpTexture(char *filename, GLuint *textureName)
{
    AUX_RGBImageRec *pBitmap = NULL;

    if(!filename)
    {
        return false;
    }

    pBitmap = auxDIBImageLoad(filename);

    if(!pBitmap)
    {
        return false;
    }

    glGenTextures(1, textureName);

    glBindTexture(GL_TEXTURE_2D, *textureName);
    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, pBitmap->sizeX, pBitmap->sizeY, GL_RGB, GL_UNSIGNED_BYTE, pBitmap->data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if(pBitmap)
    {
        if(pBitmap->data)
        {
            free(pBitmap->data);
        }

        free(pBitmap);
    }

    return true;
}
#endif
#endif                                            // #ifndef __TEXTURE_HPP__
