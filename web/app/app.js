// Thudi web (mobile) — the VST3's engine in the browser. UI, presets and state live here; sound in worklet.js.
'use strict';

const $ = (s, el = document) => el.querySelector(s);
const C = { lcdBg: '#1f52d6', lcdBg2: '#163a9e', ink: '#e8f1ff', dim: '#9db8ff', print: '#c9c6bd', printDim: '#8c8a84' };
const NOTE = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
const noteName = (n) => NOTE[((n % 12) + 12) % 12] + (Math.floor(n / 12) - 2);   // C3 = 60, like the plugin

let T = null;            // tables from the engine (params, models, pages)
let wasmBytes = null, mainWasm = null, presets = null;
let ctx = null, node = null;
const P = {}, G = {};    // id -> index
const S = {              // the machine
  kitName: 'Init', slots: [], labels: [], notes: [], soloLabel: 'INIT 808', globals: null,
  samples: new Array(17).fill(null),   // { id, name, data: Float32Array, rate }
  selected: 0, page: 0, focus: 0, mode: 'edit', browse: 'kits', browseIndex: 0, browseTop: 0,
  entries: [], message: '', messageUntil: 0, smokeT: -1, revealed: false, clipboard: null,
};
const lights = new Float32Array(16);

// ------------------------------------------------------------------ values
function toNorm(pi, x) {
  x = Math.min(pi.max, Math.max(pi.min, x));
  if (pi.scale === 'log' && pi.min <= 0) return Math.cbrt((x - pi.min) / (pi.max - pi.min));
  if (pi.scale === 'log') return Math.log(x / pi.min) / Math.log(pi.max / pi.min);
  return (x - pi.min) / (pi.max - pi.min);
}
function fromNorm(pi, n) {
  n = Math.min(1, Math.max(0, n));
  let x;
  if (pi.scale === 'log' && pi.min <= 0) x = pi.min + n * n * n * (pi.max - pi.min);
  else if (pi.scale === 'log') x = pi.min * Math.pow(pi.max / pi.min, n);
  else x = pi.min + n * (pi.max - pi.min);
  if (pi.scale === 'int' || pi.scale === 'choice') x = Math.round(x);
  return x;
}
function fmt(pi, v, short = true) {
  const id = pi.id;
  if (pi.scale === 'choice') { const c = pi.choices.split('|'); return c[Math.max(0, Math.min(c.length - 1, Math.round(v)))]; }
  if (pi.scale === 'int') {
    const i = Math.round(v);
    if (id === 'choke') return i === 0 ? 'Off' : 'Grp ' + i;
    if (id === 'output') return i === 0 ? 'Main' : 'Out ' + (i + 1);
    if (id === 'link') return i === 0 ? 'None' : 'Pad ' + i;
    if (id === 'keys_root') return noteName(i);
    return String(i);
  }
  const u = pi.unit, num = (x, d) => d > 0 ? x.toFixed(d) : String(Math.round(x));
  if (u === 'Hz') return v >= 1000 ? num(v / 1000, v >= 10000 ? 1 : 2) + (short ? 'k' : ' kHz') : num(v, v < 100 ? 1 : 0) + (short ? 'Hz' : ' Hz');
  if (u === 'ms') return v >= 1000 ? num(v / 1000, 2) + ' s' : num(v, v < 10 ? 1 : 0) + ' ms';
  if (u === '%') return Math.round(v * 100) + '%';
  if (u === 'dB') return v <= -59.9 ? '-inf' : (v > 0 ? '+' : '') + v.toFixed(1) + ' dB';
  if (u === 'st') return (v > 0 ? '+' : '') + v.toFixed(1) + ' st';
  if (u === 'ct') return (v > 0 ? '+' : '') + Math.round(v) + ' ct';
  if (u === 'bit') return v.toFixed(1) + ' bit';
  if (u === 'x') return v.toFixed(2) + 'x';
  if (id === 'pan') return Math.abs(v) < 0.01 ? 'C' : (v < 0 ? 'L' : 'R') + Math.round(Math.abs(v) * 100);
  return (v > 0 ? '+' : '') + v.toFixed(2);
}

// ------------------------------------------------------------------ engine link
const post = (m, transfer) => { if (node) node.port.postMessage(m, transfer || []); };
const pv = (slot, id) => S.slots[slot][P[id]];
const gv = (id) => S.globals[G[id]];
function setParam(slot, id, value, quiet) {
  const i = P[id]; S.slots[slot][i] = value; post({ type: 'param', slot, index: i, value });
  if (!quiet) changed();
}
function setGlobal(id, value) { S.globals[G[id]] = value; post({ type: 'global', index: G[id], value }); changed(); }
function sendAll() {
  for (let s = 0; s < T.slots; s++) post({ type: 'slot', slot: s, values: Array.from(S.slots[s]) });
  post({ type: 'globals', values: Array.from(S.globals) });
  post({ type: 'notes', notes: S.notes });
  S.samples.forEach((smp, s) => sendSample(s));
}
function sendSample(slot) {
  const smp = S.samples[slot];
  post({ type: 'sample', slot, data: smp ? smp.data : null, rate: smp ? smp.rate : 0 });
}
const editSlot = () => (gv('solo') >= 0.5 ? T.soloSlot : S.selected);
const modelOf = (slot) => Math.round(pv(slot, 'model'));
const pagesOf = (slot) => T.models[modelOf(slot)].pages;
const isSolo = () => gv('solo') >= 0.5;
const isGaerro = () => gv('gaerro') >= 0.5;

// ------------------------------------------------------------------ kits & pads
function soundFromParams(params) {
  const v = new Float32Array(T.pad.length);
  T.pad.forEach((pi, k) => { v[k] = params && params[pi.id] !== undefined ? params[pi.id] : pi.def; });
  return v;
}
function paramsFromSound(v) { const o = {}; T.pad.forEach((pi, k) => { o[pi.id] = +v[k].toPrecision(9); }); return o; }
function initSound(model) { return Float32Array.from(T.models[model].init); }

function initKit() {
  const empty = T.models.findIndex((m) => m.name === 'Empty'), b808 = T.models.findIndex((m) => m.name === '808');
  for (let i = 0; i < 16; i++) { S.slots[i] = initSound(empty); S.labels[i] = ''; S.notes[i] = 36 + i; S.samples[i] = null; }
  const bass = initSound(b808); bass[P.poly] = 0; bass[P.glide] = 0; bass[P.play_mode] = 1;
  S.slots[T.soloSlot] = bass; S.soloLabel = 'INIT 808'; S.samples[T.soloSlot] = null;
  S.globals[G.solo] = 0;
  S.kitName = 'Init';
}
async function loadKit(kit) {
  const usesSamples = (kit.pads || []).some((p) => p.sample) || (kit.solo && kit.solo.sample);
  T.global.forEach((pi, k) => {
    if (pi.id === 'gaerro') { if (usesSamples) S.globals[k] = 1; return; }
    if (kit.global && kit.global[pi.id] !== undefined) S.globals[k] = kit.global[pi.id];
  });
  for (let i = 0; i < 16; i++) {
    const p = (kit.pads || [])[i] || {};
    S.slots[i] = soundFromParams(p.params); S.labels[i] = (p.label || '').toUpperCase(); S.notes[i] = p.note || 36 + i;
    S.samples[i] = p.sample ? await samples.get(p.sample) : null;
  }
  if (kit.solo) {
    S.slots[T.soloSlot] = soundFromParams(kit.solo.params); S.soloLabel = (kit.solo.label || '').toUpperCase();
    S.samples[T.soloSlot] = kit.solo.sample ? await samples.get(kit.solo.sample) : null;
  }
  S.kitName = kit.name || 'Kit';
  sendAll(); refreshAll(); changed();
}
function kitToJson(name) {
  const g = {}; T.global.forEach((pi, k) => { g[pi.id] = S.globals[k]; });
  return {
    format: 'thudi-kit', version: 1, name, global: g,
    pads: S.slots.slice(0, 16).map((v, i) => Object.assign({ label: S.labels[i], note: S.notes[i], params: paramsFromSound(v) },
      S.samples[i] ? { sample: S.samples[i].id } : {})),
    solo: Object.assign({ label: S.soloLabel, params: paramsFromSound(S.slots[T.soloSlot]) }, S.samples[T.soloSlot] ? { sample: S.samples[T.soloSlot].id } : {}),
  };
}
function loadPad(slot, preset) {
  const v = soundFromParams(preset.params);
  v[P.output] = S.slots[slot][P.output]; v[P.link] = slot === T.soloSlot ? 0 : S.slots[slot][P.link];
  if (slot === T.soloSlot) { v[P.poly] = S.slots[slot][P.poly]; v[P.glide] = S.slots[slot][P.glide]; }   // performance switches stay
  S.slots[slot] = v;
  if (slot === T.soloSlot) S.soloLabel = (preset.label || preset.name || '').toUpperCase(); else S.labels[slot] = (preset.label || '').toUpperCase();
  S.samples[slot] = null; sendSample(slot);
  post({ type: 'slot', slot, values: Array.from(v) });
  changed(); refreshAll();
}

