/*
 * Copyright (c) 2002-2014, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2014, Professor Benoit Macq
 * Copyright (c) 2001-2003, David Janssens
 * Copyright (c) 2002-2003, Yannick Verschueren
 * Copyright (c) 2003-2007, Francois-Olivier Devaux
 * Copyright (c) 2003-2014, Antonin Descampe
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
#include "opj_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <jni.h>
#include <math.h>

#include "openjpeg.h"
#include "opj_includes.h"
#include "opj_getopt.h"
#include "color.h"
#include "convert.h"
#include "dirent.h"
#include "org_openJpeg_OpenJPEGJavaDecoder.h"

#ifndef _WIN32
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

#include "format_defs.h"
#include "java_helpers.h"
#define IS_READER 1

#define JP2_RFC3745_MAGIC "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a"
#define JP2_MAGIC "\x0d\x0a\x87\x0a"
#define J2K_CODESTREAM_MAGIC "\xff\x4f\xff\x51"
#define J2K_CODESTREAM_MAGIC "\xff\x4f\xff\x51"

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

static int buffer_format(opj_buffer_info_t* buf_info) {
 	int magic_format;
	if (!buf_info || buf_info->len < 12)
		return -1;
	if (memcmp(buf_info->buf, JP2_RFC3745_MAGIC, 12) == 0
			|| memcmp(buf_info->buf, JP2_MAGIC, 4) == 0) {
		magic_format = JP2_CFMT;
	} else {
		if (memcmp(buf_info->buf, J2K_CODESTREAM_MAGIC, 4) == 0) {
			magic_format = J2K_CFMT;
		} else
			return -1;
	}
	return magic_format;
}/*  buffer_format() */

static int ext_file_format(const char *filename) {
	unsigned int i;

	static const char *extension[] = { "j2k", "jp2", "jpt", "j2c", "jpc" };

	static const int format[] = {
	J2K_CFMT, JP2_CFMT, JPT_CFMT, J2K_CFMT, J2K_CFMT };

	char *ext = strrchr(filename, '.');

	if (ext == NULL)
		return -1;

	ext++;
	if (*ext) {
		for (i = 0; i < sizeof(format) / sizeof(*format); i++) {
			if (strnicmp(ext, extension[i], 3) == 0)
				return format[i];
		}
	}
	return -1;
}

#define JP2_RFC3745_MAGIC "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a"
#define JP2_MAGIC "\x0d\x0a\x87\x0a"
/* position 45: "\xff\x52" */
#define J2K_CODESTREAM_MAGIC "\xff\x4f\xff\x51"
static const char *bar = "\n===========================================\n";

static int infile_format(const char *fname) {
	FILE *reader;
	const char *s, *magic_s;
	int ext_format, magic_format;
	unsigned char buf[12];

	reader = fopen(fname, "rb");

	if (reader == NULL) {
		fprintf(stderr, "%s:%d:can not open file\n\t%s\n", __FILE__, __LINE__,
				fname);
		return -1;
	}
	memset(buf, 0, 12);
	fread(buf, 1, 12, reader);
	fclose(reader);

	ext_format = ext_file_format(fname);

	if (ext_format == JPT_CFMT)
		return JPT_CFMT;

	if (memcmp(buf, JP2_RFC3745_MAGIC, 12) == 0
			|| memcmp(buf, JP2_MAGIC, 4) == 0) {
		magic_format = JP2_CFMT;
		magic_s = "'.jp2'";
	} else if (memcmp(buf, J2K_CODESTREAM_MAGIC, 4) == 0) {
		magic_format = J2K_CFMT;
		magic_s = "'.j2k' or '.jpc' or '.j2c'";
	} else
		return -1;

	if (magic_format == ext_format)
		return ext_format;

	s = fname + strlen(fname) - 4;

	fputs(bar, stderr);
	fprintf(stderr, "The extension of the file\n%s\nis incorrect.\n"
			"FOUND '%s'. SHOULD BE %s", fname, s, magic_s);
	fputs(bar, stderr);

	return magic_format;
}/* infile_format() */



typedef struct img_folder{
	/** The directory path of the folder containing input images*/
	char *imgdirpath;
	/** Output format*/
	char *out_format;
	/** Enable option*/
	char set_imgdir;
	/** Enable Cod Format for output*/
	char set_out_format;

}img_fol_t;


