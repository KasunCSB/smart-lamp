# Smart Lamp Controller

WiFi-enabled smart lamp controller

**Arduino Code:**
- `mcu_smartlmp.ino` - **Main code (recommended)**
- `mcu_smartlmp_optimized.ino` - Performance optimized version  

**Control Scripts:**
- `lamp_control.bat` - Windows batch script
- `lamp_control.sh` - Linux/Mac shell script  
- `lamp_control.py` - Python script (cross-platform)

## 🔄 Code Versions

**mcu_smartlmp.ino (Recommended)**
- Easy to understand
- Standard Arduino functions  
- Perfect for beginners
- Full-featured web interface

**mcu_smartlmp_optimized.ino (Advanced)**
- Faster performance
- Memory optimized
- For advanced users

## 💻 Control Scripts

After setup, control your lamp easily with these scripts:

### Windows (Command Prompt)
```cmd
lamp_control.bat 192.168.1.100 on          # Turn on
lamp_control.bat 192.168.1.100 off         # Turn off
lamp_control.bat 192.168.1.100 timer 30    # 30 min timer
lamp_control.bat                            # Interactive menu
```

### Linux/Mac (Terminal)
```bash
chmod +x lamp_control.sh                   # Make executable
./lamp_control.sh 192.168.1.100 on         # Turn on
./lamp_control.sh 192.168.1.100 timer 60   # 1 hour timer
./lamp_control.sh                           # Interactive menu
```

### Python (Cross-platform)
```bash
pip install requests                        # Install dependency
python lamp_control.py 192.168.1.100 on    # Turn on
python lamp_control.py 192.168.1.100 status # Check status
python lamp_control.py                      # Interactive menu
```

## 🔧 TroubleshootingESP8266 with web interface and timer functionality.

## 🌟 Features

- WiFi control via web browser
- Timer function (1 minute to 12 hours) 
- Mobile-friendly interface
- Quick timer presets (1m, 5m, 30m, 1hr)
- Real-time countdown display

## 🛠️ What You Need

- ESP8266 board (NodeMCU recommended) - $2-3
- 5V relay module - $2 
- Jumper wires - less than $1
- Any AC lamp

## 🔌 Wiring

```
ESP8266 NodeMCU    →    5V Relay Module
────────────────────    ─────────────────
VIN (5V)           →    VCC
GND                →    GND  
D1 (GPIO5)         →    IN

Relay Module       →    Lamp
─────────────      →    ────
COM                →    Live wire (from wall)
NO                 →    Live wire (to lamp)
```

**⚠️ AC VOLTAGE IS DANGEROUS - Get professional help if unsure!**

## 🚀 Setup

### 1. Install Arduino IDE
- Download from [arduino.cc](https://www.arduino.cc/en/software)
- Add ESP8266 board: File → Preferences → Additional Board Manager URLs:
  ```
  http://arduino.esp8266.com/stable/package_esp8266com_index.json
  ```
- Tools → Board → Boards Manager → Install "esp8266 by ESP8266 Community"
- Select: Tools → Board → NodeMCU 1.0 (ESP-12E Module)

### 2. Configure WiFi
Open `mcu_smartlmp.ino` and change:
```cpp
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
```

### 3. Upload Code  
- Connect ESP8266 via USB
- Select correct Port in Tools → Port
- Click Upload (→ button)

### 4. Find IP Address
- Open Tools → Serial Monitor (9600 baud)
- Press Reset on ESP8266
- Note the IP address shown

### 5. Access Interface
Open web browser and go to the IP address from step 4.

## � Troubleshooting

**WiFi won't connect:**
- Check credentials are correct
- Ensure 2.4GHz network (ESP8266 doesn't support 5GHz)
- Move closer to router

**Can't upload code:**
- Check USB cable and drivers
- Try different COM port
- Hold FLASH button during upload

**Relay doesn't work:**
- Verify wiring connections
- Check relay power LED
- Test with multimeter

**Can't access web page:**
- Check IP address in Serial Monitor
- Ensure device on same WiFi network
- Try different browser

## � Files

- `mcu_smartlmp.ino` - **Main code (recommended)**
- `mcu_smartlmp_optimized.ino` - Performance optimized version

## 🔄 Code Versions

**mcu_smartlmp.ino (Recommended)**
- Easy to understand
- Standard Arduino functions  
- Perfect for beginners
- All features included

**mcu_smartlmp_optimized.ino**
- Faster performance
- Memory optimized
- Advanced users only

## 📄 License

MIT License - Free to use and modify



