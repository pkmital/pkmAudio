/*
 *  pkmAudioFeatureNormalizer.cpp
 *  audioVideoMosaic
 *
 *  Created by Mr. Magoo on 12/2/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "pkmAudioFeatureNormalizer.h"

pkmAudioFeatureNormalizer::pkmAudioFeatureNormalizer(int featureSize)
:
feature_size(featureSize)
{
	mean_vec = pkm::Mat(1, feature_size);
	var_vec = pkm::Mat(1, feature_size);
	bCalculated = false;
}

void pkmAudioFeatureNormalizer::addExample(float *descriptor, int descriptor_size)
{
    //examples.insertRowCircularly(descriptor);
	examples.push_back(descriptor, descriptor_size);
}

void pkmAudioFeatureNormalizer::calculateNormalization()
{
	if (examples.rows < 1) {
		printf("First add examples using addExample(float *descriptor)\n");
		return;
	}
	
    //if (examples.bCircularInsertionFull) {
        mean_vec = examples.mean(true);
        var_vec = examples.var(true);
    //}
    /*
    else {
        if (examples.current_row > 3) {
            printf("Not enough examples (only %d), will attempt anyway...\n", examples.current_row);
            pkm::Mat shortExamples = examples.rowRange(0, examples.current_row-1, false);
            mean_vec = shortExamples.mean(true);
            var_vec = shortExamples.var(true);
        }
        else {
            printf("Not enough examples (only %d)...\n", examples.current_row);
            return;
        }
    }
	*/
    printf("Database mean: \n");
    mean_vec.print();
    printf("Database var: \n");
    var_vec.print();
    
	bCalculated = true;
}

void pkmAudioFeatureNormalizer::normalizeFeature(float *descriptor, int descriptor_size)
{
	if (!bCalculated) {
		printf("First add examples using addExample(float *descriptor) and then calculate normalization using calculateNormalization()\n");
		return;
	}
	pkm::Mat desc(1, descriptor_size, descriptor, false);
	desc.subtract(mean_vec);
	desc.divide(var_vec);
}

void pkmAudioFeatureNormalizer::normalizeDatabase(pkm::Mat &database)
{
    static pkm::Mat mean_vec, var_vec, prev_mean_vec, prev_var_vec;
    
    mean_vec = database.mean(true);
    var_vec = database.var(true);
    
    if(prev_mean_vec.size())
    {
        mean_vec = mean_vec + prev_mean_vec;
        var_vec = var_vec + prev_var_vec;
    }
    else {
        prev_mean_vec = mean_vec;
        prev_var_vec = var_vec;
    }
	
    printf("Database mean: \n");
    mean_vec.print();
    printf("Database var: \n");
    var_vec.print();
    
	for (int i = 0; i < database.rows; i++) {
		pkm::Mat desc(1, database.cols, database.row(i), false);
		desc.subtract(mean_vec);
		desc.divide(var_vec);
	}
}

void pkmAudioFeatureNormalizer::saveNormalization()
{
    
    FILE *fp = fopen("pkmAudioFeatureNormalizer_params.txt", "w");
    if (fp != NULL) {
        fprintf(fp, "calculated: %d\n", (int)bCalculated);
        fprintf(fp, "max_examples: %d\n", max_examples);
        fprintf(fp, "feature_size: %d\n", feature_size);
        fclose(fp);
        
        mean_vec.save("pkmAudioFeatureNormalizer_mean_vec.mat");
        var_vec.save("pkmAudioFeatureNormalizer_var_vec.mat");
        examples.save("pkmAudioFeatureNormalizer_examples_vec.mat");
        
        printf("[OK] Saved audio normalization.\n");
    }
    else {
        printf("[ERROR] Could not save audio normalization!\n");
    }
}


void pkmAudioFeatureNormalizer::loadNormalization()
{
    FILE *fp = fopen("pkmAudioFeatureNormalizer_params.txt", "r");
    if (fp != NULL) {
        int calc = 0;
        fscanf(fp, "calculated: %d\n", &calc);
        fscanf(fp, "max_examples: %d\n", &max_examples);
        fscanf(fp, "feature_size: %d\n", &feature_size);
        fclose(fp);
        
        if (calc) {
            bCalculated = true;
        }
        
        mean_vec.load("pkmAudioFeatureNormalizer_mean_vec.mat");
        var_vec.load("pkmAudioFeatureNormalizer_var_vec.mat");
        examples.load("pkmAudioFeatureNormalizer_examples_vec.mat");
        
        printf("[OK] Loaded audio normalization.\n");
    }
    else {
        printf("[ERROR] No previous audio normalization exists.\n");
    }
}
void pkmAudioFeatureNormalizer::reset()
{
	examples = pkm::Mat();
	bCalculated = false;
}