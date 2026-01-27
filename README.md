# Bosch Climate 3000i Controller

<img width="873" height="310" alt="Screenshot 2025-09-01 at 21 24 55" src="https://github.com/user-attachments/assets/f44777e7-a79b-47b7-b347-da5eaeea888d" />

A modern web-based controller for Midea air conditioners using ESP8266/ESP12F with a sleek dark-themed interface.

![Climate Control Dashboard](https://img.shields.io/badge/ESP8266-Compatible-blue)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Ready-orange)
![License](https://img.shields.io/badge/License-MIT-green)

## Features

✨ **Modern Web Interface**
- Dark-themed, responsive dashboard
- Real-time AC status monitoring
- Live web console for debugging
- Over-The-Air (OTA) firmware updates

🌡️ **Full Climate Control**
- Power ON/OFF
- Temperature adjustment (17-30°C)
- Mode selection (AUTO, COOL, DRY, HEAT, FAN)
- Fan speed control (AUTO, LOW, MEDIUM, HIGH)
- Swing mode control
- Turbo and Eco modes

🔧 **Easy Setup**
- WiFi configuration via captive portal
- No hardcoded credentials
- Automatic reconnection
- Fallback AP mode

📊 **Real-Time Monitoring**
- WebSocket-based live logging
- AC status via REST API
- Temperature and mode display
- Connection status indicators

## Hardware Requirements
The necessary files BOM / Gerber / Pick and Place file are available on hardware/ for download.

- **ESP8266/ESP12E/ESP12F** microcontroller
- **UART connection** to Bosch Climate 3000i control board
- **Power supply** (3.3V for ESP module)

## Pin Configuration

| Function | GPIO Pin | Description |
|----------|----------|-------------|
| AC TX | GPIO1 | Hardware UART TX to AC |
| AC RX | GPIO3 | Hardware UART RX from AC |
| Debug TX | GPIO2 | Serial1 debug output |
| Power LED | GPIO2 | Built-in LED (optional) |

**Baud Rates:**
- AC Communication: 9600 baud (GPIO1/GPIO3)
- Debug Output: 115200 baud (GPIO2)

## Software Requirements

- [PlatformIO](https://platformio.org/) (VS Code extension recommended)
- [Visual Studio Code](https://code.visualstudio.com/)


## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

  Written by [@snackk](https://github.com/snackk)
