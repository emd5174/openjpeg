/*
 * The copyright in this software is being made available under the 2-clauses 
 * BSD License, included below. This software may be subject to other third 
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
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

#include "opj_includes.h"

opj_image_t* opj_image_create0(void) {
	opj_image_t *image = (opj_image_t*)opj_calloc(1, sizeof(opj_image_t));
	return image;
}

OPJ_BOOL opj_image_equal(opj_image_t* lhs, opj_image_t* rhs) {
	OPJ_UINT32 i;
	if (lhs == NULL || rhs == NULL)
		return OPJ_FALSE;
	if (lhs->comps == NULL || rhs->comps == NULL)
		return OPJ_FALSE;
	if (lhs->numcomps != rhs->numcomps)
		return OPJ_FALSE;
	if (lhs->color_space != rhs->color_space)
		return OPJ_FALSE;
	if (lhs->x0 != rhs->x0 || lhs->x1 != rhs->x1 || lhs->y0 != rhs->y0 || lhs->y1 != rhs->y1)
		return OPJ_FALSE;
	for (i = 0; i < lhs->numcomps; ++i) {
		if (lhs->comps[i].bpp != rhs->comps[i].bpp)
			return OPJ_FALSE;
		if (lhs->comps[i].prec != rhs->comps[i].prec)
			return OPJ_FALSE;
		if (lhs->comps[i].sgnd != rhs->comps[i].sgnd)
			return OPJ_FALSE;
		if (lhs->comps[i].w != rhs->comps[i].w)
			return OPJ_FALSE;
		if (lhs->comps[i].h != rhs->comps[i].h)
			return OPJ_FALSE;
		if (lhs->comps[i].dx != rhs->comps[i].dx)
			return OPJ_FALSE;
		if (lhs->comps[i].dy != rhs->comps[i].dy)
			return OPJ_FALSE;
	}
	return OPJ_TRUE;
}

OPJ_BOOL OPJ_CALLCONV opj_image_header_equals(opj_image_t* img,  OPJ_SIZE_T num_components,
											opj_image_cmptparm_t* component_parameters, OPJ_COLOR_SPACE color_space){

	OPJ_UINT32 i;
   if (!img || (img->numcomps !=num_components) || img->color_space != color_space )
	   return OPJ_FALSE;
	for (i = 0; i < img->numcomps; ++i) {
		if (img->comps[i].bpp != component_parameters->bpp)
			return OPJ_FALSE;
		if (img->comps[i].prec != component_parameters->prec)
			return OPJ_FALSE;
		if (img->comps[i].sgnd != component_parameters->sgnd)
			return OPJ_FALSE;
		if (img->comps[i].w != component_parameters->w)
			return OPJ_FALSE;
		if (img->comps[i].h != component_parameters->h)
			return OPJ_FALSE;
		if (img->comps[i].dx != component_parameters->dx)
			return OPJ_FALSE;
		if (img->comps[i].dy != component_parameters->dy)
			return OPJ_FALSE;
	}
	return OPJ_TRUE;
}


opj_image_t* OPJ_CALLCONV opj_image_create(OPJ_UINT32 numcmpts, opj_image_cmptparm_t *cmptparms, OPJ_COLOR_SPACE clrspc) {
	OPJ_UINT32 compno;
	opj_image_t *image = NULL;

	image = (opj_image_t*) opj_calloc(1, sizeof(opj_image_t));
	if(image) {
		image->color_space = clrspc;
		image->numcomps = numcmpts;
		/* allocate memory for the per-component information */
		image->comps = (opj_image_comp_t*)opj_calloc(1,image->numcomps * sizeof(opj_image_comp_t));
		if(!image->comps) {
			fprintf(stderr,"Unable to allocate memory for image.\n");
			opj_image_destroy(image);
			return NULL;
		}
		/* create the individual image components */
		for(compno = 0; compno < numcmpts; compno++) {
			opj_image_comp_t *comp = &image->comps[compno];
			comp->dx = cmptparms[compno].dx;
			comp->dy = cmptparms[compno].dy;
			comp->w = cmptparms[compno].w;
			comp->h = cmptparms[compno].h;
			comp->x0 = cmptparms[compno].x0;
			comp->y0 = cmptparms[compno].y0;
			comp->prec = cmptparms[compno].prec;
			comp->bpp = cmptparms[compno].bpp;
			comp->sgnd = cmptparms[compno].sgnd;
			if (cmptparms->allocate_memory) {
				comp->data = (OPJ_INT32*) opj_calloc(comp->w * comp->h, sizeof(OPJ_INT32));
				if(!comp->data) {
					fprintf(stderr,"Unable to allocate memory for image.\n");
					opj_image_destroy(image);
					return NULL;
				}
			}
			comp->owns_data = OPJ_TRUE;
		}
	}

	return image;
}

void OPJ_CALLCONV opj_image_destroy(opj_image_t *image) {
	if(image) {
		if(image->comps) {
			OPJ_UINT32 compno;

			/* image components */
			for(compno = 0; compno < image->numcomps; compno++) {
				opj_image_comp_t *image_comp = &(image->comps[compno]);
				if(image_comp->data && image_comp->owns_data) {
					opj_free(image_comp->data);
				}
			}
			opj_free(image->comps);
		}

		if(image->icc_profile_buf) {
			opj_free(image->icc_profile_buf);
		}

		opj_free(image);
	}
}

/**
 * Updates the components characteristics of the image from the coding parameters.
 *
 * @param p_image_header	the image header to update.
 * @param p_cp				the coding parameters from which to update the image.
 */