const store = {
  get(k, d) { try { const v = localStorage.getItem('thudi.' + k); return v ? JSON.parse(v) : d; } catch { return d; } },
  set(k, v) { try { localStorage.setItem('thudi.' + k, JSON.stringify(v)); } catch { flash('STORAGE FULL'); } },
};
const userKits = () => store.get('kits', {});
const userPads = () => store.get('pads', []);
function listKits() {
  const out = [{ name: 'Init', init: true }];
  presets.kits.forEach((k) => out.push({ name: k.name, kit: k }));
  Object.keys(userKits()).sort().forEach((n) => out.push({ name: n, user: true, kit: userKits()[n] }));
  return out;
}
function listPads() {
  const order = T.models.map((m) => m.name);
  const out = presets.pads.map((p) => Object.assign({}, p));
  userPads().forEach((p) => out.push(Object.assign({ user: true }, p)));
  out.sort((a, b) => order.indexOf(a.category) - order.indexOf(b.category));
  return out;
}

// samples are kept in IndexedDB so saved kits (and the session) still have their chops
const samples = {
  db: null,
  open() {
    if (this.db) return Promise.resolve(this.db);
    return new Promise((res) => {
      try {
        const r = indexedDB.open('thudi', 1);
        r.onupgradeneeded = () => r.result.createObjectStore('samples');
        r.onsuccess = () => res(this.db = r.result); r.onerror = () => res(null);
      } catch { res(null); }
    });
  },
  async put(smp) {
    const db = await this.open(); if (!db) return;
    db.transaction('samples', 'readwrite').objectStore('samples').put({ name: smp.name, rate: smp.rate, data: smp.data }, smp.id);
  },
  async get(id) {
    const db = await this.open(); if (!db) return null;
    return new Promise((res) => {
      const r = db.transaction('samples').objectStore('samples').get(id);
      r.onsuccess = () => res(r.result ? { id, name: r.result.name, rate: r.result.rate, data: r.result.data } : null);
      r.onerror = () => res(null);
    });
  },
};

// session: the machine comes back as you left it
let saveTimer = 0;
function changed() {
  clearTimeout(saveTimer);
  saveTimer = setTimeout(() => store.set('session', kitToJson(S.kitName)), 400);
  drawLcd();
}

