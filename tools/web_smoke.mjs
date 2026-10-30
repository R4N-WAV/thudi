import fs from 'fs';
const bytes = fs.readFileSync(process.argv[2]);
const wasi = { fd_close: () => 0, fd_seek: () => 0, fd_write: () => 0 };
const { instance } = await WebAssembly.instantiate(bytes, { wasi_snapshot_preview1: wasi });
const e = instance.exports; e._initialize();
const t0 = Date.now();
e.thudi_init(48000, 1);
const tables = JSON.parse((() => { const p = e.thudi_tables(); const m = new Uint8Array(e.memory.buffer); let q = p; while (m[q]) q++; return new TextDecoder().decode(m.subarray(p, q)); })());
console.log('pad params', tables.pad.length, 'models', tables.models.length, tables.models[0].pages.map(p => p.name).join(','));
const kit = JSON.parse(fs.readFileSync(process.argv[3]));
for (let i = 0; i < 16; i++) {
  const v = new Float32Array(e.memory.buffer, e.thudi_pad_params(i), tables.pad.length);
  tables.pad.forEach((pi, k) => { if (kit.pads[i].params[pi.id] !== undefined) v[k] = kit.pads[i].params[pi.id]; });
  e.thudi_set_note(i, 36 + i);
}
e.thudi_note(36, 1, 1);
let peak = 0;
for (let b = 0; b < 375; b++) { const p = e.thudi_process(128); const m = new Float32Array(e.memory.buffer, p, 128); for (const x of m) peak = Math.max(peak, Math.abs(x)); if (b === 2) console.log('light kick', new Float32Array(e.memory.buffer, e.thudi_lights(), 16)[0].toFixed(2)); }
console.log('peak', peak.toFixed(3), 'time for 1 s audio ms', Date.now() - t0);
for (const os of [1, 2]) {
  e.thudi_init(48000, os);
  let t = performance.now();
  for (let b = 0; b < 375; b++) { if (b % 47 === 0) for (const n of [36, 37, 38, 40, 41, 42]) e.thudi_note(n, 1, 1); e.thudi_process(128); }
  console.log('os', os, 'busy 1 s of audio, 6 pads hitting:', (performance.now() - t).toFixed(1), 'ms');
}
