/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"

const int PIN_TRIGGER = 17;
const int PIN_ECHO = 18;
const int TIMEOUT_US = 30000;

volatile int tempo_subida = 0;
volatile int tempo_descida = 0;
volatile bool alarme_disparado = false;

int64_t alarm_callback(alarm_id_t id, void *user_data)
{
    alarme_disparado = true;
    return 0;
}

void gpio_callback(uint gpio, uint32_t events)
{
    if (events == 0x4)
    {
        //o ultrassom desce no 0x4, é o contrario do botão
        tempo_descida = to_us_since_boot(get_absolute_time());
    }
    else if (events == 0x8)
    {
        tempo_subida = to_us_since_boot(get_absolute_time());
    }
}

void pin_init(void)
{
    gpio_init(PIN_ECHO);
    gpio_set_dir(PIN_ECHO, GPIO_IN);
    gpio_init(PIN_TRIGGER);
    gpio_set_dir(PIN_TRIGGER, GPIO_OUT);
}

int main()
{
    stdio_init_all();
    pin_init();

    gpio_set_irq_enabled_with_callback(PIN_ECHO, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    datetime_t t = {
        .year = 2025,
        .month = 03,
        .day = 19,
        .dotw = 3,
        .hour = 12,
        .min = 00,
        .sec = 00};

    rtc_init();
    rtc_set_datetime(&t);

    while (true)
    {
        int digitou = getchar_timeout_us(200);

        if (digitou == 'c')
        {
            while (1)
            {
                sleep_ms(100);
                alarme_disparado = false;
                tempo_descida = false;
                tempo_subida = false;

                gpio_put(PIN_TRIGGER, 1);
                sleep_us(10);
                gpio_put(PIN_TRIGGER, 0);

                alarm_id_t alarme = add_alarm_in_ms(50, alarm_callback, NULL, false);

                while (alarme_disparado == 0 && tempo_descida == false)
                {
                }

                if (alarme_disparado)
                {
                    printf("Falha! \n");
                    alarme_disparado = false;
                }
                else
                {
                    cancel_alarm(alarme);
                    int duracao = tempo_descida - tempo_subida;
                    float distancia = (duracao / 2.0) * 0.03403; // do video
                    rtc_get_datetime(&t);
                    char data_buffer[256];
                    char *data_str = &data_buffer[0];
                    datetime_to_str(data_str, sizeof(data_buffer), &t);
                    printf("%s - %.2f cm\n", data_str, distancia);
                }

                digitou = getchar_timeout_us(200);
                if (digitou == 'p')
                {
                    printf("Cancelando leitura...\n");
                    break;
                }
            }
        }
    }
}