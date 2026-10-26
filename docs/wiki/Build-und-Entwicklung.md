# Build und Entwicklung

[Deutsch](Build-und-Entwicklung) | [English](Build-and-development)

## Ein Build, ein Programmpaket

| Verzeichnis | Zweck |
| --- | --- |
| `build-custom-make` | Einziger lokaler Custom-Hauptbuild mit MinGW Makefiles; Tests in dessen Unterverzeichnissen. |
| `dist-custom` | Gemeinsames Programmpaket für alle Starter. |
| `.tools` | Lokale SDKs und Laufzeitabhängigkeiten. |
| `.local` | Private Konfigurationen, Archive und Download-Artefakte. |

Im sichtbaren Hauptarbeitsverzeichnis arbeiten. Ein zweiter Custom-Build oder mehrere Kopien für die Geräteprofile sind nicht nötig.

## Windows bauen

Die Werkzeuge erwarten standardmäßig MSYS2/MinGW64 unter `C:\msys64\mingw64`: GCC/G++, Fortran, CMake, MinGW Make, Qt5, Boost, FFTW und libusb. `WSJTX_MINGW_PREFIX` kann den Pfad überschreiben. Das Vorbereitungsskript lädt festgelegte Hamlib-/OmniRig-Abhängigkeiten mit Prüfsummenprüfung.

```powershell
.\tools\Setup-CustomBuild.ps1
.\tools\Build-Custom.ps1
.\tools\Test-Core.ps1
.\tools\Test-PskReporter.ps1
.\tools\Test-WebControl.ps1
.\tools\Test-Reist16.ps1
.\tools\Test-TxQuality.ps1
.\tools\Test-Ft2.ps1
.\tools\Test-PlutoBridge.ps1
.\tools\Test-HackrfBridge.ps1
.\tools\Package-Custom.ps1
.\tools\Verify-CustomPackage.ps1 -PortableOnly
```

Für Neubau und Aktualisierung des gemeinsamen Pakets:

```powershell
.\tools\Rebuild-AllCustom.ps1
# Für einen inkrementellen Lauf:
.\tools\Rebuild-AllCustom.ps1 -Incremental
```

Der Name `Rebuild-AllCustom.ps1` bleibt aus Kompatibilitätsgründen erhalten; er erzeugt keine alten Variantenordner erneut. MAP65 und zusätzliche Simulatoren gehören nicht zum Custom-Windows-Paket.

## Upstream und Releases

Die offizielle `WSJTX/wsjtx:master`-Historie wurde am 26. Oktober 2026 abgeglichen. Die zuvor formal fehlenden 103 eigenständigen Patches waren bereits enthalten; der Historien-Merge änderte den Custom-Quellbaum nicht. [Merge und Begründung](https://github.com/rudolfstepan/wsjtx/commit/ae17a3bbc9d0679e2bd806466ce7ed53313ba48c).

GitHub Actions ist derzeit absichtlich deaktiviert. **RC3 wurde lokal gebaut und geprüft.** Der vorhandene Workflow kann bei späterer Aktivierung Windows-Pakete erzeugen; das ist kein Nachweis eines aktuellen CI-Laufs. RC3 ist manuell als Latest geführt.

## Wiki pflegen

Die bearbeitbaren Wiki-Quellen liegen im Hauptrepository unter `docs/wiki`. Inhaltliche Änderungen zuerst committen und anschließend mit `python tools/Publish-Wiki.py` ins GitHub-Wiki veröffentlichen. Der Publisher erhält unbekannte vorhandene Wiki-Seiten und verwendet keinen zweiten Quellcheckout oder Git-Worktree. Jeder Themenbereich besitzt eine deutsche und eine englische Fassung mit direktem Sprachwechsel; `--check` prüft die Gegenstücke und Links vor der Veröffentlichung.

[Builddetails](https://github.com/rudolfstepan/wsjtx/blob/master/CUSTOM-BUILD.md) · [Lizenz](https://github.com/rudolfstepan/wsjtx/blob/master/COPYING) · [Markenhinweise](https://github.com/rudolfstepan/wsjtx/blob/master/TRADEMARK.md)
