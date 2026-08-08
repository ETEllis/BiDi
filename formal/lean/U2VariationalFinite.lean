/-
  U2 finite algebraic obligations.

  These lemmas establish only identity, associative composition, observable
  event order, and the finite involutions used by the scoped polarity witness.
  They do not prove a runtime Jacobian correct, establish a periodic orbit,
  certify a numerical multiplier, or assert a physical polarity law.
-/
namespace U2VariationalFinite

abbrev TangentState := Bool × Bool
abbrev TangentMap := TangentState → TangentState

def identity : TangentMap := fun x => x

def compose (after before : TangentMap) : TangentMap :=
  fun x => after (before x)

theorem identity_left (f : TangentMap) : compose identity f = f := by
  funext x
  rfl

theorem identity_right (f : TangentMap) : compose f identity = f := by
  funext x
  rfl

theorem compose_assoc (a b c : TangentMap) :
    compose a (compose b c) = compose (compose a b) c := by
  funext x
  rfl

/- Two deliberately non-commuting finite event maps.  `first` is applied
   before `second`, matching the executable convention
   M = second * first for column perturbations. -/
def first : TangentMap
  | (x, y) => (!x, y)

def second : TangentMap
  | (x, y) => (y, x)

def orderedTwoEvent : TangentMap := compose second first
def reversedTwoEvent : TangentMap := compose first second

theorem ordered_two_event_witness :
    orderedTwoEvent (false, false) = (false, true) := by
  rfl

theorem reversed_two_event_witness :
    reversedTwoEvent (false, false) = (true, false) := by
  rfl

theorem two_event_order_is_observable :
    orderedTwoEvent (false, false) ≠ reversedTwoEvent (false, false) := by
  decide

inductive Trit where
  | negative
  | aperture
  | positive
  deriving DecidableEq

def invCarrier : Trit → Trit
  | .negative => .positive
  | .aperture => .aperture
  | .positive => .negative

def tritValue : Trit → Int
  | .negative => -1
  | .aperture => 0
  | .positive => 1

theorem carrier_involution (t : Trit) :
    invCarrier (invCarrier t) = t := by
  cases t <;> rfl

theorem aperture_is_fixed : invCarrier .aperture = .aperture := by
  rfl

theorem carrier_value_reverses (t : Trit) :
    tritValue (invCarrier t) = -tritValue t := by
  cases t <;> rfl

def prefixBarrier2 (first second : Trit) : Prop :=
  0 ≤ tritValue first ∧ 0 ≤ tritValue first + tritValue second

theorem positive_negative_is_admissible :
    prefixBarrier2 .positive .negative := by
  simp [prefixBarrier2, tritValue]

/- Naive pointwise carrier inversion is not a symmetry of the oriented
   prefix barrier. This is a permanent counterexample against promoting a
   typed polarity witness into a global sign symmetry. -/
theorem negative_positive_is_held :
    ¬ prefixBarrier2 .negative .positive := by
  simp [prefixBarrier2, tritValue]

inductive ConeRole where
  | receptive
  | radiant
  deriving DecidableEq

def reverseCone : ConeRole → ConeRole
  | .receptive => .radiant
  | .radiant => .receptive

theorem cone_reversal_involution (role : ConeRole) :
    reverseCone (reverseCone role) = role := by
  cases role <;> rfl

/- Sheet parity is intentionally a distinct type from carrier polarity. -/
inductive Sheet where
  | base
  | inverted
  deriving DecidableEq

def flipSheet : Sheet → Sheet
  | .base => .inverted
  | .inverted => .base

theorem sheet_flip_involution (sheet : Sheet) :
    flipSheet (flipSheet sheet) = sheet := by
  cases sheet <;> rfl

end U2VariationalFinite