// ------------------------------------------------------------------ LCD (same layout as the plugin: 554 x 178)
const lcd = $('#lcd'), g = lcd.getContext('2d');
const LW = 554, LH = 178;
function sizeLcd() {
  const r = lcd.getBoundingClientRect(), dpr = Math.min(3, window.devicePixelRatio || 1);
  lcd.width = Math.round(r.width * dpr); lcd.height = Math.round(r.height * dpr);
  drawLcd();
}
const font = (px) => `${px}px VT323`;
const cellArea = (i) => ({ x: 6 + (i % 4) * 92, y: 54 + Math.floor(i / 4) * 46, w: 88, h: 42 });
const tabArea = () => ({ x: 6, y: 30, w: LW - 12, h: 18 });
const sideBox = () => ({ x: 378, y: 54, w: LW - 384, h: 92 });
const listRow = (i) => ({ x: 6, y: 30 + i * 23, w: LW - 12, h: 22 });
const ROWS = 5;
function text(t, x, y, w, h, align = 'left', color = C.ink, px = 18) {
  g.fillStyle = color; g.font = font(px); g.textBaseline = 'middle'; g.textAlign = align;
  g.save(); g.beginPath(); g.rect(x, y, w, h); g.clip();
  g.fillText(t, align === 'left' ? x + 3 : align === 'right' ? x + w - 3 : x + w / 2, y + h / 2 + 1);
  g.restore();
}
function inv(t, r, align, on, px) { if (on) { g.fillStyle = C.ink; g.fillRect(r.x, r.y, r.w, r.h); } text(t, r.x, r.y, r.w, r.h, align, on ? C.lcdBg2 : C.ink, px); }
function softLabels() {
  if (S.mode === 'browse') return [S.browse === 'kits' ? 'PADS' : 'KITS', 'LOAD', S.browse === 'kits' ? 'SAVE KIT' : 'SAVE PAD', 'EXIT'];
  return ['< PAGE', 'PAGE >', 'BROWSE', 'SAVE'];
}
function drawLcd() {
  if (!T || !lcd.width) return;
  g.setTransform(lcd.width / LW, 0, 0, lcd.height / LH, 0, 0);
  const grd = g.createLinearGradient(0, 0, LW, 0); grd.addColorStop(0, '#2459dc'); grd.addColorStop(1, '#1c4bc4');
  g.fillStyle = grd; g.fillRect(0, 0, LW, LH);
  if (S.smokeT >= 0) drawSmoke(); else if (S.mode === 'edit') drawEdit(); else drawBrowse();
  // soft-key labels are on the keys themselves on the phone: show the page / list name instead
  g.fillStyle = 'rgba(0,0,0,.07)';
  for (let x = 0; x < LW; x += 3) g.fillRect(x, 0, .6, LH);
  for (let y = 0; y < LH; y += 3) g.fillRect(0, y, LW, .6);
  const tg = g.createLinearGradient(0, 0, 0, 10); tg.addColorStop(0, 'rgba(0,0,0,.5)'); tg.addColorStop(1, 'rgba(0,0,0,0)');
  g.fillStyle = tg; g.fillRect(0, 0, LW, 10);
  document.querySelectorAll('.softkeys .key').forEach((b, i) => { b.textContent = softLabels()[i]; });
}
function header(left, right, msg) {
  text(left, 6, 3, LW - 12, 24, 'left', C.ink, 24);
  text(msg || right, 6, 3, LW - 12, 24, 'right', msg ? C.ink : C.dim, 24);
}
const messageNow = () => (performance.now() < S.messageUntil ? S.message : '');
function drawEdit() {
  const slot = editSlot(), model = modelOf(slot), pages = pagesOf(slot);
  S.page = Math.max(0, Math.min(pages.length - 1, S.page));
  const left = slot === T.soloSlot ? 'SOLO  ' + S.soloLabel : 'PAD ' + String(slot + 1).padStart(2, '0') + '  ' + S.labels[slot];
  let right = T.models[model].name.toUpperCase() + '  |  KIT ' + S.kitName.toUpperCase();
  if (slot === T.soloSlot) right = (pv(slot, 'poly') >= 0.5 ? 'POLY' : 'MONO') + (pv(slot, 'glide') > 0.5 ? ' GLIDE' : '') + '  |  ROOT ' + noteName(Math.round(gv('keys_root')));
  header(left, right, messageNow());
  if (T.models[model].name === 'Empty') {
    text('NO SOUND ON THIS PAD', 6, 62, LW - 12, 36, 'center', C.dim, 26);
    text('TAP BROWSE TO PICK A SOUND' + (isGaerro() ? ' (OR LOAD A SAMPLE)' : ''), 6, 100, LW - 12, 26, 'center', C.dim, 19);
    return;
  }
  const tabs = tabArea(), tw = tabs.w / pages.length;
  pages.forEach((p, i) => inv(p.name, { x: tabs.x + i * tw, y: tabs.y, w: tw - 3, h: tabs.h }, 'center', i === S.page, 18));
  const knobs = pages[S.page].knobs;
  knobs.forEach((k, i) => {
    const c = cellArea(i), pi = T.pad[k], f = i === S.focus;
    if (f) { g.fillStyle = C.ink; g.fillRect(c.x, c.y, c.w, c.h); }
    text(pi.name.toUpperCase(), c.x, c.y, c.w, 16, 'left', f ? C.lcdBg2 : C.dim, 16);
    text(fmt(pi, S.slots[slot][k]), c.x, c.y + 16, c.w, c.h - 16, 'left', f ? C.lcdBg2 : C.ink, 23);
  });
  if (T.models[model].name === 'Sample') drawWave(slot); else drawEnv(slot, model);
}
function decayShape(x, c) { return x >= 1 ? 0 : Math.pow(1 - x, Math.pow(2, 2 * c + 1)); }
function drawEnv(slot, model) {
  const r = sideBox(), v = (id) => pv(slot, id), name = T.models[model].name;
  let a, h, d, c, title;
  if (['Snare', 'Clap', 'Perc'].includes(name)) { a = v('noise_attack'); h = 0; d = v('noise_decay'); c = v('noise_curve'); title = 'NOISE ENV'; }
  else if (['Closed Hat', 'Open Hat', 'Cymbal'].includes(name)) { a = v('metal_attack'); h = 0; d = v('metal_decay'); c = 0.35; title = 'METAL ENV'; }
  else { a = v('tone_attack'); h = v('tone_hold'); d = v('tone_decay'); c = v('tone_curve'); title = 'AMP ENV'; }
  g.strokeStyle = 'rgba(232,241,255,.8)'; g.lineWidth = 1; g.strokeRect(r.x + .5, r.y + .5, r.w - 1, r.h - 1);
  const total = Math.max(1, a + h + d * 1.1);
  text(title, r.x + 2, r.y + 2, r.w - 4, 14, 'left', C.dim, 15);
  text(total >= 1000 ? (total / 1000).toFixed(2) + 's' : Math.round(total) + 'ms', r.x + 2, r.y + 2, r.w - 4, 14, 'right', C.dim, 15);
  const px = r.x + 6, py = r.y + 18, pw = r.w - 12, ph = r.h - 24;
  g.beginPath();
  for (let i = 0; i <= 90; i++) {
    const t = total * i / 90;
    const y = t < a ? (a > 0 ? t / a : 1) : t < a + h ? 1 : decayShape((t - a - h) / Math.max(0.001, d), c);
    const X = px + pw * i / 90, Y = py + ph - y * ph;
    if (i === 0) { g.moveTo(X, py + ph); g.lineTo(X, Y); } else g.lineTo(X, Y);
  }
  g.strokeStyle = C.ink; g.lineWidth = 2; g.stroke();
}
const peaksCache = new WeakMap();
function drawWave(slot) {
  const r = sideBox(), smp = S.samples[slot];
  g.strokeStyle = 'rgba(232,241,255,.8)'; g.lineWidth = 1; g.strokeRect(r.x + .5, r.y + .5, r.w - 1, r.h - 1);
  if (!smp) { text('LOAD A SAMPLE', r.x, r.y, r.w, r.h, 'center', C.dim, 15); return; }
  const px = r.x + 4, py = r.y + 16, pw = r.w - 8, ph = r.h - 20, bins = Math.floor(pw);
  let pk = peaksCache.get(smp.data);
  if (!pk) {
    pk = []; const n = smp.data.length;
    for (let b = 0; b < bins; b++) {
      let lo = 0, hi = 0; const a = Math.floor(n * b / bins), e = Math.max(a + 1, Math.floor(n * (b + 1) / bins)), st = 1 + Math.floor((e - a) / 256);
      for (let k = a; k < e; k += st) { const x = smp.data[k]; if (x < lo) lo = x; if (x > hi) hi = x; }
      pk.push([lo, hi]);
    }
    peaksCache.set(smp.data, pk);
  }
  const s0 = pv(slot, 'smp_start'), s1 = pv(slot, 'smp_end');
  const xa = px + pw * Math.min(s0, s1), xb = px + pw * Math.max(s0, s1), mid = py + ph / 2;
  g.fillStyle = 'rgba(232,241,255,.22)'; g.fillRect(xa, py, Math.max(1, xb - xa), ph);
  pk.forEach(([lo, hi], b) => { const x = px + b; g.fillStyle = x >= xa && x <= xb ? C.ink : 'rgba(157,184,255,.55)'; g.fillRect(x, mid - hi * ph / 2, 1, Math.max(1, (hi - lo) * ph / 2)); });
  g.fillStyle = C.ink;
  [xa, xb].forEach((x) => { g.fillRect(x - 1, py - 2, 2, ph + 4); g.beginPath(); g.moveTo(x - 4, py - 3); g.lineTo(x + 4, py - 3); g.lineTo(x, py + 3); g.fill(); });
  text(smp.name.toUpperCase().slice(0, 14), r.x + 2, r.y + 1, r.w - 4, 14, 'left', C.dim, 15);
  text((smp.data.length / smp.rate * Math.abs(s1 - s0)).toFixed(2) + 's', r.x + 2, r.y + 1, r.w - 4, 14, 'right', C.dim, 15);
}
function drawBrowse() {
  const slot = editSlot();
  header(S.browse === 'kits' ? 'BROWSE KITS' : (slot === T.soloSlot ? 'SOUND LIBRARY -> SOLO' : 'SOUNDS -> PAD ' + (slot + 1)),
    (S.entries.length ? S.browseIndex + 1 : 0) + '/' + S.entries.length, messageNow());
  for (let i = 0; i < ROWS; i++) {
    const idx = S.browseTop + i; if (idx >= S.entries.length) break;
    const e = S.entries[idx];
    const t = (S.browse === 'pads' ? e.category.toUpperCase().padEnd(11, ' ') + e.name : e.name) + (e.user ? '   [USER]' : '');
    inv(t, listRow(i), 'left', idx === S.browseIndex, 21);
  }
  text('SWIPE TO SCROLL  ·  TAP TWICE TO LOAD', 6, 150, LW - 12, 22, 'center', C.dim, 17);
}
function refreshEntries() {
  S.entries = S.browse === 'kits' ? listKits() : listPads();
  if (S.browse === 'kits') { const i = S.entries.findIndex((e) => e.name === S.kitName); if (i >= 0) S.browseIndex = i; }
  else { const cat = T.models[modelOf(editSlot())].name; const i = S.entries.findIndex((e) => e.category === cat); if (i >= 0) S.browseIndex = i; }
  S.browseIndex = Math.max(0, Math.min(S.entries.length - 1, S.browseIndex));
  S.browseTop = Math.max(0, Math.min(S.entries.length - ROWS, S.browseIndex - 2));
}
function scrollBrowse(d) {
  if (!S.entries.length) return;
  S.browseIndex = Math.max(0, Math.min(S.entries.length - 1, S.browseIndex + d));
  if (S.browseIndex < S.browseTop) S.browseTop = S.browseIndex;
  if (S.browseIndex >= S.browseTop + ROWS) S.browseTop = S.browseIndex - ROWS + 1;
  drawLcd();
}
function loadEntry() {
  const e = S.entries[S.browseIndex]; if (!e) return;
  if (S.browse === 'kits') { if (e.init) { initKit(); sendAll(); refreshAll(); changed(); } else loadKit(e.kit); }
  else loadPad(editSlot(), e);
  flash('LOADED ' + e.name.toUpperCase());
}
function flash(msg) { S.message = msg; S.messageUntil = performance.now() + 2000; drawLcd(); setTimeout(drawLcd, 2050); }

