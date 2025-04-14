#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "ssd1306.h"
#include "gfx.h"

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

typedef struct adc {
    int axis;
    int val;
} adc_t;

QueueHandle_t xQueueADC;

void pin_init(){
    adc_init();
}

int media_movel(uint16_t novo_valor, int lista[5]) {
    static int indice = 0;
    int soma = 0;
    lista[indice] = novo_valor;
    indice = (indice + 1) % 5;
    for (int i = 0; i < 5; i++) {
        soma += lista[i];
    }
    return soma / 5;
}

int scale_adc_value(int adc_value) {
    int centered_value = adc_value - 2047;
    int scaled_value = (centered_value * 255) / 2047;
    if (scaled_value >= -30 && scaled_value <= 30) {
        scaled_value = 0;
    }
    if (scaled_value < -255) scaled_value = -255;
    if (scaled_value > 255) scaled_value = 255;
    return scaled_value;
}

void x_task(void *p) {
    pin_init();
    adc_gpio_init(26);
    int lista[5] = {0, 0, 0, 0, 0};
    
    while (1) {
        adc_select_input(0);
        uint16_t novo_valor = adc_read();
        int valor_movel = media_movel(novo_valor, lista);
        int valor_ajustado = scale_adc_value(valor_movel);
        
        adc_t data = {
            .axis = 0, 
            .val = valor_ajustado
        };
        
        if (data.val != 0) {
            xQueueSend(xQueueADC, &data, pdMS_TO_TICKS(100));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void y_task(void *p) {
    pin_init();
    adc_gpio_init(27);
    int lista[5] = {0, 0, 0, 0, 0};
    
    while (1) {
        adc_select_input(1);
        uint16_t novo_valor = adc_read();
        int valor_movel = media_movel(novo_valor, lista);
        int valor_ajustado = scale_adc_value(valor_movel);
        
        adc_t data = {
            .axis = 1, 
            .val = valor_ajustado
        };

        if (data.val != 0) {
            xQueueSend(xQueueADC, &data, pdMS_TO_TICKS(100));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void uart_task(void *p) {
    while (1) {
        adc_t data;
        if (xQueueReceive(xQueueADC, &data, pdMS_TO_TICKS(100))) {
            putchar_raw(0xFF);
            putchar_raw(data.axis);
            putchar_raw(data.val);
            putchar_raw(data.val >> 8);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main() {
    stdio_init_all();

    xQueueADC = xQueueCreate(10, sizeof(adc_t));
    xTaskCreate(x_task, "x_task", 4095, NULL, 1, NULL);
    xTaskCreate(y_task, "y_task", 4095, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart_task", 4095, NULL, 1, NULL);
    
    vTaskStartScheduler();

    while (true)
        ;
}
