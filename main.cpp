#include <chrono>
#include <charconv>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <locale>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

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

    std::string_view trim(std::string_view value) {
        const std::size_t first = value.find_first_not_of(" \t");
        if (first == std::string_view::npos) {
            return {};
        }
        const std::size_t last = value.find_last_not_of(" \t");
        return value.substr(first, last - first + 1);
    }

    std::optional<double> parseNumber(std::string_view value) {
        value = trim(value);
        if (value.empty()) {
            return std::nullopt;
        }
        if (value.front() == '+') {
            value.remove_prefix(1);
            if (value.empty()) {
                return std::nullopt;
            }
        }
        double number = 0.0;
        const char* begin = value.data();
        const char* end = begin + value.size();
        const auto result = std::from_chars(begin, end, number);
        if (result.ec != std::errc{} || result.ptr != end || !std::isfinite(number)) {
            return std::nullopt;
        }
        return number;
    }

    std::optional<ObserverLocation> parseLocation(std::string_view value) {
        std::vector<std::string_view> fields;
        std::size_t start = 0;
        while (true) {
            const std::size_t comma = value.find(',', start);
            fields.push_back(value.substr(
                start, comma == std::string_view::npos ? comma : comma - start));
            if (comma == std::string_view::npos) {
                break;
            }
            start = comma + 1;
        }
        if (fields.size() < 3 || fields.size() > 4) {
            return std::nullopt;
        }

        const std::string_view name = trim(fields[0]);
        const std::optional<double> latitude = parseNumber(fields[1]);
        const std::optional<double> longitude = parseNumber(fields[2]);
        const std::optional<double> elevation = fields.size() == 4
            ? parseNumber(fields[3])
            : std::optional<double>(0.0);
        if (name.empty() || !latitude || !longitude || !elevation ||
            *latitude < -90.0 || *latitude > 90.0 ||
            *longitude < -180.0 || *longitude > 180.0 || *elevation < 0.0) {
            return std::nullopt;
        }

        return ObserverLocation{std::string(name), *latitude, *longitude, *elevation};
    }
}