// LCD touch: tabs, cells, list (swipe + double tap), sample markers
(function lcdTouch() {
  let start = null, lastTap = { t: 0, i: -1 }, drag = -1;
  const pos = (ev) => { const r = lcd.getBoundingClientRect(); return { x: (ev.clientX - r.left) * LW / r.width, y: (ev.clientY - r.top) * LH / r.height }; };
  const inR = (p, r) => p.x >= r.x && p.x <= r.x + r.w && p.y >= r.y && p.y <= r.y + r.h;
  lcd.addEventListener('pointerdown', (ev) => {
    if (S.smokeT >= 0) return;
    lcd.setPointerCapture(ev.pointerId);
    const p = pos(ev); start = { p, y: p.y, idx: S.browseIndex, moved: false }; drag = -1;
    const slot = editSlot();
    if (S.mode === 'edit' && T.models[modelOf(slot)].name === 'Sample' && S.samples[slot] && inR(p, sideBox())) {
      const r = sideBox(), x = (p.x - r.x - 4) / (r.w - 8);
      drag = Math.abs(x - pv(slot, 'smp_start')) <= Math.abs(x - pv(slot, 'smp_end')) ? 0 : 1;
      moveMarker(p);
    }
  });
  function moveMarker(p) {
    const r = sideBox(), x = Math.max(0, Math.min(1, (p.x - r.x - 4) / (r.w - 8)));
    setParam(editSlot(), drag === 0 ? 'smp_start' : 'smp_end', x); updateKnobs();
  }
  lcd.addEventListener('pointermove', (ev) => {
    if (!start) return;
    const p = pos(ev);
    if (drag >= 0) { moveMarker(p); return; }
    if (S.mode === 'browse' && Math.abs(p.y - start.y) > 6) {
      start.moved = true;
      const target = start.idx - Math.round((p.y - start.y) / 23);
      if (target !== S.browseIndex) scrollBrowse(target - S.browseIndex);
    }
  });
  lcd.addEventListener('pointerup', (ev) => {
    if (!start) return;
    const p = pos(ev), moved = start.moved; start = null;
    if (drag >= 0) { drag = -1; return; }
    if (moved) return;
    if (S.mode === 'browse') {
      for (let i = 0; i < ROWS; i++) if (inR(p, listRow(i)) && S.browseTop + i < S.entries.length) {
        const idx = S.browseTop + i, now = performance.now();
        if (idx === S.browseIndex && (lastTap.i === idx && now - lastTap.t < 600 || idx === lastTap.i)) loadEntry();
        S.browseIndex = idx; lastTap = { t: now, i: idx }; drawLcd(); return;
      }
      return;
    }
    const pages = pagesOf(editSlot()), tabs = tabArea();
    if (inR(p, tabs)) { S.page = Math.min(pages.length - 1, Math.floor((p.x - tabs.x) / (tabs.w / pages.length))); rebuildKnobs(); return; }
    pages[S.page].knobs.forEach((k, i) => { if (inR(p, cellArea(i))) { S.focus = i; drawLcd(); } });
  });
})();

// ------------------------------------------------------------------ GÆRRO switch-on: the pixel cigarette
function drawSmoke() {
  const t = S.smokeT, TT = 2.6, px = 3, seed = 7;
  const dot = (x, y, c) => { g.fillStyle = c; g.fillRect(Math.floor(x / px) * px, Math.floor(y / px) * px, px, px); };
  const block = (x, y, w, h, c) => { g.fillStyle = c; g.fillRect(Math.floor(x / px) * px, Math.floor(y / px) * px, Math.floor(w / px) * px, Math.floor(h / px) * px); };
  const burn = Math.max(0, Math.min(1, (t - 0.15) / (TT - 0.75)));
  const pull = Math.pow(Math.max(0, Math.sin(Math.PI * ((t * 3 / (TT - 0.6)) % 1))), 3);
  const cy = 92, h = 18, x0 = 64, fw = 66, len = 330, end = x0 + fw + (len - fw) * (1 - 0.85 * burn);
  block(x0, cy - h / 2, fw, h, C.dim);
  for (let x = x0; x < x0 + fw; x += px * 2) for (let y = cy - h / 2 + ((x % (px * 4)) < px * 2 ? 0 : px); y < cy + h / 2; y += px * 2) dot(x, y, C.lcdBg2);
  block(x0 + fw, cy - h / 2, end - x0 - fw, h, C.ink);
  block(x0 + fw, cy + h / 2 - px, end - x0 - fw, px, C.dim);
  const ash = 6 + 12 * ((burn * 3) % 1);
  for (let x = end + px; x < end + ash; x += px) for (let y = cy - h / 2 + px; y < cy + h / 2; y += px) dot(x, y, ((x / px + y / px) | 0) % 2 ? C.dim : '#3b63c9');
  g.fillStyle = `rgba(232,241,255,${0.1 + 0.25 * pull})`; g.beginPath(); g.ellipse(end + 1, cy, 17, 18, 0, 0, 7); g.fill();
  block(end - px * 2, cy - h / 2, px * 3, h, `rgba(232,241,255,${0.55 + 0.45 * pull})`);
  let r = seed;
  const rnd = () => ((r = (r * 1103515245 + 12345) & 0x7fffffff) / 0x7fffffff);
  for (let i = 0; i < 220; i++) {
    const born = rnd() * TT, life = 0.9 + rnd() * 0.8, age = t - born, exhale = rnd() < 0.4, wob = rnd();
    if (age < 0 || age > life) continue;
    const k = age / life, sx = exhale ? x0 - 10 : end + 2;
    const x = sx + (exhale ? -40 * k : 18 * k) + 10 * Math.sin(born * 9 + age * 5 + wob) * k;
    const y = (exhale ? cy - 2 : cy - h / 2) - age * (exhale ? 26 : 60) - (exhale ? 30 * k * k : 0);
    if (y < 30) continue;
    const a = 0.85 * (1 - k);
    dot(x, y, `rgba(232,241,255,${a})`); dot(x + px, y, `rgba(232,241,255,${a * 0.7})`);
    if (k > 0.3) dot(x, y - px, `rgba(232,241,255,${a * 0.55})`);
  }
  const lines = ['ROLLING...', 'LIGHTING UP...', 'DIGGING CRATES...', 'WARMING THE NEEDLE...', 'READY.'];
  text('GÆRRO MODE', 6, 4, LW - 12, 26, 'left', C.ink, 26);
  text(lines[Math.min(4, Math.floor(t / TT * 5))], 6, 4, LW - 12, 26, 'right', C.dim, 19);
  const prog = Math.min(1, t / (TT - 0.3));
  g.strokeStyle = 'rgba(232,241,255,.8)'; g.strokeRect(64.5, 140.5, 330, 12);
  g.fillStyle = C.ink; for (let x = 67; x < 67 + 324 * prog; x += 6) g.fillRect(x, 143, 4, 6);
  text(Math.round(prog * 100) + '%', 402, 136, 60, 20, 'left', C.dim, 19);
}

