/*  usbhack.c  */
#include <linux/module.h>      //Needed by all modules                 
#include <linux/kernel.h>      //Needed for KERN_INFO    
#include <linux/moduleparam.h> //Parameter processing related header files
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/usb.h>
 
#include <linux/hardirq.h>


MODULE_LICENSE("GPL");         //Declare Licesne                                                                                                                                        
MODULE_AUTHOR("Hack USB");      //Declare module author                                                                                                                                          
  
#define CLEAR_CR0    asm ("pushq %rax\n\t"             \
"movq %cr0, %rax\n\t"        \
"and $0xfffffffffffeffff, %rax\n\t"     \
"movq %rax, %cr0\n\t"        \
"popq %rax");
 
#define SET_CR0        asm ("pushq %rax\n\t"             \
"movq %cr0, %rax\n\t"         \
"or $0x00010000, %rax\n\t"     \
"movq %rax, %cr0\n\t"        \
"popq %rax");

static  u_char tmp[5];
static  int hook_usb_submit_urb(struct urb *urb,int mem_flags);

static void hook(void)
{
    u_char *buf;
        long p;
         
 
        buf = (u_char *)usb_submit_urb;
        p = (long)hook_usb_submit_urb - (long)usb_submit_urb - (long)5;
         
        //printk("%p\n",buf);
         
//       lock_kernel();
    CLEAR_CR0
     
    memcpy(tmp, buf, 5);   
     
        buf[0] = 0xe9;
        memcpy( buf + 1, &p, 4);
     
    SET_CR0
//    unlock_kernel();
     
}
 
static  void unhook(void)
{
        u_char *buf;
             
//    lock_kernel();
    CLEAR_CR0
     
        buf = (u_char *)usb_submit_urb;
        memcpy(buf, tmp, 5);
     
    SET_CR0
//    unlock_kernel();   
}

static  int hook_usb_submit_urb(struct urb *urb,int mem_flags)
{
    /*
	//urb->dev->descriptor.bDeviceClass
	00h:Device:Use class information in the Interface Descriptors
	01h:Interface:Audio 
	02h:Both:Communications and CDC Control
	03h:Interface:HID (Human Interface Device)
	05h:Interface:Physical
	06h:Interface:Image
	07h:Interface:Printer
	*/
	int	xfertype;
	struct usb_device *dev;
	struct usb_host_endpoint *ep;
	int	is_out;//1 out;0 in
	
	__u8  bDeviceClass;
	__u8  bInterfaceClass;
	int ret;

	printk(KERN_INFO "[NTSL]hook_usb_submit_urb() excute\n");


	if (!urb || urb->hcpriv || !urb->complete)
		return -EINVAL;
	dev = urb->dev;
	if ((!dev) || (dev->state < USB_STATE_UNAUTHENTICATED))
		return -ENODEV;
	ep = usb_pipe_endpoint(dev, urb->pipe);
	if (!ep)
		return -ENOENT;
	xfertype = usb_endpoint_type(&ep->desc);
	if (xfertype == USB_ENDPOINT_XFER_CONTROL) {
		struct usb_ctrlrequest *setup =
				(struct usb_ctrlrequest *) urb->setup_packet;
 
		if (!setup)
			return -ENOEXEC;
		is_out = !(setup->bRequestType & USB_DIR_IN) ||
				!setup->wLength;
	} else {
		is_out = usb_endpoint_dir_out(&ep->desc);
	}
	printk(KERN_INFO "[NTSL]urb derection is %d(in:0;out:1)\n",is_out);
	if(is_out)
	{
		printk(KERN_INFO "[NTSL]urb derection is out, use origin hook_usb_submit_urb()\n");
		//Recovery function
        unhook();          
        //Call the original function
        ret = usb_submit_urb(urb,mem_flags); 
        printk(KERN_INFO "[NTSL]origin hook_usb_submit_urb() return:%d\n",ret);
        //Restore hook
        hook();
	}
	else{
		bDeviceClass = dev->descriptor.bDeviceClass;
		printk(KERN_INFO "[NTSL]bDeviceClass = %d\n",bDeviceClass);
		//Decide the next action based on type
		switch(bDeviceClass){
			case 0:
				bInterfaceClass = dev->actconfig->interface[0]->cur_altsetting->desc.bInterfaceClass;
				printk(KERN_INFO "[NTSL]bInterfaceClass = %d\n",bInterfaceClass);
				//switch(bInterfaceClass)

				break;
			default:
				
				printk(KERN_INFO "[NTSL]kill urb\n");	
				//usb_kill_urb(urb);
				ret = usb_unlink_urb(urb);
				a= -EINVAL;
				printk(KERN_INFO "[NTSL]usb_unlink_urb return %d,a=%d\n",ret,a);
				return -EINVAL;
				break;
		}
	}
	return ret;
	

} 

 
static int __init usbhack_init(void)
{
	printk(KERN_INFO "[NSTL]replace usb_submit_urb()\n"); //printk is a kernel print function that can print log to the system log
	hook();  
    return 0;
}
 
static void __exit usbhack_exit(void)
{
    printk(KERN_INFO "[NTSL]restore usb_submit_urb()\n");	
	unhook();
}
 
//In fact, there are several usages of the module installation and uninstallation functions. You can refer to the guide in References.
module_init(usbhack_init); //Module load function, called when module is installed with modprobe or insmod command
module_exit(usbhack_exit); //Module uninstall function, called when the module is uninstalled with the rmmod command, can perform some cleanup and restore operations
