#ifdef __APPLE__
#include <GLUT/glut.h>
#include <ApplicationServices/ApplicationServices.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#endif

#include "../Utilities/Oculus.h"
#include "../Utilities/Program.h"
#include "../Utilities/FBO.h"
#include "../Utilities/Texture.h"
#include "../Utilities/Noise.h"
#include "../Utilities/OBJFile.h"
#include "../Utilities/Model.h"
#include "../Utilities/Screen.h"
#include "../Utilities/bitmap_image.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

/* Window */
#define DEFAULT_WIN_WIDTH 1280
#define DEFAULT_WIN_HEIGHT 880

/* Navigation */
#define WALKING_SPEED 0.0002f
#define LOOKING_SPEED 0.005f
#define WALKING_HEIGHT 0.005f

/* Unit conversion */
#define METER_TO_WORLD_UNITS 0.00000000472012046

using namespace::glm;
using namespace::std;
using namespace::OVR::Util::Render;

static int win_width;
static int win_height;

/* Shader variables */
static Program *mainShader;
static Program *distortionShader;
static Program *screenQuadShader;

static FBO *frameBuffer;

static Texture *sceneTexture;
static Texture *depthTexture;
static Texture *heightField;
static Texture *normalMap;
static Texture *noiseField;
static Texture *rock;
static Texture *sand;

/* OpenGL MVP variables */
static mat4 projection;
static mat4 leftProjection;
static mat4 rightProjection;

static mat4 view;
static mat4 leftView;
static mat4 rightView;
static mat4 distortionView;
static mat4 TM;

/* OpenGL camera variables */
static vec3 lightPos;
static vec3 eyePos;
static vec3 eyeDir;
static vec3 eyeLeft;
static vec3 eyeUp;
static quat eyeOrientation;

float theta, phi;

/* Oculus Rift variables */
static vec2 lensCenter;
static vec2 screenCenter;
static vec2 scaleIn;
static vec2 scaleOut;
static vec4 hmdWarpParm;
static vec4 chromAbParam;

/* Movement variables */
bool mforward, mleft, mright, mbackward;

Model *grid;
Model *sphere;
Screen *screen;

void render()
{
    mainShader->Use();
    mainShader->Reset();
    
    mat4 model = mat4(1);
    mat4 MVP = projection * view * model;
    mainShader->SetUniform("MVP", MVP);
    mainShader->SetUniform("model", model);
    
    // Set up lighting variables
    mainShader->SetUniform("illum", 1);
    mainShader->SetUniform("attenuate", 1);
    mainShader->SetUniform("lightPosition", lightPos);
    mainShader->SetUniform("baseColor", vec3(1.00, 0.55, 0.0));
    
    // Set up texturing variables
    mainShader->SetUniform("displace", 1);
    mainShader->SetUniform("heightMap", heightField, GL_TEXTURE0);
    mainShader->SetUniform("normalMap", normalMap, GL_TEXTURE1);
    mainShader->SetUniform("textured", 1);
    mainShader->SetUniform("texture", noiseField, GL_TEXTURE4);
    
    // Set up bump mapping variables
    mainShader->SetUniform ("bumpMapped", 1);
    mainShader->SetUniform("sand", sand, GL_TEXTURE2);
    mainShader->SetUniform("rock", rock, GL_TEXTURE3);
    
    // Draw landscape
    grid->Draw(*mainShader);
    
    // Set up lighting variables
    mainShader->SetUniform("illum", 1);
    mainShader->SetUniform("lightPosition", lightPos);
    mainShader->SetUniform("baseColor", vec3(1.0, 0.80, 0.50));
    
    // Set up texturing variables
    mainShader->SetUniform("attenuate", 1);
    mainShader->SetUniform("displace", 0);
    mainShader->SetUniform("textured", 0);
    mainShader->SetUniform("bumpMapped", 0);
    
    // Draw sky sphere
    sphere->Draw(*mainShader);
    
    mainShader->Unuse();
}

