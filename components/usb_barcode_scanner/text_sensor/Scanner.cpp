//#ifdef USE_ESP32 #TODO: remove

#include "esphome/core/log.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "usb/usb_host.h"
#include "usb/hid_host.h"
#include "usb/hid_usage_keyboard.h"

#include "Scanner.h"


namespace esphome {
namespace usb_barcode_scanner {


static const char *const TAG = "Scanner";

QueueHandle_t event_queue = NULL;
QueueHandle_t line_queue = NULL;

typedef struct {
    hid_host_device_handle_t handle;
    hid_host_driver_event_t event;
    void *arg;
} event_queue_t;

using line_t = unsigned char[64];

/* Main char symbol for ENTER key */
#define KEYBOARD_ENTER_MAIN_CHAR    '\r'

const uint8_t keycode2ascii [57][2] = {
    {0, 0}, /* HID_KEY_NO_PRESS        */
    {0, 0}, /* HID_KEY_ROLLOVER        */
    {0, 0}, /* HID_KEY_POST_FAIL       */
    {0, 0}, /* HID_KEY_ERROR_UNDEFINED */
    {'a', 'A'}, /* HID_KEY_A               */
    {'b', 'B'}, /* HID_KEY_B               */
    {'c', 'C'}, /* HID_KEY_C               */
    {'d', 'D'}, /* HID_KEY_D               */
    {'e', 'E'}, /* HID_KEY_E               */
    {'f', 'F'}, /* HID_KEY_F               */
    {'g', 'G'}, /* HID_KEY_G               */
    {'h', 'H'}, /* HID_KEY_H               */
    {'i', 'I'}, /* HID_KEY_I               */
    {'j', 'J'}, /* HID_KEY_J               */
    {'k', 'K'}, /* HID_KEY_K               */
    {'l', 'L'}, /* HID_KEY_L               */
    {'m', 'M'}, /* HID_KEY_M               */
    {'n', 'N'}, /* HID_KEY_N               */
    {'o', 'O'}, /* HID_KEY_O               */
    {'p', 'P'}, /* HID_KEY_P               */
    {'q', 'Q'}, /* HID_KEY_Q               */
    {'r', 'R'}, /* HID_KEY_R               */
    {'s', 'S'}, /* HID_KEY_S               */
    {'t', 'T'}, /* HID_KEY_T               */
    {'u', 'U'}, /* HID_KEY_U               */
    {'v', 'V'}, /* HID_KEY_V               */
    {'w', 'W'}, /* HID_KEY_W               */
    {'x', 'X'}, /* HID_KEY_X               */
    {'y', 'Y'}, /* HID_KEY_Y               */
    {'z', 'Z'}, /* HID_KEY_Z               */
    {'1', '!'}, /* HID_KEY_1               */
    {'2', '@'}, /* HID_KEY_2               */
    {'3', '#'}, /* HID_KEY_3               */
    {'4', '$'}, /* HID_KEY_4               */
    {'5', '%'}, /* HID_KEY_5               */
    {'6', '^'}, /* HID_KEY_6               */
    {'7', '&'}, /* HID_KEY_7               */
    {'8', '*'}, /* HID_KEY_8               */
    {'9', '('}, /* HID_KEY_9               */
    {'0', ')'}, /* HID_KEY_0               */
    {KEYBOARD_ENTER_MAIN_CHAR, KEYBOARD_ENTER_MAIN_CHAR}, /* HID_KEY_ENTER           */
    {0, 0}, /* HID_KEY_ESC             */
    {'\b', 0}, /* HID_KEY_DEL             */
    {0, 0}, /* HID_KEY_TAB             */
    {' ', ' '}, /* HID_KEY_SPACE           */
    {'-', '_'}, /* HID_KEY_MINUS           */
    {'=', '+'}, /* HID_KEY_EQUAL           */
    {'[', '{'}, /* HID_KEY_OPEN_BRACKET    */
    {']', '}'}, /* HID_KEY_CLOSE_BRACKET   */
    {'\\', '|'}, /* HID_KEY_BACK_SLASH      */
    {'\\', '|'}, /* HID_KEY_SHARP           */  // HOTFIX: for NonUS Keyboards repeat HID_KEY_BACK_SLASH
    {';', ':'}, /* HID_KEY_COLON           */
    {'\'', '"'}, /* HID_KEY_QUOTE           */
    {'`', '~'}, /* HID_KEY_TILDE           */
    {',', '<'}, /* HID_KEY_LESS            */
    {'.', '>'}, /* HID_KEY_GREATER         */
    {'/', '?'} /* HID_KEY_SLASH           */
};

/**
 * @brief Key buffer scan code search.
 *
 * @param[in] src       Pointer to source buffer where to search
 * @param[in] key       Key scancode to search
 * @param[in] length    Size of the source buffer
 */
static inline bool key_found(const uint8_t *const src, uint8_t key, unsigned int length) {
    for (unsigned int i = 0; i < length; i++) {
        if (src[i] == key) {
            return true;
        }
    }
    return false;
}

static void hid_host_keyboard_report_callback(const uint8_t *const data, const int length) {
    if (length < sizeof(hid_keyboard_input_report_boot_t)) {
        return;
    }

    hid_keyboard_input_report_boot_t *kb_report = (hid_keyboard_input_report_boot_t *)data;

    static uint8_t prev_keys[HID_KEYBOARD_KEY_MAX] = { 0 };
    static line_t line = { 0 };
    static uint8_t size = 0;

    for (int i = 0; i < HID_KEYBOARD_KEY_MAX; i++) {
        uint8_t key_code = kb_report->key[i];
        if (key_code >= HID_KEY_A && !key_found(prev_keys, key_code, HID_KEYBOARD_KEY_MAX)) {
            if (key_code <= HID_KEY_SLASH) {
                unsigned char key_char = keycode2ascii[key_code][0];
                if (!!key_char) {
                    line[size++] = key_char;
                    if (KEYBOARD_ENTER_MAIN_CHAR == key_char) {
                        line[size++] = '\n';
                        if (line_queue) {
                            xQueueSend(line_queue, line, 0);
                        }
                        size = 0;
                    }
                }
            }
        }
    }

    memcpy(prev_keys, &kb_report->key, HID_KEYBOARD_KEY_MAX);
}

void hid_host_interface_callback(hid_host_device_handle_t hid_device_handle, const hid_host_interface_event_t event, void *arg) {
    if (event == HID_HOST_INTERFACE_EVENT_DISCONNECTED) {
        ESP_LOGI(TAG, "HID Device DISCONNECTED");
        ESP_ERROR_CHECK(hid_host_device_close(hid_device_handle));
        return;
    }
    if (event == HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR) {
        ESP_LOGI(TAG, "HID Device TRANSFER_ERROR");
        return;
    }
    if (event == HID_HOST_INTERFACE_EVENT_INPUT_REPORT) {
        uint8_t data[64] = { 0 };
        size_t data_length = 0;
    
        ESP_ERROR_CHECK(hid_host_device_get_raw_input_report_data(hid_device_handle,
                                                                  data,
                                                                  64,
                                                                  &data_length));

        hid_host_keyboard_report_callback(data, data_length);
        return;
    }
    
    ESP_LOGI(TAG, "HID Device Unhandled event");
}

void hid_host_device_event(hid_host_device_handle_t hid_device_handle, const hid_host_driver_event_t event, void *arg) {
    if (event != HID_HOST_DRIVER_EVENT_CONNECTED) {
        return;
    }

    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));
    ESP_LOGI(TAG, "HID Device CONNECTED");

