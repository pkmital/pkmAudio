//
//  pkmAudioWaveform.h
//  audioInputExample
//
//  Created by Parag Mital on 4/13/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef audioInputExample_pkmAudioWaveform_h
#define audioInputExample_pkmAudioWaveform_h

#include <Accelerate/Accelerate.h>
#include "pkmEXTAudioFileReader.h"
#include "ofxUI.h"
#include "pkmAudioWindow.h"

//#define USE_MAXIMILIAN
#define USE_NSWINDOW_APP


#ifdef USE_MAXIMILIAN
    #include "maximilian.h"
    #include "maxiGrains.h"
#endif


class pkmAudioWaveform
{
public:
    pkmAudioWaveform()
    {
        bScrubbing = false;
        bRezooming = false;
        bLoop = true;
        bUserInteracting = false;
        bLoopRegion = false;
        bDrawMousePlay = false;
        bNeedsUpdate = true;
        bMoveToSample = false;
        bFollow = true;
        seconds = 0.0;
        bLoaded = false;
        mouseSample = 0;
        currentSample = 0;
        currentSampleStart = 0;
        currentSampleEnd = 0;
        gotoCurrentSample = 0;
        audioFilename = "";
        x = y = 0;
        width = height = 0;
        frameSize = 1;
        numBins = 0;
        rangeSliderWidth = 0;
        
        currentVolume = targetVolume = 1.0;
        
#ifdef USE_MAXIMILIAN
        timeStretch = NULL;
        
        currentSpeed = targetSpeed = 1.0;
        currentGrainSize = targetGrainSize = 0.5; 
        currentOverlaps = targetOverlaps = 4;
        currentRandomWidth = targetRandomWidth = 0;
        currentRandomOffset = targetRandomOffset = 0;
#endif 
    }
    
    ~pkmAudioWaveform() 
    {
        delete guiOverview;
        free(buffer);
    }

    // r determines resolution. higher is bigger windows, meaning less resolution.  set between 0.0-2.0.
    void setup(int px = 0, int py = 0, int w = 1280, int h = 100, int fS = 1024, float r = 1.5)
    {
        padding = 0;
        gui_bar_height = 15;
        
        x = px;
        y = py;
        width = w;
        height = h - gui_bar_height;
        frameSize = fS;
        buffer = (float *)malloc(sizeof(float) * fS);
        vDSP_vclr(buffer, 1, fS);
        resolution = r;
        
        guiOverview = new ofxUICanvas(x + padding, y + height + 1, w - padding*2, gui_bar_height);
        slider = new ofxUIRangeSlider(width - padding*2 - 1, gui_bar_height, 0.0, seconds, 0.0, seconds, "Time");
        slider->setColorFill(ofColor(80));
        guiOverview->addWidget(slider);
        guiOverview->getWidget("Time")->setDrawOutlineHighLight(true);
        //string fontFile = "helvetica-light-normal.ttf";
        //if (!guiOverview->setFont(ofToDataPath(fontFile)))
        //{
        //    cerr << "Unable to load font file: " << fontFile << endl;
        //    //OF_EXIT_APP(0);
        //}
        ofAddListener(guiOverview->newGUIEvent, this, &pkmAudioWaveform::guiEvent); 
        guiOverview->disableAppEventCallbacks();
        
        waveformFbo.allocate(width, height, GL_RGBA, 8);
        waveformFbo.begin();
        ofDisableAlphaBlending();
        ofSetColor(255);
        ofBackground(0);
        ofDrawRectangle(0, 0, width, height);
        waveformFbo.end();
    }
    
