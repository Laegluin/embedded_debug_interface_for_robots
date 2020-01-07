#ifndef USB_INTERFACE_H
#define USB_INTERFACE_H

#include "usbd_cdc.h"

extern USBD_CDC_ItfTypeDef USB_CDC_INTERFACE;

/// Prints a message using the USB serial device. This function immediately;
/// this means that `msg` must be valid for an indeterminate amount of time:
/// it should be either a constant or leak until the program terminates.
void usb_serial_print(char* msg);

#endif
