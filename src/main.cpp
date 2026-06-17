#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>
#include <cstring>

#define ESP32_CAN_TX_PIN halmet_yanmar::PIN_CAN_TX
#define ESP32_CAN_RX_PIN halmet_yanmar::PIN_CAN_RX

#include <NMEA2000_esp32.h>
#include <N2kMessages.h>
#include <N2kMsg.h>

#include "sensesp/signalk/signalk_output.h"
#include "sensesp_app_builder.h"

#include "config.h"

using namespace halmet_yanmar;
using namespace sensesp;

Adafruit_ADS1115 ads;
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET_PIN);
OneWire one_wire(static_cast<uint8_t>(PIN_ONEWIRE));
DallasTemperature temp_bus(&one_wire);
tNMEA2000_esp32* nmea2000 = nullptr;

volatile uint32_t rpm_pulses = 0;
volatile uint32_t last_rpm_edge_us = 0;
uint32_t last_rpm_gate_ms = 0;
uint32_t last_rpm_pulses = 0;
uint32_t last_publish_ms = 0;
uint32_t last_onewire_ms = 0;
uint32_t last_n2k_rapid_ms = 0;
uint32_t last_n2k_dynamic_ms = 0;
uint32_t last_n2k_transmission_ms = 0;
uint32_t last_n2k_fluid_ms = 0;
float current_rpm = 0.0f;
bool ads_available = false;
bool oled_available = false;
bool n2k_available = false;

float analog_values[sizeof(ANALOG_CHANNELS) / sizeof(ANALOG_CHANNELS[0])] = {};
float analog_voltages[sizeof(ANALOG_CHANNELS) / sizeof(ANALOG_CHANNELS[0])] = {};

SKOutput<float>* engine_revolutions_hz_output = nullptr;
SKOutput<String>* engine_state_output = nullptr;
SKOutput<float>* analog_outputs[sizeof(ANALOG_CHANNELS) / sizeof(ANALOG_CHANNELS[0])] = {};

struct DebouncedInputState {
  bool raw = false;
  bool stable = false;
  bool last_raw = false;
  uint32_t changed_ms = 0;
};

DebouncedInputState digital_states[sizeof(DIGITAL_CHANNELS) /
                                   sizeof(DIGITAL_CHANNELS[0])];

struct TempOutputState {
  DeviceAddress address;
  char rom_id[17];
  String sk_path;
  String config_path;
  String description;
  SKOutput<float>* output = nullptr;
  float kelvin = NAN;
};

static constexpr size_t MAX_ONEWIRE_TEMPS = 8;
TempOutputState temp_outputs[MAX_ONEWIRE_TEMPS];
size_t temp_output_count = 0;

void IRAM_ATTR rpm_isr() {
  const uint32_t now = micros();
  if (now - last_rpm_edge_us >= RPM_MIN_EDGE_US) {
    rpm_pulses++;
    last_rpm_edge_us = now;
  }
}

String address_to_rom_id(const DeviceAddress& address) {
  char id[17];
  for (uint8_t i = 0; i < 8; i++) {
    snprintf(&id[i * 2], 3, "%02X", address[i]);
  }
  id[16] = '\0';
  return String(id);
}

const TemperatureChannelConfig* find_temperature_config(const String& rom_id,
                                                        uint8_t index) {
  for (const auto& hint : TEMPERATURE_CHANNEL_HINTS) {
    if (strlen(hint.rom_id) > 0 && rom_id.equalsIgnoreCase(hint.rom_id)) {
      return &hint;
    }
  }

  if (index < sizeof(TEMPERATURE_CHANNEL_HINTS) /
                  sizeof(TEMPERATURE_CHANNEL_HINTS[0])) {
    return &TEMPERATURE_CHANNEL_HINTS[index];
  }

  return nullptr;
}

bool read_panel_signal(gpio_num_t pin) {
  const bool level = digitalRead(static_cast<uint8_t>(pin));
  return PANEL_ACTIVE_LOW ? !level : level;
}

