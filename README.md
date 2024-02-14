
## ESPHome components for USB Keybord/ USB Barcode Scanner and a Paprika2App Component

This repository contains [external components](https://esphome.io/components/external_components.html) for [ESPHome](https://esphome.io/)

### USB Barcode Scanner or Keyboard Component
 This component enables keyboard or barcode scanner connected via USB OTG port of ESP32 S2/S3 family of micro-controllers.
 Additionally the component can resolve the pure barcode into a food product name.

### Paprika3App Component
 This component allows to add items to the Paprika3App grocery list.

## Memory, PSRAM
The Paprika3App component requires a lot of memory to gzip requests (Sadly needed by the weird API). Thus, the ESP32-S2/S3 device has to have PSRAM connected and enabled, e.g.:
```yaml
psram:
  mode: octal
  speed: 80MHz
```

## ESP-IDF framework mode
Arduino framework is not supported. And flash mode must be corrected for esp-idf ESP32 S2/S3.
```yaml
esphome:
  platformio_options:
    board_build.flash_mode: dio  # default mode is wrong

esp32:
  board: esp32-s3-devkitc-1
  framework:
    type: esp-idf
```

## Import external components
The following yaml can be used to import components into your ESPHome configuration:
```yaml
external_components:
  - source: github://danielBreitlauch/esphome_components
```

## Enable barcode scanner
```yaml
text_sensor:
  - platform: usb_barcode_scanner
    name: barcode
    type: food                # optional 'barcode' will show the barcode or 'food' (default) will show the product name
    openfoodfacts_region: de # optional specify the region for lookup queries to OpenFoodFacts. Default: world
```

## Enable Paprika3App
```yaml
paprika-app-list:
  email: "xxx"      # login email of the apps account
  password: "xxx"   # login password of the apps account
  listID: "xxx-xxx-xxx-xxxx-xxxxxxx-xxxx-xxxxxxxxxxxxx" # listId of the grocery list lmk if it needs to be simpler to find this
```
### Action to add something
```yaml
text_sensor:
  - ...
    on_value:
      then:
        - paprika-app-list.add
            format: "lala: %s"    # Optional: formats the incoming text
```

## Full example where the scanned product is put into the grocery list
```yaml
esphome:
  name: barcode
  platformio_options:
    board_build.flash_mode: dio

esp32:
  board: esp32-s3-devkitc-1
  framework:
    type: esp-idf

psram:
  mode: octal
  speed: 80MHz

external_components:
  - source: github://danielBreitlauch/esphome_components

paprika-app-list:
  email: "xxx"
  password: "xxx"
  listID: "xxx-xxx-xxx-xxxx-xxxxxxx-xxxx-xxxxxxxxxxxxx"

text_sensor:
  - platform: usb_barcode_scanner
    name: barcode
    on_value:
      then:
        - paprika-app-list.add
```
