# framework_persistence.cdc -- BiDi-gated durable persistence framework.
# Binding: a store is a declared durable log the way a field is a declared
# continuum; a persistence job is a job over that log the way a commit is a
# job over a module. The load-bearing claim is that durable mutation is NOT
# a host service call reachable from source -- an `op=append` runs the
# IDENTICAL balanced-ternary barrier that governs in-memory latching, and
# only an accepted decision is permitted to reach the log. A violated
# prefix balance holds: nothing is staged, nothing is committed, and the
# sealed bytes are observably unchanged. Equilibrium is not a failed write;
# it is the absence of one.
#
# Every job records two independent identities. `replay` is the semantic
# state folded from the sealed history; `durable` is whether the raw sealed
# bytes changed. They agree everywhere except under compaction, which is
# exactly the point of separating them.

capability H6 label="persistence-framework"
framework H6 label=persistence requires=declare,recover,gate,hold,replay,attest,snapshot,uncovered,compact,fence,contend permits=store,persist

field ledger-field dt=0.125 gain=1.0 deadband=0.5

# An admissible module: trits 0+- walk prefix balances 0, 1, 0 -- never
# negative, so the barrier accepts and the append may become durable.
module ledger field=ledger-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell ledger.a module=ledger theta=1.5707963267948966 amplitude=1.0 omega=0.0
cell ledger.b module=ledger theta=0.0 amplitude=1.0 omega=0.0
cell ledger.c module=ledger theta=3.141592653589793 amplitude=1.0 omega=0.0

# A violating module: trits -+0 open at balance -1. The barrier holds, so
# no byte of this module's state may ever reach the log.
module arrears field=ledger-field belief=0.0 prior=0.0 precision=1.0 action-gain=1.0
cell arrears.a module=arrears theta=3.141592653589793 amplitude=1.0 omega=0.0
cell arrears.b module=arrears theta=0.0 amplitude=1.0 omega=0.0
cell arrears.c module=arrears theta=1.5707963267948966 amplitude=1.0 omega=0.0

# The protocol store. mode=fresh removes exactly this store's own two
# artifacts before opening, so the declared expectations below are absolute
# rather than relative to whatever a previous run left behind.
store journal dir=build/persistence-journal mode=fresh

# A second directory with TWO declared handles onto it: `contended` is the
# writer that fences, `rival` is the writer that moves the log underneath
# it. Both handles live in ONE process, so the refusal below comes from the
# fence token rather than from the interprocess lock -- the token is
# compared against on-disk state under that same lock, so the outcome is
# identical. The genuinely concurrent, two-process version of this race is
# the `store-race` suite in scripts/verify.sh.
store contended dir=build/persistence-contended mode=fresh
store rival dir=build/persistence-contended mode=open

# Recover: a freshly opened store verifies structurally at zero.
persist journal-recover store=journal op=verify expect-status=accepted expect-sealed=0 expect-events=0 expect-durable=no expect-replay=stable

# Gate: an admissible barrier decision is the ONLY thing that makes bytes
# durable. Sealed history advances and the replay identity moves with it.
persist journal-latch store=journal op=append module=ledger expect-trits=0+- expect-balance=admissible expect-status=accepted expect-reason=none expect-sealed=1 expect-events=1 expect-durable=yes expect-replay=changed

# Hold: a violated barrier writes nothing. Same store, same op, same
# instant -- only the trits differ, and the log does not move.
persist journal-hold store=journal op=append module=arrears expect-trits=-+0 expect-balance=violated expect-status=held expect-reason=balance-violation expect-sealed=1 expect-events=1 expect-durable=no expect-replay=stable

# Replay and attest are reads of the sealed prefix; neither may mutate.
persist journal-replay store=journal op=replay expect-status=accepted expect-sealed=1 expect-events=1 expect-durable=no expect-replay=stable
persist journal-attest store=journal op=attest expect-status=accepted expect-sealed=1 expect-events=1 expect-durable=no expect-replay=stable

