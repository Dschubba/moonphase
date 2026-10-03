#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <locale>
#include <optional>
#include <string>

#include "main.h"

namespace {
    using namespace moonphase;

    using Vec3 = std::array<double, 3>;

    double normalize(double value, double period) {
        value = std::fmod(value, period);
        return value < 0.0 ? value + period : value;
    }

    double toRadians(double degrees) {
        return degrees * kPi / 180.0;
    }

    double toDegrees(double radians) {
        return radians * 180.0 / kPi;
    }

    double sinDeg(double degrees) {
        return std::sin(toRadians(degrees));
    }

    double cosDeg(double degrees) {
        return std::cos(toRadians(degrees));
    }

    double dot(const Vec3& a, const Vec3& b) {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    }

    double length(const Vec3& v) {
        return std::sqrt(dot(v, v));
    }

    Vec3 subtract(const Vec3& a, const Vec3& b) {
        return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    }

    // Ekliptikale Koordinaten in ein äquatoriales Koordinatensystem (km).
    Vec3 eclipticToEquatorial(double lonDeg, double latDeg, double distance,
                              double obliquityDeg) {
        const double x = distance * cosDeg(latDeg) * cosDeg(lonDeg);
        const double y = distance * cosDeg(latDeg) * sinDeg(lonDeg);
        const double z = distance * sinDeg(latDeg);
        return {x,
                y * cosDeg(obliquityDeg) - z * sinDeg(obliquityDeg),
                y * sinDeg(obliquityDeg) + z * cosDeg(obliquityDeg)};
    }

    struct Body {
        double eclipticLongitude; // Grad
        Vec3 equatorial;          // km, geozentrisch
    };

    // Mond nach Meeus, "Astronomical Algorithms", Kap. 47 (Hauptterme).
    Body moonPosition(double t, double obliquity) {
        const double lp = 218.3164477 + 481267.88123421 * t;
        const double d = 297.8501921 + 445267.1114034 * t;
        const double m = 357.5291092 + 35999.0502909 * t;
        const double mp = 134.9633964 + 477198.8675055 * t;
        const double f = 93.2720950 + 483202.0175233 * t;

        const double longitude = lp + 6.288774 * sinDeg(mp) +
                                 1.274027 * sinDeg(2 * d - mp) +
                                 0.658314 * sinDeg(2 * d) +
                                 0.213618 * sinDeg(2 * mp) -
                                 0.185116 * sinDeg(m) -
                                 0.114332 * sinDeg(2 * f) +
                                 0.058793 * sinDeg(2 * d - 2 * mp) +
                                 0.057066 * sinDeg(2 * d - m - mp) +
                                 0.053322 * sinDeg(2 * d + mp) +
                                 0.045758 * sinDeg(2 * d - m) -
                                 0.040923 * sinDeg(m - mp) -
                                 0.034720 * sinDeg(d) -
                                 0.030383 * sinDeg(m + mp);

        const double latitude = 5.128122 * sinDeg(f) +
                                0.280602 * sinDeg(mp + f) +
                                0.277693 * sinDeg(mp - f) +
                                0.173238 * sinDeg(2 * d - f) +
                                0.055413 * sinDeg(2 * d + f - mp) +
                                0.046272 * sinDeg(2 * d - f - mp) +
                                0.032573 * sinDeg(2 * d + f) +
                                0.017198 * sinDeg(2 * mp + f);

        const double distance = 385000.56 - 20905.355 * cosDeg(mp) -
                                3699.111 * cosDeg(2 * d - mp) -
                                2955.968 * cosDeg(2 * d) -
                                569.925 * cosDeg(2 * mp) +
                                246.158 * cosDeg(2 * d - 2 * mp) -
                                204.586 * cosDeg(2 * d - m) -
                                170.733 * cosDeg(2 * d + mp) -
                                152.138 * cosDeg(2 * d - m - mp);

        return {normalize(longitude, 360.0),
                eclipticToEquatorial(longitude, latitude, distance, obliquity)};
    }

    // Sonne nach Meeus, Kap. 25 (niedrige Genauigkeit).
    Body sunPosition(double t, double obliquity) {
        const double l0 = 280.46646 + 36000.76983 * t;
        const double m = 357.52911 + 35999.05029 * t;
        const double e = 0.016708634 - 0.000042037 * t;
        const double c = (1.914602 - 0.004817 * t) * sinDeg(m) +
                         0.019993 * sinDeg(2 * m) + 0.000289 * sinDeg(3 * m);
        const double trueAnomaly = m + c;
        const double distanceAu = 1.000001018 * (1.0 - e * e) /
                                  (1.0 + e * cosDeg(trueAnomaly));
        const double longitude = l0 + c;
        return {normalize(longitude, 360.0),
                eclipticToEquatorial(longitude, 0.0,
                                     distanceAu * kAstronomicalUnitKm,
                                     obliquity)};
    }

