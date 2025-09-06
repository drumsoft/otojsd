// Otojs - live sound programming environment with JavaScript.
// otojsd - sound processing server for Otojs.
/*
    Otojs - live sound programming environment with JavaScript.
    Copyright (C) 2025- Haruka Kataoka

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

#include <CoreFoundation/CoreFoundation.h>
#include <pthread.h>

#include "otojsd.h"
#include "script_engine.h"
#include "const.h"
#include "codeserver.h"
#include "audiounit.h"
#include "aiffrecorder.h"

// ------------------------------------------------------ private functions
void script_audio_callback(AudioBuffer *outbuf, UInt32 frames, UInt32 channels);
const char *script_code_liveeval(const char *code);

void otojsd__stop(int sig);

// ------------------------------------------------ otojsd implimentation

ScriptEngine *se;

codeserver *cs;
AiffRecorder *ar;
float *recordBuffer;
bool running = false;

pthread_mutex_t mutex_for_script_engine;
pthread_cond_t cond_for_script_engine;

bool has_runtime_error;

bool input_enabled;

void otojsd_start(otojsd_options *options, const char *exec_path, char **env) {
	const char *start_code = OTOJSD_DEFAULT_STARTCODE;
	printf("otojsd - Otojs sound server - start with %s.\n", start_code);
	printf("otojsd port: %d, allowed clients: %s\n", options->port, options->allow_pattern);
	printf("*WARNING* don't expose otojsd port to network.\n");

	if (options->output) {
		printf("recording: %s\n", options->output);
		recordBuffer = (float *)malloc(options->channel);
		ar = AiffRecorder_create(options->channel, 32, options->sample_rate);
		AiffRecorder_open(ar, options->output);
	}else{
		ar = NULL;
	}

	input_enabled = options->enable_input;

	pthread_mutex_init( &mutex_for_script_engine , NULL );
	pthread_cond_init( &cond_for_script_engine, NULL );

	se = new ScriptEngine(exec_path);
	se->executeFromFile(start_code);

	has_runtime_error = false;
	audiounit_start(options->enable_input, options->channel, options->sample_rate, script_audio_callback);

	cs = codeserver_init(options->port, options->findfreeport, options->allow_pattern, options->verbose, script_code_liveeval);
	codeserver_start(cs);

	running = true;

	if (SIG_ERR == signal(SIGINT, otojsd__stop)) {
		printf("failed to set signal handler.n");
		running = false;
	}

	while(running){
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, false);
		if (!codeserver_run(cs)) {
			running = false;
		}
	}
	
	codeserver_stop(cs);

	audiounit_stop();

	if (ar) {
		AiffRecorder_close(ar);
		AiffRecorder_destroy(ar);
		free(recordBuffer);
	}

	delete se;

	pthread_mutex_destroy( &mutex_for_script_engine );
	pthread_cond_destroy( &cond_for_script_engine );

	printf("otojsd - stopped\n");
}

void otojsd__stop(int sig) {
	running = false;
}

void script_audio_callback(AudioBuffer *outbuf, UInt32 frames, UInt32 channels) {
	UInt32 channel, frame;
	pthread_mutex_lock( &mutex_for_script_engine );

	size_t data_size = frames * channels * sizeof(Float32);
	Float32 *inoutbuf = new Float32[frames * channels];

	// copy input buffer to inoutbuf
	if (input_enabled) {
		int i = 0;
		for (frame = 0; frame < frames; frame++) {
			for (channel = 0; channel < channels; channel++) {
				inoutbuf[i++] = ((Float32 *)( outbuf[channel].mData ))[frame];
			}
		}
	}

	// スクリプトエンジンで render() の実行（戻り値が count）
	RenderResult result = se->executeRender(inoutbuf, frames, channels);

	// エラー時はエラーテキストを print して has_runtime_error を true にセット
	if (result.error) {
		if ( ! has_runtime_error ) {
			has_runtime_error = true;
			printf ("render runtime error: %s\n", result.error);
		}
	} else {
		has_runtime_error = false;
		int i = 0;
		if (ar) {
			for (frame = 0; frame < frames; frame++) {
				for (channel = 0; channel < channels; channel++) {
					recordBuffer[channel] = ((Float32 *)( outbuf[channel].mData ))[frame] = inoutbuf[i++];
					if (i >= result.count) break;
				}
				AiffRecorder_write32bit(ar, (uint32_t *)recordBuffer, 1);
			}
		} else {
			for (frame = 0; frame < frames; frame++) {
				for (channel = 0; channel < channels; channel++) {
					((Float32 *)( outbuf[channel].mData ))[frame] = inoutbuf[i++];
					if (i >= result.count) break;
				}
			}
		}
	}

	delete inoutbuf;
	
	pthread_mutex_unlock( &mutex_for_script_engine );
	pthread_cond_signal( &cond_for_script_engine );
}

const char *script_code_liveeval(const char *code) {
	pthread_mutex_lock( &mutex_for_script_engine );
	pthread_cond_wait( &cond_for_script_engine, &mutex_for_script_engine );

	const char *rettext = se->executeCode(code);

	has_runtime_error = false;
	pthread_mutex_unlock( &mutex_for_script_engine );

	return rettext;
}

