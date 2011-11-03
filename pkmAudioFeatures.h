/*
 *  pkmAudioFeatures.h
 *
 *  Created by Parag K. Mital - http://pkmital.com 
 *  Contact: parag@pkmital.com
 *
 *  Copyright 2011 Parag K. Mital. All rights reserved.
 * 
 
 Copyright (C) 2011 Parag K. Mital
 
 The Software is and remains the property of Parag K Mital
 ("pkmital") The Licensee will ensure that the Copyright Notice set
 out above appears prominently wherever the Software is used.
 
 The Software is distributed under this Licence: 
 
 - on a non-exclusive basis, 
 
 - solely for non-commercial use in the hope that it will be useful, 
 
 - "AS-IS" and in order for the benefit of its educational and research
 purposes, pkmital makes clear that no condition is made or to be
 implied, nor is any representation or warranty given or to be
 implied, as to (i) the quality, accuracy or reliability of the
 Software; (ii) the suitability of the Software for any particular
 use or for use under any specific conditions; and (iii) whether use
 of the Software will infringe third-party rights.
 
 pkmital disclaims: 
 
 - all responsibility for the use which is made of the Software; and
 
 - any liability for the outcomes arising from using the Software.
 
 The Licensee may make public, results or data obtained from, dependent
 on or arising out of the use of the Software provided that any such
 publication includes a prominent statement identifying the Software as
 the source of the results or the data, including the Copyright Notice
 and stating that the Software has been made available for use by the
 Licensee under licence from pkmital and the Licensee provides a copy of
 any such publication to pkmital.
 
 The Licensee agrees to indemnify pkmital and hold them
 harmless from and against any and all claims, damages and liabilities
 asserted by third parties (including claims for negligence) which
 arise directly or indirectly from the use of the Software or any
 derivative of it or the sale of any products based on the
 Software. The Licensee undertakes to make no liability claim against
 any employee, student, agent or appointee of pkmital, in connection 
 with this Licence or the Software.
 
 
 No part of the Software may be reproduced, modified, transmitted or
 transferred in any form or by any means, electronic or mechanical,
 without the express permission of pkmital. pkmital's permission is not
 required if the said reproduction, modification, transmission or
 transference is done without financial return, the conditions of this
 Licence are imposed upon the receiver of the product, and all original
 and amended source code is included in any transmitted product. You
 may be held legally responsible for any copyright infringement that is
 caused or encouraged by your failure to abide by these terms and
 conditions.
 
 You are not permitted under this Licence to use this Software
 commercially. Use for which any financial return is received shall be
 defined as commercial use, and includes (1) integration of all or part
 of the source code or the Software into a product for sale or license
 by or on behalf of Licensee to third parties or (2) use of the
 Software or any derivative of it for research with the final aim of
 developing software products for sale or license to a third party or
 (3) use of the Software or any derivative of it for research with the
 final aim of developing non-software products for sale or license to a
 third party, or (4) use of the Software to provide any service to an
 external organisation for which payment is received. If you are
 interested in using the Software commercially, please contact pkmital to
 negotiate a licence. Contact details are: parag@pkmital.com
 
 *
 *  Usage:
 *
 *  pkmAudioFeatures *af;
 *
 *  void setup()
 *  {
 *		af = new pkmAudioFeatures();
 *		mfccs = (float *)malloc(sizeof(float) * af->getNumCoefficients());
 *  }
 *
 *  void audioReceived(float *input, int bufferSize, int nChannels)
 *  {
 *		af->computeMFCC(input, mfccs);
 *  }
 *
 */

#pragma once

#include <Accelerate/Accelerate.h>
#include "pkmMatrix.h"
#include "pkmFFT.h"
#include "stdio.h"
#include "string.h"

#define CQ_ENV_THRESH 0.001   // Sparse matrix threshold (for efficient matrix multiplicaton)	

class pkmAudioFeatures
{
public:
	pkmAudioFeatures(int sample_rate = 44100, int fft_size = 2048)
	{
		sampleRate = sample_rate;
		fftN = fft_size;
		
		setup();
	}
	
	~pkmAudioFeatures()
	{
		free(CQT);
		free(cqStart);
		free(cqStop);
		
		free(DCT);
		
		free(cqtVector);
		free(dctVector);
		
		free(fft);
		free(fft_magnitudes);
		free(fft_phases);
		
		free(foutput);
	}
	
