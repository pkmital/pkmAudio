/*
 *  pkmAudioSegmentDatabase.h
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
#include "pkmMatrix.h"
#include "pkmAudioSegment.h"
#include "pkmEXTAudioFileReader.h"
#include "flann.h"
#include <Accelerate/Accelerate.h>
#include "ofUtils.h"
#include "ofTypes.h"

// recreate without flann, use vector of mat's for featuredatabase, and use dtw for matching
// could try and do timestretching based on matched path...

//#define WITH_FLANN
//#define WITH_COSINE_DISTANCE

class pkmAudioSegmentDatabase
{
public:
	pkmAudioSegmentDatabase()
	{
        currentIdx      = 0;
        maxObjects      = -1;
		bBuiltIndex     = false;
        totalSamples    = 0;
        k				= 1;												// number of nearest neighbors
		nnIdx			= (int *)malloc(sizeof(int)*k);						// allocate near neighbor indices
		dists			= (float *)malloc(sizeof(float)*k);					// allocate near neighbor dists	
        
#ifdef WITH_FLANN
        //query			= (float *)malloc(sizeof(float)*feature_length);	// pre-allocate a query frame
		flannParams = DEFAULT_FLANN_PARAMETERS;
		flannParams.algorithm = FLANN_INDEX_LINEAR;//FLANN_INDEX_AUTOTUNED;//FLANN_INDEX_LINEAR;//FLANN_INDEX_KMEANS;// FLANN_INDEX_LINEAR;//FLANN_INDEX_AUTOTUNED;//FLANN_INDEX_LSH;//FLANN_INDEX_AUTOTUNED; // FLANN_INDEX_LSH; // FLANN_INDEX_AUTOTUNED;// FLANN_INDEX_KDTREE; //FLANN_INDEX_AUTOTUNED; //FLANN_INDEX_KDTREE;// FLANN_INDEX_LINEAR; // FLANN_INDEX_AUTOTUNED; // or FLANN_INDEX_KDTREE, FLANN_INDEX_KMEANS
        flannParams.target_precision = 0.99; 
                                                //        flannParams.sample_fraction = 0.25;
        flann_set_distance_type(FLANN_DIST_EUCLIDEAN, 2);
#endif
        
	}
	~pkmAudioSegmentDatabase()
	{
        if(bBuiltIndex)
		{
#ifdef WITH_FLANN
			flann_free_index(kdTree, &flannParams);
#endif
			bBuiltIndex = false;
		}
        
        vector<ofPtr<pkmAudioSegment> >::iterator it = audioDatabase.begin();
        while (it != audioDatabase.end()) {
            it->reset();
            it = audioDatabase.erase(it);
        }
        it = audioDatabaseToDelete.begin();
        while (it != audioDatabaseToDelete.end()) {
            it->reset();
            it = audioDatabaseToDelete.erase(it);
        }
		
		free(nnIdx);
		free(dists);
	}
    
    void resetDatabase()
    {
        vector<ofPtr<pkmAudioSegment> >::iterator it = audioDatabase.begin();
        while (it != audioDatabase.end()) {
            it->reset();
            it = audioDatabase.erase(it);
        }
        
		if (bBuiltIndex) {
#ifdef WITH_FLANN
			flann_free_index(kdTree, &flannParams);
#endif
		}
        featureDatabase = pkm::Mat();
        bBuiltIndex = false;
    }
    
    
    long long getLength()
    {
        return totalSamples;
    }
    
    int getSize()
    {
        return featureDatabase.rows;
    }
	
	void setK(int kNeighbors)
	{
		k = MAX(1,kNeighbors);
		k = MIN(20,k);
        
		nnIdx			= (int *)realloc(nnIdx, sizeof(int)*k);						// allocate near neighbor indices
		dists			= (float *)realloc(dists, sizeof(float)*k);					// allocate near neighbor dists	
	}
	
    bool bShouldAddSegment(float *data)
    {
        if(!bBuiltIndex)
            return true;
#ifdef WITH_FLANN
        if (flann_find_nearest_neighbors_index_float(kdTree, 
													 data, 
													 1, 
													 nnIdx, 
													 dists, 
													 k, 
													 &flannParams) < 0)
		{
            return true;
        }
        else if(dists[0] > 0.0001)
        {
            return true;
        }
        else 
        {
            printf("[pkmAudioSegmentDatabase]: Too similar to database, not adding: %f\n", dists[0]);
            return false;
        }
#else
        if (featureDatabase.rows > 0) 
        {
            float *testData = data;
            float bestSum = HUGE_VAL;
            int bestIdx = 0;
            int i = 0;
            
            while(i < featureDatabase.rows) {
                float *curData = featureDatabase.row(i);

                float sum = 0;
                int j = 0;
                
                while (j < featureDatabase.cols) {
                    float a = *curData++ - testData[j];
                    sum += sqrtf(a*a);
                    j++;
                }
                
                if (sum < bestSum) {
                    bestSum = sum;
                }
                
                i++;
            }
            //cout << "bestSum: " << bestSum << endl;
            return bestSum > 0.10;
        }
        else {
            return true;
        }
#endif
    }
    
    bool bShouldAddSequence(pkm::Mat featureSequence)
    {
        if(!bBuiltIndex)
            return true;
#ifdef WITH_FLANN
        if (flann_find_nearest_neighbors_index_float(kdTree, 
													 featureSequence.data, 
													 1, 
													 nnIdx, 
													 dists, 
													 k, 
													 &flannParams) < 0)
		{
            return true;
        }
        else if(dists[0] > 0.0001)
        {
            return true;
        }
        else 
        {
            printf("[pkmAudioSegmentDatabase]: Too similar to database, not adding: %f\n", dists[0]);
            return false;
        }
#else
        if (featureSequence.rows < featureDatabase.rows) 
        {
            int i = 0;
            int numFeatureSequencesToTest = featureDatabase.rows - featureSequence.rows;
            int featureSequenceLength = featureSequence.rows*featureSequence.cols;
            double minDist = HUGE_VAL;
            int minIdx = 0;
            while(i < numFeatureSequencesToTest) {
#ifdef WITH_COSINE_DISTANCE
                double thisDist = cosineDistance(featureSequence.data, featureDatabase.row(i), featureSequenceLength);
#else
                double thisDist = pkm::Mat::l1norm(featureSequence.data, featureDatabase.row(i), featureSequenceLength);
#endif
                if(thisDist < minDist)
                {
                    minIdx = i;
                    minDist = thisDist;
                }
                i++;
            }
            //cout << "dist: " << minDist << endl;
            return minDist > 0.01;
        }
        else {
            return true;
        }
#endif
    }
    
    void addAudioSegment(ofPtr<pkmAudioSegment> segment, float *descriptor, int descriptor_size)
    {
        if (maxObjects > 0 && featureDatabase.rows > maxObjects)
        {
            totalSamples -= (audioDatabase[currentIdx]->offset - audioDatabase[currentIdx]->onset);
            totalSamples += (segment->offset - segment->onset);
            audioDatabase[currentIdx].reset();
            audioDatabase[currentIdx] = segment;
            memcpy(featureDatabase.row(currentIdx), descriptor, descriptor_size*sizeof(float));
            currentIdx = (currentIdx + 1) % maxObjects;
        }
        else   
        {
            audioDatabase.push_back(segment);
            totalSamples += (segment->offset - segment->onset);
            featureDatabase.push_back(descriptor, descriptor_size);
        }
    }
    
    void addAudioSequence(ofPtr<pkmAudioSegment> segment, pkm::Mat featureSequence)
    {
        audioDatabase.push_back(segment);
        totalSamples += (segment->offset - segment->onset);
        featureDatabase.push_back(featureSequence);
        
        float currentLookup = audioDatabase.size();
        pkm::Mat lookupTable(featureSequence.rows, 1, currentLookup);
        databaseLookupTable.push_back(lookupTable);
    }
    
	// rebuild the index after segmenting
	void buildIndex()
	{		
#ifdef WITH_FLANN
		if(bBuiltIndex)
		{
			flann_free_index(kdTree, 
							 &flannParams);
			bBuiltIndex = false;
		}
        
        //printf("Building FLANN Index...");
		float speedup;
		kdTree = flann_build_index(featureDatabase.data, 
								   featureDatabase.rows, 
								   featureDatabase.cols, 
								   &speedup, 
								   &flannParams);
        //printf("[OK].\n");
        //printf("Speed up of %f over linear indexing.\n", speedup);
#endif
        
		bBuiltIndex = true;	
              
	}
	
	vector<ofPtr<pkmAudioSegment> > getNearestAudioSegments(ofPtr<pkmAudioSegment> segment)
	{
		vector<ofPtr<pkmAudioSegment> > nearestAudioSegments;
		int i;
		if (!bBuiltIndex) {
			//printf("[ERROR] First build the index with buildIndex()");
			return nearestAudioSegments;
		}
#ifdef WITH_FLANN
		if (flann_find_nearest_neighbors_index_float(kdTree, 
													 segment->descriptor.data, 
													 1, 
													 nnIdx, 
													 dists, 
													 k, 
													 &flannParams) < 0)
		{
			printf("[ERROR] No frames found for Nearest Neighbor Search!\n");
			return nearestAudioSegments;
		}
#else
        float bestSum = HUGE_VAL;
        int bestIdx = 0;
        i = 0;
        float *testData = segment->descriptor.data;
        while(i < featureDatabase.rows) {
            float *curData = featureDatabase.row(i);
            float sum = 0;
            int j = 0;
            while (j < featureDatabase.cols) {
                float a = *curData++ - testData[j];
                sum += (a*a);
                j++;
            }
            if (sum < bestSum) {
                bestSum = sum;
                bestIdx = i;
            }
            i++;
        }
        
        nnIdx[0] = bestIdx;
        dists[0] = bestSum;
#endif
		
		float sumDists = 0;
		i = 0;
		while (i < k) {
			sumDists += dists[i];
			i++;
		}
		
		for (int i = 0; i < k; i++) 
		{
            if (nnIdx[i] < audioDatabase.size()) {
                //printf("i-th idx: %d, dist: %f\n", nnIdx[i], dists[i]);
                ofPtr<pkmAudioSegment> p = (audioDatabase[nnIdx[i]]);

                if (!p->bPlaying) {
                    p->frame = 0;
                    p->bPlaying = true;
                    nearestAudioSegments.push_back(p);
                }
            }
            
            //else {
            //    printf("[ERROR] flann reutrned index %d but size of database is %d\n", nnIdx[i], audioDatabase.size());
            //    return nearestAudioSegments;
            //}
            
		}
        
		return nearestAudioSegments;
	}
    
    //vector<pkmAudioSegment> getNearestSequenceMatch(float *descriptor, int num = 1);
    
    vector<ofPtr<pkmAudioSegment> > getNearestAudioSegments(float *descriptor)
	{
		vector<ofPtr<pkmAudioSegment> > nearestAudioSegments;
		int i;
		if (!bBuiltIndex) {
			//printf("[ERROR] First build the index with buildIndex()");
			return nearestAudioSegments;
		}
#ifdef WITH_FLANN
		if (flann_find_nearest_neighbors_index_float(kdTree, 
													 descriptor, 
													 1, 
													 nnIdx, 
													 dists, 
													 k, 
													 &flannParams) < 0)
		{
			printf("[ERROR] No frames found for Nearest Neighbor Search!\n");
			return nearestAudioSegments;
		}
#else
        float bestSum = HUGE_VAL;
        int bestIdx = 0;
        i = 0;
        float *testData = descriptor;
        while(i < featureDatabase.rows) {
            float *curData = featureDatabase.row(i);
            float sum = 0;
            int j = 0;
            while (j < featureDatabase.cols) {
                float a = *curData++ - testData[j];
                sum += (a*a);
                j++;
            }
            //cout << "i: " << i << " " << sum << endl;
            if (sum < bestSum) {
                bestSum = sum;
                bestIdx = i;
            }
            i++;
        }
        
        nnIdx[0] = bestIdx;
        dists[0] = bestSum;
#endif
		
		float sumDists = 0;
		i = 0;
		while (i < k) {
			sumDists += dists[i];
			i++;
		}
		
		for (int i = 0; i < k; i++) 
		{
            if (nnIdx[i] < audioDatabase.size()) {
                //printf("K: %d, i-th idx: %d, dist: %f\n", i, nnIdx[i], dists[i]);
                ofPtr<pkmAudioSegment> p = (audioDatabase[nnIdx[i]]);
                if (!p->bPlaying) {
                    p->frame = 0;
                    p->bPlaying = true;
                    nearestAudioSegments.push_back(p);
                }
                //else {
                //    p->bNeedsReset = true;
                //}
            }
        }
		return nearestAudioSegments;
	}
    
    vector<ofPtr<pkmAudioSegment> > getNearestAudioSequences(pkm::Mat featureSequence)
    {
        vector<ofPtr<pkmAudioSegment> > nearestAudioSegments;
        int i;
        if (!bBuiltIndex) {
            //printf("[ERROR] First build the index with buildIndex()");
            return nearestAudioSegments;
        }
#ifdef WITH_FLANN
        if (flann_find_nearest_neighbors_index_float(kdTree, 
                                                     descriptor, 
                                                     1, 
                                                     nnIdx, 
                                                     dists, 
                                                     k, 
                                                     &flannParams) < 0)
        {
            printf("[ERROR] No frames found for Nearest Neighbor Search!\n");
            return nearestAudioSegments;
        }
        
        for(i = 0; i < k; i++)
        {
            nnIdx[i] = databaseLookupTable[nnIdx[i]];
        }
#else
        
        i = 0;
        int numFeatureSequencesToTest = featureDatabase.rows - featureSequence.rows;
        int featureSequenceLength = featureSequence.rows*featureSequence.cols;
        double minDist = HUGE_VAL;
        int minIdx = 0;
        while(i < numFeatureSequencesToTest) {
#ifdef WITH_COSINE_DISTANCE
            double thisDist = cosineDistance(featureSequence.data, featureDatabase.row(i), featureSequenceLength);
#else
            double thisDist = pkm::Mat::l1norm(featureSequence.data, featureDatabase.row(i), featureSequenceLength);
#endif
            if(thisDist < minDist)
            {
                nnIdx[0] = databaseLookupTable[i];
                minDist = thisDist;
            }
            i++;
        }
        
        //featureSequence.print();
        //cout << "idx: " << nnIdx[0] << " dist: " << minDist << endl;
        
        /*
        i = 0;
        int numFeatureSequencesToTest = featureDatabase.rows - featureSequence.rows + 1;
        int featureSequenceLength = featureSequence.rows * featureSequence.cols;
        pkm::Mat distances(numFeatureSequencesToTest, 1, 1.0f);
        while(i < numFeatureSequencesToTest) {
            distances[i] = cosineDistance(featureSequence.data, featureDatabase.row(i), featureSequenceLength);
            i++;
        }
        
        for(i = 0; i < k; i++)
        {
            nnIdx[i] = pkm::Mat::minIndex(distances);
            dists[i] = distances[nnIdx[i]];
            distances[nnIdx[i]] = 1.0;
            nnIdx[i] = databaseLookupTable[nnIdx[i]];
        }
        */
