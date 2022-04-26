/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2021 Jason Skuby (mytechtoybox.com)
 */

#pragma once

// GP2040 Classes
#include "modules/i2cdisplay.h"
#include "modules/neopicoleds.h"
#include "modules/pleds.h"
#include "gpmodule.h"

#include <vector>

class GP2040Core1 {
public:
	GP2040Core1();
    ~GP2040Core1();
    void setup();
    void run();
private:
    void loop();
    void configLoop();
    void setupModule(GPModule*);
	std::vector<GPModule*> modules;
    I2CDisplayModule i2cdisplay;
    NeoPicoLEDModule neopicoled;
	PLEDModule playerled;
};

