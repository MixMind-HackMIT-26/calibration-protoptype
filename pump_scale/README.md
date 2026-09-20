# Pump Scale

Arduino UNO on COM7. Serial monitor: 9600 baud, newline enabled.

## Wiring

- HX711 DT: D5
- HX711 SCK: D6
- SSD1306 SDA: D11
- SSD1306 SCL: D10
- Modules and UNO must share GND.

Display configuration: SSD1306 128x64, I2C address 0x3C, software I2C.

## First Calibration

1. Put the empty container on the sensor, stop the pumps, and let it settle.
2. Send `t` followed by Enter. Keep the container steady until `OK: zero set.`
3. Add a known mass, for example 100 g, without removing the container.
4. Send `c 100` followed by Enter. Substitute the actual added mass in grams.
5. Wait for `OK: saved counts/g = ...`. Calibration is saved in EEPROM.
6. Empty the container, replace it, and send `t` again before pumping.

At the assumed water density of 1 g/mL, grams and water milliliters have
the same numerical value. Use an independently known mass or measured
water volume for calibration, not the uncalibrated pump's nominal output.

## Daily Use

Send `t` with the empty container in place before a run. The OLED shows net
grams and equivalent mL of water with light smoothing. The displayed tenth
of a gram is a display increment, not a guarantee of measurement accuracy.
Capture uses 20 samples; wait for the serial confirmation before changing
the load. A new tare is required after reset or power-up. Opening a serial
monitor can reset the UNO; keep it open throughout a pump run.

Before calibration, the display reports raw counts only. Sensor loss is
reported as SENSOR OFFLINE. The firmware measures output; it does not
control pump power.
