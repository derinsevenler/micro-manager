#include "ThorlabsKinesisStage.h"
#include <bitset>
// #include <Thorlabs.MotionControl.IntegratedStepperMotors.h>


// Property labels, mostly copied from ThorlabsAPTStage
const char* g_PositionProp = "Current Position (microns)";
const char* g_SerialNumberProp = "Serial Number";
const char* g_ChannelProp = "Channel";
const char* g_MaxVelProp = "Maximum Velocity";
const char* g_MaxAccnProp = "Maximum Acceleration";
const char* g_MinPosProp = "Position Lower Limit (um)";
const char* g_MaxPosProp = "Position Upper Limit (um)";
const char* g_StepsPerRev = "Steps per Revolution";
const char* g_gearBoxRatio = "Gear Box Ratio";
const char* g_pitch = "Thread Pitch (microns per rev)";
const char* g_stepSizeUm = "step size (microns)";

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
	stepsPerRev_(512),
	gearBoxRatio_(67),
	pitch_(1000),
	initialized_(false),
	accel_(1),
	maxVel_(1)
{
	InitializeDefaultErrorMessages();

	this->LogMessage("ThorlabsKinesisStage Instantiated \n", true);

	// Set Pre-init properties of the adapter

	// Name
	CreateProperty(MM::g_Keyword_Name, g_KinesisStageName_, MM::String, true);
	
	// Description
	CreateProperty(MM::g_Keyword_Description, "Thorlabs Kinesis Stage", MM::String, true);

	// Serial Number:

	// Get a list of available devices
	short retval = TLI_BuildDeviceList();
	short n = TLI_GetDeviceListSize();

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

		// serialNo has type c-string.
		char serialNo[9];
		strncpy_s(serialNo, deviceInfo.serialNo, 8);
		serialNo[8] = '\0';
		
		// Add this device to the drop-down menu, creating the property if this is the first one.
		if (i == 0)
		{
			serialNumber_ = string(serialNo);
			CPropertyAction* pAct1 = new CPropertyAction (this, &ThorlabsKinesisStage::OnSerialNumber);
			CreateProperty(g_SerialNumberProp, serialNumber_.c_str(), MM::String, false, pAct1, true);
		}
		AddAllowedValue(g_SerialNumberProp, serialNo);

		p = strtok_s(NULL, ",", &next_token1);

	}

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
	// Action properties

	// Serial Number can be changed.
	CPropertyAction* snAct = new CPropertyAction( this, &ThorlabsKinesisStage::OnSerialNumber);
	CreateProperty(g_SerialNumberProp, serialNumber_.c_str(), MM::String, false, snAct);

	// Position
	// CPropertyAction* pAct = new CPropertyAction (this, &ThorlabsKinesisStage::OnPosition);
	// CreateProperty(g_PositionProp, "", MM::Float, false, pAct);


	int resp = CC_Open(serialNumber_.c_str());
	// ostringstream log1;
	// log1 << "Serial Number " << serialNumber_ << " responding to Open with " << resp;
	// this->LogMessage(log1.str(),true);
	// Sleep(1000);

	// Figure out the conversion factors and set them as read-only properties
	// Maybe we have to set them ourselves first?

	int resp3 = CC_SetMotorParamsExt(serialNumber_.c_str(), stepsPerRev_, gearBoxRatio_, pitch_);
	// ostringstream log3;
	// log3 << "CC_SetMotorParamsExt responds with " << resp3 << " and we now have stepsPerRev_" << stepsPerRev_;
	// this->LogMessage(log3.str(), true);
	// Sleep(1000);
	int resp2 = CC_GetMotorParamsExt(serialNumber_.c_str(), &stepsPerRev_, &gearBoxRatio_, &pitch_);
	// Sleep(1000);
	stepSizeUm_ = (double)( pitch_ / (stepsPerRev_*gearBoxRatio_) );
	
	// ostringstream log2;
	// log2 << "GetMotorParams is responding with " << resp2 << " where the serial number is " << serialNumber_ << " steps per rev is " << stepsPerRev_ << " gear box ratio is " << gearBoxRatio_ << " and pitch is " << pitch_ << " and the step size in microns is " << stepSizeUm_;;
	// this->LogMessage(log2.str(),true);
	CreateProperty(g_StepsPerRev, CDeviceUtils::ConvertToString(stepsPerRev_), MM::Integer, true );
	CreateProperty(g_gearBoxRatio, CDeviceUtils::ConvertToString(gearBoxRatio_), MM::Float, true);
	CreateProperty(g_pitch, CDeviceUtils::ConvertToString(pitch_), MM::Float, true);
	CreateProperty(g_stepSizeUm, CDeviceUtils::ConvertToString(stepSizeUm_), MM::Float, true);
	CreateProperty(g_PositionProp, CDeviceUtils::ConvertToString(posUm_), MM::Float, true);

	initialized_ = true;
	return DEVICE_OK;
}

