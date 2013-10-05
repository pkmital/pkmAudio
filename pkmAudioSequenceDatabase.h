//
//  pkmAudioSequenceDatabase.h
//  audioMosaicingDTW
//
//  Created by Parag Mital on 10/20/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#pragma once

#ifdef USE_GVF
#include "pkmGVF.h"
#else
#include "pkmDTW.h"
#endif

#include "pkmAudioSegment.h"
#include "pkmEXTAudioFileReader.h"
#include "ofUtils.h"
#include "ofTypes.h"

using namespace pkm;

class pkmAudioSequenceDatabase
{
public:
    pkmAudioSequenceDatabase()
    {
        
    }
    
    void addSequence(ofPtr<pkmAudioSegment> & audioSequence, Mat audioFeature)
    {
        audioSequences.push_back(audioSequence);
        database.addToDatabase(audioFeature);
    }
    
    pkmAudioSegment getNearestSequence(Mat audioFeature, vector<int> &pathi, vector<int> &pathj)
    {
        int location;
        float distance;
        database.getNearestCandidate(audioFeature, distance, location, pathi, pathj);
        
        if (pathi.size() > 0 && pathj.size() > 0) {
            audioSequences[location]->audio_rate = pathi[0] / (double) pathj[0];
        }
        else {
            audioSequences[location]->audio_rate = 1.0;
        }
        return *audioSequences[location];
    }
    
    ofPtr<pkmAudioSegment> getNearestSequence(Mat audioFeature)
    {
        int location;
        vector<int> pathi, pathj;
        float distance;
        database.getNearestCandidate(audioFeature, distance, location, pathi, pathj);
        
        if (pathi.size() > 0 && pathj.size() > 0) {
            audioSequences[location]->audio_rate = pathi[0] / (double) pathj[0];
            audioSequences[location]->frame = pathi[pathi.size()-1];
        }
        else {
            audioSequences[location]->audio_rate = 1.0;
        }
        return audioSequences[location];
    }
    
    vector<ofPtr<pkmAudioSegment> > getNearestSequences(Mat &audioFeature)
    {
        vector<ofPtr<pkmAudioSegment> > nearestSequences;
        
        nearestSequences.push_back(getNearestSequence(audioFeature));
        
        return nearestSequences;
    }
    
    
    vector<ofPtr<pkmAudioSegment> > getNearestSequences(float *audioFeature, int numFeatures)
    {
        vector<ofPtr<pkmAudioSegment> > nearestSequences;
        int location;
        vector<int> pathi, pathj;
        float distance;
        database.getNearestCandidate(audioFeature, numFeatures, distance, location, pathi, pathj);
        
        if (pathi.size() > 0 && pathj.size() > 0) {
            audioSequences[location]->audio_rate = pathi[0] / (double) pathj[0];
            audioSequences[location]->frame = pathi[pathi.size()-1];
        }
        else {
            audioSequences[location]->audio_rate = 1.0;
        }
        
        
        //audioSequences[location]->audio_rate = pathi.back() / (double)pathj.back();
        //if (pathi.size() > 0 || pathk.size() > 0) {
        //    cout << audioSequences[location]->audio_rate << ": " << pathi.size() << " " << pathj.size() << endl;
        //}
        nearestSequences.push_back(audioSequences[location]);
        return nearestSequences;
    }
    
    void save()
    {
        database.save();
        
        FILE *fp;
        fp = fopen(ofToDataPath("audio_database.txt").c_str(), "w");
        fprintf(fp, "%ld\n", audioSequences.size());
        printf("[OK] Saving %lu segments\n", audioSequences.size());
        for(vector<ofPtr<pkmAudioSegment> >::iterator it = audioSequences.begin();
            it != audioSequences.end();
            it++)
        {
            char buf[512];
            sprintf(buf, "%s", (*it)->filename.c_str());
            fprintf(fp, "%s %ld %ld %ld\n", buf, (*it)->onset, (*it)->offset, (*it)->index);
        }
        fclose(fp);
        
    }
    
    void load(bool bLoadBuffer = false)
    {
        database.load();
#ifdef USE_GVF
        database.buildDatabase();
#endif
        
        FILE *fp;
        fp = fopen(ofToDataPath("audio_database.txt").c_str(), "r");
        unsigned long database_size = 0;
        fscanf(fp, "%ld\n", &database_size);
        printf("[OK] Loading %d segments...", (int)database_size);
        for(unsigned long i = 0; i < database_size; i++)
        {
            char buf[512];
            unsigned long onset, offset, index;
            vector<float> desc;
            fscanf(fp, "%s %ld %ld %ld\n", buf, &onset, &offset, &index);
            ofPtr<pkmAudioSegment> p(new pkmAudioSegment(buf, onset, offset, desc, index, 0));
            if (bLoadBuffer) {
                pkmEXTAudioFileReader audioFile;
                if(audioFile.open(ofToDataPath(buf)))
                {
                    p->buffer = (float *)malloc(sizeof(float)*audioFile.mNumSamples);
                    p->bStoringAudioData = true;
                    audioFile.read(p->buffer, 0, audioFile.mNumSamples);
                    audioSequences.push_back(p);
                }
                else {
                    cout << "Could not open: " << buf << endl;
                }
            }
            else {
                audioSequences.push_back(p);
            }
        }
        fclose(fp);
        printf("done.\n");
    }
    
    
private:
    vector<ofPtr<pkmAudioSegment> >     audioSequences;
#ifdef USE_GVF
    pkmGVF                              database;
#else
    pkmDTW                              database;
#endif
};
