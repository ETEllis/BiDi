# Positive native U2 fixture.
#
# The six-cell frame, module belief/prior state, and discrete mode are fixed.
# Only the isolated lifted cover coordinate advances by exactly 4*pi.  The
# declared phase quotient is therefore an executable endpoint translation
# rho(theta_cover) = theta_cover - 4*pi with D(rho)=I.  This is a true relative
# recurrence, not an include-mask projection.  Its identity return tangent has
# a real Schur spectrum of unit multipliers and is marginal when no symmetry
# generator is declared for removal.

capability H5 label="task-loop-composition"
capability U1 label="universal-operator"
capability U2 label="variational-universal-operator"

field frame-field dt=0.125 gain=1.0 deadband=0.5
module agent field=frame-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell agent.a module=agent theta=0.0 amplitude=1.0 omega=0.0
cell agent.b module=agent theta=0.0 amplitude=1.0 omega=0.0
cell agent.c module=agent theta=1.5707963267948966 amplitude=1.0 omega=0.0

module context field=frame-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell context.a module=context theta=0.0 amplitude=1.0 omega=0.0
cell context.b module=context theta=1.5707963267948966 amplitude=1.0 omega=0.0
cell context.c module=context theta=3.141592653589793 amplitude=1.0 omega=0.0

field cover-field dt=0.125 gain=0.0 deadband=0.5
module cover field=cover-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell cover.phase module=cover theta=0.0 amplitude=1.0 omega=6.283185307179586

# Deliberately unrelated declaration island.  No Universal step, record,
# council, cone, or cover edge reaches it.  A source-wide manifest would add
# three inert coordinates and spurious unit multipliers; the executable-path
# closure must leave the accepted dimension and spectrum unchanged.
field dead-field dt=0.125 gain=0.0 deadband=0.5
module dead-module field=dead-field belief=91.0 prior=-37.0 precision=1.0 action-gain=1.0
cell dead.phase module=dead-module theta=2.25 amplitude=1.0 omega=99.0

# Reciprocal roles and nonzero angles remain real U1 constraints.  Zero weight
# makes this calibration fixture stationary without erasing oriented holonomy.
channel context.b -> agent.a id=fixture-receptive cone=receptive pair=fixture-cone weight=0.0 delay=0.0 angle=0.375 lines=1
channel agent.a -> context.b id=fixture-radiant cone=radiant pair=fixture-cone weight=0.0 delay=0.0 angle=-0.25 lines=4

flow frame-still field=frame-field duration=1.0 expect-theta=agent.a:0.0 tolerance=0.000001
flow cover-half field=cover-field duration=1.0 expect-theta=cover.phase:6.283185 tolerance=0.000001
flow cover-full field=cover-field duration=1.0 expect-theta=cover.phase:12.566371 tolerance=0.000001

trace fixture-record field=frame-field expect-trits=++0+0- expect-events=4
bridge fixture-key trace=fixture-record via=bridge64 expect-dyadic=110101 expect-triadic=311
council fixture-council field=frame-field members=agent,context quorum=4 expect-decision=adopt expect-dyadic=110101 expect-triadic=311
deliberate fixture-quorum council=fixture-council
evolve fixture-enact source=tests/fixtures/u2/u720_true_relative_marginal.cdc output=build/u2/enacted_true_relative.cdc coordinate=110101 append-witness=fixture-decision-memory expect-contains=fixture-decision-memory

universal fixture-u720 frame=agent cover-cell=cover.phase cover=double half-step=cover-half full-step=cover-full receptive=fixture-receptive radiant=fixture-radiant record=fixture-key decision=fixture-quorum enact=fixture-enact tolerance=0.000001 expect-half-projection=returned expect-half-sheet=inverted expect-full-projection=returned expect-full-sheet=restored expect-holonomy=0.125 expect-coordinate=110101 expect-status=accepted expect-reason=none

orbit fixture-relative-orbit universal=fixture-u720 coordinates=all quotient=phase tolerance=0.000001
variational fixture-relative-tangent orbit=fixture-relative-orbit
spectrum fixture-relative-spectrum variational=fixture-relative-tangent neutral-tolerance=0.000000001 schur-tolerance=0.0000000001
