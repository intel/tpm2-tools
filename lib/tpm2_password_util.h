#ifndef SRC_PASSWORD_UTIL_H_
#define SRC_PASSWORD_UTIL_H_

#include <sapi/tpm20.h>

/**
 * Copies a password stored in a TPM2B_AUTH structure, converting from hex if necessary, into
 * another TPM2B_AUTh structure. Source password and auth structures can be the same pointer.
 * @param password
 *  The source password.
 * @param is_hex
 *  True if the password contained in password is hex encoded.
 * @param description
 *  The description of the key used for error reporting.
 * @param auth
 *  The destination auth structure to copy the key into.
 * @return
 *  True on success and False on failure.
 */
bool tpm2_password_util_fromhex(TPM2B_AUTH *password, bool is_hex, const char *description,
        TPM2B_AUTH *auth);

/**
 * Copies a C string password into a TPM2B_AUTH structure. It logs an error on failure.
 *
 * Note: Use of a TPM2B_AUTH structure is for proper size allocation reporting and having
 * a size parameter to avoid duplicate strlen() calls.
 *
 * @param password
 *  The C string password to copy.
 * @param description
 *  A description of the password being copied for error reporting purposes.
 * @param dest
 *  The destination TPM2B_AUTH structure.
 * @return
 *  True on success, False on error.
 */
bool tpm2_password_util_copy_password(const char *password, const char *description, TPM2B_AUTH *dest);

/**
 * Convert a password argument to a valid TPM2B_AUTH structure. Passwords can
 * be specified in two forms: string and hex-string and are identified by a
 * prefix of str: and hex: respectively. No prefix assumes the str form.
 *
 * For example, a string can be specified as:
 * "1234"
 * "str:1234"
 *
 * And a hexstring via:
 * "hex:1234abcd"
 *
 * Strings are copied verbatim to the TPM2B_AUTH buffer without the terminating NULL byte,
 * Hex strings differ only from strings in that they are converted to a byte array when
 * storing. At the end of storing, the size field is set to the size of bytes of the
 * password.
 *
 * If your password starts with a hex: prefix and you need to escape it, just use the string
 * prefix to escape it, like so:
 * "str:hex:password"
 *
 * @param password
 *  The optarg containing the password string.
 * @param dest
 *  The TPM2B_AUTH structure to copy the string into.
 * @return
 *  true on success, false on failure.
 */
bool tpm2_password_util_from_optarg(const char *password, TPM2B_AUTH *dest);

#endif /* SRC_PASSWORD_UTIL_H_ */
