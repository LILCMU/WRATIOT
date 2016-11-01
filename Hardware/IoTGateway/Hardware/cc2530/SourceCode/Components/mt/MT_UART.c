/***************************************************************************************************
  Filename:       MT_UART.c
  Revised:        $Date: 2009-03-12 16:25:22 -0700 (Thu, 12 Mar 2009) $
  Revision:       $Revision: 19404 $

  Description:  This module handles anything dealing with the serial port.

  Copyright 2007 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.

***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 ***************************************************************************************************/
#include "ZComDef.h"
#include "OSAL.h"
#include "hal_uart.h"
#include "MT.h"
#include "MT_UART.h"
#include "OSAL_Memory.h"

#include "string.h"
#include "DebugTrace.h"
#include "ZDProfile.h"
#include "stdlib.h"
#include "stdio.h"
#include "zcl_general.h"
#include "ZDSecMgr.h"

#include "zcl_closures.h"


/***************************************************************************************************
 * MACROS
 ***************************************************************************************************/

/***************************************************************************************************
 * CONSTANTS
 ***************************************************************************************************/
/* State values for ZTool protocal */
#define SOP_STATE      0x00
#define CMD_STATE1     0x01
#define CMD_STATE2     0x02
#define LEN_STATE      0x03
#define DATA_STATE     0x04
#define FCS_STATE      0x05

#define PROCESS_GW_UART_SUCESSS "CMDSUCCESS"
#define PROCESS_GW_UART_WRONGTYPE "CMDWRONGTYPE"

/***************************************************************************************************
 *                                         GLOBAL VARIABLES
 ***************************************************************************************************/
/* Used to indentify the application ID for osal task */
byte App_TaskID;

/* ZTool protocal parameters */
uint8 state;
uint8  CMD_Token[2];
uint8  LEN_Token;
uint8  FSC_Token;
mtOSALSerialData_t  *pMsg;
uint8  tempDataLen;

char * args[10];

#if defined (ZAPP_P1) || defined (ZAPP_P2)
uint16  MT_UartMaxZAppBufLen;
bool    MT_UartZAppRxStatus;
#endif

CacheDeviceTable *CacheDeviceTablePtr;

void testWriteNV( void );


//void cicmdOnOff(uint16 destAddr,,char * name,char * strCommand,char * addrType);

/***************************************************************************************************
 *                                          LOCAL FUNCTIONS
 ***************************************************************************************************/

/***************************************************************************************************
 * @fn      MT_UartInit
 *
 * @brief   Initialize MT with UART support
 *
 * @param   None
 *
 * @return  None
***************************************************************************************************/
void MT_UartInit ()
{
  halUARTCfg_t uartConfig;

  /* Initialize APP ID */
  App_TaskID = 0;

  /* UART Configuration */
  uartConfig.configured           = TRUE;
  //uartConfig.baudRate             = MT_UART_DEFAULT_BAUDRATE;
  //uartConfig.baudRate             = HAL_UART_BR_9600;
  uartConfig.baudRate             = HAL_UART_BR_115200;
  uartConfig.flowControl          = FALSE;
  uartConfig.flowControlThreshold = 0;
  uartConfig.rx.maxBufSize        = MT_UART_DEFAULT_MAX_RX_BUFF;
  uartConfig.tx.maxBufSize        = MT_UART_DEFAULT_MAX_TX_BUFF;
  uartConfig.idleTimeout          = MT_UART_DEFAULT_IDLE_TIMEOUT;
  uartConfig.intEnable            = TRUE;
  /*
#if defined (ZTOOL_P1) || defined (ZTOOL_P2)
  uartConfig.callBackFunc         = MT_UartProcessZToolData;
#elif defined (ZAPP_P1) || defined (ZAPP_P2)
  uartConfig.callBackFunc         = MT_UartProcessZAppData;
#else
  uartConfig.callBackFunc         = NULL;
#endif
  */
  
  uartConfig.callBackFunc         = uartHandleCommand;
  
  /* Start UART */
#if defined (MT_UART_DEFAULT_PORT)
  HalUARTOpen (MT_UART_DEFAULT_PORT, &uartConfig);
#else
  /* Silence IAR compiler warning */
  (void)uartConfig;
#endif

  /* Initialize for ZApp */
#if defined (ZAPP_P1) || defined (ZAPP_P2)
  /* Default max bytes that ZAPP can take */
  MT_UartMaxZAppBufLen  = 1;
  MT_UartZAppRxStatus   = MT_UART_ZAPP_RX_READY;
#endif
  
  
  InitCacheDeviceTable ();

}

/***************************************************************************************************
 * @fn      MT_SerialRegisterTaskID
 *
 * @brief   This function registers the taskID of the application so it knows
 *          where to send the messages whent they come in.
 *
 * @param   void
 *
 * @return  void
 ***************************************************************************************************/
void MT_UartRegisterTaskID( byte taskID )
{
  App_TaskID = taskID;
}

