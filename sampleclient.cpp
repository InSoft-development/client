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
#include "sampleclient.h"
#include "uasession.h"
#include "samplesubscription.h"
#include "uasettings.h"
#include "libtrace.h"
#include "uaeventfilter.h"
#include "uaargument.h"
#include "uacertificatedirectoryobject.h"
#include "uatrustlistobject.h"
#include <iostream>
#include <uadir.h>
#include "uathread.h"
#include <fstream>
#include <string.h>

SampleClient::SampleClient(int d, int m, int n = 1, bool r=false, bool b=false)
{
    m_pSession = new UaSession();
    rewrite = r;
    db = NULL;
    delta = d;
    mean = m;
    ns = n;
    read_bad = b;
    m_pSampleSubscription = new SampleSubscription(delta);

    init_db();
}

SampleClient::~SampleClient()
{
    if (m_pSampleSubscription)
    {
        // delete local subscription object
        delete m_pSampleSubscription;
        m_pSampleSubscription = NULL;
    }

    if (m_pSession)
    {
        // disconnect if we're still connected
        if (m_pSession->isConnected() != OpcUa_False)
        {
            ServiceSettings serviceSettings;
            m_pSession->disconnect(serviceSettings, OpcUa_True);
        }
        delete m_pSession;
        m_pSession = NULL;
    }

    if (db)
    {
        std::cout<<"Close DB\n";
        sqlite3_close(db);
    }
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
   int i;
   for(i = 0; i<argc; i++) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

void SampleClient::init_db()
{
    std::fstream infile("kks.csv");
    std::string kks;

    while (infile >> kks)
    {
        kks_array.push_back(kks);
        slice_data[kks] = std::vector<double>();
    }

    /* Open database */
    char *zErrMsg = 0;
    int rc = sqlite3_open("data.sqlite", &db);
    if( rc ) {
       fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    }
    if (rewrite)
    {
        /* Create SQL statement */
        std::string kks_string;
        for (auto k : kks_array)
        {
            kks_string += k;
            kks_string += "\" real, \"";
        }
        if (!kks_string.empty())
            kks_string.erase(kks_string.size()-3);

        std::string sql = std::string("DROP TABLE IF EXISTS synchro_data; CREATE TABLE synchro_data ( \"") + kks_string +
                                      std::string(", \"timestamp\" timestamp with time zone NOT NULL );");
        printf("%s\n",sql.c_str());
        /* Execute SQL statement */
        rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
        if( rc != SQLITE_OK )
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }

        /* Create SQL statement */
        sql = std::string("DROP TABLE IF EXISTS dynamic_data; CREATE TABLE dynamic_data ( id text, t timestamp without time zone NOT NULL, val real, status text )");
        printf("%s\n",sql.c_str());
        /* Execute SQL statement */
        rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
        if( rc != SQLITE_OK )
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
    }


}

void SampleClient::connectionStatusChanged(
    OpcUa_UInt32             clientConnectionId,
    UaClient::ServerStatus   serverStatus)
{
    OpcUa_ReferenceParameter(clientConnectionId);

    printf("-------------------------------------------------------------\n");
    switch (serverStatus)
    {
    case UaClient::Disconnected:
        printf("Connection status changed to Disconnected\n");
        break;
    case UaClient::Connected:
        printf("Connection status changed to Connected\n");
        break;
    case UaClient::ConnectionWarningWatchdogTimeout:
        printf("Connection status changed to ConnectionWarningWatchdogTimeout\n");
        break;
    case UaClient::ConnectionErrorApiReconnect:
        printf("Connection status changed to ConnectionErrorApiReconnect\n");
        break;
    case UaClient::ServerShutdown:
        printf("Connection status changed to ServerShutdown\n");
        break;
    case UaClient::NewSessionCreated:
        printf("Connection status changed to NewSessionCreated\n");
        break;
    }
    printf("-------------------------------------------------------------\n");
}

