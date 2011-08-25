/*
 *  pkmAudioFile.h
 *  autoGrafittiMapKit
 *
 *
 *  Created by Parag K. Mital - http://pkmital.com 
 *  Contact: parag@pkmital.com
 *
 *  Copyright 2011 Parag K. Mital. All rights reserved.
 * 
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use,
 *	copy, modify, merge, publish, distribute, sublicense, and/or sell
 *	copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following
 *	conditions:
 *	
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,	
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *	OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

class pkmAudioFile
{
public:
	pkmAudioFile(int fs = 512)
	{
		buffer = 0;
		offset = 0;
		length = 0;
		weight = 0;
		playCount = 0;
		frame_size = fs;
		bPlaying = false;
	}
	
	pkmAudioFile(float *&buf, int pos, int size, float w = 1.0, int fs = 512, bool bP = false)
	{
		buffer = buf;
		offset = pos;
		length = size;
		weight = w;
		frame_size = fs;
		bPlaying = bP;
		playCount = 0;
	}
	
	~pkmAudioFile();
	
	pkmAudioFile(const pkmAudioFile &rhs)
	{
		buffer = rhs.buffer;
		offset = rhs.offset;
		length = rhs.length;
		weight = rhs.weight;
		frame_size = rhs.frame_size;
		bPlaying = rhs.bPlaying;
		playCount = rhs.playCount;
	}
	
	int getNumFrames()
	{
		return (length - offset) / frame_size;
	}
	
	
	float		*buffer;
	float		weight;
	int			offset, 
				length;
	
	float		playCount;
	
	bool		bPlaying;
	
	int			frame_size;
};