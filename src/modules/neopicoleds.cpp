/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2021 Jason Skuby (mytechtoybox.com)
 */

#include "pico/util/queue.h"

#include "AnimationStation.hpp"
#include "AnimationStorage.hpp"
#include "NeoPico.hpp"
#include "Pixel.hpp"
#include "PlayerLEDs.h"
#include "gp2040.h"
#include "modules/neopicoleds.h"
#include "modules/pleds.h"
#include "themes.h"

#include "enums.h"
#include "helper.h"

static std::vector<uint8_t> EMPTY_VECTOR;

uint32_t rgbPLEDValues[4];

// TODO: Make this a helper function
// Animation Helper for Player LEDs
PLEDAnimationState getXInputAnimationNEOPICO(uint8_t *data)
{
	PLEDAnimationState animationState =
	{
		.state = 0,
		.animation = PLED_ANIM_NONE,
		.speed = PLED_SPEED_OFF,
	};

	// Check first byte for LED payload
	if (data[0] == 0x01)
	{
		switch (data[2])
		{
			case XINPUT_PLED_BLINKALL:
			case XINPUT_PLED_ROTATE:
			case XINPUT_PLED_BLINK:
			case XINPUT_PLED_SLOWBLINK:
			case XINPUT_PLED_ALTERNATE:
				animationState.state = (PLED_STATE_LED1 | PLED_STATE_LED2 | PLED_STATE_LED3 | PLED_STATE_LED4);
				animationState.animation = PLED_ANIM_BLINK;
				animationState.speed = PLED_SPEED_FAST;
				break;

			case XINPUT_PLED_FLASH1:
			case XINPUT_PLED_ON1:
				animationState.state = PLED_STATE_LED1;
				animationState.animation = PLED_ANIM_SOLID;
				animationState.speed = PLED_SPEED_OFF;
				break;

			case XINPUT_PLED_FLASH2:
			case XINPUT_PLED_ON2:
				animationState.state = PLED_STATE_LED2;
				animationState.animation = PLED_ANIM_SOLID;
				animationState.speed = PLED_SPEED_OFF;
				break;

			case XINPUT_PLED_FLASH3:
			case XINPUT_PLED_ON3:
				animationState.state = PLED_STATE_LED3;
				animationState.animation = PLED_ANIM_SOLID;
				animationState.speed = PLED_SPEED_OFF;
				break;

			case XINPUT_PLED_FLASH4:
			case XINPUT_PLED_ON4:
				animationState.state = PLED_STATE_LED4;
				animationState.animation = PLED_ANIM_SOLID;
				animationState.speed = PLED_SPEED_OFF;
				break;

			default:
				break;
		}
	}

	return animationState;
}

bool NeoPicoLEDModule::available() {
	LEDOptions ledOptions = Storage::getInstance().getLEDOptions();
	return ledOptions.dataPin != -1;
}

void NeoPicoLEDModule::setup()
{
	LEDOptions ledOptions = Storage::getInstance().getLEDOptions();
	if (!ledOptions.useUserDefinedLEDs)
	{
		ledOptions.dataPin = BOARD_LEDS_PIN;
		ledOptions.ledFormat = LED_FORMAT;
		ledOptions.ledLayout = BUTTON_LAYOUT;
		ledOptions.ledsPerButton = LEDS_PER_PIXEL;
		ledOptions.brightnessMaximum = LED_BRIGHTNESS_MAXIMUM;
		ledOptions.brightnessSteps = LED_BRIGHTNESS_STEPS;
		ledOptions.indexUp = LEDS_DPAD_UP;
		ledOptions.indexDown = LEDS_DPAD_DOWN;
		ledOptions.indexLeft = LEDS_DPAD_LEFT;
		ledOptions.indexRight = LEDS_DPAD_RIGHT;
		ledOptions.indexB1 = LEDS_BUTTON_B1;
		ledOptions.indexB2 = LEDS_BUTTON_B2;
		ledOptions.indexB3 = LEDS_BUTTON_B3;
		ledOptions.indexB4 = LEDS_BUTTON_B4;
		ledOptions.indexL1 = LEDS_BUTTON_L1;
		ledOptions.indexR1 = LEDS_BUTTON_R1;
		ledOptions.indexL2 = LEDS_BUTTON_L2;
		ledOptions.indexR2 = LEDS_BUTTON_R2;
		ledOptions.indexS1 = LEDS_BUTTON_S1;
		ledOptions.indexS2 = LEDS_BUTTON_S2;
		ledOptions.indexL3 = LEDS_BUTTON_L3;
		ledOptions.indexR3 = LEDS_BUTTON_R3;
		ledOptions.indexA1 = LEDS_BUTTON_A1;
		ledOptions.indexA2 = LEDS_BUTTON_A2;
	}
	if ( PLED_TYPE == PLED_TYPE_RGB ) {
		neoPLEDs = new NeoPicoPlayerLEDs();
	}
	configureLEDs();
}

