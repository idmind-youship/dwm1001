/* Do all the includes */
#include <string.h>

#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "port.h"
// zephyr includes
#include <zephyr.h>
#include <sys/printk.h>

#define DEV_ID 0x0001
#define PAN_ID 0x6380

/* UWB microsecond (uus) to device time unit (dtu, around 15.65 ps) 
 * conversion factor.
 * 1 uus = 512 / 499.2 usec and 1 usec = 499.2 * 128 dtu. */
#define UUS_TO_DWT_TIME 65536

/* Delay Definitions */
// Delay after Tx to start Rx scan (UWB microseconds)
#define TX_TO_RX_DELAY_UUS 60
// Rx timeout (UWB microseconds)
#define RX_RESP_TIMEOUT_UUS 8000
// TX and Rx Antenna delays
#define TX_ANT_DLY 16436
#define RX_ANT_DLY 16436
// Delay between ranging attempts (miliseconds)
#define RANGE_PERIOD 500

/* Rx Buffer to be used in callbacks */
#define FRAME_LEN_MAX 127
static bool rx_received;
static uint8 rx_buffer[FRAME_LEN_MAX];
/* The frame sent in this example is an 802.15.4e standard blink. 
 * It is a 12-byte frame composed of the following fields:
 *     - byte 0: frame type (0xC5 for a blink).
 *     - byte 1: sequence number, incremented for each new frame.
 *     - byte 2 -> 9: device ID.
 *     - byte 10/11: frame check-sum, automatically set by DW1000.
 */
static uint8 blink_msg[] = {0xC5, 0, 'D', 'E', 'C', 'A', 'W', 'A', 'V', 'E', 0, 0};

/* 0x41 0x8C SEQ PANID_L PANID_H 2*DEST 2*SOURCE 0x61 FCS_L FCS_H*/
static uint8 poll_msg[] = {0x41, 0x88, 0x00, 0xCA, 0xDE, 0x00, 0x00, 0x00, 0x00, 0x61, 0x00, 0x00};
static uint8 final_msg[] = {0x41, 0x88, 0x00, 0xCA, 0xDE, 0x00, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/* idmind_tag_callbacks.c */
void tx_done_cb(const dwt_cb_data_t *cb_data);
void rx_ok_cb(const dwt_cb_data_t *cb_data);
void rx_timeout_cb(const dwt_cb_data_t *cb_data);
void rx_err_cb(const dwt_cb_data_t *cb_data);
int rx_message(uint8* rx_buffer);

/* idmind_tag_phases.c */
static uint64 tx_to_rx_delay;
static uint64 ranging_rx_ts;
static uint64 poll_tx_ts;
int discovery_phase(int* seq_nr, uint32* dev_id, int* anchor_id);
int ranging_phase(int* seq_nr, uint32* dev_id, int* anchor_id);

/* idmind_tag.c */
void print_header(void);
int start_dwm(void);
int config_dwm(void);
int dw_main(void);
