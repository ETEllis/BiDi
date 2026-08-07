(*
  U2 finite algebraic obligations.

  These lemmas establish only identity, associative composition, observable
  event order, and the finite involutions used by the scoped polarity witness.
  They do not prove a runtime Jacobian correct, establish a periodic orbit,
  certify a numerical multiplier, or assert a physical polarity law.
*)
From Stdlib Require Import Bool.Bool.
From Stdlib Require Import ZArith Lia.

Open Scope Z_scope.

Definition tangent_state : Type := (bool * bool)%type.
Definition tangent_map : Type := tangent_state -> tangent_state.

Definition tangent_identity : tangent_map := fun x => x.

Definition tangent_compose
    (after before : tangent_map) : tangent_map :=
  fun x => after (before x).

Theorem tangent_identity_left :
  forall (f : tangent_map) (x : tangent_state),
    tangent_compose tangent_identity f x = f x.
Proof. reflexivity. Qed.

Theorem tangent_identity_right :
  forall (f : tangent_map) (x : tangent_state),
    tangent_compose f tangent_identity x = f x.
Proof. reflexivity. Qed.

Theorem tangent_compose_assoc :
  forall (a b c : tangent_map) (x : tangent_state),
    tangent_compose a (tangent_compose b c) x =
    tangent_compose (tangent_compose a b) c x.
Proof. reflexivity. Qed.

(* `first` is applied before `second`, matching M = second * first for
   column perturbations.  The maps deliberately do not commute. *)
Definition first_event (x : tangent_state) : tangent_state :=
  let '(a, b) := x in (negb a, b).

Definition second_event (x : tangent_state) : tangent_state :=
  let '(a, b) := x in (b, a).

Definition ordered_two_event : tangent_map :=
  tangent_compose second_event first_event.

Definition reversed_two_event : tangent_map :=
  tangent_compose first_event second_event.

Theorem ordered_two_event_witness :
  ordered_two_event (false, false) = (false, true).
Proof. reflexivity. Qed.

Theorem reversed_two_event_witness :
  reversed_two_event (false, false) = (true, false).
Proof. reflexivity. Qed.

Theorem two_event_order_is_observable :
  ordered_two_event (false, false) <>
  reversed_two_event (false, false).
Proof. discriminate. Qed.

Inductive trit : Type :=
| trit_negative
| trit_aperture
| trit_positive.

Definition inv_carrier (t : trit) : trit :=
  match t with
  | trit_negative => trit_positive
  | trit_aperture => trit_aperture
  | trit_positive => trit_negative
  end.

Definition trit_value (t : trit) : Z :=
  match t with
  | trit_negative => -1
  | trit_aperture => 0
  | trit_positive => 1
  end.

Theorem carrier_involution :
  forall t : trit, inv_carrier (inv_carrier t) = t.
Proof. intros []; reflexivity. Qed.

Theorem aperture_is_fixed :
  inv_carrier trit_aperture = trit_aperture.
Proof. reflexivity. Qed.

Theorem carrier_value_reverses :
  forall t : trit, trit_value (inv_carrier t) = - trit_value t.
Proof. intros []; reflexivity. Qed.

Definition prefix_barrier_2 (first second : trit) : Prop :=
  0 <= trit_value first /\
  0 <= trit_value first + trit_value second.

Theorem positive_negative_is_admissible :
  prefix_barrier_2 trit_positive trit_negative.
Proof. unfold prefix_barrier_2; simpl; lia. Qed.

(* Naive pointwise sign inversion does not preserve the oriented prefix
   barrier; valid CDC can therefore break this scoped polarity symmetry. *)
Theorem negative_positive_is_held :
  ~ prefix_barrier_2 trit_negative trit_positive.
Proof. unfold prefix_barrier_2; simpl; lia. Qed.

Inductive cone_role : Type := receptive | radiant.

Definition reverse_cone (role : cone_role) : cone_role :=
  match role with
  | receptive => radiant
  | radiant => receptive
  end.

Theorem cone_reversal_involution :
  forall role : cone_role, reverse_cone (reverse_cone role) = role.
Proof. intros []; reflexivity. Qed.

(* Sheet parity is deliberately a different type from carrier polarity. *)
Inductive sheet : Type := base_sheet | inverted_sheet.

Definition flip_sheet (value : sheet) : sheet :=
  match value with
  | base_sheet => inverted_sheet
  | inverted_sheet => base_sheet
  end.

Theorem sheet_flip_involution :
  forall value : sheet, flip_sheet (flip_sheet value) = value.
Proof. intros []; reflexivity. Qed.