void NeoPicoLEDModule::process(Gamepad *gamepad)
{
	AnimationHotkey action = animationHotkeys(gamepad);
	if (action != HOTKEY_LEDS_NONE)
		queue_try_add(&baseAnimationQueue, &action);

	static uint8_t featureData[PLED_REPORT_SIZE];
	if (PLED_TYPE == PLED_TYPE_RGB) {
		inputMode = gamepad->options.inputMode; // HACK
		if (queue_try_remove(Storage::getInstance().GetFeatureQueue(), featureData)) {
			switch (gamepad->options.inputMode) {
				case INPUT_MODE_XINPUT:
					animationState = getXInputAnimationNEOPICO(featureData);
					break;
			}
		}
	}

	uint32_t buttonState = gamepad->state.dpad << 16 | gamepad->state.buttons;
	queue_try_add(&buttonAnimationQueue, &buttonState);
}

void NeoPicoLEDModule::loop()
{
	LEDOptions ledOptions = Storage::getInstance().getLEDOptions();
	if (ledOptions.dataPin < 0 || !time_reached(this->nextRunTime))
		return;

	AnimationHotkey action;
	if (queue_try_remove(&baseAnimationQueue, &action))
	{
		as.HandleEvent(action);
		queue_try_add(&animationSaveQueue, 0);
	}

	uint32_t buttonState;
	if (queue_try_remove(&buttonAnimationQueue, &buttonState))
	{
		vector<Pixel> pressed;

		for (auto row : matrix.pixels)
		{
			for (auto pixel : row)
			{
				if (buttonState & pixel.mask)
					pressed.push_back(pixel);
			}
		}

		if (pressed.size() > 0)
			as.HandlePressed(pressed);
		else
			as.ClearPressed();
	}

	as.Animate();
	as.ApplyBrightness(frame);

	// Apply the player LEDs to our first 4 leds if we're in NEOPIXEL mode
	if (PLED_TYPE == PLED_TYPE_RGB) {
		switch (inputMode) { // HACK
			case INPUT_MODE_XINPUT:
				for (int i = 0; i < PLED_COUNT; i++) {
					float level = (static_cast<float>(PLED_MAX_LEVEL - neoPLEDs->getLedLevels()[i]) / static_cast<float>(PLED_MAX_LEVEL));
					float brightness = as.GetBrightnessX() * level;
					rgbPLEDValues[i] = ((RGB)ColorGreen).value(neopico->GetFormat(), brightness);
					frame[PLED_PINS[i]] = rgbPLEDValues[i];
				}
		}
	}

	neopico->SetFrame(frame);
	neopico->Show();

	this->nextRunTime = make_timeout_time_ms(NeoPicoLEDModule::intervalMS);
	trySave();
}

std::vector<uint8_t> * NeoPicoLEDModule::getLEDPositions(string button, std::vector<std::vector<uint8_t>> *positions)
{
	int buttonPosition = buttonPositions[button];
	if (buttonPosition < 0)
		return &EMPTY_VECTOR;
	else
		return &positions->at(buttonPosition);
}

/**
 * @brief Create an LED layout using a 2x4 matrix.
 */
