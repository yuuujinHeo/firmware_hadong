


#include "lan2can.h"
#include "config_cocoa_general.h"
#include "function_utilities.h"
#include <stdio.h>
#include <math.h>
#include <proc/p32mz1024ech064.h>



LAN2CAN_DATA gv;


CUP_DISPENSER_INFO      CUP_DATA[MAX_CUP];
ICE_DISPENSER_INFO      ICE_DATA[MAX_ICE];
SODA_DISPENSER_INFO     SODA_DATA[MAX_SODA];
HOT_DISPENSER_INFO      HOT_DATA[MAX_HOT];
ICECREAM_DISPENSER_INFO ICECREAM_DATA[MAX_ICECREAM];
SYRUP_DISPENSER_INFO    SYRUP_DATA[MAX_SYRUP];
BARCODE_INFO            BARCODE_DATA[MAX_BARCODE];
OUTLET_INFO             OUTLET_DATA[MAX_OUTLET];
MILK_INFO               MILK_DATA[MAX_MILK];
PLATFORM_INFO           PLATFORM_DATA[MAX_PLATFORM];


unsigned int cup_connection_count[MAX_CUP] = {0,};
unsigned int ice_connection_count[MAX_ICE] = {0,};
unsigned int soda_connection_count[MAX_SODA] = {0,};
unsigned int hot_connection_count[MAX_HOT] = {0,};
unsigned int icecream_connection_count[MAX_ICECREAM] = {0,};
unsigned int syrup_connection_count[MAX_SYRUP] = {0,};
unsigned int barcode_connection_count[MAX_BARCODE] = {0,};
unsigned int outlet_connection_count[MAX_OUTLET] = {0,};
unsigned int milk_connection_count[MAX_MILK] = {0,};
unsigned int platform_connection_count[MAX_PLATFORM] = {0,};


void LAN2CAN_Initialize(void) {
    uint8_t tempIndex = 0;

    // LAN chip disable
    LAN2CAN_LANChipOff();

    // Initialize state machine
    gv.state = LAN2CAN_STATE_INIT;

    // Initialize system timer handle
    gv.sysTaskHandle = SYS_TMR_HANDLE_INVALID;
    gv.sysTmrUserHandle = SYS_TMR_HANDLE_INVALID;
    gv.sysTmrPCHandle = SYS_TMR_HANDLE_INVALID;
    gv.lanData.sysTmrLANCheckHandle = SYS_TMR_HANDLE_INVALID;
    gv.lanData.checkCount = 0;

    // Initialize CAN
    gv.canData.sysTmrCANOutHandle = SYS_TMR_HANDLE_INVALID;
    for (tempIndex = 0; tempIndex < 2; tempIndex++) {
        gv.canData.canHandle[tempIndex] = DRV_HANDLE_INVALID;
        gv.canData.canRxCount[tempIndex] = 0;
    }
    
    INIT_PINS();   
    
    OUTLET_DATA[0].velocity[0] = 70;
    OUTLET_DATA[0].velocity[1] = 50;
    OUTLET_DATA[1].velocity[0] = 70;
    OUTLET_DATA[1].velocity[1] = 50;

}

void LAN2CAN_Reset(void) {
    uint8_t tempIndex = 0;

    gv.sysTmrPCHandle = SYS_TMR_HANDLE_INVALID;

    // Reset CAN
    gv.canData.sysTmrCANOutHandle = SYS_TMR_HANDLE_INVALID;
    for (tempIndex = 0; tempIndex < 2; tempIndex++) {
        if (gv.canData.canHandle[tempIndex] != DRV_HANDLE_INVALID) {
            DRV_CAN_Close(gv.canData.canHandle[tempIndex]);
            gv.canData.canHandle[tempIndex] = DRV_HANDLE_INVALID;
        }
        gv.canData.canRxCount[tempIndex] = 0;
    }
    gv.canData.canHandle[0] = DRV_CAN_Open(DRV_CAN_INDEX_0, DRV_IO_INTENT_READWRITE | DRV_IO_INTENT_NONBLOCKING | DRV_IO_INTENT_EXCLUSIVE);
    gv.canData.canHandle[1] = DRV_CAN_Open(DRV_CAN_INDEX_1, DRV_IO_INTENT_READWRITE | DRV_IO_INTENT_NONBLOCKING | DRV_IO_INTENT_EXCLUSIVE);
    // Reset LAN
    LAN2CAN_LANChipOn();
}

int lan_connection_delay = 0;
void LAN2CAN_Tasks(void) {
    
    // Task
    LAN2CAN_TaskFunction();
    
    
    switch (gv.state) {
        case LAN2CAN_STATE_INIT:
        {
            // Reset peripheral device
            LAN2CAN_Reset();

            // Wait for the TCPIP Stack to become initialized                    
            if (TCPIP_STACK_Status(sysObj.tcpip) == SYS_STATUS_READY) {
                // now that the stack is ready we can check the
                // available interfaces
                if (TCPIP_STACK_NumberOfNetworksGet() > 0) {
                    // Get a handle to the network interface
                    // For this application only one interface is used
                    gv.lanData.netHandle = TCPIP_STACK_IndexToNet(0);
                }
                gv.state = LAN2CAN_STATE_TCPIP_WAIT_FOR_LINK;
            }
            break;
        }

        case LAN2CAN_STATE_TCPIP_WAIT_FOR_LINK:
        {
            if (TCPIP_STACK_NetIsLinked(gv.lanData.netHandle) == true) {
                // If network is linked than get IP address
                gv.lanData.ipAddr.Val = TCPIP_STACK_NetAddress(gv.lanData.netHandle);
                gv.state = LAN2CAN_STATE_SETUP_TCP_SERVER;
            }
            break;
        }

        case LAN2CAN_STATE_SETUP_TCP_SERVER:
        {
            gv.lanData.socketHandle = TCPIP_TCP_ServerOpen(IP_ADDRESS_TYPE_IPV4, SERVER_PORT, 0);
            if (gv.lanData.socketHandle != INVALID_SOCKET) {
                gv.state = LAN2CAN_STATE_WAIT_FOR_CONNECTION;

                // dummy check for reseting previous reset check
                TCPIP_TCP_WasReset(gv.lanData.socketHandle);
            }
            break;
        }

        case LAN2CAN_STATE_WAIT_FOR_CONNECTION:
        {
            lan_connection_delay = 500;
            // Waiting client connection
            if (TCPIP_TCP_IsConnected(gv.lanData.socketHandle)) {
                // Clear TCPIP Rx Buffer
                TCPIP_TCP_Discard(gv.lanData.socketHandle);

                // We got a connection
                gv.state = LAN2CAN_STATE_SERVING_CONNECTION;

                // CAN Buffer clear
//                LAN2CAN_CANClearBuffer();
            }
            break;
        }

        case LAN2CAN_STATE_SERVING_CONNECTION:
        {
            // Client is connected.
            // Now we can receive and transmit data
            if (!TCPIP_TCP_IsConnected(gv.lanData.socketHandle)) {
                gv.state = LAN2CAN_STATE_CLOSING_CONNECTION;  
            } else {
                // Check CAN message from RCR
                LAN2CAN_CANReceiveCheckFromRCR();
                // Send CAN message to the Main controller
                LAN2CAN_SendToPC();
                // Check LAN data
                LAN2CAN_LANDataParsing();
                // Check LAN connection
//                LAN2CAN_LANCheckConnection();
            }

            break;
        }

        case LAN2CAN_STATE_CLOSING_CONNECTION:
        {
            // Close the socket connection.
            TCPIP_TCP_Close(gv.lanData.socketHandle);
            gv.state = LAN2CAN_STATE_SETUP_TCP_SERVER;
            break;
        }

        case LAN2CAN_STATE_ERROR:
        {
            // Re-initialize the program.
            // When we lost connection with the Main controller and don't know the reason,
            // then we need to reset the program.
            LAN2CAN_Initialize();
            break;
        }

        default:
            break;
    }
    
}