    if (dev_params.proto != HID_PROTOCOL_KEYBOARD) {
        return;
    }

    const hid_host_device_config_t dev_config = {
        .callback = hid_host_interface_callback,
        .callback_arg = NULL
    };

    ESP_ERROR_CHECK(hid_host_device_open(hid_device_handle, &dev_config));
    if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
        ESP_ERROR_CHECK(hid_class_request_set_protocol(hid_device_handle, HID_REPORT_PROTOCOL_BOOT));
        if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
            ESP_ERROR_CHECK(hid_class_request_set_idle(hid_device_handle, 0, 0));
        }
    }
    ESP_ERROR_CHECK(hid_host_device_start(hid_device_handle));
}

static void usb_lib_task(void *arg) {
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    ESP_ERROR_CHECK(usb_host_install(&host_config));
    xTaskNotifyGive(arg);

    while (true) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            ESP_ERROR_CHECK(usb_host_device_free_all());
            break;
        }
    }

    ESP_LOGI(TAG, "USB shutdown");
    vTaskDelay(10); // Short delay to allow clients clean-up
    ESP_ERROR_CHECK(usb_host_uninstall());
    vTaskDelete(NULL);
}

void hid_host_device_callback(hid_host_device_handle_t hid_device_handle, const hid_host_driver_event_t event, void *arg) {
    if (event_queue) {
        event_queue_t evt_queue = {};
        evt_queue.handle = hid_device_handle;
        evt_queue.event = event;
        evt_queue.arg = arg;
    
        xQueueSend(event_queue, &evt_queue, 0);
    }
}

bool Scanner::setup() {
    BaseType_t task_created = xTaskCreatePinnedToCore(usb_lib_task, "usb_events", 4096, xTaskGetCurrentTaskHandle(), 2, NULL, 0);
    if (task_created != pdTRUE) {
        ESP_LOGE(TAG, "usb events lib task creation failed");
        return false;
    }

    // Wait for notification from usb_lib_task to proceed
    ulTaskNotifyTake(false, 1000);

    const hid_host_driver_config_t hid_host_driver_config = {
        .create_background_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .core_id = 0,
        .callback = hid_host_device_callback,
        .callback_arg = NULL
    };

    esp_err_t err = hid_host_install(&hid_host_driver_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "usb hid driver install failed: %s", esp_err_to_name(err));
        return false;
    }

    event_queue = xQueueCreate(10, sizeof(event_queue_t));
    line_queue = xQueueCreate(5, sizeof(line_t));
    return true;
}

optional<std::string> Scanner::barcode() {
    event_queue_t evt_queue = {};
    if (xQueueReceive(event_queue, &evt_queue, 0)) {
        hid_host_device_event(evt_queue.handle, evt_queue.event, evt_queue.arg);
    }

    line_t line;
    if (xQueueReceive(line_queue, &line, 0)) {
        return make_optional(std::string(reinterpret_cast<char*>(line)));
    }

    return optional<std::string>();
}

}  // namespace usb_barcode_scanner
}  // namespace esphome

//#endif
