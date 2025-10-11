#ifndef OTOPERL_AUDIOUNIT_H
#define OTOPERL_AUDIOUNIT_H
// Otoperl::otoperld::audiounit - run AudioUnit for otoperld.

#include <AudioUnit/AudioUnit.h>

void audiounit_start( bool enable_input, int channel, int sample_rate, void (*callback)(AudioBuffer *outbuf, UInt32 frames, UInt32 channels) );
void audiounit_stop();

#endif