#include "main.h"
#include "usb.h"
#include "web_ui.h"

#include <esp_log.h>
#include <esp_http_server.h>
#include <esp_system.h>
#include <string.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

static const char *TAG = "httpd";

static void delayed_restart(void* arg);

// Root handler to serve the web UI
static esp_err_t httpd_root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html, strlen(index_html));
    return ESP_OK;
}

static esp_err_t httpd_wakeup_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "received wake request, sending keypress");
    usb_request_keypress_send(false);

    char *resp;
    int rc = asprintf(&resp, "{\"keypress_sent\": true}");
    if (rc < 0) {
        ESP_LOGE(TAG, "asprintf() returned: %d", rc);
        return ESP_FAIL;
    }
    if (!resp) {
        ESP_LOGE(TAG, "nomem for response");
        return ESP_ERR_NO_MEM;
    }
    httpd_resp_send(req, resp, strlen(resp));
    free(resp);

    return ESP_OK;
}

// Convert MAC string like "00-11-22-33-44-55" to byte array
static bool parse_mac(const char *mac_str, uint8_t *mac_bytes) {
    if (strlen(mac_str) != 17) { // Expect format like "00-11-22-33-44-55"
        return false;
    }
    
    char byte_str[3];
    byte_str[2] = '\0';
    
    for (int i = 0; i < 6; i++) {
        byte_str[0] = mac_str[i*3];
        byte_str[1] = mac_str[i*3 + 1];
        char *end;
        mac_bytes[i] = strtol(byte_str, &end, 16);
        if (end != byte_str + 2) {
            return false;
        }
    }
    
    return true;
}

// Send Wake-on-LAN magic packet
static esp_err_t send_wol_packet(const uint8_t *mac) {
    // Create socket
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket: %s", strerror(errno));
        return ESP_FAIL;
    }
    
    // Enable broadcast
    int broadcast = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        ESP_LOGE(TAG, "Failed to set socket options: %s", strerror(errno));
        close(sock);
        return ESP_FAIL;
    }
    
    // Prepare WoL magic packet - 6 bytes of 0xFF followed by target MAC repeated 16 times
    uint8_t packet[102]; // 6 + 16*6 = 102 bytes
    memset(packet, 0xFF, 6);
    
    // Add MAC address repeated 16 times
    for (int i = 0; i < 16; i++) {
        memcpy(&packet[6 + i * 6], mac, 6);
    }
    
    // Set up broadcast address
    struct sockaddr_in broadcastAddr;
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(9); // Standard WoL port
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    
    // Send packet
    int sent = sendto(sock, packet, sizeof(packet), 0, 
                     (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
    
    close(sock);
    
    if (sent < 0) {
        ESP_LOGE(TAG, "Failed to send WoL packet: %s", strerror(errno));
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Sent WoL packet to %02x:%02x:%02x:%02x:%02x:%02x", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return ESP_OK;
}

static esp_err_t httpd_wol_get_handler(httpd_req_t *req) {
    char mac_str[18] = {0}; // To hold MAC address string (e.g., "00-11-22-33-44-55")
    bool success = false;
    
    // Get the MAC address from query parameter
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char *buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            if (httpd_query_key_value(buf, "mac", mac_str, sizeof(mac_str)) == ESP_OK) {
                ESP_LOGI(TAG, "Received WoL request for MAC: %s", mac_str);
                
                // Parse MAC string to byte array
                uint8_t mac_bytes[6];
                if (parse_mac(mac_str, mac_bytes)) {
                    // Send WoL packet
                    esp_err_t err = send_wol_packet(mac_bytes);
                    success = (err == ESP_OK);
                } else {
                    ESP_LOGE(TAG, "Invalid MAC format: %s", mac_str);
                }
            }
        }
        free(buf);
    }
    
    // Prepare response
    char *resp;
    int rc = asprintf(&resp, "{\"wol_sent\": %s}", success ? "true" : "false");
    if (rc < 0 || !resp) {
        ESP_LOGE(TAG, "Failed to prepare response");
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));
    free(resp);
    
    return ESP_OK;
}

// Handler to force restart the connected PC
static esp_err_t httpd_pc_restart_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "received PC restart request, sending restart signal");
    
    // Use the dedicated restart function
    usb_request_restart_send(false);
    
    // Send response
    char *resp;
    int rc = asprintf(&resp, "{\"pc_restart_sent\": true}");
    if (rc < 0) {
        ESP_LOGE(TAG, "asprintf() returned: %d", rc);
        return ESP_FAIL;
    }
    if (!resp) {
        ESP_LOGE(TAG, "nomem for response");
        return ESP_ERR_NO_MEM;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));
    free(resp);
    
    return ESP_OK;
}

