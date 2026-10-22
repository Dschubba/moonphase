# Moon Phase

*Deutsche Fassung: [README.de.md](README.de.md)*

A small C++ program that uses the current system date to approximate the moon
phase and the illuminated percentage of the lunar disk.

The calculation determines the positions of the moon and the sun following Meeus
(Astronomical Algorithms, ch. 25 and 47, principal correction terms) and
computes the illumination topocentrically for the observer's location. Without
a command-line option, this is Munich (47.135125° N, 11.581981° E, 519 m). The
program also prints the moon's altitude above the horizon and the direction in
which an observer has to look (azimuth in degrees, measured from north through
east, plus a 16-point compass direction such as "ESE"). The result is an
approximation and does not account for atmospheric refraction in the current
lunar altitude.

The next moonrise and moonset from the current time onward are also printed in
local system time. The event is defined as the rising or setting of the visible
upper limb of the moon; the apparent lunar radius and a standard horizon
refraction correction are approximated for this. The times account neither for
local horizon obstructions nor for current weather conditions and, because of
the simplified lunar position, are only rough estimates.

In addition, the program prints the current topocentric solar altitude,
direction and distance as well as the next sunrise and sunset. Daylight and
night duration refer to the current local calendar day and are calculated from
the apparent solar limb including a standard horizon refraction. Both values
are followed by the change compared with the previous day. On days with polar
day or polar night, the entire local day is reported as daylight or darkness,
respectively. Solar and lunar events are approximations and do not account for
local horizon obstructions or weather conditions.

For the current local calendar month, special lunar events are listed as well:
Blue Moons as the second full moon of a month or as the third full moon of a
season with four full moons, supermoons (full moon distance below 360,000 km),
and lunar eclipses visible from the observer's location. Seasons are
approximately delimited by the usual calendar dates for equinoxes and
solstices; the calculated event times and visibilities are approximations as
well.

## Building and running

CMake 3.16 or newer and a C++17-compatible compiler are required:

```sh
cmake -S . -B build
cmake --build build
./build/moonphase
```

Use `--location` to specify one or more observer locations. The argument
consists of name, latitude and longitude; the elevation above sea level in
meters is optional and defaults to 0:

```sh
./build/moonphase \
  --location "Munich,48.1372,11.5756,519" \
  --location "Berlin,52.5200,13.4050"
```

Latitude and longitude must be in the range -90 to 90 and -180 to 180,
respectively; the elevation must be at least 0 m. Without `--location`, the
default location Munich (47.135125° N, 11.581981° E, 519 m) is used.

## Building packages locally

Linux packages install the program to `/usr/bin` (to `usr/bin` in the TGZ
archive); all formats also include the README and license. The central package
version is stored in the `VERSION` file; CMake and the Arch `PKGBUILD` read it
from there. The Arch package release `pkgrel` is still maintained separately in
the `PKGBUILD`.

### Debian/Ubuntu (.deb) and RPM (.rpm)

In addition to CMake and a C++17 compiler, `dpkg-deb` is required for `.deb`
and `rpmbuild` for `.rpm`. First configure and build the project:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Then build the desired package:

```sh
cpack --config build/CPackConfig.cmake -G DEB -B build/packages
cpack --config build/CPackConfig.cmake -G RPM -B build/packages
```

### Arch Linux

`makepkg` and CMake build a package from the local source code:

```sh
makepkg -f
```

### Windows (NSIS)

On Windows, CMake, a C++17 compiler and NSIS are required. After configuring
and building, CPack creates the installer:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cpack --config build/CPackConfig.cmake -G NSIS -C Release -B build/packages
```

### macOS (DMG)

On macOS, CMake and a C++17 compiler are required. CPack creates the DMG
image:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cpack --config build/CPackConfig.cmake -G DragNDrop -B build/packages
```

Packages are built locally on the respective target platform; GitLab CI and
automatic releases are not set up. On Linux,
`cpack --config build/CPackConfig.cmake -G TGZ -B build/packages`
can create a generic archive without Debian or RPM tools.
