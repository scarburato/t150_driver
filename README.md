# T150 Force Feedback Wheel Linux drivers
**DISCLAMER**
*This is not an official driver from Thrustmaster and is provided without any kind of warranty. Loading and using this driver is at your own risk; I don't take responsibility for kernel panics, devices bricked or any other kind of inconvenience*

## Project status
At the moment the `t150` module has memory leaks in case of errors in the `t150_probe` function

### What's working
+ All axis and buttons of the wheel are reported
+ You can set the range of the wheel from 270° to 1080°
+ You can set the return force of the wheel from 0% to 100%

### What is missing
- Reading the settings from the wheel
- This driver is not used by the Kernel when the `hid` module is loaded
- Force feedback
- Force feedback settings
- Firmware upgrades

## How to install and load the driver
...
