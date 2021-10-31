#pragma once

#ifndef SND_RESAMPLER_H
#define SND_RESAMPLER_H

#include "soxr/soxr.h"
#include <cstring>


#ifndef RESAMPLER_EMPTY_BUFFER_SIZE
#define RESAMPLER_EMPTY_BUFFER_SIZE 1000
#endif

class piecewiseResample {

	int64_t oDoneTotal = 0;
	soxr_t soxrRef;
	soxr_error_t error;
	const soxr_quality_spec_t q_spec = soxr_quality_spec(SOXR_HQ, SOXR_VR);
	const soxr_io_spec_t io_spec = soxr_io_spec(SOXR_INT16_I, SOXR_INT16_I);

	inline static short emptyBuffer[RESAMPLER_EMPTY_BUFFER_SIZE];

public:
	piecewiseResample(int channelCount = 1) {

		soxrRef = soxr_create(1, 1, channelCount, &error, &io_spec, &q_spec, NULL);
	}
	~piecewiseResample() {
		soxr_delete(soxrRef);
	}
	// Just zero out the emptyBuffer
	static const bool init() {
		memset(emptyBuffer, 0, sizeof(emptyBuffer));
		return true;
	}
	inline size_t getSamples(double speed, short* outputBuffer, size_t outSamples, short* inputBuffer, size_t inputBufferLength, size_t inputBufferOffset = 0, bool loop = false);
};


static bool blahblah4235327634 = piecewiseResample::init();


// Returns input sample count used.
size_t piecewiseResample::getSamples(double speed, short* outputBuffer, size_t outSamples, short* inputBuffer, size_t inputBufferLength, size_t inputBufferOffset, bool loop ) {
	soxr_set_io_ratio(soxrRef, speed, outSamples);
	size_t inputDone = 0;
	size_t odone=0,idone=0;
	bool need_input = 1;
	do {
		size_t len = inputBufferLength-inputBufferOffset;
		if (!len) {
			// If sound is looping just continue from start again.
			if (loop && inputBufferLength > 0) {
				inputBufferOffset = 0;
				len = inputBufferLength - inputBufferOffset;
			}
			// Fill with silence until we got enough output.
			else {
				inputBuffer = (short*)&emptyBuffer;
				len = sizeof(emptyBuffer);
			}
		}
		error = soxr_process(soxrRef, inputBuffer + inputBufferOffset, len, &idone, outputBuffer, outSamples, &odone);
		
		outSamples -= odone;
		outputBuffer += odone;
		oDoneTotal += odone;
		inputDone += idone;
		inputBufferOffset += idone;

		/* If soxr_process did not provide the complete block, we must call it
		 * again, supplying more input samples: */
		need_input = outSamples != 0;

	} while (need_input && !error);


	return inputDone; // let calling code know by how much to advance input buffer index
}



#endif