// Otojs FM synthesis example.

// use 'var' to avoid error on redeclaration.
var delay = [];

function oto_render(frames, channels, input_array) {
  let output = new Float32Array(frames * channels);
  for (let f = 0; f < frames; f++) {
    // Modulator A and modulation depth
    let m1 = 0.5 * Math.sin((3.1415 * 2 * frame * 500) / sample_rate);
    let m2 = 0.5 + 0.5 * Math.sin((3.1415 * 2 * frame * 0.1) / sample_rate);
    // Modulator B and modulation depth (modulated by A)
    let m3 =
      0.5 * Math.sin((3.1415 * 2 * frame * 333) / sample_rate + m1 * m2 * 10.0);
    let m4 = 0.5 + 0.5 * Math.sin((3.1415 * 2 * frame * 0.01) / sample_rate);
    // Modulator C and modulation depth
    let m5 = 0.5 * Math.sin((3.1415 * 2 * frame * 55) / sample_rate);
    let m6 = 0.5 + 0.5 * Math.sin((3.1415 * 2 * frame * 0.008) / sample_rate);
    // Carrier (modulated by B and C)
    let v =
      0.5 *
      Math.sin(
        (3.1415 * 2 * frame * 110) / sample_rate +
          m3 * m4 * 50.0 +
          m5 * m6 * 50.0
      );

    // delay effect
    if (delay.length > sample_rate / 3) {
      v = v + 0.5 * delay.shift();
    }
    delay.push(v);

    for (let c = 0; c < channels; c++) {
      output[f * channels + c] = v;
    }
    frame++;
  }
  return output;
}
