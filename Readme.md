### TODO:

* Next:
  * Thread safety - use proper mutexes to guard shared data.
  * Pin tasks to cores, and use real task priorities
  * Check for errors from task creates, other calls
  * ESP logging
  * Add tests
  * React to flaky network:
    * Periodic background scan for better wifi channel
    * Ping local gateway, and google.com
    * If ping goes bad, try switching to a different channel or network
    * Keep a list of BSSIDs, some sort of mapping from RSSI to 95% ping latencies and 
* other integrations:
  * Chillibits ?
  * weather underground?
  * doiot.ru ?
* Bonus fun:
  * lvgl screen fun
    * No flicker
    * include wifi network, rssi, IP
    * display wifi ssid, password, IP in wifi setup captive portal
  * outliers:
    * don't produce values for the first 2m
    * but produce "raw" outputs that I can scrape
    * exponential moving average
      * also an exponential moving average of the error
      * try not producing values when the error moving average is too high
      * Error:
        * from absolute zero
        * then as the long from absolute zero
    * and then I can try circular buffers and SMA
  * keep a circular buffer of timestamped results, show SMA (10s?)
  * Two colored LED lights - one for Co2, one for AQI
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
  * Watchdog
    * reboot if no HTTP requests for 5m
    * reboot if can't ping some service?
    * reboot if can't connect to self webserver?
    * Write out to flash the number of resets
    * Figure out how to use the RTC watchdog
  * push to influxdb
  * clean up wifi code
* Gifts:
  * Trigger CO2 Calibration?
  * Reset button?
* TTGO
  * Write display routine
    * WiFi SSID, signal, IP address
    * AQI #, color
    * CO2 #, color
    * Temp / Humidity / Pressure
