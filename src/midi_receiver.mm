#include "midi_receiver.h"

#include <format>

#include "logger.h"

void midiNotyfyCallback__(const struct MIDINotification *message, void *refCon);

// private methods

CFStringRef MidiReceiver::getMIDIObjectNames_(MIDIObjectRef objectRef) {
    CFStringRef endpointName = NULL;     // kMIDIPropertyName
    CFStringRef modelName = NULL;        // kMIDIPropertyModel
    CFStringRef manufacturerName = NULL; // kMIDIPropertyManufacturer
    CFStringRef fullName = NULL;
    OSStatus err;

    err = MIDIObjectGetStringProperty(objectRef, kMIDIPropertyName, &endpointName);
    if (err != noErr) {
        logger::error(std::format("MIDIObjectGetStringProperty(Name for {}) error: {}", objectRef, (long int)err));
    }
    err = MIDIObjectGetStringProperty(objectRef, kMIDIPropertyModel, &modelName);
    if (err != noErr) {
        logger::error(std::format("MIDIObjectGetStringProperty(Model for {}) error: {}", objectRef, (long int)err));
    }
    err = MIDIObjectGetStringProperty(objectRef, kMIDIPropertyManufacturer, &manufacturerName);
    if (err != noErr) {
        logger::error(std::format("MIDIObjectGetStringProperty(Manufacturer for {}) error: {}", objectRef, (long int)err));
    }

    fullName = CFStringCreateWithFormat(
        kCFAllocatorDefault,
        NULL,
        CFSTR("%@, %@, %@"),
        manufacturerName ? manufacturerName : CFSTR(""),
        modelName ? modelName : CFSTR(""),
        endpointName ? endpointName : CFSTR("")
    );

RELEASE_AND_RETURN:
    if (endpointName) {
        CFRelease(endpointName);
    }
    if (modelName) {
        CFRelease(modelName);
    }
    if (manufacturerName) {
        CFRelease(manufacturerName);
    }
    return fullName;
}

bool MidiReceiver::isEndpointMatched_(CFStringRef endpointName) {
    if (!endpointName) {
        return false;
    }
    if (this->sourceName_.empty()) {
        return true;
    }
    std::string strEndpointName = std::string(CFStringGetCStringPtr(endpointName, kCFStringEncodingUTF8));
    std::transform(strEndpointName.cbegin(), strEndpointName.cend(), strEndpointName.begin(), toupper);
    return strEndpointName.find(this->sourceName_) != std::string::npos;
}

void MidiReceiver::connectSource_(MIDIEndpointRef sourceRef, CFStringRef name) {
    const char *namePtr = name ? CFStringGetCStringPtr(name, kCFStringEncodingUTF8) : "unknown";
    OSStatus err = MIDIPortConnectSource(inPort_, sourceRef, NULL);
    if (err == noErr) {
        logger::log(std::format("Connected to MIDI source {}", namePtr));
    } else {
        logger::error(std::format("Failed to connect to MIDI source {}, error: {}", namePtr, (long int)err));
    }
}

// public methods

MidiReceiver::MidiReceiver(std::string sourceName, void (*receiving_callback)(size_t length, const unsigned char *data)) {
    OSStatus err;

    // create MIDI client
    err = MIDIClientCreate(MIDI_CLIENT_NAME, midiNotyfyCallback__, this, &client_);
    if (err) {
        logger::error(std::format("MIDIClientCreate error: {}", (long int)err));
        return;
    }

    // create MIDI input port
    err = MIDIInputPortCreateWithProtocol(
        client_,
        MIDI_INPUT_PORT_NAME,
        kMIDIProtocol_1_0,
        &inPort_,
        ^(const MIDIEventList *eventList, void *srcConnRefCon) {
          this->midiReceiveCallback_(eventList, srcConnRefCon);
        });
    if (err) {
        logger::error(std::format("MIDIInputPortCreate error: {}", (long int)err));
        return;
    }

    this->sourceName_ = sourceName;
    std::transform(this->sourceName_.cbegin(), this->sourceName_.cend(), this->sourceName_.begin(), toupper);

    this->receiving_callback_ = receiving_callback;
}

MidiReceiver::~MidiReceiver() {
    if (inPort_) {
        MIDIPortDispose(inPort_);
    }
    if (client_) {
        MIDIClientDispose(client_);
    }
}