void uartHandleCommand( uint8 port, uint8 event ){
  
  
  
  
  //sprintf(bufferSize, "%d", countByteInRxBuffer);
  //_itoa(countByteInRxBuffer,bufferSize,10);
//  uint8 i;
//  for (i=0;*bufferInRx+i!='\r';i++)
//  {
//    
//    //HalUARTWrite(MT_UART_DEFAULT_PORT, tempThis, strlen(tempThis));
//    HalUARTWrite(port, bufferInRx+i, 1);
//    //debug_str("rx was got somethings");
//  }
//  for(uint8 i = 0;i<countByteInRxBuffer;i++){
//    HalUARTWrite(port, bufferInRx+i, 1);
//  }
  
  
  
  switch(event) {
   //case HAL_UART_TX_FULL:
     //debug_str("txfull");
   case HAL_UART_RX_FULL:
     //debug_str("rxfull");
     //break;
   case HAL_UART_RX_ABOUT_FULL:
     //debug_str("rxabfull");
     //break;
   case HAL_UART_RX_TIMEOUT:
    
    uint8 confirmExecuteFlag = 1;
    uint8 *bufferInRx;
    uint8 argBuffer[20];
    int argcount = 0;
    uint8 countArgByteInBufferRx = 0;
    uint16 countByteInRxBuffer = Hal_UART_RxBufLen(port);
    bufferInRx = osal_mem_alloc(countByteInRxBuffer);
    HalUARTRead (port, bufferInRx, countByteInRxBuffer);

    afStatus_t zdpCmdStatus;
    //char test[30];
     
    for(uint8 i = 0; i<countByteInRxBuffer; i++){
      if(bufferInRx[i] != 0x0A) {
       if(bufferInRx[i] == ' '){
         argBuffer[countArgByteInBufferRx] = '\0';
         args[argcount] = osal_mem_alloc(countArgByteInBufferRx+1);
         strcpy(args[argcount],argBuffer);
         argcount++;
         countArgByteInBufferRx = 0;
       }else{
         argBuffer[countArgByteInBufferRx] = bufferInRx[i];
         countArgByteInBufferRx++;
       }
     }
      else {
        
       argBuffer[countArgByteInBufferRx] = '\0';
       args[argcount] = osal_mem_alloc(countArgByteInBufferRx+1);
       strcpy(args[argcount],argBuffer);
       argcount++;
       countArgByteInBufferRx = 0;
        
       //This variable is used to send execute comfirmation in this loop.
       //On the other hand, some function have their own response like default response in onoff command.
       //We will send comfirmation from that callback in zcl_samplelight.c
       confirmExecuteFlag = 1;
      
       /*
        for(int k = 0;k < argcount;k++){
           debug_str(args[k]);
         }
       */
        
        
        /*IEEEREQ [REQType,DestAddr,StartIndex]
        REQType 0 : Single
                1 : Extend (For Topology)
        StartIndex may not be required when REQType is Assigned to 0.     
         */
        if(!strcmp(args[0],"IEEEREQ")){
          if(!strcmp(args[1],"0")){
            zdpCmdStatus = ZDP_IEEEAddrReq( (uint16)atoi(args[2]), 0, 0, 0);
          }else if(!strcmp(args[1],"1")){
            ZDP_IEEEAddrReq( (uint16)atoi(args[2]), 1, (byte)atoi(args[3]), 0);
          }else{
            HalUARTWrite(MT_UART_DEFAULT_PORT, PROCESS_GW_UART_WRONGTYPE, strlen(PROCESS_GW_UART_WRONGTYPE));
          }
          //sprintf(test,"zdpCmdStatus : %d",zdpCmdStatus);
          //debug_str(test);
          //debug_str(PROCESS_GW_UART_SUCESSS);
        }
        
        else if(!strcmp(args[0],"READATTR")){
          
          
          afAddrType_t *addr;
          addr = osal_mem_alloc(sizeof(afAddrType_t));
          addr->endPoint = (uint8)atoi(args[1]);
          
          if(atoi(args[2]) == 0){
              addr->addrMode = (afAddrMode_t)Addr16Bit;
              confirmExecuteFlag = 0;
            }else if(atoi(args[2]) == 1){
              addr->addrMode = (afAddrMode_t)AddrGroup;
            }else{
              addr->addrMode = (afAddrMode_t)AddrBroadcast;
            }
          
          addr->addr.shortAddr = (uint16)atoi(args[3]);
          zclReadCmd_t readCmd;
          readCmd.numAttr = 1;
          readCmd.attrID[0] = (uint16)atoi(args[5]);
          
          
          //args[4] is cluster id
          
          zcl_SendRead( 8, addr, (uint16)atoi(args[4]), &readCmd, ZCL_FRAME_CLIENT_SERVER_DIR, FALSE, 0);
          
          osal_mem_free(addr);
          
        }else if(!strcmp(args[0],"IDENTIFY")){
          afAddrType_t *addr;
          addr = osal_mem_alloc(sizeof(afAddrType_t));
          
            if(atoi(args[2]) == 0){
              addr->addrMode = (afAddrMode_t)Addr16Bit;
              confirmExecuteFlag = 0;
            }else if(atoi(args[2]) == 1){
              addr->addrMode = (afAddrMode_t)AddrGroup;
            }else{
              addr->addrMode = (afAddrMode_t)AddrBroadcast;
            }
          addr->endPoint = (uint8)atoi(args[1]);
          addr->addr.shortAddr = (uint16)atoi(args[3]);
          zclGeneral_SendIdentify( 8, addr,(uint16)atoi(args[4]), FALSE, 0);
          osal_mem_free(addr);
        }else if(!strcmp(args[0],"IDENTIFYQ")){
          afAddrType_t *addr;
          addr = osal_mem_alloc(sizeof(afAddrType_t));
            if(atoi(args[2]) == 0){
              addr->addrMode = (afAddrMode_t)Addr16Bit;
            }else if(atoi(args[2]) == 1){
              addr->addrMode = (afAddrMode_t)AddrGroup;
            }else{
              addr->addrMode = (afAddrMode_t)AddrBroadcast;
            }
          addr->endPoint = (uint8)atoi(args[1]);
          
          addr->addr.shortAddr = (uint16)atoi(args[3]);
          
          zclGeneral_SendIdentifyQuery( 8, addr,FALSE, 0);
          osal_mem_free(addr);
        }else if(!strcmp(args[0],"ACTIVEEPQ")){
          zAddrType_t destAddr;
          //uint8 retValue;
          destAddr.addrMode = Addr16Bit;
          destAddr.addr.shortAddr = (uint16)atoi(args[1]);
          ZDP_ActiveEPReq( &destAddr, (uint16)atoi(args[1]), 0);
          //HalUARTWrite(MT_UART_DEFAULT_PORT, &retValue , 1);
        }else if(!strcmp(args[0],"ONOFF")){
          afAddrType_t addr;
          addr.endPoint = (uint8)atoi(args[1]);
          if(atoi(args[2]) == 0){
              addr.addrMode = (afAddrMode_t)Addr16Bit;
              confirmExecuteFlag = 0;
            }else if(atoi(args[2]) == 1){
              addr.addrMode = (afAddrMode_t)AddrGroup;
            }else{
              addr.addrMode = (afAddrMode_t)AddrBroadcast;
            }
          addr.addr.shortAddr = (uint16)atoi(args[3]);  // assign the destination address
          if(!strcmp(args[4],"1")){
            zclGeneral_SendOnOff_CmdOn(8,&addr,FALSE,0);
          }else if(!strcmp(args[4],"0")){
            zclGeneral_SendOnOff_CmdOff(8,&addr,FALSE,0);
          }else if(!strcmp(args[4],"2")){
            zclGeneral_SendOnOff_CmdToggle(8,&addr,FALSE,0);
          }
        }else if(!strcmp(args[0],"PERMITJOIN")){
          ZDSecMgrPermitJoining(atoi(args[1]));
        }else if(!strcmp(args[0],"GETCACHETB")){
          
          RetrieveCacheDeviceTableToSerialPort((uint16)atoi(args[1]));
          
          //CacheDeviceTable *temp;
          //temp = osal_mem_alloc(sizeof(CacheDeviceTable));
          //osal_nv_read( APP_NV_CACHE_DEVICE_TABLE , 0 , sizeof(CacheDeviceTable ), temp);
          
          /*
          char msgStr[100];
          sprintf(msgStr,"cnt:%d d:%d,%d,%d,%d,%d",CacheDeviceTablePtr->CacheDeviceTableCount,CacheDeviceTablePtr->CacheDevice[0],CacheDeviceTablePtr->CacheDevice[1],CacheDeviceTablePtr->CacheDevice[2],CacheDeviceTablePtr->CacheDevice[3],CacheDeviceTablePtr->CacheDevice[6] );
                        
          HalUARTWrite(MT_UART_DEFAULT_PORT, msgStr, strlen(msgStr));
          */
          
        }

        else if(!strcmp(args[0],"DELDEVONCTB")){
          DeleteDeviceFromCacheDeviceTable( (uint16)atoi(args[1]) );
        }
        
        else if(!strcmp(args[0],"DELALLDEVONCTB")){
          DeleteAllDeviceFromCacheDeviceTable();
        }
        
        else if(!strcmp(args[0],"SPDESCQ")){
          SimpleDescriptorQuery ( (uint16)atoi(args[2]) , (uint8)atoi(args[1]) );
        }
        
        //Not Complete
        else if(!strcmp(args[0],"CPDESCQ")){
          
          char ff[20];
          zAddrType_t hh;
          hh.addr.shortAddr = (uint16)atoi(args[1]);
          hh.addrMode = 2;
          ZDP_ComplexDescReq( &hh ,(uint16)atoi(args[1]),0 );
        
          sprintf(ff," %x ",(uint16)atoi(args[1]));
          debug_str(ff);
          
        }
        
        else if(!strcmp(args[0],"DOORLOCKCMD")){
          
          afAddrType_t addr;
          //uint8 pinCode[5] = {4,1,2,3,4};
          //zclDoorLock_t pPayload;
          //pPayload.pPinRfidCode = pinCode;
          
          
          addr.endPoint = (uint8)atoi(args[1]);
          if(atoi(args[2]) == 0){
              addr.addrMode = (afAddrMode_t)Addr16Bit;
            }else if(atoi(args[2]) == 1){
              addr.addrMode = (afAddrMode_t)AddrGroup;
            }else{
              addr.addrMode = (afAddrMode_t)AddrBroadcast;
            }
          addr.addr.shortAddr = (uint16)atoi(args[3]);
          
          if(atoi(args[4]) == 1){
            zcl_SendCommand( 8 , &addr , ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
                            0, TRUE, ZCL_FRAME_CLIENT_SERVER_DIR,
                            FALSE , 0, 0, 0, NULL );
            //zclClosures_SendDoorLockLockDoor( (uint8)atoi(args[1]) , &addr , &pPayload , FALSE , 0);
            
          }else{
            zcl_SendCommand( 8 , &addr , ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK,
                            1, TRUE, ZCL_FRAME_CLIENT_SERVER_DIR,
                            FALSE , 0, 0, 0, NULL );
            //zclClosures_SendDoorLockUnlockDoor( (uint8)atoi(args[1]) , &addr , &pPayload , FALSE , 0);
          }
        
        }
        
        else if(!strcmp(args[0],"SENDCMD")){
          
          afAddrType_t addr;
          uint8 *payload_temp;
          payload_temp = osal_mem_alloc((uint16)atoi(args[6]));
          addr.endPoint = (uint8)atoi(args[1]);
          if(atoi(args[2]) == 0){
              addr.addrMode = (afAddrMode_t)Addr16Bit;
            }else if(atoi(args[2]) == 1){
              addr.addrMode = (afAddrMode_t)AddrGroup;
            }else{
              addr.addrMode = (afAddrMode_t)AddrBroadcast;
            }
          addr.addr.shortAddr = (uint16)atoi(args[3]);
          
          /* arg4 = cluster id
           * arg5 = command id
           * arg6 = payload lenght
           * arg7 = payload data (not available now until convert all to byte code)
           */
          
          memcpy(payload_temp,args[7],(uint16)atoi(args[6]));
          zcl_SendCommand( 8 , &addr , (uint16)atoi(args[4]) ,
                            (uint16)atoi(args[5]) , TRUE, ZCL_FRAME_CLIENT_SERVER_DIR,
                            FALSE , 0, 0, (uint16)atoi(args[6]) , payload_temp );
          
          //HalUARTWrite(MT_UART_DEFAULT_PORT, args[7] , 10);
          
          osal_mem_free(payload_temp);
        
        }
        
        else if(!strcmp(args[0],"DLOCKADDUSER")){
          
          zclDoorLockSetPINCode_t userDetailTuple;
          afAddrType_t addr;
          userDetailTuple.userID = (uint16)atoi(args[4]);
          userDetailTuple.userStatus = USER_STATUS_OCCUPIED_ENABLED;
          userDetailTuple.userType = USER_TYPE_UNRESTRICTED_USER;
          //fix length now
          uint8 *pPIN_temp;
          pPIN_temp = osal_mem_alloc(5);
          pPIN_temp[0] = 4;
          pPIN_temp[1] = (uint8)atoi(args[5]);
          pPIN_temp[2] = (uint8)atoi(args[6]);
          pPIN_temp[3] = (uint8)atoi(args[7]);
          pPIN_temp[4] = (uint8)atoi(args[8]);
          userDetailTuple.pPIN = pPIN_temp;
          
          
          
          addr.endPoint = (uint8)atoi(args[1]);
          if(atoi(args[2]) == 0){
              addr.addrMode = (afAddrMode_t)Addr16Bit;
            }else if(atoi(args[2]) == 1){
              addr.addrMode = (afAddrMode_t)AddrGroup;
            }else{
              addr.addrMode = (afAddrMode_t)AddrBroadcast;
            }
          addr.addr.shortAddr = (uint16)atoi(args[3]);
          
          zclClosures_SendDoorLockSetPINCodeRequest( 8 , &addr , &userDetailTuple , FALSE ,0);
          
          osal_mem_free(pPIN_temp);
          
        }
        
        else if(!strcmp(args[0],"DLOCKCLEARUSER")){
          
          afAddrType_t addr;
          
          addr.endPoint = (uint8)atoi(args[1]);
          if(atoi(args[2]) == 0){
              addr.addrMode = (afAddrMode_t)Addr16Bit;
            }else if(atoi(args[2]) == 1){
              addr.addrMode = (afAddrMode_t)AddrGroup;
            }else{
              addr.addrMode = (afAddrMode_t)AddrBroadcast;
            }
          addr.addr.shortAddr = (uint16)atoi(args[3]);
          
          //zclClosures_SendDoorLockClearPINCode(  8 , &addr , (uint16)atoi(args[4]) , FALSE ,0 );
          
          zclClosures_SendDoorLockClearAllPINCodes( 8 , &addr , FALSE ,0  );
          
        }
        
        else if(!strcmp(args[0],"SENDREPORT")){
          
          debug_str("ReportCMD");
          
          
          afAddrType_t addr;
          uint8 attrCount = 1;
          
          zclReportCmd_t *reportCmdTemp = osal_mem_alloc( sizeof(zclReportCmd_t) + (sizeof( zclReport_t ) * attrCount) );
          
          //zclReport_t reportTuple = osal_mem_alloc(sizeof(zclReport_t));
          zclReport_t reportTuple[1];
          
          uint8 attrData[2];
          attrData[0] = (uint8)atoi(args[4]) & 0xff;
          attrData[1] = (uint8)atoi(args[4]) >> 8;
          
          
          reportTuple[0].attrData = attrData;
          reportTuple[0].attrID = 5;
          reportTuple[0].dataType = ZCL_DATATYPE_UINT16;
          reportCmdTemp->attrList[0] = reportTuple[0];
          reportCmdTemp->numAttr = 1;
          
          
          addr.endPoint = (uint8)atoi(args[1]);
          if(atoi(args[2]) == 0){
              addr.addrMode = (afAddrMode_t)Addr16Bit;
            }else if(atoi(args[2]) == 1){
              addr.addrMode = (afAddrMode_t)AddrGroup;
            }else{
              addr.addrMode = (afAddrMode_t)AddrBroadcast;
            }
          addr.addr.shortAddr = (uint16)atoi(args[3]);
          
          
          zcl_SendReportCmd( 8, &addr, 0xFC01 , reportCmdTemp,
                             ZCL_FRAME_CLIENT_SERVER_DIR , FALSE, 0 );
        
          //pDiscoverExtRsp = (zclDiscoverAttrsExtRsp_t *)zcl_mem_alloc( sizeof (zclDiscoverAttrsExtRsp_t)
           //                                              + sizeof ( zclExtAttrInfo_t ) * numAttrs );
          
          //osal_mem_free(reportTuple);
          osal_mem_free(reportCmdTemp);
          
        }
        
        else if(!strcmp(args[0],"WREGGEKKO")){
          
          afAddrType_t addr;
          uint8 payload_temp[2] = {0,0};
          //register position
          payload_temp[0] = (uint8)atoi(args[4]);
          //register value
          payload_temp[1] = (uint8)atoi(args[5]);
          
          
          addr.endPoint = (uint8)atoi(args[1]);
          if(atoi(args[2]) == 0){
              addr.addrMode = (afAddrMode_t)Addr16Bit;
              confirmExecuteFlag = 0;
            }else if(atoi(args[2]) == 1){
              addr.addrMode = (afAddrMode_t)AddrGroup;
            }else{
              addr.addrMode = (afAddrMode_t)AddrBroadcast;
            }
          addr.addr.shortAddr = (uint16)atoi(args[3]);
          
          zcl_SendCommand( 8 , &addr , ZCL_CLUSTER_ID_GEN_ON_OFF,
                            0x50, TRUE, ZCL_FRAME_CLIENT_SERVER_DIR,
                            FALSE , 0, 0, 2, payload_temp );
        
        }
        
        //SerialCommandProcessStatus(1);
        
        
        
        //free memory
        for(int i = 0; i <= argcount; i++){
          osal_mem_free(args[i]);
        }
        //reset argcount
        argcount = 0;
        
        if(confirmExecuteFlag==1){
          SerialCommandProcessStatus(1);
        }
        
      }
    
  }
  
  //free memory
  osal_mem_free(bufferInRx);
    
   /* for(uint8 i = 0;*(bufferInRx+i)!='\n';i++){
      HalUARTWrite(port, bufferInRx+i, 1);
      
      
      
      
    }*/
     
//     if(*(bufferInRx+1)=='\r'){
//      HalUARTWrite(port, bufferInRx+0, 1);
//      }
//      else{
//        debug_str("in there");
//      }
   
  
  
    break;
  }
  
  
  
  /*
  char eventFire[20];
  switch(event) {
   case HAL_UART_RX_FULL:
     break;
   case HAL_UART_RX_ABOUT_FULL:
     break;
   case HAL_UART_RX_TIMEOUT:
     break;
   default:
     //sprintf(eventFire,"NoEvt:%d|",event);
     //HalUARTWrite(port, eventFire, strlen(eventFire));
     break;
  }
  */
  
}

