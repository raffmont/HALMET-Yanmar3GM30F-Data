#pragma once

#include <Arduino.h>

#include "board_pins.h"
#include "signalk_paths.h"

namespace halmet_yanmar {

// -----------------------------------------------------------------------------
// Device identity
// -----------------------------------------------------------------------------
static constexpr const char* HOSTNAME = "halmet-yanmar3gm30f-data";
static constexpr const char* APP_NAME = "HALMET Yanmar 3GM30F Data";
static constexpr const char* MODEL_SERIAL_CODE = "HALMET-YANMAR3GM30F-0001";
static constexpr const char* MODEL_ID = "HALMET-Yanmar3GM30F-Data";
static constexpr const char* SOFTWARE_VERSION = "0.3.0";
static constexpr const char* MODEL_VERSION = "0.3";

// -----------------------------------------------------------------------------
// HALMET peripheral configuration
// -----------------------------------------------------------------------------
static constexpr uint8_t ADS1115_ADDR = 0x4b;

// -----------------------------------------------------------------------------
// Sampling and filtering
// -----------------------------------------------------------------------------
static constexpr uint32_t PUBLISH_INTERVAL_MS = 1000;
static constexpr uint32_t DIGITAL_DEBOUNCE_MS = 50;
static constexpr uint32_t RPM_GATE_MS = 1000;
static constexpr uint32_t RPM_MIN_EDGE_US = 1500;
static constexpr uint32_t RPM_STALE_MS = 3000;
static constexpr uint8_t ANALOG_SAMPLES = 8;
static constexpr uint32_t ONEWIRE_INTERVAL_MS = 5000;

// Count of optocoupler pulses per crankshaft revolution.
// 1.0 = one reflective mark / magnet / slot per flywheel revolution.
static constexpr float RPM_PULSES_PER_REV = 1.0f;

// Yanmar panel lamp/alarm wires commonly switch toward ground, but this must be
// measured on the specific panel harness.
static constexpr bool PANEL_ACTIVE_LOW = true;

// HALMET front-end scale used by the board: connector range 0..32 V via ADS1115.
static constexpr float HALMET_ANALOG_CONNECTOR_MAX_V = 32.0f;
static constexpr float ADS1115_FULL_SCALE_V = 4.096f;

// -----------------------------------------------------------------------------
// NMEA 2000
// -----------------------------------------------------------------------------
static constexpr bool ENABLE_NMEA2000_OUTPUT = true;
static constexpr uint8_t N2K_DEFAULT_NODE_ADDRESS = 71;
static constexpr uint8_t N2K_ENGINE_INSTANCE = 0;
static constexpr uint8_t N2K_FUEL_TANK_INSTANCE = 0;
static constexpr double N2K_FUEL_TANK_CAPACITY_L = 200.0;

// -----------------------------------------------------------------------------
// Sensor mapping
// -----------------------------------------------------------------------------
struct DigitalChannelConfig {
  gpio_num_t pin;
  const char* name;
  const char* description;
};

static constexpr DigitalChannelConfig DIGITAL_CHANNELS[] = {
    {PIN_D1, "low_oil_pressure",
     "Yanmar panel low oil pressure warning input"},
    {PIN_D2, "high_coolant_temperature",
     "Yanmar panel high coolant temperature warning input"},
    {PIN_D3, "charge_indicator",
     "Yanmar panel alternator charge warning lamp input"},
    {PIN_D4, "key_or_aux",
     "Configurable type-B panel signal: key-on, buzzer, start, or spare"},
};

struct AnalogChannelConfig {
  uint8_t adc_channel;
  const char* name;
  const char* sk_path;
  const char* config_path;
  const char* units;
  const char* description;
  float input_min_v;
  float input_max_v;
  float output_min;
  float output_max;
};

// All calibrated paths below are Signal K schema paths with SI units.
static constexpr AnalogChannelConfig ANALOG_CHANNELS[] = {
    {0, "fuel_tank_level", signalk_paths::kFuelLevel,
     "/Yanmar3GM30F/Analog/A1 Fuel Level", "ratio",
     "Fuel tank level on A1, calibrated to 0..1", 0.50f, 4.50f, 0.0f, 1.0f},
    {1, "coolant_temperature", signalk_paths::kCoolantTemperature,
     "/Yanmar3GM30F/Analog/A2 Coolant Temperature", "K",
     "Coolant temperature sender on A2, calibrated to kelvin", 0.50f, 4.50f,
     293.15f, 393.15f},
    {2, "oil_pressure", signalk_paths::kOilPressure,
     "/Yanmar3GM30F/Analog/A3 Oil Pressure", "Pa",
     "Oil pressure sender on A3, calibrated to pascal", 0.50f, 4.50f, 0.0f,
     600000.0f},
    {3, "alternator_voltage", signalk_paths::kAlternatorVoltage,
     "/Yanmar3GM30F/Analog/A4 Alternator Voltage", "V",
     "Alternator B+ or charging voltage on A4", 0.0f, 32.0f, 0.0f, 32.0f},
};

struct TemperatureChannelConfig {
  const char* rom_id;   // 16 hex chars, e.g. "28FF641E621603A1"; empty = by index
  const char* sk_path;  // Valid Signal K temperature path
  const char* description;
};

static constexpr TemperatureChannelConfig TEMPERATURE_CHANNEL_HINTS[] = {
    {"", signalk_paths::kEngineRoomTemperature, "Engine-room air temperature"},
    {"", signalk_paths::kExhaustTemperature,
     "Exhaust elbow surface proxy temperature"},
    {"", signalk_paths::kTransmissionOilTemperature,
     "Gearbox case surface temperature"},
    {"", signalk_paths::kAlternatorTemperature, "Alternator case temperature"},
};

}  // namespace halmet_yanmar
