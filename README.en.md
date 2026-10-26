# OE3SRO Custom

[Deutsch](README.md) | **English**

**build by OE3SRO**

[![Windows build](https://github.com/rudolfstepan/wsjtx/actions/workflows/custom-windows.yml/badge.svg)](https://github.com/rudolfstepan/wsjtx/actions/workflows/custom-windows.yml)

**[REIST16 Release Candidate 3.0.2-oe3sro.7-rc3](https://github.com/rudolfstepan/wsjtx/releases/tag/oe3sro-v3.0.2-7-rc3)** —
Multisignal decoding with cancellation, additional weak-signal methods and
digital transmit level matched to FT8. [Changes and validation limits](docs/releases/3.0.2-oe3sro.7-rc3.md).
Intended for field testing; the stable release remains available separately.

**[Download Windows release](https://github.com/rudolfstepan/wsjtx/releases/latest)** —
the **windows-x64.zip** includes the application and German/English documentation.
Extract it completely, then launch **Start-WSJT-X.bat**.
The [guide](docs/WINDOWS-QUICKSTART.html) is included as **docs/index.html**
and is also available separately as **documentation.zip**.

[Version 3.0.2-oe3sro.5](docs/releases/3.0.2-oe3sro.5.md) is the first release
with **REIST16-QSO/2**, Pluto/HackRF integration and browser WebSDR reception.
PSK Reporter tools, personal DX windows and FT8/FT4/FT2 optimizations remain
included. REIST16 remains experimental.

**[Latest CI builds](https://github.com/rudolfstepan/wsjtx/actions/workflows/custom-windows.yml):**
Open the latest successful run and download the versioned
`wsjtx-OE3SRO-…-windows-x64` package under **Artifacts**. GitHub login is required.
Extract the artifact and its enclosed ZIP, then launch **Start-WSJT-X.bat**.
The EXE, decoders, DLLs, and station data are included. Configure your station
and radio on first launch; existing Custom settings are preserved.
Downloads are retained for 90 days.

**Start with a double-click:** open [start.bat](start.bat) in the repository
directory. It launches the existing custom package with the `Custom` profile.

This [fork](https://github.com/rudolfstepan/wsjtx) of
[WSJTX/wsjtx](https://github.com/WSJTX/wsjtx) adds an integrated PSK Reporter
receiver list and practical station calling tools to WSJT-X. See which
stations have reported your signal, check their logbook status, and select
or call them again directly within the app.

The custom additions are by **OE3SRO**; FT2 adapts the GPLv3 implementation
by **DG2YCB / WSJT-X Improved**. The version display, window title,
About dialog, and splash screen include **build by OE3SRO**.
The custom build currently uses **WSJT-X 3.0.2-devel** as its base; the build
scripts and package checks described here target Windows.
GitHub Actions builds the package automatically on every push, including tags,
as well as pull requests and manual runs. Filenames include the project version,
source commit, build number, and attempt; `build-info.json` and a SHA256 file
identify and verify the download.
Tags named `oe3sro-vX.Y.Z-N` also publish a persistent GitHub release with
both ZIPs and their checksums after the checks pass.
An `-rcM` suffix publishes a prerelease candidate without replacing the
stable latest release.

The [fork change history](docs/CHANGES.md) records the custom additions and
their implementation commits. The [FT2 documentation](docs/FT2.md) covers
operation, implementation, sources, and the verified build baseline.

## Experimental LAN web control

Open **View → Web control (experimental)…**, start the server, close the dialog, then open
the displayed address in a browser and enter the access key. The browser
accepts the key set on the desktop with **Start / apply key**; alternatively,
**Start / new random key** generates one. The browser
recreates the main operating panel: decodes, RX/TX controls, QSO fields,
messages, logging and the live waterfall. Both interfaces operate the same
running station. Setup and additional dialogs remain on the desktop.

![English web interface with FT8 band activity, QSO controls, transmit messages and live waterfall](docs/images/web-control-ft8-en-2026-10-25.png)

Original screenshot, 25 October 2026: **OE3SRO/P (JN88)** receiving FT8
on **14.074 MHz (20 m)**. Station and connection status appear at the top,
followed by decode lists and RX/TX actions. The three lower panels contain
the DX station, QSO controls and six transmit messages; **Wide Graph** shows
the desktop application's waterfall image. Magenta rows follow the configured
desktop formatting. The screenshot shows reception, with no completed QSO
documented. [Interface walkthrough (German)](docs/WEB-CONTROL.txt).

The **Language** selector offers German and English. **Browser language** selects
a supported browser language, falling back to English. Manual choices are saved
in the browser. Station colors, callsign highlighting and PSK Reporter markers
follow the desktop display.

The server is off after every restart. Use a trusted LAN or VPN: HTTP is
unencrypted, and disconnecting the browser does not halt transmission.
[Operating instructions and tests (German)](docs/WEB-CONTROL.txt).

## Separate HackRF One instance

`start-hackrf-ft8-17m.bat` adds a separate HackRF RX/TX instance, initially
using **FT8 at 18.100 MHz**. Its defaults are TX gain 47 dB, digital amplitude
1.0 and the internal RF amplifier enabled during TX. The independent profile
**WSJT-X - HackRF-FT8-17m** uses TCI 40004 / UDP 2243; Enable TX starts off.
See the [HackRF instructions](docs/HACKRF-WSJTX.txt).
The user reports reception of their FT8 CQ through PSK Reporter in Finland
(−25 dB) and Spain (−18 dB). This confirms remote reception, with no completed
QSO or measured output power claimed. The bridge waits for actual USB data
and preserves the TCI connection during bounded RX recovery.

`start-hackrf.bat` launches the HackRF version with its own **WSJT-X - HackRF**
profile, local CAT/audio port 40003 and UDP port 2239. USB audio uses TCI;
REIST16-QSO/2 and the other WSJT-X modes share this radio path.
[Launcher, device selection and validation](docs/HACKRF-WSJTX.txt).
Software and mocked-device checks passed; RF acceptance is still pending.

## REIST16: new experimental mode

**REIST16** is OE3SRO's experimental mode for short structured amateur-radio
contacts, included in the Windows release starting with **3.0.2-oe3sro.5**.
Select **Mode → REIST16** for CQ, locator, report, R-report, RR73 and 73,
double-click selection and Auto Seq.

Its current **QSO/2** format carries WSJT-X's 77-bit standard messages using
CRC14 and LDPC(174,91) on sixteen Gaussian FSK tones. A 12.8-second signal
fits inside a 15-second UTC slot. Tone spacing is 5 Hz, with 75 Hz between
the outer tone centres; this is not measured occupied bandwidth. REIST16
is incompatible with FT8 over the air. Update both endpoints for QSO/2;
the new receiver also accepts QSO/1. Free text and special contest formats
are outside the supported message profile.

The local extension dated 26 October raises receiver output capacity to 200
messages per slot and adds signal cancellation and repeated search at decoding
depth 2/3. Use depth 3 for dense reception. The QSO/2 wire format is unchanged.
[Changes, offline comparisons and limitations](docs/validation/reist16-multisignal/IMPROVEMENTS.md).
Additional weak-signal methods and optional CQ AP improve an independent
mixed-message −22 dB test from 7/16 to 11/16 recovered messages.
[Conditions and remaining limits](docs/validation/reist16-multisignal/WEAK-UPGRADE.md).

Synchronize both system clocks, agree on a USB frequency and select opposite
TX slots (`:00/:30` and `:15/:45`). Soundcard/USB audio works as for other
modes. PlutoSDR and HackRF use the included TCI bridges; supply the HackRF
runtime separately. `start-yaesu-websdr.bat` combines WASAPI browser reception
with Yaesu transmission in one instance.
[WebSDR and receive-only self-test](docs/WEBSDR-WSJTX.txt).

The Yaesu/Pluto screenshot from 21 October documents the earlier **QSO/1**.
QSO/2 has been checked with production audio, noise tests and WebSDR recordings;
these do not establish general FT8-equivalent sensitivity or fading tolerance.
Repeated-CQ averaging is not implemented. REIST16 sends no PSK Reporter spots
and does not claim a registered ADIF mode.
See [REIST16](docs/REIST16.md) / [QSO/2](docs/REIST16-QSO2.md),
[the profile contract](docs/REIST16.txt) and [Pluto setup](docs/PLUTO-WSJTX.txt).

![Yaesu OE3SRO / Pluto OE3SRO/P REIST16 exchange](docs/images/REIST16_QSO.png)

## New features by OE3SRO

| Feature | Benefit |
| --- | --- |
| **FT8 / FT4 / FT2 native kernels and TX diagnostics** | C++ waveforms, shared LDPC encoder, FT8 BP kernels and parallel FT4/FT2 synchronization; audio/start/ALC readings under View → TX quality — FT8 / FT4 / FT2. [FT8](docs/FT8.md), [FT4/FT2](docs/FT2.md). |
| **REIST16 (experimental)** | New 15-second CQ/QSO mode, native TX/RX and Auto Seq; Yaesu/Pluto exchange documented. [Details](docs/REIST16.md). |
| **PlutoSDR radio bridge** | Separate WSJT-X instance with TCI frequency control, PTT and USB audio conversion. [Setup](docs/PLUTO-WSJTX.txt). |
| **FT2 (experimental)** | 3.75-second periods with transmit, decode, Auto Seq, waterfall, and PSK calling tools; adapted from WSJT-X Improved by DG2YCB. |
| **PSK Reporter: Who hears me?** | Show receivers of your signal directly in WSJT-X, including callsign, grid locator, SNR, reception time, and mode. |
| **Personal DX windows** | Collect local reception history by DXCC country, band, UTC hour and month, ranked by distinct reception days; separate callsigns, modes and station/antenna setups. |
| **Call a station directly** | Fill in the callsign and locator, select the first standard message, and turn on **Enable Tx**. |
| **Receiver markers in Band Activity** | Callsigns from the receiver list appear in turquoise, bold, and underlined; a tooltip shows the reception time. |
| **Existing WSJT-X colors** | The list uses your highlighting rules and logbook, including band, mode, DXCC, grid locator, and LoTW. |
| **Saved favorites** | Mark stations you want to contact regularly and filter the receiver list to your favorites. |
| **Recent calls and recall** | Select from the last 20 history entries or call the most recently called station again using a keyboard shortcut. |
| **Next station** | Select and call a visible station, preferably one you have not called here in the current frequency/mode context. |
| **Search and filters** | Search by callsign or locator; show new callsigns, stations new on this band, favorites, previously called stations, or reports from the last 5/15/30 minutes. |
| **Call overview and Halt Tx** | View call attempts and the last call time; stop transmission directly from the list. |
| **Import existing settings** | Copy the installed WSJT-X configuration byte for byte into a separate custom profile, together with local station data and the logbook. |

![Who hears me? with favorites, logbook colors, call history, and calling tools](docs/images/psk-reporter-who-hears-me.png)

*Example view using test data. Actual row colors follow your WSJT-X settings.*

## Using FT2

Select **Mode → FT2**. The app uses 3.75-second TX/RX periods, the usual
standard messages, and Auto Seq. FT2 decodes use the `*` marker. Existing
settings are preserved; FT2 frequency defaults are added once if you have
no custom FT2 entries.

The integration adapts [WSJT-X Improved / WS Suite](https://wsjt-x-improved.sourceforge.io/)
by **Uwe Risse, DG2YCB** (GPLv3). FT2 remains experimental. Transmit/receive
WAV round trips and two official FT2 recordings passed; a user screenshot
also documents an FT2 on-air QSO on 16 October 2026. See [FT2 documentation and sources](docs/FT2.md).

## Using the features

Open **View → PSK Reporter — Who hears me?**. The view requests reception
reports for your configured callsign in the current mode. Mode and band are
read directly from the running application. It covers the last 30 minutes
on the current band; **Current frequency and mode** optionally limits the
display to the dial frequency through 5 kHz above it.
Only the latest report for each receiver is shown. Requests are limited to
one every five minutes; automatic refresh runs while the view is open.
The Refresh button shows the remaining wait. Request failures preserve existing
reports. Direct calls and Band Activity markers still require reports within
the current working frequency range.

The **QSO status** column shows **New callsign**, **Worked**, or
**Worked on band**. Colors and logbook filters also update after a new
logbook entry or changes to your highlighting rules.

### Calling a station

1. Select a station in the list.
2. Click **Call station**, press **Ctrl+Enter**, or double-click the row
   outside the favorites column.
3. The app fills in the callsign and locator, selects **Tx1**, and turns on
   **Enable Tx**. The call is transmitted in the next configured TX period,
   using your current TX frequency and TX sequence.

Direct calling supports **FT2, FT8, FT4, JT65, JT9, and Q65** in normal QSO operation
with Tx1 enabled. If transmission is already in progress, Tune or file playback
is active, or a special operating activity is selected, the view explains
why the call cannot start. After changing your own callsign, mode, or dial
frequency, refresh the receiver list before calling a station from it.

Reception reports are delayed. They show that a station received your signal,
but do not provide a reliable TX frequency or TX sequence for the receiver
or confirm that it is currently ready for a QSO.
**Open PSK Reporter map** also opens the
[PSK Reporter map](https://www.pskreporter.info/pskmap.html) in your browser.

### Calling stations again and choosing who to call

- **Favorite** saves the selected station as a favorite. A `*` in **Fav.**
  marks it. Double-clicking this column toggles the marker. The **Favorites**
  filter shows saved favorites present in the current receiver list.
- **Call recent** recalls the selected history entry. **Repeat last** calls
  the most recently called station again, even without a current reception
  report. Your own callsign, mode, and dial frequency must match the saved call.
- **Call next** follows the current filters and sorting, preferring a station
  that has not yet been called in the current context. Each click starts
  one call.
- **Calls** counts call attempts started here; **Last call (UTC)** shows
  their most recent start time. Completed QSOs are reflected in the logbook
  status.
- **Halt Tx** performs the same stop action as in the main window.

Favorites and call history are saved in the current configuration profile
and survive a restart. Receiver markers in **Band Activity** disappear after
a callsign, mode, or frequency change, or at the latest 30 minutes after
the corresponding reception report.

### Keyboard shortcuts in the PSK window

| Shortcut | Action |
| --- | --- |
| **Ctrl+F** | Focus the search field |
| **Ctrl+Enter** | Call the selected station |
| **Ctrl+R** | Call the last station again |
| **Ctrl+N** | Call the next matching station |
| **Ctrl+Shift+S** | Halt Tx |

## Building on Windows

The scripts expect **MSYS2/MinGW64 at `C:\msys64\mingw64`** with CMake,
Ninja, GCC/G++, Fortran, Qt5, Boost, FFTW, and libusb. Qt must provide
Widgets, Network, Multimedia, PrintSupport, SerialPort, Sql, WebSockets,
ActiveQt, SVG, and the Qt tools.

`Setup-CustomBuild.ps1` downloads the **Hamlib 4.7.2 SDK** with SHA256
verification and the commit- and SHA256-pinned OmniRig type library into `.tools`.
`WSJTX_MINGW_PREFIX` can override the default toolchain path.
MSYS2 and its packages must already be installed.

Run these commands from the repository directory:

```powershell
.\tools\Setup-CustomBuild.ps1
.\tools\Build-Custom.ps1
.\tools\Test-Core.ps1
.\tools\Test-TxQuality.ps1
.\tools\Test-PskReporter.ps1
.\tools\Test-Ft2.ps1
.\tools\Test-Reist16.ps1
.\tools\Package-Custom.ps1
```

To perform a clean MinGW Makefiles build and update the **single shared
package** under `dist-custom`:

```powershell
.\tools\Rebuild-AllCustom.ps1
```

This uses `build-custom-make`, refreshes the shared programs and runtime
libraries, and verifies their SHA256 hashes without importing station profiles.
All launchers use this package with independent station profiles.
Logs and the package inventory are written to `tmp/rebuild-all-…`.

The single custom build is placed in `build-custom-make`; the package with runtime
libraries, decoders, and station data is placed in `dist-custom`.
The additional **REIST16** mode uses 15-second UTC slots for short CQ/QSO messages.
See [REIST16.txt](docs/REIST16.txt) for operation, the wire profile and test limits.
**start-pluto.bat** launches a second instance using PlutoSDR through the local
CAT/USB-audio bridge: [Pluto setup](docs/PLUTO-WSJTX.txt).
The scripts build WSJT-X and its supporting utilities. MAP65 and the
additional simulators are not built for this package.

### Importing existing settings and starting the app

Before the **first** start, you can import the configuration from your
installed WSJT-X into the custom profile:

```powershell
.\tools\Copy-InstalledConfiguration.ps1
.\Start-Custom.ps1
```

The import reads `%LOCALAPPDATA%\WSJT-X\WSJT-X.ini` and creates
`%LOCALAPPDATA%\WSJT-X - Custom\WSJT-X - Custom.ini`. The INI file is copied
byte for byte and verified using SHA256. Saved configurations, settings,
and file paths are preserved; existing station data and logbooks are also
copied. A private backup is placed in `.local`. If the custom profile already
exists, the import stops and preserves it.

`Start-Custom.ps1` launches the package with `--rig-name Custom`. Use this
script for subsequent starts. Close any older custom version normally
before starting the new package.

Further details are available in [CUSTOM-BUILD.md](CUSTOM-BUILD.md) (German).
Local configurations, API responses, tools, and build outputs in `.local`,
`.tools`, `build-custom-make`, and `dist-custom` are excluded from Git.

## Checks

`Test-Core.ps1` checks buffered ADIF records at EOF, UDP target instance IDs,
and server changes during a pending DNS lookup using local fixtures.

`Test-PskReporter.ps1` uses a local HTTP test server and Qt in offscreen mode.
It checks reception filters, the latest report per station, request throttling,
malformed XML responses, colors and logbook status, Band Activity markers,
favorites and saved calls, recall in the correct context, next-station
selection, and the Halt Tx callback. These checks do not transmit over the air.

To verify the package runtime without an original installation or imported
configuration, run:

```powershell
.\tools\Verify-CustomPackage.ps1 -PortableOnly
```

The first [release 3.0.2-oe3sro.1](https://github.com/rudolfstepan/wsjtx/releases/tag/oe3sro-v3.0.2-1)
passed the complete CI build, including PSK, FT2/FT4, and package checks.
The public ZIP downloads and checksums were verified.

After importing the configuration, you can also check the package:

```powershell
.\tools\Verify-CustomPackage.ps1
```

This local check verifies the packaged runtime, OpenSSL, and preservation
of the custom configuration. For version comparison, the script expects
the original installation at `C:\WSJT\wsjtx\bin\wsjtx.exe`.

## Foundation and acknowledgments

These additions build on the work of the **WSJT Development Team** and
all WSJT-X contributors. Reception data is provided by
[PSK Reporter](https://www.pskreporter.info/pskdev.html). The station filters
are inspired by the
[GridTracker Call Roster](https://docs.gridtracker.org/latest/Making-GridTracker-Work-For-You/Using-Call-Roster.html).

The original project description and its license and trademark notices
are retained below.

---

## Weak Signal Communication Software — Upstream

### Programs

*WSJT-X*, *MAP65*, and *QMAP* are open-source, multi-platform programs designed for weak-signal digital communication by amateur radio. *WSJT-X* works with a standard SSB transceiver, while *MAP65* and *QMAP* use wideband SDR-style hardware. The programs are open source, free of charge, and licensed under the GNU General Public License.

*WSJT-X* implements communication protocols or "modes" called **FST4**, **FST4W**, **FT4**, **FT8**, **JT4**, **JT9**, **JT65**, **MSK144**, **Q65**, **WSPR**, and **Echo**. The first nine modes were designed for making reliable, confirmed QSOs in a wide variety of weak-signal propagation circumstances. These modes use timed Transmit/Receive sequences of specific lengths, synchronized with UTC. **WSPR** mode is for probing potential propagation paths with low-power transmissions, and **Echo** mode is for detecting and measuring reflections of your own signals from the Moon.

*MAP65* implements a wideband receiver for **JT65** and **Q65** signals, optimized for EME on the VHF/UHF bands. It can be used together with *Linrad* (by SM5BSZ) or with direct input from a soundcard, FUNcube Dongle, or similar hardware. The program decodes all **JT65** or **Q65** signals in a passband up to 90 kHz wide, producing a sorted band map of decoded callsigns. In a dual-polarization system, *MAP65* optimally matches the linear polarization angle of each signal, thereby eliminating problems with Faraday rotation and spatial polarization offsets. *MAP65* also handles T/R switching and generates suitable messages and audio waveforms for the selected mode.

*QMAP* is similar to *MAP65* in providing wideband reception of signals over a full EME sub-band. It works cooperatively with *WSJT-X*, supporting the **Q65** mode in both 30-second and 60-second submodes. The *QMAP* + *WSJT-X* combination provides full rig control and Doppler compensation for the EME (Earth-Moon-Earth) path.

### Mode Details

**JT4**, **JT9**, and **JT65** use nearly identical message structure, efficient compression of messages for minimal QSOs, and 60-second T/R sequences. **JT4** and **JT65** were designed for EME on the VHF, UHF, and microwave bands, while **JT9** is optimized for the MF and HF bands. **JT9** is about 2 dB more sensitive than **JT65** while using less than 10% of the bandwidth.

**FT4** and **FT8** use T/R cycles of only 7.5 and 15 s, respectively. They have become extremely popular for world-wide DXing on the HF bands. **MSK144** is designed for Meteor Scatter on the VHF bands. **Q65** offers submodes with T/R sequence lengths from 15 seconds to 5 minutes, and a wide range of tone spacings. Particular Q65 submodes are highly recommended for EME, ionospheric scatter, and other weak signal work on VHF, UHF, and microwave bands. These modes include message formats explicitly supporting nonstandard callsigns and some popular radio contests.

**FST4** and **FST4W** are designed particularly for the LF and MF bands. On these bands their fundamental sensitivities are better than other *WSJT-X* modes with the same sequence lengths, approaching the theoretical limits for their rates of information throughput. **FST4** is optimized for two-way QSOs, while **FST4W** is for quasi-beacon transmissions of **WSPR**-style messages. **FST4** and **FST4W** do not require the strict, independent time synchronization and phase locking of modes like EbNaut.

### License

Licensed under the GNU General Public License v3.0 — see [COPYING](COPYING).

### Trademark

"WSJT-X," "MAP65," and "QMAP" are trademarks of the WSJT Development Team — see [TRADEMARK.md](TRADEMARK.md) for our policy on use of these names.
