#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=rt2860v2

PKG_VERSION:=2.7.1.6
PKG_RELEASE:=6

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)


include $(INCLUDE_DIR)/package.mk

PKG_KCONFIG:=RT2860V2_AP_V24_DATA_STRUCTURE RT2860V2_AP_LED RT2860V2_AP_WSC RT2860V2_AP_WSC_V2 RT2860V2_AP_LLTD RT2860V2_AP_WDS RT2860V2_AP_WMM_ACM RT2860V2_AP_MBSS NEW_MBSSID_MODE RT2860V2_AP_APCLI RT2860V2_AP_MAC_REPEATER RT2860V2_AP_IGMP_SNOOP RT2860V2_AP_NETIF_BLOCK RT2860V2_AP_DFS RT2860V2_AP_CARRIER RT2860V2_AP_DLS RT2860V2_AP_IDS RT2860V2_HW_ANTENNA_DIVERSITY RT2860V2_AP_WAPI RT2860V2_AP_COC RT2860V2_AP_MEMORY_OPTIMIZATION RT2860V2_AP_VIDEO_TURBINE RA_CLASSIFIER RT2860V2_AP_INTELLIGENT_RATE_ADAPTION RT2860V2_AP_TXBF RT2860V2_EXT_CHANNEL_LIST RT2860V2_KTHREAD RT2860V2_AUTO_CH_SELECT_ENHANCE RT2860V2_AP_80211N_DRAFT3 RT2860V2_AP_RTMP_INTERNAL_TX_ALC RT2860V2_ADJ_PWR_CONSUMPTION_SUPPORT RT2860V2_SINGLE_SKU  INTERNAL_PA_INTERNAL_LNA INTERNAL_PA_EXTERNAL_LNA EXTERNAL_PA_EXTERNAL_LNA RT2860V2_AP_RTMP_INTERNAL_TX_ALC RT2860V2_AP_RTMP_TEMPERATURE_COMPENSATION RT2860V2_80211R_FT RT2860V2_80211R_RR RT2860V2_MCAST_RATE_SPECIFIC RT2860V2_SNMP

define KernelPackage/ralink-flash
  SUBMENU:=Other modules
  TITLE:=Ralink Flash functions.Provide some symbols for Ralink APSoC 802.11n AP driver(RT2860v2)
  DEPENDS:=@TARGET_ramips
  FILES:=$(PKG_BUILD_DIR)/mtdapi/ralink-flash.ko
  AUTOLOAD:=$(call AutoLoad,90,ralink-flash)
endef

define KernelPackage/ralink-flash/description
 Ralink Flash functions.
endef

define KernelPackage/rt2860v2
  SUBMENU:=Wireless Drivers
  TITLE:=Ralink APSoC 802.11n AP driver(RT2860v2)
  DEPENDS:=@TARGET_ramips +wireless-tools +maccalc +kmod-ralink-flash
  FILES:=$(PKG_BUILD_DIR)/rt2860v2_ap/rt2860v2_ap.ko
  AUTOLOAD:=$(call AutoLoad,91,rt2860v2_ap)
endef

define KernelPackage/rt2860v2/description
 Ralink APSoC 802.11n AP driver
endef

define KernelPackage/rt2860v2/config
	source "$(SOURCE)/config.in"
endef

SOURCE_DIR:=$(PKG_BUILD_DIR)
export SOURCE_DIR

MAKEOPTS:= -C $(LINUX_DIR) \
                ARCH="$(LINUX_KARCH)" \
                CROSS_COMPILE="$(TARGET_CROSS)" \
                M="$(PKG_BUILD_DIR)/rt2860v2_ap" \
                CONFIG_RT2860V2_AP=m \
		$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),CONFIG_$(c)=$(CONFIG_$c))) \
		modules

define Build/Prepare
	$(call Build/Prepare/Default)
	$(CP) -r src/. $(PKG_BUILD_DIR)
endef


define Build/Compile
	$(MAKE) -C $(LINUX_DIR) \
                ARCH="$(LINUX_KARCH)" \
                CROSS_COMPILE="$(TARGET_CROSS)" \
                M="$(PKG_BUILD_DIR)/mtdapi" \
                CONFIG_MTD_RALINK=m \
		modules
	$(MAKE) $(MAKEOPTS)
endef

define KernelPackage/rt2860v2/install
	$(CP) ./files/* $(1)/
endef

$(eval $(call KernelPackage,ralink-flash))
$(eval $(call KernelPackage,rt2860v2))
