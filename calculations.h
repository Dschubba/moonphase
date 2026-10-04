#pragma once

#include <optional>

namespace moonphase {
    struct Phase {
        const char* germanName;
        const char* englishName;
        const char* symbol;
    };

    struct CalculationResult {
        Phase phase;
        double ageDays;
        double illuminationPercent;
        double altitudeDeg;
        double distanceKm;
        std::optional<double> nextMoonriseJulianDay;
        std::optional<double> nextMoonsetJulianDay;
    };

    CalculationResult calculate(double julianDay);
}
