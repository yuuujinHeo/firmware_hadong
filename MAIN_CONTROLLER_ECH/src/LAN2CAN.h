
#ifndef _LAN2CAN_H
#define _LAN2CAN_H



#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "system_config.h"
#include "system_definitions.h"

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

extern "C" {

#endif

// Definitions -----------------------------------------------------------------
//#define _DEBUG_MONITOR_         // Enable debug message
// Error code definition
#define GPIF_NO_ERR              0
#define GPIF_ERR1                -1
#define GPIF_ERR2                -2
#define GPIF_ERR3                -3
#define GPIF_ERR4                -4
#define GPIF_ERR5                -5
#define GPIF_ERR6                -6
#define GPIF_ERR7                -7
#define GPIF_ERR8                -8
#define GPIF_ERR9                -9
#define GPIF_ERR10               -10    
    
#define SERVER_PORT             1977
#define CLIENT_PORT             2011
//#define __UDP__
#define __TCPIP__


typedef union _UINT16_
{
    uint8_t UINT8[2];
    uint16_t UINT16;
} UINT16_UNION, *pUINT16_UNION;
    
typedef union _FLOAT_
{
    uint8_t UINT8[4];
    float FLOAT;
} FLOAT_UNION, *pFLOAT_UNION; 
    
typedef enum
{
    LAN2CAN_STATE_INIT = 0,
    LAN2CAN_STATE_TCPIP_WAIT_FOR_LINK,
    LAN2CAN_STATE_SETUP_TCP_SERVER,
    LAN2CAN_STATE_WAIT_FOR_CONNECTION,
    LAN2CAN_STATE_SERVING_CONNECTION,
    LAN2CAN_STATE_CLOSING_CONNECTION,
    
    LAN2CAN_STATE_ERROR
} LAN2CAN_STATES;


// Client socket definition
typedef struct
{
    // IP Address
    IPV4_ADDR ipAddr;
    // Network handler
    TCPIP_NET_HANDLE netHandle;
    // Socket handler
    TCP_SOCKET socketHandle;
    // Flag to signify when the network link has dropped
	bool dropFlag;
    // LAN Rx data from Main Controller
    uint8_t msgFromClient[1500];
    // LAN Tx data to Main Controller 
    uint8_t msgToClient[1500];
    // LAN connection check timer
    SYS_TMR_HANDLE sysTmrLANCheckHandle;
    // LAN connection check count
    uint16_t checkCount;
} LAN_DATA;

// UART definition
typedef struct
{
    // UART handler
    DRV_HANDLE uartHandle;
    // UART Rx data
    uint8_t uartRxData[256];
    // UART Tx data
    uint8_t uartTxData[256];
    // Current received UART Rx data size
    uint8_t uartRxCount;
} UART_DATA;

// CAN definition
typedef struct
{
    // system timer for transmitting CAN data to the Main controller 
    SYS_TMR_HANDLE sysTmrCANOutHandle;
    // CAN driver handle
    DRV_HANDLE canHandle[2];
    // CAN Rx data
    CAN_RX_MSG_BUFFER *canRxMessage[2][256];
    // CAN Rx count
    uint8_t canRxCount[2];
} CAN_DATA;


typedef struct
{
    /* The application's current state */
    LAN2CAN_STATES state;
    LAN_DATA        lanData;
    CAN_DATA        canData;
    
    SYS_TMR_HANDLE  sysTmrPCHandle;
    SYS_TMR_HANDLE  sysTmrUserHandle;
    SYS_TMR_HANDLE  sysTaskHandle;
    
} LAN2CAN_DATA;



void LAN2CAN_Initialize(void);
void LAN2CAN_Reset(void);
void LAN2CAN_Tasks(void);

int LAN2CAN_LANChipOn(void);
int LAN2CAN_LANChipOff(void);
int LAN2CAN_LANDataParsing(void);
int LAN2CAN_LANCheckConnection(void);

int LAN2CAN_CANReceiveCheckFromRCR(void);
int LAN2CAN_CANSendToMainController(void);
int LAN2CAN_CANClearBuffer(void);


void LAN2CAN_TaskFunction(void);

// ---------------------------------------------------

void Cup_DispenseCup(int target);

void Ice_DispenseIce(int target, int ice_time_ms, int water_time_ms);
void Ice_SettingCommunicationMode(int target, int comm_mode, int cup_ignore);
void Ice_ClearStatus(int target);
void Ice_SettingTimeout(int target, int timeout);
void Ice_Reboot(int target);
void Ice_SettingAmbientTemp(int target, int tempL, int tempH);

void Barcode_RequestLastBarcode(int target);

void Syrup_DispenseByTime(int target, int type, int param);
void Syrup_DispenseByWeight(int target, int type, int param);
void Syrup_Tare(int target);
void Syrup_SetCalibParam(int target, int loadcell_id, int scale);

void Soda_DispenseByTime(int type, int param);
void Soda_DispenseByWeight(int type, int param);
void Soda_Tare();
void Soda_SetCalibParam(int loadcell_id, int scale);

void Hot_DispenseByTime(int type, int param);
void Hot_DispenseByWeight(int type, int param);
void Hot_Tare();
void Hot_SetCalibParam(int loadcell_id, int scale);

void Icecream_DispenseByTime(int type, int param);
void Icecream_DispenseByWeight(int type, int param);
void Icecream_Tare();
void Icecream_SetCalibParam(int loadcell_id, int scale);

void MotorController_FindHome(int target, int spin_door);
void MotorController_GoToPosition(int target, int spin_door, int pos);
void MotorController_EncoderZero(int target, int spin_door);


#endif /* _LAN2CAN_H */

//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNOEndRE-END

/*******************************************************************************
  of File
 */

