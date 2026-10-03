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

Die Berechnung bestimmt Position von Mond und Sonne nach Meeus
(Astronomical Algorithms, Kap. 25 und 47, Hauptkorrekturterme) und berechnet die
Beleuchtung topozentrisch für den Beobachtungsort München
(47,135125° N, 11,581981° O, 519 m). Zusätzlich wird die Höhe des Mondes über dem
Horizont ausgegeben. Der Ort lässt sich über die `kObserver...`-Konstanten in
`main.cpp` ändern. Das Ergebnis ist eine Näherung und berücksichtigt keine
atmosphärische Refraktion für die aktuelle Mondhöhe.

Außerdem werden der nächste Mondaufgang und Monduntergang ab dem aktuellen
Zeitpunkt in lokaler Systemzeit ausgegeben. Als Ereignis gilt der Auf- bzw.
Untergang des sichtbaren oberen Mondrandes; dafür werden der scheinbare
Mondradius und eine übliche Refraktionskorrektur am Horizont angenähert. Die
Zeiten berücksichtigen weder örtliche Horizontabschattung noch aktuelle
Wetterbedingungen und sind wegen der vereinfachten Mondposition nur Richtwerte.
