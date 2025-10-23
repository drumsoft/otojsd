#include <CoreFoundation/CoreFoundation.h>
#include <CoreMIDI/CoreMIDI.h>
#include <string>

const CFStringRef MIDI_CLIENT_NAME = CFSTR("otojsd");
const CFStringRef MIDI_INPUT_PORT_NAME = CFSTR("input");

class MidiReceiver {
  private:
    MIDIClientRef client_ = NULL;
    MIDIPortRef inPort_ = NULL;
    std::string sourceName_;
    void (*receiving_callback_)(size_t length, const unsigned char *data) = nullptr;

    CFStringRef getMIDIObjectNames_(MIDIObjectRef objectRef);
    bool isEndpointMatched_(CFStringRef endpointName);
    void connectSource_(MIDIEndpointRef sourceRef, CFStringRef name);

  public:
    MidiReceiver(std::string sourceName, void (*receiving_callback)(size_t length, const unsigned char *data));
    ~MidiReceiver();

    void connectDevices();

    void midiNotyfyCallback_(const struct MIDINotification *message);
    void midiReceiveCallback_(const MIDIEventList *eventList, void *srcConnRefCon);
};
