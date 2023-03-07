/*! ----------------------------------------------------------------------------
 *  @file       idmind_test.c
 *  @brief      testing of API
 *  @author     cneves
 */

#include <string.h>

#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "port.h"

// zephyr includes
#include <zephyr.h>
#include <sys/printk.h>

#define LOG_LEVEL 3
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

#define DEV_ID 0x0001
#define PAN_ID 0x6380

/* Example application name and version to display on console. */
#define APP_NAME "IDMind Calibration TWR INIT v0.1"
#define APP_AUTHOR "Carlos Neves"
#define APP_DATE "March 2023"
#define APP_WIDTH 50

/* Default communication configuration. */
static dwt_config_t config = {
    5,               /* Channel number. */
    DWT_PRF_64M,     /* Pulse repetition frequency. */
    DWT_PLEN_128,    /* Preamble length. Used in TX only. */
    DWT_PAC8,        /* Preamble acquisition chunk size. Used in RX only. */
    9,               /* TX preamble code. Used in TX only. */
    9,               /* RX preamble code. Used in RX only. */
    1,               /* 0 to use standard SFD, 1 to use non-standard SFD. */
    DWT_BR_6M8,      /* Data rate. */
    DWT_PHRMODE_STD, /* PHY header mode. */
    (129)            /* SFD timeout (preamble length + 1 + SFD length - PAC size). 
                        Used in RX only. */           
};

/* Delays settings */
#define TX_ANT_DLY 0
#define RX_ANT_DLY 0
#define POLL_TX_TO_RESP_RX_DLY_UUS 300
#define RESP_RX_TIMEOUT_UUS 6000
#define RESP_RX_TO_FINAL_TX_DLY_UUS 4000

/* UWB microsecond (uus) to device time unit (dtu, around 15.65 ps) 
 * conversion factor.
 * 1 uus = 512 / 499.2 usec and 1 usec = 499.2 * 128 dtu. */
#define UUS_TO_DWT_TIME 65536

/* Buffer to store received response message.
 * Its size is adjusted to longest frame that this example code is supposed 
 * to handle.
 */
#define RX_BUF_LEN 20
static uint8 rx_buffer[RX_BUF_LEN];

#define PERIOD 500
#define SPEED_OF_LIGHT 299702547
/*! --------------------------------------------------------------------------
 * @fn print_header()
 *
 * @brief Routine to print header with name, author and date, if defined
 *
 * @param  none
 *
 * @return none
 */
void print_header(void)
{
    /* Print header */
    char separator[APP_WIDTH+1];
    for (int i = 0; i < APP_WIDTH; i++) separator[i] = '*';
    separator[APP_WIDTH+1] = '\0';
    printk("/*%s*/\n", separator);
    /* Print centered app name header */
    char name[APP_WIDTH+1];
    int stop1 = (int)((APP_WIDTH-strlen(APP_NAME))/2);
    for (int i = 0; i < APP_WIDTH; i++) name[i] = ' ';
    for (int i = 0; i < strlen(APP_NAME); i++) name[stop1+i] = APP_NAME[i];
    name[APP_WIDTH] = '\0';
    printk("/*%s*/\n", name);
    /* Print Author */
    #ifdef APP_AUTHOR
    char author[APP_WIDTH+1];
    int stop2 = (int)((APP_WIDTH-strlen(APP_AUTHOR))/2);
    for (int i = 0; i < APP_WIDTH; i++) author[i] = ' ';
    for (int i = 0; i < strlen(APP_AUTHOR); i++) author[stop2+i] = APP_AUTHOR[i];
    author[APP_WIDTH] = '\0';
    printk("/*%s*/\n", author);
    #endif
    /* Print Date */
    #ifdef APP_DATE
    char date[APP_WIDTH+1];
    int stop3 = (int)((APP_WIDTH-strlen(APP_DATE))/2);
    for (int i = 0; i < APP_WIDTH; i++) date[i] = ' ';
    for (int i = 0; i < strlen(APP_DATE); i++) date[stop3+i] = APP_DATE[i];
    date[APP_WIDTH] = '\0';
    printk("/*%s*/\n", date);
    #endif
    /* Print footer */
    printk("/*%s*/\n",separator);
    return;
}

