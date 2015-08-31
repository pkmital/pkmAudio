/*
 *  pkmAudioFile.h
 *  autoGrafittiMapKit
 *
 *
 *  Created by Parag K. Mital - http://pkmital.com 
 *  Contact: parag@pkmital.com
 *
 *  Copyright 2011 Parag K. Mital. All rights reserved.
 
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