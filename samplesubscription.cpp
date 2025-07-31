/******************************************************************************
** Copyright (c) 2006-2020 Unified Automation GmbH. All rights reserved.
**
** Software License Agreement ("SLA") Version 2.7
**
** Unless explicitly acquired and licensed from Licensor under another
** license, the contents of this file are subject to the Software License
** Agreement ("SLA") Version 2.7, or subsequent versions
** as allowed by the SLA, and You may not copy or use this file in either
** source code or executable form, except in compliance with the terms and
** conditions of the SLA.
**
** All software distributed under the SLA is provided strictly on an
** "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
** AND LICENSOR HEREBY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT
** LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
** PURPOSE, QUIET ENJOYMENT, OR NON-INFRINGEMENT. See the SLA for specific
** language governing rights and limitations under the SLA.
**
** The complete license agreement can be found here:
** http://unifiedautomation.com/License/SLA/2.7/
**
** Project: C++ OPC Client SDK sample code
**
******************************************************************************/
#include "samplesubscription.h"
#include "uasubscription.h"
#include "uasession.h"
#include <fstream>
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>


static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
   int i;
   for(i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

SampleSubscription::SampleSubscription(int d, std::string k="kks.csv")
: m_pSession(NULL),
  m_pSubscription(NULL)
{
	kks_array = NULL;
	db = NULL;
	delta = d;
    kks_file = k;
}

SampleSubscription::~SampleSubscription()
{
	delete[] kks_array;

    if ( m_pSubscription )
    {
        deleteSubscription();
    }
    if (db)
    	sqlite3_close(db);
}

void SampleSubscription::subscriptionStatusChanged(
    OpcUa_UInt32      clientSubscriptionHandle, //!< [in] Client defined handle of the affected subscription
    const UaStatus&   status)                   //!< [in] Changed status for the subscription
{
    OpcUa_ReferenceParameter(clientSubscriptionHandle); // We use the callback only for this subscription

    fprintf(stderr, "Subscription not longer valid - failed with status %s\n", status.toString().toUtf8());
}

void SampleSubscription::dataChange(
    OpcUa_UInt32               clientSubscriptionHandle, //!< [in] Client defined handle of the affected subscription
    const UaDataNotifications& dataNotifications,        //!< [in] List of data notifications sent by the server
    const UaDiagnosticInfos&   diagnosticInfos)          //!< [in] List of diagnostic info related to the data notifications. This list can be empty.
{
//   char *zErrMsg = 0;
//   int rc;
//   std::string sql;
//    printf("-- DataChange Notification ---------------------------------\n");
    OpcUa_ReferenceParameter(clientSubscriptionHandle); // We use the callback only for this subscription
    OpcUa_ReferenceParameter(diagnosticInfos);
    OpcUa_UInt32 i = 0;


    for ( i=0; i<dataNotifications.length(); i++ )
    {
        if ( OpcUa_IsGood(dataNotifications[i].Value.StatusCode) )
        {
            UaVariant tempValue = dataNotifications[i].Value.Value;
            time_t time = UaDateTime(dataNotifications[i].Value.SourceTimestamp).toTime_t(); //. dwHighDateTime;
            OpcUa_Double val;
            std::string kks_name = kks_array[dataNotifications[i].ClientHandle];
            std::string value_str = UaVariant(tempValue).toString().toUtf8();
            if (value_str == "true") val = 1;
            if (value_str == "false") val = 0;
            else tempValue.toDouble(val);

            std::cout<< "\"-\" \""<< kks_name <<"\" \"" <<time << "\" \"" << val << "\""<< std::endl	;
		//"-" "INCONT.as_M.AM.10HAD99AM001-AM_1.Q" "1747934810" "45"
            //			(slice_data[kks_name])[iteration_count[kks_name]] = val;
//			std::cout<<  "id: " << kks_name << "[" << iteration_count[kks_name] << "]" <<  " = " << tempValue.toString().toUtf8() << "==" << val << "\n";
//            if (iteration_count[kks_name] < N-1)
//            {

//            	iteration_count[kks_name]++;
//            }
//            else
//            {
//            	double mean = 0;

//            	for (int j = 0; j < N; j ++)
//            	{
//            		std::cout<<slice_data[kks_name][j]<<" ";
//            		mean += slice_data[kks_name][j];
//            	}
//            	mean/=N;
//            	std::cout << kks_name << " = " << mean << "\n";
//				/* Create SQL statement */
//				sql = std::string("INSERT INTO dynamic_data (id,t,val) VALUES (\"") +
//						kks_name +
//				   std::string("\" , DateTime('now'), ") +
//						   std::to_string(mean) + ");";
//				//printf("%s\n",sql.c_str());
//				/* Execute SQL statement */
//				rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
//				if( rc != SQLITE_OK ){
//				fprintf(stderr, "SQL error: %s\n", zErrMsg);
//					sqlite3_free(zErrMsg);
//				}
//				for (int j = 0; j < N; j ++)
//				{
//					slice_data[kks_name][j] = 0;
//				}
//				iteration_count[kks_name] = 0;
//            }
        }
        else
        {
            UaStatus itemError(dataNotifications[i].Value.StatusCode);
            fprintf(stderr, "  Variable %s failed with status %s\n", kks_array[dataNotifications[i].ClientHandle].c_str(), itemError.toString().toUtf8());
        }
    }
//    printf("------------------------------------------------------------\n");



}

void SampleSubscription::newEvents(
    OpcUa_UInt32                clientSubscriptionHandle, //!< [in] Client defined handle of the affected subscription
    UaEventFieldLists&          eventFieldList)           //!< [in] List of event notifications sent by the server
{
    OpcUa_ReferenceParameter(clientSubscriptionHandle);
    OpcUa_ReferenceParameter(eventFieldList);
}

void SampleSubscription::init_db()
{
	/* Open database */
	char *zErrMsg = 0;
	int rc = sqlite3_open("data.sqlite", &db);
	if( rc ) {
	   fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	}
	/* Create SQL statement */
	std::string sql = std::string("DROP TABLE IF EXISTS dynamic_data; CREATE TABLE dynamic_data ( id text, t timestamp without time zone NOT NULL, val real )");
	printf("%s\n",sql.c_str());
	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
	if( rc != SQLITE_OK )
	{
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}

UaStatus SampleSubscription::createSubscription(UaSession* pSession)
{

    if ( m_pSubscription )
    {
        fprintf(stderr, "\nError: Subscription already created\n");
        return OpcUa_BadInvalidState;
    }

    m_pSession = pSession;


    UaStatus result;

    ServiceSettings serviceSettings;
    SubscriptionSettings subscriptionSettings;
    subscriptionSettings.publishingInterval = delta;

//    printf("\nCreating subscription ...\n");
    result = pSession->createSubscription(
        serviceSettings,
        this,
        1,
        subscriptionSettings,
        OpcUa_True,
        &m_pSubscription);

    if (result.isGood())
    {
//        printf("CreateSubscription succeeded\n");
    }
    else
    {
        m_pSubscription = NULL;
        fprintf(stderr, "CreateSubscription failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

UaStatus SampleSubscription::deleteSubscription()
{
    if ( m_pSubscription == NULL )
    {
        fprintf(stderr, "\nError: No Subscription created\n");
        return OpcUa_BadInvalidState;
    }

    UaStatus result;
    ServiceSettings serviceSettings;

    // let the SDK cleanup the resources for the existing subscription
//    printf("\nDeleting subscription ...\n");
    result = m_pSession->deleteSubscription(
        serviceSettings,
        &m_pSubscription);

    if (result.isGood())
    {
//        printf("DeleteSubscription succeeded\n");
    }
    else
    {
        fprintf(stderr, "DeleteSubscription failed with status %s\n", result.toString().toUtf8());
    }
    m_pSubscription = NULL;

    return result;
}


UaStatus SampleSubscription::createMonitoredItems()
{
    if ( m_pSubscription == NULL )
    {
        fprintf(stderr, "\nError: No Subscription created\n");
        return OpcUa_BadInvalidState;
    }
//    printf("create items\n");
    UaStatus result;
    OpcUa_UInt32 i;
    ServiceSettings serviceSettings;
    UaMonitoredItemCreateRequests itemsToCreate;
    itemsToCreate.create(1);
    
    UaMonitoredItemCreateResults createResults;

    std::fstream infile(kks_file.c_str());
    int ns = 1;
    int item_index = 0;
    std::string kks;
    while (infile >> kks)
    {
//        printf("%s\n", kks.c_str());
    	itemsToCreate.resize(item_index+1);
    	itemsToCreate[item_index].ItemToMonitor.AttributeId = OpcUa_Attributes_Value;
    	UaNodeId test(UaString(kks.c_str()),ns);
    	test.copyTo(&itemsToCreate[item_index].ItemToMonitor.NodeId);
    	itemsToCreate[item_index].RequestedParameters.ClientHandle = item_index;
    	itemsToCreate[item_index].RequestedParameters.SamplingInterval = 100;
    	itemsToCreate[item_index].RequestedParameters.QueueSize = 1;
    	itemsToCreate[item_index].RequestedParameters.DiscardOldest = OpcUa_True;
    	itemsToCreate[item_index].MonitoringMode = OpcUa_MonitoringMode_Reporting;
//        printf("%d%s\n",item_index,kks.c_str());
	item_index++;
    }
    infile.close();

    kks_array = new std::string[item_index];
    std::fstream infile_reoprn(kks_file.c_str());
    item_index = 0;
    while (infile_reoprn >> kks)
    {
    	kks_array[item_index] = kks;
    	iteration_count[kks]=0;
    	item_index++;
	}

//    init_db();

//    printf("\nAdding monitored items to subscription ...\n");
    result = m_pSubscription->createMonitoredItems(
        serviceSettings,
        OpcUa_TimestampsToReturn_Both,
        itemsToCreate,
        createResults);

    if (result.isGood())
    {
        // check individual results
        for (i = 0; i < createResults.length(); i++)
        {
            if (OpcUa_IsGood(createResults[i].StatusCode))
            {
//                printf("CreateMonitoredItems succeeded for item: %s\n",
//                    UaNodeId(itemsToCreate[i].ItemToMonitor.NodeId).toXmlString().toUtf8());
            }
            else
            {
                fprintf(stderr, "CreateMonitoredItems failed for item: %s - Status %s\n",
                    UaNodeId(itemsToCreate[i].ItemToMonitor.NodeId).toXmlString().toUtf8(),
                    UaStatus(createResults[i].StatusCode).toString().toUtf8());
            }
        }
    }
    // service call failed
    else
    {
        fprintf(stderr, "CreateMonitoredItems failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}
