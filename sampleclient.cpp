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
#include <signal.h>
volatile static bool exit_flag = false;
volatile static bool browse_internal = false;


void signalHandler_for_browse(int signum)
{

       printf("Interrupt in client!\n");
       if (browse_internal)
           exit_flag = true;
       else {
           raise(signum);
       }
}

SampleClient::SampleClient(int d, int m, int n = 1, bool r=false, bool b=false, std::string c = "",std::string f = "")
{
    m_pSession = new UaSession();
    db = nullptr;
    delta = d;
    mean = m;
    ns = n;
    read_bad = b;
    if (c !="" && f !="")\
    {
        printf("Can not use clickhouse and csv together\n");
        exit(1);
    }
    if (c == "" && f == "")
    {
        printf("using local data.sqlite\n");
        db = new sqlite_database(r,"data.sqlite");

    }
    else if (c != "")
    {
        printf("using clickhouse database\n");
        db = new clickhouse_database(r, c.c_str());

    }
    else if (f != "")
    {
        //
        if (f.substr(f.size() - 6) == "sqlite")
        {
            printf("using local %s database\n",f.c_str());
            db = new sqlite_database(r,f.c_str());
        }
        else
        {
            printf("using local %s csv file\n", f.c_str());
            csv_fstream.open(f);
            csv_fstream<<"id, timestamp, value, code\n";
        }

    }
    m_pSampleSubscription = new SampleSubscription(delta);
}

SampleClient::~SampleClient()
{
    if (m_pSampleSubscription)
    {
        // delete local subscription object
        delete m_pSampleSubscription;
        m_pSampleSubscription = NULL;
    }

    if (m_pSession->isConnected() == OpcUa_True)
    {
        // disconnect if we're still connected
        disconnect();
        delete m_pSession;
        m_pSession = NULL;
    }

    if (db != nullptr)
    {
        std::cout<<"Close DB\n";
        delete db;
        std::cout<<"DB Closed\n";
    }
    if  (csv_fstream.is_open())
    {
        csv_fstream.close();
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
    db->init_db(kks_array);
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
        printf("\nConnection status changed to Disconnected\n");
        break;
    case UaClient::Connected:
        printf("\nConnection status changed to Connected\n");
        break;
    case UaClient::ConnectionWarningWatchdogTimeout:
        fprintf(stderr,"Error: Connection status changed to ConnectionWarningWatchdogTimeout\n");
        break;
    case UaClient::ConnectionErrorApiReconnect:
        fprintf(stderr,"Error: Connection status changed to ConnectionErrorApiReconnect\n");
        break;
    case UaClient::ServerShutdown:
        fprintf(stderr,"Error: Connection status changed to ServerShutdown\n");
        break;
    case UaClient::NewSessionCreated:
        fprintf(stderr, "Error: Connection status changed to NewSessionCreated\n");
        break;
    }
    printf("-------------------------------------------------------------\n");
}

UaStatus SampleClient::connect(std::string server_opt)
{
    UaStatus result;
    if (server_opt != "")
        url = server_opt;
    else{
        // For now we use a hardcoded URL to connect to the local DemoServer
        std::fstream infile("server.conf");
        infile >> url;
    }

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
        fprintf(stderr, "Error: Connect failed with status %s\n", result.toString().toUtf8());
    }

//    std::cout<<"m_pSession->getEndpointUrl().toUtf8() "<<m_pSession->getEndpointUrl().toUtf8();
//    std::cout<<"m_pSession->getServerProductUri().toUtf8() "<<m_pSession->getServerProductUri().toUtf8();
//    std::cout<<"m_pSession->getServerApplicationUri().toUtf8() "<<m_pSession->getServerApplicationUri().toUtf8();
//    std::cout<<"m_pSession->currentlyUsedEndpointUrl().toUtf8() "<<m_pSession->currentlyUsedEndpointUrl().toUtf8();


    return result;
}

