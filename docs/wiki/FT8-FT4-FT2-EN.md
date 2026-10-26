# FT8, FT4 and FT2

[Deutsch](FT8-FT4-FT2) | [English](FT8-FT4-FT2-EN)

The Custom version preserves the existing on-air formats and replaces selected computationally expensive routines with native C++ kernels. Message packing, parts of synchronization and further decoder stages still use Fortran. This is not a complete C++ port.

| Mode | Cycle | Custom extensions |
| --- | --- | --- |
| FT8 | 15 s | Native waveform/LDPC kernels, existing parallel decoder and TX diagnostics. |
| FT4 | 7.5 s | Native waveform generation and parallel synchronization search. |
| FT2 | 3.75 s | Experimental integration with transmitter, decoder, Auto Seq, waterfall and PSK calling tools. |

FT2 comes from **WSJT-X Improved / WS Suite by DG2YCB**, adapted under GPLv3.

## TX diagnostics and threads

**View → TX quality — FT8 / FT4 / FT2** shows the audio device, digital attenuation, actual audio frequency after split, software start information and buffer information. ALC is displayed when supported by the Hamlib backend. An unknown ALC value is not displayed as zero.

The parallel FT8 decoder offers Auto or up to twelve workers. CPU and OpenMP limits may reduce the actual number. More threads guarantee neither greater sensitivity nor shorter runtime for every signal mixture. FT4/FT2 synchronization automatically parallelizes suitable search grids.

ALC and software timestamps are not complete measurements of distortion, USB latency or RF power.

## Documented FT2 operation

![FT2 QSO between OE3SRO and OG5O on 20 m](https://raw.githubusercontent.com/rudolfstepan/wsjtx/oe3sro-v3.0.2-7-rc3/docs/images/ft2-qso-oe3sro-og5o-2026-10-16.png)

*Original screenshot, 16 October 2026: OE3SRO with OG5O/KP20 in Finland at a 14.084 MHz dial frequency. The report, received RR73 and final transmitted 73 are visible. This is user-documented evidence of an FT2 QSO.*

Native and original kernels were compared using deterministic inputs and recorded FT8/FT4/FT2 samples. Individual kernel benchmarks are not speedup factors for the entire application. [Validation](Measurements-and-limits).

[FT8 details, DE/EN](https://github.com/rudolfstepan/wsjtx/blob/master/docs/FT8.md) · [FT2/FT4 details and attribution](https://github.com/rudolfstepan/wsjtx/blob/master/docs/FT2.md)
