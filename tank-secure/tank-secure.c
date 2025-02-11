#include <stdio.h>
#include "pico/stdlib.h"

#include "mfrc522.h"
#include "buzzer.h"
#include "ssd1306.h"
#include "menu.h"

#define LED_RED 13
#define LED_GREEN 11

int check_autentication(MFRC522Ptr_t mfrc, uint8_t *tag1, ssd1306_t *ssd_bm);

void main() {
    stdio_init_all();

    gpio_init(LED_RED);
    gpio_init(LED_GREEN);

    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_set_dir(LED_GREEN, GPIO_OUT);

    pwm_init_buzzer(BUZZER_PIN);

     // Inicializa o display OLED
    uint8_t ssd[ssd1306_buffer_length];
    struct render_area frame_area, update_area;
    init_display(ssd, &update_area);

    // Inicializa o bitmap do display OLED
    ssd1306_t ssd_bm;
    ssd1306_init_bm(&ssd_bm, 128, 64, true, 0x3C, i2c1);
    ssd1306_config(&ssd_bm);

    // Configura a área específica para atualização
    update_area.start_column = 0;
    update_area.end_column = ssd1306_width - 1;
    update_area.start_page = 2;
    update_area.end_page = ssd1306_n_pages - 1;
    calculate_render_area_buffer_length(&update_area);

    // Declare card UID's
    uint8_t tag1[] = {0xD3, 0x1C, 0xA9, 0xFA};

    MFRC522Ptr_t mfrc = MFRC522_Init();
    PCD_Init(mfrc, spi0);

    bool autenticado = false;
    char *text[] = {"Teste"};

    while(true) {
        // Desenha o menu principal
        ssd1306_draw_bitmap(&ssd_bm, reservatorio);
        // Checa a autenticação
        autenticado = check_autentication(mfrc, tag1, &ssd_bm);

        if (autenticado) {
            gpio_put(LED_RED, 0);
            gpio_put(LED_GREEN, 0);

            uint8_t ssd[ssd1306_buffer_length];
            struct render_area update_area;
            init_display(ssd, &update_area);

            while (true) {
                memset(ssd, 0, ssd1306_buffer_length);
                ssd1306_draw_string(ssd, 48, 20, "OK");
                render_on_display(ssd, &update_area);
            }
        }


    }
}

int check_autentication(MFRC522Ptr_t mfrc, uint8_t *tag1, ssd1306_t *ssd_bm) {
    printf("Waiting for card\n\r");

        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 1);

        while(!PICC_IsNewCardPresent(mfrc));
        //Select the card
        printf("Selecting card\n\r");
        PICC_ReadCardSerial(mfrc);

        //Authorization with uid
        printf("Uid is: ");
        for(int i = 0; i < 4; i++) {
            printf("%x ", mfrc->uid.uidByte[i]);
        } printf("\n\r");

    if(memcmp(mfrc->uid.uidByte, tag1, 4) == 0) {
        printf("Authentication Success\n\r");
        printf("\n");

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

        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 0);

        ssd1306_draw_bitmap(ssd_bm, reservatorio_fail);

        play_tone(BUZZER_PIN, 400, 300);

        sleep_ms(2000);

        return 0;
    }
}

