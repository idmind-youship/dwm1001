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
    printk("Starting discovery.\n");
    /* Prepare and send blink message, waiting for reply */
    printk("Sending blink %d with id %d\n", *seq_nr, DEV_ID);
    // Set sequence number
    blink_msg[1] = *seq_nr;
    (*seq_nr)++;
    // print_msg(blink_msg, 12);
    // Send message to TX Buffer
    dwt_writetxdata(sizeof(blink_msg), blink_msg, 0);
    dwt_writetxfctrl(sizeof(blink_msg), 0, 0); 
    if (dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED) == DWT_SUCCESS){
        if (rx_message(rx_buffer)==0){
            if ((rx_buffer[0] == 0x41) & (rx_buffer[1] == 0x8C) & (rx_buffer[15] == 0x20)){
                // Save anchor ID from ranging init message and given ID
                (*anchor_id) = rx_buffer[13] + 256*rx_buffer[14];
                (*dev_id) = rx_buffer[16] + 256*rx_buffer[17];
                // Set up the Poll and Final messages
                // Set device ids
                poll_msg[5] = (*anchor_id) & 0xFF;
                poll_msg[6] = ((*anchor_id) >> 8) & 0xFF;
                poll_msg[7] = (*dev_id) & 0xFF;
                poll_msg[8] = ((*dev_id) >> 8) & 0xFF;
                // Set device id
                final_msg[5] = (*anchor_id) & 0xFF;
                final_msg[6] = ((*anchor_id) >> 8) & 0xFF;
                final_msg[7] = (*dev_id) & 0xFF;
                final_msg[8] = ((*dev_id) >> 8) & 0xFF;

                printk("Received a Ranging init message from 0x%02X\n", *(anchor_id));
                print_msg(rx_buffer, 20);
                ranging_rx_ts = get_rx_timestamp_u64();
                tx_to_rx_delay = rx_buffer[12] + rx_buffer[12]*256;
                printk("Received at %llu, reports delay of %llu\n", ranging_rx_ts, tx_to_rx_delay);
                return 0;
            }
            else{
                printk("The message received is NOT a Ranging Init. H 0x%02X%02X | A 0x%02X \n", rx_buffer[0], rx_buffer[1], rx_buffer[15]);
                print_msg(rx_buffer, 22);
                return -1;
            }
        }
        else{
            printk("Did not receive a Ranging init message\n");
            return -1;
        }
    }
    else{
        printk("There was an error transmitting the Blink message.\n");
        return -1;
    }
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
    printk("Starting ranging.\n");
    uint32 resp_rx_time;
    uint32 final_tx_time;

    // Set sequence number
    poll_msg[2] = *seq_nr;
    (*seq_nr)++;
    poll_tx_ts = ranging_rx_ts + tx_to_rx_delay;
    printk("RI received at %llu and P will be sent at %llu\n", ranging_rx_ts, poll_tx_ts);
    dwt_setdelayedtrxtime(poll_tx_ts);
    dwt_writetxdata(sizeof(poll_msg), poll_msg, 0);
    dwt_writetxfctrl(sizeof(poll_msg), 0, 0); 
    if (dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED) == DWT_SUCCESS){
        printk("Poll message sent\n");
        if(rx_message(rx_buffer) == 0){
            if ((rx_buffer[0] == 0x41) & (rx_buffer[1] == 0x88) & (rx_buffer[9] == 0x50)){
                printk("Received a Poll Response\n");
                // print_msg(rx_buffer, 16);
                // Prepare to send final message to anchor
                // Set sequence number
                final_msg[2] = *seq_nr;
                (*seq_nr)++;

                resp_rx_time = get_rx_timestamp_u64();
                final_tx_time = resp_rx_time + tx_to_rx_delay;
                uint32 dt1 = resp_rx_time - poll_tx_ts;
                uint32 dt2 = final_tx_time - resp_rx_time;
                for(int i=0; i<4;i++) final_msg[10+i] = (dt1 >> 8*i) & 0xFF;
                for(int i=0; i<4;i++) final_msg[14+i] = (dt2 >> 8*i) & 0xFF;
                printk("Final Msg: ");
                print_msg(final_msg, 20);
                dwt_setdelayedtrxtime(final_tx_time);
                dwt_writetxdata(sizeof(final_msg), final_msg, 0);
                dwt_writetxfctrl(sizeof(final_msg), 0, 0); 
                if (dwt_starttx(DWT_START_TX_DELAYED) == DWT_SUCCESS){
                    printk("Final message sent\n");
                    return 0;
                }
                else{
                    printk("Failed to send final message\n");
                    return -1;
                }
            }
            else{
                printk("Message received was NOT a Poll Response\n");
                return -1;
            }
        }
        else{
            printk("Did not receive a Poll Response\n");
            return -1;
        }
    }
    else{
        printk("There was an error transmitting the Poll message");
        return -1;
    }

}