void opj_image_comp_header_update(opj_image_t * p_image_header, const struct opj_cp * p_cp)
{
	OPJ_UINT32 i, l_width, l_height;
	OPJ_INT32 l_x0, l_y0, l_x1, l_y1;
	OPJ_INT32 l_comp_x0, l_comp_y0, l_comp_x1, l_comp_y1;
	opj_image_comp_t* l_img_comp = NULL;

	l_x0 = opj_int_max((OPJ_INT32)p_cp->tx0 , (OPJ_INT32)p_image_header->x0);
	l_y0 = opj_int_max((OPJ_INT32)p_cp->ty0 , (OPJ_INT32)p_image_header->y0);
	l_x1 = opj_int_min((OPJ_INT32)(p_cp->tx0 + p_cp->tw * p_cp->tdx), (OPJ_INT32)p_image_header->x1);
	l_y1 = opj_int_min((OPJ_INT32)(p_cp->ty0 + p_cp->th * p_cp->tdy), (OPJ_INT32)p_image_header->y1);

	l_img_comp = p_image_header->comps;
	for	(i = 0; i < p_image_header->numcomps; ++i) {
		l_comp_x0 = opj_int_ceildiv(l_x0, (OPJ_INT32)l_img_comp->dx);
		l_comp_y0 = opj_int_ceildiv(l_y0, (OPJ_INT32)l_img_comp->dy);
		l_comp_x1 = opj_int_ceildiv(l_x1, (OPJ_INT32)l_img_comp->dx);
		l_comp_y1 = opj_int_ceildiv(l_y1, (OPJ_INT32)l_img_comp->dy);
		l_width = (OPJ_UINT32)opj_int_ceildivpow2(l_comp_x1 - l_comp_x0, (OPJ_INT32)l_img_comp->factor);
		l_height = (OPJ_UINT32)opj_int_ceildivpow2(l_comp_y1 - l_comp_y0, (OPJ_INT32)l_img_comp->factor);
		l_img_comp->w = l_width;
		l_img_comp->h = l_height;
		l_img_comp->x0 = (OPJ_UINT32)l_comp_x0/*l_x0*/;
		l_img_comp->y0 = (OPJ_UINT32)l_comp_y0/*l_y0*/;
		++l_img_comp;
	}
}


/**
 * Copy only header of image and its component header (no data are copied)
 * if dest image have data, they will be freed
 *
 * @param	p_image_src		the src image
 * @param	p_image_dest	the dest image
 *
 */
void opj_copy_image_header(const opj_image_t* p_image_src, opj_image_t* p_image_dest)
{
	OPJ_UINT32 compno;

	/* preconditions */
	assert(p_image_src != 00);
	assert(p_image_dest != 00);

	p_image_dest->x0 = p_image_src->x0;
	p_image_dest->y0 = p_image_src->y0;
	p_image_dest->x1 = p_image_src->x1;
	p_image_dest->y1 = p_image_src->y1;

	if (p_image_dest->comps){
		for(compno = 0; compno < p_image_dest->numcomps; compno++) {
			opj_image_comp_t *image_comp = &(p_image_dest->comps[compno]);
			if(image_comp->data && image_comp->owns_data) {
				opj_free(image_comp->data);
			}
		}
		opj_free(p_image_dest->comps);
		p_image_dest->comps = NULL;
	}

	p_image_dest->numcomps = p_image_src->numcomps;

	p_image_dest->comps = (opj_image_comp_t*) opj_malloc(p_image_dest->numcomps * sizeof(opj_image_comp_t));
	if (!p_image_dest->comps){
		p_image_dest->comps = NULL;
		p_image_dest->numcomps = 0;
		return;
	}

	for (compno=0; compno < p_image_dest->numcomps; compno++){
		memcpy( &(p_image_dest->comps[compno]),
				&(p_image_src->comps[compno]),
				sizeof(opj_image_comp_t));
		p_image_dest->comps[compno].data = NULL;
	}

	p_image_dest->color_space = p_image_src->color_space;
	p_image_dest->icc_profile_len = p_image_src->icc_profile_len;

	if (p_image_dest->icc_profile_len) {
		p_image_dest->icc_profile_buf = (OPJ_BYTE*)opj_malloc(p_image_dest->icc_profile_len);
		if (!p_image_dest->icc_profile_buf){
			p_image_dest->icc_profile_buf = NULL;
			p_image_dest->icc_profile_len = 0;
			return;
		}
		memcpy( p_image_dest->icc_profile_buf,
				p_image_src->icc_profile_buf,
				p_image_src->icc_profile_len);
		}
		else
			p_image_dest->icc_profile_buf = NULL;

	return;
}

opj_image_t* OPJ_CALLCONV opj_image_tile_create(OPJ_UINT32 numcmpts, opj_image_cmptparm_t *cmptparms, OPJ_COLOR_SPACE clrspc) {
	OPJ_UINT32 compno;
	opj_image_t *image = 00;

	image = (opj_image_t*) opj_calloc(1,sizeof(opj_image_t));
	if (image)
	{
		
		image->color_space = clrspc;
		image->numcomps = numcmpts;
		
		/* allocate memory for the per-component information */
		image->comps = (opj_image_comp_t*)opj_calloc(image->numcomps, sizeof(opj_image_comp_t));
		if (!image->comps) {
			opj_image_destroy(image);
			return 00;
		}
		
		/* create the individual image components */
		for(compno = 0; compno < numcmpts; compno++) {
			opj_image_comp_t *comp = &image->comps[compno];
			comp->dx = cmptparms[compno].dx;
			comp->dy = cmptparms[compno].dy;
			comp->w = cmptparms[compno].w;
			comp->h = cmptparms[compno].h;
			comp->x0 = cmptparms[compno].x0;
			comp->y0 = cmptparms[compno].y0;
			comp->prec = cmptparms[compno].prec;
			comp->sgnd = cmptparms[compno].sgnd;
			comp->data = 0;
		}
	}

	return image;
}