void update_digital_inputs() {
  const uint32_t now = millis();
  for (size_t i = 0; i < sizeof(DIGITAL_CHANNELS) / sizeof(DIGITAL_CHANNELS[0]);
       i++) {
    auto& state = digital_states[i];
    state.raw = read_panel_signal(DIGITAL_CHANNELS[i].pin);

    if (state.raw != state.last_raw) {
      state.last_raw = state.raw;
      state.changed_ms = now;
    }

    if ((now - state.changed_ms) >= DIGITAL_DEBOUNCE_MS) {
      state.stable = state.raw;
    }
  }
}

void update_rpm() {
  const uint32_t now_ms = millis();
  if (now_ms - last_rpm_gate_ms < RPM_GATE_MS) {
    return;
  }

  uint32_t pulses;
  uint32_t last_edge_us;
  noInterrupts();
  pulses = rpm_pulses;
  last_edge_us = last_rpm_edge_us;
  interrupts();

  const uint32_t delta = pulses - last_rpm_pulses;
  const float gate_s = (now_ms - last_rpm_gate_ms) / 1000.0f;

  if (delta == 0 && (micros() - last_edge_us) > RPM_STALE_MS * 1000UL) {
    current_rpm = 0.0f;
  } else if (gate_s > 0.0f && RPM_PULSES_PER_REV > 0.0f) {
    current_rpm = (delta / RPM_PULSES_PER_REV) * 60.0f / gate_s;
  }

  last_rpm_pulses = pulses;
  last_rpm_gate_ms = now_ms;
}

float ads_counts_to_halmet_voltage(int16_t counts) {
  const float adc_v = counts * ADS1115_FULL_SCALE_V / 32768.0f;
  return adc_v * (HALMET_ANALOG_CONNECTOR_MAX_V / ADS1115_FULL_SCALE_V);
}

float read_analog_voltage(uint8_t channel) {
  if (!ads_available) {
    return NAN;
  }

  int32_t sum = 0;
  for (uint8_t i = 0; i < ANALOG_SAMPLES; i++) {
    sum += ads.readADC_SingleEnded(channel);
  }

  const int16_t avg = static_cast<int16_t>(sum / ANALOG_SAMPLES);
  return ads_counts_to_halmet_voltage(avg);
}

float linear_calibrate(float input_v, const AnalogChannelConfig& cal) {
  if (!isfinite(input_v) || fabsf(cal.input_max_v - cal.input_min_v) < 0.0001f) {
    return NAN;
  }

  const float normalized =
      (input_v - cal.input_min_v) / (cal.input_max_v - cal.input_min_v);
  const float clamped = constrain(normalized, 0.0f, 1.0f);
  return cal.output_min + clamped * (cal.output_max - cal.output_min);
}

uint32_t n2k_unique_number() {
  return static_cast<uint32_t>(ESP.getEfuseMac() & 0x1fffffUL);
}

void setup_nmea2000() {
  if (!ENABLE_NMEA2000_OUTPUT) {
    return;
  }

  nmea2000 = new tNMEA2000_esp32(PIN_CAN_TX, PIN_CAN_RX);
  nmea2000->SetN2kCANSendFrameBufSize(250);
  nmea2000->SetN2kCANReceiveFrameBufSize(250);
  nmea2000->SetProductInformation(MODEL_SERIAL_CODE, 104, MODEL_ID,
                                  SOFTWARE_VERSION, MODEL_VERSION);
  nmea2000->SetDeviceInformation(n2k_unique_number(), 140, 50, 2046);
  nmea2000->SetMode(tNMEA2000::N2km_NodeOnly, N2K_DEFAULT_NODE_ADDRESS);
  nmea2000->EnableForward(false);
  nmea2000->Open();
  n2k_available = true;

  event_loop()->onRepeat(1, []() {
    if (nmea2000 != nullptr) {
      nmea2000->ParseMessages();
    }
  });
}

