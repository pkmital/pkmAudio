/*
 *  pkmSegmenter.cpp
 *  mosaic
 *
 *
 *  Created by Parag K. Mital - http://pkmital.com 
 *  Contact: parag@pkmital.com
 *
 *  Copyright 2011 Parag K. Mital. All rights reserved.

 
 Copyright (C) 2011 Parag K. Mital
 
 The Software is and remains the property of Parag K Mital
 ("pkmital") The Licensee will ensure that the Copyright Notice set
 out above appears prominently wherever the Software is used.
 
 The Software is distributed under this Licence: 
 
 - on a non-exclusive basis, 
 
 - solely for non-commercial use in the hope that it will be useful, 
 
 - "AS-IS" and in order for the benefit of its educational and research
 purposes, pkmital makes clear that no condition is made or to be
 implied, nor is any representation or warranty given or to be
 implied, as to (i) the quality, accuracy or reliability of the
 Software; (ii) the suitability of the Software for any particular
 use or for use under any specific conditions; and (iii) whether use
 of the Software will infringe third-party rights.
 
 pkmital disclaims: 
 
 - all responsibility for the use which is made of the Software; and
 
 - any liability for the outcomes arising from using the Software.
 
 The Licensee may make public, results or data obtained from, dependent
 on or arising out of the use of the Software provided that any such
 publication includes a prominent statement identifying the Software as
 the source of the results or the data, including the Copyright Notice
 and stating that the Software has been made available for use by the
 Licensee under licence from pkmital and the Licensee provides a copy of
 any such publication to pkmital.
 
 The Licensee agrees to indemnify pkmital and hold them
 harmless from and against any and all claims, damages and liabilities
 asserted by third parties (including claims for negligence) which
 arise directly or indirectly from the use of the Software or any
 derivative of it or the sale of any products based on the
 Software. The Licensee undertakes to make no liability claim against
 any employee, student, agent or appointee of pkmital, in connection 
 with this Licence or the Software.
 
 
 No part of the Software may be reproduced, modified, transmitted or
 transferred in any form or by any means, electronic or mechanical,
 without the express permission of pkmital. pkmital's permission is not
 required if the said reproduction, modification, transmission or
 transference is done without financial return, the conditions of this
 Licence are imposed upon the receiver of the product, and all original
 and amended source code is included in any transmitted product. You
 may be held legally responsible for any copyright infringement that is
 caused or encouraged by your failure to abide by these terms and
 conditions.
 
 You are not permitted under this Licence to use this Software
 commercially. Use for which any financial return is received shall be
 defined as commercial use, and includes (1) integration of all or part
 of the source code or the Software into a product for sale or license
 by or on behalf of Licensee to third parties or (2) use of the
 Software or any derivative of it for research with the final aim of
 developing software products for sale or license to a third party or
 (3) use of the Software or any derivative of it for research with the
 final aim of developing non-software products for sale or license to a
 third party, or (4) use of the Software to provide any service to an
 external organisation for which payment is received. If you are
 interested in using the Software commercially, please contact pkmital to
 negotiate a licence. Contact details are: parag@pkmital.com
 
 
 */

#include "pkmAudioSegmenter.h"

pkmAudioSegmenter::pkmAudioSegmenter(int frameSize)
{
    NUM_BACK_BUFFERS_FOR_FEATURE_ANALYSIS = SAMPLE_RATE*1/frameSize;
    NUM_FORE_BUFFERS_FOR_FEATURE_ANALYSIS = SAMPLE_RATE*1/frameSize;
    NUM_BUFFERS_FOR_SEGMENTATION_ANALYSIS = SAMPLE_RATE*3/frameSize;
    
    audioFeature				= new pkmAudioFeatures(SAMPLE_RATE, frameSize);
    numMFCCs					= 64;//audioFeature->getNumCoefficients();
    current_feature             = pkm::Mat(1, numMFCCs);				//= (float *)malloc(sizeof(float) * numMFCCs);
    
    feature_background_buffer	= pkm::Mat();//pkm::Mat(NUM_BACK_BUFFERS_FOR_FEATURE_ANALYSIS, numMFCCs);
    feature_foreground_buffer	= pkm::Mat();//NUM_FORE_BUFFERS_FOR_FEATURE_ANALYSIS, numMFCCs);
    background_distance_buffer	= pkm::Mat(NUM_BUFFERS_FOR_SEGMENTATION_ANALYSIS, 1);
    foreground_distance_buffer	= pkm::Mat(NUM_BUFFERS_FOR_SEGMENTATION_ANALYSIS, 1);
    feature_background_average	= pkm::Mat(1, numMFCCs);
    feature_foreground_average	= pkm::Mat(1, numMFCCs);
    
    bSegmenting					= false;
    bSegmented					= false;
    bDraw						= false;
	
	SEGMENT_THRESHOLD			= 2.0f;
    
    // recorded segment
    audioSegment				= pkm::Mat();
    audioBackgroundSegment		= pkm::Mat();
}

