/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2011, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/ 
#include <stdio.h>
#include <curl/curl.h>
#include "nfc.h"

#define         NFC_DEMO_DEBUG  1
 
int main(void)
{
	uint32_t versiondata;
    uint32_t id;

#ifdef NFC_DEMO_DEBUG
    printf("Hello!\n");
#endif
begin();
        versiondata = getFirmwareVersion();
        if (! versiondata) 
        {
                #ifdef NFC_DEMO_DEBUG
                        printf("Didn't find PN53x board\n");
                #endif
                while (1); // halt
        }
        #ifdef NFC_DEMO_DEBUG
        // Got ok data, print it out!
        printf("Found chip PN5"); 
        printf("%x\n",((versiondata>>24) & 0xFF));
        printf("Firmware ver. "); 
        printf("%d",((versiondata>>16) & 0xFF));
        printf("."); 
        printf("%d\n",((versiondata>>8) & 0xFF));
        printf("Supports "); 
        printf("%x\n",(versiondata & 0xFF));
        #endif
        // configure board to read RFID tags and cards
        SAMConfig();
        
        while(1)
        {
                // look for MiFare type cards
                id = readPassiveTargetID(PN532_MIFARE_ISO14443A);
                if (id != 0) 
                {
                #ifdef NFC_DEMO_DEBUG
                        printf("Read card #%d\n",id); 
                        CURL *curl;
				  CURLcode res;
 
				  /* In windows, this will init the winsock stuff */ 
				  curl_global_init(CURL_GLOBAL_ALL);
 
				  /* get a curl handle */ 
				  curl = curl_easy_init();
				  if(curl) {
					/* First set the URL that is about to receive our POST. This URL can
					   just as well be a https:// URL if that is what should receive the
					   data. */ 
					curl_easy_setopt(curl, CURLOPT_URL, "http://203.42.134.77/api/rfid");
					/* Now specify the POST data */ 
					char str[100];
					sprintf(str, "userID=12453&rfid=%d",id);
printf(str);					
curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str);
 
					/* Perform the request, res will get the return code */ 
					res = curl_easy_perform(curl);
					/* Check for errors */ 
					if(res != CURLE_OK)
					  fprintf(stderr, "curl_easy_perform() failed: %s\n",
							  curl_easy_strerror(res));
 
					/* always cleanup */ 
					curl_easy_cleanup(curl);
				  }
				  curl_global_cleanup();
                        
                #endif
                }
        }
  
  return 0;
}





