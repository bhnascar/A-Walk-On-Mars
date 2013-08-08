#pragma once

#include <iostream>
#include "../LibOVR/Include/OVR.h"

namespace Oculus
{
    OVR::Ptr<OVR::DeviceManager>	pManager;
    OVR::Ptr<OVR::HMDDevice>		pHMD;
    OVR::Ptr<OVR::SensorDevice>     pSensor;
    OVR::SensorFusion               FusionResult;
    OVR::HMDInfo                    Info;
    bool                            InfoLoaded;
    
    void Init();
    void Clear();
    void Output();
}
