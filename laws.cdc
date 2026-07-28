# laws.cdc -- invariant registry and law witnesses in native .cdc.

invariant balanced-ternary-carrier statement="committed outcomes are -1/0/+1 around equilibrium"
invariant dyadic-triadic-closure statement="2^6 equals 4^3 equals 64 bridge states"
invariant existence-viability statement="frames persist by bounded coherent continuity plus transition capacity"
invariant trace-order-locality statement="trace time is local to bounded observer windows"
invariant gate-abelian statement="gate composition is an abelian phase group"
invariant interfere-monoid statement="interference is a commutative monoid"
invariant rotation-linear statement="rotation distributes over interference"
invariant corefold-morphism statement="core-fold is linear and rotation-equivariant"
invariant preservation statement="commits preserve admissible nonnegative trit walks"
invariant soundness statement="accepted commits do not increase local potential"
invariant local-confluence statement="disjoint commits commute"
invariant flow-additivity statement="flow composes over duration"
invariant normalforms statement="localized committed modules are stable values"
invariant universal-closure statement="a lifted frame closes only after two turns with declared holonomy and enacts its computed record"
invariant durable-latch-or-hold statement="durable mutation happens only under an accepted balance barrier; a held decision writes nothing"
invariant replay-identity statement="replay identity depends on the sealed history, not on the log layout that carries it"

witness law-balanced-center invariant=balanced-ternary-carrier claim="balanced trit carrier sums to equilibrium"
witness law-balanced-enumeration invariant=balanced-ternary-carrier claim="all 3^6 committed walks stay in carrier"
witness law-bridge-64 invariant=dyadic-triadic-closure claim="dyadic and triadic bridge codebooks are bijective"
witness law-gate-associative invariant=gate-abelian claim="gate is associative"
witness law-gate-commutative invariant=gate-abelian claim="gate is commutative"
witness law-gate-identity invariant=gate-abelian claim="gate has identity"
witness law-gate-inverse invariant=gate-abelian claim="gate has inverse"
witness law-interfere-associative invariant=interfere-monoid claim="interference is associative"
witness law-interfere-commutative invariant=interfere-monoid claim="interference is commutative"
witness law-interfere-unit invariant=interfere-monoid claim="interference has void unit"
witness law-rotation-linear invariant=rotation-linear claim="rotation distributes over interference"
witness law-corefold-linear invariant=corefold-morphism claim="core-fold preserves superposition"
witness law-corefold-equivariant invariant=corefold-morphism claim="core-fold commutes with rotation"
witness law-corefold-nonidempotent invariant=corefold-morphism claim="core-fold abstracts strictly"
witness law-preservation-random invariant=preservation claim="commit barrier holds negative debt"
witness law-soundness-commit invariant=soundness claim="commit guard rejects potential increase"
witness law-soundness-flow-subset invariant=soundness claim="phase-balanced coupling reduces disagreement"
witness law-local-confluence invariant=local-confluence claim="disjoint commit footprints commute"
witness law-flow-additivity invariant=flow-additivity claim="split and combined durations agree"
witness law-trace-order invariant=trace-order-locality claim="smooth phase motion needs no global tick"
witness law-existence-spectrum invariant=existence-viability claim="passive through self-referential frames remain viable"
witness law-normalforms invariant=normalforms claim="localized closures are stable normal forms"
witness law-universal-closure invariant=universal-closure claim="one turn inverts the lifted sheet and two turns restore it"
witness law-durable-latch invariant=durable-latch-or-hold claim="a violated prefix balance holds the append and leaves the sealed bytes unchanged"
witness law-replay-under-compaction invariant=replay-identity claim="compaction preserves replay identity while the raw-bytes attest legitimately changes"

expect law gate-abelian
expect law interfere-monoid
expect law rotation-linear
expect law corefold-morphism
expect law balanced-ternary-carrier
expect law dyadic-triadic-closure
expect law preservation
expect law soundness
expect law existence-viability
expect law local-confluence
expect law flow-additivity
expect law trace-order-locality
expect law normalforms
expect law universal-closure
expect law durable-latch-or-hold
expect law replay-identity
