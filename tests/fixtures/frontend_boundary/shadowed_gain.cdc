# End-to-end counterexample for the attribute-key boundary.
#
# `action-gain` ends with `gain`. Under the old substring match, the field's
# gain was read from action-gain, so nest integration used 9.0 instead of
# 1.0 and the parent belief came out nine times too large -- a wrong number,
# not a missing one. The expectations below are computed from gain=1.0, so
# a regression fails here rather than silently changing arithmetic.
field shadow-field action-gain=9.0 gain=1.0 dt=0.125 deadband=0.5

module shadow-parent field=shadow-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell shadow-parent.a module=shadow-parent theta=0.0 amplitude=1.0 omega=0.0

module shadow-child field=shadow-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell shadow-child.a module=shadow-child theta=0.0 amplitude=1.0 omega=0.0

flow shadow-flow field=shadow-field duration=0.125
commit shadow-commit module=shadow-parent expect-trits=+ expect-balance=admissible expect-status=accepted
# child mean trit is +1, so parent belief becomes 0.0 + gain * 1.0.
# With the correct gain that is 1.0; with the shadowed action-gain it is 9.0.
nest shadow-nest parent=shadow-parent child=shadow-child expect-parent-belief=1.0 tolerance=0.000001
