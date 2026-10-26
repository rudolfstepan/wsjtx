# WSJT-X OE3SRO Custom

[Deutsch](Home) | [English](English)

**build by OE3SRO** · Digital modes, reception tools and SDR integration.

This version extends WSJT-X with **REIST16**, optimized FT8/FT4/FT2 kernels, an integrated PSK Reporter receiver list, personal DX windows, and a browser interface for LAN station operation.

**[Download for Windows](https://github.com/rudolfstepan/wsjtx/releases/latest)** · [Quick start](Quick-start) · [All features](Features)

Current download: **3.0.2-oe3sro.7-rc3**, based on **WSJT-X 3.0.2-devel**. RC3 is designated GitHub **Latest** at the project owner's request; its RC3 version name is retained. REIST16 and web control remain experimental extensions. The current download was built and validated locally; GitHub Actions is disabled at present.

![English browser interface with FT8 reception and live waterfall](https://raw.githubusercontent.com/rudolfstepan/wsjtx/oe3sro-v3.0.2-7-rc3/docs/images/web-control-ft8-en-2026-10-25.png)

*Original screenshot, 25 October 2026: FT8 reception on 20 m. Desktop and browser operate the same running station. This image does not document a completed QSO.*

## Main extensions

| Area | What the Custom version provides |
| --- | --- |
| [REIST16](REIST16-EN) | Dedicated CQ/QSO mode, multisignal reception, cancellation and additional weak-signal methods. |
| [FT8 / FT4 / FT2](FT8-FT4-FT2-EN) | Native C++ kernels, parallel processing and shared TX diagnostics. |
| [PSK Reporter](PSK-Reporter-EN) | Who hears me? with logbook colours, filters, favourites and direct calling tools. |
| [Personal DX windows](Personal-DX-windows) | Local reception history by DXCC, band, UTC hour and station setup. |
| [Web control](Web-control) | Main operating panel and live waterfall in the browser, in German and English. |
| [Radios and audio](Radio-and-audio) | Conventional CAT/USB audio and dedicated Pluto/HackRF bridges. |
| [WebSDR / WASAPI](WebSDR-and-WASAPI) | Decode browser audio directly, including an additional receiver window. |

## Find your way

- **Getting started:** [Installation and first reception](Quick-start).
- **Explore the interface:** [Screenshots with explanations](Screenshots-EN).
- **Assess the evidence:** [Measurements, field tests and limits](Measurements-and-limits).
- **Build it yourself:** [Build and development](Build-and-development).
- **Troubleshoot:** [Frequently asked questions](FAQ-EN).

The Custom version builds on the work of the WSJT Development Team. FT2 was adapted from the GPLv3 implementation by **DG2YCB / WSJT-X Improved**. [Source and licence](https://github.com/rudolfstepan/wsjtx) · [Change history, DE/EN](https://github.com/rudolfstepan/wsjtx/blob/master/docs/CHANGES.md).
