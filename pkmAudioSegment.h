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

using namespace std;

class pkmAudioSegment
{  
public:
    
    pkmAudioSegment() 
    : onset(0), offset(0), index(0), frame(0), video_frame(0), bPlaying(false), bNeedsReset(false)
    {
        bStoringAudioData = false;
    }
    
    pkmAudioSegment(float *buf, unsigned long on, unsigned long off,
                    unsigned long vI = 0, unsigned long fr = 0, bool playing = false) 
    :   onset(on), offset(off), index(vI), frame(fr), video_frame(0), bPlaying(playing), bNeedsReset(false)
    {
        buffer = (float *)malloc(sizeof(float) * off);
        cblas_scopy(off, buf, 1, buffer, 1);
        bStoringAudioData = true;
    }
    
    pkmAudioSegment(string f, unsigned long on, unsigned long off,
                    unsigned long vI = 0, unsigned long fr = 0, bool playing = false) 
    :   filename(f), onset(on), offset(off), index(vI), frame(fr), video_frame(0), bPlaying(playing), bNeedsReset(false)
    {
        bStoringAudioData = false;
    }
    
    pkmAudioSegment(string f, unsigned long on, unsigned long off, vector<float> desc, 
                    unsigned long vI = 0, unsigned long fr = 0, bool playing = false) 
    :   filename(f), onset(on), offset(off), descriptor(desc), index(vI), frame(fr), bPlaying(playing), video_frame(0), bNeedsReset(false)
    {
        bStoringAudioData = false;
    }
    
    pkmAudioSegment(string f, unsigned long on, unsigned long off, pkm::Mat desc, unsigned long vI = 0, unsigned long fr = 0, bool playing = false) 
    :   filename(f), onset(on), offset(off), descriptor(desc), index(vI), frame(fr), bPlaying(playing), video_frame(0), bNeedsReset(false)
    {
        bStoringAudioData = false;
    }
    
    pkmAudioSegment(string f, unsigned long on, unsigned long off, float *desc, unsigned long desc_length, unsigned long vI = 0, unsigned long fr = 0, bool playing = false) 
    :   filename(f), onset(on), offset(off), index(vI), frame(fr), bPlaying(playing), video_frame(0), bNeedsReset(false)
    {
        descriptor = pkm::Mat(1, desc_length, desc, true);
        bStoringAudioData = false;
    }
    
    pkmAudioSegment(const pkmAudioSegment &rhs)
    {
        filename = rhs.filename;
        onset = rhs.onset;
        offset = rhs.offset;
        index = rhs.index;
        frame = rhs.frame;
        descriptor = rhs.descriptor;
        bPlaying = rhs.bPlaying;
		audio_frame = rhs.audio_frame;
		video_frame = rhs.video_frame;
		audio_file_name = rhs.audio_file_name;
		video_file_name = rhs.video_file_name;
		audio_rate = rhs.audio_rate;
        video_rate = rhs.video_rate;
        current_video_frame = rhs.current_video_frame;
        
        bStoringAudioData = rhs.bStoringAudioData;
        bNeedsReset = rhs.bNeedsReset;
        bPlaying = rhs.bPlaying;
    }
    pkmAudioSegment & operator=(const pkmAudioSegment &rhs)
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
		audio_frame = rhs.audio_frame;
        video_frame = rhs.video_frame;
		audio_file_name = rhs.audio_file_name;
		video_file_name = rhs.video_file_name;
		audio_rate = rhs.audio_rate;
        video_rate = rhs.video_rate;
        current_video_frame = rhs.current_video_frame;
        
        bStoringAudioData = rhs.bStoringAudioData;
        bNeedsReset = rhs.bNeedsReset;
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
	
	pkmAudioSegment(string af_name, string vf_name, unsigned long af, unsigned long vf, unsigned long numaf, unsigned long numvf, float ar, float vr, int audio_frame_size)
	: 
	audio_file_name(af_name), video_file_name(vf_name), 
	audio_frame(af), video_frame(vf), 
	num_audio_frames(numaf), num_video_frames(numvf), 
	audio_rate(ar), video_rate(vr),
	bPlaying(false)
	{
		onset = audio_frame * audio_frame_size;
		frame = 0;
		current_video_frame = 0;
		offset = (audio_frame + num_audio_frames) * audio_frame_size;
		index = 0;
        bStoringAudioData = false;
	}
	
    ~pkmAudioSegment()
    {
        if(bStoringAudioData)
        {
            //cout << "deleting ..." << endl;
            free(buffer);
        }
        bStoringAudioData = false;
    }
    
	void setDescriptor(float *desc, int desc_length)
	{
		descriptor = pkm::Mat(1, desc_length, desc, true);
	}
    
	void setDescriptor(pkm::Mat desc)
	{
		descriptor = desc;
	}
	
	void updateVideoFrame()
	{
		current_video_frame = frame * video_rate / audio_rate;
	}
	
    string filename;
	
    unsigned long onset, offset;
    unsigned long index;
    unsigned long frame;				// current audio frame
	
    unsigned long current_video_frame;
	
	unsigned long video_frame;
	unsigned long audio_frame;
	
	string audio_file_name;
	string video_file_name;
	
	unsigned long num_audio_frames;
	unsigned long num_video_frames;
	
	double audio_rate;
	double video_rate;
    double audio_position;
    
    bool bNeedsReset;               // triggered to reset to frame 0
	bool bPlaying;                  // already playing, so frame > 0
    bool bStoringAudioData;         // store the audio buffer *
    float *buffer;
    pkm::Mat descriptor;
};