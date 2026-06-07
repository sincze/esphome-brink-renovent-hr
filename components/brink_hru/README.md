# brink_hru — Brink Renovent HR over OpenTherm (ESPHome)

ESPHome external component that controls and monitors a **Brink Renovent HR** heat
recovery ventilation unit over **OpenTherm**, exposing it to Home Assistant via the
native ESPHome API. No MQTT broker, no hand-rolled autodiscovery — just YAML, OTA, and
live logs.

It is a port of the Arduino sketch from
[tijsverkoyen/Home-Assistant-BrinkRenoventHR](https://github.com/tijsverkoyen/Home-Assistant-BrinkRenoventHR),
bundling the same raf1000-patched [ihormelnyk OpenTherm
library](https://github.com/ihormelnyk/opentherm_library). The fan command path was
cross-checked against the working [Sidiox/hrv-control](https://github.com/Sidiox/hrv-control)
firmware (the [portegi.es](https://portegi.es/blog/opentherm-wtw-2) blog).

## Why not the stock ESPHome `opentherm` component?

The stock component only knows a fixed set of predefined OpenTherm messages. The Brink
exposes most of its data through **Transparent Slave Parameters (TSP)**: a single
data-id `89` (`VentTSPEntry`) where the high byte selects the parameter index. The stock
component can't do that indexing, so this component bundles the patched library instead.

## Hardware

| Part | Notes |
|------|-------|
| Wemos D1 Mini (ESP8266) | `board: d1_mini` |
| [DIYLess Master OpenTherm Shield](https://diyless.com/product/master-opentherm-shield) | acts as OpenTherm **master** |
| Brink Renovent HR | connect the shield to the unit's OpenTherm connector (no polarity) |

Default pins match the original sketch: **`in_pin: 4`** (GPIO4 / D2), **`out_pin: 5`**
(GPIO5 / D1).

> The filter-dirty and fault indicators are read **over OpenTherm** (`VentStatus` bits),
> so you do **not** need to tap the unit's RJ12 switch/indicator connector.

## Configuration

```yaml
external_components:
  - source:
      type: local
      path: components

brink_hru:
  id: brink
  in_pin: 4
  out_pin: 5
  max_volume: 296          # U-max of your unit in m3/h (turns 0-100% into m3/h)
  update_interval: 2s      # one OpenTherm read per tick -> full refresh ~34s
  ventilation_tolerance: 4 # +/- % hysteresis for yield-to-switch (see below)

sensor:
  - platform: brink_hru
    brink_hru_id: brink
    supply_temperature:    { name: "Temperature from atmosphere" }
    exhaust_temperature:   { name: "Temperature from indoors" }
    input_volume:          { name: "Current input volume" }
    output_volume:         { name: "Current output volume" }
    fan_level:             { name: "Fan level" }
    pressure_input:        { name: "Pressure input duct" }
    pressure_output:       { name: "Pressure output duct" }
    bypass_status:         { name: "Bypass status" }
    frost_status:          { name: "Frost protection status" }
    fault_code:            { name: "Fault code" }
    imbalance:             { name: "Fixed imbalance" }

binary_sensor:
  - platform: brink_hru
    brink_hru_id: brink
    fault:            { name: "Fault indication" }
    filter_dirty:     { name: "Filter dirty" }
    ventilation_mode: { name: "Ventilation mode" }

number:
  - platform: brink_hru
    brink_hru_id: brink
    ventilation:                     { name: "Ventilation level" }       # 0-100 %
    minimum_atmospheric_temperature: { name: "Min atmospheric temp bypass" }  # U4, 5-20 °C
    minimum_indoor_temperature:      { name: "Min indoor temp bypass" }       # U5, 18-30 °C
```

### Hub options

| Option | Default | Meaning |
|--------|---------|---------|
| `in_pin` / `out_pin` | — | OpenTherm shield GPIOs (required) |
| `max_volume` | `296` | unit's max airflow (m³/h); scales the 0-100 % volume reads |
| `update_interval` | `60s` | time per tick; one OpenTherm exchange per tick (round-robin) |
| `ventilation_tolerance` | `4` | ± % band for the yield-to-switch logic |

## Entities

- **Sensors:** supply/exhaust temperature, input/output volume (m³/h), fan level, input/
  output duct pressure (Pa), bypass status (0 shut / 1 auto / 2 input-at-min), frost
  protection status, fault code, fixed imbalance.
- **Binary sensors:** fault indication, filter dirty, ventilation running.
- **Numbers (writable):** ventilation level (0-100 %), minimum atmospheric temperature
  bypass (U4), minimum indoor temperature bypass (U5).

## Ventilation control & the 3-way wall switch (yield-to-switch)

The OpenTherm fan command (`VentNomVentSet`, data-id 71) and the physical 3-way switch
drive the **same** ventilation demand — they don't stack, the last actor wins. This
component runs a **yield-to-switch** policy:

1. Setting the **Ventilation level** number in HA writes the value and remembers it.
2. Every poll it reads the actual level back:
   - within `ventilation_tolerance` of the commanded value → holds;
   - changed by more than that **and** different from the previous reading → treated as a
     person at the wall switch: the component **yields**, mirrors the new level into HA,
     and stops re-asserting;
   - drifted without an external change → **re-asserts** the commanded level.
3. HA reclaims control the moment you set the Ventilation level again.

Net effect: HA is primary, the wall switch still works for the household, and HA always
shows the true level. Tune `ventilation_tolerance` if your unit reports noisy levels.

> U4/U5 are persistent settings stored in the unit, so they are written once (no
> re-assert).

## Use from another machine (ESPHome Builder)

The component lives on GitHub, so a builder/dashboard on another machine pulls it
straight from there — no local copy needed.

**Option A — package one-liner (recommended).** The repo ships `brink-renovent.yaml`,
which wires the github `external_components` + hub + all entities. Your device config
only adds name/wifi/api/ota:

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

Override knobs by setting substitutions before the package ref, e.g.
`substitutions: { brink_max_volume: "325", brink_ventilation_tolerance: "6" }`.
See `brink-remote.example.yaml` in the repo.

**Option B — external_components only.** Skip the package and pull just the component,
then write the hub + entities yourself:

```yaml
external_components:
  - source: github://sincze/esphome-brink-renovent-hr@main
    components: [brink_hru]

brink_hru:
  id: brink
  in_pin: 4
  out_pin: 5
```

Public repo → no token needed. (Private repo → the builder machine needs a GitHub
PAT/SSH configured.)

## API encryption

Every device config above enables the encrypted ESPHome API:

```yaml
api:
  encryption:
    key: !secret api_encryption_key   # generate: openssl rand -base64 32
```

Put the key in `secrets.yaml` (gitignored; see `secrets.yaml.example`) and paste the
**same** key into the Home Assistant ESPHome integration when it adopts the device.

## Flashing & debugging

```bash
esphome run brink.yaml          # compile + upload (USB first time, OTA after)
esphome logs brink.yaml         # live logs over WiFi/serial
```

Set `logger: { level: DEBUG }` to watch each OpenTherm read/write and the yield-to-switch
decisions live — no broker, no Arduino re-flash cycle.

## Verify on first run

1. Temperatures and fan level should populate within a couple of poll cycles.
2. Compare TSP-derived values (volumes, pressures, bypass) against the unit's front panel.
3. Set the Ventilation level in HA and confirm the unit responds.
4. Nudge the physical wall switch and confirm HA mirrors the new level instead of
   fighting it. (This also confirms a wall-switch change is visible on the OpenTherm
   `getVentilation` readback for your unit.)

## Credits

- [tijsverkoyen/Home-Assistant-BrinkRenoventHR](https://github.com/tijsverkoyen/Home-Assistant-BrinkRenoventHR) — original sketch and Brink TSP mapping.
- [ihormelnyk/opentherm_library](https://github.com/ihormelnyk/opentherm_library) + raf1000's Brink patches — the bundled OpenTherm library.
- [Sidiox/hrv-control](https://github.com/Sidiox/hrv-control) / [portegi.es](https://portegi.es/blog/opentherm-wtw-2) — reference fan-control firmware.
