/*
 *  pkmAudioVideoSegment.h
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

using namespace std;

class pkmAudioVideoSegment
{  
public:
    
    pkmAudioVideoSegment() 
    : filename(""), onset(0), offset(0), index(0), frame(0), bPlaying(false)
    {
        
    }
    
    pkmAudioVideoSegment(string f, unsigned long on, unsigned long off,
                    unsigned long vI = 0, unsigned long fr = 0, bool playing = false) 
    :   filename(f), onset(on), offset(off), index(vI), frame(fr), bPlaying(playing)
    {
        
    }
    
    pkmAudioVideoSegment(string f, unsigned long on, unsigned long off, vector<float> desc, 
                    unsigned long vI = 0, unsigned long fr = 0, bool playing = false) 
    :   filename(f), onset(on), offset(off), descriptor(desc), index(vI), frame(fr), bPlaying(playing)
    {
        
    }
    
    pkmAudioVideoSegment(string f, unsigned long on, unsigned long off, pkm::Mat desc, unsigned long vI = 0, unsigned long fr = 0, bool playing = false) 
    :   filename(f), onset(on), offset(off), descriptor(desc), index(vI), frame(fr), bPlaying(playing)
    {
        
    }
    
    pkmAudioVideoSegment(string f, unsigned long on, unsigned long off, float *desc, unsigned long desc_length, unsigned long vI = 0, unsigned long fr = 0, bool playing = false) 
    :   filename(f), onset(on), offset(off), index(vI), frame(fr), bPlaying(playing)
    {
        descriptor = pkm::Mat(1, desc_length, desc, true);
    }
    
    pkmAudioVideoSegment(const pkmAudioVideoSegment &rhs)
    {
        filename = rhs.filename;
        onset = rhs.onset;
        offset = rhs.offset;
        index = rhs.index;
        frame = rhs.frame;
        descriptor = rhs.descriptor;
        bPlaying = rhs.bPlaying;
    }
    pkmAudioVideoSegment & operator=(const pkmAudioVideoSegment &rhs)
    {
        if (descriptor.data == rhs.descriptor.data) {
            return *this;
        }
        filename = rhs.filename;
        onset = rhs.onset;
        offset = rhs.offset;
        index = rhs.index;
        frame = rhs.frame;
        descriptor = rhs.descriptor;
        bPlaying = rhs.bPlaying;
        
        return *this;
    }
    
    void save(FILE **fp, bool save_feature = false)
    {
        if (*fp) {
            fprintf(*fp, "%s %d %d %d\n", filename.c_str(), onset, offset, index);
            if (save_feature) {
                
            }
        }
    }
    
    void load(FILE **fp, bool read_feature = false)
    {
        if (*fp) {
            char buf[512];
            fscanf(*fp, "%s %d %d %d\n", buf, &onset, &offset, &index);
            filename = string(buf);
            if (read_feature) {
                
            }
        }
    }
    
    string filename;
    unsigned long onset, offset;
    unsigned long index;
    unsigned long frame;
	unsigned long video_frame;
    bool bPlaying;
    pkm::Mat descriptor;
};