std::vector<std::vector<Pixel>> NeoPicoLEDModule::createLedLayoutArcadeButtons(std::vector<std::vector<uint8_t>> *positions)
{
	std::vector<std::vector<Pixel>> pixels =
	{
		{
			Pixel(buttonPositions[BUTTON_LABEL_B3], GAMEPAD_MASK_B3, *getLEDPositions(BUTTON_LABEL_B3, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_B1], GAMEPAD_MASK_B1, *getLEDPositions(BUTTON_LABEL_B1, positions)),
		},
		{
			Pixel(buttonPositions[BUTTON_LABEL_B4], GAMEPAD_MASK_B4, *getLEDPositions(BUTTON_LABEL_B4, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_B2], GAMEPAD_MASK_B2, *getLEDPositions(BUTTON_LABEL_B2, positions)),
		},
		{
			Pixel(buttonPositions[BUTTON_LABEL_R1], GAMEPAD_MASK_R1, *getLEDPositions(BUTTON_LABEL_R1, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_R2], GAMEPAD_MASK_R2, *getLEDPositions(BUTTON_LABEL_R2, positions)),
		},
		{
			Pixel(buttonPositions[BUTTON_LABEL_L1], GAMEPAD_MASK_L1, *getLEDPositions(BUTTON_LABEL_L1, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_L2], GAMEPAD_MASK_L2, *getLEDPositions(BUTTON_LABEL_L2, positions)),
		},
		{
			Pixel(buttonPositions[BUTTON_LABEL_LEFT], GAMEPAD_MASK_DL, *getLEDPositions(BUTTON_LABEL_LEFT, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_DOWN], GAMEPAD_MASK_DD, *getLEDPositions(BUTTON_LABEL_DOWN, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_RIGHT], GAMEPAD_MASK_DR, *getLEDPositions(BUTTON_LABEL_RIGHT, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_UP], GAMEPAD_MASK_DU, *getLEDPositions(BUTTON_LABEL_UP, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_S1], GAMEPAD_MASK_S1, *getLEDPositions(BUTTON_LABEL_S1, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_S2], GAMEPAD_MASK_S2, *getLEDPositions(BUTTON_LABEL_S2, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_L3], GAMEPAD_MASK_L3, *getLEDPositions(BUTTON_LABEL_L3, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_R3], GAMEPAD_MASK_R3, *getLEDPositions(BUTTON_LABEL_R3, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_A1], GAMEPAD_MASK_A1, *getLEDPositions(BUTTON_LABEL_A1, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_A2], GAMEPAD_MASK_A2, *getLEDPositions(BUTTON_LABEL_A2, positions)),
		},
	};

	return pixels;
}

/**
 * @brief Create an LED layout using a 3x8 matrix.
 */
