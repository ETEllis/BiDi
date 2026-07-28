# Counterexample for the A7 typed policy: a durable append that holds
# without declaring expect-status=held. The runtime exits 0 (nothing was
# violated), so only the typed gate can catch it.
field f dt=0.125 gain=1.0 deadband=0.5
module violating field=f belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell violating.a module=violating theta=3.141592653589793 amplitude=1.0 omega=0.0
cell violating.b module=violating theta=0.0 amplitude=1.0 omega=0.0
cell violating.c module=violating theta=1.5707963267948966 amplitude=1.0 omega=0.0
store quiet dir=build/persistence-quiet mode=fresh
persist quiet-append store=quiet op=append module=violating
