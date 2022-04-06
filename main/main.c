#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_now.h"

static const char *TAG = "DustTool";

esp_now_peer_info_t master_info = {
  .peer_addr = { 0xC8, 0xC9, 0xA3, 0xC5, 0xFA, 0x98 },
  .channel = 0,
  .encrypt = false
};

static const uint8_t hello[] = { 'H', 'E', 'L', 'L', 'O' };

static void register_master(void)
{
  ESP_LOGD(TAG, "register_master");

  esp_err_t res = esp_now_add_peer(&master_info);
  if(ESP_OK != res)
  {
    ESP_LOGE(TAG, "register_master: failed: status=%x", res);
  }
}

static void say_hello(void)
{
  ESP_LOGD(TAG, "say_hello");

  esp_err_t res = esp_now_send(master_info.peer_addr, hello, sizeof(hello));
  if(ESP_OK != res) 
  {
    ESP_LOGE(TAG, "say_hello: send failed: res=%x", res);
  }
}

static void nvs_init(void)
{
      // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
}

static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start());
}

static char *data_as_string(const uint8_t *data, int len)
{
  char *r = malloc(len + 1);
  if(r) {
    memcpy(r, data, len);
    r[len] = '\0';
  }
  return r;  
}

static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  ESP_LOGD(TAG, "espnow_send_cb: mac=" MACSTR ", status=%0x", MAC2STR(mac_addr), status);
  if(ESP_NOW_SEND_SUCCESS != status) 
  {
    // try again
    say_hello();
  }

}

static void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
  char *dstring = data_as_string(data, len);
  ESP_LOGD(TAG, "espnow_recv_cb: mac=" MACSTR ", len=%d, data=%s", 
      MAC2STR(mac_addr), len, dstring);
  
  free(dstring);
}

static esp_err_t espnow_init(void)
{
    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(espnow_recv_cb) );

    return ESP_OK;
}

void app_main(void)
{
  nvs_init();
  wifi_init();
  espnow_init();

  esp_log_level_set(TAG, ESP_LOG_DEBUG);

  register_master();
  say_hello();

  while(1)
    vTaskDelay(1);
}