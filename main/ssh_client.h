#ifndef SSH_CLIENT_H_
#define SSH_CLIENT_H_

#include <stdbool.h>

// Initialize the SSH client subsystem
bool ssh_client_init(void);

// Test SSH connection (for checking credentials)
bool ssh_client_test_connection(const char *hostname, const char *username, const char *password);

// Send shutdown command via SSH
bool ssh_client_shutdown_pc(const char *hostname, const char *username, const char *password);

// Send restart command via SSH
bool ssh_client_restart_pc(const char *hostname, const char *username, const char *password);

#endif // SSH_CLIENT_H_
