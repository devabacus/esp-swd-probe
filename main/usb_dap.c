/**
 * @file usb_dap.c
 * @brief CMSIS-DAP v2 (WinUSB, bulk) over the native full-speed USB of ESP32-S3.
 *
 * One vendor interface: bulk OUT 0x02 (requests), bulk IN 0x81 (responses).
 * Windows binds WinUSB itself through the MS OS 2.0 descriptor; pyOCD, OpenOCD
 * and probe-rs find the probe by the "CMSIS-DAP" substring of the interface string.
 */
#include "sdkconfig.h"

#ifdef CONFIG_DAP_TRANSPORT_USB

#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"

#include "main/dap_configuration.h"
#include "DAP_config.h" // DAP_PACKET_COUNT
#include "main/dap_pktring.h"
#include "main/usb_dap.h"
#include "main/usb_serial.h"

// Приём по границам передач работает ТОЛЬКО без FIFO (vendor_device.c: при CFG_TUD_VENDOR_TXRX_BUFFERED=1 tud_vendor_rx_cb
// получает buffer=NULL, bufsize=0, данные остаются во FIFO — пакеты не доходят, хост видит «CMSIS-DAP command CMD_INFO failed»).
// Так бывает с устаревшим sdkconfig: после правки sdkconfig.defaults* его надо удалить. Громкий отказ сборки вместо немого прибора.
#if CFG_TUD_VENDOR_TXRX_BUFFERED
#error "usb_dap.c: vendor FIFO включён (CONFIG_TINYUSB_VENDOR_RX_BUFSIZE>0). Удалите sdkconfig и пересоберите: нужен RX_BUFSIZE=0 из sdkconfig.defaults.esp32s3"
#endif

extern uint32_t DAP_ExecuteCommand(const uint8_t *request, uint8_t *response);

static const char *TAG = "usb_dap";

#define EP_OUT 0x02
#define EP_IN  0x81
#define EP_SIZE 64
#define VENDOR_REQUEST_MS_OS_20 1

enum { STR_LANG = 0, STR_MANUF, STR_PRODUCT, STR_SERIAL, STR_ITF };

static const char *kStrings[] = {
    (const char[]){0x09, 0x04}, // 0: English
    "esp-swd-probe",            // 1: manufacturer
    "ESP32-S3 CMSIS-DAP",       // 2: product
    "000000000000",             // 3: serial — подменяется MAC платы в usb_dap_start (usb_serial_get)
    "ESP32-S3 CMSIS-DAP v2",    // 4: interface (must contain "CMSIS-DAP")
};

static const tusb_desc_device_t kDevice = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0201, // >= 2.01: host asks for the BOS descriptor (WinUSB binding)
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = CONFIG_DAP_USB_VID,
    .idProduct = CONFIG_DAP_USB_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = STR_MANUF,
    .iProduct = STR_PRODUCT,
    .iSerialNumber = STR_SERIAL,
    .bNumConfigurations = 1,
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN)

static const uint8_t kConfig[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_VENDOR_DESCRIPTOR(0, STR_ITF, EP_OUT, EP_IN, EP_SIZE),
};

// ---- MS OS 2.0: compatible ID WINUSB + DeviceInterfaceGUID ----
#define MS_OS_20_DESC_LEN 0xA0
static const uint8_t kMsOs20[] = {
    U16_TO_U8S_LE(0x000A), U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR),
    U32_TO_U8S_LE(0x06030000), U16_TO_U8S_LE(MS_OS_20_DESC_LEN),
    // compatible ID
    U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID),
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // registry property: DeviceInterfaceGUID (REG_SZ, UTF-16LE)
    U16_TO_U8S_LE(0x0082), U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
    U16_TO_U8S_LE(0x0007), U16_TO_U8S_LE(0x0028),
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0, 'I', 0, 'n', 0, 't', 0, 'e', 0,
    'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0, 'G', 0, 'U', 0, 'I', 0, 'D', 0, 0, 0,
    U16_TO_U8S_LE(0x0050),
    '{', 0, 'C', 0, 'D', 0, 'B', 0, '3', 0, 'B', 0, '5', 0, 'A', 0, 'D', 0, '-', 0,
    '2', 0, '9', 0, '3', 0, 'B', 0, '-', 0, '4', 0, '6', 0, '6', 0, '3', 0, '-', 0,
    'A', 0, 'A', 0, '3', 0, '6', 0, '-', 0, '1', 0, 'A', 0, 'A', 0, 'E', 0, '4', 0,
    '6', 0, '4', 0, '6', 0, '3', 0, '7', 0, '7', 0, '6', 0, '}', 0, 0, 0, 0, 0,
};

_Static_assert(sizeof(kMsOs20) == MS_OS_20_DESC_LEN, "MS OS 2.0 descriptor length");

