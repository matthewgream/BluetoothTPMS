
// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------

#include <BLEAdvertisedDevice.h>

// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------

#include <algorithm>
#include <numeric>
#include <vector>
#include <optional>

// -----------------------------------------------------------------------------------------------

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
        const auto join = [](const std::vector<String>& elements, const String& delimiter) -> String {
            return elements.empty() ? String() : std::accumulate(std::next(elements.begin()), elements.end(), elements[0], [&delimiter](const String& a, const String& b) {
                return a + delimiter + b;
            });
        };
        const auto a = alarmStrings();
        return a.empty() ? String() : String("(" + join(a, ",") + ")");
    }

    virtual void dumpDebug(void) const {
        const auto toBinaryString = [](uint8_t value) -> String {
            char binStr[8];
            for (int i = 7; i >= 0; i--)
                binStr[i] = (value & (1 << (7 - i))) ? '1' : '0';
            return String(binStr, 8);
        };
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

    explicit TpmsDataBluetooth(BLEAdvertisedDevice& device)
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
        decode(reinterpret_cast<const uint8_t*>(device.getManufacturerData().c_str()), device.getManufacturerData().length());
    }
};

// -----------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------
