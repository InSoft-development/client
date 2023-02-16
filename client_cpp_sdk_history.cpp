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
#include "client_cpp_sdk.h"
#include "clientconfig.h"
#include "shutdown.h"
#include "uaunistringlist.h"
#if OPCUA_SUPPORT_PKI
#include "uapkicertificate.h"
#include "uapkirevocationlist.h"
#endif // OPCUA_SUPPORT_PKI
#include "uasettings.h"
#include "libtrace.h"
#include "uaeventfilter.h"
#include "uaargument.h"
#include "demo_datatypes.h"
#include "uacertificatedirectoryobject.h"
#include "uatrustlistobject.h"
#include <iostream>
#include <uadir.h>
#include "uathread.h"
#if defined (HAVE_VISUAL_LEAK_DETECTOR)
#include <vld.h>
#endif

#define CLIENT_CPP_SDK_ACTIVATE_TRACE    1

#ifdef WIN32
    #define kbhit _kbhit
    #define getch _getch
#else
    #include "kbhit.c"
    #include <signal.h>
    #include <limits.h>
    #define getch readch
#endif

/*============================================================================
 * main
 *===========================================================================*/
#ifdef _WIN32_WCE
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPWSTR, int)
#else
int main(int, char*[])
#endif
{
#ifdef _MSC_VER
#if defined(WIN32) && defined(_MSC_VER) && _MSC_VER >= 1400 && !defined(_WIN32_WCE) && !defined(HAVE_VISUAL_LEAK_DETECTOR)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(590);
#endif // defined(WIN32) && defined(_MSC_VER) && _MSC_VER >= 1400 && !defined(_WIN32_WCE) && !defined(HAVE_VISUAL_LEAK_DETECTOR)
#endif
#ifndef WIN32
    init_keyboard();
#endif

    int ret = 0;

    // Extract application path
    char* szAppPath = getAppPath();

    // Initialize the UA Stack platform layer
    ret = UaPlatformLayer::init();

    if ( ret == 0 )
    {
        // Initialize global variables and read configuration

        UaString sIniFileName = UaString("%1/ClientConfig.ini").arg(szAppPath);

        g_pClientSampleConfig = new ClientSampleConfig;

        g_pClientSampleConfig->loadConfiguration(sIniFileName, szAppPath);

        g_pUaSession = NULL;
        g_pCallback  = NULL;
        g_nsIndex    = OpcUa_UInt16_Max;
        g_nsIndex2   = OpcUa_UInt16_Max;

        Demo::DataTypes::registerStructuredTypes();

#if CLIENT_CPP_SDK_ACTIVATE_TRACE
        /* Activate client trace*/
        LibT::initTrace( UaTrace::Data, 10000, 5, "uaclient.log", "sample client");
        LibT::setTraceActive(true);

        /* Set Trace Settings for stack*/
        LibT::setStackTraceActive(true);
        OpcUa_Trace_ChangeTraceLevel(OPCUA_TRACE_OUTPUT_LEVEL_DEBUG);
#endif /* CLIENT_CPP_SDK_ACTIVATE_TRACE */

        mainMenu();

#if CLIENT_CPP_SDK_ACTIVATE_TRACE
        /* Close Trace */
        LibT::closeTrace();
#endif /* CLIENT_CPP_SDK_ACTIVATE_TRACE */

        delete g_pClientSampleConfig;
        g_pClientSampleConfig = NULL;
    }

    // Cleanup the UA Stack platform layer
    UaPlatformLayer::cleanup();

    if ( szAppPath ) delete [] szAppPath;

#ifndef WIN32
    close_keyboard();
#endif
    return 0;
}

/*============================================================================
 * mainMenu - Main loop for user selecting OPC UA calls
 *===========================================================================*/
void mainMenu()
{
    int                 action;
    UaStatus            status;
    UaString            sDiscoveryUrl(g_pClientSampleConfig->sDiscoveryUrl());
    UaString            sUrl(g_pClientSampleConfig->sDefaultServerUrl());
    SessionSecurityInfo sessionSecurityInfo;

    g_pCallback = new Callback;

#if OPCUA_SUPPORT_PKI
    status = g_pClientSampleConfig->setupSecurity(sessionSecurityInfo);
#endif // OPCUA_SUPPORT_PKI

    // Setup UserToken - UserPassword
    if (g_pClientSampleConfig->userTokenType() == OpcUa_UserTokenType_UserName)
    {
        sessionSecurityInfo.setUserPasswordUserIdentity(g_pClientSampleConfig->sUsername(), g_pClientSampleConfig->sPassword());
    }
#if OPCUA_SUPPORT_PKI
    // Setup UserToken - Certificate
    else if (g_pClientSampleConfig->userTokenType() == OpcUa_UserTokenType_Certificate)
    {
        sessionSecurityInfo.setCertificateUserIdentity(g_pClientSampleConfig->userCertifiate(), g_pClientSampleConfig->userPrivateKey());
    }
#endif // OPCUA_SUPPORT_PKI
    // We treat anything else as Anonymous
    else
    {
        sessionSecurityInfo.setAnonymousUserIdentity();
    }

    printMenu(0);

    /******************************************************************************/
    /* Wait for user command to execute next action.                              */
    while (!WaitForKeypress(action))
    {
        if ( action == -1 )
        {
            UaThread::msleep(100);
            continue;
        }

        switch ( action )
        {
        case 0:
            startDiscovery(sDiscoveryUrl, sessionSecurityInfo, sUrl);
            break;
        case 1:
            connect(sUrl, sessionSecurityInfo);
            break;
        case 2:
            disconnect();
            break;
        case 3:
            browse();
            break;
        case 4:
            read();
            break;
        case 5:
            write();
            break;
        case 6:
            registerNodes();
            break;
        case 7:
            unregisterNodes();
            break;
        case 8:
            subscribe();
            break;
        case 9:
            subscribeAlarms();
            break;
        case 10:
            callMethod();
            break;
        case 11:
            translate();
            break;
        case 12:
            transferSubscription(sUrl, sessionSecurityInfo);
            break;
        case 13:
            historyReadDataRaw();
            break;
        case 14:
            historyReadDataProcessed();
            break;
        case 15:
            historyReadDataAtTime();
            break;
        case 16:
            historyUpdateData();
            break;
        case 17:
            historyDeleteData();
            break;
        case 18:
            historyReadEvents();
            break;
        case 19:
            historyUpdateEvents();
            break;
        case 20:
            historyDeleteEvents();
            break;
        case 21:
            readAsync();
            break;
        case 22:
            writeAsync();
            break;
        case 23:
            gdsInteraction(sessionSecurityInfo);
            break;
        case 24:
            subscribeDurable(sUrl, sessionSecurityInfo);
            break;
        default:
            continue;
        }

        printMenu(0);
    }
    /******************************************************************************/

    if ( g_pUaSession )
    {
        disconnect();
    }

    delete g_pCallback;
}

/*============================================================================
 * connect - Connect to OPC UA Server
 *===========================================================================*/
void connect(UaString& sUrl, SessionSecurityInfo& sessionSecurityInfo)
{
    UaStatus status;

    printf("\n\n****************************************************************\n");
    printf("** Try to connect to selected server\n");
    if ( g_pUaSession )
    {
        disconnect();
    }

    g_pUaSession = new UaSession();

    SessionConnectInfo sessionConnectInfo;
    sessionConnectInfo.sApplicationName = g_pClientSampleConfig->sApplicationName();
    sessionConnectInfo.sApplicationUri  = g_pClientSampleConfig->sApplicationUri();
    sessionConnectInfo.sProductUri      = g_pClientSampleConfig->sProductUri();
    sessionConnectInfo.sSessionName     = UaString("Client_Cpp_SDK@%1").arg(g_pClientSampleConfig->sHostName());
    sessionConnectInfo.bAutomaticReconnect = OpcUa_True;

    const UaUserIdentityToken* pUserToken = sessionSecurityInfo.pUserIdentityToken();
    switch (pUserToken->getTokenType())
    {
    case OpcUa_UserTokenType_Anonymous:
        printf("Connecting with anonymous user token\n");
        break;
    case OpcUa_UserTokenType_UserName:
        printf("Connecting with user name %s\n", ((UaUserIdentityTokenUserPassword*)pUserToken)->sUserName.toUtf8());
        break;
    case OpcUa_UserTokenType_Certificate:
        printf("Connecting with certificate user token\n");
        break;
    case OpcUa_UserTokenType_IssuedToken:
        printf("Connecting with issued user token\n");
        break;
    default:
        printf("Connecting with invalid user token\n");
        break;
    }

    /*********************************************************************
     Connect to OPC UA Server
    **********************************************************************/
    status = g_pUaSession->connect(
        sUrl,                // URL of the Endpoint - from discovery or config
        sessionConnectInfo,  // General settings for connection
        sessionSecurityInfo, // Security settings
        g_pCallback);        // Callback interface
    /*********************************************************************/
    if ( status.isBad() )
    {
        delete g_pUaSession;
        g_pUaSession = NULL;
        printf("** Error: UaSession::connect failed [ret=%s]\n", status.toString().toUtf8());
        printf("****************************************************************\n");

        if ( sessionSecurityInfo.messageSecurityMode != OpcUa_MessageSecurityMode_None )
        {
            printf("\n");
            printf("------------------------------------------------------------\n");
            printf("- Make sure the client certificate is in server trust list -\n");
            printf("- Check rejected directory of server PKI store             -\n");
            printf("------------------------------------------------------------\n");
        }
        return;
    }
    else
    {
        printf("** Connected to Server %s with security mode: %s ****\n", sUrl.toUtf8(), sessionSecurityInfo.sSecurityPolicy.toUtf8());
        printf("****************************************************************\n");

        // Load node ids
        buildNodeIds();
    }
}

/*============================================================================
 * disconnect - Disconnect from OPC UA Server
 *===========================================================================*/
void disconnect()
{
    printf("\n\n****************************************************************\n");
    printf("** Disconnect from Server\n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    ServiceSettings serviceSettings;

    /*********************************************************************
     Disconnect from OPC UA Server
    **********************************************************************/
    g_pUaSession->disconnect(
        serviceSettings, // Use default settings
        OpcUa_True);       // Delete subscriptions
    /*********************************************************************/
    printf("****************************************************************\n");

    delete g_pUaSession;
    g_pUaSession = NULL;

    g_VariableNodeIds.clear();
    g_WriteVariableNodeIds.clear();
    g_ObjectNodeIds.clear();
    g_MethodNodeIds.clear();
    g_EventIds.clear();
}

/*============================================================================
 * exploreAddressSpace - explore the OPC UA Server addressspace
 *===========================================================================*/
void exploreAddressSpace(const UaNodeId& startingNode, unsigned int level)
{
    UaStatus                status;
    UaByteString            continuationPoint;
    UaReferenceDescriptions referenceDescriptions;
    ServiceSettings         serviceSettings;
    BrowseContext           browseContext;

    /*********************************************************************
     Browse Server
    **********************************************************************/
    status = g_pUaSession->browse(
        serviceSettings,
        startingNode,
        browseContext,
        continuationPoint,
        referenceDescriptions);
    /*********************************************************************/

    if ( status.isBad() )
    {
        printf("** Error: UaSession::browse of NodeId = %s failed [ret=%s]\n", startingNode.toFullString().toUtf8(), status.toString().toUtf8());
        return;
    }
    else
    {
        OpcUa_UInt32 i, j;
        for (i=0; i<referenceDescriptions.length(); i++)
        {
            printf("node: ");
            for (j=0; j<level; j++) printf("  ");
            UaNodeId referenceTypeId(referenceDescriptions[i].ReferenceTypeId);
            printf("[Ref=%s] ", referenceTypeId.toString().toUtf8() );
            UaQualifiedName browseName(referenceDescriptions[i].BrowseName);
            printf("%s ( ", browseName.toString().toUtf8() );
            if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Object) printf("Object ");
            if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Variable) printf("Variable ");
            if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Method) printf("Method ");
            if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_ObjectType) printf("ObjectType ");
            if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_VariableType) printf("VariableType ");
            if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_ReferenceType) printf("ReferenceType ");
            if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_DataType) printf("DataType ");
            if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_View) printf("View ");
            UaNodeId nodeId(referenceDescriptions[i].NodeId.NodeId);
            printf("[NodeId=%s] ", nodeId.toFullString().toUtf8() );
            printf(")\n");

            exploreAddressSpace(referenceDescriptions[i].NodeId.NodeId, level+1 );
        }

        // Check if the continuation point was set -> call browseNext
        while ( continuationPoint.length() > 0 )
        {
            /*********************************************************************
             Browse remaining nodes in the Server
            **********************************************************************/
            status = g_pUaSession->browseNext(
                serviceSettings,
                OpcUa_False,
                continuationPoint,
                referenceDescriptions);
            /*********************************************************************/

            if ( status.isBad() )
            {
                printf("** Error: UaSession::browse of NodeId = %s failed [ret=%s] **\n", startingNode.toFullString().toUtf8(), status.toString().toUtf8());
                return;
            }
            else
            {
                for (i=0; i<referenceDescriptions.length(); i++)
                {
                    printf("node: ");
                    for (j=0; j<level; j++) printf("  ");
                    UaNodeId referenceTypeId(referenceDescriptions[i].ReferenceTypeId);
                    printf("[Ref=%s] ", referenceTypeId.toString().toUtf8() );
                    UaQualifiedName browseName(referenceDescriptions[i].BrowseName);
                    printf("%s ( ", browseName.toString().toUtf8() );
                    if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Object) printf("Object ");
                    if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Variable) printf("Variable ");
                    if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Method) printf("Method ");
                    if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_ObjectType) printf("ObjectType ");
                    if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_VariableType) printf("VariableType ");
                    if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_ReferenceType) printf("ReferenceType ");
                    if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_DataType) printf("DataType ");
                    if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_View) printf("View ");
                    UaNodeId nodeId(referenceDescriptions[i].NodeId.NodeId);
                    printf("[NodeId=%s] ", nodeId.toFullString().toUtf8() );
                    printf(")\n");

                    exploreAddressSpace(referenceDescriptions[i].NodeId.NodeId, level+1 );
                }
            }
        }
    }
    return;
}

/*============================================================================
 * browse - Browse OPC UA Server
 *===========================================================================*/
void browse()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to call browse on root\n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    // Root as starting node for recursive browsing
    UaNodeId startingNode(OpcUaId_RootFolder);

    // Start recursive browsing
    exploreAddressSpace(startingNode, 1);
}

/*============================================================================
 * read - Read attribute values
 *===========================================================================*/
void read()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to call read for configured node ids\n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    OpcUa_UInt32      i;
    OpcUa_UInt32      count;
    UaStatus          status;
    UaReadValueIds    nodesToRead;
    UaDataValues      values;
    UaDiagnosticInfos diagnosticInfos;
    ServiceSettings   serviceSettings;

    // Initialize IN parameter nodesToRead
    count = g_VariableNodeIds.length();
    nodesToRead.create(count);
    for ( i=0; i<count; i++ )
    {
        g_VariableNodeIds[i].copyTo(&nodesToRead[i].NodeId);
        nodesToRead[i].AttributeId = OpcUa_Attributes_Value;
    }

    /*********************************************************************
     Call read service
    **********************************************************************/
    status = g_pUaSession->read(
        serviceSettings,                // Use default settings
        0,                              // Max age
        OpcUa_TimestampsToReturn_Both,  // Time stamps to return
        nodesToRead,                    // Array of nodes to read
        values,                         // Returns an array of values
        diagnosticInfos);               // Returns an array of diagnostic info
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::read failed [ret=%s] **\n", status.toString().toUtf8());
    }
    else
    {
        printf("** UaSession::read result **************************************\n");
        for ( i=0; i<count; i++ )
        {
            UaNodeId node(nodesToRead[i].NodeId);
            if ( OpcUa_IsGood(values[i].StatusCode) )
            {
                UaVariant tempValue = values[i].Value;
                if (tempValue.type() == OpcUaType_ExtensionObject)
                {
                    printExtensionObjects(tempValue, UaString("Variable %1").arg(node.toString()));
                }
                else
                {
                    printf("  Variable %s value = %s\n", node.toString().toUtf8(), tempValue.toString().toUtf8());
                }
            }
            else
            {
                printf("  Variable %s failed with error %s\n", node.toString().toUtf8(), UaStatus(values[i].StatusCode).toString().toUtf8());
            }
        }
        printf("****************************************************************\n");
    }
}

/*============================================================================
 * readAsnyc - Read attribute values asynchronous
 *===========================================================================*/
void readAsync()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to call read asynchronous for configured node ids\n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    OpcUa_UInt32      i;
    OpcUa_UInt32      count;
    UaStatus          status;
    UaReadValueIds    nodesToRead;
    UaDataValues      values;
    UaDiagnosticInfos diagnosticInfos;
    ServiceSettings   serviceSettings;

    // Initialize IN parameter nodesToRead
    count = g_VariableNodeIds.length();
    nodesToRead.create(count);
    for ( i=0; i<count; i++ )
    {
        g_VariableNodeIds[i].copyTo(&nodesToRead[i].NodeId);
        nodesToRead[i].AttributeId = OpcUa_Attributes_Value;
    }

    /*********************************************************************
     Call read service
    **********************************************************************/
    status = g_pUaSession->beginRead(
        serviceSettings,                // Use default settings
        0,                              // Max age
        OpcUa_TimestampsToReturn_Both,  // Time stapmps to return
        nodesToRead,                    // Array of nodes to read
        1);                             // Application definded transaction ID
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::beginRead failed [ret=%s] **\n", status.toString().toUtf8());
    }
    else
    {
        printf("** UaSession::beginRead succeded ********************************\n");
    }
}

/*============================================================================
 * write - Write attribute values
 *===========================================================================*/
void write()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to call write for configured node ids\n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    OpcUa_UInt32      i;
    OpcUa_UInt32      count;
    UaStatus          status;
    UaVariant         tempValue;
    UaWriteValues     nodesToWrite;
    UaStatusCodeArray results;
    UaDiagnosticInfos diagnosticInfos;
    ServiceSettings   serviceSettings;

    // Initialize IN parameter nodesToWrite
    count = g_WriteVariableNodeIds.length();
    nodesToWrite.create(count);
    for ( i=0; i<count; i++ )
    {
        g_WriteVariableNodeIds[i].copyTo(&nodesToWrite[i].NodeId);
        nodesToWrite[i].AttributeId = OpcUa_Attributes_Value;
        tempValue.setDouble(71);
        tempValue.copyTo(&nodesToWrite[i].Value.Value);
    }

    /*********************************************************************
     Call write service
    **********************************************************************/
    status = g_pUaSession->write(
        serviceSettings,                // Use default settings
        nodesToWrite,                   // Array of nodes to write
        results,                        // Returns an array of status codes
        diagnosticInfos);               // Returns an array of diagnostic info
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::write failed [ret=%s] **\n", status.toString().toUtf8());
    }
    else
    {
        printf("** UaSession::write result **************************************\n");
        for ( i=0; i<count; i++ )
        {
            UaNodeId node(nodesToWrite[i].NodeId);
            if ( OpcUa_IsGood(results[i]) )
            {
                printf("** Variable %s succeeded!\n", node.toString().toUtf8());
            }
            else
            {
                printf("** Variable %s failed with error %s\n", node.toString().toUtf8(), UaStatus(results[i]).toString().toUtf8());
            }
        }
        printf("****************************************************************\n");
    }
}

/*============================================================================
 * writeAsync - Write attribute values asynchronous
 *===========================================================================*/