UaStatus SampleClient::disconnect()
{

    if (m_pSession->isConnected() == OpcUa_False)
        return UaStatus(OpcUa_Good);
    else {
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
            fprintf(stderr, "Error: Disconnect failed with status %s\n", result.toString().toUtf8());
        }

        return result;
    }
}

UaStatus SampleClient::reconnect(int p)
{

    if (!m_pSession) //->isConnected() == OpcUa_False)
        fprintf(stderr, "Error: Session null in reconnect\n");
    else
         disconnect();
    UaThread::msleep(p);
    UaStatus result = connect(url);
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

    std::string kks_string;
    for (auto k : kks_array)
    {
        kks_string += "\'" + k + "\',";
    }
    kks_string += "\'timestamp\'";

    if (db)
    {
        db->init_db(kks_array);
        db->init_synchro(kks_array);
    }
    else
        csv_fstream<<kks_string<<"\n";


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
                    fprintf(stderr, "Error: Read failed for %s with status %s\n", UaNodeId(nodeToRead[item_index].NodeId).toXmlString().toUtf8(),UaStatus(values[0].StatusCode).toString().toUtf8());
        	}
    	}
   	else
    	{
        	// Service call failed
            fprintf(stderr, "Error: Read failed with status %s\n", result.toString().toUtf8());
		break;
    	}
    }

    if (iteration_count == mean-1 )
    {

        std::string value_string;
        for (auto k : kks_array)
        {
            if (slice_data[k].size())
                value_string += std::to_string(std::accumulate(slice_data[k].begin(), slice_data[k].end(), 0.0)/ slice_data[k].size()) + ",";
            else
                value_string += "null,";
        }
        value_string += "CURRENT_TIMESTAMP";

        std::string sql = std::string("INSERT INTO synchro_data ( ") + kks_string + ") VALUES(" +
                value_string + ");";
        std::cout<< "\n SQL:\n" << sql<< "\n";
        /* Execute SQL statement */
        if (db)
            db->exec(sql.c_str());
        else
            csv_fstream<<value_string<<"\n";


        iteration_count = 0;

    }
    else
        iteration_count++;

    return result;
}

