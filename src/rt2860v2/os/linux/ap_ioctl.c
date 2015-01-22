/****************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ****************************************************************************

    Module Name:
	ap_ioctl.c

    Abstract:
    IOCTL related subroutines

    Revision History:
    Who          When          What
    ---------    ----------    ----------------------------------------------
*/
#define RTMP_MODULE_OS

/*#include "rt_config.h" */
#include "rtmp_comm.h"
#include "rt_os_util.h"
#include "rt_os_net.h"
#include <linux/wireless.h>

extern
int rt_ioctl_giwscan(struct net_device *dev,
                        struct iw_request_info *info,
                        struct iw_point *data, char *extra);

struct iw_priv_args ap_privtab[] = {
{ RTPRIV_IOCTL_SET, 
/* 1024 --> 1024 + 512 */
/* larger size specific to allow 64 ACL MAC addresses to be set up all at once. */
  IW_PRIV_TYPE_CHAR | 1536, 0,
  "set"},  
{ RTPRIV_IOCTL_SHOW,
  IW_PRIV_TYPE_CHAR | 1024, 0,
  "show"},
{ RTPRIV_IOCTL_GSITESURVEY,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024 ,
  "get_site_survey"}, 
#ifdef INF_AR9
  { RTPRIV_IOCTL_GET_AR9_SHOW,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024 ,
  "ar9_show"}, 
#endif
  { RTPRIV_IOCTL_SET_WSCOOB,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024 ,
  "set_wsc_oob"}, 
{ RTPRIV_IOCTL_GET_MAC_TABLE,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024 ,
  "get_mac_table"}, 
{ RTPRIV_IOCTL_E2P,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024,
  "e2p"},
#ifdef DBG
{ RTPRIV_IOCTL_BBP,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024,
  "bbp"},
{ RTPRIV_IOCTL_MAC,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024,
  "mac"},
#ifdef RTMP_RF_RW_SUPPORT
{ RTPRIV_IOCTL_RF,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024,
  "rf"},
#endif /* RTMP_RF_RW_SUPPORT */
#endif /* DBG */

#ifdef WSC_AP_SUPPORT
{ RTPRIV_IOCTL_WSC_PROFILE,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024 ,
  "get_wsc_profile"},
#endif /* WSC_AP_SUPPORT */
{ RTPRIV_IOCTL_QUERY_BATABLE,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024 ,
  "get_ba_table"},
{ RTPRIV_IOCTL_STATISTICS,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024,
  "stat"},
{ RTPRIV_IOCTL_GET_APCLI_CONNSTATUS,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024,
  "Connstatus"}
};

static const iw_handler rt_ap_handler[]=
{
	[(SIOCGIWSCAN-SIOCSIWCOMMIT)] = (iw_handler) rt_ioctl_giwscan,
//	[(SIOCSIWSCAN-SIOCSIWCOMMIT)] = (iw_handler) rt_ioctl_ap_siwscan,
};


const struct iw_handler_def rt28xx_ap_iw_handler_def =
{
	.standard = (iw_handler *) rt_ap_handler,
#define	N(a)	(sizeof (a) / sizeof (a[0]))
	.num_standard = sizeof(rt_ap_handler) / sizeof(iw_handler),
	.private_args	= (struct iw_priv_args *) ap_privtab,
	.num_private_args	= N(ap_privtab),
#if IW_HANDLER_VERSION >= 7
	.get_wireless_stats = rt28xx_get_wireless_stats,
#endif 
};