int LAN2CAN_LANChipOff(void) {
//    PLIB_PORTS_PinClear(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_11);
    PLIB_PORTS_PinClear(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_13);
    return GPIF_NO_ERR;
}

int LAN2CAN_LANChipOn(void) {
//    PLIB_PORTS_PinSet(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_11);
    PLIB_PORTS_PinSet(PORTS_ID_0, PORT_CHANNEL_B, PORTS_BIT_POS_13);
    return GPIF_NO_ERR;
}

int LAN2CAN_LANDataParsing(void) {
    uint16_t receivedDataSize = 0;
    uint16_t tempSize = 0;
    uint8_t tempData[3];
    UINT16_UNION packetDataSize;


    tempSize = TCPIP_TCP_GetIsReady(gv.lanData.socketHandle);

    if (tempSize > 3) {
        TCPIP_TCP_ArrayPeek(gv.lanData.socketHandle, tempData, 3, 0);
        if (tempData[0] != 0x24) {
//            LAN2CAN_DEBUGMonitoring("Wrong LAN header.\n");
            TCPIP_TCP_Discard(gv.lanData.socketHandle);
            return GPIF_ERR1;
        }

        packetDataSize.UINT8[0] = tempData[1];
        packetDataSize.UINT8[1] = tempData[2];

        if (TCPIP_TCP_GetIsReady(gv.lanData.socketHandle) < packetDataSize.UINT16 + 3) {
            return GPIF_ERR2;
        }

        // Transfer the data out of the RX FIFO and into our local processing buffer.
        receivedDataSize = TCPIP_TCP_ArrayGet(gv.lanData.socketHandle, gv.lanData.msgFromClient, packetDataSize.UINT16 + 3);

        // Check packet footer
        if (gv.lanData.msgFromClient[receivedDataSize - 1] != 0x25) {
            // Wrong footer
//            LAN2CAN_DEBUGMonitoring("Wrong LAN footer.\n");
            return GPIF_ERR3;
        }

        // Packet is perfect, so we can parsing the packet.
        // Reset LAN check count
        gv.lanData.checkCount = 0;
        if (receivedDataSize == 4) {
            //            TCPIP_TCP_ArrayPut(gv.lanData.socketHandle, gv.lanData.msgToClient, currentIndex);
            //            TCPIP_TCP_Flush(gv.lanData.socketHandle);
            return GPIF_NO_ERR;
        }


        if(gv.lanData.msgFromClient[5] == 0x01) {
            // cup dispenser
            int target = gv.lanData.msgFromClient[6];
            int command = gv.lanData.msgFromClient[7];
//            int para1 = ((int)(gv.lanData.msgFromClient[8]) | (int)(gv.lanData.msgFromClient[9]<<8) | (int)(gv.lanData.msgFromClient[10]<<16) | (int)(gv.lanData.msgFromClient[11]<<24));
//            int para2 = ((int)(gv.lanData.msgFromClient[12]) | (int)(gv.lanData.msgFromClient[13]<<8) | (int)(gv.lanData.msgFromClient[14]<<16) | (int)(gv.lanData.msgFromClient[15]<<24));
            
            if(command == 0){
                // dispense cup
                Cup_DispenseCup(target);
            }
        }
        else if(gv.lanData.msgFromClient[5] == 0x02) {
            // ice dispenser
            int target = gv.lanData.msgFromClient[6];
            int command = gv.lanData.msgFromClient[7];
            int para1 = ((int)(gv.lanData.msgFromClient[8]) | (int)(gv.lanData.msgFromClient[9]<<8) | (int)(gv.lanData.msgFromClient[10]<<16) | (int)(gv.lanData.msgFromClient[11]<<24));
            int para2 = ((int)(gv.lanData.msgFromClient[12]) | (int)(gv.lanData.msgFromClient[13]<<8) | (int)(gv.lanData.msgFromClient[14]<<16) | (int)(gv.lanData.msgFromClient[15]<<24));
            
            if(command == 0){
                // dispense ice
                Ice_DispenseIce(target, para1, para2);
            }else if(command == 1){
                // comm mode
                Ice_SettingCommunicationMode(target, para1, para2);
            }else if(command == 2){
                // status clear
                Ice_ClearStatus(target);
            }else if(command == 3){
                // ice timeout 
                Ice_SettingTimeout(target, para1);
            }else if(command == 4){
                // reboot
                Ice_Reboot(target);
            }else if(command == 5){
                // ambient temperature
                Ice_SettingAmbientTemp(target, para1, para2);
            }
        }
        else if(gv.lanData.msgFromClient[5] == 0x03) {
            // syrup dispenser
            int target = gv.lanData.msgFromClient[6];
            int command = gv.lanData.msgFromClient[7];
            int para1 = ((int)(gv.lanData.msgFromClient[8]) | (int)(gv.lanData.msgFromClient[9]<<8) | (int)(gv.lanData.msgFromClient[10]<<16) | (int)(gv.lanData.msgFromClient[11]<<24));
            int para2 = ((int)(gv.lanData.msgFromClient[12]) | (int)(gv.lanData.msgFromClient[13]<<8) | (int)(gv.lanData.msgFromClient[14]<<16) | (int)(gv.lanData.msgFromClient[15]<<24));
            
            if(command == 0){
                // dispense syrup by time
                Syrup_DispenseByTime(target, para1, para2);
            }else if(command == 1){
                // dispense syrup by weight
                Syrup_DispenseByWeight(target, para1, para2);
            }else if(command == 2){
                // tare
                Syrup_Tare(target);
            }else if(command == 3){
                // set calib param
                Syrup_SetCalibParam(target, para1, para2);
            }
        }
        else if(gv.lanData.msgFromClient[5] == 0x04) {
            // barcode
            int target = gv.lanData.msgFromClient[6];
            int command = gv.lanData.msgFromClient[7];
            int para1 = ((int)(gv.lanData.msgFromClient[8]) | (int)(gv.lanData.msgFromClient[9]<<8) | (int)(gv.lanData.msgFromClient[10]<<16) | (int)(gv.lanData.msgFromClient[11]<<24));
            int para2 = ((int)(gv.lanData.msgFromClient[12]) | (int)(gv.lanData.msgFromClient[13]<<8) | (int)(gv.lanData.msgFromClient[14]<<16) | (int)(gv.lanData.msgFromClient[15]<<24));
            
            if(command == 0){
                Barcode_RequestLastBarcode(target);
            }
        }
        else if(gv.lanData.msgFromClient[5] == 0x05){
            // outlet
            int target = gv.lanData.msgFromClient[6];
            int command = gv.lanData.msgFromClient[7];
            int para1 = ((int)(gv.lanData.msgFromClient[8]) | (int)(gv.lanData.msgFromClient[9]<<8) | (int)(gv.lanData.msgFromClient[10]<<16) | (int)(gv.lanData.msgFromClient[11]<<24));
            int para2 = ((int)(gv.lanData.msgFromClient[12]) | (int)(gv.lanData.msgFromClient[13]<<8) | (int)(gv.lanData.msgFromClient[14]<<16) | (int)(gv.lanData.msgFromClient[15]<<24));
            
            if(command == 0){
                // Find Home All
                MotorController_FindHome(0, 0);
                MotorController_FindHome(0, 1);
                MotorController_FindHome(1, 0);
                MotorController_FindHome(1, 1);
            }else if(command == 1){
                // Find Home
                OUTLET_DATA[target].open_position[para1] = para2;
                MotorController_FindHome(target, para1);
            }else if(command == 2){
                // Go to Position
                MotorController_GoToPosition(target, para1, para2);
            }else if(command == 3){
                // Set Velocity
                OUTLET_DATA[target].velocity[para1] = para2;
            }else if(command == 4){
                // Enc Zero
                MotorController_EncoderZero(target, para1);
            }
        }
        else if(gv.lanData.msgFromClient[5] == 0x06) {
            // soda dispenser
            int target = gv.lanData.msgFromClient[6];
            int command = gv.lanData.msgFromClient[7];
            int para1 = ((int)(gv.lanData.msgFromClient[8]) | (int)(gv.lanData.msgFromClient[9]<<8) | (int)(gv.lanData.msgFromClient[10]<<16) | (int)(gv.lanData.msgFromClient[11]<<24));
            int para2 = ((int)(gv.lanData.msgFromClient[12]) | (int)(gv.lanData.msgFromClient[13]<<8) | (int)(gv.lanData.msgFromClient[14]<<16) | (int)(gv.lanData.msgFromClient[15]<<24));
            
            if(command == 0){
                // dispense syrup by time
                Soda_DispenseByTime(para1, para2);
            }else if(command == 1){
                // dispense syrup by weight
                Soda_DispenseByWeight(para1, para2);
            }else if(command == 2){
                // tare
                Soda_Tare();
            }else if(command == 3){
                // set calib param
                Soda_SetCalibParam(para1, para2);
            }
        }
        else if(gv.lanData.msgFromClient[5] == 0x07) {
            // hot dispenser
            int target = gv.lanData.msgFromClient[6];
            int command = gv.lanData.msgFromClient[7];
            int para1 = ((int)(gv.lanData.msgFromClient[8]) | (int)(gv.lanData.msgFromClient[9]<<8) | (int)(gv.lanData.msgFromClient[10]<<16) | (int)(gv.lanData.msgFromClient[11]<<24));
            int para2 = ((int)(gv.lanData.msgFromClient[12]) | (int)(gv.lanData.msgFromClient[13]<<8) | (int)(gv.lanData.msgFromClient[14]<<16) | (int)(gv.lanData.msgFromClient[15]<<24));
            
            if(command == 0){
                // dispense syrup by time
                Hot_DispenseByTime(para1, para2);
            }else if(command == 1){
                // dispense syrup by weight
                Hot_DispenseByWeight(para1, para2);
            }else if(command == 2){
                // tare
                Hot_Tare();
            }else if(command == 3){
                // set calib param
                Hot_SetCalibParam(para1, para2);
            }
        }
        else if(gv.lanData.msgFromClient[5] == 0x08) {
            // icecream dispenser
            int target = gv.lanData.msgFromClient[6];
            int command = gv.lanData.msgFromClient[7];
            int para1 = ((int)(gv.lanData.msgFromClient[8]) | (int)(gv.lanData.msgFromClient[9]<<8) | (int)(gv.lanData.msgFromClient[10]<<16) | (int)(gv.lanData.msgFromClient[11]<<24));
            int para2 = ((int)(gv.lanData.msgFromClient[12]) | (int)(gv.lanData.msgFromClient[13]<<8) | (int)(gv.lanData.msgFromClient[14]<<16) | (int)(gv.lanData.msgFromClient[15]<<24));
            
            if(command == 0){
                // dispense syrup by time
                Icecream_DispenseByTime(para1, para2);
            }else if(command == 1){
                // dispense syrup by weight
                Icecream_DispenseByWeight(para1, para2);
            }else if(command == 2){
                // tare
                Icecream_Tare();
            }else if(command == 3){
                // set calib param
                Icecream_SetCalibParam(para1, para2);
            }
        }
    }

    return GPIF_NO_ERR;
}