    // Beobachterposition (WGS84) im mit der Erde mitdrehenden Äquatorsystem.
    struct Observer {
        Vec3 position; // km
        Vec3 up;       // Einheitsvektor zum Zenit
    };

    Observer observerAt(double gmstDeg) {
        const double lat = toRadians(kObserverLatitudeDeg);
        const double e2 = kEarthFlattening * (2.0 - kEarthFlattening);
        const double n = kEarthRadiusKm / std::sqrt(1.0 - e2 * std::sin(lat) * std::sin(lat));
        const double rho = (n + kObserverElevationKm) * std::cos(lat);
        const double z = (n * (1.0 - e2) + kObserverElevationKm) * std::sin(lat);
        const double lst = gmstDeg + kObserverLongitudeDeg;
        return {{rho * cosDeg(lst), rho * sinDeg(lst), z},
                {std::cos(lat) * cosDeg(lst), std::cos(lat) * sinDeg(lst),
                 std::sin(lat)}};
    }

    double apparentLunarLimbAltitude(double julianDay) {
        const double t = (julianDay - 2451545.0) / 36525.0;
        const double obliquity = 23.439291 - 0.0130042 * t;
        const double gmst = normalize(
            280.46061837 + 360.98564736629 * (julianDay - 2451545.0) +
                0.000387933 * t * t, 360.0);
        const Body moon = moonPosition(t, obliquity);
        const Observer observer = observerAt(gmst);
        const Vec3 moonFromObserver = subtract(moon.equatorial, observer.position);
        const double distance = length(moonFromObserver);
        const double geometricAltitude = toDegrees(std::asin(std::clamp(
            dot(moonFromObserver, observer.up) / distance, -1.0, 1.0)));
        const double semidiameter = toDegrees(std::asin(kMoonRadiusKm / distance));
        return geometricAltitude + semidiameter + kHorizonRefractionDeg;
    }

    std::optional<double> nextRiseSet(double startJulianDay, bool rising) {
        double previousDay = startJulianDay;
        double previousAltitude = apparentLunarLimbAltitude(previousDay);
        constexpr int steps = static_cast<int>(kRiseSetSearchDays / kRiseSetStepDays);

        for (int step = 1; step <= steps; ++step) {
            const double currentDay = startJulianDay + step * kRiseSetStepDays;
            const double currentAltitude = apparentLunarLimbAltitude(currentDay);
            const bool crossed = rising
                ? previousAltitude < 0.0 && currentAltitude >= 0.0
                : previousAltitude > 0.0 && currentAltitude <= 0.0;

            if (crossed) {
                double low = previousDay;
                double high = currentDay;
                for (int iteration = 0; iteration < 40; ++iteration) {
                    const double middle = (low + high) / 2.0;
                    if (const double altitude = apparentLunarLimbAltitude(middle); (rising && altitude >= 0.0) || (!rising && altitude <= 0.0)) {
                        high = middle;
                    } else {
                        low = middle;
                    }
                }
                return (low + high) / 2.0;
            }

            previousDay = currentDay;
            previousAltitude = currentAltitude;
        }

        return std::nullopt;
    }

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

    const char* phaseName(double age, bool english) {
        if (age < kNewMoonEnd || age >= kWaningCrescentEnd) {
            return localizedText(english, "Neumond", "New Moon");
        }
        if (age < kWaxingCrescentEnd) {
            return localizedText(english, "Zunehmende Sichel", "Waxing Crescent");
        }
        if (age < kFirstQuarterEnd) {
            return localizedText(english, "Erstes Viertel", "First Quarter");
        }
        if (age < kWaxingGibbousEnd) {
            return localizedText(english, "Zunehmender Mond", "Waxing Gibbous");
        }
        if (age < kFullMoonEnd) {
            return localizedText(english, "Vollmond", "Full Moon");
        }
        if (age < kWaningGibbousEnd) {
            return localizedText(english, "Abnehmender Mond", "Waning Gibbous");
        }
        if (age < kLastQuarterEnd) {
            return localizedText(english, "Letztes Viertel", "Last Quarter");
        }
        return localizedText(english, "Abnehmende Sichel", "Waning Crescent");
    }

