#include <stdio.h>
#include "pico/stdlib.h"

#include "mfrc522.h"
#include "buzzer.h"
#include "ssd1306.h"
#include "menu.h"
#include "thingspeak.h"

#include "hardware/rtc.h"
#include "hardware/adc.h"
#include "pico/util/datetime.h"
#include "pico/cyw43_arch.h"
#include "hardware/timer.h"

#define LED_RED 13
#define LED_GREEN 11
#define BTN_PIN 5
#define TEMPERATURE_SENSOR 4

// Variáveis para o callback do timer
static volatile bool tempo_esgotado = false;
static volatile bool atualizar_leitura = false;
static volatile bool exit_acess = false;

volatile uint alarm_count = 0;

// Variáveis para leitura dos sensores
uint nivel_reservatorio = 0;
uint temperatura = 0;

// Protótipo de funções
int check_autentication(MFRC522Ptr_t mfrc, uint8_t *tag1, ssd1306_t *ssd_bm, datetime_t *t, datetime_t *alarm_time);
static void alarm_callback(void);
int read_onboard_temperature(void);
int read_tank_level(void);
void init_wifi(void);
void gpio_callback(uint gpio, uint32_t events);

void main() {
    stdio_init_all();

    gpio_init(LED_RED);
    gpio_init(LED_GREEN);

    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_set_dir(LED_GREEN, GPIO_OUT);

    pwm_init_buzzer(BUZZER_PIN);

    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_up(BTN_PIN);

    // Sensor de temperatura do raspberry pi pico w
    adc_init();
    adc_set_temp_sensor_enabled(true);

    // Joystick (simula o nivel da agua)
    adc_gpio_init(26);

    // Start the RTC
    rtc_init();

    // Define o tempo inicial como 00:00:00
    datetime_t t = {
        .year  = 2025,  // Ano qualquer, já que só vamos contar segundos
        .month = 1,
        .day   = 1,
        .dotw  = 0, // Dia da semana (opcional)
        .hour  = 0,
        .min   = 0,
        .sec   = 0
    };

    // Define o alarme para 25 segundos depois
    datetime_t alarm_time = t;
    alarm_time.sec += 5;

     // Inicializa o display OLED
    uint8_t ssd[ssd1306_buffer_length];
    struct render_area update_area;
    init_display(ssd, &update_area, false);

    // Inicializa o bitmap do display OLED
    ssd1306_t ssd_bm;
    ssd1306_init_bm(&ssd_bm, 128, 64, true, 0x3C, i2c1);
    ssd1306_config(&ssd_bm);
    ssd1306_draw_bitmap(&ssd_bm, connect_wifi);
    
    // Configura a área específica para atualização
    update_area.start_column = 88;
    update_area.end_column = 110;
    update_area.start_page = 2;
    update_area.end_page = ssd1306_n_pages - 1;
    calculate_render_area_buffer_length(&update_area);

    // Tag que está credenciada
    uint8_t tag1[] = {0xD3, 0x1C, 0xA9, 0xFA};
    // Módulo MFRC22
    MFRC522Ptr_t mfrc = MFRC522_Init();

    // Se conecta ao Wi-Fi
    init_wifi();

    // Flag para verificar autenticação
    bool autenticado = false;

    // Progama a interrupção do botão
    gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    while(true) {
        cyw43_arch_poll();  // Necessário para manter o Wi-Fi ativo

        // Desenha o menu principal
        ssd1306_config(&ssd_bm);
        ssd1306_draw_bitmap(&ssd_bm, reservatorio);
        // Checa a autenticação
        autenticado = check_autentication(mfrc, tag1, &ssd_bm, &t, &alarm_time);

        // Se a autenticação foi bem sucedida
        if (autenticado) {
            gpio_put(LED_RED, 0);
            gpio_put(LED_GREEN, 0);

            // Inicia o alarme de 25 s
            rtc_set_datetime(&t);
            rtc_set_alarm(&alarm_time, alarm_callback);
            alarm_count = 0;
            tempo_esgotado = 0;

            // Menu de informações dos sensores
            ssd1306_draw_bitmap(&ssd_bm, info);
            init_display(ssd, &update_area, true);

            // Buffers para os valores do reservatório e temperatura
            char reservatorio[4];
            char temp[4];
            while (!tempo_esgotado) {
                // ler e enviar os dados dos sensores cada 5 segundos
                if (atualizar_leitura) {
                    temperatura = read_onboard_temperature();
                    nivel_reservatorio = read_tank_level();

                    // Atualiza no OLED os valores lidos
                    memset(ssd, 0, ssd1306_buffer_length);
                    snprintf(reservatorio, sizeof(reservatorio), "%d", nivel_reservatorio);
                    ssd1306_draw_string(ssd, 23, 0, reservatorio);
                    snprintf(temp, sizeof(temp), "%d", temperatura);
                    ssd1306_draw_string(ssd, 70, 0, temp);
                    render_on_display(ssd, &update_area);

                    // Envia os dados para o ThingSpeak
                    send_data_to_thingspeak(nivel_reservatorio, "field1");
                    send_data_to_thingspeak(temperatura, "field2");

                    atualizar_leitura = false;
                }

                if (exit_acess) {
                    break;
                }
            }
            tempo_esgotado = false;
            exit_acess = false;
        }
    }
}

