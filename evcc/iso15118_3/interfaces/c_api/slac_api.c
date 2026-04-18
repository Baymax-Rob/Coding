#include "slac_api.h"

#include "pev_slac.h"
#include "node.h"
#include "timer.h"

#include <stdlib.h>
#include <string.h>

struct slac_client
{
    slac_config_t cfg;
    void *opened_interface;
    int is_open;
};

static int is_all_zero_mac(const uint8_t mac[6])
{
    return mac[0] == 0 && mac[1] == 0 && mac[2] == 0 &&
           mac[3] == 0 && mac[4] == 0 && mac[5] == 0;
}

slac_client_t *slac_client_create(const slac_config_t *cfg)
{
    slac_client_t *c = (slac_client_t *)calloc(1, sizeof(*c));
    if (!c)
        return NULL;

    if (cfg)
    {
        c->cfg = *cfg;
    }
    else
    {
        c->cfg.log_level = SLAC_LOG_NONE;
    }

    return c;
}

void slac_client_destroy(slac_client_t *client)
{
    if (!client)
        return;
    slac_client_close(client);
    free(client);
}

slac_err_t slac_client_open(slac_client_t *client, const slac_if_t *iface)
{
    if (!client || !iface || !iface->ifname || iface->ifname[0] == '\0')
        return SLAC_ERR_INVALID_ARGUMENT;
    if (client->is_open)
        return SLAC_ERR_ALREADY_OPEN;

    int prev_debug = DEBUG_DISPLAY;
    DEBUG_DISPLAY = (int)client->cfg.log_level;

    void *opened = NULL;
#ifdef __linux__
    iface_t *ifc = (iface_t *)calloc(1, sizeof(*ifc));
    opened = ifc;
#endif

    if (!opened)
    {
        DEBUG_DISPLAY = prev_debug;
        return SLAC_ERR_NO_MEMORY;
    }

    iface_error_t rc = open_interface(iface->ifname, opened);
    if (rc != IFACE_SUCCESS)
    {
        free(opened);
        DEBUG_DISPLAY = prev_debug;
        return SLAC_ERR_TRANSPORT;
    }

    client->opened_interface = opened;
    client->is_open = 1;

    DEBUG_DISPLAY = prev_debug;
    return SLAC_OK;
}

void slac_client_close(slac_client_t *client)
{
    if (!client || !client->is_open)
        return;

    void *opened = client->opened_interface;
    close_interface(&opened);

    client->opened_interface = NULL;
    client->is_open = 0;
}

static void free_evse_list(evse_item *head)
{
    if (!head)
        return;
    evse_item *cur = head;
    while (cur)
    {
        evse_item *next = cur->next;
        free(cur);
        cur = next;
    }
}

static slac_err_t precheck_vendor_status(void *opened_interface)
{
    int st = vs_get_status_req_send_cnf_receive(opened_interface);
    if (st == CONNECT_LINK)
        return SLAC_ERR_PROTOCOL;
    if (st == ERROR_LINK)
        return SLAC_ERR_TRANSPORT;
    return SLAC_OK;
}

static int verify_vendor_connected(void *opened_interface)
{
    int st = vs_get_status_cnf_receive(opened_interface);
    if (st != CONNECT_LINK && st != ERROR_LINK)
    {
        st = vs_get_status_req_send_cnf_receive(opened_interface);
    }
    return st == CONNECT_LINK;
}

