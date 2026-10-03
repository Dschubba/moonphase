#pragma once

#include <string_view>

namespace moonphase {
    inline constexpr double kSynodicMonth = 29.530588853;
    inline constexpr double kJulianDayAtUnixEpoch = 2440587.5;
    inline constexpr double kPi = 3.14159265358979323846;

    // Beobachterposition: München
    inline constexpr std::string_view kObserverLocation = "München";
    inline constexpr double kObserverLatitudeDeg = 47.135125;
    inline constexpr double kObserverLongitudeDeg = 11.581981;
    inline constexpr double kObserverElevationKm = 0.519; // 519 m

    inline constexpr double kEarthRadiusKm = 6378.137;
    inline constexpr double kEarthFlattening = 1.0 / 298.257223563;
    inline constexpr double kAstronomicalUnitKm = 149597870.7;
    inline constexpr double kMoonRadiusKm = 1737.4;
    inline constexpr double kHorizonRefractionDeg = 34.0 / 60.0;
    inline constexpr double kRiseSetSearchDays = 35.0;
    inline constexpr double kRiseSetStepDays = 10.0 / 1440.0;

    inline constexpr double kNewMoonEnd = 1.84566;
    inline constexpr double kWaxingCrescentEnd = 5.53699;
    inline constexpr double kFirstQuarterEnd = 9.22831;
    inline constexpr double kWaxingGibbousEnd = 12.91963;
    inline constexpr double kFullMoonEnd = 16.61096;
    inline constexpr double kWaningGibbousEnd = 20.30228;
    inline constexpr double kLastQuarterEnd = 23.99361;
    inline constexpr double kWaningCrescentEnd = 27.68493;
}
