/*
 * Practica: Salidas analogicas en la placa ESP32-C6
 * Materia: Internet de las Cosas
 *
 * El programa retoma el ejercicio de entrada analogica visto en clase:
 * un potenciometro se lee mediante ADC1_CH4 (GPIO4) con resolucion
 * de 12 bits. La lectura obtenida (0 a 4095) se utiliza directamente
 * como ciclo de trabajo de una salida PWM de 12 bits en GPIO10 para
 * regular el brillo de un LED externo.
 *
 * Conexiones:
 *
 * Potenciometro:
 *   - Un extremo  -> 3V3
 *   - Terminal central (cursor) -> GPIO4 / ADC1_CH4
 *   - Otro extremo -> GND
 *
 * LED:
 *   - GPIO10 -> resistencia de 220 ohm -> anodo del LED
 *   - Catodo del LED -> GND
 */

#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/ledc.h"

/* Entrada analogica utilizada en el proyecto de clase.
 * En ESP32-C6, ADC1_CH4 corresponde a GPIO4.
 */
#define ADC_CHANNEL_USED       ADC_CHANNEL_4

/* Salida PWM para el LED externo. */
#define PWM_LED_GPIO           10
#define PWM_SPEED_MODE         LEDC_LOW_SPEED_MODE
#define PWM_TIMER              LEDC_TIMER_0
#define PWM_CHANNEL            LEDC_CHANNEL_0
#define PWM_FREQUENCY_HZ       5000
#define PWM_RESOLUTION         LEDC_TIMER_12_BIT

/* 12 bits -> valores desde 0 hasta 4095. */
#define ADC_MAX_VALUE          4095

/*
 * Configura ADC1 en modo oneshot y el canal utilizado por el
 * potenciometro.
 */
static void configurar_adc(adc_oneshot_unit_handle_t *adc_handle)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .clk_src = 0,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(&init_config, adc_handle)
    );

    adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            *adc_handle,
            ADC_CHANNEL_USED,
            &channel_config
        )
    );
}

/*
 * Configura el periferico LEDC para generar una senal PWM en GPIO10.
 * Se usa resolucion de 12 bits para que el valor ADC pueda utilizarse
 * directamente como duty.
 */
static void configurar_pwm(void)
{
    ledc_timer_config_t timer_config = {
        .speed_mode = PWM_SPEED_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num = PWM_TIMER,
        .freq_hz = PWM_FREQUENCY_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };

    ESP_ERROR_CHECK(
        ledc_timer_config(&timer_config)
    );

    ledc_channel_config_t channel_config = {
        .gpio_num = PWM_LED_GPIO,
        .speed_mode = PWM_SPEED_MODE,
        .channel = PWM_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = PWM_TIMER,
        .duty = 0,
        .hpoint = 0,
    };

    ESP_ERROR_CHECK(
        ledc_channel_config(&channel_config)
    );
}

void app_main(void)
{
    adc_oneshot_unit_handle_t adc1_handle;
    int raw_value = 0;

    configurar_adc(&adc1_handle);
    configurar_pwm();

    printf("\nPractica ADC -> PWM iniciada\n");
    printf("Entrada: GPIO4 / ADC1_CH4 / 12 bits\n");
    printf("Salida: GPIO10 / PWM LEDC / 12 bits\n\n");

    while (1) {
        /*
         * Leer el valor del potenciometro.
         * El resultado esperado esta entre 0 y 4095.
         */
        ESP_ERROR_CHECK(
            adc_oneshot_read(
                adc1_handle,
                ADC_CHANNEL_USED,
                &raw_value
            )
        );

        /*
         * Limitar el valor al rango de 12 bits antes de aplicarlo
         * como duty PWM.
         */
        if (raw_value < 0) {
            raw_value = 0;
        } else if (raw_value > ADC_MAX_VALUE) {
            raw_value = ADC_MAX_VALUE;
        }

        /*
         * ADC y PWM tienen la misma resolucion, por lo que el mapeo
         * es directo: 0 -> LED apagado, 4095 -> brillo maximo.
         */
        uint32_t duty = (uint32_t)raw_value;

        ESP_ERROR_CHECK(
            ledc_set_duty(
                PWM_SPEED_MODE,
                PWM_CHANNEL,
                duty
            )
        );

        ESP_ERROR_CHECK(
            ledc_update_duty(
                PWM_SPEED_MODE,
                PWM_CHANNEL
            )
        );

        float porcentaje = (duty * 100.0f) / ADC_MAX_VALUE;

        printf(
            "ADC: %4d / 4095 | PWM: %4lu / 4095 | Brillo: %6.2f %%\n",
            raw_value,
            (unsigned long)duty,
            porcentaje
        );

        /*
         * Un periodo corto permite observar un cambio suave de brillo
         * al girar el potenciometro.
         */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
