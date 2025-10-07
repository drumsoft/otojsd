// an example with otojs-basic.js
// post otojs-basic.js first.

var ticker = Otojs.seq.ticker(120);

var kick_mml = new Otojs.seq.Mml();
kick_mml.score("Q4 L4 O1 G!G!G!G! G!G!G!G! G!G8.!G16G!G! G!G!G!G8!G8", null, true);
var kick_osc = Otojs.osc.sqr();
var kick_eg_amp = Otojs.eg.adsr(0.0, 0.15, 0.0, 0.15);
var kick_eg_filter = Otojs.eg.adsr(0.0, 0.01, 0.0, 0.01);
var kick_filter = Otojs.filt.lpf_biquad();

function kick(tick) {
    kick_mml.play(tick);
    let e_amp = kick_eg_amp(kick_mml.trigger);
    let e_filter = kick_eg_filter(kick_mml.trigger);
    let v = kick_osc(kick_mml.frequency);
    return e_amp * kick_filter(v, 56 + 60 * e_filter, 2.5);
}

var bass_mml = new Otojs.seq.Mml();
bass_mml.score("Q5 L16 O1 F!>C<FC F!~>E8!~<C F>C!<FC~ FRF8", null, true);
var bass_portamento = Otojs.ctl.portamento_light(0.06);
var bass_osc = Otojs.osc.sqr();
var bass_eg = Otojs.eg.adsr(0.01, 0.2, 0.5, 0.01);
var bass_filter = Otojs.filt.lpf_biquad();
var bass_lfo = Otojs.osc.sin();

function bass(tick) {
    bass_mml.play(tick);
    let cutoff = 550 + 300 * bass_lfo(1 / 10);
    let frequency = bass_portamento(bass_mml.frequency, bass_mml.trigger);
    let v = bass_filter(bass_osc(frequency), cutoff, 5.0);
    let amp = bass_eg(bass_mml.trigger);
    return amp * v;
}

var synth_mml = [new Otojs.seq.Mml(), new Otojs.seq.Mml(), new Otojs.seq.Mml()];
var sc = (note, repeat = 1) => `RR${note}${note}!RR${note}${note}!RR${note}${note}!RR${note}${note}!`.repeat(repeat);
synth_mml[0].score(`Q4 L16 O5 | ${sc("C",4) + sc("C",2) + sc("C",2)}`, null, true);
synth_mml[1].score(`Q4 L16 O4 | ${sc("G",4) + sc("A",2) + sc("F",2)}`, null, true);
synth_mml[2].score(`Q4 L16 O4 | ${sc("F",4) + sc("E",2) + sc("D",2)}`, null, true);
var synth_osc = [Otojs.osc.saw(), Otojs.osc.saw(), Otojs.osc.saw()];
var synth_eg = Otojs.eg.adsr(0.001, 0.4, 0.4, 0.01);
var synth_lfo = Otojs.osc.sin();
var synth_filter = Otojs.filt.lpf_biquad();

function synth(tick) {
    let mixed = 0;
    for (let i = 0; i < synth_mml.length; i++) {
        synth_mml[i].play(tick);
        let v = synth_osc[i](synth_mml[i].frequency);
        mixed += 0.33 * v;
    }
    let amp = synth_eg(synth_mml[0].trigger);
    mixed = synth_filter(mixed, 400 + 200 * synth_lfo(1 / 15) + amp * 250, 4.0);
    return amp * mixed;
}

function calc_swing(tick) {
    return 80 - 80 * Math.cos(Otojs.PI2 * tick / 480);
}

function overdrive(x) {
    if (x >= 0) {
        return 1 - Math.exp(-x);
    } else {
        return Math.exp(x) - 1;
    }
}

var synth_delay = Otojs.fx.reverb_random(0.25 * 0.75, 0.25, 100, 0.20);

function oto_render(frames, channels, input_array) {
    let output = new Float32Array(frames * channels);
    for (let f = 0; f < frames; f++) {
        let tick = ticker();
        let swing = 0.4 * calc_swing(tick);

        let v0 = overdrive(0.40 * kick(tick - swing));
        let v1 = overdrive(0.70 * bass(tick - swing));
        let v2 = overdrive(0.40 * synth(tick - swing * 0.7));

        let v = 0.33 * v0 + 0.15 * v1 + 0.23 * v2 + 0.23 * synth_delay(v2);

        for (let c = 0; c < channels; c++) {
            output[f * channels + c] = v;
        }
        frame++;
    }
    return output;
}
