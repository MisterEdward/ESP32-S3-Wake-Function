#include "ssh_client.h"

#include <esp_log.h>
#include <string.h>
#include <libssh2.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "ssh_client"

// Initialize SSH client subsystem
bool ssh_client_init(void) {
    int rc = libssh2_init(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to initialize libssh2 (%d)", rc);
        return false;
    }
    return true;
}

// Test SSH connection without executing a command
bool ssh_client_test_connection(const char *hostname, const char *username, const char *password) {
    int sock = -1;
    LIBSSH2_SESSION *session = NULL;
    bool success = false;
    struct sockaddr_in sin;
    
    ESP_LOGI(TAG, "Testing SSH connection to %s with user %s", hostname, username);
    
    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket: %d", errno);
        return false;
    }
    
    // Connect to server
    sin.sin_family = AF_INET;
    sin.sin_port = htons(22);
    sin.sin_addr.s_addr = inet_addr(hostname);
    
    if (connect(sock, (struct sockaddr*)(&sin), sizeof(sin)) != 0) {
        ESP_LOGE(TAG, "Socket connect failed: %d", errno);
        close(sock);
        return false;
    }
    
    // Create SSH session
    session = libssh2_session_init();
    if (!session) {
        ESP_LOGE(TAG, "Failed to initialize SSH session");
        close(sock);
        return false;
    }
    
    // Set blocking mode
    libssh2_session_set_blocking(session, 1);
    
    // Start SSH handshake
    ESP_LOGI(TAG, "Starting SSH handshake...");
    if (libssh2_session_handshake(session, sock) != 0) {
        ESP_LOGE(TAG, "SSH handshake failed");
        goto cleanup;
    }
    
    // Authenticate
    ESP_LOGI(TAG, "Authenticating...");
    
    // First try empty password if none provided
    const char *auth_pw = password && strlen(password) > 0 ? password : "";
    if (libssh2_userauth_password(session, username, auth_pw) == 0) {
        ESP_LOGI(TAG, "Authentication successful with %s password", 
                 strlen(auth_pw) > 0 ? "provided" : "empty");
        success = true;
        goto cleanup;
    }
    
    // If empty password failed and no password was provided, try with NULL
    if (!password || strlen(password) == 0) {
        if (libssh2_userauth_password(session, username, NULL) == 0) {
            ESP_LOGI(TAG, "Authentication successful with NULL password");
            success = true;
            goto cleanup;
        }
    }
    
    // Try keyboard-interactive as fallback
    if (!success) {
        if (libssh2_userauth_keyboard_interactive(session, username, NULL) == 0) {
            ESP_LOGI(TAG, "Authentication successful with keyboard-interactive");
            success = true;
            goto cleanup;
        }
    }
    
    // Try publickey authentication as last resort
    if (!success) {
        if (libssh2_userauth_publickey_fromfile(session, username, NULL, NULL, NULL) == 0) {
            ESP_LOGI(TAG, "Authentication successful with public key");
            success = true;
            goto cleanup;
        }
    }

    ESP_LOGE(TAG, "Authentication failed");
    
cleanup:
    if (session) {
        libssh2_session_disconnect(session, "Normal shutdown");
        libssh2_session_free(session);
    }
    if (sock != -1) {
        close(sock);
    }
    
    return success;
}

// Execute a command via SSH connection
bool ssh_execute_command(const char *hostname, const char *username, const char *password, const char *command) {
    int sock = -1;
    LIBSSH2_SESSION *session = NULL;
    LIBSSH2_CHANNEL *channel = NULL;
    bool success = false;
    struct sockaddr_in sin;
    char buffer[1024];
    
    ESP_LOGI(TAG, "Connecting to SSH server %s...", hostname);
    
    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket: %d", errno);
        return false;
    }
    
    // Connect to server
    sin.sin_family = AF_INET;
    sin.sin_port = htons(22);
    sin.sin_addr.s_addr = inet_addr(hostname);
    
    if (connect(sock, (struct sockaddr*)(&sin), sizeof(sin)) != 0) {
        ESP_LOGE(TAG, "Socket connect failed: %d", errno);
        close(sock);
        return false;
    }
    
    // Create SSH session
    session = libssh2_session_init();
    if (!session) {
        ESP_LOGE(TAG, "Failed to initialize SSH session");
        close(sock);
        return false;
    }
    
    // Set blocking mode
    libssh2_session_set_blocking(session, 1);
    
    // Start SSH handshake
    if (libssh2_session_handshake(session, sock) != 0) {
        ESP_LOGE(TAG, "SSH handshake failed");
        goto cleanup;
    }
    
    // Authenticate - first try empty password if none provided
    const char *auth_pw = password && strlen(password) > 0 ? password : "";
    if (libssh2_userauth_password(session, username, auth_pw) != 0) {
        // If that fails and no password was provided, try with NULL
        if ((!password || strlen(password) == 0) && 
            libssh2_userauth_password(session, username, NULL) != 0) {
            
            // Try keyboard interactive as fallback
            if (libssh2_userauth_keyboard_interactive(session, username, NULL) != 0) {
                ESP_LOGE(TAG, "SSH authentication failed");
                goto cleanup;
            }
        }
    }
    
    ESP_LOGI(TAG, "SSH authentication successful");
    
    // Open a channel for command execution
    channel = libssh2_channel_open_session(session);
    if (!channel) {
        ESP_LOGE(TAG, "Failed to open SSH channel");
        goto cleanup;
    }
    
    // Execute the command
    if (libssh2_channel_exec(channel, command) != 0) {
        ESP_LOGE(TAG, "Failed to execute command: %s", command);
        goto cleanup;
    }
    
    // Read command output
    int rc;
    do {
        rc = libssh2_channel_read(channel, buffer, sizeof(buffer) - 1);
        if (rc > 0) {
            buffer[rc] = 0;
            ESP_LOGI(TAG, "Command output: %s", buffer);
        }
    } while (rc > 0);
    
    // Get exit code
    int exit_code;
    libssh2_channel_get_exit_status(channel);
    libssh2_channel_get_exit_signal(channel, NULL, NULL, NULL, NULL, NULL, NULL);
    
    ESP_LOGI(TAG, "Command executed successfully: %s", command);
    success = true;
    
cleanup:
    if (channel) {
        libssh2_channel_close(channel);
        libssh2_channel_free(channel);
    }
    if (session) {
        libssh2_session_disconnect(session, "Normal shutdown");
        libssh2_session_free(session);
    }
    if (sock != -1) {
        close(sock);
    }
    
    return success;
}

// Send shutdown command via SSH
bool ssh_client_shutdown_pc(const char *hostname, const char *username, const char *password) {
    // Windows shutdown command
    const char *command = "shutdown /s /t 0";
    return ssh_execute_command(hostname, username, password, command);
}

// Send restart command via SSH
bool ssh_client_restart_pc(const char *hostname, const char *username, const char *password) {
    // Windows restart command
    const char *command = "shutdown /r /t 0";
    return ssh_execute_command(hostname, username, password, command);
}