void testWriteNV( void ){

  CacheDeviceTablePtr->CacheDeviceTableCount = 7;
  CacheDeviceTablePtr->CacheDevice[0] = 0;
  CacheDeviceTablePtr->CacheDevice[1] = 25486;
  CacheDeviceTablePtr->CacheDevice[2] = 55555;
  CacheDeviceTablePtr->CacheDevice[3] = 25486;
  CacheDeviceTablePtr->CacheDevice[6] = 10;
  //CacheDeviceTablePtr->CacheDevice[50] = 8;
  UpdateCacheDeviceTableToNV();
  
}

void SerialCommandProcessStatus( uint8 status ){
  
  char msgStr[7];
  sprintf(msgStr,"%c%c%c%c%c%c",0x54,0xfe,0x00,0x08,0x01,status);
  HalUARTWrite(MT_UART_DEFAULT_PORT, msgStr, 6);

}

void InitCacheDeviceTable ( void ){
  
  uint8 nv_status;
  CacheDeviceTablePtr = osal_mem_alloc(sizeof(CacheDeviceTable));
  nv_status = osal_nv_item_init( APP_NV_CACHE_DEVICE_TABLE, sizeof(CacheDeviceTable) , NULL );
  if( nv_status == SUCCESS ){
    osal_nv_read( APP_NV_CACHE_DEVICE_TABLE , 0 , sizeof(CacheDeviceTable ), CacheDeviceTablePtr);
    //debug_str("read old");
  }else if( nv_status == NV_ITEM_UNINIT ){
    //debug_str("read init");
    CacheDeviceTablePtr->CacheDeviceTableCount = 0;
    UpdateCacheDeviceTableToNV();
    //testWriteNV();
  }
  
}

