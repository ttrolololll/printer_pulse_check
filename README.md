# Printer Pulse Check

This project programmed via Arduino IDE uses a ESP32-DevKit to send a print job command to a locally discoverable printer on a regular interval. This is to ensure that we "use" the printer ink often enough so it does not dry out :/

It does so by using the following simple strategy:
- Find the printer IP (usually assigned via DHCP) on local network via mDNS
- Send the appropriate HTTP POST request to the printer server via the IP to trigger a diagnostic print job
- Disconnect Wifi and put ESP32 to deep sleep mode