    void setSize(int px, int py, int w, int h)
    {
        cout << "Setting size to: " << px << ", " << py << " : " << w << ", " << h << endl;
        width = w;
        height = h;
        x = px;
        y = py;
        guiOverview->getRect()->set(x + padding, y + height + 1, width - padding*2, gui_bar_height);
        slider->getRect()->setWidth(width - padding*2);
        
        
        waveformFbo.allocate(width, height, GL_RGBA, 1);
        waveformFbo.begin();
        ofSetColor(0,0,0,255);
        ofDrawRectangle(0, 0, width, height);
        waveformFbo.end();
        
        if(bLoaded)
            bNeedsUpdate = true;
    }
    
    
    bool loadFile(string filename, int initSampleRate = 44100, unsigned long sampleStart = 0, unsigned long sampleEnd = 0)
    {
        if (audioFilename != filename)
        {
#ifdef USE_MAXIMILIAN
            if(!maxiReader.load(filename))
            {
                return false;
            }
            cout << maxiReader.length << endl;
            if(timeStretch != NULL) 
            {
                delete timeStretch;
            }
            sampleRate = maxiReader.mySampleRate;
            totalSamples = maxiReader.length;
            
            timeStretch = new maxiTimestretch<hannWinFunctor>(&maxiReader);
#else
            if (!reader.open(filename, initSampleRate))
            {
                return false;
            }
            sampleRate = reader.mFrameRate;
            totalSamples = reader.mNumSamples;
#endif
            audioFilename = filename;
            
            if(sampleEnd == 0)
            {
                // initially load up 10 seconds of the audio file only
                sampleEnd = MIN(sampleStart + sampleRate * 10, totalSamples);
                rangeSliderWidth = (sampleEnd - sampleStart) / sampleRate;
                cout << "range: " << rangeSliderWidth << endl;
            }
            seconds = totalSamples / sampleRate;
            slider->setMin(0.0);
            slider->setMax(seconds);
            
            
            setRegionToZoom(sampleStart, sampleEnd);
            currentSample = sampleStart;
            bLoaded = true;
            bUserInteracting = false;
            return true;
        }
        else
        {
            setRegionToZoom(sampleStart, sampleEnd);
            currentSample = sampleStart;
            bUserInteracting = false;
            return true;
        }
    }
    
    void setRegionToZoom(unsigned long sampleStart, unsigned long sampleEnd)
    {
        if (!bRezooming && (sampleEnd - sampleStart) > 1) {
            bRezooming = true;
            
            slider->setValueLow(sampleStart / (float)sampleRate);
            slider->setValueHigh(sampleEnd / (float)sampleRate);
            
            // samples per pixels
            unsigned long numSamplesToRead = sampleEnd - sampleStart;
            float ratio = numSamplesToRead / (float)(width);
            
            // how many of them to keep for drawing
            binSize = MAX(ratio * resolution, 1);
            
            // get the resampled audio file
            numBins = numSamplesToRead / (float)binSize;
            if(binnedSamples.size() != numBins)
                binnedSamples.resize(numBins);
            
            //cout << "Samples per bin: " << binSize << " Number of bins: : " << numBins << endl;
            
            float bin[binSize];
            float max;
            unsigned long bin_i = 0;
            
            if(!rereader.open(audioFilename))
            {
                cerr << "Could not read from file!" << endl;
                currentSampleStart = currentSampleEnd = 0;
                bNeedsUpdate = false;
                return;
            }
            
            for (unsigned long i = sampleStart; i < sampleEnd; i += binSize) {
                rereader.read(bin, i, binSize);
                vDSP_maxv(bin, 1, &max, binSize);
                binnedSamples[bin_i++] = max;
            }
            rereader.close();
            
            currentSampleStart = sampleStart;
            currentSampleEnd = sampleEnd;
            
            // need to update fbo
            bNeedsUpdate = true;
            bRezooming = false;
        }
    }
    
    void setRegionToZoomS(float secondStart, float secondEnd)
    {
        setRegionToZoom(secondStart * sampleRate, secondEnd * sampleRate);        
    }
    
    void setCurrentSample(unsigned long sample)
    {
        gotoCurrentSample = MIN(MAX(0,sample),totalSamples);
        bMoveToSample = true;
    }
    
    void setCurrentSampleFromMouse(int px)
    {
        if(px > (x+padding) && px < (x + width - padding))
        {
            float r = (px - (x+padding)) / (float)(width - padding*2);
            setCurrentSample((currentSampleEnd - currentSampleStart) * r + currentSampleStart);
        }
    }
    
    void setLoopRegion(bool loop)
    {
        bLoopRegion = loop;
    }

    void setLoop(bool loop)
    {
        bLoop = loop;
    }
    
    void setFollow(bool follow)
    {
        bFollow = follow;
    }
    
    
#ifdef USE_MAXIMILIAN
    
    
    void setCurrentSpeed(float sp)
    {
        targetSpeed = sp;
    }
    
