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

// HALMET vendor extension diagnostics. These are deliberately kept out of
// the standard propulsion.* schema branches.
static constexpr const char* kVibrationBase =
    "halmet.yanmar3gm30f.vibration";
static constexpr const char* kVibrationAccelerationRms =
    "halmet.yanmar3gm30f.vibration.accelerationRms";
static constexpr const char* kVibrationPeakAcceleration =
    "halmet.yanmar3gm30f.vibration.peakAcceleration";
static constexpr const char* kVibrationPeakFrequency =
    "halmet.yanmar3gm30f.vibration.peakFrequency";
static constexpr const char* kVibrationCrankOrderAmplitude =
    "halmet.yanmar3gm30f.vibration.crankOrderAmplitude";
static constexpr const char* kVibrationFiringOrderAmplitude =
    "halmet.yanmar3gm30f.vibration.firingOrderAmplitude";
static constexpr const char* kVibrationSampleRate =
    "halmet.yanmar3gm30f.vibration.sampleRate";
static constexpr const char* kVibrationFftSize =
    "halmet.yanmar3gm30f.vibration.fftSize";
static constexpr const char* kVibrationStatus =
    "halmet.yanmar3gm30f.vibration.status";
static constexpr const char* kVibrationSpectrum =
    "halmet.yanmar3gm30f.vibration.spectrum";

}  // namespace halmet_yanmar::signalk_paths