void AddDeviceToCacheDeviceTable( uint16 nwkid ){
  
  uint16 i;
  uint8 existingFlag = 0;
  char msgStr[7];
  //check existing data
  for( i = 0 ; i < CacheDeviceTablePtr->CacheDeviceTableCount ; i++){
    if( CacheDeviceTablePtr->CacheDevice[i] == nwkid ){
      existingFlag = 1;
      break;
    }
  }
  
  if(existingFlag==0){
    if( CacheDeviceTablePtr->CacheDeviceTableCount < CACHEDEVICETABLESIZE ){
      CacheDeviceTablePtr->CacheDevice[ CacheDeviceTablePtr->CacheDeviceTableCount ] = nwkid;
      CacheDeviceTablePtr->CacheDeviceTableCount += 1;
      UpdateCacheDeviceTableToNV();
      //sprintf(msgStr,"%c%c%c%c%c%c",0x54,0xfe,0x00,0x05,0x01,0x00);
      //HalUARTWrite(MT_UART_DEFAULT_PORT, msgStr, 6);
      ReportCacheDeviceTableStatusToSerialPort( 0 );
    }else{
      //sprintf(msgStr,"%c%c%c%c%c%c",0x54,0xfe,0x00,0x05,0x01,0x01);
      //HalUARTWrite(MT_UART_DEFAULT_PORT, msgStr, 6);
      ReportCacheDeviceTableStatusToSerialPort( 1 );
    }
  }else{
    //sprintf(msgStr,"%c%c%c%c%c%c",0x54,0xfe,0x00,0x05,0x01,0x02);
    //HalUARTWrite(MT_UART_DEFAULT_PORT, msgStr, 6);
    ReportCacheDeviceTableStatusToSerialPort( 2 );
    
  }
  
}

