# usbcontrol
Monitor USB devices and control devices' access.\
Each urb must submit to USB core, and this process is achieved through [int usb_submit_urb(struct urb *urb, int mem_flags);].\
The removal of MODULE still has problems. usb's judgment did not write.