UaStatus SampleClient::connect()
{
    UaStatus result;

    // For now we use a hardcoded URL to connect to the local DemoServer
    std::fstream infile("server.conf");
    std::string url;
    infile >> url;
    UaString sURL(url.c_str());

    // Provide information about the client
    SessionConnectInfo sessionConnectInfo;
    UaString sNodeName("unknown_host");
    char szHostName[256];
    if (0 == UA_GetHostname(szHostName, 256))
    {
        sNodeName = szHostName;
    }
    sessionConnectInfo.sApplicationName = "Unified Automation Getting Started Client";
    // Use the host name to generate a unique application URI
    sessionConnectInfo.sApplicationUri  = UaString("urn:%1:UnifiedAutomation:GettingStartedClient").arg(sNodeName);
    sessionConnectInfo.sProductUri      = "urn:UnifiedAutomation:GettingStartedClient";
    sessionConnectInfo.sSessionName     = sessionConnectInfo.sApplicationUri;

    // Security settings are not initialized - we connect without security for now
    SessionSecurityInfo sessionSecurityInfo;

    printf("\nConnecting to %s\n", sURL.toUtf8());
    result = m_pSession->connect(
        sURL,
        sessionConnectInfo,
        sessionSecurityInfo,
        this);

    if (result.isGood())
    {
        printf("Connect succeeded\n");
    }
    else
    {
        printf("Connect failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

UaStatus SampleClient::disconnect()
{
    UaStatus result;

    // Default settings like timeout
    ServiceSettings serviceSettings;

    printf("\nDisconnecting ...\n");
    result = m_pSession->disconnect(
        serviceSettings,
        OpcUa_True);

    if (result.isGood())
    {
        printf("Disconnect succeeded\n");
    }
    else
    {
        printf("Disconnect failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

UaStatus SampleClient::read()
{
    static int iteration_count;
    UaStatus          result;
    ServiceSettings   serviceSettings;
    UaReadValueIds    nodeToRead;
    nodeToRead.create(1);
    UaDataValues      values;
    UaDiagnosticInfos diagnosticInfos;
    int item_index = 0;
    for (auto kks : kks_array)
    {

        // Configure one node to read
    	// We read the value of the ServerStatus -> CurrentTime
    	nodeToRead.resize(item_index+1);
    	nodeToRead[item_index].AttributeId = OpcUa_Attributes_Value;
    	UaNodeId test(UaString(kks.c_str()),ns);
    	test.copyTo(&nodeToRead[item_index].NodeId);
    	result = m_pSession->read(
        	serviceSettings,
        	0,
        	OpcUa_TimestampsToReturn_Both,
        	nodeToRead,
        	values,
        	diagnosticInfos);

    	if (result.isGood())
    	{
        	// Read service succeded - check status of read value
            if (read_bad || OpcUa_IsGood(values[0].StatusCode))
        	{
                    UaVariant tempValue = values[0].Value;
                    OpcUa_Double val;
                    tempValue.toDouble(val);
                    double value = val;
                    slice_data[kks].push_back(value);
                    printf("%s : %f\n", UaNodeId(nodeToRead[item_index].NodeId).toXmlString().toUtf8(), value);



        	}	
        	else
        	{
            		printf("Read failed for %s with status %s\n", UaNodeId(nodeToRead[item_index].NodeId).toXmlString().toUtf8(),UaStatus(values[0].StatusCode).toString().toUtf8());
        	}
    	}
   	else
    	{
        	// Service call failed
        	printf("Read failed with status %s\n", result.toString().toUtf8());
		break;
    	}
    }

    if (iteration_count == mean-1 )
    {
        std::string kks_string;
        std::string value_string;
        for (auto k : kks_array)
        {
            kks_string += "\"" + k + "\",";
            if (slice_data[k].size())
                value_string += std::to_string(std::accumulate(slice_data[k].begin(), slice_data[k].end(), 0.0)/ slice_data[k].size()) + ",";
            else
                value_string += "null,";
        }
        kks_string += "\"timestamp\"";
        value_string += "CURRENT_TIMESTAMP";

        std::string sql = std::string("INSERT INTO synchro_data ( ") + kks_string + ") VALUES(" +
                value_string + ");";
        std::cout<< "\n SQL:\n" << sql<< "\n";
        /* Execute SQL statement */
        char *zErrMsg = NULL;
        int rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
        if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        iteration_count = 0;

    }
    else
        iteration_count++;

    return result;
}

UaStatus SampleClient::readHistory(const char* t1, const char* t2, int pause, int timeout, bool read_bounds)
{
    //std::ofstream data ("data.csv");
    //data<<"kks;value;timestamp;status\n";
    UaStatus                      status;
	ServiceSettings               serviceSettings;
	serviceSettings.callTimeout = timeout;
	HistoryReadRawModifiedContext historyReadRawModifiedContext;


	UaDiagnosticInfos             diagnosticInfos;

	// Read the last 30 minutes
	UaDateTime myTime;

	//UaString sStartTime("2020-12-13T00:00:00");
	UaString sStartTime(t1);//"2021-06-01T00:00:00Z");
	historyReadRawModifiedContext.startTime = UaDateTime::fromString(sStartTime);
	UaString sEndTime(t2);//"2022-10-01T00:00:00Z");
	historyReadRawModifiedContext.endTime = UaDateTime::fromString(sEndTime);;
	historyReadRawModifiedContext.returnBounds = read_bounds ? OpcUa_True : OpcUa_False;
	//historyReadRawModifiedContext.numValuesPerNode = 10;
	// Read four aggregates from one node



    std::fstream infile("kks.csv");
    std::ofstream failed_kks("failed_kks.csv");
    int item_index = 0;
    std::string kks;

    char *zErrMsg = 0;
    int rc;
    std::string sql;

    while (infile >> kks)
    {
    	UaHistoryReadValueIds         nodesToRead;
    	nodesToRead.create(1);
    	HistoryReadDataResults        results;
    	//nodesToRead.resize(item_index+1);
    	//std::string s = "Sochi2.UNIT.AM.";
    	//std::string q = ".PV";//"-AM.Q";
    	std::string node_id = kks;//+q;
    	UaNodeId nodeToRead(UaString(node_id.c_str()),ns);
    	nodeToRead.copyTo(&nodesToRead[item_index].NodeId);


    	/*********************************************************************
         Update the history of events at an event notifier object
    	 **********************************************************************/
    	status = m_pSession->historyReadRawModified(
    			serviceSettings,
				historyReadRawModifiedContext,
				nodesToRead,
				results,
				diagnosticInfos);
    	/*********************************************************************/
        if ( status.isNotGood() )
    	{
    		printf("** Error: %s UaSession::historyReadRawModified failed [ret=%s]\n", kks.c_str(), status.toString().toUtf8());
    		failed_kks << kks << "\n";
    		continue;//return status;
    	}
    	else
    	{
    		OpcUa_UInt32 i, j;
    		for ( i=0; i<results.length(); i++ )
    		{
    			UaStatus nodeResult(results[i].m_status);
                printf("** Results %d Node=%s status=%s  length=%d\n", i, nodeToRead.toXmlString().toUtf8(), nodeResult.toString().toUtf8(), results[i].m_dataValues.length());
                if ( nodeResult.isNotGood() )
                {
                    failed_kks << kks << "\n";
                }
                sql = std::string("BEGIN;\n");
    			for ( j=0; j<results[i].m_dataValues.length(); j++ )
    			{
    				UaStatus statusOPLevel(results[i].m_dataValues[j].StatusCode);
    				std::string sourceTS = UaDateTime(results[i].m_dataValues[j].SourceTimestamp).toString().toUtf8();
    				sourceTS.pop_back();
    				sourceTS[10] = ' ';
                    if ( read_bad || OpcUa_IsGood(results[i].m_dataValues[j].StatusCode) )
    				{
    					UaVariant tempValue = results[i].m_dataValues[j].Value;
                        sql += std::string("INSERT INTO dynamic_data (id,t,val,status) VALUES (\"") +
                                kks + "\" , \"" + sourceTS.c_str() + "\", " +
                                tempValue.toString().toUtf8() + ", \"" +
                                statusOPLevel.toString().toUtf8() + "\" );\n";
                        //printf("%s\n",sql.c_str());

                    }
//    				else
//    				{
//    					fprintf(fp,"%s; ;%s;%s\n",
//    							nodeToRead.toXmlString().toUtf8(),
//								//tempValue.toString().toUtf8(),
//								sourceTS.toString().toUtf8(),
//								statusOPLevel.toString().toUtf8());
//
//    				}
    			}
                sql += std::string("COMMIT;\n");
                /* Execute SQL statement */
                rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
                if( rc != SQLITE_OK ){
                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                    sqlite3_free(zErrMsg);
                }


    		}
    		//printf("****************************************************************\n\n");

    		while ( results[0].m_continuationPoint.length() > 0 )
    		{
    			//    		printf("\n*******************************************************\n");
    			//    		printf("** More data available                          *******\n");
    			//    		printf("**        Press x to stop read                  *******\n");
    			//    		printf("**        Press c to continue                   *******\n");
    			//    		printf("*******************************************************\n");
    			//    		int action;
    			//    		/******************************************************************************/
    			//    		/* Wait for user command. */
    			//    		bool waitForInput = true;
    			//    		while (waitForInput)
    			//    		{
    			//    			if ( WaitForKeypress(action) )
    			//    			{
    			//    				// x -> Release continuation point
    			//    				historyReadRawModifiedContext.bReleaseContinuationPoints = OpcUa_True;
    			//    				waitForInput = false;
    			//    			}
    			//    			// Continue
    			//    			else if ( action == 12 ) waitForInput = false;
    			//    			// Wait
    			//    			else UaThread::msleep(100);
    			//    		}
    			//    		/******************************************************************************/
    			UaThread::msleep(pause);
    			OpcUa_ByteString_Clear(&nodesToRead[0].ContinuationPoint);
    			results[0].m_continuationPoint.copyTo(&nodesToRead[0].ContinuationPoint);
    			status = m_pSession->historyReadRawModified(
    					serviceSettings,
						historyReadRawModifiedContext,
						nodesToRead,
						results,
						diagnosticInfos);
    			if ( status.isBad() )
    			{
    				printf("** Error: %s UaSession::historyReadRawModified with CP failed [ret=%s]\n", kks.c_str(), status.toString().toUtf8());
    				failed_kks << kks << "\n";
    				break;//return status;
    			}
    			else
    			{

    				for ( i=0; i<results.length(); i++ )
    				{
    					UaStatus nodeResult(results[i].m_status);
                        printf("** ContinuationPoint Results %d Node=%s status=%s length=%d\n", i, nodeToRead.toXmlString().toUtf8(), nodeResult.toString().toUtf8(),results[i].m_dataValues.length());
                        sql = std::string("BEGIN;\n");

                        for ( j=0; j<results[i].m_dataValues.length(); j++ )
    					{
    						UaStatus statusOPLevel(results[i].m_dataValues[j].StatusCode);
    						std::string sourceTS = UaDateTime(results[i].m_dataValues[j].SourceTimestamp).toString().toUtf8();
							sourceTS.pop_back();
							sourceTS[10] = ' ';
                            if ( read_bad || OpcUa_IsGood(results[i].m_dataValues[j].StatusCode) )
    						{

                                UaVariant tempValue = results[i].m_dataValues[j].Value;
                                sql += std::string("INSERT INTO dynamic_data (id,t,val,status) VALUES (\"") +
                                        kks + "\" , \"" + sourceTS.c_str() + "\", " +
                                        tempValue.toString().toUtf8() + ", \"" +
                                        statusOPLevel.toString().toUtf8() + "\" );\n";
                                //printf("%s\n",sql.c_str());

                            }
//    						else
//    						{
//    							fprintf(fp,"%s; ;%s;%s\n",
//    									nodeToRead.toXmlString().toUtf8(),
//										//tempValue.toString().toUtf8(),
//										sourceTS.toString().toUtf8(),
//										statusOPLevel.toString().toUtf8());
//    						}
    					}
                        sql += std::string("COMMIT;\n");
                        /* Execute SQL statement */
                        rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
                        if( rc != SQLITE_OK ){
                        fprintf(stderr, "SQL error: %s\n", zErrMsg);
                            sqlite3_free(zErrMsg);
                        }
    				}
    			}
    		}
    	}
    }

    return status;

}

UaStatus SampleClient::subscribe()
{
    UaStatus result;

    result = m_pSampleSubscription->createSubscription(m_pSession);
    if ( result.isGood() )
    {
        result = m_pSampleSubscription->createMonitoredItems();
    }
    return result;
}

UaStatus SampleClient::unsubscribe()
{
    return m_pSampleSubscription->deleteSubscription();
}

UaStatus SampleClient::browseSimple(std::string kks)//const UaNodeId& nodeToBrowse, OpcUa_UInt32 maxReferencesToReturn)
{
    UaStatus result;
    UaNodeId nodeToBrowse;
    bool recursive = false;
    if (kks == "all")
    {
        nodeToBrowse = UaNodeId(OpcUaId_RootFolder);
        recursive = true;
    }
    if (kks == "begin")
        nodeToBrowse = UaNodeId(OpcUaId_ObjectsFolder);
    else
        nodeToBrowse = UaNodeId(UaString(kks.c_str()),ns);
    result = browseInternal(nodeToBrowse, 0, recursive);
    return result;
}

UaStatus SampleClient::browseInternal(const UaNodeId& nodeToBrowse, OpcUa_UInt32 maxReferencesToReturn,bool recursive)
{
    UaStatus result;
    ServiceSettings serviceSettings;
    BrowseContext browseContext;
    UaByteString continuationPoint;
    UaReferenceDescriptions referenceDescriptions;
    // configure browseContext
    browseContext.browseDirection = OpcUa_BrowseDirection_Forward;
    browseContext.referenceTypeId = OpcUaId_HierarchicalReferences;
    browseContext.includeSubtype = OpcUa_True;
    browseContext.maxReferencesToReturn = maxReferencesToReturn;
    printf("\nBrowsing from Node %s...\n", nodeToBrowse.toXmlString().toUtf8());
    result = m_pSession->browse(
        serviceSettings,
        nodeToBrowse,
        browseContext,
        continuationPoint,
        referenceDescriptions);
    if (result.isGood())
    {
        // print results
        printBrowseResults(referenceDescriptions);
        if (recursive)
            for (OpcUa_UInt32 i=0; i<referenceDescriptions.length(); i++)
            {
                browseInternal(referenceDescriptions[i].NodeId.NodeId,0,recursive);
            }
        // continue browsing
        while (continuationPoint.length() > 0)
        {
            printf("\nContinuationPoint is set. BrowseNext...\n");
            // browse next
            result = m_pSession->browseNext(
                serviceSettings,
                OpcUa_False,
                continuationPoint,
                referenceDescriptions);
            if (result.isGood())
            {
                // print results
                printBrowseResults(referenceDescriptions);
                if (recursive)
                    for (OpcUa_UInt32 i=0; i<referenceDescriptions.length(); i++)
                    {
                        browseInternal(referenceDescriptions[i].NodeId.NodeId,0,recursive);
                    }
            }
            else
            {
                // Service call failed
                printf("BrowseNext failed with status %s\n", result.toString().toUtf8());
            }
        }
    }
    else
    {
        // Service call failed
        printf("Browse failed with status %s\n", result.toString().toUtf8());
    }
    return result;
}

void SampleClient::printBrowseResults(const UaReferenceDescriptions& referenceDescriptions)
{
    OpcUa_UInt32 i;
    for (i=0; i<referenceDescriptions.length(); i++)
    {
        //printf("node: ");
        UaNodeId referenceTypeId(referenceDescriptions[i].ReferenceTypeId);
        //printf("[Ref=%s] ", referenceTypeId.toString().toUtf8() );
        UaQualifiedName browseName(referenceDescriptions[i].BrowseName);
        printf("%s\n", browseName.toString().toUtf8() );
//        printf("%s ( ", browseName.toString().toUtf8() );
//        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Object) printf("Object ");
//        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Variable) printf("Variable ");
//        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Method) printf("Method ");
//        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_ObjectType) printf("ObjectType ");
//        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_VariableType) printf("VariableType ");
//        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_ReferenceType) printf("ReferenceType ");
//        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_DataType) printf("DataType ");
//        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_View) printf("View ");
//        UaNodeId nodeId(referenceDescriptions[i].NodeId.NodeId);
//        printf("[NodeIdString=%s] ", nodeId.toFullString().toUtf8() );
//        printf("[NodeId=%d] ", nodeId. identifierNumeric());
//        printf(")\n");
    }
}

UaStatus SampleClient::returnNames()//const UaNodeId& nodeToBrowse, OpcUa_UInt32 maxReferencesToReturn)
{
    UaStatus result;
    ServiceSettings   serviceSettings;
    UaReadValueIds    nodeToRead;
    nodeToRead.create(1);
    UaDataValues      values;
    UaDiagnosticInfos diagnosticInfos;
    int item_index = 0;
    for (auto kks : kks_array)
    {

        // Configure one node to read
        // We read the value of the ServerStatus -> CurrentTime
        nodeToRead.resize(item_index+1);
        nodeToRead[item_index].AttributeId = OpcUa_Attributes_Description;
        UaNodeId test(UaString(kks.c_str()),ns);
        test.copyTo(&nodeToRead[item_index].NodeId);
        result = m_pSession->read(serviceSettings,
                                  0,
                                  OpcUa_TimestampsToReturn_Both,
                                  nodeToRead,
                                  values,
                                  diagnosticInfos);


        if (result.isGood())
        {
            // Read service succeded - check status of read value
            if (read_bad || OpcUa_IsGood(values[0].StatusCode))
            {
                    UaVariant tempValue = values[0].Value;
                    UaLocalizedText val;
                    tempValue.toLocalizedText(val);
                    printf("%s : %s\n", UaNodeId(nodeToRead[item_index].NodeId).toXmlString().toUtf8(), UaString(val.text()).toUtf8());
            }
            else
            {
                    printf("Read failed for %s with status %s\n", UaNodeId(nodeToRead[item_index].NodeId).toXmlString().toUtf8(),UaStatus(values[0].StatusCode).toString().toUtf8());
            }
        }
    else
        {
            // Service call failed
            printf("Read failed with status %s\n", result.toString().toUtf8());
        break;
        }
    }

    return result;
}
