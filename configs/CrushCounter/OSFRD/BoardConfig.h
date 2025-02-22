/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2021 Jason Skuby (mytechtoybox.com)
 */

#pragma once

#include <GamepadEnums.h>
#include "enums.h"
#include "NeoPico.hpp"
#include "PlayerLEDs.h"

#define PIN_DPAD_UP     13
#define PIN_DPAD_DOWN   11
#define PIN_DPAD_LEFT   10
#define PIN_DPAD_RIGHT  12
#define PIN_BUTTON_B1   4
#define PIN_BUTTON_B2   5
#define PIN_BUTTON_B3   0
#define PIN_BUTTON_B4   1
#define PIN_BUTTON_L1   3
#define PIN_BUTTON_R1   2
#define PIN_BUTTON_L2   7
#define PIN_BUTTON_R2   6
#define PIN_BUTTON_S1   8
#define PIN_BUTTON_S2   9
#define PIN_BUTTON_L3   17
#define PIN_BUTTON_R3   16
#define PIN_BUTTON_A1   28
#define PIN_BUTTON_A2   18

#define DEFAULT_SOCD_MODE SOCD_MODE_NEUTRAL
#define BUTTON_LAYOUT BUTTON_LAYOUT_HITBOX

#define BOARD_LEDS_PIN 14

#define LED_BRIGHTNESS_MAXIMUM 100
#define LED_BRIGHTNESS_STEPS 5
#define LED_FORMAT LED_FORMAT_GRB
#define LEDS_PER_PIXEL 1

#define LEDS_DPAD_LEFT   0
#define LEDS_DPAD_DOWN   1
#define LEDS_DPAD_RIGHT  2
#define LEDS_DPAD_UP     3
#define LEDS_BUTTON_B3   4
#define LEDS_BUTTON_B4   5
#define LEDS_BUTTON_R1   6
#define LEDS_BUTTON_L1   7
#define LEDS_BUTTON_B1   8
#define LEDS_BUTTON_B2   9
#define LEDS_BUTTON_R2   10
#define LEDS_BUTTON_L2   11

#define PLED_TYPE PLED_TYPE_RGB
#define PLED1_PIN 12
#define PLED2_PIN 13
#define PLED3_PIN 14
#define PLED4_PIN 15