INT rt28xx_ap_ioctl(
	IN	struct net_device	*net_dev, 
	IN	OUT	struct ifreq	*rq, 
	IN	INT					cmd)
{
	VOID			*pAd = NULL;
    struct iwreq	*wrqin = (struct iwreq *) rq;
	RTMP_IOCTL_INPUT_STRUCT rt_wrq, *wrq = &rt_wrq;
    INT				Status = NDIS_STATUS_SUCCESS;
    USHORT			subcmd; /*, index; */
/*	POS_COOKIE		pObj; */
	INT			apidx=0;
	UINT32		org_len;
	RT_CMD_AP_IOCTL_CONFIG IoctlConfig, *pIoctlConfig = &IoctlConfig;

	GET_PAD_FROM_NET_DEV(pAd, net_dev);	
/*	pObj = (POS_COOKIE) pAd->OS_Cookie; */

	if (pAd == NULL)
	{
		/* if 1st open fail, pAd will be free;
		   So the net_dev->priv will be NULL in 2rd open */
		return -ENETDOWN;
	}

	wrq->u.data.pointer = wrqin->u.data.pointer;
	wrq->u.data.length = wrqin->u.data.length;
	org_len = wrq->u.data.length;

	pIoctlConfig->Status = 0;
	pIoctlConfig->net_dev = net_dev;
	pIoctlConfig->priv_flags = RT_DEV_PRIV_FLAGS_GET(net_dev);
	pIoctlConfig->pCmdData = wrqin->u.data.pointer;
	pIoctlConfig->CmdId_RTPRIV_IOCTL_SET = RTPRIV_IOCTL_SET;
	pIoctlConfig->name = net_dev->name;
	pIoctlConfig->apidx = 0;

	if ((cmd != SIOCGIWPRIV) &&
		RTMP_AP_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_PREPARE, 0,
							pIoctlConfig, 0) != NDIS_STATUS_SUCCESS)
	{
		/* prepare error */
		Status = pIoctlConfig->Status;
		goto LabelExit;
	}

	apidx = pIoctlConfig->apidx;
	
    /*+ patch for SnapGear Request even the interface is down */
    if(cmd== SIOCGIWNAME){
	    DBGPRINT(RT_DEBUG_TRACE, ("IOCTL::SIOCGIWNAME\n"));

	RTMP_COM_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_SIOCGIWNAME, 0, wrqin->u.name, 0);

	    return Status;
    }/*- patch for SnapGear */


	switch(cmd)
	{
		case RTPRIV_IOCTL_ATE:
			{
				RTMP_COM_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_ATE, 0, wrqin->ifr_name, 0);
			}
			break;

		case SIOCGIFHWADDR:
			DBGPRINT(RT_DEBUG_TRACE, ("IOCTLIOCTLIOCTL::SIOCGIFHWADDR\n"));
			RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_AP_SIOCGIFHWADDR, 0, NULL, 0);
