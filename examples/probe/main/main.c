// original implementation by ebikin
// SPDX-License-Identifier: GPL-3.0-only
#include <stdio.h>
#include "ebikin_es8311.h"

// Probe-only example. Does not start I2S, so MUST NOT call configure here.
// Hardware pins below are for Atomic ECHO BASE with AtomS3; adapt for your board.
void app_main(void)
{
    ebikin_es8311_t *codec = NULL;
    const ebikin_es8311_config_t config = {
        .port = I2C_NUM_0, .sda_gpio = 38, .scl_gpio = 39,
        .address = 0x18, .bus_hz = 100000, .internal_pullups = true
    };
    esp_err_t err = ebikin_es8311_init(&config, &codec);
    if (err == ESP_OK) {
        ebikin_es8311_id_t id = {0};
        err = ebikin_es8311_probe(codec, &id);
        printf("ES8311 probe: %s; FD=%02x FE=%02x FF=%02x\n",
               esp_err_to_name(err), id.manufacturer, id.chip, id.version);
        esp_err_t cleanup = ebikin_es8311_deinit(&codec);
        if (cleanup != ESP_OK) printf("ES8311 cleanup: %s\n", esp_err_to_name(cleanup));
    } else printf("ES8311 init: %s\n", esp_err_to_name(err));
}
