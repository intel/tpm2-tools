% tpm2_changeeps(1) tpm2-tools | General Commands Manual

# NAME

**tpm2_changeeps**(1) - Replaces the active endorsement primary seed with a new
one generated off the TPM2 RNG.

# SYNOPSIS

**tpm2_changeeps** [*OPTIONS*]

# DESCRIPTION

**tpm2_changeeps**(1) - Replaces the active endorsement primary seed with a new
one generated off the TPM2 RNG. The Transient and Persistent objects under the
endorsement hierarchy are lost. This command requires platform auth.

# OPTIONS

  * **-p**, **\--auth**=_AUTH_

    Specifies the _AUTH_ for the platform. hierarchy.

  * **\--cphash**=_FILE_

    File path to record the hash of the command parameters. This is commonly
    termed as cpHash. NOTE: When this option is selected, The tool will not
    actually execute the command, it simply returns a cpHash, unless rphash
    is also required.

  * **\--rphash**=_FILE_

    File path to record the hash of the response parameters. This is commonly
    termed as rpHash.

  * **-S**, **\--session**=_FILE_:

    The session created using **tpm2_startauthsession**. This can be used to
    specify an auxiliary session for auditing and or encryption/decryption of
    the parameters.

## References

[authorization formatting](common/authorizations.md) details the methods for
specifying _AUTH_.

[common tcti options](common/tcti.md) collection of options used to configure
the various known TCTI modules.

# EXAMPLES

## Change the endorsement primary seed where the platform auth is NULL.
```bash
tpm2_changeeps
```

[returns](common/returns.md)

[limitations](common/policy-limitations.md)

[footer](common/footer.md)
