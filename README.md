[![][Build Status img]][Build Status]
[![][License img]][License]

<div>
    <a href="http://lpsc.in2p3.fr/" target="_blank">
        <img src="https://raw.githubusercontent.com/nyxlib/nyx-node/main/docs/img/logo_lpsc.svg" height="72"></a>
    &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
    <a href="http://www.in2p3.fr/" target="_blank">
        <img src="https://raw.githubusercontent.com/nyxlib/nyx-node/main/docs/img/logo_in2p3.svg" height="72"></a>
    &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
    <a href="http://www.univ-grenoble-alpes.fr/" target="_blank">
        <img src="https://raw.githubusercontent.com/nyxlib/nyx-node/main/docs/img/logo_uga.svg" height="72"></a>
</div>

# INDI ↔ Nyx Bridge

The `Nyx` project introduces a protocol, backward-compatible with [INDI 1.7](docs/specs/INDI.pdf), for controlling
astronomical hardware. It enhances INDI by supporting multiple independent nodes, each with its own embedded protocol
stack. Nodes can communicate via an [MQTT](https://mqtt.org/) broker, a [Redis](https://redis.io/) cache (data streams)
or directly over TCP, offering flexibility and scalability for distributed systems.

This repository provides an INDI server driver that implements a bridge enabling full integration of all INDI drivers
within the Nyx environment. Messages are transported over MQTT as JSON documents.

# Typical architecture

<div style="text-align: center;">
    <img src="bridge.png" style="width: 600px;" />
</div> 

# Building and installing INDI ↔ Nyx Bridge

```bash
mkdir build
cd build

cmake ..
make
sudo make install
```

# Using INDI ↔ Nyx Bridge

```bash
indiserver indi_nyx ...
```

# Configuring INDI ↔ Nyx Bridge

<div style="text-align: center;">
    <img src="indi.png" style="width: 600px;" />
</div> 

# Home page and documentation

Home page:
* https://nyxlib.org/

Documentation:
* https://nyxlib.org/documentation/

# Developer

* [Jérôme ODIER](https://annuaire.in2p3.fr/4121-4467/jerome-odier) ([CNRS/LPSC](http://lpsc.in2p3.fr/))

# A bit of classical culture

In Greek mythology, Nyx is the goddess and personification of the night. She is one of the primordial deities, born from Chaos at the dawn of creation.

Mysterious and powerful, Nyx dwells in the deepest shadows of the cosmos, from where she gives birth to many other divine figures, including Hypnos (Sleep) and Thanatos (Death).

<div style="text-align: center;">
    <img src="https://raw.githubusercontent.com/nyxlib/nyx-node/refs/heads/main/docs/img/nyx.png" style="width: 600px;" />
</div>

[Build Status]:https://github.com/nyxlib/INDI-Nyx-Bridge/actions/workflows/deploy.yml
[Build Status img]:https://github.com/nyxlib/INDI-Nyx-Bridge/actions/workflows/deploy.yml/badge.svg

[License]:https://www.gnu.org/licenses/gpl-2.0.txt
[License img]:https://img.shields.io/badge/License-GPL_2.0_only-blue.svg
