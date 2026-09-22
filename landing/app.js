"use strict";

// Phase flow and quantization are browser previews. The commit decision runs
// model.u as WebAssembly; no full CDC numeric receipt or U1/U2 result is issued.
const presets = {balanced: [0, Math.PI, 0, Math.PI], held: [Math.PI, 0, 0, Math.PI], aperture: [Math.PI / 2, Math.PI / 2, Math.PI / 2, Math.PI / 2]};
const frequencies = [1, -0.5, 0.5, -1];
const labels = ["A", "B", "C", "D"];
let phases = [...presets.balanced];
let latches = null;
let steps = 0;
let attempt = null;
const byId = id => document.getElementById(id);
const svgNS = "http://www.w3.org/2000/svg";
const signText = value => value > 0 ? "+1" : value < 0 ? "−1" : "0";
const instrumentControls = [...document.querySelectorAll(".controls button, .controls input")];
let nativeCommit = null;
let checkerMessage = "Loading the compiled prefix check…";
for (const control of instrumentControls) control.disabled = true;

function unavailable(message) {
  nativeCommit = null;
  checkerMessage = message;
  for (const control of instrumentControls) control.disabled = true;
  byId("verdict").className = "verdict held";
  byId("verdict").textContent = checkerMessage;
  byId("reason").textContent = "Commit is disabled until the compiled check and its input contract are available. Existing latches are unchanged.";
  byId("last-action").textContent = "No compiled decision is available.";
  document.querySelector(".instrument-result").dataset.engine = "unavailable";
}

function parameter(id) {
  const control = byId(id);
  const value = Number(control.value);
  if (control.value.trim() === "" || !Number.isFinite(value) || !control.checkValidity()) {
    control.reportValidity();
    return null;
  }
  return value;
}

function observation() {
  const deadband = parameter("deadband");
  if (deadband === null) return null;
  const trits = phases.map(theta => Math.cos(theta) > deadband ? 1 : Math.cos(theta) < -deadband ? -1 : 0);
  let sum = 0;
  const prefixes = trits.map(trit => sum += trit);
  return {trits, prefixes, firstNegative: prefixes.findIndex(value => value < 0)};
}

function render() {
  const state = observation();
  if (!state) return;
  const {trits, prefixes, firstNegative} = state;
  let accepted = false;
  if (nativeCommit) {
    try { accepted = nativeCommit(trits); }
    catch (error) { console.error("Prefix check refused an invalid invocation", error); unavailable("The compiled check is unavailable. Reload to try again."); }
  }
  byId("phase-circles").innerHTML = phases.map((theta, index) => {
    const degrees = theta * 180 / Math.PI;
    const x = 34 + Math.cos(theta) * 24;
    const y = 34 - Math.sin(theta) * 24;
    const normalized = ((degrees + 180) % 360 + 360) % 360 - 180;
    byId(`phase-${index}`).value = String(Math.round(normalized));
    byId(`angle-${index}`).textContent = `${degrees.toFixed(1)}°`;
    return `<div><span>${labels[index]}</span><svg viewBox="0 0 68 68" role="img" aria-label="Cell ${labels[index]}, ${degrees.toFixed(1)} degrees, trit ${trits[index]}"><circle cx="34" cy="34" r="25" class="phase-ring"/><line x1="34" y1="34" x2="${x}" y2="${y}" class="phase-hand"/><circle cx="${x}" cy="${y}" r="4" class="phase-dot"/></svg><strong>${signText(trits[index])}</strong></div>`;
  }).join("");
  byId("trit-expression").textContent = trits.map(signText).join("  ");
  const points = [0, ...prefixes].map((value, index) => [25 + index * 107.5, 98 - value * 17]);
  byId("prefix-path").setAttribute("d", points.map((point, i) => `${i ? "L" : "M"}${point[0]} ${point[1]}`).join(" "));
  byId("prefix-path").setAttribute("class", accepted ? "accepted-path" : "held-path");
  byId("prefix-plot").querySelector(".zero-line").setAttribute("y1", "98");
  byId("prefix-plot").querySelector(".zero-line").setAttribute("y2", "98");
  byId("prefix-plot").querySelector("text").setAttribute("y", "102");
  byId("prefix-points").replaceChildren();
  points.forEach(([x, y], index) => {
    const circle = document.createElementNS(svgNS, "circle");
    circle.setAttribute("cx", String(x)); circle.setAttribute("cy", String(y)); circle.setAttribute("r", "4");
    const number = document.createElementNS(svgNS, "text");
    number.setAttribute("x", String(x)); number.setAttribute("y", String(y - 11)); number.setAttribute("text-anchor", "middle");
    number.textContent = String(index ? prefixes[index - 1] : 0);
    byId("prefix-points").append(circle, number);
  });
  byId("prefix-title").textContent = `Prefix sums: ${prefixes.join(", ")}. ${nativeCommit ? accepted ? "Compiled prefix check accepts." : `First negative at cell ${labels[firstNegative]}.` : "Compiled decision unavailable."}`;
  if (!nativeCommit) {
    byId("verdict").className = "verdict held";
    byId("verdict").textContent = checkerMessage;
    byId("reason").textContent = "Commit is disabled until the compiled check and its input contract are available. Existing latches are unchanged.";
    byId("latches").textContent = latches ? `Latches: ${latches.map(signText).join("  ")}` : "Latches: empty";
    return;
  }
  byId("verdict").className = `verdict ${accepted ? "accepted" : "held"}`;
  byId("verdict").textContent = attempt === "accepted" ? "Commit accepted" : attempt === "held" ? "Commit held" : accepted ? "Commit available" : "The prefix barrier would hold";
  byId("reason").textContent = accepted ? `Prefixes ${prefixes.join(", ")} stay nonnegative. ${attempt === "accepted" ? "The current trits are now latched." : "This ordered walk may commit."}` : `Prefix ${firstNegative + 1} reaches ${prefixes[firstNegative]}. ${attempt === "held" ? "Previous latches are unchanged." : "A commit cannot pass this boundary."}`;
  byId("latches").textContent = latches ? `Latches: ${latches.map(signText).join("  ")}` : "Latches: empty";
  byId("step-count").textContent = `${steps} flow ${steps === 1 ? "step" : "steps"}`;
}