/*            if (pObj->ioctl_if < MAX_MBSSID_NUM(pAd)) */
/*    			strcpy((PSTRING) wrq->u.name, (PSTRING) pAd->ApCfg.MBSSID[pObj->ioctl_if].Bssid); */
			break;
		case SIOCSIWESSID:  /*Set ESSID */
			break;
		case SIOCGIWESSID:  /*Get ESSID */
			{
				RT_CMD_AP_IOCTL_SSID IoctlSSID, *pIoctlSSID = &IoctlSSID;
				struct iw_point *erq = &wrqin->u.essid;
				PCHAR pSsidStr = NULL;

				erq->flags=1;
              /*erq->length = pAd->ApCfg.MBSSID[pObj->ioctl_if].SsidLen; */

				pIoctlSSID->priv_flags = RT_DEV_PRIV_FLAGS_GET(net_dev);
				pIoctlSSID->apidx = apidx;
				RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_AP_SIOCGIWESSID, 0, pIoctlSSID, 0);

				pSsidStr = (PCHAR)pIoctlSSID->pSsidStr;
				erq->length = pIoctlSSID->length;


				if((erq->pointer) && (pSsidStr != NULL))
				{
					/*if(copy_to_user(erq->pointer, pAd->ApCfg.MBSSID[pObj->ioctl_if].Ssid, erq->length)) */
					if(copy_to_user(erq->pointer, pSsidStr, erq->length))
					{
						Status = RTMP_IO_EFAULT;
						break;
					}
				}
				DBGPRINT(RT_DEBUG_TRACE, ("IOCTL::SIOCGIWESSID (Len=%d, ssid=%s...)\n", erq->length, (char *)erq->pointer));
			}
			break;
		case SIOCGIWNWID: /* get network id */
		case SIOCSIWNWID: /* set network id (the cell) */
			Status = RTMP_IO_EOPNOTSUPP;
			break;
		case SIOCGIWFREQ: /* get channel/frequency (Hz) */
		{
			ULONG Channel;
			RTMP_DRIVER_CHANNEL_GET(pAd, &Channel);
			wrqin->u.freq.m = Channel; /*pAd->CommonCfg.Channel; */
			wrqin->u.freq.e = 0;
			wrqin->u.freq.i = 0;
			DBGPRINT(RT_DEBUG_TRACE, ("IOCTL::SIOCGIWFREQ:(%d)\n", Channel));

		}
			break; 
		case SIOCSIWFREQ: /*set channel/frequency (Hz) */
			Status = RTMP_IO_EOPNOTSUPP;
			break;
		case SIOCGIWNICKN:
		case SIOCSIWNICKN: /*set node name/nickname */
			Status = RTMP_IO_EOPNOTSUPP;
			break;
		case SIOCGIWRATE:  /*get default bit rate (bps) */
            {
				RT_CMD_IOCTL_RATE IoctlRate, *pIoctlRate = &IoctlRate;

				pIoctlRate->priv_flags = RT_DEV_PRIV_FLAGS_GET(net_dev);
				RTMP_DRIVER_BITRATE_GET(pAd, pIoctlRate);


			wrqin->u.bitrate.value = pIoctlRate->BitRate;
			wrqin->u.bitrate.disabled = 0;
            }
			break;
		case SIOCSIWRATE:  /*set default bit rate (bps) */
		case SIOCGIWRTS:  /* get RTS/CTS threshold (bytes) */
		case SIOCSIWRTS:  /*set RTS/CTS threshold (bytes) */
		case SIOCGIWFRAG:  /*get fragmentation thr (bytes) */
		case SIOCSIWFRAG:  /*set fragmentation thr (bytes) */
			Status = RTMP_IO_EOPNOTSUPP;	
			break;			
		case SIOCGIWENCODE:  /*get encoding token & mode */
		{
			//printk("==> SIOCGIWENCODE\n");

			RT_CMD_STA_IOCTL_SECURITY IoctlSec, *pIoctlSec = &IoctlSec;
			int max_key_len;
			struct iw_point *encoding = &wrqin->u.encoding;
			
			 max_key_len = encoding->length /*- sizeof(*ext)*/;
			 if (max_key_len < 0)
                	 {
				Status = RTMP_IO_EOPNOTSUPP;
			 	break;
			 }
			
			 memset(pIoctlSec, 0, sizeof(RT_CMD_STA_IOCTL_SECURITY));
			 pIoctlSec->KeyIdx = encoding->flags & IW_ENCODE_INDEX;
			 pIoctlSec->MaxKeyLen = max_key_len;

			if (RTMP_AP_IoctlHandle(pAd, NULL, CMD_RTPRIV_IOCTL_AP_SIOCGIWENCODEEXT, 0,
                                pIoctlSec, RT_DEV_PRIV_FLAGS_GET(net_dev)) != NDIS_STATUS_SUCCESS)
        		{
                		//ext->key_len = 0;
                		//RT_CMD_STATUS_TRANSLATE(pIoctlSec->Status);
                		//return pIoctlSec->Status;
				Status = RTMP_IO_EOPNOTSUPP;
				break;
        		}

			encoding->flags = pIoctlSec->KeyIdx;
			encoding->length = pIoctlSec->length;

			if (pIoctlSec->Alg == RT_CMD_STA_IOCTL_SECURITY_ALG_NONE)
                		encoding->flags |= IW_ENCODE_ALG_NONE;
        		else if (pIoctlSec->Alg == RT_CMD_STA_IOCTL_SECURITY_ALG_WEP)
                		encoding->flags |= IW_ENCODE_ALG_WEP;
        		else if (pIoctlSec->Alg == RT_CMD_STA_IOCTL_SECURITY_ALG_TKIP)
                		encoding->flags |= IW_ENCODE_ALG_TKIP;
        		else if (pIoctlSec->Alg == RT_CMD_STA_IOCTL_SECURITY_ALG_CCMP)
                		encoding->flags |= IW_ENCODE_ALG_CCMP;

        		if (pIoctlSec->flags & RT_CMD_STA_IOCTL_SECURITY_DISABLED)
                		encoding->flags |= IW_ENCODE_DISABLED;

			if (pIoctlSec->length && pIoctlSec->pData)
			{
				encoding->flags |= IW_ENCODE_ENABLED;
				memcpy(encoding->pointer, pIoctlSec->pData, encoding->length);
			}
		}
			break;			
		case SIOCSIWENCODE:  /*set encoding token & mode */
			Status = RTMP_IO_EOPNOTSUPP;
			break;
		case SIOCGIWAP:  /*get access point MAC addresses */
			{
/*				PCHAR pBssidStr; */

				wrqin->u.ap_addr.sa_family = ARPHRD_ETHER;
				/*memcpy(wrqin->u.ap_addr.sa_data, &pAd->ApCfg.MBSSID[pObj->ioctl_if].Bssid, ETH_ALEN); */

				RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_AP_SIOCGIWAP, 0,
								wrqin->u.ap_addr.sa_data, RT_DEV_PRIV_FLAGS_GET(net_dev));
			}
			break;
		case SIOCGIWMODE:  /*get operation mode */
			wrqin->u.mode = IW_MODE_INFRA;   /*SoftAP always on INFRA mode. */
			DBGPRINT(RT_DEBUG_TRACE,("ioctl SIOCGIWMODE=%d\n", wrqin->u.mode));			
			break;
		case SIOCSIWAP:  /*set access point MAC addresses */
		case SIOCSIWMODE:  /*set operation mode */
		case SIOCGIWSENS:   /*get sensitivity (dBm) */
		case SIOCSIWSENS:	/*set sensitivity (dBm) */
			break;	
		case SIOCGIWPOWER:  /*get Power Management settings */
		{
			DBGPRINT(RT_DEBUG_ERROR,("SIOCGIWPOWER\n"));	
                        break;		
		}			
		case SIOCSIWPOWER:  /*set Power Management settings */
			break;
		case SIOCGIWTXPOW:  /*get transmit power (dBm) */
		{
			//DBGPRINT(RT_DEBUG_ERROR,("SIOCGIWTXPOW\n"));	
			int len;
			UINT power;
	              wrqin->u.txpower.value = 0;              
                     wrqin->u.txpower.fixed = 1; 
	              wrqin->u.txpower.disabled = 0; 
                     wrqin->u.txpower.flags = IW_TXPOW_DBM; 

			RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_AP_SIOCGIWTXPOW, 0,
								&(power), RT_DEV_PRIV_FLAGS_GET(net_dev));		

			wrqin->u.txpower.value = power;
				//len = copy_to_user(wrqin->u.data.pointer, prange, sizeof(struct iw_range));
                        break;		
		}			
		case SIOCSIWTXPOW:  /*set transmit power (dBm) */
			DBGPRINT(RT_DEBUG_ERROR,("SIOCSIWTXPOW\n"));	
			break;		
		case SIOCGIWSTATS:  /*get transmit power (dBm) */
		{
			DBGPRINT(RT_DEBUG_ERROR,("SIOCGIWSTATS\n"));		
			struct iw_statistics *pStats;
			RT_CMD_IW_STATS DrvIwStats, *pDrvIwStats = &DrvIwStats;
			int len;

			GET_PAD_FROM_NET_DEV(pAd, net_dev);	


			pDrvIwStats->priv_flags = RT_DEV_PRIV_FLAGS_GET(net_dev);
			pDrvIwStats->dev_addr = (PUCHAR)net_dev->dev_addr;

			if (RTMP_DRIVER_IW_STATS_GET(pAd, pDrvIwStats) != NDIS_STATUS_SUCCESS)
				return NULL;

			pStats = (struct iw_statistics *)(pDrvIwStats->pStats);
			pStats->status = 0; /* Status - device dependent for now */


			pStats->qual.updated = 1;     /* Flags to know if updated */
			pStats->qual.qual = pDrvIwStats->qual;
			pStats->qual.level = pDrvIwStats->level;
			pStats->qual.noise = pDrvIwStats->noise;
			pStats->discard.nwid = 0;     /* Rx : Wrong nwid/essid */
			pStats->miss.beacon = 0;      /* Missed beacons/superframe */
			len = copy_to_user(wrqin->u.data.pointer, pStats, sizeof(struct iw_statistics));
                        break;		
		}				
		case SIOCGIWRETRY:	/*get retry limits and lifetime */
		case SIOCSIWRETRY:	/*set retry limits and lifetime */
			Status = RTMP_IO_EOPNOTSUPP;
			break;
		case SIOCGIWRANGE:	/*Get range of parameters */
		    {
				UINT power;
				int i,bw,shortGI;
				UCHAR phymode;

				INT OFDM_RateTable[] ={2,  4,   11,  22, 12, 18,   24,  36, 48, 72, 96, 108,};
				INT HT20_LongGI[] ={13, 26,   39,  52,  78, 104, 117, 130, 26,  52,  78, 104, 156, 208, 234, 260,};
				INT HT40_LongGI[] ={27, 54,   81, 108, 162, 216, 243, 270, 54, 108, 162, 216, 324, 432, 486, 540,};
				INT HT20_ShortGI[] ={14, 29,   43,  57,  87, 115, 130, 144, 29, 59,   87, 115, 173, 230, 260, 288,};
				INT HT40_ShortGI[] ={30, 60,   90, 120, 180, 240, 270, 300, 60, 120, 180, 240, 360, 480, 540, 600,};
				//DBGPRINT(RT_DEBUG_ERROR,("SIOCGIWRANGE:\n"));

				RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_AP_GET_PHYMODE, 0,
												&(phymode), RT_DEV_PRIV_FLAGS_GET(net_dev));	

				RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_AP_GET_BW, 0,
												&(bw), RT_DEV_PRIV_FLAGS_GET(net_dev));

				RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_AP_GET_SHORTGI, 0,
												&(shortGI), RT_DEV_PRIV_FLAGS_GET(net_dev));				

