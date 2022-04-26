#pragma once

#include "gamepad.h"

class GPModule
{
public:
	virtual bool available() = 0;
	virtual void setup() = 0;
	virtual void loop() = 0;
	virtual void process(Gamepad *gamepad) = 0;
private:
};
