#include "dronecan.h"
#include <string.h>
#include <stdlib.h>

// DSDL definitions import
// DroneCAN definitions below
#include "../libcanard/dsdl/uavcan/protocol/GetNodeInfo.h"
#include "../libcanard/dsdl/uavcan/protocol/RestartNode.h"
#include "../libcanard/dsdl/uavcan/protocol/param/ExecuteOpcode.h"
#include "../libcanard/dsdl/uavcan/protocol/dynamic_node_id/Allocation.h"
#include "../libcanard/dsdl/uavcan/protocol/file/BeginFirmwareUpdate.h"
#include "../libcanard/dsdl/uavcan/protocol/file/GetInfo.h"
#include "../libcanard/dsdl/uavcan/protocol/file/Read.h"

// uint8_t desired_node_id = 0; // can be only set once, DEPRECATED

#ifndef IFOC_NODE_NAME
#define IFOC_NODE_NAME ("com.ifoc.bootloader")
#endif

uint8_t reboot_flag = 0;

struct uavcan_protocol_NodeStatus node_status;
struct
{
    uint32_t next_dna_request_tick;
    uint32_t id_allocation_uuid_offset;
} dna;

struct
{
    uint32_t heartbeat;
} last_send_tick;

struct
{
    uint8_t uavcan_protocol_nodestatus;
    uint8_t node_allocation;
} next_transfer_id;

fw_update_struct_t fw_update;

static CanardInstance canard;
static uint8_t memory_pool[4096];

void ProcessTransfer(CanardInstance* ins, CanardRxTransfer* transfer);
bool ShouldAccept(const CanardInstance* ins, uint64_t* out,
    uint16_t data_type_id, CanardTransferType transfer_type, uint8_t source_node_id);
void SendNodeStatus(CanardInstance* ins);
void SendGetInfoResponse(CanardInstance* ins, CanardRxTransfer* transfer);
void SendRestartNodeResponse(CanardInstance* ins, CanardRxTransfer* transfer);
void SendFWUpdateResponse(CanardInstance* ins, CanardRxTransfer* transfer);
void OnFileGetInfoResponse(CanardInstance* ins, CanardRxTransfer* transfer);
void OnFileReadResponse(CanardInstance* ins, CanardRxTransfer* transfer);
void OnDNAAllocation(CanardInstance* ins, CanardRxTransfer* transfer);
void RequestDNAAllocation(CanardInstance* ins);
void RequestFileGetInfo(CanardInstance* ins);
void RequestFileRead(CanardInstance* ins);

// Callbacks
FuncRetCode OnFWUpdateGetFileSize(uint32_t size);
FuncRetCode OnFWUpdateReceiveBlock(uint32_t offset, uint8_t* data, uint32_t len);
FuncRetCode OnFWUpdateFinished(void);
FuncRetCode OnRestartNode(void);

void InitDroneCAN(void)
{
    memset(&node_status, 0, sizeof(node_status));
    memset(&dna, 0, sizeof(dna));
    memset(&last_send_tick, 0, sizeof(last_send_tick));
    memset(&next_transfer_id, 0, sizeof(next_transfer_id));
    memset(&fw_update, 0, sizeof(fw_update));
    canardInit(&canard,
        memory_pool,
        sizeof(memory_pool),
        ProcessTransfer,
        ShouldAccept,
        NULL);
    node_status.mode = UAVCAN_PROTOCOL_NODESTATUS_MODE_MAINTENANCE;
    // if(desired_node_id >= CANARD_MIN_NODE_ID && desired_node_id <= CANARD_MAX_NODE_ID) canard.node_id = desired_node_id;
}

void ProcessDroneCAN_RT(void)
{
    // #1: Response received transfer first
    if(PeekCANRxFIFO())
    {
        CANMessage message = {0};
        if(GetCANRxFIFO(&message))
        {
            CanardCANFrame frame = {0};
            frame.id = message.cob_id;
            memcpy(frame.data, message.data, message.len);
            frame.data_len = message.len;
            frame.iface_id = 0;
            canardHandleRxFrame(&canard, &frame, GetCurrentTickCount() * 1000); // process MAX 1 packet per loop
        }
    }
    // #3: Generate Tx packets
    // Continuously write till FuncRetCode::BUFFER_FULL, to get maximum throughput
    CANMessage tx_message = {0};
    tx_message.is_ext = 1;
    tx_message.is_rtr = 0;
    for(const CanardCANFrame* tx_frame = NULL; (tx_frame = canardPeekTxQueue(&canard)) != NULL;)
    {
        tx_message.cob_id = tx_frame->id;
        tx_message.len = tx_frame->data_len;
        if(tx_message.len > sizeof(tx_message.data)) tx_message.len = sizeof(tx_message.data);
        memcpy(tx_message.data, tx_frame->data, tx_message.len);
        const FuncRetCode ret = TransmitCANMessage(CAN0, &tx_message);
        if(ret == OK)
        {
            canardPopTxQueue(&canard);
            continue;
        }
        break;
    }
    if(reboot_flag)
    {
        OnRestartNode();
    }
}

