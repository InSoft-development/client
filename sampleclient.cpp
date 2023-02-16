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

SampleClient::SampleClient(int delta)
{
    m_pSession = new UaSession();
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
    UaStatus          result;
    ServiceSettings   serviceSettings;
    UaReadValueIds    nodeToRead;
    nodeToRead.create(1);
    UaDataValues      values;
    UaDiagnosticInfos diagnosticInfos;
    int ns = 1;
    std::fstream infile("kks.csv");
    int item_index = 0;
    std::string kks;
    while (infile >> kks)
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
        	if (OpcUa_IsGood(values[0].StatusCode))
        	{
            		printf("%s : %s\n", UaNodeId(nodeToRead[item_index].NodeId).toXmlString().toUtf8(), UaVariant(values[0].Value).toString().toUtf8());
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

UaStatus SampleClient::readHistory(const char* t1, const char* t2, int pause, int timeout, bool read_bounds)
{
	std::ofstream data ("data.csv");
	data<<"kks;value;timestamp;status\n";
    UaStatus                      status;
	ServiceSettings               serviceSettings;
	serviceSettings.callTimeout = timeout;
	HistoryReadRawModifiedContext historyReadRawModifiedContext;


	UaDiagnosticInfos             diagnosticInfos;
	int ns = 1;

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
    	if ( status.isBad() )
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
    			printf("** Results %d Node=%s status=%s\n", i, nodeToRead.toXmlString().toUtf8(), nodeResult.toString().toUtf8());

    			for ( j=0; j<results[i].m_dataValues.length(); j++ )
    			{
    				UaStatus statusOPLevel(results[i].m_dataValues[j].StatusCode);
    				std::string sourceTS = UaDateTime(results[i].m_dataValues[j].SourceTimestamp).toString().toUtf8();
    				sourceTS.pop_back();
    				sourceTS[10] = ' ';
    				if ( OpcUa_IsGood(results[i].m_dataValues[j].StatusCode) )
    				{
    					UaVariant tempValue = results[i].m_dataValues[j].Value;
    					data<<kks<<";"<<tempValue.toString().toUtf8()<<";"<<sourceTS.c_str()<<";"<<statusOPLevel.toString().toUtf8()<<"\n";
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
    					printf("** ContinuationPoint Results %d Node=%s status=%s\n", i, nodeToRead.toXmlString().toUtf8(), nodeResult.toString().toUtf8());

    					for ( j=0; j<results[i].m_dataValues.length(); j++ )
    					{
    						UaStatus statusOPLevel(results[i].m_dataValues[j].StatusCode);
    						std::string sourceTS = UaDateTime(results[i].m_dataValues[j].SourceTimestamp).toString().toUtf8();
							sourceTS.pop_back();
							sourceTS[10] = ' ';
    						if ( OpcUa_IsGood(results[i].m_dataValues[j].StatusCode) )
    						{
    							UaVariant tempValue = results[i].m_dataValues[j].Value;
    							data<<kks<<";"<<tempValue.toString().toUtf8()<<";"<<sourceTS.c_str()<<";"<<statusOPLevel.toString().toUtf8()<<"\n";
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