// ------------------------------------------------------------------ knobs
const knobEls = [];
function makeKnob(el, { red = false, get, set, def, label }) {
  el.innerHTML = `<div class="cap"><img src="assets/${red ? 'knob_red' : 'knob_black'}.png" alt=""><svg viewBox="-72 -72 144 144"><g class="ticks"></g><line class="ptr-s" x1="0" y1="-14" x2="0" y2="-33" stroke="rgba(0,0,0,.6)" stroke-width="6.5" stroke-linecap="round"/><line class="ptr" x1="0" y1="-14" x2="0" y2="-33" stroke="#ece6d8" stroke-width="3.6" stroke-linecap="round"/></svg></div><div class="name"></div>`;
  const ticks = el.querySelector('.ticks');
  for (let k = 0; k <= 10; k++) {
    const a = (-135 + 27 * k) * Math.PI / 180, r0 = 55, r1 = k % 5 === 0 ? 63 : 59;
    ticks.insertAdjacentHTML('beforeend', `<line x1="${r0 * Math.sin(a)}" y1="${-r0 * Math.cos(a)}" x2="${r1 * Math.sin(a)}" y2="${-r1 * Math.cos(a)}" stroke="rgba(201,198,189,.55)" stroke-width="${k % 5 ? 1.6 : 2.2}"/>`);
  }
  const k = { el, get, set, def, update() {
    const n = this.get();
    if (n === null) { el.classList.add('empty'); return; }
    el.classList.remove('empty');
    const a = -135 + 270 * n;
    el.querySelectorAll('.ptr, .ptr-s').forEach((l) => l.setAttribute('transform', `rotate(${a})`));
  } };
  el.querySelector('.name').textContent = label || '';
  let drag = null, lastTap = 0;
  el.addEventListener('pointerdown', (ev) => {
    if (k.get() === null) return;
    el.setPointerCapture(ev.pointerId);
    const now = performance.now();
    if (now - lastTap < 300 && k.def) { k.set(k.def()); k.update(); lastTap = 0; return; }
    lastTap = now;
    drag = { y: ev.clientY, x: ev.clientX, n: k.get() };
    if (k.onGrab) k.onGrab();
  });
  el.addEventListener('pointermove', (ev) => {
    if (!drag) return;
    const d = (drag.y - ev.clientY + (ev.clientX - drag.x) * 0.5) / (ev.shiftKey ? 900 : 220);
    k.set(Math.max(0, Math.min(1, drag.n + d))); k.update();
  });
  const end = () => { drag = null; };
  el.addEventListener('pointerup', end); el.addEventListener('pointercancel', end);
  k.update();
  return k;
}
function rebuildKnobs() {
  const slot = editSlot(), pages = pagesOf(slot);
  S.page = Math.max(0, Math.min(pages.length - 1, S.page));
  const list = pages[S.page].knobs;
  S.focus = Math.min(S.focus, Math.max(0, list.length - 1));
  knobEls.forEach((k, i) => {
    const id = list[i];
    k.el.querySelector('.name').textContent = id === undefined ? '' : T.pad[id].name.toUpperCase();
    k.get = () => (id === undefined ? null : toNorm(T.pad[id], S.slots[editSlot()][id]));
    k.set = (n) => { setParam(editSlot(), T.pad[id].id, fromNorm(T.pad[id], n)); S.focus = i; if (T.pad[id].id === 'model') { rebuildKnobs(); } };
    k.def = () => toNorm(T.pad[id], T.pad[id].def);
    k.update();
  });
  drawLcd();
}
function updateKnobs() { knobEls.forEach((k) => k.update()); }

// ------------------------------------------------------------------ pads
const padEls = [];
function buildPads() {
  const grid = $('#grid');
  for (let row = 3; row >= 0; row--) for (let col = 0; col < 4; col++) {
    const i = row * 4 + col;
    const cell = document.createElement('div'); cell.className = 'cell';
    cell.innerHTML = `<div class="lab"><span class="t"></span><span class="n">${i + 1}</span></div>
      <div class="pad"><div class="halo"></div><img class="up-img" src="assets/pad_up.png" alt=""><img class="down-img" src="assets/pad_down.png" alt="">
      <img class="lit lit-r" src="assets/pad_lit.png" alt=""><img class="lit lit-g" src="assets/pad_lit_g.png" alt="" style="display:none"></div>`;
    grid.appendChild(cell);
    const pad = cell.querySelector('.pad');
    padEls[i] = { cell, pad, label: cell.querySelector('.t'), lit: cell.querySelectorAll('.lit'), halo: cell.querySelector('.halo'), last: -1 };
    let hold = 0, startY = 0, startX = 0;
    pad.addEventListener('pointerdown', (ev) => {
      if (ev.button === 2) return;
      pad.setPointerCapture(ev.pointerId);
      const r = pad.getBoundingClientRect();
      const vel = Math.max(0.25, Math.min(1, 1.05 - 0.75 * (ev.clientY - r.top) / r.height));
      ensureAudio();
      post({ type: 'hit', pad: i, vel });
      pad.classList.add('down'); setPadLit(i);
      if (!isSolo() && S.selected !== i) selectPad(i);
      startY = ev.clientY; startX = ev.clientX;
      hold = setTimeout(() => { hold = 0; pad.classList.remove('down'); post({ type: 'release', pad: i }); padMenu(i); }, 550);
    });
    pad.addEventListener('pointermove', (ev) => { if (hold && Math.hypot(ev.clientX - startX, ev.clientY - startY) > 12) { clearTimeout(hold); hold = 0; } });
    const up = () => { if (hold) { clearTimeout(hold); hold = 0; } pad.classList.remove('down'); setPadLit(i); post({ type: 'release', pad: i }); };
    pad.addEventListener('pointerup', up); pad.addEventListener('pointercancel', up);
    pad.addEventListener('contextmenu', (ev) => { ev.preventDefault(); padMenu(i); });
  }
}
function setPadLit(i) {
  const p = padEls[i], down = p.pad.classList.contains('down');
  p.lit[0].src = down ? 'assets/pad_lit_down.png' : 'assets/pad_lit.png';
  p.lit[1].src = down ? 'assets/pad_lit_g_down.png' : 'assets/pad_lit_g.png';
}
function selectPad(i) {
  S.selected = i;
  padEls.forEach((p, k) => p.cell.classList.toggle('sel', k === i));
  if (!isSolo()) { if (S.mode === 'browse' && S.browse === 'pads') refreshEntries(); rebuildKnobs(); }
}
function refreshPads() {
  padEls.forEach((p, i) => { p.label.textContent = S.labels[i]; p.cell.classList.toggle('sel', i === S.selected); });
  const sl = $('#soloLabel');
  sl.hidden = !isSolo();
  sl.textContent = (S.soloLabel || 'CHOOSE A SOUND') + '  ▾';
}
function renderLights() {
  const gaerro = isGaerro();
  padEls.forEach((p, i) => {
    const l = lights[i] <= 0 ? 0 : Math.pow(Math.min(1, lights[i]), 0.6);
    if (Math.abs(l - p.last) < 0.004 && !(l === 0 && p.last > 0)) return;
    p.last = l;
    p.lit[gaerro ? 1 : 0].style.opacity = (0.88 * l).toFixed(3);
    p.lit[gaerro ? 0 : 1].style.opacity = 0;
    p.lit[0].style.display = gaerro ? 'none' : ''; p.lit[1].style.display = gaerro ? '' : 'none';
    p.halo.style.opacity = l.toFixed(3);
  });
}