void decode_help_display() {
	fprintf(stdout,"HELP\n----\n\n");
	fprintf(stdout,"- the -h option displays this help information on screen\n\n");

/* UniPG>> */
	fprintf(stdout,"List of parameters for the JPEG 2000 "
#ifdef USE_JPWL
		"+ JPWL "
#endif /* USE_JPWL */
		"decoder:\n");
/* <<UniPG */
	fprintf(stdout,"\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"  -ImgDir \n");
	fprintf(stdout,"	Image file Directory path \n");
	fprintf(stdout,"  -OutFor \n");
	fprintf(stdout,"    REQUIRED only if -ImgDir is used\n");
	fprintf(stdout,"	  Need to specify only format without filename <BMP>  \n");
	fprintf(stdout,"    Currently accepts PGM, PPM, PNM, PGX, BMP format\n");
	fprintf(stdout,"  -i <compressed file>\n");
	fprintf(stdout,"    REQUIRED only if an Input image directory not specified\n");
	fprintf(stdout,"    Currently accepts J2K-files, JP2-files and JPT-files. The file type\n");
	fprintf(stdout,"    is identified based on its suffix.\n");
	fprintf(stdout,"  -o <decompressed file>\n");
	fprintf(stdout,"    REQUIRED\n");
	fprintf(stdout,"    Currently accepts PGM-files, PPM-files, PNM-files, PGX-files and\n");
	fprintf(stdout,"    BMP-files. Binary data is written to the file (not ascii). If a PGX\n");
	fprintf(stdout,"    filename is given, there will be as many output files as there are\n");
	fprintf(stdout,"    components: an indice starting from 0 will then be appended to the\n");
	fprintf(stdout,"    output filename, just before the \"pgx\" extension. If a PGM filename\n");
	fprintf(stdout,"    is given and there are more than one component, only the first component\n");
	fprintf(stdout,"    will be written to the file.\n");
	fprintf(stdout,"  -r <reduce factor>\n");
	fprintf(stdout,"    Set the number of highest resolution levels to be discarded. The\n");
	fprintf(stdout,"    image resolution is effectively divided by 2 to the power of the\n");
	fprintf(stdout,"    number of discarded levels. The reduce factor is limited by the\n");
	fprintf(stdout,"    smallest total number of decomposition levels among tiles.\n");
	fprintf(stdout,"  -l <number of quality layers to decode>\n");
	fprintf(stdout,"    Set the maximum number of quality layers to decode. If there are\n");
	fprintf(stdout,"    less quality layers than the specified number, all the quality layers\n");
	fprintf(stdout,"    are decoded.\n");
/* UniPG>> */
#ifdef USE_JPWL
	fprintf(stdout,"  -W <options>\n");
	fprintf(stdout,"    Activates the JPWL correction capability, if the codestream complies.\n");
	fprintf(stdout,"    Options can be a comma separated list of <param=val> tokens:\n");
	fprintf(stdout,"    c, c=numcomps\n");
	fprintf(stdout,"       numcomps is the number of expected components in the codestream\n");
	fprintf(stdout,"       (search of first EPB rely upon this, default is %d)\n", JPWL_EXPECTED_COMPONENTS);
#endif /* USE_JPWL */
/* <<UniPG */
	fprintf(stdout,"\n");
}

/* -------------------------------------------------------------------------- */

int get_num_images(char *imgdirpath){
	DIR *dir;
	struct dirent* content;
	int num_images = 0;

	/*Reading the input images from given input directory*/

	dir= opendir(imgdirpath);
	if(!dir){
		fprintf(stderr,"Could not open Folder %s\n",imgdirpath);
		return 0;
	}

	while((content=readdir(dir))!=NULL){
		if(strcmp(".",content->d_name)==0 || strcmp("..",content->d_name)==0 )
			continue;
		num_images++;
	}
	return num_images;
}

int load_images(dircnt_t *dirptr, char *imgdirpath){
	DIR *dir;
	struct dirent* content;
	int i = 0;

	/*Reading the input images from given input directory*/

	dir= opendir(imgdirpath);
	if(!dir){
		fprintf(stderr,"Could not open Folder %s\n",imgdirpath);
		return 1;
	}else	{
		fprintf(stderr,"Folder opened successfully\n");
	}

	while((content=readdir(dir))!=NULL){
		if(strcmp(".",content->d_name)==0 || strcmp("..",content->d_name)==0 )
			continue;

		strcpy(dirptr->filename[i],content->d_name);
		i++;
	}
	return 0;
}

int get_file_format(char *filename) {
	unsigned int i;
	static const char *extension[] = {"pgx", "pnm", "pgm", "ppm", "bmp","tif", "raw", "tga", "j2k", "jp2", "jpt", "j2c" };
	static const int format[] = { PGX_DFMT, PXM_DFMT, PXM_DFMT, PXM_DFMT, BMP_DFMT, TIF_DFMT, RAW_DFMT, TGA_DFMT, J2K_CFMT, JP2_CFMT, JPT_CFMT, J2K_CFMT };
	char * ext = strrchr(filename, '.');
	if (ext == NULL)
		return -1;
	ext++;
	if(ext) {
		for(i = 0; i < sizeof(format)/sizeof(*format); i++) {
			if(strnicmp(ext, extension[i], 3) == 0) {
				return format[i];
			}
		}
	}

	return -1;
}


/* -------------------------------------------------------------------------- */

int parse_cmdline_decoder(int argc, char **argv, opj_dparameters_t *parameters,img_fol_t *img_fol) {
	/* parse the command line */
	int totlen;
	opj_option_t long_option[]={
		{"ImgDir",REQ_ARG, NULL ,'y'},
		{"OutFor",REQ_ARG, NULL ,'O'},
	};

/* UniPG>> */
	const char optlist[] = "i:o:r:l:hx:"

#ifdef USE_JPWL
					"W:"
#endif /* USE_JPWL */
					;
	/*for (i=0; i<argc; i++) {
		printf("[%s]",argv[i]);
	}
	printf("\n");*/

/* <<UniPG */
	totlen=sizeof(long_option);
	img_fol->set_out_format = 0;
	opj_reset_options_reading();

	while (1) {
		int c = opj_getopt_long(argc, argv,optlist,long_option,totlen);
		if (c == -1)
			break;
		switch (c) {
			case 'i':			/* input file */
			{
				char *infile = opj_optarg;
				parameters->decod_format = get_file_format(infile);
				switch(parameters->decod_format) {
					case J2K_CFMT:
					case JP2_CFMT:
					case JPT_CFMT:
						break;
					default:
						fprintf(stderr,
							"!! Unrecognized format for infile : %s [accept only *.j2k, *.jp2, *.jpc or *.jpt] !!\n\n",
							infile);
						return 1;
				}
				strncpy(parameters->infile, infile, sizeof(parameters->infile)-1);
			}
			break;

				/* ----------------------------------------------------- */

			case 'o':			/* output file */
			{
				char *outfile = opj_optarg;
				parameters->cod_format = get_file_format(outfile);
				switch(parameters->cod_format) {
					case PGX_DFMT:
					case PXM_DFMT:
					case BMP_DFMT:
					case TIF_DFMT:
					case RAW_DFMT:
					case TGA_DFMT:
						break;
					default:
						fprintf(stderr, "Unknown output format image %s [only *.pnm, *.pgm, *.ppm, *.pgx, *.bmp, *.tif, *.raw or *.tga]!! \n", outfile);
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
				parameters->cod_format = get_file_format(outformat);
				switch(parameters->cod_format) {
					case PGX_DFMT:
						img_fol->out_format = "pgx";
						break;
					case PXM_DFMT:
						img_fol->out_format = "ppm";
						break;
					case BMP_DFMT:
						img_fol->out_format = "bmp";
						break;
					case TIF_DFMT:
						img_fol->out_format = "tif";
						break;
					case RAW_DFMT:
						img_fol->out_format = "raw";
						break;
					case TGA_DFMT:
						img_fol->out_format = "raw";
						break;
					default:
						fprintf(stderr, "Unknown output format image %s [only *.pnm, *.pgm, *.ppm, *.pgx, *.bmp, *.tif, *.raw or *.tga]!! \n", outformat);
						return 1;
						break;
				}
			}
			break;

				/* ----------------------------------------------------- */


			case 'r':		/* reduce option */
			{
				sscanf(opj_optarg, "%d", &parameters->cp_reduce);
			}
			break;

				/* ----------------------------------------------------- */


			case 'l':		/* layering option */
			{
				sscanf(opj_optarg, "%d", &parameters->cp_layer);
			}
			break;

				/* ----------------------------------------------------- */

			case 'h': 			/* display an help description */
				decode_help_display();
				return 1;

				/* ------------------------------------------------------ */

			case 'y':			/* Image Directory path */
				{
					img_fol->imgdirpath = (char*)opj_malloc(strlen(opj_optarg) + 1);
					strcpy(img_fol->imgdirpath,opj_optarg);
					img_fol->set_imgdir=1;
				}
				break;
				/* ----------------------------------------------------- */
/* UniPG>> */
#ifdef USE_JPWL

			case 'W': 			/* activate JPWL correction */
			{
				char *token = NULL;

				token = strtok(opj_optarg, ",");
				while(token != NULL) {

					/* search expected number of components */
					if (*token == 'c') {

						static int compno;

						compno = JPWL_EXPECTED_COMPONENTS; /* predefined no. of components */

						if(sscanf(token, "c=%d", &compno) == 1) {
							/* Specified */
							if ((compno < 1) || (compno > 256)) {
								fprintf(stderr, "ERROR -> invalid number of components c = %d\n", compno);
								return 1;
							}
							parameters->jpwl_exp_comps = compno;

						} else if (!strcmp(token, "c")) {
							/* default */
							parameters->jpwl_exp_comps = compno; /* auto for default size */

						} else {
							fprintf(stderr, "ERROR -> invalid components specified = %s\n", token);
							return 1;
						};
					}

					/* search maximum number of tiles */
					if (*token == 't') {

						static int tileno;

						tileno = JPWL_MAXIMUM_TILES; /* maximum no. of tiles */

						if(sscanf(token, "t=%d", &tileno) == 1) {
							/* Specified */
							if ((tileno < 1) || (tileno > JPWL_MAXIMUM_TILES)) {
								fprintf(stderr, "ERROR -> invalid number of tiles t = %d\n", tileno);
								return 1;
							}
							parameters->jpwl_max_tiles = tileno;

						} else if (!strcmp(token, "t")) {
							/* default */
							parameters->jpwl_max_tiles = tileno; /* auto for default size */

						} else {
							fprintf(stderr, "ERROR -> invalid tiles specified = %s\n", token);
							return 1;
						};
					}

					/* next token or bust */
					token = strtok(NULL, ",");
				};
				parameters->jpwl_correct = true;
				fprintf(stdout, "JPWL correction capability activated\n");
				fprintf(stdout, "- expecting %d components\n", parameters->jpwl_exp_comps);
			}
			break;
#endif /* USE_JPWL */
/* <<UniPG */

				/* ----------------------------------------------------- */

			default:
				fprintf(stderr,"WARNING -> this option is not valid \"-%c %s\"\n",c, opj_optarg);
				break;
		}
	}

	/* No check for possible errors before the -i and -o options are of course not mandatory*/

	return 0;
}

/* -------------------------------------------------------------------------- */

/**
error callback returning the message to Java andexpecting a callback_variables_t client object
*/
void error_callback(const char *msg, void *client_data) {
	callback_variables_t* vars = (callback_variables_t*) client_data;
	JNIEnv *env = vars->env;
	jstring jbuffer;

	jbuffer = (*env)->NewStringUTF(env, msg);
	(*env)->ExceptionClear(env);
	(*env)->CallVoidMethod(env, *(vars->jobj), vars->error_mid, jbuffer);

	if ((*env)->ExceptionOccurred(env)) {
		fprintf(stderr,"C: Exception during call back method\n");
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
	(*env)->DeleteLocalRef(env, jbuffer);
}
/**
warning callback returning the message to Java andexpecting a callback_variables_t client object
*/
void warning_callback(const char *msg, void *client_data) {
	callback_variables_t* vars = (callback_variables_t*) client_data;
	JNIEnv *env = vars->env;
	jstring jbuffer;

	jbuffer = (*env)->NewStringUTF(env, msg);
	(*env)->ExceptionClear(env);
	(*env)->CallVoidMethod(env, *(vars->jobj), vars->message_mid, jbuffer);

	if ((*env)->ExceptionOccurred(env)) {
		fprintf(stderr,"C: Exception during call back method\n");
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
	(*env)->DeleteLocalRef(env, jbuffer);
}
/**
information callback returning the message to Java andexpecting a callback_variables_t client object
*/
void info_callback(const char *msg, void *client_data) {
	callback_variables_t* vars = (callback_variables_t*) client_data;
	JNIEnv *env = vars->env;
	jstring jbuffer;

	jbuffer = (*env)->NewStringUTF(env, msg);
	(*env)->ExceptionClear(env);
	(*env)->CallVoidMethod(env, *(vars->jobj), vars->message_mid, jbuffer);

	if ((*env)->ExceptionOccurred(env)) {
		fprintf(stderr,"C: Exception during call back method\n");
		(*env)->ExceptionDescribe(env);
		(*env)->ExceptionClear(env);
	}
	(*env)->DeleteLocalRef(env, jbuffer);
}

static const char *clr_space(OPJ_COLOR_SPACE i)
{
	if (i == OPJ_CLRSPC_SRGB)
		return "OPJ_CLRSPC_SRGB";
	if (i == OPJ_CLRSPC_GRAY)
		return "OPJ_CLRSPC_GRAY";
	if (i == OPJ_CLRSPC_SYCC)
		return "OPJ_CLRSPC_SYCC";
	if (i == OPJ_CLRSPC_UNKNOWN)
		return "OPJ_CLRSPC_UNKNOWN";

	return "CLRSPC_UNDEFINED";
}

/* --------------------------------------------------------------------------
 --------------------   MAIN METHOD, CALLED BY JAVA -----------------------*/
JNIEXPORT jint JNICALL Java_org_openJpeg_OpenJPEGJavaDecoder_internalDecodeJ2KtoImage(
		JNIEnv *env, jobject obj, jobjectArray javaParameters) {
	/* To simulate the command line parameters (taken from the javaParameters
	 * variable) and be able to re-use the 'parse_cmdline_decoder' method:
	 */
	int argc;
	const char **argv;

	const char *infile;
	int *red, *green, *blue, *alpha;
	opj_dparameters_t parameters;
	img_fol_t img_fol;
	opj_image_t *image = NULL;
	opj_codec_t *l_codec = NULL;
	opj_stream_t *l_stream = NULL;
	unsigned char *src = NULL;
	long file_length;
	int i, j;
	int width, height;
	int addR, addG, addB, addA, shiftR, shiftG, shiftB, shiftA;
	unsigned char hasAlpha, fails, free_infile;
	unsigned char rc, gc, bc, ac;
	OPJ_CODEC_FORMAT codec_format;
	opj_buffer_info_t buf_info;

	/* ==> Access variables to the Java member variables*/
	jsize arraySize;
	jclass cls;
	jobject object;
	jboolean isCopy;
	jfieldID fid;
	jbyteArray jba, jbaCompressed = NULL;
	jintArray jia;
	jbyte *jbBody, *ptrBBody,*jbBodyCompressed = NULL, *jbcBody = NULL;
	jint *jiBody, *ptrIBody;
	callback_variables_t msgErrorCallback_vars;
	/* <=== access variable to Java member variables */

	arraySize = (*env)->GetArrayLength(env, javaParameters);
	argc = (int) arraySize + 1;
	argv = (const char**) opj_malloc(argc * sizeof(char*));

	if (argv == NULL) {
		fprintf(stderr, "%s:%d:\n\tMEMORY OUT\n", __FILE__, __LINE__);
		return -1;
	}
	/* JNI reference to the calling class*/
	cls = (*env)->GetObjectClass(env, obj);

	/* Pointers to be able to call a Java method for all the info
	 * and error messages
	 */
	msgErrorCallback_vars.env = env;
	msgErrorCallback_vars.jobj = &obj;
	msgErrorCallback_vars.message_mid = (*env)->GetMethodID(env, cls,
			"logMessage", "(Ljava/lang/String;)V");
	msgErrorCallback_vars.error_mid = (*env)->GetMethodID(env, cls, "logError",
			"(Ljava/lang/String;)V");

	/* Get the String[] containing the parameters,
	 * and converts it into a char** to simulate command line arguments.
	 */
	argv[0] = "OpenJPEGJavaDecoder";/* program name: not used */

	for (i = 1; i < argc; i++) {
		object = (*env)->GetObjectArrayElement(env, javaParameters, i - 1);
		argv[i] = (char*) (*env)->GetStringUTFChars(env, object, &isCopy);
	}
#ifdef DEBUG_ARGS
	for(i=0; i<argc; i++)
	{
		printf("Arg[%d]%s\n",i,argv[i]);
	}
	printf("\n");
#endif

	opj_set_default_decoder_parameters(&parameters);

	fid = (*env)->GetFieldID(env, cls, "skippedResolutions", "I");
	parameters.cp_reduce = (short) (*env)->GetIntField(env, obj, fid);
	parameters.decod_format = J2K_CFMT;
	parameters.infile[0] = 0;
	fails = 1;
	free_infile = 0;
	image = NULL;

	j =
	/*-------------------------------------------------------------*/
	parse_cmdline_decoder(argc, (char**)argv, &parameters, &img_fol);
	/*-------------------------------------------------------------*/
	/* Release the Java arguments array*/
	for (i = 1; i < argc; i++) {
		(*env)->ReleaseStringUTFChars(env,
				(*env)->GetObjectArrayElement(env, javaParameters, i - 1),
				argv[i]);
	}
	opj_free(argv);
	argv = NULL;

	if (j == 1)
		return -1; /* Failure */

	if (parameters.decod_format == J2K_CFMT)
		codec_format = OPJ_CODEC_J2K;
	else if (parameters.decod_format == JP2_CFMT)
		codec_format = OPJ_CODEC_JP2;
	else if (parameters.decod_format == JPT_CFMT)
		codec_format = OPJ_CODEC_JPT;
	else {
		/* Already checked in parse_cmdline_decoder():
		 */
		return -1;
	}
	OPJ_BOOL usingCompressedStream = (OPJ_BOOL)OPJ_FALSE;
	/* read the input file and put it in memory into the 'src' object,
	 * if the -i option is given in JavaParameters.
	 * Implemented for debug purpose.
	 */
	if (argc == 1) /* no infile given */
	{
		/* Preparing the transfer of the codestream from Java to C*/
		usingCompressedStream = OPJ_TRUE;
		fid = (*env)->GetFieldID(env, cls, "compressedStream", "[B");
		jba = (*env)->GetObjectField(env, obj, fid);
		file_length = (*env)->GetArrayLength(env, jba);
		jbcBody = (*env)->GetByteArrayElements(env, jba, &isCopy);
		src = (unsigned char*) jbBody;
		jbaCompressed = (*env)->GetObjectField(env, obj, fid);

		if (jbaCompressed != NULL) {
			buf_info.len = (*env)->GetArrayLength(env, jbaCompressed);

			jbBodyCompressed = (*env)->GetByteArrayElements(env,
					jbaCompressed, &isCopy);

			buf_info.buf = (unsigned char*) jbBodyCompressed;
		}
		if (!buf_info.buf) {

			return -1;
		}
		buf_info.cur = buf_info.buf;
		int decod_format = buffer_format(&buf_info);

		if (decod_format == J2K_CFMT)
			codec_format = OPJ_CODEC_J2K;
		else if (decod_format == JP2_CFMT)
			codec_format = OPJ_CODEC_JP2;
		else if (decod_format == JPT_CFMT)
			codec_format = OPJ_CODEC_JPT;
		else {
			/* Already checked in parse_cmdline_decoder():
			 */
			fprintf(stderr, "%s:%d: Could not find decod_format.\n",
			__FILE__,
			__LINE__);
			return -1;
		}
		parameters.decod_format = decod_format;
	}/* if(parameters.infile[0] == 0) */

	if (usingCompressedStream == OPJ_FALSE) {
		infile = parameters.infile;

		l_stream = opj_stream_create_default_file_stream(infile, IS_READER);

		if (l_stream == NULL) {
			fprintf(stderr, "%s:%d:\n\tSTREAM failed\n", __FILE__, __LINE__);
			goto fin0;
		}

	} else {
		l_stream = opj_stream_create_buffer_stream(&buf_info, OPJ_TRUE);
		if (l_stream == NULL) {
			fprintf(stderr, "%s:%d:\n\tSTREAM failed\n", __FILE__, __LINE__);
			goto fin0;
		}
	}

	l_codec = opj_create_decompress(codec_format);

	if (l_codec == NULL) {
		fprintf(stderr, "%s:%d:\n\tCODEC failed\n", __FILE__, __LINE__);
		goto fin0;
	}

	/* catch events using our callbacks and give a local context:
	 */
	opj_set_info_handler(l_codec, info_callback, &msgErrorCallback_vars);
	opj_set_warning_handler(l_codec, warning_callback, &msgErrorCallback_vars);
	opj_set_error_handler(l_codec, error_callback, &msgErrorCallback_vars);

	if (!opj_setup_decoder(l_codec, &parameters)) {
		fprintf(stderr, "%s:%d:\n\tSETUP_DECODER failed\n", __FILE__, __LINE__);
		goto fin0;
	}
	if (!opj_read_header(l_stream, l_codec, &image)) {
		fprintf(stderr, "%s:%d:\n\tREAD_HEADER failed\n", __FILE__, __LINE__);
		goto fin0;
	}

	if (!parameters.nb_tile_to_decode) {
		/* Optional if you want decode the entire image
		 */
		if (!opj_set_decode_area(l_codec, image, parameters.DA_x0,
				parameters.DA_y0, parameters.DA_x1, parameters.DA_y1)) {
			fprintf(stderr, "%s:%d:\n\tSET_DECODE_AREA failed\n",
			__FILE__, __LINE__);
			goto fin0;
		}
		/* Get the decoded image
		 */
		if (!opj_decode(l_codec, l_stream, image)) {
			fprintf(stderr, "%s:%d:\n\tDECODE_AREA failed\n", __FILE__,
			__LINE__);
			goto fin0;
		}
	}/* if(!parameters.nb_tile_to_decode) */
	else if (!opj_get_decoded_tile(l_codec, l_stream, image,
			parameters.tile_index)) {
		fprintf(stderr, "%s:%d:\n\tDECODE_TILE failed\n", __FILE__, __LINE__);
		goto fin0;
	}

	if (!opj_end_decompress(l_codec, l_stream)) {
		fprintf(stderr, "%s:%d:\n\tEND_DECOMPRESS failed\n", __FILE__,
		__LINE__);
		goto fin0;
	}
	if (image->color_space != OPJ_CLRSPC_SYCC && image->numcomps == 3
			&& image->comps[0].dx == image->comps[0].dy
			&& image->comps[1].dx != 1)
		image->color_space = OPJ_CLRSPC_SYCC;
	else if (image->numcomps <= 2)
		image->color_space = OPJ_CLRSPC_GRAY;

	if (image->color_space == OPJ_CLRSPC_SYCC) {
		color_sycc_to_rgb(image);
	}

	if (image->icc_profile_buf) {
#if defined(OPJ_HAVE_LIBLCMS1) || defined(OPJ_HAVE_LIBLCMS2)
		color_apply_icc_profile(image);
#endif
		opj_free(image->icc_profile_buf);
		image->icc_profile_buf = NULL;
		image->icc_profile_len = 0;
	}

	fails = 0;
	fin0: if (free_infile)
		remove(parameters.infile);

	if (l_codec)
		opj_destroy_codec(l_codec);

	if (l_stream)
		opj_stream_destroy(l_stream);

	if (fails) {
		if (image)
			opj_image_destroy(image);
		if(jbaCompressed)
		{
			(*env)->ReleaseByteArrayElements(env, jbaCompressed, jbcBody, 0);
			jbaCompressed = NULL;
			jbcBody = NULL;
		}

		return -1;
	}

	/* create output image.
	 If the -o parameter is given in the JavaParameters,
	 write the decoded version into a file.
	 Implemented for debug purpose.
	 */
	switch (parameters.cod_format) {
	case PXM_DFMT: /* PNM PGM PPM */
		if (imagetopnm(image, parameters.outfile))
			fprintf(stdout, "Outfile %s not generated\n", parameters.outfile);
		else
			fprintf(stdout, "Generated Outfile %s\n", parameters.outfile);
		break;

	case PGX_DFMT: /* PGX */
		if (imagetopgx(image, parameters.outfile))
			fprintf(stdout, "Outfile %s not generated\n", parameters.outfile);
		else
			fprintf(stdout, "Generated Outfile %s\n", parameters.outfile);
		break;

	case BMP_DFMT: /* BMP */
		if (imagetobmp(image, parameters.outfile))
			fprintf(stdout, "Outfile %s not generated\n", parameters.outfile);
		else
			fprintf(stdout, "Generated Outfile %s\n", parameters.outfile);
		break;

	case TGA_DFMT:
		if (imagetotga(image, parameters.outfile))
			fprintf(stderr, "Outfile %s not generated\n", parameters.outfile);
		else
			fprintf(stdout, "Generated Outfile %s\n", parameters.outfile);
		break;

#ifdef OPJ_HAVE_LIBTIFF
		case TIF_DFMT:
		if(imagetotif(image, parameters.outfile))
		fprintf(stderr,"Outfile %s not generated\n",parameters.outfile);
		else
		fprintf(stdout,"Generated Outfile %s\n",parameters.outfile);
		break;
#endif

#ifdef OPJ_HAVE_LIBPNG
		case PNG_DFMT:
		if(imagetopng(image, parameters.outfile))
		fprintf(stderr,"Outfile %s not generated\n",parameters.outfile);
		else
		fprintf(stdout,"Generated Outfile %s\n",parameters.outfile);
		break;
#endif

	case RAW_DFMT:
		if (imagetoraw(image, parameters.outfile))
			fprintf(stderr, "Outfile %s not generated\n", parameters.outfile);
		else
			fprintf(stdout, "Generated Outfile %s\n", parameters.outfile);
		break;

	default:
		/* Can happen if output file is TIFF or PNG
		 * and OPJ_HAVE_LIBTIF or OPJ_HAVE_LIBPNG is undefined
		 */
		if (usingCompressedStream == OPJ_FALSE) {
			fprintf(stderr, "Outfile %s not generated\n", parameters.outfile);
			goto fin1;
		}
	}/* switch() */

	/* ========= Return the image to the Java structure ===============*/
/*------------- HALLO */
	width = image->comps[0].w;
	height = image->comps[0].h;
	jmethodID mid; /* Java method call */
	/* Set JAVA width and height:
	 */
	fid = (*env)->GetFieldID(env, cls, "width", "I");
	(*env)->SetIntField(env, obj, fid, width);
	fid = (*env)->GetFieldID(env, cls, "height", "I");
	(*env)->SetIntField(env, obj, fid, height);

	if ((image->numcomps >= 3 && image->comps[0].dx == image->comps[1].dx
			&& image->comps[1].dx == image->comps[2].dx
			&& image->comps[0].dy == image->comps[1].dy
			&& image->comps[1].dy == image->comps[2].dy
			&& image->comps[0].prec == image->comps[1].prec
			&& image->comps[1].prec == image->comps[2].prec)/* RGB[A] */
			|| (image->numcomps == 2 && image->comps[0].dx == image->comps[1].dx
					&& image->comps[0].dy == image->comps[1].dy
					&& image->comps[0].prec == image->comps[1].prec)) /* GA */
			{
		int pix, has_alpha4, has_alpha2, has_rgb;

		shiftA = shiftR = shiftG = shiftB = 0;
		addA = addR = addG = addB = 0;
		alpha = NULL;

		has_rgb = (image->numcomps == 3);
		has_alpha4 = (image->numcomps == 4);
		has_alpha2 = (image->numcomps == 2);
		hasAlpha = (has_alpha4 || has_alpha2);

		if (has_rgb) {
			if (image->comps[0].prec > 8)
				shiftR = image->comps[0].prec - 8;

			if (image->comps[1].prec > 8)
				shiftG = image->comps[1].prec - 8;

			if (image->comps[2].prec > 8)
				shiftB = image->comps[2].prec - 8;

			if (image->comps[0].sgnd)
				addR = (1 << (image->comps[0].prec - 1));

			if (image->comps[1].sgnd)
				addG = (1 << (image->comps[1].prec - 1));

			if (image->comps[2].sgnd)
				addB = (1 << (image->comps[2].prec - 1));

			red = image->comps[0].data;
			green = image->comps[1].data;
			blue = image->comps[2].data;

			if (has_alpha4) {
				alpha = image->comps[3].data;

				if (image->comps[3].prec > 8)
					shiftA = image->comps[3].prec - 8;

				if (image->comps[3].sgnd)
					addA = (1 << (image->comps[3].prec - 1));
			}

		} /* if(has_rgb) */
		else {
			if (image->comps[0].prec > 8)
				shiftR = image->comps[0].prec - 8;

			if (image->comps[0].sgnd)
				addR = (1 << (image->comps[0].prec - 1));

			red = green = blue = image->comps[0].data;

			if (has_alpha2) {
				alpha = image->comps[1].data;

				if (image->comps[1].prec > 8)
					shiftA = image->comps[1].prec - 8;

				if (image->comps[1].sgnd)
					addA = (1 << (image->comps[1].prec - 1));
			}
		} /* if(has_rgb) */

		ac = 255;/* 255: FULLY_OPAQUE; 0: FULLY_TRANSPARENT */

		/*
		 Allocate JAVA memory:
		 */

		mid = (*env)->GetMethodID(env, cls, "alloc24", "()V");

		(*env)->CallVoidMethod(env, obj, mid);

		/* Get the pointer to the Java structure where the data must be copied
		 */

		fid = (*env)->GetFieldID(env, cls, "image24", "[I");

		jia = (*env)->GetObjectField(env, obj, fid);

		jiBody = (*env)->GetIntArrayElements(env, jia, 0);

		ptrIBody = jiBody;

		for (i = 0; i < width * height; i++) {
			pix = addR + *red++;

			if (shiftR) {
				pix = ((pix >> shiftR) + ((pix >> (shiftR - 1)) % 2));
				if (pix > 255)
					pix = 255;
				else if (pix < 0)
					pix = 0;
			}
			rc = (unsigned char) pix;

			pix = addG + *green++;

			if (shiftG) {
				pix = ((pix >> shiftG) + ((pix >> (shiftG - 1)) % 2));
				if (pix > 255)
					pix = 255;
				else if (pix < 0)
					pix = 0;
			}
			gc = (unsigned char) pix;

			pix = addB + *blue++;

			if (shiftB) {
				pix = ((pix >> shiftB) + ((pix >> (shiftB - 1)) % 2));
				if (pix > 255)
					pix = 255;
				else if (pix < 0)
					pix = 0;
			}
			bc = (unsigned char) pix;

			if (hasAlpha) {
				pix = addA + *alpha++;

				if (shiftA) {
					pix = ((pix >> shiftA) + ((pix >> (shiftA - 1)) % 2));
					if (pix > 255)
						pix = 255;
					else if (pix < 0)
						pix = 0;
				}
				ac = (unsigned char) pix;
			}

			/*----------------------- A         R          G        B */
			*ptrIBody++ = (int) ((ac << 24) | (rc << 16) | (gc << 8) | bc);

		} /* for(i) */

		(*env)->ReleaseIntArrayElements(env, jia, jiBody, 0);

	}/* if(image->numcomps >= 3  */
	else if (image->numcomps == 1) /* Grey */
	{
		int *grey = image->comps[0].data;

		addG = (image->comps[0].sgnd ? 1 << (image->comps[0].prec - 1) : 0);

		if (image->comps[0].prec <= 8) /* FIXME */
				{
			/*
			 Allocate JAVA memory:
			 */
			mid = (*env)->GetMethodID(env, cls, "alloc8", "()V");

			(*env)->CallVoidMethod(env, obj, mid);

			fid = (*env)->GetFieldID(env, cls, "image8", "[B");
			jba = (*env)->GetObjectField(env, obj, fid);
			jbBody = (*env)->GetByteArrayElements(env, jba, 0);
			ptrBBody = jbBody;

			for (i = 0; i < width * height; i++) {
				*ptrBBody++ = (unsigned char) (addG + *grey++);
			}
			(*env)->ReleaseByteArrayElements(env, jba, jbBody, 0);

		} else /* prec[9:16] */ /*FIXME */
		{
			int *grey;
			jshortArray jsa;
			jshort *jsBody, *ptrSBody;
			int v, ushift = 0, dshift = 0, force16 = 0;

			grey = image->comps[0].data;

			if (image->comps[0].prec < 16) {
				force16 = 1;
				ushift = 16 - image->comps[0].prec;
				dshift = image->comps[0].prec - ushift;
			}

			/*
			 Allocate JAVA memory:
			 */
			mid = (*env)->GetMethodID(env, cls, "alloc16", "()V");

			(*env)->CallVoidMethod(env, obj, mid);

			fid = (*env)->GetFieldID(env, cls, "image16", "[S");
			jsa = (*env)->GetObjectField(env, obj, fid);
			jsBody = (*env)->GetShortArrayElements(env, jsa, 0);
			ptrSBody = jsBody;

			for (i = 0; i < width * height; i++) {
				v = (addG + *grey++);

				if (force16) {
					v = (v << ushift) + (v >> dshift);
				}

				*ptrSBody++ = (v & 0xffff);
			}
			(*env)->ReleaseShortArrayElements(env, jsa, jsBody, 0);
		}
	} else {
		int *grey;

		fputs(bar, stderr);
		fprintf(stderr, "%s:%d:Can show only first component of image\n"
				"  components(%d) prec(%d) color_space[%d](%s)\n"
				"  RECT(%d,%d,%d,%d)\n", __FILE__, __LINE__, image->numcomps,
				image->comps[0].prec, image->color_space,
				clr_space(image->color_space), image->x0, image->y0, image->x1,
				image->y1);

		for (i = 0; i < image->numcomps; ++i) {
			fprintf(stderr, "[%d]dx(%d) dy(%d) w(%d) h(%d) signed(%u)\n", i,
					image->comps[i].dx, image->comps[i].dy, image->comps[i].w,
					image->comps[i].h, image->comps[i].sgnd);
		}
		fputs(bar, stderr);
		/* 1 component 8 or 16 bpp image
		 */
		grey = image->comps[0].data;
		addG = (image->comps[0].sgnd ? 1 << (image->comps[0].prec - 1) : 0);

		if (image->comps[0].prec <= 8) /*FIXME */
				{
			fid = (*env)->GetFieldID(env, cls, "image8", "[B");
			jba = (*env)->GetObjectField(env, obj, fid);
			jbBody = (*env)->GetByteArrayElements(env, jba, 0);
			ptrBBody = jbBody;

			for (i = 0; i < width * height; i++) {
				*ptrBBody++ = (unsigned char) (addG + *grey++);
			}
			(*env)->ReleaseByteArrayElements(env, jba, jbBody, 0);

		} else /* prec[9:16] */ /*FIXME */
		{
			int *grey;
			jshortArray jsa;
			jshort *jsBody, *ptrSBody;
			int v, ushift = 0, dshift = 0, force16 = 0;

			grey = image->comps[0].data;
			addG = (image->comps[0].sgnd ? 1 << (image->comps[0].prec - 1) : 0);

			if (image->comps[0].prec < 16) {
				force16 = 1;
				ushift = 16 - image->comps[0].prec;
				dshift = image->comps[0].prec - ushift;
			}
			fid = (*env)->GetFieldID(env, cls, "image16", "[S");
			jsa = (*env)->GetObjectField(env, obj, fid);
			jsBody = (*env)->GetShortArrayElements(env, jsa, 0);
			ptrSBody = jsBody;

			for (i = 0; i < width * height; i++) {
				v = (addG + *grey++);

				if (force16) {
					v = (v << ushift) + (v >> dshift);
				}

				*ptrSBody++ = (v & 0xffff);
			}
			(*env)->ReleaseShortArrayElements(env, jsa, jsBody, 0);

		}
	}

	fin1: if (image)
		opj_image_destroy(image);

	if (jbaCompressed) {
		(*env)->ReleaseByteArrayElements(env, jbaCompressed, jbcBody, 0);
		jbaCompressed = NULL;
		jbcBody = NULL;
	}

	if (fails)
		return -1;
	return 0; /* OK */
} /* MAIN Java_OpenJPEGJavaDecoder_internalDecodeJ2KtoImage() */

opj_stream_t* OPJ_CALLCONV opj_stream_create_buffer_stream(
		opj_buffer_info_t* p_source_buffer, OPJ_BOOL p_is_read_stream) {
	opj_stream_t* l_stream;

	if (!p_source_buffer)
		return NULL;

	l_stream = opj_stream_default_create(p_is_read_stream);

	if (!l_stream)
		return NULL;

	opj_stream_set_user_data(l_stream, p_source_buffer, NULL);

	opj_stream_set_user_data_length(l_stream, p_source_buffer->len);

	if (p_is_read_stream)
		opj_stream_set_read_function(l_stream,
				(opj_stream_read_fn) opj_read_from_buffer);
	else
		opj_stream_set_write_function(l_stream,
				(opj_stream_write_fn) opj_write_from_buffer);
	opj_stream_set_skip_function(l_stream,
			(opj_stream_skip_fn) opj_skip_from_buffer);
	opj_stream_set_seek_function(l_stream,
			(opj_stream_seek_fn) opj_seek_from_buffer);

	return l_stream;
}

static OPJ_SIZE_T opj_read_from_buffer(void * p_buffer, OPJ_SIZE_T p_nb_bytes,
		opj_buffer_info_t* p_source_buffer) {
	OPJ_UINT32 l_nb_read;

	if (p_source_buffer->cur + p_nb_bytes
			< p_source_buffer->buf + p_source_buffer->len) {
		l_nb_read = p_nb_bytes;
	} else {
		l_nb_read = (OPJ_UINT32) (p_source_buffer->buf + p_source_buffer->len
				- p_source_buffer->cur);
	}
	memcpy(p_buffer, p_source_buffer->cur, l_nb_read);
	p_source_buffer->cur += l_nb_read;

	return l_nb_read ? l_nb_read : ((OPJ_UINT32) -1);
}

static OPJ_SIZE_T opj_write_from_buffer(void * p_buffer, OPJ_SIZE_T p_nb_bytes,
		opj_buffer_info_t* p_source_buffer) {
	memcpy(p_source_buffer->cur, p_buffer, p_nb_bytes);
	p_source_buffer->cur += p_nb_bytes;
	p_source_buffer->len += p_nb_bytes;

	return p_nb_bytes;
}

static OPJ_SIZE_T opj_skip_from_buffer(OPJ_SIZE_T p_nb_bytes,
		opj_buffer_info_t * p_source_buffer) {
	if (p_source_buffer->cur + p_nb_bytes
			< p_source_buffer->buf + p_source_buffer->len) {
		p_source_buffer->cur += p_nb_bytes;
		return p_nb_bytes;
	}
	p_source_buffer->cur = p_source_buffer->buf + p_source_buffer->len;

	return (OPJ_SIZE_T) -1;
}

static OPJ_BOOL opj_seek_from_buffer(OPJ_SIZE_T p_nb_bytes,
		opj_buffer_info_t * p_source_buffer) {
	if (p_source_buffer->cur + p_nb_bytes
			< p_source_buffer->buf + p_source_buffer->len) {
		p_source_buffer->cur += p_nb_bytes;
		return OPJ_TRUE;
	}
	p_source_buffer->cur = p_source_buffer->buf + p_source_buffer->len;

	return OPJ_FALSE;
}


