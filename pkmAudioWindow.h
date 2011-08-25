/*
 *  pkmAudioWindow.h
 *  memoryMosaic
 *
 *  Created by Parag K. Mital - http://pkmital.com 
 *  Contact: parag@pkmital.com
 *
 *  Copyright 2011 Parag K. Mital. All rights reserved.
 * 
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use,
 *	copy, modify, merge, publish, distribute, sublicense, and/or sell
 *	copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following
 *	conditions:
 *	
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,	
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *	OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <Accelerate/Accelerate.h>
class pkmAudioWindow
{
public:
	pkmAudioWindow()
	{
	}
	
	virtual ~pkmAudioWindow()
	{
		
	}
	
	static void initializeWindow(int fS = 512)
	{
		int ratio = 1;
		frameSize = fS;
		rampInLength = frameSize;
		rampOutLength = frameSize;
		window = (float *)malloc(sizeof(float) * frameSize / (ratio/2.0f));
		rampInBuffer = (float *)malloc(sizeof(float) * rampInLength);
		rampOutBuffer = (float *)malloc(sizeof(float) * rampOutLength);
		
		vDSP_hann_window(window, frameSize/(ratio/2.0f), vDSP_HANN_DENORM);
		//float scalar = 2.0f;
		//vDSP_vsmul(window, 1, &scalar, window, 1, frameSize*2);
		cblas_scopy(frameSize/ratio, window, 1, rampInBuffer, 1);
		cblas_scopy(frameSize/ratio, window+frameSize/ratio, 1, rampOutBuffer+frameSize*(ratio-1)/ratio, 1);
		
		for (int i = frameSize/ratio; i < frameSize; i++) {
			rampInBuffer[i] = 1.0f;
		}
		
		for (int i = 0; i < frameSize*(ratio-1)/ratio; i++) {
			rampOutBuffer[i] = 1.0f;
		}
		
		
		rampInBuffer[0] = 0.0f;
		rampOutBuffer[rampOutLength-1] = 0.0f;
		
		printf("Window (%d samples):\n", (int)(frameSize / (ratio/2.0f)));
		for (int i = 0; i < frameSize / (ratio/2.0f); i++) {
			printf("%f, ", window[i]);
		}
		
		printf("rampInBuffer (%d samples):\n", rampInLength);
		for (int i = 0; i < rampInLength; i++) {
			printf("%f, ", rampInBuffer[i]);
		}
		
		printf("rampOutBuffer (%d samples):\n", rampOutLength);
		for (int i = 0; i < rampOutLength; i++) {
			printf("%f, ", rampOutBuffer[i]);
		}
		printf("\n");
	}
	
	
	static void deallocate()
	{
		free(window);
		free(rampInBuffer);
		free(rampOutBuffer);
		
	}
	
	// Attack Envelope using ramps
	static int frameSize, rampInLength, rampOutLength;
	static float *rampInBuffer, *rampOutBuffer, *window;
};
