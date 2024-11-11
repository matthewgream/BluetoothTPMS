
Insights from https://github.com/andi38/TPMS.

The device (an inexpensive Tyre Pressure Monitoring System) emits 7 bytes in the manufacturer data field of its advertisement.

It's simple to decode and store this data, other than the 16 bit checksum which is of an unknown algorithm. This is an example output below. Note the MAC address will vary depending on vendor. You can typically view 2-3 bytes of the address in the application (e.g. SYTPMS).

```

BLE advertised device [38:89:00:00:36:02] -- Name: , Address: 38:89:00:00:36:02, manufacturer data: 801e180097a492, serviceUUID: 000027a5-0000-1000-8000-00805f9b34fb, rssi: -65
BLE found TPMS device: 38:89:00:00:36:02
Device:      address=38:89:00:00:36:02, name=N/A, rssi=-65, txpower=N/A
Pressure:    0.6 psi
Temperature: 24 C°
Battery:     3.0 V
Alarm:       10000000 (ZeroPressure)
    0011: <PAYLOAD>
    0000: 03 03 A5 27 03 08 42 52  08 FF 80 1E 18 00 97 A4   ...'..BR ........ 
    0010: 92                                                 .         

```
