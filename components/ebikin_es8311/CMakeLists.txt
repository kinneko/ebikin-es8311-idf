# original implementation by ebikin
# SPDX-License-Identifier: GPL-3.0-only
idf_component_register(SRCS "ebikin_es8311.c" INCLUDE_DIRS "include" REQUIRES i2cdev esp_driver_i2c)
