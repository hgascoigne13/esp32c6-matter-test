/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <inttypes.h>

#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_ota.h>

#include <app_priv.h>
#include <app_reset.h>


#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/ESP32/OpenthreadLauncher.h>
#endif

#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>

#include <stdio.h>
#include <string>

#include <i2cdev.h>
#include <bh1750.h>
#include <scd30.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include "esp_openthread.h"
#include "esp_openthread_types.h"
#include <openthread/tasklet.h>
#include <openthread/thread.h>
#endif

static const char *TAG = "app_main";
uint16_t light_endpoint_id = 0;

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

constexpr auto k_timeout_seconds = 300;

#if CONFIG_ENABLE_ENCRYPTED_OTA
extern const char decryption_key_start[] asm("_binary_esp_image_encryption_key_pem_start");
extern const char decryption_key_end[] asm("_binary_esp_image_encryption_key_pem_end");

static const char *s_decryption_key = decryption_key_start;
static const uint16_t s_decryption_key_len = decryption_key_end - decryption_key_start;
#endif // CONFIG_ENABLE_ENCRYPTED_OTA

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
        ESP_LOGI(TAG, "Interface IP Address changed");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        break;

    case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGI(TAG, "Commissioning failed, fail safe timer expired");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
        ESP_LOGI(TAG, "Commissioning session started");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped:
        ESP_LOGI(TAG, "Commissioning session stopped");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
        ESP_LOGI(TAG, "Commissioning window opened");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed:
        ESP_LOGI(TAG, "Commissioning window closed");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
        {
            ESP_LOGI(TAG, "Fabric removed successfully");
            if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0)
            {
                chip::CommissioningWindowManager & commissionMgr = chip::Server::GetInstance().GetCommissioningWindowManager();
                constexpr auto kTimeoutSeconds = chip::System::Clock::Seconds16(k_timeout_seconds);
                if (!commissionMgr.IsCommissioningWindowOpen())
                {
                    /* After removing last fabric, this example does not remove the Wi-Fi credentials
                     * and still has IP connectivity so, only advertising on DNS-SD.
                     */
                    CHIP_ERROR err = commissionMgr.OpenBasicCommissioningWindow(kTimeoutSeconds,
                                                    chip::CommissioningWindowAdvertisement::kDnssdOnly);
                    if (err != CHIP_NO_ERROR)
                    {
                        ESP_LOGE(TAG, "Failed to open commissioning window, err:%" CHIP_ERROR_FORMAT, err.Format());
                    }
                }
            }
        break;
        }

    case chip::DeviceLayer::DeviceEventType::kFabricWillBeRemoved:
        ESP_LOGI(TAG, "Fabric will be removed");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricUpdated:
        ESP_LOGI(TAG, "Fabric is updated");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricCommitted:
        ESP_LOGI(TAG, "Fabric is committed");
        break;

    default:
        break;
    }
}

// This callback is invoked when clients interact with the Identify Cluster.
// In the callback implementation, an endpoint can identify itself. (e.g., by flashing an LED or light).
static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}

// This callback is called for every attribute update. The callback implementation shall
// handle the desired attributes and return an appropriate error code. If the attribute
// is not of your interest, please do not return an error code and strictly return ESP_OK.
static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    esp_err_t err = ESP_OK;

    if (type == PRE_UPDATE) {
        /* Driver update */
        app_driver_handle_t driver_handle = (app_driver_handle_t)priv_data;
        err = app_driver_attribute_update(driver_handle, endpoint_id, cluster_id, attribute_id, val);
    }

    return err;
}

/**
 * @brief This function creates a temperature sensor endpoint and associated cluster. The created endpoint and cluster are 
 * stored in the returned matter_config_t structure. The function also sets the name and attribute_id fields of 
 * the matter_config_t structure. 
 * 
 * @param node  A pointer to the node_t structure
 * 
 * @return  A structure that contains information about the created temperature sensor endpoint and cluster
 */
matter_config_t create_temp_sensor(node_t * node) {
    matter_config_t matter_sensor;
    temperature_sensor::config_t endpoint_config;
    temperature_measurement::config_t cluster_config;

    matter_sensor.name = "Temperature Sensor";
    matter_sensor.attribute_id = chip::app::Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id;

    //Create a temperature sensor endpoint
    matter_sensor.endpoint_p = temperature_sensor::create(node, &endpoint_config, CLUSTER_FLAG_SERVER, NULL);
    if (!matter_sensor.endpoint_p) {
        ESP_LOGE(TAG, "Matter temperature endpoint creation failed");
    }
    matter_sensor.endpoint_id = endpoint::get_id(matter_sensor.endpoint_p);
    ESP_LOGI(TAG, "Temp measure endpoint created with id %d", matter_sensor.endpoint_id);

    //Create a cluster for the temperature endpoint
    matter_sensor.cluster_p = temperature_measurement::create(matter_sensor.endpoint_p, &cluster_config, CLUSTER_FLAG_SERVER);
    matter_sensor.cluster_id = cluster::get_id(matter_sensor.cluster_p);
    ESP_LOGI(TAG, "Temp measurement cluster created with cluster_id %d", matter_sensor.cluster_id);

    return matter_sensor;
}

