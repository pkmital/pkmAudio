/*
 *  pkmAudioSegmenter.h
 *  Created by Parag K. Mital - http://pkmital.com 
 *  Contact: parag@pkmital.com
 *
 *  Copyright 2011 Parag K. Mital. All rights reserved.
 * 
 
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

#pragma once

#include <Accelerate/Accelerate.h>
#include "pkmAudioFeatures.h"
#include "pkmMatrix.h"
#include "pkmCircularRecorder.h"

const int SAMPLE_RATE = 44100;
const int MAX_SEGMENT_LENGTH = SAMPLE_RATE*5;
const bool bFrameSegmentation = true;

class pkmAudioSegmenter
{
public:
	float MIN_SEGMENT_LENGTH;
	// buffer size refers to the length of the buffer to average over
	// frame size is each audio chunk
	pkmAudioSegmenter(int frameSize = 512, int fftSize = 512);
	~pkmAudioSegmenter();
	
	void setSegmentationThreshold(float thresh)
	{
		SEGMENT_THRESHOLD = MAX(0, thresh);
        //MIN_SEGMENT_LENGTH = MAX(SAMPLE_RATE / 40, thresh * SAMPLE_RATE / 8);
	}
    
    void setMinSize(float len)
    {
        MIN_SEGMENT_LENGTH = len;
    }
	
	inline float distanceMetric(float *buf1, float *buf2, int size)
	{
		return pkm::Mat::sumOfAbsoluteDifferences(buf1, buf2, size);
        //return pkmAudioFeatures::cosineDistance(buf1, buf2, size);
	}
	
	void resetBackgroundModel();
	
	// given a new frame of audio, did we detect a segment?

    
    // For SEGMENTATION
    bool update();
	bool isSegmenting();
	bool isStartedSegmenting();
    
	void resetSegment();
	// get the last recorded segment (if updateAudio() == true)
	bool getSegmentAndFeatures(float *&buf, int &buf_size, float *&features, int &feature_size);    
    // get the last recorded segment (if updateAudio() == true)
	bool getSegmentAndFeatures(std::vector<float> &buf, std::vector<float> &features);
	// get the last recorded segment (if updateAudio() == true)
	bool getSegment(float *&buf, int &buf_size);
    bool getSegment(pkm::Mat &seg);

    // For ONSET DETECTION
    bool detectedOnset();
    void getCurrentFeatureFrame(std::vector<float> &features);
	void getBackgroundSegmentAndFeatures(std::vector<float> &buf, std::vector<float> &features);
    void getBackgroundSegmentAndFeatures(float *&buf, int &bufSize, float *&features, int &featureSize);
    void getBackgroundSegment(float *&buf, int &segmentSize);
    pkm::Mat getFeatureSequence();
	// update the circular buffer detecting segments each update()
	void audioReceived(float *input, int bufferSize, int nChannels);
    
    
    
    pkm::Mat				audioSegment,audioBackgroundSegment;
    
    pkmCircularRecorder     *ringBuffer;
    float                   *alignedBuffer;

	pkmAudioFeatures		*audioFeature;
    pkm::Mat                current_feature;
	
	int						numLFCCs, maxNumFrames;
	
    pkm::Mat                feature_buffer;
    pkm::Mat                feature_average;
	pkm::Mat				feature_background_buffer;
	pkm::Mat				feature_background_average;
	pkm::Mat				feature_foreground_buffer;
	pkm::Mat				feature_foreground_average;
	pkm::Mat				feature_deviation;
	pkm::Mat				background_distance_buffer;
	pkm::Mat				foreground_distance_buffer;
    
    float                   distance, mean_distance, std_distance;
    
	float					SEGMENT_THRESHOLD;
    
    int                     NUM_BACK_BUFFERS_FOR_FEATURE_ANALYSIS;// = SAMPLE_RATE*1/FRAME_SIZE;
    int                     NUM_FORE_BUFFERS_FOR_FEATURE_ANALYSIS;// = SAMPLE_RATE*1/FRAME_SIZE;
    int                     NUM_BUFFERS_FOR_SEGMENTATION_ANALYSIS;// = SAMPLE_RATE*3/FRAME_SIZE;
	
	bool					bSegmenting, bStartedSegment, bSegmented, bDraw;
};