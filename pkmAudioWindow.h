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
#include "pkmMatrix.h"

class pkmAudioWindow
{
public:
	pkmAudioWindow()
	{
	}
	
	virtual ~pkmAudioWindow()
	{
		
	}
	
	static void initializeWindow(int ramp_length = 128)
	{
		int ratio = 1;
        
		rampInLength = ramp_length;
		rampOutLength = ramp_length;
        
        ramp_length = ramp_length * 2 + 1;
        
        window = pkm::Mat(1, ramp_length);
        rampInBuffer = pkm::Mat(1, rampInLength);
        rampOutBuffer = pkm::Mat(1, rampOutLength);
		
		vDSP_hann_window(window.data, ramp_length, vDSP_HANN_DENORM);
		
        cblas_scopy(rampInLength, window.data, 1, rampInBuffer.data, 1);
        int window_offset = ramp_length - rampOutLength;
		cblas_scopy(rampOutLength, window.data + window_offset, 1, rampOutBuffer.data, 1);
		
//		for (int i = frameSize/ratio; i < frameSize; i++) {
//			rampInBuffer[i] = 1.0f;
//		}
//		
//		for (int i = 0; i < frameSize*(ratio-1)/ratio; i++) {
//			rampOutBuffer[i] = 1.0f;
//		}

//        for(int i = 0; i < rampInLength; i++)
//        {
//            std::cout << rampInBuffer[i] << ", " << rampOutBuffer[i] << std::endl;
//        }
		
		
		rampInBuffer[0] = 0.0f;
		rampOutBuffer[rampOutLength-1] = 0.0f;
		
	}
	
	
	static void deallocate()
	{
		
	}
	
	// Attack Envelope using ramps
	static int rampInLength, rampOutLength;
    static pkm::Mat rampInBuffer, rampOutBuffer, window;
};
