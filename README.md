# Traffic Lights

A 3-lamp UK traffic light (red, amber, green) driven by a BBC micro:bit V2, controlled by a push button and over Bluetooth from a web page.

**Web control page:** https://optimiseddesign.github.io/traffic-lights/

## What's in this repo

### [`lights/`](lights/) - micro:bit firmware (runs on the light itself)

C++ firmware built with Lancaster University's CODAL runtime. It runs the UK traffic light cycle, a party mode toggled by the push button, and accepts Bluetooth commands (Nordic UART Service) to set lamp brightness and fades, change mode, and play sounds.

See [lights/README.md](lights/README.md) for wiring, the command list, build and flashing instructions, and troubleshooting.

### [`index.html`](index.html) - web Bluetooth control page

A single-page web app that connects to the light over Web Bluetooth. It lets you tap lamps on and off (with optional fades), switch between traffic, party and manual modes, play sounds, and send raw commands, with a log of everything sent and received.

It is hosted on GitHub Pages at https://optimiseddesign.github.io/traffic-lights/. It needs a browser with Web Bluetooth, such as Chrome or Edge (not Firefox or Safari).

## Quick start

1. Build the firmware and copy `lights/release/MICROBIT.hex` to the `MICROBIT` USB drive (see [lights/README.md](lights/README.md#building-the-project))
2. Open the [web control page](https://optimiseddesign.github.io/traffic-lights/) in Chrome or Edge
3. Click **Connect** and pick the traffic light from the list

## Background

Adapted from an earlier [Keyes MiniCar robot project](https://github.com/lorempipsum/digital-democracy/tree/main/robot), keeping its build system and Bluetooth command structure.

## License

See [LICENSE](LICENSE).
