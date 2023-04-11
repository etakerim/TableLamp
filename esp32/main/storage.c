#include "lamp.h"


esp_err_t nvs_config() 
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());     // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_init());      //  Retry nvs_flash_init
    }
    return err;
}

esp_err_t nvs_load(Light *light)
{
    nvs_handle_t storage;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &storage);
    if (err != ESP_OK) return err;

    size_t len = 0;
    err = nvs_get_blob(storage, "config", NULL, &len);

    // Set default values if does not exist
    if (len == 0) {
        err = nvs_set_blob(storage, "config", light, len);
        if (err != ESP_OK) return err;
        err = nvs_commit(storage);
        if (err != ESP_OK) return err;
        len = sizeof(*light);
    }

    err = nvs_get_blob(storage, "config", light, &len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

    nvs_close(storage);
    return ESP_OK;
}

esp_err_t nvs_save(Light *light)
{
    nvs_handle_t storage;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &storage);
    if (err != ESP_OK) return err;

    err = nvs_set_blob(storage, "config", light, sizeof(*light));
    if (err != ESP_OK) return err;

    err = nvs_commit(storage);
    if (err != ESP_OK) return err;
    nvs_close(storage);
    return ESP_OK;
}