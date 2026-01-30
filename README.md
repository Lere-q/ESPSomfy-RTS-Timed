# ESPSomfy-RTS-Timed
A controller for Somfy RTS shades and blinds using an ESP32, with integrated time-scheduling.

Install the stock ESPSomfy software. ( https://github.com/rstrouse/ESPSomfy-RTS/releases/download/v2.4.6/SomfyController.onboard.esp32.bin.zip )

Install SomfyController.ino.esp32.bin on your board using ESPHome at https://web.esphome.io/

Configure the Wi-Fi and set your time zone.

Go to Settings > System > Firmware > Update Application and upload the file SomfyController.littlefs.bin.

Go to Settings > System > Firmware > Update Firmware and upload the custom SomfyController.ino.esp32.bin


To add a time schedule, click on the “Clock” icon next to the Settings icon. Set your preferred options and then click “Zeitplan speichern”. After that, wait about 5–10 seconds and refresh the page.

(Currently, you need to refresh the page because of a bug that keeps the loading animation visible even when the process is finished.)
