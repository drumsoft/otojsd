/**
 * Global object for Otojs.
 */
var Otojs = {
    // global delta-time (in seconds) per frame
    dtime: 1 / sample_rate,
    // constant values
    PI2: Math.PI * 2,
    // oscillator modules
    osc: {},
    // filter modules
    filt: {},
    // eg modules
    eg: {},
    // controller modules
    ctl: {},
    // effect modules
    fx: {},
    // utility modules
    util: {},
    // sequencer modules
    seq: {},
};

// ================ oscillator modules ================

/**
 * Create a sine wave oscillator.
 * @returns {function} sine wave oscillator: (frequency:number) => number.
 */
Otojs.osc.sin = function () {
    let phase = 0;
    return (frequency) => {
        phase = (phase + frequency * Otojs.dtime) % 1;
        return Math.sin(Otojs.PI2 * phase);
    };
};

/**
 * Create a square wave oscillator.
 * @returns {function} square wave oscillator: (frequency:number) => number.
 */
Otojs.osc.sqr = function () {
    let phase = 0;
    return (frequency) => {
        phase = (phase + frequency * Otojs.dtime) % 1;
        return phase < 0.5 ? 1 : -1;
    };
};

/**
 * Create a square wave with pulse width param oscillator.
 * @returns {function} square wave + pw oscillator: (frequency:number, pw:number) => number.
 */
Otojs.osc.sqr_pw = function () {
    let phase = 0;
    // pw: pulse width (0.0 - 1.0)
    return (frequency, pw) => {
        phase = (phase + frequency * Otojs.dtime) % 1;
        return phase < pw ? 1 : -1;
    };
};

/**
 * Create a triangle wave oscillator.
 * @returns {function} triangle wave oscillator: (frequency:number) => number.
 */
Otojs.osc.tri = function () {
    let phase = 0;
    return (frequency) => {
        phase = (phase + frequency * Otojs.dtime) % 1;
        return phase < 0.25
            ? 4 * phase
            : phase < 0.75
            ? -4 * phase + 2
            : 4 * phase - 4;
    };
};

/**
 * Create a triangle/saw wave oscillator with morphing param.
 * @returns {function} triangle/saw wave oscillator: (frequency:number, morph:number) => number.
 */
Otojs.osc.tri_saw = function () {
    let phase = 0;
    // morph: 0.0(saw) .. 0.5(tri) .. 1.0(rev-saw)
    return (frequency, morph) => {
        phase = (phase + frequency * Otojs.dtime) % 1;
        if (morph == 0) {
            return -2 * (phase - 0.5);
        } else if (morph == 1.0) {
            return phase < 0.5 ? 2 * phase : 2 * (phase - 1);
        } else {
            let mhalf = morph / 2;
            return phase < mhalf ? phase / mhalf
            : phase < 1 - mhalf ? (phase - 0.5) * -2 / (1 - morph)
            : (phase - 1) / mhalf;
        }
    };
};

/**
 * Create a saw wave oscillator.
 * @returns {function} saw wave oscillator: (frequency:number) => number.
 */
Otojs.osc.saw = function () {
    let phase = 0;
    return (frequency) => {
        phase = (phase + frequency * Otojs.dtime) % 1;
        return phase < 0.5 ? -2 * phase : -2 * phase + 2;
    };
};

/**
 * Create a white noise oscillator.
 * @returns {function} white noise oscillator: () => number.
 */
Otojs.osc.noise = function () {
    return () => {
        return Math.random() * 2 - 1;
    };
};

/**
 * Create a silent oscillator.
 * @returns {function} silent oscillator: () => number.
 */
Otojs.osc.mute = function () {
    return () => {
        return 0;
    };
};

// ================ filter modules ================

/**
 * Create a low-pass filter with cut-off frequency and resonance param.
 * @returns {function} low-pass filter: (input:number, frequency:number, resonance:number) => number.
 */
Otojs.filt.lpf_sv = function () {
    let buf0 = 0;
    let buf1 = 0;
    return (input, frequency, resonance) => {
        let f = 2 * Math.sin(Math.PI * Math.min(frequency / sample_rate, 0.25));
        buf0 += f * (input - buf0 + resonance * (buf0 - buf1));
        buf1 += f * (buf0 - buf1);
        return buf1;
    };
}

Otojs.filt.lpf_biquad = function () {
    let x = 0;
    let x1 = 0;
    let x2 = 0;
    let y = 0;
    let y1 = 0;
    let y2 = 0;
    let freq_unit = Otojs.PI2 / sample_rate;
    return (input, frequency, q) => {
        let w0 = frequency * freq_unit;
        let alpha = Math.sin(w0) / (2 * q);
        let cs = Math.cos(w0);
        let b1 = 1 - cs;
        let b0 = b1 / 2;
    
        x2 = x1;
        x1 = x;
        x = input;
        y2 = y1;
        y1 = y;
        y = (b0*x + b1*x1 + b0*x2 + 2*cs*y1 - (1-alpha)*y2) / (1+alpha);

        return y;
    };
}

