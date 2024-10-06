
#include <esp_log.h>
#include <stdlib.h>
#include <string.h>
#include "esp_rom_gpio.h"

#include <esp_matter.h>
#include "bsp/esp-bsp.h"

#include <app_priv.h>
using namespace chip::app::Clusters;
using namespace esp_matter;

static const char *TAG = "app_driver";
extern uint16_t relay1_endpoint;
extern uint16_t relay2_endpoint;
extern uint16_t button1_endpoint;
extern uint16_t button2_endpoint;
button_handle_t btns[BSP_BUTTON_NUM];
int relay_to_control = 0;

static bool is_updating = false;

static void button_factory_reset_pressed_cb(void *arg, void *data)
{  
    ESP_LOGI(TAG, "Starting factory reset");
    chip::DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
}





static esp_err_t app_driver_relay_set_power(led_indicator_handle_t handle, esp_matter_attr_val_t *val)
{
#if CONFIG_BSP_LEDS_NUM > 0
    esp_err_t err = ESP_OK;

    if(relay_to_control == 1) {
        gpio_set_level((gpio_num_t)19, val->val.b ? 0 : 1); // Control Relay 1
    } else if(relay_to_control == 2) {
        gpio_set_level((gpio_num_t)20, val->val.b ? 0 : 1); // Control Relay 2
    }
    
    return err;
#else
    ESP_LOGI(TAG, "Relay set power: %d", val->val.b);
    return ESP_OK;
#endif
}


static void app_driver_button_button1_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "Button 1 pressed");  

    uint16_t endpoint_id = button1_endpoint;
    uint32_t cluster_id = OnOff::Id;
    uint32_t attribute_id = OnOff::Attributes::OnOff::Id;

    node_t *node = node::get();
    endpoint_t *endpoint = endpoint::get(node, endpoint_id);
    cluster_t *cluster = cluster::get(endpoint, cluster_id);
    attribute_t *attribute = attribute::get(cluster, attribute_id);

    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute::get_val(attribute, &val);
    val.val.b = !val.val.b;
    attribute::update(endpoint_id, cluster_id, attribute_id, &val);
}

static void app_driver_button_relay2_cb(void *arg, void *data)
{
     if (is_updating) {
        ESP_LOGW(TAG, "Callback already in progress, skipping...");
        return;
    }

    ESP_LOGI(TAG, "Button for Relay 2 pressed");  

    is_updating = true;


    ESP_LOGI(TAG, "Button for Relay 2 pressed");  

    uint16_t endpoint_id = relay2_endpoint;
    uint32_t cluster_id = OnOff::Id;
    uint32_t attribute_id = OnOff::Attributes::OnOff::Id;

    node_t *node = node::get();
    endpoint_t *endpoint = endpoint::get(node, endpoint_id);
    cluster_t *cluster = cluster::get(endpoint, cluster_id);
    attribute_t *attribute = attribute::get(cluster, attribute_id);

    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute::get_val(attribute, &val);
    val.val.b = !val.val.b;
    attribute::update(endpoint_id, cluster_id, attribute_id, &val);
        
    is_updating = false;
}


static void app_driver_button_button2_cb(void *arg, void *data){
     if (is_updating) {
        ESP_LOGW(TAG, "Callback already in progress, skipping...");
        return;
    }

    ESP_LOGI(TAG, "Button for Relay 2 pressed");  

    is_updating = true;
    
    uint16_t endpoint_id = button2_endpoint;
    uint32_t cluster_id = OnOff::Id;
    uint32_t attribute_id = OnOff::Attributes::OnOff::Id;

    node_t *node = node::get();
    endpoint_t *endpoint = endpoint::get(node, endpoint_id);
    cluster_t *cluster = cluster::get(endpoint, cluster_id);
    attribute_t *attribute = attribute::get(cluster, attribute_id);

    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute::get_val(attribute, &val);
    val.val.b = !val.val.b;
    attribute::update(endpoint_id, cluster_id, attribute_id, &val);
        // Update the button state
    is_updating = false;
}

//when an attribute is updated, this function is called
esp_err_t app_driver_attribute_update(app_driver_handle_t driver_handle, uint16_t endpoint_id, uint32_t cluster_id,
                                      uint32_t attribute_id, esp_matter_attr_val_t *val)
{
    esp_err_t err = ESP_OK;
    led_indicator_handle_t handle = (led_indicator_handle_t)driver_handle;
  
    
    // Check if the endpoint matches any relay or button endpoint
    if (endpoint_id == relay1_endpoint || endpoint_id == relay2_endpoint || 
        endpoint_id == button1_endpoint || endpoint_id == button2_endpoint) {
        
        if (cluster_id == OnOff::Id && attribute_id == OnOff::Attributes::OnOff::Id) {
            if (endpoint_id == relay1_endpoint) {
                relay_to_control = 1;  
                err = app_driver_relay_set_power(handle, val);
            } else if (endpoint_id == relay2_endpoint) {
                relay_to_control = 2;
                app_driver_button_button2_cb(NULL,NULL);
                err = app_driver_relay_set_power(handle, val);
            }
            else if( endpoint_id == button2_endpoint) {
                relay_to_control = 2;
               app_driver_button_relay2_cb(NULL,NULL);
               err = app_driver_relay_set_power(handle, val);
            }
           
            
        }
    }
 
    return err;
}



