// GP2040 includes
#include "gp2040core1.h"
#include "storage.h"

#include <iterator>

#define GAMEPAD_DEBOUNCE_MILLIS 5 // make this a class object

GP2040Core1::GP2040Core1() {
}

GP2040Core1::~GP2040Core1() {
}

void GP2040Core1::setup() {
// BOARD DEFINES HERE?
	if (iicdisplay.available()) {
		iicdisplay.setup();
		modules.push_back(&iicdisplay);
	}
	if (neopicoled.available()) {
		neopicoled.setup();
		modules.push_back(&neopicoled);
	}
	if (pled.available()) {
		pled.setup();
		modules.push_back(&pled);
	}
}

void GP2040Core1::run() {
	bool configMode = Storage::getInstance().GetConfigMode();
	if(configMode == true) {
		while (1)
			configLoop();
	} else {
		while (1)
			loop();
	}
}

void GP2040Core1::configLoop() {
	// Special state for configuration on OLED/LED/PLED?
	sleep_ms(1000);
}

void GP2040Core1::loop() {
	for (std::vector<GPModule*>::iterator it = modules.begin(); it != modules.end(); it++)
		(*it)->loop();
	sleep_us(100);
}