void discover_onewire_temperatures() {
  temp_bus.begin();
  temp_bus.setResolution(12);
  temp_bus.setWaitForConversion(false);

  const uint8_t count = temp_bus.getDeviceCount();
  temp_output_count = (static_cast<size_t>(count) < MAX_ONEWIRE_TEMPS)
                          ? static_cast<size_t>(count)
                          : MAX_ONEWIRE_TEMPS;

  for (size_t i = 0; i < temp_output_count; i++) {
    if (!temp_bus.getAddress(temp_outputs[i].address, i)) {
      temp_outputs[i].output = nullptr;
      continue;
    }

    String rom_id = address_to_rom_id(temp_outputs[i].address);
    strncpy(temp_outputs[i].rom_id, rom_id.c_str(), sizeof(temp_outputs[i].rom_id));
    temp_outputs[i].rom_id[16] = '\0';

    const auto* config = find_temperature_config(rom_id, i);
    if (config == nullptr) {
      temp_outputs[i].output = nullptr;
      Serial.printf("1-Wire sensor %u: %s ignored; no valid Signal K path configured\n",
                    static_cast<unsigned>(i + 1), rom_id.c_str());
      continue;
    }

    temp_outputs[i].sk_path = String(config->sk_path);
    temp_outputs[i].config_path = String("/Yanmar3GM30F/1Wire/") + temp_outputs[i].sk_path;
    temp_outputs[i].description = String(config->description) + " (DS18B20 " + rom_id + ")";

    temp_outputs[i].output = new SKOutput<float>(
        temp_outputs[i].sk_path.c_str(), temp_outputs[i].config_path.c_str(),
        new SKMetadata("K", temp_outputs[i].description.c_str()));
    ConfigItem(temp_outputs[i].output)
        ->set_title(temp_outputs[i].description)
        ->set_sort_order(1200 + static_cast<int>(i));

    Serial.printf("1-Wire sensor %u: %s -> %s\n", static_cast<unsigned>(i + 1),
                  rom_id.c_str(), temp_outputs[i].sk_path.c_str());
  }
}

void setup_outputs() {
  engine_revolutions_hz_output = new SKOutput<float>(
      signalk_paths::kEngineRevolutions, "/Yanmar3GM30F/RPM/Revolutions",
      new SKMetadata("Hz", "Yanmar 3GM30F crankshaft revolutions per second"));
  ConfigItem(engine_revolutions_hz_output)
      ->set_title("Engine RPM Signal K path")
      ->set_sort_order(1000);

  engine_state_output = new SKOutput<String>(
      signalk_paths::kEngineState, "/Yanmar3GM30F/Engine/State",
      new SKMetadata("", "Yanmar 3GM30F engine state"));
  ConfigItem(engine_state_output)
      ->set_title("Engine state Signal K path")
      ->set_sort_order(1001);

  for (size_t i = 0; i < sizeof(ANALOG_CHANNELS) / sizeof(ANALOG_CHANNELS[0]);
       i++) {
    analog_outputs[i] = new SKOutput<float>(
        ANALOG_CHANNELS[i].sk_path, ANALOG_CHANNELS[i].config_path,
        new SKMetadata(ANALOG_CHANNELS[i].units, ANALOG_CHANNELS[i].description));
    ConfigItem(analog_outputs[i])
        ->set_title(String(ANALOG_CHANNELS[i].description) + " Signal K path")
        ->set_sort_order(1100 + static_cast<int>(i));
  }
}

void setup_display() {
  oled_available = oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR);
  if (!oled_available) {
    Serial.println("SSD1306 display not found; local display disabled");
    return;
  }

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println(APP_NAME);
  oled.println("Booting...");
  oled.display();
  Serial.println("SSD1306 display initialized");
}

String get_display_ip_address() {
  if (WiFi.isConnected()) {
    return WiFi.localIP().toString();
  }

  const wifi_mode_t wifi_mode = WiFi.getMode();
  if (wifi_mode == WIFI_MODE_AP || wifi_mode == WIFI_MODE_APSTA) {
    return WiFi.softAPIP().toString();
  }

  return String("--");
}