// Handler to shutdown the connected PC
static esp_err_t httpd_pc_shutdown_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "received PC shutdown request, sending shutdown signal");
    
    // Use the dedicated shutdown function
    usb_request_shutdown_send(false);
    
    // Send response
    char *resp;
    int rc = asprintf(&resp, "{\"pc_shutdown_sent\": true}");
    if (rc < 0) {
        ESP_LOGE(TAG, "asprintf() returned: %d", rc);
        return ESP_FAIL;
    }
    if (!resp) {
        ESP_LOGE(TAG, "nomem for response");
        return ESP_ERR_NO_MEM;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));
    free(resp);
    
    return ESP_OK;
}

// Handler to restart the ESP32
static esp_err_t httpd_esp_restart_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Received ESP restart request");
    
    // Send response before restarting
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"restart\":true}", 15);
    
    // Schedule a restart after a short delay to allow response to be sent
    esp_timer_handle_t restart_timer;
    const esp_timer_create_args_t timer_args = {
        .callback = &delayed_restart,
        .name = "restart_timer"
    };
    
    if (esp_timer_create(&timer_args, &restart_timer) == ESP_OK) {
        // Restart after 500ms
        esp_timer_start_once(restart_timer, 500000);
    } else {
        // If creating timer fails, restart immediately
        esp_restart();
    }
    
    return ESP_OK;
}

// Callback for delayed restart
static void delayed_restart(void* arg) {
    ESP_LOGI(TAG, "Restarting ESP32...");
    esp_restart();
}

// Handler to restart applications (Parsec or Anydesk)
static esp_err_t httpd_restart_app_get_handler(httpd_req_t *req) {
    char app_name[32] = {0};
    bool success = false;
    char message[64] = {0};
    
    // Get the app parameter from query
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char *buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            if (httpd_query_key_value(buf, "app", app_name, sizeof(app_name)) == ESP_OK) {
                ESP_LOGI(TAG, "Received app restart request for: %s", app_name);
                
                if (strcmp(app_name, "parsec") == 0) {
                    // Send special keypress sequence for Parsec restart
                    ESP_LOGI(TAG, "Restarting Parsec");
                    usb_request_restart_app_parsec(false);
                    success = true;
                    strcpy(message, "Parsec restart initiated");
                } 
                else if (strcmp(app_name, "anydesk") == 0) {
                    // Send special keypress sequence for Anydesk restart
                    ESP_LOGI(TAG, "Restarting Anydesk");
                    usb_request_restart_app_anydesk(false);
                    success = true;
                    strcpy(message, "Anydesk restart initiated");
                }
                else {
                    strcpy(message, "Unknown application");
                }
            }
        }
        free(buf);
    }
    
    // Prepare response
    char *resp;
    int rc = asprintf(&resp, "{\"success\": %s, \"message\": \"%s\"}", 
                     success ? "true" : "false", message);
    if (rc < 0 || !resp) {
        ESP_LOGE(TAG, "Failed to prepare response");
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));
    free(resp);
    
    return ESP_OK;
}

void httpd_init(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "starting server on port %d", config.server_port);
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "error starting server");
        return;
    }

    // Register the root handler for web UI
    static const httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = httpd_root_get_handler
    };
    httpd_register_uri_handler(server, &root);

    static const httpd_uri_t wakeup = {
        .uri = "/wakeup",
        .method = HTTP_GET,
        .handler = httpd_wakeup_get_handler
    };
    httpd_register_uri_handler(server, &wakeup);
    
    // Register handler for WoL
    static const httpd_uri_t wol = {
        .uri = "/wol",
        .method = HTTP_GET,
        .handler = httpd_wol_get_handler,
    };
    httpd_register_uri_handler(server, &wol);

    static const httpd_uri_t pc_restart = {
        .uri = "/restart",
        .method = HTTP_GET,
        .handler = httpd_pc_restart_get_handler
    };
    httpd_register_uri_handler(server, &pc_restart);
    
    // Register handler for PC shutdown
    static const httpd_uri_t pc_shutdown = {
        .uri = "/shutdown",
        .method = HTTP_GET,
        .handler = httpd_pc_shutdown_get_handler
    };
    httpd_register_uri_handler(server, &pc_shutdown);

    // Register handler for ESP restart
    static const httpd_uri_t esp_restart = {
        .uri = "/esp-restart",
        .method = HTTP_GET,
        .handler = httpd_esp_restart_get_handler
    };
    httpd_register_uri_handler(server, &esp_restart);

    // Register handler for app restarts
    static const httpd_uri_t restart_app = {
        .uri = "/restart-app",
        .method = HTTP_GET,
        .handler = httpd_restart_app_get_handler
    };
    httpd_register_uri_handler(server, &restart_app);

    ESP_LOGI(TAG, "server started");
}
