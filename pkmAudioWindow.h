/*
 *  pkmAudioWindow.h
 *  memoryMosaic
 *
 *  Created by Parag K. Mital - http://pkmital.com 
 *  Contact: parag@pkmital.com
 *
 *  Copyright 2011 Parag K. Mital. All rights reserved.
 * 
 
 LICENSE:
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 
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
		rampInLength = frameSize / 10;
		rampOutLength = frameSize / 10;
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
		
        /*
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
        */
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
