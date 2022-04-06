/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2021 Jason Skuby (mytechtoybox.com)
 */

#ifndef GP2040CORE1_H_
#define GP2040CORE1_H_

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
    void configLoop();
    void loop();
	IICDisplayModule iicdisplay;
    NeoPicoLEDModule neopicoled;
	PLEDModule pled;
    std::vector<GPModule*> modules;
};

#endif
