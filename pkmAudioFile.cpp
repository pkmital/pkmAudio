/*
 *  pkmAudioFile.cpp
 *  autoGrafittiMapKit
 *
 *  Created by Mr. Magoo on 5/12/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "pkmAudioFile.h"
#include <stdio.h>

pkmAudioFile::~pkmAudioFile()
{
	printf("~pkmAudioFile()\n");
	buffer = 0;
	weight = offset = length = 0;
}