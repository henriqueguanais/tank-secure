#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>

extern volatile bool no_wifi;

#define WIFI_RETRY_DELAY_MS 5000
#define WIFI_MAX_RETRIES 10

void init_wifi(void);
void connect_to_wifi(void);
void check_wifi_connection(void);

#endif