	void setup()
	{
		bpoN = 12;
		
		fft = new pkmFFT(fftN);
		fftOutN = fft->fftSizeOver2;
		fft_magnitudes = (float *)malloc(sizeof(float) * fftOutN);
		fft_phases = (float *)malloc(sizeof(float) * fftOutN);
		
		// low C minus quater tone
		loEdge = 55.0 * pow(2.0, 2.5/12.0);		//55.0
		hiEdge = 8000.0;						//8000.0
		
		// Constant-Q bandwidth
		fratio = pow(2.0, 1.0/(float)bpoN);				
		cqtN = (int) floor(log(hiEdge/loEdge)/log(fratio));
		
		if(cqtN<1)
			printf("warning: cqtN not positive definite\n");
		
		// The transformation matrix (mel filters)
		CQT = (float *)malloc(sizeof(float)*cqtN*fftOutN);			
		
		// Sparse matrix coding indices
		cqStart = (int *)malloc(sizeof(int)*cqtN);					
		cqStop = (int *)malloc(sizeof(int)*cqtN);					
		
		// Full spectrum DCT matrix
		dctN = cqtN; 
		DCT = (float *)malloc(sizeof(float)*cqtN*dctN);
		
		// Our transforms (mel bands)
		cqtVector = (float *)malloc(sizeof(float)*cqtN);	
		dctVector = (float *)malloc(sizeof(float)*dctN);	
		
		foutput = (float *)malloc(sizeof(float) * dctN);
		
		// initialize maps
		createLogFreqMap();
		createDCT();
	}
	
	void createLogFreqMap()
	{
		// loop variables
		int	i = 0,
			j = 0;
				
		float *fftfrqs = (float *)malloc(sizeof(float)*fftOutN);		// Actual number of real FFT coefficients
		float *logfrqs = (float *)malloc(sizeof(float)*cqtN);			// Number of constant-Q spectral bins
		float *logfbws = (float *)malloc(sizeof(float)*cqtN);			// Bandwidths of constant-Q bins
		float *mxnorm =  (float *)malloc(sizeof(float)*cqtN);			// CQ matrix normalization coefficients

		float N = (float)fftN;
		for(i = 0; i < fftOutN; i++)
			fftfrqs[i] = i * sampleRate / N;
		
		for(i = 0; i < cqtN; i++)
		{
			logfrqs[i] = loEdge * powf(2.0,(float)i/bpoN);
			logfbws[i] = MAX(logfrqs[i] * (fratio - 1.0), sampleRate / N);
		}
		
		float ovfctr = 0.5475;					// Norm constant so CQT'*CQT close to 1.0
		float tmp,tmp2;
		float *ptr;
		float cqEnvThresh = CQ_ENV_THRESH;		// Sparse matrix threshold (for efficient matrix multiplicaton)	
		
		assert(CQT);
		
		// Build the constant-Q transform (CQT)
		ptr = CQT;
		for(i = 0; i < cqtN; i++)
		{
			mxnorm[i] = 0.0;
			tmp2 = 1.0 / (ovfctr * logfbws[i]);
			for(j = 0; j < fftOutN; j++, ptr++)
			{
				tmp = (logfrqs[i] - fftfrqs[j])*tmp2;
				tmp = expf(-0.5 * tmp*tmp);
				*ptr = tmp;								// row major transform
				mxnorm[i] += tmp*tmp;
			}      
			mxnorm[i] = 2.0 * sqrtf(mxnorm[i]);
		}
		
		// Normalize transform matrix for identity inverse
		ptr = CQT;    
		for(i = 0; i < cqtN; i++)
		{
			cqStart[i] = 0;
			cqStop[i] = 0;
			tmp = 1.0/mxnorm[i];
			for(j = 0; j < fftOutN; j++, ptr++)
			{
				*ptr *= tmp;
				if( (!cqStart[i]) && 
					(cqEnvThresh < *ptr) )
				{
					cqStart[i] = j;
				}
				else if( (!cqStop[i]) && 
						 (cqStart[i]) && 
						 (*ptr < cqEnvThresh) )
				{
					cqStop[i] = j;
				}
			}
		}

		// cleanup local dynamic memory
		free(fftfrqs);
		free(logfrqs);
		free(logfbws);
		free(mxnorm);
	}
	
