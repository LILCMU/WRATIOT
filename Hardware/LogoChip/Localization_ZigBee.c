/*********************************************************************
* INCLUDES
*/
#include "Localization_ZigBee.h"
#include "DebugTrace.h"
#include "stdio.h"

/*********************************************************************
* CONSTANTS
*/

/*********************************************************************
 * MACROS
 */
   
/*********************************************************************
 * TYPEDEFS
 */
   
/*********************************************************************
 * GLOBAL VARIABLES
 */
   
// This list should be filled with Application specific Cluster IDs.
const cId_t GenericApp_ClusterList[GENERICAPP_MAX_CLUSTERS] =
{
  GENERICAPP_CLUSTERID
};

const SimpleDescriptionFormat_t GenericApp_SimpleDesc =
{
  GENERICAPP_ENDPOINT,              //  int Endpoint;
  GENERICAPP_PROFID,                //  uint16 AppProfId[2];
  GENERICAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  GENERICAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  GENERICAPP_FLAGS,                 //  int   AppFlags:4;
  GENERICAPP_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)GenericApp_ClusterList,  //  byte *pAppInClusterList;
  GENERICAPP_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)GenericApp_ClusterList   //  byte *pAppInClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in GenericApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t GenericApp_epDesc;


   
/*********************************************************************
 * LOCAL VARIABLES
 */
uint8 localization_zigbee_TaskID;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
   
void Localization_ZigBee_APP_MSGCB( afIncomingMSGPacket_t *pckt );
   
/*********************************************************************
 * @fn          DHT_Init
 *
 * @brief       
 *
 * @param       
 *
 * @return      none
 */
void localization_zigbee_Init( uint8 task_id )
{
  localization_zigbee_TaskID = task_id;
  
  // Fill out the endpoint description.
  GenericApp_epDesc.endPoint = GENERICAPP_ENDPOINT;
  GenericApp_epDesc.task_id = &localization_zigbee_TaskID;
  GenericApp_epDesc.simpleDesc = (SimpleDescriptionFormat_t *)&GenericApp_SimpleDesc;
  GenericApp_epDesc.latencyReq = noLatencyReqs;
  
  afRegister( &GenericApp_epDesc );
  
}

/*********************************************************************
 * @fn          DHT_event_loop
 *
 * @brief       
 *
 * @param       
 *
 * @return      none
 */
uint16 localization_zigbee_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( localization_zigbee_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        
        case AF_INCOMING_MSG_CMD:
          Localization_ZigBee_APP_MSGCB( MSGpkt );
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // Next
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( localization_zigbee_TaskID );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }
  
  // Discard unknown events
  return 0;
  
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      GenericApp_MessageMSGCB
 *
 * @brief   Data message processor callback.  This function processes
 *          any incoming data - probably from other devices.  So, based
 *          on cluster ID, perform the intended action.
 *
 * @param   none
 *
 * @return  none
 */
void Localization_ZigBee_APP_MSGCB( afIncomingMSGPacket_t *pkt )
{
 // switch ( pkt->clusterId )
 // {
 //   case GENERICAPP_CLUSTERID:
      
 //    break;
 // }
  char msgStr[30];
  if( pkt->clusterId == GENERICAPP_CLUSTERID){
#if defined(LOCALIZATION_ROUTER_REPORT)
     sprintf(msgStr,"s %x rs %d rd %x\n",pkt->srcAddr.addr.shortAddr,pkt->rssi,pkt->radius);
     debug_str(msgStr);
#endif
  }
}
   
