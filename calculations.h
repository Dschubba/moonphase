#pragma once

#include <optional>
#include <string>
#include <vector>

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

    enum class MoonSpecialEventType {
        MonthlyBlueMoon,
        SeasonalBlueMoon,
        Supermoon,
        PenumbralLunarEclipse,
        PartialLunarEclipse,
        TotalLunarEclipse
    };

    struct MoonSpecialEvent {
        MoonSpecialEventType type;
        double julianDay;
    };

    struct CalculationResult {
        Phase phase;
        double ageDays;
        double illuminationPercent;
        double altitudeDeg;
        double distanceKm;
        double sunAltitudeDeg;
        double sunDistanceKm;
        std::optional<double> nextMoonriseJulianDay;
        std::optional<double> nextMoonsetJulianDay;
        std::optional<double> nextSunriseJulianDay;
        std::optional<double> nextSunsetJulianDay;
        double daylightSeconds;
        double nightSeconds;
        double daylightChangeSeconds;
        double nightChangeSeconds;
        std::vector<MoonSpecialEvent> moonSpecialEvents;
    };

    CalculationResult calculate(double julianDay, const ObserverLocation& observer);
    CalculationResult calculate(double julianDay);
}
