// The Random page script (src/random.js), driven headlessly.
//
//   node tests/random_page.test.mjs
//
// It runs against a small model of the plugin's side of the protocol -- the
// command key, nonce de-duplication, and the status it answers -- the same
// behaviour tools/tenebrae_tool.c's selftest checks in the real plugin. The
// hooks are called the way the hosts call them: drawPage with a context that
// cannot read params (stock strips getParam / setParam from draw), onMidi and
// handleBack with one that can.
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import vm from "node:vm";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
let failures = 0;
const ok = (c, m) => { if (!c) { failures++; console.error("FAIL " + m); } else console.log("ok   " + m); };

// ---- the plugin, as the page sees it ------------------------------------------
const plugin = {
  last: null, seq: 0, pending: 0, busy: 0, name: "BS Night Drive", cat: -1, anchor: "",
  rk: [0, 0, 0, 0, 0, 0, 0, 0], rolls: 0, n: 0, log: [],
  set(key, val) {
    if (key !== "rnd_cmd") return;
    const v = Number(val);
    if (v === this.last) return;            // a re-sent value: ignored
    this.last = v;
    const op = v % 100;
    this.log.push(op);
    if (op >= 10 || op === 2) {
      if (!this.pending) { this.anchor = this.name; this.pending = 1; }
      this.n++; this.rolls++;
      this.cat = op === 2 ? 0 : op - 10;
      this.name = `Rand ${["Bass", "Lead", "Pad"][this.cat] || "X"} ${this.n}`;
    } else if (op === 0) { this.pending = 0; }
    else if (op === 1 && this.pending) { this.name = this.anchor; this.pending = 0; }
    this.seq++;
  },
  get(key) {
    if (key === "rnd_cmd") return String(this.last);
    if (key === "rnd_status") return JSON.stringify({ seq: this.seq, pending: this.pending, busy: this.busy,
      name: this.name, arch: "", cat: this.cat, anchor: this.pending ? this.anchor : "", rk: this.rk });
    return null;
  },
};

// ---- the screen (the page's band) ---------------------------------------------
const W = 128, H = 45;
let px, texts, closed = 0;
function fresh() { px = Array.from({ length: H }, () => new Uint8Array(W)); texts = []; }
const drawCtx = {
  width: W, height: H,
  fillRect(x, y, w, h, c) {
    for (let j = y; j < y + h; j++) for (let i = x; i < x + w; i++) if (j >= 0 && j < H && i >= 0 && i < W) px[j][i] = c ? 1 : 0;
  },
  print(x, y, t, c) { texts.push({ x, y, t: String(t), c }); },
};
const ctx = { ...drawCtx, getParam: (k) => plugin.get(k), setParam: (k, v) => plugin.set(k, v), close: () => { closed++; return true; } };

// ---- the script ------------------------------------------------------------------
const sandbox = { Math, Date, JSON, String, Number, Array, isFinite, console };
sandbox.globalThis = sandbox;
vm.createContext(sandbox);
vm.runInContext(readFileSync(join(root, "src", "random.js"), "utf8"), sandbox);
const page = sandbox.canvas_overlay;
ok(page && typeof page.drawPage === "function" && typeof page.onMidi === "function" && typeof page.handleBack === "function",
   "random.js installs drawPage, onMidi and handleBack");

let now = 1000;
function draw(values) {
  fresh();
  now += 16;
  page.drawPage(drawCtx, { nowMs: now, values: Object.assign({ rnd_status: plugin.get("rnd_status") }, values || {}) });
}
const jog = (d) => page.onMidi(ctx, { data: [0xB0, 14, d > 0 ? d : 128 + d] });
const click = () => page.onMidi(ctx, { data: [0xB0, 3, 127] });
const line = () => texts.find((x) => x.y > 15 && x.y < 30 && x.t.length > 1);
const LINE_Y = 17, TOP_FADER = 1 + 8 + 1, BOT_FADER = 33 + 8 + 1;   // y of each row's fader track
const midOf = (cell) => cell * 32 + 3 + 13;                          // a fader's centre x
// (the track is dotted at even offsets from its start: probe empty space at odd ones)

// ---- the knob rows: labels in the knob pages' own font, faders -------------------
draw();
ok(texts.every((t) => t.y > 15 && t.y < 30), "the knob labels are not in the screen font (they are drawn in the 4x5 label font)");
ok(px[1].some((v) => v) && px[33].some((v) => v), "both knob rows draw their labels");
ok(px[TOP_FADER - 1][midOf(0)] === 1 && px[TOP_FADER][midOf(0) + 6] === 0 && px[TOP_FADER][midOf(0) - 6] === 0,
   "a bipolar fader at its default shows just its centre mark");
