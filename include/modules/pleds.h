/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2021 Jason Skuby (mytechtoybox.com)
 */

#ifndef PLEDS_H_
#define PLEDS_H_

#include "BoardConfig.h"
#include <stdint.h>
#include "AnimationStation.hpp"
#include "PlayerLEDs.h"
#include "gpmodule.h"
#include "helper.h"

// This needs to be moved to storage if we're going to share between modules
extern NeoPico *neopico;
extern AnimationStation as;

class PWMPlayerLEDs : public PlayerLEDs
{
public:
	void setup();
	void display();
};

// Player LED Module
class PLEDModule : public GPModule
{
public:
	virtual bool available();  // GPModule
	virtual void setup();
	virtual void loop();
	virtual void process(Gamepad *gamepad);
	PLEDModule() : type(PLED_TYPE) { }
	PLEDModule(PLEDType type) : type(type) { }
protected:
	PLEDType type;
	PWMPlayerLEDs * pwmLEDs = nullptr;
	PLEDAnimationState animationState;
};

extern PLEDModule pledModule;

#endif