	void createDCT()
	{

		int i,j;
		float nm = 1 / sqrtf( cqtN / 2.0 );

		assert( DCT );
		
		for( i = 0 ; i < dctN ; i++ )
			for ( j = 0 ; j < cqtN ; j++ )
				DCT[ i * cqtN + j ] = nm * cosf( i * (2.0 * j + 1) * M_PI / 2.0 / (float)cqtN );
		for ( j = 0 ; j < cqtN ; j++ )
			DCT[ j ] *= sqrtf(2.0) / 2.0;
		
	}
	
	void computeMFCCF(float *input, float *&output, int numMFCCS=-1)
	{
		// should window input buffer before FFT
		fft->forward(0, input, fft_magnitudes, fft_phases);
		
		// sparse matrix product of CQT * FFT
		int a = 0;
		float *ptr1 = 0;
		
		/*
		 for( a = 0; a < cqtN ; a++ )
		 {
		 ptr1 = cqtVector + a; // constant-Q transform vector
		 *ptr1 = 0.0;
		 ptr2 = CQT + a * fftOutN + cqStart[a];
		 ptr3 = fft_magnitudes + cqStart[a];
		 b = cqStop[a] - cqStart[a];
		 while(b--){
		 *ptr1 += *ptr2++ * *ptr3++;
		 }
		 }
		 */
		
		vDSP_mmul(fft_magnitudes, 1, CQT, 1, cqtVector, 1, 1, cqtN, fftOutN);
		
		// LFCC 
		a = cqtN;
		ptr1 = cqtVector;
		while( a-- ){
			float f = *ptr1;
			*ptr1++ = f == 0 ? 0 : log10f( f*f );
		}
		
		/*
		 a = dctN;
		 ptr2 = DCT; // point to column of DCT
		 mfccPtr = output;
		 while( a-- )
		 {
		 ptr1 = cqtVector;  // point to cqt vector
		 *mfccPtr = 0.0; 
		 b = cqtN;
		 while( b-- )
		 *mfccPtr += (float)(*ptr1++ * *ptr2++);
		 mfccPtr++;	
		 }
		 */
		
		if (numMFCCS == -1) {
			vDSP_mmul(cqtVector, 1, DCT, 1, output, 1, 1, dctN, cqtN);
			
			float n = dctN;
			vDSP_vsdiv(output, 1, &n, output, 1, dctN);
			
		}
		else {
			vDSP_mmul(cqtVector, 1, DCT, 1, foutput, 1, 1, dctN, cqtN);
			cblas_scopy(numMFCCS, foutput, 1, output, 1);
			
			float n = dctN;
			vDSP_vsdiv(output, 1, &n, output, 1, numMFCCS);
			
		}
		
		//float n = (float) dctN;
		//vDSP_vsdiv(output, 1, &n, output, 1, dctN);
		
	}
	
	void computeMFCCFromMagnitudesF(float *fft_magnitudes, float *&output, int numMFCCS=-1)
	{
		// sparse matrix product of CQT * FFT
		int a = 0;
		float *ptr1 = 0;
		
		/*
		 for( a = 0; a < cqtN ; a++ )
		 {
		 ptr1 = cqtVector + a; // constant-Q transform vector
		 *ptr1 = 0.0;
		 ptr2 = CQT + a * fftOutN + cqStart[a];
		 ptr3 = fft_magnitudes + cqStart[a];
		 b = cqStop[a] - cqStart[a];
		 while(b--){
		 *ptr1 += *ptr2++ * *ptr3++;
		 }
		 }
		 */
		
		vDSP_mmul(fft_magnitudes, 1, CQT, 1, cqtVector, 1, 1, cqtN, fftOutN);
		
		// LFCC 
		a = cqtN;
		ptr1 = cqtVector;
		while( a-- ){
			float f = *ptr1;
			*ptr1++ = log10f( f*f );
		}
		
		/*
		 a = dctN;
		 ptr2 = DCT; // point to column of DCT
		 mfccPtr = output;
		 while( a-- )
		 {
		 ptr1 = cqtVector;  // point to cqt vector
		 *mfccPtr = 0.0; 
		 b = cqtN;
		 while( b-- )
		 *mfccPtr += (float)(*ptr1++ * *ptr2++);
		 mfccPtr++;	
		 }
		 */
		
		if (numMFCCS == -1) {
			vDSP_mmul(cqtVector, 1, DCT, 1, output, 1, 1, dctN, cqtN);
			
			float n = dctN;
			vDSP_vsdiv(output, 1, &n, output, 1, dctN);

		}
		else {
			vDSP_mmul(cqtVector, 1, DCT, 1, foutput, 1, 1, dctN, cqtN);
			cblas_scopy(numMFCCS, foutput, 1, output, 1);
			
			float n = dctN;
			vDSP_vsdiv(output, 1, &n, output, 1, numMFCCS);

		}
		
		//float n = (float) dctN;
		//vDSP_vsdiv(output, 1, &n, output, 1, dctN);
		
	}
	
