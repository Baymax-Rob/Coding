/*  plc hle utils project {{{
 *
 * Copyright (C) 2018 MStar Semiconductor
 *
 * <<<Licence>>>
 *
 * }}} */
#include "evse_slac.h"

int
evse_cm_set_key_req_cnf (void *opened_interface, evse *evse, const unsigned char *dst)
{
    LOG_PRINT (LOG_INFO, "[EVSE] sends CM_SET_KEY.REQ\n");
    mme_ctx_t req_ctx, cnf_ctx;
    unsigned char send_buff[ETH_DATA_LEN];
    unsigned char recv_buff[ETH_DATA_LEN];
    unsigned int buff_len, pkt_len=0;
    unsigned char *raw_pkt;

    set_key_req key_req =
    {
        .key_type = 1,
        .my_nonce = {0},
        .your_nonce = {0},
        .cco_capability = 1,
        .pid = 4,
        .prn = {0, 0},
        .pmn = 0,
        .new_eks = 1,
    };

    memcpy (key_req.nid, evse->nid, sizeof (evse->nid));
    memcpy (key_req.new_key, evse->nmk, sizeof (evse->nmk));

    if (mme_init (&req_ctx, CM_SET_KEY_REQ, send_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;
    if (mme_put (&req_ctx, &key_req, sizeof (key_req), &buff_len))
        return MME_ERROR_NOT_INIT;

    if (mme_send (opened_interface, &req_ctx, MME_SEND_REQ_CNF, dst))
        return MME_ERROR_TXRX;

    //rewind raw_pkt pointer to begining of the packet holder
    unsigned char pkt_holder[ETH_PACKET_MAX_NOVLAN_SIZE] = {0};
    raw_pkt = pkt_holder;

    if (mme_init (&cnf_ctx, CM_SET_KEY_CNF, recv_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;
    if (mme_receive_timeout (opened_interface, raw_pkt, &pkt_len, 0, 50))
    {
        return MME_ERROR_TIMEOUT;
    }

    MME_t *recv_mme = (MME_t *) raw_pkt;
    if (recv_mme->mmtype == CM_SET_KEY_CNF)
    {
        LOG_PRINT (LOG_INFO, "[EVSE] receives CM_SET_KEY.CNF\n");
    }
    else
    {
        LOG_PRINT (LOG_DEBUG, "[EVSE] receives CM_SET_KEY.CNF with wrong mmtype!\n");
        return MME_ERROR_RESULT;
    }

    mme_remove_header (recv_mme->mmtype, raw_pkt, pkt_len, &cnf_ctx);

    set_key_cnf key_cnf;
    memset (&key_cnf, 0, sizeof (key_cnf));
    if (mme_pull (&cnf_ctx, &key_cnf, sizeof (key_cnf), &buff_len))
        return MME_ERROR_NOT_INIT;
    if (key_cnf.result)
    {
        LOG_PRINT (LOG_DEBUG, "[EVSE] executes CM_SET_KEY successfully\n");
        return MME_SUCCESS;
    }
    else
    {
        LOG_PRINT (LOG_DEBUG, "[EVSE] fails to execute CM_SET_KEY\n");
        return MME_ERROR_RESULT;
    }
}


int
evse_cm_slac_parm_req_cnf (void *opened_interface, evse *evse,
                   mme_ctx_t *req_ctx, unsigned char *dst)
{
    mme_ctx_t  cnf_ctx;
    unsigned char send_buff[ETH_DATA_LEN];
    unsigned int buff_len;

    LOG_PRINT (LOG_INFO, "[EVSE] receives CM_SLAC_PARM.REQ\n");

    slac_parm_req slac_req;
    memset (&slac_req, 0, sizeof (slac_req));
    if (mme_pull (req_ctx, &slac_req, sizeof (slac_req), &buff_len))
        return MME_ERROR_ENOUGH;

    pev_item *target = search_pev_item_list (evse->pev_item_head, dst);
    if (target == NULL)
    {
        insert_pev_item
            (evse->pev_item_head, slac_req.run_id, dst, opened_interface);
    }
    else
    {
        update_pev_item (target, slac_req.run_id, dst, opened_interface);
    }

    if (DEBUG_DISPLAY)
    {
        print_pev_item_list (evse->pev_item_head);
    }

    if (mme_init (&cnf_ctx, CM_SLAC_PARM_CNF, send_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;
    slac_parm_cnf slac_parm_cnf =
    {
        .num_sounds = C_EV_match_MNBC,
        .time_out = 0x06,
        .resp_type = 0x01,
        .application_type = 0,
        .security_type = 0
    };
    memcpy
    (slac_parm_cnf.m_sound_target, mac_broadcast, sizeof (mac_broadcast));
    memcpy
    (slac_parm_cnf.run_id, slac_req.run_id, sizeof (slac_parm_cnf.run_id));

    if (mme_put (&cnf_ctx, &slac_parm_cnf, sizeof (slac_parm_cnf), &buff_len))
        return MME_ERROR_ENOUGH;

    if (mme_send (opened_interface, &cnf_ctx, MME_SEND_REQ_CNF, dst))
    {
        return MME_ERROR_TXRX;
    }
    LOG_PRINT (LOG_INFO, "[EVSE] sends CM_SLAC_PARM.CNF\n");
    return MME_SUCCESS;
}


int
evse_cm_start_atten_char_ind_receive (mme_ctx_t *ind_ctx,
    unsigned char *dst)
{
    LOG_PRINT (LOG_INFO, "[EVSE] receives CM_START_ATTEN_CHAR.IND\n");
    unsigned int buff_len;
    start_atten_char_ind start_ind;
    int i;

    memset (&start_ind, 0, sizeof (start_ind));
    if (mme_pull (ind_ctx, &start_ind, sizeof (start_ind), &buff_len))
        return MME_ERROR_ENOUGH;

    LOG_PRINT (LOG_VERBOSE, "CM_START_ATTEN_CHAR.IND->run_id = ");
    for (i = 0; i < 8; i++)
    {
        LOG_PRINT (LOG_VERBOSE, "%02X ", start_ind.ACVarField.run_id[i]);
    }
    LOG_PRINT (LOG_VERBOSE, "\n");

    return MME_SUCCESS;
}


int
evse_cm_mnbc_sound_ind_receive (mme_ctx_t *ind_ctx)
{
    unsigned int buff_len;
    mnbc_sound_ind mnbc_ind;
    mnbc_sound_ind_q mnbc_ind_q;

    if ( (ind_ctx->tail - ind_ctx->head) == sizeof (mnbc_ind))
    {
        memset (&mnbc_ind, 0, sizeof (mnbc_ind));
        if (mme_pull (ind_ctx, &mnbc_ind, sizeof (mnbc_ind), &buff_len))
            return MME_ERROR_ENOUGH;
        LOG_PRINT (LOG_INFO, "[EVSE] receives CM_MNBC_SOUND.IND---(%d)\n",
            mnbc_ind.MSVarField.cnt);
    }
    /* This case is for other vendor */
    else
    {
        memset (&mnbc_ind_q, 0, sizeof (mnbc_ind_q));
        if (mme_pull (ind_ctx, &mnbc_ind_q, sizeof (mnbc_ind_q), &buff_len))
            return MME_ERROR_ENOUGH;

        LOG_PRINT (LOG_INFO, "[EVSE] receives CM_MNBC_SOUND.IND---(%d)\n",
            mnbc_ind_q.MSVarField.cnt);
    }
    return MME_SUCCESS;
}


int
evse_cm_atten_profile_ind_receive (evse *evse, mme_ctx_t *ind_ctx)
{
    unsigned int buff_len;
    atten_profile_ind atten_ind;
    int i;
    memset (&atten_ind, 0, sizeof (atten_ind));

    if (mme_pull (ind_ctx, &atten_ind, sizeof (atten_ind), &buff_len))
    {
        LOG_PRINT (LOG_VERBOSE, "mme_pull() error\n");
        return MME_ERROR_ENOUGH;
    }
    LOG_PRINT (LOG_INFO, "[EVSE] receives CM_ATTEN_PROFILE.IND\n");

    pev_item *tmp =
        search_pev_item_list (evse->pev_item_head, atten_ind.pev_mac);
    if (tmp == NULL)
    {
        LOG_PRINT (LOG_DEBUG,
            "[EVSE] receives CM_ATTEN_PROFILE.IND from unknown PEV\n");
        return MME_ERROR_RESULT;
    }
    tmp->receive_sounds ++;

    for (i = 0; i < attenuation_group; i++)
    {
        tmp->AAG[i] += atten_ind.AAG[i];
    }
    return MME_SUCCESS;
}


int
evse_cm_atten_char_rsp_receive (evse *evse, mme_ctx_t *ind_ctx)
{
    unsigned int buff_len;
    atten_char_rsp atten_rsp;

    memset (&atten_rsp, 0 , sizeof (atten_rsp));
    if (mme_pull (ind_ctx, &atten_rsp, sizeof (atten_rsp), &buff_len))
    {
        LOG_PRINT (LOG_VERBOSE, "mme_pull() error\n");
        return MME_ERROR_ENOUGH;
    }
    if (atten_rsp.ACVarField.result != 0x00)
    {
        return MME_ERROR_RESULT;
    }

    pev_item *tmp = search_pev_item_list (evse->pev_item_head,
                        atten_rsp.ACVarField.source_address);
    if (tmp != NULL)
    {
        tmp->matching_state = RECEIVED_CM_ATTEN_CHAR_RSP;
        LOG_PRINT (LOG_INFO, "[EVSE] reveives CM_ATTEN_CHAR.RSP\n");
#ifdef _WIN32
        int i = 0;
        for (i = 1; i < C_EV_match_retry; i++)
        {
            pthread_cancel (tmp->cm_start_atten_char_ind_timer_id[i]);
        }
#endif // _WIN32

#ifdef __linux__
        if (timer_delete (tmp->cm_start_atten_char_ind_timer_id) == 0)
        {
            if (DEBUG_DISPLAY)
            {
                LOG_PRINT (LOG_VERBOSE,
                "deletes CM_START_ATTEN_CHAR_IND_TIMER successfully!\n");
            }
        }
        else
        {
            if (DEBUG_DISPLAY)
            {
                LOG_PRINT (LOG_VERBOSE,
                "CM_START_ATTEN_CHAR_IND_TIMER was already deleted!\n");
            }
        }
#endif // __linux
    }
    return MME_SUCCESS;
}


int
evse_cm_slac_match_req_receive_cnf_send (void *opened_interface,
                         mme_ctx_t *req_ctx, evse *evse, pev_item *pev)
{
    mme_ctx_t cnf_ctx;
    unsigned char send_buff[ETH_DATA_LEN];
    unsigned int buff_len;

    slac_match_req slac_req;
    memset (&slac_req, 0, sizeof (slac_req));
    if (mme_pull (req_ctx, &slac_req, sizeof (slac_req), &buff_len))
        return MME_ERROR_ENOUGH;
    LOG_PRINT (LOG_INFO, "[EVSE] receives CM_SLAC_MATCH.REQ\n");

    if (memcmp (slac_req.MatchVarField.run_id, pev->run_id, 8) != 0)
    {
        LOG_PRINT (LOG_VERBOSE, "Run ID is not equal!\n");
        return MME_ERROR_RESULT;
    }

    if (mme_init (&cnf_ctx, CM_SLAC_MATCH_CNF,
            send_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;

    slac_match_cnf slac_cnf =
    {
        .application_type = slac_req.application_type,
        .security_type = slac_req.security_type,
        .mvflength[0] = 0x56
    };
    memcpy (slac_cnf.MatchVarField.nid, evse->nid, sizeof (evse->nid));
    memcpy (slac_cnf.MatchVarField.nmk, evse->nmk, sizeof (evse->nmk));

    memcpy (slac_cnf.MatchVarField.pev_id, slac_req.MatchVarField.pev_id,
            sizeof (slac_req.MatchVarField.pev_id));
    memcpy (slac_cnf.MatchVarField.pev_mac, slac_req.MatchVarField.pev_mac,
            sizeof (slac_req.MatchVarField.pev_mac));
    memcpy (slac_cnf.MatchVarField.evse_id, slac_req.MatchVarField.evse_id,
            sizeof (slac_req.MatchVarField.evse_id));
    memcpy (slac_cnf.MatchVarField.evse_mac, slac_req.MatchVarField.evse_mac,
            sizeof (slac_req.MatchVarField.evse_mac));
    memcpy (slac_cnf.MatchVarField.run_id, slac_req.MatchVarField.run_id,
            sizeof (slac_req.MatchVarField.run_id));

    if (mme_put (&cnf_ctx, &slac_cnf, sizeof (slac_cnf), &buff_len))
        return MME_ERROR_SPACE;

    if (mme_send (opened_interface, &cnf_ctx, MME_SEND_CNF,
                                slac_req.MatchVarField.pev_mac))
        return MME_ERROR_TXRX;

    LOG_PRINT (LOG_INFO, "[EVSE] sends CM_SLAC_MATCH.CNF\n");
    return MME_SUCCESS;
}


void
test_result_display (evse *evse, int error_flag)
{
    LOG_PRINT (LOG_VERBOSE, "\n\n\nTest Result:  ");
    if (error_flag)
        LOG_PRINT (LOG_VERBOSE, "Test Failed\n\n\n");
    else
        LOG_PRINT (LOG_VERBOSE, "Test Succeeded\n\n\n");

    LOG_PRINT(LOG_VERBOSE, "-------------------------------------------------\n");
    LOG_PRINT(LOG_VERBOSE, "[EVSE] Switched to Unoccupied\n");
    LOG_PRINT(LOG_VERBOSE, "-------------------------------------------------\n");
}
