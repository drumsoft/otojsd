let frame = 0;

function oto_render(frames, channels, input_array) {
	let output = new Float32Array(frames * channels);
	for (let f = 0; f < frames; f++) {
		let v = 0.5 * Math.sin( 3.1415 * 2 * frame * 440 / sample_rate );
		for (let c = 0; c < channels; c++) {
			output[f * channels + c] = v;
		}
		frame++;
	}
	return output;
}
