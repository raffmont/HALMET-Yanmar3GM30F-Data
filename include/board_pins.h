#pragma once

#include <Arduino.h>

namespace halmet_yanmar {

// External optocoupler added for flywheel RPM sensing.
static constexpr gpio_num_t PIN_RPM_OPTO = GPIO_NUM_17;

// HALMET digital inputs D1-D4
// pin map and the Hat Labs HALMET GPIO reference.
static constexpr gpio_num_t PIN_D1 = GPIO_NUM_23;
static constexpr gpio_num_t PIN_D2 = GPIO_NUM_25;
static constexpr gpio_num_t PIN_D3 = GPIO_NUM_27;
static constexpr gpio_num_t PIN_D4 = GPIO_NUM_26;

// HALMET I2C and NMEA 2000 CAN pins.
static constexpr gpio_num_t PIN_I2C_SDA = GPIO_NUM_21;
static constexpr gpio_num_t PIN_I2C_SCL = GPIO_NUM_22;
static constexpr gpio_num_t PIN_CAN_RX = GPIO_NUM_18;
static constexpr gpio_num_t PIN_CAN_TX = GPIO_NUM_19;

// HALMET 1-Wire connector. Confirm against the exact board revision before
// final installation.
static constexpr gpio_num_t PIN_ONEWIRE = GPIO_NUM_4;

}  // namespace halmet_yanmar
