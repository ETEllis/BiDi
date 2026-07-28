# silent_hold.cdc -- CT3 negative fixture: the runtime accepts this source
# (no inline expectations fail), but the commit on the violating module
# holds without declaring expect-status=held. The typed cdc test gate must
# flag the hold as unexpected and fail, even though every run exits 0.
field f1 dt=0.125 gain=1.0 deadband=0.5

module m1 field=f1 belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell m1.a module=m1 theta=3.141592653589793 amplitude=1.0 omega=0.0
cell m1.b module=m1 theta=0.0 amplitude=1.0 omega=0.0
cell m1.c module=m1 theta=1.5707963267948966 amplitude=1.0 omega=0.0

module m2 field=f1 belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell m2.a module=m2 theta=1.5707963267948966 amplitude=1.0 omega=0.0
cell m2.b module=m2 theta=0.0 amplitude=1.0 omega=0.0
cell m2.c module=m2 theta=3.141592653589793 amplitude=1.0 omega=0.0

module m3 field=f1 belief=0.0 prior=0.0 precision=1.0 action-gain=1.0 parent=m2
cell m3.a module=m3 theta=0.0 amplitude=1.0 omega=0.0
cell m3.b module=m3 theta=0.0 amplitude=1.0 omega=0.0
cell m3.c module=m3 theta=0.0 amplitude=1.0 omega=0.0

flow f-orient field=f1 duration=1.0
commit good-commit module=m2
commit silent-hold module=m1
nest n-integrate parent=m2 child=m3
