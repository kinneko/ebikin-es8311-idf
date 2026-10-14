// original implementation by ebikin
// SPDX-License-Identifier: GPL-3.0-only
#ifndef EBikin_ES8311_H
#define EBikin_ES8311_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ebikin_es8311 ebikin_es8311_t;

typedef struct {
    i2c_port_t port;          // e.g. I2C_NUM_0
    int sda_gpio;             // actual GPIO; no implicit board defaults
    int scl_gpio;
    uint8_t address;          // 7-bit address, normally 0x18
    uint32_t bus_hz;          // normally 100000
    bool internal_pullups;    // external pullups are recommended
} ebikin_es8311_config_t;

typedef struct {
    uint8_t manufacturer;     // register 0xfd (expected 0x83)
    uint8_t chip;             // register 0xfe (expected 0x11)
    uint8_t version;          // register 0xff (reported even if ID mismatches)
} ebikin_es8311_id_t;

// Allocates one i2cdev descriptor. Calls i2cdev_init (idempotent in pinned 2.1.2).
// Does not reset/configure hardware. *out must be NULL; on failure remains NULL.
// Do not call concurrently with operations on this handle.
esp_err_t ebikin_es8311_init(const ebikin_es8311_config_t *config, ebikin_es8311_t **out);

// Reads all three ID registers; returns ESP_ERR_NOT_FOUND for non-ES8311 IDs.
// On successful reads *id contains actual values even on ID mismatch.
esp_err_t ebikin_es8311_probe(ebikin_es8311_t *codec, ebikin_es8311_id_t *id);

// Read a single register (e.g. 0x0a for the LifeLog format diagnostic).
esp_err_t ebikin_es8311_read_register(ebikin_es8311_t *codec, uint8_t reg, uint8_t *value);

// Fixed LifeLog 16 kHz, 32-bit stereo wire-slot, BCLK-derived slave sequence.
// Caller MUST start I2S BCLK/WS before this call and keep clocks active throughout.
// Includes reset 0x00=0x1f, 20ms delay, then exact register sequence.
// Does not probe, own I2S, or perform application warmup/drain.
esp_err_t ebikin_es8311_configure(ebikin_es8311_t *codec);

// stop_mute writes 0x31=0x60, matching LifeLog stop; caller stops I2S afterward.
esp_err_t ebikin_es8311_stop_mute(ebikin_es8311_t *codec);

// Set DAC mute register 0x31 to 0x60 (muted) or 0x00 (unmuted).
// Caller must control I2S and volume; unmuting may output sound.
esp_err_t ebikin_es8311_set_dac_mute(ebikin_es8311_t *codec, bool muted);

// Optional raw DAC volume register 0x32 (0..255); no implicit volume write in configure.
esp_err_t ebikin_es8311_set_dac_volume(ebikin_es8311_t *codec, uint8_t value);

// Releases only this device; never calls global i2cdev_done (RTC may share I2C0).
// NULL *codec is an idempotent no-op. Do not race active I/O or call while other
// tasks use the handle. Does not mute or stop I2S; call stop_mute beforehand.
esp_err_t ebikin_es8311_deinit(ebikin_es8311_t **codec);

#ifdef __cplusplus
}
#endif
#endif
