# Bosch Climate 3000i Controller

<img width="873" height="310" alt="Screenshot 2025-09-01 at 21 24 55" src="https://github.com/user-attachments/assets/f44777e7-a79b-47b7-b347-da5eaeea888d" />

This project enables control of a **Bosch Climate 3000i** air conditioning unit using ESPHome and the **Midea UART interface protocol**. The controller uses an ESP12S module to communicate directly with the AC unit over UART, providing local control without cloud dependencies.

## Hardware Requirements
The necessary files BOM / Gerber / Pick and Place file are available on hardware/ for download.

- **ESP12S** module
- **UART connection** to Bosch Climate 3000i control board
- **Power supply** (3.3V for ESP module)

## Software Prerequisites

### Install ESPHome
```
pip install esphome
```

### Run ESPHome Dashboard (Optional)

To run the ESPHome dashboard locally for a web-based interface:
```
esphome dashboard ./
```

This will start a web server at `http://localhost:6052` where you can manage your ESPHome configurations.

## Building and Uploading Firmware

### 1. Identify Your Serial Device

First, identify your USB-to-serial device:
```
ls /dev/tty.usb*
```

Common formats: `/dev/tty.usbserial-XXXXXXXX` or `/dev/tty.SLAB_USBtoUART`

### 2. Build and Upload

Use the following command sequence to compile and upload the firmware:
```
esphome config bosch-climate.yaml && esphome config bosch-climate.yaml && esphome upload bosch-climate.yaml --device /dev/tty.usbserial-XXXX
```
**Note**: Replace `/dev/tty.usbserial-XXXX` with your actual device path from step 1.

### Web Interface

Access the built-in web server at: `http://bosch-climate-controller.local` or the device's IP address.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

  Written by [@snackk](https://github.com/snackk)
