#pragma once

#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include "../LibOVR/Include/OVR.h"
#include "Util_Render_Stereo.h"

namespace Oculus
{
    // Init should be placed into the initialization sequence, before
    // any other Oculus calls
    void Init();
    
    // Clear is a cleanup function to be placed at the end of
    // program executions and takes care of cleaning up Oculus related
    // info
    void Clear();
    
    // Outputs Oculus sensor data
    void Output();
    
    // Returns whether or not Oculus data was successfully loaded
    bool IsInfoLoaded();
    
    // StereoConfig is an internal class that computes view and
    // distortion information. It needs to know the current window
    // width and window height, so UpdateStereoConfig should be
    // called whenever the window is resized, and passed the
    // new window dimensions.
    void UpdateStereoConfig(float win_width, float win_height);
    
    // Returns the distortion information for the current view
    const OVR::Util::Render::DistortionConfig& GetDistortionConfig();
    
    glm::quat GetOrientation();
    float GetScreenWidth();
    float GetScreenHeight();
    float GetLensSeparationDistance();
    float GetHorizontalResolution();
    float GetVerticalResolution();
    float GetEyeToScreenDistance();
    float GetInterpupillaryDistance();
};