/*! --------------------------------------------------------------------------
 * @fn start_dwm()
 *
 * @brief Opens communication with DWM, reset and initialize
 *
 * @param  none
 *
 * @return 0 if success, error code otherwise
 */

int start_dwm(void)
{
    printk("Starting DWM Communication with ");
    /* Open SPI to communicate with DWM1001 */
    openspi();
    /* Configure device */
    port_set_dw1000_slowrate();
    if (dwt_initialise(DWT_LOADUCODE) == DWT_ERROR) {
        printk("INIT FAILED");
        k_sleep(K_MSEC(500)); // allow logging to run.
        while (1) { };
    }
    port_set_dw1000_fastrate();
    printk("Success!\n");
    return 0;
}

/*! --------------------------------------------------------------------------
 * @fn config_dwm()
 *
 * @brief Configures DWM
 *
 * @param  none
 *
 * @return 0 if success, error code otherwise
 */

int config_dwm(void)
{
    printk("Configuring DWM... ");
    /* Configure DW1000. */
    dwt_configure(&config);

    /* Configure DW1000 LEDs */
    dwt_setleds(3);

    /* Apply default antenna delay value. */
    dwt_setrxantennadelay(TX_ANT_DLY);
    dwt_settxantennadelay(RX_ANT_DLY);

    /* Set expected response's delay and timeout. */
    dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);

    k_yield();
    printk("Success!\n");
    return 0;
}

/*! --------------------------------------------------------------------------
 * @fn get_tx_timestamp_u64()
 *
 * @brief Get the TX time-stamp in a 64-bit variable.
 *        /!\ This function assumes that length of time-stamps is 40 bits, 
 *            for both TX and RX!
 *
 * @param  none
 *
 * @return  64-bit value of the read time-stamp.
 */
static uint64 get_tx_timestamp_u64(void)
{
    uint8 ts_tab[5];
    uint64 ts = 0;
    int i;

    dwt_readtxtimestamp(ts_tab);
    for (i = 4; i >= 0; i--) {
        ts <<= 8;
        ts |= ts_tab[i];
    }
    return ts;
}

/*! --------------------------------------------------------------------------
 * @fn get_rx_timestamp_u64()
 *
 * @brief Get the RX time-stamp in a 64-bit variable.
 *        /!\ This function assumes that length of time-stamps is 40 bits,
 *        for both TX and RX!
 *
 * @param  none
 *
 * @return  64-bit value of the read time-stamp.
 */
static uint64 get_rx_timestamp_u64(void)
{
    uint8 ts_tab[5];
    uint64 ts = 0;
    int i;

    dwt_readrxtimestamp(ts_tab);
    for (i = 4; i >= 0; i--) {
        ts <<= 8;
        ts |= ts_tab[i];
    }
    return ts;
}

/*! --------------------------------------------------------------------------
 * @fn final_msg_set_ts()
 *
 * @brief Fill a given timestamp field in the final message with the 
 *        given value. In the timestamp fields of the final
 *        message, the least significant byte is at the lower address.
 *
 * @param  ts_field  pointer on the first byte of the timestamp field to fill
 *         ts  timestamp value
 *
 * @return none
 */
static void final_msg_set_ts(uint8 *ts_field, uint64 ts)
{
    for (int i = 0; i < 4; i++) {
        ts_field[i] = (uint8) ts;
        ts >>= 8;
    }
}

/*! --------------------------------------------------------------------------
 * @fn main()
 *
 * @brief Application entry point.
 *
 * @param  none
 *
 * @return none
 */
