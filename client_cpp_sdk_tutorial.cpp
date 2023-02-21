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
#include <stdlib.h>
#include <getopt.h>
#include <string>
#include <csignal>

UaStatus status_run;
SampleClient* pMyClient;

void signalHandler(int signum)
{
	   printf("Interrupt!");
	   if (status_run.isGood())
	   {
		   pMyClient->unsubscribe();
		   pMyClient->disconnect();
	   }
	   if (pMyClient)
	   {
		   delete pMyClient;
		   pMyClient = NULL;
	   }
	   UaPlatformLayer::cleanup();
	   exit(signum);
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

	static struct option long_options[] =
	{
			{"delta", 1, NULL,'d'},
//			{"mean", 1, NULL,'m'},
			{"help",0,NULL,'h'},
			{0, 0, 0,0}
	};
	int delta = 1000;
//	int mean = 5; #default N 5 in samplesubscription.h

	// loop over all of the options
	int ch;
	while ((ch = getopt_long(argc, argv, "hd:", long_options, NULL)) != -1)
	{
	    // check to see if a single character or long option came through
	    switch (ch)
	    {
	    	 case 'h':
	    		 printf("read data from OPC UA\noptions:\n\
--delta(-d) miliseconds between reading from OPC UA\n\
--help(-h) this info\n");
//--mean(-e) count of averaging: 1 means we don't calculate average and send each result to DB, 5 - we calculate 5 results to one mean and send it to DB\n\

	    		 return 0;
	    		 break;
	         case 'd':
	             delta = atoi(optarg);
	             printf("delta %i, ", delta);
	             break;
//	         case 'm':
//	             mean = optarg;
//	             printf("mean %i, ", mean);
//	             break;
	    }
	}

	printf("\n\n delta = %d, ", delta);
//	printf("mean = %s, ", mean);

    UaStatus status;

    // Initialize the UA Stack platform layer
    UaPlatformLayer::init();

    // Create instance of SampleClient
    pMyClient = new SampleClient(delta);

    // Connect to OPC UA Server
    status = pMyClient->connect();

    // Connect succeeded
    if (status.isGood())
    {
        // Read values one time
        status = pMyClient->read();


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
//        status = pMyClient->disconnect();
    }

    // Close application
    //printf("\nPress Enter to close\n");
    // Wait for user command.
    //getchar();

    delete pMyClient;
    pMyClient = NULL;

    // Cleanup the UA Stack platform layer
    UaPlatformLayer::cleanup();

    return 0;
}
