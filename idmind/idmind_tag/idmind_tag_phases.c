/*! ----------------------------------------------------------------------------
 *  @file       idmind_tag_phases.c
 *  @brief      Code for TAG device. Definition of phase handling functions
 *  @author     cneves
 */

#include "idmind_tag.h"

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
uint64 get_tx_timestamp_u64(void)
{
    uint8 ts_tab[5];
    uint64 ts = 0;

    dwt_readtxtimestamp(ts_tab);
    for (int i = 4; i >= 0; i--) {
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
 *             for both TX and RX!
 *
 * @param  none
 *
 * @return  64-bit value of the read time-stamp.
 */
uint64 get_rx_timestamp_u64(void)
{
    uint8 ts_tab[5];
    uint64 ts = 0;

    dwt_readrxtimestamp(ts_tab);
    for (int i = 4; i >= 0; i--) {
        ts <<= 8;
        ts |= ts_tab[i];
    }
    return ts;
}

/*! --------------------------------------------------------------------------
 * @fn final_msg_get_ts()
 *
 * @brief Read a given timestamp value from the final message. 
 *        In the timestamp fields of the final message, the least
 *        significant byte is at the lower address.
 *
 * @param  ts_field  pointer on the first byte of the timestamp field to read
 *         ts  timestamp value
 *
 * @return none
 */
void final_msg_get_ts(const uint8 *ts_field, uint32 *ts)
{
    *ts = 0;
    for (int i = 0; i < 4; i++) {
        *ts += ts_field[i] << (i * 8);
    }
}

/*! --------------------------------------------------------------------------
 * @fn discovery_phase()
 * @brief In the discovery phase, the tag sends blink messages and waits for 
 *          ranging init reply 
 * @param  none
 * @return 0 if successful pairing, -1 otherwise
 */
int discovery_phase(int* seq_nr, uint32* dev_id, int* anchor_id)
{
    return 0;
}

/*! --------------------------------------------------------------------------
 * @fn ranging_phase()
 * @brief In the ranging phase, the tag sends poll message, waits a response and 
 *          sends final message.
 * @param  none
 * @return 0 if successful pairing, -1 otherwise
 */
int ranging_phase(int* seq_nr, uint32* dev_id, int* anchor_id)
{
    return 0;
}