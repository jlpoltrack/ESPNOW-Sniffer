//*******************************************************
// ESP-NOW Sniffer
//*******************************************************
// Promiscuous ESP-NOW sniffer — captures bridge telemetry
// and forwards it to the serial port.
// For use with ESP32.
// To use USB on ESP32C3 and ESP32S3, 'USB CDC On Boot' must be enabled in Tools.
//********************************************************
// 26. Mar. 2026
//********************************************************


//-------------------------------------------------------
// user configuration
//-------------------------------------------------------

#define BAUD_RATE           115200 // baudrate for serial connection

//#define USE_SERIAL1                // uncomment to use Serial1 instead of USB Serial for ESP32C3 and ESP32S3
//#define TX_PIN              43     // Serial1 TX pin
//#define RX_PIN              44     // Serial1 RX pin

//#define DEVICE_HAS_SINGLE_LED      // uncomment for single on/off LED
//#define DEVICE_HAS_SINGLE_LED_RGB  // uncomment for single RGB (NeoPixel/WS2812) LED
//#define LED_IO              48      // LED pin (comment out to disable)
//#define LED_ACTIVE_LOW             // uncomment if LED is active low (on = LOW)
//#define RGB_LED_COUNT       1      // number of RGB LEDs (default 1)

// hardcode the bridge MAC to skip auto-detection.
// fill in the 6 bytes of the wireless bridge's MAC address, e.g.:
//#define BRIDGE_MAC  { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF }


//-------------------------------------------------------
// platform includes
//-------------------------------------------------------

#include <WiFi.h>
#include <esp_wifi.h>


//-------------------------------------------------------
// serial port
//-------------------------------------------------------

#ifdef USE_SERIAL1
  #if !defined(TX_PIN) || !defined(RX_PIN)
    #error TX_PIN and RX_PIN must be defined when USE_SERIAL1 is enabled.
  #endif
  #define SERIAL_PORT  Serial1
#else
  #define SERIAL_PORT  Serial
#endif


//-------------------------------------------------------
// includes
//-------------------------------------------------------

#include "leds.h"
#include "ring_buffer.h"
#include "espnow_sniffer.h"


//-------------------------------------------------------
// wifi init + channel scan
//-------------------------------------------------------

const uint8_t scan_channels[] = { 1, 6, 11, 13 };

void scan_for_traffic(void)
{
    espnow_frame_received = false;
    while (true) {
        for (int i = 0; i < (int)sizeof(scan_channels); i++) {
            esp_wifi_set_channel(scan_channels[i], WIFI_SECOND_CHAN_NONE);

            unsigned long t = millis();
            while (millis() - t < 500) {
                led_tick_scanning();
                if (espnow_frame_received) return;
                delay(1);
            }
        }
    }
}

void setup_wifi(void)
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // set country to EU to enable channels 1-13
    wifi_country_t country = { .cc = "EU", .schan = 1, .nchan = 13, .policy = WIFI_COUNTRY_POLICY_MANUAL };
    esp_wifi_set_country(&country);

    // force 11b only (matches the bridge)
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B);

    // enable promiscuous mode, filter for management frames only
    wifi_promiscuous_filter_t filt = { .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT };
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_promiscuous_rx_cb(promisc_recv_cb);
    esp_wifi_set_promiscuous(true);

    // scan channels until we find espnow traffic
    scan_for_traffic();
}


//-------------------------------------------------------
// setup() and loop()
//-------------------------------------------------------

bool is_connected;
unsigned long is_connected_tlast_ms;
uint8_t buf[250];


void setup()
{
    led_init();

#if defined(USE_SERIAL1)
    SERIAL_PORT.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
#else
    SERIAL_PORT.begin(BAUD_RATE);
#endif

    rxbuf_init();
    detect_init();
    setup_wifi();

    is_connected = true;
    is_connected_tlast_ms = millis();
}


void loop()
{
    unsigned long tnow_ms = millis();

    // check if auto-detection is ready to decide
    detect_check();

    // timeout — rescan if no frames for 5 seconds
    if (is_connected && (tnow_ms - is_connected_tlast_ms > 5000)) {
        is_connected = false;
        detect_init();
        rxbuf_init(); // flush stale data so it doesn't falsely re-trigger connected
        scan_for_traffic();
        is_connected = true;
        is_connected_tlast_ms = millis();
    }

    // LED: pattern based on connection state
    if (is_connected) {
        led_tick_connected();
    } else {
        led_tick_disconnected();
    }

    // drain ring buffer to serial
    int len = rxbuf_pop(buf, sizeof(buf));
    if (len > 0) {
        SERIAL_PORT.write(buf, len);
        is_connected = true;
        is_connected_tlast_ms = tnow_ms;
    }

    delay(2);
}