/**
 * @brief Creates an illumination sensor endpoint and cluster.
 * 
 * This function creates an endpoint and a cluster for the illumination sensor. 
 * The created endpoint is of type `illuminance_sensor` and the created cluster is of type `illuminance_measurement`. 
 * The created endpoint and cluster are stored in a `matter_config_t` structure and returned as the result of the function.
 * 
 * @param node  Pointer to the node that the endpoint and cluster belong to.
 * 
 * @return  Structure containing the created endpoint and cluster information.
 */
matter_config_t create_illumination_sensor(node_t * node) {
    matter_config_t matter_sensor;
    light_sensor::config_t endpoint_config;
    illuminance_measurement::config_t cluster_config;

    matter_sensor.name = "Illumination Sensor";
    matter_sensor.attribute_id = chip::app::Clusters::IlluminanceMeasurement::Attributes::MeasuredValue::Id;

    //Create an illumination sensor endpoint
    matter_sensor.endpoint_p = light_sensor::create(node, &endpoint_config, CLUSTER_FLAG_SERVER, NULL);
    if (!matter_sensor.endpoint_p) {
        ESP_LOGE(TAG, "Matter illuminance endpoint creation failed");
    }
    matter_sensor.endpoint_id = endpoint::get_id(matter_sensor.endpoint_p);
    ESP_LOGI(TAG, "Illuminance measure endpoint created with id %d", matter_sensor.endpoint_id);

    //Create a cluster for the illumination endpoint
    matter_sensor.cluster_p = illuminance_measurement::create(matter_sensor.endpoint_p, &cluster_config, CLUSTER_FLAG_SERVER);
    matter_sensor.cluster_id = cluster::get_id(matter_sensor.cluster_p);
    ESP_LOGI(TAG, "Illuminance measurement cluster created with cluster_id %d", matter_sensor.cluster_id);

    return matter_sensor;
}

/**
 * @brief Updates the matter sensor with new values
 * 
 * @param matter_config Configuration structure for matter sensor
 * @param val Pointer to the new attribute value to be set
 */
void update_matter_sensor(matter_config_t matter_config, esp_matter_attr_val_t * val) {
    update(matter_config.endpoint_id, matter_config.cluster_id, matter_config.attribute_id, val);
    
    if (val->type == ESP_MATTER_VAL_TYPE_BOOLEAN) {
        ESP_LOGI(TAG, "%s updated with value: %d | attribute id: %lu", matter_config.name.c_str(), val->val.b, matter_config.attribute_id);
    } else if (val->type == ESP_MATTER_VAL_TYPE_NULLABLE_INT16) {
        ESP_LOGI(TAG, "%s updated with value: %d", matter_config.name.c_str(), val->val.i16);
    }
}

extern "C" void app_main()
{
    esp_err_t err = ESP_OK;

    /* Initialize the ESP NVS layer */
    nvs_flash_init();

    matter_config_t illumination_sensor_config;
    matter_config_t temp_sensor_config;

    matter_attr_val_scd30_reading_t scd30_reading;
    i2c_dev_t scd30_dev_descriptor = {(i2c_port_t)0};

    i2c_dev_t bh1750_dev_descriptor;
    esp_matter_attr_val_t bh1750_reading;

    ESP_ERROR_CHECK(i2cdev_init());

    bh1750_sensor_init(&bh1750_dev_descriptor);
    scd30_sensor_init(&scd30_dev_descriptor);
    //bmp280_i2c_hal_init();

    /* Create a Matter node and add the mandatory Root Node device type on endpoint 0 */
    node::config_t node_config;
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
    if (!node) {
        ESP_LOGE(TAG, "Matter node creation failed");
    }

    illumination_sensor_config = create_illumination_sensor(node);
    temp_sensor_config = create_temp_sensor(node);

    #if CHIP_DEVICE_CONFIG_ENABLE_THREAD
        /* Set OpenThread platform config */
        esp_openthread_platform_config_t config = {
            .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
            .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
            .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
        };
        set_openthread_platform_config(&config);
    #endif

    /* Matter start */
    err = esp_matter::start(app_event_cb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Matter start failed: %d", err);
    }

    #if CONFIG_ENABLE_ENCRYPTED_OTA
        err = esp_matter_ota_requestor_encrypted_init(s_decryption_key, s_decryption_key_len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialized the encrypted OTA, err: %d", err);
        }
    #endif // CONFIG_ENABLE_ENCRYPTED_OTA

    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::wifi_register_commands();
    esp_matter::console::init();

    //int32_t data;

    while(true) {
        // get sensor reading from BH1750
        bh1750_reading = bh1750_sensor_update(&bh1750_dev_descriptor);
        scd30_reading = scd30_sensor_update(&scd30_dev_descriptor);

        if(scd30_reading.data_ready) {
            update_matter_sensor(temp_sensor_config, &(scd30_reading.temperature_reading));
        }

        // update matter with the new sensor reading
        update_matter_sensor(illumination_sensor_config, &bh1750_reading);

        printf("\n");

        // wait one second before repeating the process
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
