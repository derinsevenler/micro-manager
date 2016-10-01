///////////////////////////////////////////////////////////////////////////////
// FILE:          ThorlabsKinesisStage.h
// PROJECT:       Micro-Manager
// SUBSYSTEM:     DeviceAdapters
//-----------------------------------------------------------------------------
// DESCRIPTION:   Control of Thorlabs stages using the Kinesis library
//
// COPYRIGHT:     Derin Sevenler, Boston University, 2016
//
// LICENSE:       This file is distributed under the BSD license.
//
//                This file is distributed in the hope that it will be useful,
//                but WITHOUT ANY WARRANTY; without even the implied warranty
//                of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//                IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//                CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//                INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES.
//
// AUTHOR:        Derin Sevenler, Boston University, 2016

#ifndef _THORLABKINESISSTAGE_H_
#define _THORLABKINESISSTAGE_H_

#include "../../MMDevice/MMDevice.h"
#include "../../MMDevice/DeviceBase.h"
#include "Thorlabs.MotionControl.KCube.DCServo.h"
#include <string>
#include <sstream>


class ThorlabsKinesisStage : public CStageBase<ThorlabsKinesisStage>
{
public:
	ThorlabsKinesisStage();
	~ThorlabsKinesisStage();

	// Device API
	// ----------
	int Initialize();
	int Shutdown();
	void GetName(char* name) const;
	bool Busy();

	// Stage API from DemoCamera/CDemoStage
    // ---------

    int SetPositionUm(double posUm);
    int GetPositionUm(double& posUm);
    double GetStepSize() {return stepSizeUm_;}
    int SetPositionSteps(long steps);
    int GetPositionSteps(long& steps);
    int SetOrigin(){ return DEVICE_OK; } // i.e. not implemented.
    int GetLimits(double&lower, double& upper);

    // int Move(double /*v*/){return DEVICE_OK;}

    bool IsContinuousFocusDrive() const {return false;}
    int IsStageSequenceable(bool& isSequenceable) const {isSequenceable = false; return DEVICE_OK;} // i.e., it is not sequencable.

    // action interface
    // ----------------


    int OnSerialNumber(MM::PropertyBase* pProp, MM::ActionType eAct);
    int OnPosition(MM::PropertyBase* pProp, MM::ActionType eAct);
    int OnVelocity(MM::PropertyBase* pProp, MM::ActionType eAct);
    int OnAccel(MM::PropertyBase* pProp, MM::ActionType eAct);

private:
	// Generic stage-related parameters
	double posUm_;
	bool busy_;
	bool initialized_;
	double lowerLimit_;
	double upperLimit_;
	double stepsPerRev_;
	double gearBoxRatio_;
	double pitch_;
	double stepSizeUm_;

	// Required by Kinesis
	std::string serialNumber_;
	long accel_;
	long maxVel_;

	// helpers
	int OnStagePositionChanged(long totalSteps);
	int GetVelParam(long vel);
	int SetVelParam(long vel);
	int GetAccelParam(long accel);
	int SetAccelParam(long accel);
	// const char * serialStr();
};

#endif