void writeAsync()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to call write asynchronous for configured node ids\n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    OpcUa_UInt32      i;
    OpcUa_UInt32      count;
    UaStatus          status;
    UaVariant         tempValue;
    UaWriteValues     nodesToWrite;
    UaStatusCodeArray results;
    UaDiagnosticInfos diagnosticInfos;
    ServiceSettings   serviceSettings;

    // Initialize IN parameter nodesToWrite
    count = g_WriteVariableNodeIds.length();
    nodesToWrite.create(count);
    for ( i=0; i<count; i++ )
    {
        g_WriteVariableNodeIds[i].copyTo(&nodesToWrite[i].NodeId);
        nodesToWrite[i].AttributeId = OpcUa_Attributes_Value;
        tempValue.setDouble(71);
        tempValue.copyTo(&nodesToWrite[i].Value.Value);
    }

    /*********************************************************************
     Call write service
    **********************************************************************/
    status = g_pUaSession->beginWrite(
        serviceSettings,                // Use default settings
        nodesToWrite,                   // Array of nodes to write
        1);                             // Application definded transaction ID
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::beginWrite failed [ret=%s] **\n", status.toString().toUtf8());
    }
    else
    {
        printf("** UaSession::beginWrite succeded ********************************\n");
    }
}

/*============================================================================
 * registerNodes - RegisterNodes for frequently use in read or write
 *===========================================================================*/
void registerNodes()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to call registerNodes for configured node ids\n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    OpcUa_UInt32      i;
    OpcUa_UInt32      count;
    UaStatus          status;
    UaNodeIdArray     nodesToRegister;
    UaNodeIdArray     registeredNodes;
    ServiceSettings   serviceSettings;

    // Initialize IN parameter nodesToRegister
    count = g_VariableNodeIds.length();
    nodesToRegister.create(count);
    for ( i=0; i<count; i++ )
    {
        g_VariableNodeIds[i].copyTo(&nodesToRegister[i]);
    }

    /*********************************************************************
     Call registerNodes service
    **********************************************************************/
    status = g_pUaSession->registerNodes(
        serviceSettings,     // Use default settings
        nodesToRegister,     // Array of nodeIds to register
        registeredNodes);    // Returns an array of registered nodeIds
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::registerNodes failed [ret=%s] **\n", status.toString().toUtf8());
    }
    else
    {
        printf("** UaSession::registerNodes succeeded\n");
        for ( i=0; i<count; i++ )
        {
            // Copy back returned NodeIds
            g_VariableNodeIds[i] = registeredNodes[i];
        }
        printf("****************************************************************\n");
    }
}

/*============================================================================
 * unregisterNodes - RegisterNodes for frequently use in read or write
 *===========================================================================*/
void unregisterNodes()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to call unregisterNodes for removing node ids\n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    OpcUa_UInt32      i;
    OpcUa_UInt32      count;
    UaStatus          status;
    UaNodeIdArray     nodesToUnregister;
    ServiceSettings   serviceSettings;

    // Initialize IN parameter nodesToRegister
    count = g_VariableNodeIds.length();
    nodesToUnregister.create(count);
    for ( i=0; i<count; i++ )
    {
        g_VariableNodeIds[i].copyTo(&nodesToUnregister[i]);
    }

    /*********************************************************************
     Call registerNodes service
    **********************************************************************/
    status = g_pUaSession->unregisterNodes(
        serviceSettings,     // Use default settings
        nodesToUnregister);  // Array of nodeIds to unregister
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::unregisterNodes failed [ret=%s] **\n", status.toString().toUtf8());
    }
    else
    {
        printf("** UaSession::unregisterNodes succeeded\n");
    }

    buildNodeIds();

    printf("****************************************************************\n");
}

/*============================================================================
 * subscribe - Subscribe for data changes
 *===========================================================================*/
void subscribe()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to create a subscription\n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    UaStatus             status;
    UaSubscription*      pUaSubscription       = NULL;
    SubscriptionSettings subscriptionSettings;
    subscriptionSettings.publishingInterval    = 500;
    ServiceSettings      serviceSettings;

    /*********************************************************************
     Create a Subscription
    **********************************************************************/
    status = g_pUaSession->createSubscription(
        serviceSettings,        // Use default settings
        g_pCallback,            // Callback object
        0,                      // We have only one subscription, handle is not needed
        subscriptionSettings,   // general settings
        OpcUa_True,             // Publishing enabled
        &pUaSubscription);      // Returned Subscription instance
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::createSubscription failed [ret=%s] **\n", status.toString().toUtf8());
    }
    else
    {
        printf("****************************************************************\n");
        printf("** Try to create monitored items\n");

        OpcUa_UInt32                  i;
        OpcUa_UInt32                  count;
        UaMonitoredItemCreateRequests monitoredItemCreateRequests;
        UaMonitoredItemCreateResults  monitoredItemCreateResults;

        // Initialize IN parameter monitoredItemCreateRequests
        count = g_VariableNodeIds.length();
        monitoredItemCreateRequests.create(count + 1); // We create also an event monitored item
        for ( i=0; i<count; i++ )
        {
            g_VariableNodeIds[i].copyTo(&monitoredItemCreateRequests[i].ItemToMonitor.NodeId);
            monitoredItemCreateRequests[i].ItemToMonitor.AttributeId = OpcUa_Attributes_Value;
            monitoredItemCreateRequests[i].MonitoringMode = OpcUa_MonitoringMode_Reporting;
            monitoredItemCreateRequests[i].RequestedParameters.ClientHandle = i+1;
            monitoredItemCreateRequests[i].RequestedParameters.SamplingInterval = 1000;
            monitoredItemCreateRequests[i].RequestedParameters.QueueSize = 1;
            monitoredItemCreateRequests[i].RequestedParameters.DiscardOldest = OpcUa_True;

            // Sample code for creation of data change filter
            /*
            OpcUa_DataChangeFilter* pDataChangeFilter = NULL;
            OpcUa_EncodeableObject_CreateExtension(
                &OpcUa_DataChangeFilter_EncodeableType,
                &monitoredItemCreateRequests[i].RequestedParameters.Filter,
                (OpcUa_Void**)&pDataChangeFilter);
            if ( pDataChangeFilter )
            {
                // Deadband setting
                pDataChangeFilter->DeadbandType = OpcUa_DeadbandType_Absolute;
                pDataChangeFilter->DeadbandValue = 0.1; // 0.1% of last value
                // Trigger setting (default is StatusValue)
                pDataChangeFilter->Trigger      = OpcUa_DataChangeTrigger_StatusValue;
            }
            */
        }

        // ------------------------------------------------------
        // ------------------------------------------------------
        // Set Event item
        monitoredItemCreateRequests[count].ItemToMonitor.NodeId.Identifier.Numeric = OpcUaId_Server;
        monitoredItemCreateRequests[count].ItemToMonitor.AttributeId = OpcUa_Attributes_EventNotifier;
        monitoredItemCreateRequests[count].MonitoringMode = OpcUa_MonitoringMode_Reporting;
        monitoredItemCreateRequests[count].RequestedParameters.ClientHandle = count+1;
        monitoredItemCreateRequests[count].RequestedParameters.SamplingInterval = 0; // 0 is required by OPC UA spec
        monitoredItemCreateRequests[count].RequestedParameters.QueueSize = 0;

        UaEventFilter            eventFilter;
        UaSimpleAttributeOperand selectElement;
        UaContentFilter*         pContentFilter        = NULL;
        UaContentFilterElement*  pContentFilterElement = NULL;
        UaFilterOperand*         pOperand              = NULL;

        // -------------------------------------------------------------------------
        // Define select clause with 5 event fields to be returned with every event
        // -------------------------------------------------------------------------
        selectElement.setBrowsePathElement(0, UaQualifiedName("Message", 0), 1);
        eventFilter.setSelectClauseElement(0, selectElement, 5);
        selectElement.setBrowsePathElement(0, UaQualifiedName("SourceName", 0), 1);
        eventFilter.setSelectClauseElement(1, selectElement, 5);
        selectElement.setBrowsePathElement(0, UaQualifiedName("EventType", 0), 1);
        eventFilter.setSelectClauseElement(2, selectElement, 5);
        selectElement.setBrowsePathElement(0, UaQualifiedName("Temperature", 3), 1);
        eventFilter.setSelectClauseElement(3, selectElement, 5);
        selectElement.setBrowsePathElement(0, UaQualifiedName("ConditionName", 0), 1);
        eventFilter.setSelectClauseElement(4, selectElement, 5);
        // -------------------------------------------------------------------------
        // Set the event field names requested in addition to Message
        g_pCallback->m_eventFields.clear();
        g_pCallback->m_eventFields.create(4);
        UaString sEventField;
        sEventField = "SourceName";
        sEventField.copyTo(&g_pCallback->m_eventFields[0]);
        sEventField = "EventType";
        sEventField.copyTo(&g_pCallback->m_eventFields[1]);
        sEventField = "Temperature";
        sEventField.copyTo(&g_pCallback->m_eventFields[2]);
        sEventField = "ConditionName";
        sEventField.copyTo(&g_pCallback->m_eventFields[3]);
        // -------------------------------------------------------------------------

        // -------------------------------------------------------------------------
        // Define the where clause to filter the events sent to the client
        // Filter is ( (Severity > 100) AND (OfType(ControllerEventType) OR OfType(OffNormalAlarmType)) )
        // Represented as three ContentFilterElements
        // [0] [1] AND [2]
        // [1] Severity > 100
        // [2] [3] OR [4]
        // [3] OfType(ControllerEventType)
        // [4] OfType(OffNormalAlarmType)
        // -------------------------------------------------------------------------
        pContentFilter = new UaContentFilter;
        // [0] [1] AND [2] ------------------------------------------
        pContentFilterElement = new UaContentFilterElement;
        // Operator And
        pContentFilterElement->setFilterOperator(OpcUa_FilterOperator_And);
        // Operand 1 (Element)
        pOperand = new UaElementOperand;
        ((UaElementOperand*)pOperand)->setIndex(1);
        pContentFilterElement->setFilterOperand(0, pOperand, 2);
        // Operand 2 (Element)
        pOperand = new UaElementOperand;
        ((UaElementOperand*)pOperand)->setIndex(2);
        pContentFilterElement->setFilterOperand(1, pOperand, 2);
        pContentFilter->setContentFilterElement(0, pContentFilterElement, 5);
        // [1] Severity > 100  --------------------------------------
        pContentFilterElement = new UaContentFilterElement;
        // Operator GreaterThan
        pContentFilterElement->setFilterOperator(OpcUa_FilterOperator_GreaterThan);
        // Operand 1 (SimpleAttribute)
        pOperand = new UaSimpleAttributeOperand;
        ((UaSimpleAttributeOperand*)pOperand)->setBrowsePathElement(0, UaQualifiedName("Severity", 0), 1);;
        pContentFilterElement->setFilterOperand(0, pOperand, 2);
        // Operand 2 (Literal)
        pOperand = new UaLiteralOperand;
        ((UaLiteralOperand*)pOperand)->setLiteralValue(UaVariant((OpcUa_UInt16)100));
        pContentFilterElement->setFilterOperand(1, pOperand, 2);
        pContentFilter->setContentFilterElement(1, pContentFilterElement, 5);
        // [2] [3] OR [4] ------------------------------------------
        pContentFilterElement = new UaContentFilterElement;
        // Operator And
        pContentFilterElement->setFilterOperator(OpcUa_FilterOperator_Or);
        // Operand 1 (Element)
        pOperand = new UaElementOperand;
        ((UaElementOperand*)pOperand)->setIndex(3);
        pContentFilterElement->setFilterOperand(0, pOperand, 2);
        // Operand 2 (Element)
        pOperand = new UaElementOperand;
        ((UaElementOperand*)pOperand)->setIndex(4);
        pContentFilterElement->setFilterOperand(1, pOperand, 2);
        pContentFilter->setContentFilterElement(2, pContentFilterElement, 5);
        // [3] OfTpype(ControllerEventType) --------------------------
        pContentFilterElement = new UaContentFilterElement;
        // Operator OfType
        pContentFilterElement->setFilterOperator(OpcUa_FilterOperator_OfType);
        // Operand 1 (Literal)
        pOperand = new UaLiteralOperand;
        ((UaLiteralOperand*)pOperand)->setLiteralValue(UaVariant(UaNodeId(4000, 3))); // ControllerEventType
        pContentFilterElement->setFilterOperand(0, pOperand, 1);
        pContentFilter->setContentFilterElement(3, pContentFilterElement, 5);
        // [4] OfTpype(OffNormalAlarmType) --------------------------
        pContentFilterElement = new UaContentFilterElement;
        // Operator OfType
        pContentFilterElement->setFilterOperator(OpcUa_FilterOperator_OfType);
        // Operand 1 (Literal)
        pOperand = new UaLiteralOperand;
        ((UaLiteralOperand*)pOperand)->setLiteralValue(UaVariant(UaNodeId(OpcUaId_OffNormalAlarmType)));
        pContentFilterElement->setFilterOperand(0, pOperand, 1);
        pContentFilter->setContentFilterElement(4, pContentFilterElement, 5);
        //
        eventFilter.setWhereClause(pContentFilter);
        // -------------------------------------------------------------------------

        // Detach EventFilter to the create monitored item request
        eventFilter.detachFilter(monitoredItemCreateRequests[count].RequestedParameters.Filter);
        // ------------------------------------------------------
        // ------------------------------------------------------

        /*********************************************************************
         Call createMonitoredItems service
        **********************************************************************/
        status = pUaSubscription->createMonitoredItems(
            serviceSettings,               // Use default settings
            OpcUa_TimestampsToReturn_Both, // Select timestamps to return
            monitoredItemCreateRequests,   // monitored items to create
            monitoredItemCreateResults);   // Returned monitored items create result
        /*********************************************************************/
        if ( status.isBad() )
        {
            printf("** Error: UaSession::createMonitoredItems failed [ret=%s]\n", status.toString().toUtf8());
            printf("****************************************************************\n");
            return;
        }
        else
        {
            printf("** UaSession::createMonitoredItems result **********************\n");
            for ( i=0; i<count; i++ )
            {
                UaNodeId node(monitoredItemCreateRequests[i].ItemToMonitor.NodeId);
                if ( OpcUa_IsGood(monitoredItemCreateResults[i].StatusCode) )
                {
                    printf("** Variable %s MonitoredItemId = %d\n", node.toString().toUtf8(), monitoredItemCreateResults[i].MonitoredItemId);
                }
                else
                {
                    printf("** Variable %s failed with error %s\n", node.toString().toUtf8(), UaStatus(monitoredItemCreateResults[i].StatusCode).toString().toUtf8());
                }
            }
            if ( OpcUa_IsGood(monitoredItemCreateResults[count].StatusCode) )
            {
                printf("** Event MonitoredItemId = %d\n", monitoredItemCreateResults[count].MonitoredItemId);
            }
            else
            {
                printf("** Event MonitoredItem for Server Object failed!\n");
            }
            printf("****************************************************************\n");
        }
    }

    printf("\n*******************************************************\n");
    printf("*******************************************************\n");
    printf("**        Press x to stop subscription          *******\n");
    printf("*******************************************************\n");
    printf("*******************************************************\n");

    int action;
    /******************************************************************************/
    /* Wait for user command to stop the subscription. */
    while (!WaitForKeypress(action))
    {
        UaThread::msleep(100);
    }
    /******************************************************************************/

    /*********************************************************************
     Delete Subscription
    **********************************************************************/
    status = g_pUaSession->deleteSubscription(
        serviceSettings,    // Use default settings
        &pUaSubscription);  // Subcription
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** UaSession::deleteSubscription failed! **********************\n");
        printf("****************************************************************\n");
    }
    else
    {
        pUaSubscription = NULL;
        printf("** UaSession::deleteSubscription succeeded!\n");
        printf("****************************************************************\n");
    }
}