#endif
        
        for (int i = 0; i < k; i++) 
        {
            if (nnIdx[i] < audioDatabase.size()) {
                //printf("K: %d, i-th idx: %d, dist: %f\n", i, nnIdx[i], dists[i]);
                ofPtr<pkmAudioSegment> p = (audioDatabase[nnIdx[i]]);
                if (!p->bPlaying) {
                    p->frame = 0;
                    p->bPlaying = true;
                    nearestAudioSegments.push_back(p);
                }
                //else {
                //    p->bNeedsReset = true;
                //}
            }
        }
        return nearestAudioSegments;
    }
    
    void saveIndex()
    {
        
        if (bBuiltIndex) {
            char buf[256];
            sprintf(buf, "%s", ofToDataPath("flann_index.mat").c_str());
#ifdef WITH_FLANN
            if(flann_save_index(kdTree, buf))
                printf("[ERROR] Could not save flann index!\n");
            else
#endif
                printf("[OK] Saved database with %d features\n", featureDatabase.rows);
        }
         
    }
    
    void loadIndex()
    {
        
        char buf[256];
        sprintf(buf, "%s", ofToDataPath("flann_index.mat").c_str());
#ifdef WITH_FLANN
        kdTree = flann_load_index(buf, 
                                  featureDatabase.data, 
                                  featureDatabase.rows, 
                                  featureDatabase.cols);
#endif
        printf("[OK] Loaded database with %d features\n", featureDatabase.rows);
        
        bBuiltIndex = true;
         
    }
	
    void save()
    {
        featureDatabase.save(ofToDataPath("audio_database.mat"));
        databaseLookupTable.save(ofToDataPath("audio_table.mat"));
        FILE *fp;
        fp = fopen(ofToDataPath("parameters.txt").c_str(), "w");
        //fprintf(fp, "feature_length: %d\n", feature_length);
        fprintf(fp, "k: %d\n", k);
        fclose(fp);
        
        FILE *fp2;
        fp2 = fopen(ofToDataPath("audio_database.txt").c_str(), "w");
        fprintf(fp2, "%ld\n", audioDatabase.size());
        printf("[OK] Saving %lu segments\n", audioDatabase.size());
        for(vector<ofPtr<pkmAudioSegment> >::iterator it = audioDatabase.begin();
            it != audioDatabase.end();
            it++)
        {
            char buf[512];
            sprintf(buf, "%s", (*it)->filename.c_str());
            fprintf(fp2, "%s %ld %ld %ld\n", buf, (*it)->onset, (*it)->offset, (*it)->index);
            //printf("saved segment %ld - %ld from file %s\n", it->onset, it->offset, buf);
        }
        fclose(fp2);
        
    }
    
    void load(bool bLoadBuffer = false)
    {
        featureDatabase.load(ofToDataPath("audio_database.mat"));
        databaseLookupTable.load(ofToDataPath("audio_table.mat"));
        FILE *fp;
        fp = fopen(ofToDataPath("parameters.txt").c_str(), "r");
        //fscanf(fp, "feature_length: %d\n", &feature_length);
        fscanf(fp, "k: %d", &k);
        fclose(fp);
        
        FILE *fp2;
        fp2 = fopen(ofToDataPath("audio_database.txt").c_str(), "r");
        unsigned long database_size = 0;
        fscanf(fp2, "%ld\n", &database_size);
        printf("[OK] Loading %d segments\n", database_size);
        for(unsigned long i = 0; i < database_size; i++)
        {
            char buf[512];
            unsigned long onset, offset, index;
            vector<float> desc;
            fscanf(fp2, "%s %ld %ld %ld\n", buf, &onset, &offset, &index);
            ofPtr<pkmAudioSegment> p(new pkmAudioSegment(buf, onset, offset, desc, index, 0));
            if (bLoadBuffer) {
                pkmEXTAudioFileReader audioFile;
                if(audioFile.open(ofToDataPath(buf)))
                {
                    p->buffer = (float *)malloc(sizeof(float)*audioFile.mNumSamples);
                    p->bStoringAudioData = true;
                    audioFile.read(p->buffer, 0, audioFile.mNumSamples);
                    audioDatabase.push_back(p);
                }
                else {
                    cout << "Could not open: " << buf << endl;
                }
            }
            else {
                audioDatabase.push_back(p);
            }
            //            (*it)->load(&fp2);
            //printf("loaded segment %ld - %ld from file %s\n", onset, offset, buf);
        }
        fclose(fp2);
        
        //free(query);
        //query = (float *)malloc(sizeof(float)*feature_length);	// pre-allocate a query frame
        
        
        databaseMatches = pkm::Mat(1, featureDatabase.rows, true);
        //databaseMatches = pkm::Mat(featureDatabase.rows+1, featureDatabase.cols, true);
    }
	
    void setMaxObjects(int maxObj)
    {
        maxObjects = maxObj;
    }
    
    vector<ofPtr<pkmAudioSegment> >           audioDatabaseToDelete;
	vector<ofPtr<pkmAudioSegment> >           audioDatabase;
	int                                 feature_length;
	// For kNN - using C-version of FlANN
	pkm::Mat							featureDatabase;
    pkm::Mat                            databaseMatches;
    pkm::Mat                            databaseLookupTable;
    
    long long                           totalSamples;
    
    int                                 maxObjects, currentIdx;
    
	int									*nnIdx;			// near neighbor indices of the featureDatabase
	float								*dists;			// near neighbor distances from the query to each featureDatabase member
