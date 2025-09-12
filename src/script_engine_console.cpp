#include "script_engine_console.h"
#include "logger.h"

#define SET_CALLBACK(context, object, name, callback)                    \
    object->Set(context,                                                 \
                v8::String::NewFromUtf8(isolate, name).ToLocalChecked(), \
                v8::Function::New(context, callback).ToLocalChecked())   \
        .Check();

#define ISOLATE                               \
    v8::Isolate *isolate = args.GetIsolate(); \
    v8::HandleScope handle_scope(isolate);

#define SET_MESSAGES                                         \
    const char **messages = new const char *[args.Length()]; \
    for (int i = 0; i < args.Length(); ++i) {                \
        v8::String::Utf8Value str(isolate, args[i]);         \
        messages[i] = strdup(*str);                          \
    }

#define FREE_MESSAGES                         \
    for (int i = 0; i < args.Length(); ++i) { \
        free((void *)messages[i]);            \
    }                                         \
    delete[] messages;

void callback_console_log(const v8::FunctionCallbackInfo<v8::Value> &args) {
    if (args.Length() < 1) {
        return;
    }
    ISOLATE
    SET_MESSAGES
    logger::log(args.Length(), messages);
    FREE_MESSAGES
}

void callback_console_info(const v8::FunctionCallbackInfo<v8::Value> &args) {
    if (args.Length() < 1) {
        return;
    }
    ISOLATE
    SET_MESSAGES
    logger::info(args.Length(), messages);
    FREE_MESSAGES
}

void callback_console_debug(const v8::FunctionCallbackInfo<v8::Value> &args) {
    if (args.Length() < 1) {
        return;
    }
    ISOLATE
    SET_MESSAGES
    logger::debug(args.Length(), messages);
    FREE_MESSAGES
}

void callback_console_warn(const v8::FunctionCallbackInfo<v8::Value> &args) {
    if (args.Length() < 1) {
        return;
    }
    ISOLATE
    SET_MESSAGES
    logger::warn(args.Length(), messages);
    FREE_MESSAGES
}

void callback_console_error(const v8::FunctionCallbackInfo<v8::Value> &args) {
    if (args.Length() < 1) {
        return;
    }
    ISOLATE
    SET_MESSAGES
    logger::error(args.Length(), messages);
    FREE_MESSAGES
}

void callback_console_assert(const v8::FunctionCallbackInfo<v8::Value> &args) {
    if (args.Length() < 2) {
        return;
    }
    ISOLATE

    bool condition = args[0]->BooleanValue(isolate);

    const char **messages = new const char *[args.Length() - 1];
    for (int i = 0; i < args.Length() - 1; ++i) {
        v8::String::Utf8Value str(isolate, args[i + 1]);
        messages[i] = strdup(*str);
    }

    logger::assert(condition, args.Length() - 1, messages);

    for (int i = 0; i < args.Length() - 1; ++i) {
        free((void *)messages[i]);
    }
    delete[] messages;
}

// Setup console object in the V8 context
void script_engine_console::setup(v8::Isolate *isolate, v8::Local<v8::Context> context) {
    // the console object
    v8::Local<v8::Object> console = v8::Object::New(isolate);

    SET_CALLBACK(context, console, "log", callback_console_log);
    SET_CALLBACK(context, console, "info", callback_console_info);
    SET_CALLBACK(context, console, "debug", callback_console_debug);
    SET_CALLBACK(context, console, "warn", callback_console_warn);
    SET_CALLBACK(context, console, "error", callback_console_error);
    SET_CALLBACK(context, console, "assert", callback_console_assert);

    // Set console to global
    context->Global()->Set(context, v8::String::NewFromUtf8(isolate, "console").ToLocalChecked(), console).Check();
};