    float getCurrentSpeed()
    {
        return targetSpeed;
    }
    
    void setCurrentGrainSize(float grain)
    {
        targetGrainSize = grain;
    }
    
    float getCurrentGrainSize()
    {
        return targetGrainSize;
    }
    
    void setCurrentRandomWidth(float width)
    {
        targetRandomWidth = width;
    }
    
    float getCurrentRandomWidth()
    {
        return targetRandomWidth;
    }
    
    void setCurrentRandomOffset(float offset)
    {
        targetRandomOffset = offset;
    }
    
    float getCurrentRandomOffset()
    {
        return targetRandomOffset;
    }
    
    void setCurrentOverlaps(float ov)
    {
        targetOverlaps = ov;
    }
#endif
    
    
    void setCurrentVolume(float vol)
    {
        targetVolume = vol;
    }
    
    float getCurrentVolume()
    {
        return targetVolume;
    }
    
    unsigned long getLength()
    {
        return totalSamples;
    } 
    
    unsigned long getCurrentSample()
    {
        return currentSample;
    }
    
    unsigned long getCurrentFrame()
    {
        return currentSample / frameSize;
    }
    
    void updateWaveform()
    {
        if(bRezooming)
            return;
        
        guiOverview->getWidget("Time")->setDrawOutlineHighLight(true);
        guiOverview->getWidget("Time")->setDrawOutline(true);
        waveformFbo.begin();
        ofBackground(0);        
        ofPushMatrix();
        ofEnableSmoothing();
        float amplitude = (height - gui_bar_height) / 2.0;
        ofFill();
        ofSetColor(50);
        ofDrawRectangle(padding, padding, width - padding*2, height + gui_bar_height - padding*2);
        ofNoFill();
        ofSetColor(255);
        ofDrawRectangle(padding + 1, padding + 1, width - padding*2 - 2, height- 2*padding - 2);
        ofSetLineWidth(1.0);
        ofTranslate(padding, height/2);
        
        if(numBins > 0)
        {
            ofSetColor(30,200,80);
            float width_step = (float)(width - padding*2) / (float)numBins;
            for (unsigned long i = 0; i < numBins; i++) {
                float h = binnedSamples[i]*amplitude;
                ofDrawRectangle(i*width_step, -h, width_step, h*2);
            }
        }
        
        ofDisableSmoothing();
        ofPopMatrix();
        waveformFbo.end();
        bNeedsUpdate = false;
    }
    
    // passing in x,y here will not affect the mouse interaction stuff so will mess everything up.
    // better not to move the drawing unless you don't need mouse interaction.  or just reinitalize the
    // waveform object with the x,y position so everything knows about the x,y position. 
    // the ofxUI stuff needs the x,y position when creating the canvas and is why all this shit needs to 
    // be done :/
    void draw(int px = 0, int py = 0) 
    {
        
        if(bLoaded && bFollow && !bUserInteracting && !bLoopRegion)
        {
            float w = (rangeSliderWidth / 2.0) *sampleRate;
            if( currentSample > w && w <= (totalSamples / 2.0) && (currentSample + w <= totalSamples) )
            {
                setRegionToZoom(currentSample - w, currentSample + w);
                //slider->setValueLow((currentSample - w) / (float)sampleRate);
                //slider->setValueHigh((currentSample + w) / (float)sampleRate);
            }
        }
        
        // don't translate this one since it has its position internally
        guiOverview->draw();
        
        ofPushMatrix();
        ofTranslate(x + px, y + py);
        if(bLoaded && bNeedsUpdate)
        {
            updateWaveform();
        }
        
        ofSetColor(255);
        ofEnableAlphaBlending();
        waveformFbo.draw(0, 0);
        if(bDrawMousePlay)
        {
            double pt = (mouseSample - currentSampleStart) / (double)(currentSampleEnd - currentSampleStart);
            ofSetColor(20, 220, 20);
            float width_step = (float)(width - padding*2);
            ofDrawRectangle(pt*width_step + padding, 0, 1, height);
            ofDrawTriangle(pt*width_step + padding, 0,
                       pt*width_step + padding, gui_bar_height, 
                       pt*width_step + padding + 10, gui_bar_height/2);
        }
        
        ofDisableAlphaBlending();
        
        if(currentSample > currentSampleStart && currentSample < currentSampleEnd)
        {
            double pt = (currentSample - currentSampleStart) / (double)(currentSampleEnd - currentSampleStart);
            ofSetColor(220, 20, 20);
            float width_step = (float)(width - padding*2);
            ofDrawRectangle(pt*width_step + padding, 0, 1, height);
        }
        
        // draw current play marker
        double pt = (currentSample) / (double)(totalSamples);
        ofSetColor(220, 20, 20);
        float width_step = (float)(width - padding*2);
        ofDrawRectangle(pt*width_step + padding, height + 1, 1, gui_bar_height);
        
        ofPopMatrix();
        
        
    }
    
