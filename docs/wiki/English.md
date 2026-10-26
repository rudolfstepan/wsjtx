# WSJT-X OE3SRO Custom — English overview

**build by OE3SRO** · [Download latest release](https://github.com/rudolfstepan/wsjtx/releases/latest) · [Deutsch](Home)

Current download: **3.0.2-oe3sro.7-rc3**, based on **WSJT-X 3.0.2-devel**. RC3 is designated Latest on GitHub at the project owner's request. Its RC3 version name is retained; REIST16 and LAN web control remain experimental. The release was built and verified locally while GitHub Actions is disabled.

![English LAN web interface with FT8 reception](https://raw.githubusercontent.com/rudolfstepan/wsjtx/oe3sro-v3.0.2-7-rc3/docs/images/web-control-ft8-en-2026-10-25.png)

*Original screenshot, 25 October 2026: FT8 reception on 20 m. It demonstrates the browser interface, not a completed QSO.*

## Features

- **[REIST16-QSO/2](REIST16):** experimental structured CQ/QSO mode, 15-second slots, multisignal decoding with cancellation, additional weak-signal methods and optional CQ-structure AP. Select Mode → REIST16 and Decode → Deep. It is not compatible with FT8 on air.
- **[FT8 / FT4 / FT2](FT8-FT4-FT2):** native C++ kernels, parallel processing and TX diagnostics. This is not a complete C++ rewrite. FT2 was adapted from DG2YCB's GPLv3 WSJT-X Improved implementation.
- **[Who hears me?](PSK-Reporter):** integrated PSK Reporter receiver list, logbook colours, filters, favourites, recent calls and direct calling tools.
- **[Personal DX windows](DX-Zeitfenster):** local empirical reception history, not a live propagation or QSO-probability forecast.
- **[LAN web control](Websteuerung):** German/English main operating panel, logging and a real desktop waterfall image. Setup remains on the desktop.
- **[Pluto / HackRF](Funkgeraete-und-Audio):** separate profiles with local TCI bridges for control and audio.
- **[WebSDR / WASAPI](WebSDR-und-WASAPI):** decode browser audio directly, using an additional receive-only instance or the combined Yaesu-TX/WebSDR-RX option.

## Install and update

Extract the **entire windows-x64.zip** and run **Start-WSJT-X.bat**. Keep the DLLs and data with the program. Configure your station, radio and audio in Settings. The offline guide is `docs/index.html`; a separate documentation ZIP and SHA256 checksums are provided. Existing Custom profiles are reused without a fresh import.

The REIST16 transmit level is approximately **1.94 dB higher** than the earlier version at unchanged downstream attenuation. Recheck radio drive and ALC. Rig/Fake It keeps all REIST16 tone centres inside 1500–2000 Hz while preserving the selected RF position.

## Evidence and scope

Fixed synthetic comparisons and real WebSDR reception show improvements, including additional successful weak REIST16 decodes. They do not establish universal FT8 superiority or real-time performance under all conditions. Raw field recordings remain private. [Measurements and limits](Messungen-und-Grenzen) · [English repository README](https://github.com/rudolfstepan/wsjtx/blob/master/README.en.md).
