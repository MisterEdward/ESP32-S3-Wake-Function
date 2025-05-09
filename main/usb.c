#include "main.h"
#include "led.h"

#include <tinyusb.h>
#include <class/hid/hid_device.h>
#include <esp_log.h>
#include <event_groups.h>
#include <driver/gpio.h>
#include <sdkconfig.h>

static const char *TAG = "usb";

#define USB_EVENT_KEYPRESS	(1 << 0)
#define USB_EVENT_RESTART   (1 << 1)
#define USB_EVENT_SHUTDOWN  (1 << 2)
static EventGroupHandle_t usb_event_group = NULL;

// Forward declaration for the send_key function
static void send_key(uint8_t keycode, uint8_t modifier, int hold_ms, int release_ms);

#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD))
};

const char *hid_string_descriptor[5] = {
	(char[]) {
 0x09, 0x04
},  // 0: is supported language is English (0x0409)
"ESP",						// 1: Manufacturer
"Wakeup Keyboard Device",	// 2: Product
"123456",					// 3: Serials, should use chip ID
"Example HID interface",	// 4: HID
};

static const uint8_t hid_configuration_descriptor[] = {
	// Configuration number, interface count, string index, total length, attribute, power in mA
	TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

	// Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
	TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
};

// Invoked when received GET HID REPORT DESCRIPTOR request
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
	return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
	(void)instance;
	(void)report_id;
	(void)report_type;
	(void)buffer;
	(void)reqlen;

	return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
}

bool usb_is_pc_connected(void) {
    return tud_mounted();
}

void usb_request_keypress_send(bool from_isr) {
	if (from_isr)
		xEventGroupSetBitsFromISR(usb_event_group, USB_EVENT_KEYPRESS, NULL);
	else
		xEventGroupSetBits(usb_event_group, USB_EVENT_KEYPRESS);
}

void usb_request_restart_send(bool from_isr) {
    if (from_isr)
        xEventGroupSetBitsFromISR(usb_event_group, USB_EVENT_RESTART, NULL);
    else
        xEventGroupSetBits(usb_event_group, USB_EVENT_RESTART);
}

void usb_request_shutdown_send(bool from_isr) {
    if (from_isr)
        xEventGroupSetBitsFromISR(usb_event_group, USB_EVENT_SHUTDOWN, NULL);
    else
        xEventGroupSetBits(usb_event_group, USB_EVENT_SHUTDOWN);
}

// Restart Parsec application - kill and restart
void usb_request_restart_app_parsec(bool from_isr) {
    static const uint8_t keys[] = {
        0x04, // 'a' key - Alt+F4 to close
        0x00, // no key pressed - release
        0x16, // 's' key - for search/start menu
        0x00, // release
        0x13, // 'p' key - for typing "parsec"
        0x00,
        0x04, // 'a' key
        0x00,
        0x15, // 'r' key
        0x00,
        0x16, // 's' key
        0x00,
        0x08, // 'e' key
        0x00,
        0x06, // 'c' key
        0x00,
        0x28, // Enter key
        0x00
    };

    if (from_isr) {
        // Handle ISR case if needed
    } else {
        for (size_t i = 0; i < sizeof(keys); i++) {
            send_key(keys[i], 0, 100, 100);
        }
    }
}

// Restart Anydesk application - kill and restart
void usb_request_restart_app_anydesk(bool from_isr) {
    static const uint8_t keys[] = {
        0x04, // 'a' key - Alt+F4 to close
        0x00, // no key pressed - release
        0x16, // 's' key - for search/start menu
        0x00, // release
        0x04, // 'a' key - for typing "anydesk"
        0x00,
        0x11, // 'n' key
        0x00,
        0x1C, // 'y' key
        0x00,
        0x07, // 'd' key
        0x00,
        0x08, // 'e' key
        0x00,
        0x16, // 's' key
        0x00,
        0x0B, // 'h' key
        0x00,
        0x28, // Enter key
        0x00
    };

    if (from_isr) {
        // Handle ISR case if needed
    } else {
        for (size_t i = 0; i < sizeof(keys); i++) {
            send_key(keys[i], 0, 100, 100);
        }
    }
}

// Helper function to send a keyboard keypress and release with delay
static void send_key(uint8_t keycode, uint8_t modifier, int hold_ms, int release_ms) {
    uint8_t keys[6] = { keycode };
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, modifier, keys);
    vTaskDelay(pdMS_TO_TICKS(hold_ms));
    tud_hid_keyboard_report(HID_ITF_PROTOCOL_KEYBOARD, 0, NULL); // Release all keys
    vTaskDelay(pdMS_TO_TICKS(release_ms));
}

