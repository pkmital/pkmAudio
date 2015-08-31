

/*
 *  pkmAudioSpectralFlux.cpp
 *  avSaliency
 *
 *  Created by Mr. Magoo on 8/30/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */


#include "pkmAudioSpectralFlux.h"

pkmAudioSpectralFlux::pkmAudioSpectralFlux(int fS, int fftSize, int sampleRate)
{
    bInit = bSegmenting = false;
    frameSize = fS;
    fluxHistorySize = sampleRate / fS * 0.3;
    fft = new pkmFFT(fftSize);
    magnitudes = pkm::Mat(1, fftSize / 2, true);
    phases = pkm::Mat(1, fftSize / 2, true);
    fluxHistory = pkm::Mat(fluxHistorySize, fftSize / 2, true);
    
    meanFlux = pkm::Mat(1, fftSize / 2, true);
    meanSqrFlux = pkm::Mat(1, fftSize / 2, true);
    stdFlux = pkm::Mat(1, fftSize / 2, true);
    
    numFramesSinceLastOnset = 0;
    minSegmentLength = sampleRate / fS * 0.05;
    threshold = 0.25;
    currentFlux = 0;
    
//    fluxWaveform = pkm::Mat(fS, 1, true);
    
    
    alpha = 0.01;
    
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
    
    return getFlux(magnitudes.data, magnitudes.size());
}


float pkmAudioSpectralFlux::getFlux(float *magnitudes, int length)
{   
    
    if (bInit) {
        pkm::Mat magnitudesMat(1, length, magnitudes, true);
        
        if (fluxHistory.bCircularInsertionFull) {
            
#ifdef NO_IIR
            fluxHistory.getMeanAndStdDev(meanFlux, stdFlux);
            
#else
            pkm::Mat mag(1, length, magnitudes, false);

            meanSqrFlux.multiply(1 - alpha);
            pkm::Mat magSqr = mag;
            magSqr.sqr();
            magSqr.multiply(alpha);
            meanSqrFlux.add(magSqr);

            meanFlux.multiply(1 - alpha);
            mag.multiply(alpha);
            meanFlux.add(mag);
            
            pkm::Mat meanSqr = meanFlux;
            meanSqr.sqr();
            meanSqrFlux.subtract(meanSqr, stdFlux);
            stdFlux = meanSqrFlux;
            stdFlux.sqrt();
#endif
            magnitudesMat.subtract(meanFlux);
            magnitudesMat.divide(stdFlux);
            
            magnitudesMat.sqr();

            currentFlux = magnitudesMat.sumAll() / magnitudesMat.size();
        }
        else
            currentFlux = 0;
        
//        fluxWaveform.insertRowCircularly(&currentFlux);
        fluxHistory.insertRowCircularly(magnitudes);
        
    }
    else {
        currentFlux = 0;
        bInit = true;
    }
    
    return currentFlux;
}

bool pkmAudioSpectralFlux::detectOnset(float *audioSignal)
{
    fft->forward(0, audioSignal, magnitudes.data, phases.data);
    
    return detectOnset(magnitudes.data, magnitudes.size());
}


bool pkmAudioSpectralFlux::detectOnset(float *magnitudes, int length)
{
    currentFlux = getFlux(magnitudes, length);
    
//    std::cout << "flux: " << currentFlux << " frames since last onset: " << numFramesSinceLastOnset << std::endl;
    
    if (isnan(currentFlux) | isinf(currentFlux)) {
        numFramesSinceLastOnset = 0;
        return false;
    }
    else {
        numFramesSinceLastOnset++;
    }
    
    
    if ((currentFlux > threshold) && (numFramesSinceLastOnset > minSegmentLength)) {
        numFramesSinceLastOnset = 0;
        return true;
    }
    else {
        return false;
    }
}

void pkmAudioSpectralFlux::setOnsetThreshold(float thresh)
{
    //    alpha = thresh;
    threshold = thresh;
}

void pkmAudioSpectralFlux::setIIRAlpha(float a)
{
    alpha = a;
}

float pkmAudioSpectralFlux::getOnsetThreshold()
{
    return threshold;
}

void pkmAudioSpectralFlux::setMinSegmentLength(int frames)
{
    minSegmentLength = MAX(4,frames);
}

void pkmAudioSpectralFlux::setFrameSize(int fS)
{
    frameSize = fS;
}

float pkmAudioSpectralFlux::getMinSegmentLength()
{
    return minSegmentLength;
}











