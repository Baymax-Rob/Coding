#ifndef libmme_inc_mmtype_h
#define libmme_inc_mmtype_h
/*  plc hle utils project {{{
 *
 * Copyright (C) 2018 MStar Semiconductor
 *
 * <<<Licence>>>
 *
 * }}} */

#define MMTYPE_BASE(mmtype) ((mmtype) & 0xFFFC)

#define CC_MIN 0x0000
#define CC_MAX 0x1fff
#define CP_MIN 0x2000
#define CP_MAX 0x3fff
#define NN_MIN 0x4000
#define NN_MAX 0x5fff
#define CM_MIN 0x6000
#define CM_MAX 0x7fff
#define DRV_MIN 0xb000
#define DRV_MAX 0xbfff
#define VS_MIN 0xa000
#define VS_MAX 0xafff
#define MS_MIN 0x8000
#define MS_MAX 0x801c
#define IMAC_MIN 0xa800
#define IMAC_MAX 0xa804


#define MMTYPE_IS_CC(mmtype) \
                ((mmtype) >= CC_MIN && (mmtype) <= CC_MAX)
#define MMTYPE_IS_CP(mmtype) \
                ((mmtype) >= CP_MIN && (mmtype) <= CP_MAX)
#define MMTYPE_IS_NN(mmtype) \
                ((mmtype) >= NN_MIN && (mmtype) <= NN_MAX)
#define MMTYPE_IS_CM(mmtype) \
                ((mmtype) >= CM_MIN && (mmtype) <= CM_MAX)
#define MMTYPE_IS_DRV(mmtype) \
                ((mmtype) >= DRV_MIN && (mmtype) <= DRV_MAX)
#define MMTYPE_IS_VS(mmtype) \
                ((mmtype) >= VS_MIN && (mmtype) <= VS_MAX)
#define MMTYPE_IS_MS(mmtype) \
                ((mmtype) >= MS_MIN && (mmtype) <= MS_MAX)
#define MMTYPE_IS_IMAC(mmtype) \
                ((mmtype) >= IMAC_MIN && (mmtype) <= IMAC_MAX)


#define CC_CCO_APPOINT 0x0000
#define CC_LINK_INFO 0x0008
#define CC_DISCOVER_LIST 0x0014
#define CC_WHO_RU 0x002C
#define CM_UNASSOCIATED_STA 0x6000
#define CM_ENCRYPTED_PAYLOAD 0x6004
#define CM_SET_KEY 0x6008
#define CM_GET_KEY 0x600C
#define CM_AMP_MAP 0x601C
#define CM_BRG_INFO 0x6020


#define CM_CONN_INFO 0x6030
#define CM_STA_CAP 0x6034
#define CM_NW_INFO 0x6038
#define CM_GET_BEACON 0x603C
#define CM_HFID 0x6040
#define CM_MME_ERROR 0x6044
#define CM_MNBC_SOUND 0x6074
#define CM_ATTEN_PROFILE 0x6084
#define CM_SLAC_PARM 0x6064
#define CM_SLAC_MATCH 0x607c
#define CM_START_ATTEN_CHAR 0x6068
#define CM_ATTEN_CHAR 0x606c
#define VS_SET_KEY 0xA00C
#define VS_GET_STATUS 0xA030
#define MME_CNF 0x01
#define MME_IND 0x02
#define MME_REQ 0x00
#define MME_RSP 0x03


enum mmtype_t
{
    CC_CCO_APPOINT_REQ = CC_CCO_APPOINT + MME_REQ,
    CC_CCO_APPOINT_CNF = CC_CCO_APPOINT + MME_CNF,
    CC_LINK_INFO_REQ = CC_LINK_INFO + MME_REQ,
    CC_LINK_INFO_CNF = CC_LINK_INFO + MME_CNF,
    CC_LINK_INFO_IND = CC_LINK_INFO + MME_IND,
    CC_LINK_INFO_RSP = CC_LINK_INFO + MME_RSP,
    CC_DISCOVER_LIST_REQ = CC_DISCOVER_LIST + MME_REQ,
    CC_DISCOVER_LIST_CNF = CC_DISCOVER_LIST + MME_CNF,
    CC_DISCOVER_LIST_IND = CC_DISCOVER_LIST + MME_IND,
    CC_WHO_RU_REQ = CC_WHO_RU + MME_REQ,
    CC_WHO_RU_CNF = CC_WHO_RU + MME_CNF,
    CM_UNASSOCIATED_STA_IND = CM_UNASSOCIATED_STA + MME_IND,
    CM_ENCRYPTED_PAYLOAD_IND = CM_ENCRYPTED_PAYLOAD + MME_IND,
    CM_ENCRYPTED_PAYLOAD_RSP = CM_ENCRYPTED_PAYLOAD + MME_RSP,
    CM_SET_KEY_REQ = CM_SET_KEY + MME_REQ,
    CM_SET_KEY_CNF = CM_SET_KEY + MME_CNF,
    CM_GET_KEY_REQ = CM_GET_KEY + MME_REQ,
    CM_GET_KEY_CNF = CM_GET_KEY + MME_CNF,
    CM_SLAC_PARM_REQ = CM_SLAC_PARM + MME_REQ,
    CM_SLAC_PARM_CNF = CM_SLAC_PARM + MME_CNF,
    CM_SLAC_MATCH_REQ = CM_SLAC_MATCH + MME_REQ,
    CM_SLAC_MATCH_CNF = CM_SLAC_MATCH + MME_CNF,
    CM_START_ATTEN_CHAR_IND = CM_START_ATTEN_CHAR + MME_IND,
    CM_ATTEN_CHAR_IND = CM_ATTEN_CHAR + MME_IND,
    CM_ATTEN_CHAR_RSP = CM_ATTEN_CHAR + MME_RSP,
    CM_ATTEN_PROFILE_IND = CM_ATTEN_PROFILE + MME_IND,
    CM_AMP_MAP_REQ = CM_AMP_MAP + MME_REQ,
    CM_AMP_MAP_CNF = CM_AMP_MAP + MME_CNF,
    CM_BRG_INFO_REQ = CM_BRG_INFO + MME_REQ,
    CM_BRG_INFO_CNF = CM_BRG_INFO + MME_CNF,
    CM_CONN_INFO_REQ = CM_CONN_INFO + MME_REQ,
    CM_CONN_INFO_CNF = CM_CONN_INFO + MME_CNF,
    CM_STA_CAP_REQ = CM_STA_CAP + MME_REQ,
    CM_STA_CAP_CNF = CM_STA_CAP + MME_CNF,
    CM_NW_INFO_REQ = CM_NW_INFO + MME_REQ,
    CM_NW_INFO_CNF = CM_NW_INFO + MME_CNF,
    CM_GET_BEACON_REQ = CM_GET_BEACON + MME_REQ,
    CM_GET_BEACON_CNF = CM_GET_BEACON + MME_CNF,
    CM_HFID_REQ = CM_HFID + MME_REQ,
    CM_HFID_CNF = CM_HFID + MME_CNF,
    CM_MME_ERROR_IND = CM_MME_ERROR + MME_IND,
    CM_MNBC_SOUND_IND = CM_MNBC_SOUND + MME_IND,
    VS_SET_KEY_REQ = VS_SET_KEY + MME_REQ,
    VS_SET_KEY_CNF = VS_SET_KEY + MME_CNF,
    VS_GET_STATUS_REQ = VS_GET_STATUS + MME_REQ,
    VS_GET_STATUS_CNF = VS_GET_STATUS + MME_CNF,

};
typedef enum mmtype_t mmtype_t;

#endif /* libmme_inc_mmtype_h */
