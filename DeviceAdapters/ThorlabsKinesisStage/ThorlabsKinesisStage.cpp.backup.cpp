#include "ThorlabsKinesisStage.h"
// #include <Thorlabs.MotionControl.IntegratedStepperMotors.h>


// Property labels, mostly copied from ThorlabsAPTStage
const char* g_PositionProp = "Position";
const char* g_SerialNumberProp = "Serial Number";
const char* g_ChannelProp = "Channel";
const char* g_MaxVelProp = "Maximum Velocity";
const char* g_MaxAccnProp = "Maximum Acceleration";
const char* g_MinPosProp = "Position Lower Limit (um)";
const char* g_MaxPosProp = "Position Upper Limit (um)";
const char* g_StepsPerRev = "Steps per Revolution";
const char* g_gearBoxRatio = "Gear Box Ratio";
const char* g_pitch = "Thread Pitch, mm/rev";
const char* g_stepSizeUm = "Step Size, microns";

const char* g_KinesisStageName_ = "ThorlabsKinesisStage";


using namespace std;

//////////////////////////////////////////////////////////////////////////////////
// Exported MMDevice API
//////////////////////////////////////////////////////////////////////////////////

MODULE_API void InitializeModuleData()
{
	RegisterDevice(g_KinesisStageName_, MM::StageDevice, "Single-axis Kinesis interface");
}                                                            

MODULE_API MM::Device* CreateDevice(const char* deviceName)                  
{
	if (strcmp(deviceName, g_KinesisStageName_) == 0)
	{ 
		ThorlabsKinesisStage* newDev = new ThorlabsKinesisStage();
		return newDev;
	}
	else
	{	
		return 0;
	}
}

MODULE_API void DeleteDevice(MM::Device* pDevice)
{
	delete pDevice;
}


ThorlabsKinesisStage::ThorlabsKinesisStage(void) :
	stepSizeUm_(1),
	serialNumber_(""),
	initialized_(false),
	accel_(1),
	maxVel_(1),
	stepsPerRev_(666),
	gearBoxRatio_(777),
	pitch_(999)
{
	InitializeDefaultErrorMessages();

	this->LogMessage("ThorlabsKinesisStage Instantiated \n", true);

	// Set Pre-init properties of the adapter

	// Name
	CreateStringProperty(MM::g_Keyword_Name, g_KinesisStageName_, true);
	
	// Description
	CreateStringProperty(MM::g_Keyword_Description, "Thorlabs Kinesis Stage", true);

	// Serial Number:

	// Get a list of available devices
	short retval = TLI_BuildDeviceList();
	short n = TLI_GetDeviceListSize();
	
	ostringstream oss;
	oss << retval << " build return code. " << n << " devices available.";
	this->LogMessage(oss.str(), true);

	char serialNos[100];
	TLI_GetDeviceListByTypeExt(serialNos, 100, 27); // we are only capturing K-cube DC servo devices
	char *next_token1 = NULL;
	char *p = strtok_s(serialNos, ",", &next_token1);
	for (int i = 0; i < n && n < 64; i++)
	{
		// Get and parse this device's serial number and description
		TLI_DeviceInfo deviceInfo;
		TLI_GetDeviceInfo(p, &deviceInfo);
		char desc[65];
		strncpy_s(desc, deviceInfo.description, 64);
		desc[64] = '\0';

		char serialNo[8];
		strncpy_s(serialNo, deviceInfo.serialNo, 8);

		ostringstream oss2;
		oss2 << serialNo << " is one serial number.";
		this->LogMessage(oss2.str(), true);
		
		// Add this device to the drop-down menu, and over-write default serialNumber_
		if (i == 0)
		{
			strncpy(serialNumber_, serialNo, 8);
			this->LogMessage(serialNumber_, true);
		}
		
		AddAllowedValue(g_SerialNumberProp, serialNo);
		
		p = strtok_s(NULL, ",", &next_token1);
	}

	CPropertyAction* pAct1 = new CPropertyAction (this, &ThorlabsKinesisStage::OnSerialNumber);
	CreateStringProperty(g_SerialNumberProp, serialNumber_, false, pAct1, true);

}