void update_display() {
  if (!oled_available) {
    return;
  }

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("Yanmar 3GM30F");
  oled.printf("IP: %s\n", get_display_ip_address().c_str());
  oled.printf("RPM: %.0f\n", current_rpm);
  oled.printf("State: %s\n", current_rpm > 50.0f ? "started" : "stopped");
  oled.printf("Oil:%d Temp:%d Alt:%d\n", digital_states[0].stable,
              digital_states[1].stable, digital_states[2].stable);
  oled.printf("SD20 seal: %s\n", digital_states[3].stable ? "ALARM" : "OK");

  if (isfinite(analog_values[0])) {
    oled.printf("Fuel: %.0f%%\n", analog_values[0] * 100.0f);
  } else {
    oled.println("Fuel: --");
  }

  if (isfinite(analog_values[1]) && isfinite(analog_values[2])) {
    oled.printf("C:%.0fC O:%.1fbar\n", analog_values[1] - 273.15f,
                analog_values[2] / 100000.0f);
  } else {
    oled.println("C:-- O:--");
  }

  oled.display();
}

void publish_outputs() {
  update_digital_inputs();
  update_rpm();

  if (engine_revolutions_hz_output != nullptr) {
    engine_revolutions_hz_output->set(current_rpm / 60.0f);
  }
  if (engine_state_output != nullptr) {
    engine_state_output->set((current_rpm > 50.0f) ? String("started")
                                                   : String("stopped"));
  }

  for (size_t i = 0; i < sizeof(ANALOG_CHANNELS) / sizeof(ANALOG_CHANNELS[0]);
       i++) {
    analog_voltages[i] = read_analog_voltage(ANALOG_CHANNELS[i].adc_channel);
    analog_values[i] = linear_calibrate(analog_voltages[i], ANALOG_CHANNELS[i]);

    if (analog_outputs[i] != nullptr && isfinite(analog_values[i])) {
      analog_outputs[i]->set(analog_values[i]);
    }
  }

  Serial.printf("rpm=%.1f d1_low_oil=%d d2_high_temp=%d d3_charge=%d d4_sd20_seal=%d\n",
                current_rpm, digital_states[0].stable, digital_states[1].stable,
                digital_states[2].stable, digital_states[3].stable);
  update_display();
}

void update_onewire_temperatures() {
  const uint32_t now = millis();
  if (now - last_onewire_ms < ONEWIRE_INTERVAL_MS) {
    return;
  }
  last_onewire_ms = now;

  temp_bus.requestTemperatures();

  for (size_t i = 0; i < temp_output_count; i++) {
    if (temp_outputs[i].output == nullptr) {
      continue;
    }

    const float celsius = temp_bus.getTempC(temp_outputs[i].address);
    if (celsius == DEVICE_DISCONNECTED_C || !isfinite(celsius)) {
      continue;
    }

    temp_outputs[i].kelvin = celsius + 273.15f;
    temp_outputs[i].output->set(temp_outputs[i].kelvin);
  }
}

void send_n2k_engine_rapid() {
  tN2kMsg msg;
  SetN2kEngineParamRapid(msg, N2K_ENGINE_INSTANCE, current_rpm, N2kDoubleNA,
                         N2kInt8NA);
  nmea2000->SendMsg(msg);
}

void send_n2k_engine_dynamic() {
  tN2kEngineDiscreteStatus1 status1 = 0;
  status1.Bits.LowOilPressure = digital_states[0].stable;
  status1.Bits.OverTemperature = digital_states[1].stable;
  status1.Bits.ChargeIndicator = digital_states[2].stable;
  status1.Bits.CheckEngine = status1.Bits.LowOilPressure ||
                             status1.Bits.OverTemperature ||
                             status1.Bits.ChargeIndicator;

  tN2kEngineDiscreteStatus2 status2 = 0;

  const double coolant_temp_k = isfinite(analog_values[1]) ? analog_values[1] : N2kDoubleNA;
  const double oil_pressure_pa = isfinite(analog_values[2]) ? analog_values[2] : N2kDoubleNA;
  const double alternator_v = isfinite(analog_values[3]) ? analog_values[3] : N2kDoubleNA;

  tN2kMsg msg;
  SetN2kEngineDynamicParam(msg, N2K_ENGINE_INSTANCE, oil_pressure_pa,
                           N2kDoubleNA, coolant_temp_k, alternator_v,
                           N2kDoubleNA, N2kDoubleNA, N2kDoubleNA, N2kDoubleNA,
                           N2kInt8NA, N2kInt8NA, status1, status2);
  nmea2000->SendMsg(msg);
}

