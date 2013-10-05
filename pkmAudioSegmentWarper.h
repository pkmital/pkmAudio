//
//  pkmAudioSegmentWarper.h
//  audioMosaicingDTWTS
//
//  Created by Parag Mital on 10/31/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#pragma once

#include "pkmAudioSegmentDatabase.h"
#include "pkmAudioSegment.h"
#include "pkmMatrix.h"
#include "maximilian.h"
#include "maxiGrains.h"
#include "pkmAudioWindow.h"
#include "ofMain.h"

#define USE_DIRAC

#ifdef USE_DIRAC
#include "Dirac.h"
#endif

class pkmAudioSegmentWarper
{
public:  
        
    enum warpConditions {
        warpConstant,
        warpReduce,
        warpIncrease,
        warpUnknown
    };
    
    pkmAudioSegmentWarper()
    {
        
    }
    
    static void constantWarp(pkmAudioSegment &sourceSegment, 
                             float fixedSpeed, 
                             unsigned long targetLength,
                             pkm::Mat &warpedAudio)
    {
        warpedAudio.reset(1, targetLength, 0.0f);
        maxiSample s;
        s.load(ofToDataPath(sourceSegment.filename));
        maxiTimestretch<hannWinFunctor> ts(&s);
        float grainSize = 1.0;
        if(fixedSpeed > 1)
            grainSize = 0.00125;
        else if(fixedSpeed == 1)
            grainSize = 0.00125;
        else
            grainSize = 0.0005125;
        
        for (int i = 0; i < targetLength; i++) {
            warpedAudio.data[i] = ts.play(fixedSpeed, grainSize, 4);
            //warpedAudio.data[i] = (float)s.play(fixedSpeed);
        }
        vDSP_vmul(warpedAudio.data, 1, pkmAudioWindow::rampInBuffer, 1, warpedAudio.data, 1, pkmAudioWindow::rampInLength);
        vDSP_vmul(warpedAudio.data + targetLength - pkmAudioWindow::rampOutLength, 1, pkmAudioWindow::rampOutBuffer, 1, warpedAudio.data + targetLength - pkmAudioWindow::rampOutLength, 1, pkmAudioWindow::rampOutLength);
    }
    
        
    static pkm::Mat dynamicWarp(pkmAudioSegment sourceSegment, 
                                vector<int> sourcePath, 
                                vector<int> targetPath, 
                                unsigned int targetFrames,
                                unsigned int frameSize)
    {
        int targetLength = targetFrames * frameSize;
        int sourceLength = sourcePath.size();
        pkm::Mat warpedAudio(1, targetLength, 0.0f);
        
        if (sourceLength > 0)
        {
            int prevIdxSource = sourcePath.back();
            int prevIdxTarget = targetPath.back();
            warpConditions previousState = warpUnknown;
            pkm::Mat warpCurve;
            int previousStateLength = 1;
            
            // get speed for every frame
            for (int i = sourceLength - 2; i >= 0; i--) {
                if (sourcePath[i] == prevIdxSource &&               // reduce source speed
                    targetPath[i] != prevIdxTarget)
                {
                    if (previousState == warpReduce) {              // same state
                        previousStateLength++;
                    }
                    else if(previousState == warpUnknown) {
                        previousStateLength++;
                        previousState = warpReduce;
                    }
                    else {                                          // state changed
                        float speed = getSpeedForState(previousState, previousStateLength);
                        for (int j = 0; j < previousStateLength; j++) {
                            warpCurve.push_back(speed);
                        }
                        previousStateLength = 1;
                        previousState = warpReduce;
                    }
                }
                else if(sourcePath[i] != prevIdxSource &&           // increase source speed
                        targetPath[i] == prevIdxTarget)
                {
                    if (previousState == warpIncrease) {            // same state
                        previousStateLength++;
                    }
                    else if(previousState == warpUnknown) {
                        previousStateLength++;
                        previousState = warpIncrease;
                    }
                    else {                                          // state changed
                        float speed = getSpeedForState(previousState, previousStateLength);
                        //for (int j = 0; j < previousStateLength; j++) {
                        warpCurve.push_back(speed);
                        //}
                        previousStateLength = 1;
                        previousState = warpIncrease;
                    }
                }
                else if(sourcePath[i] != prevIdxSource &&           // constant source speed
                        targetPath[i] != prevIdxTarget)
                {
                    if (previousState == warpConstant) {            // same state
                        previousStateLength++;
                    }
                    else if(previousState == warpUnknown) {
                        previousStateLength++;
                        previousState = warpConstant;
                    }
                    else {                                          // state changed
                        float speed = getSpeedForState(previousState, previousStateLength);
                        for (int j = 0; j < previousStateLength; j++) {
                            warpCurve.push_back(speed);
                        }
                        previousStateLength = 1;
                        previousState = warpConstant;
                    }
                }
                else {
                    cout << "Uh..." << endl;
                }
            }
            float speed = getSpeedForState(previousState, previousStateLength);
            for (int j = 0; j < previousStateLength; j++) {
                warpCurve.push_back(speed);
            }
            pkm::Mat speeds = pkm::Mat::resize(warpCurve, targetFrames*frameSize);
            
            
//            pkm::Mat speeds(1, targetFrames*frameSize, true);
//            pkm::Mat sourcePathMat(1, sourcePath.size()), targetPathMat(1, targetPath.size());
//            
//            for(int i = 0; i < targetPath.size(); i++)
//            {
//                sourcePathMat[sourcePath.size() - 1 - i] = sourcePath[i] + 1;
//                targetPathMat[targetPath.size() - 1 - i] = targetPath[i] + 1;
//            }
//            
//            pkm::Mat sourcePathRsz = pkm::Mat::resize(sourcePathMat, targetFrames*frameSize);
//            pkm::Mat targetPathRsz = pkm::Mat::resize(targetPathMat, targetFrames*frameSize);
//            sourcePathRsz.divide(targetPathRsz, speeds);
//            //targetPathRsz.divide(sourcePathRsz, speeds);
        
#ifdef USE_DIRAC
            maxiSample s;
            s.load(ofToDataPath(sourceSegment.filename));
            
            //void *dirac = DiracCreateInterleaved(kDiracLambdaPreview, kDiracQualityPreview, 1, 44100, &diracReadSample, (void *)&s);
            void *dirac = DiracCreateInterleaved(kDiracLambda3, kDiracQualityBest, 1, 44100, &diracReadSample, (void *)&s);
            if (!dirac) {
                printf("!! ERROR !!\n\n\tCould not create DIRAC instance\n\tCheck number of channels and sample rate!\n");
                exit(-1);
            }
            float pitch     = pow(2., 0./12.); // pitch shift (0 semitones)
            float formant   = pow(2., 0./12.); // formant shift (0 semitones). N.b. formants are reciprocal to pitch in natural transposing.
            
            DiracSetProperty(kDiracPropertyPitchFactor, pitch, dirac);
            DiracSetProperty(kDiracPropertyFormantFactor, formant, dirac);
            
            float sample;
            float *ptr = warpedAudio.data;
            for (int sample_i = 0; sample_i < targetFrames * frameSize; sample_i++)
            {
                DiracSetProperty(kDiracPropertyTimeFactor, speeds[sample_i], dirac);
                long ret = DiracProcessInterleaved(&sample, 1, dirac);
                *ptr++ = sample;
            }
            
            DiracDestroy( dirac );
#else
            
            // warp audio using calculated speed for each audio frame
            maxiSample s;
            s.load(ofToDataPath(sourceSegment.filename));
            maxiTimestretch<hannWinFunctor> ts(&s);
            float *ptr = warpedAudio.data;
            int overlaps = 4;
            float grainSize = 0.00125;
            for (int sample_i = 0; sample_i < targetFrames * frameSize; sample_i++)
            {
                float sample = (float)ts.play(speeds[sample_i], grainSize, overlaps);
                *ptr++ = sample;
            }
#endif
            
            // window
            vDSP_vmul(warpedAudio.data, 1, pkmAudioWindow::rampInBuffer, 1, warpedAudio.data, 1, pkmAudioWindow::rampInLength);
            vDSP_vmul(warpedAudio.data + targetLength - pkmAudioWindow::rampOutLength, 1, pkmAudioWindow::rampOutBuffer, 1, warpedAudio.data + targetLength - pkmAudioWindow::rampOutLength, 1, pkmAudioWindow::rampOutLength);
        }
        
        return warpedAudio;
    }
    
protected:
    
    static float getSpeedForState(warpConditions state, int pathLength)
    {
        if (state == warpReduce) {
            return ( 1.0 / (float) pathLength );
        }
        else if (state == warpIncrease) {
            return pathLength;
        }
        else
            return 1.0;
    }
    
private:
    
#ifdef USE_DIRAC
    void *dirac;
    
    static long diracReadSample(float *data, long numFrames, void *userData)
    {
        if(!data)
            return 0;
        
        maxiSample *sample = (maxiSample *)userData;
        
        for (int i = 0; i < numFrames; i++) {
            data[i] = sample->play();
        }
        
        return numFrames;
    }
    
#endif
    
};











    