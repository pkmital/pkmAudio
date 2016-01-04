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

#include <Accelerate/Accelerate.h>

#include "ofMain.h"

#ifdef WITH_FLANN
    #include <flann/flann.h>
#endif

#include "GUI.hpp"


template<typename T>
T abs_template(T t)
{
    return t>0 ? t : -t;
}

class pkmAudioSegmentDatabase
{
public:
	pkmAudioSegmentDatabase()
	{
        currentIdx      = 0;
        maxObjects      = -1;
		bBuiltIndex     = false;
        totalSamples    = 0;
        
        // number of nearest neighbors
        k				= 1;
		nnIdx.resize(k);
        
        // allocate near neighbor dists
		dists.resize(k);
        
        bBuildingScreenMapping = false;
        bBuiltScreenMapping = false;
        bSelectingFromMapping = false;
        bShouldUpdateMapping = true;
        
#ifdef WITH_FLANN
        //query			= (float *)malloc(sizeof(float)*feature_length);	// pre-allocate a query frame
		flannParams = DEFAULT_FLANN_PARAMETERS;
		flannParams.algorithm = FLANN_INDEX_LINEAR;
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
        
        vector<shared_ptr<pkmAudioSegment> >::iterator it = audioDatabase.begin();
        while (it != audioDatabase.end()) {
            it->reset();
            it = audioDatabase.erase(it);
        }
        it = audioDatabaseToDelete.begin();
        while (it != audioDatabaseToDelete.end()) {
            it->reset();
            it = audioDatabaseToDelete.erase(it);
        }
	}
    
    void setScaling(float amt)
    {
        scaling = amt;
//        GUI::Warehouse::fontSmall().loadFont("FiraSans-Light.ttf", 13*scaling);
    }
    
    void resetDatabase()
    {
        bMutex.lock();
        vector<shared_ptr<pkmAudioSegment> >::iterator it = audioDatabase.begin();
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
        bBuiltScreenMapping = false;
        bMutex.unlock();
    }
    
    
    long long getLength()
    {
        return totalSamples;
    }
    
    int getSize()
    {
        return std::min<int>(maxObjects,featureDatabase.rows);
    }
	
	void setK(int kNeighbors)
	{
		k = MAX(1,kNeighbors);
		k = MIN(20,k);
        
		nnIdx.resize(k);
		dists.resize(k);
	}
	
    bool bShouldAddSegment(float *data)
    {
        bMutex.lock();
        if(!bBuiltIndex)
        {
            bMutex.unlock();
            return ~isnan(data[0]) && ~isinf(data[0]);
        }
#ifdef WITH_FLANN
        if (flann_find_nearest_neighbors_index_float(kdTree, 
													 data, 
													 1, 
													 nnIdx, 
													 dists, 
													 k, 
													 &flannParams) < 0)
		{
            bMutex.unlock();
            return true;
        }
        else if(dists[0] > 0.001)
        {
            //cout << "adding: " << dists[0] << endl;
            bMutex.unlock();
            return true;
        }
        else 
        {
//            printf("[pkmAudioSegmentDatabase]: Too similar to database, not adding: %f\n", dists[0]);
            bMutex.unlock();
            return false;
        }
#else
        if (featureDatabase.rows > 0) 
        {
            float bestSum = HUGE_VAL;
            int i = 0;
            
            while(i < featureDatabase.rows) {

                float sum = 0;
                int j = 0;
                
                while (j < featureDatabase.cols) {
                    float a = featureDatabase.row(i)[j] - data[j];
                    sum += abs(a);
                    j++;
                }
                
                if (sum < bestSum) {
                    bestSum = sum;
                }
                
                i++;
            }
//            cout << "bestSum: " << bestSum << endl;
            bMutex.unlock();
            return ~isinf(bestSum) && ~isnan(bestSum) && bestSum > 0.0010;
        }
        else {
            bMutex.unlock();
            return true;
        }
#endif
            bMutex.unlock();
        return false;
    }
    