# Uncovered: compaction with no base to compact against holds rather than
# discarding a record nothing accounts for.
persist journal-compact-early store=journal op=compact expect-status=held expect-reason=compact-uncovered expect-sealed=1 expect-events=1 expect-durable=no expect-replay=stable

# Snapshot writes the base beside the log; the log itself is untouched.
persist journal-snapshot store=journal op=snapshot expect-status=accepted expect-sealed=1 expect-events=1 expect-durable=no expect-replay=stable

# Compact: the sealed BYTES change while the replay identity does not.
# That divergence is the proof that replay reads history, not layout.
persist journal-compact store=journal op=compact expect-status=accepted expect-sealed=1 expect-events=1 expect-durable=yes expect-replay=stable

# Fence: `contended` arms a compare-and-set at the sealed count it observed.
persist contended-fence store=contended op=fence seal=0 expect-status=accepted expect-sealed=0 expect-events=0 expect-durable=no expect-replay=stable

# `rival` then commits to the same directory, moving the log.
persist rival-latch store=rival op=append module=ledger expect-trits=0+- expect-balance=admissible expect-status=accepted expect-sealed=1 expect-events=1 expect-durable=yes expect-replay=changed

# Contend: `contended` now attempts an append whose barrier decision is
# perfectly admissible. It still writes nothing, because its view is stale.
# The reported sealed=0 IS that stale view -- which is precisely why the
# compare-and-set refused before a byte was written. A stale handle is
# SPENT after this: its cached counters no longer describe the log, so
# cdc_store.h requires it to be reopened rather than reused, and no further
# job writes through it here.
persist contended-stale store=contended op=append module=ledger expect-trits=0+- expect-balance=admissible expect-status=held expect-reason=fence-violation expect-sealed=0 expect-events=0 expect-durable=no expect-replay=stable

witness persistence-declare-native capability=H6 framework=persistence role=declare store=journal claim="a durable store is a declared term, not a host service the source reaches around the language to call"
witness persistence-recover-native capability=H6 framework=persistence role=recover persistence=journal-recover claim="a freshly opened store verifies structurally at zero sealed transactions"
witness persistence-gate-native capability=H6 framework=persistence role=gate persistence=journal-latch claim="durable mutation happens only under an accepted balanced-ternary barrier decision"
witness persistence-hold-native capability=H6 framework=persistence role=hold persistence=journal-hold claim="an append whose prefix balance is violated holds and leaves the sealed bytes unchanged"
witness persistence-replay-native capability=H6 framework=persistence role=replay persistence=journal-replay claim="replay folds the sealed history into a deterministic state identity without mutating the log"
witness persistence-attest-native capability=H6 framework=persistence role=attest persistence=journal-attest claim="attest digests the raw sealed bytes as evidence identity without mutating the log"
witness persistence-snapshot-native capability=H6 framework=persistence role=snapshot persistence=journal-snapshot claim="a snapshot records the compaction base beside the log and leaves the log itself untouched"
witness persistence-uncovered-native capability=H6 framework=persistence role=uncovered persistence=journal-compact-early claim="compaction with no covering base holds rather than discarding unaccounted records"
witness persistence-compact-native capability=H6 framework=persistence role=compact persistence=journal-compact claim="compaction preserves replay identity while the raw-bytes attest legitimately changes"
witness persistence-fence-native capability=H6 framework=persistence role=fence persistence=contended-fence claim="a writer arms a compare-and-set at the sealed count it observed"
witness persistence-contend-native capability=H6 framework=persistence role=contend persistence=contended-stale claim="an admissible append from a stale writer is refused by the fence before any byte is written"

expect capability H6
expect framework H6 complete
expect store persistence-declare-native
expect persistence persistence-recover-native
expect persistence persistence-gate-native
expect persistence persistence-hold-native
expect persistence persistence-replay-native
expect persistence persistence-attest-native
expect persistence persistence-snapshot-native
expect persistence persistence-uncovered-native
expect persistence persistence-compact-native
expect persistence persistence-fence-native
expect persistence persistence-contend-native
