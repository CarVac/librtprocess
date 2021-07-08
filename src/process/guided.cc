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
#include <math.h>
#include <assert.h>
#include <algorithm>
#include "librtprocess.h"
#include "boxblur.h"




template<typename _Tp>
inline static const _Tp& LIM(const _Tp& a, const _Tp& b, const _Tp& c)
{
    return std::max(b, std::min(a, c));
}



static int computePadding(rp_guided_filter_params_t* par)
{
  double radius = par->radius;
  int iradius;
  double subsampling = par->subsampling;
  int isubsampling;
  int scale = par->common.scale;
  int padding;
  int s;

  for( s = 1; s <= scale; s++ ) {
    radius /= 2;
    subsampling /= 2;
  }
  iradius = (radius < 1) ? 1 : (int)(floor(radius));
  isubsampling = (subsampling < 1) ? 1 : (int)(floor(subsampling));

  padding = iradius * 2 + isubsampling;

  return padding;
}


void guidedFilterComputeRoIout(rp_roi_rect_t* rin, rp_roi_rect_t* rout, rp_guided_filter_params_t* par)
{
  int padding = computePadding(par);

  rout->left = rin->left - padding;
  rout->top = rin->top - padding;
  rout->width = rin->width + padding*2;
  rout->height = rin->height + padding*2;
}


void guidedFilterComputeRoIin(rp_roi_rect_t* rin, rp_roi_rect_t* rout, rp_guided_filter_params_t* par)
{
  int padding = computePadding(par);

  rin->left = rout->left + padding;
  rin->top = rout->top + padding;
  rin->width = rout->width - padding*2;
  rin->height = rout->height - padding*2;
}


