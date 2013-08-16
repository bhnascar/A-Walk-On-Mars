#include "Oculus.h"

using namespace::std;

namespace Oculus
{
    OVR::Ptr<OVR::DeviceManager>	pManager;
    OVR::Ptr<OVR::HMDDevice>		pHMD;
    OVR::Ptr<OVR::SensorDevice>     pSensor;
    OVR::SensorFusion               FusionResult;
    OVR::HMDInfo                    Info;
    DistortionConfig                *Distortion;
    float                           DistortionFitX, DistortionFitY;
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
        
        Distortion = new DistortionConfig(1.0f, 0.18f, 0.115f);
        DistortionFitX = -1.0f;
        DistortionFitY = 0.0f;
        
        if (pSensor)
        {
            FusionResult.AttachToSensor(pSensor);
        }
        else {
            // Default distortion for it.
            Distortion->SetCoefficients(1.0f, 0.22f, 0.24f);
            Distortion->Scale = 1.0f;
            
            // Load fake values
            Info.HResolution            = 1280;
            Info.VResolution            = 800;
            Info.HScreenSize            = 0.14976f;
            Info.VScreenSize            = Info.HScreenSize / (1280.0f / 800.0f);
            Info.InterpupillaryDistance = 0.064f;
            Info.LensSeparationDistance = 0.0635f;
            Info.EyeToScreenDistance    = 0.041f;
            Info.DistortionK[0]         = Distortion->K[0];
            Info.DistortionK[1]         = Distortion->K[1];
            Info.DistortionK[2]         = Distortion->K[2];
            Info.DistortionK[3]         = 0;
        }
    }

    void Clear()
    {
        pSensor.Clear();
        pHMD.Clear();
        pManager.Clear();
        delete Distortion;
        
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
    
    bool IsInfoLoaded()
    {
        return InfoLoaded;
    }
    
    DistortionConfig *GetDistortionConfig()
    {
        return Distortion;
    }
    
    void updateDistortionOffsetAndScale(float win_width, float win_height)
    {
        // Distortion center shift is stored separately, since it isn't affected
        // by the eye distance.
        float lensOffset        = Info.LensSeparationDistance * 0.5f;
        float lensShift         = Info.HScreenSize * 0.25f - lensOffset;
        float lensViewportShift = 4.0f * lensShift / Info.HScreenSize;
        Distortion->XCenterOffset= lensViewportShift;
        
        // Compute distortion scale from DistortionFitX & DistortionFitY.
        // Fit value of 0.0 means "no fit".
        if ((fabs(DistortionFitX) < 0.0001f) &&  (fabs(DistortionFitY) < 0.0001f))
        {
            Distortion->Scale = 1.0f;
        }
        else
        {
            // Convert fit value to distortion-centered coordinates before fit radius
            // calculation.
            float stereoAspect = 0.5f * float(win_width) / float(win_height);
            float dx           = DistortionFitX - Distortion->XCenterOffset;
            float dy           = DistortionFitY / stereoAspect;
            float fitRadius    = sqrt(dx * dx + dy * dy);
            Distortion->Scale  = Distortion->DistortionFn(fitRadius) / fitRadius;
        }
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