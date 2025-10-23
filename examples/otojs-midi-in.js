// Otojs midi input example.
// set --midi-source "" option to enable midi input.

var frequency = 440;
var velocity = 0;
var pitchbend = 0;
var modulation = 0;
var key_pressing = 0;

var phase = 0;
function sine_synth(frequency, velocity, pitchbend, modulation) {
  const freq_bended = frequency * Math.pow(2, pitchbend * 2 / 12); // apply pich bend
  phase += 2 * Math.PI * freq_bended / sample_rate;
  let value = Math.sin(phase);
  value = Math.min(1, Math.max(-1, value * (1 + modulation))); // apply modulation as overdrive effect
  value *= (Math.pow(10, velocity) - 1) / 9; // apply velocity to amplitude
  return value;
}

function oto_render(frames, channels, input_array, midi_input) {
  if (midi_input) {
    // parse MIDI input
    for (let packet of midi_input) {
      // packet: Uint8Array is a Universal MIDI Packet.
      if (packet[0] >> 4 === 0x2) { // MIDI 1.0 Channel Voice Message
        const event_type = packet[1] & 0xf0;
        switch (event_type) { // ignore channel number
          case 0x90: case 0x80: // Note On/Note Off
            if (event_type === 0x90 && packet[3] > 0) {
              frequency = 440 * Math.pow(2, (packet[2] - 69) / 12);
              velocity = packet[3] / 127;
              key_pressing = packet[2];
              console.log(`Note On: note=${packet[2]} velocity=${packet[3]}`);
            } else { // some device sends Note On with velocity 0 as Note Off
              if (key_pressing === packet[2]) {
                velocity = 0;
              }
              console.log(`Note Off: note=${packet[2]} velocity=${packet[3]}`);
            }
            break;
          case 0xe0: // Pitch Bend
            let bend = ((packet[3] << 7) | packet[2]) - 8192;
            pitchbend = bend / 8192; // -1.0 to +1.0
            console.log("Pitch Bend: " + bend);
            break;
          case 0xb0: // Control Change
            if (packet[2] === 0x01) { // Modulation Wheel
              modulation = packet[3] / 127;
            }
            console.log(`Control Change ${packet[2]}: ${packet[3]}`);
            break;
        }
      }
    }
  }

  let output = new Float32Array(frames * channels);
  for (let f = 0; f < frames; f++) {
    const v = sine_synth(frequency, velocity, pitchbend, modulation);
    for (let c = 0; c < channels; c++) {
      output[f * channels + c] = v;
    }
    frame++;
  }
  return output;
}
