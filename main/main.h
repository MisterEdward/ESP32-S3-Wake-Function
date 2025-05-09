#ifndef MAIN_H_
#define MAIN_H_

#include <string.h>
#include <stdbool.h> // Add this line

void httpd_init(void);
void gpio_init(void);
bool usb_is_pc_connected(void); // Add this line

#endif // MAIN_H_
