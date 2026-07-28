# Counterexample: a violated barrier claiming it made bytes durable. The
# runtime observes durability rather than trusting the declaration, so this
# must fail closed.
field f dt=0.125 gain=1.0 deadband=0.5
module violating field=f belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell violating.a module=violating theta=3.141592653589793 amplitude=1.0 omega=0.0
cell violating.b module=violating theta=0.0 amplitude=1.0 omega=0.0
cell violating.c module=violating theta=1.5707963267948966 amplitude=1.0 omega=0.0
store over dir=build/persistence-overclaim mode=fresh
persist over-claim store=over op=append module=violating expect-status=held expect-reason=balance-violation expect-durable=yes