pkmAudioSegmenter::~pkmAudioSegmenter()
{
    delete audioFeature;
}

void pkmAudioSegmenter::resetBackgroundModel()
{
    feature_background_buffer = pkm::Mat();//	= pkm::Mat(NUM_BACK_BUFFERS_FOR_FEATURE_ANALYSIS, numMFCCs);
}

bool pkmAudioSegmenter::isSegmenting()
{
    return bSegmenting;
}

void pkmAudioSegmenter::resetSegment()
{
    audioSegment = pkm::Mat();
    bSegmented = false;
}

bool pkmAudioSegmenter::detectedOnset()
{
    if (feature_background_buffer.rows > 0)
    {
        // find the average of the past N feature frames
        feature_background_average = feature_background_buffer.mean();
        
        // get the distance from the current frame and the previous N frame's average 
        // (note this can be any metric)
        float distance = 
        distanceMetric(feature_background_average.data, 
                       &current_feature[0], 
                       feature_background_average.cols);
        
        // add the current distance to the previous M distances buffer
        background_distance_buffer.insertRowCircularly(&distance);
        
        // calculate the mean and deviation for analysis
        float mean_distance = pkm::Mat::mean(background_distance_buffer.data, background_distance_buffer.rows);
        float std_distance = sqrtf(fabs(pkm::Mat::var(background_distance_buffer.data, background_distance_buffer.rows)));		
        
        //printf("distance: %f, mean_distance: %f, std_distance: %f\n", distance, mean_distance, std_distance);
        // if it is an outlier, then we have detected an event
        if ( std_distance > 0 && (fabs(distance - mean_distance) - SEGMENT_THRESHOLD*std_distance) > 0)
        {    
            //printf("ONSET\n");
            return true;
        }
        else {
            //printf("NO ONSET\n");
            return false;
        }

    }   
    else {
        return false;
    }

}

