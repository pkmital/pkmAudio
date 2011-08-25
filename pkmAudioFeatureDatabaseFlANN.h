/*
 *  pkmAudioFeatureDatabaseFlANN.h
 *  autoGrafittiMapKit
 *
 *  Created by Mr. Magoo on 5/12/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once
#include <vector>
using namespace std;
#include "pkmAudioFeatures.h"
#include "pkmAudioFileAnalyzer.h"
#include "pkmMatrix.h"
#include "pkmAudioFile.h"
#include <flann.h>
#include <Accelerate/Accelerate.h>

// include removal of sound from database and freeing memory
// segmentation based on average segment's distance to database
// write segments to extaf, and load from disk using ptr


// at 32 features, with 32-bits per feature, this is 128 kB per 1024 feature frames
// at 86 feature frames per second (44100/512), 1024 feature frames is about 11 seconds of features
// at 1024 * 128 feature frames, it is about 24 minutes of features, 
// taking up 16834 kB, or 16 MB (with 512 frame audio, the audio will take 256 MB)

//#define MAX_FEATURE_FRAMES 1024 * 128	// 256 MB for 24 minutes: 1024 * 128 * 32 * 32 bits / 8 bits per byte / 1024 kbytes per byte / 1024 megabytes per byte
#define MAX_FEATURE_FRAMES 1024 * 64	// 128 MB for 12 minutes: 8 MB for features, 128 MB for audio, and 12 minutes total, storing everything in RAM
#define MAX_PLAY_COUNT 30

class pkmAudioFeatureDatabaseFlANN
{
public:
	pkmAudioFeatureDatabaseFlANN(int sample_rate = 44100, 
							int fft_size = 512,
							int feature_length = 32)
	{
		sampleRate = sample_rate;
		fftN = fft_size;
		bBuiltIndex = false;
		analyzer = new pkmAudioFileAnalyzer(sampleRate, fftN);
		
		numFeatures = feature_length;
		numFrames = MAX_FEATURE_FRAMES;
		
		k				= 1;												// number of nearest neighbors
		nnIdx			= (int *)malloc(sizeof(int)*k);						// allocate near neighbor indices
		dists			= (float *)malloc(sizeof(float)*k);					// allocate near neighbor dists	
		query			= (float *)malloc(sizeof(float)*feature_length);	// pre-allocate a query frame
		dataset.reset(numFrames, numFeatures, true);						// allocate our dataset
		
		flannParams = DEFAULT_FLANN_PARAMETERS;
		flannParams.algorithm = FLANN_INDEX_AUTOTUNED; // or FLANN_INDEX_KDTREE, FLANN_INDEX_KMEANS
		flannParams.target_precision = 0.9; 

	}
	~pkmAudioFeatureDatabaseFlANN()
	{
		delete analyzer;
		
		// we free here because in upper level the segmenter allocates this data
		// this is really stupid but a solution for now.
		for (int i = 0; i < unique_buffers.size(); i++) {
			free(unique_buffers[i]);
		}

		if(bBuiltIndex)
		{
			flann_free_index(kdTree, &flannParams);
			bBuiltIndex = false;
		}
		
		free(nnIdx);
		free(dists);
		free(query);
	}
	
	// see if our pre-allocated dataset has any frames left - 
	// assuming we keep every frame, and don't overwrite any frames
	inline bool bIsEnoughFramesLeft(int numFramesToCompute)
	{
		// our dataset uses a circular matrix, so we know the current row, and
		// how many rows are left before it is full
		return (dataset.current_row + numFramesToCompute) < dataset.rows;
	}
	
	// check the buffer size, and see how many frames of data we'd require
	// and if there is any data left, then we can segment it
	// also does a really stupid top-down test, based on the NN distance and 
	// a fixed threshold
	inline bool bShouldSegment(float *&buffer, int buffer_size)
	{
		// are there frames left?
		int numFramesToCompute = buffer_size / fftN;
		if(!bIsEnoughFramesLeft(numFramesToCompute))
			return false;
		
		
		// there are frames left,
		// is the distance to an existing segment too close? - engage in 
		// reinforcement learning, or do not learn it for a threshold.
		if (bBuiltIndex) {
			// get the mfcc features of the incoming frame
			analyzer->analyzeFile(buffer, fftN, 1, query, numFeatures);
			
			if (flann_find_nearest_neighbors_index_float(kdTree, 
														 query, 
														 1, 
														 nnIdx, 
														 dists, 
														 1, 
														 &flannParams) < 0)
			{
				printf("[ERROR] No frames found for Nearest Neighbor Search!\n");
				return true;
			}
			
			printf("distance: %f\n", dists[0]);
			return (dists[0] > 0.0020);	// threshold determined from empirical testing... 
		}
		else
			return true;
			
	}

	void setK(int kNeighbors)
	{
		k = MAX(1,kNeighbors);
		k = MIN(20,k);
	}
	
	// automatically check if the sound already exists (within a distance threshold 
	// on the first mfcc frame, and check if there are enough frames left!)
	bool addSound(float *&buffer, int buffer_size, bool bAutoCheck = true)
	{
		if (bAutoCheck && !bShouldSegment(buffer, buffer_size)) {
			return false;
		}
		
		// get the features for this buffer for every frame
		int buffer_frames = buffer_size / fftN;
		float *	feature_matrix	= dataset.data + dataset.current_row*numFeatures;
		analyzer->analyzeFile(buffer,				// analyze the sound file
							  buffer_size,			// sound's total length
							  buffer_frames,		// total number of frames for this audio file
							  feature_matrix,		// every audio frames mfccs
							  numFeatures);			// number of coefficients
		
		// our dataset is now filled with numFrames more frames of mfccs
		dataset.current_row += buffer_frames;

		// store each frame of audio in a vector of pkmAudioFile 
		// (stores the original sound file, and the offset of the soundfile)
		vector<pkmAudioFile *> sound_lut;
		/*
		analyzer->createLookupTable(buffer,				// analyze the sound file
									buffer_size,		// sound's total length
									buffer_frames,		// total number of frames for this audio file
									sound_lut);			// each frame's offset of the audio buffer 		
		*/
		
		int samplesPerFrame = buffer_size / buffer_frames;
		for (int i = 0; i < buffer_frames; i++) 
		{
			pkmAudioFile *p = new pkmAudioFile(buffer, i*samplesPerFrame, buffer_size);
			sound_lut.push_back(p);
		}
		
		// add each frame as a pkmAudioFile to our audio_database
		audio_database.insert(audio_database.end(), sound_lut.begin(), sound_lut.end());
		
		// keep the buffer pointers for quick deallocation - this will eventually be
		// implemented as a extAudioFile for quick disk i/o
		unique_buffers.push_back(buffer);
		
		//printf("audio_files: %d, audio-frames: %d\n", unique_buffers.size(), dataset.current_row);
		
		return true;
	}
	
	// rebuild the index after segmenting
	void buildIndex()
	{		
		if(bBuiltIndex)
		{
			flann_free_index(kdTree, 
							 &flannParams);
			bBuiltIndex = false;
		}

		// i have no idea what this value means or should be...
		float speedup = 2.0;
		kdTree = flann_build_index(dataset.data, 
								   dataset.current_row, 
								   dataset.cols, 
								   &speedup, 
								   &flannParams);
		bBuiltIndex = true;	
	}
	
	/*
	// get called in the audio requested thread at audio rate
	vector<pkmAudioFile> getNearestFrame(float *&frame, int bufferSize)
	{
		vector<pkmAudioFile> nearestAudioFrames;
		if (bufferSize != fftN) {
			//printf("[ERROR] Buffer size does not match database settings");
			return nearestAudioFrames;
		}
		if (!bBuiltIndex) {
			//printf("[ERROR] First build the index with buildIndex()");
			return nearestAudioFrames;
		}
		
		// get the mfcc features of the incoming frame
		analyzer->analyzeFile(frame, bufferSize, 1, query, numFeatures);
		
		if (flann_find_nearest_neighbors_index_float(kdTree, 
													 query, 
													 1, 
													 nnIdx, 
													 dists, 
													 k, 
													 &flannParams) < 0)
		{
			printf("[ERROR] No frames found for Nearest Neighbor Search!\n");
			return nearestAudioFrames;
		}
		
		float sumDists = 0;
		int i = 0;
		while (i < k) {
			sumDists += dists[i];
			i++;
		}
		
		for (int i = 0; i < k; i++) {
			//printf("i-th idx: %d, dist: %f\n", nnIdx[i], dists[i]);
			pkmAudioFile p = audio_database[nnIdx[i]];
			if (k == 1) {
				p.weight = 1.0;
			}
			else
			{
				p.weight =  1.0 - (dists[i] / sumDists);
			}
			//printf("i-th idx: %d, dist: %f, norm_dist: %f\n", nnIdx[i], dists[i], p.weight);
			
			//nearestAudioFrames.push_back(audio_database[nnIdx[i]]);
			nearestAudioFrames.push_back(p);
		}

		return nearestAudioFrames;
	}
	*/
	
	// get called in the audio requested thread at audio rate
	vector<pkmAudioFile *> getNearestFramePtrs(float *&frame, int bufferSize)
	{
		vector<pkmAudioFile *> nearestAudioFrames;
		if (bufferSize != fftN) {
			//printf("[ERROR] Buffer size does not match database settings");
			return nearestAudioFrames;
		}
		if (!bBuiltIndex) {
			//printf("[ERROR] First build the index with buildIndex()");
			return nearestAudioFrames;
		}
		
		// get the mfcc features of the incoming frame
		analyzer->analyzeFile(frame, bufferSize, 1, query, numFeatures);
		
		if (flann_find_nearest_neighbors_index_float(kdTree, 
													 query, 
													 1, 
													 nnIdx, 
													 dists, 
													 k, 
													 &flannParams) < 0)
		{
			//printf("[ERROR] No frames found for Nearest Neighbor Search!\n");
			return nearestAudioFrames;
		}
		
		float sumDists = 0;
		int i = 0;
		while (i < k) {
			sumDists += dists[i];
			i++;
		}
		
		for (int i = 0; i < k; i++) 
		{
			//printf("i-th idx: %d, dist: %f\n", nnIdx[i], dists[i]);
			pkmAudioFile *p = audio_database[nnIdx[i]];
			p->playCount++;
			if (p->playCount >= MAX_PLAY_COUNT) {
				overplayed_audio_database.push_back(p);
			}
			if(!p->bPlaying && p->playCount < MAX_PLAY_COUNT)
			{
				/*
				if (k == 1) {
					p->weight = 1.0;
				}
				else
				{
					p->weight =  1.0 - (dists[i] / sumDists);
				}
				 */
				p->weight = 1.0 - p->playCount / (float)MAX_PLAY_COUNT;
				//printf("i-th idx: %d, dist: %f, norm_dist: %f\n", nnIdx[i], dists[i], p.weight);
				//nearestAudioFrames.push_back(audio_database[nnIdx[i]]);
				nearestAudioFrames.push_back(p);
			}
		}
		
		// slowly bring the play count back down
		vector<pkmAudioFile *>::iterator it = overplayed_audio_database.begin(); 
		while(it != overplayed_audio_database.end()) 
		{
			(*it)->playCount-=0.2;
			if ((*it)->playCount <= 5) {
				it = overplayed_audio_database.erase(it);
			}
			else {
				it++;
			}

		}
		
		return nearestAudioFrames;
	}
	
	inline int size()
	{
		return unique_buffers.size();
	}
	
	
	int									sampleRate, 
										fftN;
	
	pkmAudioFileAnalyzer				*analyzer;
	vector<pkmAudioFile *>				audio_database;
	vector<pkmAudioFile *>				overplayed_audio_database;
	vector<float *>						unique_buffers;
	
	// For kNN - using C-version of FlANN
	pkm::Mat							dataset;		// feature dataset
	float								*query;			// feature query 
	int									*nnIdx;			// near neighbor indices of the dataset
	float								*dists;			// near neighbor distances from the query to each dataset member
	flann_index_t						kdTree;			// kd-tree reference representing all the feature-frames
	struct FLANNParameters				flannParams;	// index parameters are stored here
	
	/*
	// For kNN - using C++ version of FlANN
	 flann::Matrix<float>				dataset;		// positions of each filter (dataset)
	 flann::Matrix<float>				query;			// query position
	 flann::Matrix<int>					nnIdx;			// near neighbor indices
	 flann::Matrix<float>				dists;			// near neighbor distances
	 flann::Index<flann::L2<float> >	kdTree;			// kd-tree representing all the filters
	*/
	
	/*
	 ANNpointArray						positions;		// positions of each filter
	 ANNkd_tree							*kdTree;		// distances to nearest HRTFs
	 ANNidxArray						nnIdx;			// near neighbor indices
	 ANNdistArray						dists;			// near neighbor distances
	*/
	
	int									k;				// number of nearest neighbors
	
	int									numFeatures,	// dimension of each point
										numFrames;		// number of feature point frames

	bool								bBuiltIndex, 
										bSemaphore;
};