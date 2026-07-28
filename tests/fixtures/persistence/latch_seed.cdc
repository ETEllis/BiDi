# Seeds a store with exactly one accepted (admissible) durable append.
# Paired with hold_writes_nothing.cdc: verify.sh copies the sealed log
# after this file runs, replays the held case over the same directory, and
# byte-compares. The "a hold writes nothing" claim is then checked OUTSIDE
# the runtime that makes it.
field f dt=0.125 gain=1.0 deadband=0.5
module admissible field=f belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell admissible.a module=admissible theta=1.5707963267948966 amplitude=1.0 omega=0.0
cell admissible.b module=admissible theta=0.0 amplitude=1.0 omega=0.0
cell admissible.c module=admissible theta=3.141592653589793 amplitude=1.0 omega=0.0
store seed dir=build/persistence-bytes mode=fresh
persist seed-latch store=seed op=append module=admissible expect-trits=0+- expect-balance=admissible expect-status=accepted expect-sealed=1 expect-durable=yes