/*============================================================================
* subscribe - Subscribe durable for data changes
*===========================================================================*/
void subscribeDurable(UaString& sUrl, SessionSecurityInfo& sessionSecurityInfo)
{
    printf("\n\n****************************************************************\n");
    printf("** Try to create a subscription\n");
    if (g_pUaSession == NULL)
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    UaStatus             status;
    UaSubscription*      pUaSubscription = NULL;
    SubscriptionSettings subscriptionSettings;
    subscriptionSettings.publishingInterval = 500;
    subscriptionSettings.lifetimeCount = 20;
    ServiceSettings      serviceSettings;

    /*********************************************************************
    Create a Subscription
    **********************************************************************/
    status = g_pUaSession->createSubscription(
        serviceSettings,        // Use default settings
        g_pCallback,            // Callback object
        0,                      // We have only one subscription, handle is not needed
        subscriptionSettings,   // general settings
        OpcUa_True,             // Publishing enabled
        &pUaSubscription);      // Returned Subscription instance
                                /*********************************************************************/
    if (status.isBad())
    {
        printf("** Error: UaSession::createSubscription failed [ret=%s] **\n", status.toString().toUtf8());
    }
    else
    {
        CallIn callRequest;
        CallOut results;
        callRequest.objectId.setNodeId(OpcUaId_Server, 0);
        callRequest.methodId.setNodeId(OpcUaId_Server_SetSubscriptionDurable, 0);

        UaVariant uvInput;
        callRequest.inputArguments.create(2);
        // SubscriptionId
        uvInput.setUInt32(pUaSubscription->subscriptionId());
        uvInput.copyTo(&callRequest.inputArguments[0]);
        // LifeTimeInHours
        uvInput.setUInt32(20);
        uvInput.copyTo(&callRequest.inputArguments[1]);

        status = g_pUaSession->call(
            serviceSettings,
            callRequest,
            results);
        if (status.isGood())
        {
            status = results.callResult.statusCode();
        }
        if (status.isBad())
        {
            printf("** Error: SetDurableSubscription failed [ret=%s] **\n", status.toString().toUtf8());
            return;
        }

        printf("****************************************************************\n");
        printf("** Try to create monitored items\n");

        OpcUa_UInt32                  i;
        OpcUa_UInt32                  count;
        UaMonitoredItemCreateRequests monitoredItemCreateRequests;
        UaMonitoredItemCreateResults  monitoredItemCreateResults;

        // Initialize IN parameter monitoredItemCreateRequests
        count = g_VariableNodeIds.length();
        monitoredItemCreateRequests.create(count + 1); // We create also an event monitored item
        for (i = 0; i<count; i++)
        {
            g_VariableNodeIds[i].copyTo(&monitoredItemCreateRequests[i].ItemToMonitor.NodeId);
            monitoredItemCreateRequests[i].ItemToMonitor.AttributeId = OpcUa_Attributes_Value;
            monitoredItemCreateRequests[i].MonitoringMode = OpcUa_MonitoringMode_Reporting;
            monitoredItemCreateRequests[i].RequestedParameters.ClientHandle = i + 1;
            monitoredItemCreateRequests[i].RequestedParameters.SamplingInterval = 50;
            monitoredItemCreateRequests[i].RequestedParameters.QueueSize = 1000000;
            monitoredItemCreateRequests[i].RequestedParameters.DiscardOldest = OpcUa_True;

            // Sample code for creation of data change filter
            /*
            OpcUa_DataChangeFilter* pDataChangeFilter = NULL;
            OpcUa_EncodeableObject_CreateExtension(
            &OpcUa_DataChangeFilter_EncodeableType,
            &monitoredItemCreateRequests[i].RequestedParameters.Filter,
            (OpcUa_Void**)&pDataChangeFilter);
            if ( pDataChangeFilter )
            {
            // Deadband setting
            pDataChangeFilter->DeadbandType = OpcUa_DeadbandType_Absolute;
            pDataChangeFilter->DeadbandValue = 0.1; // 0.1% of last value
            // Trigger setting (default is StatusValue)
            pDataChangeFilter->Trigger      = OpcUa_DataChangeTrigger_StatusValue;
            }
            */
        }

        // ------------------------------------------------------
        // ------------------------------------------------------
        // Set Event item
        monitoredItemCreateRequests[count].ItemToMonitor.NodeId.Identifier.Numeric = OpcUaId_Server;
        monitoredItemCreateRequests[count].ItemToMonitor.AttributeId = OpcUa_Attributes_EventNotifier;
        monitoredItemCreateRequests[count].MonitoringMode = OpcUa_MonitoringMode_Reporting;
        monitoredItemCreateRequests[count].RequestedParameters.ClientHandle = count + 1;
        monitoredItemCreateRequests[count].RequestedParameters.SamplingInterval = 0; // 0 is required by OPC UA spec
        monitoredItemCreateRequests[count].RequestedParameters.QueueSize = 0;

        UaEventFilter            eventFilter;
        UaSimpleAttributeOperand selectElement;
        UaContentFilter*         pContentFilter = NULL;
        UaContentFilterElement*  pContentFilterElement = NULL;
        UaFilterOperand*         pOperand = NULL;

        // -------------------------------------------------------------------------
        // Define select clause with 5 event fields to be returned with every event
        // -------------------------------------------------------------------------
        selectElement.setBrowsePathElement(0, UaQualifiedName("Message", 0), 1);
        eventFilter.setSelectClauseElement(0, selectElement, 5);
        selectElement.setBrowsePathElement(0, UaQualifiedName("SourceName", 0), 1);
        eventFilter.setSelectClauseElement(1, selectElement, 5);
        selectElement.setBrowsePathElement(0, UaQualifiedName("EventType", 0), 1);
        eventFilter.setSelectClauseElement(2, selectElement, 5);
        selectElement.setBrowsePathElement(0, UaQualifiedName("Temperature", 3), 1);
        eventFilter.setSelectClauseElement(3, selectElement, 5);
        selectElement.setBrowsePathElement(0, UaQualifiedName("ConditionName", 0), 1);
        eventFilter.setSelectClauseElement(4, selectElement, 5);
        // -------------------------------------------------------------------------
        // Set the event field names requested in addition to Message
        g_pCallback->m_eventFields.clear();
        g_pCallback->m_eventFields.create(4);
        UaString sEventField;
        sEventField = "SourceName";
        sEventField.copyTo(&g_pCallback->m_eventFields[0]);
        sEventField = "EventType";
        sEventField.copyTo(&g_pCallback->m_eventFields[1]);
        sEventField = "Temperature";
        sEventField.copyTo(&g_pCallback->m_eventFields[2]);
        sEventField = "ConditionName";
        sEventField.copyTo(&g_pCallback->m_eventFields[3]);
        // -------------------------------------------------------------------------

        // -------------------------------------------------------------------------
        // Define the where clause to filter the events sent to the client
        // Filter is ( (Severity > 100) AND (OfType(ControllerEventType) OR OfType(OffNormalAlarmType)) )
        // Represented as three ContentFilterElements
        // [0] [1] AND [2]
        // [1] Severity > 100
        // [2] [3] AND [4]
        // [3] OfType(ControllerEventType)
        // [4] OfType(OffNormalAlarmType)
        // -------------------------------------------------------------------------
        pContentFilter = new UaContentFilter;
        // [0] [1] AND [2] ------------------------------------------
        pContentFilterElement = new UaContentFilterElement;
        // Operator And
        pContentFilterElement->setFilterOperator(OpcUa_FilterOperator_And);
        // Operand 1 (Element)
        pOperand = new UaElementOperand;
        ((UaElementOperand*)pOperand)->setIndex(1);
        pContentFilterElement->setFilterOperand(0, pOperand, 2);
        // Operand 2 (Element)
        pOperand = new UaElementOperand;
        ((UaElementOperand*)pOperand)->setIndex(2);
        pContentFilterElement->setFilterOperand(1, pOperand, 2);
        pContentFilter->setContentFilterElement(0, pContentFilterElement, 5);
        // [1] Severity > 100  --------------------------------------
        pContentFilterElement = new UaContentFilterElement;
        // Operator GreaterThan
        pContentFilterElement->setFilterOperator(OpcUa_FilterOperator_GreaterThan);
        // Operand 1 (SimpleAttribute)
        pOperand = new UaSimpleAttributeOperand;
        ((UaSimpleAttributeOperand*)pOperand)->setBrowsePathElement(0, UaQualifiedName("Severity", 0), 1);;
        pContentFilterElement->setFilterOperand(0, pOperand, 2);
        // Operand 2 (Literal)
        pOperand = new UaLiteralOperand;
        ((UaLiteralOperand*)pOperand)->setLiteralValue(UaVariant((OpcUa_UInt16)100));
        pContentFilterElement->setFilterOperand(1, pOperand, 2);
        pContentFilter->setContentFilterElement(1, pContentFilterElement, 5);
        // [2] [3] OR [4] ------------------------------------------
        pContentFilterElement = new UaContentFilterElement;
        // Operator And
        pContentFilterElement->setFilterOperator(OpcUa_FilterOperator_Or);
        // Operand 1 (Element)
        pOperand = new UaElementOperand;
        ((UaElementOperand*)pOperand)->setIndex(3);
        pContentFilterElement->setFilterOperand(0, pOperand, 2);
        // Operand 2 (Element)
        pOperand = new UaElementOperand;
        ((UaElementOperand*)pOperand)->setIndex(4);
        pContentFilterElement->setFilterOperand(1, pOperand, 2);
        pContentFilter->setContentFilterElement(2, pContentFilterElement, 5);
        // [3] OfTpype(ControllerEventType) --------------------------
        pContentFilterElement = new UaContentFilterElement;
        // Operator OfType
        pContentFilterElement->setFilterOperator(OpcUa_FilterOperator_OfType);
        // Operand 1 (Literal)
        pOperand = new UaLiteralOperand;
        ((UaLiteralOperand*)pOperand)->setLiteralValue(UaVariant(UaNodeId(4000, 3))); // ControllerEventType
        pContentFilterElement->setFilterOperand(0, pOperand, 1);
        pContentFilter->setContentFilterElement(3, pContentFilterElement, 5);
        // [4] OfTpype(OffNormalAlarmType) --------------------------
        pContentFilterElement = new UaContentFilterElement;
        // Operator OfType
        pContentFilterElement->setFilterOperator(OpcUa_FilterOperator_OfType);
        // Operand 1 (Literal)
        pOperand = new UaLiteralOperand;
        ((UaLiteralOperand*)pOperand)->setLiteralValue(UaVariant(UaNodeId(OpcUaId_OffNormalAlarmType)));
        pContentFilterElement->setFilterOperand(0, pOperand, 1);
        pContentFilter->setContentFilterElement(4, pContentFilterElement, 5);
        //
        eventFilter.setWhereClause(pContentFilter);
        // -------------------------------------------------------------------------

        // Detach EventFilter to the create monitored item request
        eventFilter.detachFilter(monitoredItemCreateRequests[count].RequestedParameters.Filter);
        // ------------------------------------------------------
        // ------------------------------------------------------

        /*********************************************************************
        Call createMonitoredItems service
        **********************************************************************/
        status = pUaSubscription->createMonitoredItems(
            serviceSettings,               // Use default settings
            OpcUa_TimestampsToReturn_Both, // Select timestamps to return
            monitoredItemCreateRequests,   // monitored items to create
            monitoredItemCreateResults);   // Returned monitored items create result
                                           /*********************************************************************/
        if (status.isBad())
        {
            printf("** Error: UaSession::createMonitoredItems failed [ret=%s]\n", status.toString().toUtf8());
            printf("****************************************************************\n");
            return;
        }
        else
        {
            printf("** UaSession::createMonitoredItems result **********************\n");
            for (i = 0; i<count; i++)
            {
                UaNodeId node(monitoredItemCreateRequests[i].ItemToMonitor.NodeId);
                if (OpcUa_IsGood(monitoredItemCreateResults[i].StatusCode))
                {
                    printf("** Variable %s MonitoredItemId = %d\n", node.toString().toUtf8(), monitoredItemCreateResults[i].MonitoredItemId);
                }
                else
                {
                    printf("** Variable %s failed with error %s\n", node.toString().toUtf8(), UaStatus(monitoredItemCreateResults[i].StatusCode).toString().toUtf8());
                }
            }
            if (OpcUa_IsGood(monitoredItemCreateResults[count].StatusCode))
            {
                printf("** Event MonitoredItemId = %d\n", monitoredItemCreateResults[count].MonitoredItemId);
            }
            else
            {
                printf("** Event MonitoredItem for Server Object failed!\n");
            }
            printf("****************************************************************\n");
        }
    }

    printf("\n*******************************************************\n");
    printf("*******************************************************\n");
    printf("** We wait for 3 seconds then session is closed  ******\n");
    printf("*******************************************************\n");
    printf("*******************************************************\n");

    UaThread::sleep(3);

    OpcUa_UInt32 subscriptionId = pUaSubscription->subscriptionId();
    UaSubscription* pUaSubscription2 = NULL;

    bool doLoop = true;
    while (doLoop)
    {
        doLoop = false;

        // Disconnect from server but keep durable subcription
        g_pUaSession->disconnect(serviceSettings, OpcUa_False);
        // This emulates a disconnected client that comes back later to continue with data delivery


        printf("\n*******************************************************\n");
        printf("*******************************************************\n");
        printf("**        Press x to recreate session           *******\n");
        printf("**        and transfer durable subscription     *******\n");
        printf("*******************************************************\n");
        printf("*******************************************************\n");

        int action;
        /******************************************************************************/
        /* Wait for user command to stop the subscription. */
        while (!WaitForKeypress(action))
        {
            UaThread::msleep(100);
        }
        /******************************************************************************/


        SessionConnectInfo sessionConnectInfo;
        sessionConnectInfo.sApplicationName = g_pClientSampleConfig->sApplicationName();
        sessionConnectInfo.sApplicationUri = g_pClientSampleConfig->sApplicationUri();
        sessionConnectInfo.sProductUri = g_pClientSampleConfig->sProductUri();
        sessionConnectInfo.sSessionName = UaString("Client_Cpp_SDK@%1").arg(g_pClientSampleConfig->sHostName());
        sessionConnectInfo.bAutomaticReconnect = OpcUa_True;

        /*********************************************************************
        Connect to OPC UA Server
        **********************************************************************/
        status = g_pUaSession->connect(
            sUrl,                // URL of the Endpoint - from discovery or config
            sessionConnectInfo,  // General settings for connection
            sessionSecurityInfo, // Security settings
            g_pCallback);        // Callback interface
                                 /*********************************************************************/
        if (status.isBad())
        {
            delete g_pUaSession;
            g_pUaSession = NULL;
            printf("** Error: UaSession::connect failed [ret=%s]\n", status.toString().toUtf8());
            printf("****************************************************************\n");

            return;
        }

        UaUInt32Array   availableSequenceNumbers;

        status = g_pUaSession->transferSubscription(
            serviceSettings,
            g_pCallback,
            0,
            subscriptionId,
            subscriptionSettings,
            OpcUa_True,
            OpcUa_True,
            &pUaSubscription2,
            availableSequenceNumbers);
        if (status.isBad())
        {
            printf("** Error: UaSession::transferSubscription failed [ret=%s] **\n", status.toString().toUtf8());
        }
        else
        {
            printf("** OK: UaSession::transferSubscription succeeded **\n");
        }


        printf("\n*******************************************************\n");
        printf("*******************************************************\n");
        printf("**    Press x to delete durabel subscription          *\n");
        printf("**    Press 1 to continue disconnect / reconnect loop *\n");
        printf("*******************************************************\n");
        printf("*******************************************************\n");

        /******************************************************************************/
        /* Wait for user command to stop the subscription. */
        while (!WaitForKeypress(action))
        {
            if (action == 1)
            {
                doLoop = true;
                break;
            }
            UaThread::msleep(100);
        }
        /******************************************************************************/
    }

    /*********************************************************************
    Delete Subscription
    **********************************************************************/
    status = g_pUaSession->deleteSubscription(
        serviceSettings,    // Use default settings
        &pUaSubscription2);  // Subcription
                            /*********************************************************************/
    if (status.isBad())
    {
        printf("** UaSession::deleteSubscription failed! **********************\n");
        printf("****************************************************************\n");
    }
    else
    {
        pUaSubscription = NULL;
        printf("** UaSession::deleteSubscription succeeded!\n");
        printf("****************************************************************\n");
    }
}

/*============================================================================
 * subscribeAlarms - Subscribe for data changes to receive alarms
 *===========================================================================*/
