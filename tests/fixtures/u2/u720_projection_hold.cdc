# U2 integration fixture over the real U1 execution semantics.
#
# The cover returns after 720 degrees, but the complete executable state does
# not: latches populate, parent belief accumulates, and child prior is
# overwritten.  The first orbit must therefore hold full recurrence.  The
# second intentionally selects only the cover phase; it is a projected tangent
# diagnostic and must not authorize monodromy/Floquet output.

capability H5 label="task-loop-composition"
capability U1 label="universal-operator"
capability U2 label="variational-universal-operator"

field loop-field dt=0.125 gain=1.0 deadband=0.5

module agent field=loop-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell agent.a module=agent theta=0.0 amplitude=1.0 omega=0.0
cell agent.b module=agent theta=0.0 amplitude=1.0 omega=0.0
cell agent.c module=agent theta=1.5707963267948966 amplitude=1.0 omega=0.0

module context field=loop-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell context.a module=context theta=0.0 amplitude=1.0 omega=0.0
cell context.b module=context theta=1.5707963267948966 amplitude=1.0 omega=0.0
cell context.c module=context theta=3.141592653589793 amplitude=1.0 omega=0.0

field loop-cover-field dt=0.125 gain=0.0 deadband=0.5
module loop-cover field=loop-cover-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell loop-cover.phase module=loop-cover theta=0.0 amplitude=1.0 omega=6.283185307179586

channel agent.c -> agent.b weight=0.25 delay=0.0 angle=0.0 lines=1
channel context.b -> agent.a id=loop-receptive cone=receptive pair=loop-cone weight=0.01 delay=0.0 angle=0.375 lines=1
channel agent.a -> context.b id=loop-radiant cone=radiant pair=loop-cone weight=0.01 delay=0.0 angle=-0.25 lines=4

flow loop-sense-1 field=loop-field duration=1.0 expect-theta=agent.b:0.25 tolerance=0.000001
commit loop-act-1 module=agent expect-trits=++0 expect-balance=admissible expect-status=accepted expect-reason=none
nest loop-integrate-1 parent=context child=agent expect-parent-belief=0.666667 expect-child-prior=0.666667 tolerance=0.000001
flow loop-turn-half field=loop-cover-field duration=1.0 expect-theta=loop-cover.phase:6.283185 tolerance=0.000001

flow loop-sense-2 field=loop-field duration=1.0 expect-theta=agent.b:0.492228 tolerance=0.000001
commit loop-act-2 module=agent expect-trits=++0 expect-balance=admissible expect-status=accepted expect-reason=none
nest loop-integrate-2 parent=context child=agent expect-parent-belief=1.333333 expect-child-prior=1.333333 tolerance=0.000001
flow loop-turn-full field=loop-cover-field duration=1.0 expect-theta=loop-cover.phase:12.566371 tolerance=0.000001

trace loop-record field=loop-field expect-trits=++0+0- expect-events=4
bridge loop-key trace=loop-record via=bridge64 expect-dyadic=110101 expect-triadic=311
council loop-council field=loop-field members=agent,context quorum=4 expect-decision=adopt expect-dyadic=110101 expect-triadic=311
deliberate loop-quorum council=loop-council
evolve loop-enact source=tests/fixtures/u2/u720_projection_hold.cdc output=build/u2/enacted_projection_hold.cdc coordinate=110101 append-witness=loop-decision-memory expect-contains=loop-decision-memory

universal loop-u720 frame=agent cover-cell=loop-cover.phase cover=double half-step=loop-turn-half full-step=loop-turn-full receptive=loop-receptive radiant=loop-radiant record=loop-key decision=loop-quorum enact=loop-enact tolerance=0.000001 expect-half-projection=returned expect-half-sheet=inverted expect-full-projection=returned expect-full-sheet=restored expect-holonomy=0.125 expect-coordinate=110101 expect-status=accepted expect-reason=none

orbit u720-full universal=loop-u720 coordinates=all quotient=none tolerance=0.000001
variational u720-full-tangent orbit=u720-full
spectrum u720-full-spectrum variational=u720-full-tangent

orbit u720-cover-projection universal=loop-u720 coordinates=loop-cover.phase quotient=none tolerance=0.000001
variational u720-cover-tangent orbit=u720-cover-projection
spectrum u720-cover-spectrum variational=u720-cover-tangent
