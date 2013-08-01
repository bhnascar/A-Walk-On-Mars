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
#include "Bitmap.h"

class Texture
{
public:
    Texture(GLenum format);
    Texture(GLfloat width, GLfloat height, GLenum format);
    Texture(GLfloat width, GLfloat height, GLenum format, GLfloat data[]);
    Texture(std::string filename);
    ~Texture();
    
    /** Returns the float data */
    char *GetData();
    
    /** Get dimensions */
    GLfloat GetWidth() { return width; }
    GLfloat GetHeight() { return height; }
    
    /** Loads this texture onto the GPU */
    virtual void Bind();
    
    /** Returns the texture's id */
    GLuint GetID();
    
protected:
    GLfloat width, height;
    GLfloat *data;
    GLuint id;
    GLenum format;
    
    Bitmap *bitmap;
    
    void LoadTexFile(char *filename);
};
