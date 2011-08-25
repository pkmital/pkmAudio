/*
 *  pkmAudioFileGPS.h
 *  memoryMosaicSpatialization
 *
 *  Created by Mr. Magoo on 7/7/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */


class pkmAudioFileGPS : public pkmAudioFile
{
public:
	pkmAudioFileGPS(int fs = 512)
	:	pkmAudioFile(fs)
	{
		
	}
	
	pkmAudioFileGPS(float *&buf, int pos, int size, float w = 1.0, int fs = 512)
	:	pkmAudioFile(buf, pos, size, w, fs)
	{

	}
	
	pkmAudioFileGPS(float *&buf, int pos, int size, float w = 1.0, int fs = 512)
	:	pkmAudioFile(buf, pos, size, w, fs)
	{
		
	}
	
	~pkmAudioFile()
	{
		buffer = 0;
		weight = offset = length = 0;
	}
	
	pkmAudioFile(const pkmAudioFile &rhs)
	{
		buffer = rhs.buffer;
		offset = rhs.offset;
		length = rhs.length;
		weight = rhs.weight;
		frame_size = rhs.frame_size;
	}
	
	int getNumFrames()
	{
		return (length - offset) / frame_size;
	}
	
	pkmBinauralizer		binauralizer;
	
	/********************
	 base class:

	 float		*buffer;
	 float		weight;
	 int		offset, 
				length,
				frame_size;

	********************/
};