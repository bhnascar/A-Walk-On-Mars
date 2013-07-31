#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#endif

#include <iostream>

#include "Program.h"
#include "Texture.h"
#include "ParticleSystem.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

/* Must be a power of 2 */
#define ARR_SIZE 128
#define FEATURE_SIZE 32

using namespace::glm;
using namespace::std;

static ParticleSystem *particle_sys;
static Program *program;
static Texture *heightField;

static mat4 projection;
static mat4 view;
static vec3 eyePos;

static float *path;
static float count;

static fquat orientation;
bool key_right, key_left, key_up, key_down;
bool roll_left, roll_right;
bool zoom_in, zoom_out;

/** Random terrain generation */

int sign() {
    return ((arc4random() % 2) ? 1 : -1);
}

float rand2(float min, float max) {
    return min + (float)random() / RAND_MAX * (max - min);
}

float *arrayGen()
{
    float* arr = new float[ARR_SIZE * ARR_SIZE];
    for (int i = 0; i < ARR_SIZE * ARR_SIZE; i++) {
        arr[i] = 0;
    }
    return arr;
}

void logMap(float *map)
{
    std::cout.precision(3);
    std::cout << std::fixed;
    
    for (int i = 0; i < ARR_SIZE * ARR_SIZE; i++) {
        if (i % ARR_SIZE == 0)
            cout << endl;
        cout << map[i] << "\t";
    }
    cout << endl;
}

float sample(float *map, int x, int y)
{
    return map[((y & (ARR_SIZE - 1)) * ARR_SIZE) + (x & (ARR_SIZE - 1))];
}

void setSample(float *map, int x, int y, float value)
{
    map[((y & (ARR_SIZE - 1)) * ARR_SIZE) + (x & (ARR_SIZE - 1))] = value;
}

void seed(float *map, float featureSize)
{
    for (int i = 0; i < ARR_SIZE; i += featureSize) {
        for (int j = 0; j < ARR_SIZE; j += featureSize) {
            setSample(map, i, j, rand2(-1.0, 1.0));
        }
    }
}

/* Square step */
void square(float *map, float halfSquare, float x, float y, float rand)
{
    float value = sample(map, x - halfSquare, y - halfSquare) + // Upper left
                  sample(map, x + halfSquare, y - halfSquare) + // Upper right
                  sample(map, x - halfSquare, y + halfSquare) + // Lower left
                  sample(map, x + halfSquare, y + halfSquare);  // Lower right

    setSample(map, x, y, value / 4.0 + rand);
}

/* Diamond step */
void diamond(float *map, float halfSquare, float x, float y, float rand)
{
    float value = sample(map, x, y - halfSquare) + // Top
                  sample(map, x, y + halfSquare) + // Bottom
                  sample(map, x - halfSquare, y) + // Left
                  sample(map, x + halfSquare, y);  // Right
    
    setSample(map, x, y, value / 4.0 + rand);
}

/* Diamond square */
void diamondSquare(float *map, float featureSize)
{
    int squareSize = featureSize;
    float scale = 1.0;
    
    while (squareSize > 1)
    {
        float halfSquare = squareSize / 2;
        
        // Square step
        for (int i = halfSquare; i < ARR_SIZE + halfSquare; i += squareSize) {
            for (int j = halfSquare; j < ARR_SIZE + halfSquare; j += squareSize) {
                square(map, halfSquare, i, j, scale * rand2(-1.0, 1.0));
            }
        }
        
        // Diamond step
        for (int i = halfSquare; i < ARR_SIZE + halfSquare; i += squareSize) {
            for (int j = halfSquare; j < ARR_SIZE + halfSquare; j += squareSize) {
                diamond(map, halfSquare, i - halfSquare, j, scale * rand2(-1.0, 1.0));
                diamond(map, halfSquare, i, j - halfSquare, scale * rand2(-1.0, 1.0));
            }
        }
        
        scale /= 2.0;
        squareSize /= 2;
    }
}

void normalize(float *map)
{
    float max = 0.0;
    for (int i = 0; i < ARR_SIZE * ARR_SIZE; i++) {
        map[i] += 1.0;
        if (map[i] > max)
            max = map[i];
    }
    for (int i = 0; i < ARR_SIZE * ARR_SIZE; i++) {
        map[i] /= max;
    }
}

void midpointDisplace(float *path, int lower, int upper, float scale)
{
    int midpoint = (lower + upper) / 2;
    if (midpoint == lower || midpoint == upper)
        return;
    
    float gap = upper - lower;
    path[midpoint] = (path[upper] + path[lower]) / 2.0 + scale * rand2(-gap, gap);
    
    midpointDisplace(path, lower, midpoint, scale / 2.0);
    midpointDisplace(path, midpoint, upper, scale / 2.0);
}

