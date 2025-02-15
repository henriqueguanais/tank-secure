#include "wifi.h"
#include <stdio.h>
#include "pico/cyw43_arch.h"
#include "settings.h"

volatile bool no_wifi = false;

// Inicializa o Wi-Fi
void init_wifi(void) {
    if (cyw43_arch_init()) {
        printf("Falha ao iniciar Wi-Fi\n");
        return;
    }
    cyw43_arch_enable_sta_mode();
    connect_to_wifi();
}

// Conecta ao Wi-Fi
void connect_to_wifi(void) {
    int attempts = 0;
    while (attempts < WIFI_MAX_RETRIES) {
        printf("Tentando se conectar ao Wi-Fi...\n");
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000) == 0) {
            printf("Wi-fi conectado!\n");
            return;
        }
        printf("Falha ao conectar. Tentando novamente em %d segundos...\n", WIFI_RETRY_DELAY_MS / 1000);
        sleep_ms(WIFI_RETRY_DELAY_MS);
        attempts++;
    }
    printf("Falha ao conectar ao Wi-Fi\n");
}

// Verifica a conexão Wi-Fi
void check_wifi_connection(void) {
    printf("Thread de verificação de conexão Wi-Fi iniciada\n");
    
    while (true) {
        printf("\nVerificando conexão Wi-Fi...\n");
        
        int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
        printf("Status: %d\n", status);

        if (status != CYW43_LINK_UP) {
            printf("Conexão Wi-Fi perdida. Tentando reconectar...\n");
            no_wifi = true;  // Atualiza a flag indicando que a conexão caiu
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); // Desliga o LED
            connect_to_wifi();
        } else {
            no_wifi = false; // Wi-Fi está ativo
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); // Liga o LED
        }

        sleep_ms(10000); // Verifica a conexão a cada 10 segundos
    }
}