void updateView()
{
    // Centered view matrix
    view = glm::lookAt(eyePos,           // Eye
                       eyePos + eyeDir,  // Apple
                       eyeUp);           // Up
    
    // View transformation translation in world units.
    if (Oculus::IsInfoLoaded()) {
        float halfIPD  = Oculus::GetInterpupillaryDistance() * METER_TO_WORLD_UNITS;
        leftView = glm::translate(mat4(1), vec3(halfIPD, 0, 0)) * view;
        rightView = glm::translate(mat4(1), vec3(-halfIPD, 0, 0)) * view;
    }
    else {
        leftView = view;
        rightView = view;
    }
}

void updateDistortion(Viewport& VP)
{
    const DistortionConfig& config = Oculus::GetDistortionConfig();
    
    float w = float(VP.w) / win_width;
    float h = float(VP.h) / win_height;
    float x = float(VP.x) / win_width;
    float y = float(VP.y) / win_height;
    
    float as = float(VP.w) / float(VP.h);
    
    // We are using 1/4 of DistortionCenter offset value here, since it is
    // relative to [-1,1] range that gets mapped to [0, 0.5].
    if (x > 0)
        lensCenter = vec2(x + (w - config.XCenterOffset * 0.5f) * 0.5f, y + h * 0.5f);
    else
        lensCenter = vec2(x + (w + config.XCenterOffset * 0.5f) * 0.5f, y + h * 0.5f);
    screenCenter = vec2(x + w * 0.5f, y + h * 0.5f);
    
    // MA: This is more correct but we would need higher-res texture vertically; we should adopt this
    // once we have asymmetric input texture scale.
    float scaleFactor = 1.0f / config.Scale;
    scaleOut = vec2((w / 2) * scaleFactor, (h / 2) * scaleFactor * as);
    scaleIn = vec2((2 / w), (2 / h) / as);
    hmdWarpParm = vec4(config.K[0], config.K[1], config.K[2], config.K[3]);
    
    TM = mat4(vec4(w, 0, 0, 0),
              vec4(0, h, 0, 0),
              vec4(0, 0, 0, 0),
              vec4(x, y, 0, 1));
    
    distortionView = mat4(vec4(2, 0, 0, 0),
                          vec4(0, 2, 0, 0),
                          vec4(0, 0, 0, 0),
                          vec4(-1, -1, 0, 1));
    
    // Chromatic abberation fix
    chromAbParam = vec4(config.ChromaticAberration[0],
                        config.ChromaticAberration[1],
                        config.ChromaticAberration[2],
                        config.ChromaticAberration[3]);
}

// Render from frame buffer to screen, with barrel distortion for Oculus
void barrelDistort()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    distortionShader->Use();
    distortionShader->Reset();
    
    // Render left
    glViewport(0, 0, win_width / 2, win_height);
    struct Viewport left(0, 0, win_width / 2, win_height);
    updateDistortion(left);
    distortionShader->SetUniform("view", distortionView);
    distortionShader->SetUniform("TM", TM);
    distortionShader->SetUniform("Scale", scaleOut);
    distortionShader->SetUniform("ScaleIn", scaleIn);
    distortionShader->SetUniform("LensCenter", lensCenter);
    distortionShader->SetUniform("ScreenCenter", screenCenter);
    distortionShader->SetUniform("HmdWarpParam", hmdWarpParm);
    distortionShader->SetUniform("ChromAbParam", chromAbParam);
    distortionShader->SetUniform("scene", sceneTexture, GL_TEXTURE0);
    screen->Draw(*distortionShader);
    
    // Render right
    glViewport(win_width / 2, 0, win_width / 2, win_height);
    struct Viewport right(win_width / 2, 0, win_width / 2, win_height);
    updateDistortion(right);
    distortionShader->SetUniform("view", distortionView);
    distortionShader->SetUniform("TM", TM);
    distortionShader->SetUniform("Scale", scaleOut);
    distortionShader->SetUniform("ScaleIn", scaleIn);
    distortionShader->SetUniform("LensCenter", lensCenter);
    distortionShader->SetUniform("ScreenCenter", screenCenter);
    distortionShader->SetUniform("HmdWarpParam", hmdWarpParm);
    distortionShader->SetUniform("ChromAbParam", chromAbParam);
    distortionShader->SetUniform("scene", sceneTexture, GL_TEXTURE0);
    screen->Draw(*distortionShader);
    
    distortionShader->Unuse();
}

