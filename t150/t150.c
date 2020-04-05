#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/usb/ch9.h>
#include <linux/completion.h>

#include "t150.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dario Pagani <>");
MODULE_DESCRIPTION("ThrustMaster T150 steering wheel driver");