void send_n2k_transmission_dynamic() {
  const bool sd20_seal_alarm = digital_states[3].stable;
  const double transmission_oil_temp_k =
      (temp_output_count > 2 && isfinite(temp_outputs[2].kelvin))
          ? temp_outputs[2].kelvin
          : N2kDoubleNA;

  tN2kMsg msg;
  SetN2kTransmissionParameters(msg, N2K_ENGINE_INSTANCE, N2kTG_Unknown,
                               N2kDoubleNA, transmission_oil_temp_k,
                               sd20_seal_alarm, false, false, false,
                               sd20_seal_alarm);
  nmea2000->SendMsg(msg);
}

void send_n2k_fluid_level() {
  const double tank_level_pct = isfinite(analog_values[0]) ? 100.0 * analog_values[0] : N2kDoubleNA;
  tN2kMsg msg;
  SetN2kFluidLevel(msg, N2K_FUEL_TANK_INSTANCE, N2kft_Fuel, tank_level_pct,
                   N2K_FUEL_TANK_CAPACITY_L);
  nmea2000->SendMsg(msg);
}

void update_nmea2000() {
  if (!n2k_available || nmea2000 == nullptr) {
    return;
  }

  const uint32_t now = millis();
  if (now - last_n2k_rapid_ms >= nmea2000_mapping::kEngineRapidPeriodMs) {
    last_n2k_rapid_ms = now;
    send_n2k_engine_rapid();
  }
  if (now - last_n2k_dynamic_ms >= nmea2000_mapping::kEngineDynamicPeriodMs) {
    last_n2k_dynamic_ms = now;
    send_n2k_engine_dynamic();
  }
  if (now - last_n2k_transmission_ms >=
      nmea2000_mapping::kTransmissionDynamicPeriodMs) {
    last_n2k_transmission_ms = now;
    send_n2k_transmission_dynamic();
  }
  if (now - last_n2k_fluid_ms >= nmea2000_mapping::kFluidLevelPeriodMs) {
    last_n2k_fluid_ms = now;
    send_n2k_fluid_level();
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.printf("%s booting\n", APP_NAME);

  SensESPAppBuilder builder;
  sensesp_app = (&builder)->set_hostname(HOSTNAME)->get_app();

  pinMode(static_cast<uint8_t>(PIN_RPM_OPTO), INPUT_PULLUP);
  attachInterrupt(static_cast<uint8_t>(PIN_RPM_OPTO), rpm_isr, FALLING);

  for (size_t i = 0; i < sizeof(DIGITAL_CHANNELS) / sizeof(DIGITAL_CHANNELS[0]);
       i++) {
    pinMode(static_cast<uint8_t>(DIGITAL_CHANNELS[i].pin), INPUT_PULLUP);
    digital_states[i].changed_ms = millis();
    digital_states[i].last_raw = read_panel_signal(DIGITAL_CHANNELS[i].pin);
    digital_states[i].stable = digital_states[i].last_raw;
  }

  Wire.begin(static_cast<int>(PIN_I2C_SDA), static_cast<int>(PIN_I2C_SCL));
  setup_display();
  ads_available = ads.begin(ADS1115_ADDR, &Wire);
  if (ads_available) {
    ads.setGain(GAIN_ONE);
    Serial.println("ADS1115 initialized");
  } else {
    Serial.println("ADS1115 not found; analog channels will not publish");
  }

  setup_outputs();
  discover_onewire_temperatures();
  setup_nmea2000();

  last_publish_ms = millis();
  last_rpm_gate_ms = millis();
  last_onewire_ms = millis() - ONEWIRE_INTERVAL_MS;
}

void loop() {
  const uint32_t now = millis();
  if (now - last_publish_ms >= PUBLISH_INTERVAL_MS) {
    last_publish_ms = now;
    publish_outputs();
  }

  update_onewire_temperatures();
  update_nmea2000();
  event_loop()->tick();
}