    bool mouseDragged(int px, int py)
    {
        if (bScrubbing) {
            setCurrentSampleFromMouse(px);
            return true;
        }
        else
            return false;
    }
    
    bool mousePressed(int px, int py)
    {
        if(py > y && py < (y + height + gui_bar_height))
        {
            if(py < (y + gui_bar_height))
            {
                setCurrentSampleFromMouse(px);
                bScrubbing = true;
            }
            return true;
        }
        else
        {
            return false;
        }
    }
    
    bool mouseMoved(int px, int py)
    {
        if(py > y && py < (y + height + gui_bar_height))
        {
            if(py < (y + gui_bar_height))
            {
                bDrawMousePlay = true;
                if(px > (x+padding) && px < (x + width - padding))
                {
                    float r = (px - (x+padding)) / (float)(width - padding*2);
                    mouseSample = (currentSampleEnd - currentSampleStart) * r + currentSampleStart;
                }
            }
            if(px > (x+padding) && px < (x + width - padding))
            {
                bUserInteracting = true;
            }
            return true;
        }
        else
        {
            bUserInteracting = false;
            mouseSample = 0;
            bDrawMousePlay = false;
            return false;
        }
    }
    
    void mouseReleased(int px, int py)
    {
        bScrubbing = false;
    }
    
    void guiEvent(ofxUIEventArgs &e)
    {
        string name = e.widget->getName();
        
        if (name == "Time") {
            ofxUIRangeSlider *slider = (ofxUIRangeSlider *) e.widget;    
            setRegionToZoomS(slider->getScaledValueLow(), slider->getScaledValueHigh());
            
            rangeSliderWidth = (slider->getScaledValueHigh() - slider->getScaledValueLow());
            cout << "range: " << rangeSliderWidth << endl;
            
            slider->setIncrement(1000.0 / (float)totalSamples);
        }
    }
    
    bool isPlaying()
    {
        return currentSample < currentSampleEnd;
    }
    
    void update()
    {
        
        float mixFactor = 0.95;
        currentVolume = targetVolume * (1-mixFactor) + currentVolume * mixFactor;
#ifdef USE_MAXIMILIAN
        currentGrainSize = targetGrainSize * (1-mixFactor) + currentGrainSize * mixFactor;
        currentSpeed = targetSpeed * (1-mixFactor) + currentSpeed * mixFactor;
        currentRandomOffset = targetRandomOffset * (1-mixFactor) + currentRandomOffset * mixFactor;
        currentRandomWidth = targetRandomWidth * (1-mixFactor) + currentRandomWidth * mixFactor;
        currentOverlaps = targetOverlaps * (1-mixFactor) + currentOverlaps * mixFactor;
#endif
        
    }
    