/*				struct iw_range range; */
				struct iw_range *prange = NULL;
				UINT32 len;

				/* allocate memory */
				os_alloc_mem(NULL, (UCHAR **)&prange, sizeof(struct iw_range));
				if (prange == NULL)
				{
					DBGPRINT(RT_DEBUG_ERROR, ("%s: Allocate memory fail!!!\n", __FUNCTION__));
					break;
				}

				memset(prange, 0, sizeof(struct iw_range));
				prange->we_version_compiled = WIRELESS_EXT;
				prange->we_version_source = 14;

				 prange->throughput = 27 * 1000 * 1000;
				 
				/*
					what is correct max? This was not
					documented exactly. At least
					69 has been observed.
				*/
				prange->max_qual.qual = 100;
				prange->max_qual.level = 0; /* dB */
				prange->max_qual.noise = 0; /* dB */
				prange->max_qual.updated = 7; 

				prange->avg_qual.qual = 70;
				prange->avg_qual.level = 0; 
				 prange->avg_qual.noise = 0;
				 prange->avg_qual.updated = 7;

				if (phymode < 5)
				{
					prange->num_bitrates = 12;
					 for (i = 0; i < prange->num_bitrates; i++)
	                				 prange->bitrate[i] = (OFDM_RateTable[i]) *
	                     			500000;
				} 
