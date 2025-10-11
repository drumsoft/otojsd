#ifndef SCRIPT_ENGINE_H
#define SCRIPT_ENGINE_H
// Otojsd::ScriptEngine - JavaScript engine wrapper for otojsd.

#include <libplatform/libplatform.h>
#include <v8.h>

struct RenderResult {
    // number if output samples (channels * frames);
    int count;
    // error message if any
    char *error;
};

class ScriptEngine {
    // V8 environment
    std::unique_ptr<v8::Platform> platform_;
    v8::Isolate::CreateParams create_params_;
    v8::Isolate *isolate_;
    v8::Global<v8::Context> context_;

    // Object Cache
    v8::Global<v8::String> no_file_name_;

    // keep render function reference
    v8::Global<v8::Function> render_;

    void resetRender_(v8::Local<v8::Context> context);

public:
    ScriptEngine(const char *exec_path);
    ~ScriptEngine();

    // Execute the given JavaScript code and return the error message if any.
    const char *executeCode(const char *code);

    // Execute the given JavaScript file and return the error message if any.
    const char *executeFromFile(const char *filename);

    // Call the render function with the given input buffer and return the output samples.
    RenderResult executeRender(float *inoutbuf, unsigned int frames, unsigned int channels);

    // Set global variable.
    void setGlobalVariable(const char *name, double value);
};

#endif