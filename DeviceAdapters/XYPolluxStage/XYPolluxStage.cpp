#include "XYPolluxStage.h"
#include <iostream>

const char* g_XYPolluxStageName_ = "XYPolluxStage";
const char* g_XYPolluxStageDescription_ = "XY Pollux Stage";

using namespace std;

XYPolluxStage::XYPolluxStage() :
	stepSizeXUm_(0.1),
	stepSizeYUm_(0.1),
	initialized_(false),
	port_("Undefined"),
	asciiTerminator_("\r\n"),
	axisX_(1),
	axisY_(2)
{

	// Set preinit properties - constants about the hardware

	// Port
	CPropertyAction *pAct = new CPropertyAction (this, &XYPolluxStage::OnPort);
	CreateProperty(MM::g_Keyword_Port, "Undefined", MM::String, false, pAct, true);

	//X axis number (it must 1-16)
	pAct = new CPropertyAction (this, &XYPolluxStage::OnAxisX);
	CreateIntegerProperty("X axis number", axisX_, false, pAct, true);
	SetPropertyLimits("X axis number", 1, 16);
	UpdateProperty("X axis number");

	//Y axis number
	pAct = new CPropertyAction (this, &XYPolluxStage::OnAxisY);
	CreateIntegerProperty("Y axis number", axisY_, false, pAct, true);
	SetPropertyLimits("Y axis number", 1, 16);

}

XYPolluxStage::~XYPolluxStage()
{
	Shutdown();
}

//////////////////////////////////////////////////////////////////////////////////
// MMDevice API
//////////////////////////////////////////////////////////////////////////////////
// This part works, don't change
MODULE_API void InitializeModuleData()
{
	RegisterDevice(g_XYPolluxStageName_, MM::XYStageDevice, g_XYPolluxStageDescription_);
}                                                            