bool pkmAudioSegmenter::update() 
{
    bStartedSegment = false;
    if (feature_background_buffer.rows > 0)
    {
        // find the average of the past N feature frames
        feature_background_average = feature_background_buffer.mean();
        
        // get the distance from the current frame and the previous N frame's average 
        // (note this can be any metric)
        float distance = 
        distanceMetric(feature_background_average.data, 
                       &current_feature[0], 
                       feature_background_average.cols);
        
        // if we aren't segmenting, add the current distance to the previous M distances buffer
        if (!bSegmenting) {
            // add the current distance to the previous M distances buffer
            background_distance_buffer.insertRowCircularly(&distance);
            
        }
        
        // calculate the mean and deviation for analysis
        float mean_distance = pkm::Mat::mean(background_distance_buffer.data, background_distance_buffer.rows);
        float std_distance = sqrtf(fabs(pkm::Mat::var(background_distance_buffer.data, background_distance_buffer.rows)));		
        
        //printf("distance: %f, mean_distance: %f, std_distance: %f\n", distance, mean_distance, std_distance);
        
        // if it is an outlier, then we have detected an event
        if (!bSegmenting && 
            (fabs(distance - mean_distance) - SEGMENT_THRESHOLD*std_distance) > 0)
        {
            bSegmenting = true;
            bStartedSegment = true;
            
            feature_foreground_buffer = pkm::Mat();
            feature_foreground_buffer.push_back(current_feature);
            foreground_distance_buffer.reset(NUM_FORE_BUFFERS_FOR_FEATURE_ANALYSIS, 1);
        }
        // we are segmenting, check for conditions to stop segmenting if we are
        // passed the minimum segment length
        else if(bSegmenting && 
                (audioSegment.rows * audioSegment.cols) > MIN_SEGMENT_LENGTH)
        {
            // too similar to the original background, no more event
            if( (fabs(distance - mean_distance) - 0.3f*std_distance) < 0 )
            {
                bSegmenting = false;
                bSegmented = true;
            }
            // too big a file, stop segmenting
            else if((audioSegment.rows * audioSegment.cols) >= MAX_SEGMENT_LENGTH)
            {
                //resetBackgroundModel();
                bSegmenting = false;
                bSegmented = true;
            }
            // 
            else 
            {
                // find the average of the past N feature frames
                feature_foreground_average = feature_foreground_buffer.mean();
                
                // get the distance from the current frame and the 
                // previous N frame's average (note this can be any metric)
                float fore_distance = distanceMetric(feature_foreground_average.data, 
                                                     &current_feature[0], 
                                                     feature_foreground_average.cols);
                
                // if we aren't segmenting, add the current distance to the previous M distances buffer
                foreground_distance_buffer.insertRowCircularly(&distance);
                
                // calculate the mean and deviation for analysis
                float mean_fore_distance = 
                pkm::Mat::mean(foreground_distance_buffer.data, 
                               foreground_distance_buffer.rows);
                float std_fore_distance = 
                sqrtf(fabs(pkm::Mat::var(foreground_distance_buffer.data, 
                                         foreground_distance_buffer.rows)));	
                
                if (// foreground_distance_buffer.bCircularInsertionFull && 
                    (fabs(fore_distance - mean_fore_distance) - SEGMENT_THRESHOLD*std_fore_distance) > 0)
                {
                    bSegmenting = false;
                    bSegmented = true;
                }
            }
        }
    }
    else {
        // find the average of the past N feature frames
        feature_background_average = feature_background_buffer.sum() / (float)NUM_BACK_BUFFERS_FOR_FEATURE_ANALYSIS;
        
        // get the distance from the current frame and the previous N frame's average (note this can be any metric)
        float distance = pkm::Mat::sumOfAbsoluteDifferences(feature_background_average.data, 
                                                            &current_feature[0], 
                                                            feature_background_average.cols);
        
        // if we aren't segmenting, add the current distance to the previous M distances buffer
        if (!bSegmenting) {
            background_distance_buffer.insertRowCircularly(&distance);
        }
    }
    
    return bSegmented;
}

// get the last recorded segment (if updateAudio() == true)
void pkmAudioSegmenter::getSegmentAndFeatures(float *&buf, int &buf_size, float *&features, int &feature_size)
{
    if (!bSegmented) {
        printf("[ERROR]: Should only call this function once and only if update() returns true!");
        return;
    }
    
    buf_size = audioSegment.rows * audioSegment.cols;
    //printf("segmenting %d samples\n", buf_size);
    buf = (float *)malloc(sizeof(float) * buf_size);
    cblas_scopy(buf_size, audioSegment.data, 1, buf, 1);
    audioSegment = pkm::Mat();
    
    feature_size = numMFCCs;
    features = (float *)malloc(sizeof(float) * feature_size);
    cblas_scopy(feature_size, feature_foreground_average.data, 1, features, 1);
    //cblas_scopy(feature_size, feature_foreground_buffer.data, 1, features, 1);
    
    bSegmented = false;
}

// get the last recorded segment (if updateAudio() == true)
void pkmAudioSegmenter::getSegmentAndFeatures(vector<float> &buf, vector<float> &features)
{
    if (!bSegmented) {
        printf("[ERROR]: Should only call this function once and only if update() returns true!");
        return;
    }
    //printf("segmenting %d samples\n", buf_size);
    int bufSize = audioSegment.rows * audioSegment.cols;
    buf.resize(bufSize);
    cblas_scopy(bufSize, audioSegment.data, 1, &buf[0], 1);
    audioSegment = pkm::Mat();
    audioBackgroundSegment = pkm::Mat();
    
    features.resize(numMFCCs);
    //cblas_scopy(numMFCCs, feature_foreground_average.data, 1, &features[0], 1);
    cblas_scopy(numMFCCs, feature_foreground_buffer.data, 1, &features[0], 1);
    
    bSegmented = false;
}

// get the last recorded segment (if updateAudio() == true)
void pkmAudioSegmenter::getSegment(float *&buf, int &buf_size)
{
    if (!bSegmented) {
        printf("[ERROR]: Should only call this function once and only if update() returns true!");
        return;
    }
    buf_size = audioSegment.rows * audioSegment.cols;
    //printf("segmenting %d samples\n", buf_size);
    buf = (float *)malloc(sizeof(float) * buf_size);
    cblas_scopy(buf_size, audioSegment.data, 1, buf, 1);
    audioSegment = pkm::Mat();
    
    bSegmented = false;
}

