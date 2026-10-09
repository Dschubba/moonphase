#include "calculations.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <optional>
#include <stdexcept>
#include <utility>

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

        double apparentLimbAltitude(double julianDay, const ObserverLocation& location,
                                    bool sun) {
            const double t = (julianDay - 2451545.0) / 36525.0;
            const double obliquity = 23.439291 - 0.0130042 * t;
            const double gmst = normalize(
                280.46061837 + 360.98564736629 * (julianDay - 2451545.0) +
                    0.000387933 * t * t, 360.0);
            const Body body = sun ? sunPosition(t, obliquity) : moonPosition(t, obliquity);
            const Observer observer = observerAt(gmst, location);
            const Vec3 bodyFromObserver = subtract(body.equatorial, observer.position);
            const double distance = length(bodyFromObserver);
            const double geometricAltitude = toDegrees(std::asin(std::clamp(
                dot(bodyFromObserver, observer.up) / distance, -1.0, 1.0)));
            const double radius = sun ? kSunRadiusKm : kMoonRadiusKm;
            const double semidiameter = toDegrees(std::asin(radius / distance));
            return geometricAltitude + semidiameter + kHorizonRefractionDeg;
        }

        std::optional<double> nextRiseSet(double startJulianDay, bool rising,
                                          const ObserverLocation& location, bool sun) {
            double previousDay = startJulianDay;
            double previousAltitude = apparentLimbAltitude(previousDay, location, sun);
            constexpr int steps = static_cast<int>(kRiseSetSearchDays / kRiseSetStepDays);

            for (int step = 1; step <= steps; ++step) {
                const double currentDay = startJulianDay + step * kRiseSetStepDays;
                const double currentAltitude =
                    apparentLimbAltitude(currentDay, location, sun);
                const bool crossed = rising
                    ? previousAltitude < 0.0 && currentAltitude >= 0.0
                    : previousAltitude > 0.0 && currentAltitude <= 0.0;

                if (crossed) {
                    double low = previousDay;
                    double high = currentDay;
                    for (int iteration = 0; iteration < 40; ++iteration) {
                        const double middle = (low + high) / 2.0;
                        const double altitude = apparentLimbAltitude(middle, location, sun);
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

        double daylightWithinInterval(std::time_t start, std::time_t end,
                                      const ObserverLocation& location) {
            const auto altitudeAt = [&location](double unixTime) {
                const double day = kJulianDayAtUnixEpoch + unixTime / 86400.0;
                return apparentLimbAltitude(day, location, true);
            };
            const double endSeconds = static_cast<double>(end);
            double previousSeconds = static_cast<double>(start);
            double previousAltitude = altitudeAt(previousSeconds);
            double daylight = 0.0;
            constexpr double sampleIntervalSeconds = 600.0;

            while (previousSeconds < endSeconds) {
                const double currentSeconds =
                    std::min(previousSeconds + sampleIntervalSeconds, endSeconds);
                const double currentAltitude = altitudeAt(currentSeconds);
                const bool rising = previousAltitude < 0.0 && currentAltitude >= 0.0;
                const bool setting = previousAltitude >= 0.0 && currentAltitude < 0.0;

                if (rising || setting) {
                    double low = previousSeconds;
                    double high = currentSeconds;
                    for (int iteration = 0; iteration < 40; ++iteration) {
                        const double middle = (low + high) / 2.0;
                        const bool aboveHorizon = altitudeAt(middle) >= 0.0;
                        if (aboveHorizon == rising) {
                            high = middle;
                        } else {
                            low = middle;
                        }
                    }
                    const double crossing = (low + high) / 2.0;
                    daylight += rising ? currentSeconds - crossing
                                       : crossing - previousSeconds;
                } else if (previousAltitude >= 0.0) {
                    daylight += currentSeconds - previousSeconds;
                }

                previousSeconds = currentSeconds;
                previousAltitude = currentAltitude;
            }

            return daylight;
        }

        struct DaylightSummary {
            double daylightSeconds;
            double nightSeconds;
            double daylightChangeSeconds;
            double nightChangeSeconds;
        };

        DaylightSummary daylightDuration(double julianDay,
                                         const ObserverLocation& location) {
            const double unixSeconds =
                (julianDay - kJulianDayAtUnixEpoch) * 86400.0;
            const auto currentTime = static_cast<std::time_t>(unixSeconds);
            const std::tm* localTimePointer = std::localtime(&currentTime);
            if (localTimePointer == nullptr) {
                throw std::runtime_error("Could not determine the local date.");
            }

            std::tm midnight = *localTimePointer;
            midnight.tm_hour = 0;
            midnight.tm_min = 0;
            midnight.tm_sec = 0;
            midnight.tm_isdst = -1;
            const std::time_t start = std::mktime(&midnight);
            std::tm nextMidnight = midnight;
            ++nextMidnight.tm_mday;
            nextMidnight.tm_isdst = -1;
            const std::time_t end = std::mktime(&nextMidnight);
            std::tm previousMidnight = midnight;
            --previousMidnight.tm_mday;
            previousMidnight.tm_isdst = -1;
            const std::time_t previousStart = std::mktime(&previousMidnight);
            if (start == static_cast<std::time_t>(-1) ||
                end == static_cast<std::time_t>(-1) ||
                previousStart == static_cast<std::time_t>(-1) ||
                end <= start || start <= previousStart) {
                throw std::runtime_error("Could not determine the local day boundaries.");
            }

            const double daylight = daylightWithinInterval(start, end, location);
            const double previousDaylight =
                daylightWithinInterval(previousStart, start, location);
            const double night = std::difftime(end, start) - daylight;
            const double previousNight =
                std::difftime(start, previousStart) - previousDaylight;
            return {daylight, night, daylight - previousDaylight,
                    night - previousNight};
        }

        struct PhaseEvent {
            double julianDay;
            bool fullMoon;
        };

        double phaseOffset(double julianDay, double targetDeg) {
            const double t = (julianDay - 2451545.0) / 36525.0;
            const double obliquity = 23.439291 - 0.0130042 * t;
            const Body moon = moonPosition(t, obliquity);
            const Body sun = sunPosition(t, obliquity);
            return normalize(moon.eclipticLongitude - sun.eclipticLongitude -
                                 targetDeg + 180.0,
                             360.0) - 180.0;
        }

        std::vector<double> phaseEvents(double startJulianDay, double endJulianDay,
                                        double targetDeg) {
            constexpr double stepDays = 0.25;
            double previousDay = startJulianDay;
            double previousOffset = phaseOffset(previousDay, targetDeg);
            std::vector<double> events;

            for (double currentDay = startJulianDay + stepDays;
                 currentDay <= endJulianDay; currentDay += stepDays) {
                const double currentOffset = phaseOffset(currentDay, targetDeg);
                if (previousOffset < 0.0 && currentOffset >= 0.0) {
                    double low = previousDay;
                    double high = currentDay;
                    for (int iteration = 0; iteration < 40; ++iteration) {
                        const double middle = (low + high) / 2.0;
                        if (phaseOffset(middle, targetDeg) >= 0.0) {
                            high = middle;
                        } else {
                            low = middle;
                        }
                    }
                    events.push_back((low + high) / 2.0);
                }
                previousDay = currentDay;
                previousOffset = currentOffset;
            }
            return events;
        }

        std::time_t localDateTime(int year, int month, int day) {
            std::tm date{};
            date.tm_year = year - 1900;
            date.tm_mon = month;
            date.tm_mday = day;
            date.tm_isdst = -1;
            return std::mktime(&date);
        }

        std::vector<MoonSpecialEvent> moonSpecialEvents(
            double julianDay, const ObserverLocation& location) {
            const auto localTimestamp = static_cast<std::time_t>(
                (julianDay - kJulianDayAtUnixEpoch) * 86400.0);
            const std::tm* localDatePointer = std::localtime(&localTimestamp);
            if (localDatePointer == nullptr) {
                throw std::runtime_error("Could not determine the local date.");
            }
            const std::tm localDate = *localDatePointer;

            std::tm monthStart = localDate;
            monthStart.tm_mday = 1;
            monthStart.tm_hour = 0;
            monthStart.tm_min = 0;
            monthStart.tm_sec = 0;
            monthStart.tm_isdst = -1;
            const std::time_t monthStartTime = std::mktime(&monthStart);
            std::tm monthEnd = monthStart;
            ++monthEnd.tm_mon;
            monthEnd.tm_isdst = -1;
            const std::time_t monthEndTime = std::mktime(&monthEnd);
            if (monthStartTime == static_cast<std::time_t>(-1) ||
                monthEndTime == static_cast<std::time_t>(-1) ||
                monthEndTime <= monthStartTime) {
                throw std::runtime_error("Could not determine the local month boundaries.");
            }

            const double monthStartDay = kJulianDayAtUnixEpoch +
                static_cast<double>(monthStartTime) / 86400.0;
            const double monthEndDay = kJulianDayAtUnixEpoch +
                static_cast<double>(monthEndTime) / 86400.0;
            constexpr double searchPaddingDays = 100.0;
            const double searchStart = monthStartDay - searchPaddingDays;
            const double searchEnd = monthEndDay + searchPaddingDays;
            std::vector<PhaseEvent> phases;
            for (double fullMoon : phaseEvents(searchStart, searchEnd, 180.0)) {
                phases.push_back({fullMoon, true});
            }
            for (double newMoon : phaseEvents(searchStart, searchEnd, 0.0)) {
                phases.push_back({newMoon, false});
            }
            std::sort(phases.begin(), phases.end(),
                      [](const PhaseEvent& a, const PhaseEvent& b) {
                          return a.julianDay < b.julianDay;
                      });

            std::vector<MoonSpecialEvent> specialEvents;
            std::vector<double> fullMoonsThisMonth;
            for (const PhaseEvent& phase : phases) {
                if (phase.fullMoon && phase.julianDay >= monthStartDay &&
                    phase.julianDay < monthEndDay) {
                    fullMoonsThisMonth.push_back(phase.julianDay);
                }
            }

            for (std::size_t i = 1; i < fullMoonsThisMonth.size(); ++i) {
                specialEvents.push_back(
                    {MoonSpecialEventType::MonthlyBlueMoon, fullMoonsThisMonth[i]});
            }

            std::vector<std::time_t> seasonBoundaries;
            constexpr std::array<std::array<int, 2>, 4> seasons{{
                {{2, 20}}, {{5, 21}}, {{8, 22}}, {{11, 21}}
            }};
            for (int year = localDate.tm_year + 1899;
                 year <= localDate.tm_year + 1901; ++year) {
                for (const auto& season : seasons) {
                    const std::time_t boundary =
                        localDateTime(year, season[0], season[1]);
                    if (boundary != static_cast<std::time_t>(-1)) {
                        seasonBoundaries.push_back(boundary);
                    }
                }
            }
            std::sort(seasonBoundaries.begin(), seasonBoundaries.end());

            for (double fullMoon : fullMoonsThisMonth) {
                const auto nextBoundary = std::upper_bound(
                    seasonBoundaries.begin(), seasonBoundaries.end(),
                    static_cast<std::time_t>((fullMoon - kJulianDayAtUnixEpoch) * 86400.0));
                if (nextBoundary == seasonBoundaries.begin() ||
                    nextBoundary == seasonBoundaries.end()) {
                    continue;
                }
                const double seasonStart = kJulianDayAtUnixEpoch +
                    static_cast<double>(*(nextBoundary - 1)) / 86400.0;
                const double seasonEnd = kJulianDayAtUnixEpoch +
                    static_cast<double>(*nextBoundary) / 86400.0;
                std::vector<double> fullMoonsInSeason;
                for (const PhaseEvent& phase : phases) {
                    if (phase.fullMoon && phase.julianDay >= seasonStart &&
                        phase.julianDay < seasonEnd) {
                        fullMoonsInSeason.push_back(phase.julianDay);
                    }
                }
                if (fullMoonsInSeason.size() == 4 &&
                    std::abs(fullMoonsInSeason[2] - fullMoon) < 0.01) {
                    specialEvents.push_back(
                        {MoonSpecialEventType::SeasonalBlueMoon, fullMoon});
                }
            }

            for (double fullMoon : fullMoonsThisMonth) {
                const double t = (fullMoon - 2451545.0) / 36525.0;
                const Body moon = moonPosition(t, 23.439291 - 0.0130042 * t);
                const double geocentricDistance = length(moon.equatorial);
                if (geocentricDistance <= 360000.0) {
                    specialEvents.push_back(
                        {MoonSpecialEventType::Supermoon, fullMoon});
                }

                const auto eclipse = [fullMoon, &location](double offsetHours) {
                    const double eventDay = fullMoon + offsetHours / 24.0;
                    const double eventT = (eventDay - 2451545.0) / 36525.0;
                    const double obliquity = 23.439291 - 0.0130042 * eventT;
                    const Body moonAtTime = moonPosition(eventT, obliquity);
                    const Body sunAtTime = sunPosition(eventT, obliquity);
                    const double moonDistance = length(moonAtTime.equatorial);
                    const double sunDistance = length(sunAtTime.equatorial);
                    const double umbraRadius = kEarthRadiusKm -
                        moonDistance * (kSunRadiusKm - kEarthRadiusKm) / sunDistance;
                    const double penumbraRadius = kEarthRadiusKm +
                        moonDistance * (kSunRadiusKm + kEarthRadiusKm) / sunDistance;
                    const double moonAngularRadius =
                        toDegrees(std::asin(kMoonRadiusKm / moonDistance));
                    const double umbraAngularRadius =
                        toDegrees(std::asin(std::clamp(umbraRadius / moonDistance, 0.0, 1.0)));
                    const double penumbraAngularRadius =
                        toDegrees(std::asin(std::clamp(penumbraRadius / moonDistance, 0.0, 1.0)));
                    const Vec3 antiSun{-sunAtTime.equatorial[0],
                                       -sunAtTime.equatorial[1],
                                       -sunAtTime.equatorial[2]};
                    const double separation = toDegrees(std::acos(std::clamp(
                        dot(moonAtTime.equatorial, antiSun) /
                            (moonDistance * sunDistance),
                        -1.0, 1.0)));
                    const bool eclipseInProgress =
                        separation <= penumbraAngularRadius + moonAngularRadius;
                    if (!eclipseInProgress ||
                        apparentLimbAltitude(eventDay, location, false) <= 0.0) {
                        return 0;
                    }
                    if (separation <= umbraAngularRadius - moonAngularRadius) {
                        return 3;
                    }
                    if (separation <= umbraAngularRadius + moonAngularRadius) {
                        return 2;
                    }
                    return 1;
                };

                int visibleEclipseSeverity = 0;
                for (int sample = -30; sample <= 30; ++sample) {
                    visibleEclipseSeverity =
                        std::max(visibleEclipseSeverity, eclipse(sample / 6.0));
                }

                if (visibleEclipseSeverity == 3) {
                    specialEvents.push_back(
                        {MoonSpecialEventType::TotalLunarEclipse, fullMoon});
                } else if (visibleEclipseSeverity == 2) {
                    specialEvents.push_back(
                        {MoonSpecialEventType::PartialLunarEclipse, fullMoon});
                } else if (visibleEclipseSeverity == 1) {
                    specialEvents.push_back(
                        {MoonSpecialEventType::PenumbralLunarEclipse, fullMoon});
                }
            }

            std::sort(specialEvents.begin(), specialEvents.end(),
                      [](const MoonSpecialEvent& a, const MoonSpecialEvent& b) {
                          return a.julianDay < b.julianDay;
                      });
            return specialEvents;
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
        const double sunAltitude = toDegrees(std::asin(std::clamp(
            dot(sunFromObserver, observer.up) / sunDistance, -1.0, 1.0)));
        const DaylightSummary daylight = daylightDuration(julianDay, location);

        return {phaseForAge(age), age, illumination, altitude, moonDistance,
                sunAltitude, sunDistance,
                nextRiseSet(julianDay, true, location, false),
                nextRiseSet(julianDay, false, location, false),
                nextRiseSet(julianDay, true, location, true),
                nextRiseSet(julianDay, false, location, true),
                daylight.daylightSeconds, daylight.nightSeconds,
                daylight.daylightChangeSeconds, daylight.nightChangeSeconds,
                moonSpecialEvents(julianDay, location)};
    }

    CalculationResult calculate(double julianDay) {
        const ObserverLocation defaultLocation{
            std::string(kObserverLocation), kObserverLatitudeDeg, kObserverLongitudeDeg,
            kObserverElevationKm * 1000.0};
        return calculate(julianDay, defaultLocation);
    }
}
