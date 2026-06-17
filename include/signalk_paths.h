#pragma once

namespace halmet_yanmar::signalk_paths {

static constexpr const char* kEngineBase = "propulsion.main";
static constexpr const char* kEngineRevolutions = "propulsion.main.revolutions";
static constexpr const char* kEngineState = "propulsion.main.state";
static constexpr const char* kCoolantTemperature =
    "propulsion.main.coolantTemperature";
static constexpr const char* kOilPressure = "propulsion.main.oilPressure";
static constexpr const char* kAlternatorVoltage =
    "propulsion.main.alternatorVoltage";
static constexpr const char* kFuelLevel = "tanks.fuel.main.currentLevel";
static constexpr const char* kEngineRoomTemperature =
    "environment.inside.engineRoom.temperature";
static constexpr const char* kExhaustTemperature =
    "propulsion.main.exhaustTemperature";
static constexpr const char* kTransmissionOilTemperature =
    "propulsion.main.transmission.oilTemperature";
static constexpr const char* kAlternatorTemperature =
    "electrical.alternators.main.temperature";

}  // namespace halmet_yanmar::signalk_paths