void subscribeAlarms()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to create a subscription\n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    UaStatus             status;
    UaSubscription*      pUaSubscription       = NULL;
    SubscriptionSettings subscriptionSettings;
    subscriptionSettings.publishingInterval    = 500;
    ServiceSettings      serviceSettings;
    CallbackAlarms       callbackAlarms;


    /*********************************************************************
     Create a Subscription
    **********************************************************************/
    status = g_pUaSession->createSubscription(
        serviceSettings,        // Use default settings
        &callbackAlarms,        // Callback object
        0,                      // We have only one subscription, handle is not needed
        subscriptionSettings,   // general settings
        OpcUa_True,             // Publishing enabled
        &pUaSubscription);      // Returned Subscription instance
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::createSubscription failed [ret=%s] **\n", status.toString().toUtf8());
    }
    else
    {
        printf("****************************************************************\n");
        printf("** Try to create monitored items\n");

        UaMonitoredItemCreateRequests monitoredItemCreateRequests;
        UaMonitoredItemCreateResults  monitoredItemCreateResults;

        // Initialize IN parameter monitoredItemCreateRequests - just use one monitored item
        monitoredItemCreateRequests.create(1);

        // ------------------------------------------------------
        // ------------------------------------------------------
        // Set Event item
        monitoredItemCreateRequests[0].ItemToMonitor.NodeId.Identifier.Numeric = OpcUaId_Server;
        monitoredItemCreateRequests[0].ItemToMonitor.AttributeId = OpcUa_Attributes_EventNotifier;
        monitoredItemCreateRequests[0].MonitoringMode = OpcUa_MonitoringMode_Reporting;
        monitoredItemCreateRequests[0].RequestedParameters.ClientHandle = 1;
        monitoredItemCreateRequests[0].RequestedParameters.SamplingInterval = 0; // 0 is required by OPC UA spec
        monitoredItemCreateRequests[0].RequestedParameters.QueueSize = 0;

        UaEventFilter            eventFilter;
        UaSimpleAttributeOperand selectElement;
        UaContentFilter*         pContentFilter        = NULL;
        UaContentFilterElement*  pContentFilterElement = NULL;
        UaFilterOperand*         pOperand              = NULL;

        // -------------------------------------------------------------------------
        // Define select clause with a set of event fields to be returned with every event
        // In addition we need a select clause to receive alarms
        // -------------------------------------------------------------------------
        // Select EventId (BaseEventType)
        selectElement.setBrowsePathElement(0, UaQualifiedName("EventId", 0), 1);
        eventFilter.setSelectClauseElement(0, selectElement, 15);
        // Select EventType (BaseEventType)
        selectElement.setBrowsePathElement(0, UaQualifiedName("EventType", 0), 1);
        eventFilter.setSelectClauseElement(1, selectElement, 15);
        // Select SourceName (BaseEventType)
        selectElement.setBrowsePathElement(0, UaQualifiedName("SourceName", 0), 1);
        eventFilter.setSelectClauseElement(2, selectElement, 15);
        // Select Time (BaseEventType)
        selectElement.setBrowsePathElement(0, UaQualifiedName("Time", 0), 1);
        eventFilter.setSelectClauseElement(3, selectElement, 15);
        // Select Message (BaseEventType)
        selectElement.setBrowsePathElement(0, UaQualifiedName("Message", 0), 1);
        eventFilter.setSelectClauseElement(4, selectElement, 15);
        // Select Severity (BaseEventType)
        selectElement.setBrowsePathElement(0, UaQualifiedName("Severity", 0), 1);
        eventFilter.setSelectClauseElement(5, selectElement, 15);
        // Select BranchId (ConditionType)
        selectElement.setBrowsePathElement(0, UaQualifiedName("BranchId", 0), 1);
        eventFilter.setSelectClauseElement(6, selectElement, 15);
        // Select ConditionName (ConditionType)
        selectElement.setBrowsePathElement(0, UaQualifiedName("ConditionName", 0), 1);
        eventFilter.setSelectClauseElement(7, selectElement, 15);
        // Select Retain (ConditionType)
        selectElement.setBrowsePathElement(0, UaQualifiedName("Retain", 0), 1);
        eventFilter.setSelectClauseElement(8, selectElement, 15);
        // Select AckedState -> Id (AcknowledgableConditionType)
        selectElement.setBrowsePathElement(0, UaQualifiedName("AckedState", 0), 1);
        selectElement.setBrowsePathElement(1, UaQualifiedName("Id", 0), 2);
        eventFilter.setSelectClauseElement(9, selectElement, 15);
        // Select ConfirmedState -> Id (AcknowledgableConditionType)
        selectElement.setBrowsePathElement(0, UaQualifiedName("ConfirmedState", 0), 1);
        selectElement.setBrowsePathElement(1, UaQualifiedName("Id", 0), 2);
        eventFilter.setSelectClauseElement(10, selectElement, 15);
        selectElement.clearBrowsePath();
        // Select ActiveState (AlarmConditionType)
        selectElement.setBrowsePathElement(0, UaQualifiedName("ActiveState", 0), 1);
        eventFilter.setSelectClauseElement(11, selectElement, 15);
        // Select ActiveState -> Id (AlarmConditionType)
        selectElement.setBrowsePathElement(0, UaQualifiedName("ActiveState", 0), 2);
        selectElement.setBrowsePathElement(1, UaQualifiedName("Id", 0), 2);
        eventFilter.setSelectClauseElement(12, selectElement, 15);
        // Select ActiveState -> EffectiveDisplayName (AlarmConditionType)
        selectElement.setBrowsePathElement(0, UaQualifiedName("ActiveState", 0), 2);
        selectElement.setBrowsePathElement(1, UaQualifiedName("EffectiveDisplayName", 0), 2);
        eventFilter.setSelectClauseElement(13, selectElement, 15);
        // Select Condition NodeId
        // This is special - the path is empty and the requested Attribute is NodeId
        selectElement.setTypeId(UaNodeId(OpcUaId_ConditionType));
        selectElement.setAttributeId(OpcUa_Attributes_NodeId);
        selectElement.clearBrowsePath();
        eventFilter.setSelectClauseElement(14, selectElement, 15);
        // -------------------------------------------------------------------------

        // -------------------------------------------------------------------------
        // Define the where clause to filter for OfType(ConditionType)
        // -------------------------------------------------------------------------
        pContentFilter = new UaContentFilter;

        // OfType(ConditionType) --------------------------
        pContentFilterElement = new UaContentFilterElement;
        // Operator OfType
        pContentFilterElement->setFilterOperator(OpcUa_FilterOperator_OfType);
        // Operand 1 (Literal)
        pOperand = new UaLiteralOperand;
        ((UaLiteralOperand*)pOperand)->setLiteralValue(UaVariant(UaNodeId(OpcUaId_ConditionType)));
        pContentFilterElement->setFilterOperand(0, pOperand, 1);
        pContentFilter->setContentFilterElement(0, pContentFilterElement, 1);
        //
        eventFilter.setWhereClause(pContentFilter);
        // -------------------------------------------------------------------------

        // Detach EventFilter to the create monitored item request
        eventFilter.detachFilter(monitoredItemCreateRequests[0].RequestedParameters.Filter);
        // ------------------------------------------------------
        // ------------------------------------------------------

        /*********************************************************************
         Call createMonitoredItems service
        **********************************************************************/
        status = pUaSubscription->createMonitoredItems(
            serviceSettings,               // Use default settings
            OpcUa_TimestampsToReturn_Both, // Select timestamps to return
            monitoredItemCreateRequests,   // monitored items to create
            monitoredItemCreateResults);   // Returned monitored items create result
        /*********************************************************************/
        if ( status.isBad() )
        {
            printf("** Error: UaSession::createMonitoredItems failed [ret=%s]\n", status.toString().toUtf8());
            printf("****************************************************************\n");
            return;
        }
        else
        {
            printf("** UaSession::createMonitoredItems result **********************\n");

            if ( OpcUa_IsGood(monitoredItemCreateResults[0].StatusCode) )
            {
                printf("** Event MonitoredItemId = %d\n", monitoredItemCreateResults[0].MonitoredItemId);
            }
            else
            {
                printf("** Event MonitoredItem for Server Object failed!\n");
            }
            printf("****************************************************************\n");
        }
    }

    // call refresh to get all active events
    if ( status.isGood() )
    {
        CallIn  callRequest;
        CallOut callResults;

        callRequest.objectId.setNodeId(OpcUaId_ConditionType, 0);
        callRequest.methodId.setNodeId(OpcUaId_ConditionType_ConditionRefresh, 0);
        callRequest.inputArguments.create(1);

        // set subscriptionId
        UaVariant varTmp;
        varTmp.setUInt32(pUaSubscription->subscriptionId());
        varTmp.copyTo(&callRequest.inputArguments[0]);

        /*********************************************************************
         Call the call service
        **********************************************************************/
        status = g_pUaSession->call(
            serviceSettings,    // Use default settings
            callRequest,        // Parameters for call configured above
            callResults);       // Results for the call
        /*********************************************************************/
        if ( status.isBad() )
        {
            printf("** Error: UaSession::call refresh failed [ret=%s]\n", status.toString().toUtf8());
        }
        else
        {
            printf("**UaSession::call refresh succeeded!\n");
        }
    }

    printf("\n************************************************************\n");
    printf("**************************************************************\n");
    printf("**     Press x to stop subscription                    *******\n");
    printf("**     Press a to acknowledge all                      *******\n");
    printf("**     Press c to confirm all                          *******\n");
    printf("**     Press number to acknowledge / confirm single    *******\n");
    printf("**************************************************************\n");
    printf("**************************************************************\n");

    int action;
    /******************************************************************************/
    /* Wait for user command. */
    while (!WaitForKeypress(action))
    {
        if ( action != -1 )
        {
            bool bCallAcknowledge = false;
            bool bCallConfirm = false;

            UaCallMethodRequests    callMethodRequests;
            UaCallMethodResults     callMethodResults;
            UaDiagnosticInfos       diagnosticInfos;

            // acknowledge all
            if ( action == 10 )
            {
                // get list of conditions
                std::vector<EventObject> listEvents = callbackAlarms.getEventList();
                std::vector<EventObject>::iterator it = listEvents.begin();

                callMethodRequests.create((OpcUa_UInt32) listEvents.size());
                int iAckCount = 0;

                while (it != listEvents.end())
                {
                    if ( it->ackedStateId == OpcUa_False )
                    {
                        // set object id
                        it->conditionNodeId.copyTo(&callMethodRequests[iAckCount].ObjectId);

                        // set method id
                        UaNodeId(OpcUaId_AcknowledgeableConditionType_Acknowledge).copyTo(&callMethodRequests[iAckCount].MethodId);

                        // set input arguments
                        UaVariantArray inputArguments;
                        inputArguments.create(2);

                        UaVariant vEventId;
                        vEventId.setByteString(it->eventId, OpcUa_False);
                        vEventId.copyTo(&inputArguments[0]);

                        UaLocalizedText ltComment("en", "acknowledge all from sample client");
                        UaVariant vComment;
                        vComment.setLocalizedText(ltComment);
                        vComment.copyTo(&inputArguments[1]);

                        callMethodRequests[iAckCount].NoOfInputArguments = inputArguments.length();
                        callMethodRequests[iAckCount].InputArguments = inputArguments.detach();

                        iAckCount++;
                    }
                    it++;
                }
                callMethodRequests.resize(iAckCount);

                if ( iAckCount > 0 )
                {
                    bCallAcknowledge = true;
                }
            }
            // confirm all
            else if ( action == 12)
            {
                // get list of conditions
                std::vector<EventObject> listEvents = callbackAlarms.getEventList();
                std::vector<EventObject>::iterator it = listEvents.begin();

                callMethodRequests.create((OpcUa_UInt32) listEvents.size());
                int iConfirmCount = 0;

                while (it != listEvents.end())
                {
                    if ( it->confirmedStateId == OpcUa_False )
                    {
                        // set object id
                        it->conditionNodeId.copyTo(&callMethodRequests[iConfirmCount].ObjectId);

                        // set method id
                        UaNodeId(OpcUaId_AcknowledgeableConditionType_Confirm).copyTo(&callMethodRequests[iConfirmCount].MethodId);

                        // set input arguments
                        UaVariantArray inputArguments;
                        inputArguments.create(2);

                        UaVariant vEventId;
                        vEventId.setByteString(it->eventId, OpcUa_False);
                        vEventId.copyTo(&inputArguments[0]);

                        UaLocalizedText ltComment("en", "confirm all from sample client");
                        UaVariant vComment;
                        vComment.setLocalizedText(ltComment);
                        vComment.copyTo(&inputArguments[1]);

                        callMethodRequests[iConfirmCount].NoOfInputArguments = inputArguments.length();
                        callMethodRequests[iConfirmCount].InputArguments = inputArguments.detach();

                        iConfirmCount++;
                    }
                    it++;
                }
                callMethodRequests.resize(iConfirmCount);

                if ( iConfirmCount > 0 )
                {
                    bCallConfirm = true;
                }
            }
            // acknowledge / confirm single entry
            // this only works for entries 0 - 9
            else if ( action >= 0 && action < 10 )
            {
                // get list of conditions
                std::vector<EventObject> listEvents = callbackAlarms.getEventList();

                if ( action >= 0 && action < (int)listEvents.size() )
                {
                    // acknowledge if not acknowledged jet
                    if ( listEvents[action].ackedStateId == OpcUa_False )
                    {
                        callMethodRequests.create(1);

                        // set object id
                        listEvents[action].conditionNodeId.copyTo(&callMethodRequests[0].ObjectId);

                        // set method id
                        UaNodeId(OpcUaId_AcknowledgeableConditionType_Acknowledge).copyTo(&callMethodRequests[0].MethodId);

                        // set input arguments
                        UaVariantArray inputArguments;
                        inputArguments.create(2);

                        UaVariant vEventId;
                        vEventId.setByteString(listEvents[action].eventId, OpcUa_False);
                        vEventId.copyTo(&inputArguments[0]);

                        UaLocalizedText ltComment("en", "acknowledge from sample client");
                        UaVariant vComment;
                        vComment.setLocalizedText(ltComment);
                        vComment.copyTo(&inputArguments[1]);

                        callMethodRequests[0].NoOfInputArguments = inputArguments.length();
                        callMethodRequests[0].InputArguments = inputArguments.detach();

                        bCallAcknowledge = true;
                    }
                    // confirm if not confirmed jet
                    if ( listEvents[action].confirmedStateId == OpcUa_False )
                    {
                        callMethodRequests.create(1);

                        // set object id
                        listEvents[action].conditionNodeId.copyTo(&callMethodRequests[0].ObjectId);

                        // set method id
                        UaNodeId(OpcUaId_AcknowledgeableConditionType_Confirm).copyTo(&callMethodRequests[0].MethodId);

                        // set input arguments
                        UaVariantArray inputArguments;
                        inputArguments.create(2);

                        UaVariant vEventId;
                        vEventId.setByteString(listEvents[action].eventId, OpcUa_False);
                        vEventId.copyTo(&inputArguments[0]);

                        UaLocalizedText ltComment("en", "confirm from sample client");
                        UaVariant vComment;
                        vComment.setLocalizedText(ltComment);
                        vComment.copyTo(&inputArguments[1]);

                        callMethodRequests[0].NoOfInputArguments = inputArguments.length();
                        callMethodRequests[0].InputArguments = inputArguments.detach();

                        bCallConfirm = true;
                    }
                }
            }

            /*********************************************************************
            Call the call service
            **********************************************************************/
            if ( bCallConfirm || bCallAcknowledge )
            {
                if ( bCallAcknowledge )
                {
                    printf("-- Call Acknowledge -----------------------------------\n");
                    bCallAcknowledge = false;
                    /* ignore dead assignement included for comprehensiveness */
                    (void)bCallAcknowledge;
                }
                else
                {
                    printf("-- Call Confirm ---------------------------------------\n");
                    bCallConfirm = false;
                    /* ignore dead assignement included for comprehensiveness */
                    (void)bCallConfirm;
                }
                status = g_pUaSession->callList(
                    serviceSettings,    // Use default settings
                    callMethodRequests, // Parameters for call configured above
                    callMethodResults,  // Results for the call
                    diagnosticInfos);
                /*********************************************************************/
            }
        }

        UaThread::msleep(100);
    }
    /******************************************************************************/

    /*********************************************************************
     Delete Subscription
    **********************************************************************/
    status = g_pUaSession->deleteSubscription(
        serviceSettings,    // Use default settings
        &pUaSubscription);  // Subcription
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** UaSession::deleteSubscription failed! **********************\n");
        printf("****************************************************************\n");
    }
    else
    {
        pUaSubscription = NULL;
        printf("** UaSession::deleteSubscription succeeded!\n");
        printf("****************************************************************\n");
    }
}

/*============================================================================
 * Call a method
 *===========================================================================*/
void callMethod()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to call a method\n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    if (g_ObjectNodeIds.length() == 0 || g_MethodNodeIds.length() == 0)
    {
        printf("** Error in Configuration. ObjectId or MethodId not set\n");
        printf("****************************************************************\n");
        return;
    }

    UaStatus           status;
    OpcUa_UInt32       i = 0;
    CallIn             callRequest;
    CallOut            callResult;
    UaVariant          vTemp;
    ServiceSettings    serviceSettings;

    callRequest.objectId = g_ObjectNodeIds[0];
    callRequest.methodId = g_MethodNodeIds[0];

    UaArguments inputArguments;
    UaArguments outputArguments;
    g_pUaSession->getMethodArguments(serviceSettings, callRequest.methodId, inputArguments, outputArguments);

    if ( inputArguments.length() > 0 )
    {
        // Create input arguments value array
        callRequest.inputArguments.create(inputArguments.length());

        // Get input arguments
        printf("  Please enter the %d input arguments\n", inputArguments.length());
        for ( i=0; i<inputArguments.length(); i++ )
        {
            char argumentValue[256];
            UaString argumentName(inputArguments[i].Name);
            printf("   [%d] Enter %s  ", i, argumentName.toUtf8());
            if ( fgets(argumentValue, 256, stdin) != NULL )
            {
                // remove trailing line feed - fgets delivers string plus line feed
                size_t strLen = strlen(argumentValue);
                if (strLen > 0 && argumentValue[(strLen-1)] == '\n')
                {
                    argumentValue[(int)(strLen-1)] = '\0';
                }

                UaVariant tempValue(argumentValue);
                // Try to change to built-in time, more complex types are not supported here
                tempValue.changeType((OpcUa_BuiltInType)inputArguments[i].DataType.Identifier.Numeric, OpcUa_False);
                tempValue.copyTo(&callRequest.inputArguments[i]);
            }
            else
            {
                printf("   Reading argument [%d] %s failed\n", i, argumentName.toUtf8());
            }
        }
    }

    /*********************************************************************
     Call Method
    **********************************************************************/
    status = g_pUaSession->call(
        serviceSettings,    // Use default settings
        callRequest,        // In parameters and settings for the method call
        callResult);        // Out parameters and results returned from the method call
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::call failed [ret=%s]\n", status.toString().toUtf8());
    }
    else
    {
        printf("** UaSession::call succeeded!\n");

        if ( outputArguments.length() != callResult.outputArguments.length() )
        {
            printf("** UaSession::call - number of output arguments does not match OutputArguments property\n");
        }
        else
        {
            for ( i=0; i<callResult.outputArguments.length(); i++ )
            {
                UaString argumentName(outputArguments[i].Name);
                vTemp = callResult.outputArguments[i];
                if (vTemp.type() == OpcUaType_ExtensionObject)
                {
                    printExtensionObjects(vTemp, UaString("Out parameter %1").arg(argumentName));
                }
                else
                {
                    printf("  Out parameter %s: %s\n", argumentName.toUtf8(), vTemp.toString().toUtf8());
                }
            }
        }
    }
    printf("****************************************************************\n");
}

/*============================================================================
 * translateBrowsePathsToNodeIds - translate a browse path to a node id by OPC UA Server
 *===========================================================================*/
void translate()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to call translateBrowsePathsToNodeIds\n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    UaStatus                status;
    UaDiagnosticInfos       diagnosticInfos;
    ServiceSettings         serviceSettings;
    UaBrowsePaths           browsePaths;
    UaBrowsePathResults     browsePathResults;
    UaRelativePathElements  pathElements;
    OpcUa_UInt32            i;
    OpcUa_UInt32            itemCount;

    // Initialize IN parameters for translate
    // Create array with one elements
    itemCount = 3;
    browsePaths.create(itemCount);

    // Set first path
    printf("**    Path[0] start objects folder -> Server -> NamespaceArray\n");
    browsePaths[0].StartingNode.Identifier.Numeric = OpcUaId_ObjectsFolder;
    pathElements.create(2);
    pathElements[0].IncludeSubtypes = OpcUa_True;
    pathElements[0].IsInverse       = OpcUa_False;
    pathElements[0].ReferenceTypeId.Identifier.Numeric = OpcUaId_HierarchicalReferences;
    OpcUa_String_AttachReadOnly(&pathElements[0].TargetName.Name, "Server");
    pathElements[0].TargetName.NamespaceIndex = 0;
    pathElements[1].IncludeSubtypes = OpcUa_True;
    pathElements[1].IsInverse       = OpcUa_False;
    pathElements[1].ReferenceTypeId.Identifier.Numeric = OpcUaId_HierarchicalReferences;
    OpcUa_String_AttachReadOnly(&pathElements[1].TargetName.Name, "NamespaceArray");
    pathElements[1].TargetName.NamespaceIndex = 0;

    browsePaths[0].RelativePath.NoOfElements = pathElements.length();
    browsePaths[0].RelativePath.Elements = pathElements.detach();

    // Set second path
    printf("**    Path[1] start objects folder -> BuildingAutomation -> AirConditioner_1 -> Temperature\n");
    browsePaths[1].StartingNode.Identifier.Numeric = OpcUaId_ObjectsFolder;
    pathElements.create(3);
    pathElements[0].IncludeSubtypes = OpcUa_True;
    pathElements[0].IsInverse       = OpcUa_False;
    pathElements[0].ReferenceTypeId.Identifier.Numeric = OpcUaId_HierarchicalReferences;
    OpcUa_String_AttachReadOnly(&pathElements[0].TargetName.Name, "BuildingAutomation");
    pathElements[0].TargetName.NamespaceIndex = g_nsIndex;
    pathElements[1].IncludeSubtypes = OpcUa_True;
    pathElements[1].IsInverse       = OpcUa_False;
    pathElements[1].ReferenceTypeId.Identifier.Numeric = OpcUaId_HierarchicalReferences;
    OpcUa_String_AttachReadOnly(&pathElements[1].TargetName.Name, "AirConditioner_1");
    pathElements[1].TargetName.NamespaceIndex = g_nsIndex;
    pathElements[2].IncludeSubtypes = OpcUa_True;
    pathElements[2].IsInverse       = OpcUa_False;
    pathElements[2].ReferenceTypeId.Identifier.Numeric = OpcUaId_HierarchicalReferences;
    OpcUa_String_AttachReadOnly(&pathElements[2].TargetName.Name, "Temperature");
    pathElements[2].TargetName.NamespaceIndex = g_nsIndex;

    browsePaths[1].RelativePath.NoOfElements = pathElements.length();
    browsePaths[1].RelativePath.Elements = pathElements.detach();

     // Set third path
    printf("**    Path[1] start BuildingAutomation -> AirConditioner_1 -> TemperatureSetPoint\n");
    UaNodeId tempStartingNode("BuildingAutomation", g_nsIndex);
    tempStartingNode.copyTo(&browsePaths[2].StartingNode);
    pathElements.create(2);
    pathElements[0].IncludeSubtypes = OpcUa_True;
    pathElements[0].IsInverse       = OpcUa_False;
    pathElements[0].ReferenceTypeId.Identifier.Numeric = OpcUaId_HierarchicalReferences;
    OpcUa_String_AttachReadOnly(&pathElements[0].TargetName.Name, "AirConditioner_1");
    pathElements[0].TargetName.NamespaceIndex = g_nsIndex;
    pathElements[1].IncludeSubtypes = OpcUa_True;
    pathElements[1].IsInverse       = OpcUa_False;
    pathElements[1].ReferenceTypeId.Identifier.Numeric = OpcUaId_HierarchicalReferences;
    OpcUa_String_AttachReadOnly(&pathElements[1].TargetName.Name, "TemperatureSetPoint");
    pathElements[1].TargetName.NamespaceIndex = g_nsIndex;

    browsePaths[2].RelativePath.NoOfElements = pathElements.length();
    browsePaths[2].RelativePath.Elements = pathElements.detach();
    printf("****************************************************************\n");

    /*********************************************************************
     Browse Root of Server
    **********************************************************************/
    status = g_pUaSession->translateBrowsePathsToNodeIds(
        serviceSettings, // Use default settings
        browsePaths,
        browsePathResults,
        diagnosticInfos);
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::translateBrowsePathsToNodeIds failed [ret=%s]\n", status.toString().toUtf8());
        return;
    }
    else
    {
        for ( i=0; i < browsePathResults.length(); i++ )
        {
            if ( OpcUa_IsGood(browsePathResults[i].StatusCode) )
            {
                UaNodeId tempNode(browsePathResults[i].Targets[0].TargetId.NodeId);
                printf("**    Path[%d] result = %s\n", i, tempNode.toFullString().toUtf8());
            }
            else
            {
                printf("**    Path[%d] failed with error %s\n", i, UaStatus(browsePathResults[i].StatusCode).toString().toUtf8());
            }
        }
        printf("** UaSession::translateBrowsePathsToNodeIds succeeded\n");
        printf("****************************************************************\n");
    }
}

/*============================================================================
 * transferSubscription - transfer a subscription from one session to another session
 *===========================================================================*/
