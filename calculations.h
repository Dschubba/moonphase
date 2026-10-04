#pragma once

#include <optional>
#include <string>

namespace moonphase {
    struct ObserverLocation {
        std::string name;
        double latitudeDeg;
        double longitudeDeg;
        double elevationMeters;
    };

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

    CalculationResult calculate(double julianDay, const ObserverLocation& observer);
    CalculationResult calculate(double julianDay);
}
