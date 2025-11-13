// Copyright 2020-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __LW_MAC802154_INTERFACE_H__
#define __LW_MAC802154_INTERFACE_H__

#include "stdint.h"

/* --------------------------------------------- Global Definitions */
#define MACL_ISR_TX_COMPLETE                0x0A
#define MACL_ISR_RX_FRAME                   0x0B
#define MACL_ISR_CH_ED                      0x0C
#define MACL_ISR_CH_CCA                     0x0D
#define MACL_ISR_HW_TIMEOUT                 0x0E
#define MACL_ISR_RX_ACK                     0x0F
#define MACL_ISR_RX_SFD                     0x10
#define MACL_ISR_TX_SFD                     0x11

#define MAC_ADATA_OFFSET_PL                 0x00
#define MAC_ENCRYPTION_BLOCK_SIZE           0x10
/**
 *  The number of symbols forming the basic time period used by the CSMA-CA algorithm.
 */
#define LW_MAC_A_UNIT_BACKOFF_PERIOD                20

/* Definitions of API_SUCCESS & API_FAILURE */
#define API_SUCCESS             0x0000
#define API_FAILURE             0xFFFF

/* Definition for True/False */
#define LW_FALSE                                   0
#define LW_TRUE                                    1

/* Status flag - for Free and Allocated */
#define LW_FREE                         0x00
#define LW_ALLOCATED                    0x01
#define LW_INVALID_BUFFER_INDEX         0xFF
 /* RSSI Lower Limit in dBm */
#define LW_MAC_RSSI_LOWER_LIMIT                          -94

 /* RSSI Upper Limit in dBm */
#define LW_MAC_RSSI_UPPER_LIMIT                          -30

/* Offset Value to translate RSSI to LQI value */
#define LW_MAC_RSSI_TO_LQI_OFFSET                        94

/* Multiplier for ranslating RSSI to LQI value */
#define LW_MAC_RSSI_TO_LQI_OFFSET_MULTIPLIER              4
/**
 * Note:
 * Have taken below MAC Enumeration values from
 * IEEE Std 802.15.4-2006 specification.
 */
#define LW_MAC_ENUM_BEACON_LOSS                     0xE0
#define LW_MAC_ENUM_CH_ACCESS_FAILURE               0xE1
#define LW_MAC_ENUM_COUNTER_ERROR                   0xDB
#define LW_MAC_ENUM_DENIED                          0xE2
#define LW_MAC_ENUM_DISABLE_TRX_FAILURE             0xE3
#define LW_MAC_ENUM_FRAME_TOO_LONG                  0xE5
#define LW_MAC_ENUM_IMPROPER_KEY_TYPE               0xDC
#define LW_MAC_ENUM_IMPROPER_SEC_LVL                0xDD
#define LW_MAC_ENUM_INVALID_ADDR                    0xF5
#define LW_MAC_ENUM_INVALID_GTS                     0xE6
#define LW_MAC_ENUM_INVALID_HANDLE                  0xE7
#define LW_MAC_ENUM_INVALID_INDEX                   0xF9
#define LW_MAC_ENUM_INVALID_PARAMETER               0xE8
#define LW_MAC_ENUM_LIMIT_REACHED                   0xFA
#define LW_MAC_ENUM_NO_ACK                          0xE9
#define LW_MAC_ENUM_NO_BEACON                       0xEA
#define LW_MAC_ENUM_NO_DATA                         0xEB
#define LW_MAC_ENUM_NO_SRT_ADDR                     0xEC
#define LW_MAC_ENUM_ON_TIME_TOO_LONG                0xF6
#define LW_MAC_ENUM_OUT_OF_CAP                      0xED
#define LW_MAC_ENUM_PAN_ID_CONFLICT                 0xEE
#define LW_MAC_ENUM_READ_ONLY                       0xFB
#define LW_MAC_ENUM_REALIGNMENT                     0xEF
#define LW_MAC_ENUM_SCAN_IN_PROGRESS                0xFC
#define LW_MAC_ENUM_SECURITY_ERROR                  0xE4
#define LW_MAC_ENUM_SUPERFRAME_OVERLAP              0xFD
#define LW_MAC_ENUM_TRACKING_OFF                    0xF8
#define LW_MAC_ENUM_TRANSACTION_EXPIRED             0xF0
#define LW_MAC_ENUM_TRANSACTION_OVERFLOW            0xF1
#define LW_MAC_ENUM_TX_ACTIVE                       0xF2
#define LW_MAC_ENUM_UNAVAILABLE_KEY                 0xF3
#define LW_MAC_ENUM_UNSUPPORTED_ATTRIBUTE           0xF4
#define LW_MAC_ENUM_UNSUPPORTED_LEGACY              0xDE
#define LW_MAC_ENUM_UNSUPPORTED_SECURITY            0xDF

