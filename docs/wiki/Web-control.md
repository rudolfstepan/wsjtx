# LAN web control

[Deutsch](Websteuerung) | [English](Web-control)

The experimental web interface reproduces the main operating panel of the running WSJT-X instance. Desktop and browser share the same decoder, CAT/audio connection and QSO session.

## Start the server

1. On the station computer, open **View → Web control (experimental)…**.
2. Choose a port, optionally enter your own **Access key**, and press **Start / apply key**. Leaving the field empty generates a random key.
3. Open the displayed address in a browser and connect using the key.
4. Use **Stop web control** to disable the service.

The server starts disabled each time the application starts. Use different ports for multiple instances. The browser interface requires neither an additional web server nor Node.js.

![English web control showing FT8 reception](https://raw.githubusercontent.com/rudolfstepan/wsjtx/oe3sro-v3.0.2-7-rc3/docs/images/web-control-ft8-en-2026-10-25.png)

*Original screenshot, 25 October 2026: OE3SRO/P, JN88, receiving FT8 on 20 m. Magenta rows reproduce desktop formatting. The image documents reception.*

## Operating areas

| Area | Functions |
| --- | --- |
| Status and selection | Callsign, locator, mode, dial frequency and RX/TX state. |
| Decode lists | Band Activity and RX Frequency with desktop colours; double-click/double-tap to select. |
| Actions | Monitor, Stop, Decode, Erase, Enable Tx, Halt Tx and Tune. |
| DX / QSO | Callsign, locator, report, RX/TX audio frequency, Auto Seq, TX slot and Hold Tx Freq. |
| Transmit messages | Select/edit Tx1 through Tx6 and invoke available immediate-send actions. |
| Log QSO | Edit and accept or discard the normal logging confirmation in the browser. |
| Wide Graph | Actual desktop waterfall/spectrum image; Qt controls visible inside the image are not clickable. |

Setup, audio/radio settings and additional special dialogs remain on the desktop. Audio is not streamed to the browser. Change RX/TX values using the numeric fields.

## German and English

![German web control](https://raw.githubusercontent.com/rudolfstepan/wsjtx/oe3sro-v3.0.2-7-rc3/docs/images/web-control-ft8-2026-10-25.png)

Language selection offers **Deutsch**, **English** and **browser language**. The browser remembers the language choice; it does not persist the access key. Changing the language sends no radio control command.

## Connection and access

The key grants operating access, including transmission. Use the service on a trusted LAN or through a VPN: the connection is **HTTP without TLS**. Disconnecting the browser does not stop an ongoing transmission. Use **Halt Tx** or the desktop controls to stop it.

[Detailed repository guide, German](https://github.com/rudolfstepan/wsjtx/blob/master/docs/WEB-CONTROL.txt)