void SimpleDescriptorQuery ( uint16 nwkid , uint8 ep ){

  zAddrType_t *addr = osal_mem_alloc(sizeof(zAddrType_t));
  addr->addrMode = (afAddrMode_t)Addr16Bit;
  addr->addr.shortAddr = nwkid;
  ZDP_SimpleDescReq( addr, nwkid , ep, 0);
  osal_mem_free(addr);

}

void ReportCacheDeviceTableStatusToSerialPort( uint8 status ){

  char msgStr[7];
  sprintf(msgStr,"%c%c%c%c%c%c",0x54,0xfe,0x00,0x05,0x01,status);
  HalUARTWrite(MT_UART_DEFAULT_PORT, msgStr, 6);

}

void DeleteDeviceFromCacheDeviceTable( uint16 nwkid ){
  
  uint16 i;
  uint8 findFlag = 0;
  for( i=0 ; i< CacheDeviceTablePtr->CacheDeviceTableCount ; i++ ){
    
    if( CacheDeviceTablePtr->CacheDevice[i] == nwkid && findFlag == 0){
      findFlag = 1;
    }else if(findFlag == 1){
      CacheDeviceTablePtr->CacheDevice[i-1] = CacheDeviceTablePtr->CacheDevice[i];
    }
  
  }
  
  if(findFlag == 1){
    CacheDeviceTablePtr->CacheDeviceTableCount -= 1;
    UpdateCacheDeviceTableToNV();
    ReportCacheDeviceTableStatusToSerialPort(4);
  }else{
    ReportCacheDeviceTableStatusToSerialPort(3);
  }
  

}