int ThorlabsKinesisStage::Shutdown()
{
	if(initialized_)
	{
		initialized_ = false;
	}
	CC_Close(serialNumber_.c_str());
	return DEVICE_OK;
}

void ThorlabsKinesisStage::GetName(char* Name) const
{
	CDeviceUtils::CopyLimitedString(Name, g_KinesisStageName_);
}

bool ThorlabsKinesisStage::Busy()
{

	int resp = CC_RequestStatusBits(serialNumber_.c_str());
	Sleep(200);
	DWORD status = CC_GetStatusBits(serialNumber_.c_str());
	unsigned short isMovingCW = (status & 0x00000010);
	unsigned short isMovingCCW = (status & 0x00000020);
	bool isBusy = (bool)( isMovingCW || isMovingCCW);

	// ostringstream log1;
	// log1 << "Request for Status bits response was " << resp << "  and Status bit is  " << status << " and CW is " << isMovingCW << " and CCW is " << isMovingCCW << " and isBusy " << isBusy;
	// this->LogMessage(log1.str(), true);
	return (bool)(isBusy);
}

// Stage API ------------------------------------------------------------------------------

int ThorlabsKinesisStage::SetPositionSteps(long steps)
{
	ostringstream log1;
	log1 << "SetPosition to total steps of " << steps;
	this->LogMessage(log1.str(), true);
	return OnStagePositionChanged(steps);
}

int ThorlabsKinesisStage::SetPositionUm(double newPosUm)
{
	posUm_ = newPosUm;
	ostringstream log1;
	log1 << "SetPosition to total microns of " << newPosUm;
	this->LogMessage(log1.str(), true);
	long newSteps = newPosUm/stepSizeUm_;
	return SetPositionSteps(newSteps);
}

int ThorlabsKinesisStage::GetPositionSteps(long& steps)
{
	// Get the current position in steps, from the device. 
	CC_RequestPosition(serialNumber_.c_str());
	steps = CC_GetPosition(serialNumber_.c_str());

	// ostringstream log1;
	// log1 << "Getting Position in total steps, it is  " << steps;
	// this->LogMessage(log1.str(), true);

	return DEVICE_OK;
}

int ThorlabsKinesisStage::GetPositionUm(double& posUm)
{
	long curPosSteps;
	GetPositionSteps(curPosSteps);
	posUm = (double)curPosSteps*stepSizeUm_;
	ostringstream log1;
	log1 << "Getting Position in microns, it is  " << posUm;
	this->LogMessage(log1.str(), true);
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
		pProp->Set(serialNumber_.c_str());
	}
	else if (eAct == MM::AfterSet)
	{

		string newSerialNumber;
		pProp->Get(newSerialNumber);
		serialNumber_ = newSerialNumber;
	}

	// myMsg << "Serial Number set to " << serialNumber_ "\n";
	// this->LogMessage(myMsg.str().c_str(), true);

	return DEVICE_OK;
}

// Private methods ------------------------------------------------------------------------------


int ThorlabsKinesisStage::OnStagePositionChanged(long totalSteps)
{
	ostringstream posStr;
	posStr << "Stage Position with serial number " << serialNumber_ << " changed to in steps " << totalSteps ;
	this->LogMessage(posStr.str(), true);

	// int newPosition = floor(float(posUm)/1000); // in mm
	CC_SetMoveAbsolutePosition(serialNumber_.c_str(), totalSteps);
	CC_MoveAbsolute(serialNumber_.c_str());
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