// ------------------------------------------------------------------ sheets (menus)
function sheet(title, build) {
  $('#sheetTitle').textContent = title;
  const body = $('#sheetBody'); body.innerHTML = '';
  build(body);
  $('#sheet').hidden = false;
}
const closeSheet = () => { $('#sheet').hidden = true; };
$('#sheetClose').onclick = closeSheet;
$('#sheet').addEventListener('pointerdown', (ev) => { if (ev.target.id === 'sheet') closeSheet(); });
function item(body, label, fn, sub) {
  const b = document.createElement('button'); b.className = 'menu-item'; b.innerHTML = label + (sub ? `<span class="sub">${sub}</span>` : '');
  b.onclick = () => { fn(); }; body.appendChild(b); return b;
}
function group(body, t) { const d = document.createElement('div'); d.className = 'menu-group'; d.textContent = t; body.appendChild(d); }
function ask(title, initial, ok) {
  sheet(title, (b) => {
    const inp = document.createElement('input'); inp.className = 'sheet-input'; inp.value = initial; inp.maxLength = 24; b.appendChild(inp);
    item(b, 'OK', () => { const v = inp.value.trim(); if (v) { closeSheet(); ok(v); } });
    setTimeout(() => inp.focus(), 50);
    inp.onkeydown = (e) => { if (e.key === 'Enter') { const v = inp.value.trim(); if (v) { closeSheet(); ok(v); } } };
  });
}
function libraryMenu(body, slot) {
  let cat = null;
  listPads().forEach((e) => {
    if (e.category !== cat) { cat = e.category; group(body, cat.toUpperCase()); }
    item(body, (e.user ? '★ ' : '') + e.name, () => { loadPad(slot, e); closeSheet(); flash('LOADED ' + e.name.toUpperCase()); });
  });
}
function padMenu(i) {
  if (isSolo()) { soloMenu(); return; }
  selectPad(i);
  sheet('PAD ' + (i + 1) + '  ' + S.labels[i], (b) => {
    item(b, 'Choose sound…', () => sheet('SOUND → PAD ' + (i + 1), (bb) => libraryMenu(bb, i)));
    if (isGaerro()) item(b, 'Load sample / chop from here…', () => { closeSheet(); pickSample(i); });
    item(b, 'Rename…', () => ask('RENAME PAD ' + (i + 1), S.labels[i], (t) => { S.labels[i] = t.toUpperCase().slice(0, 16); refreshPads(); changed(); }));
    item(b, 'Copy pad', () => { S.clipboard = { v: Float32Array.from(S.slots[i]), label: S.labels[i], sample: S.samples[i] }; closeSheet(); flash('COPIED'); });
    if (S.clipboard) item(b, 'Paste pad', () => {
      S.slots[i] = Float32Array.from(S.clipboard.v); S.labels[i] = S.clipboard.label; S.samples[i] = S.clipboard.sample;
      post({ type: 'slot', slot: i, values: Array.from(S.slots[i]) }); sendSample(i); closeSheet(); refreshAll(); changed();
    });
    item(b, 'Save as pad preset…', () => ask('SAVE PAD PRESET', S.labels[i] || 'My Sound', (t) => savePad(i, t)));
    item(b, 'Clear pad', () => {
      S.slots[i] = initSound(T.models.findIndex((m) => m.name === 'Empty')); S.labels[i] = ''; S.samples[i] = null;
      post({ type: 'slot', slot: i, values: Array.from(S.slots[i]) }); sendSample(i); closeSheet(); refreshAll(); changed();
    });
    group(b, 'CHOKE GROUP');
    const chips = document.createElement('div'); chips.className = 'chips'; b.appendChild(chips);
    for (let c = 0; c <= 8; c++) {
      const bt = document.createElement('button'); bt.textContent = c === 0 ? 'Off' : c;
      if (Math.round(pv(i, 'choke')) === c) bt.style.background = C.lcdBg;
      bt.onclick = () => { setParam(i, 'choke', c); closeSheet(); };
      chips.appendChild(bt);
    }
    group(b, 'PLAY MODE');
    item(b, 'One-shot', () => { setParam(i, 'play_mode', 0); closeSheet(); }, pv(i, 'play_mode') < 0.5 ? '●' : '');
    item(b, 'Gate (note length)', () => { setParam(i, 'play_mode', 1); closeSheet(); }, pv(i, 'play_mode') >= 0.5 ? '●' : '');
  });
}
function soloMenu() {
  sheet('SOLO  ' + S.soloLabel, (b) => {
    item(b, 'Choose sound…', () => sheet('SOUND → SOLO', (bb) => libraryMenu(bb, T.soloSlot)));
    item(b, 'Rename…', () => ask('RENAME SOLO SOUND', S.soloLabel, (t) => { S.soloLabel = t.toUpperCase(); refreshAll(); changed(); }));
    item(b, 'Mono (legato)', () => { setParam(T.soloSlot, 'poly', 0); refreshAll(); closeSheet(); }, pv(T.soloSlot, 'poly') < 0.5 ? '●' : '');
    item(b, 'Poly', () => { setParam(T.soloSlot, 'poly', 1); refreshAll(); closeSheet(); }, pv(T.soloSlot, 'poly') >= 0.5 ? '●' : '');
    item(b, 'Glide', () => { toggleGlide(); closeSheet(); }, pv(T.soloSlot, 'glide') > 0.5 ? '●' : '');
    group(b, 'PAD 1 PLAYS');
    const chips = document.createElement('div'); chips.className = 'chips'; b.appendChild(chips);
    for (let n = 24; n <= 84; n += 12) {
      const bt = document.createElement('button'); bt.textContent = noteName(n);
      if (Math.round(gv('keys_root')) === n) bt.style.background = C.lcdBg;
      bt.onclick = () => { setGlobal('keys_root', n); closeSheet(); };
      chips.appendChild(bt);
    }
    item(b, 'Leave solo mode', () => { setSolo(false); closeSheet(); });
  });
}
function savePad(slot, name) {
  const list = userPads().filter((p) => p.name !== name);
  list.push({ category: T.models[modelOf(slot)].name, name, label: slot === T.soloSlot ? S.soloLabel : S.labels[slot], params: paramsFromSound(S.slots[slot]) });
  store.set('pads', list); flash('SAVED PAD ' + name.toUpperCase());
}
function saveKit(name) {
  const kits = userKits(); kits[name] = kitToJson(name); store.set('kits', kits);
  S.kitName = name; refreshAll(); changed(); flash('SAVED KIT ' + name.toUpperCase());
}
function saveMenu() {
  sheet('SAVE', (b) => {
    if (userKits()[S.kitName]) item(b, `Save kit “${S.kitName}”`, () => { closeSheet(); saveKit(S.kitName); });
    item(b, 'Save kit as…', () => ask('SAVE KIT', S.kitName === 'Init' ? 'My Kit' : S.kitName, saveKit));
    const slot = editSlot();
    item(b, slot === T.soloSlot ? 'Save solo sound as preset…' : 'Save pad ' + (slot + 1) + ' as preset…',
      () => ask('SAVE PAD PRESET', (slot === T.soloSlot ? S.soloLabel : S.labels[slot]) || 'My Sound', (t) => savePad(slot, t)));
    item(b, 'Export kit file (.thudikit)…', () => { closeSheet(); exportKit(); }, 'opens in the plugin too');
    item(b, 'Import kit file…', () => { closeSheet(); $('#kitIn').click(); });
    item(b, 'New empty kit (Init)', () => { closeSheet(); initKit(); sendAll(); refreshAll(); changed(); flash('INIT: EMPTY KIT'); });
  });
}
function exportKit() {
  const json = JSON.stringify(kitToJson(S.kitName), null, 2);
  const a = document.createElement('a'); a.href = URL.createObjectURL(new Blob([json], { type: 'application/json' }));
  a.download = S.kitName + '.thudikit'; a.click(); setTimeout(() => URL.revokeObjectURL(a.href), 2000);
}
$('#kitIn').onchange = async (ev) => {
  const f = ev.target.files[0]; ev.target.value = ''; if (!f) return;
  try { const k = JSON.parse(await f.text()); if (k.format !== 'thudi-kit') throw 0; await loadKit(k); flash('LOADED ' + (k.name || '').toUpperCase()); }
  catch { flash('NOT A THUDI KIT'); }
};

