/*  plc hle utils project {{{
 *
 * Copyright (C) 2018 MStar Semiconductor
 *
 * <<<Licence>>>
 *
 * }}} */


#include "mstar_vs.h"

int
vs_get_status_req_send (void *opened_interface)
{
    mme_ctx_t req_ctx;
    unsigned char send_buff[ETH_DATA_LEN];

    if (mme_init (&req_ctx, VS_GET_STATUS_REQ, send_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;
    if (mme_send (opened_interface, &req_ctx, MME_SEND_REQ_CNF, mac_link_local))
    {
        return MME_ERROR_TXRX;
    }
    else
    {
        LOG_PRINT (LOG_DEBUG, "[EVSE] send VS_GET_STATUS.REQ\n");
        return MME_SUCCESS;
    }
}


int
vs_get_status_cnf_receive (void *opened_interface)
{
    mme_ctx_t cnf_ctx;
    unsigned char recv_buff[ETH_DATA_LEN];
    unsigned int num_len = 0, pkt_len = 0;
    mme_error_t result;
    unsigned char *raw_pkt;

    //rewind raw_pkt pointer to begining of the packet holder
    unsigned char pkt_holder[ETH_PACKET_MAX_NOVLAN_SIZE] = {0};
    raw_pkt = pkt_holder;

    if (mme_init (&cnf_ctx, VS_GET_STATUS_CNF, recv_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;

    // TT_match_join max = 12 sec, TP_link_ready_notification max = 1 sec
    if (mme_receive_timeout (opened_interface, raw_pkt, &pkt_len, 13, 0))
        return ERROR_LINK;

    MME_t *recv_mme = (MME_t *)raw_pkt;

    if (recv_mme->mmtype == VS_GET_STATUS_CNF)
    {
        LOG_PRINT (LOG_DEBUG, "Receives VS_GET_STATUS.CNF\n");
    }
    else
    {
        LOG_PRINT (LOG_DEBUG, "Fails to receive VS_GET_STATUS.CNF \
with wrong mmtype \n");
        return ERROR_LINK;
    }

    mme_remove_header (recv_mme->mmtype, raw_pkt, pkt_len, &cnf_ctx);

    vs_get_status_cnf vs_cnf;
    memset (&vs_cnf, 0, sizeof (vs_cnf));
    result = mme_pull (&cnf_ctx, &vs_cnf, sizeof (vs_cnf), &num_len);
    if (result != MME_SUCCESS)
    {
        LOG_PRINT (LOG_VERBOSE, "mme_pull() error, result=%d\n", result);
        return ERROR_LINK;
    }
    LOG_PRINT (LOG_DEBUG, "VS_GET_STATUS_CNF.Link_State = %d \
(1:connected, 0:disconnected)\n", vs_cnf.link_state);
    return vs_cnf.link_state;       // 1:connected,  0:disconnected
}


int
vs_get_status_req_send_cnf_receive (void *opened_interface)
{

    mme_ctx_t cnf_ctx, req_ctx;
    unsigned char recv_buff[ETH_DATA_LEN], send_buff[ETH_DATA_LEN];
    unsigned int num_len=0, pkt_len=0;
    unsigned char *raw_pkt;

    LOG_PRINT (LOG_DEBUG, "Sends VS_GET_STATUS.REQ\n");

    if (mme_init (&req_ctx, VS_GET_STATUS_REQ, send_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;
    if (mme_send (opened_interface, &req_ctx,
        MME_SEND_REQ_CNF, mac_link_local))
        return MME_ERROR_TXRX;

    //rewind raw_pkt pointer to begining of the packet holder
    unsigned char pkt_holder[ETH_PACKET_MAX_NOVLAN_SIZE] = {0};
    raw_pkt = pkt_holder;

    if (mme_init (&cnf_ctx, VS_GET_STATUS_CNF, recv_buff, ETH_DATA_LEN))
        return MME_ERROR_NOT_INIT;
    if (mme_receive_timeout (opened_interface, raw_pkt, &pkt_len, 1, 0))
        return ERROR_LINK;
    MME_t *recv_mme = (MME_t *) raw_pkt;

    if (recv_mme->mmtype == VS_GET_STATUS_CNF)
    {
        LOG_PRINT (LOG_DEBUG, "Receives VS_GET_STATUS.CNF\n");
    }
    else
    {
        LOG_PRINT (LOG_DEBUG, "Fails to receive VS_GET_STATUS.CNF \
with wrong mmtype\n");
        return ERROR_LINK;
    }
    mme_remove_header (recv_mme->mmtype, raw_pkt, pkt_len, &cnf_ctx);

    vs_get_status_cnf vs_cnf;
    memset (&vs_cnf, 0, sizeof (vs_cnf));
    if (mme_pull (&cnf_ctx, &vs_cnf, sizeof (vs_cnf), &num_len))
    {
        LOG_PRINT (LOG_VERBOSE, "mme_pull() error\n");
        return ERROR_LINK;
    }
    LOG_PRINT (LOG_DEBUG, "VS_GET_STATUS_CNF.Link_State = %d \
(1:connected, 0:disconnected)\n", vs_cnf.link_state);
    return 0;
}
