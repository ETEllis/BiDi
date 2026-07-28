#include "../../runtime/cdc_authority.h"
#include "../../runtime/cdc_transport.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr)                                                            \
    do {                                                                       \
        if (!(expr)) {                                                         \
            fprintf(stderr, "control-plane check failed at %s:%d: %s\n",      \
                    __FILE__, __LINE__, #expr);                                \
            return 0;                                                          \
        }                                                                      \
    } while (0)

static const uint8_t KEY[CDC_TRANSPORT_TAG_SIZE] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
};

static void set_id(char *out, const char *value) {
    memcpy(out, value, strlen(value) + 1);
}

static void make_nonce(uint8_t nonce[CDC_TRANSPORT_NONCE_SIZE],
                       uint8_t value) {
    size_t i;
    for (i = 0; i < CDC_TRANSPORT_NONCE_SIZE; i++) {
        nonce[i] = (uint8_t)(value + i);
    }
}

static int make_envelope(cdc_transport_envelope *envelope, uint64_t sequence,
                         const uint8_t *parent, uint8_t nonce_value,
                         const char *payload) {
    cdc_transport_envelope_init(envelope);
    envelope->schema_version = 1;
    envelope->message_type = CDC_TRANSPORT_PROPOSAL;
    set_id(envelope->sender, "node-a");
    set_id(envelope->recipient, "supervisor");
    set_id(envelope->frame, "cortical-column");
    set_id(envelope->key_id, "lab-key-1");
    set_id(envelope->lease_id, "lease-1");
    envelope->authority_action = CDC_AUTH_PROPOSE;
    envelope->horizon = 12;
    envelope->sequence = sequence;
    envelope->logical_clock = 100 + sequence;
    make_nonce(envelope->nonce, nonce_value);
    if (parent) {
        memcpy(envelope->causal_parent, parent, CDC_TRANSPORT_TAG_SIZE);
    }
    if (!cdc_transport_envelope_set_payload(envelope, payload,
                                            strlen(payload))) {
        return 0;
    }
    return cdc_transport_envelope_sign(envelope, KEY);
}

static void request_from_envelope(cdc_authority_request *request,
                                  const cdc_transport_envelope *envelope,
                                  uint64_t now) {
    memset(request, 0, sizeof(*request));
    set_id(request->lease_id, envelope->lease_id);
    set_id(request->subject, envelope->sender);
    set_id(request->frame, envelope->frame);
    request->action = envelope->authority_action;
    request->horizon = envelope->horizon;
    request->now = now;
    request->approvals = 3;
    memcpy(request->nonce, envelope->nonce, CDC_AUTHORITY_NONCE_SIZE);
    memcpy(request->proposal_digest, envelope->payload_digest,
           sizeof(request->proposal_digest));
}

static int admit(cdc_transport_peer *peer, cdc_authority_guard *guard,
                 cdc_transport_envelope *envelope, uint64_t now,
                 unsigned *mutations) {
    cdc_transport_ticket transport_ticket;
    cdc_authority_ticket authority_ticket;
    cdc_authority_request request;
    CHECK(cdc_transport_inspect(peer, envelope, KEY, &transport_ticket) ==
          CDC_TRANSPORT_ACCEPT);
    request_from_envelope(&request, envelope, now);
    CHECK(cdc_authority_check(guard, &request, &authority_ticket) ==
          CDC_AUTHORITY_ACCEPT);
    CHECK(cdc_authority_consume(guard, &authority_ticket, now) ==
          CDC_AUTHORITY_ACCEPT);
    CHECK(cdc_transport_accept(peer, &transport_ticket) ==
          CDC_TRANSPORT_ACCEPT);
    (*mutations)++;
    return 1;
}

