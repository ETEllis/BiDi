# rftc.cdc -- canonical C3 language surface for relational frame computation.
#
# These declarations name the executable boundary. They do not self-promote a
# classical distributed runtime to a quantum claim; that boundary remains
# explicit in docs/rftc/RFTC_FULL_BUILD_SPEC.md and the evidence gates.

capability R1 label="reference-frame-snapshot"
capability R2 label="immutable-frame-reduction"
capability R3 label="oriented-logical-complex"
capability R4 label="topological-sector-guard"
capability R5 label="scoped-distributed-authority"
capability R6 label="authenticated-causal-transport"

frame cortical-column scale=mesoscopic members=node/* window=50ms clock=hybrid-logical reducer=rftc-v1
reduce cortical-column outputs=R,Psi,dispersion,winding,classDigest,freshness minimum-members=16 stale-after=150ms
complex column-ring frame=cortical-column orientation=clockwise adjacency=ring boundary=closed
topology phase-sector complex=column-ring invariant=winding allowed=local-continuous transition=phase-slip
authority cell-controller frame=cortical-column permits=observe,propose enact=quorum:3/5 expiry-policy=required nonce-policy=unique revocation=fail-closed
transport lab-mesh protocol=authenticated-stream schema=rftc-wire-v1 replay=rftc-replay-v1 ordering=causal partition=hold

witness R1-frame-form capability=R1 claim="grammar-1 carries a versioned frame snapshot declaration"
witness R2-reduce-form capability=R2 claim="grammar-1 carries immutable reduction and freshness bounds"
witness R3-complex-form capability=R3 claim="grammar-1 carries oriented adjacency and boundary ownership"
witness R4-topology-form capability=R4 claim="grammar-1 carries protected sectors and explicit transition events"
witness R5-authority-form capability=R5 claim="grammar-1 carries scoped quorum authority with expiry nonce and revocation policy"
witness R6-transport-form capability=R6 claim="grammar-1 carries authenticated causal transport with partition hold"

expect capability R1
expect capability R2
expect capability R3
expect capability R4
expect capability R5
expect capability R6
