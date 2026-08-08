# framework_loop.cdc -- the generic task loop as one continuous composition.
# One declared organism travels the whole loop: sense -> gate -> act ->
# integrate -> record -> consolidate -> recall -> decide -> enact -> refine,
# and then begins the next cycle. The `run` and `interpret` modes execute two
# full sense/act/integrate cycles over one shared state object -- the second
# cycle's expectations (theta 0.492228, belief 1.333333) are only reachable
# through state carried forward from the first, so the composition is
# executed, not merely described. The `surface`, `council`, and `evolve`
# modes exercise gate, record, recall, refine, key, decide, and enact over
# the same declared source. The decision coordinate equals the recorded
# episode key (110101 -> 311): what the organism records is what it decides.

capability H5 label="task-loop-composition"
capability U1 label="universal-operator"
capability U2 label="variational-universal-operator"
framework H5 label=loop requires=sense,act,integrate,recur-sense,recur-act,recur-integrate,gate,record,recall,refine,key,cycle-count,proceduralize,skilled-execution,decide,enact,close permits=flow,commit,nest,guard,trace,measure,policy,bridge,counter,compile,interpret,council,evolve,universal

field loop-field dt=0.125 gain=1.0 deadband=0.5

module agent field=loop-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell agent.a module=agent theta=0.0 amplitude=1.0 omega=0.0
cell agent.b module=agent theta=0.0 amplitude=1.0 omega=0.0
cell agent.c module=agent theta=1.5707963267948966 amplitude=1.0 omega=0.0

module context field=loop-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell context.a module=context theta=0.0 amplitude=1.0 omega=0.0
cell context.b module=context theta=1.5707963267948966 amplitude=1.0 omega=0.0
cell context.c module=context theta=3.141592653589793 amplitude=1.0 omega=0.0

# Lifted double-cover frame: projected phase modulo 2*pi, winding count, and
# Z2 sheet live here, in a separate field so the six-cell record is untouched.
field loop-cover-field dt=0.125 gain=0.0 deadband=0.5
module loop-cover field=loop-cover-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell loop-cover.phase module=loop-cover theta=0.0 amplitude=1.0 omega=6.283185307179586

channel agent.c -> agent.b weight=0.25 delay=0.0 angle=0.0 lines=1

# Reciprocal causal cones: simultaneously receptive and radiant angularly
# biased channels between frame and context, active in the same flow
# evaluation. Oriented loop holonomy is 0.375 + (-0.25) = 0.125 radians.
channel context.b -> agent.a id=loop-receptive cone=receptive pair=loop-cone weight=0.01 delay=0.0 angle=0.375 lines=1
channel agent.a -> context.b id=loop-radiant cone=radiant pair=loop-cone weight=0.01 delay=0.0 angle=-0.25 lines=4

# Cycle 1: sense, act, integrate, then lift the cover frame through one turn.
flow loop-sense-1 field=loop-field duration=1.0 expect-theta=agent.b:0.25 tolerance=0.000001
commit loop-act-1 module=agent expect-trits=++0 expect-balance=admissible expect-status=accepted expect-reason=none
nest loop-integrate-1 parent=context child=agent expect-parent-belief=0.666667 expect-child-prior=0.666667 tolerance=0.000001
flow loop-turn-half field=loop-cover-field duration=1.0 expect-theta=loop-cover.phase:6.283185 tolerance=0.000001

# Cycle 2: the same organism senses again from its moved phase, its context
# belief accumulates, and the cover frame completes the second turn -- carried
# state is what these expectations check.
flow loop-sense-2 field=loop-field duration=1.0 expect-theta=agent.b:0.492228 tolerance=0.000001
commit loop-act-2 module=agent expect-trits=++0 expect-balance=admissible expect-status=accepted expect-reason=none
nest loop-integrate-2 parent=context child=agent expect-parent-belief=1.333333 expect-child-prior=1.333333 tolerance=0.000001
flow loop-turn-full field=loop-cover-field duration=1.0 expect-theta=loop-cover.phase:12.566371 tolerance=0.000001

# Gate, record, recall, refine, key, and cycle count over the same organism.
guard loop-gate cell=agent.c expect-state=open
trace loop-record field=loop-field expect-trits=++0+0- expect-events=4
measure loop-recall observer=context target=agent mode=commit expect-outcome=++0 expect-potential=nonincrease
policy loop-refine window=loop-record sampling=local commit=guarded adapt=recursive expect-sampling=local expect-commit=guarded expect-adapt=recursive
bridge loop-key trace=loop-record via=bridge64 expect-dyadic=110101 expect-triadic=311
counter loop-cycle value=0 increment=2 decrement=0 expect-value=2

# The whole organism proceduralizes: its declarative source compiles to
# reducer IR and the interpreter re-executes both cycles as the skilled path.
compile loop-ir source=framework_loop.cdc expect-ops=8 expect-flow=4 expect-commit=2 expect-nest=2
interpret loop-ir-exec source=framework_loop.cdc expect-ops=8 expect-flow=4 expect-commit=2 expect-nest=2

