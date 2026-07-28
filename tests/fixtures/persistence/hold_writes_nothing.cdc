# Opens the store seeded by latch_seed.cdc and attempts three violating
# appends. Every one must hold; the sealed log must be byte-identical to
# the copy verify.sh took before this file ran.
field f dt=0.125 gain=1.0 deadband=0.5
module violating field=f belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell violating.a module=violating theta=3.141592653589793 amplitude=1.0 omega=0.0
cell violating.b module=violating theta=0.0 amplitude=1.0 omega=0.0
cell violating.c module=violating theta=1.5707963267948966 amplitude=1.0 omega=0.0
store seed dir=build/persistence-bytes mode=open
persist hold-one store=seed op=append module=violating expect-trits=-+0 expect-balance=violated expect-status=held expect-reason=balance-violation expect-sealed=1 expect-durable=no expect-replay=stable
persist hold-two store=seed op=append module=violating expect-status=held expect-reason=balance-violation expect-sealed=1 expect-durable=no expect-replay=stable
persist hold-three store=seed op=append module=violating expect-status=held expect-reason=balance-violation expect-sealed=1 expect-durable=no expect-replay=stable