void MidiReceiver::connectDevices() {
    ItemCount count = MIDIGetNumberOfSources();
    for (ItemCount i = 0; i < count; ++i) {
        MIDIEndpointRef sourceRef = MIDIGetSource(i); // MIDIEndpointRef = MIDIObjectRef
        CFStringRef name = getMIDIObjectNames_(sourceRef);
        logger::info(std::format("Found MIDI source: {}", name ? CFStringGetCStringPtr(name, kCFStringEncodingUTF8) : "unknown"));
        if (this->isEndpointMatched_(name)) {
            this->connectSource_(sourceRef, name);
        }
        if (name) {
            CFRelease(name);
        }
    }
}

void MidiReceiver::midiNotyfyCallback_(const struct MIDINotification *message) {
    CFStringRef name = NULL;
    MIDIObjectAddRemoveNotification *addRemoveRef = NULL;
    MIDIObjectPropertyChangeNotification *changeRef = NULL;
    MIDIIOErrorNotification *errorRef = NULL;
    switch (message->messageID) {
    case kMIDIMsgObjectAdded:
        addRemoveRef = (MIDIObjectAddRemoveNotification *)message;
        switch (addRemoveRef->childType) {
            case kMIDIObjectType_Source:
                name = getMIDIObjectNames_(addRemoveRef->child);
                logger::info(std::format("New MIDI source added: {}", name ? CFStringGetCStringPtr(name, kCFStringEncodingUTF8) : "unknown"));
                if (this->isEndpointMatched_(name)) {
                    this->connectSource_(addRemoveRef->child, name);
                }
                if (name) {
                    CFRelease(name);
                }
                break;
            case kMIDIObjectType_Destination:
                // When implementing MIDI transmission, connect here.
                break;
            default:
                break;
        }
        break;
    case kMIDIMsgObjectRemoved:
        addRemoveRef = (MIDIObjectAddRemoveNotification *)message;
        switch (addRemoveRef->childType) {
            case kMIDIObjectType_Source:
                name = getMIDIObjectNames_(addRemoveRef->child);
                logger::info(std::format("MIDI source removed: {}", name ? CFStringGetCStringPtr(name, kCFStringEncodingUTF8) : "unknown"));
                if (name) {
                    CFRelease(name);
                }
                break;
            case kMIDIObjectType_Destination:
                // When implementing MIDI transmission, manage disconnection here.
                break;
            default:
                break;
        }
        break;
    case kMIDIMsgIOError:
        errorRef = (MIDIIOErrorNotification *)message;
        name = getMIDIObjectNames_(errorRef->driverDevice);
        logger::warn(std::format("I/O Error on MIDI object: {}, error code: {}",
                                 name ? CFStringGetCStringPtr(name, kCFStringEncodingUTF8) : "unknown",
                                 (long int)errorRef->errorCode));
        break;
    case kMIDIMsgSetupChanged:
    case kMIDIMsgPropertyChanged:
    case kMIDIMsgThruConnectionsChanged:
    case kMIDIMsgSerialPortOwnerChanged:
    default:
        // do nothing
        break;
    }
}

void MidiReceiver::midiReceiveCallback_(const MIDIEventList *eventList, void *srcConnRefCon) {
    size_t total_size = 0;
    unsigned char *buffer = NULL;
    unsigned char *cur_buffer = NULL;
    const MIDIEventPacket *cur_packet = NULL;

    // Calculate total size
    cur_packet = &eventList->packet[0];
    for (unsigned i = 0; i < eventList->numPackets; ++i) {
        total_size += cur_packet->wordCount * 4;
        cur_packet = MIDIEventPacketNext(cur_packet);
    }

    // Copy data to buffer
    cur_buffer = buffer = new unsigned char[total_size];
    cur_packet = &eventList->packet[0];
    for (unsigned i = 0; i < eventList->numPackets; ++i) {
        for (unsigned j = 0; j < cur_packet->wordCount; ++j) {
            UInt32 word = cur_packet->words[j];
            cur_buffer[0] = (word >> 24) & 0xFF;
            cur_buffer[1] = (word >> 16) & 0xFF;
            cur_buffer[2] = (word >> 8) & 0xFF;
            cur_buffer[3] = word & 0xFF;
            cur_buffer += 4;
        }
        cur_packet = MIDIEventPacketNext(cur_packet);
    }

    if (this->receiving_callback_) {
        this->receiving_callback_(total_size, buffer);
    }
    delete[] buffer;
}

void midiNotyfyCallback__(const struct MIDINotification *message, void *refCon) {
    if (refCon) {
        MidiReceiver *receiver = static_cast<MidiReceiver *>(refCon);
        receiver->midiNotyfyCallback_(message);
    }
}