void DeleteAllDeviceFromCacheDeviceTable ( void ){
  
  CacheDeviceTablePtr->CacheDeviceTableCount = 0;
  UpdateCacheDeviceTableToNV();
  ReportCacheDeviceTableStatusToSerialPort( 5 );

}

void RetrieveCacheDeviceTableToSerialPort ( uint16 startIndex ){

  uint16 i;
  uint16 limitEachRound = startIndex+50;
  uint8 tailCount;
  uint8 *startIndexTemp = intToByteArray(startIndex,2);
  uint8 *CacheDeviceAmount = intToByteArray( CacheDeviceTablePtr->CacheDeviceTableCount ,2);
  uint8 byteCount;
  //uint8 byteCount = 9+( CacheDeviceTablePtr->CacheDeviceTableCount*2 );
  if( (CacheDeviceTablePtr->CacheDeviceTableCount - startIndex)>=50  ){
    
    byteCount = 9+( 50*2 );
  
  }else{
    
    byteCount = 9+( (CacheDeviceTablePtr->CacheDeviceTableCount - startIndex)*2 );
  
  }
  
  
  //debug_str("");
  
  char *msgStr = osal_mem_alloc(byteCount+1);
  sprintf(msgStr,"%c%c%c%c%c%c%c%c%c",0x54,0xfe,0x00,0x04,byteCount-5,*CacheDeviceAmount,*(CacheDeviceAmount+1),*startIndexTemp,*(startIndexTemp+1) );
  tailCount = 9;
  for( i=startIndex ; (i<(CacheDeviceTablePtr->CacheDeviceTableCount)) && (i<limitEachRound) ; i++ ){
    char temp[3];
    uint8 *nwkid = intToByteArray(CacheDeviceTablePtr->CacheDevice[i],2);
    sprintf(temp,"%c%c",*nwkid,*(nwkid+1));
    memcpy(msgStr+(tailCount),temp,2);
    tailCount+=2;
    osal_mem_free(nwkid);
  }
  
  HalUARTWrite(MT_UART_DEFAULT_PORT, msgStr, byteCount);
  osal_mem_free(msgStr);
  osal_mem_free(startIndexTemp);
  osal_mem_free(CacheDeviceAmount);
  

}

