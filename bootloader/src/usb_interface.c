#include "usb_interface.h"
#include "main.h"
#include <string.h>
#include <usbd_core.h>

#define NUM_SEND_BUFS 4
static uint8_t SEND_BUFS[NUM_SEND_BUFS][USB_MAX_EP0_SIZE];
static size_t CURRENT_SEND_BUF_IDX = 0;

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
    USBD_CDC_SetTxBuffer(&USB_DEVICE, SEND_BUFS[CURRENT_SEND_BUF_IDX], 0);
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
    USBD_CDC_ReceivePacket(&USB_DEVICE);

    USBD_CDC_HandleTypeDef* cdc_handle = (USBD_CDC_HandleTypeDef*) USB_DEVICE.pClassData;

    if (cdc_handle->TxState != 0) {
        if (CURRENT_SEND_BUF_IDX + 1 >= NUM_SEND_BUFS) {
            return USBD_BUSY;
        }

        CURRENT_SEND_BUF_IDX++;
    } else {
        CURRENT_SEND_BUF_IDX = 0;
    }

    memcpy(SEND_BUFS[CURRENT_SEND_BUF_IDX], buf, *len);
    USBD_CDC_SetTxBuffer(&USB_DEVICE, SEND_BUFS[CURRENT_SEND_BUF_IDX], *len);
    return USBD_CDC_TransmitPacket(&USB_DEVICE);
}
