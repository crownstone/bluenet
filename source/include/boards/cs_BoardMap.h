/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Feb 21, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cfg/cs_Boards.h>
#include <protocol/cs_UicrPacket.h>
#include <stdint.h>

cs_uicr_data_t getUicrData(uint32_t boardVersion) {
	cs_uicr_data_t data = {
			.board = boardVersion,
			.productRegionFamily.fields.reserved = 0xFF,
			.majorMinorPatch.fields.reserved = 0xFF,
			.productionDateHousing.fields.reserved = 0xFF,
	};
	switch (boardVersion) {
		case ACR01B1A: {
			return (cs_uicr_data_t) {
				.board = boardVersion,
				.productRegionFamily = {
						.fields = {
								.productType = PRODUCT_CROWNSTONE_BUILTIN_ZERO,
								.region = 1,
								.productFamily = 1,
								.reserved = 0xFF
						}
				},
				.majorMinorPatch = {
						.fields = {
								.patch = 0,
								.minor = 1,
								.major = 0,
								.reserved = 0xFF
						}
				},
				.productionDateHousing = {
						.fields = {
								.housing = 0,
								.week = 0,
								.year = 0,
								.reserved = 0xFF
						}
				}
			};
		}
		case ACR01B1B: {
			data.productRegionFamily.fields.productType = PRODUCT_CROWNSTONE_BUILTIN_ZERO;
			data.productRegionFamily.fields.region = 1;
			data.productRegionFamily.fields.productFamily = 1;
			data.majorMinorPatch.fields.major = 0;
			data.majorMinorPatch.fields.minor = 2;
			data.majorMinorPatch.fields.patch = 0;
			data.productionDateHousing.fields.housing = 0;
			data.productionDateHousing.fields.week = 0;
			data.productionDateHousing.fields.year = 0;
			break;
		}
		case ACR01B1C: {
			data.productRegionFamily.fields.productType = PRODUCT_CROWNSTONE_BUILTIN_ZERO;
			data.productRegionFamily.fields.region = 1;
			data.productRegionFamily.fields.productFamily = 1;
			data.majorMinorPatch.fields.major = 0;
			data.majorMinorPatch.fields.minor = 3;
			data.majorMinorPatch.fields.patch = 0;
			data.productionDateHousing.fields.housing = 0;
			data.productionDateHousing.fields.week = 0;
			data.productionDateHousing.fields.year = 0;
			break;
		}
		case ACR01B1D: {
			data.productRegionFamily.fields.productType = PRODUCT_CROWNSTONE_BUILTIN_ZERO;
			data.productRegionFamily.fields.region = 1;
			data.productRegionFamily.fields.productFamily = 1;
			data.majorMinorPatch.fields.major = 0;
			data.majorMinorPatch.fields.minor = 4;
			data.majorMinorPatch.fields.patch = 0;
			data.productionDateHousing.fields.housing = 0;
			data.productionDateHousing.fields.week = 0;
			data.productionDateHousing.fields.year = 0;
			break;
		}
		case ACR01B1E: {
			data.productRegionFamily.fields.productType = PRODUCT_CROWNSTONE_BUILTIN_ZERO;
			data.productRegionFamily.fields.region = 1;
			data.productRegionFamily.fields.productFamily = 1;
			data.majorMinorPatch.fields.major = 0;
			data.majorMinorPatch.fields.minor = 5;
			data.majorMinorPatch.fields.patch = 0;
			data.productionDateHousing.fields.housing = 0;
			data.productionDateHousing.fields.week = 0;
			data.productionDateHousing.fields.year = 0;
			break;
		}
		case ACR01B10B: {
			data.productRegionFamily.fields.productType = PRODUCT_CROWNSTONE_BUILTIN_ONE;
			data.productRegionFamily.fields.region = 1;
			data.productRegionFamily.fields.productFamily = 1;
			data.majorMinorPatch.fields.major = 0;
			data.majorMinorPatch.fields.minor = 0;
			data.majorMinorPatch.fields.patch = 0;
			data.productionDateHousing.fields.housing = 0;
			data.productionDateHousing.fields.week = 0;
			data.productionDateHousing.fields.year = 0;
			break;
		}
		case ACR01B10D: {
			data.productRegionFamily.fields.productType = PRODUCT_CROWNSTONE_BUILTIN_ONE;
			data.productRegionFamily.fields.region = 1;
			data.productRegionFamily.fields.productFamily = 1;
			data.majorMinorPatch.fields.major = 0;
			data.majorMinorPatch.fields.minor = 1;
			data.majorMinorPatch.fields.patch = 0;
			data.productionDateHousing.fields.housing = 0;
			data.productionDateHousing.fields.week = 0;
			data.productionDateHousing.fields.year = 0;
			break;
		}
		case ACR01B13B: {
			data.productRegionFamily.fields.productType = PRODUCT_CROWNSTONE_BUILTIN_TWO;
			data.productRegionFamily.fields.region = 1;
			data.productRegionFamily.fields.productFamily = 1;
			data.majorMinorPatch.fields.major = 0;
			data.majorMinorPatch.fields.minor = 1;
			data.majorMinorPatch.fields.patch = 0;
			data.productionDateHousing.fields.housing = 0;
			data.productionDateHousing.fields.week = 0;
			data.productionDateHousing.fields.year = 0;
			break;
		}
		case ACR01B15A: {
			data.productRegionFamily.fields.productType = PRODUCT_CROWNSTONE_BUILTIN_TWO;
			data.productRegionFamily.fields.region = 1;
			data.productRegionFamily.fields.productFamily = 1;
			data.majorMinorPatch.fields.major = 0;
			data.majorMinorPatch.fields.minor = 2;
			data.majorMinorPatch.fields.patch = 0;
			data.productionDateHousing.fields.housing = 0;
			data.productionDateHousing.fields.week = 0;
			data.productionDateHousing.fields.year = 0;
			break;
		}
		case ACR01B2A: {
			data.productRegionFamily.fields.productType = PRODUCT_CROWNSTONE_PLUG_ZERO;
			data.productRegionFamily.fields.region = 1;
			data.productRegionFamily.fields.productFamily = 1;
			data.majorMinorPatch.fields.major = 0;
			data.majorMinorPatch.fields.minor = 1;
			data.majorMinorPatch.fields.patch = 0;
			data.productionDateHousing.fields.housing = 0;
			data.productionDateHousing.fields.week = 0;
			data.productionDateHousing.fields.year = 0;
			break;
		}
		case ACR01B2B: {
			data.productRegionFamily.fields.productType = PRODUCT_CROWNSTONE_PLUG_ZERO;
			data.productRegionFamily.fields.region = 1;
			data.productRegionFamily.fields.productFamily = 1;
			data.majorMinorPatch.fields.major = 0;
			data.majorMinorPatch.fields.minor = 2;
			data.majorMinorPatch.fields.patch = 0;
			data.productionDateHousing.fields.housing = 0;
			data.productionDateHousing.fields.week = 0;
			data.productionDateHousing.fields.year = 0;
			break;
		}
		case ACR01B2C: {
			data.productRegionFamily.fields.productType = PRODUCT_CROWNSTONE_PLUG_ZERO;
			data.productRegionFamily.fields.region = 1;
			data.productRegionFamily.fields.productFamily = 1;
			data.majorMinorPatch.fields.major = 1;
			data.majorMinorPatch.fields.minor = 0;
			data.majorMinorPatch.fields.patch = 0;
			data.productionDateHousing.fields.housing = 0;
			data.productionDateHousing.fields.week = 0;
			data.productionDateHousing.fields.year = 0;
			break;
		}
		case ACR01B2E: {
			data.productRegionFamily.fields.productType = PRODUCT_CROWNSTONE_PLUG_ZERO;
			data.productRegionFamily.fields.region = 1;
			data.productRegionFamily.fields.productFamily = 1;
			data.majorMinorPatch.fields.major = 1;
			data.majorMinorPatch.fields.minor = 1;
			data.majorMinorPatch.fields.patch = 0;
			data.productionDateHousing.fields.housing = 0;
			data.productionDateHousing.fields.week = 0;
			data.productionDateHousing.fields.year = 0;
			break;
		}
		case ACR01B2G: {
			data.productRegionFamily.fields.productType = PRODUCT_CROWNSTONE_PLUG_ZERO;
			data.productRegionFamily.fields.region = 1;
			data.productRegionFamily.fields.productFamily = 1;
			data.majorMinorPatch.fields.major = 1;
			data.majorMinorPatch.fields.minor = 3;
			data.majorMinorPatch.fields.patch = 0;
			data.productionDateHousing.fields.housing = 0;
			data.productionDateHousing.fields.week = 0;
			data.productionDateHousing.fields.year = 0;
			break;
		}
		case ACR01B11A: {
			data.productRegionFamily.fields.productType = PRODUCT_CROWNSTONE_PLUG_ONE;
			data.productRegionFamily.fields.region = 3;
			data.productRegionFamily.fields.productFamily = 3;
			data.majorMinorPatch.fields.major = 0;
			data.majorMinorPatch.fields.minor = 0;
			data.majorMinorPatch.fields.patch = 0;
			data.productionDateHousing.fields.housing = 0;
			data.productionDateHousing.fields.week = 0;
			data.productionDateHousing.fields.year = 0;
			break;
		}
		case GUIDESTONE: {
			data.productRegionFamily.fields.productType = PRODUCT_GUIDESTONE;
			data.productRegionFamily.fields.region = 1;
			data.productRegionFamily.fields.productFamily = 1;
			data.majorMinorPatch.fields.major = 1;
			data.majorMinorPatch.fields.minor = 0;
			data.majorMinorPatch.fields.patch = 0;
			data.productionDateHousing.fields.housing = 0;
			data.productionDateHousing.fields.week = 0;
			data.productionDateHousing.fields.year = 0;
			break;
		}
		case CS_USB_DONGLE: {
			data.productRegionFamily.fields.productType = PRODUCT_CROWNSTONE_USB_DONGLE;
			data.productRegionFamily.fields.region = 1;
			data.productRegionFamily.fields.productFamily = 1;
			data.majorMinorPatch.fields.major = 0;
			data.majorMinorPatch.fields.minor = 0;
			data.majorMinorPatch.fields.patch = 0;
			data.productionDateHousing.fields.housing = 0;
			data.productionDateHousing.fields.week = 0;
			data.productionDateHousing.fields.year = 0;
			break;
		}
		case PCA10036:
		case PCA10040:
		case PCA10056:
		case PCA10100: {
			data.productRegionFamily.fields.productType = PRODUCT_DEV_BOARD;
			data.productRegionFamily.fields.region = 0xFF;
			data.productRegionFamily.fields.productFamily = 0xFF;
			data.majorMinorPatch.fields.major = 0xFF;
			data.majorMinorPatch.fields.minor = 0xFF;
			data.majorMinorPatch.fields.patch = 0xFF;
			data.productionDateHousing.fields.housing = 0xFF;
			data.productionDateHousing.fields.week = 0xFF;
			data.productionDateHousing.fields.year = 0xFF;
			break;
		}
	}
	return data;
}

