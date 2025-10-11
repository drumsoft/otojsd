// Otojs sinewave combination example.

// oto_render generates audio samples.
// should return Float32Array(frames * channels).
function oto_render(frames, channels, input_array) {
  let output = new Float32Array(frames * channels);
  for (let f = 0; f < frames; f++) {
    // sinewave oscillator 1 (110Hz)
    let v1 = 0.5 * Math.sin((3.1415 * 2 * frame * 110) / sample_rate);
    // sinewave oscillator 2 (220Hz)
    let v2 = 0.5 * Math.sin((3.1415 * 2 * frame * 220) / sample_rate);
    // sinewave oscillator 3 (880Hz)
    let v3 = 0.5 * Math.sin((3.1415 * 2 * frame * 880) / sample_rate);
    // AM modulator for v1 and v2 (stereo LFO)
    let m1 = 0.5 + 0.5 * Math.sin((3.1415 * 2 * frame * 1.0) / sample_rate);
    // AM modulator for v3 (on/off)
    let m2 = (Math.floor(frame / (sample_rate / 8)) % 16) % 3 == 1 ? 1 : 0;

    for (let c = 0; c < channels; c += 2) {
      output[f * channels + c] = m1 * v1 + (1 - m1) * v2 + m2 * v3;
      output[f * channels + c + 1] = (1 - m1) * v1 + m1 * v2 + m2 * v3;
    }
    frame++;
  }
  return output;
}
