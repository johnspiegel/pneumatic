### TODO:

* Next:
    * refactor to do AQI classes as a list of structs.
    * setup ttgo t-display
    * Use up NTP
    * keep a circular buffer of timestamped results, show SMA
    * Two colored LED lights - one for Co2, one for AQI
    * clean up wifi code
    * Pin tasks to cores, and use real task priorities
    * Check for errors from task creates, other calls
    * ESP logging
    * Thread safety - use proper mutexes to guard shared data.
    * Add tests
* other integrations:
    * push to influxdb
    * Chillibits ?
    * weather underground?
    * doiot.ru ?
* Bonus fun:
    * OTA updates?
    * Auto-Refresh status page - query option (ESP async webserver)
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
    * Render time to web page, Wifi info, RSSI, channel, etc., number of wifi disconnects
    * Periodic background scan for better wifi channel
* Gifts:
    * Trigger CO2 Calibration?
    * Reset button?
* TTGO
    * Write display routine
        * WiFi SSID, signal, IP address
        * AQI #, color
        * CO2 #, color
        * Temp / Humidity / Pressure