int LAN2CAN_LANCheckConnection(void) {
    // Set 1msec system timer
    if (gv.lanData.sysTmrLANCheckHandle == SYS_TMR_HANDLE_INVALID) {
        gv.lanData.sysTmrLANCheckHandle = SYS_TMR_DelayMS(10);
        return GPIF_NO_ERR;
    }
    if (SYS_TMR_DelayStatusGet(gv.lanData.sysTmrLANCheckHandle) == false) {
        return GPIF_NO_ERR;
    }
    gv.lanData.sysTmrLANCheckHandle = SYS_TMR_DelayMS(10);
    gv.lanData.checkCount++;

    if(lan_connection_delay > 0)
        lan_connection_delay--;
    
    // 100*10 = 1000msec = 1sec
    if (gv.lanData.checkCount > 60) {//ORIGINAL 200
        gv.lanData.checkCount = 0;
        gv.state = LAN2CAN_STATE_CLOSING_CONNECTION;
        //gv.state = GPIF_STATE_ERROR;
    }

    return GPIF_NO_ERR;
}


int LAN2CAN_CANReceiveCheckFromRCR(void) {
    CAN_RX_MSG_BUFFER *can_msg;

    // CAN channel 0
    if ((PLIB_CAN_ChannelEventGet(CAN_ID_1, CAN_CHANNEL1) & CAN_RX_CHANNEL_NOT_EMPTY) == CAN_RX_CHANNEL_NOT_EMPTY) {
        /* Get a pointer to RX message buffer */
        can_msg = (CAN_RX_MSG_BUFFER *) PLIB_CAN_ReceivedMessageGet(CAN_ID_1, CAN_CHANNEL1);
        int id = can_msg->msgSID.sid;
        
        if(id >= 0x110 && id < 0x120){
            // ice dispenser
            int ice_id = id - 0x110;
            if(ice_id < MAX_ICE) {
                ice_connection_count[ice_id] = 0;
                if(can_msg->data[0] == 0x00){
                    ICE_DATA[ice_id].data_connection = can_msg->data[1];
                    ICE_DATA[ice_id].ice_out_count = can_msg->data[2];
                    ICE_DATA[ice_id].status1 = can_msg->data[3];
                    ICE_DATA[ice_id].status2 = can_msg->data[4];
                    // dummy ICE_DATA[ice_id]. = can_msg->data[5];
                    ICE_DATA[ice_id].last_ice_out_time = can_msg->data[6];
                    ICE_DATA[ice_id].last_water_out_time = can_msg->data[7];
                }else if(can_msg->data[0] == 0x01){
                    ICE_DATA[ice_id].comm_mode = can_msg->data[1]&0x01;
                    ICE_DATA[ice_id].cup_ignore = (can_msg->data[1]>>1)&0x01;
                    ICE_DATA[ice_id].setting_ambient_temp_high = can_msg->data[2];
                    ICE_DATA[ice_id].setting_ambient_temp_low = can_msg->data[3];
                    ICE_DATA[ice_id].ambient_temp = can_msg->data[4];
                    ICE_DATA[ice_id].evaporator_temp = can_msg->data[5];
                    ICE_DATA[ice_id].condensor_temp = can_msg->data[6];
                    ICE_DATA[ice_id].timeout_sec = can_msg->data[7];
                }
            }
        }
        else if(id >= 0x130 && id < 0x140){
            // cup dispenser
            int cup_id = id - 0x130;
            if(cup_id < MAX_CUP) {
                cup_connection_count[cup_id] = 0;
                CUP_DATA[cup_id].ultra_sonic_data = can_msg->data[0];
                CUP_DATA[cup_id].status = can_msg->data[1];
                CUP_DATA[cup_id].cup_out_count = can_msg->data[2];
                CUP_DATA[cup_id].last_error = can_msg->data[3];
                CUP_DATA[cup_id].dispense_op_flag = can_msg->data[4];
            }
        }
        else if(id == 0x151){
            // soda dispenser
            int soda_id = 0;
            soda_connection_count[soda_id] = 0;
            SODA_DATA[soda_id].loadcell[0].value = (short)((can_msg->data[0]) | ((can_msg->data[1])<<8));
            SODA_DATA[soda_id].loadcell[0].error = can_msg->data[2];
            SODA_DATA[soda_id].loadcell[0].calib_param = (short)((can_msg->data[3]) | ((can_msg->data[4])<<8));
            SODA_DATA[soda_id].out_state = can_msg->data[5];
            SODA_DATA[soda_id].out_count = can_msg->data[6];
            SODA_DATA[soda_id].last_error = can_msg->data[7];
        }
        else if(id == 0x152){
            // hot dispenser
            int hot_id = 0;
            hot_connection_count[hot_id] = 0;
            if(can_msg->data[0] == 0){
                HOT_DATA[hot_id].loadcell[0].value = (short)((can_msg->data[1]) | ((can_msg->data[2])<<8));
                HOT_DATA[hot_id].loadcell[0].error = can_msg->data[3];
                HOT_DATA[hot_id].loadcell[0].calib_param = (short)((can_msg->data[4]) | ((can_msg->data[5])<<8));
                HOT_DATA[hot_id].loadcell[1].value = (short)((can_msg->data[6]) | ((can_msg->data[7])<<8));
            }else if(can_msg->data[0] == 1){
                HOT_DATA[hot_id].loadcell[1].error = can_msg->data[1];
                HOT_DATA[hot_id].loadcell[1].calib_param = (short)((can_msg->data[2]) | ((can_msg->data[3])<<8));
                HOT_DATA[hot_id].out_state = can_msg->data[4];
                HOT_DATA[hot_id].out_count = can_msg->data[5];
                HOT_DATA[hot_id].last_error = can_msg->data[6];
            }
            
        }
        else if(id == 0x153){
            // icecream dispenser
            int icecream_id = 0;
            icecream_connection_count[icecream_id] = 0;
            if(can_msg->data[0] == 0){
                ICECREAM_DATA[icecream_id].loadcell[0].value = (short)((can_msg->data[1]) | ((can_msg->data[2])<<8));
                ICECREAM_DATA[icecream_id].loadcell[0].error = can_msg->data[3];
                ICECREAM_DATA[icecream_id].loadcell[0].calib_param = (short)((can_msg->data[4]) | ((can_msg->data[5])<<8));
                ICECREAM_DATA[icecream_id].loadcell[1].value = (short)((can_msg->data[6]) | ((can_msg->data[7])<<8));
            }else if(can_msg->data[0] == 1){
                ICECREAM_DATA[icecream_id].loadcell[1].error = can_msg->data[1];
                ICECREAM_DATA[icecream_id].loadcell[1].calib_param = (short)((can_msg->data[2]) | ((can_msg->data[3])<<8));
                ICECREAM_DATA[icecream_id].out_state = can_msg->data[4];
                ICECREAM_DATA[icecream_id].out_count = can_msg->data[5];
                ICECREAM_DATA[icecream_id].last_error = can_msg->data[6];
            }
        }
        else if(id >= 0x170 && id < 0x180){
            // syrup dispenser
            int syrup_id = id - 0x170;
            if(syrup_id < MAX_SYRUP) {
                syrup_connection_count[syrup_id] = 0;
                SYRUP_DATA[syrup_id].loadcell[0].value = (short)((can_msg->data[0]) | ((can_msg->data[1])<<8));
                SYRUP_DATA[syrup_id].loadcell[0].error = can_msg->data[2];
                SYRUP_DATA[syrup_id].loadcell[0].calib_param = (short)((can_msg->data[3]) | ((can_msg->data[4])<<8));
                SYRUP_DATA[syrup_id].out_state = can_msg->data[5];
                SYRUP_DATA[syrup_id].out_count = can_msg->data[6];
                SYRUP_DATA[syrup_id].last_error = can_msg->data[7];
            }
        }
        else if(id >= 0x210 && id < 0x220){
            // barcode
            int barcode_id = id - 0x210;
            if(barcode_id < MAX_BARCODE) {
                barcode_connection_count[barcode_id] = 0;
                if(can_msg->data[0] != 255){
                    if(can_msg->data[1] == 0){
                        BARCODE_DATA[barcode_id].barcode_data[0] = can_msg->data[2];
                        BARCODE_DATA[barcode_id].barcode_data[1] = can_msg->data[3];
                        BARCODE_DATA[barcode_id].barcode_data[2] = can_msg->data[4];
                        BARCODE_DATA[barcode_id].barcode_data[3] = can_msg->data[5];
                        BARCODE_DATA[barcode_id].barcode_data[4] = can_msg->data[6];
                        BARCODE_DATA[barcode_id].barcode_data[5] = can_msg->data[7];
                    }else if(can_msg->data[1] == 1){
                        BARCODE_DATA[barcode_id].barcode_data[6] = can_msg->data[2];
                        BARCODE_DATA[barcode_id].barcode_data[7] = can_msg->data[3];
                        BARCODE_DATA[barcode_id].barcode_data[8] = can_msg->data[4];
                        BARCODE_DATA[barcode_id].barcode_data[9] = can_msg->data[5];
                        BARCODE_DATA[barcode_id].barcode_data[10] = can_msg->data[6];
                        BARCODE_DATA[barcode_id].barcode_data[11] = can_msg->data[7];
                    }else if(can_msg->data[1] == 2){
                        BARCODE_DATA[barcode_id].barcode_data[12] = can_msg->data[2];
                        BARCODE_DATA[barcode_id].barcode_data[13] = can_msg->data[3];
                        BARCODE_DATA[barcode_id].barcode_data[14] = can_msg->data[4];
                        BARCODE_DATA[barcode_id].barcode_data[15] = can_msg->data[5];
                        BARCODE_DATA[barcode_id].barcode_data[16] = can_msg->data[6];
                        BARCODE_DATA[barcode_id].barcode_data[17] = can_msg->data[7];
                    }else if(can_msg->data[1] == 3){
                        BARCODE_DATA[barcode_id].barcode_data[18] = can_msg->data[2];
                        BARCODE_DATA[barcode_id].barcode_data[19] = can_msg->data[3];
                        BARCODE_DATA[barcode_id].barcode_data[20] = can_msg->data[4];
                        BARCODE_DATA[barcode_id].barcode_data[21] = can_msg->data[5];
                        BARCODE_DATA[barcode_id].barcode_data[22] = can_msg->data[6];
                        BARCODE_DATA[barcode_id].barcode_data[23] = can_msg->data[7];

                        BARCODE_DATA[barcode_id].barcode_count = can_msg->data[0];
                    }
                }
            }
        }
        else if(id == 0x350){
            // milk weight
            if(can_msg->data[0] < 4){
                milk_connection_count[0] = 0;
                MILK_DATA[0].loadcell_raw[can_msg->data[0]] = (can_msg->data[1]) | ((can_msg->data[2]) << 8) | ((can_msg->data[3]) << 16) | ((can_msg->data[4]) << 24);
            }
        }
        else if(id == 0x410){
            // platform sensor
            platform_connection_count[0] = 0;
            PLATFORM_DATA[0].sensorA = can_msg->data[0];
            PLATFORM_DATA[0].sensorB = can_msg->data[1];
        }
        else if(id >= 0x50 && id < 0x60){
            // outlet -- encoder
            int mc = id-0x50;
            int outlet_id = mc/2;
            if(outlet_id < MAX_OUTLET){
                outlet_connection_count[outlet_id] = 0;

                int enc = (can_msg->data[0]) | ((can_msg->data[1]) << 8) | ((can_msg->data[2]) << 16);
                if (enc & 0x800000) {
                    enc |= (0xFF000000);
                }            
                short cur = (can_msg->data[3]) | ((can_msg->data[4]) << 8);
                unsigned char stat = can_msg->data[5];
                unsigned char status = (stat & 0b10111101); // init op --> set zero for not cofusing
                unsigned char op_status;
                unsigned char init_status;
                if(stat&0b01000000){
                    op_status = 1;
                }else{
                    op_status = 0;
                }
                if(stat&0b00000010){
                    init_status = 1;
                }else{
                    init_status = 0;
                }            

                if(mc%2 == 0){
                    // SPIN
                    OUTLET_DATA[outlet_id].position[0] = enc;
                    OUTLET_DATA[outlet_id].current[0] = cur;
                    OUTLET_DATA[outlet_id].status[0] = status;
                    OUTLET_DATA[outlet_id].operating_status[0] = op_status;
                    OUTLET_DATA[outlet_id].init_status[0] = init_status;
                }else{
                    // DOOR
                    OUTLET_DATA[outlet_id].position[1] = enc;
                    OUTLET_DATA[outlet_id].current[1] = cur;
                    OUTLET_DATA[outlet_id].status[1] = status;
                    OUTLET_DATA[outlet_id].operating_status[1] = op_status;
                    OUTLET_DATA[outlet_id].init_status[1] = init_status;
                }
                OUTLET_DATA[outlet_id].sonar_sensor_data = can_msg->data[6];
                OUTLET_DATA[outlet_id].sonar_sensor_error = (can_msg->data[7]>>7)&0x01;
            }
        }
        
        PLIB_CAN_ChannelUpdate(CAN_ID_1, CAN_CHANNEL1);
    }
    return GPIF_NO_ERR;
}


