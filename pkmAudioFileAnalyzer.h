/*
 *  pkmAudioFileAnalyzer.h
 *  autoGrafittiMapKit
 *
 *  Created by Parag K. Mital - http://pkmital.com 
 *  Contact: parag@pkmital.com
 *
 *  Copyright 2011 Parag K. Mital. All rights reserved.
 * 

 
 LICENSE:
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 
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