UaStatus SampleClient::readHistory(const char* t1, const char* t2, int pause, int timeout, bool read_bounds)
{

    if (db)
        init_db();
//    return 0;
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

    std::string sql;
    int id = 0;
    int N_rows = 0;
    while (infile >> kks)
    {
        if (db)
            id = db->id(kks);
        else
            ++id;
        std::cout<<"\n\nID: "<<id<<"\n\n";
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
            fprintf(stderr, "** Error: %s UaSession::historyReadRawModified failed [ret=%s]\n", kks.c_str(), status.toString().toUtf8());
//            printf("** id %d Node=%s status=%s \n",id, nodeToRead.toXmlString().toUtf8(), status.toString().toUtf8());
            failed_kks << kks << " " << status.toString().toUtf8() << "\n";
    		continue;//return status;
    	}
    	else
    	{
    		OpcUa_UInt32 i, j;
    		for ( i=0; i<results.length(); i++ )
    		{
                if (results[i].m_dataValues.length() ==0)
                {
                    printf("** id %d Node=%s status=empty_data_warning\n",id, nodeToRead.toXmlString().toUtf8());
                    break;
                }
                std::string first_value = (UaVariant(results[i].m_dataValues[0].Value).toString().toUtf8());
                if (first_value.find_first_not_of("0123456789.,-Ee") != std::string::npos &&
                        first_value != "true" && first_value != "false")
                {
                    failed_kks << kks << " text field \n";
                    printf("** id %d Node=%s status=text_field_warning \n",id, nodeToRead.toXmlString().toUtf8());
                    break;

                }
    			UaStatus nodeResult(results[i].m_status);
                printf("** id %d Results %d Node=%s status=%s  length=%d\n",id, i, nodeToRead.toXmlString().toUtf8(), nodeResult.toString().toUtf8(), results[i].m_dataValues.length());
                N_rows += results[i].m_dataValues.length();
                if ( nodeResult.isNotGood() )
                {
                    failed_kks << kks << " " << status.toString().toUtf8() << "\n";
                }
                sql = std::string("INSERT INTO dynamic_data (id,t,val,status) VALUES ");
                bool not_empty = false;
    			for ( j=0; j<results[i].m_dataValues.length(); j++ )
    			{
    				UaStatus statusOPLevel(results[i].m_dataValues[j].StatusCode);
    				std::string sourceTS = UaDateTime(results[i].m_dataValues[j].SourceTimestamp).toString().toUtf8();
    				sourceTS.pop_back();
    				sourceTS[10] = ' ';
                    if ( read_bad || OpcUa_IsGood(results[i].m_dataValues[j].StatusCode) )
    				{
                        not_empty = true;
                        std::string value = UaVariant(results[i].m_dataValues[j].Value).toString().toUtf8();
                        if (!db) // using local csv file
                            csv_fstream<<kks<<","<<sourceTS.c_str()<<"," <<
                                         value << ",\'" <<
                                         statusOPLevel.toString().toUtf8()<<"'\n";
                        else
                        {
                            if (value == "true") value = "1";
                            if (value == "false") value = "0";

                            sql += std::string(" (") +
                                std::to_string(id) + " , \'" + sourceTS + "\', " +
                                value + ", " + std::to_string(results[i].m_dataValues[j].StatusCode) + "),\n";
                                //value + ", \'" +
                                //statusOPLevel.toString().toUtf8() + "\' ),\n";
                        }


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
                sql.pop_back();
                sql.pop_back();
                sql += ";";
                if (not_empty)
                {
                    if (db)
                    {
//                        printf("1: %s\n", sql.c_str());
                        db->exec( sql.c_str());
                    }
                    else
                        csv_fstream.flush();
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
                    fprintf(stderr, "** Error: %s UaSession::historyReadRawModified with CP failed [ret=%s]\n", kks.c_str(), status.toString().toUtf8());
                    failed_kks << kks << " " << status.toString().toUtf8() << "\n";
//                    printf("** id %d Node=%s status=%s \n",id, nodeToRead.toXmlString().toUtf8(), status.toString().toUtf8());
    				break;//return status;
    			}
    			else
    			{

    				for ( i=0; i<results.length(); i++ )
    				{
    					UaStatus nodeResult(results[i].m_status);
                        printf("** ContinuationPoint id %d Results %d Node=%s status=%s length=%d\n", id, i, nodeToRead.toXmlString().toUtf8(), nodeResult.toString().toUtf8(),results[i].m_dataValues.length());
                        N_rows += results[i].m_dataValues.length();
                        sql = std::string("INSERT INTO dynamic_data (id,t,val,status) VALUES ");

                        for ( j=0; j<results[i].m_dataValues.length(); j++ )
    					{
    						UaStatus statusOPLevel(results[i].m_dataValues[j].StatusCode);
    						std::string sourceTS = UaDateTime(results[i].m_dataValues[j].SourceTimestamp).toString().toUtf8();
							sourceTS.pop_back();
							sourceTS[10] = ' ';
                            if ( read_bad || OpcUa_IsGood(results[i].m_dataValues[j].StatusCode) )
    						{

                                std::string value = UaVariant(results[i].m_dataValues[j].Value).toString().toUtf8();
                                if (!db) // using local csv file
                                    csv_fstream<<kks<<","<<sourceTS.c_str()<<"," <<
                                                 value << ",\'" <<
                                                 statusOPLevel.toString().toUtf8()<<"'\n";
                                else
                                {
                                    if (value == "true") value = "1";
                                    if (value == "false") value = "0";

                                    sql += std::string(" (") +
                                        std::to_string(id) + " , \'" + sourceTS + "\', " +
                                        value + ", " + std::to_string(results[i].m_dataValues[j].StatusCode) + "),\n";

//                                        value + ", \'" +
//                                        statusOPLevel.toString().toUtf8() + "\' ),\n";
                                }



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
                        sql.pop_back();
                        sql.pop_back();
                        sql += ";";
                        if (j>0)
                        {

                            if (db)
                            {
//                                printf("2: %s\n", sql.c_str());
                                db->exec( sql.c_str());
                            }
                            else
                                csv_fstream.flush();
                        }

    				}
    			}
    		}
    	}
        std::cout<<"\nN_rows="<<N_rows<<"\n";
        if (N_rows > 0)
        {
            N_rows = 0;
            std::cout<<"\nKKS WITH HISTORY: "<<kks<<"\n";

            reconnect(pause*3);
        }

    }
    if (db)
    {
        printf("\noptimizing db\n");
        db->finalize_db();
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

UaStatus SampleClient::browseSimple(std::string kks, std::string recursive, std::string csv_file)//const UaNodeId& nodeToBrowse, OpcUa_UInt32 maxReferencesToReturn)
{
    UaStatus result;
    UaNodeId nodeToBrowse;
    if (kks == "all")
    {
        nodeToBrowse = UaNodeId(OpcUaId_RootFolder);
    }
    else if (kks == "begin" or kks == "")
        nodeToBrowse = UaNodeId(OpcUaId_ObjectsFolder);
    else
        nodeToBrowse = UaNodeId(UaString(kks.c_str()),ns);
    if (csv_file != "")
        kks_fstream = std::fopen(csv_file.c_str(), "w");
    else
        kks_fstream = stdout;
    browse_internal = true;
    signal(SIGINT, signalHandler_for_browse);
    signal(SIGTERM, signalHandler_for_browse);

    result = browseInternal(nodeToBrowse, 0, recursive);
    if (csv_file != "") std::fclose(kks_fstream);
    return result;
}

UaStatus SampleClient::browseInternal(const UaNodeId& nodeToBrowse, OpcUa_UInt32 maxReferencesToReturn,std::string  recursive)
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
    browseContext.nodeClassMask=2;
    browseContext.maxReferencesToReturn = maxReferencesToReturn;
//    printf("\nBrowsing from Node %s...\n", nodeToBrowse.toXmlString().toUtf8());
    result = m_pSession->browse(
        serviceSettings,
        nodeToBrowse,
        browseContext,
        continuationPoint,
        referenceDescriptions);
    if (result.isGood())
    {
        // print results
        printBrowseResults(referenceDescriptions, recursive);
        if (recursive != "false")
            for (OpcUa_UInt32 i=0; i<referenceDescriptions.length() && exit_flag != true; i++)
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
                printBrowseResults(referenceDescriptions,recursive);
                if (recursive != "false")
                    for (OpcUa_UInt32 i=0; i<referenceDescriptions.length() && exit_flag != true; i++)
                    {
                        browseInternal(referenceDescriptions[i].NodeId.NodeId,0,recursive);
                    }
            }
            else
            {
                // Service call failed
                fprintf(stderr, "Error: BrowseNext failed with status %s\n", result.toString().toUtf8());
            }
        }
    }
    else
    {
        // Service call failed
        fprintf(stderr, "Error: Browse failed with status %s\n", result.toString().toUtf8());
    }

    return result;

}

void SampleClient::printBrowseResults(const UaReferenceDescriptions& referenceDescriptions, std::string type_match)
{

    UaStatus result;
    ServiceSettings   serviceSettings;
    UaReadValueIds    nodeToRead;
    nodeToRead.create(1);
    UaDataValues      values;
    UaDiagnosticInfos diagnosticInfos;


    OpcUa_UInt32 i;
    for (i=0; i<referenceDescriptions.length(); i++)
    {
        std::string kks_name, type="", description="";
        nodeToRead.resize(1);
        UaNodeId nodeId(referenceDescriptions[i].NodeId.NodeId);
        //std::cout<<nodeId.toString().toUtf8()<<"\n";

        nodeToRead[0].AttributeId = OpcUa_Attributes_Description;
        UaNodeId test(UaString(nodeId.toString().toUtf8()),ns);

        /*
        test.copyTo(&nodeToRead[0].NodeId);
        nodeId.copyTo(&nodeToRead[0].NodeId);
        result = m_pSession->read(serviceSettings,
                                  0,
                                  OpcUa_TimestampsToReturn_Both,
                                  nodeToRead,
                                  values,
                                  diagnosticInfos);


        if (result.isGood())
        {
            if (read_bad || OpcUa_IsGood(values[0].StatusCode))
            {
                    UaVariant tempValue = values[0].Value;
                    UaLocalizedText val;
                    tempValue.toLocalizedText(val);
                    description  = UaString(val.text()).toUtf8();
            }

        }
        else
        {
            // Service call failed
            fprintf(stderr, "read description for %s failed with status %s\n", nodeId.toString().toUtf8(), result.toString().toUtf8());
        }*/

        nodeToRead[0].AttributeId = OpcUa_Attributes_DataType;
        UaNodeId test_type(UaString(nodeId.toString().toUtf8()),ns);
        test.copyTo(&nodeToRead[0].NodeId);
        result = m_pSession->read(serviceSettings,
                                  0,
                                  OpcUa_TimestampsToReturn_Both,
                                  nodeToRead,
                                  values,
                                  diagnosticInfos);

        if (result.isGood())
        {
            if (read_bad || OpcUa_IsGood(values[0].StatusCode))
            {
                UaVariant tempValue = values[0].Value;
                UaNodeId val;
                tempValue.toNodeId(val);
                nodeToRead[0].AttributeId = OpcUa_Attributes_DisplayName;
                val.copyTo(&nodeToRead[0].NodeId);
                result = m_pSession->read(serviceSettings,
                                          0,
                                          OpcUa_TimestampsToReturn_Both,
                                          nodeToRead,
                                          values,
                                          diagnosticInfos);
                if (result.isGood())
                {
                    if (read_bad || OpcUa_IsGood(values[0].StatusCode))
                    {
                        UaVariant tempValue1 = values[0].Value;
                        UaLocalizedText val_type;
                        tempValue1.toLocalizedText(val_type);

                        type = UaString(val_type.text()).toUtf8();
                    }
                }
                else
                {
                    // Service call failed
                    fprintf(stderr, "Error: read data type disaplay name for %s failed with status %s\n", nodeId.toString().toUtf8(), result.toString().toUtf8());
                }

            }
        }
        else
        {
            // Service call failed
            fprintf(stderr, "Error: read data type for %s failed with status %s\n", nodeId.toString().toUtf8(), result.toString().toUtf8());
        }
    if (type_match == "all" ||type_match == "false" || type == type_match)
            fprintf(kks_fstream, "\n%s\n", nodeId.toString().toUtf8());//<<"\n"; //";"<<type<<";\""<< description<<"\"\n";

    }
}

//UaStatus SampleClient::returnNames()//const UaNodeId& nodeToBrowse, OpcUa_UInt32 maxReferencesToReturn)
//{
//    UaStatus result, result_type;
//    ServiceSettings   serviceSettings;
//    UaReadValueIds    nodeToRead,nodeToReadType;
//    nodeToRead.create(1);
//    nodeToReadType.create(1);
//    UaDataValues      values, values_type,values1;
//    UaDiagnosticInfos diagnosticInfos;
//    for (auto kks : kks_array)
//    {

//        //nodeToRead.resize(1);
//        nodeToRead[0].AttributeId = OpcUa_Attributes_Description;
//        UaNodeId test(UaString(kks.c_str()),ns);
//        test.copyTo(&nodeToRead[0].NodeId);
//        result = m_pSession->read(serviceSettings,
//                                  0,
//                                  OpcUa_TimestampsToReturn_Both,
//                                  nodeToRead,
//                                  values,
//                                  diagnosticInfos);
//        //OpcUa_Attributes_DataType
//        nodeToRead[0].AttributeId = OpcUa_Attributes_DataType;
//        UaNodeId test_type(UaString(kks.c_str()),ns);
//        test_type.copyTo(&nodeToRead[0].NodeId);
//        result_type = m_pSession->read(serviceSettings,
//                                  0,
//                                  OpcUa_TimestampsToReturn_Both,
//                                  nodeToRead,
//                                  values_type,
//                                  diagnosticInfos);


//        if (result.isGood() and result_type.isGood())
//        {
//            // Read service succeded - check status of read value
//            if (read_bad || (OpcUa_IsGood(values[0].StatusCode) && OpcUa_IsGood(values_type[0].StatusCode)))
//            {
//                    UaVariant tempValue = values[0].Value;
//                    UaLocalizedText val;
//                    tempValue.toLocalizedText(val);

//                    UaVariant tempValue_type = values_type[0].Value;
//                    UaNodeId val_type;
//                    tempValue_type.toNodeId(val_type);

//                    nodeToReadType[0].AttributeId = OpcUa_Attributes_DisplayName;
//                    val_type.copyTo(&nodeToReadType[0].NodeId);
//                    result = m_pSession->read(serviceSettings,
//                                              0,
//                                              OpcUa_TimestampsToReturn_Both,
//                                              nodeToReadType,
//                                              values1,
//                                              diagnosticInfos);
//                    UaVariant tempValue1 = values1[0].Value;
//                    UaLocalizedText val1;
//                    tempValue1.toLocalizedText(val1);

//                    printf("%s;%s;%s\n", UaNodeId(nodeToRead[0].NodeId).toXmlString().toUtf8(), UaString(val1.text()).toUtf8(), UaString(val.text()).toUtf8());
//                    //printf("%s : %s \n", UaNodeId(nodeToRead[0].NodeId).toXmlString().toUtf8(), UaString(val1.text()).toUtf8());
//            }
//            else
//            {
//                    printf("Read failed for %s with status %s\n", UaNodeId(nodeToRead[0].NodeId).toXmlString().toUtf8(),UaStatus(values[0].StatusCode).toString().toUtf8());
//            }
//        }
//    else
//        {
//            // Service call failed
//            printf("Read failed with status %s\n", result.toString().toUtf8());
//        break;
//        }
//    }

//    return result;
//}

sqlite_database::sqlite_database(bool r,const char* f)
{
    /* Open database */
    int rc = sqlite3_open(f, &sq_db);
    if( rc ) {
       fprintf(stderr, "Error: Can't open database: %s\n", sqlite3_errmsg(sq_db));
    }
    rewrite = r;
}


sqlite_database::~sqlite_database()
{
    sqlite3_close(sq_db);
}

void sqlite_database::init_synchro(std::vector<std::string> kks_array)
{
    printf("init sqlite tables\n");
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
                                      std::string(", \"timestamp\" timestamp );");
        printf("%s\n",sql.c_str());
        /* Execute SQL statement */
        exec(sql.c_str());
    }
}

void sqlite_database::init_db(std::vector<std::string> kks_array)
{
    printf("init sqlite tables\n");
    if (rewrite)
    {
        /* Create SQL statement */
//        std::string kks_string;
//        for (auto k : kks_array)
//        {
//            kks_string += k;
//            kks_string += "\" real, \"";
//        }
//        if (!kks_string.empty())
//            kks_string.erase(kks_string.size()-3);

//        std::string sql = std::string("DROP TABLE IF EXISTS synchro_data; CREATE TABLE synchro_data ( \"") + kks_string +
//                                      std::string(", \"timestamp\" timestamp );");
//        printf("%s\n",sql.c_str());
//        /* Execute SQL statement */
//        exec(sql.c_str());

        std::string  sql = std::string("DROP TABLE IF EXISTS static_data; DROP TABLE IF EXISTS dynamic_data;");
        printf("%s\n",sql.c_str());

        /* Execute SQL statement */
        exec(sql.c_str());
    }
    std::string sql = std::string("CREATE TABLE IF NOT EXISTS static_data ( id int, name text, description text, PRIMARY KEY(\"id\"));");
    printf("%s\n",sql.c_str());
    /* Execute SQL statement */
    exec(sql.c_str());

    /* Create SQL statement */
    sql = std::string("CREATE TABLE IF NOT EXISTS dynamic_data ( id int, t timestamp,"
                      " val real, status int )");

    printf("%s\n",sql.c_str());
    /* Execute SQL statement */
    exec(sql.c_str());

    int i = 0;
    if (!rewrite)
    {
        sql = "SELECT MAX(id) from static_data";
        char *zErrMsg = 0;
        int rc;
        static int id;
        id = -1;
        rc = sqlite3_exec(sq_db, sql.c_str(), [](void *, int argc, char **argv, char **) -> int {
                              if (argc>0 && argv[0] && std::strlen(argv[0])>0) id = std::atoi(argv[0]);
                              return 0;
                           }, 0, &zErrMsg);
        if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else
        {
            if (id > 0)
                i = id;
        }

    }
    std::cout<<"max id = " << i <<"\n";
    bool need_merge = false;
    sql = std::string("INSERT INTO static_data (id,name) VALUES ");
    for (auto k : kks_array)
    {
        if (id(k) < 1)
        {
            sql += "(" + std::to_string(++i) + ", \'" + k + "\'),\n";
            need_merge = true;
        }

    }
    sql.pop_back();
    sql.pop_back();
    sql += ";";

    if (need_merge)
    {
        printf("%s\n",sql.c_str());
        exec( sql.c_str());
    }


    //exec("CREATE INDEX IF NOT EXISTS \"idd\" ON \"dynamic_data\"(\"id\"  ASC)");


}

int sqlite_database::exec(const char* sql)
{
    char *zErrMsg = 0;
    int rc;
    rc = sqlite3_exec(sq_db, sql, callback, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    return rc;

}

int sqlite_database::id(std::string kks)
{
    //std::cout<<kks<<"\n";
    std::string sql = "SELECT id from static_data where name = \"" + kks + "\"";
    char *zErrMsg = 0;
    int rc;
    static int id;
    id = -1;
    rc = sqlite3_exec(sq_db, sql.c_str(), [](void *, int argc, char **argv, char **) -> int {
                          //std::cout<<argv[0]<<"\n";
                          if (argc>0 && argv[0] && std::strlen(argv[0])>0) id = std::atoi(argv[0]);
                          return 0;
                       }, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    }
    else
    {
        //std::cout<<"\n" << kks<<" id = "<<id<<"\n";
        return id;
    }
}


void sqlite_database::finalize_db()
{
    exec("DELETE FROM dynamic_data WHERE rowid NOT IN (\
         SELECT MIN(rowid) FROM dynamic_data GROUP BY id, t, val, status\
       );VACUUM;");
}


clickhouse_database::clickhouse_database(bool r, const char* server)
{
    printf("Initialize client connection\n");
    ch_db = new clickhouse::Client(clickhouse::ClientOptions().SetHost(server)
                                   .SetUser("default")
                                   .SetPassword("asdf"));
    rewrite = r;
}


clickhouse_database::~clickhouse_database()
{
    delete ch_db;
}

void clickhouse_database::init_synchro(std::vector<std::string> kks_array)
{
    printf("init clickhouse tables\n");
    if (rewrite)
    {
        /* Create SQL statement */
        std::string kks_string;
        for (auto k : kks_array)
        {
            kks_string += k;
            kks_string += "\" Float64, \"";
        }
        if (!kks_string.empty())
            kks_string.erase(kks_string.size()-3);

        std::string sql = std::string("DROP TABLE IF EXISTS synchro_data;");
        printf("%s\n",sql.c_str());
        /* Execute SQL statement */
        exec(sql.c_str());
        sql = std::string("CREATE TABLE synchro_data ( \"") + kks_string +
                                      std::string(", \"timestamp\" DateTime64(3,'Europe/Moscow') ) "
                                                  " ENGINE = MergeTree() PARTITION BY toYYYYMMDD(timestamp)"
                                                  " ORDER BY (timestamp) PRIMARY KEY (timestamp)");
        printf("%s\n",sql.c_str());
        /* Execute SQL statement */
        exec(sql.c_str());
    }
}

void clickhouse_database::init_db(std::vector<std::string> kks_array)
{
    printf("init clickhouse tables\n");
    if (rewrite)
    {
        //init_synchro(kks_array);

        std::string  sql = std::string("DROP TABLE IF EXISTS static_data;");
        printf("%s\n",sql.c_str());
        exec(sql.c_str());

        sql = std::string("DROP TABLE IF EXISTS dynamic_data;");
        printf("%s\n",sql.c_str());
        /* Execute SQL statement */
        exec(sql.c_str());
    }

    std::string sql = std::string("CREATE TABLE IF NOT EXISTS static_data ( id UInt64, name text, description text ) "
                                  " ENGINE = MergeTree()"
                                  " ORDER BY (id) PRIMARY KEY (id)");
    printf("%s\n",sql.c_str());
    /* Execute SQL statement */
    exec(sql.c_str());

    /* Create SQL statement */

    /* Create SQL statement */
    sql = std::string("CREATE TABLE IF NOT EXISTS  dynamic_data ( id UInt64, t DateTime64(3,'Europe/Moscow'), "
                      "val Float64, status UInt64 ) ENGINE = MergeTree()"
                      " PARTITION BY (id,toYYYYMM(t)) ORDER BY (t) PRIMARY KEY (id,t)");
    printf("%s\n",sql.c_str());
    /* Execute SQL statement */
    exec(sql.c_str());



    int i = 0;
    if (!rewrite)
    {
        sql = "SELECT MAX(id) from static_data;";
        static int id;
        id = -1;
        ch_db->Select(sql.c_str(), [](const clickhouse::Block& block)
                {   if (block.GetRowCount() == 0) return;
                    else id = block[0]->As<clickhouse::ColumnUInt64>()->At(0);
                }
            );
        if (id > 0) i = id;
    }
    std::cout<<"max id = " << i <<"\n";
    bool need_merge = false;
    sql = std::string("INSERT INTO static_data (id,name) VALUES ");
    for (auto k : kks_array)
    {
        std::cout<<k<<" " << id(k)<<"\n";
        if (id(k) < 1)
        {
            sql += "(" + std::to_string(++i) + ", \'" + k + "\'),\n";
            need_merge = true;
        }
    }
    sql.pop_back();
    sql.pop_back();
    sql += ";";
    //std::cout<<kks_array.size()<<" elements inserted to static_data\n";
    if (need_merge)
    {
        printf("%s\n",sql.c_str());
        exec( sql.c_str());
    }

}

int clickhouse_database::exec(const char* sql)
{
    ch_db->Execute(sql);
    return 0;
}

int clickhouse_database::id(std::string kks)
{
    std::string sql = std::string("SELECT id FROM static_data WHERE name = \'") + kks + "\'";
    static int id;
    id = -1;
    ch_db->Select(sql.c_str(), [](const clickhouse::Block& block)
            {
                if (block.GetRowCount() == 0) return;
                else id = block[0]->As<clickhouse::ColumnUInt64>()->At(0);
            }
        );

    return id;
}

void clickhouse_database::finalize_db()
{
    exec("OPTIMIZE TABLE dynamic_data DEDUPLICATE");
}


