/*
 *  pkmAudioFeatureNormalizer.h
 *  audioVideoMosaic
 *
 *  Created by Mr. Magoo on 12/2/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */
#pragma once
#include "pkmMatrix.h"

class pkmAudioFeatureNormalizer
{
public:
	pkmAudioFeatureNormalizer(int featureSize = 64);
	void addExample(float *descriptor, int descriptor_size);
    void setMaxExamples(int max);
	void calculateNormalization();
	void normalizeFeature(float *descriptor, int descriptor_size);
    void saveNormalization();
    void loadNormalization();
	void reset();
    
	static void normalizeDatabase(pkm::Mat &database);
	
	bool bCalculated;
	int feature_size;
	pkm::Mat examples;
	pkm::Mat mean_vec, 
			var_vec;
    
    int max_examples;
};