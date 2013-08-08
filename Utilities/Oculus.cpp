#include "Oculus.h"

using namespace::std;
using namespace::OVR;

void Oculus::Init()
{
	System::Init();
    
	pManager = *DeviceManager::Create();
    
	pHMD = *pManager->EnumerateDevices<HMDDevice>().CreateDevice();
    
	if (pHMD)
    {
        InfoLoaded = pHMD->GetDeviceInfo(&Info);
        
        pSensor = *pHMD->GetSensor();
	}
	else
	{
        pSensor = *pManager->EnumerateDevices<SensorDevice>().CreateDevice();
	}
    
	if (pSensor)
	{
        FusionResult.AttachToSensor(pSensor);
	}
}

void Oculus::Clear()
{
	pSensor.Clear();
    pHMD.Clear();
	pManager.Clear();
    
	System::Destroy();
}

void Oculus::Output()
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