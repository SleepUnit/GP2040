// GP2040 includes
#include "gp2040.h"
#include "storage.h"
#include "helper.h"
#include "usb_driver.h"

// Pico includes
#include "pico/bootrom.h"
#include "pico/util/queue.h"

// TinyUSB
#include "tusb.h"

// RNDIS
#include "rndis/rndis.h"

#define GAMEPAD_DEBOUNCE_MILLIS 5 // make this a class object

GP2040::GP2040() {
	Storage::getInstance().SetGamepad(new Gamepad(GAMEPAD_DEBOUNCE_MILLIS));
}

GP2040::~GP2040() {
}

void GP2040::setup() {
    // Start storage before anything else
	GamepadStore.start();
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	gamepad->setup();

	// Check for input mode override
	gamepad->read();
	InputMode inputMode = gamepad->options.inputMode;
	if (gamepad->pressedS2())
		inputMode = INPUT_MODE_CONFIG;
	else if (gamepad->pressedB3())
		inputMode = INPUT_MODE_HID;
	else if (gamepad->pressedB1())
		inputMode = INPUT_MODE_SWITCH;
	else if (gamepad->pressedB2())
		inputMode = INPUT_MODE_XINPUT;
	else if (gamepad->pressedF1() && gamepad->pressedUp())
		reset_usb_boot(0, 0);

	queue_init(Storage::getInstance().GetQueue(), sizeof(Gamepad), 1);

	Storage::getInstance().SetConfigMode(inputMode == INPUT_MODE_CONFIG);
	if (inputMode != gamepad->options.inputMode && 
			inputMode != INPUT_MODE_CONFIG)
	{
		gamepad->options.inputMode = inputMode;
		gamepad->save(); // MPGS save
	}

	initialize_driver(inputMode);

	// Config mode means we go into the web server (MAKE THIS A CLASS)
	if (inputMode == INPUT_MODE_CONFIG ) {
		rndis_init();
	}
}

void GP2040::run() {
	bool configMode = Storage::getInstance().GetConfigMode();
	if(configMode == true) { 	// Configuration Mode
		while (1)
			configLoop();
	} else {					// Standard Gamepad
		while (1)
			loop();
	}
}

void GP2040::loop() {
	Gamepad * gamepad = Storage::getInstance().GetGamepad();

	// Move these to class variables
	static void *report;
	static const uint64_t intervalUS = 100; // 0.1ms check, more accurate hopefully
	static const uint32_t intervalMS = 1;
	static uint64_t nextRuntime = 0;
	static uint8_t featureData[32] = { };
	static Gamepad snapshot;

	//if (getMicro() - nextRuntime < 0) { // fix for unsigned
	if (getMillis() - nextRuntime < 0) { // unsigned problem?
		sleep_us(50); // give some hz back while we wait
		return;
	}

	gamepad->read();
#if GAMEPAD_DEBOUNCE_MILLIS > 0
	gamepad->debounce();
#endif
	gamepad->hotkey();
	gamepad->process();
	send_report(gamepad->getReport(), gamepad->getReportSize());

	memset(featureData, 0, sizeof(featureData));
	receive_report(featureData);

// This is sending our feature data to the Player LED module, not the right way
//	if (featureData[0])
//		queue_try_add(&pledModule.featureQueue, featureData);

	tud_task();

	if (queue_is_empty(Storage::getInstance().GetQueue())) {
		memcpy(&snapshot, &gamepad, sizeof(Gamepad));
		queue_try_add(Storage::getInstance().GetQueue(), &snapshot);
	}

	//nextRuntime = getMicro() + intervalUS;
	nextRuntime = getMillis() + intervalMS;
}

void GP2040::configLoop() {
	rndis_task(); // change with whatever
	sleep_us(50);
}
