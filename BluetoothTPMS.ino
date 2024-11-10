
// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------

#include <Arduino.h>

// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------

__attribute__((unused)) static void dump(const char* label, const uint8_t* data, const size_t size, const size_t offs = 0) {
    static constexpr const char lookup[] = "0123456789ABCDEF";
    std::array<char, (16 * 3 + 2) + 1 + (16 * 1 + 2) + 1> buffer;
    Serial.printf("    %04X: <%s>\n", size, label);
    for (size_t i = 0; i < size; i += 16) {
        auto* position = buffer.data();
        for (size_t j = 0; j < 16; j++) {
            if ((i + j) < size)
                *position++ = lookup[(data[i + j] >> 4) & 0x0F], *position++ = lookup[(data[i + j] >> 0) & 0x0F], *position++ = ' ';
            else
                *position++ = ' ', *position++ = ' ', *position++ = ' ';
            if ((j + 1) % 8 == 0)
                *position++ = ' ';
        }
        *position++ = ' ';
        for (size_t j = 0; j < 16; j++) {
            if ((i + j) < size)
                *position++ = isprint(data[i + j]) ? (char)data[i + j] : '.';
            else
                *position++ = ' ';
            if ((j + 1) % 8 == 0)
                *position++ = ' ';
        }
        *position++ = '\0';
        Serial.printf("    %04X: %s\n", offs + i, buffer.data());
    }
}

__attribute__((unused)) static String toBinaryString(uint8_t value) {
    char binStr[8];
    for (int i = 7; i >= 0; i--)
        binStr[i] = (value & (1 << (7 - i))) ? '1' : '0';
    return String(binStr, 8);
}

#include <algorithm>
#include <numeric>
#include <vector>

__attribute__((unused)) static String join(const std::vector<String>& elements, const String& delimiter) {
    return elements.empty() ? String() : std::accumulate(std::next(elements.begin()), elements.end(), elements[0], [&delimiter](const String& a, const String& b) {
        return a + delimiter + b;
    });
}

// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------

#include <BLEAdvertisedDevice.h>

// -----------------------------------------------------------------------------------------------

/*

as derived from https://github.com/andi38/TPMS

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

*/

struct TpmsData {

    enum Alarm : uint8_t {
        ZeroPressure = 1 << 7,
        Rotating = 1 << 6,
        StandingIdleFor15mins = 1 << 5,
        BeginRotating = 1 << 4,
        DecreasingPressureBelow207Psi = 1 << 3,
        RisingPressure = 1 << 2,
        DecreasingPressureAbove207Psi = 1 << 1,
        Unspecified = 1 << 0,
        LowBattery = 0xFF
    };

    uint16_t pressure{};
    uint8_t temperature{};
    uint8_t battery{};
    uint8_t alarms{};

    bool _valid{ false };

    std::vector<String> alarmStrings() const {
        static const std::pair<Alarm, const char*> mappings[] = {
            { Alarm::ZeroPressure, "ZeroPressure" },
            { Alarm::Rotating, "Rotating" },
            { Alarm::StandingIdleFor15mins, "StandingIdleFor15mins" },
            { Alarm::BeginRotating, "BeginRotating" },
            { Alarm::DecreasingPressureBelow207Psi, "DecreasingPressureBelow207Psi" },
            { Alarm::RisingPressure, "RisingPressure" },
            { Alarm::DecreasingPressureAbove207Psi, "DecreasingPressureAbove207Psi" },
            { Alarm::Unspecified, "Unspecified" },
            { Alarm::LowBattery, "LowBattery" }
        };
        std::vector<String> result;
        for (const auto& [alarm, string] : mappings)
            if ((alarms & static_cast<uint8_t>(alarm)) == static_cast<uint8_t>(alarm))
                result.push_back(string);
        return result;
    }
    String toString(Alarm) const {
        auto a = alarmStrings();
        return a.empty() ? String() : String("(" + join(a, ",") + ")");
    }

    virtual void dumpDebug(void) const {
        Serial.printf("Pressure:    %.1f psi\n", static_cast<float>(pressure) / 10.0);
        Serial.printf("Temperature: %u C°\n", temperature);
        Serial.printf("Battery:     %.1f V\n", static_cast<float>(battery) / 10.0);
        Serial.printf("Alarm:       %s %s\n", toBinaryString(alarms).c_str(), toString(static_cast<Alarm>(alarms)).c_str());
    }