int LAN2CAN_SendToPC(void) {
    // Set 200msec system timer
    if (gv.sysTmrPCHandle == SYS_TMR_HANDLE_INVALID) {
        gv.sysTmrPCHandle = SYS_TMR_DelayMS(100);
        return;
    }

    if (SYS_TMR_DelayStatusGet(gv.sysTmrPCHandle) == false) {
        return;
    }
    gv.sysTmrPCHandle = SYS_TMR_DelayMS(100);
    
    
    uint16_t currentIndex = 0;
    uint16_t dataSize = 0;

    int i;
     
    // Cup Dispenser --------------------------------------------------------------------------------------
    gv.lanData.msgToClient[currentIndex] = 0x24;        currentIndex++;
    // Calculate data size
    dataSize = 4 + 6*MAX_CUP;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize);        currentIndex++;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize >> 8);   currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x01;       currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x00;       currentIndex++;
    // Data type: Cup Dispenser (0xA1)
    gv.lanData.msgToClient[currentIndex] = 0xA1;       currentIndex++;

   
    for(i=0; i<MAX_CUP; i++){
        gv.lanData.msgToClient[currentIndex] = CUP_DATA[i].connection_status;   currentIndex++;
        gv.lanData.msgToClient[currentIndex] = CUP_DATA[i].ultra_sonic_data;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = CUP_DATA[i].status;              currentIndex++;
        gv.lanData.msgToClient[currentIndex] = CUP_DATA[i].cup_out_count;       currentIndex++;
        gv.lanData.msgToClient[currentIndex] = CUP_DATA[i].last_error;          currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = CUP_DATA[i].dispense_op_flag;    currentIndex++;
    }
    gv.lanData.msgToClient[currentIndex] = 0x25;
    currentIndex++;
    // ----------------------------------------------------------------------------------------------------
    
    // Ice Dispenser --------------------------------------------------------------------------------------
    gv.lanData.msgToClient[currentIndex] = 0x24;        currentIndex++;
    // Calculate data size
    dataSize = 4 + 15*MAX_ICE;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize);        currentIndex++;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize >> 8);   currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x01;       currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x00;       currentIndex++;
    // Data type: Ice Dispenser (0xA2)
    gv.lanData.msgToClient[currentIndex] = 0xA2;       currentIndex++;

    for(i=0; i<MAX_ICE; i++){
        gv.lanData.msgToClient[currentIndex] = ICE_DATA[i].connection_status;           currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ICE_DATA[i].data_connection;             currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ICE_DATA[i].comm_mode;                   currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ICE_DATA[i].cup_ignore;                  currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ICE_DATA[i].setting_ambient_temp_high;   currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = ICE_DATA[i].setting_ambient_temp_low;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ICE_DATA[i].ambient_temp;                currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ICE_DATA[i].evaporator_temp;             currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ICE_DATA[i].condensor_temp;              currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ICE_DATA[i].timeout_sec;                 currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = ICE_DATA[i].last_ice_out_time;           currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ICE_DATA[i].last_water_out_time;         currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ICE_DATA[i].status1;                     currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ICE_DATA[i].status2;                     currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ICE_DATA[i].ice_out_count;               currentIndex++;
    }
    gv.lanData.msgToClient[currentIndex] = 0x25;
    currentIndex++;
    // ----------------------------------------------------------------------------------------------------
    
    // Syrup Dispenser -------------------------------------------------------------------------------------
    gv.lanData.msgToClient[currentIndex] = 0x24;        currentIndex++;
    // Calculate data size
    dataSize = 4 + 9*MAX_SYRUP;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize);        currentIndex++;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize >> 8);   currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x01;       currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x00;       currentIndex++;
    // Data type: Syrup Dispenser (0xA3)
    gv.lanData.msgToClient[currentIndex] = 0xA3;       currentIndex++;

    for(i=0; i<MAX_SYRUP; i++){
        gv.lanData.msgToClient[currentIndex] = SYRUP_DATA[i].connection_status;                      currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (SYRUP_DATA[i].loadcell[0].value)&0xFF;               currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((SYRUP_DATA[i].loadcell[0].value)>>8)&0xFF;          currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (SYRUP_DATA[i].loadcell[0].calib_param)&0xFF;         currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((SYRUP_DATA[i].loadcell[0].calib_param)>>8)&0xFF;    currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = SYRUP_DATA[i].loadcell[0].error;                     currentIndex++;
        gv.lanData.msgToClient[currentIndex] = SYRUP_DATA[i].out_state;                             currentIndex++;
        gv.lanData.msgToClient[currentIndex] = SYRUP_DATA[i].out_count;                             currentIndex++;
        gv.lanData.msgToClient[currentIndex] = SYRUP_DATA[i].last_error;                            currentIndex++;
    }
    gv.lanData.msgToClient[currentIndex] = 0x25;
    currentIndex++;
    // ----------------------------------------------------------------------------------------------------
        
    // Barcode --------------------------------------------------------------------------------------------
    gv.lanData.msgToClient[currentIndex] = 0x24;        currentIndex++;
    // Calculate data size
    dataSize = 4 + (2+MAX_BARCODE_DATA)*MAX_BARCODE;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize);        currentIndex++;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize >> 8);   currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x01;       currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x00;       currentIndex++;
    // Data type: Barcode (0xA4)
    gv.lanData.msgToClient[currentIndex] = 0xA4;       currentIndex++;

   
    for(i=0; i<MAX_BARCODE; i++){
        gv.lanData.msgToClient[currentIndex] = BARCODE_DATA[i].connection_status;   currentIndex++;
        gv.lanData.msgToClient[currentIndex] = BARCODE_DATA[i].barcode_count;    currentIndex++;
        int j;
        for(j=0; j<MAX_BARCODE_DATA; j++){
            gv.lanData.msgToClient[currentIndex] = BARCODE_DATA[i].barcode_data[j]; currentIndex++;
        }
    }
    gv.lanData.msgToClient[currentIndex] = 0x25;
    currentIndex++;
    // ----------------------------------------------------------------------------------------------------
    
    
    // Outlet ---------------------------------------------------------------------------------------------
    gv.lanData.msgToClient[currentIndex] = 0x24;        currentIndex++;
    // Calculate data size
    dataSize = 4 + 35*MAX_OUTLET;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize);        currentIndex++;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize >> 8);   currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x01;       currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x00;       currentIndex++;
    // Data type: Outlet (0xA5)
    gv.lanData.msgToClient[currentIndex] = 0xA5;       currentIndex++;

   
    for(i=0; i<MAX_OUTLET; i++){
        gv.lanData.msgToClient[currentIndex] = OUTLET_DATA[i].connection_status;   currentIndex++;
        gv.lanData.msgToClient[currentIndex] = OUTLET_DATA[i].position[0]&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].position[0]>>8)&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].position[0]>>16)&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].position[0]>>24)&0xFF;    currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = OUTLET_DATA[i].position[1]&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].position[1]>>8)&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].position[1]>>16)&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].position[1]>>24)&0xFF;    currentIndex++;        
        gv.lanData.msgToClient[currentIndex] = OUTLET_DATA[i].current[0]&0xFF;    currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].current[0]>>8)&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].current[0]>>16)&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].current[0]>>24)&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = OUTLET_DATA[i].current[1]&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].current[1]>>8)&0xFF;    currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].current[1]>>16)&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].current[1]>>24)&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = OUTLET_DATA[i].status[0];   currentIndex++;
        gv.lanData.msgToClient[currentIndex] = OUTLET_DATA[i].status[1];   currentIndex++;
        gv.lanData.msgToClient[currentIndex] = OUTLET_DATA[i].init_status[0];   currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = OUTLET_DATA[i].init_status[1];   currentIndex++;
        gv.lanData.msgToClient[currentIndex] = OUTLET_DATA[i].operating_status[0];   currentIndex++;
        gv.lanData.msgToClient[currentIndex] = OUTLET_DATA[i].operating_status[1];   currentIndex++;
        gv.lanData.msgToClient[currentIndex] = OUTLET_DATA[i].sonar_sensor_data;   currentIndex++;        
        gv.lanData.msgToClient[currentIndex] = OUTLET_DATA[i].sonar_sensor_error;   currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = OUTLET_DATA[i].open_position[0]&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].open_position[0]>>8)&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].open_position[0]>>16)&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].open_position[0]>>24)&0xFF;    currentIndex++;        
        gv.lanData.msgToClient[currentIndex] = OUTLET_DATA[i].open_position[1]&0xFF;    currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].open_position[1]>>8)&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].open_position[1]>>16)&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (OUTLET_DATA[i].open_position[1]>>24)&0xFF;    currentIndex++;
        gv.lanData.msgToClient[currentIndex] = OUTLET_DATA[i].velocity[0];   currentIndex++;        
        gv.lanData.msgToClient[currentIndex] = OUTLET_DATA[i].velocity[1];   currentIndex++;
    }
    gv.lanData.msgToClient[currentIndex] = 0x25;
    currentIndex++;
    // ----------------------------------------------------------------------------------------------------
    
    // Soda Dispenser -------------------------------------------------------------------------------------
    gv.lanData.msgToClient[currentIndex] = 0x24;        currentIndex++;
    // Calculate data size
    dataSize = 4 + 9*MAX_SODA;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize);        currentIndex++;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize >> 8);   currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x01;       currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x00;       currentIndex++;
    // Data type: Soda Dispenser (0xA6)
    gv.lanData.msgToClient[currentIndex] = 0xA6;       currentIndex++;

    for(i=0; i<MAX_SODA; i++){
        gv.lanData.msgToClient[currentIndex] = SODA_DATA[i].connection_status;                      currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (SODA_DATA[i].loadcell[0].value)&0xFF;               currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((SODA_DATA[i].loadcell[0].value)>>8)&0xFF;          currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (SODA_DATA[i].loadcell[0].calib_param)&0xFF;         currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((SODA_DATA[i].loadcell[0].calib_param)>>8)&0xFF;    currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = SODA_DATA[i].loadcell[0].error;                     currentIndex++;
        gv.lanData.msgToClient[currentIndex] = SODA_DATA[i].out_state;                             currentIndex++;
        gv.lanData.msgToClient[currentIndex] = SODA_DATA[i].out_count;                             currentIndex++;
        gv.lanData.msgToClient[currentIndex] = SODA_DATA[i].last_error;                            currentIndex++;
    }
    gv.lanData.msgToClient[currentIndex] = 0x25;
    currentIndex++;
    // ----------------------------------------------------------------------------------------------------

    // Hot Dispenser -------------------------------------------------------------------------------------
    gv.lanData.msgToClient[currentIndex] = 0x24;        currentIndex++;
    // Calculate data size
    dataSize = 4 + 14*MAX_HOT;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize);        currentIndex++;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize >> 8);   currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x01;       currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x00;       currentIndex++;
    // Data type: Hot Dispenser (0xA7)
    gv.lanData.msgToClient[currentIndex] = 0xA7;       currentIndex++;

    for(i=0; i<MAX_HOT; i++){
        gv.lanData.msgToClient[currentIndex] = HOT_DATA[i].connection_status;                      currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (HOT_DATA[i].loadcell[0].value)&0xFF;               currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((HOT_DATA[i].loadcell[0].value)>>8)&0xFF;          currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (HOT_DATA[i].loadcell[0].calib_param)&0xFF;         currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((HOT_DATA[i].loadcell[0].calib_param)>>8)&0xFF;    currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = HOT_DATA[i].loadcell[0].error;                     currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (HOT_DATA[i].loadcell[1].value)&0xFF;               currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((HOT_DATA[i].loadcell[1].value)>>8)&0xFF;          currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (HOT_DATA[i].loadcell[1].calib_param)&0xFF;         currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((HOT_DATA[i].loadcell[1].calib_param)>>8)&0xFF;    currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = HOT_DATA[i].loadcell[1].error;                     currentIndex++;
        gv.lanData.msgToClient[currentIndex] = HOT_DATA[i].out_state;                             currentIndex++;
        gv.lanData.msgToClient[currentIndex] = HOT_DATA[i].out_count;                             currentIndex++;
        gv.lanData.msgToClient[currentIndex] = HOT_DATA[i].last_error;                            currentIndex++;
    }
    gv.lanData.msgToClient[currentIndex] = 0x25;
    currentIndex++;
    // ----------------------------------------------------------------------------------------------------
    
    // Icecream Dispenser -------------------------------------------------------------------------------------
    gv.lanData.msgToClient[currentIndex] = 0x24;        currentIndex++;
    // Calculate data size
    dataSize = 4 + 14*MAX_ICECREAM;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize);        currentIndex++;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize >> 8);   currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x01;       currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x00;       currentIndex++;
    // Data type: Icecream Dispenser (0xA8)
    gv.lanData.msgToClient[currentIndex] = 0xA8;       currentIndex++;

    for(i=0; i<MAX_ICECREAM; i++){
        gv.lanData.msgToClient[currentIndex] = ICECREAM_DATA[i].connection_status;                      currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (ICECREAM_DATA[i].loadcell[0].value)&0xFF;               currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((ICECREAM_DATA[i].loadcell[0].value)>>8)&0xFF;          currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (ICECREAM_DATA[i].loadcell[0].calib_param)&0xFF;         currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((ICECREAM_DATA[i].loadcell[0].calib_param)>>8)&0xFF;    currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = ICECREAM_DATA[i].loadcell[0].error;                     currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (ICECREAM_DATA[i].loadcell[1].value)&0xFF;               currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((ICECREAM_DATA[i].loadcell[1].value)>>8)&0xFF;          currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (ICECREAM_DATA[i].loadcell[1].calib_param)&0xFF;         currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((ICECREAM_DATA[i].loadcell[1].calib_param)>>8)&0xFF;    currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = ICECREAM_DATA[i].loadcell[1].error;                     currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ICECREAM_DATA[i].out_state;                             currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ICECREAM_DATA[i].out_count;                             currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ICECREAM_DATA[i].last_error;                            currentIndex++;
    }
    gv.lanData.msgToClient[currentIndex] = 0x25;
    currentIndex++;
    // ----------------------------------------------------------------------------------------------------
    
    // Mllk Weight -------------------------------------------------------------------------------------
    gv.lanData.msgToClient[currentIndex] = 0x24;        currentIndex++;
    // Calculate data size
    dataSize = 4 + 17*MAX_MILK;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize);        currentIndex++;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize >> 8);   currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x01;       currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x00;       currentIndex++;
    // Data type: Icecream Dispenser (0xA9)
    gv.lanData.msgToClient[currentIndex] = 0xA9;       currentIndex++;

    for(i=0; i<MAX_MILK; i++){
        gv.lanData.msgToClient[currentIndex] = MILK_DATA[i].connection_status;                  currentIndex++;
        gv.lanData.msgToClient[currentIndex] = (MILK_DATA[i].loadcell_raw[0])&0xFF;             currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((MILK_DATA[i].loadcell_raw[0])>>8)&0xFF;        currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((MILK_DATA[i].loadcell_raw[0])>>16)&0xFF;       currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((MILK_DATA[i].loadcell_raw[0])>>24)&0xFF;       currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = (MILK_DATA[i].loadcell_raw[1])&0xFF;             currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((MILK_DATA[i].loadcell_raw[1])>>8)&0xFF;        currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((MILK_DATA[i].loadcell_raw[1])>>16)&0xFF;       currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((MILK_DATA[i].loadcell_raw[1])>>24)&0xFF;       currentIndex++;        
        gv.lanData.msgToClient[currentIndex] = (MILK_DATA[i].loadcell_raw[2])&0xFF;             currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = ((MILK_DATA[i].loadcell_raw[2])>>8)&0xFF;        currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((MILK_DATA[i].loadcell_raw[2])>>16)&0xFF;       currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((MILK_DATA[i].loadcell_raw[2])>>24)&0xFF;       currentIndex++;        
        gv.lanData.msgToClient[currentIndex] = (MILK_DATA[i].loadcell_raw[3])&0xFF;             currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((MILK_DATA[i].loadcell_raw[3])>>8)&0xFF;        currentIndex++;
        
        gv.lanData.msgToClient[currentIndex] = ((MILK_DATA[i].loadcell_raw[3])>>16)&0xFF;       currentIndex++;
        gv.lanData.msgToClient[currentIndex] = ((MILK_DATA[i].loadcell_raw[3])>>24)&0xFF;       currentIndex++;
    }
    gv.lanData.msgToClient[currentIndex] = 0x25;
    currentIndex++;
    // ----------------------------------------------------------------------------------------------------
    
    // Platform Sensor -------------------------------------------------------------------------------------
    gv.lanData.msgToClient[currentIndex] = 0x24;        currentIndex++;
    // Calculate data size
    dataSize = 4 + 3*MAX_PLATFORM;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize);        currentIndex++;
    gv.lanData.msgToClient[currentIndex] = (uint8_t) (dataSize >> 8);   currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x01;       currentIndex++;
    gv.lanData.msgToClient[currentIndex] = 0x00;       currentIndex++;
    // Data type: Icecream Dispenser (0xAA)
    gv.lanData.msgToClient[currentIndex] = 0xAA;       currentIndex++;

    for(i=0; i<MAX_PLATFORM; i++){
        gv.lanData.msgToClient[currentIndex] = PLATFORM_DATA[i].connection_status;                  currentIndex++;
        gv.lanData.msgToClient[currentIndex] = PLATFORM_DATA[i].sensorA;            currentIndex++;
        gv.lanData.msgToClient[currentIndex] = PLATFORM_DATA[i].sensorB;            currentIndex++;
    }
    gv.lanData.msgToClient[currentIndex] = 0x25;
    currentIndex++;
    // ----------------------------------------------------------------------------------------------------
    
    TCPIP_TCP_ArrayPut(gv.lanData.socketHandle, gv.lanData.msgToClient, currentIndex);
    TCPIP_TCP_Flush(gv.lanData.socketHandle);
    
    return GPIF_NO_ERR;
}