void ProcessDroneCAN_1KHz(void)
{
    node_status.uptime_sec = GetUptimeSeconds(); // update uptime_sec
    const uint8_t anonymous = canard.node_id == CANARD_BROADCAST_NODE_ID || canard.node_id > CANARD_MAX_NODE_ID;
    if(!anonymous)
    {
        uint32_t interval_ms = UAVCAN_PROTOCOL_NODESTATUS_MAX_BROADCASTING_PERIOD_MS;
        if(fw_update.file_server_node_id > 0 && fw_update.file_full_size > 0) interval_ms /= 2; // faster status, smoother VSSC report
        if((GetCurrentTickCount() - last_send_tick.heartbeat) >= interval_ms)
        {
            last_send_tick.heartbeat = GetCurrentTickCount();
            canardCleanupStaleTransfers(&canard, GetCurrentTickCount() * 1000);
            SendNodeStatus(&canard);
        }
    }
    else // waiting for Dynamic Node-ID Allocation (DNA)
    {
        if(GetCurrentTickCount() > dna.next_dna_request_tick)
        {
            RequestDNAAllocation(&canard);
        }
    }
}

void ProcessDroneCAN_10KHz(void)
{
    if(fw_update.error) node_status.health = UAVCAN_PROTOCOL_NODESTATUS_HEALTH_ERROR;
    else
    {
        if(reset_flag.bit.window_wdt_reset || reset_flag.bit.wdt_reset) node_status.health = UAVCAN_PROTOCOL_NODESTATUS_HEALTH_WARNING;
        else node_status.health = UAVCAN_PROTOCOL_NODESTATUS_HEALTH_OK;
    }

    if(fw_update.file_full_size > 0) // vssc represents percentage
    {
        node_status.vendor_specific_status_code = (fw_update.offset * 100) / fw_update.file_full_size;
        if(node_status.vendor_specific_status_code > 100) node_status.vendor_specific_status_code = 100;
    }
    else node_status.vendor_specific_status_code = 0;

    if(fw_update.file_server_node_id >= CANARD_MIN_NODE_ID && fw_update.file_server_node_id <= CANARD_MAX_NODE_ID)
    {
        if(fw_update.file_full_size == 0)
        {
            RequestFileGetInfo(&canard);
            node_status.mode = UAVCAN_PROTOCOL_NODESTATUS_MODE_MAINTENANCE;
        }
        else // start download
        {
            node_status.mode = UAVCAN_PROTOCOL_NODESTATUS_MODE_SOFTWARE_UPDATE;
            RequestFileRead(&canard);
            if(fw_update.last_response_tick > 0 && GetCurrentTickCount() - fw_update.last_response_tick >= 5000)
            {
                fw_update.file_server_node_id = 0; // exit transfer
            }
        }
    }
    else
    {
        node_status.mode = UAVCAN_PROTOCOL_NODESTATUS_MODE_MAINTENANCE;
    }
}

void ProcessTransfer(CanardInstance* ins, CanardRxTransfer* transfer)
{
    switch(transfer->transfer_type)
    {
        case CanardTransferTypeRequest:
        {
            switch(transfer->data_type_id)
            {
                case UAVCAN_PROTOCOL_GETNODEINFO_ID:
                {
                    SendGetInfoResponse(ins, transfer);
                    break;
                }
                case UAVCAN_PROTOCOL_RESTARTNODE_ID:
                {
                    SendRestartNodeResponse(ins, transfer);
                    break;
                }
                case UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_ID:
                {
                    // SendExecuteOpcodeResponse(transfer);
                    break;
                }
                case UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_ID:
                {
                    SendFWUpdateResponse(ins, transfer);
                    break;
                }
                default: break;
            }
            break;
        }
        case CanardTransferTypeResponse:
        {
            switch(transfer->data_type_id)
            {
                case UAVCAN_PROTOCOL_FILE_GETINFO_ID:
                {
                    OnFileGetInfoResponse(ins, transfer);
                    break;
                }
                case UAVCAN_PROTOCOL_FILE_READ_ID:
                {
                    OnFileReadResponse(ins, transfer);
                    break;
                }
                default: break;
            }
            break;
        }
        case CanardTransferTypeBroadcast:
        {
            switch(transfer->data_type_id)
            {
                case UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID:
                {
                    OnDNAAllocation(ins, transfer);
                    break;
                }
                default: break;
            }
            break;
        }
        default: break;
    }
}

