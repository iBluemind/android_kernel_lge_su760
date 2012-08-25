/*
 * omap-abe-coef.h
 *
 * Copyright (C) 2010 Texas Instruments
 *
 * Contact: Ricardo Neri <ricardo.neri@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef __OMAP_ABE_COEFFICIENTS_H__
#define __OMAP_ABE_COEFFICIENTS_H__

/*
 * ABE CONST AREA FOR EQUALIZER COEFFICIENTS
 *
 * TODO: These coefficents are for demonstration purposes. Set of
 * coefficients can be computed for specific needs.
 */

#define NBDL1EQ_PROFILES 6  /* Number of supported DL1EQ profiles */
#define NBDL1COEFFS 25      /* Number of coefficients for DL1EQ profiles */
#define NBDL20EQ_PROFILES 6 /* Number of supported DL2EQ_L profiles */
#define NBDL21EQ_PROFILES 6 /* Number of supported DL2EQ_R profiles */
#define NBDL2COEFFS 25      /* Number of coefficients of DL2EQ profiles */
#define NBAMICEQ_PROFILES 4 /* Number of supported AMICEQ profiles */
#define NBAMICCOEFFS 19     /* Number of coefficients of AMICEQ profiles */
#define NBSDTEQ_PROFILES 4  /* Number of supported SDTEQ profiles */
#define NBSDTCOEFFS 9       /* Number of coefficients for SDTEQ profiles */
#define NBDMICEQ_PROFILES 4 /* Number of supported DMICEQ profiles */
#define NBDMICCOEFFS 19     /* Number of coefficients of DMICEQ profiles */

/*
 * Coefficients for DL1EQ
 */
const s32 dl1_equ_coeffs[NBDL1EQ_PROFILES][NBDL1COEFFS] = {
/* Flat response with Gain = 0dB */
				{0, 0, 0, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 0x040002, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 0, 0, 0},

/* 800Hz cut-off frequency and Gain = 0dB  */
				{0, 0, 0, 0, 0, 0, 0, 0, 0, -7554223,
				708210, -708206, 7554225, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 6802833, -682266, 731554},

/* 800Hz cut-off frequency and Gain = -12dB */
				{0, 0, 0, 0, 0, 0, 0, 0, 0, -3777112,
				5665669, -5665667, 3777112, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 6802833, -682266, 731554},

/* 800Hz cut-off frequency and Gain = -20dB */
				{0, 0, 0, 0, 0, 0, 0, 0, 0, -1510844,
				4532536, -4532536, 1510844, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 6802833, -682266, 731554},
/* 4kHz cut-off frequency - Gain = 0dB Low pass - Att 20dB */
				{3, 3, 1379708, -6726356, 8230633, -410286,
				499162, -522034, 499162, -410286, 8230633,
				-6726356, 1379708, 3, 3, -336216, 3624336,
				-337466, 1277386, -3283062, 5917890, -7564798,
				6779594, -4092070, 1510274},    
/* 4kHz cut-off frequency - Gain = 0dB Low pass - Att 20dB */
//				{3, 2251996, 17903, 6190124, 3115340, 8042992,
//				7059644, 7059644, 8042992, 3115340, 6190124,
//				17903, 2251996, 3, -301608, -508736, -1357452,
//				-3202756, -789364, -7584747, 7591373, -710254,
//				658890, -852766, 429186},
				{0, 0, 0, 0, 0, 0, 0, 0, 0, 190000,
				570000, 570000, 190000, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 5811800, -366998, 514590},
};
/*
 * Coefficients for DL2EQ_L
 */
const s32 dl20_equ_coeffs[NBDL20EQ_PROFILES][NBDL2COEFFS] = {
/* Flat response with Gain = 0dB */
				{0, 0, 0, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 0x040002, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 0, 0, 0},
/* LGCNS_CHANGE, [bonggon.kim] , 2011-01-25,  Change 800Hz to 450Hz*/
/* 450Hz cut-off frequency and Gain = 0dB  */
                                          {0, 0, 0, 0, 0, 0, 0, 0, 0, -7908683,
                                         741442, -741438, 7908685, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, 7456217, -726466, 755554},


/* 800Hz cut-off frequency and Gain = -12dB */
				{0, 0, 0, 0, 0, 0, 0, 0, 0, -3777112,
				5665669, -5665667, 3777112, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 6802833, -682266, 731554},

/* 800Hz cut-off frequency and Gain = -20dB */
				{0, 0, 0, 0, 0, 0, 0, 0, 0, -1510844,
				4532536, -4532536, 1510844, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 6802833, -682266, 731554},

/* 4kHz cut-off frequency - Gain = 0dB Low pass - Att 20dB */
				{3, 3, 1379708, -6726356, 8230633, -410286,
				499162, -522034, 499162, -410286, 8230633,
				-6726356, 1379708, 3, 3, -336216, 3624336,
				-337466, 1277386, -3283062, 5917890, -7564798,
				6779594, -4092070, 1510274},         

/* 4kHz cut-off frequency - Gain = 0dB Low pass - Att 20dB */
				{3, 2251996, 17903, 6190124, 3115340, 8042992,
				7059644, 7059644, 8042992, 3115340, 6190124,
				17903, 2251996, 3, -301608, -508736, -1357452,
				-3202756, -789364, -7584747, 7591373, -710254,
				658890, -852766, 429186},
};

/*
 * Coefficients for DL2_EQ_R
 */
