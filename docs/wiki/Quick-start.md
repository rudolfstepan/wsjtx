# Installation and first reception

[Deutsch](Schnellstart) | [English](Quick-start)

## Install the Windows package

1. Open the [latest release](https://github.com/rudolfstepan/wsjtx/releases/latest) and download **windows-x64.zip**.
2. Extract the **entire ZIP** into its own folder. Keep `bin`, `share`, DLLs and documentation together.
3. Optionally verify the accompanying SHA256 checksum:

   ```powershell
   Get-FileHash .\wsjtx-OE3SRO-3.0.2-oe3sro.7-rc3-windows-x64.zip -Algorithm SHA256
   ```

4. Run **Start-WSJT-X.bat**. On first launch, configure your callsign, locator, radio and audio in **Settings**.
5. Enable **Monitor** and check reception first. The system clock must be sufficiently synchronized for the selected mode.

The offline guide is **docs/index.html**. The separate **documentation.zip** contains the guide and measurement reports without the application.

## Choose a launcher

| Task | Launcher |
| --- | --- |
| Normal Custom window in the extracted download | `Start-WSJT-X.bat` |
| Normal Custom window in the source project | `start.bat` |
| Additional browser-audio receiver | `start-wasapi.bat` or `start-websdr.bat` |
| Yaesu TX and browser-audio RX in one instance | `start-yaesu-websdr.bat` |
| PlutoSDR in its own instance | `start-pluto.bat` |
| HackRF in its own instance | `start-hackrf.bat` |
| HackRF FT8 profile, initially on 17 m | `start-hackrf-ft8-17m.bat` |
| Check your REIST16 transmission with a separate WebSDR receiver | `start-reist16-selftest.bat` |

All launchers use the same program package. Separate profiles keep radio settings apart. Do not open the same CAT port or radio with two transmitter instances simultaneously.

## Try REIST16

Select **Mode → REIST16** and **Decode → Deep**. The additional CQ-structure trial is controlled by **Decode → Enable CQ AP (REIST16)**. The other station must also support QSO/2. Agree on the frequency and use opposite TX slots.

The current version raises REIST16 audio by approximately **1.94 dB** compared with the earlier implementation. Recheck drive and ALC on the actual radio. [Complete REIST16 guide](REIST16-EN).

## Update

Close the old instance normally, extract the complete new package into its own folder, and launch it there. Existing Custom profiles are reused; updates do not require another configuration import. [Profiles and audio](Radio-and-audio).
