/*  plc hle utils project {{{
 *
 * Copyright (C) 2018 MStar Semiconductor
 *
 * <<<Licence>>>
 *
 * }}} */
#include "pev_slac.h"
int min_num_sounds = 10;    // how many Number of M-Sounds is sufficient

int
pev_cm_set_key_req_send (void *opened_interface, pev *pev,
    const unsigned char *dst)
{
    LOG_PRINT (LOG_INFO, "[PEV] sends CM_SET_KEY.REQ\n");
    mme_ctx_t req_ctx, cnf_ctx;
    unsigned char send_buff[ETH_DATA_LEN], recv_buff[ETH_DATA_LEN];
    unsigned int buff_len = 0, pkt_len = 0;
    int i;
    unsigned char *raw_pkt;

    set_key_req key_req =
    {
        .key_type = 1,
        .my_nonce = {0},
        .your_nonce = {0},
        .pid = 4,
        .prn = {0, 0},
        .pmn = 0,
        .new_eks = 1,
    };
    memcpy (key_req.nid, pev->received_nid, sizeof (pev->received_nid));
    memcpy (key_req.new_key, pev->received_nmk, sizeof (pev->received_nmk));

    LOG_PRINT (LOG_VERBOSE, "NID = \n");
    for (i = 0; i < 7; i++)
    {
        LOG_PRINT (LOG_VERBOSE, "%02x \n", key_req.nid[i]);
    }
    LOG_PRINT (LOG_VERBOSE, "NMK = \n");
    for (i = 0; i < 16; i++)
    {
        LOG_PRINT (LOG_VERBOSE, "%02x \n", key_req.new_key[i]);
    }
    LOG_PRINT (LOG_VERBOSE, "\n");

    if (mme_init (&req_ctx, CM_SET_KEY_REQ, send_buff, ETH_DATA_LEN))
       return  MME_ERROR_NOT_INIT;
    if (mme_put (&req_ctx, &key_req, sizeof (key_req), &buff_len))
        return MME_ERROR_NOT_INIT;

    if (mme_send (opened_interface, &req_ctx, MME_SEND_IND, dst))
        return MME_ERROR_TXRX;

    //rewind raw_pkt pointer to begining of the packet holder
    unsigned char pkt_holder[ETH_PACKET_MAX_NOVLAN_SIZE] = {0};
    raw_pkt = pkt_holder;

    if (mme_init (&cnf_ctx, CM_SET_KEY_CNF, recv_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;
    if (mme_receive_timeout (opened_interface, raw_pkt, &pkt_len, 0, 30))
    {
        return MME_ERROR_TIMEOUT;
    }
    MME_t *recv_mme = (MME_t *) raw_pkt;

    if (cnf_ctx.mmtype == CM_SET_KEY_CNF)
    {
        LOG_PRINT (LOG_INFO, "[PEV] receives CM_SET_KEY.CNF\n");
    }
    else
    {
        LOG_PRINT (LOG_VERBOSE,
            "[PEV] receives CM_SET_KEY.CNF with wrong mmtype!\n");
    }

    mme_remove_header (recv_mme->mmtype, raw_pkt, pkt_len, &cnf_ctx);

    set_key_cnf key_cnf;
    if (mme_pull (&cnf_ctx, &key_cnf, sizeof(key_cnf), &buff_len))
        return MME_ERROR_ENOUGH;
    if (key_cnf.result)
    {
        LOG_PRINT (LOG_VERBOSE, "[PEV] executes CM_SET_KEY successfully\n");
    }
    else
    {
        LOG_PRINT (LOG_VERBOSE,
        "[PEV] fails to execute CM_SET_KEY! result = %d\n", key_cnf.result);
        return MME_ERROR_RESULT;
    }

    return MME_SUCCESS;
}


int
pev_cm_set_key_req_disconnect (void *opened_interface, pev *pev, const unsigned char *dst)
{
    LOG_PRINT (LOG_INFO, "[PEV] sends CM_SET_KEY.REQ\n");
    mme_ctx_t req_ctx, cnf_ctx;
    unsigned char send_buff[ETH_DATA_LEN], recv_buff[ETH_DATA_LEN];
    unsigned int buff_len, pkt_len=0;
    int i;
    unsigned char *raw_pkt;

    set_key_req key_req =
    {
        .key_type = 1,
        .my_nonce = {0},
        .your_nonce = {0},
        .pid = 4,
        .prn = {0, 0},
        .pmn = 0,
        .new_eks = 1,
    };

    memcpy (key_req.nid, pev->nid, sizeof (pev->nid));
    memcpy (key_req.new_key, pev->nmk, sizeof (pev->nmk));

    LOG_PRINT (LOG_VERBOSE, "NID = \n");
    for (i = 0; i < 7; i++)
    {
        LOG_PRINT (LOG_VERBOSE, "%02x \n", key_req.nid[i]);
    }
    LOG_PRINT (LOG_VERBOSE, "NMK = \n");
    for (i = 0; i < 16; i++)
    {
        LOG_PRINT (LOG_VERBOSE, "%02x \n", key_req.new_key[i]);
    }
    LOG_PRINT (LOG_VERBOSE, "\n");

    if (mme_init (&req_ctx, CM_SET_KEY_REQ, send_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;
    if (mme_put (&req_ctx, &key_req, sizeof (key_req), &buff_len))
        return MME_ERROR_NOT_INIT;
    if (mme_send (opened_interface, &req_ctx, MME_SEND_IND, dst))
        return MME_ERROR_TXRX;

    // rewind raw_pkt pointer to begining of the packet holder
    unsigned char pkt_holder[ETH_PACKET_MAX_NOVLAN_SIZE] = {0};
    raw_pkt = pkt_holder;

    if (mme_init (&cnf_ctx, CM_SET_KEY_CNF, recv_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;
    if (mme_receive_timeout (opened_interface, raw_pkt, &pkt_len, 0, 30))
    {
        return MME_ERROR_TIMEOUT;
    }
    MME_t *recv_mme = (MME_t *) raw_pkt;

    if (cnf_ctx.mmtype == CM_SET_KEY_CNF)
    {
        LOG_PRINT (LOG_INFO, "[PEV] receives CM_SET_KEY.CNF\n");
    }
    else
    {
        LOG_PRINT (LOG_VERBOSE,
            "[PEV] receives CM_SET_KEY.CNF with wrong mmtype!!!\n");
        return MME_ERROR_RESULT;
    }
    mme_remove_header (recv_mme->mmtype, raw_pkt, pkt_len, &cnf_ctx);

    set_key_cnf key_cnf;
    if (mme_pull (&cnf_ctx, &key_cnf, sizeof (key_cnf), &buff_len))
        return MME_ERROR_ENOUGH;
    if (key_cnf.result)
    {
        LOG_PRINT (LOG_DEBUG, "[PEV] executes CM_SET_KEY successfully\n");
        return MME_SUCCESS;
    }
    else
    {
        LOG_PRINT (LOG_DEBUG, "[PEV] fails to execute CM_SET_KEY!\n");
        return MME_ERROR_RESULT;
    }
}


int
pev_cm_slac_parm_req_send (void *opened_interface, pev *pev)
{
    unsigned char send_buff[ETH_DATA_LEN];
    mme_ctx_t req_ctx;
    unsigned int num_len = 0;

    if (mme_init (&req_ctx, CM_SLAC_PARM_REQ, send_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;

    slac_parm_req slac_req =
    {
        .application_type = 0,
        .security_type = 0,
        .run_id = {1, 2, 3, 4, 5, 6, 7, 8}
    };

    memcpy (pev->run_id, slac_req.run_id, sizeof (slac_req.run_id));
    if (mme_put (&req_ctx, &slac_req, sizeof (slac_req), &num_len))
        return MME_ERROR_NOT_INIT;

    LOG_PRINT (LOG_INFO, "[PEV] sends CM_SLAC_PARM.REQ\n");

    if (mme_send (opened_interface, &req_ctx, MME_SEND_REQ_CNF,
        mac_broadcast))
        return MME_ERROR_TXRX;

    return MME_SUCCESS;
}


int
pev_cm_slac_parm_cnf_receive (void *opened_interface, pev *pev)
{
    unsigned char *pev_mac;
#ifdef _WIN32
    hpav_channel_t *channel = (hpav_channel_t *)opened_interface;
    pev_mac = channel->mac_addr;
#endif // _WIN32
#ifdef __linux__
    iface_t *iface = (iface_t *)opened_interface;
    pev_mac = iface->mac;
#endif // __linux__

    int i = 0;
    unsigned char recv_buff[ETH_DATA_LEN];
    mme_ctx_t  cnf_ctx;
    unsigned int pkt_len = 0, num_len = 0;
    unsigned char *raw_pkt;

    //rewind raw_pkt pointer to begining of the packet holder
    unsigned char pkt_holder[ETH_PACKET_MAX_NOVLAN_SIZE] = {0};
    raw_pkt = pkt_holder;


    if (mme_init (&cnf_ctx, CM_SLAC_PARM_CNF, recv_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;

    if (mme_receive_timeout (opened_interface, raw_pkt, &pkt_len, 0, 30))
    {
        return MME_ERROR_TIMEOUT;
    }

    MME_t *recv_mme = (MME_t *) raw_pkt;
    if (mme_check_dest_mac (recv_mme->mme_dest, pev_mac))
    {
        return MME_ERROR_RESULT;
    }

    if (recv_mme->mmtype == CM_SLAC_PARM_CNF)
    {
        LOG_PRINT (LOG_INFO, "[PEV] receives CM_SLAC_PARM.CNF\n");
    }
    else
    {
        LOG_PRINT (LOG_VERBOSE,
            "[PEV] receives CM_SLAC_PARM.CNF with wrong mmtype = 0x%X!!!\n",
            recv_mme->mmtype);
        return MME_ERROR_RESULT;
    }

    LOG_PRINT (LOG_VERBOSE, "the destination MAC of CM_SLAC_PARM.CNF = ");
    for (i = 0; i < 6; i++)
    {
        LOG_PRINT (LOG_VERBOSE, "0x%02x ", recv_mme->mme_dest[i]);
    }
    LOG_PRINT (LOG_VERBOSE, "\n");

    mme_remove_header (recv_mme->mmtype, raw_pkt, pkt_len, &cnf_ctx);

    slac_parm_cnf slac_cnf;
    memset (&slac_cnf, 0, sizeof (slac_cnf));
    if (mme_pull (&cnf_ctx, &slac_cnf, sizeof (slac_cnf), &num_len))
    {
        return MME_ERROR_ENOUGH;
    }
    if (memcmp (pev->run_id, slac_cnf.run_id, 8))
    {
        LOG_PRINT (LOG_VERBOSE, "run ID is not equal!\n");
        return MME_ERROR_RESULT;
    }
    else
    {
        insert_evse_item (pev->evse_item_head, recv_mme->mme_src);
        if (DEBUG_DISPLAY)
        {
            print_evse_item_list (pev->evse_item_head);
        }
        pev->potential_EVSE++;
        return MME_SUCCESS;
    }
}


int
pev_cm_start_atten_char_ind_send (void *opened_interface)
{
    mme_ctx_t req_ctx;
    unsigned char send_buff[ETH_DATA_LEN];
    unsigned int result_length;
    int i;

    if (mme_init (&req_ctx, CM_START_ATTEN_CHAR_IND,
        send_buff, ETH_DATA_LEN))
    {
        return MME_ERROR_NOT_INIT;
    }

    start_atten_char_ind start_ind =
    {
        .application_type = 0,
        .security_type = 0,
        .ACVarField.num_sounds = C_EV_match_MNBC,
        .ACVarField.time_out = TT_EVSE_match_MNBC,
        .ACVarField.resp_type = 1,
        .ACVarField.forwarding_sta = {0},
        .ACVarField.run_id = {1, 2, 3, 4, 5, 6, 7, 8}
    };
#ifdef _WIN32
    hpav_channel_t *channel = (hpav_channel_t *)opened_interface;
    memcpy (start_ind.ACVarField.forwarding_sta, channel->mac_addr,
            sizeof(start_ind.ACVarField.forwarding_sta));
#endif
#ifdef __linux__
    iface_t *iface = (iface_t *)opened_interface;
    memcpy(start_ind.ACVarField.forwarding_sta, iface->mac,
            sizeof(start_ind.ACVarField.forwarding_sta));
#endif

    if (mme_put (&req_ctx, &start_ind, sizeof (start_ind), &result_length))
    {
        return MME_ERROR_ENOUGH;
    }

    int c_ev_start_atten_char_ind = C_EV_start_atten_char_inds;
    for (i = 0; i < c_ev_start_atten_char_ind; i++)
    {
        LOG_PRINT (LOG_INFO, "[PEV] sends CM_START_ATTEN_CHAR.IND\n");
        sleep_ms (TP_EV_batch_msg_interval);

        if (mme_send
            (opened_interface, &req_ctx, MME_SEND_IND, mac_broadcast))
        {
            return MME_ERROR_TXRX;
        }
    }
    return MME_SUCCESS;
}


int
pev_cm_mnbc_sound_ind_send (void *opened_interface)
{
    mme_ctx_t req_ctx;
    unsigned char send_buff[ETH_DATA_LEN];
    unsigned int num_len = 0;
    int i;

    if (mme_init (&req_ctx, CM_MNBC_SOUND_IND, send_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;
    mnbc_sound_ind mnbc_ind =
    {
        .application_type = 0,
        .security_type = 0,
        .MSVarField.sender_ID = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa},
        .MSVarField.cnt = C_EV_match_MNBC,
        .MSVarField.run_id = {1, 2, 3, 4, 5, 6, 7, 8},
        .MSVarField.rsvd = {0},
        .MSVarField.rnd = {0}
    };

    int c_ev_match_mnbc = C_EV_match_MNBC;
    for (i = 0; i < c_ev_match_mnbc; i++)
    {
        mnbc_ind.MSVarField.cnt = c_ev_match_mnbc - i - 1;

        if (mme_put (&req_ctx, &mnbc_ind, sizeof (mnbc_ind), &num_len))
            return MME_ERROR_ENOUGH;
        sleep_ms (TP_EV_batch_msg_interval);

        LOG_PRINT (LOG_INFO, "[PEV] sends CM_MNBC_SOUND.IND---(%d)\n", i);

        if (mme_send (opened_interface, &req_ctx,
            MME_SEND_IND, mac_broadcast))
        {
            LOG_PRINT (LOG_VERBOSE, " [PEV] mme_send() error\n");
            return MME_ERROR_TXRX;
        }

        if (mme_init (&req_ctx, CM_MNBC_SOUND_IND, send_buff, ETH_DATA_LEN))
            return MME_ERROR_NOT_INIT;
    }
    return MME_SUCCESS;
}


int
pev_cm_atten_char_ind_receive (void *opened_interface, pev *pev,
                             atten_char_ind *atten_ind, unsigned char *src)
{
    unsigned char *pev_mac;
#ifdef _WIN32
    hpav_channel_t *channel = (hpav_channel_t *)opened_interface;
    pev_mac = channel->mac_addr;
#endif // _WIN32

#ifdef __linux__
    iface_t *iface = (iface_t *)opened_interface;
    pev_mac = iface->mac;
#endif // __linux__

    int i;
    unsigned char recv_buff[ETH_DATA_LEN];
    mme_ctx_t cnf_ctx;
    unsigned int pkt_len = 0, num_len = 0;
    unsigned char *raw_pkt;

    // rewind raw_pkt pointer to begining of the packet holder
    unsigned char pkt_holder[ETH_PACKET_MAX_NOVLAN_SIZE] = {0};
    raw_pkt = pkt_holder;

    if (mme_init (&cnf_ctx, CM_ATTEN_CHAR_IND, recv_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;

    if (mme_receive_timeout (opened_interface, raw_pkt, &pkt_len, 0, 30))
        return MME_ERROR_TIMEOUT;

    MME_t *recv_mme = (MME_t *) raw_pkt;
    if (mme_check_dest_mac (recv_mme->mme_dest, pev_mac))
    {
        return MME_ERROR_RESULT;
    }
    memcpy (src, recv_mme->mme_src, sizeof (recv_mme->mme_src));

    if (recv_mme->mmtype == CM_ATTEN_CHAR_IND)
    {
        LOG_PRINT (LOG_INFO, "[PEV] receives CM_ATTEN_CHAR.IND\n");
    }
    else
    {
        LOG_PRINT (LOG_DEBUG,
            "[PEV] receives CM_ATTEN_CHAR.IND with wrong mmtype = 0x%X!!!\n",
            recv_mme->mmtype);
        return MME_ERROR_RESULT;
    }

    LOG_PRINT (LOG_VERBOSE, " This ind was sent from \n");
    for (i = 0; i < 6; i++)
    {
        LOG_PRINT (LOG_VERBOSE, "0x%02x \n", recv_mme->mme_dest[i]);
    }
    LOG_PRINT (LOG_VERBOSE, "\n");

    mme_remove_header (recv_mme->mmtype, raw_pkt, pkt_len, &cnf_ctx);

    memset (atten_ind, 0, sizeof (*atten_ind));
    if (mme_pull (&cnf_ctx, atten_ind, sizeof (*atten_ind), &num_len))
    {
        LOG_PRINT (LOG_VERBOSE, "mme_pull() error\n");
        return MME_ERROR_ENOUGH;
    }

    if (memcmp (pev->run_id, atten_ind->ACVarField.run_id, 8))
    {
        LOG_PRINT (LOG_VERBOSE, "received run ID is not equal!\n");
        return MME_ERROR_RESULT;
    }
    /**
     *  It is up to the EV to decide what number of M-SOUNDs used for
     *  the attenuation profile is sufficient for its decision.
     *  Refs [V2G3-A09-36]
     */
    if (atten_ind->ACVarField.num_sounds < min_num_sounds)
    {
        LOG_PRINT (LOG_VERBOSE,
        "Received Number of M-Sounds %d is less than MIN_NUM_SOUNDS %d\n",
        atten_ind->ACVarField.num_sounds, min_num_sounds);

    }

    int average_AAG = 0;
    for (i = 0; i < attenuation_group; i++)
    {
        average_AAG += atten_ind->ACVarField.ATTEN_PROFILE.AAG[i];
    }
    average_AAG /= attenuation_group;
    LOG_PRINT (LOG_INFO, "Average Attenuation Value = %d\n", average_AAG);

    evse_item *target_evse =
        search_evse_item_list (pev->evse_item_head, recv_mme->mme_src);
    if (target_evse == NULL)
    {
        // did not find the specific EVSE
        LOG_PRINT (LOG_VERBOSE,
            "search_evse_list() didn't find the EVSE\n");
        // you need to insert_evse_item and set flag
        insert_evse_item (pev->evse_item_head, recv_mme->mme_src);

        // you don't have to add pev->potential_EVSE
        print_evse_item_list (pev->evse_item_head);

        evse_item *target_evse2 = search_evse_item_list
            (pev->evse_item_head, recv_mme->mme_src);
        target_evse2->send_cn_atten_char_ind_flag = 1;
        target_evse2->AAG = average_AAG;

        LOG_PRINT (LOG_VERBOSE,
            "after set the send_CM_ATTEN_CHAR_IND_flag\n");
        print_evse_item_list (pev->evse_item_head);
    }
    else
    {
        // you found the specific EVSE and then you should set the flag
        target_evse->send_cn_atten_char_ind_flag = 1;
        target_evse->AAG = average_AAG;
        pev->potential_EVSE--;

        LOG_PRINT (LOG_VERBOSE, "search_evse_item_list() return EVSE's MAC= \n");
        for (i = 0; i < 6; i++)
        {
            LOG_PRINT (LOG_VERBOSE, "0x%02x \n", target_evse->mac[i]);
        }
        LOG_PRINT (LOG_VERBOSE, "\n");
    }
    return MME_SUCCESS;
}


int
pev_cm_atten_char_rsp_send (void *opened_interface,
                         atten_char_ind *atten_ind, unsigned char *dst)
{
    mme_ctx_t req_ctx;
    unsigned char send_buff[ETH_DATA_LEN];
    unsigned int num_len = 0;

    LOG_PRINT (LOG_INFO, "[PEV] sends CM_ATTEN_CHAR.RSP\n");
    if (mme_init (&req_ctx, CM_ATTEN_CHAR_RSP, send_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;

    atten_char_rsp atten_rsp =
    {
        .application_type =  atten_ind->application_type,
        .security_type = atten_ind->security_type
    };
    memcpy (atten_rsp.ACVarField.source_address,
        atten_ind->ACVarField.source_address,
        sizeof (atten_ind->ACVarField.source_address));
    memcpy (atten_rsp.ACVarField.run_id,
        atten_ind->ACVarField.run_id,
        sizeof (atten_ind->ACVarField.run_id));
    memcpy (atten_rsp.ACVarField.source_ID,
        atten_ind->ACVarField.source_ID,
        sizeof (atten_ind->ACVarField.source_ID));
    memcpy (atten_rsp.ACVarField.resp_ID,
        atten_ind->ACVarField.resp_ID,
        sizeof (atten_ind->ACVarField.resp_ID));
    atten_rsp.ACVarField.result = 0;

    if (mme_put (&req_ctx, &atten_rsp, sizeof (atten_rsp), &num_len))
        return MME_ERROR_ENOUGH;

    if (mme_send (opened_interface, &req_ctx, MME_RECEIVE_IND_RSP, dst))
    {
        LOG_PRINT (LOG_VERBOSE, "[PEV] mme_send() error\n");
        return MME_ERROR_TXRX;
    }
    return MME_SUCCESS;
}


int
pev_cm_slac_match_req_send (void *opened_interface, evse_item *evse)
{
    unsigned char *pev_mac;
#ifdef _WIN32
    hpav_channel_t *channel = (hpav_channel_t *)opened_interface;
    pev_mac = channel->mac_addr;
#endif // _WIN32
#ifdef __linux__
    iface_t *iface = (iface_t *)opened_interface;
    pev_mac = iface->mac;
#endif // __linux__

    mme_ctx_t req_ctx;
    unsigned char send_buff[ETH_DATA_LEN];
    unsigned int num_len = 0;
    int i;

    LOG_PRINT (LOG_INFO, "[PEV] sends CM_SLAC_MATCH.REQ to ");
    for (i = 0; i < 6; i++)
    {
        LOG_PRINT (LOG_INFO, "%02x", evse->mac[i]);
        if (i < 5)   LOG_PRINT (LOG_INFO, ":");
    }
    LOG_PRINT (LOG_INFO, "\n");
    if (mme_init (&req_ctx, CM_SLAC_MATCH_REQ, send_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;
    slac_match_req match_req =
    {
        .application_type = 0 ,
        .security_type =  0 ,
        .mvflength[0] = 0x3e,
        .mvflength[1] = 0,
        .MatchVarField.run_id = {1, 2, 3, 4, 5, 6, 7, 8}
    };

    memcpy (match_req.MatchVarField.pev_mac, pev_mac,
        sizeof (match_req.MatchVarField.pev_mac));
    memcpy (match_req.MatchVarField.evse_mac, evse->mac,
        sizeof (match_req.MatchVarField.evse_mac));

    if (mme_put (&req_ctx, &match_req, sizeof (match_req), &num_len))
        return MME_ERROR_ENOUGH;
    if (mme_send (opened_interface, &req_ctx, MME_SEND_REQ_CNF, evse->mac))
    {
        LOG_PRINT (LOG_VERBOSE, "[PEV] mme_send() error\n");
        return MME_ERROR_TXRX;
    }
    return MME_SUCCESS;
}


int
pev_cm_slac_match_cnf_receive (void *opened_interface, pev *pev)
{
    unsigned char *pev_mac;
#ifdef _WIN32
    hpav_channel_t *channel = (hpav_channel_t *)opened_interface;
    pev_mac = channel->mac_addr;
#endif // _WIN32
#ifdef __linux__
    iface_t *iface = (iface_t *)opened_interface;
    pev_mac = iface->mac;
#endif // __linux__

    mme_ctx_t cnf_ctx;
    unsigned char recv_buff[ETH_DATA_LEN];
    unsigned int i, pkt_len = 0, num_len = 0;
    unsigned char *raw_pkt;

    //rewind raw_pkt pointer to begining of the packet holder
    unsigned char pkt_holder[ETH_PACKET_MAX_NOVLAN_SIZE] = {0};
    raw_pkt = pkt_holder;

    if (mme_init (&cnf_ctx, CM_SLAC_MATCH_CNF, recv_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;

    if (mme_receive_timeout (opened_interface, raw_pkt, &pkt_len, 0, 200))
    {
        return MME_ERROR_TIMEOUT;
    }

    MME_t *recv_mme = (MME_t *) raw_pkt;
    if (mme_check_dest_mac (recv_mme->mme_dest, pev_mac))
    {
        return MME_ERROR_RESULT;
    }

    if (recv_mme->mmtype == CM_SLAC_MATCH_CNF)
    {
        LOG_PRINT (LOG_INFO, "[PEV] receives CM_SLAC_MATCH.CNF\n");
    }
    else
    {
        LOG_PRINT (LOG_VERBOSE,
            "[PEV] receives CM_SLAC_MATCH.CNF with wrong mmtype!\n");
        return MME_ERROR_RESULT;
    }

    mme_remove_header (recv_mme->mmtype, raw_pkt, pkt_len, &cnf_ctx);

    slac_match_cnf match_cnf;
    memset (&match_cnf, 0, sizeof (match_cnf));
    if (mme_pull (&cnf_ctx, &match_cnf, sizeof (match_cnf), &num_len))
        return MME_ERROR_ENOUGH;

    if (memcmp (pev->run_id, match_cnf.MatchVarField.run_id, 8))
    {
        LOG_PRINT (LOG_VERBOSE, "run ID is not equal!\n");
        return MME_ERROR_RESULT;
    }

    memcpy (pev->received_nid, match_cnf.MatchVarField.nid,
            sizeof (match_cnf.MatchVarField.nid));
    memcpy (pev->received_nmk, match_cnf.MatchVarField.nmk,
            sizeof (match_cnf.MatchVarField.nmk));

    LOG_PRINT (LOG_VERBOSE, "received NID from EVSE = \n");
    for (i = 0; i < 7; i++)
    {
        LOG_PRINT (LOG_VERBOSE, "%02x \n", match_cnf.MatchVarField.nid[i]);
    }
    LOG_PRINT (LOG_VERBOSE, "received NMK from EVSE = \n");
    for (i = 0; i < 16; i++)
    {
        LOG_PRINT (LOG_VERBOSE, "%02x \n", match_cnf.MatchVarField.nmk[i]);
    }
    LOG_PRINT (LOG_VERBOSE, "\n");

    return MME_SUCCESS;
}
