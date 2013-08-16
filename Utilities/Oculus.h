#pragma once

#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include "../LibOVR/Include/OVR.h"

//-----------------------------------------------------------------------------------
// ***** DistortionConfig

// DistortionConfig Provides controls for the distortion shader.
//  - K[0] - K[3] are coefficients for the distortion function.
//  - XCenterOffset is the offset of lens distortion center from the
//    center of one-eye screen half. [-1, 1] Range.
//  - Scale is a factor of how much larger will the input image be,
//    with a factor of 1.0f being no scaling. An inverse of this
//    value is applied to sampled UV coordinates (1/Scale).
//  - ChromaticAberration is an array of parameters for controlling
//    additional Red and Blue scaling in order to reduce chromatic aberration
//    caused by the Rift lenses.
class DistortionConfig
{
public:
    DistortionConfig(float k0 = 1.0f, float k1 = 0.0f, float k2 = 0.0f, float k3 = 0.0f)
    : XCenterOffset(0), YCenterOffset(0), Scale(1.0f)
    {
        SetCoefficients(k0, k1, k2, k3);
        SetChromaticAberration();
    }
    
    void SetCoefficients(float k0, float k1 = 0.0f, float k2 = 0.0f, float k3 = 0.0f)
    { K[0] = k0; K[1] = k1;  K[2] = k2; K[3] = k3; }
    
    void SetChromaticAberration(float red1 = 1.0f, float red2 = 0.0f, float blue1 = 1.0f, float blue2 = 0.0f)
    { ChromaticAberration[0] = red1; ChromaticAberration[1] = red2; ChromaticAberration[2] = blue1; ChromaticAberration[3] = blue2; }
    
    
    // DistortionFn applies distortion equation to the argument. The returned
    // value should match distortion equation used in shader.
    float  DistortionFn(float r) const
    {
        float rsq   = r * r;
        float scale = r * (K[0] + K[1] * rsq + K[2] * rsq * rsq + K[3] * rsq * rsq * rsq);
        return scale;
    }
    
    // DistortionFnInverse computes the inverse of the distortion function on an argument.
    float DistortionFnInverse(float r);
    
    float   K[4];
    float   XCenterOffset, YCenterOffset;
    float   Scale;
    
    float   ChromaticAberration[4]; // Additional per-channel scaling is applied after distortion:
    // Index [0] - Red channel constant coefficient.
    // Index [1] - Red channel r^2 coefficient.
    // Index [2] - Blue channel constant coefficient.
    // Index [3] - Blue channel r^2 coefficient.
};


namespace Oculus
{
    void Init();
    void Clear();
    void Output();
    bool IsInfoLoaded();
    
    DistortionConfig *GetDistortionConfig();
    void updateDistortionOffsetAndScale(float win_width, float win_height);
    glm::quat GetOrientation();
    float GetScreenWidth();
    float GetScreenHeight();
    float GetLensSeparationDistance();
    float GetHorizontalResolution();
    float GetVerticalResolution();
    float GetEyeToScreenDistance();
    float GetInterpupillaryDistance();
};