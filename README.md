[![][Build Status img]][Build Status]
[![][License img]][License]

<a href="https://lpsc.in2p3.fr/" target="_blank"><img src="https://raw.githubusercontent.com/nyxlib/nyx-node/refs/heads/main/docs/img/logo_lpsc.svg" alt="LPSC" height="72" /></a>
&nbsp;&nbsp;&nbsp;&nbsp;
<a href="https://www.in2p3.fr/" target="_blank"><img src="https://raw.githubusercontent.com/nyxlib/nyx-node/refs/heads/main/docs/img//logo_in2p3.svg" alt="IN2P3" height="72" /></a>
&nbsp;&nbsp;&nbsp;&nbsp;
<a href="https://www.univ-grenoble-alpes.fr/" target="_blank"><img src="https://raw.githubusercontent.com/nyxlib/nyx-node/refs/heads/main/docs/img//logo_uga.svg" alt="UGA" height="72" /></a>

# Nyx Node

The `Nyx` project introduces a protocol, backward-compatible with [INDI 1.7](docs/specs/INDI.pdf), for controlling
astronomical hardware. It enhances INDI by supporting multiple independent nodes, each with its own embedded protocol
stack. Nodes can communicate via an [MQTT](https://mqtt.org/) broker, a [Redis](https://redis.io/) cache (data streams)
or directly over TCP, offering flexibility and scalability for distributed systems.


# Typical architecture

<div style="text-align: center;">
    <img src="https://raw.githubusercontent.com/nyxlib/nyx-node/refs/heads/main/docs/img/nyx.svg" style="width: 600px;" />
</div> 

# Build instructions

```bash
mkdir build
cd build

cmake ..
make
sudo make install
```

# Home page and documentation

Home page:
* https://nyxlib.org/

Documentation:
* https://nyxlib.org/documentation/

# Developer

* [Jérôme ODIER](https://annuaire.in2p3.fr/4121-4467/jerome-odier) ([CNRS/LPSC](http://lpsc.in2p3.fr/))

# A bit of history

In Greek mythology, Nyx is the goddess and personification of the night.

<div style="text-align: center;">
    <img src="https://raw.githubusercontent.com/nyxlib/nyx-node/refs/heads/main/docs/img/nyx.png" style="width: 600px;" />
</div>

[Build Status]:https://github.com/nyxlib/INDI-Nyx-Bridge/actions/workflows/deploy.yml
[Build Status img]:https://github.com/nyxlib/INDI-Nyx-Bridge/actions/workflows/deploy.yml/badge.svg

[License]:https://www.gnu.org/licenses/gpl-2.0.txt
[License img]:https://img.shields.io/badge/License-GPL_2.0_only-blue.svg
