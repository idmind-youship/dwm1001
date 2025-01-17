/*! ----------------------------------------------------------------------------
 *  @file       idmind_tag.c
 *  @brief      Code for TAG device. It should send blinks, wait for ranging init
 *                  send Poll, wait response and send Final
 *  @author     cneves
 */

#include "idmind_tag.h"

static uint8 rx_buffer[FRAME_LEN_MAX];

#define LOG_LEVEL 3
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

/* Example application name and version to display on console. */
#define APP_NAME "IDMind TAG TWR INIT v0.1"
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

#define SPEED_OF_LIGHT 299702547

/*! --------------------------------------------------------------------------
 * @fn print_header()
 * @brief Routine to print header with name, author and date, if defined
 * @param  none
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
 * @brief Opens communication with DWM, reset and initialize
 * @param  none
 * @return 0 if success, error code otherwise
 */
int start_dwm(void)
{
    printk("Starting DWM Communication\n");
    /* Open SPI to communicate with DWM1001 */
    openspi();
    /* Configure device */
    reset_DW1000(); 
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
 * @brief Configures DWM
 * @param  none
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
    dwt_setrxaftertxdelay(TX_TO_RX_DELAY_UUS);
    dwt_setrxtimeout(RX_RESP_TIMEOUT_UUS);

    k_yield();
    printk("Success!\n");
    return 0;
}

/*! --------------------------------------------------------------------------
 * @fn main()
 * @brief Application entry point.
 * @param  none
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

    /* Initialization of main loop */
    bool discovery = true;
    int seq_nr = 0;

    // Preparation of messages
    uint16 anchor_id = 0;
    uint16 tag_short_id = 0;
    uint64 resp_delay = 0;
    uint64 rx_ranging_init_ts = 0;
    uint64 tx_poll_ts = 0;

    /*****************************************/
    /*          Setup Blink Message          */
    /*****************************************/
    uint8 blink_msg[12];
    for(int i=0; i<12; i++) blink_msg[i] = 0;
    // Header
    blink_msg[0] = 0xC5;
    // Set device id on Blink message
    for(int i = 0; i < 8; i++) blink_msg[2+i] = (DEV_ID >> 8*i) && 0xFF;  
    /*****************************************/
    /*          Setup Poll Message          */
    /*****************************************/
    uint8 poll_msg[12];
    for(int i=0; i<12; i++) poll_msg[i] = 0;
    // Header
    poll_msg[0] = 0x41;
    poll_msg[1] = 0x88;
    // Network
    poll_msg[3] = PAN_ID & 0xFF;
    poll_msg[4] = (PAN_ID >> 8) & 0xFF;
    // Destination address is set in real time.
    // Short Source address is set on real time
    poll_msg[9] = 0x61;

    /*****************************************/
    /*          Setup Final Message          */
    /*****************************************/
    uint8 final_msg[20];
    for(int i=0; i<12; i++) final_msg[i] = 0;
    // Header
    final_msg[0] = 0x41;
    final_msg[1] = 0x88;
    // Network
    final_msg[3] = PAN_ID & 0xFF;
    final_msg[4] = (PAN_ID >> 8) & 0xFF;
    // Destination address is set in real time.
    // Short Source address is set on real time
    final_msg[9] = 0x69;

    int iter = 0;
    uint16 frame_len = 0;
    uint32 status_reg = 0;
    while(1)
    {
        iter++;
        printk("============ Iter %d =============\n", iter);

        /*************************************/
        /*          DISCOVERY PHASE          */
        /*************************************/
        if(discovery){
            // Send Blink Message
            seq_nr++;
            blink_msg[1] = seq_nr;

            dwt_writetxdata(12, blink_msg, 0);
            dwt_writetxfctrl(12, 0, 0); 
            printk("Sending Blink: ");
            print_msg(blink_msg, 12);
            if (dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) == DWT_ERROR){
                printk("Error sending Blink");
                Sleep(BIG_PERIOD);
                continue;
            }

            if (rx_message(rx_buffer) != 0){
                printk("Did not receive Ranging Init message.\n");
                Sleep(BIG_PERIOD);
                continue;
            }
            // If message received is Ranging Init, prepare for Ranging Phase
            if ((rx_buffer[0] == 0x41) & (rx_buffer[1] == 0x8C) & (rx_buffer[15] == 0x20)){
                print_msg(rx_buffer, 22);
                anchor_id = rx_buffer[13]+(rx_buffer[14]<<8);
                tag_short_id = rx_buffer[16]+(rx_buffer[17]<<8);
                resp_delay = (rx_buffer[18]+(rx_buffer[19]<<8)); // UWB microseconds
                rx_ranging_init_ts = get_rx_timestamp_u64();
                printk("Received a Ranging Init from Anchor %u, set id to %u and delay to %llumicros\n", anchor_id, tag_short_id, resp_delay);
                discovery = false;
            }
            else{
                printk("Expected a Ranging Init, received smothing else:\n");
                print_msg(rx_buffer, dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFL_MASK_1023);
            } // end of ranging init handling
        }

        if(!discovery){
            /*************************************/
            /*           RANGING PHASE           */
            /*************************************/
            // Tag sends a Poll mesage, delayed by resp_delay
            poll_msg[2] = seq_nr;
            for(int i=0; i<2; i++) poll_msg[5+i] = (anchor_id >> 8*i) & 0xFF;
            for(int i=0; i<2; i++) poll_msg[7+i] = (tag_short_id >> 8*i) & 0xFF;
            tx_poll_ts = (rx_ranging_init_ts + (UUS_TO_DWT_TIME*resp_delay));
            printk("RxR: %llu | Delay: %llu | TxP: %lu\n", rx_ranging_init_ts, (UUS_TO_DWT_TIME*resp_delay), tx_poll_ts);
            dwt_setdelayedtrxtime((uint32)(tx_poll_ts>>8));
            dwt_writetxdata(12, poll_msg, 0);
            dwt_writetxfctrl(12, 0, 0); 
            // printk("Sending Poll: ");
            // print_msg(poll_msg, 12);
            if (dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED) == DWT_ERROR){
                printk("Error sending Poll Message.\n");
                discovery = true;
                Sleep(PERIOD);
                continue;
            }
            
        }
        discovery = true;
        Sleep(PERIOD);

    }
    return 0;
}
