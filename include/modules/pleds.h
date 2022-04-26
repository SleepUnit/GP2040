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

#define PLED_REPORT_SIZE 32

#ifndef PLED1_PIN
#define PLED1_PIN -1
#endif
#ifndef PLED2_PIN
#define PLED2_PIN -1
#endif
#ifndef PLED3_PIN
#define PLED3_PIN -1
#endif
#ifndef PLED4_PIN
#define PLED4_PIN -1
#endif
#ifndef PLED_TYPE
#define PLED_TYPE PLED_TYPE_NONE
#endif

#define PLED_MASK_ALL ((1U << PLED1_PIN) | (1U << PLED2_PIN) | (1U << PLED3_PIN) | (1U << PLED4_PIN))

const int PLED_PINS[] = {PLED1_PIN, PLED2_PIN, PLED3_PIN, PLED4_PIN};

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
