

/*
 *  pkmAudioSpectralFlux.cpp
 *  avSaliency
 *
 *  Created by Mr. Magoo on 8/30/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "pkmAudioSpectralFlux.h"

pkmAudioSpectralFlux::pkmAudioSpectralFlux(int fS, int fftSize)
{
    bInit = false;
    frameSize = fS;           
    fluxHistorySize = 44100 / fS * 3.0;
    fft = new pkmFFT(fftSize);
    magnitudes = pkm::Mat(1, fftSize / 2, true);
    phases = pkm::Mat(1, fftSize / 2, true);
    fluxHistory = pkm::Mat(fluxHistorySize, fftSize / 2, true);
    numFramesSinceLastOnset = 0;
    minSegmentLength = 44100.0 / fS * 0.5;  // half second
    threshold = 1;
    currentFlux = 0;
    smoothedFlux = 0;
}

pkmAudioSpectralFlux::~pkmAudioSpectralFlux()
{
    delete fft;
}

float pkmAudioSpectralFlux::getCurrentFlux()
{
    return currentFlux;
}

float pkmAudioSpectralFlux::getFlux(float *audioSignal)
{   
    fft->forward(0, audioSignal, magnitudes.data, phases.data);
    
    if (bInit) {
        pkm::Mat meanFlux = fluxHistory.mean();
        pkm::Mat stdFlux = fluxHistory.stddev();
        
        fluxHistory.insertRowCircularly(magnitudes.data);
        
        magnitudes.subtract(meanFlux);
        magnitudes.abs();
        magnitudes.divide(stdFlux);
        currentFlux = pkm::Mat::mean(magnitudes.data, magnitudes.rows * magnitudes.cols);
    }
    else {
        currentFlux = 0;
        bInit = true;
    }
    
    return currentFlux;
}


float pkmAudioSpectralFlux::getFlux(float *magnitudes, int length)
{   
    
    if (bInit) {
        pkm::Mat meanFlux = fluxHistory.mean();
        pkm::Mat stdFlux = fluxHistory.stddev();
        
        fluxHistory.insertRowCircularly(magnitudes);
        
        pkm::Mat magnitudesMat(1, length, magnitudes, true);
        
        magnitudesMat.subtract(meanFlux);
        magnitudesMat.abs();
        magnitudesMat.divide(stdFlux);
        
        currentFlux = pkm::Mat::mean(magnitudesMat.data, length);
    }
    else {
        currentFlux = 0;
        bInit = true;
    }
    
    return currentFlux;
}

bool pkmAudioSpectralFlux::detectOnset(float *audioSignal)
{
    currentFlux = getFlux(audioSignal);
    if (currentFlux > threshold && numFramesSinceLastOnset > minSegmentLength) {
        numFramesSinceLastOnset = 0;
        return true;
    }
    else {
        numFramesSinceLastOnset++;
        return false;
    }
}


bool pkmAudioSpectralFlux::detectOnset(float *magnitudes, int length)
{
    currentFlux = getFlux(magnitudes, length);
    
    if (isnan(currentFlux) | isinf(currentFlux)) {
        numFramesSinceLastOnset = 0;
        return false;
    }
    else {
        numFramesSinceLastOnset++;
    }
    
    if (currentFlux > smoothedFlux)
        smoothedFlux = 0.5 * (smoothedFlux - currentFlux) + currentFlux;
    else
        smoothedFlux = 0.85 * (smoothedFlux - currentFlux) + currentFlux; 
    
    //cout << "flux: " << currentFlux << " smoothed: " << smoothedFlux << endl;
    if (smoothedFlux > threshold && numFramesSinceLastOnset > minSegmentLength) {
        //cout << "segment: " <<  smoothedFlux << endl;
        //smoothedFlux = 0;
        numFramesSinceLastOnset = 0;
        return true;
    }
    else {
        numFramesSinceLastOnset++;
        return false;
    }
}

void pkmAudioSpectralFlux::setOnsetThreshold(float thresh)
{
    threshold = thresh;
    cout << "threshold: " << threshold << endl;
}

float pkmAudioSpectralFlux::getOnsetThreshold()
{
    return threshold;
}

void pkmAudioSpectralFlux::setMinSegmentLength(int frames)
{
    minSegmentLength = frames;
}

float pkmAudioSpectralFlux::getMinSegmentLength()
{
    return minSegmentLength;
}











