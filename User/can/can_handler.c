#include "can_handler.h"
#include <stdbool.h>

kfifo_static_t can_rx_fifo;

#ifdef GD32G5X3
// Classic CAN mode
uint32_t FILTER_FORMAT_AND_NUMBER = CAN_RXFIFO_FILTER_A_NUM_8;
uint8_t FIRST_AVAIL_TX_MAILBOX_IDX = 8;
uint8_t LAST_AVAIL_TX_MAILBOX_IDX = 31;
#endif

void InitCAN(uint32_t hcan)
{
    kfifo_init_s(&can_rx_fifo, KFIFO_STATIC_BUF_SIZE);

    can_deinit(hcan);

    can_parameter_struct can_param;
    can_struct_para_init(CAN_INIT_STRUCT, &can_param);

    can_param.internal_counter_source = CAN_TIMER_SOURCE_BIT_CLOCK;
    can_param.self_reception = DISABLE;
    can_param.mb_tx_order = CAN_TX_LOW_NUM_MB_FIRST;
    can_param.mb_tx_abort_enable = ENABLE;
    can_param.local_priority_enable = DISABLE;
    can_param.mb_rx_ide_rtr_type = CAN_IDE_RTR_FILTERED;
    can_param.mb_remote_frame = CAN_STORE_REMOTE_REQUEST_FRAME;
    can_param.rx_private_filter_queue_enable = ENABLE; // RPFQEN = 1, we will have separate mask & id for each filter
    can_param.edge_filter_enable = ENABLE; // EFDIS = 0, See User Manual P1321
    can_param.protocol_exception_enable = DISABLE;
    can_param.rx_filter_order = CAN_RX_FILTER_ORDER_FIFO_FIRST; // Rx FIFO first (mailbox not used)
    can_param.memory_size = CAN_MEMSIZE_32_UNIT; // 512 BYTES
    can_param.mb_public_filter = 0U;

#ifdef GD32G5X3
    can_param.prescaler = 6U; // BRP
    can_param.resync_jump_width = 9U; // SJW
    can_param.prop_time_segment = 13U;
    can_param.time_segment_1 = 13U; // time_segment_1 + pts = TSEG1
    can_param.time_segment_2 = 9U;
#else
#warning "CAN baudrate calculation result not set for current platform!"
#endif

    can_init(hcan, &can_param);

    can_auto_busoff_recovery_enable(hcan);
    can_time_sync_disable(hcan);
    can_bsp_mode_config(hcan, CAN_BSP_MODE_THREE_SAMPLES);
    can_bsp_syn_config(hcan, CAN_BSP_TWO_STAGES_SYN);

    // Rx FIFO & Filter settings
    can_fifo_parameter_struct can_fifo_param;
    can_struct_para_init(CAN_FIFO_INIT_STRUCT, &can_fifo_param);

    can_fifo_param.dma_enable = DISABLE;
    can_fifo_param.filter_format_and_number = CAN_RXFIFO_FILTER_A_NUM_8; // Maximum number of 8 filters
    can_fifo_param.fifo_public_filter = 0; // use private mask

    can_rx_fifo_config(hcan, &can_fifo_param);

    /* Mailbox RAM (512 bytes) split in Classic CAN Mode:
     * each mailbox is 16bytes, with a total of 32
     * Rx FIFO: [0:5]
     * FILTER (8, each 4 bytes -> 2 mailbox): [6:7]
     * Tx Mailbox (usable): [8:31] with a total of 24 mailbox.
     */

    // initialize tx mailbox
    for(uint8_t i = FIRST_AVAIL_TX_MAILBOX_IDX; i <= LAST_AVAIL_TX_MAILBOX_IDX; i++)
    {
        can_mailbox_transmit_inactive(hcan, i);
        CAN_STAT(hcan) = STAT_MS(i);
        (void)(CAN_TIMER(hcan));
    }

    can_interrupt_enable(hcan, CAN_INT_FIFO_AVAILABLE);

    can_operation_mode_enter(hcan, CAN_NORMAL_MODE);
}

void OnCANIRQ(uint32_t hcan)
{
    while(can_interrupt_flag_get(hcan, CAN_INT_FLAG_FIFO_AVAILABLE))
    {
        can_rx_fifo_struct rx_msg;
        can_rx_fifo_read(hcan, &rx_msg);
        CANMessage msg = {0};
        msg.is_ext = (rx_msg.ide != 0);
        msg.cob_id = rx_msg.id;
        msg.is_rtr = (rx_msg.rtr != 0);
        msg.len = msg.is_rtr ? 0 : rx_msg.dlc;
        if(msg.len > sizeof(msg.data)) msg.len = sizeof(msg.data);
        memcpy(msg.data, rx_msg.data, msg.len);
        // ProcessIncomingMsg(msg);
        kfifo_put_s(&can_rx_fifo, (char*)&msg, sizeof(msg));
        // can_interrupt_flag_clear(hcan, CAN_INT_FLAG_FIFO_AVAILABLE);
        (void)CAN_TIMER(hcan);
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

bool IsMailboxEmpty(uint32_t hcan, const uint8_t mb)
{
    if(CAN_STAT(hcan) & STAT_MS(mb))
    {
        CAN_STAT(hcan) = STAT_MS(mb);
        (void)CAN_TIMER(hcan);
    }
    const uint32_t status_code = can_mailbox_code_get(hcan, mb);
    if(status_code == CAN_MB_TX_STATUS_INACTIVE || status_code == CAN_MB_TX_STATUS_ABORT) return true;
    return false;
}

FuncRetCode TransmitCANMessage(uint32_t hcan, CANMessage* msg)
{
    for(uint8_t i = FIRST_AVAIL_TX_MAILBOX_IDX; i <= LAST_AVAIL_TX_MAILBOX_IDX; i++)
    {
        if(!IsMailboxEmpty(hcan, i)) continue;
        can_mailbox_descriptor_struct desc;
        can_struct_para_init(CAN_MDSC_STRUCT, &desc);
        desc.ide = msg->is_ext;
        desc.rtr = msg->is_rtr;
        desc.id = msg->cob_id;
        desc.prio = 0U;
        desc.data_bytes = msg->is_rtr ? 0U : msg->len;
        desc.code = CAN_MB_TX_STATUS_DATA;
        memcpy(desc.data, msg->data, desc.data_bytes);
        can_mailbox_config(hcan, i, &desc);
        return OK;
    }
    return BUFFER_FULL;
}