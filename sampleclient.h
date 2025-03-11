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
#ifndef SAMPLECLIENT_H
#define SAMPLECLIENT_H

#include "uabase.h"
#include "uaclientsdk.h"
#include <map>
#include <string.h>
#include <fstream>
#include <vector>
#include <numeric>
#include <sqlite3.h>
#include <clickhouse/client.h>

class SampleSubscription;

using namespace UaClientSdk;

class database
{
public:
    bool rewrite = false;
    virtual void init_db(std::vector<std::string>) = 0;
    virtual int exec(const char*) = 0;
    virtual ~database(){};
    virtual void init_synchro(std::vector<std::string>) = 0;
    virtual void finalize_db() = 0;
    virtual int id(std::string) = 0;
    virtual std::string now() = 0;
};

class sqlite_database : public database
{
public:
    sqlite_database(bool, const char*);
    void init_db(std::vector<std::string>);
    int exec(const char*);
    ~sqlite_database();
    void init_synchro(std::vector<std::string>);
    void finalize_db();
    int id(std::string);
    std::string now() {return std::string("CURRENT_TIMESTAMP");}
private:
    sqlite3 *sq_db;
};

class clickhouse_database : public database
{
public:
    clickhouse_database(bool, const char*);
    void init_db(std::vector<std::string>);
    int exec(const char*);
    ~clickhouse_database();
    void init_synchro(std::vector<std::string>);
    void finalize_db();
    int id(std::string);
    std::string now() {return std::string("now()");}
private:
    clickhouse::Client* ch_db;
};

class SampleClient : public UaSessionCallback
{
    UA_DISABLE_COPY(SampleClient);
public:
    SampleClient(int,int,int,bool,bool,std::string,std::string);
    virtual ~SampleClient();

    // UaSessionCallback implementation ----------------------------------------------------
    virtual void connectionStatusChanged(OpcUa_UInt32 clientConnectionId, UaClient::ServerStatus serverStatus);
    // UaSessionCallback implementation ------------------------------------------------------

    // OPC UA service calls
    UaStatus connect(std::string);
    UaStatus disconnect();
    UaStatus reconnect(int);
    void online_db_init();
    UaStatus read_online();
    UaStatus readHistory(const char*,const char*,int,int,bool);
    UaStatus subscribe();
    UaStatus unsubscribe();
//    UaStatus returnNames();
    UaStatus browseSimple(std::string, std::string, std::string);
    UaStatus browseInternal(const UaNodeId& nodeToBrowse, OpcUa_UInt32 maxReferencesToReturn, std::string  recursive);
    void printBrowseResults(const UaReferenceDescriptions& referenceDescriptions, std::string type_match);


private:
    UaSession*          m_pSession;
    std::string url;
    SampleSubscription* m_pSampleSubscription;
    bool read_bad;
    int delta;
    int mean;
    unsigned short ns;
    std::map<std::string,std::vector<double>> slice_data;
    std::vector<std::string> kks_array;
    std::string kks_string;
    FILE* kks_fstream;
    database* db;
    std::ofstream csv_fstream;
    void init_db();
};



#endif // SAMPLECLIENT_H

