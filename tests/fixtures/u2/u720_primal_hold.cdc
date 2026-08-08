# Negative native U2 fixture: the declared receptive/radiant pair does not
# reverse endpoints. U1 therefore holds before it creates an admitted primal
# snapshot. Stability must emit a typed primal hold with no tangent, endpoint
# states, monodromy, spectrum, or evolve effect.

field frame-field dt=0.125 gain=0.0 deadband=0.5
module agent field=frame-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell agent.a module=agent theta=0.0 amplitude=1.0 omega=0.0
cell agent.b module=agent theta=1.5707963267948966 amplitude=1.0 omega=0.0

field cover-field dt=0.125 gain=0.0 deadband=0.5
module cover field=cover-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell cover.phase module=cover theta=0.0 amplitude=1.0 omega=6.283185307179586

channel agent.a -> agent.b id=bad-receptive cone=receptive pair=bad-pair weight=0.0 delay=0.0 angle=0.375 lines=1
channel agent.a -> agent.b id=bad-radiant cone=radiant pair=bad-pair weight=0.0 delay=0.0 angle=-0.25 lines=1

flow frame-still field=frame-field duration=1.0
flow cover-half field=cover-field duration=1.0
flow cover-full field=cover-field duration=1.0

trace primal-hold-trace field=frame-field
bridge primal-hold-record trace=primal-hold-trace via=bridge64
council primal-hold-council field=frame-field members=agent quorum=1
deliberate primal-hold-decision council=primal-hold-council
evolve primal-hold-enact source=tests/fixtures/u2/u720_primal_hold.cdc output=build/u2/enacted_primal_hold.cdc coordinate=0 append-witness=must-not-appear

universal primal-hold-u1 frame=agent cover-cell=cover.phase cover=double half-step=cover-half full-step=cover-full receptive=bad-receptive radiant=bad-radiant record=primal-hold-record decision=primal-hold-decision enact=primal-hold-enact tolerance=0.000001 expect-status=held expect-reason=cone-not-reciprocal

orbit primal-hold-orbit universal=primal-hold-u1 coordinates=all quotient=none tolerance=0.000001
variational primal-hold-tangent orbit=primal-hold-orbit
spectrum primal-hold-spectrum variational=primal-hold-tangent neutral-tolerance=0.000000001 schur-tolerance=0.0000000001
