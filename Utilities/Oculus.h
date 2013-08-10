#pragma once

#include <iostream>
#include <glm/gtc/quaternion.hpp>
#include "../LibOVR/Include/OVR.h"

namespace Oculus
{
    void Init();
    void Clear();
    void Output();
    bool IsInfoLoaded();
    
    glm::quat GetOrientation();
    float GetScreenWidth();
    float GetScreenHeight();
    float GetLensSeparationDistance();
    float GetHorizontalResolution();
    float GetVerticalResolution();
    float GetEyeToScreenDistance();
    float GetInterpupillaryDistance();
};