int LAN2CAN_CANClearBuffer(void) {
     // CAN channel 0
    PLIB_CAN_ChannelReset(CAN_ID_1, CAN_CHANNEL0);
    while (PLIB_CAN_ChannelResetIsComplete(CAN_ID_1, CAN_CHANNEL0) != true);
    PLIB_CAN_ChannelReset(CAN_ID_1, CAN_CHANNEL1);
    while (PLIB_CAN_ChannelResetIsComplete(CAN_ID_1, CAN_CHANNEL1) != true);
    gv.canData.canRxCount[0] = 0;

    // CAN channel1
    PLIB_CAN_ChannelReset(CAN_ID_2, CAN_CHANNEL0);
    while (PLIB_CAN_ChannelResetIsComplete(CAN_ID_2, CAN_CHANNEL0) != true);
    PLIB_CAN_ChannelReset(CAN_ID_2, CAN_CHANNEL1);
    while (PLIB_CAN_ChannelResetIsComplete(CAN_ID_2, CAN_CHANNEL1) != true);
    gv.canData.canRxCount[1] = 0;

    return GPIF_NO_ERR;
}




void LAN2CAN_TaskFunction(void){    
    // Loop Timer 100msec
    if (gv.sysTaskHandle == SYS_TMR_HANDLE_INVALID) {
        gv.sysTaskHandle = SYS_TMR_DelayMS(100);
        return;
    }
    if (SYS_TMR_DelayStatusGet(gv.sysTaskHandle) == false) {
        return;
    }
    gv.sysTaskHandle = SYS_TMR_DelayMS(100);
    
    
    // device connection check
    int i;
    for(i=0; i<MAX_CUP; i++){
        cup_connection_count[i]++;
        if(cup_connection_count[i] > 10){
            CUP_DATA[i].connection_status = 0;
        }else{
            CUP_DATA[i].connection_status = 1;
        }
    }
    for(i=0; i<MAX_ICE; i++){
        ice_connection_count[i]++;
        if(ice_connection_count[i] > 10){
            ICE_DATA[i].connection_status = 0;
        }else{
            ICE_DATA[i].connection_status = 1;
        }
    }
    for(i=0; i<MAX_SYRUP; i++){
        syrup_connection_count[i]++;
        if(syrup_connection_count[i] > 10){
            SYRUP_DATA[i].connection_status = 0;
        }else{
            SYRUP_DATA[i].connection_status = 1;
        }
    }
    for(i=0; i<MAX_SODA; i++){
        soda_connection_count[i]++;
        if(soda_connection_count[i] > 10){
            SODA_DATA[i].connection_status = 0;
        }else{
            SODA_DATA[i].connection_status = 1;
        }
    }
    for(i=0; i<MAX_HOT; i++){
        hot_connection_count[i]++;
        if(hot_connection_count[i] > 10){
            HOT_DATA[i].connection_status = 0;
        }else{
            HOT_DATA[i].connection_status = 1;
        }
    }
    for(i=0; i<MAX_ICECREAM; i++){
        icecream_connection_count[i]++;
        if(icecream_connection_count[i] > 10){
            ICECREAM_DATA[i].connection_status = 0;
        }else{
            ICECREAM_DATA[i].connection_status = 1;
        }
    }
    for(i=0; i<MAX_BARCODE; i++){
        barcode_connection_count[i]++;
        if(barcode_connection_count[i] > 10){
            BARCODE_DATA[i].connection_status = 0;
        }else{
            BARCODE_DATA[i].connection_status = 1;
        }
    }
    for(i=0; i<MAX_OUTLET; i++){
        outlet_connection_count[i]++;
        if(outlet_connection_count[i] > 10){
            OUTLET_DATA[i].connection_status = 0;
        }else{
            OUTLET_DATA[i].connection_status = 1;
        }
    }
    for(i=0; i<MAX_MILK; i++){
        milk_connection_count[i]++;
        if(milk_connection_count[i] > 10){
            MILK_DATA[i].connection_status = 0;
        }else{
            MILK_DATA[i].connection_status = 1;
        }
    }
    for(i=0; i<MAX_PLATFORM; i++){
        platform_connection_count[i]++;
        if(platform_connection_count[i] > 10){
            PLATFORM_DATA[i].connection_status = 0;
        }else{
            PLATFORM_DATA[i].connection_status = 1;
        }
    }
}