static int test_control_plane(void) {
    cdc_transport_peer peer;
    cdc_authority_guard guard;
    cdc_authority_lease lease;
    cdc_transport_envelope first, second, gap, wrong_parent,
        regressed_clock, replay_nonce, fresh_third;
    cdc_transport_ticket transport_ticket;
    cdc_authority_ticket authority_ticket;
    cdc_authority_request request, variant;
    uint8_t wrong_key[CDC_TRANSPORT_TAG_SIZE] = {0};
    uint8_t zero_parent[CDC_TRANSPORT_TAG_SIZE] = {0};
    unsigned mutations = 0;

    CHECK(cdc_transport_peer_init(&peer, 1, "supervisor", "lab-key-1", 4096,
                                  8));
    CHECK(cdc_authority_guard_init(&guard, 8, 32));
    memset(&lease, 0, sizeof(lease));
    lease.version = 1;
    set_id(lease.lease_id, "lease-1");
    set_id(lease.subject, "node-a");
    set_id(lease.frame, "cortical-column");
    lease.actions = CDC_AUTH_OBSERVE | CDC_AUTH_PROPOSE;
    lease.horizon_start = 10;
    lease.horizon_end = 20;
    lease.not_before = 1000;
    lease.expires_at = 2000;
    lease.quorum_required = 3;
    lease.quorum_total = 5;
    CHECK(cdc_authority_add(&guard, &lease) == CDC_AUTHORITY_ACCEPT);

    CHECK(make_envelope(&first, 1, NULL, 1, "proposal-one"));

    /* Authentication and partition failures never create stream state. */
    set_id(first.recipient, "wrong-supervisor");
    CHECK(cdc_transport_envelope_sign(&first, KEY));
    CHECK(cdc_transport_inspect(&peer, &first, KEY, &transport_ticket) ==
          CDC_TRANSPORT_REJECT_RECIPIENT);
    set_id(first.recipient, "supervisor");
    CHECK(cdc_transport_envelope_sign(&first, KEY));
    first.payload[0] ^= 1;
    CHECK(cdc_transport_inspect(&peer, &first, KEY, &transport_ticket) ==
          CDC_TRANSPORT_REJECT_PAYLOAD);
    first.payload[0] ^= 1;
    first.mac[0] ^= 1;
    CHECK(cdc_transport_inspect(&peer, &first, KEY, &transport_ticket) ==
          CDC_TRANSPORT_REJECT_AUTH);
    first.mac[0] ^= 1;
    CHECK(cdc_transport_inspect(&peer, &first, wrong_key, &transport_ticket) ==
          CDC_TRANSPORT_REJECT_AUTH);
    cdc_transport_peer_set_partitioned(&peer, 1);
    CHECK(cdc_transport_inspect(&peer, &first, KEY, &transport_ticket) ==
          CDC_TRANSPORT_HOLD_PARTITION);
    cdc_transport_peer_set_partitioned(&peer, 0);
    CHECK(peer.stream_count == 0 && mutations == 0);

    CHECK(admit(&peer, &guard, &first, 1100, &mutations));
    CHECK(peer.stream_count == 1 && peer.streams[0].last_sequence == 1);
    CHECK(mutations == 1);
    CHECK(cdc_transport_inspect(&peer, &first, KEY, &transport_ticket) ==
          CDC_TRANSPORT_HOLD_DUPLICATE);
    CHECK(mutations == 1);

    CHECK(make_envelope(&gap, 3, first.envelope_digest, 3, "gap"));
    CHECK(cdc_transport_inspect(&peer, &gap, KEY, &transport_ticket) ==
          CDC_TRANSPORT_HOLD_CAUSAL_GAP);
    CHECK(make_envelope(&wrong_parent, 2, zero_parent, 2, "wrong-parent"));
    CHECK(cdc_transport_inspect(&peer, &wrong_parent, KEY, &transport_ticket) ==
          CDC_TRANSPORT_HOLD_CAUSAL_PARENT);
    CHECK(make_envelope(&regressed_clock, 2, first.envelope_digest, 7,
                        "regressed-clock"));
    regressed_clock.logical_clock = first.logical_clock;
    CHECK(cdc_transport_envelope_sign(&regressed_clock, KEY));
    CHECK(cdc_transport_inspect(&peer, &regressed_clock, KEY,
                                &transport_ticket) ==
          CDC_TRANSPORT_HOLD_CAUSAL_CLOCK);
    CHECK(peer.streams[0].last_sequence == 1 && mutations == 1);

    CHECK(make_envelope(&second, 2, first.envelope_digest, 2, "proposal-two"));
    request_from_envelope(&request, &second, 1100);
    variant = request;
    set_id(variant.frame, "wrong-frame");
    CHECK(cdc_authority_check(&guard, &variant, &authority_ticket) ==
          CDC_AUTHORITY_REJECT_FRAME);
    variant = request;
    set_id(variant.subject, "wrong-node");
    CHECK(cdc_authority_check(&guard, &variant, &authority_ticket) ==
          CDC_AUTHORITY_REJECT_SUBJECT);
    variant = request;
    memset(variant.proposal_digest, 0, sizeof(variant.proposal_digest));
    CHECK(cdc_authority_check(&guard, &variant, &authority_ticket) ==
          CDC_AUTHORITY_REJECT_ARGUMENT);
    variant = request;
    variant.horizon = 21;
    CHECK(cdc_authority_check(&guard, &variant, &authority_ticket) ==
          CDC_AUTHORITY_REJECT_HORIZON);
    variant = request;
    variant.action = CDC_AUTH_ENACT;
    CHECK(cdc_authority_check(&guard, &variant, &authority_ticket) ==
          CDC_AUTHORITY_REJECT_ACTION);
    variant = request;
    variant.approvals = 2;
    CHECK(cdc_authority_check(&guard, &variant, &authority_ticket) ==
          CDC_AUTHORITY_REJECT_QUORUM);
    variant = request;
    variant.now = 2000;
    CHECK(cdc_authority_check(&guard, &variant, &authority_ticket) ==
          CDC_AUTHORITY_REJECT_EXPIRED);
    CHECK(mutations == 1);

    CHECK(admit(&peer, &guard, &second, 1100, &mutations));
    CHECK(peer.streams[0].last_sequence == 2 && mutations == 2);
    CHECK(cdc_transport_inspect(&peer, &first, KEY, &transport_ticket) ==
          CDC_TRANSPORT_REJECT_REPLAY);

    /* A transport-valid packet with a reused authority nonce is refused
     * before transport acceptance, so the stream and application stand still. */
    CHECK(make_envelope(&replay_nonce, 3, second.envelope_digest, 2,
                        "reused-nonce"));
    CHECK(cdc_transport_inspect(&peer, &replay_nonce, KEY,
                                &transport_ticket) == CDC_TRANSPORT_ACCEPT);
    request_from_envelope(&request, &replay_nonce, 1100);
    CHECK(cdc_authority_check(&guard, &request, &authority_ticket) ==
          CDC_AUTHORITY_REJECT_REPLAY);
    CHECK(peer.streams[0].last_sequence == 2 && mutations == 2);

    CHECK(make_envelope(&fresh_third, 3, second.envelope_digest, 4,
                        "proposal-three"));
    CHECK(admit(&peer, &guard, &fresh_third, 1100, &mutations));
    CHECK(peer.streams[0].last_sequence == 3 && mutations == 3);

    cdc_transport_envelope_free(&first);
    cdc_transport_envelope_free(&second);
    cdc_transport_envelope_free(&gap);
    cdc_transport_envelope_free(&wrong_parent);
    cdc_transport_envelope_free(&regressed_clock);
    cdc_transport_envelope_free(&replay_nonce);
    cdc_transport_envelope_free(&fresh_third);
    cdc_authority_guard_free(&guard);
    cdc_transport_peer_free(&peer);
    return 1;
}

