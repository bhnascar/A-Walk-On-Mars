#ifdef __APPLE__
#include <GLUT/glut.h>
#include <ApplicationServices/ApplicationServices.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#endif

#include "Program.h"
#include "Texture.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "bitmap_image.hpp"

/* Must be a power of 2 */
#define ARR_SIZE 1024
#define FEATURE_SIZE 1

using namespace::glm;
using namespace::std;

static Program *program;
static Texture *heightField;
static Texture *noiseField;
static Texture *cracks;

static int win_width;
static int win_height;

static mat4 projection;
static mat4 view;

static vec3 eyePos;
static vec3 eyeDir;
static vec3 eyeLeft;
static vec3 eyeUp;
static quat eyeOrientation;

static float *path;
static float count;

static bitmap_image *image;

static fquat orientation;
bool mforward, mleft, mright, mbackward;
bool aleft, aright, aup, adown;
float theta, phi;

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
        map[i] *= 0.1;
        map[i] += 0.6;
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
    
    noiseField = new Texture(ARR_SIZE, ARR_SIZE, GL_LUMINANCE, map);
    cracks = new Texture("rock.bmp");
    heightField = new Texture("mars.bmp");
    
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
    
    // logPath(path);
    
    return path;
}

void display() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    program->Use();
    
    glPushMatrix();
    
    mat4 model = mat4(1) * glm::mat4_cast(orientation);
    view = glm::lookAt(eyePos,           // Eye
                       eyePos + eyeDir,  // Apple
                       eyeUp);           // Up
    
    mat4 MVP = projection * view * model;
    program->SetUniform("MVP", MVP);
    program->SetUniform("illum", 0);
    program->SetUniform("displace", 1);
    program->SetUniform("textured", 1);
    program->SetUniform("base_color", vec3(1.00, 0.55, 0.0));
    program->SetUniform("heightField", heightField, GL_TEXTURE0);
    program->SetUniform("noiseField", noiseField, GL_TEXTURE1);
    program->SetUniform("cracks", cracks, GL_TEXTURE2);
    
    float detailLevel = 600.0f;
    float halfSize = detailLevel / 2.0f;
    float increment = 1.0f / detailLevel;
    
    // Heavily tesselated square
    program->SetUniform("illum", 1);
    glBegin(GL_TRIANGLES);
    for (int i = -halfSize; i < halfSize; i++) {
        for (int j = -halfSize; j < halfSize; j++) {
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
    
    program->SetUniform("MVP", MVP);
    program->SetUniform("textured", 0);
    program->SetUniform("displace", 1);
    program->SetUniform("base_color", vec3(1.0, 0.75, 0.30));
    
    glutSolidSphere(1.0, 10, 10);
    
    program->Unuse();
    
    glPopMatrix();
    
    glutSwapBuffers();
}

void reshape(int w, int h) {
    win_width = w;
    win_height = h;
    
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Resize viewport
    glViewport(0, 0, w, h);
    
    glMatrixMode(GL_PROJECTION);
    float ratio = (float)w / h;
    projection = glm::perspective(75.0f,        // Field of view
                                  ratio,        // Aspect ratio
                                  0.001f,         // Near clipping plane
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
        case 'a':
            mleft = true;
            break;
        case 'd':
            mright = true;
            break;
        case 's':
            mbackward = true;
            break;
        case 'w':
            mforward = true;
            break;
        default:
            break;
    }
}

/* GLUT key up callback */
void keyboard_up(unsigned char key, int x, int y)
{
    switch(key) {
        case 'a':
            mleft = false;
            break;
        case 'd':
            mright = false;
            break;
        case 's':
            mbackward = false;
            break;
        case 'w':
            mforward = false;
            break;
        default:
            break;
    }
}

/* GLUT arrow key down callback */
void arrow_down(int key, int x, int y)
{
    switch(key) {
        case GLUT_KEY_DOWN:
            adown = true;
            break;
        case GLUT_KEY_UP:
            aup = true;
            break;
        case GLUT_KEY_LEFT:
            aleft = true;
            break;
        case GLUT_KEY_RIGHT:
            aright = true;
            break;
        default:
            break;
    }
}

/* GLUT arrow key up callback */
void arrow_up(int key, int x, int y)
{
    switch(key) {
        case GLUT_KEY_DOWN:
            adown = false;
            break;
        case GLUT_KEY_UP:
            aup = false;
            break;
        case GLUT_KEY_LEFT:
            aleft = false;
            break;
        case GLUT_KEY_RIGHT:
            aright = false;
            break;
        default:
            break;
    }
}


void mouse(int x, int y)
{
    glutWarpPointer(win_width / 2, win_height / 2);
    
    // Compute new orientation
    theta += 0.005 * (win_width / 2 - x);
    phi   += 0.005 * (win_height / 2 - y);
    
    // Clamp up-down
    if (phi > M_PI / 2)
        phi = M_PI / 2;
    else if (phi < -M_PI / 2)
        phi = -M_PI / 2;
}

/* x,y from -0.5 to 0.5 */
float fetchZ(float x, float y)
{
    // Convert from (-0.5, 0.5) to (0, 1)
    x += 0.5;
    y += 0.5;
    
    // Convert to pixels (0 - 600)
    x *= image->width();
    y *= image->height();
    unsigned int xLoc = floorf(x);
    unsigned int yLoc = floorf(y);
    unsigned int xNext = ceilf(x);
    unsigned int yNext = ceilf(y);
    xLoc %= (int)image->width();
    yLoc %= (int)image->height();
    xNext %= (int)image->width();
    yNext %= (int)image->height();
    
    // Flip
    yLoc = image->height() - yLoc;
    yNext = image->height() - yNext;
    
    // Get past height
    unsigned char r = 0, g = 0, b = 0;
    image->get_pixel(xLoc, yLoc, r, g, b);
    float height = (r + g + b) / 765.0f;
    
    // Get next height
    image->get_pixel(xNext, yNext, r, g, b);
    float nextHeight = (r + g + b) / 765.0f;
    
    // Interpolate between
    float gap = (nextHeight - height);
    float full_dist = sqrt(pow(xNext - xLoc, 2) + pow(yNext - yLoc, 2));
    float my_dist = sqrt(pow(x - xLoc, 2) + pow(y - yLoc, 2));
    float finalHeight = height + gap * (my_dist / full_dist);
    
    return 0.05 * finalHeight + 0.005;
}

void animate()
{
    float speed = 0.0005f;
    
    if (mforward)
        eyePos += speed * eyeDir;
    if (mbackward)
        eyePos -= speed * eyeDir;
    if (mleft)
        eyePos += speed * eyeLeft;
    if (mright)
        eyePos -= speed * eyeLeft;
    
    if (aup)
        eyePos.z -= 0.01;
    if (adown)
        eyePos.z += 0.01;
    
    eyePos.z = fetchZ(eyePos.x, eyePos.y);
    
    // Compute view vectors
    quat orientation = eyeOrientation * fquat(vec3(phi, 0, theta));
    eyeDir = orientation * vec3(0, 1, 0);
    eyeUp = orientation * vec3(0, 0, 1);
    eyeLeft = orientation * vec3(-1, 0, 0);
    
    ::count += 0.001;
    
    glutPostRedisplay();
}

int main(int argc, char * argv[])
{
    // Glut init
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
    //glutInitWindowSize(600, 600);
    glutInitWindowSize(1280, 880);
    
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
    glutPassiveMotionFunc(mouse);
    
    // Enable the necessary goodies
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    
#ifdef __APPLE__
	CGSetLocalEventsSuppressionInterval(0.0);
#endif
    
    srand((unsigned int)time(NULL));
    program = new Program("Shaders/wirevertex.vert", "Shaders/wirefragment.frag");

    eyeOrientation = fquat();
    eyePos = vec3(0, 0, 0);
    eyeUp = vec3(0, 0, 1);
    eyeDir = vec3(0, 1, 0);
    eyeLeft = vec3(-1, 0, 0);
    
    orientation = fquat();
    
    image = new bitmap_image("mars.bmp");
    
    mapGen();
    path = pathGen();
    
    glutMainLoop();
    
    return 0;
}