slac_err_t slac_match_once(
    slac_client_t *client,
    const slac_match_request_t *req,
    slac_match_result_t *out)
{
    if (!client || !req || !out)
        return SLAC_ERR_INVALID_ARGUMENT;
    if (!client->is_open || !client->opened_interface)
        return SLAC_ERR_NOT_OPEN;

    memset(out, 0, sizeof(*out));

    if (req->alias_mac_compatible)
    {
        if (is_all_zero_mac(req->alias_dest_mac))
            return SLAC_ERR_INVALID_ARGUMENT;
        memcpy(other_mac_link_local, req->alias_dest_mac, 6);
    }

    int prev_debug = DEBUG_DISPLAY;
    DEBUG_DISPLAY = (int)client->cfg.log_level;

    int error_flag = 0;

    pev pev_ctx = {
        .state = DISCONNECTED,
        .current_state = SENT_CM_SLAC_PARM_REQ,
        .cm_slac_parm_timer_flag = 1,
        .cm_slac_parm_req_retry = C_EV_match_retry,
        .potential_EVSE = 0,
        .cm_start_atten_char_timer_flag = 1,
        .cm_slac_match_req_retry = C_EV_match_retry,
        .evse_item_head = NULL,
    };

    pev_ctx.evse_item_head = (evse_item *)calloc(1, sizeof(evse_item));
    if (!pev_ctx.evse_item_head)
    {
        DEBUG_DISPLAY = prev_debug;
        return SLAC_ERR_NO_MEMORY;
    }
    pev_ctx.evse_item_head->next = NULL;

    memcpy(pev_ctx.nid, req->key.nid, sizeof(pev_ctx.nid));
    memcpy(pev_ctx.nmk, req->key.nmk, sizeof(pev_ctx.nmk));

    if (req->vendor_status_check && !req->alias_mac_compatible)
    {
        slac_err_t check = precheck_vendor_status(client->opened_interface);
        if (check != SLAC_OK)
        {
            free_evse_list(pev_ctx.evse_item_head);
            DEBUG_DISPLAY = prev_debug;
            return check;
        }
    }

    if (pev_ctx.state == DISCONNECTED)
    {
        pev_ctx.state = UNMATCHED;
    }

    if (pev_ctx.state == UNMATCHED)
    {
        while ((pev_ctx.current_state == SENT_CM_SLAC_PARM_REQ) &&
               (pev_ctx.cm_slac_parm_req_retry > 0))
        {
            int rc = pev_cm_slac_parm_req_send(client->opened_interface, &pev_ctx);
            if (rc != MME_SUCCESS)
            {
                error_flag = 1;
                break;
            }

            pev_ctx.cm_slac_parm_timer_flag = 1;
            pev_ctx.cm_slac_parm_req_retry--;

            make_pev_timer(&pev_ctx, TT_match_response, 0);

            while (pev_ctx.cm_slac_parm_timer_flag)
            {
                rc = pev_cm_slac_parm_cnf_receive(client->opened_interface, &pev_ctx);
                if (rc == MME_SUCCESS)
                {
                    pev_ctx.current_state = RECEIVED_CM_SLAC_PARM_CNF;
                }
            }

            if (pev_ctx.cm_slac_parm_req_retry == 0 &&
                pev_ctx.current_state != RECEIVED_CM_SLAC_PARM_CNF)
            {
                error_flag = 1;
            }
        }

        if (!error_flag && pev_ctx.current_state == RECEIVED_CM_SLAC_PARM_CNF)
        {
            make_pev_timer(&pev_ctx, TT_EV_atten_results, 0);
            if (pev_cm_start_atten_char_ind_send(client->opened_interface) != MME_SUCCESS)
                error_flag = 1;
            if (pev_cm_mnbc_sound_ind_send(client->opened_interface) != MME_SUCCESS)
                error_flag = 1;
            pev_ctx.current_state = SENT_CM_START_ATTEN_CHAR_IND;
        }

        if (!error_flag && pev_ctx.current_state == SENT_CM_START_ATTEN_CHAR_IND)
        {
            atten_char_ind atten_ind;
            unsigned char rsp_dest[ETH_ALEN];

            while (pev_ctx.cm_start_atten_char_timer_flag)
            {
                int rc = pev_cm_atten_char_ind_receive(
                    client->opened_interface, &pev_ctx, &atten_ind, &rsp_dest[0]);
                if (rc == MME_SUCCESS)
                {
                    pev_ctx.current_state = RECEIVED_CM_ATTEN_CHAR_IND;
                    rc = pev_cm_atten_char_rsp_send(
                        client->opened_interface, &atten_ind, &rsp_dest[0]);
                    if (rc != MME_SUCCESS)
                    {
                        error_flag = 1;
                        break;
                    }
                }
            }

            if (pev_ctx.current_state != RECEIVED_CM_ATTEN_CHAR_IND)
                error_flag = 1;
        }

        while (!error_flag &&
               pev_ctx.current_state == RECEIVED_CM_ATTEN_CHAR_IND &&
               pev_ctx.cm_slac_match_req_retry > 0)
        {
            evse_item *min_node = select_evse_item_list(pev_ctx.evse_item_head);
            if (!min_node)
            {
                error_flag = 1;
                break;
            }

            int rc = pev_cm_slac_match_req_send(client->opened_interface, min_node);
            if (rc != MME_SUCCESS)
            {
                error_flag = 1;
                break;
            }
            rc = pev_cm_slac_match_cnf_receive(client->opened_interface, &pev_ctx);
            if (rc == MME_SUCCESS)
            {
                pev_ctx.current_state = RECEIVED_CM_SLAC_MATCH_CNF;
                memcpy(out->selected_evse_mac, min_node->mac, 6);
                break;
            }

            pev_ctx.cm_slac_match_req_retry--;
            if (pev_ctx.cm_slac_match_req_retry == 0)
                error_flag = 1;
        }

        if (!error_flag && pev_ctx.current_state == RECEIVED_CM_SLAC_MATCH_CNF)
        {
            const unsigned char *dst = mac_link_local;
            if (req->alias_mac_compatible)
                dst = other_mac_link_local;

            int rc = pev_cm_set_key_req_send(client->opened_interface, &pev_ctx, dst);
            if (rc != MME_SUCCESS)
            {
                error_flag = 1;
            }
            else
            {
                out->matched = 1;
                pev_ctx.state = MATCHED;
            }
        }
    }

    if (out->matched && req->vendor_status_check && !req->alias_mac_compatible)
    {
        if (!verify_vendor_connected(client->opened_interface))
        {
            out->matched = 0;
            error_flag = 1;
        }
    }

    if (out->matched && req->exec_disconnection)
    {
        /* Keep legacy behavior: wait and disconnect via set-key. */
        sleep_ms(CHARGING_TIME);
        (void)pev_cm_set_key_req_disconnect(client->opened_interface, &pev_ctx, mac_link_local);
    }

    free_evse_list(pev_ctx.evse_item_head);
    DEBUG_DISPLAY = prev_debug;

    if (error_flag)
        return SLAC_ERR_PROTOCOL;
    return SLAC_OK;
}

