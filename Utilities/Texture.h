#pragma once

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#endif

#include <iostream>
#include <stdlib.h>
#include <string>
#include <glm/glm.hpp>
#include "bitmap_image.hpp"

class Texture
{
public:
    Texture(GLenum format);
    Texture(GLfloat width, GLfloat height, GLenum format);
    Texture(GLfloat width, GLfloat height, GLenum format, GLfloat data[]);
    Texture(bitmap_image *image);
    Texture(std::string filename);
    ~Texture();
    
    /** Returns the image data */
    virtual const unsigned char *GetData();
    virtual bitmap_image *GetBitmap();
    
    /** Get dimensions */
    virtual GLfloat GetWidth() { return width; }
    virtual GLfloat GetHeight() { return height; }
    
    /** Loads this texture onto the GPU */
    virtual void Bind();
    
    /** Returns a normal map created by interpreting
     this texture as a height map */
    Texture *GetNormalMap();
    
    /** Returns the texture's id */
    GLuint GetID();
    
protected:
    Texture() {}
    
    GLfloat width, height;
    GLfloat *data;
    GLuint id;
    GLenum format;
    
    bitmap_image *bitmap;
};