// ================ eg modules ================

/**
 * Create an ADSR envelope generator.
 * @param {number} attack time in seconds
 * @param {number} decay time in seconds
 * @param {number} sustain level 0.0 .. 1.0
 * @param {number} release time in seconds
 * @returns {function} ADSR envelope generator: (trigger:boolean) => number (level 0.0 .. 1.0)
 */
Otojs.eg.adsr = function (attack, decay, sustain, release) {
    const min_time = 0.0005; // minimum time for each phase (in seconds)
    let is_on = false;
    let level = 0;
    let start_level = 0;
    let elapsed = 0;
    attack = Math.max(attack, min_time);
    decay = Math.max(decay, min_time);
    release = Math.max(release, min_time);
    let s_attack = (1 - start_level) / attack;
    let s_decay = -(1 - sustain) / decay;
    let s_release = -start_level / release;
    return (trigger) => {
        if (trigger !== is_on) {
            // note on/off state changed
            is_on = trigger;
            start_level = level;
            s_attack = (1 - start_level) / attack;
            s_release = -start_level / release;
            elapsed = 0;
        }
        switch (is_on) {
            case true:
                level = elapsed <= attack ? s_attack * elapsed + start_level // attack phase
                : elapsed < attack + decay ? s_decay * (elapsed - attack) + 1 // decay phase
                : sustain; // sustain phase
                break;
            case false:
                level = elapsed < release ? s_release * elapsed + start_level // release phase
                : 0; // off
                break;
        }
        elapsed += Otojs.dtime;
        return level;
    };
};

// ================ controller modules ================

/**
 * a portamento controller.
 * frequency and trigger come from sequencer.
 * @returns {function} portamento frequency: (frequency:number, trigger:boolean, time:number) => number
 */
Otojs.ctl.portamento = function () {
    let cv = 0;
    let isOn = false;
    return (frequency, trigger, time) =>{
        const tv = Math.log2(frequency / 261.626);
        const triggered = trigger && !isOn;
        isOn = trigger;
        if (triggered || time === 0) {
            cv = tv;
            return frequency;
        } else {
            cv = tv + (cv - tv) * Math.exp(-Otojs.dtime / time);
            return 261.626 * Math.pow(2, cv);
        }
    }
}

/**
 * a light-weight version of portamento controller.
 * @param {number} time for portamento in seconds
 * @returns {function} portamento frequency: (frequency:number, trigger:boolean) => number
 */
Otojs.ctl.portamento_light = function (time) {
    let cf = 0;
    let isOn = false;
    let decay = Math.exp(-Otojs.dtime / time);
    return (frequency, trigger) =>{
        if ((trigger && !isOn) || time === 0) {
            cf = frequency;
        } else {
            cf = frequency + (cf - frequency) * decay;
        }
        isOn = trigger;
        return cf;
    }    
}

// ================ effect modules ================

/**
 * Create a reverb that multiple echos are randomly generated.
 * @param {number} start time that first echo may comes (in seconds)
 * @param {number} length length of echos after start (in seconds)
 * @param {number} density a number of echoes (integer)
 * @param {number} feedback internal feedback amount.
 * @returns 
 */
Otojs.fx.reverb_random = function (start, length, density, feedback) {
    let max_samples = Math.ceil((length + start) * sample_rate);
    let buffer = new Otojs.util.RingBuffer(max_samples);
    let echoes = new Array(density);
    for (let i = 0; i < echoes.length; i++) {
        let delay_time = Math.random() * length + start;
        let delay_samples = Math.ceil(delay_time * sample_rate);
        let decay = Math.pow(1 / density, delay_time / (length + start));
        echoes[i] = [delay_samples, i % 2 == 0 ? decay : -decay];
    }
    return (input) => {
        let sum = 0;
        for (let echo of echoes) {
            sum += echo[1] * buffer.get(echo[0]);
        }
        buffer.push((1 - feedback) * input + feedback * sum);
        return sum;
    };
}

// =============== utility modules ================

/**
 * 値の追加と（確保したサイズ内で）過去の値の参照のみ可能なリングバッファ.
 * パフォーマンスのため, 確保するバッファサイズは2の冪乗数の Float64Array とする.
 * @param {number} size 確保するバッファサイズ 実際にはこれ以上のサイズが確保される.
 */
Otojs.util.RingBuffer = class {
    constructor(size) {
        let bufferSize = Math.pow(2, Math.ceil(Math.log2(size)));
        this.max = bufferSize - 1;
        this.buffer = new Float64Array(bufferSize);
        this.cur = 0;
    }
    push(value) {
        this.buffer[this.cur] = value;
        this.cur = (this.cur + 1) & this.max;
    }
    get(offset) {
        return this.buffer[(this.cur - offset) & this.max];
    }
}

