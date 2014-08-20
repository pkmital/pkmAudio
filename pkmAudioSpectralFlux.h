/*
 *  pkmAudioSpectralFlux.h
 *  avSaliency
 *
 *  Created by Mr. Magoo on 8/30/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

#include "pkmSTFT.h"
#include "pkmMatrix.h"

class pkmAudioSpectralFlux
{
public:
    pkmAudioSpectralFlux(int fS = 512, int fftSize = 2048, int sampleRate = 44100);
    ~pkmAudioSpectralFlux();
    
    float getCurrentFlux();
    float getFlux(float *audioSignal);
    float getFlux(float *magnitudes, int length);
    
    float getOnsetThreshold();
    float getMinSegmentLength();
    
    bool detectOnset(float *audioSignal);
    bool detectOnset(float *magnitudes, int length);
    
    void setMinSegmentLength(int frames);
    void setOnsetThreshold(float thresh);
    void setIIRAlpha(float alpha);
    
    pkm::Mat fluxWaveform;
    
protected:
    pkmFFT *fft;
    float currentFlux, deltaFlux, previousFlux;
    pkm::Mat meanFlux, meanSqrFlux, stdFlux;
    pkm::Mat magnitudes, phases, fluxHistory;
    int frameSize;
    int fluxHistorySize;
    bool bInit, bSegmenting;
    int minSegmentLength;
    int numFramesSinceLastOnset;
    float threshold;
    
    
    // IIR alpha
    float alpha;
};