void transferSubscription(UaString& sUrl, SessionSecurityInfo& sessionSecurityInfo)
{
    printf("\n\n****************************************************************\n");
    printf("** Try to create a subscription\n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    UaStatus             status;
    UaSubscription*      pUaSubscription1      = NULL;
    SubscriptionSettings subscriptionSettings;
    subscriptionSettings.publishingInterval    = 500;
    ServiceSettings      serviceSettings;
    UaSession*           pNewSession = new UaSession();

    SessionConnectInfo sessionConnectInfo;
    sessionConnectInfo.sApplicationName = g_pClientSampleConfig->sApplicationName();
    sessionConnectInfo.sApplicationUri  = g_pClientSampleConfig->sApplicationUri();
    sessionConnectInfo.sProductUri      = g_pClientSampleConfig->sProductUri();
    sessionConnectInfo.sSessionName     = UaString("Client_Cpp_SDK_Transfer@%1").arg(g_pClientSampleConfig->sHostName());

    status = pNewSession->connect(
        sUrl,                // URL of the Endpoint - from discovery or config
        sessionConnectInfo,  // General settings for connection
        sessionSecurityInfo, // Security settings
        g_pCallback);        // Callback interface
    if ( status.isBad() )
    {
        delete pNewSession;
        pNewSession = NULL;
        printf("** Error: UaSession::connect failed [ret=%s]\n", status.toString().toUtf8());
        printf("****************************************************************\n");
        return;
    }

    /*********************************************************************
     Create a Subscription
    **********************************************************************/
    status = g_pUaSession->createSubscription(
        serviceSettings,        // Use default settings
        g_pCallback,            // Callback object
        0,                      // We have only one subscription, handle is not needed
        subscriptionSettings,   // general settings
        OpcUa_True,             // Publishing enabled
        &pUaSubscription1);      // Returned Subscription instance
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::createSubscription failed [ret=%s] **\n", status.toString().toUtf8());
    }
    else
    {
        printf("** OK: UaSession::createSubscription succeeded **\n");

        OpcUa_UInt32                  i;
        OpcUa_UInt32                  count;
        UaMonitoredItemCreateRequests monitoredItemCreateRequests;
        UaMonitoredItemCreateResults  monitoredItemCreateResults;

        // Initialize IN parameter monitoredItemCreateRequests
        count = g_VariableNodeIds.length();
        monitoredItemCreateRequests.create(count);
        for ( i=0; i<count; i++ )
        {
            g_VariableNodeIds[i].copyTo(&monitoredItemCreateRequests[i].ItemToMonitor.NodeId);
            monitoredItemCreateRequests[i].ItemToMonitor.AttributeId = OpcUa_Attributes_Value;
            monitoredItemCreateRequests[i].MonitoringMode = OpcUa_MonitoringMode_Reporting;
            monitoredItemCreateRequests[i].RequestedParameters.ClientHandle = i+1;
            monitoredItemCreateRequests[i].RequestedParameters.SamplingInterval = 1000;
            monitoredItemCreateRequests[i].RequestedParameters.QueueSize = 1;
            monitoredItemCreateRequests[i].RequestedParameters.DiscardOldest = OpcUa_True;
        }
        /*********************************************************************
         Call createMonitoredItems service
        **********************************************************************/
        status = pUaSubscription1->createMonitoredItems(
            serviceSettings,               // Use default settings
            OpcUa_TimestampsToReturn_Both, // Select timestamps to return
            monitoredItemCreateRequests,   // monitored items to create
            monitoredItemCreateResults);   // Returned monitored items create result
        /*********************************************************************/
        if ( status.isBad() )
        {
            printf("** Error: UaSession::createMonitoredItems failed [ret=%s]\n", status.toString().toUtf8());
            printf("****************************************************************\n");
        }

        printf("\n*******************************************************\n");
        printf("*******************************************************\n");
        printf("**        Press x to transfer subscription      *******\n");
        printf("*******************************************************\n");
        printf("*******************************************************\n");

        int action;
        /******************************************************************************/
        /* Wait for user command. */
        while (!WaitForKeypress(action))
        {
            UaThread::msleep(100);
        }
        /******************************************************************************/

        UaUInt32Array   availableSequenceNumbers;
        UaSubscription* pUaSubscription2      = NULL;

        status = pNewSession->transferSubscription(
            serviceSettings,
            g_pCallback,
            0,
            pUaSubscription1->subscriptionId(),
            subscriptionSettings,
            OpcUa_True,
            OpcUa_True,
            &pUaSubscription2,
            availableSequenceNumbers);
        if ( status.isBad() )
        {
            printf("** Error: UaSession::transferSubscription failed [ret=%s] **\n", status.toString().toUtf8());
        }
        else
        {
            printf("** OK: UaSession::transferSubscription succeeded **\n");
        }

        status = g_pUaSession->deleteSubscription(serviceSettings, &pUaSubscription1);
        if ( status.isBad() )
        {
            printf("** OK: UaSession::deleteSubscription 1 failed [ret=%s] **\n", status.toString().toUtf8());
        }
        else
        {
            printf("** Error: UaSession::deleteSubscription succeeded **\n");
        }

        printf("\n*******************************************************\n");
        printf("*******************************************************\n");
        printf("**        Press x to stop subscription          *******\n");
        printf("*******************************************************\n");
        printf("*******************************************************\n");

        /******************************************************************************/
        /* Wait for user command. */
        while (!WaitForKeypress(action))
        {
            UaThread::msleep(100);
        }
        /******************************************************************************/

        status = pNewSession->deleteSubscription(serviceSettings, &pUaSubscription2);
        if ( status.isBad() )
        {
            printf("** Error: UaSession::deleteSubscription 2 failed [ret=%s] **\n", status.toString().toUtf8());
        }
        else
        {
            printf("** OK: UaSession::deleteSubscription succeeded **\n");
        }
    }

    pNewSession->disconnect(serviceSettings, OpcUa_True);
    delete pNewSession;
}

/*============================================================================
 * historyReadDataRaw - read raw data history
 *===========================================================================*/
void historyReadDataRaw()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to read raw data history\n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }
    if ( g_HistoryDataNodeIds.length() < 1 )
    {
        printf("** Error: No history node configured\n");
        printf("****************************************************************\n");
        return;
    }

    UaStatus                      status;
    ServiceSettings               serviceSettings;
    HistoryReadRawModifiedContext historyReadRawModifiedContext;
    UaHistoryReadValueIds         nodesToRead;
    HistoryReadDataResults        results;
    UaDiagnosticInfos             diagnosticInfos;

    UaNodeId nodeToRead(g_HistoryDataNodeIds[0]);

    // Read the last 30 minutes
    UaDateTime timeSetting = UaDateTime::now();
    historyReadRawModifiedContext.startTime = timeSetting;
    timeSetting.addMilliSecs(-1800000);
    historyReadRawModifiedContext.endTime = timeSetting;
    historyReadRawModifiedContext.returnBounds = OpcUa_True;
    historyReadRawModifiedContext.numValuesPerNode = 100;

    // Read four aggregates from one node
    nodesToRead.create(1);
    nodeToRead.copyTo(&nodesToRead[0].NodeId);

    /*********************************************************************
     Update the history of events at an event notifier object
    **********************************************************************/
    status = g_pUaSession->historyReadRawModified(
        serviceSettings,
        historyReadRawModifiedContext,
        nodesToRead,
        results,
        diagnosticInfos);
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::historyReadRawModified failed [ret=%s]\n", status.toString().toUtf8());
        return;
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
                UaDateTime sourceTS(results[i].m_dataValues[j].SourceTimestamp);
                if ( OpcUa_IsGood(results[i].m_dataValues[j].StatusCode) )
                {
                    UaVariant tempValue = results[i].m_dataValues[j].Value;
                    printf("**    [%d] value %s ts %s status %s\n",
                        j,
                        tempValue.toString().toUtf8(),
                        sourceTS.toTimeString().toUtf8(),
                        statusOPLevel.toString().toUtf8());
                }
                else
                {
                    printf("**    [%d] status %s ts %s\n",
                        j,
                        statusOPLevel.toString().toUtf8(),
                        sourceTS.toTimeString().toUtf8());
                }
            }
        }
        printf("****************************************************************\n\n");

        while ( results[0].m_continuationPoint.length() > 0 )
        {
            printf("\n*******************************************************\n");
            printf("** More data available                          *******\n");
            printf("**        Press x to stop read                  *******\n");
            printf("**        Press c to continue                   *******\n");
            printf("*******************************************************\n");
            int action;
            /******************************************************************************/
            /* Wait for user command. */
            bool waitForInput = true;
            while (waitForInput)
            {
                if ( WaitForKeypress(action) )
                {
                    // x -> Release continuation point
                    historyReadRawModifiedContext.bReleaseContinuationPoints = OpcUa_True;
                    waitForInput = false;
                }
                // Continue
                else if ( action == 12 ) waitForInput = false;
                // Wait
                else UaThread::msleep(100);
            }
            /******************************************************************************/

            OpcUa_ByteString_Clear(&nodesToRead[0].ContinuationPoint);
            results[0].m_continuationPoint.copyTo(&nodesToRead[0].ContinuationPoint);
            status = g_pUaSession->historyReadRawModified(
                serviceSettings,
                historyReadRawModifiedContext,
                nodesToRead,
                results,
                diagnosticInfos);
            if ( status.isBad() )
            {
                printf("** Error: UaSession::historyReadRawModified with CP failed [ret=%s]\n", status.toString().toUtf8());
                return;
            }
            else
            {
                for ( i=0; i<results.length(); i++ )
                {
                    UaStatus nodeResult(results[i].m_status);
                    printf("** Results %d Node=%s status=%s\n", i, nodeToRead.toXmlString().toUtf8(), nodeResult.toString().toUtf8());

                    for ( j=0; j<results[i].m_dataValues.length(); j++ )
                    {
                        UaStatus statusOPLevel(results[i].m_dataValues[j].StatusCode);
                        UaDateTime sourceTS(results[i].m_dataValues[j].SourceTimestamp);
                        if ( OpcUa_IsGood(results[i].m_dataValues[j].StatusCode) )
                        {
                            UaVariant tempValue = results[i].m_dataValues[j].Value;
                            printf("**    [%d] value %s ts %s status %s\n",
                                j,
                                tempValue.toString().toUtf8(),
                                sourceTS.toTimeString().toUtf8(),
                                statusOPLevel.toString().toUtf8());
                        }
                        else
                        {
                            printf("**    [%d] status %s ts %s\n",
                                j,
                                statusOPLevel.toString().toUtf8(),
                                sourceTS.toTimeString().toUtf8());
                        }
                    }
                }
            }
        }
    }
}

/*============================================================================
 * historyReadDataProcessed - read processed data history
 *===========================================================================*/
void historyReadDataProcessed()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to read processed data history\n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }
    if ( g_HistoryDataNodeIds.length() < 1 )
    {
        printf("** Error: No history node configured\n");
        printf("****************************************************************\n");
        return;
    }

    UaStatus                    status;
    ServiceSettings             serviceSettings;
    HistoryReadProcessedContext historyReadProcessedContext;
    UaHistoryReadValueIds       nodesToRead;
    HistoryReadDataResults      results;
    UaDiagnosticInfos           diagnosticInfos;

    UaNodeId nodeToRead(g_HistoryDataNodeIds[0]);

    // Read the last 30 minutes
    UaDateTime timeSetting = UaDateTime::now();
    historyReadProcessedContext.startTime = timeSetting;
    timeSetting.addMilliSecs(-1800000);
    historyReadProcessedContext.endTime = timeSetting;
    // Calculate per minute
    historyReadProcessedContext.processingInterval = 60000;

    // Request Count, Minimum, Maximum and Average
    historyReadProcessedContext.aggregateTypes.create(4);
    historyReadProcessedContext.aggregateTypes[0].Identifier.Numeric = OpcUaId_AggregateFunction_Count;
    historyReadProcessedContext.aggregateTypes[1].Identifier.Numeric = OpcUaId_AggregateFunction_Minimum;
    historyReadProcessedContext.aggregateTypes[2].Identifier.Numeric = OpcUaId_AggregateFunction_Maximum;
    historyReadProcessedContext.aggregateTypes[3].Identifier.Numeric = OpcUaId_AggregateFunction_Average;

    // Read four aggregates from one node
    nodesToRead.create(4);
    nodeToRead.copyTo(&nodesToRead[0].NodeId);
    nodeToRead.copyTo(&nodesToRead[1].NodeId);
    nodeToRead.copyTo(&nodesToRead[2].NodeId);
    nodeToRead.copyTo(&nodesToRead[3].NodeId);

    /*********************************************************************
     Update the history of events at an event notifier object
    **********************************************************************/
    status = g_pUaSession->historyReadProcessed(
        serviceSettings,
        historyReadProcessedContext,
        nodesToRead,
        results,
        diagnosticInfos);
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::historyReadProcessed failed [ret=%s]\n", status.toString().toUtf8());
        return;
    }
    else
    {
        OpcUa_UInt32 i, j;
        for ( i=0; i<results.length(); i++ )
        {
            UaStatus nodeResult(results[i].m_status);
            if ( i==0 )      printf("** Results %d Node=%s Aggregate=Count status=%s\n", i, nodeToRead.toXmlString().toUtf8(), nodeResult.toString().toUtf8());
            else if ( i==1 ) printf("** Results %d Node=%s Aggregate=Minimum status=%s\n", i, nodeToRead.toXmlString().toUtf8(), nodeResult.toString().toUtf8());
            else if ( i==2 ) printf("** Results %d Node=%s Aggregate=Maximum status=%s\n", i, nodeToRead.toXmlString().toUtf8(), nodeResult.toString().toUtf8());
            else if ( i==3 ) printf("** Results %d Node=%s Aggregate=Average status=%s\n", i, nodeToRead.toXmlString().toUtf8(), nodeResult.toString().toUtf8());
            else             printf("** Unexpected Results %d\n", i);

            for ( j=0; j<results[i].m_dataValues.length(); j++ )
            {
                UaStatus statusOPLevel(results[i].m_dataValues[j].StatusCode);
                UaDateTime sourceTS(results[i].m_dataValues[j].SourceTimestamp);
                if ( OpcUa_IsGood(results[i].m_dataValues[j].StatusCode) )
                {
                    UaVariant tempValue = results[i].m_dataValues[j].Value;
                    printf("**    [%d] value %s ts %s status %s\n",
                        j,
                        tempValue.toString().toUtf8(),
                        sourceTS.toTimeString().toUtf8(),
                        statusOPLevel.toString().toUtf8());
                }
                else
                {
                    printf("**    [%d] status %s ts %s\n",
                        j,
                        statusOPLevel.toString().toUtf8(),
                        sourceTS.toTimeString().toUtf8());
                }
            }
        }
    }
}

/*============================================================================
 * historyReadDataAtTime - read data history at timestamps
 *===========================================================================*/
void historyReadDataAtTime()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to read data history at timestamps \n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }
    if ( g_HistoryDataNodeIds.length() < 1 )
    {
        printf("** Error: No history node configured\n");
        printf("****************************************************************\n");
        return;
    }

    UaStatus                    status;
    ServiceSettings             serviceSettings;
    HistoryReadAtTimeContext    historyReadAtTimeContext;
    UaHistoryReadValueIds       nodesToRead;
    HistoryReadDataResults      results;
    UaDiagnosticInfos           diagnosticInfos;
    OpcUa_UInt32 i, j, count;

    UaNodeId nodeToRead(g_HistoryDataNodeIds[0]);

    count = 10;

    // Read the values from the last 30 seconds
    historyReadAtTimeContext.useSimpleBounds = OpcUa_True;
    historyReadAtTimeContext.requestedTimes.create(count);
    UaDateTime timeSetting = UaDateTime::now();
    for ( i=0; i<count; i++ )
    {
        timeSetting.addMilliSecs(-1000);
        timeSetting.copyTo(&historyReadAtTimeContext.requestedTimes[i]);
    }

    // Read at time from one node
    nodesToRead.create(1);
    nodeToRead.copyTo(&nodesToRead[0].NodeId);

    /*********************************************************************
     Update the history of events at an event notifier object
    **********************************************************************/
    status = g_pUaSession->historyReadAtTime(
        serviceSettings,
        historyReadAtTimeContext,
        nodesToRead,
        results,
        diagnosticInfos);
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::historyReadAtTime failed [ret=%s]\n", status.toString().toUtf8());
        return;
    }
    else
    {
        for ( i=0; i<results.length(); i++ )
        {
            UaStatus nodeResult(results[i].m_status);
            printf("** Results %d Node=%s status=%s\n", i, nodeToRead.toXmlString().toUtf8(), nodeResult.toString().toUtf8());

            for ( j=0; j<results[i].m_dataValues.length(); j++ )
            {
                if ( j >= count )
                {
                    printf("**    More results returned than the %d requested\n", count);
                    break;
                }
                UaDateTime requTS(historyReadAtTimeContext.requestedTimes[j]);
                printf("**    [%d] Requested TS %s\n", j, requTS.toTimeString().toUtf8());
                UaStatus statusOPLevel(results[i].m_dataValues[j].StatusCode);
                UaDateTime sourceTS(results[i].m_dataValues[j].SourceTimestamp);
                if ( OpcUa_IsGood(results[i].m_dataValues[j].StatusCode) )
                {
                    UaVariant tempValue = results[i].m_dataValues[j].Value;
                    printf("**         value %s ts %s status %s\n",
                        tempValue.toString().toUtf8(),
                        sourceTS.toTimeString().toUtf8(),
                        statusOPLevel.toString().toUtf8());
                }
                else
                {
                    printf("**         status %s ts %s\n",
                        statusOPLevel.toString().toUtf8(),
                        sourceTS.toTimeString().toUtf8());
                }
            }
        }
    }
}

/*============================================================================
 * historyUpdateData - update data history
 *===========================================================================*/
void historyUpdateData()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to update data history \n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    if ( g_HistoryDataNodeIds.length() < 1 )
    {
        printf("** Error: No history node configured\n");
        printf("****************************************************************\n");
        return;
    }

    UaStatus                    status;
    ServiceSettings             serviceSettings;
    UpdateDataDetails           updateDataDetails;
    UaHistoryUpdateResults      results;
    UaDiagnosticInfos           diagnosticInfos;
    UaVariant                   value;
    UaDateTime                  dtNow;

    // setup data to write into history
    updateDataDetails.create(1);
    updateDataDetails[0].m_nodeId = g_HistoryDataNodeIds[0];
    updateDataDetails[0].m_PerformInsertReplace = OpcUa_PerformUpdateType_Update;
    updateDataDetails[0].m_dataValues.create(10);
    dtNow = UaDateTime::now();
    dtNow.addSecs(-20);

    // fill values to write into history
    for (int i = 0; i < 10; i++)
    {
        value.setDouble(i*i);
        value.copyTo(&updateDataDetails[0].m_dataValues[i].Value);
        updateDataDetails[0].m_dataValues[i].SourceTimestamp = dtNow;
        updateDataDetails[0].m_dataValues[i].ServerTimestamp = dtNow;
        dtNow.addSecs(1);
    }

    /*********************************************************************
     Update the history of events at an event notifier object
    **********************************************************************/
    status = g_pUaSession->historyUpdateData(
        serviceSettings,
        updateDataDetails,
        results,
        diagnosticInfos);
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::historyUpdateData failed [ret=%s]\n", status.toString().toUtf8());
        return;
    }
    else
    {
        OpcUa_Int32 i;

        if ( OpcUa_IsGood(results[0].StatusCode) )
        {
            printf("historyUpdateData returned:\n");
        }
        else
        {
            printf("historyUpdateData operation returned status = %s:\n", UaStatus(results[0].StatusCode).toString().toUtf8());
        }

        for ( i=0; i<results[0].NoOfOperationResults; i++ )
        {
            if ( OpcUa_IsNotGood(results[0].OperationResults[i]) )
            {
                printf("historyUpdateData operation[%d] failed with status = %s:\n", i, UaStatus(results[0].OperationResults[i]).toString().toUtf8());
            }
        }
    }
}

/*============================================================================
 * historyDeleteData - delete data history
 *===========================================================================*/
void historyDeleteData()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to delete data history \n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    if (g_HistoryDataNodeIds.length() < 1)
    {
        printf("** Error: No history node configured\n");
        printf("****************************************************************\n");
        return;
    }

    // delete the last 30 seoncdes of the history
    UaStatus                    status;
    ServiceSettings             serviceSettings;
    DeleteRawModifiedDetails    deleteDetails;
    UaHistoryUpdateResults      results;
    UaDiagnosticInfos           diagnosticInfos;

    deleteDetails.create(1);
    deleteDetails[0].m_startTime = UaDateTime::now();
    deleteDetails[0].m_startTime.addSecs(-30);
    deleteDetails[0].m_endTime = UaDateTime::now();
    deleteDetails[0].m_nodeId = g_HistoryDataNodeIds[0];
    deleteDetails[0].m_IsDeleteModified = OpcUa_False;

    /*********************************************************************
     Delete part of the history
    **********************************************************************/
    status = g_pUaSession->historyDeleteRawModified(
        serviceSettings, // Use default settings
        deleteDetails,
        results,
        diagnosticInfos);
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::historyDeleteRawModified failed [ret=%s]\n", status.toString().toUtf8());
        return;
    }
    else
    {
        if ( OpcUa_IsGood(results[0].StatusCode) )
        {
            printf("historyDeleteRawModified succeeeded.\n");
        }
        else
        {
            printf("historyDeleteRawModified operation returned status = %s:\n", UaStatus(results[0].StatusCode).toString().toUtf8());
        }

        OpcUa_Int32 i;
        for ( i=0; i<results[0].NoOfOperationResults; i++ )
        {
            if ( OpcUa_IsNotGood(results[0].OperationResults[i]) )
            {
                printf("historyDeleteRawModified operation[%d] failed with status = %s:\n", i, UaStatus(results[0].OperationResults[i]).toString().toUtf8());
            }
        }
    }
}

