#include "main.h"
#include "usb.h"

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <sdkconfig.h>

#define APP_BUTTON (GPIO_NUM_0)

static void IRAM_ATTR gpio_isr_handler(void *arg) {
	usb_request_keypress_send(true);  // Keep using this for normal button press
}

void gpio_init(void) {
	// Initialize button that will trigger HID reports
	const gpio_config_t boot_button_config = {
		.pin_bit_mask = BIT64(APP_BUTTON),
		.mode = GPIO_MODE_INPUT,
		.intr_type = GPIO_INTR_POSEDGE,
		.pull_up_en = true,
		.pull_down_en = false,
	};
	ESP_ERROR_CHECK(gpio_config(&boot_button_config));
	gpio_install_isr_service(0);
	gpio_isr_handler_add(APP_BUTTON, gpio_isr_handler, NULL);
    
    // Initialize reset pin if enabled
    #if CONFIG_ESP_WAKEUP_KEYPRESS_RESET_GPIO_ENABLED
    const gpio_config_t reset_pin_config = {
        .pin_bit_mask = BIT64(CONFIG_ESP_WAKEUP_KEYPRESS_RESET_GPIO_NUM),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up_en = false,
        .pull_down_en = false,
    };
    ESP_ERROR_CHECK(gpio_config(&reset_pin_config));
    // Set reset pin to inactive state (typically high)
    gpio_set_level(CONFIG_ESP_WAKEUP_KEYPRESS_RESET_GPIO_NUM, 1);
    #endif
}
