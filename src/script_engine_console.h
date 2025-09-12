#ifndef SCRIPT_ENGINE_CONSOLE_H
#define SCRIPT_ENGINE_CONSOLE_H

#include <v8.h>

namespace script_engine_console {

// Setup console object in the V8 context
void setup(v8::Isolate *isolate, v8::Local<v8::Context> context);

}; // namespace script_engine_console

#endif // SCRIPT_ENGINE_CONSOLE_H