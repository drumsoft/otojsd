// Otojsd::ScriptEngine - JavaScript engine wrapper for otojsd.

#include <string>

#include "script_engine.h"
#include "script_engine_console.h"
#include "const.h"

// internal functions prototypes
const char *ToCString(const v8::String::Utf8Value &value);
const char *ExecuteString(v8::Isolate *isolate, v8::Local<v8::Context> context, v8::Local<v8::String> source, v8::Local<v8::String> name);
v8::MaybeLocal<v8::String> ReadFile(v8::Isolate *isolate, const char *name);
const char *FormatException(v8::Isolate *isolate, v8::TryCatch *try_catch);

// -------------------- create/destroy

ScriptEngine::ScriptEngine(const char *exec_path) {
    // Initialize V8.
    v8::V8::InitializeICUDefaultLocation(exec_path);
    v8::V8::InitializeExternalStartupData(exec_path);
    this->platform_ = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform_.get());
    v8::V8::Initialize();

    // Create a new Isolate and make it the current one.
    this->create_params_.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    this->isolate_ = v8::Isolate::New(create_params_);
    v8::Isolate::Scope isolate_scope(this->isolate_);

    // Create a stack-allocated handle scope.
    v8::HandleScope handle_scope(this->isolate_);

    // Create a new context and store it in the global context_ and enter it.
    v8::Local<v8::Context> context = v8::Context::New(this->isolate_);
    this->context_.Reset(this->isolate_, context);
    v8::Context::Scope context_scope(context);

    // Setup console object
    script_engine_console::setup(this->isolate_, context);

    // Create reusable string cache.
    v8::Local<v8::String> no_file_name_local = v8::String::NewFromUtf8(isolate_, "posted").ToLocalChecked();
    this->no_file_name_.Reset(this->isolate_, no_file_name_local);
}

ScriptEngine::~ScriptEngine() {
    render_.Reset();
    context_.Reset();
    no_file_name_.Reset();

    isolate_->Dispose();
    v8::V8::Dispose();
    v8::V8::DisposePlatform();
    delete create_params_.array_buffer_allocator;
}

// -------------------- private functions

void ScriptEngine::resetRender_(v8::Local<v8::Context> context) {
    v8::Local<v8::String> process_name = v8::String::NewFromUtf8(this->isolate_, RENDER_FUNCTION_NAME).ToLocalChecked();
    v8::Local<v8::Value> process_val;
    // If there is no render function, or if it is not a function, bail out
    if (
        !context->Global()->Get(context, process_name).ToLocal(&process_val) ||
        !process_val->IsFunction()
    ) {
        return;
    }
    // cast it to a Function and store it.
    v8::Local<v8::Function> render_fun = process_val.As<v8::Function>();
    render_.Reset(this->isolate_, render_fun);
}

// -------------------- public functions

// Execute the given JavaScript code and return the error message if any.
const char *ScriptEngine::executeCode(const char *code) {
    v8::Isolate::Scope isolate_scope(this->isolate_);
    v8::HandleScope handle_scope(this->isolate_);
    v8::Local<v8::Context> local_context = this->context_.Get(this->isolate_);
    v8::Context::Scope context_scope(local_context);

    v8::Local<v8::String> local_no_file_name = this->no_file_name_.Get(this->isolate_);

    v8::Local<v8::String> source = v8::String::NewFromUtf8(this->isolate_, code).ToLocalChecked();
    const char *result = ExecuteString(isolate_, local_context, source, local_no_file_name);
    if (result == nullptr) {
        this->resetRender_(local_context);
    }
    return result;
}

// Execute the given JavaScript file and return the error message if any.
const char *ScriptEngine::executeFromFile(const char *filename) {
    v8::Isolate::Scope isolate_scope(this->isolate_);
    v8::HandleScope handle_scope(this->isolate_);
    v8::Local<v8::Context> local_context = this->context_.Get(this->isolate_);
    v8::Context::Scope context_scope(local_context);

    v8::Local<v8::String> file_name = v8::String::NewFromUtf8(isolate_, filename).ToLocalChecked();
    v8::Local<v8::String> source;
    if (!ReadFile(isolate_, filename).ToLocal(&source)) {
        fprintf(stderr, "Error reading '%s'\n", filename);
        exit(1);
    }
    const char *result = ExecuteString(isolate_, local_context, source, file_name);
    if (result == nullptr) {
        this->resetRender_(local_context);
    }
    return result;
}

