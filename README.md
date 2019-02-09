# ESP32-http-websocket
*ESP32 + HTTP server + websockets + Bootstrap + Husarnet. A simple project template showing how to use those technologies to create a fast, pretty and secure web UI hosted on ESP32. Works in LAN and over the internet. Written using Arduino framework.*

Open Arduino IDE and follow these steps:

**1. Install Husarnet IDF for ESP32:**
* open ```File -> Preferences```
* in a field **Additional Board Manager URLs** add this link: `https://files.husarion.com/arduino/package_esp32_index.json`
* open ```Tools -> Board: ... -> Boards Manager ...```
* Search for `esp32-husarnet by Husarion`
* Click Install button

**2. Select ESP32 dev board:**
* open ```Tools -> Board```
* select ***ESP32 Dev Module*** under "ESP32 Arduino (Husarnet)" section

**3. Install ArduinoJson library:**
* open ```Tools -> Manage Libraries...```
* search for `ArduinoJson`
* select version `5.13.3`
* click install button

**4. Program ESP32 board:**
* Open **ESP32-http-websocket.ino** project
* modify line 32 with your Husarnet `join code` (get on https://app.husarnet.com)
* modify lines 11 - 20 to add your Wi-Fi network credentials
* upload project to your ESP32 board.

**4. Open WebUI:**
There are two options:
1. log into your account at https://app.husarnet.com, find `esp32websocket` device that you just connected and click `web UI` button. You can also click `esp32websocket` to open "Element settings" and select `Make the Web UI public` if you want to have a publically available address. In such a scenerio Husarnet proxy servers are used to provide you a web UI.
2. P2P option - add your laptop to the same Husarnet network as ESP32 board. In that scenerio no proxy servers are used and you connect to ESP32 with a very low latency directly with no port forwarding on your router! Currenly only Linux client is available, so open your Linux terminal and type:

1. ```$ curl https://install.husarnet.com/install.sh | sudo bash``` to install Husarnet.
2. ```$ husarnet join XXXXXXXXXXXXXXXXXXXXXXX mylaptop``` replace XXX...X with your own `join code` (see point 4), and click enter.

At this stage your ESP32 and your laptop are in the same VLAN network. The best hostname support is in Mozilla Firefox web browser (other browser also work, but you have to use IPv6 address of your device that you will find at https://app.husarnet.com) and type:
```http://esp32websocket:8000```
You should see a web UI to controll your ESP32 now.


