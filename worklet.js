// Thudi web — AudioWorklet: runs the plugin's C++ engine (thudi.wasm) on the audio thread.
const WASI = { fd_close: () => 0, fd_seek: () => 0, fd_write: () => 0, proc_exit: () => 0 };

class ThudiProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.e = null;
    this.queue = [];
    this.lightTick = 0;
    this.numParams = options.processorOptions.numParams || 0;
    this.port.onmessage = (ev) => this.onMessage(ev.data);
    WebAssembly.instantiate(options.processorOptions.wasm, { wasi_snapshot_preview1: WASI }).then(({ instance }) => {
      this.e = instance.exports;
      this.e._initialize();
      this.e.thudi_init(sampleRate, options.processorOptions.oversampling || 1);
      const q = this.queue; this.queue = [];
      q.forEach((m) => this.onMessage(m));
      this.port.postMessage({ type: 'ready' });
    });
  }

  f32(ptr, n) { return new Float32Array(this.e.memory.buffer, ptr, n); }

  onMessage(m) {
    const e = this.e;
    if (!e) { this.queue.push(m); return; }
    switch (m.type) {
      case 'slot': this.f32(e.thudi_pad_params(m.slot), m.values.length).set(m.values); break;
      case 'param': this.f32(e.thudi_pad_params(m.slot), m.index + 1)[m.index] = m.value; break;
      case 'globals': this.f32(e.thudi_globals(), m.values.length).set(m.values); break;
      case 'global': this.f32(e.thudi_globals(), m.index + 1)[m.index] = m.value; break;
      case 'notes': m.notes.forEach((n, i) => e.thudi_set_note(i, n)); break;
      case 'hit': e.thudi_hit(m.pad, m.vel); break;
      case 'release': e.thudi_release(m.pad); break;
      case 'note': e.thudi_note(m.note, m.vel, m.on ? 1 : 0); break;
      case 'reset': e.thudi_reset(); break;
      case 'os': e.thudi_init(sampleRate, m.factor); break;
      case 'sample': {
        if (!m.data) { e.thudi_set_sample(m.slot, 0, 0, 0); break; }
        const p = e.thudi_alloc(m.data.length * 4);
        this.f32(p, m.data.length).set(m.data);
        e.thudi_set_sample(m.slot, p, m.data.length, m.rate);
        e.thudi_free(p);
        break;
      }
      case 'chop': {
        const p = e.thudi_alloc(m.data.length * 4), out = e.thudi_alloc(4 * 64);
        this.f32(p, m.data.length).set(m.data);
        const n = e.thudi_chop(p, m.data.length, m.rate, m.slices, out);
        const pts = Array.from(new Int32Array(e.memory.buffer, out, n));
        e.thudi_free(p); e.thudi_free(out);
        this.port.postMessage({ type: 'chopped', id: m.id, points: pts });
        break;
      }
    }
  }

  process(inputs, outputs) {
    const out = outputs[0];
    if (!this.e || !out || out.length === 0) return true;
    const n = out[0].length;
    const p = this.e.thudi_process(n);
    const stride = this.e.thudi_mix_stride();
    const mem = this.e.memory.buffer;
    out[0].set(new Float32Array(mem, p, n));
    if (out[1]) out[1].set(new Float32Array(mem, p + stride * 4, n));
    if (++this.lightTick >= 4) {   // ~ every 10 ms: pad lights for the page
      this.lightTick = 0;
      this.port.postMessage({ type: 'lights', v: Float32Array.from(this.f32(this.e.thudi_lights(), 16)) });
    }
    return true;
  }
}

registerProcessor('thudi', ThudiProcessor);