ThorlabsKinesisStage::~ThorlabsKinesisStage(void)
{
	Shutdown();
}

// Device API ------------------------------------------------------------------------------


int ThorlabsKinesisStage::Initialize()
{
	if(initialized_)
		return DEVICE_OK;

	int resp;

	// Action properties

	// Serial Number can be changed.
	// CPropertyAction* snAct = new CPropertyAction( this, &ThorlabsKinesisStage::OnSerialNumber);
	// CreateProperty(g_SerialNumberProp, &serialNumber_, MM::String, false, snAct);

	// Position
	// CPropertyAction* pAct = new CPropertyAction (this, &ThorlabsKinesisStage::OnPosition);
	// CreateProperty(g_PositionProp, "", MM::Float, false, pAct);


	resp = CC_Open(serialNumber_);
	ostringstream log1;
	Sleep(1000);
	log1 << "Serial Number " << serialNumber_ << " responding with " << resp;
	this->LogMessage(log1.str(),true);
	resp = 999;
	

	// Figure out the conversion factors and set them as read-only properties
	resp = CC_GetMotorParamsExt(serialNumber_, &stepsPerRev_, &gearBoxRatio_, &pitch_);
	Sleep(1000);
	stepSizeUm_ = 100;
	// stepSizeUm_ = (double)(stepsPerRev_*gearBoxRatio_ / pitch_);
	ostringstream log2;
	log2 << resp << " is the response. Steps per rev "  << stepsPerRev_ <<  ",  stepsize is " << stepSizeUm_ << " gear box ratio is " << gearBoxRatio_ << " and pitch is " << pitch_;
	this->LogMessage(log2.str(),true);
	CreateProperty(g_StepsPerRev, CDeviceUtils::ConvertToString(stepsPerRev_), MM::Integer, true );
	CreateProperty(g_gearBoxRatio, CDeviceUtils::ConvertToString(gearBoxRatio_), MM::Float, true);
	CreateProperty(g_pitch, CDeviceUtils::ConvertToString(pitch_), MM::Float, true);
	// CreateProperty(g_stepSizeUm, CDeviceUtils::ConvertToString(stepSizeUm_), MM::Float, true);

	initialized_ = true;
	return DEVICE_OK;
}

int ThorlabsKinesisStage::Shutdown()
{
	if(initialized_)
	{
		initialized_ = false;
	}
	CC_Close(serialNumber_);
	return DEVICE_OK;
}

void ThorlabsKinesisStage::GetName(char* Name) const
{
	CDeviceUtils::CopyLimitedString(Name, g_KinesisStageName_);
}

bool ThorlabsKinesisStage::Busy()
{
	// I hope this is ok?
	return false;
}

// Stage API ------------------------------------------------------------------------------

int ThorlabsKinesisStage::SetPositionSteps(long steps)
{
	return OnStagePositionChanged(steps);
}

int ThorlabsKinesisStage::SetPositionUm(double newPosUm)
{
	long newSteps = newPosUm/stepSizeUm_;
	return SetPositionSteps(newSteps);
}

int ThorlabsKinesisStage::GetPositionSteps(long& steps)
{
	// Get the current position in steps, from the device. 
	CC_RequestPosition(serialNumber_);
	Sleep(100);
	long curPosSteps = CC_GetPosition(serialNumber_);

	return DEVICE_OK;
}

int ThorlabsKinesisStage::GetPositionUm(double& posUm)
{
	long curPosSteps;
	GetPositionSteps(curPosSteps);
	posUm = (double)curPosSteps*stepSizeUm_;
	return DEVICE_OK;
}

int ThorlabsKinesisStage::GetLimits(double&lower, double& upper)
{
	lower = lowerLimit_;
	upper = upperLimit_;
	return DEVICE_OK;
}


// Action Handlers ------------------------------------------------------------------------------

// int ThorlabsKinesisStage::OnPosition(MM::PropertyBase* pProp, MM::ActionType eAct)
// {
// 	// Essentially copied from ThorlabsAPTStage
// 	if (eAct == MM::BeforeGet)
// 	{
// 		double pos;
// 		int ret = GetPositionUm(pos);
// 		if (ret != DEVICE_OK)
// 			return ret;