float *mapGen()
{
    float *map = arrayGen();
    seed(map, FEATURE_SIZE);
    diamondSquare(map, FEATURE_SIZE);
    normalize(map);
    
    //heightField = new Texture(ARR_SIZE, ARR_SIZE, GL_LUMINANCE, map);
    heightField = new Texture("field.bmp");
    
    return map;
}

/** Random path generation */

void logPath(float *path)
{
    std::cout.precision(3);
    std::cout << std::fixed;
    
    for (int i = 0; i < ARR_SIZE; i++)
    {
        cout << "(" << path[i] << ", " << (float)i/ARR_SIZE << ")\t";
    }
    cout << endl;
}

float *pathGen()
{
    float *path = new float[ARR_SIZE];
    
    // Seed path
    path[0] = rand2(0, ARR_SIZE);
    path[ARR_SIZE - 1] = rand2(0, ARR_SIZE);
    
    // Midpoint displacement
    midpointDisplace(path, 0, ARR_SIZE - 1, 0.5);
    
    // Pseudo-normalize
    for (int i = 0; i < ARR_SIZE; i++) {
        path[i] /= ARR_SIZE;
    }
    
    logPath(path);
    
    return path;
}

void display() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    program->Use();
    
    glLoadIdentity();
    
    
    /* Path generation
     glLineWidth(3.0);
     glBegin(GL_LINE_STRIP);
     for (int i = 0; i < ARR_SIZE; i++) {
     glVertex3f(path[i] - 0.5, (float)i / ARR_SIZE - 0.5, 0.0);
     }
     glEnd();*/
    
    mat4 model = mat4(1) * glm::mat4_cast(orientation);
    view = glm::lookAt(eyePos,         // Eye
                       vec3(0, 0, 0),  // Apple
                       vec3(0, 1, 0)); // Up
    
    mat4 MVP = projection * view * model;
    program->SetUniform("MVP", MVP);
    program->SetUniform("illum", 0);
    program->SetUniform("displace", 1);
    program->SetUniform("base_color", vec3(0.0, 0.4, 0.5));
    program->SetUniform("heightField", heightField, GL_TEXTURE0);
    
    float halfSize = ARR_SIZE / 2;
    float increment = 1.0f / ARR_SIZE;
    /* for (int i = -ARR_SIZE / 2; i < ARR_SIZE / 2; i++) {
        for (int j = -ARR_SIZE / 2; j < ARR_SIZE / 2; j++) {
            glLineWidth(2.0);
            glBegin(GL_LINE_LOOP);
            glTexCoord2f((i + halfSize + 1) * increment, (j + halfSize + 1) * increment);
            glVertex3f((i + 1) * increment, (j + 1) * increment, 0);
            
            glTexCoord2f((i + halfSize) * increment, (j + halfSize + 1) * increment);
            glVertex3f(i * increment, (j + 1) * increment, 0);
            
            glTexCoord2f((i + halfSize) * increment, (j + halfSize) * increment);
            glVertex3f(i * increment, j * increment, 0);
            glEnd();
            
            glBegin(GL_LINE_LOOP);
            glTexCoord2f((i + halfSize + 1) * increment, (j + halfSize + 1) * increment);
            glVertex3f((i + 1) * increment, (j + 1) * increment, 0);
            
            glTexCoord2f((i + halfSize) * increment, (j + halfSize) * increment);
            glVertex3f(i * increment, j * increment, 0);
            
            glTexCoord2f((i + halfSize + 1) * increment, (j + halfSize) * increment);
            glVertex3f((i + 1) * increment, j * increment, 0);
            glEnd();
        }
    } */
    
    // Phong shading
    program->SetUniform("illum", 1);
    glBegin(GL_TRIANGLES);
    for (int i = -ARR_SIZE / 2; i < ARR_SIZE / 2; i++) {
        for (int j = -ARR_SIZE / 2; j < ARR_SIZE / 2; j++) {
            glTexCoord2f((i + halfSize + 1) * increment, (j + halfSize + 1) * increment);
            glVertex3f((i + 1) * increment, (j + 1) * increment, 0);
            
            glTexCoord2f((i + halfSize) * increment, (j + halfSize + 1) * increment);
            glVertex3f(i * increment, (j + 1) * increment, 0);
            
            glTexCoord2f((i + halfSize) * increment, (j + halfSize) * increment);
            glVertex3f(i * increment, j * increment, 0);
            
            glTexCoord2f((i + halfSize + 1) * increment, (j + halfSize + 1) * increment);
            glVertex3f((i + 1) * increment, (j + 1) * increment, 0);
            
            glTexCoord2f((i + halfSize) * increment, (j + halfSize) * increment);
            glVertex3f(i * increment, j * increment, 0);
            
            glTexCoord2f((i + halfSize + 1) * increment, (j + halfSize) * increment);
            glVertex3f((i + 1) * increment, j * increment, 0);
        }
    }
    glEnd();
    
    glPopMatrix();
    
    program->SetUniform("displace", 0);
    
    //particle_sys->Draw(*program, projection * view, eyePos, inverse(view));
    
    program->Unuse();
    
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Resize viewport
    glViewport(0, 0, w, h);
    
    glMatrixMode(GL_PROJECTION);
    float ratio = (float)w / h;
    projection = glm::perspective(75.0f,        // Field of view
                                  ratio,        // Aspect ratio
                                  0.1f,         // Near clipping plane
                                  100.0f);      // Far clipping plane
    glMatrixMode(GL_MODELVIEW);
    
    glutPostRedisplay();
}

