# REIST16-QSO/2

[Deutsch](REIST16) | [English](REIST16-EN)

REIST16 is an experimental mode by OE3SRO for short structured amateur-radio contacts. **QSO/2 is the current on-air format.** It is not compatible with FT8 on air. Both stations need QSO/2; the current receiver also recognizes older QSO/1 signals.

## Setup

1. Synchronize both system clocks and agree on the same USB frequency.
2. Select **Mode → REIST16**.
3. For weak or densely spaced signals, select **Decode → Deep**.
4. **Decode → Enable CQ AP (REIST16)** controls the additional CQ-structure trial. It defaults to enabled and applies from depth 2.
5. Use opposite TX slots: one station at `:00/:30`, the other at `:15/:45`.

Supported exchanges include CQ, callsign/locator, report, R-report, RR73 and 73, including double-click selection and Auto Seq. The six transmit fields accept up to 37 characters, but the text must also be a supported standard message. Free text and special contest formats are outside this profile.

## Improvements in RC3

- **200 instead of 32 output entries:** successful stations do not consume the failed-hypothesis budget.
- **Signal cancellation:** from depth 2, confirmed signals are reconstructed and subtracted; subsequent searches use the cleaned buffer.
- **Weak signals:** depth 3 checks additional coherent frequency hypotheses and an exact GFSK reference for the known synchronization symbols.
- **CQ AP:** uses CQ structure only, without supplied callsigns, locators or expected message text. CRC, re-encoding and payload checks remain required.

A capacity of 200 does not guarantee reception of 200 stations. Dense slots require additional computation. Each transmitter still sends one message per slot.

## Audio and transmit level

Selecting the mode defaults to **Tx 1000 Hz**. You can then choose another transmit position. With **Settings → Radio → Split Operation → Rig/Fake It**, the program shifts the TX VFO in the opposite direction and places all 16 tone centres inside 1500–2000 Hz. The selected RF position is preserved. Without split, the selected audio frequency is output directly.

The previous extra amplitude factor of 0.8 is removed: approximately **+1.94 dB audio** at unchanged downstream attenuation. Digital peak level now matches FT8. **Recheck radio drive and ALC after updating.** This is not a measured RF-power gain.

## Technical parameters

| Parameter | QSO/2 |
| --- | --- |
| UTC slot / nominal start | 15 s / +0.5 s |
| Signal duration | 12.8 s |
| Modulation | 16-GFSK, 200 ms per symbol |
| Tone spacing / outer tone centres | 5 Hz / 75 Hz |
| Message / error protection | 77 bits / CRC14 + LDPC(174,91) |
| TX / RX | 48 kHz / 12 kHz |

75 Hz describes the outer tone centres, not measured occupied bandwidth. The receiver improvements preserve the existing QSO/2 on-air format.

## Operating example

![Yaesu and Pluto exchange using the earlier REIST16-QSO/1 format](https://raw.githubusercontent.com/rudolfstepan/wsjtx/oe3sro-v3.0.2-7-rc3/docs/images/REIST16_QSO.png)

*Original screenshot, 21 October 2026: Yaesu/Custom on the left, Pluto on the right, using the earlier **QSO/1** format. It documents that exchange, not QSO/2 sensitivity. The old tone span visible in the image does not apply to QSO/2.*

REIST16 does not upload PSK Reporter spots, and no registered ADIF mode is claimed. CQ averaging across multiple slots is not implemented. [Measurements and limits](Measurements-and-limits).

[Full profile specification, DE/EN](https://github.com/rudolfstepan/wsjtx/blob/master/docs/REIST16-QSO2.md) · [Repository operating documentation](https://github.com/rudolfstepan/wsjtx/blob/master/docs/REIST16.md)
