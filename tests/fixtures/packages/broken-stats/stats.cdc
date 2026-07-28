# broken-stats -- counterexample package: its contract names a witness that
# does not exist, so installation must HOLD and write nothing.
field bs-field dt=0.125 gain=1.0 deadband=0.5
module bs field=bs-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell bs.a module=bs theta=0.0 amplitude=1.0 omega=0.0
flow bs-flow field=bs-field duration=0.125
commit bs-commit module=bs expect-status=accepted
nest bs-nest parent=bs child=bs
expect witness missing-witness
