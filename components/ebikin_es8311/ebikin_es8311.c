// original implementation by ebikin
// SPDX-License-Identifier: GPL-3.0-only
#include "ebikin_es8311.h"
#include "i2cdev.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>

struct ebikin_es8311 { i2c_dev_t dev; };

static esp_err_t write_register(ebikin_es8311_t *codec, uint8_t reg, uint8_t value)
{
    return i2c_dev_write_reg(&codec->dev, reg, &value, 1);
}

esp_err_t ebikin_es8311_init(const ebikin_es8311_config_t *config, ebikin_es8311_t **out)
{
    if (!config || !out || *out || config->port < 0 || config->port >= I2C_NUM_MAX ||
        config->sda_gpio < 0 || config->scl_gpio < 0 || config->sda_gpio == config->scl_gpio ||
        config->address < 0x08 || config->address > 0x77 || !config->bus_hz)
        return ESP_ERR_INVALID_ARG;
    ebikin_es8311_t *codec = calloc(1, sizeof(*codec));
    if (!codec) return ESP_ERR_NO_MEM;
    codec->dev.port = config->port;
    codec->dev.addr = config->address;
    codec->dev.cfg.sda_io_num = config->sda_gpio;
    codec->dev.cfg.scl_io_num = config->scl_gpio;
    codec->dev.cfg.sda_pullup_en = config->internal_pullups;
    codec->dev.cfg.scl_pullup_en = config->internal_pullups;
    codec->dev.cfg.master.clk_speed = config->bus_hz;
    esp_err_t err = i2cdev_init();
    if (err == ESP_OK) err = i2c_dev_create_mutex(&codec->dev);
    if (err != ESP_OK) {
        if (codec->dev.mutex) i2c_dev_delete_mutex(&codec->dev);
        free(codec);
        return err;
    }
    *out = codec;
    return ESP_OK;
}

esp_err_t ebikin_es8311_probe(ebikin_es8311_t *codec, ebikin_es8311_id_t *id)
{
    if (!codec || !id) return ESP_ERR_INVALID_ARG;
    esp_err_t err = i2c_dev_read_reg(&codec->dev, 0xfd, &id->manufacturer, 1);
    if (err != ESP_OK) return err;
    err = i2c_dev_read_reg(&codec->dev, 0xfe, &id->chip, 1);
    if (err != ESP_OK) return err;
    err = i2c_dev_read_reg(&codec->dev, 0xff, &id->version, 1);
    if (err != ESP_OK) return err;
    return id->manufacturer == 0x83 && id->chip == 0x11 ? ESP_OK : ESP_ERR_NOT_FOUND;
}

esp_err_t ebikin_es8311_read_register(ebikin_es8311_t *codec, uint8_t reg, uint8_t *value)
{
    if (!codec || !value) return ESP_ERR_INVALID_ARG;
    return i2c_dev_read_reg(&codec->dev, reg, value, 1);
}

esp_err_t ebikin_es8311_configure(ebikin_es8311_t *codec)
{
    if (!codec) return ESP_ERR_INVALID_ARG;
    // Original LifeLog audio_capture.c:111-114, including duplicate writes to 0x00.
    esp_err_t err = write_register(codec, 0x00, 0x1f);
    if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(20));
    static const uint8_t settings[][2] = {
        {0x00,0},{0x00,0x80},{0x01,0xbf},{0x02,0x10},{0x03,0x10},
        {0x04,0x10},{0x05,0},{0x06,3},{0x07,0},{0x08,0xff},
        {0x09,0x10},{0x0a,0x10},{0x0d,1},{0x0e,2},{0x12,0},
        {0x13,0x10},{0x1c,0x6a},{0x37,8},{0x17,0xff},{0x14,0x1a},
        {0x16,0},{0x31,0x60}
    };
    for (size_t i = 0; i < sizeof(settings) / sizeof(settings[0]); ++i) {
        err = write_register(codec, settings[i][0], settings[i][1]);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

esp_err_t ebikin_es8311_stop_mute(ebikin_es8311_t *codec)
{
    return codec ? write_register(codec, 0x31, 0x60) : ESP_ERR_INVALID_ARG;
}

esp_err_t ebikin_es8311_set_dac_mute(ebikin_es8311_t *codec, bool muted)
{
    return codec ? write_register(codec, 0x31, muted ? 0x60 : 0x00) : ESP_ERR_INVALID_ARG;
}

esp_err_t ebikin_es8311_set_dac_volume(ebikin_es8311_t *codec, uint8_t value)
{
    return codec ? write_register(codec, 0x32, value) : ESP_ERR_INVALID_ARG;
}

esp_err_t ebikin_es8311_deinit(ebikin_es8311_t **codec)
{
    if (!codec) return ESP_ERR_INVALID_ARG;
    if (!*codec) return ESP_OK;
    esp_err_t err = i2c_dev_delete_mutex(&(*codec)->dev);
    if (err != ESP_OK) return err;
    free(*codec);
    *codec = NULL;
    return ESP_OK;
}