plugin.rk = [1, -1, 0, 0, 0, 0, 0, 0];
draw();
ok(px[TOP_FADER][midOf(0) + 10] === 1 && px[TOP_FADER][midOf(0) - 6] === 0, "at +1 it fills from the centre to the right");
ok(px[TOP_FADER][midOf(1) - 10] === 1 && px[TOP_FADER][midOf(1) + 6] === 0, "at -1 it fills from the centre to the left");
plugin.rk = [0, 0, 0, 0, 0, 0, 0.5, 0];
draw();
ok(px[BOT_FADER][2 * 32 + 3 + 2] === 1 && px[BOT_FADER][2 * 32 + 3 + 21] === 0, "WILD is one-sided: it fills from its left end");
draw({ rnd_wild: "1" });
ok(px[BOT_FADER][2 * 32 + 3 + 24] === 1, "a value the host hands the draw wins over the status");
plugin.rk = [0, 0, 0, 0, 0, 0, 0, 0];

// ---- the generator line: a door, drawn un-entered first --------------------------
draw();
ok(line().t === "Vary Current" && line().c === 1, "the line starts on Vary Current, not highlighted");
ok(px[LINE_Y - 2][4] === 1 && px[LINE_Y - 2][7] === 1, "un-entered, the host's corner brackets frame the line");
jog(1);
draw();
ok(line().t === "Bass" && line().c === 0 && px[LINE_Y][60] === 1, "the first turn enters: the line is highlighted, now Bass");
ok(texts.some((t) => t.t === "<") && texts.some((t) => t.t === ">"), "arrows show there is more either way");
for (let i = 0; i < 20; i++) jog(1);
draw();
ok(line().t === "FX" && !texts.some((t) => t.t === ">"), "turning right stops at FX");
for (let i = 0; i < 20; i++) jog(-1);
draw();
ok(line().t === "Vary Current" && !texts.some((t) => t.t === "<"), "turning left stops at Vary Current");

// ---- generating ----------------------------------------------------------------------
jog(1); click();
ok(plugin.log.join() === "10" && plugin.pending === 1, "clicking on Bass makes a bass (op 10)");
const first = plugin.last;
click();
ok(plugin.last !== first && plugin.log.join() === "10,10", "clicking again makes another, with a new value");
const rolls = plugin.rolls;
for (let i = 0; i < 3; i++) plugin.set("rnd_cmd", String(plugin.last));
ok(plugin.rolls === rolls, "a re-sent command value makes nothing (dAVEBOx re-sends until confirmed)");
jog(-1); click();
ok(plugin.log[plugin.log.length - 1] === 2, "Vary Current sends op 2");

// ---- Back while a new sound is playing asks ---------------------------------------
ok(page.handleBack(ctx) === true, "Back with a sound not kept stays on the page");
draw();
ok(texts.some((x) => /^Keep Rand /.test(x.t)) && texts.some((x) => x.t === "Keep") && texts.some((x) => x.t === "Revert"),
   "and asks: Keep, or Revert");
ok(texts.some((x) => x.t.includes("BS Night Drive")), "naming the sound Revert goes back to");
ok(page.handleBack(ctx) === true, "Back in the question returns to the page");
draw();
ok(line().t === "Vary Current", "the page is back, on the same generator");
page.handleBack(ctx);
jog(1); click();              // Revert
ok(plugin.log[plugin.log.length - 1] === 1 && plugin.pending === 0 && plugin.name === "BS Night Drive" && closed === 1,
   "choosing Revert puts the sound back and leaves the page");
draw(); jog(1); click();      // generate, then keep it
page.handleBack(ctx);
click();                      // Keep is the default
ok(plugin.log[plugin.log.length - 1] === 0 && plugin.pending === 0 && closed === 2, "Keep (the default) keeps it and leaves");
ok(page.handleBack(ctx) === false, "Back with nothing to keep leaves at once");

// ---- a slow host is not "you left"; a real absence is ------------------------------
draw(); jog(1);
now += 600;
draw();
ok(px[LINE_Y][60] === 1, "a 600 ms stall in drawing does not drop you out of the page");
click();                      // a sound, not kept
now += 5000;
draw();
ok(px[LINE_Y - 2][4] === 1, "after real time away the page is drawn un-entered again");
ok(page.handleBack(ctx) === true, "and Back still offers Revert for the sound left playing");

// ---- the draw never reads params ------------------------------------------------
let threw = null;
try { draw(); } catch (e) { threw = e; }
ok(!threw, "drawPage runs with a context that cannot read params (stock strips them)");

console.log(failures ? `\n${failures} FAILED` : "\nrandom page ok");
process.exit(failures ? 1 : 0);