// Checa a autenticação do usuário, pelo módulo RFID
int check_autentication(MFRC522Ptr_t mfrc, uint8_t *tag1, ssd1306_t *ssd_bm, datetime_t *t, datetime_t *alarm_time) {
    // Inicializa o módulo RFID
    PCD_Init(mfrc, spi0);
    // Flag para sair do modo alerta
    bool sair_alerta = false;

    // Inicia o alarme de 25 s
    rtc_set_datetime(t);
    rtc_set_alarm(alarm_time, alarm_callback);

    // Espera a leitura de um cartão
    while(!PICC_IsNewCardPresent(mfrc)){
        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 1);

        // Se o timer der 5 s, le os sensores 
        if (atualizar_leitura) {
            temperatura = read_onboard_temperature();
            nivel_reservatorio = read_tank_level();
            // Caso o nivel do reservatorio estiver abaixo de 15%, modo de alerta
            if (nivel_reservatorio < 15) {
                ssd1306_draw_bitmap(ssd_bm, alerta);
                sair_alerta = false;
                PCD_Init(mfrc, spi0);
            }
            else if (!sair_alerta) {
                ssd1306_draw_bitmap(ssd_bm, reservatorio);
                sair_alerta = true;
                PCD_Init(mfrc, spi0);
            }
            atualizar_leitura = false;
        }
        if (tempo_esgotado) {
            // Envia os dados para o ThingSpeak a cada 25 segundos
            send_data_to_thingspeak(nivel_reservatorio, "field1");
            send_data_to_thingspeak(temperatura, "field2");
            
            // Reinicia o timer de 25 segundos
            rtc_set_datetime(t);
            rtc_set_alarm(alarm_time, alarm_callback);
            tempo_esgotado = false;
        }

    }
    // Le o valor do cartão
    PICC_ReadCardSerial(mfrc);

    // Print do número do cartão
    printf("Uid is: ");
    for(int i = 0; i < 4; i++) {
        printf("%x ", mfrc->uid.uidByte[i]);
    } printf("\n\r");

    // Verifica se o cartão está credenciado
    if(memcmp(mfrc->uid.uidByte, tag1, 4) == 0) {
        printf("Authentication Success\n\r");
        printf("\n");

        // Envia qual foi o usuário que entrou para o ThingSpeak
        send_data_to_thingspeak(mfrc->uid.uidByte[0], "field3");

        // Exibe a confirmação no OLED, acende LED e ativa buzzer
        gpio_put(LED_RED, 0);
        gpio_put(LED_GREEN, 1);
        
        ssd1306_draw_bitmap(ssd_bm, reservatorio_ok);
    
        play_tone(BUZZER_PIN, 900, 100);
        sleep_ms(50);
        play_tone(BUZZER_PIN, 900, 100);

        sleep_ms(2000);

        return 1;
    } else {
        printf("Authentication Failed\n\r");
        printf("\n");

        // Exibe a confirmação no OLED, acende LED e ativa buzzer
        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 0);
        
        ssd1306_draw_bitmap(ssd_bm, reservatorio_fail);

        play_tone(BUZZER_PIN, 400, 300);

        sleep_ms(2000);

        return 0;
    }
}

// Função de callback para o alarme
static void alarm_callback(void) {
    datetime_t t = {0};

    rtc_get_datetime(&t);
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];

    datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    printf("Alarm tempo_esgotado At %s\n", datetime_str);
    stdio_flush();

    // incrementa a cada 5 segundos
    alarm_count++;
    // flag que habilita a leitura dos sensores
    atualizar_leitura = true;
    // Se o alarme foi acionado 5 vezes, o tempo esgotou
    if (alarm_count >= 5) {
        tempo_esgotado = true;
        alarm_count = 0;
    }
    else {
        // redefine um novo alarme para 5 segundos depois
        datetime_t alarm_time = t;
        alarm_time.sec += 5;
        rtc_set_alarm(&alarm_time, alarm_callback);
    }
}

// Faz a leitura do sensor de temperatura interno da placa
int read_onboard_temperature(void) {
    adc_select_input(TEMPERATURE_SENSOR);
    const float conversionFactor = 3.3f / (1 << 12);

    float adc = (float)adc_read() * conversionFactor;
    float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

    return (uint)tempC;
}

// Faz a leitura do sensor de nivel do tank(joystick)
int read_tank_level(void) {
    adc_select_input(0);

    int level = adc_read();
    level = level * 100 / 4095;

    if (level < 15) {
        gpio_put(LED_GREEN, 0);
        gpio_put(LED_RED, 1);
        play_tone(BUZZER_PIN, 400, 300);
        gpio_put(LED_RED, 0);

    }
    return level;
}

void init_wifi(void) {
    // Inicializa Wi-Fi
    if (cyw43_arch_init()) {
        printf("Falha ao iniciar Wi-Fi\n");
    }
    cyw43_arch_enable_sta_mode();

    // Conectar ao Wi-Fi
    printf("Conectando ao Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
    }
    printf("Wi-Fi conectado!\n");
}

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == BTN_PIN) {
        exit_acess = true;
    }
}