# Decide and enact: the organism deliberates over its own modules, adopts the
# coordinate its trace records, and writes that decision into source memory.
council loop-council field=loop-field members=agent,context quorum=4 expect-decision=adopt expect-dyadic=110101 expect-triadic=311
deliberate loop-quorum council=loop-council
evolve loop-enact source=framework_loop.cdc output=build/enacted_loop.cdc coordinate=110101 append-witness=loop-decision-memory expect-contains=loop-decision-memory

# Universal Operator: the guarded, scale-relative closure of the whole loop.
# bidi-gamma-delta stays the open transport; U closes it -- reciprocal
# receptive/radiant cones, internal BiDi reductions, a lifted frame that
# closes only after 720 degrees with declared holonomy, and enactment of the
# runtime-computed record.
universal loop-u720 frame=agent cover-cell=loop-cover.phase cover=double half-step=loop-turn-half full-step=loop-turn-full receptive=loop-receptive radiant=loop-radiant record=loop-key decision=loop-quorum enact=loop-enact tolerance=0.000001 expect-half-projection=returned expect-half-sheet=inverted expect-full-projection=returned expect-full-sheet=restored expect-holonomy=0.125 expect-coordinate=110101 expect-status=accepted expect-reason=none

# Variational Universal Operator: differentiate the exact accepted U1 path, then
# ask the full executable state to return. The lifted cover closes, but the
# latches, context belief, and child prior do not. The correct public result is
# therefore a bound path tangent plus a typed recurrence hold. No monodromy or
# characteristic multipliers are authorized for this canonical loop.
orbit loop-u720-full universal=loop-u720 coordinates=all quotient=none tolerance=0.000001
variational loop-u720-tangent orbit=loop-u720-full
spectrum loop-u720-spectrum variational=loop-u720-tangent neutral-tolerance=0.000000001 schur-tolerance=0.0000000001

witness loop-sense-native invariant=flow-additivity capability=H5 framework=loop role=sense reducer=loop-sense-1 claim="the loop senses through continuous flow before any commitment"
witness loop-act-native invariant=preservation capability=H5 framework=loop role=act reducer=loop-act-1 claim="the loop acts through an accepted balanced-ternary commit"
witness loop-integrate-native invariant=existence-viability capability=H5 framework=loop role=integrate reducer=loop-integrate-1 claim="acted coherence integrates into nested context belief"
witness loop-recur-sense-native invariant=flow-additivity capability=H5 framework=loop role=recur-sense reducer=loop-sense-2 claim="the second sense phase starts from the phase the first cycle produced"
witness loop-recur-act-native invariant=preservation capability=H5 framework=loop role=recur-act reducer=loop-act-2 claim="the second act commits from carried continuous state"
witness loop-recur-integrate-native invariant=existence-viability capability=H5 framework=loop role=recur-integrate reducer=loop-integrate-2 claim="context belief accumulates across cycles instead of resetting"
witness loop-gate-native capability=H5 framework=loop role=gate guard=loop-gate claim="the loop gates action on an open crossing state"
witness loop-record-native invariant=trace-order-locality capability=H5 framework=loop role=record trace=loop-record claim="the loop records its configuration as a window-local ternary trace"
witness loop-recall-native invariant=soundness capability=H5 framework=loop role=recall measure=loop-recall claim="the loop recalls its agent state as a committing measurement"
witness loop-refine-native invariant=trace-order-locality capability=H5 framework=loop role=refine policy=loop-refine claim="the loop refines through recursive window policy"
witness loop-key-native invariant=dyadic-triadic-closure capability=H5 framework=loop role=key bridge=loop-key claim="the recorded configuration projects to one bridge64 coordinate"
witness loop-cycle-native capability=H5 framework=loop role=cycle-count counter=loop-cycle claim="cycle count is a local counter across both executed cycles"
witness loop-compile-native capability=H5 framework=loop role=proceduralize compile=loop-ir claim="the whole loop proceduralizes into reducer IR"
witness loop-interpret-native capability=H5 framework=loop role=skilled-execution interpret=loop-ir-exec claim="the compiled loop IR re-executes both cycles through the interpreter"
witness loop-decide-native invariant=dyadic-triadic-closure capability=H5 framework=loop role=decide council=loop-quorum claim="the loop deliberates over its own modules and adopts its recorded coordinate"
witness loop-enact-native capability=H5 framework=loop role=enact evolution=loop-enact claim="the adopted decision is enacted into durable source memory"
witness loop-universal-native invariant=universal-closure capability=U1 framework=loop role=close universal=loop-u720 claim="the carried task loop closes its lifted frame after two turns and enacts its runtime-computed record"
witness loop-u2-native capability=U2 spectrum=loop-u720-spectrum claim="the accepted U1 path has a source-bound tangent; full-state recurrence is held before monodromy because latches, belief, prior, and lifted phase do not return"

expect capability H5
expect capability U1
expect capability U2
expect framework H5 complete
expect reducer loop-sense-native
expect reducer loop-act-native
expect reducer loop-integrate-native
expect reducer loop-recur-sense-native
expect reducer loop-recur-act-native
expect reducer loop-recur-integrate-native
expect guard loop-gate-native
expect trace loop-record-native
expect measure loop-recall-native
expect policy loop-refine-native
expect bridge loop-key-native
expect counter loop-cycle-native
expect compile loop-compile-native
expect interpret loop-interpret-native
expect council loop-decide-native
expect evolution loop-enact-native
expect universal loop-universal-native
expect spectrum loop-u2-native
