/*
 *  pkmAudioWindow.cpp
 *  memoryMosaic
 *
 *  Created by Mr. Magoo on 7/7/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "pkmAudioWindow.h"


int pkmAudioWindow::frameSize;
int pkmAudioWindow::rampInLength;
int pkmAudioWindow::rampOutLength;
float * pkmAudioWindow::rampInBuffer;
float * pkmAudioWindow::rampOutBuffer;
float * pkmAudioWindow::window;