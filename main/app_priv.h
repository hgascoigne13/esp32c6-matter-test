/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include <string.h>
#include <i2cdev.h>
#include <bh1750.h>
#include <esp_matter_core.h>
#include <app_reset.h>
#include <driver/gpio.h>

#include <esp_err.h>
#include <esp_matter.h>

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include "esp_openthread_types.h"
#endif

/** Standard max values (used for remapping attributes) */
#define STANDARD_BRIGHTNESS 100
#define STANDARD_HUE 360
#define STANDARD_SATURATION 100
#define STANDARD_TEMPERATURE_FACTOR 1000000

/** Matter max values (used for remapping attributes) */
#define MATTER_BRIGHTNESS 254
#define MATTER_HUE 254
#define MATTER_SATURATION 254
#define MATTER_TEMPERATURE_FACTOR 1000000

/** Default attribute values used during initialization */
#define DEFAULT_POWER true
#define DEFAULT_BRIGHTNESS 64
#define DEFAULT_HUE 128
#define DEFAULT_SATURATION 254

/** BH1750 settings **/
using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace esp_matter::cluster;

#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif

#define BH1750_ADDR BH1750_ADDR_LO
#define TEST_LED_GPIO GPIO_NUM_13

#define CONFIG_EXAMPLE_I2C_MASTER_SDA 6
#define CONFIG_EXAMPLE_I2C_MASTER_SCL 7

typedef void *app_driver_handle_t;

/** Initialize the light driver
 *
 * This initializes the light driver associated with the selected board.
 *
 * @return Handle on success.
 * @return NULL in case of failure.
 */
app_driver_handle_t app_driver_light_init();

/** Initialize the button driver
 *
 * This initializes the button driver associated with the selected board.
 *
 * @return Handle on success.
 * @return NULL in case of failure.
 */
app_driver_handle_t app_driver_button_init();

/**
 * @brief Holds information related to a particular functionality, such as a sensor or a switch
*/
typedef struct {
   std::string name; /**< The name of the functionality, represented as a std::string*/
   endpoint_t * endpoint_p; /**< A pointer to the endpoint_t type object*/
   uint16_t endpoint_id; /**< The id of the endpoint, represented as an uint16_t*/
   cluster_t * cluster_p; /**< A pointer to the cluster_t type object*/
   uint16_t cluster_id; /**< The id of the cluster, represented as an uint16_t*/
   uint32_t attribute_id; /**< The id of the attribute, represented as an uint32_t*/
} matter_config_t;

/**
 * @struct Matter Attribute Values for SCD30 Readings Type
 * 
 * @brief Holds information related to the scd30 as matter attribute values
 * 
 * @var data_ready (bool) 
 * Indicates if the data is ready to be read
 * @var co2_reading (esp_matter_attr_val_t) 
 * CO2 data, which should be in ppm
 * @var temperature_reading (esp_matter_attr_val_t) 
 * Temperature reading, which should be in Celsius
 * @var humidity_reading (esp_matter_attr_val_t) 
 * Humidity reading, which should be a percent
*/
typedef struct {
   bool data_ready;
   esp_matter_attr_val_t co2_reading;
   esp_matter_attr_val_t temperature_reading;
   esp_matter_attr_val_t humidity_reading;
} matter_attr_val_scd30_reading_t; 

void bh1750_sensor_init(i2c_dev_t * bh1750_dev_descriptor);
esp_matter_attr_val_t bh1750_sensor_update(i2c_dev_t * bh1750_dev_descriptor);

void scd30_sensor_init(i2c_dev_t * scd30_dev_descriptor);
matter_attr_val_scd30_reading_t scd30_sensor_update(i2c_dev_t * scd30_dev_descriptor);

/** Driver Update
 *
 * This API should be called to update the driver for the attribute being updated.
 * This is usually called from the common `app_attribute_update_cb()`.
 *
 * @param[in] endpoint_id Endpoint ID of the attribute.
 * @param[in] cluster_id Cluster ID of the attribute.
 * @param[in] attribute_id Attribute ID of the attribute.
 * @param[in] val Pointer to `esp_matter_attr_val_t`. Use appropriate elements as per the value type.
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t app_driver_attribute_update(app_driver_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                      uint32_t attribute_id, esp_matter_attr_val_t *val);


/* Set the attribute drivers to their default values from the created data model.
 *
 * @param[in] endpoint_id Endpoint ID of the driver.
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t app_driver_light_set_defaults(uint16_t endpoint_id);

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#define ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG()                                           \
    {                                                                                   \
        .radio_mode = RADIO_MODE_NATIVE,                                                \
    }

#define ESP_OPENTHREAD_DEFAULT_HOST_CONFIG()                                            \
    {                                                                                   \
        .host_connection_mode = HOST_CONNECTION_MODE_NONE,                              \
    }

#define ESP_OPENTHREAD_DEFAULT_PORT_CONFIG()                                            \
    {                                                                                   \
        .storage_partition_name = "nvs", .netif_queue_size = 10, .task_queue_size = 10, \
    }
#endif
