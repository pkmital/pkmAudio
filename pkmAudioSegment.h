/*
 *  pkmAudioSegment.h
 *  CHoG
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
#include <string>
#include "pkmMatrix.h"

//#define DO_TIMESTRETCH

#ifdef DO_TIMESTRETCH
#include "pkmTimeStretcher.h"
#endif

using namespace std;

class pkmAudioSegment
{  
public:
    
    pkmAudioSegment() 
    : onset(0), offset(0), index(0), frame(0), bPlaying(false), bNeedsReset(false)
    {
        bStoringAudioData = false;
        bStoringDescriptor = false;
    }
    
    pkmAudioSegment(shared_ptr<pkm::Mat> buf,
                    unsigned long onset,
                    unsigned long offset,
                    unsigned long index,
                    unsigned long frame = 0,
                    bool playing = false,
                    bool needs_reset = false)
    :   buffer(buf), onset(onset), offset(offset), index(index), frame(frame), bPlaying(playing), bNeedsReset(needs_reset)
    {
        audio_rate = 1.0;
        bStoringAudioData = true;
        bStoringDescriptor = false;
#ifdef DO_TIMESTRETCH
        ts.setup(buf);
#endif
    }
    
    pkmAudioSegment(string filename,
                    unsigned long onset,
                    unsigned long offset,
                    shared_ptr<pkm::Mat> descriptor,
                    unsigned long index,
                    unsigned long frame,
                    bool playing = false,
                    bool needs_reset = false)
    :   filename(filename), onset(onset), offset(offset), descriptor(descriptor), index(index), frame(frame), bPlaying(playing), bNeedsReset(needs_reset)
    {
        audio_rate = 1.0;
        bStoringAudioData = false;
        bStoringDescriptor = true;
    }
    
    #ifdef DO_TIMESTRETCH
    float ts_play(float speed, float rate, float grainLength, int overlaps, float posmod = 0.0)
    {
        index = ts.getPosition();
        float sample = ts.play(rate, speed, grainLength, overlaps, posmod);
        audio_rate = speed;
        return sample;
    }
    
    void resetPosition()
    {
        if(audio_rate > 0)
        {
            ts.setPosition(0);
        }
        else
        {
            ts.setPosition(1.0);
        }
    }
    
    bool finished()
    {
        if(audio_rate < 0)
            return index == 0;
        else
            return index == (offset-onset-1);
    }
    
    bool started()
    {
        if(audio_rate > 0)
            return index == 0;
        else
            return index == (offset-onset-1);
    }
    #endif
    
    void save(FILE **fp)
    {
        if (*fp) {
            fprintf(*fp, "%s %lu %lu %ld\n", filename.c_str(), onset, offset, index);
        }
    }
    
    void load(FILE **fp)
    {
        if (*fp) {
            char buf[512];
            fscanf(*fp, "%s %ld %ld %ld\n", buf, &onset, &offset, &index);
            filename = string(buf);
        }
    }
	
    ~pkmAudioSegment()
    {
        bStoringAudioData = false;
    }
    
    string filename;
	
    unsigned long onset, offset;
    unsigned long index;
    unsigned long frame;				// current audio frame
		
	double audio_rate;
    
    bool bNeedsReset;               // triggered to reset to frame 0
	bool bPlaying;                  // already playing, so frame > 0
    bool bStoringAudioData;         // store the audio buffer *
    bool bStoringDescriptor;

    shared_ptr<pkm::Mat> buffer;
    shared_ptr<pkm::Mat> descriptor;
    
    #ifdef DO_TIMESTRETCH
    pkmPitchStretch<hannWinFunctor> ts;
    pkm::Mat stretched_buffer;
    #endif
};