std::vector<std::vector<Pixel>> NeoPicoLEDModule::createLedLayoutArcadeHitbox(vector<vector<uint8_t>> *positions)
{
	std::vector<std::vector<Pixel>> pixels =
	{
		{
			Pixel(buttonPositions[BUTTON_LABEL_LEFT], GAMEPAD_MASK_DL, *getLEDPositions(BUTTON_LABEL_LEFT, positions)),
			NO_PIXEL,
			NO_PIXEL,
		},
		{
			Pixel(buttonPositions[BUTTON_LABEL_DOWN], GAMEPAD_MASK_DD, *getLEDPositions(BUTTON_LABEL_DOWN, positions)),
			NO_PIXEL,
			NO_PIXEL,
		},
		{
			Pixel(buttonPositions[BUTTON_LABEL_RIGHT], GAMEPAD_MASK_DR, *getLEDPositions(BUTTON_LABEL_RIGHT, positions)),
			NO_PIXEL,
			NO_PIXEL,
		},
		{
			NO_PIXEL,
			Pixel(buttonPositions[BUTTON_LABEL_UP], GAMEPAD_MASK_DU, *getLEDPositions(BUTTON_LABEL_UP, positions)),
		},
		{
			Pixel(buttonPositions[BUTTON_LABEL_B3], GAMEPAD_MASK_B3, *getLEDPositions(BUTTON_LABEL_B3, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_B1], GAMEPAD_MASK_B1, *getLEDPositions(BUTTON_LABEL_B1, positions)),
			NO_PIXEL,
		},
		{
			Pixel(buttonPositions[BUTTON_LABEL_B4], GAMEPAD_MASK_B4, *getLEDPositions(BUTTON_LABEL_B4, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_B2], GAMEPAD_MASK_B2, *getLEDPositions(BUTTON_LABEL_B2, positions)),
			NO_PIXEL,
		},
		{
			Pixel(buttonPositions[BUTTON_LABEL_R1], GAMEPAD_MASK_R1, *getLEDPositions(BUTTON_LABEL_R1, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_R2], GAMEPAD_MASK_R2, *getLEDPositions(BUTTON_LABEL_R2, positions)),
			NO_PIXEL,
		},
		{
			Pixel(buttonPositions[BUTTON_LABEL_L1], GAMEPAD_MASK_L1, *getLEDPositions(BUTTON_LABEL_L1, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_L2], GAMEPAD_MASK_L2, *getLEDPositions(BUTTON_LABEL_L2, positions)),
			NO_PIXEL,
		},
		{
			Pixel(buttonPositions[BUTTON_LABEL_S1], GAMEPAD_MASK_S1, *getLEDPositions(BUTTON_LABEL_S1, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_S2], GAMEPAD_MASK_S2, *getLEDPositions(BUTTON_LABEL_S2, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_L3], GAMEPAD_MASK_L3, *getLEDPositions(BUTTON_LABEL_L3, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_R3], GAMEPAD_MASK_R3, *getLEDPositions(BUTTON_LABEL_R3, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_A1], GAMEPAD_MASK_A1, *getLEDPositions(BUTTON_LABEL_A1, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_A2], GAMEPAD_MASK_A2, *getLEDPositions(BUTTON_LABEL_A2, positions)),
		},
	};

	return pixels;
}

/**
 * @brief Create an LED layout using a 2x7 matrix.
 */
std::vector<std::vector<Pixel>> NeoPicoLEDModule::createLedLayoutArcadeWasd(std::vector<std::vector<uint8_t>> *positions)
{
	std::vector<std::vector<Pixel>> pixels =
	{
		{
			NO_PIXEL,
			Pixel(buttonPositions[BUTTON_LABEL_LEFT], GAMEPAD_MASK_DL, *getLEDPositions(BUTTON_LABEL_LEFT, positions)),
		},
		{
			Pixel(buttonPositions[BUTTON_LABEL_UP], GAMEPAD_MASK_DU, *getLEDPositions(BUTTON_LABEL_UP, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_DOWN], GAMEPAD_MASK_DD, *getLEDPositions(BUTTON_LABEL_DOWN, positions)),
		},
		{
			NO_PIXEL,
			Pixel(buttonPositions[BUTTON_LABEL_RIGHT], GAMEPAD_MASK_DR, *getLEDPositions(BUTTON_LABEL_RIGHT, positions)),
		},
		{
			Pixel(buttonPositions[BUTTON_LABEL_B3], GAMEPAD_MASK_B3, *getLEDPositions(BUTTON_LABEL_B3, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_B1], GAMEPAD_MASK_B1, *getLEDPositions(BUTTON_LABEL_B1, positions)),
		},
		{
			Pixel(buttonPositions[BUTTON_LABEL_B4], GAMEPAD_MASK_B4, *getLEDPositions(BUTTON_LABEL_B4, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_B2], GAMEPAD_MASK_B2, *getLEDPositions(BUTTON_LABEL_B2, positions)),
		},
		{
			Pixel(buttonPositions[BUTTON_LABEL_R1], GAMEPAD_MASK_R1, *getLEDPositions(BUTTON_LABEL_R1, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_R2], GAMEPAD_MASK_R2, *getLEDPositions(BUTTON_LABEL_R2, positions)),
		},
		{
			Pixel(buttonPositions[BUTTON_LABEL_L1], GAMEPAD_MASK_L1, *getLEDPositions(BUTTON_LABEL_L1, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_L2], GAMEPAD_MASK_L2, *getLEDPositions(BUTTON_LABEL_L2, positions)),
		},
		{
			Pixel(buttonPositions[BUTTON_LABEL_S1], GAMEPAD_MASK_S1, *getLEDPositions(BUTTON_LABEL_S1, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_S2], GAMEPAD_MASK_S2, *getLEDPositions(BUTTON_LABEL_S2, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_L3], GAMEPAD_MASK_L3, *getLEDPositions(BUTTON_LABEL_L3, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_R3], GAMEPAD_MASK_R3, *getLEDPositions(BUTTON_LABEL_R3, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_A1], GAMEPAD_MASK_A1, *getLEDPositions(BUTTON_LABEL_A1, positions)),
			Pixel(buttonPositions[BUTTON_LABEL_A2], GAMEPAD_MASK_A2, *getLEDPositions(BUTTON_LABEL_A2, positions)),
		},
	};

	return pixels;
}

