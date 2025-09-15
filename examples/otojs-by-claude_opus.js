// Sample code by Claude Opus 4.1

var delay_left = [];
var delay_right = [];
var lfo_phase = 0;
var envelope_phase = 0;

function oto_render(frames, channels, input_array) {
    let output = new Float32Array(frames * channels);
    
    for (let f = 0; f < frames; f++) {
        // グローバルLFO（ゆっくりとした揺れ）
        let global_lfo = 0.5 + 0.5 * Math.sin(2 * Math.PI * frame * 0.2 / sample_rate);
        
        // ベースライン - パンチの効いた低音
        let bass_seq = [55, 55, 110, 82.5, 55, 110, 55, 82.5];
        let bass_idx = Math.floor(frame / (sample_rate / 4)) % bass_seq.length;
        let bass_freq = bass_seq[bass_idx];
        let bass_env = Math.max(0, 1 - ((frame % (sample_rate / 4)) / (sample_rate / 8)));
        
        // FM変調でベースに倍音を追加
        let bass_mod = Math.sin(2 * Math.PI * frame * bass_freq * 3 / sample_rate) * bass_env * 2;
        let bass = Math.sin(2 * Math.PI * frame * bass_freq / sample_rate + bass_mod) * bass_env * 0.3;
        
        // アルペジオ - きらめくような高音
        let arp_seq = [440, 550, 660, 880, 660, 550, 440, 330];
        let arp_idx = Math.floor(frame / (sample_rate / 16)) % arp_seq.length;
        let arp_freq = arp_seq[arp_idx];
        let arp_env = Math.max(0, 1 - ((frame % (sample_rate / 16)) / (sample_rate / 32)));
        
        // アルペジオにビブラート追加
        let vibrato = Math.sin(2 * Math.PI * frame * 6 / sample_rate) * 10 * global_lfo;
        let arp = Math.sin(2 * Math.PI * frame * (arp_freq + vibrato) / sample_rate) * arp_env * 0.15;
        
        // パッドサウンド - 複数の周波数を重ねた厚みのある音
        let pad_lfo = Math.sin(2 * Math.PI * frame * 0.3 / sample_rate);
        let pad1 = Math.sin(2 * Math.PI * frame * 220 / sample_rate + pad_lfo * 0.5) * 0.1;
        let pad2 = Math.sin(2 * Math.PI * frame * 330 / sample_rate - pad_lfo * 0.3) * 0.08;
        let pad3 = Math.sin(2 * Math.PI * frame * 275 / sample_rate) * 0.06;
        let pad = (pad1 + pad2 + pad3) * (0.5 + global_lfo * 0.5);
        
        // ノイズハット - リズムのアクセント
        let hat_trigger = (Math.floor(frame / (sample_rate / 8)) % 4) == 2;
        let hat_env = hat_trigger ? Math.max(0, 1 - ((frame % (sample_rate / 8)) / (sample_rate / 64))) : 0;
        let hat = (Math.random() - 0.5) * hat_env * 0.1;
        
        // 全体をミックス
        let mix = bass + arp + pad + hat;
        
        // ディレイエフェクト（左右で異なる遅延時間）
        let delay_time_left = Math.floor(sample_rate * 0.375);  // 3/8拍
        let delay_time_right = Math.floor(sample_rate * 0.25);  // 1/4拍
        
        if (delay_left.length > delay_time_left) {
            mix += delay_left.shift() * 0.3;
        }
        if (delay_right.length > delay_time_right) {
            mix += delay_right.shift() * 0.3;
        }
        
        delay_left.push(mix);
        delay_right.push(mix);
        
        // 軽いディストーション
        mix = Math.tanh(mix * 1.5);
        
        // ステレオ出力（左右で微妙に異なる処理）
        for (let c = 0; c < channels; c += 2) {
            // 左チャンネル - パッドを強調
            output[f * channels + c] = mix + pad * 0.2 * (1 - global_lfo);
            // 右チャンネル - アルペジオを強調
            output[f * channels + c + 1] = mix + arp * 0.2 * global_lfo;
        }
        
        frame++;
    }
    
    return output;
}