    void readFrameAndIncrement(float *buf)
    {
        
        if(!bLoaded)
            return;
        
        update();
        
        vDSP_vclr(buf, 1, frameSize);
        
        unsigned long startSample, endSample;
        bool loop;
        if(bLoopRegion)
        {
            startSample = currentSampleStart;
            endSample = currentSampleEnd;
            loop = bLoopRegion;
        }
        else
        {
            startSample = 0;
            endSample = totalSamples;
            loop = bLoop;
        }
            
        // cross-fade
        if (bMoveToSample)
        {
            // fade out current frame
            int samplesLeft = MIN(frameSize, endSample - currentSample);
#ifdef USE_MAXIMILIAN
            if(currentRandomWidth > 0)
            {
                for (int i = 0; i < samplesLeft; i++) {
                    buf[i] = timeStretch->play(currentSpeed, currentGrainSize, currentOverlaps, (rand() % 1000) * (currentRandomWidth / 1000.0) - currentRandomOffset); //maxiReader.play();
                }
            }
            else
            {
                for (int i = 0; i < samplesLeft; i++) {
                    buf[i] = timeStretch->play(currentSpeed, currentGrainSize, currentOverlaps, 0); //maxiReader.play();
                }
            }
#else
            reader.read(buf, currentSample, samplesLeft);
#endif
            if(frameSize > samplesLeft)
                vDSP_vclr(buf + samplesLeft, 1, frameSize - samplesLeft);
            vDSP_vmul(buf + (frameSize - pkmAudioWindow::rampOutLength), 1, 
                      pkmAudioWindow::rampOutBuffer, 1, 
                      buf + (frameSize - pkmAudioWindow::rampOutLength), 1, pkmAudioWindow::rampOutLength);
            // fade in new frame
            currentSample = gotoCurrentSample;
#ifdef USE_MAXIMILIAN
            timeStretch->setPosition(currentSample);
            if(currentRandomWidth > 0)
            {
                for (int i = 0; i < frameSize; i++) {
                    buffer[i] = timeStretch->play(currentSpeed, currentGrainSize, currentOverlaps, (rand() % 1000) * (currentRandomWidth / 1000.0) - currentRandomOffset); //maxiReader.play();
                }
            }
            else
            {
                for (int i = 0; i < frameSize; i++) {
                    buffer[i] = timeStretch->play(currentSpeed, currentGrainSize, currentOverlaps, 0); //maxiReader.play();
                }
            }
#else
            reader.read(buffer, currentSample, frameSize);
#endif
            vDSP_vmul(buffer, 1, 
                      pkmAudioWindow::rampInBuffer, 1, 
                      buffer, 1, pkmAudioWindow::rampInLength);
            
            vDSP_vadd(buf, 1, buffer, 1, buf, 1, frameSize);
            
#ifdef USE_MAXIMILIAN
            currentSample = timeStretch->getPosition();
     
#else
            currentSample += frameSize;
#endif
            bMoveToSample = false;
        }
        // fade in
        else if (currentSample == startSample)
        {
            
#ifdef USE_MAXIMILIAN
            if(currentRandomWidth > 0)
            {
                for (int i = 0; i < frameSize; i++) {
                    buf[i] = timeStretch->play(currentSpeed, currentGrainSize, currentOverlaps, (rand() % 1000) * (currentRandomWidth / 1000.0) - currentRandomOffset); //maxiReader.play();
                }
            }
            else
            {
                for (int i = 0; i < frameSize; i++) {
                    buf[i] = timeStretch->play(currentSpeed, currentGrainSize, currentOverlaps, 0); //maxiReader.play();
                }
            }
#else
            reader.read(buf, currentSample, frameSize);
#endif
            vDSP_vmul(buf, 1, 
                      pkmAudioWindow::rampInBuffer, 1, 
                      buf, 1, pkmAudioWindow::rampInLength);
#ifdef USE_MAXIMILIAN
            currentSample = timeStretch->getPosition();
#else
            currentSample += frameSize;
#endif
            
        }
        // read normal
        else if (currentSample + frameSize < endSample)
        {
#ifdef USE_MAXIMILIAN
            if(currentRandomWidth > 0)
            {
                for (int i = 0; i < frameSize; i++) {
                    buf[i] = timeStretch->play(currentSpeed, currentGrainSize, currentOverlaps, (rand() % 1000) * (currentRandomWidth / 1000.0) - currentRandomOffset); //maxiReader.play();
                }
            }
            else
            {
                for (int i = 0; i < frameSize; i++) {
                    buf[i] = timeStretch->play(currentSpeed, currentGrainSize, currentOverlaps, 0); //maxiReader.play();
                }
            }
            currentSample = timeStretch->getPosition();
#else
            reader.read(buf, currentSample, frameSize);
            currentSample += frameSize;
#endif
        }
        // fade out
        else if (currentSample < endSample)
        {
            int samplesLeft = endSample - currentSample;
#ifdef USE_MAXIMILIAN
            if(currentRandomWidth > 0)
            {
                for (int i = 0; i < samplesLeft; i++) {
                    buf[i] = timeStretch->play(currentSpeed, currentGrainSize, currentOverlaps, (rand() % 1000) * (currentRandomWidth / 1000.0) - currentRandomOffset); //maxiReader.play();
                }
            }
            else
            {
                for (int i = 0; i < samplesLeft; i++) {
                    buf[i] = timeStretch->play(currentSpeed, currentGrainSize, currentOverlaps, 0); //maxiReader.play();
                }
            }
#else
            reader.read(buf, currentSample, samplesLeft);
#endif
            if(frameSize > samplesLeft)
                vDSP_vclr(buf + samplesLeft, 1, frameSize - samplesLeft);
            vDSP_vmul(buf + (frameSize - pkmAudioWindow::rampOutLength), 1, 
                      pkmAudioWindow::rampOutBuffer, 1, 
                      buf + (frameSize - pkmAudioWindow::rampOutLength), 1, pkmAudioWindow::rampOutLength);
            loop ?
                currentSample = startSample
            :
#ifdef USE_MAXIMILIAN
                currentSample = timeStretch->getPosition();
#else
                currentSample += frameSize;
#endif
        }
        
        // volume
        vDSP_vsmul(buf, 1, &currentVolume, buf, 1, frameSize);
    }
    
    
    