// cup--------------------------------------------------------------------------
void Cup_DispenseCup(int target){
    int id = 0x120+target;
    unsigned char data[8];
    data[0] = 0;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 1, data);
}

// ice--------------------------------------------------------------------------
void Ice_DispenseIce(int target, int ice_time_ms, int water_time_ms){
    int id = 0x100+target;
    unsigned short ice_time_100ms = ice_time_ms/100;
    unsigned short water_time_100ms = water_time_ms/100;
    unsigned char data[8];
    data[0] = 0;
    data[1] = ice_time_100ms&0xFF;
    data[2] = (ice_time_100ms>>8)&0xFF;
    data[3] = water_time_100ms&0xFF;
    data[4] = (water_time_100ms>>8)&0xFF;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 5, data);
}
void Ice_SettingCommunicationMode(int target, int comm_mode, int cup_ignore){
    int id = 0x100+target;
    unsigned char data[8];
    data[0] = 1;
    data[1] = comm_mode;
    data[2] = cup_ignore;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 3, data);
}
void Ice_ClearStatus(int target){
    int id = 0x100+target;
    unsigned char data[8];
    data[0] = 2;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 1, data);
}
void Ice_SettingTimeout(int target, int timeout){
    int id = 0x100+target;
    unsigned char data[8];
    data[0] = 3;
    data[1] = timeout;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 2, data);
}
void Ice_Reboot(int target){
    int id = 0x100+target;
    unsigned char data[8];
    data[0] = 4;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 1, data);
}
void Ice_SettingAmbientTemp(int target, int tempL, int tempH){
    int id = 0x100+target;
    unsigned char data[8];
    data[0] = 5;
    data[1] = tempL;
    data[2] = tempH;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 3, data);
}