esp_err_t app_driver_relay1_set_defaults(uint16_t endpoint_id)
{
   esp_err_t err = ESP_OK;
    void *priv_data = endpoint::get_priv_data(endpoint_id);
    led_indicator_handle_t handle = (led_indicator_handle_t)priv_data;
    node_t *node = node::get();
    endpoint_t *endpoint = endpoint::get(node, endpoint_id);
    cluster_t *cluster = NULL;
    attribute_t *attribute = NULL;
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);

    cluster = cluster::get(endpoint, OnOff::Id);
    attribute = attribute::get(cluster, OnOff::Attributes::OnOff::Id);
    attribute::get_val(attribute, &val);
    err |= app_driver_relay_set_power(handle, &val);

    return err;
}

esp_err_t app_driver_relay2_set_defaults(uint16_t endpoint_id)
{
    esp_err_t err = ESP_OK;
    void *priv_data = endpoint::get_priv_data(endpoint_id);
    led_indicator_handle_t handle = (led_indicator_handle_t)priv_data;
    node_t *node = node::get();
    endpoint_t *endpoint = endpoint::get(node, endpoint_id);
    cluster_t *cluster = NULL;
    attribute_t *attribute = NULL;
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);

    cluster = cluster::get(endpoint, OnOff::Id);
    attribute = attribute::get(cluster, OnOff::Attributes::OnOff::Id);
    attribute::get_val(attribute, &val);
    err |= app_driver_relay_set_power(handle, &val);


    return err;
}

app_driver_handle_t app_driver_relay1_init()
{
#if CONFIG_BSP_LEDS_NUM > 0

    esp_rom_gpio_pad_select_gpio((gpio_num_t)19);

    /* Set GPIO direction as output */
    gpio_set_direction((gpio_num_t)19, GPIO_MODE_OUTPUT);

    /* Turn off relays initially */
    gpio_set_level((gpio_num_t)19, 1);
    return (app_driver_handle_t)ESP_OK;
#else
    return NULL;
#endif
}

app_driver_handle_t app_driver_relay2_init()
{
#if CONFIG_BSP_LEDS_NUM > 0

    esp_rom_gpio_pad_select_gpio((gpio_num_t)20);

    /* Set GPIO direction as output */
    gpio_set_direction((gpio_num_t)20, GPIO_MODE_OUTPUT);

    /* Turn off relays initially */
    gpio_set_level((gpio_num_t)20, 1);
    return (app_driver_handle_t)ESP_OK;
#else
    return NULL;
#endif
}

app_driver_handle_t app_driver_button2_init()
{

    button_handle_t btns[BSP_BUTTON_NUM];
    ESP_ERROR_CHECK(bsp_iot_button_create(btns, NULL, BSP_BUTTON_NUM));
    ESP_ERROR_CHECK(iot_button_register_cb(btns[1], BUTTON_PRESS_DOWN, app_driver_button_button2_cb, NULL));
     ESP_ERROR_CHECK(iot_button_register_cb(btns[1], BUTTON_PRESS_DOWN, app_driver_button_relay2_cb, NULL));
    return (app_driver_handle_t)btns[0];
    
}

app_driver_handle_t app_driver_button1_init()
{
    button_handle_t btns[BSP_BUTTON_NUM];
    ESP_ERROR_CHECK(bsp_iot_button_create(btns, NULL, BSP_BUTTON_NUM));
    ESP_ERROR_CHECK(iot_button_register_cb(btns[0], BUTTON_PRESS_DOWN, app_driver_button_button1_cb, NULL));
    
    return (app_driver_handle_t)btns[0];
}

app_driver_handle_t app_driver_button3_init()
{
    
    button_handle_t btns[BSP_BUTTON_NUM];
    ESP_ERROR_CHECK(bsp_iot_button_create(btns, NULL, BSP_BUTTON_NUM));
    ESP_ERROR_CHECK(iot_button_register_cb(btns[2], BUTTON_PRESS_DOWN, button_factory_reset_pressed_cb, NULL));
    return (app_driver_handle_t)btns[0];
}