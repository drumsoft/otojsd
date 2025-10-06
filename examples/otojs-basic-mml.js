// check mml sequencer in otojs-basic.js
// post otojs-basic.js first.

var bass_osc = Otojs.osc.tri();
var bass_eg = Otojs.eg.adsr(0.001, 0.4, 0.6, 0.2);

var ticker = Otojs.seq.ticker(120);
var bass_mml = new Otojs.seq.Mml();

bass_mml.score(
    //                            |      C  D  E F  G  A  B  R   | > C  <  B              A      GF ED4     |
    "Q3 L8 O3 |CDEFGABR|>C<BAGFED4| q.70 N48n50f-e# a--g++b8.r16 | o4C!~O3 B16+8^8-8.~ l16A~A~L8 GF!ED8..R32 ",
    null, true);

function oto_render(frames, channels, input_array) {
    let output = new Float32Array(frames * channels);
    for (let f = 0; f < frames; f++) {
        let tick = ticker();
        bass_mml.play(tick);
        let v = bass_osc(bass_mml.frequency);
        let amp = bass_eg(bass_mml.trigger) * (bass_mml.accent ? 1.0 : 0.8);
        for (let c = 0; c < channels; c++) {
            output[f * channels + c] = amp * v;
        }
        frame++;
    }
    return output;
}
