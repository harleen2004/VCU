#ifndef _CANMANAGER_H
#define _CANMANAGER_H

#include "IO_Driver.h"
#include "IO_CAN.h"

#include "avlTree.h"
#include "motorController.h"
#include "instrumentCluster.h"
#include "bms.h"
#include "wheelSpeeds.h"
#include "safety.h"
#include "LaunchControl.h"
#include "drs.h"
//#include "sensorCalculations.h"

typedef enum
{
    CAN0_HIPRI,
    CAN1_LOPRI
} CanChannel;
//CAN0: 48 messages per handle (48 read, 48 write)
//CAN1: 16 messages per handle



typedef struct _CanManager {
    //AVLNode* incomingTree;
    //AVLNode* outgoingTree;


    SerialManager* sm;

    ubyte1 canMessageLimit;
    
    //These are our four FIFO queues.  All messages should come/go through one of these queues.
    //Functions shall have a CanChannel enum (see header) parameter.  Direction (send/receive is not
    //specified by this parameter.  The CAN0/CAN1 is selected based on the parameter passed in, and 
    //Read/Write is selected based on the function that is being called (get/send)
    ubyte1 can0_busSpeed;
    ubyte1 can0_readHandle;
    ubyte1 can0_read_messageLimit;
    ubyte1 can0_writeHandle;
    ubyte1 can0_write_messageLimit;

    ubyte1 can1_busSpeed;
    ubyte1 can1_readHandle;
    ubyte1 can1_read_messageLimit;
    ubyte1 can1_writeHandle;
    ubyte1 can1_write_messageLimit;
    
    IO_ErrorType ioErr_can0_Init;
    IO_ErrorType ioErr_can1_Init;

    IO_ErrorType ioErr_can0_fifoInit_R;
    IO_ErrorType ioErr_can0_fifoInit_W;
    IO_ErrorType ioErr_can1_fifoInit_R;
    IO_ErrorType ioErr_can1_fifoInit_W;

    IO_ErrorType ioErr_can0_read;
    IO_ErrorType ioErr_can0_write;
    IO_ErrorType ioErr_can1_read;
    IO_ErrorType ioErr_can1_write;

    ubyte4 sendDelayus;


    //WARNING: These values are not initialized - be careful to only access
    //pointers that have been previously assigned
    //AVLNode* canMessageHistory[0x7FF];
    AVLNode* canMessageHistory[0x7FF];
} CanManager;

typedef struct _CanMessageNode CanMessageNode;

//Note: Sum of messageLimits must be < 128 (hardware only does 128 total messages)
CanManager *CanManager_new(ubyte2 can0_busSpeed, ubyte1 can0_read_messageLimit, ubyte1 can0_write_messageLimit, ubyte2 can1_busSpeed, ubyte1 can1_read_messageLimit, ubyte1 can1_write_messageLimit, ubyte4 defaultSendDelayus, SerialManager *sm);
IO_ErrorType CanManager_send(CanManager *me, CanChannel channel, IO_CAN_DATA_FRAME canMessages[], ubyte1 canMessageCount);

//Reads and distributes can messages to their appropriate subsystem objects so they can updates themselves
void CanManager_read(CanManager *me, CanChannel channel, MotorController *mcm, InstrumentCluster *ic, BatteryManagementSystem *bms, SafetyChecker *sc);

void canOutput_sendSensorMessages(CanManager *me);
//void canOutput_sendMCUControl(CanManager* me, MotorController* mcm, bool sendEvenIfNoChanges);
void canOutput_sendDebugMessage(CanManager *me, TorqueEncoder *tps, BrakePressureSensor *bps, MotorController *mcm, InstrumentCluster *ic, BatteryManagementSystem *bms, WheelSpeeds *wss, SafetyChecker *sc, LaunchControl *lc, DRS *drs);

ubyte1 CanManager_getReadStatus(CanManager *me, CanChannel channel);

#endif // _CANMANAGER_H is defined