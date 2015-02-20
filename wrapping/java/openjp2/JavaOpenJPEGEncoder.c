/*
 * Copyright (c) 2002-2007, Communications and Remote Sensing Laboratory, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2007, Professor Benoit Macq
 * Copyright (c) 2001-2003, David Janssens
 * Copyright (c) 2002-2003, Yannick Verschueren
 * Copyright (c) 2003-2007, Francois-Olivier Devaux and Antonin Descampe
 * Copyright (c) 2005, Herve Drolon, FreeImage Team 
 * Copyright (c) 2006-2007, Parvatha Elangovan
 * Copyright (c) 2007, Patrick Piscaglia (Telemis)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <jni.h>
#include <math.h>
#include <sys/stat.h>


#include "openjpeg.h"
#include "opj_includes.h"
#include "opj_getopt.h"
#include "org_openJpeg_OpenJPEGJavaEncoder.h"

#include "format_defs.h"
#include "java_helpers.h"

#define IS_READER 0

#define CINEMA_24_CS 1302083	/*Codestream length for 24fps*/
#define CINEMA_48_CS 651041		/*Codestream length for 48fps*/
#define COMP_24_CS 1041666		/*Maximum size per color component for 2K & 4K @ 24fps*/
#define COMP_48_CS 520833		/*Maximum size per color component for 2K @ 48fps*/

typedef struct callback_variables {
	JNIEnv *env;
	/** 'jclass' object used to call a Java method from the C */
	jobject *jobj;
	/** 'jclass' object used to call a Java method from the C */
	jmethodID message_mid;
	jmethodID error_mid;
} callback_variables_t;

typedef struct dircnt{
	/** Buffer for holding images read from Directory*/
	char *filename_buf;
	/** Pointer to the buffer*/
	char **filename;
}dircnt_t;

typedef struct img_folder{
	/** The directory path of the folder containing input images*/
	char *imgdirpath;
	/** Output format*/
	char *out_format;
	/** Enable option*/
	char set_imgdir;
	/** Enable Cod Format for output*/
	char set_out_format;
	/** User specified rate stored in case of cinema option*/
	float *rates;
}img_fol_t;

