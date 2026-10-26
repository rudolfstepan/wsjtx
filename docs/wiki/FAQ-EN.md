# Frequently asked questions and troubleshooting

[Deutsch](FAQ) | [English](FAQ-EN)

## REIST16 is set to Fast or Normal

Select **Decode → Deep** for the additional weak-signal methods. CQ AP is controlled separately by **Enable CQ AP (REIST16)**. Settings apply to each instance/profile.

## The other station cannot receive REIST16

Both stations must support QSO/2. FT8 cannot decode a REIST16 signal. Check system time, USB frequency, audio input, filter passband and reception of the complete frame. [Setup](REIST16-EN).

## REIST16 does not appear on PSK Reporter

REIST16 does not upload PSK Reporter spots. The integrated receiver list does not register an additional mode with the reporting service.

## Tx shows 1000 Hz but the output audio is higher

With Rig Split/Fake It, the application shifts the TX VFO and keeps audio in the filter centre. The selected RF position remains unchanged. Without split, the selected audio frequency is used directly.

## The PSK map works but Refresh is waiting

The browser map and XML API may respond independently. Wait for the query cooldown or HTTP 429/503 pause; repeated clicks do not shorten it. Existing reports remain visible.

## WebSDR is silent or produces no decodes

Select the exact Windows endpoint through which the browser is playing. Use the same USB dial frequency and mode in the browser and WSJT-X. Browser buffering can move complete frames outside the capture window. [WASAPI](WebSDR-and-WASAPI).

## The web interface cannot connect

Start the service on the desktop, use the displayed address/port, and enter the current key. The server is off after an application restart. Setup and modal desktop dialogs may block changes. [Web control](Web-control).

## Different radios need separate installations

The launchers share one package; independent profiles retain radio settings. Use the appropriate Pluto, HackRF or WebSDR launcher. Do not run two instances against the same CAT port.

## Information useful for a problem report

Record the About version, launcher/profile, mode, operating system, audio/radio path and exact error message. For decoder problems, state whether a complete WAV recording is available. Share private configurations, keys and raw recordings only deliberately.
