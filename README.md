# Mondphase

Ein kleines C++-Programm, das anhand des aktuellen Systemdatums die Mondphase
und den prozentual beleuchteten Anteil der Mondscheibe näherungsweise berechnet.

## Kompilieren und starten

Benötigt werden CMake 3.16 oder neuer sowie ein C++17-kompatibler Compiler:

```sh
cmake -S . -B build
cmake --build build
./build/moonphase
```

## Pakete lokal erstellen

Linux-Pakete installieren das Programm nach `/usr/bin` (im TGZ-Archiv nach
`usr/bin`); alle Formate enthalten außerdem README und Lizenz. Die Paketversion
steht in `CMakeLists.txt` und `PKGBUILD`.

### Debian/Ubuntu (.deb) und RPM (.rpm)

Zusätzlich zu CMake und einem C++17-Compiler werden für `.deb` `dpkg-deb` und
für `.rpm` `rpmbuild` benötigt. Zunächst das Projekt konfigurieren und bauen:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Danach jeweils das gewünschte Paket bauen:

```sh
cpack --config build/CPackConfig.cmake -G DEB -B build/packages
cpack --config build/CPackConfig.cmake -G RPM -B build/packages
```

### Arch Linux

Mit `makepkg` und CMake lässt sich aus dem lokalen Quellcode ein Paket bauen:

```sh
makepkg -f
```

### Windows (NSIS)

Auf Windows werden CMake, ein C++17-Compiler und NSIS benötigt. Nach dem
Konfigurieren und Bauen erzeugt CPack den Installer:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cpack --config build/CPackConfig.cmake -G NSIS -C Release -B build/packages
```

### macOS (DMG)

Auf macOS werden CMake und ein C++17-Compiler benötigt. CPack erzeugt das
DMG-Image:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cpack --config build/CPackConfig.cmake -G DragNDrop -B build/packages
```

Pakete werden lokal auf der jeweiligen Zielplattform erstellt; GitLab-CI oder
automatische Veröffentlichungen sind nicht eingerichtet. Unter Linux kann
`cpack --config build/CPackConfig.cmake -G TGZ -B build/packages`
ein generisches Archiv ohne Debian- oder RPM-Werkzeuge erstellen.

Die Berechnung bestimmt Position von Mond und Sonne nach Meeus
(Astronomical Algorithms, Kap. 25 und 47, Hauptkorrekturterme) und berechnet die
Beleuchtung topozentrisch für den Beobachtungsort München
(47,135125° N, 11,581981° O, 519 m). Zusätzlich wird die Höhe des Mondes über dem
Horizont ausgegeben. Der Ort lässt sich über die `kObserver...`-Konstanten in
`main.h` ändern. Das Ergebnis ist eine Näherung und berücksichtigt keine
atmosphärische Refraktion für die aktuelle Mondhöhe.

Außerdem werden der nächste Mondaufgang und Monduntergang ab dem aktuellen
Zeitpunkt in lokaler Systemzeit ausgegeben. Als Ereignis gilt der Auf- bzw.
Untergang des sichtbaren oberen Mondrandes; dafür werden der scheinbare
Mondradius und eine übliche Refraktionskorrektur am Horizont angenähert. Die
Zeiten berücksichtigen weder örtliche Horizontabschattung noch aktuelle
Wetterbedingungen und sind wegen der vereinfachten Mondposition nur Richtwerte.