/*============================================================================
 * historyReadEvents - read event history
 *===========================================================================*/
void historyReadEvents()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to read event history \n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    if (g_EventTriggerObjects.length() < 1)
    {
        printf("** Error: No EventTriggerObject node configured\n");
        printf("****************************************************************\n");
        return;
    }

    if (g_EventTriggerMethods.length() < 1)
    {
        printf("** Error: No EventTriggerMethod node configured\n");
        printf("****************************************************************\n");
        return;
    }

    // call methods to generate events
    UaStatus           status;
    ServiceSettings    serviceSettings;
    CallIn             callRequest;
    CallOut            callResult;

    if (g_EventIds.length() < 3)
    {
        // Call Methods to generate events
        callRequest.objectId = g_EventTriggerObjects[0];
        callRequest.methodId = g_EventTriggerMethods[0];
        status = g_pUaSession->call(
            serviceSettings,    // Use default settings
            callRequest,        // In parameters and settings for the method call
            callResult);        // Out parameters and results returned from the method call
        if ( status.isBad() )
        {
            printf("** Error: UaSession::call failed [ret=%s]\n", status.toString().toUtf8());
        }
        // Call Methods to generate events
        callRequest.objectId = g_EventTriggerObjects[1];
        callRequest.methodId = g_EventTriggerMethods[1];
        status = g_pUaSession->call(
            serviceSettings,    // Use default settings
            callRequest,        // In parameters and settings for the method call
            callResult);        // Out parameters and results returned from the method call
        if ( status.isBad() )
        {
            printf("** Error: UaSession::call failed [ret=%s]\n", status.toString().toUtf8());
        }
    }

    HistoryReadEventContext  historyReadEventContext;
    UaHistoryReadValueIds    nodesToRead;
    HistoryReadEventResults  results;
    UaDiagnosticInfos        diagnosticInfos;
    UaSimpleAttributeOperand selectElement;

    nodesToRead.create(1);
    g_HistoryEventNodeId.copyTo(&nodesToRead[0].NodeId);

    // show events of last hour
    historyReadEventContext.startTime = UaDateTime::now();
    historyReadEventContext.endTime = UaDateTime::now();
    historyReadEventContext.startTime.addMilliSecs(-3600000);
    historyReadEventContext.numValuesPerNode = 10;

    // Define select clause with 4 event fields to be returned with every event
    selectElement.setBrowsePathElement(0, UaQualifiedName("Time", 0), 1);
    historyReadEventContext.eventFilter.setSelectClauseElement(0, selectElement, 4);
    selectElement.setBrowsePathElement(0, UaQualifiedName("Message", 0), 1);
    historyReadEventContext.eventFilter.setSelectClauseElement(1, selectElement, 4);
    selectElement.setBrowsePathElement(0, UaQualifiedName("SourceName", 0), 1);
    historyReadEventContext.eventFilter.setSelectClauseElement(2, selectElement, 4);
    selectElement.setBrowsePathElement(0, UaQualifiedName("EventId", 0), 1);
    historyReadEventContext.eventFilter.setSelectClauseElement(3, selectElement, 4);

    /*********************************************************************
     Read the history of events from an event notifier object
    **********************************************************************/
    status = g_pUaSession->historyReadEvent(
        serviceSettings,
        historyReadEventContext,
        nodesToRead,
        results,
        diagnosticInfos);
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::historyReadEvent failed [ret=%s]\n", status.toString().toUtf8());
        return;
    }
    else
    {
        // save the eventIds for later use in historyDeleteEvents
        g_EventIds.clear();
        g_EventIds.create(results[0].m_events.length());
        OpcUa_UInt32 i;
        OpcUa_UInt32 j;

        if ( results[0].m_status.isGood() )
        {
            printf("HistoryReadEvent returned:\n");
        }
        else
        {
            printf("HistoryReadEvent operation returned status = %s:\n", results[0].m_status.toString().toUtf8());
        }
        j = 0;
        for ( i=0; i<results[0].m_events.length(); i++ )
        {
            if ( results[0].m_events[i].NoOfEventFields == 4 )
            {
                UaVariant uvTime(results[0].m_events[i].EventFields[0]);
                UaVariant uvMessage(results[0].m_events[i].EventFields[1]);
                UaVariant uvSource(results[0].m_events[i].EventFields[2]);
                printf("%s %s %s \n", uvTime.toString().toUtf8(), uvSource.toString().toUtf8(), uvMessage.toString().toUtf8());

                UaVariant uvEventId(results[0].m_events[i].EventFields[3]);
                UaByteString bsEventId;
                if (OpcUa_IsGood(uvEventId.toByteString(bsEventId)))
                {
                    bsEventId.copyTo(&g_EventIds[j]);
                    j++;
                }
            }
            else
            {
                printf("Invalid event\n");
            }
        }
        g_EventIds.resize(j);

        while ( results[0].m_continuationPoint.length() > 0 )
        {
            printf("\n*******************************************************\n");
            printf("** More data available                          *******\n");
            printf("**        Press x to stop read                  *******\n");
            printf("**        Press c to continue                   *******\n");
            printf("*******************************************************\n");
            int action;
            /******************************************************************************/
            /* Wait for user command. */
            bool waitForInput = true;
            while (waitForInput)
            {
                if ( WaitForKeypress(action) )
                {
                    // x -> Release continuation point
                    historyReadEventContext.bReleaseContinuationPoints = OpcUa_True;
                    waitForInput = false;
                }
                // Continue
                else if ( action == 12 ) waitForInput = false;
                // Wait
                else UaThread::msleep(100);
            }
            /******************************************************************************/

            OpcUa_ByteString_Clear(&nodesToRead[0].ContinuationPoint);
            results[0].m_continuationPoint.copyTo(&nodesToRead[0].ContinuationPoint);
            status = g_pUaSession->historyReadEvent(
                serviceSettings,
                historyReadEventContext,
                nodesToRead,
                results,
                diagnosticInfos);
            if ( status.isBad() )
            {
                printf("** Error: UaSession::historyReadEvent with CP failed [ret=%s]\n", status.toString().toUtf8());
                return;
            }
            else
            {
                g_EventIds.resize(results[0].m_events.length() + g_EventIds.length());
                if ( results[0].m_status.isGood() )
                {
                    printf("HistoryReadEvent returned:\n");
                }
                else
                {
                    printf("HistoryReadEvent operation returned status = %s:\n", results[0].m_status.toString().toUtf8());
                }
                for ( i=0; i<results[0].m_events.length(); i++ )
                {
                    if ( results[0].m_events[i].NoOfEventFields == 4 )
                    {
                        UaVariant uvTime(results[0].m_events[i].EventFields[0]);
                        UaVariant uvMessage(results[0].m_events[i].EventFields[1]);
                        UaVariant uvSource(results[0].m_events[i].EventFields[2]);
                        printf("%s %s %s \n", uvTime.toString().toUtf8(), uvSource.toString().toUtf8(), uvMessage.toString().toUtf8());

                        UaVariant uvEventId(results[0].m_events[i].EventFields[3]);
                        UaByteString bsEventId;
                        if (OpcUa_IsGood(uvEventId.toByteString(bsEventId)))
                        {
                            bsEventId.copyTo(&g_EventIds[j]);
                            j++;
                        }
                    }
                    else
                    {
                        printf("Invalid event\n");
                    }
                }
                g_EventIds.resize(j);
            }
        }
    }
}

/*============================================================================
 * historyUpdateEvents - update event history
 *===========================================================================*/
void historyUpdateEvents()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to update event history \n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    UaStatus                    status;
    ServiceSettings             serviceSettings;
    UpdateEventDetails          updateEventDetails;
    UaHistoryUpdateResults      results;
    UaDiagnosticInfos           diagnosticInfos;
    UaSimpleAttributeOperand    selectElement;
    OpcUa_Int32                 i;

    updateEventDetails.create(1);
    updateEventDetails[0].m_PerformInsertReplace = OpcUa_PerformUpdateType_Update;
    updateEventDetails[0].m_nodeId = g_HistoryEventNodeId;

    // Define select clause with 4 event fields to update
    selectElement.setBrowsePathElement(0, UaQualifiedName("Time", 0), 1);
    updateEventDetails[0].m_eventFilter.setSelectClauseElement(0, selectElement, 4);
    selectElement.setBrowsePathElement(0, UaQualifiedName("Message", 0), 1);
    updateEventDetails[0].m_eventFilter.setSelectClauseElement(1, selectElement, 4);
    selectElement.setBrowsePathElement(0, UaQualifiedName("SourceName", 0), 1);
    updateEventDetails[0].m_eventFilter.setSelectClauseElement(2, selectElement, 4);
    selectElement.setBrowsePathElement(0, UaQualifiedName("EventType", 0), 1);
    updateEventDetails[0].m_eventFilter.setSelectClauseElement(3, selectElement, 4);

    // add 3 events with 4 fields each
    updateEventDetails[0].m_eventData.create(3);
    UaVariantArray eventFields;
    UaVariant value;
    UaDateTime dateTime = UaDateTime::now();
    UaHistoryEventFieldList eventFieldList;
    dateTime.addSecs(-10);

    for ( i=0; i<3; i++ )
    {
        eventFields.create(4);
        // set time
        dateTime.addSecs(1);
        value.setDateTime(dateTime);
        value.copyTo(&eventFields[0]);
        // set messsage
        value.setLocalizedText(UaLocalizedText("en",UaString("UpdateMessage_%1").arg(i+1)));
        value.copyTo(&eventFields[1]);
        // set source name
        value.setString(UaString("AirConditioner_%1").arg(i+1));
        value.copyTo(&eventFields[2]);
        // set event type - we use base event type here
        value.setNodeId(UaNodeId(OpcUaId_BaseEventType, 0));
        value.copyTo(&eventFields[3]);
        eventFieldList.setEventFields(eventFields);
        OpcUa_HistoryEventFieldList_Initialize(&updateEventDetails[0].m_eventData[i]);
        eventFieldList.copyTo(&updateEventDetails[0].m_eventData[i]);
    }

    /*********************************************************************
     Update the history of events at an event notifier object
    **********************************************************************/
    status = g_pUaSession->historyUpdateEvents(
        serviceSettings, // Use default settings
        updateEventDetails,
        results,
        diagnosticInfos);
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::historyUpdateEvents failed [ret=%s]\n", status.toString().toUtf8());
        return;
    }
    else
    {
        if ( OpcUa_IsGood(results[0].StatusCode) )
        {
            printf("historyUpdateEvents succeeded\n");

            for ( i=0; i<results[0].NoOfOperationResults; i++)
            {
                if ( OpcUa_IsNotGood(results[0].OperationResults[i]) )
                {
                    printf("OperationResults[%d] = %s:\n", i, UaStatus(results[0].OperationResults[i]).toString().toUtf8());
                }
            }
        }
        else
        {
            printf("historyUpdateEvents operation returned status = %s:\n", UaStatus(results[0].StatusCode).toString().toUtf8());
        }
    }
}

/*============================================================================
 * historyDeleteEvents - delete event history
 *===========================================================================*/
void historyDeleteEvents()
{
    printf("\n\n****************************************************************\n");
    printf("** Try to delete event history \n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    // we need some data to delete - the eventIds are filled in void historyReadEvents
    if (g_EventIds.length() < 3)
    {
        historyReadEvents();
    }

    if (g_EventIds.length() < 3)
    {
        printf("\n\n****************************************************************\n");
        printf("** Error: Not enough event history available - need at least 3 events\n");
        printf("****************************************************************\n");
        return;
    }

    // delete first 2 entries in the event history
    UaStatus                status;
    ServiceSettings         serviceSettings;
    DeleteEventDetails      deleteDetails;
    UaHistoryUpdateResults  results;
    UaDiagnosticInfos       diagnosticInfos;

    deleteDetails.create(1);
    deleteDetails[0].m_eventIds.create(2);
    deleteDetails[0].m_nodeId = g_HistoryEventNodeId;
    UaByteString::cloneTo(g_EventIds[0], deleteDetails[0].m_eventIds[0]);
    UaByteString::cloneTo(g_EventIds[1], deleteDetails[0].m_eventIds[1]);
    UaByteStringArray bsTmpArray = g_EventIds;
    g_EventIds.clear();
    g_EventIds.resize(bsTmpArray.length()-2);
    for (OpcUa_UInt32 i = 2; i < bsTmpArray.length(); i++)
    {
        g_EventIds[i-2] = bsTmpArray[i];
    }
    bsTmpArray.detach();

    /*********************************************************************
     Delete part of the event history
    **********************************************************************/
    status = g_pUaSession->historyDeleteEvents(
        serviceSettings, // Use default settings
        deleteDetails,
        results,
        diagnosticInfos);
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::historyDeleteEvents failed [ret=%s]\n", status.toString().toUtf8());
        return;
    }
    else
    {
        OpcUa_Int32 i;

        if ( OpcUa_IsGood(results[0].StatusCode) )
        {
            printf("historyDeleteEvents returned:\n");
        }
        else
        {
            printf("historyDeleteEvents operation returned status = %s:\n", UaStatus(results[0].StatusCode).toString().toUtf8());
        }

        for ( i=0; i<results[0].NoOfOperationResults; i++ )
        {
            if ( OpcUa_IsNotGood(results[0].OperationResults[i]) )
            {
                printf("historyDeleteEvents operation[%d] failed with status = %s:\n", i, UaStatus(results[0].OperationResults[i]).toString().toUtf8());
            }
        }
    }
}

/*============================================================================
 * Build node ids
 *===========================================================================*/
void buildNodeIds()
{
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        return;
    }

    UaStatus              status;
    UaReadValueIds        nodesToRead;
    UaDataValues          values;
    UaDiagnosticInfos     diagnosticInfos;
    ServiceSettings       serviceSettings;

    nodesToRead.create(1);
    // Set node id for name space array
    nodesToRead[0].NodeId.Identifier.Numeric = OpcUaId_Server_NamespaceArray;
    nodesToRead[0].AttributeId = OpcUa_Attributes_Value;

    /*********************************************************************
     Call read service
    **********************************************************************/
    status = g_pUaSession->read(
        serviceSettings,                // Use default settings
        0,                              // Max age
        OpcUa_TimestampsToReturn_Both,  // Time stapmps to return
        nodesToRead,                    // Array of nodes to read
        values,                         // Returns an array of values
        diagnosticInfos);               // Returns an array of diagnostic info
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::read failed [ret=0x%x] *********\n", status.statusCode());
    }
    else
    {
        if ( OpcUa_IsGood(values[0].StatusCode) )
        {
            UaVariant tempValue = values[0].Value;
            UaStringArray nameSpaceUris;
            status = tempValue.toStringArray(nameSpaceUris);
            if ( status.isGood() )
            {
                OpcUa_UInt16      i;
                OpcUa_UInt32      count;
                OpcUa_UInt32      count2;

                count = (OpcUa_UInt16)nameSpaceUris.length();
                // Check first if we have the rigth name space index
                UaString sTempNsUri1(g_pClientSampleConfig->m_sNamespaceUri);
                UaString sTempNsUri2(g_pClientSampleConfig->m_sNamespaceUri2);
                UaString sTemp2;

                // We need to find the right NamespaceIndex
                for ( i=0; i<count; i++ )
                {
                    sTemp2 = &nameSpaceUris[i];
                    if ( sTempNsUri1 == sTemp2 )
                    {
                        g_nsIndex = i;
                    }
                    if ( sTempNsUri2 == sTemp2 )
                    {
                        g_nsIndex2 = i;
                    }
                }

                if ( g_nsIndex == OpcUa_UInt16_Max )
                {
                    printf("** Searching for name space uri failed!\n");
                    return;
                }

                count = g_pClientSampleConfig->m_VariableNodeIds.length();
                count2 = g_pClientSampleConfig->m_VariableNodeIds2.length();
                g_VariableNodeIds.create(count + count2);
                for ( i=0; i<count; i++ )
                {
                    g_VariableNodeIds[i].setNodeId( g_pClientSampleConfig->m_VariableNodeIds[i], g_nsIndex );
                }
                if ( g_nsIndex2 != OpcUa_UInt16_Max )
                {
                    for ( i=(OpcUa_UInt16)count; i<count + count2; i++)
                    {
                        g_VariableNodeIds[i].setNodeId( g_pClientSampleConfig->m_VariableNodeIds2[i-count], g_nsIndex2 );
                    }
                }

                g_ObjectNodeIds.create(1);
                g_MethodNodeIds.create(1);
                bool writeVariable2Set = !g_pClientSampleConfig->m_sWriteVariableNodeId2.isEmpty();
                g_WriteVariableNodeIds.create(writeVariable2Set ? 2 : 1);
                g_WriteVariableNodeIds[0].setNodeId( g_pClientSampleConfig->m_sWriteVariableNodeId, g_nsIndex );
                if (writeVariable2Set)
                {
                    g_WriteVariableNodeIds[1].setNodeId( g_pClientSampleConfig->m_sWriteVariableNodeId2, g_nsIndex2 );
                }
                g_ObjectNodeIds[0].setNodeId( g_pClientSampleConfig->m_sObjectNodeId, g_nsIndex );
                g_MethodNodeIds[0].setNodeId( g_pClientSampleConfig->m_sMethodNodeId, g_nsIndex );
                g_HistoryDataNodeIds.create(1);
                g_HistoryDataNodeIds[0].setNodeId( g_pClientSampleConfig->m_sHistoryDataId, g_nsIndex );
                g_HistoryEventNodeId.setNodeId( g_pClientSampleConfig->m_sHistoryEventNotifierId, g_nsIndex );
                g_EventTriggerObjects.create(2);
                g_EventTriggerObjects[0].setNodeId( g_pClientSampleConfig->m_sEventTriggerObject1, g_nsIndex );
                g_EventTriggerObjects[1].setNodeId( g_pClientSampleConfig->m_sEventTriggerObject2, g_nsIndex );
                g_EventTriggerMethods.create(2);
                g_EventTriggerMethods[0].setNodeId( g_pClientSampleConfig->m_sEventTriggerMethod1, g_nsIndex );
                g_EventTriggerMethods[1].setNodeId( g_pClientSampleConfig->m_sEventTriggerMethod2, g_nsIndex );
            }
        }
        else
        {
            printf("** Variable OpcUaId_Server_NamespaceArray failed!\n");
        }
    }
}

/*============================================================================
 * startDiscovery - Find available Endpoints
 *===========================================================================*/