void display()
{
    // First we fix the view matrices
    updateView();
    
    // Render to frame buffer
    frameBuffer->Use();
    frameBuffer->SetDepthTexture(depthTexture);
    frameBuffer->SetColorTexture(sceneTexture, GL_COLOR_ATTACHMENT0);
    frameBuffer->SetDrawTarget(GL_COLOR_ATTACHMENT0);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render left
    glViewport(0, 0, win_width / 2, win_height);
    projection = leftProjection;
    view = leftView;
    render();
    
    // Render right
    glViewport(win_width / 2, 0, win_width / 2, win_height);
    projection = rightProjection;
    view = rightView;
    render();
    
    frameBuffer->Unuse();
    
    // Render to screen from texture in the framebuffer,
    // with fixing for distortion and chromatic aberration
    barrelDistort();
    
    glutSwapBuffers();
}

void reshape(int w, int h)
{
    win_width = w;
    win_height = h;
    
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Resize framebuffer
    if (frameBuffer) {
        delete frameBuffer;
    }
    frameBuffer = new FBO(win_width, win_height);
    
    // Resize scene texture
    if (sceneTexture) {
        delete sceneTexture;
        delete depthTexture;
    }
    sceneTexture = new Texture(win_width, win_height, GL_RGBA);
    depthTexture = new Texture(win_width, win_height, GL_DEPTH_COMPONENT);
    
    // Resize viewport
    glViewport(0, 0, w, h);
    
    // Update projections
    glMatrixMode(GL_PROJECTION);
    
    if (Oculus::IsInfoLoaded()) {
        // Compute Aspect Ratio. Stereo mode cuts width in half.
        float ratio = (float)(Oculus::GetHorizontalResolution() * 0.5f) / Oculus::GetVerticalResolution();
        
        // Compute Vertical FOV based on distance.
        float halfScreenSize = Oculus::GetScreenHeight() / 2.0f;
        float FOV = 2.0f * atan(halfScreenSize / Oculus::GetEyeToScreenDistance());
        float FOV_degrees = FOV * 180.0 / M_PI;
        
        projection = glm::perspective(FOV_degrees,  // Field of view
                                      ratio,        // Aspect ratio
                                      0.00001f,      // Near clipping plane
                                      1000.0f);      // Far clipping plane
        
        // Post-projection viewport coordinates range from (-1.0, 1.0), with the
        // center of the left viewport falling at (1/4) of horizontal screen size.
        // We need to shift this projection center to match with the lens center.
        // We compute this shift in physical units (meters) to correct
        // for different screen sizes and then rescale to viewport coordinates.
        float viewCenter             = Oculus::GetScreenWidth() * 0.25f;
        float eyeProjectionShift     = viewCenter - Oculus::GetLensSeparationDistance() * 0.5f;
        float projectionCenterOffset = 4.0f * eyeProjectionShift / Oculus::GetScreenWidth();

        leftProjection = glm::translate(mat4(1), vec3(projectionCenterOffset, 0, 0)) * projection;
        rightProjection = glm::translate(mat4(1), vec3(-projectionCenterOffset, 0, 0)) * projection;
    }
    else {
        // Compute Aspect Ratio. Stereo mode cuts width in half.
        float ratio = (float)(win_width * 0.5f) / win_height;
        
        projection = glm::perspective(85.0f,        // Field of view
                                      ratio,        // Aspect ratio
                                      0.00001f,      // Near clipping plane
                                      1000.0f);      // Far clipping plane
        
        leftProjection = projection;
        rightProjection = projection;
    }
    
    Oculus::UpdateStereoConfig(win_width, win_height);
     
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


void mouse(int x, int y)
{
    float width = win_width, height = win_height;
    
    if (!Oculus::IsInfoLoaded()) {
        glutWarpPointer(win_width / 2, win_height / 2);
    }
    else {
        width = Oculus::GetHorizontalResolution();
        height = Oculus::GetVerticalResolution();
        glutWarpPointer(width / 2, height / 2);
    }
    
    // Compute new orientation
    theta += LOOKING_SPEED * (width / 2 - x);
    phi   += LOOKING_SPEED * (height / 2 - y);
    
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
    
    float height = image->get_interpolated_height(x, y);
    
    return 0.05 * height + WALKING_HEIGHT;
}

void animate()
{
    // Move
    if (mforward)
        eyePos += WALKING_SPEED * eyeDir;
    if (mbackward)
        eyePos -= WALKING_SPEED * eyeDir;
    if (mleft)
        eyePos += WALKING_SPEED * eyeLeft;
    if (mright)
        eyePos -= WALKING_SPEED * eyeLeft;
    eyePos.z = fetchZ(eyePos.x, eyePos.y);
    
    // Update camera (view vectors)
    eyeOrientation = normalize(fquat(vec3(0, 0, theta)));
    if (Oculus::IsInfoLoaded()) {
        eyeOrientation = normalize(eyeOrientation * Oculus::GetOrientation());
    }
    eyeUp = normalize(eyeOrientation * vec3(0, 0, 1));
    eyeDir = normalize(eyeOrientation * vec3(0, 1, 0));
    eyeLeft = normalize(eyeOrientation * vec3(-1, 0, 0));
    
    // Redraw
    glutPostRedisplay();
}

void initGlobals()
{
    // Initialize shaders
    mainShader = new Program("Shaders/main.vert", "Shaders/main.frag");
    distortionShader = new Program("Shaders/distort.vert", "Shaders/distort2.frag");
    screenQuadShader = new Program("Shaders/quad.vert", "Shaders/quad.frag");
    
    // Initialize camera, lighting
    eyeOrientation = fquat();
    eyePos = vec3(0, 0, 0);
    eyeUp = vec3(0, 0, 1);
    eyeDir = vec3(0, 1, 0);
    eyeLeft = vec3(-1, 0, 0);
    lightPos = vec3(0.0, 0.0, 1.5);
    
    // Initialie textures
    rock = new Texture("Textures/rock.bmp");
    sand = new Texture("Textures/sand.bmp");
    heightField = new Texture("Textures/mars.bmp");
    normalMap = heightField->GetNormalMap();
    noiseField = new Noise();
    
    // Load models
    OBJFile obj("Models/grid.obj");
    grid = obj.GenModel();
    
    OBJFile sph("Models/icosphere.obj");
    sphere = sph.GenModel();
    
    screen = new Screen();
}

int main(int argc, char * argv[])
{
    // Glut init
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT);
    glutCreateWindow("A Walk on Mars");
    glutPositionWindow(0, 0);
    glutFullScreen();
    
    // Register callback functions
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard_down);
    glutKeyboardUpFunc(keyboard_up);
    glutIdleFunc(animate);
    glutPassiveMotionFunc(mouse);
    
    // Enable the necessary goodies
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    
    // Oculus init
    Oculus::Init();
    Oculus::Output();
    
#ifdef __APPLE__
	CGSetLocalEventsSuppressionInterval(0.0);
#endif
    
    initGlobals();
    
    glutMainLoop();
    
    Oculus::Clear();
    
    return 0;
}