#ifdef DOT11_N_SUPPORT				
				else {

					if (bw == 0)
					{
						if (shortGI ==0 )
						{
							prange->num_bitrates = sizeof(HT20_LongGI)/sizeof(HT20_LongGI[0]);
							 for (i = 0; i < prange->num_bitrates; i++)
			                				 prange->bitrate[i] = (HT20_LongGI[i]) *
			                     			500000;						
						} else {
							prange->num_bitrates = sizeof(HT20_ShortGI)/sizeof(HT20_ShortGI[0]);
							 for (i = 0; i < prange->num_bitrates; i++)
			                				 prange->bitrate[i] = (HT20_ShortGI[i]) *
			                     			500000;						
						}
					} else {
						if (shortGI ==0 )
						{
							prange->num_bitrates = sizeof(HT40_LongGI)/sizeof(HT40_LongGI[0]);
							 for (i = 0; i < prange->num_bitrates; i++)
			                				 prange->bitrate[i] = (HT40_LongGI[i]) *
			                     			500000;						
						} else {
							prange->num_bitrates = sizeof(HT40_ShortGI)/sizeof(HT40_ShortGI[0]);
							 for (i = 0; i < prange->num_bitrates; i++)
			                				 prange->bitrate[i] = (HT40_ShortGI[i]) *
			                     			500000;						
						}
					}

				}
				
