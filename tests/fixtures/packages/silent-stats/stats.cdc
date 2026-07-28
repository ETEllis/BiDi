# silent-stats -- counterexample package: executable jobs but ZERO
# expectations. Zero executed checks carry no evidence, so installing this
# must be refused rather than quietly accepted.
field ss-field dt=0.125 gain=1.0 deadband=0.5
module ss field=ss-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell ss.a module=ss theta=0.0 amplitude=1.0 omega=0.0
flow ss-flow field=ss-field duration=0.125
commit ss-commit module=ss
nest ss-nest parent=ss child=ss
