# ESP-NOW Sniffer

ESP-NOW Sniffer is a promiscuous-mode ESP-NOW packet capture tool for ESP32 microcontrollers. It passively monitors WiFi management frames on channels 1, 6, 11, and 13, identifies Espressif ESP-NOW action frames by their vendor-specific OUI, extracts the serial payload, and forwards it to the serial port. The firmware supports the ESP32, ESP32-C3, and ESP32-S3 with configurable serial output (USB or UART) and optional LED status indicators (standard or NeoPixel/WS2812).

The sniffer automatically locks onto a unicast ESP-NOW pair by observing traffic patterns — the device transmitting the most packets is selected as the target. A hardcoded MAC can also be provided to skip auto-detection. Once locked on, only frames from the target are forwarded while all other traffic is discarded. If the link goes idle for five seconds the sniffer automatically rescans for activity.