#define TRD_TX_INTR                                                     0x0001

/**
 * Trd_rx_event:
 * If this bit is set it indicates thread packet is received.
 */
#define TRD_RX_INTR                                                     0x0002
/**
 * Ack_tx_done:
 * If this bit is set it indicates acknowledgement packet is transmited.
 */
#define TRD_TX_ACK_INTR                                                 0x0004
/**
 * Ack_rx_done:
 * If this bit is set it indicates acknowledgement packet is received.
 */
#define TRD_RX_ACK_INTR                                                 0x0008
/**
 * Rx_aborted:
 * If this bit is set it indicates reception is aborted.
 */
#define TRD_RX_ABORT_INTR                                               0x0010
/**
 * Tx_aborted:
 * If this bit is set it indicates transmission is aborted.
 */
#define TRD_TX_ABORT_INTR                                               0x0020
/**
 * Ed_done:
 * If this bit is set indicates energy detection is done.
 */
#define TRD_ED_DONE_INTR                                                0x0040
/**
 * Mac_timer_overflow:
 * If this bit is set indicates mac timer counted its maximum.
 */
#define TRD_MAC_TIMER_INTR                                              0x0100


typedef uint16_t    API_RESULT;
/* Definition of API_RESULT */
typedef uint16_t    API_RESULT;

/* MACL Data Structures */
typedef API_RESULT (* LW_MACL_ISR_HANDLER)
                   (
                      /* IN */ uint8_t              event_type,
                      /* IN */ uint16_t             event_result,
                      /* IN */ void             * data,
                      /* IN */ uint16_t             datalen
                   );

/**
 * @brief Initialize the EM timer.
 */
void lw_mac802154_em_timer_init(void);
/**
 * @brief Initialize the lw component of mac802154.
 */
void lw_mac802154_lw_init(void);

/**
 * @brief Start the timer0 of the lw component of mac802154.
 */
void lw_mac802154_lw_timer0_start(void);

/**
 * @brief Stop the timer0 of the lw component of mac802154.
 */
void lw_mac802154_lw_timer0_stop(void);

/**
 * @brief Set the maximum value of timer0 of the lw component of mac802154.
 * @param value The maximum value to be set for timer0.
 */
void lw_mac802154_lw_timer0_set_max_value(uint32_t value);

/**
 * @brief Transmit a MAC frame using the lw MACL component.
 * @param p_data Pointer to the data to be transmitted.
 * @param len Length of the data to be transmitted.
 * @param options Transmission options.
 * @return API_RESULT indicating the result of the operation.
 */
API_RESULT lw_mac802154_lw_macl_tx_frame_pl(uint8_t *p_data, uint16_t len, uint8_t options);

/**
 * @brief Configure the receive function of the lw MACL component.
 * @param enable Enable or disable the receive function.
 * @param context Configuration context.
 * @return API_RESULT indicating the result of the operation.
 */
API_RESULT lw_mac802154_lw_macl_rx_config_pl(uint8_t enable, uint8_t context);

/**
 * @brief Generate a random number using the lw MACL component.
 * @param input Input value for random number generation.
 * @return Generated random number.
 */
uint32_t lw_mac802154_lw_macl_generate_random_number_pl(uint32_t input);

/**
 * @brief Start the hardware timer of the lw MACL component.
 * @param value Timer value.
 * @param flag Timer flag.
 * @param context Timer context.
 * @return API_RESULT indicating the result of the operation.
 */
API_RESULT lw_mac802154_lw_macl_start_hw_timer_pl(uint32_t value, uint8_t flag, uint8_t context);

