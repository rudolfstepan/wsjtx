# PSK Reporter — Who hears me?

[Deutsch](PSK-Reporter) | [English](PSK-Reporter-EN)

**View → PSK Reporter — Who hears me?** lists stations reporting your transmitted signal. Callsign, mode and band come from the running application. The view covers the last 30 minutes and shows each receiver's latest matching report.

![Who hears me with favourites, filters and calling history](https://raw.githubusercontent.com/rudolfstepan/wsjtx/oe3sro-v3.0.2-7-rc3/docs/images/psk-reporter-who-hears-me.png)

*Example using **test data**. Actual colours follow your WSJT-X highlighting rules and local logbook.*

## Display and filters

- Search by callsign or locator.
- Filter reports from the last 5, 15 or 30 minutes.
- Filter new callsigns, new on band, favourites and previously called stations.
- **Current frequency and mode** optionally limits the range to the current dial frequency plus 5 kHz.
- **QSO status** distinguishes new callsigns, worked stations and stations worked on this band.

Reporting stations are marked turquoise, bold and underlined in **Band Activity**, with a reception-time tooltip. Favourites and the last 20 calls are retained in the configuration profile.

## Call a station

Select a row and use **Call station**, **Ctrl+Enter** or double-click. This copies the callsign/locator, selects Tx1 and enables **Enable Tx** for the configured transmit window. Supported modes are FT2, FT8, FT4, JT65, JT9 and Q65 during normal QSO operation.

**Call recent**, **Repeat last** and **Call next** assist with further individual calling attempts. **Halt Tx** performs the same stop as the main window. Reception reports are delayed and do not reliably indicate the receiver's transmit frequency, sequence or present willingness to make a QSO.

| Shortcut | Action |
| --- | --- |
| Ctrl+F | Search |
| Ctrl+Enter | Call the selected station |
| Ctrl+R | Repeat the last call |
| Ctrl+N | Call the next matching station |
| Ctrl+Shift+S | Halt Tx |

## Query cooldown

Regular queries are available at most every five minutes. HTTP 429/503 increases the pause from ten minutes to a maximum of 60 minutes; longer server instructions take precedence. The countdown survives restarts. Errors do not erase existing reports. The browser map may work independently of the API.

REIST16 has no PSK Reporter integration. [Personal DX windows](Personal-DX-windows) use observed reports from the supported modes.
