#ifndef CORE_AUDIO_UTILITIES_H
#define CORE_AUDIO_UTILITIES_H
// Otoperl::otoperld::coreaudioutilities - run Core Audio Utilities.

void setSamplingRateToDevice(AudioObjectID objectID, Float64 samplingRate);

AudioObjectID getDefaultDeviceID(bool isInput);

AudioObjectID getInputOutputDevice(AudioObjectID inputDevice, AudioObjectID outputDevice, int channels);

AudioBufferList * allocAudioBufferList(UInt32 channels, UInt32 frames);

void deallocAudioBufferList(AudioBufferList *bufferList);

#endif