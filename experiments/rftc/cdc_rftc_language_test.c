#include "../../runtime/cdc_ast.h"
#include "../../runtime/cdc_diagnostic.h"
#include "../../runtime/cdc_parser.h"

#include <stdio.h>
#include <string.h>

static int parse_ok(const char *source, cdc_unit *unit,
                    cdc_diag_list *diags) {
    cdc_diag_list_init(diags);
    if (!cdc_unit_parse_buffer(source, strlen(source), "<rftc-test>", unit,
                               diags)) {
        return 0;
    }
    return diags->errors == 0;
}

static int test_six_forms(void) {
    static const char source[] =
        "frame cortical-column scale=mesoscopic members=node/* window=50ms "
        "clock=hybrid-logical reducer=rftc-v1\n"
        "reduce cortical-column outputs=R,Psi,dispersion,winding,classDigest "
        "minimum-members=16 stale-after=150ms\n"
        "complex column-ring frame=cortical-column orientation=clockwise "
        "adjacency=ring boundary=closed\n"
        "topology phase-sector complex=column-ring invariant=winding "
        "allowed=local-continuous transition=phase-slip\n"
        "authority cell-controller frame=cortical-column "
        "permits=observe,propose enact=quorum:3/5 "
        "expires=2026-07-29T00:00:00Z\n"
        "transport lab-mesh protocol=quic-mtls schema=rftc-wire-v1 "
        "replay=rftc-replay-v1 partition=hold\n";
    static const char *const directives[] = {
        "frame", "reduce", "complex", "topology", "authority", "transport",
    };
    static const char *const ids[] = {
        "cortical-column", "cortical-column", "column-ring",
        "phase-sector", "cell-controller", "lab-mesh",
    };
    cdc_unit unit;
    cdc_diag_list diags;
    size_t i;
    int ok = 1;

    if (!parse_ok(source, &unit, &diags)) {
        cdc_diag_list_print(&diags, stderr);
        cdc_diag_list_free(&diags);
        cdc_unit_free(&unit);
        return 0;
    }
    if (unit.count != 6) {
        fprintf(stderr, "expected six RFTC forms, got %zu\n", unit.count);
        ok = 0;
    }
    for (i = 0; ok && i < 6; i++) {
        const cdc_stmt *stmt = &unit.stmts[i];
        const char *id = cdc_stmt_arg(stmt, 0);
        if (stmt->kind != CDC_STMT_FORM ||
            strcmp(cdc_stmt_directive(stmt), directives[i]) != 0 ||
            !id || strcmp(id, ids[i]) != 0) {
            fprintf(stderr, "RFTC form %zu classified incorrectly\n", i);
            ok = 0;
        }
    }
    if (ok &&
        (strcmp(cdc_stmt_attr(&unit.stmts[0], "clock"),
                "hybrid-logical") != 0 ||
         strcmp(cdc_stmt_attr(&unit.stmts[4], "enact"), "quorum:3/5") != 0 ||
         strcmp(cdc_stmt_attr(&unit.stmts[5], "partition"), "hold") != 0)) {
        fprintf(stderr, "RFTC attributes were not preserved\n");
        ok = 0;
    }

    cdc_diag_list_free(&diags);
    cdc_unit_free(&unit);
    return ok;
}

static int expect_rejected(const char *source, const char *code) {
    cdc_unit unit;
    cdc_diag_list diags;
    int found = 0;
    size_t i;

    cdc_diag_list_init(&diags);
    if (!cdc_unit_parse_buffer(source, strlen(source), "<rftc-negative>",
                               &unit, &diags)) {
        cdc_diag_list_free(&diags);
        cdc_unit_free(&unit);
        return 0;
    }
    for (i = 0; i < diags.count; i++) {
        if (strcmp(diags.items[i].code, code) == 0) {
            found = 1;
        }
    }
    cdc_diag_list_free(&diags);
    cdc_unit_free(&unit);
    return found;
}

int main(void) {
    if (!test_six_forms()) {
        return 1;
    }
    if (!expect_rejected("frame window=50ms\n", "CDC021")) {
        fputs("frame without an id did not fail closed\n", stderr);
        return 1;
    }
    if (!expect_rejected("qubit-spread lab\n", "CDC020")) {
        fputs("unknown quantum-shaped alias did not fail closed\n", stderr);
        return 1;
    }
    puts("RFTC language forms: six accepted, malformed/unknown rejected");
    return 0;
}
