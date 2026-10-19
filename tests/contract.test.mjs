// What the module SERVES, checked against what the host can draw.
//
//   node tests/contract.test.mjs        (build/tenebrae_tool must exist)
//
// The contract is read out of the real plugin, built for this machine, so a
// passing run is about the JSON the device will be handed and not about a
// source literal. The host's own planner and validator are imported from the
// checkout beside this repo (../schwung-current); without it those checks are
// skipped, loudly.
import { execFileSync } from "node:child_process";
import { existsSync, readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const tool = join(root, "build", "tenebrae_tool");
const get = (k) => JSON.parse(execFileSync(tool, ["json", k], { encoding: "utf8" }));

let failures = 0;
const ok = (c, m) => { if (!c) { failures++; console.error("FAIL " + m); } else console.log("ok   " + m); };

const hierarchy = get("ui_hierarchy");
const chainParams = get("chain_params");
const state = get("state");
const byKey = new Map(chainParams.map((p) => [p.key, p]));
const levels = hierarchy.levels;

// ---- the shape of the brief: 3 synth pages, 2 effect pages --------------------
const links = levels.root.params.filter((p) => p && p.level).map((p) => p.level);
ok(JSON.stringify(links) === JSON.stringify(["osc", "filter", "amp", "motion", "space"]),
   "root links five pages: three for the synth, two for effects");
ok(levels.root.list_param === "preset" && levels.root.count_param === "preset_count",
   "root is the preset browser");
ok(!levels.root.knobs, "root carries no knobs (the browser is page one, then the five pages)");

const seen = new Set();
for (const id of links) {
  const l = levels[id];
  ok(Array.isArray(l.knobs) && l.knobs.length === 8, `${id}: exactly eight knobs`);
  ok(JSON.stringify(l.knobs) === JSON.stringify(l.params), `${id}: params and knobs agree`);
  for (const k of l.knobs) {
    ok(byKey.has(k), `${id}: ${k} is declared in chain_params`);
    ok(!seen.has(k), `${id}: ${k} sits on one page only`);
    seen.add(k);
  }
}
ok(seen.size === chainParams.length, `every declared parameter is on a page (${seen.size} of ${chainParams.length})`);

// ---- declarations the host acts on -------------------------------------------
for (const p of chainParams) {
  if (p.options) {
    ok(p.type === "enum", `${p.key}: a labelled parameter says "enum"`);
    ok(p.short_options && p.short_options.length === p.options.length, `${p.key}: one short option per option`);
    ok(Number.isInteger(p.default) && p.default >= 0 && p.default < p.options.length, `${p.key}: default is a valid index`);
  } else {
    ok(p.type === "float" && p.max === 1 && (p.min === 0 || p.min === -1), `${p.key}: float 0..1 or -1..1`);
    ok(p.default >= p.min && p.default <= p.max, `${p.key}: default inside its range`);
  }
  ok(typeof p.short_name === "string" && p.short_name.length <= 6, `${p.key}: short name fits the cell`);
  ok(p.key in state, `${p.key}: saved in the state blob`);
}

// a graphic must sit inside one row of four
const rowOf = new Map();
for (const id of links) levels[id].knobs.forEach((k, i) => rowOf.set(k, `${id}:${Math.floor(i / 4)}`));
const groups = new Map();
for (const p of chainParams) if (p.viz && p.viz.group) {
  if (!groups.has(p.viz.group)) groups.set(p.viz.group, []);
  groups.get(p.viz.group).push(p.key);
}
for (const [g, keys] of groups) {
  ok(new Set(keys.map((k) => rowOf.get(k))).size === 1, `graphic "${g}" sits in one row (${keys.join(", ")})`);
  const l = levels[rowOf.get(keys[0]).split(":")[0]].knobs;
  const idx = keys.map((k) => l.indexOf(k)).sort((a, b) => a - b);
  ok(idx[idx.length - 1] - idx[0] === idx.length - 1, `graphic "${g}" is contiguous`);
}

// ---- module.json carries the same parameters -----------------------------------
const mod = JSON.parse(readFileSync(join(root, "src", "module.json"), "utf8"));
const mj = mod.capabilities.chain_params;
ok(mj.length === chainParams.length, "module.json lists every parameter (the host's modulation table)");
for (const m of mj) {
  const s = byKey.get(m.key);
  const same = s && s.type === m.type && s.min === m.min && s.max === m.max &&
               JSON.stringify(s.options) === JSON.stringify(m.options) && s.default === m.default;
  if (!same) ok(false, `module.json ${m.key} matches what the plugin serves (run scripts/build.sh)`);
}
ok(!("ui_hierarchy" in mod) && !("ui_hierarchy" in mod.capabilities),
   "module.json carries no hierarchy (a synth's is read from the plugin only)");
ok(mod.capabilities.component_type === "sound_generator" && mod.dsp === "dsp.so", "module.json: sound generator, dsp.so");

// ---- help.json: the loader only reads `children`, and lines are not wrapped -----
const help = JSON.parse(readFileSync(join(root, "src", "help.json"), "utf8"));
ok(Array.isArray(help.children) && help.children.length > 0, "help.json has top-level children");
let longest = 0, nonAscii = 0;
(function walk(n) {
  for (const c of n.children || []) walk(c);
  for (const l of n.lines || []) { longest = Math.max(longest, l.length); if (/[^\x20-\x7e]/.test(l)) nonAscii++; }
})(help);
ok(longest <= 21, `help lines fit the screen (longest ${longest} characters)`);
ok(nonAscii === 0, "help text is plain ASCII");

// ---- the host's own planner and validator ---------------------------------------
const hostDir = join(root, "..", "schwung-current", "src", "shared", "param_pages");
if (!existsSync(hostDir)) {
  console.log("SKIP host planner checks: ../schwung-current not found");
} else {
  const { validateContract } = await import(pathToFileURL(join(hostDir, "validate_contract.mjs")));
  const { planPages, PAGE_KNOBS, PAGE_PRESET } = await import(pathToFileURL(join(hostDir, "page_plan.mjs")));
  const { resolveViz } = await import(pathToFileURL(join(hostDir, "viz.mjs")));
  const r = validateContract({ id: "tenebrae", hierarchy, chainParams, capabilities: mod.capabilities });
  const bad = r.findings.filter((f) => f.level !== "info");
  for (const f of r.findings) console.log(`     host ${f.level}: ${f.rule} - ${f.message}`);
  ok(bad.length === 0, "host validator: no errors or warnings");
  const { pages } = planPages({ hierarchy, chainParams });
  ok(pages[0].kind === PAGE_PRESET, "host plans the preset browser first");
  const grids = pages.filter((p) => p.kind === PAGE_KNOBS);
  ok(grids.length === 5, `host plans five knob pages (${grids.map((p) => p.name).join(" / ")})`);
  ok(grids.every((p, i) => JSON.stringify(p.keys) === JSON.stringify(levels[links[i]].knobs)),
     "each planned page holds its level's eight knobs, in order");
  ok(pages.length === 6, "nothing else is planned");
}

console.log(failures ? `\n${failures} FAILED` : "\ncontract ok");
process.exit(failures ? 1 : 0);
