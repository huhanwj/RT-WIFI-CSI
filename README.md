# RT-WIFI-CSI
This project aims to merge Atheros CSI Tool with RT-WiFi project.

Both tools are built on ath9k driver.
## Source of this project
**Atheros CSI Tool**: https://github.com/xieyaxiongfly/Atheros-CSI-Tool

*Manual for Atheros CSI Tool*: https://wands.sg/research/wifi/AtherosCSI/document/Atheros-CSI-Tool-User-Guide.pdf

**RT-WiFi Project**: https://www.cs.utexas.edu/users/cps/rt-wifi/RT-WiFi-v0.1.tgz

*Manual for RT-WiFi Project*: https://www.cs.utexas.edu/users/cps/rt-wifi/RT-WiFi%20user%20guide%20v0.1.pdf

## Problems to solve
* New Kconfig file and makefile
* Solve the kernel version issue: 4.15(displayed as 4.1.10+) for Atheros CSI Tool and 3.13 for RT-WiFi project

## Branch introduction
This branch is based on the Atheros CSI tool and we try to merge the RT-WiFi code into the driver.
