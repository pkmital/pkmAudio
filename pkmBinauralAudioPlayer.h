/*
 *  pkmBinauralAudioPlayer.h
 *  memoryMosaicFlANN
 *
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

class pkmBinauralAudioPlayer
{
public:
	pkmBinauralAudioPlayer(pkmAudioFile &myFile, int frame_size = 512, int num_frames_to_play = 0, bool loop = true)
	{
		audioFile = myFile;			
		assert((audioFile.length - audioFile.offset) > 0);
		
		//printf("audiofile size: %d, offset: %d\n", audioFile.length, audioFile.offset);
		frameSize = frame_size;		
		if (loop) {
			framesToPlay = num_frames_to_play;
		}
		else {
			framesToPlay = MIN(num_frames_to_play, (myFile.length - myFile.offset) / frame_size);
		}
		
		bLoop = loop;
		empty = (float *)malloc(sizeof(float) * frameSize);
		rampedBuffer = (float *)malloc(sizeof(float) * frameSize);
		memset(empty, 0, sizeof(float)*frameSize);
	}
	~pkmAudioPlayer()
	{
		free(empty);
		free(rampedBuffer);
	}
	bool initialize()
	{
		//printf("initialize: \n");
		// default play to the end of the file from the starting offset (if it exists)
		if (framesToPlay == 0) {
			framesToPlay = (audioFile.length - audioFile.offset) / frameSize;
		}
		
		// if the frames to play are too high, decrement it until it matches the end of the file
		while ( (audioFile.length < (audioFile.offset + frameSize*framesToPlay)) )
		{
			framesToPlay--;
		}
		
		//printf("frames to play: %d\n", framesToPlay);
		
		if (framesToPlay < MIN_FRAMES) {
			framesToPlay = 1;
			audioFile.buffer = empty;
			audioFile.offset = 0;
			return false;
		}
		// start at the first frame (past the offset if any)
		currentFrame = 0;
		return true;
	}
	
    bool isLastFrame()
    {
        return (currentFrame >= framesToPlay-1);
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
				printf("[ERROR]: EOF reached! Returning an empty buffer!");
				return empty;
			}
		}
		// fade in
		if (currentFrame == 1) {
			cblas_scopy(frameSize, audioFile.buffer + audioFile.offset + offset, 1, rampedBuffer, 1);
			vDSP_vmul(rampedBuffer, 1, pkmAudioWindow::rampInBuffer, 1, rampedBuffer, 1, pkmAudioWindow::rampInLength);
			//printf("i: %f\n", rampedBuffer[0]);
			return rampedBuffer;
		}
		// fade out
		else if(currentFrame == framesToPlay) {
			//printf("f\n");
			cblas_scopy(frameSize, audioFile.buffer + audioFile.offset + offset, 1, rampedBuffer, 1);
			vDSP_vmul(rampedBuffer + frameSize - pkmAudioWindow::rampOutLength, 1, 
					  pkmAudioWindow::rampOutBuffer + frameSize - pkmAudioWindow::rampOutLength, 1, 
					  rampedBuffer + frameSize - pkmAudioWindow::rampOutLength, 1, pkmAudioWindow::rampOutLength);
			//printf("o: %f\n", rampedBuffer[frameSize-1]);
			return rampedBuffer;			
		}
		// no fade
		else
			return (audioFile.buffer + audioFile.offset + offset);
	}
	
	bool isFinished()
	{
		return !bLoop && currentFrame >= framesToPlay;
	}
	
	pkmAudioFile		audioFile;
	
	int					frameSize;
	int					framesToPlay;
	int					currentFrame;
	bool				bLoop;
	int					rampInLength, rampOutLength;
	float				*empty, *rampedBuffer;
	
};
