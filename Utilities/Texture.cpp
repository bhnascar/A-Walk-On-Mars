#include "Texture.h"

using namespace::std;
using namespace::glm;

Texture::Texture(GLenum format)
{
    Texture(0, 0, format);
}

Texture::Texture(GLfloat width, GLfloat height, GLenum format)
{
    Texture(0, 0, format, NULL);
}

Texture::Texture(GLfloat width, GLfloat height, GLenum format, GLfloat data[])
{
    Texture::width = width;
    Texture::height = height;
    Texture::format = format;
    glGenTextures(1, &id);
    Texture::data = data;
    bitmap = NULL;
    Bind();
}

Texture::Texture(bitmap_image *image)
{
    bitmap = image;
    width = bitmap->width();
    height = bitmap->height();
    Bind();
}

Texture::Texture(string filename)
{
    bitmap = new bitmap_image(filename);
    width = bitmap->width();
    height = bitmap->height();
    Bind();
}

Texture::~Texture()
{
    glDeleteTextures(1, &id);
    if (bitmap)
        delete bitmap;
}

const unsigned char *Texture::GetData()
{
    return bitmap->data();
}

bitmap_image *Texture::GetBitmap()
{
    return bitmap;
}

GLuint Texture::GetID()
{
    return id;
}

Texture *Texture::GetNormalMap()
{
    bitmap_image *image = new bitmap_image(bitmap->width(), bitmap->height());
    for (int x = 0; x < bitmap->width(); x++)
    {
        for (int y = 0; y < bitmap->height(); y++)
        {
            // Approximate gradient
            float dx = (bitmap->get_height(x + 1, y) - bitmap->get_height(x - 1, y)) / 2.0f;
            float dy = (bitmap->get_height(x, y + 1) - bitmap->get_height(x, y - 1)) / 2.0f;
            
            // Some encoding necessary
            // Normal coordinates can be from -1 to 1
            // We need to change it to the range 0 to 1
            vec3 x_dir = vec3(1, 0, 70 * dx);
            vec3 y_dir = vec3(0, 1, 70 * dy);
            vec3 normal = normalize(cross(x_dir, y_dir)) / 2.0f;
            normal += vec3(0.5);
            
            unsigned char r = 255 * normal.r;
            unsigned char g = 255 * normal.g;
            unsigned char b = 255 * normal.b;
            image->set_pixel(x, y, r, g, b);
        }
    }
    image->save_image("normals.bmp");
    return new Texture(image);
}

void Texture::Bind()
{
    glBindTexture(GL_TEXTURE_2D, id);
    if (bitmap) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, bitmap->data());
    }
    else if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_FLOAT, data);
    }
    else {
        if (format == GL_DEPTH_COMPONENT)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, (GLsizei) width, (GLsizei) height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        else
            glTexImage2D(GL_TEXTURE_2D, 0, format, (GLsizei) width, (GLsizei) height, 0, format, GL_UNSIGNED_BYTE, NULL);
    }
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}