void UpdateCacheDeviceTableToNV( void ){
  
  osal_nv_write( APP_NV_CACHE_DEVICE_TABLE , 0 , sizeof(CacheDeviceTable) , CacheDeviceTablePtr);

}


/***************************************************************************************************
 * @fn          convert int to byte array
 * 
 * @param       uint16 intVal - integer
 * @param       uint8 byteLenght
 *
 * @return      return Byte Array pointer (Big-endian)
 *              Don't forget to free memory by osal_mem_free()
 ***************************************************************************************************/

uint8 * intToByteArray(uint16 intVal,uint8 byteLenght){
    uint8 *temp;
    temp = osal_mem_alloc(byteLenght);
    for(uint8 i = 0; i < byteLenght ; i++){
        *(temp+((byteLenght-1)-i)) = ((intVal >> (i*8)) & 0xff);
    }
    return temp;
}

/***************************************************************************************************
 * @fn      SPIMgr_CalcFCS
 *
 * @brief   Calculate the FCS of a message buffer by XOR'ing each byte.
 *          Remember to NOT include SOP and FCS fields, so start at the CMD field.
 *
 * @param   byte *msg_ptr - message pointer
 * @param   byte len - length (in bytes) of message
 *
 * @return  result byte
 ***************************************************************************************************/
byte MT_UartCalcFCS( uint8 *msg_ptr, uint8 len )
{
  byte x;
  byte xorResult;

  xorResult = 0;

  for ( x = 0; x < len; x++, msg_ptr++ )
    xorResult = xorResult ^ *msg_ptr;

  return ( xorResult );
}


/***************************************************************************************************
 * @fn      MT_UartProcessZToolData
 *
 * @brief   | SOP | Data Length  |   CMD   |   Data   |  FCS  |
 *          |  1  |     1        |    2    |  0-Len   |   1   |
 *
 *          Parses the data and determine either is SPI or just simply serial data
 *          then send the data to correct place (MT or APP)
 *
 * @param   port     - UART port
 *          event    - Event that causes the callback
 *
 *
 * @return  None
 ***************************************************************************************************/
void MT_UartProcessZToolData ( uint8 port, uint8 event )
{
  uint8  ch;
  uint8  bytesInRxBuffer;
  
  (void)event;  // Intentionally unreferenced parameter

  while (Hal_UART_RxBufLen(port))
  {
    HalUARTRead (port, &ch, 1);

    switch (state)
    {
      case SOP_STATE:
        if (ch == MT_UART_SOF)
          state = LEN_STATE;
        break;

      case LEN_STATE:
        LEN_Token = ch;

        tempDataLen = 0;

        /* Allocate memory for the data */
        pMsg = (mtOSALSerialData_t *)osal_msg_allocate( sizeof ( mtOSALSerialData_t ) +
                                                        MT_RPC_FRAME_HDR_SZ + LEN_Token );

        if (pMsg)
        {
          /* Fill up what we can */
          pMsg->hdr.event = CMD_SERIAL_MSG;
          pMsg->msg = (uint8*)(pMsg+1);
          pMsg->msg[MT_RPC_POS_LEN] = LEN_Token;
          state = CMD_STATE1;
        }
        else
        {
          state = SOP_STATE;
          return;
        }
        break;

      case CMD_STATE1:
        pMsg->msg[MT_RPC_POS_CMD0] = ch;
        state = CMD_STATE2;
        break;

      case CMD_STATE2:
        pMsg->msg[MT_RPC_POS_CMD1] = ch;
        /* If there is no data, skip to FCS state */
        if (LEN_Token)
        {
          state = DATA_STATE;
        }
        else
        {
          state = FCS_STATE;
        }
        break;

      case DATA_STATE:

        /* Fill in the buffer the first byte of the data */
        pMsg->msg[MT_RPC_FRAME_HDR_SZ + tempDataLen++] = ch;

        /* Check number of bytes left in the Rx buffer */
        bytesInRxBuffer = Hal_UART_RxBufLen(port);

        /* If the remain of the data is there, read them all, otherwise, just read enough */
        if (bytesInRxBuffer <= LEN_Token - tempDataLen)
        {
          HalUARTRead (port, &pMsg->msg[MT_RPC_FRAME_HDR_SZ + tempDataLen], bytesInRxBuffer);
          tempDataLen += bytesInRxBuffer;
        }
        else
        {
          HalUARTRead (port, &pMsg->msg[MT_RPC_FRAME_HDR_SZ + tempDataLen], LEN_Token - tempDataLen);
          tempDataLen += (LEN_Token - tempDataLen);
        }

        /* If number of bytes read is equal to data length, time to move on to FCS */
        if ( tempDataLen == LEN_Token )
            state = FCS_STATE;

        break;

      case FCS_STATE:

        FSC_Token = ch;

        /* Make sure it's correct */
        if ((MT_UartCalcFCS ((uint8*)&pMsg->msg[0], MT_RPC_FRAME_HDR_SZ + LEN_Token) == FSC_Token))
        {
          osal_msg_send( App_TaskID, (byte *)pMsg );
        }
        else
        {
          /* deallocate the msg */
          osal_msg_deallocate ( (uint8 *)pMsg );
        }

        /* Reset the state, send or discard the buffers at this point */
        state = SOP_STATE;

        break;

      default:
       break;
    }
  }
}

