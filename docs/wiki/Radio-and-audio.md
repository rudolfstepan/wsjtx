# Radios, audio and profiles

[Deutsch](Funkgeraete-und-Audio) | [English](Radio-and-audio)

All local launchers share **dist-custom/bin**. Separate WSJT-X profiles keep different radios and tasks apart; separate copies of the executable are not required.

| Task | Profile | Connection |
| --- | --- | --- |
| Normal station, for example Yaesu | Custom | Existing CAT/PTT and sound-card/USB-audio settings. |
| Additional browser-audio receiver | WebSDR | WASAPI loopback, explicitly receive-only. |
| PlutoSDR | Pluto | Local TCI bridge, normally port 40002. |
| HackRF | HackRF | Local TCI bridge, normally port 40003. |
| HackRF FT8 on 17 m | HackRF-FT8-17m | Separate first-start defaults: FT8/18.100 MHz, TCI 40004. |

Profiles are stored under `%LOCALAPPDATA%`. Existing settings are preserved. Updates do not require a new import. Simultaneous transmitter instances must not compete for the same CAT port or SDR.

## Conventional transceiver

Configure the radio, CAT/PTT and audio in **Settings**. For USB audio, select the correct input and output. Rig Split or Fake It can keep actual audio in the filter centre while compensating the dial shift. Digital attenuation is not a measurement of RF power.

## PlutoSDR

`start-pluto.bat` starts the bridge and a separate Pluto instance. Frequency, PTT and audio travel together over TCI; no virtual audio cable is needed. Device URI and frequency can be supplied to the PowerShell launcher. Closing the instance shuts down the bridge gracefully.

Only callsign and locator are copied from Custom when the profile is first created. CAT/audio settings and logbook remain separate. [Detailed Pluto setup and limits, German](https://github.com/rudolfstepan/wsjtx/blob/master/docs/PLUTO-WSJTX.txt).

## HackRF One

`start-hackrf.bat` starts a dedicated HackRF instance. `start-hackrf-ft8-17m.bat` uses a separate profile initially configured for FT8/18.100 MHz. Later frequency/mode choices persist. HackRF is half-duplex: reception is switched for transmission.

The 17-m option deliberately defaults to high TX settings: TX VGA 47 dB, IQ amplitude 1.0 and the internal RF amplifier during transmission. These settings do not specify calibrated output power. Check parameters and **Enable Tx** before operating.

The locally built **RC3 package includes** Pluto and HackRF runtime files under `bin/pluto` and `bin/hackrf`. Check the contents of self-built or other packages. This does not automatically install Windows USB drivers. Select a serial number when multiple HackRF devices are connected.

The software reports USB/audio failures and discards late TX requests instead of replaying them later. Simulation tests do not replace full RF acceptance testing. [Detailed HackRF guide, German](https://github.com/rudolfstepan/wsjtx/blob/master/docs/HACKRF-WSJTX.txt).
