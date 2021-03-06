/*	$NetBSD: if_gre.h,v 1.13 2003/11/10 08:51:52 wiz Exp $ */
/*	 $FreeBSD: src/sys/net/if_gre.h,v 1.13.10.2.4.1 2009/04/15 03:14:26 kensmith Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Heiko W.Rupp <hwr@pilhuhn.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _NET_IF_GRE_H
#define _NET_IF_GRE_H

/*
 * should include these
 */
#include <sys/systm.h>
#include <sys/appleapiopts.h>
#include <net/kpi_interface.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

#include "gre_config.h"

#define GRE_CONTROL_NAME "org.gmshake.nke.gre_control"

/*
 * Version of the WCCP, need to be configured manually since
 * header for version 2 is the same but IP payload is prepended
 * with additional 4-bytes field.
 */
typedef enum {
	WCCP_V1 = 0,
	WCCP_V2
} wccp_ver_t;

struct gre_softc {
    TAILQ_ENTRY(gre_softc)  sc_list;
    struct gre_softc        *pcb_next;
	ifnet_t             sc_ifp;
    volatile uint32_t   sc_refcnt;  /* reference count */
    lck_mtx_t           *mtx;		/* interface mutex */

    struct sockaddr gre_psrc; /* Physical src addr */
	struct sockaddr gre_pdst; /* Physical dst addr */
    
    u_int8_t    proto_flag; /* gre local address protocols */
#define AF_INET_PRESENT         0x01
#define AF_INET6_PRESENT        0x02
#define AF_APPLETALK_PRESENT    0x04
	u_int8_t    encap_proto;		/* protocol of encapsulator, IPPROTO_GRE, IPPROTO_MOBILE ... */
    u_int16_t   is_detaching;		/* state of the interface */
    
	uint32_t    called;		/* infinite recursion preventer */
	uint32_t    key;		/* key included in outgoing GRE packets */  /* zero means none */
	wccp_ver_t  wccp_ver;	/* version of the WCCP */
    
    bpf_packet_func bpf_input;
    bpf_packet_func bpf_output;
#if USE_IP_OUTPUT
    struct route route;   /* route used for ip_output */
#endif
};

struct gre_h {
	u_int16_t flags;	/* GRE flags */
	u_int16_t ptype;	/* protocol type of payload, typically Ether protocol type */
	uint32_t options[0];	/* optional options */
    /*
     *  from here on: fields are optional, presence indicated by flags
     *
     u_int_16 checksum	checksum (one-complements of GRE header
     and payload
     Present if (ck_pres | rt_pres == 1).
     Valid if (ck_pres == 1).
     u_int_16 offset		offset from start of routing filed to
     first octet of active SRE (see below).
     Present if (ck_pres | rt_pres == 1).
     Valid if (rt_pres == 1).
     u_int_32 key		inserted by encapsulator e.g. for
     authentication
     Present if (key_pres ==1 ).
     u_int_32 seq_num	Sequence number to allow for packet order
     Present if (seq_pres ==1 ).
     struct gre_sre[] routing Routing fileds (see below)
     Present if (rt_pres == 1)
     */
}__attribute__((__packed__));

struct greip {
	struct ip       gi_i;
	struct gre_h    gi_g;
}__attribute__((__packed__));

#ifdef DEPLOY
struct greip6 {
	struct ip6_hdr  gi_i;
	struct gre_h    gi_g;
}__attribute__((__packed__));
#endif

#define gi_ver      gi_i.ip_v
#define gi_hlen     gi_i.ip_hl
#define gi_pr		gi_i.ip_p
#define gi_len		gi_i.ip_len
#define gi_src		gi_i.ip_src
#define gi_dst		gi_i.ip_dst
#define gi_sum      gi_i.ip_sum

#define gi_ptype	gi_g.ptype
#define gi_flags	gi_g.flags
#define gi_options	gi_g.options


#define GRE_CP		0x8000  /* Checksum Present */
#define GRE_RP		0x4000  /* Routing Present */
#define GRE_KP		0x2000  /* Key Present */
#define GRE_SP		0x1000  /* Sequence Present */
#define GRE_SS		0x0800	/* Strict Source Route */

/*
 * CISCO uses special type for GRE tunnel created as part of WCCP
 * connection, while in fact those packets are just IPv4 encapsulated
 * into GRE.
 */
#define WCCP_PROTOCOL_TYPE	0x883E

/*
 * gre_sre defines a Source route Entry. These are needed if packets
 * should be routed over more than one tunnel hop by hop
 */
struct gre_sre {
	u_int16_t sre_family;   /* address family */
	u_char	sre_offset;     /* offset to first octet of active entry */
	u_char	sre_length;     /* number of octets in the SRE. sre_lengthl==0 -> last entry. */
	u_char	*sre_rtinfo;    /* the routing information */
};

struct greioctl {
	int unit;
	struct in_addr addr;
};

/* for mobile encaps */

struct mobile_h {
	u_int16_t proto;		/* protocol and S-bit */
	u_int16_t hcrc;			/* header checksum */
	u_int32_t odst;			/* original destination address */
	u_int32_t osrc;			/* original source addr, if S-bit set */
}__attribute__((__packed__));

struct mobip_h {
	struct ip	mi;
	struct mobile_h	mh;
}__attribute__((__packed__));


#define MOB_H_SIZ_S		(sizeof(struct mobile_h) - sizeof(u_int32_t))
#define MOB_H_SIZ_L		(sizeof(struct mobile_h))
#define MOB_H_SBIT	0x0080

#define	GRE_TTL	30
#define GRE_MAXUNIT	0x7fff	/* ifp->if_unit is only 15 bits(short int) */

#ifndef IPPROTO_MOBILE
#define IPPROTO_MOBILE          55              /* IP Mobility */
#endif


/*
 * ioctls needed to manipulate the interface
 */

#define GRESADDRS	_IOW('i', 101, struct ifreq)
#define GRESADDRD	_IOW('i', 102, struct ifreq)
#define GREGADDRS	_IOWR('i', 103, struct ifreq)
#define GREGADDRD	_IOWR('i', 104, struct ifreq)
#define GRESPROTO	_IOW('i' , 105, struct ifreq)
#define GREGPROTO	_IOWR('i', 106, struct ifreq)
#define GREGKEY		_IOWR('i', 107, struct ifreq)
#define GRESKEY		_IOW('i', 108, struct ifreq)

/*
 * usefull macro
 */
#ifndef in_hosteq
#define in_hosteq(s, t) ((s).s_addr == (t).s_addr)
#endif

#ifndef satosin
#define satosin(sa)     ((struct sockaddr_in *)(sa))
#endif

#ifndef sintosa
#define sintosa(sin)    ((struct sockaddr *)(sin))
#endif

extern void gre_sc_reference(struct gre_softc *sc);
extern void gre_sc_release(struct gre_softc *sc);

extern int gre_proto_register(void);
extern void gre_proto_unregister(void);

extern int gre_if_init(void);
extern int gre_if_dispose(void);
extern int gre_if_attach(void);

extern u_int16_t    gre_in_cksum(u_int16_t *p, u_int len);


#endif
