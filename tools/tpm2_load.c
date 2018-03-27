//**********************************************************************;
// Copyright (c) 2015-2018, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// 3. Neither the name of Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//**********************************************************************;

#include <stdarg.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stdbool.h>

#include <tss2/tss2_sys.h>

#include "files.h"
#include "log.h"
#include "tpm2_auth_util.h"
#include "tpm2_options.h"
#include "tpm2_session.h"
#include "tpm2_tool.h"
#include "tpm2_util.h"

TPM2_HANDLE handle;

typedef struct tpm_load_ctx tpm_load_ctx;
struct tpm_load_ctx {
    struct {
        TPMS_AUTH_COMMAND session_data;
        tpm2_session *session;
    } auth;
    TPMI_DH_OBJECT parent_handle;
    TPM2B_PUBLIC  in_public;
    TPM2B_PRIVATE in_private;
    char *out_file;
    char *context_file;
    char *context_parent_file;
    struct {
        UINT8 H : 1;
        UINT8 u : 1;
        UINT8 r : 1;
        UINT8 c : 1;
        UINT8 C : 1;
    } flags;
};

static tpm_load_ctx ctx = {
    .auth = { .session_data = TPMS_AUTH_COMMAND_INIT(TPM2_RS_PW) },
};

int load (TSS2_SYS_CONTEXT *sapi_context) {
    UINT32 rval;
    TSS2L_SYS_AUTH_COMMAND sessionsData = { 1, { ctx.auth.session_data }};
    TSS2L_SYS_AUTH_RESPONSE sessionsDataOut;

    TPM2B_NAME nameExt = TPM2B_TYPE_INIT(TPM2B_NAME, name);

    rval = TSS2_RETRY_EXP(Tss2_Sys_Load(sapi_context,
                         ctx.parent_handle,
                         &sessionsData,
                         &ctx.in_private,
                         &ctx.in_public,
                         &handle,
                         &nameExt,
                         &sessionsDataOut));
    if(rval != TPM2_RC_SUCCESS)
    {
        LOG_PERR(Tss2_Sys_Load, rval);
        return -1;
    }
    tpm2_tool_output("handle: 0x%08x\n", handle);

    if (ctx.out_file) {
        if(!files_save_bytes_to_file(ctx.out_file, nameExt.name, nameExt.size)) {
            return -2;
        }
    }

    return 0;
}

static bool on_option(char key, char *value) {

    bool res;

    switch(key) {
    case 'H':
        if (!tpm2_util_string_to_uint32(value, &ctx.parent_handle)) {
            LOG_ERR("Invalid parent key handle, got\"%s\"", value);
                return false;
        }
        ctx.flags.H = 1;
        break;
    case 'P':
        res = tpm2_auth_util_from_optarg(value, &ctx.auth.session_data,
                &ctx.auth.session);
        if (!res) {
            LOG_ERR("Invalid parent key authorization, got\"%s\"", value);
            return false;
        }
        break;
    case 'u':
        if(!files_load_public(value, &ctx.in_public)) {
            return false;;
        }
        ctx.flags.u = 1;
        break;
    case 'r':
        res = files_load_private(value, &ctx.in_private);
        if(!res) {
            return false;
        }
        ctx.flags.r = 1;
        break;
    case 'n':
        ctx.out_file = value;
        if(files_does_file_exist(ctx.out_file)) {
            return false;
        }
        break;
    case 'c':
        ctx.context_parent_file = value;
        if(ctx.context_parent_file == NULL || ctx.context_parent_file[0] == '\0') {
                return false;
        }
        ctx.flags.c = 1;
        break;
    case 'C':
        ctx.context_file = value;
        if(ctx.context_file == NULL || ctx.context_file[0] == '\0') {
            return false;
        }
        ctx.flags.C = 1;
        break;
    }

    return true;
}

bool tpm2_tool_onstart(tpm2_options **opts) {

    const struct option topts[] = {
      { "parent",               required_argument, NULL, 'H' },
      { "auth-parent",          required_argument, NULL, 'P' },
      { "pubfile",              required_argument, NULL, 'u' },
      { "privfile",             required_argument, NULL, 'r' },
      { "name",                 required_argument, NULL, 'n' },
      { "context",              required_argument, NULL, 'C' },
      { "context-parent",       required_argument, NULL, 'c' },
    };

    *opts = tpm2_options_new("H:P:u:r:n:C:c:", ARRAY_LEN(topts), topts,
                             on_option, NULL, TPM2_OPTIONS_SHOW_USAGE);

    return *opts != NULL;
}

int tpm2_tool_onrun(TSS2_SYS_CONTEXT *sapi_context, tpm2_option_flags flags) {

    UNUSED(flags);

    int rc = 1;
    bool result;

    if ((!ctx.flags.H && !ctx.flags.c) || (!ctx.flags.u || !ctx.flags.r)) {
        LOG_ERR("Expected options (H or c) and u and r");
        goto out;
    }

    if(ctx.flags.c) {
        result = files_load_tpm_context_from_path(sapi_context,
                    &ctx.parent_handle,
                    ctx.context_parent_file);
        if (!result) {
            goto out;
        }
    }

    int tmp_rc = load(sapi_context);
    if (tmp_rc) {
        goto out;
    }

    if (ctx.flags.C) {
        result = files_save_tpm_context_to_path(sapi_context,
                    handle,
                    ctx.context_file);
        if (!result) {
            goto out;
        }
    }

    rc = 0;

out:
    result = tpm2_session_save(sapi_context, ctx.auth.session, NULL);
    if (!result) {
        rc = 1;
    }

    return rc;
}

void tpm2_onexit(void) {

    tpm2_session_free(&ctx.auth.session);
}
