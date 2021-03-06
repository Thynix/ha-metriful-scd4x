# Home Assistant Environment Monitoring

This reports temperature, air pressure, humidity, illumination, sound level,
and CO2 concentration to Home Assistant over WiFi.

This repo is a [PlatformIO](https://platformio.org/) project.

## Hardware

Requires:

* [Metriful MS430](https://www.metriful.com/ms430)

Known to work with these boards:

* [Adafruit Feather M0 WiFi](https://www.adafruit.com/product/3061) or [Adafruit Feather HUZZAH with ESP8266](https://www.adafruit.com/product/2821)

Optional:

* [SCD40 CO2 sensor](https://www.adafruit.com/product/5187) or [SCD30 CO2 sensor](https://www.adafruit.com/product/4867)
* [Adafruit FeatherWing OLED - 128x32 OLED Add-on For Feather](https://www.adafruit.com/product/2900)

## Configuration

Copy `include/settings.h_example` to `include/settings.h` and set its values as appropriate.

## License

MIT, except portions of this code concerning the CO2 sensor are licensed as
follows:

```
/*
 * Copyright (c) 2021, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 ```
