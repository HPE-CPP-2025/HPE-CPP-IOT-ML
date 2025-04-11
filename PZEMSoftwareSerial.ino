/*
 PZEM004T CSV Logger for Serial Monitor
 Works with manual copy-paste from Serial Monitor
*/

#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>
#include <TimeLib.h>

#if defined(ESP32)
    #error "SoftwareSerial is not supported on ESP32"
#endif

#define PZEM_RX_PIN 12
#define PZEM_TX_PIN 13

SoftwareSerial pzemSerial(PZEM_RX_PIN, PZEM_TX_PIN);
PZEM004Tv30 pzem(pzemSerial);

// Sampling every 1 second
const unsigned long SAMPLE_INTERVAL = 1000;
unsigned long lastSampleTime = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);  // Give serial monitor time to connect

    setTime(compileTime());  // Set initial time
    pzem.resetEnergy();      // Reset energy counter

    Serial.println("PZEM004T CSV Logger - Manual Save via Serial Monitor");
    Serial.println("----------------------------------------------------");
    Serial.println("Commands: T = sync time, R = reset energy, H = print CSV header");
    Serial.println();
    printHeader();  // First CSV line
}

void loop() {
    processSerialCommands();

    if (millis() - lastSampleTime >= SAMPLE_INTERVAL) {
        lastSampleTime = millis();
        logMeasurements();
    }
}

void processSerialCommands() {
    if (Serial.available() > 0) {
        char c = Serial.read();
        if (c == 'T') {
            unsigned long pctime = 0;
            for (int i = 0; i < 10; i++) {
                while (!Serial.available());
                char digit = Serial.read();
                if (digit >= '0' && digit <= '9') {
                    pctime = (10 * pctime) + (digit - '0');
                }
            }
            setTime(pctime);
            Serial.println("Time synced.");
        } else if (c == 'H') {
            printHeader();
        } else if (c == 'R') {
            pzem.resetEnergy();
            Serial.println("Energy meter reset.");
        }
    }
}

void printHeader() {
    Serial.println("Timestamp,Address,Voltage(V),Current(A),Power(W),Energy(kWh),Frequency(Hz),PF");
}

void logMeasurements() {
    String timestamp = getTimestamp();
    uint8_t address = pzem.readAddress();

    float values[] = {
        pzem.voltage(),
        pzem.current(),
        pzem.power(),
        pzem.energy(),
        pzem.frequency(),
        pzem.pf()
    };

    Serial.print(timestamp);
    Serial.print(",");
    Serial.print(address, HEX);

    for (int i = 0; i < 6; i++) {
        Serial.print(",");
        if (isnan(values[i])) {
            Serial.print("Error");
        } else {
            if (i == 3) Serial.print(values[i], 3);  // Energy
            else if (i == 4) Serial.print(values[i], 1);  // Frequency
            else Serial.print(values[i]);  // Default for others
        }
    }
    Serial.println();
}

String getTimestamp() {
    char buffer[20];
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", year(), month(), day(), hour(), minute(), second());
    return String(buffer);
}

time_t compileTime() {
    const char* date = __DATE__;  // e.g. "Apr 09 2025"
    const char* time = __TIME__;  // e.g. "14:25:36"

    static const char* months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    int month = 0;
    for (int i = 0; i < 12; i++) {
        if (strncmp(date, months[i], 3) == 0) {
            month = i + 1;
            break;
        }
    }

    int day = atoi(date + 4);
    int year = atoi(date + 7);
    int hour = atoi(time);
    int minute = atoi(time + 3);
    int second = atoi(time + 6);

    tmElements_t tm;
    tm.Year = year - 1970;
    tm.Month = month;
    tm.Day = day;
    tm.Hour = hour;
    tm.Minute = minute;
    tm.Second = second;

    return makeTime(tm);
}