MODULE_API MM::Device* CreateDevice(const char* deviceName)                  
{
	if (strcmp(deviceName, g_XYPolluxStageName_) == 0)
	{
		return new XYPolluxStage();
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

















////////////////////////////
// API Functions for devices
////////////////////////////

void XYPolluxStage::GetName(char* name) const
{
	CDeviceUtils::CopyLimitedString(name,g_XYPolluxStageName_);
}

int XYPolluxStage::Initialize()
{
	this->LogMessage("XYPolluxStage::Initialize\n", true);
	if (initialized_)
	{
		return DEVICE_OK;
	}

	core_ = GetCoreCallback();
	
	// create 
	CPropertyAction* pAct;

	UpdateProperty(MM::g_Keyword_Port);
		UpdateProperty("Y axis number");
	

	// Initialize Speed (in mm/s)
	pAct = new CPropertyAction (this, &XYPolluxStage::OnSpeedX);
	CreateFloatProperty("Speed X [mm/s]", 5, false, pAct);
	UpdateProperty("Speed X [mm/s]");

	pAct = new CPropertyAction (this, &XYPolluxStage::OnSpeedY);
	CreateFloatProperty("Speed Y [mm/s]", 5, false, pAct);
	UpdateProperty("Speed Y [mm/s]");

	// Initialize Acceleration (in mm/s²)
	pAct = new CPropertyAction (this, &XYPolluxStage::OnAccelX);
	CreateFloatProperty("Acceleration X [mm/s^2]", 100, false, pAct);
	UpdateProperty("Acceleration X [mm/s^2]");

	pAct = new CPropertyAction (this, &XYPolluxStage::OnAccelY);
	CreateFloatProperty("Acceleration Y [mm/s^2]", 100, false, pAct);
	UpdateProperty("Acceleration Y [mm/s^2]");

	initialized_ = true;

	this->LogMessage("XYPolluxStage::Initialization Complete\n",true);
	return DEVICE_OK;
}

int XYPolluxStage::Shutdown()
{
	this->LogMessage("XYPolluxStage::Shutdown\n",true);
	if (initialized_)
	{
		initialized_ = false;
	}
	return DEVICE_OK;

}

bool XYPolluxStage::Busy()
{
	// Ask if either stage is busy
	int isBusy;
	stringstream sstm;
	string req, firstWord;
	vector<string> reply;
	int queryOK;

	// get x
	sstm << (int)axisX_ << " nst";
	req = sstm.str();
	queryOK = queryCommand(req,reply);
	if (queryOK != DEVICE_OK)
	{
		return queryOK;
	}
	firstWord = reply[0];
	isBusy = ::atof(firstWord.c_str());
	if (isBusy == 1)
	{
		return true;
	}

	// clear sstm
	sstm.str(string());
	sstm.clear();

	// get y
	sstm << (int)axisY_ << " nst";
	req = sstm.str();
	queryOK = queryCommand(req,reply);
	if (queryOK != DEVICE_OK)
	{
		return queryOK;
	}
	firstWord = reply[0];
	isBusy = ::atof(firstWord.c_str());
	if (isBusy == 1)
	{
		return true;
	}

	return false;
}

///////////////////////////
// API Functions for stages
///////////////////////////

int XYPolluxStage::GetLimitsUm(double& xMin, double& xMax, double& yMin, double& yMax)
{
	// "Get the minimum and maximum x and y positions, in microns"
	this->LogMessage("XYStage::GetLimitsUm\n", true);

	long xMinmm, xMaxmm, yMinmm, yMaxmm;
	stringstream sstm;
	string req, firstWord, secondWord;
	vector<string> reply;
	int queryOK;

	// get x min and max
	sstm << (int)axisX_ << " getnlimit";
	req = sstm.str();
	queryOK = queryCommand(req,reply);
	if (queryOK != DEVICE_OK)
	{
		return queryOK;
	}
	firstWord = reply[0];
	secondWord= reply[1];
	xMinmm = ::atof(firstWord.c_str());	// in mm
	xMaxmm = ::atof(secondWord.c_str());
	xMin = xMinmm*1000;					// in microns
	xMax = xMaxmm*1000;

	// clear sstm
	sstm.str(string());
	sstm.clear();

	// get y min and max
	sstm << (int)axisY_ << " getnlimit";
	req = sstm.str();
	queryOK = queryCommand(req,reply);
	if (queryOK != DEVICE_OK)
	{
		return queryOK;
	}
	firstWord = reply[0];
	secondWord= reply[1];
	yMinmm = ::atof(firstWord.c_str());	// in mm
	yMaxmm = ::atof(secondWord.c_str());
	yMin = yMinmm*1000;					// in microns
	yMax = yMaxmm*1000;
	
	return DEVICE_OK;
}

int XYPolluxStage::GetStepLimits(long& xMin, long& xMax, long& yMin, long& yMax)
{
	// "get the minimum and maximum position limits, in units of 'step'"

	// get the limits in microns
	double xMinUm, xMaxUm, yMinUm, yMaxUm;
	int ret;
	ret = GetLimitsUm(xMinUm, xMaxUm,yMinUm,yMaxUm);
	if (ret != DEVICE_OK)
	{
		return ret;
	}
	xMin = xMinUm/stepSizeXUm_;	// microns/(microns/step) = steps
	xMax = xMaxUm/stepSizeXUm_;
	yMin = yMinUm/stepSizeYUm_;
	yMax = yMaxUm/stepSizeYUm_;

	return DEVICE_OK;
}

int XYPolluxStage::Move(double vx, double vy)
{
	// "start moving with velocity (vx,vy) in mm/s"

	this->LogMessage("XYPolluxStage::Move\n",true);
	
	//vx and vy are in units of mm/s, which is Venus uses
	stringstream sstmx, sstmy;
	string command;
	int ret;


	// command X
	sstmx << setprecision(5) << fixed << vx << " " << (int)axisX_ << " speed";
	command = sstmx.str();
	ret = sendCommand(command);
	if (ret != DEVICE_OK)
	{
		return ret;
	}

	// command Y
	sstmy << setprecision(5) << fixed << vy << " " << (int)axisY_ << " speed";
	command = sstmy.str();
	ret = sendCommand(command);
	if (ret != DEVICE_OK)
	{
		return ret;
	}
	
	return DEVICE_OK;
}

int XYPolluxStage::SetPositionSteps(long x, long y)
{
	stringstream os;
	os << "MOVING ABSOLUTELY: STEPS " << x << " X, " << y << " Y";
	this->LogMessage(os.str().c_str(), false);
	// move to the 'x', 'y' position, in step units
	double xmm,ymm;
	stringstream sstm;
	string command;
	int ret;

	// convert to mm

	xmm = x*(stepSizeXUm_/1000);
	ymm = y*(stepSizeYUm_/1000);

	// command X
	sstm << fixed << setprecision(5) << xmm << " " << (int)axisX_ << " nmove";
	command = sstm.str();
	this->LogMessage(sstm.str().c_str(), false);
	ret = sendCommand(command);
	if (ret != DEVICE_OK)
	{
		return ret;
	}

	// clear sstm
	sstm.str(string());
	sstm.clear();
	Sleep(10); // sleep 100ms

	// command Y
	sstm << fixed << setprecision(5) << ymm << " " << (int)axisY_ << " nmove";
	command = sstm.str();
	this->LogMessage(sstm.str().c_str(), false);
	ret = sendCommand(command);
	if (ret != DEVICE_OK)
	{
		return ret;
	}
	return DEVICE_OK;
	
}

int XYPolluxStage::GetPositionSteps(long& x, long& y)
{
	// Get the current position, in units of 'steps'
	double xmm, ymm;
	stringstream sstm;
	string req, firstWord;
	vector<string> reply;
	int queryOK;

	// get x
	sstm << (int)axisX_ << " npos";
	req = sstm.str();
	queryOK = queryCommand(req,reply);
	if (queryOK != DEVICE_OK)
	{
		return queryOK;
	}
	firstWord = reply[0];

	xmm = ::atof(firstWord.c_str());	// in units of mm
	x = xmm*1000/stepSizeXUm_;		// "mms/(mm/step) = steps"

	// clear sstm
	sstm.str(string());
	sstm.clear();

	// get y
	sstm << (int)axisY_ << " npos";
	req = sstm.str();
	queryOK = queryCommand(req,reply);
	if (queryOK != DEVICE_OK)
	{
		return queryOK;
	}
	firstWord = reply[1];
	ymm = ::atof(firstWord.c_str());	// in units of mm
	y = ymm*1000/stepSizeYUm_;		// "mms/(mm/step) = steps"

	return DEVICE_OK;
}

int XYPolluxStage::SetRelativePositionSteps(long x, long y)
{
	// move to the 'x', 'y' position, in step units
	double xmm,ymm;
	stringstream sstm;
	string command;
	int ret;

	// convert to mm

	xmm = x*stepSizeXUm_/1000;
	ymm = y*stepSizeYUm_/1000;

	ostringstream os;
	os << "DEBUG MOVE ALONG X: " << x << " in STEPS, " << xmm << " in millimeters";
	this->LogMessage(os.str().c_str(), false);


	// command X
	sstm << fixed << setprecision(5) << xmm << " " << (int)axisX_ << " nrmove";
	command = sstm.str();
	ret = sendCommand(command);
	if (ret != DEVICE_OK)
	{
		return ret;
	}

	// clear sstm
	sstm.str(string());
	sstm.clear();

	// command Y
	sstm  << fixed << setprecision(5)<< ymm << " " << (int)axisY_ << " nrmove";
	command = sstm.str();
	ret = sendCommand(command);
	if (ret != DEVICE_OK)
	{
		return ret;
	}
	
	return DEVICE_OK;
}

int XYPolluxStage::Home()
{
	// Homing re-calibrates the stages using the hardware limit switches, and re-assigns the origin to the 'cal' limit switches

	stringstream sstm;
	string command;
	int ret;
	
	// first, set the minimum limits by going to 'cal' limit-switch

	// command X
	sstm << (int)axisX_ << " ncal";
	command = sstm.str();
	ret = sendCommand(command);
	if (ret != DEVICE_OK)
	{
		return ret;
	}

	// clear sstm
	sstm.str(string());
	sstm.clear();

	// command Y
	sstm << (int)axisY_ << " ncal";
	command = sstm.str();
	ret = sendCommand(command);
	if (ret != DEVICE_OK)
	{
		return ret;
	}

	// clear sstm
	sstm.str(string());
	sstm.clear();

	// next, set the maximum limits by going to the 'rm' limit switch

	// command X
	sstm << (int)axisX_ << " nrm";
	command = sstm.str();
	ret = sendCommand(command);
	if (ret != DEVICE_OK)
	{
		return ret;
	}

	// clear sstm
	sstm.str(string());
	sstm.clear();

	// command Y
	sstm << (int)axisY_ << " nrm";
	command = sstm.str();
	ret = sendCommand(command);
	if (ret != DEVICE_OK)
	{
		return ret;
	}

	return DEVICE_OK;
}

int XYPolluxStage::Stop()
{
	// Command both x and y stages to stop immediately
	string command = "\x03";
	int ret;

	ret = sendCommand(command);
	if (ret != DEVICE_OK)
	{
		return ret;
	}
	return DEVICE_OK;
}

int XYPolluxStage::SetOrigin()
{
	// "Set the current position as (0,0)"
	int ret;
	ret = SetXOrigin();
	if (ret != DEVICE_OK)
	{
		return ret;
	}
	ret = SetYOrigin();
	if (ret != DEVICE_OK)
	{
		return ret;
	}
	return DEVICE_OK;
}

int XYPolluxStage::SetXOrigin()
{
	// "Set the current position of the X axis to 0"
	stringstream sstm;
	string command;
	int ret;

	sstm << "0.0 " << (int)axisX_ << " setnpos";
	command = sstm.str();
	ret = sendCommand(command);
	if (ret != DEVICE_OK)
	{
		return ret;
	}
	return DEVICE_OK;
}

int XYPolluxStage::SetYOrigin()
{
	// "Set the current position of the Y axis to 0"
	stringstream sstm;
	string command;
	int ret;

	sstm << "0.0 " << (int)axisY_ << " setnpos";
	command = sstm.str();
	ret = sendCommand(command);
	if (ret != DEVICE_OK)
	{
		return ret;
	}
	return DEVICE_OK;
}

///////////
// Updaters
///////////

int XYPolluxStage::OnPort(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	if (eAct == MM::BeforeGet)
	{
		pProp->Set(port_.c_str());
	}
	else if (eAct == MM::AfterSet)
	{
		pProp->Get(port_);
	}
	return DEVICE_OK;
}

int XYPolluxStage::OnAxisX(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	this->LogMessage("XYStage::OnAxisX\n", true);

	if (eAct == MM::BeforeGet)
	{
		pProp->Set(axisX_);
	}
	else if (eAct == MM::AfterSet)
	{
		pProp->Get(axisX_);
	}
	return DEVICE_OK;
}

int XYPolluxStage::OnAxisY(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	this->LogMessage("XYStage::OnAxisY\n", true);

	if (eAct == MM::BeforeGet)
	{
		pProp->Set(axisY_);
	}
	else if (eAct == MM::AfterSet)
	{
		pProp->Get(axisY_);
	}
	return DEVICE_OK;
}

int XYPolluxStage::OnSpeedX(MM::PropertyBase* pProp, MM::ActionType eAct)
{ 
	this->LogMessage("XYStage::OnSpeedX\n", true);

	return OnSpeed(axisX_, pProp, eAct);
}

int XYPolluxStage::OnSpeedY(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	this->LogMessage("XYStage::OnSpeedY\n", true);

	return OnSpeed(axisY_, pProp, eAct);
}

int XYPolluxStage::OnAccelX(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	this->LogMessage("XYStage::OnAccelX\n", true);

	return OnAccel(axisX_, pProp, eAct);
}

int XYPolluxStage::OnAccelY(MM::PropertyBase* pProp, MM::ActionType eAct)
{
	this->LogMessage("XYStage::OnAccelY\n", true);

	return OnAccel(axisY_, pProp, eAct);
}


///////////////////////////
// Private Helper functions
///////////////////////////

int XYPolluxStage::OnSpeed(double axis, MM::PropertyBase* pProp, MM::ActionType eAct) const
{
	// speed is in units of mm/s

	this->LogMessage("XYPolluxStage::OnSpeed\n", true);

	if (eAct == MM::BeforeGet)
	{
		double speed;
		stringstream sstm;
		string req, firstWord;
		vector<string> reply;
		int queryOK;

		// get current speed
		sstm << (int)axis << " getnvel";
		req = sstm.str();
		queryOK = queryCommand(req,reply);
		if (queryOK != DEVICE_OK)
		{
			return queryOK;
		}
		firstWord = reply[0];
		speed = ::atof(firstWord.c_str());	// in units of mm/s
		pProp->Set(speed);
	}
	else if (eAct == MM::AfterSet)
	{
		double speed;
		int ret;
		stringstream sstm;
		string command;

		pProp->Get(speed);
		sstm << setprecision(5) << fixed << speed << " " << (int)axis << " setnvel";
		command = sstm.str();
		this->LogMessage("XYPolluxStage::OnSpeed\n", true);
		ret = sendCommand(command);
		if (ret != DEVICE_OK)
		{
			return ret;
		}
	}
	return DEVICE_OK;
}

int XYPolluxStage::OnAccel(double axis, MM::PropertyBase* pProp, MM::ActionType eAct) const
{
	this->LogMessage("XYStage::OnAccel\n", true);

	if (eAct == MM::BeforeGet)
	{
		double accel;
		stringstream sstm;
		string req, firstWord;
		vector<string> reply;
		int queryOK;

		// get current speed
		sstm << (int) axis << " getnaccel";
		req = sstm.str();
		queryOK = queryCommand(req,reply);
		if (queryOK != DEVICE_OK)
		{
			return queryOK;
		}
		firstWord = reply[0];
		accel = ::atof(firstWord.c_str());	// in units of mm/s
		pProp->Set(accel);
	}
	else if (eAct == MM::AfterSet)
	{
		double accel;
		int ret;
		stringstream sstm;
		string command;

		pProp->Get(accel);
		sstm << setprecision(5) << fixed << accel << " " << (int)axis << " setnaccel";
		command = sstm.str();
		ret = sendCommand(command);
		if (ret != DEVICE_OK)
		{
			return ret;
		}
	}
	return DEVICE_OK;

}


// serial command handlers -------------------------------------------------------------
int XYPolluxStage::sendCommand(const string command) const
{
	// use this if you want to send a command and are not interested in the response
	this->LogMessage("XYPolluxStage::sendCommand\n", true);
	stringstream os;
	os << "SENDING COMMAND: " << command;
	this->LogMessage(os.str().c_str(), false);
	string baseCommand = "";
	baseCommand+= command;
	return core_->SetSerialCommand(device_, port_.c_str(), baseCommand.c_str(), asciiTerminator_.c_str());
}

int XYPolluxStage::queryCommand(const string command, vector<string>& reply) const
{
	int err = core_->PurgeSerial(device_, port_.c_str());
	if (err != DEVICE_OK)
      return err;
	// use this if you are interested in the response
	this->LogMessage("XYPolluxStage::queryCommand\n", true);
	const size_t BUFSIZE = 2048;
	char buf[BUFSIZE] = {'\0'};

	// send the query
	int ret = sendCommand(command);
	if (ret!= DEVICE_OK)
	{
		return ret;
	}
	// read in the reply
	ret = core_->GetSerialAnswer(device_, port_.c_str(), BUFSIZE, buf, asciiTerminator_.c_str());
	
	if (ret!= DEVICE_OK)
	{
		return ret;
	}

	// parse 'res' into 'reply'
	string res = buf;
	
	if (res.length() <1)
	{
		return DEVICE_SERIAL_INVALID_RESPONSE;
	}
	CDeviceUtils::Tokenize(res,reply," ");
	
	return DEVICE_OK;
}

// COMMUNICATION "clear buffer" utility function:
int XYPolluxStage::clearPort() const
{
	core_->LogMessage(device_, "XYPolluxStage::ClearPort\n", true);

	const int bufSize = 255;
	unsigned char clear[bufSize];
	unsigned long read = bufSize;
	int ret;
	while ((int) read == bufSize)
	{
		ret = core_->ReadFromSerial(device_, port_.c_str(), clear, bufSize, read);
		cout << bufSize << endl;
		cout << read << endl;
		if (ret != DEVICE_OK) 
		{
			return ret;
		}
	}
	return DEVICE_OK;      
}