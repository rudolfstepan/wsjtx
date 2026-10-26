# Feature overview

[Deutsch](Funktionen) | [English](Features)

The Custom extensions complement the existing WSJT-X modes. This overview distinguishes the added features from experimental development status.

## Modes and signal processing

| Feature | Scope | Guide |
| --- | --- | --- |
| REIST16-QSO/2 | 77-bit standard messages, CRC14 + LDPC, 16-GFSK, 15-second slots, Auto Seq and normal QSO exchanges. | [REIST16](REIST16-EN) |
| REIST16 multisignal reception | Output capacity 200; signal cancellation and repeated searches from depth 2. A transmitter still sends one message per slot. | [Measurements](Measurements-and-limits) |
| REIST16 weak-signal methods | GFSK sync reference, multiple coherent hypotheses and separately switchable CQ AP. | [REIST16](REIST16-EN) |
| FT8 / FT4 / FT2 | Native waveform/LDPC kernels and parallel processing of selected decoder stages. | [FT8, FT4, FT2](FT8-FT4-FT2-EN) |
| FT2 | Experimental 3.75-second mode, Auto Seq, waterfall and calling tools; adapted from DG2YCB. | [FT2 on-air example](FT8-FT4-FT2-EN) |
| TX diagnostics | Audio device, digital attenuation, audio frequency after split, start/buffer information and available ALC data. | [FT8, FT4, FT2](FT8-FT4-FT2-EN) |

## Reception reports and calling tools

| Feature | Benefit |
| --- | --- |
| Who hears me? | Receivers reporting your signal, with callsign, locator, SNR, time, frequency and mode. |
| Logbook colours / QSO status | Existing WSJT-X highlighting, Worked, Worked on band and new callsigns. |
| Band Activity markers | Reporting callsigns appear turquoise, bold and underlined, with a reception-time tooltip. |
| Direct calling | Copy callsign/locator, select Tx1 and enable transmission. |
| Filters | Callsign/locator, time, favourites, new callsigns, new on band and calling status. |
| Favourites and history | Save favourites, select the last 20 calls, and repeat the last call. |
| Call next | Select a matching visible station and start one call. |
| Halt Tx | Stop transmission directly from the receiver-list window. |
| Personal DX windows | Observed DXCC/band/month/UTC windows, separated by your callsign, mode and station setup. |

[PSK Reporter operation](PSK-Reporter-EN) · [Personal DX windows](Personal-DX-windows)

## Radios, operation and distribution

- **Web control:** main operating panel in a browser, German/English, actual desktop colours, QSO logging confirmation and live waterfall. [Guide](Web-control).
- **Pluto / HackRF:** separate instances with TCI for frequency, PTT and audio, using independent profiles. [Guide](Radio-and-audio).
- **WASAPI:** decode browser audio without a virtual audio cable, using an additional receive-only window or combined Yaesu TX/WebSDR RX. [Guide](WebSDR-and-WASAPI).
- **Configuration:** an optional first import in the source project creates a separate Custom profile; existing profiles are not overwritten.
- **One package, one build:** `dist-custom` for all launchers and `build-custom-make` for local compilation. Downloads include documentation, runtime libraries and checksums.

REIST16 and web control remain experimental. No complete C++ port, universal superiority over FT8, or multicarrier REIST16 transmitter is claimed. [Validation and limits](Measurements-and-limits).