void startDiscovery(const UaString& sDiscoveryUrl, SessionSecurityInfo& sessionSecurityInfo, UaString &sUrl)
{
    UaStatus                    status;
    UaString                    sTempUrl;
    UaApplicationDescriptions   applicationDescriptions;
    UaObjectArray<UaString>     arrayUrls;
    UaObjectArray<UaByteString> arrayServerCertificates;
    UaObjectArray<UaString>     arraySecurityPolicies;
    UaUInt32Array               arraySecurityMode;
    UaEndpointDescriptions      endpointDescriptions;
    OpcUa_UInt32                endpointCount = 0;
    UaString                    sTemp;
    ServiceSettings             serviceSettings;

    arrayUrls.create(10);
    arrayServerCertificates.create(10);
    arraySecurityPolicies.create(10);
    arraySecurityMode.create(10);

    /******************************************************************************/
    // Begin Discovery
    printf("\n\n");
    printf("****************************************************************\n");
    printf("** Call findServers and getEndpoints for each Server\n");
    UaDiscovery *             pUaDiscovery = new UaDiscovery();

    /*********************************************************************
     Find available servers
    **********************************************************************/
    status = pUaDiscovery->findServers(
        serviceSettings,         // Use default settings
        sDiscoveryUrl,           // Discovery Server Url
        sessionSecurityInfo,     // Use general settings for client
        applicationDescriptions);
    /*********************************************************************/

    if ( status.isBad() )
    {
        printf("** Error: UaDiscovery::findServers failed [ret=0x%x] *********\n", status.statusCode());
        return;
    }

    for ( OpcUa_UInt32 i=0; i<applicationDescriptions.length(); i++ )
    {
        for ( OpcUa_Int32 j=0; j<applicationDescriptions[i].NoOfDiscoveryUrls; j++ )
        {
            sTempUrl = &applicationDescriptions[i].DiscoveryUrls[j];
            const char* psUrl = sTempUrl.toUtf8();
            if ( sTempUrl.length() > 10 && psUrl[0] == 'o' && psUrl[1] == 'p' && psUrl[2] == 'c' && psUrl[3] == '.' &&
                 psUrl[4] == 't' && psUrl[5] == 'c' && psUrl[6] == 'p' )
            {
                /*********************************************************************
                 Get Endpoints from Server
                **********************************************************************/
                status = pUaDiscovery->getEndpoints(
                    serviceSettings,        // Use default settings
                    sTempUrl,               // Discovery URL of the found Server
                    sessionSecurityInfo,    // Use general settings for client
                    endpointDescriptions);  // Return
                /*********************************************************************/
                if ( status.isBad() )
                {
                    printf("** Error: UaDiscovery::getEndpoints failed [ret=0x%x] *********\n", status.statusCode());
                    return;
                }
                for ( OpcUa_UInt32 k=0; k<endpointDescriptions.length(); k++ )
                {
                    sTempUrl = &endpointDescriptions[k].EndpointUrl;
                    psUrl = sTempUrl.toUtf8();
                    if ( sTempUrl.length() > 10 && psUrl[0] == 'o' && psUrl[1] == 'p' && psUrl[2] == 'c' && psUrl[3] == '.' &&
                         psUrl[4] == 't' && psUrl[5] == 'c' && psUrl[6] == 'p' )
                    {
                        if ( endpointCount >= 10 )
                        {
                            // We can select only 10 endpoints
                            break;
                        }

                        // Store settings necessary to connect for selection
                        arrayUrls[endpointCount] = &endpointDescriptions[k].EndpointUrl;
                        arrayServerCertificates[endpointCount] = endpointDescriptions[k].ServerCertificate;
                        arraySecurityPolicies[endpointCount] = &endpointDescriptions[k].SecurityPolicyUri;
                        arraySecurityMode[endpointCount] = endpointDescriptions[k].SecurityMode;

                        printf("** Endpoint[%d] *************************************************\n", endpointCount);
                        sTemp = &applicationDescriptions[i].ApplicationUri;
                        printf(" Server Uri       %s\n", sTemp.toUtf8());
                        sTemp = &applicationDescriptions[i].ApplicationName.Text;
                        printf(" Server Name      %s\n", sTemp.toUtf8());
                        sTemp = &endpointDescriptions[k].EndpointUrl;
                        printf(" Endpoint URL     %s\n", sTemp.toUtf8());
                        sTemp = &endpointDescriptions[k].SecurityPolicyUri;
                        printf(" Security Policy  %s\n", sTemp.toUtf8());
                        sTemp = "Invalid";
                        if ( endpointDescriptions[k].SecurityMode == OpcUa_MessageSecurityMode_None )
                        {
                            sTemp = "None";
                        }
                        if ( endpointDescriptions[k].SecurityMode == OpcUa_MessageSecurityMode_Sign )
                        {
                            sTemp = "Sign";
                        }
                        if ( endpointDescriptions[k].SecurityMode == OpcUa_MessageSecurityMode_SignAndEncrypt )
                        {
                            sTemp = "SignAndEncrypt";
                        }
                        printf(" Security Mode    %s\n", sTemp.toUtf8());
                        printf("****************************************************************\n");
                        endpointCount++;
                    }
                }
            }
        }
    }
    delete pUaDiscovery;
    // END Discovery
    /******************************************************************************/

    printMenu(10);

    /******************************************************************************/
    /* Wait for user command to execute next action.                              */
    int action;
    while (!WaitForKeypress(action))
    {
        if ( action >= 0 && action < (int)endpointCount )
        {
            // Copy URL and security settings from selected server
            sUrl = arrayUrls[action];
            sessionSecurityInfo.serverCertificate = arrayServerCertificates[action];
            sessionSecurityInfo.sSecurityPolicy = arraySecurityPolicies[action];
            sessionSecurityInfo.messageSecurityMode = (OpcUa_MessageSecurityMode)arraySecurityMode[action];

#if OPCUA_SUPPORT_PKI
            // Verifiy if the certificate is already trusted
            if ( sessionSecurityInfo.verifyServerCertificate().isBad() )
            {
                printf("\n");
                printf("\n");
                printf("\n");
                printf("-------------------------------------------------------\n");
                printf("- The following certificate is not trusted yet        -\n");
                printf("-------------------------------------------------------\n");
                g_pClientSampleConfig->printCertificateData(arrayServerCertificates[action]);

                // Ask the user if he wants to trust the certificate
                printMenu(100);
                while (!WaitForKeypress(action))
                {
                    if ( action == 1 )
                    {
                        // Store server certificate in trust list
                        // The resulting name should be stored together with the configuration information for the server connection
                        UaString certificateName;
                        sessionSecurityInfo.saveServerCertificate(certificateName);
                        printf("\n");
                        printf("------------------------------------------------------------\n");
                        printf("- Stored server certificate in the client trust list       -\n");
                        printf("- %s\n", g_pClientSampleConfig->sCertificateTrustListLocation().toUtf8());
                        printf("------------------------------------------------------------\n");
                        printf("- Make sure the client certificate is in server trust list -\n");
                        printf("------------------------------------------------------------\n");
                        break;
                    }
                    else if ( action == 2 )
                    {
                        // Accept temporarily - so we tell the SDK to skip the certificate check
                        sessionSecurityInfo.doServerCertificateVerify = OpcUa_False;
                        break;
                    }
                    else if (action == 3)
                    {
                        // Certificate rejected - reset the flag on sessionSecurityInfo
                        sessionSecurityInfo.doServerCertificateVerify = OpcUa_True;
                        break;
                    }
                    UaThread::msleep(100);
                }
            }
#endif // OPCUA_SUPPORT_PKI

            break;
        }
        UaThread::msleep(100);
    }
}

/*============================================================================
 * gdsInteraction - Update TrustList, Sign certificate, create new key pair
 *===========================================================================*/

UaStatus findEndointWithBestSecurity(const UaString &sDiscoveryUrl, SessionSecurityInfo &sessionSecurityInfo)
{
#if (OPCUA_SUPPORT_SECURITYPOLICY_BASIC128RSA15 == OPCUA_CONFIG_OFF) && (OPCUA_SUPPORT_SECURITYPOLICY_BASIC256 == OPCUA_CONFIG_OFF) && (OPCUA_SUPPORT_SECURITYPOLICY_BASIC256SHA256 == OPCUA_CONFIG_OFF) && (OPCUA_SUPPORT_SECURITYPOLICY_AES128SHA256RSAOAEP == OPCUA_CONFIG_OFF) && (OPCUA_SUPPORT_SECURITYPOLICY_AES256SHA256RSAPSS == OPCUA_CONFIG_OFF)
    OpcUa_ReferenceParameter(sDiscoveryUrl);
    OpcUa_ReferenceParameter(sessionSecurityInfo);
    printf("** All security profiles are turned off.\n");
    printf("***********************************************************************************\n");
    return OpcUa_Bad;
#else // (OPCUA_SUPPORT_SECURITYPOLICY_BASIC128RSA15 == OPCUA_CONFIG_OFF) && (OPCUA_SUPPORT_SECURITYPOLICY_BASIC256 == OPCUA_CONFIG_OFF) && (OPCUA_SUPPORT_SECURITYPOLICY_BASIC256SHA256 == OPCUA_CONFIG_OFF)

    UaStatus ret;
    UaDiscovery discovery;
    ServiceSettings serviceSettings;
    UaEndpointDescriptions endpoints;
    ret = discovery.getEndpoints(serviceSettings, sDiscoveryUrl, sessionSecurityInfo, endpoints);
    if (ret.isBad())
    {
        printf("** Error: UaDiscovery::getEndpoints at GDS failed [ret=%s]\n", ret.toString().toUtf8());
        printf("****************************************************************\n");
        return ret;
    }

    OpcUa_Byte selectedsecurityLevel = 0;
    OpcUa_UInt32 selectedEndpoint = 0;
    for (OpcUa_UInt32 u=0; u<endpoints.length(); u++)
    {
        const OpcUa_EndpointDescription &endpoint = endpoints[u];
        if (endpoint.SecurityLevel > selectedsecurityLevel)
        {
            UaString securityPolicy = endpoint.SecurityPolicyUri;
            switch (endpoint.SecurityMode)
            {
            case OpcUa_MessageSecurityMode_Sign:
            case OpcUa_MessageSecurityMode_SignAndEncrypt:
                // check for known security policies
                if ( false
#if OPCUA_SUPPORT_SECURITYPOLICY_BASIC128RSA15
                        || securityPolicy == OpcUa_SecurityPolicy_Basic128Rsa15
#endif
#if OPCUA_SUPPORT_SECURITYPOLICY_BASIC256
                        || securityPolicy == OpcUa_SecurityPolicy_Basic256
#endif
#if OPCUA_SUPPORT_SECURITYPOLICY_BASIC256SHA256
                        || securityPolicy == OpcUa_SecurityPolicy_Basic256Sha256
#endif
#if OPCUA_SUPPORT_SECURITYPOLICY_AES128SHA256RSAOAEP
                        || securityPolicy == OpcUa_SecurityPolicy_Aes128Sha256RsaOaep
#endif
#if OPCUA_SUPPORT_SECURITYPOLICY_AES256SHA256RSAPSS
                        || securityPolicy == OpcUa_SecurityPolicy_Aes256Sha256RsaPss
#endif
                     )
                {
                    selectedEndpoint = u;
                    selectedsecurityLevel = endpoint.SecurityLevel;
                }
                break;
            default:
                // ignore unknown and invalid security modes
                break;
            }
        }
    }
    if (selectedsecurityLevel > 0)
    {
        sessionSecurityInfo.messageSecurityMode = endpoints[selectedEndpoint].SecurityMode;
        // attach the SecurityPolicyUri and ServerCertificate.
        sessionSecurityInfo.sSecurityPolicy.attach(&endpoints[selectedEndpoint].SecurityPolicyUri);
        OpcUa_String_Initialize(&endpoints[selectedEndpoint].SecurityPolicyUri);
        sessionSecurityInfo.serverCertificate.attach(&endpoints[selectedEndpoint].ServerCertificate);
        OpcUa_ByteString_Initialize(&endpoints[selectedEndpoint].ServerCertificate);
    }
    else
    {
        printf("** Error: could not find a secure endoint");
        printf("****************************************************************\n");
        ret = OpcUa_Bad;
    }

    return ret;
#endif // (OPCUA_SUPPORT_SECURITYPOLICY_BASIC128RSA15 == OPCUA_CONFIG_OFF) && (OPCUA_SUPPORT_SECURITYPOLICY_BASIC256 == OPCUA_CONFIG_OFF) && (OPCUA_SUPPORT_SECURITYPOLICY_BASIC256SHA256 == OPCUA_CONFIG_OFF)
}

UaString stringFromCommandLine(const char *szText)
{
    printf("%s", szText);
    char szRet[100];
#ifndef WIN32
    close_keyboard();
#endif
    std::cin >> szRet;
#ifndef WIN32
    init_keyboard();
#endif
    return szRet;
}

UaString passwordFromCommandLine(const char *szText)
{
#ifndef _WIN32_WCE
    printf("%s", szText);
    char c = (char) getch();
    int pos = 0;
    char szPassword[100];
    while (c != 13 && c != 10 && pos < 99)
    {
        if (c == 8)
        {
            if (pos > 0)
            {
                pos--;
            }
        }
        else
        {
            printf("*");
            szPassword[pos++] = c;
        }
        c = (char) getch();
    }
    szPassword[pos] = 0;
    return szPassword;
#else
    return stringFromCommandLine(szText);
#endif
}

bool registerApplication(UaCertificateDirectoryObject &gds, ServiceSettings &serviceSettings)
{
    UaStatus status;
    UaLocalizedTextArray appNames;
    appNames.create(1);
    g_pClientSampleConfig->sApplicationName().copyTo(&appNames[0].Text);
    UaNodeId applicationId;

    // Setup client application record
    OpcUaGds::ApplicationRecordDataType application;
    application.setApplicationUri(g_pClientSampleConfig->sApplicationUri());
    application.setApplicationType(OpcUa_ApplicationType_Client);
    application.setApplicationNames(appNames);
    application.setProductUri(g_pClientSampleConfig->sProductUri());

    // Register client with GDS
    status = gds.registerApplication(serviceSettings, application, applicationId);
    if ( status.isBad() )
    {
        printf("** Error: CertificateDirectory::registerApplication failed [ret=%s]\n", status.toString().toUtf8());
        printf("****************************************************************\n");
        return false;
    }

    printf("** Registration succeeded ApplicationId=%s\n", applicationId.toXmlString().toUtf8());

    // Store application ID
    g_pClientSampleConfig->setGdsApplicationObjectId(applicationId.toXmlString());

    return true;
}

#if OPCUA_SUPPORT_PKI
void saveCertificate(const UaPkiCertificate &certificate)
{
    if (!g_pClientSampleConfig->useWindowsStore())
    {
        certificate.toDERFile(g_pClientSampleConfig->sClientCertificateFilePath());
    }
    else
    {
#if OPCUA_SUPPORT_PKI_WIN32
        certificate.toWindowsStore(
                    g_pClientSampleConfig->windowsStoreLocation(),
                    g_pClientSampleConfig->sWindowsStoreName());
        g_pClientSampleConfig->setWindowsClientCertificateThumbprint(UaString(certificate.thumbPrint().toHex()));
#endif
    }
}

void saveCertificates(const UaPkiCertificate &certificate, const UaPkiPrivateKey &privateKey)
{
    UaPkiRsaKeyPair keyPair((EVP_PKEY*)(const EVP_PKEY*)privateKey);
    if (!g_pClientSampleConfig->useWindowsStore())
    {
        certificate.toDERFile(g_pClientSampleConfig->sClientCertificateFilePath());
        keyPair.toPEMFile(g_pClientSampleConfig->sClientPrivateKeyFilePath(), 0 );
    }
    else
    {
#if OPCUA_SUPPORT_PKI_WIN32
        certificate.toWindowsStoreWithPrivateKey(
                    g_pClientSampleConfig->windowsStoreLocation(),
                    g_pClientSampleConfig->sWindowsStoreName(),
                    keyPair);
        g_pClientSampleConfig->setWindowsClientCertificateThumbprint(UaString(certificate.thumbPrint().toHex()));
#endif
    }
}

void saveIssuerCertificates(const UaByteStringArray &certificateDatas)
{
    for (OpcUa_UInt32 u=0; u<certificateDatas.length(); u++)
    {
        const UaByteString &certificateData = certificateDatas[u];
        UaPkiCertificate issuerCertificate = UaPkiCertificate::fromDER(certificateData);
        if (!g_pClientSampleConfig->useWindowsStore())
        {
            UaDir dirHelper("");
            UaUniString usClientCertificatePath(dirHelper.filePath(UaDir::fromNativeSeparators(g_pClientSampleConfig->sIssuersCertificatesLocation().toUtf16())));
            dirHelper.mkpath(usClientCertificatePath);
            issuerCertificate.toDERFile(UaString("%1/%2 [%3].der")
                                     .arg(g_pClientSampleConfig->sIssuersCertificatesLocation())
                                     .arg(issuerCertificate.commonName())
                                     .arg(issuerCertificate.thumbPrint().toHex()));
        }
        else
        {
    #if OPCUA_SUPPORT_PKI_WIN32
            // ToDo
    #endif
        }
    }
}
#endif // OPCUA_SUPPORT_PKI

