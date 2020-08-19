# T150 Force Feedback Wheel Linux drivers
**DISCLAMER**
*This is not an official driver from Thrustmaster and is provided without any kind of warranty. Loading and using this driver is at your own risk; I don't take responsibility for kernel panics, devices bricked or any other kind of inconvenience*

## Project status
The driver is to be partially re-written as a hid driver instead of an usb driver.

### What's working 👌
+ All axis and buttons of the wheel are reported¹
+ You can set the range of the wheel from 270° to 1080°
+ You can set the return force of the wheel from 0% to 100%
+ Force feedback (partially)
  * You can set the global force feedback scale from 0% to 100%
  * Settable gain `FF_GAIN`
  * Periodic effects:  `FF_SINE`, `FF_SAW_UP` and `FF_SAW_DOWN`
  * Constant effects:  `FF_CONSTANT`
  * Condition effects: `FF_SPRING`
  * Damper effects:    `FF_DAMPER`
+ Firmware version is reported

¹: Except for the shifter buttons, because I don't have the PRO version. However, I've guessed the clutch axis.

### What is missing 🚧
- Reading the settings from the wheel
- Force feedback (partially)
- Force feedback settings
- Firmware upgrades

## How to use the driver
**Always put the switch of your wheel to the `PS3` position before plug it into your machine!**

### Switch the wheel from FFB to full T150
When attached to your machine the wheel reports itself as `Thrustmaster FFB Wheel`, in this mode not all functionalities
are available. In order to switch to the `Thrustmaster T150RS` we have to send the following USB control packet to the 
wheel:

| bRequestType | bRequest | wValue | wIndex | wLength |
|--------------|----------|--------|--------|---------|
| `0x41`       | `83`     |`0x0006`| `0`    | `0`     |

To do so we can use the [`hid-tminit`](https://github.com/scarburato/hid-tminit) driver (See the install section, if you use the install script it should do it automatically) xor you can write a simple userspace applications like  [this one](https://gitlab.com/her0/tmdrv) thanks to `libusb`.

When the wheel receives the control packet it will reset and re-appear in the system as a T150.

### Setting up the wheel parameters
You can edit the settings of each wheel attached to the machine by writing the sysfs attributes usually found in the 
subdirectories at `/sys/devices`. You can see in `dmesg` what path in /sys the input subsystem assigned to the wheel:
for example if you see `input: Thrustmaster T150 steering wheel as /devices/pci0000:00/0000:00:14.0/usb1/1-1/input/input27`
then the attributes will be located at `sys/devices/pci0000:00/0000:00:14.0/usb1/1-1/input/input27/device/`.

This table contains a summary of each attribute

|Attribute          |Value                         |Description                                                       |
|-------------------|------------------------------|------------------------------------------------------------------|
|`range`            |decimal from `270` to `1080`  |How far the wheel turns                                           |
|`autocenter`       |decimal from `0` to `100`     |The force used to re-center the wheel                             |
|`enable_autocenter`|boolean `y` xor `n`           |Use the user defined return force or let the game handle it trough ffb|
|`gain`             |decimal from `0` to `100`     |Force feedback intensity. 0 no effects are reproduced             |
|`firmware_version` |decimal                       |Read only, the current firmware running on the wheel              |

## How to install and load the driver
You can try to run `install.sh` as root, the script should: copy the udev rules and other files in their appropiate positions, build and install the DKMS modules and add them to the list of modules to be loaded at boot. 

To check if the modules are loaded check the output of `lsmod | grep t150` and `lsmod | grep hid-tminit`.

### Manually 
Copy the udev rules into `/etc/udev/rules.d/` and reload the udev rules (or reboot)...

#### Build the drivers
For a simple build: install all the required tools to compile (like `build-essential`, `linux-headers` etc...) and run
```
make
```
into the t150 and hid-tminit folders. Now you can load the .ko files with `insmod` and unload them with `rmmod`
