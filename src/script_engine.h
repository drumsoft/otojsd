#ifndef SCRIPT_ENGINE_H
#define SCRIPT_ENGINE_H
// Otojs - live sound programming environment with JavaScript.
// otojsd - sound processing server for Otojs.
// Otojsd::ScriptEngine - JavaScript engine wrapper for otojsd.
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
};

#endif