// 		pProp->Set(pos);
// 	}
// 	else if (eAct == MM::AfterSet)
// 	{
// 		double pos;
// 		pProp->Get(pos);
// 		int ret = SetPositionUm(pos);
// 		if (ret != DEVICE_OK)
// 			return ret;
	
// 		posUm_ = pos;
// 	}

//    return DEVICE_OK;
// }

int ThorlabsKinesisStage::OnVelocity(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	// Essentially copied from ThorlabsAPTStage
	if (eAct == MM::BeforeGet)
	{
		long vel = 0; // default value
		int ret = GetVelParam(vel);
		if (ret != DEVICE_OK)
			return ret;

		pProp->Set(vel);
	}
	else if (eAct == MM::AfterSet)
	{
		long vel;
		pProp->Get(vel);
		int ret = SetVelParam(vel);
		if (ret != DEVICE_OK)
			return ret;
	}

   return DEVICE_OK;
}

int ThorlabsKinesisStage::OnAccel(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	// Essentially copied from ThorlabsAPTStage
	if (eAct == MM::BeforeGet)
	{
		long accel = 0; //default value
		int ret = GetAccelParam(accel);
		if (ret != DEVICE_OK)
			return ret;

		pProp->Set(accel);
	}
	else if (eAct == MM::AfterSet)
	{
		long accel;
		pProp->Get(accel);
		int ret = SetAccelParam(accel);
		if (ret != DEVICE_OK)
			return ret;
	}

   return DEVICE_OK;
}


int ThorlabsKinesisStage::OnSerialNumber(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	// Essentially copied from ThorlabsAPTStage
	if (eAct == MM::BeforeGet)
	{
		this->LogMessage("Setting the serial number", true);
		pProp->Set(serialNumber_);
		this->LogMessage(serialNumber_);
	}
	else if (eAct == MM::AfterSet)
	{
		// long newSerialNumber;
		// pProp->Get(newSerialNumber);
		// // convert this to a char array and put it.
		// sprintf(serialNumber_,"%d",newSerialNumber);
		string newSerialNumber;
		pProp->Get(newSerialNumber);
		strncpy(serialNumber_, newSerialNumber.c_str(), 8);		
	}

	// myMsg << "Serial Number set to " << serialNumber_ "\n";
	// this->LogMessage(myMsg.str().c_str(), true);

	return DEVICE_OK;
}

// Private methods ------------------------------------------------------------------------------


int ThorlabsKinesisStage::OnStagePositionChanged(long totalSteps)
{
	// this->LogMessage("Stage Position Changed", true);
	// CC_RequestPosition(serialNumber_);
	// Sleep(100);
	// long curPos = CC_GetPosition(serialNumber_);
	// ostringstream posStr;
	// posStr << curPos << " is the current position.";
	// this->LogMessage(posStr.str(), true);

	// int newPosition = floor(float(posUm)/1000); // in mm
	CC_SetMoveAbsolutePosition(serialNumber_, totalSteps);
	CC_MoveAbsolute(serialNumber_);
	return DEVICE_OK;
}

int ThorlabsKinesisStage::GetVelParam(long vel)
{
	int tempVel = 0;
	int tempAccel;
	// CC_GetVelParams(serialStr(), &tempVel, &tempAccel);
	vel = (long)tempVel;
	return DEVICE_OK;
}

int ThorlabsKinesisStage::SetVelParam(long vel)
{
	// CC_SetVelParams(serialStr(), vel, accel_);
	maxVel_ = vel;
	return DEVICE_OK;
}

int ThorlabsKinesisStage::GetAccelParam(long accel)
{
	int tempVel;
	int tempAccel = 0;
	// CC_GetVelParams(serialStr(), &tempVel, &tempAccel);
	accel = (long)tempAccel;
	return DEVICE_OK;
}

int ThorlabsKinesisStage::SetAccelParam(long accel)
{
	// CC_SetVelParams(serialStr(), maxVel_, accel);
	accel_ = accel;
	return DEVICE_OK;
}