/**
 * @brief Start the Clear Channel Assessment (CCA) operation of the lw MACL component.
 * @param duration Duration of the CCA operation.
 * @return API_RESULT indicating the result of the operation.
 */
API_RESULT lw_mac802154_lw_macl_start_cca_pl(uint32_t duration);

/**
 * @brief Register the hardware interrupt service routine of the lw MACL component.
 */
void lw_mac802154_lw_macl_hw_isr_register(void);

/**
 * @brief Initialize the lw MACL component.
 * @return API_RESULT indicating the result of the operation.
 */
API_RESULT lw_mac802154_lw_macl_init_pl(void);

/**
 * @brief Register the interrupt service routine for the lw MACL component.
 * @param isr_handler Pointer to the interrupt service routine.
 * @return API_RESULT indicating the result of the operation.
 */
API_RESULT lw_mac802154_lw_macl_register_isr_pl(LW_MACL_ISR_HANDLER isr_handler);

/**
 * @brief Get the current channel of the lw MAC component.
 * @param channel Pointer to store the current channel value.
 * @return API_RESULT indicating the result of the operation.
 */
API_RESULT lw_mac802154_lw_mac_get_cur_channel_pl(uint8_t *channel);

/**
 * @brief Set the channel of the lw MAC component.
 * @param channel Channel value to be set.
 * @return API_RESULT indicating the result of the operation.
 */
API_RESULT lw_mac802154_lw_mac_set_channel_pl(uint8_t channel);

/**
 * @brief Read the THREAD_CTRL_CONFIG register.
 * @return Value of the THREAD_CTRL_CONFIG register.
 */
uint16_t lw_mac802154_le_read_THRAD_CTRL_CONFIG(void);

/**
 * @brief Write a value to the THREAD_CTRL_CONFIG register.
 * @param value Value to be written to the register.
 */
void lw_mac802154_le_write_THREAD_CTRL_CONFIG(uint16_t value);

/**
 * @brief Write the stop command.
 */
void lw_mac802154_le_write_command_rx_stop(void);

/**
 * @brief Get the MAC PAN ID.
 * @return Value of the MAC PAN ID.
 */
uint16_t lw_mac802154_get_mac_pan_id(void);

/**
 * @brief Set the MAC PAN ID.
 * @param pan_id Value of the MAC PAN ID to be set.
 */
void lw_mac802154_set_mac_pan_id(uint16_t pan_id);

/**
 * @brief Get the short address of the MAC.
 * @return Value of the short address.
 */
uint16_t lw_mac802154_get_short_address(void);

/**
 * @brief Set the short address of the MAC.
 * @param short_address Value of the short address to be set.
 */
void lw_mac802154_set_short_address(uint16_t short_address);

/**
 * @brief Get the 0th part of the extended MAC address.
 * @return Value of the 0th part of the extended MAC address.
 */
uint16_t lw_mac802154_get_mac_extnd_addr0(void);

/**
 * @brief Get the 2nd part of the extended MAC address.
 * @return Value of the 2nd part of the extended MAC address.
 */
uint16_t lw_mac802154_get_mac_extnd_addr2(void);

/**
 * @brief Get the 1st part of the extended MAC address.
 * @return Value of the 1st part of the extended MAC address.
 */
uint16_t lw_mac802154_get_mac_extnd_addr1(void);

/**
 * @brief Get the 3rd part of the extended MAC address.
 * @return Value of the 3rd part of the extended MAC address.
 */
uint16_t lw_mac802154_get_mac_extnd_addr3(void);

/**
 * @brief Set the 0th part of the extended MAC address.
 * @param extnd_addr0 Value of the 0th part of the extended MAC address to be set.
 */
void lw_mac802154_set_mac_extnd_addr0(uint16_t extnd_addr0);

/**
 * @brief Set the 1st part of the extended MAC address.
 * @param extnd_addr1 Value of the 1st part of the extended MAC address to be set.
 */
void lw_mac802154_set_mac_extnd_addr1(uint16_t extnd_addr1);

/**
 * @brief Set the 2nd part of the extended MAC address.
 * @param extnd_addr2 Value of the 2nd part of the extended MAC address to be set.
 */
void lw_mac802154_set_mac_extnd_addr2(uint16_t extnd_addr2);

