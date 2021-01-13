# RT-WIFI-CSI

This project aims to merge Atheros CSI Tool with RT-WiFi project.

**Read the Issues and README first before starting coding.**

Both tools are built on ath9k driver.

### Notice for downloading this repository

Do NOT try to clone or download the ZIP file and unzip it under Windows Environment (Chinese Lang.), it will raise an directory error (unclear about the detailed reason but I guess is related to encoding schemes as I am using a Chinese lang version of Windows).

## Branch introduction

This branch is based on the Atheros CSI tool and we try to merge the RT-WiFi code into the driver.

This also integrates three userspace tools for CSI collection and analysis.
Namely,

##### UserTool for Atheros CSI Tool
https://github.com/xieyaxiongfly/Atheros-CSI-Tool-UserSpace-APP
   * Note: To correctly analyze the CSI, you need to use *mex* provided by MATLAB to compile the C source code (tested under ubuntu 14.04/20.04, Windows 10 Pro 2004/20H2, macOS Catalina 10.15.6/BigSur 11.1, diff os will output diff mex output files) to use the *read_csi* function. 
   * Change the `"ieee-be"` to `"ieee-le"` in Line 46 of *read_log_file.m* if you cannot read the packets correctly. 
##### csi-server-openwrt
https://github.com/putterer/csi-server-openwrt
   * A server for shipping channel state information obtained using the Atheros CSI Tool via UDP in real time. 
   * Note: The author is somehow uncertain about the functionality of the server side under x86 systems as no tests are conducted.
##### csiread
https://github.com/citysu/csiread    
   * Parse channel state information from raw CSI data file in Python.
## Source of this project

**Atheros CSI Tool**: https://github.com/xieyaxiongfly/Atheros-CSI-Tool

*Manual for Atheros CSI Tool*: https://wands.sg/research/wifi/AtherosCSI/document/Atheros-CSI-Tool-User-Guide.pdf

**RT-WiFi Project**: https://www.cs.utexas.edu/users/cps/rt-wifi/RT-WiFi-v0.1.tgz

*Manual for RT-WiFi Project*: https://www.cs.utexas.edu/users/cps/rt-wifi/RT-WiFi%20user%20guide%20v0.1.pdf

## Problems solved

### Building side

* New Kconfig file and makefile
* Solve the kernel version issue: 4.15(displayed as 4.1.10+) for Atheros CSI Tool and 3.13 for RT-WiFi project
* For compatability reasons, removing one debugging interface (cfg80211 DebugFS entries) included in RT-WiFi project and being unclear whether it matters. 

### Notice for building

Do NOT put the folder under some folder with space in its name like *Untitled folder*, it will result in a **make error** during the `sudo make modules_install`.

## Current Progress

**Build Succeeded!** with `uname -r` displaying *4.1.10+*.

* CSI Tool works fine without RT-WiFi, can correctly collect and readable CSI packets.
* RT-WiFi functionality will conflict with CSI receiving, fix needed.