// ------------------------------------------------------------------ soft keys, modes
document.querySelectorAll('.softkeys .key').forEach((b) => b.addEventListener('click', () => softKey(+b.dataset.k)));
function softKey(i) {
  if (S.smokeT >= 0) return;
  if (S.mode === 'edit') {
    const n = pagesOf(editSlot()).length;
    if (i === 0) { S.page = (S.page - 1 + n) % n; rebuildKnobs(); }
    else if (i === 1) { S.page = (S.page + 1) % n; rebuildKnobs(); }
    else if (i === 2) {
      S.mode = 'browse';
      S.browse = editSlot() === T.soloSlot || T.models[modelOf(editSlot())].name === 'Empty' ? 'pads' : 'kits';
      refreshEntries(); drawLcd();
    } else saveMenu();
  } else {
    if (i === 0) { S.browse = S.browse === 'kits' ? 'pads' : 'kits'; refreshEntries(); drawLcd(); }
    else if (i === 1) loadEntry();
    else if (i === 2) {
      if (S.browse === 'kits') ask('SAVE KIT', S.kitName === 'Init' ? 'My Kit' : S.kitName, (t) => { saveKit(t); refreshEntries(); drawLcd(); });
      else { const slot = editSlot(); ask('SAVE PAD PRESET', (slot === T.soloSlot ? S.soloLabel : S.labels[slot]) || 'My Sound', (t) => { savePad(slot, t); refreshEntries(); drawLcd(); }); }
    } else { S.mode = 'edit'; rebuildKnobs(); }
  }
}
$('#kitBtn').onclick = () => { S.mode = 'browse'; S.browse = 'kits'; refreshEntries(); drawLcd(); };
$('#soloBtn').onclick = () => setSolo(!isSolo());
$('#soloLabel').onclick = soloMenu;
$('#monoBtn').onclick = () => { setParam(T.soloSlot, 'poly', pv(T.soloSlot, 'poly') >= 0.5 ? 0 : 1); refreshAll(); };
$('#glideBtn').onclick = toggleGlide;
function toggleGlide() {
  const cur = pv(T.soloSlot, 'glide');
  if (cur > 0.5) { store.set('soloGlide', cur); setParam(T.soloSlot, 'glide', 0); } else setParam(T.soloSlot, 'glide', store.get('soloGlide', 70));
  refreshAll();
}
function setSolo(on) {
  setGlobal('solo', on ? 1 : 0); S.page = 0;
  if (S.mode === 'browse') refreshEntries();
  flash(on ? 'SOLO: ONE SOUND ON EVERY NOTE' : 'SOLO OFF');
  refreshAll();
}
function refreshAll() {
  $('#kitName').textContent = S.kitName.toUpperCase();
  document.body.classList.toggle('solo', isSolo());
  document.body.classList.toggle('gaerro', isGaerro());
  $('#soloBtn').classList.toggle('on', isSolo());
  $('#monoBtn').hidden = $('#glideBtn').hidden = !isSolo();
  $('#monoBtn').textContent = pv(T.soloSlot, 'poly') >= 0.5 ? 'POLY' : 'MONO';
  $('#glideBtn').classList.toggle('on', pv(T.soloSlot, 'glide') > 0.5);
  $('#gaerroBtn').hidden = !S.revealed && !isGaerro();
  $('#gaerroBtn').classList.toggle('on', isGaerro());
  padEls.forEach((p) => { p.last = -1; });
  refreshPads(); rebuildKnobs(); globalKnobs.forEach((k) => k.update());
}

// ------------------------------------------------------------------ GÆRRO
let plaqueTap = 0;
$('#plaque').addEventListener('click', () => {
  const now = performance.now();
  if (now - plaqueTap < 400) {
    if (!S.revealed) {
      S.revealed = true; store.set('gaerro', true);
      const b = $('#gaerroBtn'); b.hidden = false;
      b.animate([{ transform: 'translateY(-60px) scale(.6)', opacity: 0 }, { transform: 'none', opacity: 1 }], { duration: 450, easing: 'cubic-bezier(.2,1.4,.4,1)' });
      flash('GÆRRO MODE UNLOCKED');
    }
    plaqueTap = 0;
  } else plaqueTap = now;
});
$('#gaerroBtn').onclick = () => setGaerro(!isGaerro());

// where the stickers sit in the phone layout (parent section, CSS box, kind, delay in s)
const STICKERS = [
  ['g01_throwup', '#knobs', 'left:-7%;top:2%;width:114%;z-index:-1', 'tag', 0.10],
  ['g02_r4n', '#top', 'left:24%;top:-8px;width:44px;z-index:3', 'sticker', 0.85],
  ['g04_nosamples', '#display', 'left:-3%;bottom:30px;width:30%;z-index:3', 'sticker', 1.0],
  ['g03_hello', '#display', 'right:-3%;bottom:30px;width:25%;z-index:3', 'sticker', 1.12],
  ['g06_88', '#grid', 'left:43.5%;top:45%;width:13%;z-index:-1', 'sticker', 1.35],
  ['g11_record', '#pads', 'right:-11%;top:22%;width:22%;z-index:-1', 'sticker', 1.45],
  ['g12_vinyl', '#pads', 'left:-6%;top:62%;width:16%;z-index:-1', 'sticker', 1.55],
  ['g14_crate', '#pads', 'left:-5%;top:14%;width:8%;z-index:-1', 'sticker', 1.62],
  ['g07_tape', '#machine', 'left:2%;bottom:8px;width:52%;z-index:0', 'tag', 1.8],
  ['g09_r4n_tag', '#top', 'right:18%;top:-6px;width:18%;z-index:1', 'tag', 2.05],
  ['g17_star', '#knobs', 'right:-3%;bottom:-6%;width:9%;z-index:-1', 'tag', 2.2],
  ['g13_stay', '#machine', 'right:3%;bottom:10px;width:38%;z-index:0', 'sticker', 2.35],
];
let stickerEls = [];
function buildStickers() {
  stickerEls = STICKERS.map(([f, parent, css, kind, delay]) => {
    const im = document.createElement('img');
    im.src = 'assets/gaerro/' + f + '.png'; im.className = 'sticker' + (kind === 'tag' ? ' tag' : '');
    im.style.cssText = 'position:absolute;' + css; im.alt = '';
    $(parent).appendChild(im);
    return { im, delay };
  });
}
let smokeRaf = 0;
function setGaerro(on, animate = true) {
  setGlobal('gaerro', on ? 1 : 0);
  stickerEls.forEach(({ im, delay }) => {
    im.classList.remove('fadeout');
    if (on) { if (animate) setTimeout(() => im.classList.add('on'), delay * 1000); else im.classList.add('on'); }
    else { im.classList.add('fadeout'); setTimeout(() => { im.classList.remove('on'); im.classList.remove('fadeout'); }, 650); }
  });
  if (on && animate) {
    const t0 = performance.now(); S.smokeT = 0;
    const tick = () => { S.smokeT = (performance.now() - t0) / 1000; if (S.smokeT > 2.6) { S.smokeT = -1; drawLcd(); return; } drawLcd(); smokeRaf = requestAnimationFrame(tick); };
    cancelAnimationFrame(smokeRaf); tick();
  }
  if (!on) { S.smokeT = -1; flash('BACK TO CLEAN'); }
  refreshAll();
}