    bool bShouldAddSequence(pkm::Mat featureSequence)
    {
        
        if(!bBuiltIndex)
            return true;
        
        bMutex.lock();
        
#ifdef WITH_FLANN
        if (flann_find_nearest_neighbors_index_float(kdTree, 
													 featureSequence.data, 
													 1, 
													 nnIdx, 
													 dists, 
													 k, 
													 &flannParams) < 0)
		{
            bMutex.unlock();
            return true;
        }
        else if(dists[0] > 0.001)
        {
            bMutex.unlock();
            return true;
        }
        else 
        {
//            printf("[pkmAudioSegmentDatabase]: Too similar to database, not adding: %f\n", dists[0]);
            bMutex.unlock();
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
//                    minIdx = i;
                    minDist = thisDist;
                }
                i++;
            }
//            cout << "dist: " << minDist << endl;
            bMutex.unlock();
            return ~isinf(minDist) && ~isnan(minDist) && minDist > 0.0010;
        }
        else {
            bMutex.unlock();
            return true;
        }
#endif
            bMutex.unlock();
    }
    
    void addAudioSegment(shared_ptr<pkmAudioSegment> segment, float *descriptor, int descriptor_size)
    {
        
        bMutex.lock();
        
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
        
        bMutex.unlock();
    }
    
    void addAudioSequence(shared_ptr<pkmAudioSegment> segment, pkm::Mat featureSequence)
    {
        
        bMutex.lock();
        
        audioDatabase.push_back(segment);
        totalSamples += (segment->offset - segment->onset);
        featureDatabase.push_back(featureSequence);
        
        float currentLookup = audioDatabase.size();
        pkm::Mat lookupTable(featureSequence.rows, 1, currentLookup);
        databaseLookupTable.push_back(lookupTable);

        bMutex.unlock();
    }
    
	// rebuild the index after segmenting
	void buildIndex()
	{
        
        bMutex.lock();
        
#ifdef WITH_FLANN
		if(bBuiltIndex)
		{
			flann_free_index(kdTree, 
							 &flannParams);
            bMutex = false;
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
        
        bMutex.unlock();
        
	}
    
    void setShouldUpdateScreenMapping(bool b)
    {
        bShouldUpdateMapping = b;
    }
    
    void buildScreenMapping()
    {
        if (bShouldUpdateMapping && featureDatabase.rows > 5) {
            
            bMutex.lock();
            
            normalizedDatabase = featureDatabase.rowRange(1, getSize(), false);
            pkm::Mat X_t = normalizedDatabase.getTranspose();

            pkm::Mat covDatabase = X_t.GEMM(normalizedDatabase) / static_cast<float>(featureDatabase.rows - 1);
            
            pkm::Mat U, S, V_t;
            if (covDatabase.svd(U, S, V_t) <= 0)
            {
                bScreenMappingMutex.lock();
                pkm::Mat m = S.colRange(0, 5, true).sqrt().getDiagMat();
                xy_mapping = V_t.colRange(0, 5, true).GEMM(m);
                
                if (!bBuiltScreenMapping) {
                    xy_mapping_interp = xy_mapping;
                }
                bBuiltScreenMapping = true;
                
                bScreenMappingMutex.unlock();
                bMutex.unlock();
            }
            else {
                bMutex.unlock();
                resetDatabase();
            }
        }
    }
    
    void updateScreenMapping()
    {
        if(bBuiltScreenMapping) {
            
            bScreenMappingMutex.lock();
            
            float alpha = 0.95;
            
            if(xy_mapping_interp.size() != xy_mapping.size())
                xy_mapping_interp.resize(xy_mapping.rows, xy_mapping.cols);
            
            xy_mapping_interp.multiply(alpha);
            xy_mapping_interp.add(xy_mapping * (1.0 - alpha));
            xys_update = featureDatabase.rowRange(1, getSize(), true).GEMM(xy_mapping_interp);
            
            // center
            pkm::Mat means = xys_update.mean(true);
            if(means_interp.size())
            {
                means_interp.multiply(alpha);
                means_interp.add(means * (1.0 - alpha));
            }
            else
                means_interp = means;
            
            xys = pkm::Mat(xys_update.rows, xys_update.cols);
            xys_mouse = pkm::Mat(xys_update.rows, 2);
            
            for (int c = 0; c < xys.cols; c++) {
                float m = means_interp.data[c] * -1.0;
                if (isnan(m) || isinf(m)) {
                    m = 0.0;
                }
                vDSP_vsadd(&(xys_update.data[c]), xys.cols, &m, &(xys.data[c]), xys.cols, xys.rows);
            }
            
            // divide by max
            pkm::Mat maxs = pkm::Mat::abs(xys).max(false);
            if(maxs_interp.size())
            {
                maxs_interp.multiply(alpha);
                maxs_interp.add(maxs * (1.0 - alpha));
            }
            else
                maxs_interp = maxs;
            
            for (int c = 0; c < xys.cols; c++) {
                float m = maxs_interp.data[c];//std::min<float>(0.75, std::max<float>(fabs(maxs_interp.data[c]), 0.000001));
                if (isnan(m) || isinf(m)) {
                    m = 1.0;
                }
                vDSP_vsdiv(&(xys.data[c]), xys.cols, &m, &(xys.data[c]), xys.cols, xys.rows);
            }
            
            
//            xys.print();
            
            bScreenMappingMutex.unlock();
        }
    }
    
    void setOrientation(ofPoint orientation)
    {
        this->orientation = orientation;
    }
    
    void drawDatabase(int width, int height, float radius = 5.0f, float audio_rate = 86.0f, float sample_rate = 44100.0f, bool b_draw_axes = false)
    {
        radius *= scaling;
        if(bBuiltScreenMapping) {
            
            bMutex.lock();
            bScreenMappingMutex.lock();
            
            ofSetLineWidth(2.0 * scaling);
            ofEnableAntiAliasing();
            ofEnableSmoothing();
            ofPushMatrix();
            ofFill();
            
            float prev_x = -1;
            float prev_r = 0, prev_g = 0, prev_b = 0;
            ofVec3f prev_v;
            for (int row_i = 0; row_i < std::min<int>(maxObjects, audioDatabase.size())-1; row_i++) {
                float x = (xys.row(row_i)[0] * (width / 2.5));
                float y = (xys.row(row_i)[1] * (height / 2.5));
                float z = xys.row(row_i)[2] * 100.0;
                float z2 = 0.0;//xys.row(row_i)[3];
                float z3 = 0.0;//xys.row(row_i)[4];
                
                ofVec3f v(x, y, z);
                v.rotate(orientation.x, orientation.y, orientation.z);
                
                v.x += (width / 2.0);
                v.y += (height / 2.0);
                
                xys_mouse.row(row_i)[0] = v.x;
                xys_mouse.row(row_i)[1] = v.y;

                float r = std::max<float>(0.0, std::min<float>(220, 120 + z));
                float g = std::min<float>(200, 120 + 100.0*z2);
                float b = std::min<float>(200, 120 + 100.0*z3);
                float a = 220.0;
                ofSetColor(r, g, b, a);
                
                if (audioDatabase[row_i+1]->bPlaying) {
                    float this_radius = radius + radius * (audioDatabase[row_i+1]->index) / (float)audioDatabase[row_i+1]->offset * (audioDatabase[row_i+1]->offset / sample_rate);
                    ofDrawCircle(v, this_radius);
                    if (prev_x != -1) {
//                        ofLine(v, prev_v);
                        ofMesh m;
                        m.addVertex(v);
                        m.addColor(ofColor(r, g, b, a));
                        m.addVertex(prev_v);
                        m.addColor(ofColor(prev_r, prev_g, prev_b, a));
                        m.draw(OF_MESH_WIREFRAME);
                    }
                    
                    ofNoFill();
                    this_radius = radius + radius * (audioDatabase[row_i+1]->offset / sample_rate);
                    if (audioDatabase[row_i+1]->bNeedsReset)
                        ofSetColor(140, 180, 140, a);
                    else
                        ofSetColor(255, a);
                    if(this_radius > 5)
                        ofDrawCircle(v, this_radius);
                    ofFill();
                    
                    prev_x = x;
                    prev_v = v;
                    prev_r = r;
                    prev_g = g;
                    prev_b = b;
                }
                else {
                    float this_radius = radius + radius * (audioDatabase[row_i+1]->offset / sample_rate);
                    ofDrawCircle(v, this_radius);
                }
            }
            
            
            ofSetLineWidth(1.0);
            ofSetColor(255);
            ofFill();
            
            ofPopMatrix();
            
            bMutex.unlock();
            bScreenMappingMutex.unlock();
        }
        else {
//            string str="Make some noise!";
//            GUI::Warehouse::fontSmall().drawString(str, width / 2 - GUI::Warehouse::fontSmall().stringWidth(str) / 2.0, height / 1.25);
        }
    }
    // implement unselect from database
    vector<shared_ptr<pkmAudioSegment> > selectFromDatabase(float x1, float y1, int width, int height)
    {
        
        vector<shared_ptr<pkmAudioSegment> > nearestAudioSegments;
//        int k = 1;
        
        if(bBuiltScreenMapping)
        {
            if (bSelectingFromMapping) {
                return nearestAudioSegments;
            }
            bSelectingFromMapping = true;
            
            
            bScreenMappingMutex.lock();
            
            float sum = 0;
            
            for (int m = 0; m < k; m++) {
                nnIdx[m] = 0;
                dists[m] = HUGE_VALF;
            }
            
            
            for (int row_i = 0; row_i < std::min<int>(maxObjects,xys_mouse.rows); row_i++) {
                float x2 = xys_mouse.row(row_i)[0];
                float y2 = xys_mouse.row(row_i)[1];
                
                sum = abs_template<float>(x1 - x2) + abs_template<float>(y1 - y2);
                
                int n = row_i + 1;
                for (int m = 0; m < k; m++) {
                    if (sum < dists[m]) {
                        float temp = dists[m];
                        dists[m] = sum;
                        sum = temp;
                        
                        int temp2 = nnIdx[m];
                        nnIdx[m]= n;
                        n = temp2;
                    }
                }
            }

            
            bScreenMappingMutex.unlock();
            
            bMutex.lock();

            for (int i = 0; i < k; i++)
            {
                if (nnIdx[i] < audioDatabase.size()) {
                    shared_ptr<pkmAudioSegment> p = (audioDatabase[nnIdx[i]]);
                    nearestAudioSegments.push_back(p);
                }
            }
            
            bMutex.unlock();
        }
        
        bSelectingFromMapping = false;
        return nearestAudioSegments;
    }
    
    
    void unSelectFromDatabase(float x, float y, int width, int height)
    {
        //        int k = 1;
        
        if(bBuiltScreenMapping)
        {
            if (bSelectingFromMapping) {
                return;
            }
            bSelectingFromMapping = true;
            
            
            bScreenMappingMutex.lock();
            
            float sum = 0;
            
            for (int m = 0; m < k; m++) {
                nnIdx[m] = 0;
                dists[m] = HUGE_VALF;
            }
            
//            x -= (width / 2.0);
//            y -= (height / 2.0);
//            
//            x /= (width / 4.0);
//            y /= (height / 4.0);
            
            for (int row_i = 0; row_i < std::min<int>(maxObjects,xys_mouse.rows); row_i++) {
                float this_x = xys_mouse.row(row_i)[0];
                float this_y = xys_mouse.row(row_i)[1];
                
                sum = ofDist(this_x, this_y, x, y);
                
                int n = row_i + 1;
                for (int m = 0; m < k; m++) {
                    if (sum < dists[m]) {
                        float temp = dists[m];
                        dists[m] = sum;
                        sum = temp;
                        
                        int temp2 = nnIdx[m];
                        nnIdx[m]= n;
                        n = temp2;
                    }
                }
            }
            
            bScreenMappingMutex.unlock();
            
            
            bMutex.lock();
            //            cout << "size: " << audioDatabase.size() << endl;
            for (int i = 0; i < k; i++)
            {
                if (nnIdx[i] < audioDatabase.size()) {
                    //printf("i-th idx: %d, dist: %f\n", nnIdx[i], dists[i]);
                    shared_ptr<pkmAudioSegment> p = (audioDatabase[nnIdx[i]]);
                    
                    if (p != NULL && p->bPlaying) {
                        p->bNeedsReset = true;
                    }
                }
            }
            
            bMutex.unlock();
        }
        
        bSelectingFromMapping = false;
        return;
    }
    
    
    void removeFromDatabase(float x, float y, int width, int height, float radius=20)
    {
        
        if(bBuiltScreenMapping)
        {
            if (bSelectingFromMapping) {
                return;
            }
            bSelectingFromMapping = true;
            
            
            bScreenMappingMutex.lock();
            int k = 1;
            vector<int> nnIdx;
            vector<float> dists;
            nnIdx.resize(1);
            dists.resize(1);
            
            float sum = 0;
            
            for (int m = 0; m < k; m++) {
                nnIdx[m] = 0;
                dists[m] = HUGE_VALF;
            }
            
//            x -= (width / 2.0);
//            y -= (height / 2.0);
//            
//            x /= (width / 4.0);
//            y /= (height / 4.0);
            
            for (int row_i = 0; row_i < std::min<int>(maxObjects, xys_mouse.rows); row_i++) {
                float this_x = xys_mouse.row(row_i)[0];
                float this_y = xys_mouse.row(row_i)[1];
                
                sum = ofDist(this_x, this_y, x, y);
                
                int n = row_i + 1;
                for (int m = 0; m < k; m++) {
                    if (sum < dists[m]) {
                        float temp = dists[m];
                        dists[m] = sum;
                        sum = temp;
                        
                        int temp2 = nnIdx[m];
                        nnIdx[m]= n;
                        n = temp2;
                    }
                }
            }
            
            bScreenMappingMutex.unlock();
            
            
            bMutex.lock();
            //            cout << "size: " << audioDatabase.size() << endl;
            for (int i = 0; i < k; i++)
            {
                if (nnIdx[i] < audioDatabase.size() && dists[i] < radius) {
                    shared_ptr<pkmAudioSegment> p = (audioDatabase[nnIdx[i]]);
                    
                    if (p != NULL && p->bPlaying) {
                        p->bNeedsReset = true;
                    }
                    
                    audioDatabase.erase(audioDatabase.begin() + nnIdx[i]);
                    if(nnIdx[i] == 0)
                    {
                        featureDatabase = featureDatabase.rowRange(nnIdx[i], featureDatabase.rows, true);
                    }
                    else if(nnIdx[i] == featureDatabase.rows-1)
                    {
                        featureDatabase = featureDatabase.rowRange(0, nnIdx[i], true);
                    }
                    else
                    {
                        pkm::Mat newMat = featureDatabase.rowRange(0, nnIdx[i], true);
                        newMat.push_back(featureDatabase.rowRange(nnIdx[i] + 1, featureDatabase.rows, true));
                        featureDatabase = newMat;
                    }
                }
            }
            
            bMutex.unlock();
        }
        
        bSelectingFromMapping = false;
        return;
    }
    
//
	vector<shared_ptr<pkmAudioSegment> > getNearestAudioSegments(shared_ptr<pkmAudioSegment> segment)
	{
        
        bMutex.lock();
        
		vector<shared_ptr<pkmAudioSegment> > nearestAudioSegments;
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
            
            bMutex.unlock();
			return nearestAudioSegments;
		}
#else

        for (int m = 0; m < k; m++) {
            nnIdx[m] = 0;
            dists[m] = HUGE_VALF;
        }
        
        i = 0;
        float *testData = segment->descriptor->data;
        while(i < std::min<int>(maxObjects,featureDatabase.rows)) {
            float *curData = featureDatabase.row(i);
            float sum = 0;
            int j = 0;
            while (j < featureDatabase.cols) {
                float a = *curData++ - testData[j];
                sum += abs(a);
                j++;
            }
            
//            cout << "i: " << i << " dist: " << sum << endl;
            
            int n = i;
            for (int m = 0; m < k; m++) {
                if (sum < dists[m]) {
                    float temp = dists[m];
                    dists[m] = sum;
                    sum = temp;
                    
                    int temp2 = nnIdx[m];
                    nnIdx[m]= n;
                    n = temp2;
                }
            }
            
            i++;
        }
        
#endif
		
		for (int i = 0; i < k; i++) 
		{
            if (nnIdx[i] < audioDatabase.size()) {
                //printf("i-th idx: %d, dist: %f\n", nnIdx[i], dists[i]);
                shared_ptr<pkmAudioSegment> p = (audioDatabase[nnIdx[i]]);

                if (!p->bPlaying) {
                    p->frame = 0;
                    p->index = 0;
                    p->bPlaying = true;
                    nearestAudioSegments.push_back(p);
                }
            }
            
            //else {
            //    printf("[ERROR] flann reutrned index %d but size of database is %d\n", nnIdx[i], audioDatabase.size());
            //    return nearestAudioSegments;
            //}
            
		}
        
        bMutex.unlock();
		return nearestAudioSegments;
	}
    
    //vector<pkmAudioSegment> getNearestSequenceMatch(float *descriptor, int num = 1);
    
    vector<shared_ptr<pkmAudioSegment> > getNearestAudioSegments(float *descriptor)
	{
        
        bMutex.lock();
        
		vector<shared_ptr<pkmAudioSegment> > nearestAudioSegments;
		int i;
		if (!bBuiltIndex) {
			//printf("[ERROR] First build the index with buildIndex()");
            
            bMutex.unlock();
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
            
            bMutex.unlock();
			return nearestAudioSegments;
		}
#else
        for (int m = 0; m < k; m++) {
            nnIdx[m] = 0;
            dists[m] = HUGE_VALF;
        }
        
        i = 0;
        float *testData = descriptor;
        while(i < std::min<int>(maxObjects,featureDatabase.rows)) {
            float sum = 0;
            int j = 0;
            while (j < featureDatabase.cols) {
                float a = featureDatabase.row(i)[j] - testData[j];
                sum += abs(a);
                j++;
            }
            
//            cout << "i: " << i << " " << sum << endl;
            
            int n = i;
            for (int m = 0; m < k; m++) {
                if (sum < dists[m]) {
                    float temp = dists[m];
                    dists[m] = sum;
                    sum = temp;
                    
                    int temp2 = nnIdx[m];
                    nnIdx[m]= n;
                    n = temp2;
                }
            }
            
            i++;
        }
        
#endif
		
		float sumDists = 0;
		i = 0;
		while (i < k) {
			sumDists += dists[i];
			i++;
		}
		
		for (int i = 0; i < k; i++) 
		{
            if (nnIdx[i] < audioDatabase.size() && dists[i] < 100000.0f) {
//                printf("K: %d, i-th idx: %d, dist: %f\n", i, nnIdx[i], dists[i]);
                shared_ptr<pkmAudioSegment> p = (audioDatabase[nnIdx[i]]);
                if (!p->bPlaying) {
                    p->index = 0;
                    p->frame = 0;
                    p->bPlaying = true;
                    nearestAudioSegments.push_back(p);
                }
                //else {
                //    p->bNeedsReset = true;
                //}
            }
        }
        bMutex.unlock();
		return nearestAudioSegments;
	}
    
    vector<shared_ptr<pkmAudioSegment> > getNearestAudioSequences(pkm::Mat featureSequence)
    {
        
        bMutex.lock();
        
        vector<shared_ptr<pkmAudioSegment> > nearestAudioSegments;
        int i;
        if (!bBuiltIndex) {
            //printf("[ERROR] First build the index with buildIndex()");
            
            bMutex.unlock();
            return nearestAudioSegments;
        }
#ifdef WITH_FLANN
        if (flann_find_nearest_neighbors_index_float(kdTree, 
                                                     featureSequence.data, 
                                                     1, 
                                                     nnIdx, 
                                                     dists, 
                                                     k, 
                                                     &flannParams) < 0)
        {
            printf("[ERROR] No frames found for Nearest Neighbor Search!\n");
            
            bMutex.unlock();
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
                shared_ptr<pkmAudioSegment> p = (audioDatabase[nnIdx[i]]);
                if (!p->bPlaying) {
                    p->frame = 0;
                    p->index = 0;
                    p->bPlaying = true;
                    nearestAudioSegments.push_back(p);
                }
                //else {
                //    p->bNeedsReset = true;
                //}
            }
        }
        bMutex.unlock();
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
        bMutex.lock();
        
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
        for(vector<shared_ptr<pkmAudioSegment> >::iterator it = audioDatabase.begin();
            it != audioDatabase.end();
            it++)
        {
            char buf[512];
            sprintf(buf, "%s", (*it)->filename.c_str());
            fprintf(fp2, "%s %ld %ld %ld\n", buf, (*it)->onset, (*it)->offset, (*it)->index);
            //printf("saved segment %ld - %ld from file %s\n", it->onset, it->offset, buf);
        }
        fclose(fp2);
        
        bMutex.unlock();
    }
    
    void load(bool bLoadBuffer = false)
    {
        bMutex.lock();
        
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
            shared_ptr<pkm::Mat> desc = shared_ptr<pkm::Mat>(new pkm::Mat());
            fscanf(fp2, "%s %ld %ld %ld\n", buf, &onset, &offset, &index);
            shared_ptr<pkmAudioSegment> p(new pkmAudioSegment(buf, onset, offset, desc, index, 0));
            if (bLoadBuffer) {
                pkmEXTAudioFileReader audioFile;
                if(audioFile.open(ofToDataPath(buf)))
                {
                    
                    p->buffer = shared_ptr<pkm::Mat>(new pkm::Mat(1, audioFile.mNumSamples));
                    p->bStoringAudioData = true;
                    audioFile.read(p->buffer->data, 0, audioFile.mNumSamples);
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
        
        bMutex.unlock();
        //databaseMatches = pkm::Mat(featureDatabase.rows+1, featureDatabase.cols, true);
    }
	
    void setMaxObjects(int maxObj)
    {
        maxObjects = maxObj;
    }
    
    vector<shared_ptr<pkmAudioSegment> >           audioDatabaseToDelete;
	vector<shared_ptr<pkmAudioSegment> >           audioDatabase;
	int                                 feature_length;
	// For kNN - using C-version of FlANN
	pkm::Mat							featureDatabase;
    pkm::Mat                            normalizedDatabase;
    pkm::Mat                            xy_mapping, xy_mapping_interp, means_interp, maxs_interp;
    pkm::Mat                            xys_update, xys, xys_mouse;
    pkm::Mat                            S;
    pkm::Mat                            databaseMatches;
    pkm::Mat                            databaseLookupTable;
    
    ofPoint                             orientation;
    
    ofTrueTypeFont                      font;
    
    long long                           totalSamples;
    
    int                                 maxObjects, currentIdx;
    
	vector<int>							nnIdx;			// near neighbor indices of the featureDatabase
	vector<float>                       dists;			// near neighbor distances from the query to each featureDatabase member
#ifdef WITH_FLANN
	flann_index_t						kdTree;			// kd-tree reference representing all the feature-frames
	struct FLANNParameters				flannParams;	// index parameters are stored here
#endif
	
	int									k;				// number of nearest neighbors
	
	int									numFeatures,	// dimension of each point
                                        numFrames;		// number of feature point frames
    
	bool								bBuiltIndex, bBuiltScreenMapping, bSelectingFromMapping, bBuildingScreenMapping, bShouldUpdateMapping;
    
    std::mutex                          bMutex, bScreenMappingMutex;
    
    float                               scaling;
    
//    ofxPostProcessing                   post;
//    ofxDOF                              dof;
//    ofxDofShader                        dofshader;
    ofFbo                               fbo;
    
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