const s32 dl21_equ_coeffs[NBDL20EQ_PROFILES][NBDL2COEFFS] = {
/* Flat response with Gain = 0dB */
				{0, 0, 0, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 0x040002, 0, 0,
				0, 0, 0, 0, 0,
				0, 0, 0, 0, 0},
/* LGCNS_CHANGE, [bonggon.kim] , 2011-01-25,  Change 800Hz to 450Hz*/
/* 450Hz cut-off frequency and Gain = 0dB  */
                                          {0, 0, 0, 0, 0, 0, 0, 0, 0, -7908683,
                                         741442, -741438, 7908685, 0, 0, 0, 0,
                                          0, 0, 0, 0, 0, 7456217, -726466, 755554},

/*800Hz cut-off frequency and Gain = -12dB */
				{0, 0, 0, 0, 0, 0, 0, 0, 0, -3777112,
				5665669, -5665667, 3777112, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 6802833, -682266, 731554},

/* 800Hz cut-off frequency and Gain = -20dB */
				{0, 0, 0, 0, 0, 0, 0, 0, 0, -1510844,
				4532536, -4532536, 1510844, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 6802833, -682266, 731554},

/* 4kHz cut-off frequency - Gain = 0dB Low pass - Att 20dB */
				{3, 3, 1379708, -6726356, 8230633, -410286,
				499162, -522034, 499162, -410286, 8230633,
				-6726356, 1379708, 3, 3, -336216, 3624336,
				-337466, 1277386, -3283062, 5917890, -7564798,
				6779594, -4092070, 1510274},         

/* 4kHz cut-off frequency - Gain = 0dB Low pass - Att 20dB */
				{3, 2251996, 17903, 6190124, 3115340, 8042992,
				7059644, 7059644, 8042992, 3115340, 6190124,
				17903, 2251996, 3, -301608, -508736, -1357452,
				-3202756, -789364, -7584747, 7591373, -710254,
				658890, -852766, 429186},
};

/*
 * Coefficients for DMICEQ
 */
const u32 dmic_equ_coeffs[NBDMICEQ_PROFILES][NBDMICCOEFFS] = {
/* Flat response with Gain = 0dB */
				{0, 0,
				0, 0, 0, 0, 0,
				0, 0, 0x040002, 0, 0,
				0, 0, 0, 0, 0,
				0, 0},

/* 20kHz cut-off frequency and Gain = 0dB */
				{-4119413, -192384, -341428, -348088,
				-151380, 151380, 348088, 341428, 192384,
				4119419, 1938156, -6935719, 775202,
				-1801934, 2997698, -3692214, 3406822,
				-2280190, 1042982},

/* 20kHz cut-off frequency and Gain = -12dB */
				{-1029873, -3078121, -5462817, -5569389,
				-2422069, 2422071, 5569391, 5462819,
				3078123, 1029875, 1938188, -6935811,
				775210, -1801950, 2997722, -3692238,
				3406838, -2280198, 1042982},

/* 20kHz cut-off frequency and Gain = -18dB */
				{-514937, -1539061, -2731409, -2784693,
				-1211033, 1211035, 2784695, 2731411,
				1539063, 514939, 1938188, -6935811,
				775210, -1801950, 2997722, -3692238,
				3406838, -2280198, 1042982},
};

/*
 * Coefficients for AMICEQ
 */
const u32 amic_equ_coeffs[NBAMICEQ_PROFILES][NBAMICCOEFFS] = {
/* 20kHz cut-off frequency and Gain = 0dB stop-band : -90dB*/
//				{-4119413, -192384, -341428, -348088,
//				-151380, 151380, 348088, 341428, 192384,
//				4119419, 1938156, -6935719, 775202,
//				-1801934, 2997698, -3692214, 3406822,
//				-2280190, 1042982},	

/* 14.1kHz cut-off frequency and Gain = 0dB stop-band : -43 dB*/	/*CRT monitor generate 15.6kHz Tone. */	
{-325800, 895468, -1761032, 2300724, -2597128, 2597128, -2300724, 1761032, -895468, 325800,
3475408, -450406, 1768598, -4297282, 7124454, -8365266, 6982502, -4017578, 1461874},


/* 20kHz cut-off frequency and Gain = -12dB */
				{-1029873, -3078121, -5462817, -5569389,
				-2422069, 2422071, 5569391, 5462819,
				3078123, 1029875, 1938188, -6935811,
				775210, -1801950, 2997722, -3692238,
				3406838, -2280198, 1042982},

/* 20kHz cut-off frequency and Gain = -18dB */
				{-514937, -1539061, -2731409, -2784693,
				-1211033, 1211035, 2784695, 2731411,
				1539063, 514939, 1938188, -6935811,
				775210, -1801950, 2997722, -3692238,
				3406838, -2280198, 1042982},
				
/* Flat response with Gain = 0dB */
				{0, 0,
				0, 0, 0, 0, 0,
				0, 0, 0x040002, 0, 0,
				0, 0, 0, 0, 0,
				0, 0},
};


/*
 * Coefficients for SDTEQ
 */
const u32 sdt_equ_coeffs[NBSDTEQ_PROFILES][NBSDTCOEFFS] = {
/* Flat response with Gain = 0dB */
				{0, 0, 0, 0, 0x040002, 0, 0, 0, 0},

/* 800Hz cut-off frequency and Gain = 0dB  */
				{0, -7554223, 708210, -708206, 7554225,
				0, 6802833, -682266, 731554},

/* 800Hz cut-off frequency and Gain = -12dB */
				{0, -3777112, 5665669, -5665667, 3777112,
				0, 6802833, -682266, 731554},

/* 800Hz cut-off frequency and Gain = -20dB */
				{0, -1510844, 4532536, -4532536, 1510844,
				0, 6802833, -682266, 731554}
};

#endif	/* End of __OMAP_ABE_COEFFICIENTS_H__ */
