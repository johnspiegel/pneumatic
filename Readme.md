TODO:
    * Hygiene:
        * Render time to web page, Wifi info, RSSI, channel, etc., number of wifi disconnects
        * Periodic background scan for better wifi channel
        * Two colored LED lights - one for Co2, one for AQI
        * clean up wifi code
        * Reorganize Code - move bme stuff to separate files, set up task poll functions
        * Add tests
        * Pin tasks to cores, and use real task priorities
        * Check for errrors from task creates
        * refactor to do AQI classes as a list of structs.
        * Thread safety - use proper mutexes to guard shared data.

    * other integrations:
        * local influxdb
        * Chillibits ?
        * opensensemap ?
        * weather underground?
        * doiot.ru ?

    * Bonus fun:
        * Auto-Refresh status page - query option (ESP async webserver)
        * keep a circular buffer of the last minute, show SMA
        * Keep timestamps for sensor readings
        * quantize logs, show hourly graph (minutely?)
        * Keep logs on flash?
        * try polling BME less often, less self-heating?
        * advanced log dump web page?
        * ambient light sensor, adjust led brightness?
        * Captive portal - use WIFI_MODE_APSTA?
    * Reliable
        * watchdog
        * serial to webpage

    * Gifts:
        * Captive Portal
        * Save Wifi SSID, password
        * Trigger CO2 Calibration?
        * Reset button?
        * OTA updates?
    * TTGO
        * Write display routine
            * WiFi SSID, signal, IP address
            * AQI #, color
            * CO2 #, color
            * Temp / Humidity / Pressure
    
  
protobuf on embedded
tinygo - what's building now?
rpi - get 8gb + big sdcard or a drive
tasmota - install it on my 8266's
