# ESP32 ECU Simulator & GPIO Controller

This project turns an ESP32 into a versatile tool for automotive diagnostics and electronics projects. It provides a web interface and physical controls to:
- Generate crank-angle sensor (CKP) signals for various engine patterns (e.g., 60-2, 36-1).
- Control GPIO pins.
- Simulate ignition signals.
- Control a relay with simple ON/OFF or PWM signals.
- Display real-time status on a small OLED screen.

## Hardware Setup

### Required Components
- ESP32 Development Board
- 0.96" I2C OLED Display (128x32)
- 3x Push Buttons
- Jumper Wires

### Pinout
| Pin | Function                 | Component        |
|-----|--------------------------|------------------|
| 2   | General Purpose Output   | "LED" GPIO       |
| 12  | General Purpose Output   | "P12" GPIO       |
| 13  | Input (Pull-up)          | Button "Up"      |
| 14  | Input (Pull-up)          | Button "Down"    |
| 15  | Input (Pull-up)          | Button "Select"  |
| 17  | Crank Signal Output      | CKP Signal       |
| 18  | General Purpose Output   | "P18" GPIO       |
| 19  | General Purpose Output   | "P19" GPIO       |
| 21  | I2C Data (SDA)           | OLED Display     |
| 22  | I2C Clock (SCL)          | OLED Display     |
| 23  | Ignition Signal Output   | Ignition Signal  |
| 25  | Relay Control Output     | Relay            |

**Note:** Ensure your push buttons are wired to ground, as the inputs are configured with internal pull-up resistors.

## Usage

### Wi-Fi Setup (First Time)
1.  On first boot, or if it can't connect to a known network, the ESP32 will create its own Wi-Fi network called **"ECU-Simulator-AP"**.
2.  Connect to this network with your phone or computer.
3.  A captive portal page should open automatically. If not, open a web browser and navigate to `192.168.4.1`.
4.  On the portal page, click "Configure WiFi", select your home network from the list, enter your password, and click "Save".
5.  The ESP32 will save the credentials and restart, connecting automatically to your network from now on.

### Web Interface
Once connected to your network:
1.  The ESP32's IP address will be printed to the Serial Monitor on startup and will also be shown on the OLED display.
2.  Open a web browser and navigate to the IP address of the ESP32.

The web interface allows you to:
- **Toggle GPIO Pins:** Turn the four general-purpose output pins ON or OFF.
- **Control Crank Signal:** Set the RPM and select the tooth pattern for the crank signal generator.
- **Control Ignition:** Adjust the dwell time and ignition advance angle.
- **Control Relay:** Turn the relay ON, OFF, or drive it with a PWM signal of a specific duty cycle.
- **Navigate Display:** Cycle through the screens on the OLED display.

### Physical Interface
The device can be controlled using the three push buttons:
- **Up / Down:** (Currently not assigned to a function, can be implemented in the future).
- **Select:** Cycles through the different screens on the OLED display (Main Status, GPIO Status, Log).

### Signal Graphing
The web interface includes a real-time graph that displays the signal from an external source.
- **Hardware:** Connect your external signal source to **Pin 5**.
- **Web UI:** The graph will automatically display the signal data when you load the web page.