// syrup------------------------------------------------------------------------
void Syrup_DispenseByTime(int target, int type, int param){
    int id = 0x160+target;
    unsigned char data[8];
    data[0] = 0;
    data[1] = type;
    data[2] = param&0xFF;
    data[3] = (param>>8)&0xFF;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 4, data);
}
void Syrup_DispenseByWeight(int target, int type, int param){
    int id = 0x160+target;
    unsigned char data[8];
    data[0] = 1;
    data[1] = type;
    data[2] = param&0xFF;
    data[3] = (param>>8)&0xFF;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 4, data);
}
void Syrup_Tare(int target){
    int id = 0x160+target;
    unsigned char data[8];
    data[0] = 2;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 1, data);
}
void Syrup_SetCalibParam(int target, int loadcell_id, int scale){
    int id = 0x160+target;
    unsigned char data[8];
    data[0] = 3;
    data[1] = 0;    //loadcell_id;
    data[2] = scale&0xFF;
    data[3] = (scale>>8)&0xFF;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 4, data);
}

// soda-------------------------------------------------------------------------
void Soda_DispenseByTime(int type, int param){
    int id = 0x141;
    unsigned char data[8];
    data[0] = 0;
    data[1] = type;
    data[2] = param&0xFF;
    data[3] = (param>>8)&0xFF;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 4, data);
}
void Soda_DispenseByWeight(int type, int param){
    int id = 0x141;
    unsigned char data[8];
    data[0] = 1;
    data[1] = type;
    data[2] = param&0xFF;
    data[3] = (param>>8)&0xFF;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 4, data);
}
void Soda_Tare(){
    int id = 0x141;
    unsigned char data[8];
    data[0] = 2;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 1, data);
}
void Soda_SetCalibParam(int loadcell_id, int scale){
    int id = 0x141;
    unsigned char data[8];
    data[0] = 3;
    data[1] = 0;    //loadcell_id;
    data[2] = scale&0xFF;
    data[3] = (scale>>8)&0xFF;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 4, data);
}

