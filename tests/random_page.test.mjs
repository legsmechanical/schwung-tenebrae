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
  last: null, seq: 0, pending: 0, busy: 0, name: "BS Night Drive", arch: "", cat: -1, anchor: "",
  base: "BS Night Drive", rolls: 0, n: 0, log: [],
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
      this.arch = op === 2 ? "Vary" : ["Sub", "Sync Scream", "Glass"][this.cat] || "Thing";
      this.name = `Rand ${["Bass", "Lead", "Pad"][this.cat] || "X"} ${this.n}`;
    } else if (op === 0) { this.pending = 0; }
    else if (op === 1 && this.pending) { this.name = this.anchor; this.pending = 0; this.arch = ""; }
    this.seq++;
  },
  get(key) {
    if (key === "rnd_cmd") return String(this.last);
    if (key === "rnd_status") return JSON.stringify({ seq: this.seq, pending: this.pending, busy: this.busy,
      name: this.name, arch: this.arch, cat: this.cat, anchor: this.pending ? this.anchor : "" });
    return null;
  },
};

// ---- the screen ------------------------------------------------------------------
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
const sandbox = { Math, Date, JSON, String, Number, console };
sandbox.globalThis = sandbox;
vm.createContext(sandbox);
vm.runInContext(readFileSync(join(root, "src", "random.js"), "utf8"), sandbox);
const page = sandbox.canvas_overlay;
ok(page && typeof page.drawPage === "function" && typeof page.onMidi === "function" && typeof page.handleBack === "function",
   "random.js installs drawPage, onMidi and handleBack");

let now = 1000;
function draw() {
  fresh();
  now += 16;
  page.drawPage(drawCtx, { nowMs: now, values: { rnd_status: plugin.get("rnd_status") } });
}
const jog = (d) => page.onMidi(ctx, { data: [0xB0, 14, d > 0 ? d : 128 + d] });
const click = () => page.onMidi(ctx, { data: [0xB0, 3, 127] });
const label = (t) => texts.find((x) => x.t === t);

// ---- arriving: a door, drawn un-entered ---------------------------------------
draw();
ok(px[0][4] && px[0][7] && px[2][4] && px[44][4] && px[0][123], "un-entered, the host's corner brackets are drawn");
ok(!px[0][60], "and no row is highlighted");
ok(label("Vary Current") && label("Bass") && label("Keys") && !label("Pluck"), "five rows show: Vary Current, then the categories");
ok(px[1][126] === 1 || px[3][126] === 1, "a scrollbar shows there is more below");

// ---- entering is inferred from the first gesture ------------------------------
jog(1);
draw();
ok(!px[0][4] || px[0][60], "the first gesture enters: the brackets go");
ok(px[8][60] === 0 && px[9][60] === 1 && px[17][60] === 1 && px[18][60] === 0 && label("Bass").c === 0,
   "the cursor moved to Bass, highlighted in the host's geometry (one row above the text, 9 tall)");
for (let i = 0; i < 20; i++) jog(1);
draw();
ok(label("FX") && label("FX").c === 0 && !label("Vary Current"), "scrolling reaches FX and stops there");
for (let i = 0; i < 20; i++) jog(-1);
draw();

// ---- rolling ------------------------------------------------------------------------
jog(1);                       // Bass
click();
draw();
ok(plugin.log.join() === "10" && plugin.pending === 1, "clicking Bass sends one roll (op 10)");
ok(texts.every((x) => ["Vary Current", "Bass", "Lead", "Pad", "Keys", "Pluck"].includes(x.t)), "the list shows only its rows");
const first = plugin.last;
click();
ok(plugin.last !== first && plugin.log.join() === "10,10", "clicking again rolls again, with a new value");
// a host re-sending an unconfirmed write sends the same value: one roll, not four
const rolls = plugin.rolls;
for (let i = 0; i < 3; i++) plugin.set("rnd_cmd", String(plugin.last));
ok(plugin.rolls === rolls, "a re-sent command value rolls nothing (dAVEBOx re-sends until confirmed)");
jog(-1);                      // Vary Current
click();
ok(plugin.log[plugin.log.length - 1] === 2, "Vary Current sends op 2");

// ---- Back while a roll is playing asks -----------------------------------------
ok(page.handleBack(ctx) === true, "Back with a roll not kept stays on the page");
draw();
ok(texts.some((x) => /^Keep Rand /.test(x.t)) && label("Keep") && label("Revert"), "and asks: Keep the roll, or Revert");
ok(texts.some((x) => x.t.includes("BS Night Drive")), "naming the sound Revert goes back to");
ok(page.handleBack(ctx) === true, "Back in the question returns to the list");
draw();
ok(label("Bass"), "the list is back");
page.handleBack(ctx);
jog(1);                       // Revert
click();
ok(plugin.log[plugin.log.length - 1] === 1 && plugin.pending === 0 && plugin.name === "BS Night Drive" && closed === 1,
   "choosing Revert puts the sound back and leaves the page");
draw(); jog(1); click();      // roll Bass again, then keep it
page.handleBack(ctx);
click();                      // Keep is the default
ok(plugin.log[plugin.log.length - 1] === 0 && plugin.pending === 0 && closed === 2, "Keep (the default) keeps it and leaves");
ok(page.handleBack(ctx) === false, "Back with nothing to keep leaves at once");

// ---- Shift+jog leaves silently; coming back -------------------------------------
draw(); jog(1); click();      // a roll, not kept
now += 5000;                  // the page is not drawn while you are elsewhere
draw();
ok(px[0][4] === 1, "after time away the page is drawn un-entered again");
ok(page.handleBack(ctx) === true, "and Back still offers Revert for the roll left playing");

// ---- a slow host is not "you left" ------------------------------------------------
draw(); jog(1);
now += 600;                   // the host skipped frames for 600 ms
draw();
ok(px[0][4] === 0 && texts.some((x) => x.c === 0), "a 600 ms stall in drawing does not drop you out of the list");

// ---- the draw never reads params ------------------------------------------------
let threw = null;
try { draw(); } catch (e) { threw = e; }
ok(!threw, "drawPage runs with a context that cannot read params (stock strips them)");

console.log(failures ? `\n${failures} FAILED` : "\nrandom page ok");
process.exit(failures ? 1 : 0);