void guidedFilterProcess(rp_roi_t* guide, rp_roi_t* src, rp_roi_t* dest, rp_guided_filter_params_t* par)
{
  bool multithread = false;
  int subsampling = par->subsampling;
  int r = par->radius;
  float epsilon = par->threshold;
  int W = src->rect.width;
  int H = src->rect.height;

  enum Op { MUL, DIVEPSILON, ADD, SUB, ADDMUL, SUBMUL };

  const auto apply =
      [=](Op op, int border, rp_roi_t* res, rp_roi_t* ra, rp_roi_t* rb, rp_roi_t* rc=NULL) -> void
      {
        for(int ch = 0; ch < ra->nchannels; ch++) {
          const int w = res->rect.width-border;
          const int h = res->rect.height-border;
          float** a = ra->data[ch];
          float** b = rb->data[ch];
          float** c = rc->data[ch];

#ifdef _OPENMP
          #pragma omp parallel for if (multithread)
#endif
          for (int y = border; y < h; ++y) {
              for (int x = border; x < w; ++x) {
                  float r;
                  float aa = a[y][x];
                  float bb = b[y][x];
                  switch (op) {
                  case MUL:
                      r = aa * bb;
                      //std::cout<<"r = aa * bb: "<<r<<" = "<<aa<<" * "<<bb<<std::endl;
                      break;
                  case DIVEPSILON:
                      r = aa / (bb + epsilon);
                      //std::cout<<"r = aa / (bb + epsilon): "<<r<<" = "<<aa<<" / "<<bb+epsilon<<std::endl;
                      break;
                  case ADD:
                      r = aa + bb;
                      //std::cout<<"r = aa + bb: "<<r<<" = "<<aa<<" + "<<bb<<std::endl;
                      break;
                  case SUB:
                      r = aa - bb;
                      //std::cout<<"r = aa - bb: "<<r<<" = "<<aa<<" - "<<bb<<std::endl;
                      break;
                  case ADDMUL:
                      r = aa * bb + c[y][x];
                      //std::cout<<"r = aa * bb + c[y][x]: "<<r<<" = "<<aa<<" * "<<bb<<" + "<<c[y][x]<<std::endl;
                      break;
                  case SUBMUL:
                      r = c[y][x] - (aa * bb);
                      //std::cout<<"r = c[y][x] - (aa * bb): "<<r<<" = "<<c[y][x]<<" - "<<aa<<" * "<<bb<<std::endl;
                      break;
                  default:
                      assert(false);
                      r = 0;
                      break;
                  }
                  res->data[ch][y][x] = r;
                  //if( op==ADDMUL && r<-100 ) getchar();
              }
          }
        }
      };

  // use the terminology of the paper (Algorithm 2)
  rp_roi_t* I = guide;
  rp_roi_t* p = src;
  rp_roi_t* q = dest;

  //AlignedBuffer<float> blur_buf(I->rect.width * I->rect.height);
  float * blur_buf;
  if( posix_memalign( (void**)(&blur_buf), 16, sizeof(float) * I->rect.width * I->rect.height ) != 0 ) {
    return;
  }

  const auto f_mean =
      [&](rp_roi_t* d, rp_roi_t* s, int rad, int border=0) -> void
      {
        for(int ch = 0; ch < s->nchannels; ch++) {
          rad = LIM(rad, 0, (std::min(s->rect.width, s->rect.height) - 1) / 2 - 1);
          float **src = s->data[ch];
          float **dst = d->data[ch];
          librtprocess::boxblur<float, float>(src, dst, blur_buf, rad, rad, s->rect.width, s->rect.height);
        }
      };


  const auto f_subsample =
      [=](rp_roi_t* d, rp_roi_t* s) -> void
      {
          //rescaleBilinear(s, d, multithread);
      };

  const auto f_upsample = f_subsample;

  //return;

  const int w = W / subsampling;
  const int h = H / subsampling;
  rp_roi_rect_t subsampled_rect = {
      I->rect.left / subsampling,
      I->rect.top / subsampling,
      w, h
  };

  rp_roi_t* I1 = NULL; //(w, h);
  rp_roi_t* p1 = NULL; //(w, h);

  if(subsampling > 1) {
    I1 = rp_roi_new(&subsampled_rect, I->nchannels);
    p1 = rp_roi_new(&subsampled_rect, I->nchannels);

    f_subsample(I1, I);
    f_subsample(p1, p);
  } else {
    I1 = I;
    p1 = p;
  }


  float r1 = float(r) / subsampling;
  //int border = 0;

  rp_roi_t* meanI = NULL;
  meanI = rp_roi_new(&subsampled_rect, I->nchannels);
  f_mean(meanI, I1, r1, 0);

  rp_roi_t* meanp = NULL;
  meanp = rp_roi_new(&subsampled_rect, I->nchannels);
  f_mean(meanp, p1, r1, 0);

  rp_roi_t* corrIp = p1;
  apply(MUL, 0, corrIp, I1, p1);
  f_mean(corrIp, corrIp, r1, 0);

  rp_roi_t* corrI = I1;
  apply(MUL, 0, corrI, I1, I1);
  f_mean(corrI, corrI, r1, 0);

  rp_roi_t* varI = corrI;
  apply(SUBMUL, r1, varI, meanI, meanI, corrI);

  rp_roi_t* covIp = corrIp;
  apply(SUBMUL, r1, covIp, meanI, meanp, corrIp);

  rp_roi_t* a = varI;
  apply(DIVEPSILON, r1, a, covIp, varI);

  rp_roi_t* b = covIp;
  apply(SUBMUL, r1, b, a, meanI, meanp);

  rp_roi_t* meana = a;
  f_mean(meana, a, r1, r1);

  rp_roi_t* meanb = b;
  f_mean(meanb, b, r1, r1);

  if( subsampling > 1 ) {
    rp_roi_t* meanA;
    meanA = rp_roi_new(&(src->rect), I->nchannels);
    f_upsample(meanA, meana);

    rp_roi_t* meanB;
    meanB = rp_roi_new(&(src->rect), I->nchannels);
    f_upsample(meanB, meanb);

    apply(ADDMUL, r*2, q, meanA, I, meanB);
  } else {
    rp_roi_t* meanA = meana;
    rp_roi_t* meanB = meanb;

    apply(ADDMUL, r*2, q, meanA, I, meanB);
  }
}
