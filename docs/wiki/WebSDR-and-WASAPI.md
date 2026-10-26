# WebSDR and WASAPI loopback

[Deutsch](WebSDR-und-WASAPI) | [English](WebSDR-and-WASAPI)

WASAPI loopback captures the Windows playback endpoint used by the browser directly. It needs neither a virtual audio cable nor a Stereo Mix input.

## Additional receiver

**start-wasapi.bat** or **start-websdr.bat** starts the separate **WebSDR** profile with `--receive-only`. It can receive alongside the normal transmitter instance. The launcher enables no transmitter and does not change the transmitter profile.

1. In the browser, select the desired WebSDR frequency and USB mode.
2. Play browser audio through the Windows endpoint selected in the launcher.
3. Select the matching mode and dial frequency in the receiver window.
4. Enable **Monitor**. For REIST16, select **Decode → Deep**.

The profile remembers its mode/frequency choice. The launcher's default endpoint name matches the original workstation; on other systems, supply the exact name:

```powershell
.\Start-Wasapi.ps1 -PlaybackDevice 'EXACT WINDOWS PLAYBACK ENDPOINT NAME'
```

The entire output mix is captured, including other applications and system sounds.

## Yaesu TX with WebSDR RX

**start-yaesu-websdr.bat** uses exactly **one Custom instance**: CAT/PTT and transmit audio stay with the Yaesu, while reception comes from the browser. Close an existing Custom instance normally first. CAT does not retune the browser automatically.

For a simultaneously visible additional receive-only window, use the normal station and the separate WebSDR profile instead. Do not run the same Custom configuration twice as a transmitter.

## REIST16 self-test

**start-reist16-selftest.bat** prepares separate TX and RX test profiles. The TX profile initially copies the existing Custom configuration; the RX profile has no CAT connection. The launcher enables no transmission. Close the normal Custom instance so only the test transmitter uses the CAT port.

## Account for delay

WebSDR, browser and network buffering can delay audio. Receive-only REIST16 captures through 14.400 seconds of the slot; transceive operation captures through 13.824 seconds to leave time for an automatic reply. This does not guarantee decoding of arbitrarily buffered browser audio. An incomplete frame may not decode.

[Detailed WebSDR guide, German](https://github.com/rudolfstepan/wsjtx/blob/master/docs/WEBSDR-WSJTX.txt)
