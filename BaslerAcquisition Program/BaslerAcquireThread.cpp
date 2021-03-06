/**********************************************************************************
Filename	: BaslerAcquireThread.cpp
Authors		: Kevin Wong, Yifan Jian, Jing Xu, Marinko Sarunic
Published	: March 14th, 2013

Copyright (C) 2012 Biomedical Optics Research Group - Simon Fraser University

This file is part of a free software. Details of this software has been described 
in the paper: 

"Jian Y, Wong K, Sarunic MV; Graphics processing unit accelerated optical coherence 
tomography processing at megahertz axial scan rate and high resolution video rate 
volumetric rendering. J. Biomed. Opt. 0001;18(2):026002-026002.  
doi:10.1117/1.JBO.18.2.026002."

Please refer to this paper for further information about this software. Redistribution 
and modification of this code is restricted to academic purposes ONLY, provided that 
the following conditions are met:
-	Redistribution of this code must retain the above copyright notice, this list of 
	conditions and the following disclaimer
-	Any use, disclosure, reproduction, or redistribution of this software outside of 
	academic purposes is strictly prohibited

*DISCLAIMER*
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
SHALL THE COPYRIGHT OWNERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR 
TORT (INCLUDING NEGLIGENCE OR OTHERWISE)ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are
those of the authors and should not be interpreted as representing official
policies, either expressed or implied.
**********************************************************************************/

#include "niimaq.h"
#include "BaslerAcquireThread.h"

#define		NUM_RING_BUFFERS		250

char intfName[64];
static short *ImaqBuffer = NULL; 
static short         *ImaqBuffers[NUM_RING_BUFFERS]; 
static SESSION_ID   Sid = 0;
static BUFLIST_ID   Bid = 0;
static INTERFACE_ID Iid = 0;
static uInt32        AcqWinWidth;
static uInt32        AcqWinHeight;

void AcquireThreadBasler::BaslerAcquire (void)
{
	sprintf_s(intfName,  "img0");//img0 Basler///img3 Atmel
	imgInterfaceOpen (intfName, &Iid);
	imgSessionOpen (Iid, &Sid);
	AcqWinWidth =  globalOptions->IMAGEWIDTH;
	AcqWinHeight = globalOptions->IMAGEHEIGHT;
	imgSetAttribute2 (Sid, IMG_ATTR_ROI_WIDTH, AcqWinWidth);
	imgSetAttribute2 (Sid, IMG_ATTR_ROI_HEIGHT, AcqWinHeight);
    imgSessionTriggerConfigure2(Sid, IMG_SIGNAL_EXTERNAL, 0, 
                                        IMG_TRIG_POLAR_ACTIVEH, 5000, 
                                        IMG_TRIG_ACTION_BUFFER);

	for(int i=0; i<NUM_RING_BUFFERS; i++)
		ImaqBuffers[i] = NULL; 
	// Setup and launch the ring acquisition
	imgRingSetup (Sid, NUM_RING_BUFFERS, (void**)ImaqBuffers, 0, TRUE);
	uInt32  currBufNum;
	
    //Infinite Loop
	//Temporarily we have incorporated an infinite loop. Stop the program execution by closing the program. 
	//Threads can also be suspended and resumed via the start/stop buttons
	while (true)
	{
		DataFramePos = RawData; //Reset the DataFramePos pointer to the beginning, pointed to by RawData

		if (globalOptions->volumeReady < 1)
		{
			for (int framenum=0; framenum < globalOptions->NumFramesPerVol; framenum++)
			{
				if (breakLoop)
					break;

				imgSessionExamineBuffer2 (Sid, IMG_CURRENT_BUFFER , &currBufNum, (void**)&ImaqBuffer);

				if (!ImaqBuffer) { //Check if Basler Camera is on and properly connected
					printf("ERROR: Basler Camera is either off, or miscommunication with framegrabber.\n Exiting Program...\n");
					Sleep(2000);
					exit(1);
				}

				memcpy(DataFramePos, ImaqBuffer, AcqWinHeight*AcqWinWidth*sizeof(unsigned short));
				DataFramePos = &DataFramePos[AcqWinHeight*AcqWinWidth];
				imgSessionReleaseBuffer (Sid);
				
				if(!globalOptions->bVolumeScan)
					DataFramePos = RawData;
			}

			if (breakLoop) {
				breakLoop = false; //Reset the breakLoop flag after each time
			} else {
				globalOptions->volumeReady++;
			}
		}	
		else
		{
			if(globalOptions->saveVolumeflag == true)
			{		    
				fwrite(DataFramePos,2,AcqWinWidth*AcqWinHeight*globalOptions->NumFramesPerVol,globalOptions->fid);
				globalOptions->saveVolumeflag =false;
				fclose(globalOptions->fid);
				
				ScanThrPtr->InitializeSyncAndScan();	
			}
			globalOptions->volumeReady = 0;
		}
	}
}
	