    bool                        bFollow;
    
private:
    int                         x,y;
    int                         width, height;
    float                       resolution;
    float                       rangeSliderWidth;
    
    int                         frameSize;
    unsigned long               currentSample,          // for playback
                                currentSampleStart,     // current starting sample of the zoomed in region 
                                currentSampleEnd,       // current ending sample of the zoomed in region 
                                gotoCurrentSample,      // sample to fade into
                                mouseSample;            // which sample the mouse is currently selecting
    float                       seconds;
    unsigned long               numBins;
    int                         binSize;
    float                       *samples,
                                *buffer;
    vector<float>               binnedSamples;
    
    unsigned long               sampleRate, totalSamples;
    string                      audioFilename;
    float                       currentVolume, targetVolume;
#ifdef USE_MAXIMILIAN 
    maxiSample                  maxiReader, maxiReReader;
    maxiTimestretch<hannWinFunctor> *timeStretch;
    float                       currentSpeed, currentGrainSize, currentRandomOffset, currentRandomWidth, currentOverlaps;
    float                       targetSpeed, targetGrainSize, targetRandomOffset, targetRandomWidth, targetOverlaps;
#else
    pkmEXTAudioFileReader       reader;
#endif
    pkmEXTAudioFileReader       rereader;
    
    int                         padding;            // padding on sides
    int                         gui_bar_height;     // height of the zoom range slider
    ofxUICanvas                 *guiOverview;
    ofxUIRangeSlider            *slider;
    
    ofFbo                       waveformFbo;
    
    bool                        bLoaded, 
                                bNeedsUpdate, 
                                bUserInteracting,
                                bDrawMousePlay, 
                                bLoop, 
                                bLoopRegion, 
                                bRezooming, 
                                bMoveToSample, 
                                bScrubbing;
};





#ifdef USE_NSWINDOW_APP
#include "ofxNSWindowApp.h"

class pkmAudioWaveformApp : public ofxNSWindowApp
{
public:
    //--------------------------------------------------------------
    void setup()
    {
        frameSize = 1024;
        pkmAudioWindow::initializeWindow();
        ofSoundStreamSetup(2, 0, 44100, frameSize, 4);
        
        ofSetWindowShape(600, 200);
    }
    //--------------------------------------------------------------
    
    //--------------------------------------------------------------
    void allocate(string audioURL)
    {
        audioWaveform = new pkmAudioWaveform();
        audioWaveform->setup(0, 0, ofGetWidth(), ofGetHeight(), frameSize);
        audioWaveform->loadFile(audioURL);
    }
    //--------------------------------------------------------------
    
    //--------------------------------------------------------------
    void update()
    {
        audioWaveform->update();
    }
    //--------------------------------------------------------------
    
    //--------------------------------------------------------------
    void draw();
    //--------------------------------------------------------------
private:
    pkmAudioWaveform *audioWaveform;
    int frameSize;
};

#endif


#endif

