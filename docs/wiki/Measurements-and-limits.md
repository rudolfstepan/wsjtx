# Measurements, field tests and limits

[Deutsch](Messungen-und-Grenzen) | [English](Measurements-and-limits)

The Custom version is checked with reproducible software tests and real reception recordings. These types of evidence support different conclusions.

## REIST16

| Test | Observation | Limit of the evidence |
| --- | --- | --- |
| Dense synthetic mixtures, two seeds | Depth 3 receives, among other cases, 48 stations spaced 50 Hz apart and 60 spaced 60 Hz apart. | 48 × 50 Hz produces a 2425-Hz outer-tone-centre span, not evidence of reception inside an exact 2400-Hz filter. |
| Independent mixed test at −22 dB SNR2500 | Previously 7/16, now 11/16; 10/16 with CQ AP disabled. | Fixed finite set; one QSO case still trails FT8 at −20 dB, and no decoder succeeds at −24/−26 dB in this series. |
| Same real recording with added noise | At 8.5 dB additional digital attenuation: 7/10 instead of 4/10; at 9 dB: 2/10 instead of 0/10. | Not a calibrated RF-power change. |
| Four further real recordings | Three REIST16 CQs at estimated −16/−17/−18 dB, all without CQ AP. The last requires depth 3. | No quantified REIST16 advantage over FT8: an FT8 transmission could not be identified reliably. |
| Transmit-level test | Active digital RMS practically matches FT8; waveform preserved without PCM overflow. | Does not establish actual RF power or radio linearity. |

Original field recordings remain private. Published material contains aggregate results, methods and limitations. A capacity of 200 output entries does not promise 200 simultaneously decodable stations.

## Further checks

- FT8/FT4/FT2 kernels against the original implementations, including encoders, LDPC, waveforms and recorded samples.
- Actual modulator/audio-thread handoff and IPC worker without radio hardware.
- Logbook/UDP regressions, a PSK Reporter test server and web-control checks.
- Pluto/HackRF bridges and TCI protocols in simulation.
- Freshly extracted RC3 package with a Windows-only PATH: runtime libraries, branding, resources, a weak CQ case and 48 stations.

## What the tests do not establish

The tests do not establish universal superiority over FT8, complete RF acceptance of every SDR path, or guaranteed real-time performance at arbitrary load. A faster individual kernel does not imply the same speedup for the entire application.

## Evidence

- [Multisignal reception](https://github.com/rudolfstepan/wsjtx/blob/master/docs/validation/reist16-multisignal/IMPROVEMENTS.md)
- [Weak-signal comparison](https://github.com/rudolfstepan/wsjtx/blob/master/docs/validation/reist16-multisignal/WEAK-UPGRADE.md)
- [Digital transmit-level tests](https://github.com/rudolfstepan/wsjtx/blob/master/docs/validation/reist16-multisignal/TX-AUDIO.md)
- [Summary of real reception trials](https://github.com/rudolfstepan/wsjtx/blob/master/docs/validation/reist16-multisignal/FIELD-TESTS.md)
- [RC3 archive and runtime verification](https://github.com/rudolfstepan/wsjtx/blob/master/docs/validation/reist16-multisignal/release-rc3-runtime.json)

The detailed reports retain their repository language; this page provides the complete English summary of observations and limits.