static int test_stale_tickets_and_revocation(void) {
    cdc_transport_peer peer;
    cdc_authority_guard guard;
    cdc_authority_lease lease;
    cdc_transport_envelope envelope;
    cdc_transport_ticket transport_ticket;
    cdc_authority_request request;
    cdc_authority_ticket authority_ticket, forged_ticket;

    CHECK(cdc_transport_peer_init(&peer, 1, "supervisor", "lab-key-1", 4096,
                                  2));
    CHECK(cdc_authority_guard_init(&guard, 2, 4));
    memset(&lease, 0, sizeof(lease));
    lease.version = 1;
    set_id(lease.lease_id, "lease-1");
    set_id(lease.subject, "node-a");
    set_id(lease.frame, "cortical-column");
    lease.actions = CDC_AUTH_PROPOSE;
    lease.horizon_start = 0;
    lease.horizon_end = 9;
    lease.not_before = 1;
    lease.expires_at = 100;
    lease.quorum_required = 1;
    lease.quorum_total = 1;
    CHECK(cdc_authority_add(&guard, &lease) == CDC_AUTHORITY_ACCEPT);
    CHECK(make_envelope(&envelope, 1, NULL, 9, "stale-ticket"));
    CHECK(cdc_transport_inspect(&peer, &envelope, KEY, &transport_ticket) ==
          CDC_TRANSPORT_ACCEPT);
    CHECK(cdc_transport_accept(&peer, &transport_ticket) ==
          CDC_TRANSPORT_ACCEPT);
    CHECK(cdc_transport_accept(&peer, &transport_ticket) ==
          CDC_TRANSPORT_REJECT_STALE_TICKET);

    request_from_envelope(&request, &envelope, 10);
    request.horizon = 1;
    request.approvals = 1;
    CHECK(cdc_authority_check(&guard, &request, &authority_ticket) ==
          CDC_AUTHORITY_ACCEPT);
    forged_ticket = authority_ticket;
    set_id(forged_ticket.subject, "forged-node");
    CHECK(cdc_authority_prepare_consume(&guard, &forged_ticket, 10) ==
          CDC_AUTHORITY_REJECT_SUBJECT);
    forged_ticket = authority_ticket;
    forged_ticket.horizon = 10;
    CHECK(cdc_authority_prepare_consume(&guard, &forged_ticket, 10) ==
          CDC_AUTHORITY_REJECT_HORIZON);
    forged_ticket = authority_ticket;
    memset(forged_ticket.proposal_digest, 0,
           sizeof(forged_ticket.proposal_digest));
    CHECK(cdc_authority_prepare_consume(&guard, &forged_ticket, 10) ==
          CDC_AUTHORITY_REJECT_ARGUMENT);
    CHECK(cdc_authority_revoke(&guard, "lease-1") == CDC_AUTHORITY_ACCEPT);
    CHECK(cdc_authority_consume(&guard, &authority_ticket, 10) ==
          CDC_AUTHORITY_REJECT_STALE_TICKET);
    CHECK(cdc_authority_check(&guard, &request, &authority_ticket) ==
          CDC_AUTHORITY_REJECT_REVOKED);

    cdc_transport_envelope_free(&envelope);
    cdc_authority_guard_free(&guard);
    cdc_transport_peer_free(&peer);
    return 1;
}

int main(void) {
    if (!test_control_plane() || !test_stale_tickets_and_revocation()) {
        return 1;
    }
    puts("RFTC control plane PASS: auth=keyed scope/horizon/expiry/quorum "
         "replay=refused partition/gap/clock=hold mutation=guarded");
    return 0;
}
