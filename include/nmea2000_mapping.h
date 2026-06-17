#pragma once

namespace halmet_yanmar::nmea2000_mapping {

static constexpr unsigned long kEngineRapidPgn = 127488;
static constexpr unsigned long kEngineDynamicPgn = 127489;
static constexpr unsigned long kFluidLevelPgn = 127505;

static constexpr unsigned long kEngineRapidPeriodMs = 100;
static constexpr unsigned long kEngineDynamicPeriodMs = 500;
static constexpr unsigned long kFluidLevelPeriodMs = 2500;

}  // namespace halmet_yanmar::nmea2000_mapping
