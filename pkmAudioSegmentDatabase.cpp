/*
 *  pkmAudioSegmentDatabase.cpp
 *  mosaicingNonRT
 *
 *  Created by Mr. Magoo on 9/13/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "pkmAudioSegmentDatabase.h"
/*
vector<pkmAudioSegment> pkmAudioSegmentDatabase::getNearestSequenceMatch(float *descriptor, int num)
{
    databaseMatches.clear();
    unsigned long idx;
    float * db;
    vector<float> db_dynamic;
    db_dynamic.resize(num*featureDatabase.cols);
    if(true)
    {
        for (int i = 0; i <= featureDatabase.rows - num; i++) {
            if (num > 1) {
                for (int j = 0; j < num; j++) {
                    cblas_scopy(featureDatabase.cols, featureDatabase.row(j), 1, &(db_dynamic[j*featureDatabase.cols]), 1);
                }
                db = &(db_dynamic[0]);
            }
            else {
                db = featureDatabase.row(i);
            }

            float f = cosineDistance(db, descriptor, num*featureDatabase.cols);
            databaseMatches[i] = isnan(f) ? 1e10 : f;
        }
        
        idx = pkm::Mat::minIndex(databaseMatches);
    }
    else {
        
        sequenceMatch(descriptor,                                   // input feature
                      databaseMatches.data,                         // output convolution
                      featureDatabase.data,                         // database
                      num,                                          // input feature length
                      featureDatabase.rows,                         // database length
                      0,                                            // low selector
                      num*featureDatabase.cols,                     // high selector
                      featureDatabase.cols);                        // full feature length
        idx = pkm::Mat::maxIndex(databaseMatches);
    }
    
    
    vector<pkmAudioSegment> nearestAudioSegments;
    pkmAudioSegment p = audioDatabase[idx];
    
    //printf("idx: %ld (db idx: %ld, db filename: %s)\n", idx, audioDatabase[idx].onset / 2048, audioDatabase[idx].filename.c_str());
    p.frame = 0;
    nearestAudioSegments.push_back(p);
    
    return nearestAudioSegments;
}
 
 
 
 
 
 
*/