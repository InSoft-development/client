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
#include "uaplatformlayer.h"
#include "sampleclient.h"
#include "uathread.h"
#include <stdlib.h>
#include <getopt.h>
#include <string>
#include <csignal>

UaStatus status_run;
SampleClient* pMyClient;
bool exit_flag = false;
bool online = false;

void signalHandler(int signum)
{

       printf("Interrupt!\n");
       if (online)
           exit_flag = true;
       else {
//           if (status_run.isGood())
//           {
//               //pMyClient->unsubscribe();
//               pMyClient->disconnect(); // deprecated: in destructor
//           }
           if (pMyClient)
           {
               pMyClient->disconnect();
               delete pMyClient;
               pMyClient = NULL;
           }
           UaPlatformLayer::cleanup();
           exit(signum);
       }
}
/*============================================================================
 * main
 *===========================================================================*/
#ifdef _WIN32_WCE
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPWSTR, int)
#else
int main(int argc, char*argv[])
#endif
{

	// register signal SIGINT and signal handler

   signal(SIGINT, signalHandler);
   signal(SIGTERM, signalHandler);

	static struct option long_options[] =
	{
            {"online",0,NULL,'o'},
            {"clickhouse-server",0, NULL,'u'},
            {"file",0, NULL,'f'},
            {"history",0,NULL,'i'},
            {"delta", 0, NULL,'d'},
            {"mean", 0, NULL,'m'},
            {"ns", 0, NULL,'n'},
			{"help",0,NULL,'h'},
            {"begin", 0, NULL,'b'},
            {"end", 0, NULL,'e'},
            {"pause", 0, NULL,'p'},
            {"timeout", 0, NULL,'t'},
            {"read-bounds", 0, NULL,'r'},
            {"no-bounds", 0, NULL,'n'},
            {"rewrite", 0, NULL,'w'},
            {"read-bad", 0, NULL,'x'},
            {"kks", 0, NULL,'k'},
            {"recursive", 0, NULL,'c'},
            {"opc-server",0,NULL,'a'},
            {0, 0, 0, 0}
	};

    uint delta = 1000;
    int mean = 5;
    unsigned short ns = 1;
    std::string begin = "";
    std::string end = "";
    int pause = 50000, timeout = 100;
    bool read_bounds = false;
    bool rewrite = false;
    bool read_bad = false;
    bool kks_mode = false;
    bool history_mode = true;
    std::string kks = "";
    std::string recursive = "false";
    std::string clickhouse = "";
    std::string csv_file = "";
    std::string opc_server = "";
	// loop over all of the options
	int ch;
    while ((ch = getopt_long(argc, argv, "hou:f:d:m:s:b:e:p:t:rnwxk:ic:a:", long_options, NULL)) != -1)
	{
	    // check to see if a single character or long option came through
	    switch (ch)
	    {
	    	 case 'h':
	    		 printf("read data from OPC UA\noptions:\n\
--help(-h) this info\n\
--opc-server (-a) opc server address \n\
--clickhouse-server (-u) clickhouse server ip (table dynamic_data and static_data would be used)\n\
--file (-f) store result in local csv file\n\
--ns(-s) number of space (1 by default)\n\
KKS  MODE:\n\
--kks(-k) <id> kks browse mode. List subobjects from <id> object. \"all\" - from root folder, \"begin\" - \
from begin of object folder. Results would be printed to out - or file, if  (-f) used \n\
--recursive (-c) <type> read tags recursively from all objects subobjects, filtered by type, type \"all\" or \"false\" \
means no filtration. Storing to file, if (-f) \n\
ONLINE:\n\
--online(-o) online mode \n\
--delta(-d) miliseconds between reading from OPC UA, default 1000\n\
--mean(-m) count of averaging: 1 means we don't calculate average and send each slice to DB, \
5 - we calculate 5 slices to one mean and send it to DB. default 5\n\
HISTORY MODE:\n\
--history(-i) history mode (default)\n\
--begin(-b) <timestamp> in YYYY-MM-DDTHH:MM:SS.MMMZ format (e.g. 2021-06-01T00:00:00.000Z\n\
--end(-e) <timestramp>\n\
--pause(-p) <miliseconds> pause between requests. default 5000. After successfull reading of each tag - \
connection would be disconnected, wait 3*pause and reconnecetd. It's not recommended to set pause less, then 10. \
It's not recommended to read more than 1 million records from one tag at one time - \
use more short intervals (one month for example) foe big tags\n\
--timeout(-t) <ms> maximum timeout, that we are waiting for response from server\n\
--read-bounds(-r) if we need to read bounds\n\
--no-bounds(-n) if we don\'t want read bounds (default)\n\
--rewrite(-w) rewrite db:e xisted tables dynamic_data and static_data would be dropped\n\
--read-bad(-x) read also bad values (default false)\n");
                return 0;
            case 'o':
                online = true;
                history_mode = false;
                printf("online mode");
                break;
            case 'u':
                clickhouse = optarg;
                printf("clickhouse_server %s, ", clickhouse.c_str());
                break;
            case 'f':
                csv_file = optarg;
                printf("data output %s, ", csv_file.c_str());
                break;
            case 'd':
                delta = atoi(optarg);
                printf("delta %i, ", delta);
                break;
            case 'm':
                mean = atoi(optarg);
                printf("mean %i, ", mean);
                break;
            case 's':
                ns = atoi(optarg);
                printf("ns %i, ", ns);
                break;
            case 'b':
                begin = optarg;
                printf("begin %s, ", begin.c_str());
                break;
            case 'e':
                end = optarg;
                printf("end %s, ", end.c_str());
                break;
            case 'p':
                pause = atoi(optarg);
                printf("pause %i, ", pause);
                break;
            case 't':
                timeout = atoi(optarg);
                printf("timeout %i, ", timeout);
                break;
            case 'r':
                read_bounds = true;
                printf("read bounds, ");
                break;
            case 'n':
                read_bounds = false;
                printf("don\'t read bounds, ");
                break;
            case 'w':
                rewrite = true;
                printf("rewrite db, ");
                break;
            case 'x':
                read_bad = true;
                printf("read_bad, ");
                break;
            case 'k':
                kks_mode = true;
                online = false;
                history_mode = false;
                printf("read kks, ");
                kks = optarg;
                break;
            case 'c':
                recursive = optarg;
                printf("recursive, type= %s, ", recursive.c_str());
                break;
            case 'i':
               history_mode = true;
               online = false;
               printf("read history, ");
               break;
            case 'a':
                opc_server = optarg;
                printf("opc_server %s, ", recursive.c_str());
                break;


	    }
	}

    if (online)
    {
        printf("ONLINE\n\n delta = %d, ", delta);
        printf("mean = %d, ", mean);
        printf("ns = %d, ", ns);
    }
    else if (kks_mode)
    {
        printf("KKS\n\n ns = %d, ", ns);
        if (recursive != "false")
            printf("list of kks recursively from id= %s, type = %s", kks.c_str(), recursive.c_str());
        else
            printf("list of subobjects from id= %s, ", kks.c_str());


    }
    else if (history_mode)
    {
        if (begin == "")
        {
            printf("begin time not pointed");
            exit(1);
        }
        if (end == "")
        {
            printf("end time not pointed");
            exit(1);
        }
        printf("HISTORY\n\n begin = %s, ", begin.c_str());
        printf("end = %s, ", end.c_str());
        printf("pause = %i, ", pause);
        printf("timeout = %i, ", timeout);
        printf("read bounds = %s \n", read_bounds?"true":"false");
    }
    if (clickhouse != "")
        printf("using clickhouse\n");
    printf("rewrite = %s \n", rewrite?"true":"false");

    // Initialize the UA Stack platform layer
    UaPlatformLayer::init();

    // Create instance of SampleClient
    pMyClient = new SampleClient(delta,mean,ns,rewrite,read_bad,clickhouse,csv_file);

    // Connect to OPC UA Server
    status_run = pMyClient->connect(opc_server);

    // Connect succeeded
    if (status_run.isGood())
    {
        if (online)
        {
            // Read values one time
            while(!exit_flag)
            {
                status_run = pMyClient->read();
                UaThread::msleep(delta);
            }
        }
        else if (kks_mode)
        {
                status_run = pMyClient->browseSimple(kks.c_str(),recursive,csv_file);
        }
        else if (history_mode)
        {
            status_run = pMyClient->readHistory(begin.c_str(),end.c_str(),pause,timeout,read_bounds);
        }


        // Wait for user command.
        //printf("\nPress Enter to create subscription\n");
        //printf("Press Enter again to stop subscription\n");
        //getchar();

        // Create subscription
//        status_run = pMyClient->subscribe();

//        if (status_run.isGood())
//        {
//            // Wait for user command.
//            getchar();

//            // Delete subscription
//            status = pMyClient->unsubscribe();
//        }

//        // Wait for user command.
//        //printf("\nPress Enter to disconnect\n");
//        //getchar();

//        // Disconnect from OPC UA Server
//        pMyClient->disconnect(); // deprecated: in destructor
    }

    // Close application
    //printf("\nPress Enter to close\n");
    // Wait for user command.
    //getchar();

    delete pMyClient;
    pMyClient = NULL;
    printf("deleted pMyClient\n");

    // Cleanup the UA Stack platform layer
    UaPlatformLayer::cleanup();
    printf("UaPlatformLayer::cleanup\n");

    return 0;
}