function startTrial(name = "balanced") {
  phases = [...presets[name]]; latches = null; steps = 0; attempt = null;
  for (const button of document.querySelectorAll("[data-preset]")) button.setAttribute("aria-pressed", String(button.dataset.preset === name));
  byId("last-action").textContent = "New trial. Latches cleared.";
  render();
}

for (const button of document.querySelectorAll("[data-preset]")) button.addEventListener("click", () => startTrial(button.dataset.preset));
for (let i = 0; i < 4; i++) byId(`phase-${i}`).addEventListener("input", event => {
  phases[i] = Number(event.target.value) * Math.PI / 180;
  latches = null; steps = 0; attempt = null;
  for (const button of document.querySelectorAll("[data-preset]")) button.setAttribute("aria-pressed", "false");
  byId("last-action").textContent = "Phase edited. New trial.";
  render();
});
byId("deadband").addEventListener("input", () => {attempt = null; render();});
byId("flow").addEventListener("click", () => {
  const duration = parameter("duration"), gain = parameter("gain");
  if (duration === null || gain === null) return;
  const previous = [...phases];
  phases = previous.map((theta, i) => theta + frequencies[i] * duration + gain * duration * Math.sin(previous[(i + 3) % 4] - theta));
  steps++; attempt = null;
  byId("last-action").textContent = `Synchronous flow, d = ${duration}.`;
  render();
});
byId("commit").addEventListener("click", () => {
  if (!nativeCommit) return;
  const state = observation();
  if (!state) return;
  try { attempt = nativeCommit(state.trits) ? "accepted" : "held"; }
  catch (error) {
    console.error("Commit refused an invalid invocation", error);
    unavailable("The compiled check is unavailable. Reload to try again.");
    return;
  }
  if (attempt === "accepted") latches = [...state.trits];
  byId("last-action").textContent = attempt === "accepted" ? "Commit wrote four latches." : "Commit held. No latch writes.";
  render();
});
byId("reset").addEventListener("click", () => {byId("duration").value = "0.25"; byId("gain").value = "0.15"; byId("deadband").value = "0.5"; startTrial();});
render();

async function loadCommitCheck() {
  const [moduleReply, receiptReply] = await Promise.all([fetch("model.wasm"), fetch("model.wasm.json")]);
  if (!moduleReply.ok || !receiptReply.ok) throw new Error("Compiled assets are unavailable");
  const [binary, receipt] = await Promise.all([moduleReply.arrayBuffer(), receiptReply.json()]);
  const names = ["a", "b", "c", "d"];
  if (receipt.contract?.entry !== "prefix_admissible" || receipt.contract?.return_type !== "Bool" ||
      receipt.abi_bounds?.length !== names.length || names.some((name, i) =>
        receipt.abi_bounds[i].name !== name || receipt.abi_bounds[i].low !== "-1" || receipt.abi_bounds[i].high !== "1")) {
    throw new Error("The compiled trit contract does not match this instrument");
  }
  const digest = new Uint8Array(await crypto.subtle.digest("SHA-256", binary));
  const hash = [...digest].map(byte => byte.toString(16).padStart(2, "0")).join("");
  if (hash !== receipt.binary_sha256) throw new Error("The compiled module does not match its receipt");
  const {instance} = await WebAssembly.instantiate(binary, {});
  const check = instance.exports.prefix_admissible;
  if (typeof check !== "function" || check.length !== names.length) throw new Error("Prefix export is unavailable");
  const bounds = receipt.abi_bounds.map(bound => [BigInt(bound.low), BigInt(bound.high)]);
  nativeCommit = trits => {
    if (trits.length !== bounds.length) throw new RangeError("Four ordered trits are required");
    const arguments64 = trits.map((trit, i) => {
      if (!Number.isSafeInteger(trit)) throw new RangeError("Exact trit required");
      const integer = BigInt(trit);
      if (integer < bounds[i][0] || integer > bounds[i][1]) throw new RangeError("Trit is outside the compiled input contract");
      return integer;
    });
    const result = check(...arguments64);
    if (result !== 0 && result !== 1) throw new Error("Prefix check returned a non-Boolean result");
    return result === 1;
  };
  document.querySelector(".instrument-result").dataset.engine = "u-wasm";
  for (const control of instrumentControls) control.disabled = false;
  byId("last-action").textContent = "Compiled U prefix check ready.";
  render();
}
loadCommitCheck().catch(error => {
  console.error("Unable to load the compiled prefix check", error);
  unavailable("The compiled check could not load. Reload to try again.");
});
