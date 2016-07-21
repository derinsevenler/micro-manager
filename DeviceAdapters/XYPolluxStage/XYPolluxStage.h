#ifndef _XYPOLLUXSTAGE_

#define _XYPOLLUXSTAGE_

#include "../../MMDevice/MMDevice.h"
#include "../../MMDevice/DeviceBase.h"
#include "../../MMDevice/ModuleInterface.h"

#include <sstream>
#include <string>
#include <iomanip>

extern const char* g_XYPolluxStageName;
extern const char* g_XYStageDescription;


class XYPolluxStage : public CXYStageBase<XYPolluxStage>
{
public:
	XYPolluxStage();
	~XYPolluxStage();

	// Device API
	// ----------
	int Initialize();
	int Shutdown();
	void GetName(char* name) const;
	bool Busy();


	// XYStage API
	// -----------

	int GetLimitsUm(double& xMin, double& xMax, double& yMin, double& yMax);
	int GetStepLimits(long& xMin, long& xMax, long& yMin, long& yMax);
	int Move(double vx, double vy);
	int SetPositionSteps(long x, long y);
	int GetPositionSteps(long& x, long& y);
	int SetRelativePositionSteps(long x, long y);
	int Home();
	int Stop();
	int SetOrigin();
	int SetXOrigin();
	int SetYOrigin();
	int SetAdapterOrigin();
	

	double GetStepSizeXUm() {return stepSizeXUm_;}
	double GetStepSizeYUm() {return stepSizeYUm_;}
	int IsXYStageSequenceable(bool& isSequenceable) const {isSequenceable = false; return DEVICE_OK;}


	// updaters
	// --------
	int OnPort		(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnAxisX		(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnAxisY		(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnSpeedX	(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnSpeedY	(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnAccelX	(MM::PropertyBase* pProp, MM::ActionType eAct);
	int OnAccelY	(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
	
	// private helper functions4
	int OnSpeed(double axis, MM::PropertyBase* pProp, MM::ActionType eAct) const;
	int OnAccel(double axis, MM::PropertyBase* pProp, MM::ActionType eAct) const;

	int sendCommand(const std::string command) const;
	int queryCommand(const std::string command, std::vector<std::string>& reply) const;
	int clearPort() const;

	// private class variables
	bool		initialized_;
	std::string port_;
	MM::Device	*device_;
	MM::Core	*core_;

	double stepSizeXUm_;
	double stepSizeYUm_;

	double axisX_;
	double axisY_;

	std::string asciiTerminator_;

};


#endif