#define BOS_TOTAL_LEN (TUD_BOS_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN)
static const uint8_t kBos[] = {
    TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN, 1),
    TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, VENDOR_REQUEST_MS_OS_20),
};

uint8_t const *tud_descriptor_bos_cb(void)
{
    return kBos;
}

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    if (stage != CONTROL_STAGE_SETUP) {
        return true;
    }
    if (request->bRequest == VENDOR_REQUEST_MS_OS_20 && request->wIndex == 7) {
        return tud_control_xfer(rhport, request, (void *)kMsOs20, MS_OS_20_DESC_LEN);
    }
    return false;
}

// ---- DAP packets ----
// Приём по границам USB-передач: RX_BUFSIZE=0, пакет приходит в tud_vendor_rx_cb целиком и кладётся в кольцо
// из DAP_PACKET_COUNT пакетов (main/dap_pktring.*). Поток байт (прежнее tud_vendor_read) склеивал два коротких запроса.
#define SEND_TIMEOUT_TICKS 200 // ≈ 200 тиков FreeRTOS без прогресса записи — ответ бросаем
#define LOG_FIRST_N 8          // сколько первых обменов печатать в журнал
static unsigned sLogged;
static TaskHandle_t sDapTask;
static dap_pkt_t sSlots[DAP_PACKET_COUNT];
static dap_pktring_t sRing;

#if CFG_TUD_API_V0_19_COMPAT
void tud_vendor_rx_cb(uint8_t idx, const uint8_t *buffer, uint16_t bufsize)
#else
void tud_vendor_rx_cb(uint8_t idx, const uint8_t *buffer, uint32_t bufsize)
#endif
{
    (void)idx;
    // Переполнение — пакет теряется и считается (хост шлёт не более DAP_PACKET_COUNT запросов, переполнения быть не должно).
    dap_pktring_push(&sRing, buffer, bufsize);
    // буфер действителен только внутри коллбэка (скопировали); приём TinyUSB перевооружает сам после возврата
    if (sDapTask) {
        xTaskNotifyGive(sDapTask); // runs in the TinyUSB task, not in an ISR
    }
}

static void dap_usb_task(void *arg)
{
    static dap_pkt_t pkt;
    static uint8_t resp[DAP_PACKET_SIZE];

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // по одному, ответ строго по порядку запросов
        while (dap_pktring_pop(&sRing, &pkt)) {
            if (pkt.len == 0) {
                continue;
            }
            uint32_t r = DAP_ExecuteCommand(pkt.data, resp) & 0xFFFF;
            // без FIFO tud_vendor_write отправляет ≤ EPSIZE одной передачей, 0 — предыдущий ответ ещё в пути
            // (или конечная точка не открыта). Ждём не дольше SEND_TIMEOUT_TICKS: иначе при отсоединённом хосте задача зависает.
            uint32_t sent = 0;
            uint32_t waited = 0;
            while (sent < r) {
                uint32_t w = tud_vendor_write(resp + sent, r - sent);
                sent += w;
                if (w == 0) {
                    if (++waited > SEND_TIMEOUT_TICKS) {
                        ESP_LOGW(TAG, "tx timeout: cmd 0x%02x, sent %u of %u", pkt.data[0], (unsigned)sent, (unsigned)r);
                        break;
                    }
                    vTaskDelay(1);
                }
            }
            if (sLogged < LOG_FIRST_N) { // диагностика первых обменов: доходит ли запрос и уходит ли ответ
                sLogged++;
                ESP_LOGI(TAG, "dap #%u: rx %u B cmd 0x%02x -> resp %u B, sent %u", (unsigned)sLogged,
                         (unsigned)pkt.len, pkt.data[0], (unsigned)r, (unsigned)sent);
            }
        }
    }
}

void usb_dap_start(void)
{
    kStrings[STR_SERIAL] = usb_serial_get(); // у каждой платы свой серийный (раньше у всех «000001»)
    tinyusb_config_t cfg = TINYUSB_DEFAULT_CONFIG();
    cfg.descriptor.device = &kDevice;
    cfg.descriptor.string = kStrings;
    cfg.descriptor.string_count = sizeof(kStrings) / sizeof(kStrings[0]);
    cfg.descriptor.full_speed_config = kConfig;

    dap_pktring_init(&sRing, sSlots, DAP_PACKET_COUNT);
    xTaskCreatePinnedToCore(dap_usb_task, "dap_usb", 4096, NULL, 10, &sDapTask, 1);
    ESP_ERROR_CHECK(tinyusb_driver_install(&cfg));
    ESP_LOGI(TAG, "CMSIS-DAP v2 on native USB, packet %d bytes", DAP_PACKET_SIZE);
}

#endif // CONFIG_DAP_TRANSPORT_USB