#ifdef WITH_FLANN
	flann_index_t						kdTree;			// kd-tree reference representing all the feature-frames
	struct FLANNParameters				flannParams;	// index parameters are stored here
#endif
	
	int									k;				// number of nearest neighbors
	
	int									numFeatures,	// dimension of each point
                                        numFrames;		// number of feature point frames
    
	bool								bBuiltIndex;
    
    
    void sequenceMatch(
                       float* V, /* current input sequence */
                       float* M, /* output convolution */
                       float* D, /* database sequence */
                       int w, /* input sequence length */
                       int n, /* database length */
                       int a, /* low element selector */
                       int b, /* high element selector */
                       int d);
    
    inline float cosineDistance(float *x, float *y, unsigned int count) {
        float dotProd, magX, magY;
        float *tmp = (float*)malloc(count * sizeof(float));
        
        vDSP_dotpr(x, 1, y, 1, &dotProd, count);
        
        vDSP_vsq(x, 1, tmp, 1, count);
        vDSP_sve(tmp, 1, &magX, count);
        magX = sqrtf(magX);
        
        vDSP_vsq(y, 1, tmp, 1, count);
        vDSP_sve(tmp, 1, &magY, count);
        magY = sqrtf(magY);
        
        free(tmp);
        tmp = NULL;
        
        return 1.0 - (dotProd / (magX * magY));
    }
};