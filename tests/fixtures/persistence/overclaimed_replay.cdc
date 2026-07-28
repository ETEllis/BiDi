# Counterexample: compaction claiming it changed the replay identity.
# Compaction rewrites layout, never history, so this must fail closed.
field f dt=0.125 gain=1.0 deadband=0.5
module admissible field=f belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell admissible.a module=admissible theta=1.5707963267948966 amplitude=1.0 omega=0.0
cell admissible.b module=admissible theta=0.0 amplitude=1.0 omega=0.0
cell admissible.c module=admissible theta=3.141592653589793 amplitude=1.0 omega=0.0
store over dir=build/persistence-overclaim-replay mode=fresh
persist over-latch store=over op=append module=admissible expect-status=accepted expect-sealed=1
persist over-snapshot store=over op=snapshot expect-status=accepted
persist over-compact store=over op=compact expect-status=accepted expect-durable=yes expect-replay=changed
