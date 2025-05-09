#ifndef USB_H_
#define USB_H_

#include <stdbool.h>

void usb_request_keypress_send(bool from_isr);
void usb_request_restart_send(bool from_isr);
void usb_request_shutdown_send(bool from_isr);
void usb_request_restart_app_parsec(bool from_isr);
void usb_request_restart_app_anydesk(bool from_isr);
void usb_init(void);

#endif // USB_H_