/**
 * @brief Set the 3rd part of the extended MAC address.
 * @param extnd_addr3 Value of the 3rd part of the extended MAC address to be set.
 */
void lw_mac802154_set_mac_extnd_addr3(uint16_t extnd_addr3);

/**
 * @brief Get the CCA threshold value.
 * @return Value of the CCA threshold.
 */
uint16_t lw_mac802154_get_cca_threshold(void);

/**
 * @brief Set the CCA threshold value.
 * @param cca_threshold Value of the CCA threshold to be set.
 */
void lw_mac802154_set_cca_threshold(uint16_t cca_threshold);

/**
 * @brief Extended configuration for the receive function of the lw MACL component.
 * @param enable Enable or disable the receive function.
 * @return API_RESULT indicating the result of the operation.
 */
API_RESULT lw_mac802154_lw_macl_rx_config_pl_ext(uint8_t enable);

/**
 * @brief Start the Energy Detection (ED) operation of the lw MACL component.
 * @param duration Duration of the ED operation.
 * @param context ED operation context.
 * @return API_RESULT indicating the result of the operation.
 */
API_RESULT lw_mac802154_lw_macl_start_ed_pl(uint32_t duration, uint8_t context);

/**
 * @brief Convert RSSI value to LQI value.
 * @param rssi RSSI value to be converted.
 * @return Converted LQI value.
 */
uint8_t lw_mac802154_lw_mac_convert_rssi_to_lqi(uint8_t rssi);

/**
 * @brief Write the command to stop continuous receive.
 */
void lw_mac802154_write_command_register_cont_rx_stop(void);

/**
 * @brief Disable the thread hardware filter.
 */
void lw_mac802154_thread_hw_filter_disable(void);

/**
 * @brief Start the thread continuous receive function.
 */
void lw_mac802154_thread_continuous_rx_start(void);

/**
 * @brief Start the thread receive function.
 */
void lw_mac802154_thread_rx_start(void);

/**
 * @brief Write the command to start Energy Detection (ED).
 */
void lw_mac802154_write_command_register_ed_start(void);

/**
 * @brief Start transmission of a MAC frame with ACK in the start phase.
 * @param mac_frame Pointer to the MAC frame to be transmitted.
 * @param mac_frame_len Length of the MAC frame.
 * @param context Transmission context.
 * @return API_RESULT indicating the result of the operation.
 */
API_RESULT lw_mac802154_lw_macl_tx_frame_pl_ack_start_tx(uint8_t *mac_frame, uint16_t mac_frame_len, uint8_t context);

/**
 * @brief Transmit the payload of a MAC frame with ACK.
 * @param mac_frame Pointer to the MAC frame to be transmitted.
 * @param mac_frame_len Length of the MAC frame.
 * @param context Transmission context.
 * @return API_RESULT indicating the result of the operation.
 */
API_RESULT lw_mac802154_lw_macl_tx_frame_pl_ack_payload(uint8_t *mac_frame, uint16_t mac_frame_len, uint8_t context);

/**
 * @brief Read the REG0xCF_AGC_CTRL0 register.
 * @return Value of the REG0xCF_AGC_CTRL0 register.
 */
uint16_t lw_mac802154_read_register_REG0xCF_AGC_CTRL0(void);

/**
 * @brief Write a value to the REG0xCF_AGC_CTRL0 register.
 * @param value Value to be written to the register.
 */
void lw_mac802154_write_register_REG0xCF_AGC_CTRL0(uint16_t value);

/**
 * @brief Read the thread interrupt status register.
 * @return Value of the thread interrupt status register.
 */
uint16_t lw_mac802154_read_register_thread_intr_status(void);

/**
 * @brief Write a value to the thread interrupt status register.
 * @param value Value to be written to the register.
 */
void lw_mac802154_write_register_thread_intr_status_clear(uint16_t value);

/**
 * @brief Retrieve the value of the thread MAC receive status register specifically
 *        related to the receive carrier sense (RX CS) status.
 *
 * This function is used to obtain the current status of the receive carrier sense
 * from the thread MAC receive status register. The returned value can be used to
 * determine the carrier sensing result during the receive operation.
 *
 * @return uint16_t The value of the thread MAC receive status register for receive carrier sense.
 */