bool ShouldAccept(const CanardInstance* ins, uint64_t* out,
    uint16_t data_type_id, CanardTransferType transfer_type, uint8_t source_node_id)
{
    switch(transfer_type)
    {
        case CanardTransferTypeRequest:
        {
            switch(data_type_id)
            {
                case UAVCAN_PROTOCOL_GETNODEINFO_ID:
                {
                    *out = UAVCAN_PROTOCOL_GETNODEINFO_REQUEST_SIGNATURE;
                    return true;
                }
                case UAVCAN_PROTOCOL_RESTARTNODE_ID:
                {
                    *out = UAVCAN_PROTOCOL_RESTARTNODE_REQUEST_SIGNATURE;
                    return true;
                }
                // case UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_ID:
                // {
                //     *out = UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_REQUEST_SIGNATURE;
                //     return true;
                // }
                case UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_ID:
                {
                    *out = UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_REQUEST_SIGNATURE;
                    return true;
                }
                default: break;
            }
            break;
        }
        case CanardTransferTypeResponse:
        {
            switch(data_type_id)
            {
                case UAVCAN_PROTOCOL_FILE_GETINFO_ID:
                {
                    *out = UAVCAN_PROTOCOL_FILE_GETINFO_RESPONSE_SIGNATURE;
                    return true;
                }
                case UAVCAN_PROTOCOL_FILE_READ_ID:
                {
                    *out = UAVCAN_PROTOCOL_FILE_READ_RESPONSE_SIGNATURE;
                    return true;
                }
                default: break;
            }
            break;
        }
        case CanardTransferTypeBroadcast:
        {
            switch(data_type_id)
            {
                case UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID:
                {
                    *out = UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_SIGNATURE;
                    return true;
                }
                default: break;
            }
            break;
        }
        default: break;
    }
    return false;
}

void SendNodeStatus(CanardInstance* ins)
{
    uint8_t buffer[UAVCAN_PROTOCOL_NODESTATUS_MAX_SIZE];
    const uint16_t len = uavcan_protocol_NodeStatus_encode(&node_status, buffer);

    canardBroadcast(ins,
                    UAVCAN_PROTOCOL_NODESTATUS_SIGNATURE,
                    UAVCAN_PROTOCOL_NODESTATUS_ID,
                    &next_transfer_id.uavcan_protocol_nodestatus,
                    CANARD_TRANSFER_PRIORITY_LOW,
                    buffer,
                    len);
}

void SendGetInfoResponse(CanardInstance* ins, CanardRxTransfer* transfer)
{
    uint8_t buffer[UAVCAN_PROTOCOL_GETNODEINFO_RESPONSE_MAX_SIZE];
    struct uavcan_protocol_GetNodeInfoResponse response = {0};
    response.status = node_status;
    response.software_version.major = (uint8_t)YEAR();
    response.software_version.minor = (uint8_t)MONTH();
    response.software_version.optional_field_flags = UAVCAN_PROTOCOL_SOFTWAREVERSION_OPTIONAL_FIELD_FLAG_VCS_COMMIT |
                                                     UAVCAN_PROTOCOL_SOFTWAREVERSION_OPTIONAL_FIELD_FLAG_IMAGE_CRC;
    response.software_version.vcs_commit = (uint32_t)dec_to_hex_byte(MONTH()) << 24 |
                                           (uint32_t)dec_to_hex_byte(DAY()) << 16 |
                                           (uint32_t)dec_to_hex_byte(HOUR()) << 8 |
                                           (uint32_t)dec_to_hex_byte(MINUTE()) << 0;
    response.software_version.image_crc = GetFirmwareCRC64();
    response.hardware_version.major = 1;
    response.hardware_version.minor = 0;
    const uint32_t sn = GetSerialNumber();
    memcpy(response.hardware_version.unique_id, &sn, sizeof(sn));
    response.name.len = strlen(IFOC_NODE_NAME);
    memcpy(response.name.data, IFOC_NODE_NAME, response.name.len);

    const uint16_t total_size = uavcan_protocol_GetNodeInfoResponse_encode(&response, buffer);

    canardReleaseRxTransferPayload(ins, transfer);
    canardRequestOrRespond(ins,
                           transfer->source_node_id,
                           UAVCAN_PROTOCOL_GETNODEINFO_SIGNATURE,
                           UAVCAN_PROTOCOL_GETNODEINFO_ID,
                           &transfer->transfer_id,
                           transfer->priority,
                           CanardResponse,
                           buffer,
                           total_size);
}

