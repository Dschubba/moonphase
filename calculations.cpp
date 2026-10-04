#include "calculations.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

#include "main.h"

namespace moonphase {
    namespace {
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
            double eclipticLongitude;
            Vec3 equatorial;
        };

        // Meeus, Astronomical Algorithms, chapter 47 (principal terms).
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

        // Meeus, chapter 25 (low-precision solar coordinates).
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

        struct Observer {
            Vec3 position;
            Vec3 up;
        };

        // WGS84 observer position in the Earth-fixed equatorial frame.
        Observer observerAt(double gmstDeg, const ObserverLocation& location) {
            const double lat = toRadians(location.latitudeDeg);
            const double e2 = kEarthFlattening * (2.0 - kEarthFlattening);
            const double n = kEarthRadiusKm /
                             std::sqrt(1.0 - e2 * std::sin(lat) * std::sin(lat));
            const double elevationKm = location.elevationMeters / 1000.0;
            const double rho = (n + elevationKm) * std::cos(lat);
            const double z = (n * (1.0 - e2) + elevationKm) * std::sin(lat);
            const double lst = gmstDeg + location.longitudeDeg;
            return {{rho * cosDeg(lst), rho * sinDeg(lst), z},
                    {std::cos(lat) * cosDeg(lst), std::cos(lat) * sinDeg(lst),
                     std::sin(lat)}};
        }

        double apparentLunarLimbAltitude(double julianDay,
                                         const ObserverLocation& location) {
            const double t = (julianDay - 2451545.0) / 36525.0;
            const double obliquity = 23.439291 - 0.0130042 * t;
            const double gmst = normalize(
                280.46061837 + 360.98564736629 * (julianDay - 2451545.0) +
                    0.000387933 * t * t, 360.0);
            const Body moon = moonPosition(t, obliquity);
            const Observer observer = observerAt(gmst, location);
            const Vec3 moonFromObserver = subtract(moon.equatorial, observer.position);
            const double distance = length(moonFromObserver);
            const double geometricAltitude = toDegrees(std::asin(std::clamp(
                dot(moonFromObserver, observer.up) / distance, -1.0, 1.0)));
            const double semidiameter = toDegrees(std::asin(kMoonRadiusKm / distance));
            return geometricAltitude + semidiameter + kHorizonRefractionDeg;
        }

        std::optional<double> nextRiseSet(double startJulianDay, bool rising,
                                          const ObserverLocation& location) {
            double previousDay = startJulianDay;
            double previousAltitude = apparentLunarLimbAltitude(previousDay, location);
            constexpr int steps = static_cast<int>(kRiseSetSearchDays / kRiseSetStepDays);

            for (int step = 1; step <= steps; ++step) {
                const double currentDay = startJulianDay + step * kRiseSetStepDays;
                const double currentAltitude =
                    apparentLunarLimbAltitude(currentDay, location);
                const bool crossed = rising
                    ? previousAltitude < 0.0 && currentAltitude >= 0.0
                    : previousAltitude > 0.0 && currentAltitude <= 0.0;

                if (crossed) {
                    double low = previousDay;
                    double high = currentDay;
                    for (int iteration = 0; iteration < 40; ++iteration) {
                        const double middle = (low + high) / 2.0;
                        const double altitude = apparentLunarLimbAltitude(middle, location);
                        if ((rising && altitude >= 0.0) ||
                            (!rising && altitude <= 0.0)) {
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

        Phase phaseForAge(double age) {
            struct PhaseDefinition {
                double startAge;
                const char* germanName;
                const char* englishName;
                const char* symbol;
            };
            constexpr std::array<PhaseDefinition, 8> phases{{
                {kWaningCrescentEnd, "Neumond", "New Moon", "🌑"},
                {kNewMoonEnd, "Zunehmende Sichel", "Waxing Crescent", "🌒"},
                {kWaxingCrescentEnd, "Erstes Viertel", "First Quarter", "🌓"},
                {kFirstQuarterEnd, "Zunehmender Mond", "Waxing Gibbous", "🌔"},
                {kWaxingGibbousEnd, "Vollmond", "Full Moon", "🌕"},
                {kFullMoonEnd, "Abnehmender Mond", "Waning Gibbous", "🌖"},
                {kWaningGibbousEnd, "Letztes Viertel", "Last Quarter", "🌗"},
                {kLastQuarterEnd, "Abnehmende Sichel", "Waning Crescent", "🌘"},
            }};

            if (age < phases[1].startAge || age >= phases[0].startAge) {
                return {phases[0].germanName, phases[0].englishName, phases[0].symbol};
            }
            for (std::size_t i = phases.size(); i-- > 1;) {
                if (age >= phases[i].startAge) {
                    return {phases[i].germanName, phases[i].englishName, phases[i].symbol};
                }
            }
            return {phases[0].germanName, phases[0].englishName, phases[0].symbol};
        }
    }

    CalculationResult calculate(double julianDay, const ObserverLocation& location) {
        const double t = (julianDay - 2451545.0) / 36525.0;
        const double obliquity = 23.439291 - 0.0130042 * t;
        const double gmst = normalize(
            280.46061837 + 360.98564736629 * (julianDay - 2451545.0) +
                0.000387933 * t * t, 360.0);

        const Body moon = moonPosition(t, obliquity);
        const Body sun = sunPosition(t, obliquity);
        const Observer observer = observerAt(gmst, location);

        const double elongation = normalize(moon.eclipticLongitude -
                                             sun.eclipticLongitude, 360.0);
        const double age = elongation / 360.0 * kSynodicMonth;

        const Vec3 moonFromObserver = subtract(moon.equatorial, observer.position);
        const Vec3 sunFromObserver = subtract(sun.equatorial, observer.position);
        const double moonDistance = length(moonFromObserver);
        const double sunDistance = length(sunFromObserver);
        const double cosPsi = dot(moonFromObserver, sunFromObserver) /
                              (moonDistance * sunDistance);
        const double sinPsi = std::sqrt(std::max(0.0, 1.0 - cosPsi * cosPsi));
        const double phaseAngle = std::atan2(
            sunDistance * sinPsi, moonDistance - sunDistance * cosPsi);
        const double illumination = (1.0 + std::cos(phaseAngle)) * 50.0;
        const double altitude = toDegrees(std::asin(std::clamp(
            dot(moonFromObserver, observer.up) / moonDistance, -1.0, 1.0)));

        return {phaseForAge(age), age, illumination, altitude, moonDistance,
                nextRiseSet(julianDay, true, location),
                nextRiseSet(julianDay, false, location)};
    }

    CalculationResult calculate(double julianDay) {
        const ObserverLocation defaultLocation{
            std::string(kObserverLocation), kObserverLatitudeDeg, kObserverLongitudeDeg,
            kObserverElevationKm * 1000.0};
        return calculate(julianDay, defaultLocation);
    }
}
