#include "usb_interface.h"
#include "main.h"
#include "protocol.h"
#include <string.h>
#include <usbd_core.h>

static Bootloader BOOTLOADER;
static uint8_t RECV_BUF[USB_MAX_EP0_SIZE];

static int8_t cdc_init(void);
static int8_t cdc_deinit(void);
static int8_t cdc_control(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t cdc_receive(uint8_t* pbuf, uint32_t* Len);

USBD_CDC_ItfTypeDef USB_CDC_INTERFACE = {.Init = cdc_init,
                                         .DeInit = cdc_deinit,
                                         .Control = cdc_control,
                                         .Receive = cdc_receive};

static int8_t cdc_init(void) {
    bootloader_init(&BOOTLOADER);

    USBD_CDC_SetTxBuffer(&USB_DEVICE, NULL, 0);
    USBD_CDC_SetRxBuffer(&USB_DEVICE, RECV_BUF);

    return USBD_OK;
}

static int8_t cdc_deinit(void) {
    return USBD_OK;
}

static int8_t cdc_control(uint8_t cmd, uint8_t* buf, uint16_t len) {
    (void) buf;
    (void) len;

    switch (cmd) {
        case CDC_SET_LINE_CODING:
        case CDC_GET_LINE_CODING:
        case CDC_SEND_ENCAPSULATED_COMMAND:
        case CDC_GET_ENCAPSULATED_RESPONSE:
        case CDC_SET_COMM_FEATURE:
        case CDC_GET_COMM_FEATURE:
        case CDC_CLEAR_COMM_FEATURE:
        case CDC_SET_CONTROL_LINE_STATE:
        case CDC_SEND_BREAK:
        default: { break; }
    }

    return USBD_OK;
}

static int8_t cdc_receive(uint8_t* buf, uint32_t* len) {
    USBD_CDC_SetRxBuffer(&USB_DEVICE, buf);

    USBD_StatusTypeDef result = USBD_CDC_ReceivePacket(&USB_DEVICE);
    if (result != USBD_OK) {
        return result;
    }

    bootloader_process(&BOOTLOADER, buf, *len);
    return USBD_OK;
}

void usb_serial_print(char* msg) {
    USBD_CDC_SetTxBuffer(&USB_DEVICE, (uint8_t*) msg, strlen(msg));

    if (USBD_CDC_TransmitPacket(&USB_DEVICE) != USBD_OK) {
        on_error();
    }
}
