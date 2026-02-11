# ESPSomfy-RTS-Timed

This is based on https://github.com/rstrouse/ESPSomfy-RTS

# This will be replaced with a new project called Skyeo!

### I still appreciate any tips, feedback, and support from the community.

### More details will follow soon on how to transition to Skyeo.

### Current users are encouraged to try Skyeo once it’s released.

### This project will remain here for reference until Skyeo is fully ready.

### Future development will continue under Skyeo.





A controller for Somfy RTS shades and blinds using an ESP32, with integrated time-scheduling.

⚠️ Important: I recommend backing up everything or setting this up before adding any blinds. I am not responsible for any data loss or malfunctions.

Note: This is currently only tested on ESP32 and available in German only. It was made for version v2.4.6, but maby work also for newer versions.



1. Install the default ESPSomfy software using ESPHome:
   
Warning: This is a direct download:

Download the stock software from:
https://github.com/rstrouse/ESPSomfy-RTS/releases/download/v2.4.6/SomfyController.onboard.esp32.bin.zip
   
Or the current version for your board:
https://github.com/rstrouse/ESPSomfy-RTS/releases
   
3. Install the default ESPSomfy software on your ESP32 board using ESPHome:
   https://web.esphome.io/

4. Configure your Wi-Fi and set your time zone.


Add the "addon":

1. Go to Settings > System > Firmware > Update Application and upload the file: SomfyController.littlefs.bin.

2. Go to Settings > System > Firmware > Update Firmware and upload the custom SomfyController.ino.esp32.bin.

Add a time schedule:
1. Click on the Clock icon next to the Settings icon, set your preferred options, then click „Zeitplan speichern“. Wait about 5–10 seconds and refresh the page.

Note: Currently, you need to refresh the page due to a bug that keeps the loading animation visible even after the process is finished.

I appreciate bug reports, feature requests, and general feedback.
You are also welcome to ask for help or suggest improvements to this guide.

<img width="1536" height="1024" alt="dc96d906-3e8a-46d7-89f8-8ca52686c180" src="https://github.com/user-attachments/assets/ee9584cf-a1f9-4417-a63f-6dfa39c4545e" />
