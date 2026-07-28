# ternary-stats -- demo package: a balanced-ternary triad that carries its
# own evidence. Trits 0+- walk prefix balances 0, 1, 0 (admissible), the
# latched mean is equilibrium, and the witness + expectations are what make
# the package installable: a package with no checks is refused.
field ts-field dt=0.125 gain=1.0 deadband=0.5
module ts field=ts-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell ts.a module=ts theta=1.5707963267948966 amplitude=1.0 omega=0.0
cell ts.b module=ts theta=0.0 amplitude=1.0 omega=0.0
cell ts.c module=ts theta=3.141592653589793 amplitude=1.0 omega=0.0
flow ts-flow field=ts-field duration=0.125
commit ts-commit module=ts expect-trits=0+- expect-balance=admissible expect-status=accepted expect-reason=none
nest ts-nest parent=ts child=ts expect-parent-belief=0.0 tolerance=0.000001
witness ts-mean reducer=ts-commit claim="balanced trits sum to equilibrium"
expect witness ts-mean
