#include "can_handler.h"

kfifo_static_t can_rx_fifo;

void InitCAN(can_type *hcan)
{
    kfifo_init_s(&can_rx_fifo, KFIFO_STATIC_BUF_SIZE);

    can_reset(hcan);

    can_base_type can_base_struct =
    {
        .mode_selection = CAN_MODE_COMMUNICATE,
        .ttc_enable = FALSE,
        .aebo_enable = TRUE,   // Automatic exit bus-off
        .aed_enable = TRUE,    // Automatic exit from doze(hibernation)
        .prsf_enable = FALSE,  // PROHIBIT retransmission when sending fails (disable == Auto Retransmission)
        .mdrsel_selection = CAN_DISCARDING_FIRST_RECEIVED,
        .mmssr_selection = CAN_SENDING_BY_REQUEST
    };
    can_base_init(hcan, &can_base_struct);

    can_baudrate_type can_baudrate_struct;
    can_baudrate_default_para_init(&can_baudrate_struct);

    // Using baudcal.exe: Device = CANFDNET_TCP
#ifdef AT32F403Axx // CAN bus on APB1 @ 120 MHz, rsaw(sync jump width) [1,4], bts1(tseg1) [1,16], bts2(tseg2) [1,8]
    can_baudrate_struct.baudrate_div = 10;        // BRP
    can_baudrate_struct.rsaw_size = CAN_RSAW_3TQ; // SJW
    can_baudrate_struct.bts1_size = CAN_BTS1_8TQ; // TSEG1
    can_baudrate_struct.bts2_size = CAN_BTS2_3TQ; // TSEG2
#endif
    can_baudrate_set(hcan, &can_baudrate_struct);

    can_filter_init_type can_filter_init_struct =
    {
        .filter_activate_enable = TRUE,
        .filter_mode = CAN_FILTER_MODE_ID_MASK,
        .filter_fifo = CAN_FILTER_FIFO0,
        .filter_number = 0,
        .filter_bit = CAN_FILTER_32BIT,
        .filter_id_high = 0x0000,
        .filter_id_low = 0x0000,
        .filter_mask_high = 0x0000,
        .filter_mask_low = 0x0000
    };

    can_filter_init(hcan, &can_filter_init_struct);

    can_interrupt_enable(hcan, CAN_RF0MIEN_INT | CAN_ETRIEN_INT | CAN_EOIEN_INT, TRUE);
}

void OnCANIRQ(can_type *hcan)
{
    // if(can_interrupt_flag_get(hcan, CAN_ETR_FLAG) != RESET) // error irq
    if(hcan->ests_bit.etr)
    {
        // can_flag_clear(hcan, CAN_ETR_FLAG);
        hcan->msts = CAN_MSTS_EOIF_VAL;
        hcan->ests = 0;
    }
    // else if(can_interrupt_flag_get(hcan, CAN_RF0MN_FLAG) != RESET)
    else if(hcan->rf0_bit.rf0mn)
    {
        can_rx_message_type rx_msg = {0};
        can_message_receive(hcan, CAN_RX_FIFO0, &rx_msg);

        if(rx_msg.id_type != CAN_ID_EXTENDED || rx_msg.frame_type == CAN_TFT_REMOTE) return;

        CANMessage dt_can_msg = {0};
        dt_can_msg.is_ext = rx_msg.id_type == CAN_ID_EXTENDED;
        dt_can_msg.cob_id = dt_can_msg.is_ext ? rx_msg.extended_id : rx_msg.standard_id;
        dt_can_msg.is_rtr = rx_msg.frame_type == CAN_TFT_REMOTE;
        dt_can_msg.len = (uint8_t)rx_msg.dlc;
        if(dt_can_msg.len > sizeof(dt_can_msg.data)) dt_can_msg.len = sizeof(dt_can_msg.data);
        memcpy(dt_can_msg.data, rx_msg.data, dt_can_msg.len);
        // ProcessIncomingMsg(dt_can_msg);

        // send to kfifo
        kfifo_put_s(&can_rx_fifo, (char*)&dt_can_msg, sizeof(dt_can_msg));
    }
}

uint8_t PeekCANRxFIFO(void)
{
    return kfifo_used_s(&can_rx_fifo) >= sizeof(CANMessage);
}

uint8_t GetCANRxFIFO(CANMessage* dst)
{
    memset(dst, 0, sizeof(CANMessage));
    const uint32_t len = kfifo_get_s(&can_rx_fifo, (char*)dst, sizeof(CANMessage));
    return len == sizeof(CANMessage);
}

FuncRetCode TransmitCANMessage(can_type* hcan, CANMessage* msg)
{
    can_tx_message_type tx_msg = {0};
    if(msg->is_ext)
    {
        tx_msg.standard_id = 0;
        tx_msg.extended_id = msg->cob_id;
        tx_msg.id_type = CAN_ID_EXTENDED;
    }
    else
    {
        tx_msg.standard_id = (uint16_t)msg->cob_id;
        tx_msg.extended_id = 0;
        tx_msg.id_type = CAN_ID_STANDARD;
    }
    tx_msg.frame_type = msg->is_rtr ? CAN_TFT_REMOTE : CAN_TFT_DATA;
    tx_msg.dlc = msg->len;
    memcpy(tx_msg.data, msg->data, msg->len);
    const uint8_t selected_mailbox = can_message_transmit(hcan, &tx_msg);

    if(selected_mailbox == CAN_TX_STATUS_NO_EMPTY)
    {
        return BUFFER_FULL;
    }

    return OK;
}