void SendRestartNodeResponse(CanardInstance* ins, CanardRxTransfer* transfer)
{
    struct uavcan_protocol_RestartNodeRequest request = {0};
    if(uavcan_protocol_RestartNodeRequest_decode(transfer, &request)) return;
    const bool is_restart_valid = request.magic_number == UAVCAN_PROTOCOL_RESTARTNODE_REQUEST_MAGIC_NUMBER;

    struct uavcan_protocol_RestartNodeResponse response = {0};
    response.ok = is_restart_valid;

    uint8_t buffer[UAVCAN_PROTOCOL_RESTARTNODE_RESPONSE_MAX_SIZE];
    const uint32_t len = uavcan_protocol_RestartNodeResponse_encode(&response, buffer);

    canardReleaseRxTransferPayload(&canard, transfer);
    canardRequestOrRespond(&canard,
                           transfer->source_node_id,
                           UAVCAN_PROTOCOL_RESTARTNODE_RESPONSE_SIGNATURE,
                           UAVCAN_PROTOCOL_RESTARTNODE_RESPONSE_ID,
                           &transfer->transfer_id,
                           transfer->priority,
                           CanardResponse,
                           buffer,
                           len);

    if(is_restart_valid) reboot_flag = 1;
}

void SendFWUpdateResponse(CanardInstance* ins, CanardRxTransfer* transfer)
{
    struct uavcan_protocol_file_BeginFirmwareUpdateRequest request = {0};
    if(uavcan_protocol_file_BeginFirmwareUpdateRequest_decode(transfer, &request)) return;
    const uint32_t path_len = request.image_file_remote_path.path.len;

    struct uavcan_protocol_file_BeginFirmwareUpdateResponse response = {0};

    // reject path_len != sizeof(fw_update.path) (7)
    static_assert(sizeof(fw_update.path) == 7, "");
    if(path_len != sizeof(fw_update.path))
    {
        response.error = UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_RESPONSE_ERROR_UNKNOWN;
        goto response;
    }

    // check for repeated request, with same node_id & path
    if(fw_update.file_server_node_id == request.source_node_id &&
        memcmp(fw_update.path, request.image_file_remote_path.path.data, path_len) == 0)
    {
        // if ongoing
        if(fw_update.file_full_size > 0 && fw_update.offset < fw_update.file_full_size)
        {
            response.error = UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_RESPONSE_ERROR_IN_PROGRESS;
            goto response;
        }
    }

    // reset fw_update with given information, Read trigger: file_server_node_id != 0 && file_full_size > 0
    // GetInfo trigger: file_server_node_id != 0 && file_full_size == 0
    fw_update.file_server_node_id = 0;
    memset(&fw_update, 0, sizeof(fw_update));
    memcpy(fw_update.path, request.image_file_remote_path.path.data, path_len);
    response.error = UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_RESPONSE_ERROR_OK;
    // Send response first, then trigger

    uint8_t buffer[UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_RESPONSE_MAX_SIZE] = {0};
    uint32_t total_size = 0;
response:
    total_size = uavcan_protocol_file_BeginFirmwareUpdateResponse_encode(&response, buffer);

    canardReleaseRxTransferPayload(ins, transfer);
    canardRequestOrRespond(ins,
        transfer->source_node_id,
        UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_SIGNATURE,
        UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_ID,
        &transfer->transfer_id,
        transfer->priority,
        CanardResponse,
        buffer,
        total_size);

    if(response.error == UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_RESPONSE_ERROR_OK)
    {
        // trigger GetInfo
        fw_update.file_server_node_id = request.source_node_id;
    }
}

