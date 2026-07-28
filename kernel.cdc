# kernel.cdc -- native self-hosting contract for BiDi Coherence-Delta Calculus.
# Python may load and check this file, but the language/kernel contract lives here.

kernel bidi stage=2 target=cdc
  term cell channel module field counter trace window measurement bridge policy
  term lifted-frame universal-record
  term durable-store persist-record

  rule flow commit nest relation trace trace-order window measure adapt synchronize
  rule existence-viability dyadic-triadic-closure invariant-check witness-check
  rule universal-close persist-gate replay-identity

  provides parser-state reducer-state witness-state trace-window-state
  provides balanced-ternary-carrier angular-phase path-relation invariant-gate
  provides dyadic-triadic-bridge closure-64 closure-512 closure-4096
  provides operational-bridge-runtime bridge-coordinate-runtime bridge64-grid interactive-bridge-grid native-reducer-runtime native-compiler-ir finite-proof-checker native-ir-interpreter native-full-surface-runtime
  provides trace-order-locality local-time local-counter-synchrony
  provides observer-window-coupling recursive-window-policy shared-state-commit
  provides council-deliberation bridge-coordinate-self-evolution
  provides existence-viability agency-spectrum
  provides transition-framework procedural-framework episodic-framework deliberative-framework persistence-framework
  provides framework-contract task-loop-composition bidi-gated-durable-mutation
  provides universal-operator radiant-receptive-cones lifted-720-closure phase-holonomy
  provides native-witness-suite native-capability-suite native-self-hosting-contract

  bootloader read-source parse-lines collect-native-declarations verify-expectations report

  expect native substrate == cdc
  expect host-debt <= 1
  expect python-files == 0
  expect bootloader minimal == true
  expect terms >= 14
  expect rules >= 17
  expect invariants >= 16
  expect witnesses >= 4824
  expect capabilities >= 39
  expect frameworks >= 6
  expect frameworks closed
  expect provides parser-state reducer-state witness-state trace-window-state
  expect provides balanced-ternary-carrier angular-phase path-relation invariant-gate
  expect provides dyadic-triadic-bridge closure-64 closure-512 closure-4096
  expect provides operational-bridge-runtime bridge-coordinate-runtime bridge64-grid interactive-bridge-grid native-reducer-runtime native-compiler-ir finite-proof-checker native-ir-interpreter native-full-surface-runtime
  expect provides trace-order-locality local-time local-counter-synchrony
  expect provides observer-window-coupling recursive-window-policy shared-state-commit
  expect provides council-deliberation bridge-coordinate-self-evolution
  expect provides existence-viability agency-spectrum
  expect provides transition-framework procedural-framework episodic-framework deliberative-framework persistence-framework
  expect provides framework-contract task-loop-composition bidi-gated-durable-mutation
  expect provides universal-operator radiant-receptive-cones lifted-720-closure phase-holonomy
  expect provides native-witness-suite native-capability-suite native-self-hosting-contract
end