// hot--------------------------------------------------------------------------
void Hot_DispenseByTime(int type, int param){
    int id = 0x142;
    unsigned char data[8];
    data[0] = 0;
    data[1] = type;
    data[2] = param&0xFF;
    data[3] = (param>>8)&0xFF;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 4, data);
}
void Hot_DispenseByWeight(int type, int param){
    int id = 0x142;
    unsigned char data[8];
    data[0] = 1;
    data[1] = type;
    data[2] = param&0xFF;
    data[3] = (param>>8)&0xFF;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 4, data);
}
void Hot_Tare(){
    int id = 0x142;
    unsigned char data[8];
    data[0] = 2;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 1, data);
}
void Hot_SetCalibParam(int loadcell_id, int scale){
    int id = 0x142;
    unsigned char data[8];
    data[0] = 3;
    data[1] = loadcell_id;
    data[2] = scale&0xFF;
    data[3] = (scale>>8)&0xFF;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 4, data);
}

// icecream---------------------------------------------------------------------
void Icecream_DispenseByTime(int type, int param){
    int id = 0x143;
    unsigned char data[8];
    data[0] = 0;
    data[1] = type;
    data[2] = param&0xFF;
    data[3] = (param>>8)&0xFF;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 4, data);
}
void Icecream_DispenseByWeight(int type, int param){
    int id = 0x143;
    unsigned char data[8];
    data[0] = 1;
    data[1] = type;
    data[2] = param&0xFF;
    data[3] = (param>>8)&0xFF;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 4, data);
}
void Icecream_Tare(){
    int id = 0x143;
    unsigned char data[8];
    data[0] = 2;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 1, data);
}
void Icecream_SetCalibParam(int loadcell_id, int scale){
    int id = 0x143;
    unsigned char data[8];
    data[0] = 3;
    data[1] = loadcell_id;
    data[2] = scale&0xFF;
    data[3] = (scale>>8)&0xFF;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 4, data);
}

// barcode----------------------------------------------------------------------
void Barcode_RequestLastBarcode(int target){
    int id = 0x200+target;
    unsigned char data[8];
    data[0] = 0;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 1, data);
}

// outlet-----------------------------------------------------------------------
void MotorController_FindHome(int target, int spin_door){
    int id = 0x10+target*2+spin_door;
    unsigned char data[8];
    data[0] = 0xC3;
    data[1] = OUTLET_DATA[target].open_position[spin_door] & 0xFF;
    data[2] = (OUTLET_DATA[target].open_position[spin_door]>>8) & 0xFF;
    data[3] = (OUTLET_DATA[target].open_position[spin_door]>>16) & 0xFF;
    data[4] = (OUTLET_DATA[target].open_position[spin_door]>>24) & 0xFF;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 5, data);
}
void MotorController_GoToPosition(int target, int spin_door, int pos){
    int id = 0x10+target*2+spin_door;
    unsigned char data[8];
    data[0] = 0xE1;
    data[1] = 1;
    data[2] = pos&0xFF;
    data[3] = (pos>>8)&0xFF;
    data[4] = (pos>>16)&0xFF;
    data[5] = (pos>>24)&0xFF;
    unsigned short vel = OUTLET_DATA[target].velocity[spin_door];
    data[6] = vel & 0xFF;
    data[7] = (vel>>8) & 0xFF;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 8, data);
}
void MotorController_EncoderZero(int target, int spin_door){
    int id = 0x10+target*2+spin_door;
    unsigned char data[8];
    data[0] = 0x06;
    data[1] = 0;
    data[2] = 0;
    DRV_CAN0_ChannelMessageTransmit(CAN_CHANNEL0, id, 3, data);
}
