// GP2040 includes
#include "gp2040.h"
#include "helper.h"
#include "modules/pleds.h"
#include "storage.h"
#include "usb_driver.h"

// Pico includes
#include "pico/bootrom.h"
#include "pico/util/queue.h"

// TinyUSB
#include "tusb.h"

// RNDIS
#include "rndis/rndis.h"

#define GAMEPAD_DEBOUNCE_MILLIS 5 // make this a class object

GP2040::GP2040() : nextRuntime(0) {
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

	// Initialize Cross-Core Communications
	queue_init(Storage::getInstance().GetGamepadQueue(), sizeof(Gamepad), 1);
	queue_init(Storage::getInstance().GetFeatureQueue(), PLED_REPORT_SIZE, 20);

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

	if (nextRuntime > getMicro()) { // fix for unsigned
		sleep_us(50); // Give some time back to our CPU (lower power consumption)
		return;
	}

	// gpio reads
	gamepad->read();
#if GAMEPAD_DEBOUNCE_MILLIS > 0
	gamepad->debounce();
#endif
	gamepad->hotkey();
	gamepad->process(); // send to USB controller host

	// Get Player LED features on USB
	send_report(gamepad->getReport(), gamepad->getReportSize());
	memset(featureData, 0, sizeof(featureData));
	receive_report(featureData);
	tud_task();

	// Setup Queue for our Module Data (Player LEDs)
	if (featureData[0])
		queue_try_add(Storage::getInstance().GetFeatureQueue(), featureData);
	
	// Setup Queue for our Module Data (Gamepad Input)
	if (queue_is_empty(Storage::getInstance().GetGamepadQueue())) {
		memcpy(&snapshot, gamepad, sizeof(Gamepad));
		queue_try_add(Storage::getInstance().GetGamepadQueue(), &snapshot);
	}

	nextRuntime = getMicro() + GAMEPAD_POLL_MICRO;
}

void GP2040::configLoop() {
	rndis_task(); // change with whatever
	sleep_us(50);
}