// ================ sequencer modules ================

/**
 * Create a ticker function that is called once for every frame and returns the elapsed time in ticks.
 * @param {number} bpm
 * @param {number} offset in ticks
 * @returns 
 */
Otojs.seq.ticker = function (bpm, offset = 0) {
    // set dticks (delta-ticks per frame)
    let dticks = (960 * bpm) / 60 / sample_rate;
    let current = offset;
    return () => {
        current += dticks;
        return current;
    };
};

/**
 * Class for MML (Music Macro Language) sequencer.
 */
Otojs.seq.Mml = class {
    /**
     * Create an MML sequencer instance.
     * @param {number} tune_a4 fine tuning of A4 in Hz (default: 440)
     */
    constructor(tune_a4 = 440) {
        // configuration
        this.tune_a4 = tune_a4;
        // interface of status
        this.frequency = 0;
        this.trigger = false;
        this.accent = false;
        // playback configuration
        this.offset = 0;
        this.loop = false;
        this.loop_length = 0;
        // internal event list
        this.events = []; // { tick:number, frequency:number, trigger:bool, accent:bool }
        this.next_index = 0;
        this.next_tick = undefined;
        this.prev_tick = 0;
    }
    /**
     * Play the sequencer at the given tick.
     * called once per frame with tick from ticker and `.frequency` `.trigger` `.accent` are updated.
     * @param {number} tick 
     */
    play(tick) {
        let t = tick - this.offset;
        if (this.loop) {
            t = t % this.loop_length;
            if (t < this.prev_tick) {
                // events looped
                this.next_index = 0;
                this.next_tick = this.events.length > 0 ? this.events[0].tick : undefined;
            }
        }
        while (this.next_tick !== undefined && this.next_tick <= t) {
            let next = this.events[this.next_index];
            this.frequency = next.frequency;
            this.trigger = next.trigger;
            this.accent = next.accent;
            this.next_index++;
            this.next_tick = this.next_index < this.events.length ? this.events[this.next_index].tick : undefined;
        }
        this.prev_tick = t;
    }
    /**
     * Give MML string and parse it for play.
     * @param {string} mml the MML string
     * @param {number} offset in ticks if you want play the score after some delay
     * @param {bool} loop true to enable looping
     * @param {number} loop_length in ticks if not given, the total length of the score is used
     */
    score(mml, offset, loop, loop_length) {
        const label_characters = "CDEFGABNRO><LQ";
        const modifier_characters = "#+-";
        const length_characters = "0123456789.+-^";
        const post_modifier_characters = "!~";
        const delimiters = "| \n\r\t";
        const number_by_label = { 'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11 };
        const tune_a4 = this.tune_a4;
        // musical state
        let current_tick = 0;
        let octave = 4;
        let default_length = 960; // in ticks
        let gate = 7 / 8; // gate time ratio (or ticks if gate_absolute is true)
        let gate_absolute = false; // absolute gate time mode

        // the event list
        let events = []; // { tick:number, frequency:number, trigger:bool, accent:bool }


        // parser functions
        // parse note fractional value into ticks
        function parse_length(length_str) {
            let total_length = 0;
            let adding = true; // true: +, false: -
            let number = "";
            let dots = "";
            function parse_and_flush() {
                let n = parseInt(number);
                if (isNaN(n) || n == 0) { return; }
                let length = 4 * 960 / n;
                if (dots) {
                    length *= 2 - 1 / Math.pow(2, dots.length);
                }
                if (adding) {
                    total_length += length;
                } else {
                    total_length -= length;
                }
                number = "";
                dots = "";
            }
            for (let c of length_str) {
                if ('0' <= c && c <= '9') {
                    number += c;
                } else if (c === '.') {
                    dots += c;
                } else if (c === '+' || c === '^' || c === '-') {
                    parse_and_flush();
                    adding = c !== '-';
                }
            }
            parse_and_flush();
            return total_length;
        }
        // parse note modifier into semitone offset
        function parse_modifier(modifier_str) {
            let modifier = 0;
            for (let c of modifier_str) {
                if (c === '+' || c === '#') {
                    modifier += 1;
                } else if (c === '-') {
                    modifier -= 1;
                }
            }
            return modifier;
        }
        // parse toknized note into events and update current_tick
        function parse_note(note) {
            if (note === undefined) { return; }
            if (note.label in number_by_label || note.label === 'N') {
                let note_number;
                let length;
                if (note.label === 'N') {
                    note_number = parseInt(note.length);
                    if (isNaN(note_number)) {
                        current_tick += length;
                        return;
                    }
                    length = default_length;
                } else {
                    note_number = number_by_label[note.label] + (octave + 1) * 12 + parse_modifier(note.modifier);
                    length = note.length ? parse_length(note.length) : default_length;
                }
                let frequency = tune_a4 * Math.pow(2, (note_number - 69) / 12);
                let gate_time = gate_absolute ? gate : Math.floor(length * gate);
                let accent = note.post.includes('!');
                let legato = note.post.includes('~');
                events.push({ tick: current_tick, frequency, trigger: true, accent });
                if (!legato) {
                    events.push({ tick: current_tick + gate_time, frequency, trigger: false, accent });
                }
                current_tick += length;
            } else if (note.label === 'R') {
                let length = note.length ? parse_length(note.length) : default_length;
                current_tick += length;
            } else if (note.label === 'O') {
                let o = parseInt(note.length);
                if (isNaN(o)) { return; }
                octave = o;
            } else if (note.label === '>') {
                octave += 1;
            } else if (note.label === '<') {
                octave -= 1;
            } else if (note.label === 'L') {
                if (note.length) {
                    default_length = parse_length(note.length);
                }
            } else if (note.label === 'Q') {
                let g = parseFloat(note.length);
                if (isNaN(g)) { return; }
                if (note.post.includes('!')) {
                    gate_absolute = true;
                    if (1 <= g) {
                        gate = default_length * g / 8;
                    } else {
                        gate = default_length * g;
                    }
                } else {
                    gate_absolute = false;
                    if (1 <= g) {
                        gate = g / 8;
                    } else {
                        gate = g;
                    }
                }
            }
        }
        // report error
        function report_error(message, mml, position) {
            throw new Error(`${message} at ${position} in MML\n${mml}\n${' '.repeat(position)}^`);
        }
        // parser state
        let state = 0; // 0: label, 1: modifier, 2: length, 3: post-modifier
        let note = undefined; // {label, modifier, length, post}

        // start parsing
        for (let index in mml) {
            let c = mml[index].toUpperCase();
            // same letter loop （同じ文字の再評価を行うためのループ）
            while (true) {
                switch (state) {
                    case 0: // input may be label
                        if (label_characters.includes(c)) {
                            note = { label: c, modifier: "", length: "", post: "" };
                            state = 1;
                        } else if (delimiters.includes(c)) {
                            // ignore delimiters
                        } else {
                            // unexpected character
                            report_error(`Unexpected character '${c}'`, mml, index);
                        }
                        break;
                    case 1: // input may be modifier
                        if (modifier_characters.includes(c)) {
                            note.modifier += c;
                        } else if (label_characters.includes(c)) {
                            parse_note(note);
                            note = undefined;
                            state = 0;
                            continue; // re-evaluate in note state
                        } else if (length_characters.includes(c)) {
                            state = 2;
                            continue; // re-evaluate in length state
                        } else if (post_modifier_characters.includes(c)) {
                            state = 3;
                            continue; // re-evaluate in post-modifier state
                        } else if (delimiters.includes(c)) {
                            // note is finished if delimiter found
                            parse_note(note);
                            note = undefined;
                            state = 0;
                        } else {
                            // unexpected character
                            report_error(`Unexpected character '${c}'`, mml, index);
                        }
                        break;
                    case 2: // input may be length
                        if (length_characters.includes(c)) {
                            note.length += c;
                        } else if (label_characters.includes(c)) {
                            parse_note(note);
                            note = undefined;
                            state = 0;
                            continue; // re-evaluate in note state
                        } else if (post_modifier_characters.includes(c)) {
                            state = 3;
                            continue; // re-evaluate in post-modifier state
                        } else if (delimiters.includes(c)) {
                            // note is finished if delimiter found
                            parse_note(note);
                            note = undefined;
                            state = 0;
                        } else {
                            // unexpected character
                            report_error(`Unexpected character '${c}'`, mml, index);
                        }
                        break;
                    case 3: // input may be post-modifier
                        if (post_modifier_characters.includes(c)) {
                            note.post += c;
                        } else if (label_characters.includes(c)) {
                            parse_note(note);
                            note = undefined;
                            state = 0;
                            continue; // re-evaluate in note state
                        } else if (delimiters.includes(c)) {
                            // note is finished if delimiter found
                            parse_note(note);
                            note = undefined;
                            state = 0;
                        } else {
                            // unexpected character
                            report_error(`Unexpected character '${c}'`, mml, index);
                        }
                        break;
                }
                break; // exit same letter loop
            }
        }
        if (note) {
            parse_note(note);
        }

        this.events = events;
        if (offset !== undefined) {
            this.offset = offset;
        }
        if (loop !== undefined) {
            this.loop = loop;
        }
        this.loop_length = loop_length ? loop_length : current_tick;
        this.next_index = 0;
        this.next_tick = this.events.length > 0 ? this.events[0].tick : undefined;
    }
};