std::vector<std::vector<Pixel>> NeoPicoLEDModule::createLedButtonLayout(ButtonLayout layout, std::vector<vector<uint8_t>> *positions)
{
	switch (layout)
	{
		case BUTTON_LAYOUT_ARCADE:
			return createLedLayoutArcadeButtons(positions);

		case BUTTON_LAYOUT_HITBOX:
			return createLedLayoutArcadeHitbox(positions);

		case BUTTON_LAYOUT_WASD:
			return createLedLayoutArcadeWasd(positions);
	}
}

std::vector<std::vector<Pixel>> NeoPicoLEDModule::createLedButtonLayout(ButtonLayout layout, uint8_t ledsPerPixel, uint8_t ledButtonCount)
{
	vector<vector<uint8_t>> positions(ledButtonCount);
	for (int i = 0; i != ledButtonCount; i++)
	{
		positions[i].resize(ledsPerPixel);
		for (int l = 0; l != ledsPerPixel; l++)
			positions[i][l] = (i * ledsPerPixel) + l;
	}

	return createLedButtonLayout(layout, &positions);
}

uint8_t NeoPicoLEDModule::setupButtonPositions()
{
	LEDOptions ledOptions = Storage::getInstance().getLEDOptions();
	buttonPositions.clear();
	buttonPositions.emplace(BUTTON_LABEL_UP, ledOptions.indexUp);
	buttonPositions.emplace(BUTTON_LABEL_DOWN, ledOptions.indexDown);
	buttonPositions.emplace(BUTTON_LABEL_LEFT, ledOptions.indexLeft);
	buttonPositions.emplace(BUTTON_LABEL_RIGHT, ledOptions.indexRight);
	buttonPositions.emplace(BUTTON_LABEL_B1, ledOptions.indexB1);
	buttonPositions.emplace(BUTTON_LABEL_B2, ledOptions.indexB2);
	buttonPositions.emplace(BUTTON_LABEL_B3, ledOptions.indexB3);
	buttonPositions.emplace(BUTTON_LABEL_B4, ledOptions.indexB4);
	buttonPositions.emplace(BUTTON_LABEL_L1, ledOptions.indexL1);
	buttonPositions.emplace(BUTTON_LABEL_R1, ledOptions.indexR1);
	buttonPositions.emplace(BUTTON_LABEL_L2, ledOptions.indexL2);
	buttonPositions.emplace(BUTTON_LABEL_R2, ledOptions.indexR2);
	buttonPositions.emplace(BUTTON_LABEL_S1, ledOptions.indexS1);
	buttonPositions.emplace(BUTTON_LABEL_S2, ledOptions.indexS2);
	buttonPositions.emplace(BUTTON_LABEL_L3, ledOptions.indexL3);
	buttonPositions.emplace(BUTTON_LABEL_R3, ledOptions.indexR3);
	buttonPositions.emplace(BUTTON_LABEL_A1, ledOptions.indexA1);
	buttonPositions.emplace(BUTTON_LABEL_A2, ledOptions.indexA2);
	uint8_t buttonCount = 0;
	for (auto const buttonPosition : buttonPositions)
	{
		if (buttonPosition.second != -1)
			buttonCount++;
	}

	return buttonCount;
}

