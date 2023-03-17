/* Do all the includes */
#include <string.h>

#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "port.h"
// zephyr includes
#include <zephyr.h>
#include <sys/printk.h>

#define DEV_ID 0x00AC
#define PAN_ID 0x6380
#define MAX_DEVICES 4
#define PERIOD 10
#define SPEED_OF_LIGHT 299702547

/* UWB microsecond (uus) to device time unit (dtu, around 15.65 ps) 
 * conversion factor.
 * 1 uus = 512 / 499.2 usec and 1 usec = 499.2 * 128 dtu. */
#define UUS_TO_DWT_TIME 65536

/* Delay Definitions */
// Delay after Tx to start Rx scan (UWB microseconds)
#define TX_TO_RX_DELAY_UUS 60
// Rx timeout (UWB microseconds)
#define RX_RESP_TIMEOUT_UUS 5000

// Delay between Poll Rx and Response Tx (UWB microseconds) sent in ranging init
#define POLL_RX_TO_RESP_TX_DLY_UUS 6000

// TX and Rx Antenna delays
#define TX_ANT_DLY 16436
#define RX_ANT_DLY 16436
// Delay between ranging attempts (miliseconds)
#define RANGE_PERIOD 500

static bool flag_interrupt;
/* Rx Buffer to be used in callbacks */
#define FRAME_LEN_MAX 127
static bool rx_received;
static uint8 rx_buffer[FRAME_LEN_MAX];
/* 0x41 0x8C SEQ PANID_L PANID_H 8*DEST 2*SOURCE 0x20 TAG_SHORT_L TAG_SHORT_H RESP_DELAY_L RESP_DELAY_H FCS_L FCS_H*/
// static uint8 ranging_init[] = {0x41, 0x8C, 0x00, 0xCA, 0xDE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// static uint8 ranging_init[22];
/* 0x41 0x88 SEQ PANID_L PANID_H 2*DEST 2*SOURCE 0x50 4*TOF FCS_L FCS_H */
// static uint8 resp_msg[] = {0x41, 0x88, 0x00, 0xCA, 0xDE, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static int ranging_tx_ts;
static int poll_rx_ts;
static int resp_tx_ts;

/* idmind_anchor_callbacks.c */
void tx_done_cb(const dwt_cb_data_t *cb_data);
void rx_ok_cb(const dwt_cb_data_t *cb_data);
void rx_timeout_cb(const dwt_cb_data_t *cb_data);
void rx_err_cb(const dwt_cb_data_t *cb_data);
int rx_message(uint8* rx_buffer);

/* idmind_anchor_phases.c */
void print_msg(char* msg, int size);
uint64 get_tx_timestamp_u64(void);
uint64 get_rx_timestamp_u64(void);
void final_msg_get_ts(const uint8 *ts_field, uint32 *ts);
int discovery_phase(int* seq_nr, uint32 dev_id, bool* dev_list, uint8* ranging_init_msg);
int ranging_phase(int* seq_nr, uint32 dev_id, bool* dev_list);

/* idmind_anchor.c */
void print_header(void);
int start_dwm(void);
int config_dwm(void);
void enable_flag_interrupt(void);
int dw_main(void);