    void decode(const uint8_t* data, size_t size) {
        alarms = data[0];
        battery = data[1];
        temperature = data[2];
        pressure = ((static_cast<uint16_t>(data[3]) << 8) | static_cast<uint16_t>(data[4])) - 145;
        uint16_t checksum = (static_cast<uint16_t>(data[5]) << 8) | static_cast<uint16_t>(data[6]);
        (void)checksum;
    }

    bool valid() const {
        return true;    // for now
    }
    TpmsData(const uint8_t* data, size_t size) {
        decode(data, size);
    }
    TpmsData() {}
};

#include <optional>

// -----------------------------------------------------------------------------------------------

struct TpmsDataBluetooth : public TpmsData {

    const BLEAddress address;
    const std::optional<String> name;
    const std::optional<int> rssi;
    const std::optional<uint8_t> txpower;

    TpmsDataBluetooth(BLEAdvertisedDevice& device)
        : address(device.getAddress()),
          name(device.haveName() ? std::optional<String>(device.getName()) : std::nullopt), rssi(device.haveRSSI() ? std::optional<int>(device.getRSSI()) : std::nullopt), txpower(device.haveTXPower() ? std::optional<uint8_t>(device.getTXPower()) : std::nullopt) {
        import(device);
    }

    void dumpDebug(void) const override {
        Serial.printf("Device:      address=%s, name=%s, rssi=%s, txpower=%s\n", const_cast<BLEAddress&>(address).toString().c_str(), name.has_value() ? name.value().c_str() : "N/A", rssi.has_value() ? String(*rssi).c_str() : "N/A", txpower.has_value() ? String(*txpower).c_str() : "N/A");
        TpmsData::dumpDebug();
    }

    static TpmsDataBluetooth fromAdvertisedDevice(BLEAdvertisedDevice& device) {
        return TpmsDataBluetooth(device);
    }

private:
    void import(BLEAdvertisedDevice& device) {
        decode((uint8_t*)device.getManufacturerData().c_str(), device.getManufacturerData().length());
    }
};

// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------

#include <BLEDevice.h>
#include <BLEScan.h>

class TpmsScanner : protected BLEAdvertisedDeviceCallbacks {

    void onResult(BLEAdvertisedDevice device) {

        const auto address = device.getAddress().toString();
        Serial.printf("BLE advertised device [%s] -- %s\n", address.c_str(), device.toString().c_str());

        if (std::ranges::find(_addresses, address) != _addresses.end()) {
            Serial.printf("BLE found TPMS device: %s\n", address.c_str());

            auto tpms = TpmsDataBluetooth::fromAdvertisedDevice(device);
            if (tpms.valid())
                tpms.dumpDebug();

            dump("PAYLOAD", device.getPayload(), device.getPayloadLength());
        }
    }

    static constexpr int SCAN_TIME = 5;
    BLEScan* scanner = nullptr;
    const std::vector<String> _addresses;

public:

    explicit TpmsScanner(const std::initializer_list<String>& addresses)
        : _addresses(addresses) {}

    void scan() {
        Serial.printf("TpmsScanner::scan (%d)\n", SCAN_TIME);

        if (!scanner->start(SCAN_TIME))
            Serial.println("BLEScan failed");
    }

    void start() {
        Serial.printf("TpmsScanner::start\n");

        BLEDevice::init("");
        scanner = BLEDevice::getScan();
        scanner->setAdvertisedDeviceCallbacks(this);
        scanner->setActiveScan(false);    // not needed, just need the manufacturer data in the advertisement
        scanner->setInterval(75);
        scanner->setWindow(50);
    }
};

// -----------------------------------------------------------------------------------------------

TpmsScanner tpmsScanner({ "38:89:00:00:36:02", "38:8b:00:00:ed:63" });

void setup() {
    Serial.begin(115200);
    delay(2 * 1000);
    Serial.println("UP");
    tpmsScanner.start();
}
void loop() {
    tpmsScanner.scan();
}

// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------