void OnFileGetInfoResponse(CanardInstance* ins, CanardRxTransfer* transfer)
{
    if((transfer->transfer_id + 1) % 32 != fw_update.transfer_id || transfer->source_node_id != fw_update.file_server_node_id) return; // not for us
    struct uavcan_protocol_file_GetInfoResponse response = {0};
    if(uavcan_protocol_file_GetInfoResponse_decode(transfer, &response)) return;
    if(response.error.value != UAVCAN_PROTOCOL_FILE_ERROR_OK ||
        !(response.entry_type.flags & UAVCAN_PROTOCOL_FILE_ENTRYTYPE_FLAG_FILE) ||
        !(response.entry_type.flags & UAVCAN_PROTOCOL_FILE_ENTRYTYPE_FLAG_READABLE))
    {
        goto reset;
    }
    // check file size validity:
    const uint32_t size = response.size;
    const uint32_t app_size = FLASH_USER_START_ADDR - FLASH_APP_START_ADDR;
    if(size == 0 || size >= app_size) goto reset;
    // file size valid.
    if(OnFWUpdateGetFileSize(size) == OK)
    {
        fw_update.transfer_id = 0;
        fw_update.last_requested_tick = 0;
        fw_update.file_full_size = size;
        return;
    }
reset:
    fw_update.file_server_node_id = 0;
    memset(&fw_update, 0, sizeof(fw_update));
}

void OnDNAAllocation(CanardInstance* ins, CanardRxTransfer* transfer)
{
    if(ins->node_id != CANARD_BROADCAST_NODE_ID) return;

    const uint32_t current_tick = GetCurrentTickCount();
    srand(current_tick); // update random seed
    dna.next_dna_request_tick = current_tick + UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS +
                                (rand() % UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_FOLLOWUP_DELAY_MS);

    if(transfer->source_node_id == CANARD_BROADCAST_NODE_ID) // received another anonymous node's request, invalid
    {
        dna.id_allocation_uuid_offset = 0;
        return;
    }

    struct uavcan_protocol_dynamic_node_id_Allocation msg = {0};
    if(uavcan_protocol_dynamic_node_id_Allocation_decode(transfer, &msg)) return;

    uint8_t uuid_buffer[sizeof(msg.unique_id.data)] = {0};
    const uint32_t serial_number = GetSerialNumber();
    memcpy(uuid_buffer, &serial_number, sizeof(serial_number));

    if(memcmp(msg.unique_id.data, uuid_buffer, sizeof(msg.unique_id.data)) != 0) // uuid mismatch
    {
        dna.id_allocation_uuid_offset = 0;
        return;
    }

    if(msg.unique_id.len < sizeof(msg.unique_id.data))
    {
        // The allocator has confirmed part of the UUID, switching to the next stage.
        dna.id_allocation_uuid_offset = msg.unique_id.len;
        dna.next_dna_request_tick -= UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS;
    }
    else
    {
        // allocation complete
        ins->node_id = msg.node_id;
    }
}

void RequestDNAAllocation(CanardInstance* ins)
{
    const uint32_t current_tick = GetCurrentTickCount();
    srand(current_tick); // update random seed
    dna.next_dna_request_tick = current_tick + UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS +
                                (rand() % UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_FOLLOWUP_DELAY_MS);
#if CANARD_ENABLE_CANFD
    uint8_t allocation_request[CANARD_CANFD_FRAME_MAX_DATA_LEN - 1] = {0};
#else
    uint8_t allocation_request[CANARD_CAN_FRAME_MAX_DATA_LEN - 1] = {0};
#endif

    // if(desired_node_id >= CANARD_MIN_NODE_ID && desired_node_id <= CANARD_MAX_NODE_ID) allocation_request[0] = (desired_node_id << 1U);
    // else allocation_request[0] = ((CANARD_MAX_NODE_ID - 1) << 1U);

    allocation_request[0] = ((CANARD_MAX_NODE_ID - 1) << 1U);

    if(dna.id_allocation_uuid_offset == 0) allocation_request[0] |= 1; // first part of uuid

    uint8_t uuid_buffer[16] = {0};
    const uint32_t serial_number = GetSerialNumber();
    memcpy(uuid_buffer, &serial_number, sizeof(serial_number));

    uint8_t uid_size = (uint8_t)(sizeof(uuid_buffer) - dna.id_allocation_uuid_offset);

#if !CANARD_ENABLE_CANFD
    // static constexpr uint8_t MAX_LEN_OF_UUID_IN_SINGLE_REQUEST = 6;
    if(uid_size > 6) uid_size = 6;
#endif

    memmove(&allocation_request[1], &uuid_buffer[dna.id_allocation_uuid_offset], uid_size);

    canardBroadcast(ins,
    UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_SIGNATURE,
    UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID,
    &next_transfer_id.node_allocation,
    CANARD_TRANSFER_PRIORITY_LOW,
    &allocation_request[0],
    (uint16_t)(uid_size + 1));

    dna.id_allocation_uuid_offset = 0; // Preparing for timeout; if response is received, this value will be updated from the callback.
}