void NeoPicoLEDModule::configureLEDs()
{
	LEDOptions ledOptions = Storage::getInstance().getLEDOptions();
	nextRunTime = make_timeout_time_ms(10000); // Set crazy timeout to prevent loop from running while we reconfigure
	uint8_t buttonCount = setupButtonPositions();
	vector<vector<Pixel>> pixels = createLedButtonLayout(ledOptions.ledLayout, ledOptions.ledsPerButton, buttonCount);
	matrix.setup(pixels, ledOptions.ledsPerButton);
	ledCount = matrix.getLedCount();
	if (PLED_TYPE == PLED_TYPE_RGB && PLED_COUNT > 0)
		ledCount += PLED_COUNT;

	queue_free(&baseAnimationQueue);
	queue_free(&buttonAnimationQueue);
	queue_free(&animationSaveQueue);

	queue_init(&baseAnimationQueue, sizeof(AnimationHotkey), 1);
	queue_init(&buttonAnimationQueue, sizeof(uint32_t), 1);
	queue_init(&animationSaveQueue, sizeof(int), 1);

	if (neopico != NULL)
		neopico->Off();

	delete neopico;
	neopico = new NeoPico(ledOptions.dataPin, ledCount, ledOptions.ledFormat);
	neopico->Off();

	Animation::format = ledOptions.ledFormat;
	AnimationStation::ConfigureBrightness(ledOptions.brightnessMaximum, ledOptions.brightnessSteps);
	AnimationStation::SetOptions(AnimationStore.getAnimationOptions());
	addStaticThemes(ledOptions);
	as.SetMode(AnimationStation::options.baseAnimationIndex);
	as.SetMatrix(matrix);

	nextRunTime = make_timeout_time_ms(0); // Reset timeout
}

void NeoPicoLEDModule::trySave()
{
	static int saveValue = 0;

	if (queue_try_remove(&animationSaveQueue, &saveValue))
		AnimationStore.save();
}

AnimationHotkey animationHotkeys(Gamepad *gamepad)
{
	AnimationHotkey action = HOTKEY_LEDS_NONE;

	if (gamepad->pressedF1())
	{
		if (gamepad->pressedB3())
		{
			action = HOTKEY_LEDS_ANIMATION_UP;
			gamepad->state.buttons &= ~(GAMEPAD_MASK_B3 | gamepad->f1Mask);
		}
		else if (gamepad->pressedB1())
		{
			action = HOTKEY_LEDS_ANIMATION_DOWN;
			gamepad->state.buttons &= ~(GAMEPAD_MASK_B1 | gamepad->f1Mask);
		}
		else if (gamepad->pressedB4())
		{
			action = HOTKEY_LEDS_BRIGHTNESS_UP;
			gamepad->state.buttons &= ~(GAMEPAD_MASK_B4 | gamepad->f1Mask);
		}
		else if (gamepad->pressedB2())
		{
			action = HOTKEY_LEDS_BRIGHTNESS_DOWN;
			gamepad->state.buttons &= ~(GAMEPAD_MASK_B2 | gamepad->f1Mask);
		}
		else if (gamepad->pressedR1())
		{
			action = HOTKEY_LEDS_PARAMETER_UP;
			gamepad->state.buttons &= ~(GAMEPAD_MASK_R1 | gamepad->f1Mask);
		}
		else if (gamepad->pressedR2())
		{
			action = HOTKEY_LEDS_PARAMETER_DOWN;
			gamepad->state.buttons &= ~(GAMEPAD_MASK_R2 | gamepad->f1Mask);
		}
		else if (gamepad->pressedL1())
		{
			action = HOTKEY_LEDS_PRESS_PARAMETER_UP;
			gamepad->state.buttons &= ~(GAMEPAD_MASK_L1 | gamepad->f1Mask);
		}
		else if (gamepad->pressedL2())
		{
			action = HOTKEY_LEDS_PRESS_PARAMETER_DOWN;
			gamepad->state.buttons &= ~(GAMEPAD_MASK_L2 | gamepad->f1Mask);
		}
	}

	return action;
}
