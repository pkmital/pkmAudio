/*
 *  pkmAudioFileAnalyzer.h
 *  autoGrafittiMapKit
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
 */

#pragma once
#include <vector>
using namespace std;
#include "pkmAudioFeatures.h"
#include "pkmMatrix.h"
#include "pkmAudioFile.h"

class pkmAudioFileAnalyzer
{
public:
	// 44100 / 512 * 93 
	pkmAudioFileAnalyzer(int sample_rate = 44100, 
						 int fft_size = 512, 
						 int hop_size = 0,
						 int num_features = 32)			// not implemented
	{
		sampleRate = sample_rate;
		fftN = fft_size;
		mfccAnalyzer = new pkmAudioFeatures(sampleRate, fftN);
		numFeatures = num_features;
	}
	
	~pkmAudioFileAnalyzer()
	{
		delete mfccAnalyzer;
	}
	
	void analyzeFile(float *&buffer,					// in
					 int samples,						// in
					 vector<double *> &feature_matrix,	// out - allocated for you
					 vector<pkmAudioFile> &sound_lut,	// out
					 int &num_frames,					// out
					 int &num_features)					// out
	{
		num_frames = samples / fftN;
		num_features = numFeatures;
		//num_features = mfccAnalyzer->getNumCoefficients();
		for (int i = 0; i < num_frames; i++) 
		{
			//double *featureFrame = (double *)malloc(sizeof(double) * mfccAnalyzer->getNumCoefficients());
			double *featureFrame = (double *)malloc(sizeof(double) * numFeatures);
			mfccAnalyzer->computeMFCCD(buffer + i*fftN, featureFrame, numFeatures);
			feature_matrix.push_back(featureFrame);
			sound_lut.push_back(pkmAudioFile(buffer, i*fftN, samples));
		}
	}
	
	void analyzeFile(float *&buffer,					// in
					 int samples,						// in
					 vector<float *> &feature_matrix,	// out - allocated for you
					 vector<pkmAudioFile> &sound_lut,	// out
					 int &num_frames,					// out
					 int &num_features)					// out
	{
		num_frames = samples / fftN;
		num_features = numFeatures;
		//num_features = mfccAnalyzer->getNumCoefficients();
		for (int i = 0; i < num_frames; i++) 
		{
			//double *featureFrame = (double *)malloc(sizeof(double) * mfccAnalyzer->getNumCoefficients());
			float *featureFrame = (float *)malloc(sizeof(float) * numFeatures);
			mfccAnalyzer->computeMFCCF(buffer + i*fftN, featureFrame, numFeatures);
			feature_matrix.push_back(featureFrame);
			sound_lut.push_back(pkmAudioFile(buffer, i*fftN, samples));
		}
	}
	
	void analyzeFile(float *buffer,						// in		- the sound buffer
					 int samples,						// in		- length of the buffer (frame aligned!)
					 int num_frames,					// in		- total number of frames to analyse
					 float *& feature_matrix,			// in/out	- you've already allocated this as a matrix with known feature size
					 int num_features)					// in		- number of features per feature-frame
					 

	{
		int samplesPerFrame = samples / num_frames;
		for (int i = 0; i < num_frames; i++) 
		{
			float *featureFrame = feature_matrix + i*num_features;
			mfccAnalyzer->computeMFCCF(buffer + i*samplesPerFrame, 
									  featureFrame, 
									  num_features);
		}
	}

	void createLookupTable(float *buffer,					// in		- the sound buffer
						   int samples,						// in		- length of the buffer (frame aligned!)
						   int num_frames,					// in		- total number of frames to analyse
						   vector<pkmAudioFile> &sound_lut)	// out		- creates a lut of each sound-frame for you
	{
		int samplesPerFrame = samples / num_frames;
		for (int i = 0; i < num_frames; i++) 
		{
			sound_lut.push_back(pkmAudioFile(buffer, i*samplesPerFrame, samples));
		}
	}
	int						numFeatures;
	int						sampleRate, 
							fftN;
	pkmAudioFeatures		*mfccAnalyzer;
};
