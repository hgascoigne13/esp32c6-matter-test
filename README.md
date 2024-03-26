# ESP32-C6 Sensor Test with Matter and Thread
## Introduction
This project demonstrates a proof of concept for an IoT sensor setup to measure temperature and illuminance with the Matter Standard and Thread protocol.

## Required Hardware
- Espressif ESP32-C6 SoC
- BH1750 Illuminance Sensor (connected via I2C protocol)
- SCD30 CO2, Humidity, and Temperature Sensor (connected via I2C protocol)
- Matter Hub (Apple TV 4K 3rd Generation with Wi-Fi + Ethernet)
- iPhone with Apple Home app installed

## Purpose
This code presents an example of a Thread-enabled IoT sensor setup with the Matter Standard. This project can be used as a starting point for creating similar IoT sensor applications.

## Setup Procedure
It should be noted that this project was created and tested on an Intel Mac, but likely could be modified to work with Apple Silicon Macs, Linux machines and Windows. 

### Step 1: Clone Repository

```
git clone --recursive https://github.com/hgascoigne13/esp32c6-matter-test.git
```

### Step 2: Install and Configure ESP-IDF and VSCode Extension
- Install prerequisites
```
# Install prerequisites
brew install cmake ninja dfu-util
xcode-select --install

# Clone version 5.1.3 of the ESP-IDF repo
mkdir -p ~/esp
cd ~/esp
git clone -b v5.1.3 --recursive https://github.com/espressif/esp-idf.git

# Install esp32c6
cd ~/esp/esp-idf
./install.sh esp32, esp32c6
. ./export.sh
```
- Install Visual Studio Code if needed
- Install and configure the VSCode extension "Espressif IDF" by following [these](https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/tutorial/install.md) instructions

### Step 3: Setting Up Matter
Follow the instructions available [here](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html) to clone the esp-matter SDK into the esp folder directory. Make sure to clone v1.2 of esp-matter. 

It should be noted that the following commands need to be run from the esp directory (where esp-idf and esp-matter folders are) each time a new terminal is opened:
```
cd esp-idf; source ./export.sh; cd ..
cd esp-matter; source ./export.sh; cd ..
```

### Step 4: Setting Up the Environment
- Change directory to esp32c6-matter-test
```
cd [PATH]/esp32-matter-sensor-test
```
- Set ESP_MATTER_PATH
```
export ESP_MATTER_PATH="[# PATH to esp matter (e.g. /Users/esp-user/esp-matter)]"
```
- Set target
```
idf.py set-target esp32c6
```

### Step 5: Configure the Hardware
Connect the BH1750 and SCD30 sensors to the ESP32-C6 using I2C protocol.

### Step 6: Running the Program
To run the program with Thread enabled, first ensure the following configurations are set in the sdkconfig file:
```
# Enable OpenThread
CONFIG_OPENTHREAD_ENABLED=y
# Disable Wi-Fi Station
CONFIG_ENABLE_WIFI_STATION=n
# Disable minimal mDNS
CONFIG_USE_MINIMAL_MDNS=n
```
- Build the program
```
idf.py build
```
- Flash the program to the ESP32-C6
```
idf.py -p [PORT_NAME] flash
```
- Monitor the program in the terminal
```
idf.py monitor
```

### Step 7: Add the Matter Node to Apple Home App
* Set up the Apple TV as a Thread Border Router
  - Follow the instructions [here](https://support.apple.com/en-us/102557) to set up the Apple TV as a Home Hub
  - If it is not recognized as a Thread Border Router in the Apple Home App at any point, try connecting via Wi-Fi instead of Ethernet
* In the Apple Home App, click Add Accessory and scan [this](https://project-chip.github.io/connectedhomeip/qrcode.html?data=MT%3AY.K9042C00KA0648G00) QR code with the camera
* Follow the prompts on the app to finish the setup of the sensors

## Limitations
This code has been tested with a very specific set of components and is not designed to be portable. 

## Opportunities for Future Work
This project contains a basic implementation for an IoT-connected temperature and illuminance sensor setup. There is potential for future updates and improvements, such as:
- adding more sensors/endpoints
- applying this setup to practical applications
- investigating JTAG debugging possibilities
- adding support for remote management and control
  
