// Otojs scale example.

// the scale by note number from A4(440Hz).
var scale = [0, 5, 11, 16, 21, 26, 31, 36, 41, 47, 52, 57, 62, 67, 72, 77];

var bpm = 135;
// frames per 16th note
var frames_16th = (sample_rate * 60) / bpm / 4;

function create_frequency_scale(note_offset) {
  return scale.map((n) => 440 * Math.pow(2, (n + note_offset) / 12));
}

function create_osc(scales, step_offset, pan, amplitude) {
  return (frame) => {
    let step = Math.floor(frame / frames_16th);
    let elapsed = frame % frames_16th;
    let freq = scales[(step + step_offset) % scales.length];
    let v = Math.sin((3.1415 * 2 * frame * freq) / sample_rate);
    let envelope = 1 - elapsed / frames_16th;
    let amp = amplitude * envelope;
    pan_amp = 0.5 + 0.5 * Math.sin(pan);
    pan += (Math.PI * 2) / (10 * sample_rate);
    return [v * amp * pan_amp, v * amp * (1 - pan_amp)];
  };
}

var osc1 = create_osc(create_frequency_scale(-29), 8, Math.PI * 0, 0.3);
var osc2 = create_osc(create_frequency_scale(-48), 0, Math.PI * 0.5, 0.3);
var osc3 = create_osc(create_frequency_scale(-41), 4, Math.PI * 1.0, 0.3);
var osc4 = create_osc(create_frequency_scale(-36), 12, Math.PI * 1.5, 0.3);

function create_delay(length, feedback) {
  let delay = [];
  return (input) => {
    let output;
    if (delay.length > length) {
      output = input + delay.shift() * feedback;
    } else {
      output = input;
    }
    delay.push(output);
    return output;
  };
}

var delay1 = create_delay(frames_16th * 3, 0.3);
var delay2 = create_delay(frames_16th * 3, 0.3);

function oto_render(frames, channels, input_array) {
  let output = new Float32Array(frames * channels);
  for (let f = 0; f < frames; f++) {
    let [v1l, v1r] = osc1(frame);
    let [v2l, v2r] = osc2(frame);
    let [v3l, v3r] = osc3(frame);
    let [v4l, v4r] = osc4(frame);

    for (let c = 0; c < channels; c += 2) {
      output[f * channels + c] = delay1(v1l + v2l + v3l + v4l);
      output[f * channels + c + 1] = delay2(v1r + v2r + v3r + v4r);
    }
    frame++;
  }
  return output;
}
