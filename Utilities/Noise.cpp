#include "Noise.h"

/* Must be a power of 2 */
#define ARR_SIZE 1024
#define FEATURE_SIZE 1

using namespace std;

/** Random terrain generation 
 * Diamond square algorithm
 */

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
    
    return path;
}

/** Noise wrapper class begins here */

Noise::Noise()
{
    float *map = arrayGen();
    seed(map, FEATURE_SIZE);
    diamondSquare(map, FEATURE_SIZE);
    normalize(map);
    
    width = ARR_SIZE;
    height = ARR_SIZE;
    format = GL_LUMINANCE;
    glGenTextures(1, &id);
    data = map;
    bitmap = NULL;
    Bind();
}
