#include <stdio.h>
#include <string.h>
#include "lwip/ip4_addr.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "thingspeak.h"
#include "pico/stdlib.h"

volatile bool dns_resolved = false;

// Callback para quando a requisição HTTP for concluída
err_t http_request_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    if (err == ERR_OK) {
        printf("Conectado ao servidor!\n");
    } else {
        printf("Erro na conexão: %d\n", err);
    }
    return ERR_OK;
}

// Callback para quando o DNS for resolvido
void dns_callback(const char *name, const ip_addr_t *ipaddr, void *arg) {
    if (ipaddr) {
        printf("DNS resolvido\n");
        dns_resolved = true;
    } else {
        printf("Falha na resolução do DNS\n");
        dns_resolved = false;
    }
}

// Função para enviar dados ao ThingSpeak
void send_data_to_thingspeak(float value, const char *thingspeak_field) {
    char request[256];
    snprintf(request, sizeof(request), "GET /update?api_key=%s&%s=%.2f HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
             THINGSPEAK_API_KEY, thingspeak_field, value, THINGSPEAK_URL);

    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB TCP\n");
        return;
    }

    ip_addr_t server_ip;
    err_t err = dns_gethostbyname(THINGSPEAK_URL, &server_ip, dns_callback, NULL);

    while (!dns_resolved) {
        sleep_ms(100);
    }

    if (dns_resolved) {
        err = tcp_connect(pcb, &server_ip, 80, http_request_callback);
        if (err == ERR_OK) {
            tcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
            tcp_output(pcb);
            printf("Enviando dado ao ThingSpeak: %.2f\n", value);
        }
    } else {
        printf("Erro ao resolver o DNS: %d\n", err);
    }
}