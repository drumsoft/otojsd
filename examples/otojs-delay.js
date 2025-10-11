// Otojs input and delay example.
// set --enable-input option to enable input.

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

var delay = [
  create_delay(0.5 * sample_rate, 0.6),
  create_delay(0.75 * sample_rate, 0.6),
];

function oto_render(frames, channels, input_array) {
  let output = new Float32Array(frames * channels);
  let input_channels = input_array.length / frames;
  for (let f = 0; f < frames; f++) {
    for (let c = 0; c < channels; c++) {
      output[f * channels + c] = delay[c % delay.length](
        input_array[f * input_channels + (c % input_channels)]
      );
    }
    frame++;
  }
  return output;
}
