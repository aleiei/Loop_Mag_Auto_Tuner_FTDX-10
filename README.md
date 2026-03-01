# Loop Mag Auto Tuner (FTDX-10)

Firmware per **Magnetic Loop Auto Tuner** basato su Arduino Mega 2560 e radio Yaesu FTDX-10.

## Stato del progetto

Progetto in sviluppo attivo.

## Struttura attuale

- `src/main.cpp`: sorgente unico del firmware (logica CAT, display, SD, setup/loop)

## Build

Compilazione con PlatformIO:

```bash
pio run
```

Upload su scheda:

```bash
pio run -t upload
```

## Schema rapido collegamenti

### Yaesu FTDX-10 (CAT)

- **FTDX-10 CAT TX** → **Mega RX1 (pin 19)**
- **FTDX-10 CAT RX** → **Mega TX1 (pin 18)**
- **FTDX-10 GND** ↔ **Mega GND**

### Display ILI9341 (SPI)

- **CS** → pin **48**
- **RST** → pin **47**
- **DC** → pin **46**
- **MOSI** → pin **51**
- **MISO** → pin **50**
- **SCK** → pin **52**
- **VCC/GND** → alimentazione e massa compatibili con il modulo

### SD card (SPI)

- **SD CS** → pin **4**
- **SPI condivisa** con display: **MOSI 51 / MISO 50 / SCK 52**

### Altri pin principali firmware

- **Driver stepper**: `DIR=41`, `STEP=40`, `RESET=38`, `SLEEP=39`
- **SWR sensor**: `A10`
- **Joystick**: `A12` (X), `A13` (Y)
- **Pulsanti**: `42`, `35`, `36`, `37`, `34`
- **Network shield CS**: `10`

## Note

- Il codice è stato consolidato in un solo file `.cpp` per semplificare manutenzione e debug.
- Il buzzer è stato rimosso dall'implementazione corrente.
