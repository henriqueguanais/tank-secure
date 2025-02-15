#ifndef THINGSPEAK_H
#define THINGSPEAK_H

#include "lwip/tcp.h"
#include "settings.h"

err_t http_request_callback(void *arg, struct tcp_pcb *tpcb, err_t err);
void dns_callback(const char *name, const ip_addr_t *ipaddr, void *arg);
void send_data_to_thingspeak(float value, const char *thingspeak_field);


#endif