int main(int argc, char* argv[]) {
    const std::locale systemLocale("");
    std::locale::global(systemLocale);
    std::cout.imbue(systemLocale);
    std::cerr.imbue(systemLocale);
    const std::string localeName = systemLocale.name();
    const bool english = localeName.rfind("de", 0) != 0 &&
                         localeName.rfind("German", 0) != 0;

    std::vector<ObserverLocation> locations;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) != "--location" || i + 1 >= argc) {
            std::cerr << localizedText(
                english,
                "Verwendung: moonphase [--location \"Name,Breitengrad,Längengrad[,Höhe_m]\"]...\n",
                "Usage: moonphase [--location \"Name,latitude,longitude[,elevation_m]\"]...\n");
            return 1;
        }
        const std::optional<ObserverLocation> location = parseLocation(argv[++i]);
        if (!location) {
            std::cerr << localizedText(
                english,
                "Ungültiger Standort. Erwartet: Name,Breitengrad,Längengrad[,Höhe_m] "
                "(Breitengrad -90 bis 90, Längengrad -180 bis 180, Höhe mindestens 0 m).\n",
                "Invalid location. Expected: Name,latitude,longitude[,elevation_m] "
                "(latitude -90 to 90, longitude -180 to 180, elevation at least 0 m).\n");
            return 1;
        }
        locations.push_back(*location);
    }
    if (locations.empty()) {
        locations.push_back({std::string(kObserverLocation), kObserverLatitudeDeg,
                             kObserverLongitudeDeg, kObserverElevationKm * 1000.0});
    }

    const auto now = std::chrono::system_clock::now();
    const double unixDays =
        std::chrono::duration<double>(now.time_since_epoch()).count() / 86400.0;
    const double julianDay = kJulianDayAtUnixEpoch + unixDays;

    const std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    const std::tm* localDatePointer = std::localtime(&currentTime);
    if (localDatePointer == nullptr) {
        std::cerr << localizedText(
            english, "Das aktuelle Datum konnte nicht ermittelt werden.",
            "The current date could not be determined.") << "\n";
        return 1;
    }
    const std::tm localDate = *localDatePointer;

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

    const auto printDuration = [english](const char* germanLabel, const char* englishLabel,
                                         double seconds, double changeSeconds) {
        const long long totalSeconds = std::llround(seconds);
        const long long changeTotalSeconds = std::llround(changeSeconds);
        const long long durationMinutes = totalSeconds / 60;
        const long long absoluteChange = std::abs(changeTotalSeconds);
        const auto printDifference = [english](long long value) {
            std::cout << value / 60 << " "
                      << localizedText(english, "Min. ", "min. ")
                      << std::setfill('0') << std::setw(2) << value % 60
                      << localizedText(english, " Sek.", " sec.")
                      << std::setfill(' ');
        };
        std::cout << localizedText(english, germanLabel, englishLabel);
        std::cout << std::setfill('0') << std::setw(2) << durationMinutes / 60 << ":"
                  << std::setw(2) << durationMinutes % 60 << " h ("
                  << localizedText(english, "zum Vortag: ", "vs. previous day: ")
                  << (changeTotalSeconds > 0 ? "+" : changeTotalSeconds < 0 ? "-" : "");
        printDifference(absoluteChange);
        std::cout << ")\n" << std::setfill(' ');
    };

    for (std::size_t i = 0; i < locations.size(); ++i) {
        const ObserverLocation& location = locations[i];
        const auto
        [phase, ageDays, illuminationPercent, altitudeDeg, distanceKm,
         sunAltitudeDeg, sunDistanceKm, nextMoonriseJulianDay, nextMoonsetJulianDay,
         nextSunriseJulianDay, nextSunsetJulianDay, daylightSeconds, nightSeconds,
         daylightChangeSeconds, nightChangeSeconds, moonSpecialEvents] =
            calculate(julianDay, location);
        const char* latitudeDirection = location.latitudeDeg < 0.0 ? "S" : "N";
        const char* longitudeDirection = location.longitudeDeg < 0.0
            ? "W"
            : localizedText(english, "O", "E");
        if (i > 0) {
            std::cout << "\n";
        }
        std::cout << localizedText(english, "Mondphase am ", "Moon phase on ")
                  << std::put_time(&localDate, "%x %X") << "\n"
                  << localizedText(english, "Beobachtungsort: ", "Location: ")
                  << location.name << " ("
                  << std::fixed << std::setprecision(4) << std::abs(location.latitudeDeg)
                  << "° " << latitudeDirection << ", "
                  << std::abs(location.longitudeDeg) << "° " << longitudeDirection << ")\n"
                  << localizedText(english, "Aktuelle Mondphase: ", "Current moon phase: ")
                  << phase.symbol << " "
                  << localizedText(english, phase.germanName, phase.englishName)
                  << "\n"
                  << std::setprecision(1)
                  << localizedText(english, "Beleuchtung: ", "Illumination: ")
                  << illuminationPercent << " %\n"
                  << localizedText(english, "Höhe über dem Horizont: ",
                                   "Altitude above horizon: ")
                  << altitudeDeg << "° ("
                  << localizedText(english,
                                   altitudeDeg > 0.0
                                       ? "sichtbar"
                                       : "unter dem Horizont",
                                   altitudeDeg > 0.0 ? "visible" : "below the horizon")
                  << ")\n"
                  << localizedText(english, "Alter: ", "Age: ") << ageDays << " "
                  << localizedText(english, "Tage seit Neumond", "days since new moon") << "\n"
                  << localizedText(english, "Entfernung: ", "Distance: ")
                  << std::fixed << std::setprecision(0) << distanceKm << " km\n";

        printEvent("Mondaufgang: ", "Moonrise: ", nextMoonriseJulianDay);
        printEvent("Monduntergang: ", "Moonset: ", nextMoonsetJulianDay);
        std::cout << localizedText(english, "Mondbesonderheiten im aktuellen Monat:\n",
                                   "Moon highlights this month:\n");
        if (moonSpecialEvents.empty()) {
            std::cout << localizedText(english, "Keine besonderen Ereignisse.\n",
                                       "No special events.\n");
        }
        for (const MoonSpecialEvent& event : moonSpecialEvents) {
            const std::optional<std::tm> eventTime = localTimeAt(event.julianDay);
            if (!eventTime) {
                std::cout << localizedText(english, "Ereigniszeit nicht darstellbar\n",
                                           "Event time could not be represented\n");
                continue;
            }
            const char* eventName = "";
            switch (event.type) {
                case MoonSpecialEventType::MonthlyBlueMoon:
                    eventName = localizedText(english, "Blue Moon (zweiter Vollmond des Monats)",
                                              "Blue Moon (second full moon of the month)");
                    break;
                case MoonSpecialEventType::SeasonalBlueMoon:
                    eventName = localizedText(english, "Blue Moon (dritter Vollmond der Jahreszeit)",
                                              "Blue Moon (third full moon of the season)");
                    break;
                case MoonSpecialEventType::Supermoon:
                    eventName = localizedText(english, "Supermond",
                                              "Supermoon");
                    break;
                case MoonSpecialEventType::PenumbralLunarEclipse:
                    eventName = localizedText(english, "Halbschatten-Mondfinsternis",
                                              "Penumbral lunar eclipse");
                    break;
                case MoonSpecialEventType::PartialLunarEclipse:
                    eventName = localizedText(english, "Partielle Mondfinsternis",
                                              "Partial lunar eclipse");
                    break;
                case MoonSpecialEventType::TotalLunarEclipse:
                    eventName = localizedText(english, "Totale Mondfinsternis",
                                              "Total lunar eclipse");
                    break;
            }
            std::cout << "  " << std::put_time(&*eventTime, "%x %X")
                      << " - " << eventName << "\n";
        }
        std::cout << "\n" << localizedText(english, "Sonnenhöhe: ", "Solar altitude: ")
                  << std::setprecision(1) << sunAltitudeDeg << "°\n"
                  << localizedText(english, "Sonnenentfernung: ", "Solar distance: ")
                  << std::fixed << std::setprecision(0) << sunDistanceKm << " km\n";
        printEvent("Sonnenaufgang: ", "Sunrise: ", nextSunriseJulianDay);
        printEvent("Sonnenuntergang: ", "Sunset: ", nextSunsetJulianDay);
        printDuration("Tageslänge: ", "Daylight duration: ", daylightSeconds,
                      daylightChangeSeconds);
        printDuration("Nachtlänge: ", "Night duration: ", nightSeconds,
                      nightChangeSeconds);
    }
}
