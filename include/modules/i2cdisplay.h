/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2021 Jason Skuby (mytechtoybox.com)
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <string>
#include <hardware/i2c.h>
#include "OneBitDisplay.h"
#include "BoardConfig.h"
#include "gpmodule.h"
#include "gamepad.h"

#ifndef BUTTON_LAYOUT
#define BUTTON_LAYOUT BUTTON_LAYOUT_ARCADE
#endif

#ifndef HAS_I2C_DISPLAY
#define HAS_I2C_DISPLAY 0
#endif

#ifndef DISPLAY_I2C_ADDR
#define DISPLAY_I2C_ADDR 0x3C
#endif

#ifndef DISPLAY_SIZE
#define DISPLAY_SIZE OLED_128x64
#endif

#ifndef DISPLAY_FLIP
#define DISPLAY_FLIP 0
#endif

#ifndef DISPLAY_INVERT
#define DISPLAY_INVERT 0
#endif

#ifndef DISPLAY_USEWIRE
#define DISPLAY_USEWIRE 1
#endif

#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN -1
#endif

#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN -1
#endif

#ifndef I2C_BLOCK
#define I2C_BLOCK i2c0
#endif

#ifndef I2C_SPEED
#define I2C_SPEED 400000
#endif

// i2C OLED Display
class I2CDisplayModule : public GPModule
{
public:
	virtual bool available();  // GPModule
	virtual void setup();
	virtual void loop();
	virtual void process(Gamepad *gamepad);
	void clearScreen(int render); // DisplayModule
	void drawHitbox(int startX, int startY, int buttonRadius, int buttonPadding, Gamepad *gamepad);
	void drawWasdBox(int startX, int startY, int buttonRadius, int buttonPadding, Gamepad *gamepad);
	void drawArcadeStick(int startX, int startY, int buttonRadius, int buttonPadding, Gamepad *gamepad);
	void drawStatusBar();
	void setStatusBar(Gamepad *gamepad);
	uint8_t ucBackBuffer[1024];
	OBDISP obd;
	std::string statusBar;
};

#endif
