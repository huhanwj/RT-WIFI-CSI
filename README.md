# RT-WIFI-CSI

This project aims to merge Atheros CSI Tool with RT-WiFi project.

**Read the Issues and README first before starting coding.**

Both tools are built on ath9k driver.

### Notice for downloading this repository

Do NOT try to clone or download the ZIP file and unzip it under Windows Environment (Chinese Lang.), it will raise an directory error (unclear about the detailed reason but I guess is related to encoding schemes as I am using a Chinese lang version of Windows).

All of our testing is done on x86 platforms, we don't know whether this is compatible with ARM chips.
## Installation
First, remember to download or clone the whole repository.
### System Requirements
Any Ubuntu system with kernel version being less or equal to 5.4.
### Installation of required packages
Some packages are needed (may still have missing packages when compiling, add the missing packages yourself depending on what error you encounter)

`sudo apt-get install build-essential bison bc flex libncurses5-dev libncursesw5-dev libssl-dev libelf-dev`

Then, run the following command in this folder

`make menuconfig`

The default config is configured installing CSI Tool without RT-WIFI, you may check it manually.

Next compile the kernel modules
```
make -j4
sudo make modules_install
sudo make install
```
Note here that the "-j4" is used for accelerating the compiling process. The number "4" is select according to the number of threads that your CPU have. If your CPU has hyperthreading technology (HT), the number should be 2 times your CPU cores (e.g. most Intel Core i7 CPUs, Intel Xeon CPUs and AMD Ryzen CPUs). For CPUs not supporting such technology, which normally depends on the settings of chip manufacturer (e.g. i7-9700K does not support HT but i7-8700K does), just put in the number of your CPU cores (e.g. the CPUs used for my testing, which are Intel Core i5-4590T and Intel Core i5-9600K).
## Branch introduction

This branch is based on the Atheros CSI tool and we try to merge the RT-WiFi code into the driver.

This also integrates three userspace tools for CSI collection and analysis.
Namely,

##### UserTool for Atheros CSI Tool
https://github.com/wands-wireless/Atheros-CSI-Tool-UserSpace-APP
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

**Atheros CSI Tool**: https://github.com/wands-wireless/Atheros-CSI-Tool

*Manual for Atheros CSI Tool*: https://wands.sg/research/wifi/AtherosCSI/document/Atheros-CSI-Tool-User-Guide.pdf

**RT-WiFi Project**: https://www.cs.utexas.edu/users/cps/rt-wifi/RT-WiFi-v0.1.tgz

*Manual for RT-WiFi Project*: https://www.cs.utexas.edu/users/cps/rt-wifi/RT-WiFi%20user%20guide%20v0.1.pdf

### Notice for building

Do NOT put the folder under some folder with space in its name like *Untitled folder*, it will result in a **make error** during the `sudo make modules_install`.

If you find that `uname -r` does not give `5.4.0-Hybrid`, then you can modify `/etc/default/grub` and deactivate `GRUB_HIDDEN_TIMEOUT=0` by commenting it or if there is no `GRUB_HIDDEN_TIMEOUT=0`, just change `GRUB_TIMEOUT` to -1. Then run `sudo upgrade-grub` to update the grub and reboot the system. You can see the grub menu and select advanced options for ubuntu and select our customized kernel.

If you encounter the error stating

`make[1]: *** No rule to make target 'debian/canonical-certs.pem', needed by 'certs/x509_certificate_list'.  Stop.`,

you need to modify `.config` file.

In your kernel configuration file you will find this line:

`CONFIG_SYSTEM_TRUSTED_KEYS="debian/canonical-certs.pem"`

Change it to this:

`CONFIG_SYSTEM_TRUSTED_KEYS=""`

**IMPORTANT**: Disable Secure Boot before installing the kernel.

**Know Issues**: An unknown issue will cause boot failure in Lenovo ThinkCentre M920q (i5-9500T) (Possibly due to Lenovo BIOS or Intel microcode issue, both tested under the newest version).