void gdsInteraction(SessionSecurityInfo &sessionSecurityInfo)
{
    printf("\n****************************************************************\n");
    printf("** Start interaction with GDS server\n");

#if OPCUA_SUPPORT_PKI
    UaString sGdsServerUrl = g_pClientSampleConfig->sGdsServerUrl();

    if (sGdsServerUrl.size() < 10 )
    {
        sGdsServerUrl = stringFromCommandLine("** Enter GDS Server Endpoint URL.\n");
        g_pClientSampleConfig->setGdsServerUrl(sGdsServerUrl);
    }

    UaSession gdsSession;
    UaStatus  status;
    UaNodeId  nullNodeId;

    SessionConnectInfo sessionConnectInfo;
    sessionConnectInfo.sApplicationName = g_pClientSampleConfig->sApplicationName();
    sessionConnectInfo.sApplicationUri  = g_pClientSampleConfig->sApplicationUri();
    sessionConnectInfo.sProductUri      = g_pClientSampleConfig->sProductUri();
    sessionConnectInfo.sSessionName     = UaString("Client_Cpp_SDK@%1").arg(g_pClientSampleConfig->sHostName());
    sessionConnectInfo.bAutomaticReconnect = OpcUa_True;
    sessionConnectInfo.clientConnectionId = 2;

    SessionSecurityInfo gdsSessionSecurityInfo;
    g_pClientSampleConfig->setupSecurity(gdsSessionSecurityInfo);
    gdsSessionSecurityInfo.doServerCertificateVerify = OpcUa_False;

    UaString sGdsUser = g_pClientSampleConfig->sGdsUser();
    UaString sGdsPassword = g_pClientSampleConfig->sGdsPassword();
    bool isApplicationAuthentication = true;
    if (sGdsUser.isEmpty())
    {
        sGdsUser = stringFromCommandLine("** Enter GDS user name for initial registration\n   Enter x to use application authentication for updates.\n   Enter u to change back to self-signed certificate.\n");
        if (sGdsUser == "x")
        {
            sGdsUser = "";
        }
        else if (sGdsUser == "u")
        {
            // Delete application id and create self-signed certificate
            g_pClientSampleConfig->setGdsApplicationObjectId("");
            printf("** Create new self signed certificate\n");
            g_pClientSampleConfig->createCertificates(true);
            printf("** Created new self signed certificate\n");
            printf("** New certificate will be used for new connection.\n");
            printf("****************************************************************\n");
            return;
        }
    }
    if (!sGdsUser.isEmpty() && sGdsPassword.isEmpty())
    {
        sGdsPassword = passwordFromCommandLine("** Please enter the password for GDS user.\n");
    }
    if (!sGdsUser.isEmpty())
    {
        isApplicationAuthentication = false;
        gdsSessionSecurityInfo.setUserPasswordUserIdentity(sGdsUser, sGdsPassword);
    }

    status = findEndointWithBestSecurity(sGdsServerUrl, gdsSessionSecurityInfo);
    if (status.isBad())
    {
        printf("** Could not find a secure endpoint at %s. Connecting without security.)\n", sGdsServerUrl.toUtf8());
        printf("****************************************************************\n");
    }

    status = gdsSession.connect(
        sGdsServerUrl,  // URL of the GDS Endpoint
        sessionConnectInfo,  // General settings for connection
        gdsSessionSecurityInfo, // Security settings
        g_pCallback);        // Callback interface
    if ( status.isBad() )
    {
        printf("** Error: UaSession::connect to GDS failed [ret=%s]\n", status.toString().toUtf8());
        printf("****************************************************************\n");

        if (g_pClientSampleConfig->sGdsApplicationObjectId().length() > 2)
        {
            printMenu(40);

            /******************************************************************************/
            /* Wait for user command to execute next action.                              */
            int action;
            while (!WaitForKeypress(action))
            {
                if (action == 1)
                {
                    // Delete application ID
                    g_pClientSampleConfig->setGdsApplicationObjectId("");

                    printf("** Create new self signed certificate\n");

                    g_pClientSampleConfig->createCertificates(true);

                    printf("** Created new self signed certificate\n");
                    break;
                }
            }
        }
        return;
    }

    UaCertificateDirectoryObject gds(&gdsSession);
    ServiceSettings serviceSettings;

    printf("** Connected to GDS server %s\n", sGdsServerUrl.toUtf8());
    // Check if the application is already registered
    if ( g_pClientSampleConfig->sGdsApplicationObjectId().size() < 2 )
    {
        printf("** Application not registered, try to register\n");

        bool registerSucceeded = registerApplication(gds, serviceSettings);
        if (!registerSucceeded)
        {
            return;
        }
    }
    else
    {
        printf("** Check if the Application registration is valid\n");

        UaNodeId applicationId(UaNodeId::fromXmlString(g_pClientSampleConfig->sGdsApplicationObjectId()));
        OpcUaGds::ApplicationRecordDataType application;

        // Check client registration with GDS
        status = gds.getApplication(serviceSettings, applicationId, application);
        if ( status.isBad() )
        {
            printf("** Error: CertificateDirectory::getApplication failed [ret=%s]\n", status.toString().toUtf8());
            printf("****************************************************************\n");
            printf("** Try to register\n");
            bool registerSucceeded = registerApplication(gds, serviceSettings);
            if (!registerSucceeded)
            {
                return;
            }
        }
        else
        {
            printf("****************************************************************\n");
        }
    }

    printMenu(20);

    /******************************************************************************/
    /* Wait for user command to execute next action.                              */
    int action;
    while (!WaitForKeypress(action))
    {
        if ( action == 1 )
        {
            printf("\n****************************************************************\n");
            printf("** Update trust list - first get trustlistId from GDS\n");
            UaNodeId applicationId(UaNodeId::fromXmlString(g_pClientSampleConfig->sGdsApplicationObjectId()));
            UaNodeId trustListId;

            // Get trustlistId from GDS
            status = gds.getTrustList(serviceSettings, applicationId, 0, trustListId);
            if ( status.isBad() )
            {
                printf("** Error: CertificateDirectory::getTrustList failed [ret=%s]\n", status.toString().toUtf8());
                printf("****************************************************************\n");
                return;
            }

            printf("** Load trust list from GDS\n");
            UaTrustListObject trustList(&gdsSession);
            UaTrustListDataType trustListData;
            status = trustList.readTrustList(serviceSettings, trustListId, (OpcUa_UInt32) OpcUa_TrustListMasks_All, trustListData);
            if ( status.isBad() )
            {
                printf("** Error: TrustList::readTrustList failed [ret=%s]\n", status.toString().toUtf8());
                printf("****************************************************************\n");
                return;
            }

            printf("** Store trust list on disc\n");
            status = trustList.saveTrustListAsFiles(
                g_pClientSampleConfig->sCertificateRevocationListLocation(),
                g_pClientSampleConfig->sCertificateTrustListLocation(),
                g_pClientSampleConfig->sIssuersRevocationListLocation(),
                g_pClientSampleConfig->sIssuersCertificatesLocation(),
                trustListData);
            if ( status.isBad() )
            {
                printf("** Error: TrustList::readTrustList failed [ret=%s]\n", status.toString().toUtf8());
                printf("****************************************************************\n");
                return;
            }
            printf("** Finished update of trust list from GDS\n");

            // Check certificate with GDS
            printf("**\n** Check certificate status\n");
            OpcUa_Boolean isUpdateRequired = OpcUa_False;
            status = gds.getCertificateStatus(serviceSettings, applicationId, nullNodeId, nullNodeId, isUpdateRequired);
            if (status.isBad())
            {
                printf("** Error: CertificateDirectory::getCertificateStatus failed [ret=%s]\n", status.toString().toUtf8());
                printf("****************************************************************\n");
                return;
            }
            if (isUpdateRequired == OpcUa_False)
            {
                printf("** No certificate update required\n");
            }
            else
            {
                printf("** Certificate update required, run GDS interaction again and choose 2 or 3\n");
            }
            printf("****************************************************************\n\n");

            break;
        }
        else if ( action == 2 )
        {
            printf("\n****************************************************************\n");
            printf("** Get signed certificate and trust list from GDS\n");

            UaNodeId applicationId(UaNodeId::fromXmlString(g_pClientSampleConfig->sGdsApplicationObjectId()));
            UaNodeId requestId;
            UaNodeId trustListId;

            // create certificate request
            UaPkiCertificate certificate = UaPkiCertificate::fromDER(gdsSessionSecurityInfo.clientCertificate);
            UaPkiPrivateKey privateKey = UaPkiPrivateKey::fromDER(gdsSessionSecurityInfo.getClientPrivateKeyDer(), UaPkiKeyType_RSA);
            UaPkiCSR csr = certificate.createCSR(privateKey, UaPkiCertificate::SignatureAlgorithm_Sha256);
            UaByteArray csrData = csr.toDER();
            OpcUa_ByteString byteString;
            OpcUa_ByteString_Initialize(&byteString);
            byteString.Length = csrData.size();
            byteString.Data = csrData.detach();
            UaByteString bsCsr;
            bsCsr.attach(&byteString);
            OpcUa_ByteString_Initialize(&byteString);

            printf("** Send certificate signing request to GDS\n");

            status = gds.startSigningRequest(
                        serviceSettings,
                        applicationId,
                        nullNodeId,
                        nullNodeId,
                        bsCsr,
                        requestId);
            if ( status.isBad() )
            {
                printf("** Error: CertificateDirectory::startSigningRequest failed [ret=%s]\n", status.toString().toUtf8());
                printf("****************************************************************\n");
                return;
            }

            printf("** Request succeeded check request status\n");

            do
            {
                UaByteString bsCertificate;
                UaByteString bsPrivateKey;
                UaByteStringArray issuerCertificates;
                // Request new key pair from GDS
                status = gds.finishRequest(serviceSettings, applicationId, requestId, bsCertificate, bsPrivateKey, issuerCertificates);
                if ( status == OpcUa_BadNothingToDo )
                {
                    printf("** Request not completet\n");
                    status = OpcUa_Bad;

                    printMenu(30);

                    /******************************************************************************/
                    /* Wait for user command to execute next action.                              */
                    while (!WaitForKeypress(action))
                    {
                        if ( action == 1 )
                        {
                            status = OpcUa_BadNothingToDo;
                            break;
                        }
                        UaThread::msleep(100);
                    }
                }
                else if ( status.isBad() )
                {
                    printf("** Error: CertificateDirectory::finishRequest failed [ret=%s]\n", status.toString().toUtf8());
                    printf("****************************************************************\n");
                    return;
                }
                else
                {
                    // Get trustlistId from GDS
                    status = gds.getTrustList(serviceSettings, applicationId, 0, trustListId);
                    if (status.isBad())
                    {
                        printf("** Error: CertificateDirectory::getTrustList failed [ret=%s]\n", status.toString().toUtf8());
                        if (isApplicationAuthentication)
                        {
                            printf("** We need a new connection to update trust list with new application identity\n");
                            printf("   Run GDS interaction again and choose 1 for trust list update\n\n");
                        }
                        else
                        {
                            printf("****************************************************************\n");
                            return;
                        }
                    }
                    else
                    {
                        printf("** Load trust list from GDS\n");
                        UaTrustListObject trustList(&gdsSession);
                        UaTrustListDataType trustListData;
                        status = trustList.readTrustList(serviceSettings, trustListId, (OpcUa_UInt32)OpcUa_TrustListMasks_All, trustListData);
                        if (status.isBad())
                        {
                            printf("** Error: TrustList::readTrustList failed [ret=%s]\n", status.toString().toUtf8());
                            printf("****************************************************************\n");
                            return;
                        }

                        printf("** Store trust list on disc\n");
                        status = trustList.saveTrustListAsFiles(
                            g_pClientSampleConfig->sCertificateRevocationListLocation(),
                            g_pClientSampleConfig->sCertificateTrustListLocation(),
                            g_pClientSampleConfig->sIssuersRevocationListLocation(),
                            g_pClientSampleConfig->sIssuersCertificatesLocation(),
                            trustListData);
                        if (status.isBad())
                        {
                            printf("** Error: TrustList::readTrustList failed [ret=%s]\n", status.toString().toUtf8());
                            printf("****************************************************************\n");
                            return;
                        }
                    }

                    printf("** Store new certificate on disc\n");
                    UaPkiCertificate newCertificate = UaPkiCertificate::fromDER(bsCertificate);
                    // save certificate of GDS as issuer certificate
                    saveCertificate(newCertificate);
                    saveIssuerCertificates(issuerCertificates);

                    // Load new certificate
                    sessionSecurityInfo.loadClientCertificateOpenSSL(
                        g_pClientSampleConfig->sClientCertificateFilePath(),
                        g_pClientSampleConfig->sClientPrivateKeyFilePath());


                    if (g_pUaSession && g_pUaSession->isConnected() != OpcUa_False)
                    {
                        g_pUaSession->changeClientCertificate(sessionSecurityInfo);

                        printf("** Client certificate and trust list updated. New certificate set on active connection.\n");
                        printf("****************************************************************\n");
                    }
                    else
                    {
                        printf("** Client certificate and trust list updated. New certificate will be used for new connection.\n");
                        printf("****************************************************************\n");
                    }
                }
            } while ( status == OpcUa_BadNothingToDo );

            break;
        }
        else if ( action == 3 )
        {
            printf("\n****************************************************************\n");
            printf("** Get signed key pair and trust list from GDS\n");
            UaNodeId applicationId(UaNodeId::fromXmlString(g_pClientSampleConfig->sGdsApplicationObjectId()));
            UaStringArray domainNames;
            domainNames.create(1);
            g_pClientSampleConfig->sHostName().copyTo(&domainNames[0]);
            UaNodeId requestId;
            UaNodeId trustListId;

            printf("** Request new public and private key from GDS\n");

            // Request new key pair from GDS
            status = gds.startNewKeyPairRequest(serviceSettings, applicationId, nullNodeId, nullNodeId, "", domainNames, "PEM", "", requestId);
            if ( status.isBad() )
            {
                printf("** Error: CertificateDirectory::startNewKeyPairRequest failed [ret=%s]\n", status.toString().toUtf8());
                printf("****************************************************************\n");
                return;
            }

            printf("** Request succeeded check request status\n");

            do
            {
                UaByteString bsCertificate;
                UaByteString bsPrivateKey;
                UaByteStringArray issuerCertificates;
                // Request new key pair from GDS
                status = gds.finishRequest(serviceSettings, applicationId, requestId, bsCertificate, bsPrivateKey, issuerCertificates);
                if ( status == OpcUa_BadNothingToDo )
                {
                    printf("** Request not completet\n");
                    status = OpcUa_Bad;

                    printMenu(30);

                    /******************************************************************************/
                    /* Wait for user command to execute next action.                              */
                    while (!WaitForKeypress(action))
                    {
                        if ( action == 1 )
                        {
                            status = OpcUa_BadNothingToDo;
                            break;
                        }
                        UaThread::msleep(100);
                    }
                }
                else if ( status.isBad() )
                {
                    printf("** Error: CertificateDirectory::finishRequest failed [ret=%s]\n", status.toString().toUtf8());
                    printf("****************************************************************\n");
                    return;
                }
                else
                {
                    // Get trustlistId from GDS
                    status = gds.getTrustList(serviceSettings, applicationId, 0, trustListId);
                    if (status.isBad())
                    {
                        printf("** Error: CertificateDirectory::getTrustList failed [ret=%s]\n", status.toString().toUtf8());
                        if (isApplicationAuthentication)
                        {
                            printf("** We need a new connection to update trust list with new application identity\n");
                            printf("   Run GDS interaction again and choose 1 for trust list update\n\n");
                        }
                        else
                        {
                            printf("****************************************************************\n");
                            return;
                        }
                    }
                    else
                    {
                        printf("** Load trust list from GDS\n");
                        UaTrustListObject trustList(&gdsSession);
                        UaTrustListDataType trustListData;
                        status = trustList.readTrustList(serviceSettings, trustListId, (OpcUa_UInt32)OpcUa_TrustListMasks_All, trustListData);
                        if (status.isBad())
                        {
                            printf("** Error: TrustList::readTrustList failed [ret=%s]\n", status.toString().toUtf8());
                            printf("****************************************************************\n");
                            return;
                        }

                        printf("** Store trust list on disc\n");
                        status = trustList.saveTrustListAsFiles(
                            g_pClientSampleConfig->sCertificateRevocationListLocation(),
                            g_pClientSampleConfig->sCertificateTrustListLocation(),
                            g_pClientSampleConfig->sIssuersRevocationListLocation(),
                            g_pClientSampleConfig->sIssuersCertificatesLocation(),
                            trustListData);
                        if (status.isBad())
                        {
                            printf("** Error: TrustList::readTrustList failed [ret=%s]\n", status.toString().toUtf8());
                            printf("****************************************************************\n");
                            return;
                        }
                    }

                    printf("** Store new key pair on disc\n");

                    // store cert
                    UaPkiCertificate certificate = UaPkiCertificate::fromDER(bsCertificate);
                    UaPkiPrivateKey privateKey = UaPkiPrivateKey::fromPEM(bsPrivateKey, 0);
                    if (certificate.isValid() && privateKey.getErrors().size() == 0)
                    {
                        // save certificate of GDS as issuer certificate
                        saveCertificates(certificate, privateKey);
                        saveIssuerCertificates(issuerCertificates);

                        // Load new certificate
                        sessionSecurityInfo.loadClientCertificateOpenSSL(
                            g_pClientSampleConfig->sClientCertificateFilePath(),
                            g_pClientSampleConfig->sClientPrivateKeyFilePath());

                        if (g_pUaSession && g_pUaSession->isConnected() != OpcUa_False)
                        {
                            g_pUaSession->changeClientCertificate(sessionSecurityInfo);

                            printf("** Client certificate updated. New certificate set on active connection.\n");
                            printf("****************************************************************\n");
                        }
                        else
                        {
                            printf("** Client certificate updated. New certificate will be used for new connection.\n");
                            printf("****************************************************************\n");
                        }
                    }
                }
            } while ( status == OpcUa_BadNothingToDo );

            break;
        }
        else if (action == 4)
        {
            printf("\n****************************************************************\n");
            printf("** Unregister application from GDS\n");

            UaNodeId applicationId(UaNodeId::fromXmlString(g_pClientSampleConfig->sGdsApplicationObjectId()));

            status = gds.unregisterApplication(serviceSettings, applicationId);
            if (status.isBad())
            {
                printf("** Error: CertificateDirectory::unregisterApplication failed [ret=%s]\n", status.toString().toUtf8());
            }
            else
            {
                printf("** Unregister succeeded\n");
            }

            // Delete application ID
            g_pClientSampleConfig->setGdsApplicationObjectId("");

            printf("** Create new self signed certificate\n");

            g_pClientSampleConfig->createCertificates(true);

            printf("** Created new self signed certificate\n");
            break;
        }
        UaThread::msleep(100);
    }
#else // OPCUA_SUPPORT_PKI
    OpcUa_ReferenceParameter(sessionSecurityInfo);
    printf("** Error: no PKI library available\n");
    printf("**************************************************************************************\n");
#endif // OPCUA_SUPPORT_PKI
}

/*============================================================================
 * printMenu - Print the available actions
 *===========================================================================*/
void printMenu(int level)
{
    if ( level == 0 )
    {
        printf("\n");
        printf("\n");
        printf("\n");
        printf("-------------------------------------------------------\n");
        printf("- Press x to close client                             -\n");
        printf("-------------------------------------------------------\n");
        printf("- Press 0 to start discovery                          -\n");
        printf("- Press 1 to connect to server                        -\n");
        printf("- Press 2 to disconnect from server                   -\n");
        printf("- Press 3 to browse server                            -\n");
        printf("- Press 4 to read values  (press l to read async)     -\n");
        printf("- Press 5 to write values (press m to write async)    -\n");
        printf("- Press 6 to register nodes                           -\n");
        printf("- Press 7 to unregister nodes                         -\n");
        printf("- Press 8 to create a subscription                    -\n");
        printf("- Press 9 to subscribe for alarms                     -\n");
        printf("- Press a to call a method                            -\n");
        printf("- Press b to translate browse path to nodeId          -\n");
        printf("- Press c to transfer subscription                    -\n");
        printf("- Press d to history read raw data                    -\n");
        printf("- Press e to history read processed data              -\n");
        printf("- Press f to history read data at timestamps          -\n");
        printf("- Press g to history update data                      -\n");
        printf("- Press h to history delete data                      -\n");
        printf("- Press i to history read event                       -\n");
        printf("- Press j to history update event                     -\n");
        printf("- Press k to history delete event                     -\n");
        printf("- Press n to run GDS interaction                      -\n");
        printf("- Press o to subscribe durable                        -\n");
        printf("-------------------------------------------------------\n");
    }
    else if ( level == 100 )
    {
        printf("-------------------------------------------------------\n");
        printf("- Press x to return to previous menu                  -\n");
        printf("-------------------------------------------------------\n");
        printf("- Press 1 to add certificate to trust list            -\n");
        printf("- Press 2 to temporarily accept the certificate       -\n");
        printf("- Press 3 to reject the certificate                   -\n");
        printf("-------------------------------------------------------\n");
    }
    else if ( level == 1000 )
    {
        printf("\n");
        printf("\n");
        printf("\n");
        printf("-------------------------------------------------------\n");
        printf("- Press x to close client                             -\n");
        printf("-------------------------------------------------------\n");
    }
    else
    {
        printf("\n");
        printf("\n");
        printf("\n");
        printf("-------------------------------------------------------\n");
        printf("- Press x to return to previous menu                  -\n");
        printf("-------------------------------------------------------\n");
        if ( level == 10 )
        {
            printf("- Press index of endpoint you want to connect to      -\n");
            printf("- The server certificate gets stored to the trust list-\n");
            printf("-------------------------------------------------------\n");
        }
        else if ( level == 20 )
        {
            printf("- Press 1 to update trust list from GDS and check for certificate update\n");
            printf("- Press 2 to get/update signed certificate from GDS and update trust list\n");
            printf("- Press 3 to get/update new key pair from GDS and update trust list\n");
            printf("- Press 4 to unregister application\n");
            printf("-------------------------------------------------------\n");
        }
        else if ( level == 30 )
        {
            printf("- Press 1 to check request status again               -\n");
            printf("-------------------------------------------------------\n");
        }
        else if (level == 40)
        {
            printf("- Press 1 to reset GDS registration                   -\n");
            printf("-------------------------------------------------------\n");
        }
    }
}

/*============================================================================
 * WaitForKeypress
 *===========================================================================*/
bool WaitForKeypress(int& action)
{
    action = -1;

    std::map<int, int> mapActions;
    // map keys 0-9 to values 0-9
    for (int i = 0; i <= 9; i++)
    {
        mapActions['0' + i]         = i;
    }
    // map keys a-o to values 10-24
    for (int i = 0; i <= 'o'-'a'; i++)
    {
        mapActions['a' + i] = i + 10;
        mapActions['A' + i] = i + 10;
    }

#ifndef _WIN32_WCE
    if (kbhit())
    {
        int iChar = getch();
        if (iChar == 'x')
        {
            return true;
        }
        else if (mapActions.find(iChar) != mapActions.end())
        {
            action = mapActions[iChar];
        }
    }
#else
    if (GetAsyncKeyState('X'))
    {
        return true;
    }
    else
    {
        std::map<int, int>::const_iterator it = mapActions.begin();
        while (it != mapActions.end())
        {
            if (GetAsyncKeyState(it->first))
            {
                action = it->second;
                break;
            }
            ++it;
        }
    }
#endif
    return false;
}


