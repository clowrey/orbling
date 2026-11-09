# Install

A virtual pet with nine mini games (five of them times-tables and division practice) for the round
**Guition JC3636W518** touch display. Source and instructions:
[github.com/clowrey/orbling](https://github.com/clowrey/orbling).

1. Use Chrome or Edge on a computer.
2. Plug the board in with a USB-C data cable.
3. Click the button, pick the board's serial port and install.
4. When it asks, choose your Wi-Fi network. Home Assistant then finds the pet on its own.

<esp-web-install-button manifest="firmware/orbling.manifest.json"></esp-web-install-button>

<script type="module" src="https://unpkg.com/esp-web-tools@10/dist/web/install-button.js?module"></script>

Installing replaces whatever firmware is on the board. Later updates from Home Assistant keep the pet.
