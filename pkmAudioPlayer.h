/*
 *  pkmAudioPlayer.h
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
 
 Windows playback
 Loop functionality, or stop at end 
 
 */

#include "pkmAudioFile.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "pkmAudioWindow.h"
#include <Accelerate/Accelerate.h>

#define MIN_FRAMES 2

#ifndef MIN
#define MIN(X,Y) (X) > (Y) ? (Y) : (X)
#endif

class pkmAudioPlayer
{
public:
	pkmAudioPlayer(pkmAudioFile *myFile, int frame_size = 512, int num_frames_to_play = 0, bool loop = true)
	{
		audioFile = myFile;			
		assert((audioFile->length - audioFile->offset) > 0);
		
		//printf("audiofile size: %d, offset: %d\n", audioFile->length, audioFile->offset);
		frameSize = frame_size;		
		if (loop) {
			framesToPlay = num_frames_to_play;
		}
		else {
			framesToPlay = MIN(num_frames_to_play, (myFile->length - myFile->offset) / frame_size);
		}
		
		bLoop = loop;
		//empty = (float *)malloc(sizeof(float) * frameSize);
		rampedBuffer = (float *)malloc(sizeof(float) * frameSize);
		//memset(empty, 0, sizeof(float)*frameSize);
	}
	~pkmAudioPlayer()
	{
		//free(empty);
		free(rampedBuffer);
	}
	bool initialize()
	{
			// default play to the end of the file from the starting offset (if it exists)
		if (framesToPlay < MIN_FRAMES ||
			// if the frames to play are too high
			(audioFile->length < (audioFile->offset + frameSize*framesToPlay))) 
		{
			framesToPlay = (audioFile->length - audioFile->offset) / frameSize;
		}
		
		if (framesToPlay < MIN_FRAMES) 
		{
			framesToPlay = 1;
			// don't touch this data! 
			//audioFile->buffer = empty;
			//audioFile->offset = 0;
			currentFrame = 0;
			//printf("[ERROR]: audioPlayer failed to initialized - Not enough frames.");
			return false;
		}
		
		//printf("audioPlayer initialized with %d frames to play\n", framesToPlay);
		// start at the first frame (past the offset if any)
		currentFrame = 0;
		return true;
	}
	
    inline bool isLastFrame()
    {
        return (currentFrame == framesToPlay-1);
    }
	
	// get the next audio frame to play 
	float * getNextFrame()
	{
		int offset = currentFrame*frameSize;
		if (bLoop) {
			currentFrame = (currentFrame + 1) % framesToPlay;
		}
		else {
			currentFrame++;
			if (currentFrame > framesToPlay) {
				// shouldn't get here so long as the user checks for:
				// isLastFrame() before getNextFrame()
				printf("[ERROR]: EOF reached! Returning an empty buffer!");
				vDSP_vclr(rampedBuffer, 1, frameSize);
				return rampedBuffer;
			}
		}
		// fade in
		if (currentFrame == 1) {
			cblas_scopy(frameSize, audioFile->buffer + audioFile->offset + offset, 1, rampedBuffer, 1);
			vDSP_vmul(rampedBuffer, 1, pkmAudioWindow::rampInBuffer, 1, rampedBuffer, 1, pkmAudioWindow::rampInLength);
			//printf("%d/%d: %f\n", currentFrame, framesToPlay, rampedBuffer[0]);
			return rampedBuffer;
		}
		// fade out
		else if(currentFrame == framesToPlay || currentFrame == 0) {
			//printf("f\n");
			cblas_scopy(frameSize, audioFile->buffer + audioFile->offset + offset, 1, rampedBuffer, 1);
			vDSP_vmul(rampedBuffer + frameSize - pkmAudioWindow::rampOutLength, 1, 
					  pkmAudioWindow::rampOutBuffer + frameSize - pkmAudioWindow::rampOutLength, 1, 
					  rampedBuffer + frameSize - pkmAudioWindow::rampOutLength, 1, pkmAudioWindow::rampOutLength);
			//printf("%d/%d: %f\n", currentFrame, framesToPlay, rampedBuffer[frameSize-1]);
			return rampedBuffer;			
		}
		// no fade
		else
			return (audioFile->buffer + audioFile->offset + offset);
	}
	
	bool isFinished()
	{
		return !bLoop && currentFrame > framesToPlay;
	}
	
	pkmAudioFile		*audioFile;
	
	int					frameSize;
	int					framesToPlay;
	int					currentFrame;
	bool				bLoop;
	int					rampInLength, rampOutLength;
	float				*empty, *rampedBuffer;

};