uint16_t lw_mac802154_get_register_thread_mac_rx_status_rx_cs(void);

/**
 * @brief Stop the thread transmission operation.
 */
void lw_mac802154_thread_tx_stop(void);

/**
 * @brief Stop the thread Energy Detection (ED) scan operation.
 */
void lw_mac802154_thread_ed_scan_stop(void);

/**
 * @brief Stop the thread timer.
 */
void lw_mac802154_thread_timer_stop(void);

/**
 * @brief Stop the hardware timer of the lw MACL component.
 * This function is used to stop the hardware timer that was previously started
 * by the lw MACL component. It can be used to halt ongoing timer operations
 * and clean up any associated resources.
 * @return API_RESULT indicating the result of the operation.
 */
API_RESULT lw_mac802154_lw_macl_stop_hw_timer_pl(void);

/**
 * @brief Retrieve the value of the thread MAC receive status register.
 * This function is used to obtain the current status of the thread MAC receive operation.
 * The returned value can be used to determine the receive status information.
 * @return uint16_t The value of the thread MAC receive status register.
 */
uint16_t lw_mac802154_get_register_thread_mac_rx_status(void);

/**
 * @brief Retrieve the value of the thread MAC transmit status register.
 * This function is used to obtain the current status of the thread MAC transmit operation.
 * The returned value can be used to determine the transmit status information.
 * @return uint16_t The value of the thread MAC transmit status register.
 */
uint16_t lw_mac802154_get_register_thread_mac_tx_status(void);

/**
 * @brief Retrieve the value of the thread interrupt enable register.
 * This function reads the thread interrupt enable register to obtain the current
 * interrupt enable status. Each bit in the returned value corresponds to a specific
 * thread interrupt, allowing the caller to determine which interrupts are enabled.
 * @return uint16_t The value of the thread interrupt enable register.
 */
uint16_t lw_mac802154_get_register_thread_intr_en(void);

/**
 * @brief Set the value of the thread interrupt enable register.
 * This function writes the provided value to the thread interrupt enable register.
 * Each bit in the input value corresponds to a specific thread interrupt, allowing
 * the caller to enable or disable specific interrupts.
 * @param val The value to be written to the thread interrupt enable register.
 */
void lw_mac802154_set_register_thread_intr_en(uint16_t val);

/**
 * @brief Perform a soft reset on the thread component.
 * 
 * This function is used to perform a soft reset operation on the thread component.
 * A soft reset typically clears the internal state of the component while maintaining
 * the current hardware configuration, allowing the thread component to be reinitialized
 * and resume normal operation.
 */
void lw_mac802154_thread_soft_reset(void);

/**
 * @brief Enable the functionality for detecting when the Receive Start of Frame Delimiter (RX SFD) is matched.
 *
 * This function enables the hardware or software mechanism responsible for identifying
 * when the RX SFD pattern has been successfully matched during frame reception.
 * Once enabled, the system can respond to the SFD match event appropriately.
 */
void lw_mac802154_int_rx_sfd_matched_en(void);

// Enable the functionality for detecting when the Transmit Start of Frame Delimiter (TX SFD) is matched.
// This function enables the hardware or software mechanism to identify
// when the TX SFD pattern has been successfully matched during the frame transmission process.
// After enabling, the system can trigger corresponding operations upon detecting the SFD match event,
// which is crucial for ensuring the correct start of frame transmission and maintaining synchronization.
void lw_mac802154_int_tx_sfd_matched_en(void);

// @brief Enable the detection of both Receive and Transmit Start of Frame Delimiter (SFD) match events.
//
// This function activates the mechanisms that monitor for successful matches of the
// RX SFD during frame reception and TX SFD during frame transmission. Once enabled,
// the system can take appropriate actions upon detecting these SFD match events,
// which play a vital role in maintaining proper synchronization and ensuring the
// correct initiation of frame reception and transmission operations.
void lw_mac802154_int_rx_tx_sfd_matched_en(void);

/**
 * @brief Enable or disable automatic TX acknowledgement.
 *
 * @oaram enable Set to 1 to enable, 0 to disable.
 */
void lw_mac802154_lw_macl_set_auto_tx_ack(uint8_t enable);

#endif