// Call the render function with the given input buffer and return the output samples.
RenderResult ScriptEngine::executeRender(float *inoutbuf, unsigned int frames, unsigned int channels) {
    RenderResult result = {0, nullptr};

    v8::Isolate::Scope isolate_scope(this->isolate_);
    v8::HandleScope handle_scope(this->isolate_);
    v8::Local<v8::Context> local_context = this->context_.Get(this->isolate_);
    v8::Context::Scope context_scope(local_context);

    v8::TryCatch try_catch(this->isolate_);

    // create arguments for render(frames, channels, input_array)
    const int argc = 3;
    v8::Local<v8::Number> frames_arg = v8::Number::New(this->isolate_, frames);
    v8::Local<v8::Number> channels_arg = v8::Number::New(this->isolate_, channels);
    v8::Local<v8::Float32Array> input_array;
    v8::Local<v8::Primitive> undefined = v8::Undefined(this->isolate_);
    if (inoutbuf) {
        size_t byte_length = frames * channels * sizeof(float);
        v8::Local<v8::ArrayBuffer> array_buffer = v8::ArrayBuffer::New(isolate_, byte_length);
        void* buffer_data = array_buffer->GetBackingStore()->Data();
        memcpy(buffer_data, inoutbuf, byte_length);
        input_array = v8::Float32Array::New(array_buffer, 0, frames * channels);
    }
    v8::Local<v8::Value> argv[argc] = {
        frames_arg,
        channels_arg,
        inoutbuf ? (v8::Local<v8::Value>)input_array : (v8::Local<v8::Value>)undefined
    };

    // call render()
    v8::Local<v8::Function> render = this->render_.Get(this->isolate_);

    // check result
    v8::Local<v8::Value> call_result;
    if (!render->Call(local_context, local_context->Global(), argc, argv).ToLocal(&call_result)) {
        v8::String::Utf8Value error(this->isolate_, try_catch.Exception());
        result.error = strdup(*error);
        return result;
    }
    if (!call_result->IsFloat32Array()) {
        result.error = strdup("Return value from render is not Float32Array");
        return result;
    }

    // copy result to inoutbuf
    v8::Local<v8::Float32Array> ret_array = call_result.As<v8::Float32Array>();
    std::shared_ptr<v8::BackingStore> backing = ret_array->Buffer()->GetBackingStore();
    memcpy(inoutbuf, static_cast<float *>(backing->Data()), ret_array->Length() * sizeof(float));
    result.count = ret_array->Length();

    return result;
}

// Set global variable.
void ScriptEngine::setGlobalVariable(const char *name, double value) {
    v8::Isolate::Scope isolate_scope(this->isolate_);
    v8::HandleScope handle_scope(this->isolate_);
    v8::Local<v8::Context> local_context = this->context_.Get(this->isolate_);
    v8::Context::Scope context_scope(local_context);

    v8::Local<v8::String> var_name = v8::String::NewFromUtf8(this->isolate_, name).ToLocalChecked();
    v8::Local<v8::Number> var_value = v8::Number::New(this->isolate_, value);
    local_context->Global()->Set(local_context, var_name, var_value).FromJust();
}

// -------------------- internal functions

// Extracts a C string from a V8 Utf8Value.
const char *ToCString(const v8::String::Utf8Value &value) {
    return *value ? *value : "<string conversion failed>";
}

// Executes a string within the current v8 context.
const char *ExecuteString(v8::Isolate *isolate, v8::Local<v8::Context> context,
                   v8::Local<v8::String> source, v8::Local<v8::String> name) {
    v8::TryCatch try_catch(isolate);
    v8::ScriptOrigin origin(name);
    v8::Local<v8::Script> script;
    if (!v8::Script::Compile(context, source, &origin).ToLocal(&script)) {
        return FormatException(isolate, &try_catch);
    }
    if (script->Run(context).IsEmpty()) {
        return FormatException(isolate, &try_catch);
    }
    return nullptr;
}

v8::MaybeLocal<v8::String> ReadFile(v8::Isolate *isolate, const char *name) {
    FILE *file = fopen(name, "rb");
    if (file == nullptr)
        return {};

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    char *chars = new char[size + 1];
    chars[size] = '\0';
    for (size_t i = 0; i < size;) {
        i += fread(&chars[i], 1, size - i, file);
        if (ferror(file)) {
            fclose(file);
            return {};
        }
    }
    fclose(file);
    v8::MaybeLocal<v8::String> result = v8::String::NewFromUtf8(
        isolate, chars, v8::NewStringType::kNormal, static_cast<int>(size));
    delete[] chars;
    return result;
}

const char *FormatException(v8::Isolate *isolate, v8::TryCatch *try_catch) {
    std::string result = "";
    v8::String::Utf8Value exception(isolate, try_catch->Exception());
    const char *exception_string = ToCString(exception);
    v8::Local<v8::Message> message = try_catch->Message();
    if (message.IsEmpty()) {
        result += exception_string;
    } else {
        // Print (filename):(line number): (message).
        v8::String::Utf8Value filename(isolate,
                                       message->GetScriptOrigin().ResourceName());
        v8::Local<v8::Context> context(isolate->GetCurrentContext());
        const char *filename_string = ToCString(filename);
        int linenum = message->GetLineNumber(context).FromJust();
        int start = message->GetStartColumn(context).FromJust();
        int end = message->GetEndColumn(context).FromJust();
        result += filename_string;
        result += ":";
        result += std::to_string(linenum);
        result += ":";
        result += std::to_string(start);
        result += ": ";
        result += exception_string;
        result += "\n";
        // Print line of source code.
        v8::String::Utf8Value sourceline(
            isolate, message->GetSourceLine(context).ToLocalChecked());
        const char *sourceline_string = ToCString(sourceline);
        result += sourceline_string;
        result += "\n";
        // Print wavy underline (GetUnderline is deprecated).
        for (int i = 0; i < start; i++) {
            result += " ";
        }
        for (int i = start; i < end; i++) {
            result += "^";
        }
        result += "\n";
        v8::Local<v8::Value> stack_trace_string;
        if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) &&
            stack_trace_string->IsString() &&
            stack_trace_string.As<v8::String>()->Length() > 0) {
            v8::String::Utf8Value stack_trace(isolate, stack_trace_string);
            const char *err = ToCString(stack_trace);
            result += err;
            result += "\n";
        }
    }
    return strdup(result.c_str());
}