    const char* phaseSymbol(double age) {
        if (age < kNewMoonEnd || age >= kWaningCrescentEnd) {
            return "🌑";
        }
        if (age < kWaxingCrescentEnd) {
            return "🌒";
        }
        if (age < kFirstQuarterEnd) {
            return "🌓";
        }
        if (age < kWaxingGibbousEnd) {
            return "🌔";
        }
        if (age < kFullMoonEnd) {
            return "🌕";
        }
        if (age < kWaningGibbousEnd) {
            return "🌖";
        }
        if (age < kLastQuarterEnd) {
            return "🌗";
        }
        return "🌘";
    }
} // namespace

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
    const double t = (julianDay - 2451545.0) / 36525.0;

    const double obliquity = 23.439291 - 0.0130042 * t;
    const double gmst = normalize(
        280.46061837 + 360.98564736629 * (julianDay - 2451545.0) +
            0.000387933 * t * t, 360.0);

    const Body moon = moonPosition(t, obliquity);
    const Body sun = sunPosition(t, obliquity);
    const Observer observer = observerAt(gmst);

    // Alter und Phasenname folgen der geozentrischen Elongation.
    const double elongation = normalize(moon.eclipticLongitude - sun.eclipticLongitude, 360.0);
    const double age = elongation / 360.0 * kSynodicMonth;

    // Beleuchtung aus Sicht des Beobachters (Parallaxe berücksichtigt).
    const Vec3 moonFromObserver = subtract(moon.equatorial, observer.position);
    const Vec3 sunFromObserver = subtract(sun.equatorial, observer.position);
    const double moonDistance = length(moonFromObserver);
    const double sunDistance = length(sunFromObserver);
    const double cosPsi = dot(moonFromObserver, sunFromObserver) / (moonDistance * sunDistance);
    const double sinPsi = std::sqrt(std::max(0.0, 1.0 - cosPsi * cosPsi));
    const double phaseAngle = std::atan2(sunDistance * sinPsi, moonDistance - sunDistance * cosPsi);
    const double illumination = (1.0 + std::cos(phaseAngle)) * 50.0;

    const double altitude = toDegrees(std::asin(dot(moonFromObserver, observer.up) / moonDistance));
    const std::optional<double> nextMoonrise = nextRiseSet(julianDay, true);
    const std::optional<double> nextMoonset = nextRiseSet(julianDay, false);

    const std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    const std::tm* localDatePointer = std::localtime(&currentTime);
    if (localDatePointer == nullptr) {
        std::cerr << localizedText(
            english, "Das aktuelle Datum konnte nicht ermittelt werden.",
            "The current date could not be determined.") << "\n";
        return 1;
    }
    const std::tm localDate = *localDatePointer;

    const char* location = english ? "Munich" : kObserverLocation.data();
    std::cout << localizedText(english, "Mondphase am ", "Moon phase on ")
              << std::put_time(&localDate, "%x %X") << "\n"
              << localizedText(english, "Beobachtungsort: ", "Location: ")
              << location << " ("
              << std::fixed << std::setprecision(4) << kObserverLatitudeDeg
              << "° N, " << kObserverLongitudeDeg << "° "
              << localizedText(english, "O", "E") << ")\n"
              << localizedText(english, "Aktuelle Mondphase: ", "Current moon phase: ")
              << phaseSymbol(age) << " " << phaseName(age, english) << "\n"
              << std::setprecision(1)
              << localizedText(english, "Beleuchtung: ", "Illumination: ")
              << illumination << " %\n"
              << localizedText(english, "Höhe über dem Horizont: ",
                               "Altitude above horizon: ")
              << altitude << "° ("
              << localizedText(english, altitude > 0.0 ? "sichtbar" : "unter dem Horizont",
                               altitude > 0.0 ? "visible" : "below the horizon") << ")\n"
              << localizedText(english, "Alter: ", "Age: ") << age << " "
              << localizedText(english, "Tage seit Neumond", "days since new moon") << "\n"
              << localizedText(english, "Entfernung: ", "Distance: ")
              << std::fixed << std::setprecision(0) << moonDistance << " km\n";

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

    printEvent("Mondaufgang: ", "Moonrise: ", nextMoonrise);
    printEvent("Monduntergang: ", "Moonset: ", nextMoonset);
}
