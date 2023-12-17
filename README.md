# Thrustmaster T150 and TMX Force Feedback Wheel Linux drivers
**DISCLAMER**
*This is not an official driver from Thrustmaster and is provided without any kind of warranty. Loading and using this driver is at your own risk; I don't take responsibility for kernel panics, devices bricked or any other kind of inconvenience*

## Project status

### What's working 👌
+ All axis and buttons of the wheel are reported
+ You can set the range of the T150 wheel from 270° to 1080°
+ You can set the range of the TMX wheel from 270° to 900°
+ You can set the return force of the wheel from 0% to 100%
+ Force feedback (partially)
  * You can set the global force feedback scale from 0% to 100%
  * Settable gain `FF_GAIN`
  * Periodic effects:  `FF_SINE`, `FF_SAW_UP` and `FF_SAW_DOWN`
  * Constant effects:  `FF_CONSTANT`
  * Condition effects: `FF_SPRING`
  * Damper effects:    `FF_DAMPER`
+ Firmware version is reported

### What is missing 🚧
- Reading the settings from the wheel
- Force feedback (partially)
- Force feedback settings
- Firmware upgrades
- Handling of range changes from the wheel

### Switch the wheel from FFB to full T150
Since kernel version 5.13, hid_thrustmaster automatically switch some thrustmaster devices into full mode. The T150 is covered by this driver. The TMX wheel does not have support in the hid_thrustmaster and requries either using [TMX-Driver](https://github.com/emtek995/TMX-driver) (kernel driver) or [tmdrv](https://github.com/her001/tmdrv) (python userspace driver) to intialize.

## How to use the driver
**For T150, always put the switch of your wheel to the `PS3` position before plug it into your machine!**
For the TMX, plug the wheel into your machine and load your preffered intialization driver. The mode switch will only be used to swap the clutch and accelerator pedals if using the T3PA-Pro in hangdown mode.


### Setting up the wheel parameters
You can edit the settings of each wheel attached to the machine by writing the sysfs attributes usually found in the 
subdirectories at `/sys/devices`. You can see in `dmesg` what path in /sys the input subsystem assigned to the wheel:
for example if you see `input: Thrustmaster T150 steering wheel as /devices/pci0000:00/0000:00:14.0/usb1/1-1/input/input27`
then the attributes will be located at `sys/devices/pci0000:00/0000:00:14.0/usb1/1-1/input/input27/device/`.

This table contains a summary of each attribute

|Attribute          |Value                         |Description                                                       |
|-------------------|------------------------------|------------------------------------------------------------------|
|`range`            |decimal from `270` to `1080`  |How far the wheel turns (TMX limited to 900)                      |
|`autocenter`       |decimal from `0` to `100`     |The force used to re-center the wheel                             |
|`enable_autocenter`|boolean `y` xor `n`           |Use the user defined return force or let the game handle it trough ffb|
|`gain`             |decimal from `0` to `100`     |Force feedback intensity. 0 no effects are reproduced             |
|`firmware_version` |decimal                       |Read only, the current firmware running on the wheel              |

### Custom defaults
To automatically set the wheel to some custom default settings when plugged you'll have to write a simple udev rule. In `/etc/udev/rules.d` create a text file called something like `99-t150-defaults.rules` and write a rule like this below. Refer to the output of `udevadm info --attribute-walk /sys/devices/${WHEEL_DEVICE_PATH}` in case rules from example below do not match.
```
SUBSYSTEM=="hid", ATTRS{driver}=="hid-t150", ATTR{range}="270", ATTR{gain}="75"
```
Then run `udevadm control --reload` and `udevadm trigger` to re-load the rules. 
The rule in the example should set the turning range to 270°.

## How to install and load the driver
You can try to run `install.sh` as root, the script should: copy the udev rules and other files in their appropriate positions, build and install the DKMS modules and add them to the list of modules to be loaded at boot. 

To check if the modules are loaded check the output of `lsmod | grep hid-t150`.

### Manually 
Copy the udev rules into `/etc/udev/rules.d/` and reload the udev rules (or reboot)...

#### Build the drivers
For a simple build: install all the required tools to compile (like `build-essential`, `linux-headers` etc...) and run
```
make
```
into the t150 folder. Now you can load the .ko file with `insmod` and unload with `rmmod`
