/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdlib.h>

#include "files.h"
#include "log.h"
#include "tpm2_alg_util.h"
#include "tpm2_convert.h"
#include "tpm2_policy.h"
#include "tpm2_tool.h"

typedef struct tpm2_policysigned_ctx tpm2_policysigned_ctx;
struct tpm2_policysigned_ctx {
    const char *session_path;
    tpm2_session *session;

    const char *policy_digest_path;

    TPMT_SIGNATURE signature;
    TPMI_ALG_SIG_SCHEME format;
    TPMI_ALG_HASH halg;
    char *sig_file_path;

    const char *context_arg;
    tpm2_loaded_object key_context_object;

    union {
        struct {
            UINT8 halg :1;
            UINT8 sig :1;
            UINT8 fmt :1;
        };
        UINT8 all;
    } flags;
};

static tpm2_policysigned_ctx ctx;

static bool on_option(char key, char *value) {

    switch (key) {
    case 'L':
        ctx.policy_digest_path = value;
        break;
    case 'S':
        ctx.session_path = value;
        break;
    case 'g':
        ctx.halg = tpm2_alg_util_from_optarg(value, tpm2_alg_util_flags_hash);
        if (ctx.halg == TPM2_ALG_ERROR) {
            LOG_ERR("Unable to convert algorithm, got: \"%s\"", value);
            return false;
        }
        ctx.flags.halg = 1;
        break;
    case 'f':
        ctx.format = tpm2_alg_util_from_optarg(value, tpm2_alg_util_flags_sig);
        if (ctx.format == TPM2_ALG_ERROR) {
            LOG_ERR("Unknown signing scheme, got: \"%s\"", value);
            return false;
        }
        ctx.flags.fmt = 1;
        break;
    case 's':
        ctx.sig_file_path = value;
        ctx.flags.sig = 1;
        break;
    case 'c':
        ctx.context_arg = value;
        break;
    }

    return true;
}

bool tpm2_tool_onstart(tpm2_options **opts) {

    static struct option topts[] = {
        { "policy",         required_argument, NULL, 'L' },
        { "session",        required_argument, NULL, 'S' },
        { "hash-algorithm", required_argument, NULL, 'g' },
        { "signature",      required_argument, NULL, 's' },
        { "format",         required_argument, NULL, 'f' },
        { "key-context",    required_argument, NULL, 'c' },
    };

    *opts = tpm2_options_new("L:S:g:s:f:c:", ARRAY_LEN(topts), topts, on_option,
    NULL, 0);

    return *opts != NULL;
}

bool is_input_option_args_valid(void) {

    if (!ctx.session_path) {
        LOG_ERR("Must specify -S session file.");
        return false;
    }

    if (!(ctx.context_arg && ctx.flags.sig && ctx.flags.halg)) {
        LOG_ERR("--key-context, --sig, --hash-algorithm are required");
        return false;
    }

    return true;
}

tool_rc tpm2_tool_onrun(ESYS_CONTEXT *ectx, tpm2_option_flags flags) {

    UNUSED(flags);

    bool retval = is_input_option_args_valid();
    if (!retval) {
        return tool_rc_option_error;
    }

    if (ctx.flags.sig) {
        tpm2_convert_sig_fmt fmt =
                ctx.flags.fmt ? signature_format_plain : signature_format_tss;
        bool res = tpm2_convert_sig_load(ctx.sig_file_path, fmt, ctx.format,
                ctx.halg, &ctx.signature);
        if (!res) {
            return tool_rc_general_error;
        }
    }

    /*
     * For signature verification only object load is needed, not auth.
     */
    tool_rc tmp_rc = tpm2_util_object_load(ectx, ctx.context_arg,
            &ctx.key_context_object,
            TPM2_HANDLES_FLAGS_TRANSIENT|TPM2_HANDLES_FLAGS_PERSISTENT);
    if (tmp_rc != tool_rc_success) {
        return tmp_rc;
    }


    tool_rc rc = tpm2_session_restore(ectx, ctx.session_path, false,
            &ctx.session);
    if (rc != tool_rc_success) {
        return rc;
    }

    rc = tpm2_policy_build_policysigned(ectx, ctx.session,
        &ctx.key_context_object, &ctx.signature);
    if (rc != tool_rc_success) {
        LOG_ERR("Could not build policysigned TPM");
        return rc;
    }

    return tpm2_policy_tool_finish(ectx, ctx.session, ctx.policy_digest_path);
}

tool_rc tpm2_tool_onstop(ESYS_CONTEXT *ectx) {

    UNUSED(ectx);
    return tpm2_session_close(&ctx.session);
}