static void usb_task(void *pvParameters) {
	ESP_LOGI(TAG, "usb task started");

	while (1) {
		EventBits_t bits = xEventGroupWaitBits(usb_event_group, 
                                              USB_EVENT_KEYPRESS | USB_EVENT_RESTART | USB_EVENT_SHUTDOWN, 
                                              pdTRUE, pdFALSE, portMAX_DELAY);
        
		if (bits & USB_EVENT_KEYPRESS) {
			xEventGroupClearBits(usb_event_group, USB_EVENT_KEYPRESS);

			if (tud_mounted()) {
				ESP_LOGI(TAG, "sending wakeup signal");
				led_handle_keypress_on();

				tud_remote_wakeup();

				vTaskDelay(pdMS_TO_TICKS(50));

				led_handle_keypress_off();
			} else {
				ESP_LOGI(TAG, "not mounted, not sending keypress");
            }
		}
        
        if (bits & USB_EVENT_RESTART) {
            xEventGroupClearBits(usb_event_group, USB_EVENT_RESTART);
            
            if (tud_mounted()) {
                ESP_LOGI(TAG, "sending PC restart signal via shutdown command");
                led_handle_keypress_on();
                
                // The following sequence uses USB HID to send keyboard commands to restart the PC
                // via the Windows Run dialog (Win+R) and the "shutdown /r /t 0" command.
                // This software-based approach replaces previous potential methods such as:
                // 1. A keyboard sequence like Ctrl+Alt+Del.
                // 2. A "GPIO-based reset", which would involve using an ESP32 GPIO pin
                //    connected to the PC's motherboard reset header to trigger a hardware reset.
                // The current method provides a more graceful restart initiated by the OS.

                // Ensure PC is awake
                tud_remote_wakeup();
                vTaskDelay(pdMS_TO_TICKS(500)); // Wait for PC to respond
                
                // Press Win + R to open Run dialog
                send_key(HID_KEY_R, KEYBOARD_MODIFIER_LEFTGUI, 100, 300);
                
                // Type "shutdown /r /t 0"
                send_key(HID_KEY_S, 0, 100, 50);
                send_key(HID_KEY_H, 0, 100, 50);
                send_key(HID_KEY_U, 0, 100, 50);
                send_key(HID_KEY_T, 0, 100, 50);
                send_key(HID_KEY_D, 0, 100, 50);
                send_key(HID_KEY_O, 0, 100, 50);
                send_key(HID_KEY_W, 0, 100, 50);
                send_key(HID_KEY_N, 0, 100, 50);
                send_key(HID_KEY_SPACE, 0, 100, 50);
                send_key(HID_KEY_SLASH, 0, 100, 50); // '/'
                send_key(HID_KEY_R, 0, 100, 50);     // 'r'
                send_key(HID_KEY_SPACE, 0, 100, 50);
                send_key(HID_KEY_SLASH, 0, 100, 50); // '/'
                send_key(HID_KEY_T, 0, 100, 50);     // 't'
                send_key(HID_KEY_SPACE, 0, 100, 50);
                send_key(HID_KEY_0, 0, 100, 50);     // '0'
                
                // Press Enter
                send_key(HID_KEY_ENTER, 0, 100, 50);
                
                led_handle_keypress_off();
            } else {
                ESP_LOGI(TAG, "not mounted, not sending restart signal");
            }
        }

        if (bits & USB_EVENT_SHUTDOWN) {
            xEventGroupClearBits(usb_event_group, USB_EVENT_SHUTDOWN);
            
            if (tud_mounted()) {
                ESP_LOGI(TAG, "sending PC shutdown signal");
                led_handle_keypress_on();
                
                tud_remote_wakeup();
                vTaskDelay(pdMS_TO_TICKS(500));
                
                send_key(HID_KEY_R, KEYBOARD_MODIFIER_LEFTGUI, 100, 300);
                
                send_key(HID_KEY_S, 0, 100, 50);
                send_key(HID_KEY_H, 0, 100, 50);
                send_key(HID_KEY_U, 0, 100, 50);
                send_key(HID_KEY_T, 0, 100, 50);
                send_key(HID_KEY_D, 0, 100, 50);
                send_key(HID_KEY_O, 0, 100, 50);
                send_key(HID_KEY_W, 0, 100, 50);
                send_key(HID_KEY_N, 0, 100, 50);
                send_key(HID_KEY_SPACE, 0, 100, 50);
                send_key(HID_KEY_SLASH, 0, 100, 50);
                send_key(HID_KEY_S, 0, 100, 50);
                send_key(HID_KEY_SPACE, 0, 100, 50);
                send_key(HID_KEY_SLASH, 0, 100, 50);
                send_key(HID_KEY_T, 0, 100, 50);
                send_key(HID_KEY_SPACE, 0, 100, 50);
                send_key(HID_KEY_0, 0, 100, 50);
                
                send_key(HID_KEY_ENTER, 0, 100, 50);
                
                led_handle_keypress_off();
            } else {
                ESP_LOGI(TAG, "not mounted, not sending shutdown signal");
            }
        }
	}
}

void usb_init(void) {
	ESP_LOGI(TAG, "usb init");
	usb_event_group = xEventGroupCreate();
	const tinyusb_config_t tusb_cfg = {
		.device_descriptor = NULL,
		.string_descriptor = hid_string_descriptor,
		.string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
		.external_phy = false,
		.configuration_descriptor = hid_configuration_descriptor,
	};

	ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
	if (xTaskCreate(usb_task, "usb_task", 4096, NULL, 5, NULL) != pdPASS)
		ESP_LOGE(TAG, "error creating usb task");
	ESP_LOGI(TAG, "usb init done");
}