static void encode_help_display() 
{
	fprintf(stdout,"HELP\n----\n\n");
	fprintf(stdout,"- the -h option displays this help information on screen\n\n");

/* UniPG>> */
	fprintf(stdout,"List of parameters for the JPEG 2000 "
#ifdef USE_JPWL
		"+ JPWL "
#endif /* USE_JPWL */
		"encoder:\n");
/* <<UniPG */
	fprintf(stdout,"\n");
	fprintf(stdout,"REMARKS:\n");
	fprintf(stdout,"---------\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"The markers written to the main_header are : SOC SIZ COD QCD COM.\n");
	fprintf(stdout,"COD and QCD never appear in the tile_header.\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"- This coder can encode a mega image, a test was made on a 24000x24000 pixels \n");
	fprintf(stdout,"color image.  You need enough disk space memory (twice the original) to encode \n");
	fprintf(stdout,"the image,i.e. for a 1.5 GB image you need a minimum of 3GB of disk memory)\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"By default:\n");
	fprintf(stdout,"------------\n");
	fprintf(stdout,"\n");
	fprintf(stdout," * Lossless\n");
	fprintf(stdout," * 1 tile\n");
	fprintf(stdout," * Size of precinct : 2^15 x 2^15 (means 1 precinct)\n");
	fprintf(stdout," * Size of code-block : 64 x 64\n");
	fprintf(stdout," * Number of resolutions: 6\n");
	fprintf(stdout," * No SOP marker in the codestream\n");
	fprintf(stdout," * No EPH marker in the codestream\n");
	fprintf(stdout," * No sub-sampling in x or y direction\n");
	fprintf(stdout," * No mode switch activated\n");
	fprintf(stdout," * Progression order: LRCP\n");
	fprintf(stdout," * No index file\n");
	fprintf(stdout," * No ROI upshifted\n");
	fprintf(stdout," * No offset of the origin of the image\n");
	fprintf(stdout," * No offset of the origin of the tiles\n");
	fprintf(stdout," * Reversible DWT 5-3\n");
/* UniPG>> */
#ifdef USE_JPWL
	fprintf(stdout," * No JPWL protection\n");
#endif /* USE_JPWL */
/* <<UniPG */
	fprintf(stdout,"\n");
	fprintf(stdout,"Parameters:\n");
	fprintf(stdout,"------------\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"Required Parameters (except with -h):\n");
	fprintf(stdout,"-i           : source file  (-i source.pnm also *.pgm, *.ppm, *.bmp, *.tif, *.raw, *.tga) \n");
	fprintf(stdout,"    When using this option -o must be used\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-o           : destination file (-o dest.j2k or .jp2) \n");
	fprintf(stdout,"\n");
	fprintf(stdout,"Optional Parameters:\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-h           : display the help information \n ");
	fprintf(stdout,"\n");
	fprintf(stdout,"-cinema2K    : Digital Cinema 2K profile compliant codestream for 2K resolution.(-cinema2k 24 or 48) \n");
	fprintf(stdout,"	  Need to specify the frames per second for a 2K resolution. Only 24 or 48 fps is allowed\n"); 
	fprintf(stdout,"\n");
	fprintf(stdout,"-cinema4K    : Digital Cinema 4K profile compliant codestream for 4K resolution \n");
	fprintf(stdout,"	  Frames per second not required. Default value is 24fps\n"); 
	fprintf(stdout,"\n");
	fprintf(stdout,"-r           : different compression ratios for successive layers (-r 20,10,5)\n ");
	fprintf(stdout,"	         - The rate specified for each quality level is the desired \n");
	fprintf(stdout,"	           compression factor.\n");
	fprintf(stdout,"		   Example: -r 20,10,1 means quality 1: compress 20x, \n");
	fprintf(stdout,"		     quality 2: compress 10x and quality 3: compress lossless\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"               (options -r and -q cannot be used together)\n ");
	fprintf(stdout,"\n");

	fprintf(stdout,"-q           : different psnr for successive layers (-q 30,40,50) \n ");

	fprintf(stdout,"               (options -r and -q cannot be used together)\n ");

	fprintf(stdout,"\n");
	fprintf(stdout,"-n           : number of resolutions (-n 3) \n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-b           : size of code block (-b 32,32) \n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-c           : size of precinct (-c 128,128) \n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-t           : size of tile (-t 512,512) \n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-p           : progression order (-p LRCP) [LRCP, RLCP, RPCL, PCRL, CPRL] \n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-s           : subsampling factor (-s 2,2) [-s X,Y] \n");
	fprintf(stdout,"	     Remark: subsampling bigger than 2 can produce error\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-POC         : Progression order change (-POC T1=0,0,1,5,3,CPRL/T1=5,0,1,6,3,CPRL) \n");
	fprintf(stdout,"      Example: T1=0,0,1,5,3,CPRL \n");
	fprintf(stdout,"			 : Ttilenumber=Resolution num start,Component num start,Layer num end,Resolution num end,Component num end,Progression order\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-SOP         : write SOP marker before each packet \n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-EPH         : write EPH marker after each header packet \n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-M           : mode switch(-M 3) [1=BYPASS(LAZY) 2=RESET 4=RESTART(TERMALL)\n");
	fprintf(stdout,"                 8=VSC 16=ERTERM(SEGTERM) 32=SEGMARK(SEGSYM)] \n");
	fprintf(stdout,"                 Indicate multiple modes by adding their values. \n");
	fprintf(stdout,"                 ex: RESTART(4) + RESET(2) + SEGMARK(32) = -M 38\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-TP          : devide packets of every tile into tile-parts (-TP R) [R, L, C]\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-x           : create an index file *.Idx (-x index_name.Idx) \n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-ROI         : c=%%d,U=%%d : quantization indices upshifted \n");
	fprintf(stdout,"               for component c=%%d [%%d = 0,1,2]\n");
	fprintf(stdout,"               with a value of U=%%d [0 <= %%d <= 37] (i.e. -ROI c=0,U=25) \n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-d           : offset of the origin of the image (-d 150,300) \n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-T           : offset of the origin of the tiles (-T 100,75) \n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-I           : use the irreversible DWT 9-7 (-I) \n");
	fprintf(stdout,"\n");
	fprintf(stdout,"-jpip        : write jpip codestream index box in JP2 output file\n");
	fprintf(stdout,"               NOTICE: currently supports only RPCL order\n");
	fprintf(stdout,"\n");
/* UniPG>> */
#ifdef USE_JPWL
	fprintf(stdout,"-W           : adoption of JPWL (Part 11) capabilities (-W params)\n");
	fprintf(stdout,"               The parameters can be written and repeated in any order:\n");
	fprintf(stdout,"               [h<tilepart><=type>,s<tilepart><=method>,a=<addr>,...\n");
	fprintf(stdout,"                ...,z=<size>,g=<range>,p<tilepart:pack><=type>]\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"                 h selects the header error protection (EPB): 'type' can be\n");
	fprintf(stdout,"                   [0=none 1,absent=predefined 16=CRC-16 32=CRC-32 37-128=RS]\n");
	fprintf(stdout,"                   if 'tilepart' is absent, it is for main and tile headers\n");
	fprintf(stdout,"                   if 'tilepart' is present, it applies from that tile\n");
	fprintf(stdout,"                     onwards, up to the next h<> spec, or to the last tilepart\n");
	fprintf(stdout,"                     in the codestream (max. %d specs)\n", JPWL_MAX_NO_TILESPECS);
	fprintf(stdout,"\n");
	fprintf(stdout,"                 p selects the packet error protection (EEP/UEP with EPBs)\n");
	fprintf(stdout,"                  to be applied to raw data: 'type' can be\n");
	fprintf(stdout,"                   [0=none 1,absent=predefined 16=CRC-16 32=CRC-32 37-128=RS]\n");
	fprintf(stdout,"                   if 'tilepart:pack' is absent, it is from tile 0, packet 0\n");
	fprintf(stdout,"                   if 'tilepart:pack' is present, it applies from that tile\n");
	fprintf(stdout,"                     and that packet onwards, up to the next packet spec\n");
	fprintf(stdout,"                     or to the last packet in the last tilepart in the stream\n");
	fprintf(stdout,"                     (max. %d specs)\n", JPWL_MAX_NO_PACKSPECS);
	fprintf(stdout,"\n");
	fprintf(stdout,"                 s enables sensitivity data insertion (ESD): 'method' can be\n");
	fprintf(stdout,"                   [-1=NO ESD 0=RELATIVE ERROR 1=MSE 2=MSE REDUCTION 3=PSNR\n");
	fprintf(stdout,"                    4=PSNR INCREMENT 5=MAXERR 6=TSE 7=RESERVED]\n");
	fprintf(stdout,"                   if 'tilepart' is absent, it is for main header only\n");
	fprintf(stdout,"                   if 'tilepart' is present, it applies from that tile\n");
	fprintf(stdout,"                     onwards, up to the next s<> spec, or to the last tilepart\n");
	fprintf(stdout,"                     in the codestream (max. %d specs)\n", JPWL_MAX_NO_TILESPECS);
	fprintf(stdout,"\n");
	fprintf(stdout,"                 g determines the addressing mode: <range> can be\n");
	fprintf(stdout,"                   [0=PACKET 1=BYTE RANGE 2=PACKET RANGE]\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"                 a determines the size of data addressing: <addr> can be\n");
	fprintf(stdout,"                   2/4 bytes (small/large codestreams). If not set, auto-mode\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"                 z determines the size of sensitivity values: <size> can be\n");
	fprintf(stdout,"                   1/2 bytes, for the transformed pseudo-floating point value\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"                 ex.:\n");
	fprintf(stdout,"                   h,h0=64,h3=16,h5=32,p0=78,p0:24=56,p1,p3:0=0,p3:20=32,s=0,\n");
	fprintf(stdout,"                     s0=6,s3=-1,a=0,g=1,z=1\n");
	fprintf(stdout,"                 means\n");
	fprintf(stdout,"                   predefined EPB in MH, rs(64,32) from TPH 0 to TPH 2,\n");
	fprintf(stdout,"                   CRC-16 in TPH 3 and TPH 4, CRC-32 in remaining TPHs,\n");
	fprintf(stdout,"                   UEP rs(78,32) for packets 0 to 23 of tile 0,\n");
	fprintf(stdout,"                   UEP rs(56,32) for packs. 24 to the last of tilepart 0,\n");
	fprintf(stdout,"                   UEP rs default for packets of tilepart 1,\n");
	fprintf(stdout,"                   no UEP for packets 0 to 19 of tilepart 3,\n");
	fprintf(stdout,"                   UEP CRC-32 for packs. 20 of tilepart 3 to last tilepart,\n");
	fprintf(stdout,"                   relative sensitivity ESD for MH,\n");
	fprintf(stdout,"                   TSE ESD from TPH 0 to TPH 2, byte range with automatic\n");
	fprintf(stdout,"                   size of addresses and 1 byte for each sensitivity value\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"                 ex.:\n");
	fprintf(stdout,"                       h,s,p\n");
	fprintf(stdout,"                 means\n");
	fprintf(stdout,"                   default protection to headers (MH and TPHs) as well as\n");
	fprintf(stdout,"                   data packets, one ESD in MH\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"                 N.B.: use the following recommendations when specifying\n");
	fprintf(stdout,"                       the JPWL parameters list\n");
	fprintf(stdout,"                   - when you use UEP, always pair the 'p' option with 'h'\n");
	fprintf(stdout,"                 \n");
#endif /* USE_JPWL */
/* <<UniPG */
	fprintf(stdout,"IMPORTANT:\n");
	fprintf(stdout,"-----------\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"The index file has the structure below:\n");
	fprintf(stdout,"---------------------------------------\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"Image_height Image_width\n");
	fprintf(stdout,"progression order\n");
	fprintf(stdout,"Tiles_size_X Tiles_size_Y\n");
	fprintf(stdout,"Tiles_nb_X Tiles_nb_Y\n");
	fprintf(stdout,"Components_nb\n");
	fprintf(stdout,"Layers_nb\n");
	fprintf(stdout,"decomposition_levels\n");
	fprintf(stdout,"[Precincts_size_X_res_Nr Precincts_size_Y_res_Nr]...\n");
	fprintf(stdout,"   [Precincts_size_X_res_0 Precincts_size_Y_res_0]\n");
	fprintf(stdout,"Main_header_start_position\n");
	fprintf(stdout,"Main_header_end_position\n");
	fprintf(stdout,"Codestream_size\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"INFO ON TILES\n");
	fprintf(stdout,"tileno start_pos end_hd end_tile nbparts disto nbpix disto/nbpix\n");
	fprintf(stdout,"Tile_0 start_pos end_Theader end_pos NumParts TotalDisto NumPix MaxMSE\n");
	fprintf(stdout,"Tile_1   ''           ''        ''        ''       ''    ''      ''\n");
	fprintf(stdout,"...\n");
	fprintf(stdout,"Tile_Nt   ''           ''        ''        ''       ''    ''     ''\n");
	fprintf(stdout,"...\n");
	fprintf(stdout,"TILE 0 DETAILS\n");
	fprintf(stdout,"part_nb tileno num_packs start_pos end_tph_pos end_pos\n");
	fprintf(stdout,"...\n");
	fprintf(stdout,"Progression_string\n");
	fprintf(stdout,"pack_nb tileno layno resno compno precno start_pos end_ph_pos end_pos disto\n");
	fprintf(stdout,"Tpacket_0 Tile layer res. comp. prec. start_pos end_pos disto\n");
	fprintf(stdout,"...\n");
	fprintf(stdout,"Tpacket_Np ''   ''    ''   ''    ''       ''       ''     ''\n");

	fprintf(stdout,"MaxDisto\n");

	fprintf(stdout,"TotalDisto\n\n");
}


static OPJ_PROG_ORDER give_progression(const char progression[4]) 
{
	if(strncmp(progression, "LRCP", 4) == 0) {
		return OPJ_LRCP;
	}
	if(strncmp(progression, "RLCP", 4) == 0) {
		return OPJ_RLCP;
	}
	if(strncmp(progression, "RPCL", 4) == 0) {
		return OPJ_RPCL;
	}
	if(strncmp(progression, "PCRL", 4) == 0) {
		return OPJ_PCRL;
	}
	if(strncmp(progression, "CPRL", 4) == 0) {
		return OPJ_CPRL;
	}

	return OPJ_PROG_UNKNOWN;
}

static int initialise_4K_poc(opj_poc_t *POC, int numres)
{
	POC[0].tile  = 1; 
	POC[0].resno0  = 0; 
	POC[0].compno0 = 0;
	POC[0].layno1  = 1;
	POC[0].resno1  = numres-1;
	POC[0].compno1 = 3;
	POC[0].prg1 = OPJ_CPRL;
	POC[1].tile  = 1;
	POC[1].resno0  = numres-1; 
	POC[1].compno0 = 0;
	POC[1].layno1  = 1;
	POC[1].resno1  = numres;
	POC[1].compno1 = 3;
	POC[1].prg1 = OPJ_CPRL;
	return 2;
}

static void cinema_parameters(opj_cparameters_t *parameters)
{
	parameters->tile_size_on = OPJ_FALSE;
	parameters->cp_tdx=1;
	parameters->cp_tdy=1;
	
	/*Tile part*/
	parameters->tp_flag = 'C';
	parameters->tp_on = 1;

	/*Tile and Image shall be at (0,0)*/
	parameters->cp_tx0 = 0;
	parameters->cp_ty0 = 0;
	parameters->image_offset_x0 = 0;
	parameters->image_offset_y0 = 0;

	/*Codeblock size= 32*32*/
	parameters->cblockw_init = 32;	
	parameters->cblockh_init = 32;
	parameters->csty |= 0x01;

	/*The progression order shall be CPRL*/
	parameters->prog_order = OPJ_CPRL;

	/* No ROI */
	parameters->roi_compno = -1;

	parameters->subsampling_dx = 1;		parameters->subsampling_dy = 1;

	/* 9-7 transform */
	parameters->irreversible = 1;

}

static void cinema_setup_encoder(opj_cparameters_t *parameters,
	opj_image_t *image, img_fol_t *img_fol)
{
	int i;
	float temp_rate;

	switch(parameters->cp_cinema){
	case OPJ_CINEMA2K_24:
	case OPJ_CINEMA2K_48:
		if(parameters->numresolution > 6){
			parameters->numresolution = 6;
		}
		if(!((image->comps[0].w == 2048) | (image->comps[0].h == 1080))){
			fprintf(stdout,"Image coordinates %d x %d is not 2K compliant.\nJPEG Digital Cinema Profile-3"
				"(2K profile) compliance requires that at least one of coordinates match 2048 x 1080\n",
				image->comps[0].w,image->comps[0].h);
			parameters->cp_rsiz = OPJ_STD_RSIZ;
		}
		break;
	
	case OPJ_CINEMA4K_24:
		if(parameters->numresolution < 1){
				parameters->numresolution = 1;
			}else if(parameters->numresolution > 7){
				parameters->numresolution = 7;
			}
		if(!((image->comps[0].w == 4096) | (image->comps[0].h == 2160))){
			fprintf(stdout,"Image coordinates %d x %d is not 4K compliant.\nJPEG Digital Cinema Profile-4" 
				"(4K profile) compliance requires that at least one of coordinates match 4096 x 2160\n",
				image->comps[0].w,image->comps[0].h);
			parameters->cp_rsiz = OPJ_STD_RSIZ;
		}
		parameters->numpocs = initialise_4K_poc(parameters->POC,parameters->numresolution);
		break;
	default:return;
	}

	switch(parameters->cp_cinema){
		case OPJ_CINEMA2K_24:
		case OPJ_CINEMA4K_24:
			for(i=0 ; i<parameters->tcp_numlayers ; i++){
				temp_rate = 0 ;
				if(img_fol->rates[i]== 0){
					parameters->tcp_rates[0]= ((float) (image->numcomps * image->comps[0].w * image->comps[0].h * image->comps[0].prec))/ 
					(CINEMA_24_CS * 8 * image->comps[0].dx * image->comps[0].dy);
				}else{
					temp_rate =((float) (image->numcomps * image->comps[0].w * image->comps[0].h * image->comps[0].prec))/ 
						(img_fol->rates[i] * 8 * image->comps[0].dx * image->comps[0].dy);
					if(temp_rate > CINEMA_24_CS ){
						parameters->tcp_rates[i]= ((float) (image->numcomps * image->comps[0].w * image->comps[0].h * image->comps[0].prec))/ 
						(CINEMA_24_CS * 8 * image->comps[0].dx * image->comps[0].dy);
					}else{
						parameters->tcp_rates[i]= img_fol->rates[i];
					}
				}
			}
			parameters->max_comp_size = COMP_24_CS;
			break;
		
		case OPJ_CINEMA2K_48:
			for(i=0 ; i<parameters->tcp_numlayers ; i++){
				temp_rate = 0 ;
				if(img_fol->rates[i]== 0){
					parameters->tcp_rates[0]= ((float) (image->numcomps * image->comps[0].w * image->comps[0].h * image->comps[0].prec))/ 
					(CINEMA_48_CS * 8 * image->comps[0].dx * image->comps[0].dy);
				}else{
					temp_rate =((float) (image->numcomps * image->comps[0].w * image->comps[0].h * image->comps[0].prec))/ 
						(img_fol->rates[i] * 8 * image->comps[0].dx * image->comps[0].dy);
					if(temp_rate > CINEMA_48_CS ){
						parameters->tcp_rates[0]= ((float) (image->numcomps * image->comps[0].w * image->comps[0].h * image->comps[0].prec))/ 
						(CINEMA_48_CS * 8 * image->comps[0].dx * image->comps[0].dy);
					}else{
						parameters->tcp_rates[i]= img_fol->rates[i];
					}
				}
			}
			parameters->max_comp_size = COMP_48_CS;
			break;
		default:return;
	}
	parameters->cp_disto_alloc = 1;
}


/* ------------------------------------------------------------------ */
static int parse_cmdline_encoder(int argc, char * const argv[], 
	opj_cparameters_t *parameters,
	img_fol_t *img_fol, char *indexfilename) 
{
	int i, j,totlen;
	opj_option_t long_option[]={
		{"cinema2K",REQ_ARG, NULL ,'w'},
		{"cinema4K",NO_ARG, NULL ,'y'},
		{"TP",REQ_ARG, NULL ,'u'},
		{"SOP",NO_ARG, NULL ,'S'},
		{"EPH",NO_ARG, NULL ,'E'},
		{"POC",REQ_ARG, NULL ,'P'},
		{"ROI",REQ_ARG, NULL ,'R'},
		{"jpip",NO_ARG, NULL, 'J'}
	};

	/* parse the command line */
/* UniPG>> */
	const char optlist[] = "i:o:hr:q:n:b:c:t:p:s:SEM:x:R:d:T:If:P:C:F:u:"
#ifdef USE_JPWL
		"W:"
#endif /* USE_JPWL */
		;
#ifdef DEBUG_ARGS
	for(i=0; i<argc; i++) 
   {
	printf("[%s]",argv[i]);
   }
	printf("\n");
#endif

	totlen=sizeof(long_option);
	img_fol->set_out_format=0;
	reset_options_reading();

	while(1) {
    int c = opj_getopt_long(argc, argv, optlist,long_option,totlen);
		if(c == -1)
			break;
		switch(c) {

				/* ----------------------------------------------------- */

			case 'o':			/* output file */
			{
				char *outfile = opj_optarg;
				parameters->cod_format = outfile_format(outfile);
				switch(parameters->cod_format) {
					case J2K_CFMT:
					case JP2_CFMT:
						break;
					default:
						fprintf(stderr, "Unknown output format image %s [only *.j2k, *.j2c or *.jp2]!! \n", outfile);
						return 1;
				}
				strncpy(parameters->outfile, outfile, sizeof(parameters->outfile)-1);
			}
			break;

				/* ----------------------------------------------------- */
			case 'O':			/* output format */
				{
					char outformat[50];
					char *of = opj_optarg;
					sprintf(outformat,".%s",of);
					img_fol->set_out_format = 1;
					parameters->cod_format = outfile_format(outformat);
					switch(parameters->cod_format) {
						case J2K_CFMT:
						case JP2_CFMT:
							img_fol->out_format = opj_optarg;
							break;
						default:
							fprintf(stderr, "Unknown output format image [only j2k, j2c, jp2]!! \n");
							return 1;
					}
				}
				break;

				/* ----------------------------------------------------- */
			case 'r':			/* rates rates/distorsion */
			{
				char *s = opj_optarg;
				while(sscanf(s, "%f", &parameters->tcp_rates[parameters->tcp_numlayers]) == 1) {
					parameters->tcp_numlayers++;
					while(*s && *s != ',') {
						s++;
					}
					if(!*s)
						break;
					s++;
				}
				parameters->cp_disto_alloc = 1;
			}
			break;

				/* ----------------------------------------------------- */

			case 'q':			/* add fixed_quality */
			{
				char *s = opj_optarg;
				while(sscanf(s, "%f", &parameters->tcp_distoratio[parameters->tcp_numlayers]) == 1) {
					parameters->tcp_numlayers++;
					while(*s && *s != ',') {
						s++;
					}
					if(!*s)
						break;
					s++;
				}
				parameters->cp_fixed_quality = 1;
			}
			break;

				/* dda */
				/* ----------------------------------------------------- */

			case 'f':			/* mod fixed_quality (before : -q) */
			{
				int *row = NULL, *col = NULL;
				int numlayers = 0, numresolution = 0, matrix_width = 0;

				char *s = opj_optarg;
				sscanf(s, "%d", &numlayers);
				s++;
				if(numlayers > 9)
					s++;

				parameters->tcp_numlayers = numlayers;
				numresolution = parameters->numresolution;
				matrix_width = numresolution * 3;
				parameters->cp_matrice = (int *) opj_malloc(numlayers * matrix_width * sizeof(int));
				s = s + 2;

				for(i = 0; i < numlayers; i++) {
					row = &parameters->cp_matrice[i * matrix_width];
					col = row;
					parameters->tcp_rates[i] = 1;
					sscanf(s, "%d,", &col[0]);
					s += 2;
					if(col[0] > 9)
						s++;
					col[1] = 0;
					col[2] = 0;
					for(j = 1; j < numresolution; j++) {
						col += 3;
						sscanf(s, "%d,%d,%d", &col[0], &col[1], &col[2]);
						s += 6;
						if(col[0] > 9)
							s++;
						if(col[1] > 9)
							s++;
						if(col[2] > 9)
							s++;
					}
					if(i < numlayers - 1)
						s++;
				}
				parameters->cp_fixed_alloc = 1;
			}
			break;

				/* ----------------------------------------------------- */

			case 't':			/* tiles */
			{
				sscanf(opj_optarg, "%d,%d", &parameters->cp_tdx, &parameters->cp_tdy);
				parameters->tile_size_on = OPJ_TRUE;
			}
			break;

				/* ----------------------------------------------------- */

			case 'n':			/* resolution */
			{
				sscanf(opj_optarg, "%d", &parameters->numresolution);
			}
			break;

				/* ----------------------------------------------------- */
			case 'c':			/* precinct dimension */
			{
				char sep;
				int res_spec = 0;

				char *s = opj_optarg;
				do {
					sep = 0;
					sscanf(s, "[%d,%d]%c", &parameters->prcw_init[res_spec],
                                 &parameters->prch_init[res_spec], &sep);
					parameters->csty |= 0x01;
					res_spec++;
					s = strpbrk(s, "]") + 2;
				}
				while(sep == ',');
				parameters->res_spec = res_spec;
			}
			break;

				/* ----------------------------------------------------- */

			case 'b':			/* code-block dimension */
			{
				int cblockw_init = 0, cblockh_init = 0;
				sscanf(opj_optarg, "%d,%d", &cblockw_init, &cblockh_init);
				if(cblockw_init * cblockh_init > 4096 || cblockw_init > 1024
					|| cblockw_init < 4 || cblockh_init > 1024 || cblockh_init < 4) {
					fprintf(stderr,
						"!! Size of code_block error (option -b) !!\n\nRestriction :\n"
            "    * width*height<=4096\n    * 4<=width,height<= 1024\n\n");
					return 1;
				}
				parameters->cblockw_init = cblockw_init;
				parameters->cblockh_init = cblockh_init;
			}
			break;

				/* ----------------------------------------------------- */

			case 'x':			/* creation of index file */
			{
				char *index = opj_optarg;
				strncpy(indexfilename, index, OPJ_PATH_LEN);
			}
			break;

				/* ----------------------------------------------------- */

			case 'p':			/* progression order */
			{
				char progression[4];

				strncpy(progression, opj_optarg, 4);
				parameters->prog_order = give_progression(progression);
				if(parameters->prog_order == -1) {
					fprintf(stderr, "Unrecognized progression order "
            "[LRCP, RLCP, RPCL, PCRL, CPRL] !!\n");
					return 1;
				}
			}
			break;

				/* ----------------------------------------------------- */

			case 's':			/* subsampling factor */
			{
				if(sscanf(opj_optarg, "%d,%d", &parameters->subsampling_dx,
                                    &parameters->subsampling_dy) != 2) {
					fprintf(stderr,	"'-s' sub-sampling argument error !  [-s dx,dy]\n");
					return 1;
				}
			}
			break;

				/* ----------------------------------------------------- */

			case 'd':			/* coordonnate of the reference grid */
			{
				if(sscanf(opj_optarg, "%d,%d", &parameters->image_offset_x0,
                                    &parameters->image_offset_y0) != 2) {
					fprintf(stderr,	"-d 'coordonnate of the reference grid' argument "
            "error !! [-d x0,y0]\n");
					return 1;
				}
			}
			break;

				/* ----------------------------------------------------- */

			case 'h':			/* display an help description */
				encode_help_display();
				return 1;

				/* ----------------------------------------------------- */

			case 'P':			/* POC */
			{
				int numpocs = 0;		/* number of progression order change (POC) default 0 */
				opj_poc_t *POC = NULL;	/* POC : used in case of Progression order change */

				char *s = opj_optarg;
				POC = parameters->POC;

				while(sscanf(s, "T%d=%d,%d,%d,%d,%d,%4s", &POC[numpocs].tile,
					&POC[numpocs].resno0, &POC[numpocs].compno0,
					&POC[numpocs].layno1, &POC[numpocs].resno1,
					&POC[numpocs].compno1, POC[numpocs].progorder) == 7) {
					POC[numpocs].prg1 = give_progression(POC[numpocs].progorder);
					numpocs++;
					while(*s && *s != '/') {
						s++;
					}
					if(!*s) {
						break;
					}
					s++;
				}
				parameters->numpocs = numpocs;
			}
			break;

				/* ------------------------------------------------------ */

			case 'S':			/* SOP marker */
			{
				parameters->csty |= 0x02;
			}
			break;

				/* ------------------------------------------------------ */

			case 'E':			/* EPH marker */
			{
				parameters->csty |= 0x04;
			}
			break;

				/* ------------------------------------------------------ */

			case 'M':			/* Mode switch pas tous au point !! */
			{
				int value = 0;
				if(sscanf(opj_optarg, "%d", &value) == 1) {
					for(i = 0; i <= 5; i++) {
						int cache = value & (1 << i);
						if(cache)
							parameters->mode |= (1 << i);
					}
				}
			}
			break;

				/* ------------------------------------------------------ */

			case 'R':			/* ROI */
			{
				if(sscanf(opj_optarg, "c=%d,U=%d", &parameters->roi_compno,
                                           &parameters->roi_shift) != 2) {
					fprintf(stderr, "ROI error !! [-ROI c='compno',U='shift']\n");
					return 1;
				}
			}
			break;

				/* ------------------------------------------------------ */

			case 'T':			/* Tile offset */
			{
				if(sscanf(opj_optarg, "%d,%d", &parameters->cp_tx0, &parameters->cp_ty0) != 2) {
					fprintf(stderr, "-T 'tile offset' argument error !! [-T X0,Y0]");
					return 1;
				}
			}
			break;

				/* ------------------------------------------------------ */

			case 'C':			/* add a comment */
			{
				parameters->cp_comment = (char*)opj_malloc(strlen(opj_optarg) + 1);
				if(parameters->cp_comment) {
					strcpy(parameters->cp_comment, opj_optarg);
				}
			}
			break;


				/* ------------------------------------------------------ */

			case 'I':			/* reversible or not */
			{
				parameters->irreversible = 1;
			}
			break;

			/* ------------------------------------------------------ */
			
			case 'u':			/* Tile part generation*/
			{
				parameters->tp_flag = opj_optarg[0];
				parameters->tp_on = 1;
			}
			break;	

				/* ------------------------------------------------------ */
			
			case 'z':			/* Image Directory path */
			{
				img_fol->imgdirpath = (char*)opj_malloc(strlen(opj_optarg) + 1);
				strcpy(img_fol->imgdirpath,opj_optarg);
				img_fol->set_imgdir=1;
			}
			break;

				/* ------------------------------------------------------ */
			
			case 'w':			/* Digital Cinema 2K profile compliance*/
			{
				int fps=0;
				sscanf(opj_optarg,"%d",&fps);
				if(fps == 24){
					parameters->cp_cinema = OPJ_CINEMA2K_24;
				}else if(fps == 48 ){
					parameters->cp_cinema = OPJ_CINEMA2K_48;
				}else {
					fprintf(stderr,"Incorrect value!! must be 24 or 48\n");
					return 1;
				}
				fprintf(stdout,"CINEMA 2K compliant codestream\n");
				parameters->cp_rsiz = OPJ_CINEMA2K;
				
			}
			break;
				
				/* ------------------------------------------------------ */
			
			case 'y':			/* Digital Cinema 4K profile compliance*/
			{
				parameters->cp_cinema = OPJ_CINEMA4K_24;
				fprintf(stdout,"CINEMA 4K compliant codestream\n");
				parameters->cp_rsiz = OPJ_CINEMA4K;
			}
			break;
				
				/* ------------------------------------------------------ */

/* UniPG>> */
#ifdef USE_JPWL
				/* ------------------------------------------------------ */
			
			case 'W':			/* JPWL capabilities switched on */
			{
				char *token = NULL;
				int hprot, pprot, sens, addr, size, range;

				/* we need to enable indexing */
				if(!indexfilename) {
					strncpy(indexfilename, JPWL_PRIVATEINDEX_NAME, OPJ_PATH_LEN);
				}

				/* search for different protection methods */

				/* break the option in comma points and parse the result */
				token = strtok(opj_optarg, ",");
				while(token != NULL) {

					/* search header error protection method */
					if(*token == 'h') {

						static int tile = 0, tilespec = 0, lasttileno = 0;

						hprot = 1; /* predefined method */

						if(sscanf(token, "h=%d", &hprot) == 1) {
							/* Main header, specified */
							if(!((hprot == 0) || (hprot == 1) || (hprot == 16) || (hprot == 32) ||
								((hprot >= 37) && (hprot <= 128)))) {
								fprintf(stderr, "ERROR -> invalid main header protection method h = %d\n", hprot);
								return 1;
							}
							parameters->jpwl_hprot_MH = hprot;

						} else if(sscanf(token, "h%d=%d", &tile, &hprot) == 2) {
							/* Tile part header, specified */
							if(!((hprot == 0) || (hprot == 1) || (hprot == 16) || (hprot == 32) ||
								((hprot >= 37) && (hprot <= 128)))) {
								fprintf(stderr, "ERROR -> invalid tile part header protection method h = %d\n", hprot);
								return 1;
							}
							if(tile < 0) {
								fprintf(stderr, "ERROR -> invalid tile part number on protection method t = %d\n", tile);
								return 1;
							}
							if(tilespec < JPWL_MAX_NO_TILESPECS) {
								parameters->jpwl_hprot_TPH_tileno[tilespec] = lasttileno = tile;
								parameters->jpwl_hprot_TPH[tilespec++] = hprot;
							}

						} else if(sscanf(token, "h%d", &tile) == 1) {
							/* Tile part header, unspecified */
							if(tile < 0) {
								fprintf(stderr, "ERROR -> invalid tile part number on protection method t = %d\n", tile);
								return 1;
							}
							if(tilespec < JPWL_MAX_NO_TILESPECS) {
								parameters->jpwl_hprot_TPH_tileno[tilespec] = lasttileno = tile;
								parameters->jpwl_hprot_TPH[tilespec++] = hprot;
							}


						} else if(!strcmp(token, "h")) {
							/* Main header, unspecified */
							parameters->jpwl_hprot_MH = hprot;

						} else {
							fprintf(stderr, "ERROR -> invalid protection method selection = %s\n", token);
							return 1;
						};

					}

					/* search packet error protection method */
					if(*token == 'p') {

						static int pack = 0, tile = 0, packspec = 0, lastpackno = 0;

						pprot = 1; /* predefined method */

						if(sscanf(token, "p=%d", &pprot) == 1) {
							/* Method for all tiles and all packets */
							if(!((pprot == 0) || (pprot == 1) || (pprot == 16) || (pprot == 32) ||
								((pprot >= 37) && (pprot <= 128)))) {
								fprintf(stderr, "ERROR -> invalid default packet protection method p = %d\n", pprot);
								return 1;
							}
							parameters->jpwl_pprot_tileno[0] = 0;
							parameters->jpwl_pprot_packno[0] = 0;
							parameters->jpwl_pprot[0] = pprot;

						} else if(sscanf(token, "p%d=%d", &tile, &pprot) == 2) {
							/* method specified from that tile on */
							if(!((pprot == 0) || (pprot == 1) || (pprot == 16) || (pprot == 32) ||
								((pprot >= 37) && (pprot <= 128)))) {
								fprintf(stderr, "ERROR -> invalid packet protection method p = %d\n", pprot);
								return 1;
							}
							if(tile < 0) {
								fprintf(stderr, "ERROR -> invalid tile part number on protection method p = %d\n", tile);
								return 1;
							}
							if(packspec < JPWL_MAX_NO_PACKSPECS) {
								parameters->jpwl_pprot_tileno[packspec] = tile;
								parameters->jpwl_pprot_packno[packspec] = 0;
								parameters->jpwl_pprot[packspec++] = pprot;
							}

						} else if(sscanf(token, "p%d:%d=%d", &tile, &pack, &pprot) == 3) {
							/* method fully specified from that tile and that packet on */
							if(!((pprot == 0) || (pprot == 1) || (pprot == 16) || (pprot == 32) ||
								((pprot >= 37) && (pprot <= 128)))) {
								fprintf(stderr, "ERROR -> invalid packet protection method p = %d\n", pprot);
								return 1;
							}
							if(tile < 0) {
								fprintf(stderr, "ERROR -> invalid tile part number on protection method p = %d\n", tile);
								return 1;
							}
							if(pack < 0) {
								fprintf(stderr, "ERROR -> invalid packet number on protection method p = %d\n", pack);
								return 1;
							}
							if(packspec < JPWL_MAX_NO_PACKSPECS) {
								parameters->jpwl_pprot_tileno[packspec] = tile;
								parameters->jpwl_pprot_packno[packspec] = pack;
								parameters->jpwl_pprot[packspec++] = pprot;
							}

						} else if(sscanf(token, "p%d:%d", &tile, &pack) == 2) {
							/* default method from that tile and that packet on */
							if(!((pprot == 0) || (pprot == 1) || (pprot == 16) || (pprot == 32) ||
								((pprot >= 37) && (pprot <= 128)))) {
								fprintf(stderr, "ERROR -> invalid packet protection method p = %d\n", pprot);
								return 1;
							}
							if(tile < 0) {
								fprintf(stderr, "ERROR -> invalid tile part number on protection method p = %d\n", tile);
								return 1;
							}
							if(pack < 0) {
								fprintf(stderr, "ERROR -> invalid packet number on protection method p = %d\n", pack);
								return 1;
							}
							if(packspec < JPWL_MAX_NO_PACKSPECS) {
								parameters->jpwl_pprot_tileno[packspec] = tile;
								parameters->jpwl_pprot_packno[packspec] = pack;
								parameters->jpwl_pprot[packspec++] = pprot;
							}

						} else if(sscanf(token, "p%d", &tile) == 1) {
							/* default from a tile on */
							if(tile < 0) {
								fprintf(stderr, "ERROR -> invalid tile part number on protection method p = %d\n", tile);
								return 1;
							}
							if(packspec < JPWL_MAX_NO_PACKSPECS) {
								parameters->jpwl_pprot_tileno[packspec] = tile;
								parameters->jpwl_pprot_packno[packspec] = 0;
								parameters->jpwl_pprot[packspec++] = pprot;
							}


						} else if(!strcmp(token, "p")) {
							/* all default */
							parameters->jpwl_pprot_tileno[0] = 0;
							parameters->jpwl_pprot_packno[0] = 0;
							parameters->jpwl_pprot[0] = pprot;

						} else {
							fprintf(stderr, "ERROR -> invalid protection method selection = %s\n", token);
							return 1;
						};

					}

					/* search sensitivity method */
					if(*token == 's') {

						static int tile = 0, tilespec = 0, lasttileno = 0;

						sens = 0; /* predefined: relative error */

						if(sscanf(token, "s=%d", &sens) == 1) {
							/* Main header, specified */
							if((sens < -1) || (sens > 7)) {
								fprintf(stderr, "ERROR -> invalid main header sensitivity method s = %d\n", sens);
								return 1;
							}
							parameters->jpwl_sens_MH = sens;

						} else if(sscanf(token, "s%d=%d", &tile, &sens) == 2) {
							/* Tile part header, specified */
							if((sens < -1) || (sens > 7)) {
								fprintf(stderr, "ERROR -> invalid tile part header sensitivity method s = %d\n", sens);
								return 1;
							}
							if(tile < 0) {
								fprintf(stderr, "ERROR -> invalid tile part number on sensitivity method t = %d\n", tile);
								return 1;
							}
							if(tilespec < JPWL_MAX_NO_TILESPECS) {
								parameters->jpwl_sens_TPH_tileno[tilespec] = lasttileno = tile;
								parameters->jpwl_sens_TPH[tilespec++] = sens;
							}

						} else if(sscanf(token, "s%d", &tile) == 1) {
							/* Tile part header, unspecified */
							if(tile < 0) {
								fprintf(stderr, "ERROR -> invalid tile part number on sensitivity method t = %d\n", tile);
								return 1;
							}
							if(tilespec < JPWL_MAX_NO_TILESPECS) {
								parameters->jpwl_sens_TPH_tileno[tilespec] = lasttileno = tile;
								parameters->jpwl_sens_TPH[tilespec++] = hprot;
							}

						} else if(!strcmp(token, "s")) {
							/* Main header, unspecified */
							parameters->jpwl_sens_MH = sens;

						} else {
							fprintf(stderr, "ERROR -> invalid sensitivity method selection = %s\n", token);
							return 1;
						};
						
						parameters->jpwl_sens_size = 2; /* 2 bytes for default size */
					}

					/* search addressing size */
					if(*token == 'a') {

						static int tile = 0, tilespec = 0, lasttileno = 0;

						addr = 0; /* predefined: auto */

						if(sscanf(token, "a=%d", &addr) == 1) {
							/* Specified */
							if((addr != 0) && (addr != 2) && (addr != 4)) {
								fprintf(stderr, "ERROR -> invalid addressing size a = %d\n", addr);
								return 1;
							}
							parameters->jpwl_sens_addr = addr;

						} else if(!strcmp(token, "a")) {
							/* default */
							parameters->jpwl_sens_addr = addr; /* auto for default size */

						} else {
							fprintf(stderr, "ERROR -> invalid addressing selection = %s\n", token);
							return 1;
						};
						
					}

					/* search sensitivity size */
					if(*token == 'z') {

						static int tile = 0, tilespec = 0, lasttileno = 0;

						size = 1; /* predefined: 1 byte */

						if(sscanf(token, "z=%d", &size) == 1) {
							/* Specified */
							if((size != 0) && (size != 1) && (size != 2)) {
								fprintf(stderr, "ERROR -> invalid sensitivity size z = %d\n", size);
								return 1;
							}
							parameters->jpwl_sens_size = size;

						} else if(!strcmp(token, "a")) {
							/* default */
							parameters->jpwl_sens_size = size; /* 1 for default size */

						} else {
							fprintf(stderr, "ERROR -> invalid size selection = %s\n", token);
							return 1;
						};
						
					}

					/* search range method */
					if(*token == 'g') {

						static int tile = 0, tilespec = 0, lasttileno = 0;

						range = 0; /* predefined: 0 (packet) */

						if(sscanf(token, "g=%d", &range) == 1) {
							/* Specified */
							if((range < 0) || (range > 3)) {
								fprintf(stderr, "ERROR -> invalid sensitivity range method g = %d\n", range);
								return 1;
							}
							parameters->jpwl_sens_range = range;

						} else if(!strcmp(token, "g")) {
							/* default */
							parameters->jpwl_sens_range = range;

						} else {
							fprintf(stderr, "ERROR -> invalid range selection = %s\n", token);
							return 1;
						};
						
					}

					/* next token or bust */
					token = strtok(NULL, ",");
				};


				/* some info */
				fprintf(stdout, "Info: JPWL capabilities enabled\n");
				parameters->jpwl_epc_on = true;

			}
			break;
#endif /* USE_JPWL */
/* <<UniPG */
/* ------------------------------------------------------ */
			
			break;
				/* ------------------------------------------------------ */

			default:
				fprintf(stderr, "ERROR -> Command line not valid\n");
				return 1;
		}
	}

	/* check for possible errors */
	if(parameters->cp_cinema){
		if(parameters->tcp_numlayers > 1){
			parameters->cp_rsiz = OPJ_STD_RSIZ;
     	fprintf(stdout,"Warning: DC profiles do not allow more than one quality layer. The codestream created will not be compliant with the DC profile\n");
		}
	}

	if((parameters->cp_disto_alloc || parameters->cp_fixed_alloc || parameters->cp_fixed_quality)
		&& (!(parameters->cp_disto_alloc ^ parameters->cp_fixed_alloc ^ parameters->cp_fixed_quality))) {
		fprintf(stderr, "Error: options -r -q and -f cannot be used together !!\n");
		return 1;
	}				/* mod fixed_quality */

	/* if no rate entered, lossless by default */
	if(parameters->tcp_numlayers == 0) {
		parameters->tcp_rates[0] = 0;	/* MOD antonin : losslessbug */
		parameters->tcp_numlayers++;
		parameters->cp_disto_alloc = 1;
	}

	if((parameters->cp_tx0 > parameters->image_offset_x0) || (parameters->cp_ty0 > parameters->image_offset_y0)) {
		fprintf(stderr,
			"Error: Tile offset dimension is unnappropriate --> TX0(%d)<=IMG_X0(%d) TYO(%d)<=IMG_Y0(%d) \n",
			parameters->cp_tx0, parameters->image_offset_x0, parameters->cp_ty0, parameters->image_offset_y0);
		return 1;
	}

	for(i = 0; i < parameters->numpocs; i++) {
		if(parameters->POC[i].prg == -1) {
			fprintf(stderr,
				"Unrecognized progression order in option -P (POC n %d) [LRCP, RLCP, RPCL, PCRL, CPRL] !!\n",
				i + 1);
		}
	}

	return 0;
}

/* -------------------------------------------------------------------------- 
   ------------ Get the image byte[] from the Java object -------------------*/

static opj_image_t* loadImage(opj_cparameters_t *parameters, 
	JNIEnv *env, jobject obj, jclass cls) 
{
	opj_image_t * image = NULL;
	int *red, *green, *blue;
	opj_image_cmptparm_t cmptparm[4];
	OPJ_COLOR_SPACE color_space;
	int i,width,height,depth;
	int numcomps, sub_dx, sub_dy;
	jfieldID	fid;
	jint		ji;
	jbyteArray	jba;
	jshortArray jsa;
	jintArray	jia;
	int			len;
	jbyte		*jbBody;
	jshort		*jsBody;
	jint		*jiBody;
	jboolean		isCopy;

	/* Image width, height and depth*/
	fid = (*env)->GetFieldID(env, cls,"width", "I");
	ji = (*env)->GetIntField(env, obj, fid);
	width = (int)ji;

	fid = (*env)->GetFieldID(env, cls,"height", "I");
	ji = (*env)->GetIntField(env, obj, fid);
	height = (int)ji;
	
	fid = (*env)->GetFieldID(env, cls,"depth", "I");
	ji = (*env)->GetIntField(env, obj, fid);
	depth = (int)ji;

	if(width <= 0 || height <= 0 || depth <= 0)
   {
	fprintf(stderr,"%s:%d:\n\tERROR in loadImage:\n\t"
	 "width(%d) height(%d) depth(%d)\n",__FILE__,__LINE__,
	 width,height,depth);
	return NULL;
   }
	if(depth <=16) 
   {
	numcomps = 1;
	color_space = OPJ_CLRSPC_GRAY;
   } 
	else 
   {
	numcomps = 3;
	color_space = OPJ_CLRSPC_SRGB;
   }
	memset(&cmptparm[0], 0, numcomps * sizeof(opj_image_cmptparm_t));

	sub_dx = parameters->subsampling_dx;
	sub_dy = parameters->subsampling_dy;

	for(i = 0; i < numcomps; i++) 
   {
	cmptparm[i].prec = depth/numcomps;
	cmptparm[i].bpp = depth/numcomps;
	cmptparm[i].sgnd = 0;
	cmptparm[i].dx = sub_dx;
	cmptparm[i].dy = sub_dy;
	cmptparm[i].w = width;
	cmptparm[i].h = height;
   }
/*----------------------------------------------------------------*/
	image = opj_image_create(numcomps, &cmptparm[0], color_space);
/*----------------------------------------------------------------*/
	if (!image) 
   {
	fprintf(stderr,"%s:%d:\n\tERROR: opj_image_create failed.\n",
	 __FILE__,__LINE__);
	return NULL;
   }

/* set image offset and reference grid:
*/
    image->x0 = parameters->image_offset_x0;
    image->y0 = parameters->image_offset_y0;
    image->x1 = image->x0 + (width  - 1) * sub_dx + 1 + image->x0;
    image->y1 = image->y0 + (height - 1) * sub_dy + 1 + image->y0;

	if(numcomps == 1)
   {
	red = image->comps[0].data;

	if(depth <= 8)/*FIXME SIGNED */
  {
	fid = (*env)->GetFieldID(env, cls,"image8", "[B");	/* byteArray []*/
	jba = (*env)->GetObjectField(env, obj, fid);
	len = (*env)->GetArrayLength(env, jba);

	jbBody = (*env)->GetPrimitiveArrayCritical(env, jba, &isCopy);

	for(i=0; i< len;i++) 
	 *red++ = jbBody[i];

	(*env)->ReleasePrimitiveArrayCritical(env, jba, jbBody, 0);
  }
	else
	if(depth <= 16)
  {
    fid = (*env)->GetFieldID(env, cls,"image16", "[S"); /* shortArray []*/
    jsa = (*env)->GetObjectField(env, obj, fid);
    len = (*env)->GetArrayLength(env, jsa);

    jsBody = (*env)->GetPrimitiveArrayCritical(env, jsa, &isCopy);

    for(i=0; i< len;i++)
     *red++ = jsBody[i];

    (*env)->ReleasePrimitiveArrayCritical(env, jsa, jsBody, 0);
  }
   }
	else
/*	if(numcomps == 3) */
   {
	int v;

	red   = image->comps[0].data;
	green = image->comps[1].data;
	blue  = image->comps[2].data;

	fid = (*env)->GetFieldID(env, cls,"image24", "[I");	/* intArray []*/
	jia = (*env)->GetObjectField(env, obj, fid);
	len = (*env)->GetArrayLength(env, jia);

	jiBody = (*env)->GetPrimitiveArrayCritical(env, jia, &isCopy);

	for(i = 0; i < len; ++i)
  {
	v = (int)jiBody[i];
	
	*red++ =  (v>>16)&0xff;
	*green++ = (v>>8)&0xff;
	*blue++ = v&0xff;

  }	/*for(i = 0 */

	(*env)->ReleasePrimitiveArrayCritical(env, jia, jiBody, 0);
   }

	return image;

}/* loadImage() */


static unsigned char *dump_file(const char *infile, const char *outfile,
	int format, OPJ_OFF_T *out_len)
{
	FILE *fout = NULL;
	opj_dparameters_t parameters;
	opj_image_t* image = NULL;
	opj_codec_t* l_codec = NULL;
	opj_stream_t *l_stream = NULL;
	unsigned char *buf;
	OPJ_OFF_T len;
	int flags = OPJ_IMG_INFO | OPJ_J2K_MH_INFO | OPJ_J2K_MH_IND;
	OPJ_CODEC_FORMAT codec_format;

	if(format == J2K_CFMT)
	 codec_format = OPJ_CODEC_J2K;
	else
	if(format == JP2_CFMT)
	 codec_format = OPJ_CODEC_JP2;
	else
	if(format == JPT_CFMT)
	 codec_format = OPJ_CODEC_JPT;
	else
	 return NULL;

	opj_set_default_decoder_parameters(&parameters);

	strcpy(parameters.outfile, outfile);

	parameters.decod_format = format;

	fout = fopen(outfile,"w");

	if(!fout)
   {
	fprintf(stderr, "dump_file: failed to open %s for writing\n",
	 outfile);
	return NULL;
   }
	strcpy(parameters.infile, infile);

	buf = NULL;

	l_stream =
	 opj_stream_create_default_file_stream(infile, 1);/*READ*/

	if(!l_stream)
   {
	fprintf(stderr, "dump_file: opj_stream_create_default_file_stream"
	 " failed.\n");
	goto fin;
   }

	l_codec = opj_create_decompress(codec_format);

	if(!l_codec)
   {
	fprintf(stderr, "dump_file: opj_create_decompress failed.\n");
	goto fin;
   }

	if( !opj_setup_decoder(l_codec, &parameters) )
   {
	fprintf(stderr, "dump_file: opj_setup_decoder failed.\n");
	goto fin;
   }

	if(! opj_read_header(l_stream, l_codec, &image))
   {
	fprintf(stderr, "dump_file: opj_read_header failed.\n");
	goto fin;
   }
/*-------------------------------------------*/
	opj_dump_codec(l_codec, flags, fout);
/*-------------------------------------------*/
    len = (OPJ_OFF_T)OPJ_FTELL(fout);
    OPJ_FSEEK(fout, 0, SEEK_SET);

	buf = (unsigned char*)opj_malloc(len + 4);

	if(buf)
   {
	fread(buf, 1, len, fout);
	*out_len = len; buf[len] = 0;
   }
fin:

	if(l_stream) opj_stream_destroy(l_stream);

	if(l_codec) opj_destroy_codec(l_codec);

	if(image) opj_image_destroy(image);

	fclose(fout);

	return buf;

}/* dump_file() */

/* ------------------------------------------------------------------------
   ------------------   MAIN METHOD, CALLED BY JAVA -----------------------*/
JNIEXPORT jlong JNICALL _Java_org_openJpeg_OpenJPEGJavaEncoder_internalEncodeImageToJ2K(JNIEnv *env, jobject obj, jobjectArray javaParameters)
{
/* To simulate the command line parameters (taken from the javaParameters
 * variable) and be able to re-use the 'parse_cmdline_encoder' method:
*/
	int argc;
	const char **argv;

	opj_cparameters_t parameters;	/* compression parameters */
	img_fol_t img_fol;
	int i,j,num_images,imageno;
/* Codestream information structure :*/
	char indexfilename[OPJ_PATH_LEN];
/* ==> Access variables to the Java member variables*/
	jsize		arraySize;
	jclass		cls;
	jobject		object;
	jboolean	isCopy;
	jfieldID	fid;
	jbyteArray	jba;
	jbyte		*jbBody;
	callback_variables_t msgErrorCallback_vars;
/* <== access variable to the Java member variables.*/

/* For the encoding and storage into the file*/
	OPJ_OFF_T codestream_length = -1;

/* JNI reference to the calling class*/
	cls = (*env)->GetObjectClass(env, obj);
	
/* Pointers to be able to call a Java method for all the info 
   and error messages
*/
	msgErrorCallback_vars.env = env;
	msgErrorCallback_vars.jobj = &obj;
	msgErrorCallback_vars.message_mid = 
	 (*env)->GetMethodID(env, cls, "logMessage", "(Ljava/lang/String;)V");
	msgErrorCallback_vars.error_mid = 
	 (*env)->GetMethodID(env, cls, "logError", "(Ljava/lang/String;)V");

	arraySize = (*env)->GetArrayLength(env, javaParameters);
	argc = (int) arraySize +1;
	argv = (const char**)opj_malloc(argc*sizeof(char*));

	argv[0] = "ProgramName"; /* The program name: unused */

	for(i=1; i<argc; i++) 
   {
	object = (*env)->GetObjectArrayElement(env, javaParameters, i-1);
	argv[i] = (char*)(*env)->GetStringUTFChars(env, object, &isCopy);
   }

#ifdef DEBUG_ARGS
	for(i=0; i<argc; i++) 
   {
	printf("[%s] ",argv[i]);
   }
	puts("\n");
#endif

	opj_set_default_encoder_parameters(&parameters);

	parameters.cod_format = J2K_CFMT;

	*indexfilename = 0;
	memset(&img_fol,0,sizeof(img_fol_t));

	j =
/*-------------------------------------------------------------------------*/ 
	 parse_cmdline_encoder(argc, (char *const*)argv, &parameters,&img_fol,
		indexfilename);
/*-------------------------------------------------------------------------*/ 

/* Release the Java arguments array*/
	for(i=1; i<argc; i++)
   {  
	(*env)->ReleaseStringUTFChars(env, 
		(*env)->GetObjectArrayElement(env, javaParameters, i-1), argv[i]);
   }
	opj_free(argv);

	if(j == 1) return -1;/* Failure */

	/*TESTS for parameters.outfile */
 
	if(parameters.cp_cinema)
   {
	cinema_parameters(&parameters);
   }
/* Create comment for codestream */
	if(parameters.cp_comment == NULL) 
   {
	const char comment[] = "Created by JavaOpenJPEG version ";
	const size_t clen = strlen(comment);
	const char *version = opj_version();
/* UniPG>> */
#ifdef USE_JPWL
	parameters.cp_comment = (char*)opj_malloc(clen+strlen(version)+11);
	sprintf(parameters.cp_comment,"%s%s with JPWL", comment, version);
#else
	parameters.cp_comment = (char*)opj_malloc(clen+strlen(version)+1);
	sprintf(parameters.cp_comment,"%s%s", comment, version);
#endif
/* <<UniPG */

   }

/* Read directory if necessary */
	num_images = 1;

/* Encoding image one by one */
	for(imageno=0;imageno<num_images;imageno++)
   {
    opj_stream_t *l_stream;
    opj_codec_t* l_codec;
    opj_image_t *image;
	OPJ_CODEC_FORMAT codec_format;

	l_stream = NULL;
	l_codec = NULL;
/*-------------------------------------------------*/
	image = loadImage(&parameters, env, obj, cls);
/*-------------------------------------------------*/
	if(!image) 
  {
	fprintf(stderr, "Unable to load image\n");
	goto fin;
  }
/* Decide if MCT should be used */
	parameters.tcp_mct = image->numcomps == 3 ? 1 : 0;

	if(parameters.cp_cinema)
  {
	cinema_setup_encoder(&parameters,image,&img_fol);
  }

/* get a J2K compressor handle */
	if(parameters.cod_format == J2K_CFMT)
	 codec_format = OPJ_CODEC_J2K;
	else
	if(parameters.cod_format == JP2_CFMT)
	 codec_format = OPJ_CODEC_JP2;
	else
  {	
/* Already checked in parse_cmdline() */
	goto fin;
  }
	l_codec = opj_create_compress(codec_format);

	if(l_codec == NULL) goto fin;

/* catch events using our callbacks and give a local context:
*/
	opj_set_info_handler(l_codec, info_callback,&msgErrorCallback_vars);
	opj_set_warning_handler(l_codec, warning_callback,&msgErrorCallback_vars);
	opj_set_error_handler(l_codec, error_callback,&msgErrorCallback_vars);

	if( !opj_setup_encoder(l_codec, &parameters, image)) goto fin;

	l_stream = 
	 opj_stream_create_default_file_stream(parameters.outfile, IS_READER);

	if(l_stream == NULL) goto fin;

	if( !opj_start_compress(l_codec,image,l_stream)) goto fin;

	if( !opj_encode(l_codec, l_stream)) goto fin;

	if( !opj_end_compress(l_codec, l_stream)) goto fin;

/* Write the generated codestream to the Java pre-allocated 
   compressedStream byte[] 
*/
  {
	unsigned char *buf;
	FILE *reader;

	if((reader = fopen(parameters.outfile, "rb")) == NULL)
 {
	printf("%s:%d:Can not open outfile\n\t%s\n",__FILE__,__LINE__,
	 parameters.outfile);
	goto fin;
 }
	OPJ_FSEEK(reader, 0, SEEK_END);
	codestream_length = (OPJ_OFF_T)OPJ_FTELL(reader);
	OPJ_FSEEK(reader, 0, SEEK_SET);

	if(codestream_length <= 0) goto fin;

	buf = (unsigned char*)opj_malloc(codestream_length);

	if(buf == NULL) goto fin;

	fread(buf, 1, codestream_length, reader);
	fclose(reader);

	fid = (*env)->GetFieldID(env, cls,"compressedStream", "[B");
	jba = (*env)->GetObjectField(env, obj, fid);

	jbBody = (*env)->GetPrimitiveArrayCritical(env, jba, 0);
	memcpy(jbBody, buf, codestream_length);

	opj_free(buf);
	(*env)->ReleasePrimitiveArrayCritical(env, jba, jbBody, 0);
  }

  {
	char *fname, *root_dir;
	size_t len;

	fname = NULL;
#ifdef _WIN32
    if((root_dir = (char *) getenv("UserProfile")))
     root_dir = (char *)strdup(root_dir);
    else
     root_dir = (char *)strdup("C:\\Windows\\Temp");
	len = strlen(root_dir);

	fname = (char*)opj_malloc(len + 32);

/*	if(fname)
     sprintf_s(fname, len+30, "%s/.java_decoder_infile", root_dir);*/
#else
    root_dir = strdup(getenv("HOME"));
	len = strlen(root_dir);

	fname = (char*)opj_malloc(strlen(root_dir) + 24);

	if(fname)
     snprintf(fname, len+30, "%s/.java_decoder_infile", root_dir);
#endif

	if(fname)
 {
	unsigned char *buf = NULL;
	OPJ_OFF_T buf_len = 0;

	buf = 
/*-----------------------------------------------------------------------*/
	dump_file(parameters.outfile, fname, parameters.cod_format, &buf_len);
/*-----------------------------------------------------------------------*/
	remove(fname);
	opj_free(fname); fname = NULL;

		if(buf)
	   {
		fid = (*env)->GetFieldID(env, cls,"compressedStream", "[B");
		jba = (*env)->NewByteArray(env, buf_len+1);
		jbBody = (*env)->GetPrimitiveArrayCritical(env, jba, 0);

		memcpy(jbBody, buf, buf_len);

		(*env)->ReleasePrimitiveArrayCritical(env, jba, jbBody, 0);
		(*env)->SetObjectField(env, obj, fid, jba);

		if(indexfilename[0]
#ifdef USE_JPWL
		&& strcmp(indexfilename, JPWL_PRIVATEINDEX_NAME)
#endif
		  )
	  {
		FILE *writer;

		writer = fopen(indexfilename, "wb");

		if(writer)
	 {
		fwrite(buf, 1, len, writer);

		fclose(writer);
	 }
	  }
		opj_free(buf);
	   }
		
 }	/* if(fname) */
  }
fin:
	if(l_stream) opj_stream_destroy(l_stream);

	if(l_codec) opj_destroy_codec(l_codec);

	if(image) opj_image_destroy(image);

	}/* for(imageno=0  */

	if(parameters.cp_comment) opj_free(parameters.cp_comment);
	if(parameters.cp_matrice) opj_free(parameters.cp_matrice);
	if(parameters.cp_cinema)  opj_free(img_fol.rates);

	return codestream_length;
}/* MAIN Java_org_openJpeg_OpenJPEGJavaEncoder_internalEncodeImageToJ2K() */

