
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_cluster.h>
#include <esp_matter_console.h>
#include <esp_matter_ota.h>

#include <common_macros.h>
#include <app_priv.h>
#include <app_reset.h>
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/ESP32/OpenthreadLauncher.h>
#endif

#include <app/server/CommissioningWindowManager.h>
#include <app/clusters/diagnostic-logs-server/diagnostic-logs-server.h>
#include <diagnostic-logs-provider-delegate-impl.h>
#include <app/server/Server.h>

#include <app/util/attribute-storage.h> // This might be necessary for attributes
#include <app/clusters/on-off-server/on-off-server.h> // For OnOff cluster specifics


static const char *TAG = "app_main";
uint16_t relay1_endpoint = 2;
uint16_t relay2_endpoint = 3;
uint16_t button1_endpoint = 0;
uint16_t button2_endpoint = 1;



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
#endif 

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

    case chip::DeviceLayer::DeviceEventType::kBLEDeinitialized:
        ESP_LOGI(TAG, "BLE deinitialized and memory reclaimed");
        break;

    default:
        break;
    }
}


// This callback is invoked when clients interact with the Identify Cluster.

static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}


static esp_err_t relay1_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    esp_err_t err = ESP_OK;

    if (type == PRE_UPDATE) {
        // Driver update 
        app_driver_handle_t driver_handle1 = (app_driver_handle_t)priv_data;
        err = app_driver_attribute_update(driver_handle1, endpoint_id, cluster_id, attribute_id, val);
    }

    return err;
}


static esp_err_t relay2_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    esp_err_t err = ESP_OK;

   if (type == PRE_UPDATE) {
        app_driver_handle_t driver_handle2 = (app_driver_handle_t)priv_data;
        err = app_driver_attribute_update(driver_handle2, endpoint_id, cluster_id, attribute_id, val);
        
      
   }

    return err;
}


static esp_err_t button1_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    esp_err_t err = ESP_OK;

    if (type == PRE_UPDATE) {
        app_driver_handle_t driver_handle3 = (app_driver_handle_t)priv_data;
        err = app_driver_attribute_update(driver_handle3, endpoint_id, cluster_id, attribute_id, val);
    }

    return err;
}



extern "C" void app_main()
{
    esp_err_t err = ESP_OK;

    /* Initialize the ESP NVS layer */
    nvs_flash_init();

    /* Initialize driver */
    app_driver_handle_t relay1_handle = app_driver_relay1_init();
    app_driver_handle_t relay2_handle = app_driver_relay2_init();
    app_driver_handle_t button1_handle = app_driver_button1_init();
    app_driver_handle_t button2_handle = app_driver_button2_init();
    app_driver_handle_t button3_handle = app_driver_button3_init();

    
    node::config_t node_config1;
    node::config_t node_config2;
    node::config_t node_config3;

    // node handle can be used to add/modify other endpoints.
    node_t *node1 = node::create(&node_config1, relay1_attribute_update_cb, app_identification_cb);
    ABORT_APP_ON_FAILURE(node1 != nullptr, ESP_LOGE(TAG, "Failed to create Matter node"));

    node_t *node2 = node::create(&node_config2, relay2_attribute_update_cb, app_identification_cb);
    ABORT_APP_ON_FAILURE(node2 != nullptr, ESP_LOGE(TAG, "Failed to create Matter node"));

    node_t *node3 = node::create(&node_config3, button1_attribute_update_cb, app_identification_cb);
    ABORT_APP_ON_FAILURE(node1 != nullptr, ESP_LOGE(TAG, "Failed to create Matter node"));

    


    // add diagnostic logs cluster on root endpoint
    cluster::diagnostic_logs::config_t diag_logs_config;
    endpoint_t *root_ep1 = endpoint::get(node1, 0); // get the root node ep
    cluster::diagnostic_logs::create(root_ep1, &diag_logs_config, CLUSTER_FLAG_SERVER);

    endpoint_t *root_ep2 = endpoint::get(node2, 1); // get the root node ep
    cluster::diagnostic_logs::create(root_ep2, &diag_logs_config, CLUSTER_FLAG_SERVER);

    endpoint_t *root_ep3 = endpoint::get(node3, 2); // get the root node ep
    cluster::diagnostic_logs::create(root_ep3, &diag_logs_config, CLUSTER_FLAG_SERVER);

    endpoint_t *root_ep4 = endpoint::get(node2, 3); // get the root node ep
    cluster::diagnostic_logs::create(root_ep2, &diag_logs_config, CLUSTER_FLAG_SERVER);


    extended_color_light::config_t button1_config;
    extended_color_light::config_t button2_config;
    extended_color_light::config_t relay1_config;
    extended_color_light::config_t relay2_config;

    // Add button 1 endpoint
    endpoint_t *button1 = extended_color_light::create(node3, &button1_config, ENDPOINT_FLAG_NONE, button1_handle);
    ABORT_APP_ON_FAILURE(button1 != nullptr, ESP_LOGE(TAG, "Failed to create button 1 endpoint"));

    button1_endpoint = endpoint::get_id(button1);
    ESP_LOGI(TAG, "Light created with endpoint_id %d", button1_endpoint);

    // Add button 2 endpoint
    endpoint_t *button2 = extended_color_light::create(node2, &button2_config, ENDPOINT_FLAG_NONE, button2_handle);
    ABORT_APP_ON_FAILURE(button2 != nullptr, ESP_LOGE(TAG, "Failed to create button 2 endpoint"));

    button2_endpoint = endpoint::get_id(button2);
    ESP_LOGI(TAG, "Light created with endpoint_id %d", button2_endpoint);

    // Add relay 1 endpoint
    endpoint_t *relay1 = extended_color_light::create(node1, &relay1_config, ENDPOINT_FLAG_NONE, relay1_handle);
    ABORT_APP_ON_FAILURE(relay1 != nullptr, ESP_LOGE(TAG, "Failed to create relay 1 endpoint"));

    relay1_endpoint = endpoint::get_id(relay1);
    ESP_LOGI(TAG, "Light created with endpoint_id %d", relay1_endpoint);

    // Add relay 2 endpoint
    endpoint_t *relay2 = extended_color_light::create(node2, &relay2_config, ENDPOINT_FLAG_NONE, relay2_handle);
    ABORT_APP_ON_FAILURE(relay2 != nullptr, ESP_LOGE(TAG, "Failed to create relay 2 endpoint"));

    relay2_endpoint = endpoint::get_id(relay2);
    ESP_LOGI(TAG, "Light created with endpoint_id %d", relay2_endpoint);

   
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
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to start Matter, err:%d", err));

    /* Starting driver with default values */
    app_driver_relay1_set_defaults(relay1_endpoint);
    app_driver_relay2_set_defaults(relay2_endpoint);
  


#if CONFIG_ENABLE_ENCRYPTED_OTA
    err = esp_matter_ota_requestor_encrypted_init(s_decryption_key, s_decryption_key_len);
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to initialized the encrypted OTA, err: %d", err));
#endif // CONFIG_ENABLE_ENCRYPTED_OTA

#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::wifi_register_commands();
#if CONFIG_OPENTHREAD_CLI
    esp_matter::console::otcli_register_commands();
#endif
    esp_matter::console::init();
#endif
}

using namespace chip::app::Clusters::DiagnosticLogs;
void emberAfDiagnosticLogsClusterInitCallback(chip::EndpointId endpoint)
{
    auto & logProvider = LogProvider::GetInstance();
    DiagnosticLogsServer::Instance().SetDiagnosticLogsProviderDelegate(endpoint, &logProvider);
}