#endif				
				   prange->max_rts = 2347;
				    prange->min_frag = MIN_FRAG_THRESHOLD;
				prange->max_frag = MAX_FRAG_THRESHOLD;	

				  prange->encoding_size[0] = 5;
        			 prange->encoding_size[1] = 13;
         			prange->num_encoding_sizes = 2;
         			prange->max_encoding_tokens = 4;

				INT bg[] ={2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472,2477,2482,2487};
			        i = 0;
			        {
			                 for (i = 1; i <14; i ++) {
			 
			                         prange->freq[i-1].i = i;
			                         prange->freq[i-1].m = bg[i-1] * 100000;
			                        prange->freq[i-1].e = 1;
			                 }
			         }		

					prange->num_channels =13;
					prange->num_frequency = 13;

				 prange->event_capa[0] = (IW_EVENT_CAPA_K_0 |
                                 IW_EVENT_CAPA_MASK(SIOCGIWTHRSPY) |
                                 IW_EVENT_CAPA_MASK(SIOCGIWAP) |
                                 IW_EVENT_CAPA_MASK(SIOCGIWSCAN));
        			 prange->event_capa[1] = IW_EVENT_CAPA_K_1;

				prange->enc_capa = IW_ENC_CAPA_WPA | IW_ENC_CAPA_WPA2 |
                 			IW_ENC_CAPA_CIPHER_TKIP | IW_ENC_CAPA_CIPHER_CCMP;
 
         			prange->scan_capa = IW_SCAN_CAPA_ESSID | IW_SCAN_CAPA_TYPE;

 				prange->num_txpower = 2;
     				prange->txpower_capa = IW_TXPOW_DBM|IW_TXPOW_RANGE;
				prange->txpower[0] = -20;
				prange->txpower[0] = -90;	
				len = copy_to_user(wrqin->u.data.pointer, prange, sizeof(struct iw_range));
				os_free_mem(NULL, prange);
		    }
		    break;
		    
		case RT_PRIV_IOCTL:
		case RT_PRIV_IOCTL_EXT:
		{
			subcmd = wrqin->u.data.flags;

			Status = RTMP_AP_IoctlHandle(pAd, wrq, CMD_RT_PRIV_IOCTL, subcmd, wrqin->u.data.pointer, 0);
		}
			break;
		
#ifdef HOSTAPD_SUPPORT
		case SIOCSIWGENIE:
			DBGPRINT(RT_DEBUG_TRACE,("ioctl SIOCSIWGENIE apidx=%d\n",apidx));
			DBGPRINT(RT_DEBUG_TRACE,("ioctl SIOCSIWGENIE length=%d, pointer=%x\n", wrqin->u.data.length, wrqin->u.data.pointer));


			RTMP_AP_IoctlHandle(pAd, wrqin, CMD_RTPRIV_IOCTL_AP_SIOCSIWGENIE, 0, NULL, 0);
			break;
#endif /* HOSTAPD_SUPPORT */

		case SIOCGIWPRIV:
			if (wrqin->u.data.pointer) 
			{
				if ( access_ok(VERIFY_WRITE, wrqin->u.data.pointer, sizeof(ap_privtab)) != TRUE)
					break;
				if ((sizeof(ap_privtab) / sizeof(ap_privtab[0])) <= wrq->u.data.length)
				{
					wrqin->u.data.length = sizeof(ap_privtab) / sizeof(ap_privtab[0]);
					if (copy_to_user(wrqin->u.data.pointer, ap_privtab, sizeof(ap_privtab)))
						Status = RTMP_IO_EFAULT;
				}
				else
					Status = RTMP_IO_E2BIG;
			}
			break;
		case RTPRIV_IOCTL_SET:
			{
				if( access_ok(VERIFY_READ, wrqin->u.data.pointer, wrqin->u.data.length) == TRUE)
					Status = RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_SET, 0, NULL, 0);
			}
			break;
		    
		case RTPRIV_IOCTL_SHOW:
			{
				if( access_ok(VERIFY_READ, wrqin->u.data.pointer, wrqin->u.data.length) == TRUE)
					Status = RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_SHOW, 0, NULL, 0);
			}
			break;	
			
