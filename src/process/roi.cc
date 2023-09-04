/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include "librtprocess.h"



rp_roi_t* rp_roi_new(rp_roi_rect_t* rect, int nchannels)
{
  int left = rect->left, top = rect->top, width = rect->width, height = rect->height;
  int ci, ri, ch;
  int alignment = 16;
  int aligned_width = (width/4) * 4 + 4;
  rp_roi_t* roi = (rp_roi_t*)malloc(sizeof(rp_roi_t));
  if(roi == NULL) return roi;

  roi->rect.left = left;
  roi->rect.top = top;
  roi->rect.width = width;
  roi->rect.height = height;
  roi->nchannels = nchannels;
  roi->buf = NULL;
  roi->data = NULL;

  /* Sanity checks */
  if( width <= 0 || height <=0 || nchannels <= 0) {
    rp_roi_free(roi);
    return NULL;
  }

  /* roi->data = (float***)malloc( sizeof(float**) * nchannels ); */
  if( roi->data == NULL ) {
    rp_roi_free(roi);
    return NULL;
  }
  memset(roi->data, 0, sizeof(float**) * nchannels);

  for( ch = 0; ch < nchannels; ch+=1 ) {
    /* roi->data[ch] = (float**)malloc( sizeof(float*) * height ); */
    if( posix_memalign( (void**)(&(roi->data[ch])), alignment, sizeof(float*) * height ) != 0 ) {
      rp_roi_free(roi);
      return NULL;
    }
    memset(roi->data[ch], 0, sizeof(float*) * height);
    roi->data[ch] = roi->data[ch] - top;
  }

  /* We need a buffer to store the pixel values */
  /* roi->buf = (float*)malloc( sizeof(float) * aligned_width * height * nchannels ); */
  if( posix_memalign( (void**)(&(roi->buf)), alignment, sizeof(float*) * aligned_width * height * nchannels ) != 0 ) {
    rp_roi_free(roi);
    return NULL;
  }

  /* init row pointers */
  for( ch = 0; ch < nchannels; ch+=1 ) {
    for( ri = 0; ri < height; ri+=1 ) {
      roi->data[ch][ri+top] = roi->buf + (aligned_width*height*ch - left);
    }
  }

  return roi;
}


rp_roi_t* rp_roi_new_from_data(rp_roi_rect_t* rect, rp_roi_rect_t* rect_in, int nchannels, int rowstride, int interleaved, float* input, ...)
{
  int left = rect->left, top = rect->top, width = rect->width, height = rect->height;
  int left_in = rect_in->left, top_in = rect_in->top;
  int ci, ri, ch;
  int alignment = 16;
  int aligned_width = (width/4) * 4 + 4;
  rp_roi_t* roi = (rp_roi_t*)malloc(sizeof(rp_roi_t));
  if(roi == NULL) return roi;

  roi->rect.left = left;
  roi->rect.top = top;
  roi->rect.width = width;
  roi->rect.height = height;
  roi->nchannels = nchannels;
  roi->buf = NULL;
  roi->data = NULL;

  /* Sanity checks */
  if( width <= 0 || height <=0 || nchannels <= 0 ) {
    rp_roi_free(roi);
    return NULL;
  }
  if( rect->width != rect_in->width || rect->height != rect_in->height ) {
    rp_roi_free(roi);
    return NULL;
  }

  /* roi->data = (float***)malloc( sizeof(float**) * nchannels ); */
  if( roi->data == NULL ) {
    rp_roi_free(roi);
    return NULL;
  }
  memset(roi->data, 0, sizeof(float**) * nchannels);

  for( ch = 0; ch < nchannels; ch+=1 ) {
    /* roi->data[ch] = (float**)malloc( sizeof(float*) * height ); */
    if( posix_memalign( (void**)(&(roi->data[ch])), alignment, sizeof(float*) * height ) != 0 ) {
      rp_roi_free(roi);
      return NULL;
    }
    memset(roi->data[ch], 0, sizeof(float*) * height);
    roi->data[ch] = roi->data[ch] - top;
  }

  if(interleaved > 0) {
    /* If the input data is interleaved, we need a buffer to store the non-interleaved pixel values */
    /* roi->buf = (float*)malloc( sizeof(float) * aligned_width * height * nchannels ); */
    if( posix_memalign( (void**)(&(roi->buf)), alignment, sizeof(float*) * aligned_width * height * nchannels ) != 0 ) {
      rp_roi_free(roi);
      return NULL;
    }

    /* init row pointers */
    for( ch = 0; ch < nchannels; ch+=1 ) {
      for( ri = 0; ri < height; ri+=1 ) {
        roi->data[ch][ri+top] = roi->buf + (aligned_width*height*ch - left);
      }
    }

    /* copy the interleaved pixels into the non-interleaved RoI buffer */
    for( ri = 0; ri < height; ri+=1 ) {
      for( ci = 0; ci < width; ci+=1 ) {
        for( ch = 0; ch < nchannels; ch+=1 ) {
          roi->data[ch][ri+top][ci+left] = input[(ri+top_in)*rowstride + (ci+left_in)*nchannels + ch];
        }
      }
    }
  } else {
    /* If the input data is non-interleaved, we can directly copy pointers.
       In this case we expect one input buffer for each image channel. */
  }

  return roi;
}


void rp_roi_free(rp_roi_t* roi)
{

}
