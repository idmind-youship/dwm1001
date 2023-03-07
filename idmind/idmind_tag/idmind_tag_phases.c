/*! ----------------------------------------------------------------------------
 *  @file       idmind_tag_phases.c
 *  @brief      Code for TAG device. Definition of phase handling functions
 *  @author     cneves
 */

#include "idmind_tag.h"

/* The frame sent in this example is an 802.15.4e standard blink. 
 * It is a 12-byte frame composed of the following fields:
 *     - byte 0: frame type (0xC5 for a blink).
 *     - byte 1: sequence number, incremented for each new frame.
 *     - byte 2 -> 9: device ID.
 *     - byte 10/11: frame check-sum, automatically set by DW1000.
 */
static uint8 blink_msg[] = {0xC5, 0, 'D', 'E', 'C', 'A', 'W', 'A', 'V', 'E', 0, 0};

/*! --------------------------------------------------------------------------
 * @fn print_msg()
 * @brief Auxiliary function to print messages
 * @param  none
 * @return none
 */
void print_msg(char* msg, int size)
{
    printk("M: ");
    for(int i = 0; i < size; i++) printk("0x%02X ", msg[i]);
    printk("\n");
}

/*! --------------------------------------------------------------------------
 * @fn discovery_phase()
 * @brief In the discovery phase, the tag sends blink messages and waits for 
 *          ranging init reply 
 * @param  none
 * @return 0 if successful pairing, -1 otherwise
 */
int discovery_phase(int* seq_nr, uint32 dev_id, int* anchor_id)
{
    static uint32 status_reg = 0;
    printk("Starting discovery.\n");
    /* Prepare and send blink message, waiting for reply */
    printk("Sending blink %d with id %ld\n", *seq_nr, dev_id);
    // Set sequence number
    blink_msg[1] = *seq_nr;  
    // Set device id
    for(int i = 0; i < 8; i++) blink_msg[2+i] = (dev_id >> 8*i) && 0xFF;  
    print_msg(blink_msg, 12);
    // Send message to TX Buffer
    dwt_writetxdata(sizeof(blink_msg), blink_msg, 0);
    dwt_writetxfctrl(sizeof(blink_msg), 0, 0); 
    dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);
    (*seq_nr)++;
    // Wait for reply (simplicity, but maybe we should use callbacks?)
    while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR))){}

    printk("Received ranging init from anchor %d.\n", *anchor_id);
    return 0;
}

/*! --------------------------------------------------------------------------
 * @fn ranging_phase()
 * @brief In the ranging phase, the tag sends poll message, waits a response and 
 *          sends final message.
 * @param  none
 * @return 0 if successful pairing, -1 otherwise
 */
int ranging_phase(void)
{
    printk("Starting ranging.\n");
    return 0;
}