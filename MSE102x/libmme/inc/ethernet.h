#ifndef libmme_inc_ethernet_h
#define libmme_inc_ethernet_h
/*  plc hle utils project {{{
 *
 * Copyright (C) 2018 MStar Semiconductor
 *
 * <<<Licence>>>
 *
 * }}} */
/**
 * \file    common/include/ethernet.h
 * \brief   Defines concerning the Ethernet Norme.
 * \ingroup common_defs
 *
 * Based on the norme 802.3
 * Information: chapter 12.3.2.1.1 in homeplugav specification.
 */

/** HomePlug AV packet           */
#define ETH_P_HPAV       0x88E1

/** Ethernet Mac Address size. */
#define ETH_MAC_ADDRESS_SIZE 6

/** Ethernet type IP. */
#define ETH_TYPE_IP 0x0800

/** Define the Ethernet type for one VLAN, in little endian */
#define ETH_TYPE_VLAN 0x0081

/** Ethernet type 802.1ad. */
#define ETH_TYPE_8021AD 0x88A8

/** Ethernet type QinQ. */
#define ETH_TYPE_QINQ 0x9100

/** Define the MTYPE size. */
#define ETH_TYPE_SIZE 2

/** Ethernet VLAN Tag complete size. */
#define ETH_VLANTAG_SIZE 4

/** Vlan size returned depend of the eth_type.
 * eth_type must be in little endian (no swap16 so).
 * Twice vlan is not added, because the plc driver don't managed it. */
#define ETH_GET_VLANTAG_SIZE(a) ( \
     (a) == (ETH_TYPE_VLAN) ?  ETH_VLANTAG_SIZE : 0 )

/** Macro which return true, if a simple vlan is detected. */
#define ETH_IS_VLANTAG(a) \
    ((a) == (ETH_TYPE_VLAN) ? true : false)

/** Ethernet packet minimum payload size. */
#define ETH_PACKET_MIN_PAYLOAD_SIZE 46

/** Ethernet packet maximum payload size. */
#define ETH_PACKET_MAX_PAYLOAD_SIZE 1500

/** Define the ethernet type offset (=12). */
#define ETH_TYPE_OFFSET ((ETH_MAC_ADDRESS_SIZE)*2)

/** Ethernet minimum size allowed (=14). */
#define ETH_PACKET_MIN_SIZE_ALLOWED (ETH_TYPE_OFFSET + ETH_TYPE_SIZE)

/** Ethernet packet minimum size without vlan (=60). */
#define ETH_PACKET_MIN_NOVLAN_SIZE (ETH_PACKET_MIN_SIZE_ALLOWED \
                                    + ETH_PACKET_MIN_PAYLOAD_SIZE)

/** Ethernet packet minimum size with one vlan (=64). */
#define ETH_PACKET_MIN_VLAN_SIZE (ETH_PACKET_MIN_NOVLAN_SIZE \
                                  + ETH_VLANTAG_SIZE)

/** Ethernet packet minimum size with twice vlan (=68). */
#define ETH_PACKET_MIN_TWICE_VLAN_SIZE (ETH_PACKET_MIN_NOVLAN_SIZE \
                                            + 2*(ETH_VLANTAG_SIZE))

/** Ethernet packet minimum of minimum size. */
#define ETH_PACKET_MIN_SIZE (ETH_PACKET_MIN_NOVLAN_SIZE)


/** Ethernet packet maximum size without vlan (=1514). */
#define ETH_PACKET_MAX_NOVLAN_SIZE (ETH_PACKET_MIN_SIZE_ALLOWED \
                                + ETH_PACKET_MAX_PAYLOAD_SIZE)

/** Ethernet packet maximum size with one vlan (=1518). */
#define ETH_PACKET_MAX_VLAN_SIZE (ETH_PACKET_MAX_NOVLAN_SIZE \
                                  + ETH_VLANTAG_SIZE)

/** Ethernet packet maximum size with twice vlan (=1522). */
#define ETH_PACKET_MAX_TWICE_VLAN_SIZE (ETH_PACKET_MAX_NOVLAN_SIZE \
                                        + 2*(ETH_VLANTAG_SIZE))

/** Return ethernet packet min size. */
#define ETH_PACKET_GET_MIN_SIZE(eth_type) \
    (ETH_PACKET_MIN_NOVLAN_SIZE + ETH_GET_VLANTAG_SIZE(eth_type))

/** Return ethernet packet max size. */
#define ETH_PACKET_GET_MAX_SIZE(eth_type) \
    (ETH_PACKET_MAX_NOVLAN_SIZE + ETH_GET_VLANTAG_SIZE(eth_type))

#endif /* libmme_inc_ethernet_h */
