#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <locale>
#include <optional>
#include <string>

#include "calculations.h"
#include "main.h"

namespace {
    using namespace moonphase;

    std::optional<std::tm> localTimeAt(double julianDay) {
        const double unixSeconds = (julianDay - kJulianDayAtUnixEpoch) * 86400.0;
        const auto eventTime = static_cast<std::time_t>(unixSeconds);
        const std::tm* localTime = std::localtime(&eventTime);
        if (localTime == nullptr) {
            return std::nullopt;
        }
        return *localTime;
    }

    const char* localizedText(bool english, const char* german, const char* englishText) {
        return english ? englishText : german;
    }
}

int main() {
    const std::locale systemLocale("");
    std::locale::global(systemLocale);
    std::cout.imbue(systemLocale);
    std::cerr.imbue(systemLocale);
    const std::string localeName = systemLocale.name();
    const bool english = localeName.rfind("de", 0) != 0 &&
                         localeName.rfind("German", 0) != 0;

    const auto now = std::chrono::system_clock::now();
    const double unixDays =
        std::chrono::duration<double>(now.time_since_epoch()).count() / 86400.0;
    const double julianDay = kJulianDayAtUnixEpoch + unixDays;
    const CalculationResult result = calculate(julianDay);

    const std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    const std::tm* localDatePointer = std::localtime(&currentTime);
    if (localDatePointer == nullptr) {
        std::cerr << localizedText(
            english, "Das aktuelle Datum konnte nicht ermittelt werden.",
            "The current date could not be determined.") << "\n";
        return 1;
    }
    const std::tm localDate = *localDatePointer;

    const char* latitudeDirection = kObserverLatitudeDeg < 0.0 ? "S" : "N";
    const char* longitudeDirection = kObserverLongitudeDeg < 0.0
        ? "W"
        : localizedText(english, "O", "E");
    std::cout << localizedText(english, "Mondphase am ", "Moon phase on ")
              << std::put_time(&localDate, "%x %X") << "\n"
              << localizedText(english, "Beobachtungsort: ", "Location: ")
              << kObserverLocation << " ("
              << std::fixed << std::setprecision(4) << std::abs(kObserverLatitudeDeg)
              << "° " << latitudeDirection << ", "
              << std::abs(kObserverLongitudeDeg) << "° " << longitudeDirection << ")\n"
              << localizedText(english, "Aktuelle Mondphase: ", "Current moon phase: ")
              << result.phase.symbol << " "
              << localizedText(english, result.phase.germanName, result.phase.englishName)
              << "\n"
              << std::setprecision(1)
              << localizedText(english, "Beleuchtung: ", "Illumination: ")
              << result.illuminationPercent << " %\n"
              << localizedText(english, "Höhe über dem Horizont: ",
                               "Altitude above horizon: ")
              << result.altitudeDeg << "° ("
              << localizedText(english,
                               result.altitudeDeg > 0.0 ? "sichtbar" : "unter dem Horizont",
                               result.altitudeDeg > 0.0 ? "visible" : "below the horizon")
              << ")\n"
              << localizedText(english, "Alter: ", "Age: ") << result.ageDays << " "
              << localizedText(english, "Tage seit Neumond", "days since new moon") << "\n"
              << localizedText(english, "Entfernung: ", "Distance: ")
              << std::fixed << std::setprecision(0) << result.distanceKm << " km\n";

    const auto printEvent = [english](const char* germanLabel, const char* englishLabel,
                                      const std::optional<double>& eventDay) {
        std::cout << localizedText(english, germanLabel, englishLabel);
        if (!eventDay) {
            std::cout << localizedText(english, "nicht in den nächsten 35 Tagen",
                                       "not within the next 35 days") << "\n";
            return;
        }
        const std::optional<std::tm> eventTime = localTimeAt(*eventDay);
        if (!eventTime) {
            std::cout << localizedText(english, "Zeitpunkt nicht darstellbar",
                                       "time could not be represented") << "\n";
            return;
        }
        std::cout << std::put_time(&*eventTime, "%x %X") << "\n";
    };

    printEvent("Mondaufgang: ", "Moonrise: ", result.nextMoonriseJulianDay);
    printEvent("Monduntergang: ", "Moonset: ", result.nextMoonsetJulianDay);
}
