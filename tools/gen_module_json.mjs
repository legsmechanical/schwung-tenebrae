/* Write the plugin's own chain_params into src/module.json.
 *
 *   node tools/gen_module_json.mjs        (run by scripts/build.sh, on the host)
 *
 * This is NOT the on-screen UI: a sound generator's pages come from the
 * plugin's ui_hierarchy and nowhere else. It is the host's parameter-metadata
 * table, which the chain host builds by reading module.json off disk when the
 * synth loads, and which modulation targets, patch recall and value smoothing
 * resolve against. A key missing there fails silently.
 *
 * The block is taken from build/tenebrae_tool -- the real plugin, built for this
 * machine -- so it cannot drift from what the module serves. Only the fields
 * the host's C parser reads are kept, to keep the file small.
 */
import { execFileSync } from "node:child_process";
import { readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");
const served = JSON.parse(execFileSync(join(root, "build", "tenebrae_tool"), ["json", "chain_params"], { encoding: "utf8" }));
const keep = ["key", "name", "type", "min", "max", "step", "default", "unit", "options"];
const params = served.map((p) => Object.fromEntries(keep.filter((k) => k in p).map((k) => [k, p[k]])));

const path = join(root, "src", "module.json");
const mod = JSON.parse(readFileSync(path, "utf8"));
mod.capabilities.chain_params = params;
/* one parameter per line: small enough for every loader, still diffable */
const head = JSON.stringify({ ...mod, capabilities: { ...mod.capabilities, chain_params: "@@" } }, null, 2);
const body = "[\n" + params.map((p) => "      " + JSON.stringify(p)).join(",\n") + "\n    ]";
writeFileSync(path, head.replace('"@@"', body) + "\n");
console.log(`module.json: ${params.length} chain_params, ${readFileSync(path).length} bytes`);