/* GLUT key down callback */
void keyboard_down(unsigned char key, int x, int y)
{
    switch(key) {
        case 27:    // Escape key
            exit(0);
            break;
        case 'a':   // Roll left
            roll_left = true;
            break;
        case 'd':   // Roll right
            roll_right = true;
            break;
        case 'q':
            zoom_in = true;
            break;
        case 'w':
            zoom_out = true;
            break;
        default:
            break;
    }
}

/* GLUT key up callback */
void keyboard_up(unsigned char key, int x, int y)
{
    switch(key) {
        case 'a':   // Roll left
            roll_left = false;
            break;
        case 'd':   // Roll right
            roll_right = false;
            break;
        case 'q':
            zoom_in = false;
            break;
        case 'w':
            zoom_out = false;
            break;
        default:
            break;
    }
}

/* GLUT 'special' key down callback */
void arrow_down(int key, int x, int y)
{
    switch(key) {
        case GLUT_KEY_LEFT:
            key_left = true;
            break;
        case GLUT_KEY_RIGHT:
            key_right = true;
            break;
        case GLUT_KEY_DOWN:
            key_down = true;
            break;
        case GLUT_KEY_UP:
            key_up = true;
            break;
        default:
            break;
    }
}

/* GLUT 'special' key up callback */
void arrow_up(int key, int x, int y)
{
    switch(key) {
        case GLUT_KEY_LEFT:
            key_left = false;
            break;
        case GLUT_KEY_RIGHT:
            key_right = false;
            break;
        case GLUT_KEY_DOWN:
            key_down = false;
            break;
        case GLUT_KEY_UP:
            key_up = false;
            break;
        default:
            break;
    }
}

void animate()
{
    float yaw = 0;
    float pitch = 0;
    float roll = 0;
    
    if (key_left)
        yaw = -M_PI / 180;
    if (key_right)
        yaw = M_PI / 180;
    if (key_up)
        pitch = -M_PI / 180;
    if (key_down)
        pitch = M_PI / 180;
    if (roll_left)
        roll = -M_PI / 180;
    if (roll_right)
        roll = M_PI / 180;
    
    // Rotate model
    orientation = orientation * fquat(yaw, 0.0, 1.0, 0.0);
    orientation = orientation * fquat(pitch, 1.0, 0.0, 0.0);
    orientation = orientation * fquat(roll, 0.0, 0.0, 1.0);
    orientation = glm::normalize(orientation);
    
    // Zoom
    if (zoom_in)
        eyePos -= vec3(0, 0, 0.01);
    else if (zoom_out)
        eyePos += vec3(0, 0, 0.01);
    
    ::count += 0.001;
    particle_sys->Update(::count);
    
    glutPostRedisplay();
}

int main(int argc, char * argv[])
{
    // Glut init
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(1024, 768);
    
    glutCreateWindow("Procedural terrain demo");
    glutPositionWindow(0, 0);
    
    // Register callback functions
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard_down);
    glutKeyboardUpFunc(keyboard_up);
    glutSpecialFunc(arrow_down);
    glutSpecialUpFunc(arrow_up);
    glutIdleFunc(animate);
    
    // Enable the necessary goodies
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    // glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    
    srand((unsigned int)time(NULL));
    program = new Program("Shaders/wirevertex.vert", "Shaders/wirefragment.frag");
    particle_sys = new ParticleSystem();
    particle_sys->AddFluidCluster(vec3(0, 0, 0),
                                  vec3(5.5, 0, 5.5),
                                  vec3(0.0, 0.9, 0.0));
    orientation = fquat();
    eyePos = vec3(0, 0, 1);
    mapGen();
    path = pathGen();
    
    glutMainLoop();
    
    return 0;
}