#ifdef INF_AR9
#ifdef AR9_MAPI_SUPPORT
		case RTPRIV_IOCTL_GET_AR9_SHOW:
			{
				if( access_ok(VERIFY_READ, wrqin->u.data.pointer, wrqin->u.data.length) == TRUE)
					Status = RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_GET_AR9_SHOW, 0, NULL, 0);
			}	
		    break;
#endif /*AR9_MAPI_SUPPORT*/
#endif /* INF_AR9 */

#ifdef WSC_AP_SUPPORT
		case RTPRIV_IOCTL_SET_WSCOOB:
			RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_SET_WSCOOB, 0, NULL, 0);
		    break;
#endif/*WSC_AP_SUPPORT*/

/* modified by Red@Ralink, 2009/09/30 */
		case RTPRIV_IOCTL_GET_MAC_TABLE:
			RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_GET_MAC_TABLE, 0, NULL, 0);
		    break;

		case RTPRIV_IOCTL_GET_MAC_TABLE_STRUCT:
			RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_GET_MAC_TABLE_STRUCT, 0, NULL, 0);
			break;
/* end of modification */

#ifdef AP_SCAN_SUPPORT
		case RTPRIV_IOCTL_GSITESURVEY:
			RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_GSITESURVEY, 0, NULL, 0);
			break;
#endif /* AP_SCAN_SUPPORT */

		case RTPRIV_IOCTL_STATISTICS:
			RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_STATISTICS, 0, NULL, 0);
			break;

		case RTPRIV_IOCTL_GET_APCLI_CONNSTATUS:
			printk("-->RTPRIV_IOCTL_GET_APCLI_CONNSTATUS\n");
			RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_CONNSTATUS, 0, NULL, 0);
			break;
			
#ifdef WSC_AP_SUPPORT
		case RTPRIV_IOCTL_WSC_PROFILE:
			RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_WSC_PROFILE, 0, NULL, 0);
		    break;
#endif /* WSC_AP_SUPPORT */
#ifdef DOT11_N_SUPPORT
		case RTPRIV_IOCTL_QUERY_BATABLE:
			RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_QUERY_BATABLE, 0, NULL, 0);
		    break;
#endif /* DOT11_N_SUPPORT */
		case RTPRIV_IOCTL_E2P:
			RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_E2P, 0, NULL, 0);
			break;

#ifdef DBG
		case RTPRIV_IOCTL_BBP:
			RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_BBP, 0, NULL, 0);
			break;
			
		case RTPRIV_IOCTL_MAC:
			RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_MAC, 0, NULL, 0);
			break;
            
#ifdef RTMP_RF_RW_SUPPORT
		case RTPRIV_IOCTL_RF:
			RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_RF, 0, NULL, 0);
			break;
#endif /* RTMP_RF_RW_SUPPORT */
#endif /* DBG */
		case SIOCSIWSCAN:
			printk("====> SIOCSIWSCAN: %d\n", RT_DEV_PRIV_FLAGS_GET(net_dev));
			if (RTMP_AP_IoctlHandle(pAd, wrq, CMD_RTPRIV_IOCTL_AP_SIOCSIWSCAN, 0, 
			          net_dev, pIoctlConfig->priv_flags) != NDIS_STATUS_SUCCESS)
                        {
                                Status = RTMP_IO_EOPNOTSUPP;
                                break;
                        }

			break;
		default:
/*			DBGPRINT(RT_DEBUG_ERROR, ("IOCTL::unknown IOCTL's cmd = 0x%08x\n", cmd)); */
			Status = RTMP_IO_EOPNOTSUPP;
			break;
	}

LabelExit:
	if (Status != 0)
	{
		RT_CMD_STATUS_TRANSLATE(Status);
	}
	else
	{
		/*
			If wrq length is modified, we reset the lenght of origin wrq;

			Or we can not modify it because the address of wrq->u.data.length
			maybe same as other union field, ex: iw_range, etc.

			if the length is not changed but we change it, the value for other
			union will also be changed, this is not correct.
		*/
		if (wrq->u.data.length != org_len)
			wrqin->u.data.length = wrq->u.data.length;
	}

	return Status;
}
