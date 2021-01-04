# RT-WIFI-CSI
This project aims to merge Atheros CSI Tool with RT-WiFi project.

Both tools are built on ath9k driver.
## Branch introduction
This branch is based on the Atheros CSI tool and we try to merge the RT-WiFi code into the driver.
## Source of this project
**Atheros CSI Tool**: https://github.com/xieyaxiongfly/Atheros-CSI-Tool

*Manual for Atheros CSI Tool*: https://wands.sg/research/wifi/AtherosCSI/document/Atheros-CSI-Tool-User-Guide.pdf

**RT-WiFi Project**: https://www.cs.utexas.edu/users/cps/rt-wifi/RT-WiFi-v0.1.tgz

*Manual for RT-WiFi Project*: https://www.cs.utexas.edu/users/cps/rt-wifi/RT-WiFi%20user%20guide%20v0.1.pdf

**UserTool for Atheros CSI Tool**: https://github.com/xieyaxiongfly/Atheros-CSI-Tool-UserSpace-APP
* Note: To correctly analyze the CSI, you need to use *mex* provided by MATLAB to compile the C source code (tested under ubuntu 14.04/20.04, Windows 10 Pro 2004/20H2, macOS Catalina 10.15.6/BigSur 11.1, diff os will output diff mex output files) to use the *read_csi* function.
## Problems to solve
* New Kconfig file and makefile
* Solve the kernel version issue: 4.15(displayed as 4.1.10+) for Atheros CSI Tool and 3.13 for RT-WiFi project
## Build log
### First Build 
* Net configuration has redefinition problems and variable confliction. *Fix try by discarding all RT-WiFi files.*
* Lots of missing members in RT-WiFi struct. *Forget to merge two* **ath9k.h** *.*
### To do after first build
* ~~Discarding all RT-WiFi files in network API configuration part.~~
* ~~Merging **ath9k.h** and try again.~~
* Fixing bugs brought by new *ath9k* driver.
### Second Build
* Reference problems related to the source codes.
* Solve the difference between the two tools and now `make -j6` will not report error.
## Notice
Do NOT put the folder under some folder with space in its name like *Untitled folder*, it will result in a **make error** during the `sudo make modules_install`.
## Current Progress
**Build Succeeded!** with `uname -r` displaying *4.1.10+*.
* CSI Tool works fine, can correctly collect and readable CSI packets.
* ~~RT-WiFi test is scheduled tomorrow.~~
* RT-WiFi functionality will conflict with CSI receiving, fix needed.

