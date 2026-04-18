#ifndef ISO15118_3_INTERFACES_C_API_SLAC_API_H
#define ISO15118_3_INTERFACES_C_API_SLAC_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Public, stable C API (v1) for ISO 15118-3 SLAC (PEV side).
 *
 * Design goals:
 * - No process termination (no exit()).
 * - No requirement to include internal headers in external callers.
 * - Minimal surface for first iteration; can extend without breaking ABI.
 */

typedef enum slac_log_level
{
    SLAC_LOG_NONE = 0,
    SLAC_LOG_INFO = 1,
    SLAC_LOG_DEBUG = 2,
} slac_log_level_t;

typedef enum slac_err
{
    SLAC_OK = 0,

    SLAC_ERR_INVALID_ARGUMENT = -10,
    SLAC_ERR_NOT_OPEN = -11,
    SLAC_ERR_ALREADY_OPEN = -12,
    SLAC_ERR_NO_MEMORY = -13,

    SLAC_ERR_TRANSPORT = -20,
    SLAC_ERR_TIMEOUT = -21,
    SLAC_ERR_PROTOCOL = -22,
    SLAC_ERR_INTERNAL = -23,
} slac_err_t;

typedef struct slac_config
{
    slac_log_level_t log_level;
} slac_config_t;

typedef struct slac_if
{
    const char *ifname;
} slac_if_t;

typedef struct slac_key_material
{
    uint8_t nid[7];
    uint8_t nmk[16];
} slac_key_material_t;

typedef struct slac_match_request
{
    slac_key_material_t key;

    /**
     * If non-zero: use an alias destination MAC when sending CM_SET_KEY.
     * When enabled, caller must provide alias_dest_mac.
     */
    int alias_mac_compatible;
    uint8_t alias_dest_mac[6];

    /**
     * If non-zero: perform vendor status check (VS_GET_STATUS) before/after.
     * This currently mirrors existing test-program behavior.
     */
    int vendor_status_check;

    /**
     * If non-zero: after a successful match, wait and disconnect by set-key.
     * This matches legacy CLI "exec_disconnection" behavior.
     */
    int exec_disconnection;
} slac_match_request_t;

typedef struct slac_match_result
{
    int matched; /* 0/1 */
    uint8_t selected_evse_mac[6];
} slac_match_result_t;

typedef struct slac_client slac_client_t;

slac_client_t *slac_client_create(const slac_config_t *cfg);
void slac_client_destroy(slac_client_t *client);

slac_err_t slac_client_open(slac_client_t *client, const slac_if_t *iface);
void slac_client_close(slac_client_t *client);

slac_err_t slac_match_once(
    slac_client_t *client,
    const slac_match_request_t *req,
    slac_match_result_t *out);

#ifdef __cplusplus
}
#endif

#endif /* ISO15118_3_INTERFACES_C_API_SLAC_API_H */
