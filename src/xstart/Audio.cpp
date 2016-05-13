#include "Audio.h"

FILE* _fopen_nolock(const char* filename, const char* mode) {
	return _fsopen(filename, mode, _SH_DENYNO);
}

//-----------------------------------------------------------------------------
// PortAudioCallback
//-----------------------------------------------------------------------------
int PortAudioCallback(const void* input, void* output, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* user) {
	AudioDevice* device = (AudioDevice*)user;
	device->locked = true;
	device->processInput((float*)input, framesPerBuffer);
	device->processOutput((float*)output, framesPerBuffer);
	device->locked = false;
	return paContinue;
};
