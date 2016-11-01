#ifndef LOCALIZATION_ZIGBEE_H
#define LOCALIZATION_ZIGBEE_H
/*********************************************************************
* INCLUDES
*/
#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"

/*********************************************************************
* CONSTANTS
*/

//This is a custom cluster for experiment.
//It is not approved by ZigBee Alliance.
#define LOCALIZATION_ZIGBEE_CLUSTER_ID     0xFC02

#define GENERICAPP_ENDPOINT           10

#define GENERICAPP_PROFID             0x0F04
#define GENERICAPP_DEVICEID           0x0001
#define GENERICAPP_DEVICE_VERSION     0
#define GENERICAPP_FLAGS              0

#define GENERICAPP_MAX_CLUSTERS       1
#define GENERICAPP_CLUSTERID          1

// Send Message Timeout
#define GENERICAPP_SEND_MSG_TIMEOUT   5000     // Every 5 seconds

// Application Events (OSAL) - These are bit weighted definitions.
#define GENERICAPP_SEND_MSG_EVT       0x0001

extern endPointDesc_t GenericApp_epDesc;



/*********************************************************************
 * MACROS
 */
   
/*********************************************************************
 * FUNCTIONS
 */

 /*
  * Initialization for the task
  */
extern void localization_zigbee_Init( byte task_id );

  /*
   *  Event Process for the task
   */
extern UINT16 localization_zigbee_event_loop( byte task_id, UINT16 events );


#endif