// sampling: pick an audio file, chop it over the pads
let samplePad = 0;
function pickSample(pad) { samplePad = pad; $('#fileIn').click(); }
$('#fileIn').onchange = async (ev) => { const f = ev.target.files[0]; ev.target.value = ''; if (f) chopFile(f, samplePad); };
async function chopFile(file, firstPad) {
  ensureAudio();
  let buf;
  try { buf = await ctx.decodeAudioData(await file.arrayBuffer()); } catch { flash("CAN'T READ " + file.name.toUpperCase()); return; }
  const n = buf.length, data = new Float32Array(n);
  for (let c = 0; c < buf.numberOfChannels; c++) { const ch = buf.getChannelData(c); for (let i = 0; i < n; i++) data[i] += ch[i] / buf.numberOfChannels; }
  const name = file.name.replace(/\.[^.]+$/, '');
  const smp = { id: name + '_' + n + '_' + Math.round(buf.sampleRate), name, rate: buf.sampleRate, data };
  samples.put(smp);
  const max = 16 - firstPad;
  sheet('CHOP ' + file.name.toUpperCase(), (b) => {
    group(b, 'SPREAD OVER HOW MANY PADS? (FROM PAD ' + (firstPad + 1) + ')');
    const chips = document.createElement('div'); chips.className = 'chips'; b.appendChild(chips);
    for (let k = 1; k <= max; k++) {
      const bt = document.createElement('button'); bt.textContent = k === 1 ? '1 (whole)' : k;
      bt.onclick = () => { closeSheet(); chopInto(smp, firstPad, k); };
      chips.appendChild(bt);
    }
  });
}
function chopInto(smp, firstPad, slices) {
  const e = mainWasm, f32 = (p, n) => new Float32Array(e.memory.buffer, p, n);
  const p = e.thudi_alloc(smp.data.length * 4), out = e.thudi_alloc(4 * 64);
  f32(p, smp.data.length).set(smp.data);
  const cnt = e.thudi_chop(p, smp.data.length, smp.rate, slices, out);
  const pts = Array.from(new Int32Array(e.memory.buffer, out, cnt));
  e.thudi_free(p); e.thudi_free(out);
  const model = T.models.findIndex((m) => m.name === 'Sample');
  pts.forEach((a, k) => {
    const pad = firstPad + k, v = initSound(model);
    v[P.smp_start] = a / smp.data.length; v[P.smp_end] = k + 1 < pts.length ? pts[k + 1] / smp.data.length : 1; v[P.choke] = 0;
    v[P.output] = S.slots[pad][P.output];
    S.slots[pad] = v; S.labels[pad] = 'CHOP ' + (k + 1); S.samples[pad] = smp;
    post({ type: 'slot', slot: pad, values: Array.from(v) }); sendSample(pad);
  });
  if (slices > 1) S.kitName = smp.name.toUpperCase().slice(0, 20);
  if (isSolo()) setGlobal('solo', 0);
  S.mode = 'edit'; S.page = 0; selectPad(firstPad); refreshAll(); changed();
  flash('CHOPPED INTO ' + pts.length + (pts.length === 1 ? ' PAD' : ' PADS'));
}
// desktop: drop an audio file anywhere
window.addEventListener('dragover', (e) => { if (isGaerro()) e.preventDefault(); });
window.addEventListener('drop', (e) => {
  if (!isGaerro()) return; e.preventDefault();
  const f = [...e.dataTransfer.files].find((x) => /audio|\.(wav|aiff?|mp3|flac|ogg|m4a)$/i.test(x.type + x.name));
  if (!f) return;
  const target = document.elementFromPoint(e.clientX, e.clientY);
  const i = padEls.findIndex((p) => p.cell.contains(target));
  chopFile(f, i >= 0 ? i : 0);
});

// ------------------------------------------------------------------ global knobs
const globalKnobs = [];
function globalKnob(el, id, label, red) {
  const pi = T.global[G[id]];
  const k = makeKnob(el, { red, label, get: () => toNorm(pi, gv(id)), set: (n) => setGlobal(id, fromNorm(pi, n)), def: () => toNorm(pi, pi.def) });
  globalKnobs.push(k);
}

// ------------------------------------------------------------------ audio
async function ensureAudio() {
  if (ctx) { if (ctx.state !== 'running') ctx.resume(); return; }
  ctx = new (window.AudioContext || window.webkitAudioContext)({ latencyHint: 'interactive' });
  await ctx.audioWorklet.addModule('worklet.js');
  node = new AudioWorkletNode(ctx, 'thudi', {
    numberOfInputs: 0, numberOfOutputs: 1, outputChannelCount: [2],
    processorOptions: { wasm: wasmBytes, oversampling: store.get('oversampling', 1), numParams: T.pad.length },
  });
  node.port.onmessage = (ev) => { if (ev.data.type === 'lights') lights.set(ev.data.v); };
  node.connect(ctx.destination);
  sendAll();
  if (ctx.state !== 'running') await ctx.resume();
}
function midi() {
  if (!navigator.requestMIDIAccess) return;
  navigator.requestMIDIAccess().then((acc) => {
    const hook = (inp) => { inp.onmidimessage = (m) => {
      const [st, n, v] = m.data, c = st & 0xf0;
      if (c === 0x90 && v > 0) { ensureAudio(); post({ type: 'note', note: n, vel: v / 127, on: true }); }
      else if (c === 0x80 || (c === 0x90 && v === 0)) post({ type: 'note', note: n, vel: 0, on: false });
    }; };
    acc.inputs.forEach(hook); acc.onstatechange = (e) => { if (e.port.type === 'input' && e.port.state === 'connected') hook(e.port); };
  }).catch(() => {});
}

function loop() { renderLights(); requestAnimationFrame(loop); }

// ------------------------------------------------------------------ boot
async function boot() {
  const [wb, pr] = await Promise.all([fetch('thudi.wasm').then((r) => r.arrayBuffer()), fetch('presets.json').then((r) => r.json())]);
  wasmBytes = wb; presets = pr;
  const { instance } = await WebAssembly.instantiate(wb, { wasi_snapshot_preview1: { fd_close: () => 0, fd_seek: () => 0, fd_write: () => 0, proc_exit: () => 0 } });
  mainWasm = instance.exports; mainWasm._initialize();
  const p = mainWasm.thudi_tables(), m = new Uint8Array(mainWasm.memory.buffer); let q = p; while (m[q]) q++;
  T = JSON.parse(new TextDecoder().decode(m.subarray(p, q)));
  T.pad.forEach((pi, i) => { P[pi.id] = i; });
  T.global.forEach((pi, i) => { G[pi.id] = i; });
  S.globals = Float32Array.from(T.global.map((pi) => pi.def));
  S.slots = Array.from({ length: T.slots }, () => Float32Array.from(T.pad.map((pi) => pi.def)));
  S.revealed = store.get('gaerro', false);
  initKit();

  for (let i = 0; i < 8; i++) {
    const el = document.createElement('div'); el.className = 'knob'; $('#knobs').appendChild(el);
    const k = makeKnob(el, { get: () => null, set: () => {} });
    k.onGrab = () => { S.focus = i; drawLcd(); };
    knobEls.push(k);
  }
  globalKnob($('#masterKnob'), 'master', 'MASTER', true);
  globalKnob($('#vinylKnob'), 'vinyl', 'VINYL');
  globalKnob($('#dirtKnob'), 'gaerro_dirt', 'DIRT');
  buildPads(); buildStickers();
  const session = store.get('session', null);
  if (session) await loadKit(session);
  refreshAll();
  if (isGaerro()) setGaerro(true, false);
  await document.fonts.ready;
  sizeLcd();
  new ResizeObserver(sizeLcd).observe(lcd);
  loop();
}

$('#powerBtn').onclick = async () => {
  $('#power').classList.add('off');
  try { await ensureAudio(); midi(); } catch (e) { flash('AUDIO NOT AVAILABLE'); console.error(e); }
};
boot().catch((e) => { console.error(e); document.body.insertAdjacentHTML('beforeend', '<div class="toast">Could not start Thudi: ' + e.message + '</div>'); });
if ('serviceWorker' in navigator && location.protocol === 'https:') navigator.serviceWorker.register('sw.js').catch(() => {});