int dw_main(void)
{
    /* Print Header */
    print_header();
    /* Start communication with DWM */
    start_dwm();
    /* Configure DWM */
    config_dwm();

    /* Messages to be used */
    /* - byte 0/1: frame control (0x8841 to indicate a data frame using 16-bit addressing).
       - byte 2: sequence number, incremented for each new frame.
       - byte 3/4: PAN ID (0x6380).
       - byte 5/6: destination address
       - byte 7/8: source address
       - byte 9: function code (specific values to indicate which message it is in the ranging process).
    */
    static uint8 tx_poll_msg[] = {0x41, 0x88, 0, 0x80, 0x63, 0, 0, 0x01, 0x00, 0x21, 0, 0};
    static uint64 poll_tx_ts;
    static uint64 resp_rx_ts;
    /* Frame sequence number, incremented after each transmission. */
    static uint8 frame_seq_nb = 0;
    uint16 destiny = 0x0000;
    /* Status reg copy, used for debugging */
    static uint32 status_reg = 0;
    while(1)
    {
        // Preparations
        frame_seq_nb++;
        Sleep(PERIOD);

        // Prepare polling message
        printk("Frame sequence %d\n", frame_seq_nb);
        destiny = 0x0002;
        tx_poll_msg[2] = frame_seq_nb;
        tx_poll_msg[5] = destiny & 0xFF;
        tx_poll_msg[6] = (destiny >> 8) & 0xFF;

        /* Zero offset in TX buffer. */
        dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0); 
        /* Zero offset in TX buffer, ranging. */
        dwt_writetxfctrl(sizeof(tx_poll_msg), 0, 1); 
        /* Start transmission and wait for response */
        dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);
        /* Wait for frame reception or timeout */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR))){ };

        /* If a message was received and it was a good frame, process it*/
        if (status_reg & SYS_STATUS_RXFCG) {
            printk("Response was received, checking validity.\n");
            /* Clear good RX frame event and TX frame sent in the DW1000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG | SYS_STATUS_TXFRS);

            /* A frame has been received, read it into the local buffer. */
            uint32 frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_MASK;
            if (frame_len <= RX_BUF_LEN) {
                dwt_readrxdata(rx_buffer, frame_len, 0);
            }

            /* Check message validity */
            bool valid_msg = true;
            /* Start by checking headers */
            if (memcmp(rx_buffer, tx_poll_msg, 5) != 0){
                printk("Reply received with wrong headers.\n");
                for(int i=0;i<5;i++) printk("%d vs %d\n", rx_buffer[i], tx_poll_msg[i]);
                valid_msg = false;
            }
            /* Now check for destiny and source */
            if ((rx_buffer[5] != tx_poll_msg[7]) | (rx_buffer[6] != tx_poll_msg[8])){
                printk("Wrong destination. %d %d vs %d %d\n", rx_buffer[5], rx_buffer[6], tx_poll_msg[7], tx_poll_msg[8]);
                valid_msg = false;
            }
            if ((rx_buffer[7] != tx_poll_msg[5]) | (rx_buffer[8] != tx_poll_msg[6])){
                printk("Wrong souce.\n");
                valid_msg = false;
            }
            if (!valid_msg){
                printk("The reply was not valid.\n");
                /* Clear RX error/timeout events in the DW1000 status register. */
                dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
                /* Reset RX to properly reinitialise LDE operation. */
                dwt_rxreset();
                continue;
            }
            /* Retrieve poll transmission and response reception timestamp. */
            poll_tx_ts = get_tx_timestamp_u64();
            resp_rx_ts = get_rx_timestamp_u64();
            printk("Message TX at %lld and Rx at %lld: %lld.\n", poll_tx_ts, resp_rx_ts, resp_rx_ts-poll_tx_ts);
            double dt_uuwb = resp_rx_ts-poll_tx_ts;
            double dt_uus = dt_uuwb/UUS_TO_DWT_TIME;
            printk("")


            double distance = (resp_rx_ts-poll_tx_ts) * DWT_TIME_UNITS * SPEED_OF_LIGHT / UUS_TO_DWT_TIME;
            printk("Estimated distance: %fm.\n", distance);

            /* Compute final message transmission time. See NOTE 10 below. */
            uint32 final_tx_time = (resp_rx_ts + 
                (RESP_RX_TO_FINAL_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;

        }
        else{
            printk("Timeout after Poll message for %d. \n", destiny);
            /* Clear RX error/timeout events in the DW1000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
            /* Reset RX to properly reinitialise LDE operation. */
            dwt_rxreset();
            continue;
        }

    }

    return 0;
}
