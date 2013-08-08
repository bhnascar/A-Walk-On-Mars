#ifdef __APPLE__
#include <GLUT/glut.h>
#include <ApplicationServices/ApplicationServices.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#endif

#include "../Utilities/Program.h"
#include "../Utilities/Texture.h"
#include "../Utilities/Noise.h"
#include "../Utilities/OBJFile.h"
#include "../Utilities/Model.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../Utilities/bitmap_image.hpp"

/* Window */
#define DEFAULT_WIN_WIDTH 1280
#define DEFAULT_WIN_HEIGHT 880

/* Navigation */
#define WALKING_SPEED 0.0001f
#define LOOKING_SPEED 0.005f
#define WALKING_HEIGHT 0.005f

using namespace::glm;
using namespace::std;

static Program *program;
static Texture *heightField;
static Texture *normalMap;
static Texture *noiseField;
static Texture *sand;

static int win_width;
static int win_height;

static mat4 projection;
static mat4 view;

static vec3 lightPos;
static vec3 eyePos;
static vec3 eyeDir;
static vec3 eyeLeft;
static vec3 eyeUp;
static quat eyeOrientation;

bool mforward, mleft, mright, mbackward;
bool aleft, aright, aup, adown;
float theta, phi;

Model *grid;
Model *sphere;

void display() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    program->Use();
    
    mat4 model = mat4(1);
    view = glm::lookAt(eyePos,           // Eye
                       eyePos + eyeDir,  // Apple
                       eyeUp);           // Up
    
    mat4 MVP = projection * view * model;
    program->SetUniform("MVP", MVP);
    program->SetUniform("model", model);
    program->SetUniform("baseColor", vec3(1.00, 0.55, 0.0));
    
    program->SetUniform("illum", 1);
    program->SetUniform("attenuate", 1);
    program->SetUniform("lightPosition", lightPos);
    
    program->SetUniform("displace", 1);
    program->SetUniform("heightMap", heightField, GL_TEXTURE0);
    program->SetUniform("normalMap", normalMap, GL_TEXTURE1);
    program->SetUniform("sand", sand, GL_TEXTURE2);
    
    program->SetUniform("textured", 1);
    program->SetUniform("texture", noiseField, GL_TEXTURE2);
    
    grid->Draw(*program);
    
    program->SetUniform("illum", 1);
    program->SetUniform("lightPosition", lightPos);
    program->SetUniform("baseColor", vec3(1.0, 0.80, 0.50));
    
    program->SetUniform("attenuate", 1);
    program->SetUniform("displace", 0);
    program->SetUniform("textured", 0);
    
    sphere->Draw(*program);
    
    program->Unuse();
    
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
                                  0.0001f,      // Near clipping plane
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
    theta += LOOKING_SPEED * (win_width / 2 - x);
    phi   += LOOKING_SPEED * (win_height / 2 - y);
    
    // Clamp up-down
    if (phi > M_PI / 2)
        phi = M_PI / 2;
    else if (phi < -M_PI / 2)
        phi = -M_PI / 2;
}

/* x,y from -1.0 to 1.0 */
float fetchZ(float x, float y)
{
    bitmap_image *image = heightField->GetBitmap();
    
    // Convert from (-1, 1) to (0, 1)
    x += 1.0;
    y += 1.0;
    x /= 2;
    y /= 2;
    
    // Convert to pixel array indices (0 - 600)
    x *= image->width();
    y *= image->height();
    
    vec2 dir = normalize(vec2(eyeDir.x, eyeDir.y));
    float height = image->get_interpolated_height(x, y);
    
    return 0.1 * height + WALKING_HEIGHT;
}

void animate()
{
    if (mforward)
        eyePos += WALKING_SPEED * eyeDir;
    if (mbackward)
        eyePos -= WALKING_SPEED * eyeDir;
    if (mleft)
        eyePos += WALKING_SPEED * eyeLeft;
    if (mright)
        eyePos -= WALKING_SPEED * eyeLeft;
    
    if (aup)
        eyePos.z -= 0.01;
    if (adown)
        eyePos.z += 0.01;
    
    cout << "(" << eyePos.x << ", " << eyePos.y << ")" << endl;
    eyePos.z = fetchZ(eyePos.x, eyePos.y);
    
    // Compute view vectorsw
    quat orientation = normalize(eyeOrientation * fquat(vec3(phi, 0, theta)));
    eyeUp = normalize(orientation * vec3(0, 0, 1));
    eyeDir = normalize(orientation * vec3(0, 1, 0));
    eyeLeft = normalize(orientation * vec3(-1, 0, 0));
    
    glutPostRedisplay();
}

void initGlobals()
{
    srand((unsigned int)time(NULL));
    program = new Program("Shaders/main.vert", "Shaders/main.frag");
    
    eyeOrientation = fquat();
    eyePos = vec3(0, 0, 0);
    eyeUp = vec3(0, 0, 1);
    eyeDir = vec3(0, 1, 0);
    eyeLeft = vec3(-1, 0, 0);
    lightPos = vec3(0.0, 0.0, 1.5);
    
    sand = new Texture("Textures/sand.bmp");
    heightField = new Texture("Textures/mars.bmp");
    normalMap = heightField->GetNormalMap();
    noiseField = new Noise();
    
    OBJFile obj("Models/grid.obj");
    grid = obj.GenModel();
    
    OBJFile sph("Models/icosphere.obj");
    sphere = sph.GenModel();
}

int main(int argc, char * argv[])
{
    // Glut init
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT);
    
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
    
    initGlobals();
    
    glutMainLoop();
    
    return 0;
}

