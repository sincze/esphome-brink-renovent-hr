# Brink Renovent HR → Home Assistant (ESPHome, OpenTherm)

Control and monitor a **Brink Renovent HR** heat-recovery ventilation unit from Home
Assistant over **OpenTherm**, as a native **ESPHome** device. No MQTT broker, no
hand-rolled autodiscovery — just YAML, OTA updates, an encrypted API, and live logs in
your browser.

And — importantly — **Home Assistant and the physical 3-way wall buttons keep working at
the same time.**

## What it does

A Wemos D1 Mini with a Master OpenTherm Shield talks to the Brink unit and exposes the
full data set to Home Assistant:

- supply / exhaust temperatures, input / output air volume, duct pressures,
- fan level, bypass status, frost-protection status, fixed imbalance,
- fault indication + fault code, **filter-dirty** indicator,
- writable controls: ventilation level (0–100 %), bypass temperature thresholds (U4/U5).

The rich Brink-specific values ride on OpenTherm *Transparent Slave Parameters* (TSP),
which the stock ESPHome `opentherm` component can't read — so this project bundles a
Brink-patched OpenTherm library in a small custom component.

## Two control paths, side by side

| Path | How |
|------|-----|
| **Home Assistant** | Encrypted ESPHome API — set the Ventilation level / U4 / U5, read everything. |
| **Physical 3-way wall buttons** | The unit's own switch keeps working as normal. |

They both drive the *same* ventilation demand, so the component runs a **yield-to-switch**
policy:

- Home Assistant is **primary and sticky** — the level you set is held and re-asserted
  if the unit drifts.
- If someone **flips the wall switch**, that change **wins**: the component stops
  fighting it and mirrors the new level back into Home Assistant. The switch never
  appears broken.
- Home Assistant **reclaims control** the moment you set the level again in HA.

So: you run everything from HA, but the family can still use the wall switch and it just
works — and HA always shows the true level. The **filter-dirty and fault indicators are
read over OpenTherm too**, so the wall panel's lights are mirrored in HA without any
extra wiring.

## Hardware

- **Wemos D1 Mini** (ESP8266)
- **[DIYLess Master OpenTherm Shield](https://diyless.com/product/master-opentherm-shield)**
- Connected to the Brink Renovent HR's OpenTherm connector (no polarity)
- Pins: `in_pin` = GPIO4 (D2), `out_pin` = GPIO5 (D1)

## Quick start

On your ESPHome Builder / dashboard, create a device config:

```yaml
substitutions:
  name: brink-renovent
esphome:
  name: ${name}

packages:
  brink: github://sincze/esphome-brink-renovent-hr/brink-renovent.yaml@main

logger:
api:
  encryption:
    key: !secret api_encryption_key
ota:
  - platform: esphome
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  ap: {}
captive_portal:
```

Add to `secrets.yaml` (see [`secrets.yaml.example`](secrets.yaml.example)):

```yaml
wifi_ssid: "your-ssid"
wifi_password: "your-password"
api_encryption_key: "..."   # generate: openssl rand -base64 32
```

Flash once over USB from the ESPHome Builder (after that it's OTA), then open the
device's **LOGS** screen in the browser to watch each OpenTherm read/write and the
yield-to-switch decisions live. Use the **same** `api_encryption_key` when Home
Assistant adopts the device.

Full configuration reference, all options and entities, and the
`external_components`-only alternative are in
**[`components/brink_hru/README.md`](components/brink_hru/README.md)**.

## Credits

- [tijsverkoyen/Home-Assistant-BrinkRenoventHR](https://github.com/tijsverkoyen/Home-Assistant-BrinkRenoventHR) — original Arduino + MQTT sketch and the Brink TSP mapping.
- [ihormelnyk/opentherm_library](https://github.com/ihormelnyk/opentherm_library) + raf1000's Brink patches — the bundled OpenTherm library.
- [Sidiox/hrv-control](https://github.com/Sidiox/hrv-control) / [portegi.es](https://portegi.es/blog/opentherm-wtw-2) — reference fan-control firmware.

## License

MIT. The bundled OpenTherm library retains its original MIT license (© Ihor Melnyk).
