# Mondphase

Ein kleines C++-Programm, das anhand des aktuellen Systemdatums die Mondphase
und den prozentual beleuchteten Anteil der Mondscheibe näherungsweise berechnet.

Die Berechnung bestimmt Position von Mond und Sonne nach Meeus
(Astronomical Algorithms, Kap. 25 und 47, Hauptkorrekturterme) und berechnet die
Beleuchtung topozentrisch für den Beobachtungsort. Ohne CLI-Angabe ist dies
München (47,135125° N, 11,581981° O, 519 m). Zusätzlich wird die Höhe des Mondes
über dem Horizont ausgegeben. Das Ergebnis ist eine Näherung und berücksichtigt
keine atmosphärische Refraktion für die aktuelle Mondhöhe.

Außerdem werden der nächste Mondaufgang und Monduntergang ab dem aktuellen
Zeitpunkt in lokaler Systemzeit ausgegeben. Als Ereignis gilt der Auf- bzw.
Untergang des sichtbaren oberen Mondrandes; dafür werden der scheinbare
Mondradius und eine übliche Refraktionskorrektur am Horizont angenähert. Die
Zeiten berücksichtigen weder örtliche Horizontabschattung noch aktuelle
Wetterbedingungen und sind wegen der vereinfachten Mondposition nur Richtwerte.

Zusätzlich gibt das Programm die aktuelle topozentrische Sonnenhöhe und
-entfernung sowie den nächsten Sonnenaufgang und Sonnenuntergang aus. Die
Tages- und Nachtlänge beziehen sich auf den aktuellen lokalen Kalendertag und
werden aus dem scheinbaren Sonnenrand einschließlich einer üblichen
Horizontrefraktion berechnet. Hinter beiden Werten steht außerdem die
Veränderung gegenüber dem Vortag. An Tagen mit Polartag oder Polarnacht wird die
gesamte lokale Tageslänge als Tageslicht beziehungsweise Dunkelheit ausgewiesen. Sonnen- und
Mondereignisse sind Näherungswerte und berücksichtigen keine örtliche
Horizontabschattung oder Wetterbedingungen.

Für den laufenden lokalen Kalendermonat werden außerdem besondere
Mondereignisse aufgeführt: Blue Moons als zweiter Vollmond eines Monats oder
als dritter Vollmond einer Jahreszeit mit vier Vollmonden, Supermonde
(Vollmondentfernung unter 360.000 km) sowie am Beobachtungsort sichtbare
Mondfinsternisse. Die Jahreszeiten werden näherungsweise mit den üblichen
Kalendertagen für Tagundnachtgleichen und Sonnenwenden abgegrenzt; die
berechneten Ereigniszeiten und Sichtbarkeiten sind ebenfalls Näherungswerte.

## Kompilieren und starten

Benötigt werden CMake 3.16 oder neuer sowie ein C++17-kompatibler Compiler:

```sh
cmake -S . -B build
cmake --build build
./build/moonphase
```

Mit `--location` lassen sich ein oder mehrere Beobachtungsorte angeben. Das
Argument besteht aus Name, Breitengrad und Längengrad; die Höhe über dem
Meeresspiegel in Metern ist optional und beträgt standardmäßig 0:

```sh
./build/moonphase \
  --location "München,48.1372,11.5756,519" \
  --location "Berlin,52.5200,13.4050"
```

Breiten- und Längengrade müssen im Bereich -90 bis 90 bzw. -180 bis 180 liegen;
die Höhe muss mindestens 0 m betragen. Ohne `--location` wird weiterhin der
Standardort München (47,135125° N, 11,581981° O, 519 m) verwendet.

## Pakete lokal erstellen

Linux-Pakete installieren das Programm nach `/usr/bin` (im TGZ-Archiv nach
`usr/bin`); alle Formate enthalten außerdem README und Lizenz. Die zentrale
Paketversion steht in der Datei `VERSION`; CMake und das Arch-`PKGBUILD` lesen
sie von dort. Die Arch-Paketrevision `pkgrel` wird weiterhin separat im
`PKGBUILD` verwaltet.

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