#if defined (ZAPP_P1) || defined (ZAPP_P2)
/***************************************************************************************************
 * @fn      MT_UartProcessZAppData
 *
 * @brief   | SOP | CMD  |   Data Length   | FSC  |
 *          |  1  |  2   |       1         |  1   |
 *
 *          Parses the data and determine either is SPI or just simply serial data
 *          then send the data to correct place (MT or APP)
 *
 * @param   port    - UART port
 *          event   - Event that causes the callback
 *
 *
 * @return  None
 ***************************************************************************************************/
void MT_UartProcessZAppData ( uint8 port, uint8 event )
{

  osal_event_hdr_t  *msg_ptr;
  uint16 length = 0;
  uint16 rxBufLen  = Hal_UART_RxBufLen(MT_UART_DEFAULT_PORT);

  /*
     If maxZAppBufferLength is 0 or larger than current length
     the entire length of the current buffer is returned.
  */
  if ((MT_UartMaxZAppBufLen != 0) && (MT_UartMaxZAppBufLen <= rxBufLen))
  {
    length = MT_UartMaxZAppBufLen;
  }
  else
  {
    length = rxBufLen;
  }

  /* Verify events */
  if (event == HAL_UART_TX_FULL)
  {
    // Do something when TX if full
    return;
  }

  if (event & ( HAL_UART_RX_FULL | HAL_UART_RX_ABOUT_FULL | HAL_UART_RX_TIMEOUT))
  {
    if ( App_TaskID )
    {
      /*
         If Application is ready to receive and there is something
         in the Rx buffer then send it up
      */
      if ((MT_UartZAppRxStatus == MT_UART_ZAPP_RX_READY ) && (length != 0))
      {
        /* Disable App flow control until it processes the current data */
         MT_UartAppFlowControl (MT_UART_ZAPP_RX_NOT_READY);

        /* 2 more bytes are added, 1 for CMD type, other for length */
        msg_ptr = (osal_event_hdr_t *)osal_msg_allocate( length + sizeof(osal_event_hdr_t) );
        if ( msg_ptr )
        {
          msg_ptr->event = SPI_INCOMING_ZAPP_DATA;
          msg_ptr->status = length;

          /* Read the data of Rx buffer */
          HalUARTRead( MT_UART_DEFAULT_PORT, (uint8 *)(msg_ptr + 1), length );

          /* Send the raw data to application...or where ever */
          osal_msg_send( App_TaskID, (uint8 *)msg_ptr );
        }
      }
    }
  }
}

/***************************************************************************************************
 * @fn      SPIMgr_ZAppBufferLengthRegister
 *
 * @brief
 *
 * @param   maxLen - Max Length that the application wants at a time
 *
 * @return  None
 *
 ***************************************************************************************************/
void MT_UartZAppBufferLengthRegister ( uint16 maxLen )
{
  /* If the maxLen is larger than the RX buff, something is not right */
  if (maxLen <= MT_UART_DEFAULT_MAX_RX_BUFF)
    MT_UartMaxZAppBufLen = maxLen;
  else
    MT_UartMaxZAppBufLen = 1; /* default is 1 byte */
}

/***************************************************************************************************
 * @fn      SPIMgr_AppFlowControl
 *
 * @brief
 *
 * @param   status - ready to send or not
 *
 * @return  None
 *
 ***************************************************************************************************/
void MT_UartAppFlowControl ( bool status )
{

  /* Make sure only update if needed */
  if (status != MT_UartZAppRxStatus )
  {
    MT_UartZAppRxStatus = status;
  }

  /* App is ready to read again, ProcessZAppData have to be triggered too */
  if (status == MT_UART_ZAPP_RX_READY)
  {
    MT_UartProcessZAppData (MT_UART_DEFAULT_PORT, HAL_UART_RX_TIMEOUT );
  }

}

#endif //ZAPP

/***************************************************************************************************
***************************************************************************************************/