pkm::Mat pkmAudioSegmenter::getFeatureSequence()
{
    pkm::Mat feature = feature_background_buffer;
    
    audioBackgroundSegment = pkm::Mat();
    feature_background_buffer = current_feature;
    
    return feature;
    
}

void pkmAudioSegmenter::getCurrentFeatureFrame(vector<float> &features)
{
    audioBackgroundSegment = pkm::Mat();
    
    features.resize(numMFCCs);
    //cblas_scopy(numMFCCs, feature_background_average.data, 1, &features[0], 1);
    cblas_scopy(numMFCCs, current_feature.data, 1, &features[0], 1);
    
    feature_background_buffer = current_feature;
}

void pkmAudioSegmenter::getBackgroundSegmentAndFeatures(float *&buf, int &segmentSize, float *&features, int &featureSize)
{
    //printf("segmenting %d samples\n", buf_size);
    segmentSize = audioBackgroundSegment.rows * audioBackgroundSegment.cols;
    featureSize = numMFCCs;
    buf = (float *)malloc(sizeof(float) * segmentSize);
    cblas_scopy(segmentSize, audioBackgroundSegment.data, 1, buf, 1);
    audioBackgroundSegment = pkm::Mat();
    
    features = (float *)malloc(sizeof(float) * numMFCCs);
    //cblas_scopy(numMFCCs, feature_background_average.data, 1, &features[0], 1);
    cblas_scopy(numMFCCs, feature_background_buffer.data, 1, features, 1);
    
    feature_background_buffer = current_feature;
}

void pkmAudioSegmenter::getBackgroundSegmentAndFeatures(vector<float> &buf, vector<float> &features)
{
    //printf("segmenting %d samples\n", buf_size);
    int bufSize = audioBackgroundSegment.rows * audioBackgroundSegment.cols;
    buf.resize(bufSize);
    cblas_scopy(bufSize, audioBackgroundSegment.data, 1, &buf[0], 1);
    audioBackgroundSegment = pkm::Mat();
    
    features.resize(numMFCCs);
    //cblas_scopy(numMFCCs, feature_background_average.data, 1, &features[0], 1);
    cblas_scopy(numMFCCs, feature_background_buffer.data, 1, &features[0], 1);
    
    feature_background_buffer = current_feature;
}


// update the circular buffer detecting segments each update()
void pkmAudioSegmenter::audioReceived(float *input, int bufferSize, int nChannels)
{
    audioFeature->computeMFCCF(input, current_feature.data, numMFCCs);
    
    if (!current_feature.isNaN()) {
        // store as foreground
        if (bSegmenting) {
            // store the feature
            if (feature_foreground_buffer.rows > NUM_FORE_BUFFERS_FOR_FEATURE_ANALYSIS) {
                feature_foreground_buffer.insertRowCircularly(current_feature.data);
            }
            else {
                feature_foreground_buffer.push_back(current_feature);
            }
            // store the audio
            if (audioSegment.rows > NUM_FORE_BUFFERS_FOR_FEATURE_ANALYSIS) {
                audioSegment.insertRowCircularly(input);
            }
            else {
                audioSegment.push_back(input, bufferSize);
            }
            
        }
        // store as background
        else {
            // store the feature
            if (feature_background_buffer.rows > NUM_BACK_BUFFERS_FOR_FEATURE_ANALYSIS) {
                feature_background_buffer.insertRowCircularly(current_feature.data);
            }
            else {
                feature_background_buffer.push_back(current_feature);
            }
            // store the audio
            if (audioBackgroundSegment.rows > NUM_BACK_BUFFERS_FOR_FEATURE_ANALYSIS) {
                audioBackgroundSegment.insertRowCircularly(input);
            }
            else {
                audioBackgroundSegment.push_back(input, bufferSize);
            }
        }
        /*
        printf("fg size: %d, bg size: %d, fg buffer: %d, bg buffer: %d, bg dist: %d, fg dist: %d\n", 
               audioSegment.rows,
               audioBackgroundSegment.rows,
               feature_foreground_buffer.rows,
               feature_background_buffer.rows,
               background_distance_buffer.rows,
               foreground_distance_buffer.rows);
        */
    }
}