	float *getMagnitudes()
	{
		return fft_magnitudes;
	}
	
	float *getPhases()
	{
		return fft_phases;
	}
	
	void computeMFCCD(float *input, double*& output, int numMFCCS = -1)
	{
		
		// should window input buffer before FFT
		fft->forward(0, input, fft_magnitudes, fft_phases);
		
		// sparse matrix product of CQT * FFT
		int a = 0;
		float *ptr1 = 0;
		
		vDSP_mmul(fft_magnitudes, 1, CQT, 1, cqtVector, 1, 1, cqtN, fftOutN);
		
		// LFCC 
		a = cqtN;
		ptr1 = cqtVector;
		while( a-- ){
			float f = *ptr1;
			*ptr1++ = log10f( f*f );
		}
		
		vDSP_mmul(cqtVector, 1, DCT, 1, foutput, 1, 1, dctN, cqtN);
		if (numMFCCS == -1) {
			vDSP_vspdp(foutput, 1, output, 1, dctN);
			
			double n = dctN;
			vDSP_vsdivD(output, 1, &n, output, 1, dctN);
			//double rms_feature = pkm::Mat::rms(output, cqtN);
			//output[1] = expf(rms_feature);
		}
		else {
			vDSP_vspdp(foutput, 1, output, 1, numMFCCS);
			
			double n = dctN;
			vDSP_vsdivD(output, 1, &n, output, 1, numMFCCS);
			//double rms_feature = pkm::Mat::rms(output, cqtN);
			//output[1] = expf(rms_feature);
		}

	}
	
	void computeMFCCFromMagnitudesD(float *fft_magnitudes, double*& output, int numMFCCS = -1)
	{
		
		// sparse matrix product of CQT * FFT
		int a = 0;
		float *ptr1 = 0;
		
		vDSP_mmul(fft_magnitudes, 1, CQT, 1, cqtVector, 1, 1, cqtN, fftOutN);
		
		// LFCC 
		a = cqtN;
		ptr1 = cqtVector;
		while( a-- ){
			float f = *ptr1;
			*ptr1++ = log10f( f*f );
		}
		
		vDSP_mmul(cqtVector, 1, DCT, 1, foutput, 1, 1, dctN, cqtN);
		if (numMFCCS == -1) {
			vDSP_vspdp(foutput, 1, output, 1, dctN);
			
			double n = dctN;
			vDSP_vsdivD(output, 1, &n, output, 1, dctN);
			//double rms_feature = pkm::Mat::rms(output, cqtN);
			//output[1] = expf(rms_feature);
		}
		else {
			vDSP_vspdp(foutput, 1, output, 1, numMFCCS);
			
			double n = dctN;
			vDSP_vsdivD(output, 1, &n, output, 1, numMFCCS);
			//double rms_feature = pkm::Mat::rms(output, cqtN);
			//output[1] = expf(rms_feature);
		}
		
	}
	
	inline int getNumCoefficients()
	{
		return dctN;
	}
	
	
private:
	float			*sample_data,
					*powerSpectrum;
	
	float			*discreteCosineTransformStorage,		// storage
					*constantQTransformStorage;

	float			*DCT,									// coefficients
					*CQT;
	
	float			*cqtVector,								// transforms
					*dctVector;
	
	int				*cqStart,								// sparse matrix indices
					*cqStop;
	
	float			loEdge,									// tranform range
					hiEdge;
	
	float			fratio;
	
	pkmFFT			*fft;
	
	float			*fft_magnitudes,
					*fft_phases;

	float			*foutput;
	
	int				bpoN,
					cqtN,
					dctN,
					fftN,
					fftOutN,
					sampleRate;
    
	
};

