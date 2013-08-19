#include "Oculus.h"

using namespace::std;
using namespace::OVR::Util::Render;

namespace Oculus
{
    OVR::Ptr<OVR::DeviceManager>	pManager;
    OVR::Ptr<OVR::HMDDevice>		pHMD;
    OVR::Ptr<OVR::SensorDevice>     pSensor;
    OVR::SensorFusion               FusionResult;
    OVR::HMDInfo                    Info;
    OVR::Util::Render::StereoConfig Stereo;
    float                           RenderScale;
    bool                            InfoLoaded;
    
    void Init()
    {
        OVR::System::Init();
        
        pManager = *OVR::DeviceManager::Create();
        
        pHMD = *pManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
        
        if (pHMD)
        {
            InfoLoaded = pHMD->GetDeviceInfo(&Info);
            pSensor = *pHMD->GetSensor();
        }
        else
        {
            pSensor = *pManager->EnumerateDevices<OVR::SensorDevice>().CreateDevice();
        }
        
        if (pSensor)
        {
            FusionResult.AttachToSensor(pSensor);
        }
    }

    void Clear()
    {
        pSensor.Clear();
        pHMD.Clear();
        pManager.Clear();
        
        OVR::System::Destroy();
    }

    void Output()
    {
        cout << "----- Oculus Console -----" << endl;
        
        if (pHMD)
        {
            cout << " [x] HMD Found" << endl;
        }
        else
        {
            cout << " [ ] HMD Not Found" << endl;
        }
        
        if (pSensor)
        {
            cout << " [x] Sensor Found" << endl;
        }
        else
        {
            cout << " [ ] Sensor Not Found" << endl;
        }
        
        cout << "--------------------------" << endl;
        
        if (InfoLoaded)
        {
            cout << " DisplayDeviceName: " << Info.DisplayDeviceName << endl;
            cout << " ProductName: " << Info.ProductName << endl;
            cout << " Manufacturer: " << Info.Manufacturer << endl;
            cout << " Version: " << Info.Version << endl;
            cout << " HResolution: " << Info.HResolution<< endl;
            cout << " VResolution: " << Info.VResolution<< endl;
            cout << " HScreenSize: " << Info.HScreenSize<< endl;
            cout << " VScreenSize: " << Info.VScreenSize<< endl;
            cout << " VScreenCenter: " << Info.VScreenCenter<< endl;
            cout << " EyeToScreenDistance: " << Info.EyeToScreenDistance << endl;
            cout << " LensSeparationDistance: " << Info.LensSeparationDistance << endl;
            cout << " InterpupillaryDistance: " << Info.InterpupillaryDistance << endl;
            cout << " DistortionK[0]: " << Info.DistortionK[0] << endl;
            cout << " DistortionK[1]: " << Info.DistortionK[1] << endl;
            cout << " DistortionK[2]: " << Info.DistortionK[2] << endl;
            cout << "--------------------------" << endl;
        }
    }
    
    void UpdateStereoConfig(float win_width, float win_height)
    {
        if (pHMD) {
            // Obtain setup data from the HMD and initialize StereoConfig
            // for stereo rendering.
            pHMD->GetDeviceInfo(&Info);
            Stereo.SetFullViewport(Viewport(0,0, win_width, win_height));
            Stereo.SetStereoMode(Stereo_LeftRight_Multipass);
            Stereo.SetHMDInfo(Info);
            Stereo.SetDistortionFitPointVP(-1.0f, 0.0f);
            RenderScale = Stereo.GetDistortionScale();
        }
    }
    
    const OVR::Util::Render::DistortionConfig& GetDistortionConfig()
    {
        return Stereo.GetDistortionConfig();
    }
    
    bool IsInfoLoaded()
    {
        return InfoLoaded;
    }
    
    glm::quat GetOrientation()
    {
        OVR::Quatf f = FusionResult.GetOrientation();
        return glm::quat(f.w, f.x, -f.z, f.y);
    }
    
    float GetScreenWidth()
    {
        if (!InfoLoaded) {
            cerr << "Warning: HMD Info not loaded!" << endl;
        }
        return Info.HScreenSize;
    }
    
    float GetScreenHeight()
    {
        if (!InfoLoaded) {
            cerr << "Warning: HMD Info not loaded!" << endl;
        }
        return Info.VScreenSize;
    }
    
    float GetLensSeparationDistance()
    {
        if (!InfoLoaded) {
            cerr << "Warning: HMD Info not loaded!" << endl;
        }
        return Info.LensSeparationDistance;
    }
    
    float GetHorizontalResolution()
    {
        if (!InfoLoaded) {
            cerr << "Warning: HMD Info not loaded!" << endl;
        }
        return Info.HResolution;
    }
    
    float GetVerticalResolution()
    {
        if (!InfoLoaded) {
            cerr << "Warning: HMD Info not loaded!" << endl;
        }
        return Info.VResolution;
    }
    
    float GetEyeToScreenDistance()
    {
        if (!InfoLoaded) {
            cerr << "Warning: HMD Info not loaded!" << endl;
        }
        return Info.EyeToScreenDistance;
    }
    
    float GetInterpupillaryDistance()
    {
        if (!InfoLoaded) {
            cerr << "Warning: HMD Info not loaded!" << endl;
        }
        return Info.InterpupillaryDistance;
    }
}