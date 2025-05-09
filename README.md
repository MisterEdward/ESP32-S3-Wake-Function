# ESP Wakeup Keypress

A firmware for ESP32 devices to act as a USB keyboard and wake up the computer
when a HTTP request is received.

The story behind this is that my computer won't power on for Wake on LAN magic
packets for some unkown Linux/UEFI reason (all needed settings are enabled for
the network adapter, it's not a configuration issue). ESP Wakeup Keypress is
the solution I came up with - I can make a GET request to the ESP device and
it wakes up the computer for me.

## Features

- Modern, responsive web UI with dark mode support
- USB keyboard wake-up functionality
- Wake-on-LAN (WoL) functionality as a fallback option
- Visual status indicators for connection state
- Animated background with subtle gradients
- Dark mode toggle

## Usage

- Run `./00-init.sh`, this will download and install ESP-IDF and all its
  dependencies
- Run `./00-set-target.sh esp32s3` (replace `esp32s3` with your target)
- Edit the following settings in `sdkconfig`:

  - CONFIG_ESP_WAKEUP_KEYPRESS_WIFI_SSID
  - CONFIG_ESP_WAKEUP_KEYPRESS_WIFI_PASSWORD
  - CONFIG_ESP_WAKEUP_KEYPRESS_LED_STRIP_GPIO_NUM
  - CONFIG_TINYUSB_HID_KEYBOARD_ENABLED=y (Ensure this is set for HID System Control features like PC Restart/Shutdown)

I'm using a ESP32-S3-DevKitC which has the LED strip at GPIO48, but the latest
release of the devkit (ESP32-S3-DevKitC-1) has it on GPIO38.

- Run `./01-build.sh`
- Create the file `defport-flash` and put `/dev/ttyACM0` into it (or the port
  where your ESP can be programmed)
- Put your ESP into bootloader mode by pressing and holding the GPIO0 button
  while pressing the reset button
- Run `./02-flash.sh`
- Create the file `defport-monitor` and put `/dev/ttyUSB0` into it (or the port
  where your ESP serial console is)
- Run `./03-monitor.sh`
- Press the reset button on your board to exit bootloader mode

Once your device is running, simply navigate to its IP address in a web browser to access 
the modern web UI. From there you can:

- Use the "Wake Computer (USB)" button to send a keypress wake signal
- Use the "Wake Computer (WoL)" button to send a Wake-on-LAN magic packet
- Toggle between light and dark mode using the moon/sun icon

For command-line usage, you can also directly access the endpoints:
- `http://esp-wakeup-keypress.localdomain/wakeup` - USB wake-up
- `http://esp-wakeup-keypress.localdomain/wol?mac=70-85-C2-FA-D0-27` - WoL wake-up
- `http://esp-wakeup-keypress.localdomain/restart` - Restart PC
- `http://esp-wakeup-keypress.localdomain/shutdown` - Shutdown PC

## Troubleshooting

### Wake-up Issues

If your computer does not wake up for the virtual keypress, then create this
shell script at `/lib/systemd/system-sleep/00-esp-wakeup-enable.sh`:

```bash
#!/bin/bash

# Action script to enable wake after suspend by keyboard or mouse

if [ "$1" = post ]; then
    KB="$(lsusb -tvv | grep -A 1 303a:4004 | awk 'NR==2 {print $1}')"
    echo enabled > ${KB}/power/wakeup
fi

if [ "$1" = pre ]; then
    KB="$(lsusb -tvv | grep -A 1 303a:4004 | awk 'NR==2 {print $1}')"
    echo enabled > ${KB}/power/wakeup
fi
```

### Shutdown/Restart Issues

If your computer does not respond to shutdown or restart commands:

1. **BIOS/UEFI Settings**: Ensure USB power management is properly configured in your BIOS/UEFI settings.

2. **OS Permissions**: Some operating systems require additional permissions for USB devices to control power state:
   - For Windows: Make sure the device is recognized as a HID System Control device in Device Manager
   - For Linux: You may need to add udev rules or adjust power management settings

3. **Alternative Methods**: The firmware attempts multiple methods to control power:
   - First it tries HID System Control signals
   - Then it falls back to keyboard shortcuts (Windows: Win+R to run shutdown commands)
   - If neither works, you may need to adjust the keyboard layout in the code for your OS

4. **USB Recognition**: Make sure your computer recognizes the ESP32 as both a keyboard and system control device:
   ```bash
   # For Linux systems
   lsusb -v | grep -A 5 "System Control"
   ```
