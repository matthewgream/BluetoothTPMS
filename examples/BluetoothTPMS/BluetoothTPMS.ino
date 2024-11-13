
// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------

#include <Arduino.h>

#include "BluetoothTPMS.hpp"

void dumpDebug (const char *label, const uint8_t *data, const size_t size, const size_t offs = 0);

// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------

#include <BLEDevice.h>
#include <BLEScan.h>

class TpmsScanner : protected BLEAdvertisedDeviceCallbacks {

    void onResult (BLEAdvertisedDevice device) {

        const auto address = device.getAddress ().toString ();
        Serial.printf ("BLE advertised device [%s] -- %s\n", address.c_str (), device.toString ().c_str ());

        if (std::ranges::find (_addresses, address) != _addresses.end ()) {
            Serial.printf ("BLE found TPMS device: %s\n", address.c_str ());

            auto tpms = TpmsDataBluetooth::fromAdvertisedDevice (device);
            if (tpms.valid ())
                tpms.dumpDebug ();

            dumpDebug ("PAYLOAD", device.getPayload (), device.getPayloadLength ());
        }
    }

    static constexpr int SCAN_TIME = 5;
    BLEScan *scanner = nullptr;
    const std::vector<String> _addresses;

public:
    explicit TpmsScanner (const std::initializer_list<String> &addresses) :
        _addresses (addresses) { }

    void scan () {
        Serial.printf ("TpmsScanner::scan (%d)\n", SCAN_TIME);

        if (! scanner->start (SCAN_TIME))
            Serial.println ("BLEScan failed");
    }

    void start () {
        Serial.printf ("TpmsScanner::start\n");

        BLEDevice::init ("");
        scanner = BLEDevice::getScan ();
        scanner->setAdvertisedDeviceCallbacks (this);
        scanner->setActiveScan (false);    // not needed, just need the manufacturer data in the advertisement
        scanner->setInterval (75);
        scanner->setWindow (50);
    }
};

// -----------------------------------------------------------------------------------------------

TpmsScanner tpmsScanner ({ "38:89:00:00:36:02", "38:8b:00:00:ed:63" });

void setup () {
    Serial.begin (115200);
    delay (2 * 1000);
    Serial.println ("UP");
    tpmsScanner.start ();
}
void loop () {
    tpmsScanner.scan ();
}

// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------

void dumpDebug (const char *label, const uint8_t *data, const size_t size, const size_t offs) {
    static constexpr const char lookup [] = "0123456789ABCDEF";
    std::array<char, (16 * 3 + 2) + 1 + (16 * 1 + 2) + 1> buffer;
    Serial.printf ("    %04X: <%s>\n", size, label);
    for (size_t i = 0; i < size; i += 16) {
        auto *position = buffer.data ();
        for (size_t j = 0; j < 16; j++) {
            if ((i + j) < size)
                *position++ = lookup [(data [i + j] >> 4) & 0x0F], *position++ = lookup [(data [i + j] >> 0) & 0x0F], *position++ = ' ';
            else
                *position++ = ' ', *position++ = ' ', *position++ = ' ';
            if ((j + 1) % 8 == 0)
                *position++ = ' ';
        }
        *position++ = ' ';
        for (size_t j = 0; j < 16; j++) {
            if ((i + j) < size)
                *position++ = isprint (data [i + j]) ? (char) data [i + j] : '.';
            else
                *position++ = ' ';
            if ((j + 1) % 8 == 0)
                *position++ = ' ';
        }
        *position++ = '\0';
        Serial.printf ("    %04X: %s\n", offs + i, buffer.data ());
    }
}

// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------