void RequestFileGetInfo(CanardInstance* ins)
{
    if(fw_update.file_server_node_id >= CANARD_MIN_NODE_ID && fw_update.file_server_node_id <= CANARD_MAX_NODE_ID)
    {
        if(fw_update.file_full_size == 0) // request
        {
            if(GetCurrentTickCount() - fw_update.last_requested_tick > 100)
            {
                struct uavcan_protocol_file_GetInfoRequest request = {0};
                memcpy(request.path.path.data, fw_update.path, sizeof(fw_update.path));
                request.path.path.len = sizeof(fw_update.path);
                uint8_t buffer[UAVCAN_PROTOCOL_FILE_GETINFO_REQUEST_MAX_SIZE] = {0};
                const uint16_t size = uavcan_protocol_file_GetInfoRequest_encode(&request, buffer);

                canardRequestOrRespond(ins,
                    fw_update.file_server_node_id,
                    UAVCAN_PROTOCOL_FILE_GETINFO_REQUEST_SIGNATURE,
                    UAVCAN_PROTOCOL_FILE_GETINFO_REQUEST_ID,
                    &fw_update.transfer_id,
                    CANARD_TRANSFER_PRIORITY_MEDIUM,
                    CanardRequest,
                    buffer,
                    size);

                fw_update.last_requested_tick = GetCurrentTickCount();
            }
        }
    }
}

void RequestFileRead(CanardInstance* ins)
{
    if(fw_update.file_server_node_id >= CANARD_MIN_NODE_ID && fw_update.file_server_node_id <= CANARD_MAX_NODE_ID)
    {
        if(fw_update.file_full_size > 0)
        {
            if(fw_update.last_requested_offset != fw_update.offset || GetCurrentTickCount() - fw_update.last_requested_tick > 100)
            {
                uint8_t buffer[UAVCAN_PROTOCOL_FILE_READ_REQUEST_MAX_SIZE] = {0};
                struct uavcan_protocol_file_ReadRequest request = {0};
                request.path.path.len = sizeof(fw_update.path);
                memcpy(request.path.path.data, fw_update.path, sizeof(fw_update.path));
                request.offset = fw_update.offset;
                const uint16_t total_size = uavcan_protocol_file_ReadRequest_encode(&request, buffer);
                canardRequestOrRespond(ins,
                    fw_update.file_server_node_id,
                    UAVCAN_PROTOCOL_FILE_READ_REQUEST_SIGNATURE,
                    UAVCAN_PROTOCOL_FILE_READ_REQUEST_ID,
                    &fw_update.transfer_id,
                    CANARD_TRANSFER_PRIORITY_HIGH,
                    CanardRequest,
                    buffer,
                    total_size);
                fw_update.last_requested_tick = GetCurrentTickCount();
                fw_update.last_requested_offset = fw_update.offset;
            }
        }
    }
}

void OnFileReadResponse(CanardInstance* ins, CanardRxTransfer* transfer)
{
    if((transfer->transfer_id + 1) % 32 != fw_update.transfer_id || transfer->source_node_id != fw_update.file_server_node_id) return; // not for us
    if(fw_update.file_full_size == 0) return; // no write if size is not determined
    struct uavcan_protocol_file_ReadResponse response = {0};
    if(uavcan_protocol_file_ReadResponse_decode(transfer, &response)) return;
    if(response.error.value != UAVCAN_PROTOCOL_FILE_ERROR_OK)
    {
        fw_update.file_server_node_id = 0;
        memset(&fw_update, 0, sizeof(fw_update));
        fw_update.error = 1;
        return;
    }
    // write...

    OnFWUpdateReceiveBlock(fw_update.offset, response.data.data, response.data.len);

    // write successful
    fw_update.offset += response.data.len;
    if(fw_update.offset >= fw_update.file_full_size) // upgrade done
    {
        fw_update.file_server_node_id = 0;
        fw_update.error = OnFWUpdateFinished() != OK;
        // memset(&fw_update, 0, sizeof(fw_update));
    }
    else
    {
        fw_update.last_response_tick = GetCurrentTickCount();
    }
}