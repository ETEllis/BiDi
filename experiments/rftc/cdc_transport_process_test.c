#define _POSIX_C_SOURCE 200809L

#include "../../runtime/cdc_authority.h"
#include "../../runtime/cdc_transport.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHECK(expr)                                                            \
    do {                                                                       \
        if (!(expr)) {                                                         \
            fprintf(stderr, "process transport check failed at %s:%d: %s\n",  \
                    __FILE__, __LINE__, #expr);                                \
            goto cleanup;                                                      \
        }                                                                      \
    } while (0)

static const uint8_t KEY[CDC_TRANSPORT_TAG_SIZE] = {
    0x31, 0x7a, 0x55, 0x09, 0x6d, 0x81, 0x22, 0xa4,
    0x05, 0xb2, 0xf1, 0x44, 0x99, 0x3e, 0x62, 0x17,
    0xc0, 0x12, 0x73, 0x8b, 0x41, 0x56, 0xea, 0xdd,
    0x10, 0x66, 0x39, 0x28, 0xf0, 0x7e, 0x4d, 0x93,
};

static int write_all(int fd, const void *data, size_t length) {
    const uint8_t *bytes = data;
    while (length > 0) {
        ssize_t written = write(fd, bytes, length);
        if (written < 0 && errno == EINTR) {
            continue;
        }
        if (written <= 0) {
            return 0;
        }
        bytes += written;
        length -= (size_t)written;
    }
    return 1;
}

static int read_all(int fd, void *data, size_t length) {
    uint8_t *bytes = data;
    while (length > 0) {
        ssize_t got = read(fd, bytes, length);
        if (got < 0 && errno == EINTR) {
            continue;
        }
        if (got <= 0) {
            return 0;
        }
        bytes += got;
        length -= (size_t)got;
    }
    return 1;
}

static void encode_length(uint64_t length, uint8_t out[8]) {
    size_t i;
    for (i = 0; i < 8; i++) {
        out[7 - i] = (uint8_t)(length >> (i * 8));
    }
}

static uint64_t decode_length(const uint8_t bytes[8]) {
    uint64_t value = 0;
    size_t i;
    for (i = 0; i < 8; i++) {
        value = (value << 8) | bytes[i];
    }
    return value;
}

static void set_id(char *out, const char *value) {
    memcpy(out, value, strlen(value) + 1);
}

static void child_send(int fd) {
    cdc_transport_envelope envelope;
    uint8_t *wire = NULL;
    size_t wire_length = 0;
    uint8_t length_bytes[8];
    size_t i;
    cdc_transport_envelope_init(&envelope);
    envelope.schema_version = 1;
    envelope.message_type = CDC_TRANSPORT_PROPOSAL;
    set_id(envelope.sender, "process-node");
    set_id(envelope.recipient, "process-supervisor");
    set_id(envelope.frame, "distributed-frame");
    set_id(envelope.key_id, "process-key");
    set_id(envelope.lease_id, "process-lease");
    envelope.authority_action = CDC_AUTH_PROPOSE;
    envelope.horizon = 42;
    envelope.sequence = 1;
    envelope.logical_clock = 9001;
    for (i = 0; i < sizeof(envelope.nonce); i++) {
        envelope.nonce[i] = (uint8_t)(0xa0 + i);
    }
    if (!cdc_transport_envelope_set_payload(
            &envelope, "cross-process-proposal",
            strlen("cross-process-proposal")) ||
        !cdc_transport_envelope_sign(&envelope, KEY) ||
        !cdc_transport_envelope_encode(&envelope, &wire, &wire_length)) {
        cdc_transport_envelope_free(&envelope);
        _exit(2);
    }
    encode_length((uint64_t)wire_length, length_bytes);
    if (!write_all(fd, length_bytes, sizeof(length_bytes)) ||
        !write_all(fd, wire, wire_length)) {
        free(wire);
        cdc_transport_envelope_free(&envelope);
        _exit(3);
    }
    free(wire);
    cdc_transport_envelope_free(&envelope);
    _exit(0);
}

static size_t find_payload(const uint8_t *wire, size_t wire_length,
                           const char *needle) {
    size_t needle_length = strlen(needle);
    size_t i;
    for (i = 0; i + needle_length <= wire_length; i++) {
        if (memcmp(wire + i, needle, needle_length) == 0) {
            return i;
        }
    }
    return SIZE_MAX;
}

int main(void) {
    int sockets[2] = {-1, -1};
    pid_t child = -1;
    uint8_t length_bytes[8];
    uint64_t encoded_length;
    uint8_t *wire = NULL;
    uint8_t *roundtrip = NULL;
    uint8_t *malformed = NULL;
    size_t roundtrip_length = 0;
    size_t payload_offset;
    size_t mutation_index;
    unsigned mutations_refused = 0;
    cdc_transport_envelope envelope, decoded_tamper, scratch;
    cdc_transport_peer peer;
    cdc_transport_ticket transport_ticket;
    cdc_authority_guard guard;
    cdc_authority_lease lease;
    cdc_authority_request request;
    cdc_authority_ticket authority_ticket;
    int status;
    int ok = 0;

    cdc_transport_envelope_init(&envelope);
    cdc_transport_envelope_init(&decoded_tamper);
    cdc_transport_envelope_init(&scratch);
    memset(&peer, 0, sizeof(peer));
    memset(&guard, 0, sizeof(guard));

    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    child = fork();
    CHECK(child >= 0);
    if (child == 0) {
        close(sockets[0]);
        child_send(sockets[1]);
    }
    close(sockets[1]);
    sockets[1] = -1;
    CHECK(read_all(sockets[0], length_bytes, sizeof(length_bytes)));
    encoded_length = decode_length(length_bytes);
    CHECK(encoded_length > 0 && encoded_length <= 4096);
    wire = malloc((size_t)encoded_length);
    CHECK(wire != NULL);
    CHECK(read_all(sockets[0], wire, (size_t)encoded_length));
    CHECK(waitpid(child, &status, 0) == child);
    child = -1;
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);

    CHECK(cdc_transport_envelope_decode(wire, (size_t)encoded_length, 1024,
                                        &envelope));
    CHECK(cdc_transport_envelope_encode(&envelope, &roundtrip,
                                        &roundtrip_length));
    CHECK(roundtrip_length == (size_t)encoded_length);
    CHECK(memcmp(roundtrip, wire, roundtrip_length) == 0);
    malformed = malloc((size_t)encoded_length + 1);
    CHECK(malformed != NULL);

    CHECK(cdc_transport_peer_init(&peer, 1, "process-supervisor",
                                  "process-key", 1024, 4));
    for (mutation_index = 0; mutation_index < (size_t)encoded_length;
         mutation_index++) {
        memcpy(malformed, wire, (size_t)encoded_length);
        malformed[mutation_index] ^= 1;
        if (cdc_transport_envelope_decode(malformed, (size_t)encoded_length,
                                          1024, &scratch)) {
            CHECK(cdc_transport_inspect(&peer, &scratch, KEY,
                                        &transport_ticket) !=
                  CDC_TRANSPORT_ACCEPT);
            cdc_transport_envelope_free(&scratch);
        }
        mutations_refused++;
    }
    CHECK(mutations_refused == (unsigned)encoded_length);

    CHECK(cdc_authority_guard_init(&guard, 4, 16));
    memset(&lease, 0, sizeof(lease));
    lease.version = 1;
    set_id(lease.lease_id, "process-lease");
    set_id(lease.subject, "process-node");
    set_id(lease.frame, "distributed-frame");
    lease.actions = CDC_AUTH_PROPOSE;
    lease.horizon_start = 40;
    lease.horizon_end = 50;
    lease.not_before = 100;
    lease.expires_at = 200;
    lease.quorum_required = 2;
    lease.quorum_total = 3;
    CHECK(cdc_authority_add(&guard, &lease) == CDC_AUTHORITY_ACCEPT);

    CHECK(cdc_transport_inspect(&peer, &envelope, KEY, &transport_ticket) ==
          CDC_TRANSPORT_ACCEPT);
    memset(&request, 0, sizeof(request));
    set_id(request.lease_id, envelope.lease_id);
    set_id(request.subject, envelope.sender);
    set_id(request.frame, envelope.frame);
    request.action = envelope.authority_action;
    request.horizon = envelope.horizon;
    request.now = 150;
    request.approvals = 2;
    memcpy(request.nonce, envelope.nonce, sizeof(request.nonce));
    memcpy(request.proposal_digest, envelope.payload_digest,
           sizeof(request.proposal_digest));
    CHECK(cdc_authority_check(&guard, &request, &authority_ticket) ==
          CDC_AUTHORITY_ACCEPT);
    CHECK(cdc_authority_consume(&guard, &authority_ticket, request.now) ==
          CDC_AUTHORITY_ACCEPT);
    CHECK(cdc_transport_accept(&peer, &transport_ticket) ==
          CDC_TRANSPORT_ACCEPT);
    CHECK(peer.stream_count == 1 && peer.streams[0].last_sequence == 1);
    CHECK(cdc_transport_inspect(&peer, &envelope, KEY, &transport_ticket) ==
          CDC_TRANSPORT_HOLD_DUPLICATE);

    CHECK(!cdc_transport_envelope_decode(wire, (size_t)encoded_length - 1,
                                         1024, &scratch));
    memcpy(malformed, wire, (size_t)encoded_length);
    malformed[encoded_length] = 0;
    CHECK(!cdc_transport_envelope_decode(malformed,
                                         (size_t)encoded_length + 1, 1024,
                                         &scratch));
    malformed[0] ^= 1;
    CHECK(!cdc_transport_envelope_decode(malformed,
                                         (size_t)encoded_length + 1, 1024,
                                         &scratch));
    malformed[0] ^= 1;

    payload_offset =
        find_payload(wire, (size_t)encoded_length, "cross-process-proposal");
    CHECK(payload_offset != SIZE_MAX);
    memcpy(malformed, wire, (size_t)encoded_length);
    malformed[payload_offset] ^= 1;
    CHECK(cdc_transport_envelope_decode(malformed, (size_t)encoded_length,
                                        1024, &decoded_tamper));
    CHECK(cdc_transport_inspect(&peer, &decoded_tamper, KEY,
                                &transport_ticket) ==
          CDC_TRANSPORT_REJECT_PAYLOAD);

    ok = 1;

cleanup:
    if (child > 0) {
        kill(child, SIGKILL);
        waitpid(child, NULL, 0);
    }
    if (sockets[0] >= 0) {
        close(sockets[0]);
    }
    if (sockets[1] >= 0) {
        close(sockets[1]);
    }
    free(wire);
    free(roundtrip);
    free(malformed);
    cdc_transport_envelope_free(&envelope);
    cdc_transport_envelope_free(&decoded_tamper);
    cdc_transport_envelope_free(&scratch);
    cdc_transport_peer_free(&peer);
    cdc_authority_guard_free(&guard);
    if (!ok) {
        return 1;
    }
    printf("RFTC cross-process transport PASS: wire=canonical auth=verified "
           "authority=consumed replay=held malformed=refused mutations=%u\n",
           mutations_refused);
    return 0;
}
