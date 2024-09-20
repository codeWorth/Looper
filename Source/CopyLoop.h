#pragma once

#include "Loop.h"

template<typename T>
class CopyLoop : public Loop<T> {
public:
	CopyLoop<T>() : startedCopy(false) {}

	/*
	* Based on current position of the playhead, copy the given buffer to the corresponding position in the loop.
	* @param buffer Buffer of values to copy.
	* @param currentSample Location of the playhead in samples.
	* @param bufferSize Number of values to copy.
	*/
	void writeBuffer(const T* buffer, size_t currentSample, int bufferSize) override {
		assert(bufferSize <= getSize());
		size_t loopSample = getLoopSample(currentSample, bufferSize);

		// copy to the end of loop from buffer
		size_t samplesToEnd = getSize() - loopSample;
		copyAll(buffer, loopSample, samplesToEnd > bufferSize ? bufferSize : samplesToEnd);

		if (bufferSize >= samplesToEnd) {	// if we copied to or over the loop border
			if (startedCopy) {	// if we had already started, swap in
				swapData(*copyTarget);
			} else {
				startedCopy = true;
			}
		}

		// copy remaining to start of loop if needed
		if (bufferSize > samplesToEnd) {
			copyAll(buffer + samplesToEnd, 0, bufferSize - samplesToEnd);
		}
	}

	void setupCopy(Loop<T>* copyTarget) {
		startedCopy = false;
		this->copyTarget = copyTarget;
	}

private:
	bool startedCopy;
	Loop<T>* copyTarget;

};