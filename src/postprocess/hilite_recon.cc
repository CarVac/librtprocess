////////////////////////////////////////////////////////////////
//
//      Highlight reconstruction
//
//      copyright (c) 2008-2011  Emil Martinec <ejmartin@uchicago.edu>
//
//
// code dated: June 16, 2011
//
//  hilite_recon.cc is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////

#include <cstddef>
#include <cmath>
#include "array2D.h"
#include "librtprocess.h"
#include "rt_math.h"
#include "opthelper.h"

//#define VERBOSE

using librtprocess::SQR;
using librtprocess::max;
using librtprocess::min;

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void boxblur2(float** src, float** dst, float** temp, int startY, int startX, int H, int W, int box )
{
    //box blur image channel; box size = 2*box+1
    //horizontal blur
#ifdef _OPENMP
    #pragma omp parallel for
#endif

    for (int row = 0; row < H; row++) {
        int len = box + 1;
	temp[row][0] = src[row + startY][startX] / len;

        for (int j = 1; j <= box; j++) {
	    temp[row][0] += src[row + startY][j + startX] / len;
        }

        for (int col = 1; col <= box; col++) {
            temp[row][col] = (temp[row][col - 1] * len + src[row + startY][col + box + startX]) / (len + 1);
            len ++;
        }

        for (int col = box + 1; col < W - box; col++) {
            temp[row][col] = temp[row][col - 1] + (src[row + startY][col + box + startX] - src[row + startY][col - box - 1 + startX]) / len;
        }

        for (int col = W - box; col < W; col++) {
            temp[row][col] = (temp[row][col - 1] * len - src[row + startY][col - box - 1 + startX]) / (len - 1);
            len --;
        }
    }

#ifdef __SSE2__
    //vertical blur
#ifdef _OPENMP
    #pragma omp parallel
#endif
    {
        float len = box + 1;
        vfloat lenv = F2V( len );
        vfloat lenp1v = F2V( len + 1.0f );
        vfloat onev = F2V( 1.0f );
        vfloat tempv, temp2v;
#ifdef _OPENMP
        #pragma omp for nowait
#endif

        for (int col = 0; col < W - 7; col += 8) {
            tempv = LVFU(temp[0][col]) / lenv;
            temp2v = LVFU(temp[0][col + 4]) / lenv;

            for (int i = 1; i <= box; i++) {
                tempv = tempv + LVFU(temp[i][col]) / lenv;
                temp2v = temp2v + LVFU(temp[i][col + 4]) / lenv;
            }

            _mm_storeu_ps( &dst[0][col], tempv);
            _mm_storeu_ps( &dst[0][col + 4], temp2v);

            for (int row = 1; row <= box; row++) {
                tempv = (tempv * lenv + LVFU(temp[(row + box)][col])) / lenp1v;
                temp2v = (temp2v * lenv + LVFU(temp[(row + box)][col + 4])) / lenp1v;
                _mm_storeu_ps( &dst[row][col], tempv);
                _mm_storeu_ps( &dst[row][col + 4], temp2v);
                lenv = lenp1v;
                lenp1v = lenp1v + onev;
            }

            for (int row = box + 1; row < H - box; row++) {
                tempv = tempv + (LVFU(temp[(row + box)][col]) - LVFU(temp[(row - box - 1)][col])) / lenv;
                temp2v = temp2v + (LVFU(temp[(row + box)][col + 4]) - LVFU(temp[(row - box - 1)][col + 4])) / lenv;
                _mm_storeu_ps( &dst[row][col], tempv);
                _mm_storeu_ps( &dst[row][col + 4], temp2v);
            }

            for (int row = H - box; row < H; row++) {
                lenp1v = lenv;
                lenv = lenv - onev;
                tempv = (tempv * lenp1v - LVFU(temp[(row - box - 1)][col])) / lenv;
                temp2v = (temp2v * lenp1v - LVFU(temp[(row - box - 1)][col + 4])) / lenv;
                _mm_storeu_ps( &dst[row][col], tempv );
                _mm_storeu_ps( &dst[row][col + 4], temp2v );
            }
        }

#ifdef _OPENMP
        #pragma omp single
#endif
        {
            for (int col = W - (W % 8); col < W - 3; col += 4) {
                tempv = LVFU(temp[0][col]) / lenv;

                for (int i = 1; i <= box; i++) {
                    tempv = tempv + LVFU(temp[i][col]) / lenv;
                }

                _mm_storeu_ps( &dst[0][col], tempv);

                for (int row = 1; row <= box; row++) {
                    tempv = (tempv * lenv + LVFU(temp[(row + box)][col])) / lenp1v;
                    _mm_storeu_ps( &dst[row][col], tempv);
                    lenv = lenp1v;
                    lenp1v = lenp1v + onev;
                }

                for (int row = box + 1; row < H - box; row++) {
                    tempv = tempv + (LVFU(temp[(row + box)][col]) - LVFU(temp[(row - box - 1)][col])) / lenv;
                    _mm_storeu_ps( &dst[row][col], tempv);
                }

                for (int row = H - box; row < H; row++) {
                    lenp1v = lenv;
                    lenv = lenv - onev;
                    tempv = (tempv * lenp1v - LVFU(temp[(row - box - 1)][col])) / lenv;
                    _mm_storeu_ps( &dst[row][col], tempv );
                }
            }

            for (int col = W - (W % 4); col < W; col++) {
                int llen = box + 1;
                dst[0][col] = temp[0][col] / llen;

                for (int i = 1; i <= box; i++) {
                    dst[0][col] += temp[i][col] / llen;
                }

                for (int row = 1; row <= box; row++) {
                    dst[row][col] = (dst[(row - 1)][col] * llen + temp[(row + box)][col]) / (llen + 1);
                    llen ++;
                }

                for (int row = box + 1; row < H - box; row++) {
                    dst[row][col] = dst[(row - 1)][col] + (temp[(row + box)][col] - temp[(row - box - 1)][col]) / llen;
                }

                for (int row = H - box; row < H; row++) {
                    dst[row][col] = (dst[(row - 1)][col] * llen - temp[(row - box - 1)][col]) / (llen - 1);
                    llen --;
                }
            }
        }
    }

#else
    //vertical blur
#ifdef _OPENMP
    #pragma omp parallel for
#endif

    for (int col = 0; col < W; col++) {
        int len = box + 1;
        dst[0][col] = temp[0][col] / len;

        for (int i = 1; i <= box; i++) {
            dst[0][col] += temp[i][col] / len;
        }

        for (int row = 1; row <= box; row++) {
            dst[row][col] = (dst[(row - 1)][col] * len + temp[(row + box)][col]) / (len + 1);
            len ++;
        }

        for (int row = box + 1; row < H - box; row++) {
            dst[row][col] = dst[(row - 1)][col] + (temp[(row + box)][col] - temp[(row - box - 1)][col]) / len;
        }

        for (int row = H - box; row < H; row++) {
            dst[row][col] = (dst[(row - 1)][col] * len - temp[(row - box - 1)][col]) / (len - 1);
            len --;
        }
    }

#endif

}

void boxblur_resamp(float **src, float **dst, float ** temp, int H, int W, int box, int samp )
{

#ifdef _OPENMP
    #pragma omp parallel
#endif
    {
#ifdef _OPENMP
        #pragma omp for
#endif

        //box blur image channel; box size = 2*box+1
        //horizontal blur
        for (int row = 0; row < H; row++)
        {
            int len = box + 1;
            float tempval = src[row][0] / len;

            for (int j = 1; j <= box; j++) {
                tempval += src[row][j] / len;
            }

            temp[row][0] = tempval;

            for (int col = 1; col <= box; col++) {
                tempval = (tempval * len + src[row][col + box]) / (len + 1);

                if(col % samp == 0) {
                    temp[row][col / samp] = tempval;
                }

                len ++;
            }

            float oneByLen = 1.f / (float)len;

            for (int col = box + 1; col < W - box; col++) {
                tempval = tempval + (src[row][col + box] - src[row][col - box - 1]) * oneByLen;

                if(col % samp == 0) {
                    temp[row][col / samp] = tempval;
                }
            }

            for (int col = W - box; col < W; col++) {
                tempval = (tempval * len - src[row][col - box - 1]) / (len - 1);

                if(col % samp == 0) {
                    temp[row][col / samp] = tempval;
                }

                len --;
            }
        }
    }

    static const int numCols = 8;   // process numCols columns at once for better L1 CPU cache usage
#ifdef _OPENMP
    #pragma omp parallel
#endif
    {
        float tempvalN[numCols] ALIGNED16;
#ifdef _OPENMP
        #pragma omp for nowait
#endif

        //vertical blur
        for (int col = 0; col < (W / samp) - (numCols - 1); col += numCols) {
            int len = box + 1;

            for(int n = 0; n < numCols; n++) {
                tempvalN[n] = temp[0][col + n] / len;
            }

            for (int i = 1; i <= box; i++) {
                for(int n = 0; n < numCols; n++) {
                    tempvalN[n] += temp[i][col + n] / len;
                }
            }

            for(int n = 0; n < numCols; n++) {
                dst[0][col + n] = tempvalN[n];
            }

            for (int row = 1; row <= box; row++) {
                for(int n = 0; n < numCols; n++) {
                    tempvalN[n] = (tempvalN[n] * len + temp[(row + box)][col + n]) / (len + 1);
                }

                if(row % samp == 0) {
                    for(int n = 0; n < numCols; n++) {
                        dst[row / samp][col + n] = tempvalN[n];
                    }
                }

                len ++;
            }

            for (int row = box + 1; row < H - box; row++) {
                for(int n = 0; n < numCols; n++) {
                    tempvalN[n] = tempvalN[n] + (temp[(row + box)][col + n] - temp[(row - box - 1)][col + n]) / len;
                }

                if(row % samp == 0) {
                    for(int n = 0; n < numCols; n++) {
                        dst[row / samp][col + n] = tempvalN[n];
                    }
                }
            }

            for (int row = H - box; row < H; row++) {
                for(int n = 0; n < numCols; n++) {
                    tempvalN[n] = (tempvalN[n] * len - temp[(row - box - 1)][col + n]) / (len - 1);
                }

                if(row % samp == 0) {
                    for(int n = 0; n < numCols; n++) {
                        dst[row / samp][col + n] = tempvalN[n];
                    }
                }

                len --;
            }
        }

        // process remaining columns
#ifdef _OPENMP
        #pragma omp single
#endif
        {

            //vertical blur
            for (int col = (W / samp) - ((W / samp) % numCols); col < W / samp; col++) {
                int len = box + 1;
                float tempval = temp[0][col] / len;

                for (int i = 1; i <= box; i++) {
                    tempval += temp[i][col] / len;
                }

                dst[0][col] = tempval;

                for (int row = 1; row <= box; row++) {
                    tempval = (tempval * len + temp[(row + box)][col]) / (len + 1);

                    if(row % samp == 0) {
                        dst[row / samp][col] = tempval;
                    }

                    len ++;
                }

                for (int row = box + 1; row < H - box; row++) {
                    tempval = tempval + (temp[(row + box)][col] - temp[(row - box - 1)][col]) / len;

                    if(row % samp == 0) {
                        dst[row / samp][col] = tempval;
                    }
                }

                for (int row = H - box; row < H; row++) {
                    tempval = (tempval * len - temp[(row - box - 1)][col]) / (len - 1);

                    if(row % samp == 0) {
                        dst[row / samp][col] = tempval;
                    }

                    len --;
                }
            }
        }
    }


}

rpError HLRecovery_inpaint (const int width, const int height, float** red, float** green, float** blue, const float chmax[3], const float clmax[3], const std::function<bool(double)> &setProgCancel)
{
    double progress = 0.0;

    setProgCancel(progress);


    constexpr int range = 2;
    constexpr int pitch = 4;

    constexpr float threshpct = 0.25f;
    constexpr float maxpct = 0.95f;
    constexpr float epsilon = 0.00001f;
    //%%%%%%%%%%%%%%%%%%%%
    //for blend algorithm:
    constexpr float blendthresh = 1.0;
    constexpr int ColorCount = 3;
    // Transform matrixes rgb>lab and back
    constexpr float trans[ColorCount][ColorCount] =
    { { 1.f, 1.f, 1.f }, { 1.7320508f, -1.7320508f, 0.f }, { -1.f, -1.f, 2.f } };
    constexpr float itrans[ColorCount][ColorCount] =
    { { 1.f, 0.8660254f, -0.5f }, { 1.f, -0.8660254f, -0.5f }, { 1.f, 0.f, 1.f } };

#ifdef VERBOSE
        for(int c = 0; c < 3; c++) {
            printf("chmax[%d] : %f\tclmax[%d] : %f\tratio[%d] : %f\n", c, chmax[c], c, clmax[c], c, chmax[c] / clmax[c]);
        }
#endif
    /*What are chmax and clmax?
    clmax is the raw clip level.
    In RawTherapee it's calculated as
    clmax[i] * (c_white[i] - cblacksom[i]) * scale_mul[i]
    cblacksom = max(c_black[i] + black_lev[i], 0.0f)
    c_white is the white level
    scale_mul is the white balance multipliers that scale so that the image ends up 0-65535
    For monochrome it's just 65535/(c_white-c_black)
    For color, it's scale_mul[c] = (pre_mul[c]/maxpremul)*65535/(c_white-c_black)

    chmax is the actual highest value in the channel for the image after multiplying by scale_mul
    */


    float factor[3];

    for(int c = 0; c < ColorCount; c++) {
        factor[c] = chmax[c] / clmax[c];
    }

    float minFactor = min(factor[0], factor[1], factor[2]);

    if(minFactor > 1.f) { // all 3 channels clipped
        // calculate clip factor per channel
        for (int c = 0; c < ColorCount; c++) {
            factor[c] /= minFactor;
        }

        // get max clip factor
        int maxpos = 0;
        float maxValNew = 0.f;

        for (int c = 0; c < ColorCount; c++) {
            if(chmax[c] / factor[c] > maxValNew) {
                maxValNew = chmax[c] / factor[c];
                maxpos = c;
            }
        }

        float clipFactor = clmax[maxpos] / maxValNew;

        if(clipFactor < maxpct)

            // if max clipFactor < maxpct (0.95) adjust per channel factors
            for (int c = 0; c < ColorCount; c++) {
                factor[c] *= (maxpct / clipFactor);
            }
    } else {
        factor[0] = factor[1] = factor[2] = 1.f;
    }

#ifdef VERBOSE
        for (int c = 0; c < ColorCount; c++) {
            printf("correction factor[%d] : %f\n", c, factor[c]);
        }
#endif
    float max_f[3], thresh[3];

    for (int c = 0; c < ColorCount; c++) {
        thresh[c] = chmax[c] * threshpct / factor[c];
        max_f[c] = chmax[c] * maxpct / factor[c];
    }

    float whitept = max(max_f[0], max_f[1], max_f[2]);
    float clippt  = min(max_f[0], max_f[1], max_f[2]);
    float medpt   = max_f[0] + max_f[1] + max_f[2] - whitept - clippt;
    float blendpt = blendthresh * clippt;
    float medFactor[3];

    for (int c = 0; c < ColorCount; c++) {
        medFactor[c] = max(1.0f, max_f[c] / medpt) / (-blendpt);
    }

    int minx = width - 1;
    int maxx = 0;
    int miny = height - 1;
    int maxy = 0;

// Current MSVC version doesn't support these way of calling OMP
#ifndef _MSC_VER
    #pragma omp parallel for reduction(min:minx,miny) reduction(max:maxx,maxy) schedule(dynamic, 16)
#endif
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j< width; ++j) {
            if(red[i][j] >= max_f[0] || green[i][j] >= max_f[1] || blue[i][j] >= max_f[2]) {
                minx = std::min(minx, j);
                maxx = std::max(maxx, j);
                miny = std::min(miny, i);
                maxy = std::max(maxy, i);
            }
        }
    }


    constexpr int blurBorder = 256;
    minx = std::max(0, minx - blurBorder);
    miny = std::max(0, miny - blurBorder);
    maxx = std::min(width - 1, maxx + blurBorder);
    maxy = std::min(height - 1, maxy + blurBorder);
    const int blurWidth = maxx - minx + 1;
    const int blurHeight = maxy - miny + 1;

    multi_array2D<float, 3> channelblur(blurWidth, blurHeight, 0, 48);
    array2D<float> temp(blurWidth, blurHeight); // allocate temporary buffer

    // blur RGB channels

    boxblur2(red, channelblur[0], temp, miny, minx, blurHeight, blurWidth, 4);

    progress += 0.05;
    setProgCancel(progress);

    boxblur2(green, channelblur[1], temp, miny, minx, blurHeight, blurWidth, 4);

    progress += 0.05;
    setProgCancel(progress);

    boxblur2(blue, channelblur[2], temp, miny, minx, blurHeight, blurWidth, 4);

    progress += 0.05;
    setProgCancel(progress);

    // reduce channel blur to one array
#ifdef _OPENMP
    #pragma omp parallel for
#endif

    for(int i = 0; i < blurHeight; i++)
        for(int j = 0; j < blurWidth; j++) {
            channelblur[0][i][j] = fabsf(channelblur[0][i][j] - red[i + miny][j + minx]) + fabsf(channelblur[1][i][j] - green[i + miny][j + minx]) + fabsf(channelblur[2][i][j] - blue[i + miny][j + minx]);
        }

    for (int c = 1; c < 3; c++) {
        channelblur[c].free();    //free up some memory
    }

    progress += 0.05;
    setProgCancel(progress);

    multi_array2D<float, 4> hilite_full(blurWidth, blurHeight, ARRAY2D_CLEAR_DATA, 32);

    progress += 0.10;
    setProgCancel(progress);

    double hipass_sum = 0.f;
    int hipass_norm = 0;

    // set up which pixels are clipped or near clipping
#ifdef _OPENMP
    #pragma omp parallel for reduction(+:hipass_sum,hipass_norm) schedule(dynamic,16)
#endif

    for (int i = 0; i < blurHeight; i++) {
        for (int j = 0; j < blurWidth; j++) {
            //if one or more channels is highlight but none are blown, add to highlight accumulator
            if ((red[i + miny][j + minx] > thresh[0] || green[i + miny][j + minx] > thresh[1] || blue[i + miny][j + minx] > thresh[2]) &&
                    (red[i + miny][j + minx] < max_f[0] && green[i + miny][j + minx] < max_f[1] && blue[i + miny][j + minx] < max_f[2])) {

                hipass_sum += static_cast<double>(channelblur[0][i][j]);
                hipass_norm ++;

                hilite_full[0][i][j] = red[i + miny][j + minx];
                hilite_full[1][i][j] = green[i + miny][j + minx];
                hilite_full[2][i][j] = blue[i + miny][j + minx];
                hilite_full[3][i][j] = 1.f;

            }
        }
    }//end of filling highlight array

    float hipass_ave = 2.0 * hipass_sum / (hipass_norm + static_cast<double>(epsilon));

    progress += 0.05;
    setProgCancel(progress);

    array2D<float> hilite_full4(blurWidth, blurHeight);
    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    //blur highlight data
    boxblur2(hilite_full[3], hilite_full4, temp, 0, 0, blurHeight, blurWidth, 1);

    temp.free(); // free temporary buffer

    progress += 0.05;
    setProgCancel(progress);

#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic,16)
#endif

    for (int i = 0; i < blurHeight; i++) {
        for (int j = 0; j < blurWidth; j++) {
            if (channelblur[0][i][j] > hipass_ave) {
                //too much variation
                hilite_full[0][i][j] = hilite_full[1][i][j] = hilite_full[2][i][j] = hilite_full[3][i][j] = 0.f;
                continue;
            }

            if (hilite_full4[i][j] > epsilon && hilite_full4[i][j] < 0.95f) {
                //too near an edge, could risk using CA affected pixels, therefore omit
                hilite_full[0][i][j] = hilite_full[1][i][j] = hilite_full[2][i][j] = hilite_full[3][i][j] = 0.f;
            }
        }
    }

    channelblur[0].free();    //free up some memory
    hilite_full4.free();    //free up some memory

    int hfh = (blurHeight - (blurHeight % pitch)) / pitch;
    int hfw = (blurWidth - (blurWidth % pitch)) / pitch;

    multi_array2D<float, 4> hilite(hfw + 1, hfh + 1, ARRAY2D_CLEAR_DATA, 48);

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    // blur and resample highlight data; range=size of blur, pitch=sample spacing

    array2D<float> temp2((blurWidth / pitch) + ((blurWidth % pitch) == 0 ? 0 : 1), blurHeight);

    for (int m = 0; m < 4; m++) {
        boxblur_resamp(hilite_full[m], hilite[m], temp2, blurHeight, blurWidth, range, pitch);

        progress += 0.05;
        setProgCancel(progress);
    }

    temp2.free();

    for (int c = 0; c < 4; c++) {
        hilite_full[c].free();    //free up some memory
    }

    multi_array2D<float, 8> hilite_dir(hfw, hfh, ARRAY2D_CLEAR_DATA, 64);
    // for faster processing we create two buffers using (height,width) instead of (width,height)
    multi_array2D<float, 4> hilite_dir0(hfh, hfw, ARRAY2D_CLEAR_DATA, 64);
    multi_array2D<float, 4> hilite_dir4(hfh, hfw, ARRAY2D_CLEAR_DATA, 64);

    progress += 0.05;
    setProgCancel(progress);

    //fill gaps in highlight map by directional extension
    //raster scan from four corners
    for (int j = 1; j < hfw - 1; j++) {
        for (int i = 2; i < hfh - 2; i++) {
            //from left
            if (hilite[3][i][j] > epsilon) {
                hilite_dir0[3][j][i] = 1.f;
            } else {
                hilite_dir0[3][j][i] = (hilite_dir0[0 + 3][j - 1][i - 2] + hilite_dir0[0 + 3][j - 1][i - 1] + hilite_dir0[0 + 3][j - 1][i] + hilite_dir0[0 + 3][j - 1][i + 1] + hilite_dir0[0 + 3][j - 1][i + 2]) == 0.f ? 0.f : 0.1f;
            }
        }

        if(hilite[3][2][j] <= epsilon) {
            hilite_dir[0 + 3][0][j]  = hilite_dir0[3][j][2];
        }

        if(hilite[3][3][j] <= epsilon) {
            hilite_dir[0 + 3][1][j]  = hilite_dir0[3][j][3];
        }

        if(hilite[3][hfh - 3][j] <= epsilon) {
            hilite_dir[4 + 3][hfh - 1][j] = hilite_dir0[3][j][hfh - 3];
        }

        if(hilite[3][hfh - 4][j] <= epsilon) {
            hilite_dir[4 + 3][hfh - 2][j] = hilite_dir0[3][j][hfh - 4];
        }
    }

    for (int i = 2; i < hfh - 2; i++) {
        if(hilite[3][i][hfw - 2] <= epsilon) {
            hilite_dir4[3][hfw - 1][i] = hilite_dir0[3][hfw - 2][i];
        }
    }


#ifdef _OPENMP
    #pragma omp parallel for
#endif

    for (int c = 0; c < 3; c++) {
        for (int j = 1; j < hfw - 1; j++) {
            for (int i = 2; i < hfh - 2; i++) {
                //from left
                if (hilite[3][i][j] > epsilon) {
                    hilite_dir0[c][j][i] = hilite[c][i][j] / hilite[3][i][j];
                } else {
                    hilite_dir0[c][j][i] = 0.1f * ((hilite_dir0[0 + c][j - 1][i - 2] + hilite_dir0[0 + c][j - 1][i - 1] + hilite_dir0[0 + c][j - 1][i] + hilite_dir0[0 + c][j - 1][i + 1] + hilite_dir0[0 + c][j - 1][i + 2]) /
                                                   (hilite_dir0[0 + 3][j - 1][i - 2] + hilite_dir0[0 + 3][j - 1][i - 1] + hilite_dir0[0 + 3][j - 1][i] + hilite_dir0[0 + 3][j - 1][i + 1] + hilite_dir0[0 + 3][j - 1][i + 2] + epsilon));
                }
            }

            if(hilite[3][2][j] <= epsilon) {
                hilite_dir[0 + c][0][j]  = hilite_dir0[c][j][2];
            }

            if(hilite[3][3][j] <= epsilon) {
                hilite_dir[0 + c][1][j]  = hilite_dir0[c][j][3];
            }

            if(hilite[3][hfh - 3][j] <= epsilon) {
                hilite_dir[4 + c][hfh - 1][j] = hilite_dir0[c][j][hfh - 3];
            }

            if(hilite[3][hfh - 4][j] <= epsilon) {
                hilite_dir[4 + c][hfh - 2][j] = hilite_dir0[c][j][hfh - 4];
            }
        }

        for (int i = 2; i < hfh - 2; i++) {
            if(hilite[3][i][hfw - 2] <= epsilon) {
                hilite_dir4[c][hfw - 1][i] = hilite_dir0[c][hfw - 2][i];
            }
        }
    }

    progress += 0.05;
    setProgCancel(progress);

    for (int j = hfw - 2; j > 0; j--) {
        for (int i = 2; i < hfh - 2; i++) {
            //from right
            if (hilite[3][i][j] > epsilon) {
                hilite_dir4[3][j][i] = 1.f;
            } else {
                hilite_dir4[3][j][i] = (hilite_dir4[3][(j + 1)][(i - 2)] + hilite_dir4[3][(j + 1)][(i - 1)] + hilite_dir4[3][(j + 1)][(i)] + hilite_dir4[3][(j + 1)][(i + 1)] + hilite_dir4[3][(j + 1)][(i + 2)]) == 0.f ? 0.f : 0.1f;
            }
        }

        if(hilite[3][2][j] <= epsilon) {
            hilite_dir[0 + 3][0][j] += hilite_dir4[3][j][2];
        }

        if(hilite[3][hfh - 3][j] <= epsilon) {
            hilite_dir[4 + 3][hfh - 1][j] += hilite_dir4[3][j][hfh - 3];
        }
    }

    for (int i = 2; i < hfh - 2; i++) {
        if(hilite[3][i][0] <= epsilon) {
            hilite_dir[0 + 3][i - 2][0] += hilite_dir4[3][0][i];
            hilite_dir[4 + 3][i + 2][0] += hilite_dir4[3][0][i];
        }

        if(hilite[3][i][1] <= epsilon) {
            hilite_dir[0 + 3][i - 2][1] += hilite_dir4[3][1][i];
            hilite_dir[4 + 3][i + 2][1] += hilite_dir4[3][1][i];
        }

        if(hilite[3][i][hfw - 2] <= epsilon) {
            hilite_dir[0 + 3][i - 2][hfw - 2] += hilite_dir4[3][hfw - 2][i];
            hilite_dir[4 + 3][i + 2][hfw - 2] += hilite_dir4[3][hfw - 2][i];
        }
    }

#ifdef _OPENMP
    #pragma omp parallel for
#endif

    for (int c = 0; c < 3; c++) {
        for (int j = hfw - 2; j > 0; j--) {
            for (int i = 2; i < hfh - 2; i++) {
                //from right
                if (hilite[3][i][j] > epsilon) {
                    hilite_dir4[c][j][i] = hilite[c][i][j] / hilite[3][i][j];
                } else {
                    hilite_dir4[c][j][i] = 0.1f * ((hilite_dir4[c][(j + 1)][(i - 2)] + hilite_dir4[c][(j + 1)][(i - 1)] + hilite_dir4[c][(j + 1)][(i)] + hilite_dir4[c][(j + 1)][(i + 1)] + hilite_dir4[c][(j + 1)][(i + 2)]) /
                                                  (hilite_dir4[3][(j + 1)][(i - 2)] + hilite_dir4[3][(j + 1)][(i - 1)] + hilite_dir4[3][(j + 1)][(i)] + hilite_dir4[3][(j + 1)][(i + 1)] + hilite_dir4[3][(j + 1)][(i + 2)] + epsilon));
                }
            }

            if(hilite[3][2][j] <= epsilon) {
                hilite_dir[0 + c][0][j] += hilite_dir4[c][j][2];
            }

            if(hilite[3][hfh - 3][j] <= epsilon) {
                hilite_dir[4 + c][hfh - 1][j] += hilite_dir4[c][j][hfh - 3];
            }
        }

        for (int i = 2; i < hfh - 2; i++) {
            if(hilite[3][i][0] <= epsilon) {
                hilite_dir[0 + c][i - 2][0] += hilite_dir4[c][0][i];
                hilite_dir[4 + c][i + 2][0] += hilite_dir4[c][0][i];
            }

            if(hilite[3][i][1] <= epsilon) {
                hilite_dir[0 + c][i - 2][1] += hilite_dir4[c][1][i];
                hilite_dir[4 + c][i + 2][1] += hilite_dir4[c][1][i];
            }

            if(hilite[3][i][hfw - 2] <= epsilon) {
                hilite_dir[0 + c][i - 2][hfw - 2] += hilite_dir4[c][hfw - 2][i];
                hilite_dir[4 + c][i + 2][hfw - 2] += hilite_dir4[c][hfw - 2][i];
            }
        }
    }

    progress += 0.05;
    setProgCancel(progress);

    for (int i = 1; i < hfh - 1; i++)
        for (int j = 2; j < hfw - 2; j++) {
            //from top
            if (hilite[3][i][j] > epsilon) {
                hilite_dir[0 + 3][i][j] = 1.f;
            } else {
                hilite_dir[0 + 3][i][j] = (hilite_dir[0 + 3][i - 1][j - 2] + hilite_dir[0 + 3][i - 1][j - 1] + hilite_dir[0 + 3][i - 1][j] + hilite_dir[0 + 3][i - 1][j + 1] + hilite_dir[0 + 3][i - 1][j + 2]) == 0.f ? 0.f : 0.1f;
            }
        }

    for (int j = 2; j < hfw - 2; j++) {
        if(hilite[3][hfh - 2][j] <= epsilon) {
            hilite_dir[4 + 3][hfh - 1][j] += hilite_dir[0 + 3][hfh - 2][j];
        }
    }

#ifdef _OPENMP
    #pragma omp parallel for
#endif

    for (int c = 0; c < 3; c++) {
        for (int i = 1; i < hfh - 1; i++) {
            for (int j = 2; j < hfw - 2; j++) {
                //from top
                if (hilite[3][i][j] > epsilon) {
                    hilite_dir[0 + c][i][j] = hilite[c][i][j] / hilite[3][i][j];
                } else {
                    hilite_dir[0 + c][i][j] = 0.1f * ((hilite_dir[0 + c][i - 1][j - 2] + hilite_dir[0 + c][i - 1][j - 1] + hilite_dir[0 + c][i - 1][j] + hilite_dir[0 + c][i - 1][j + 1] + hilite_dir[0 + c][i - 1][j + 2]) /
                                                     (hilite_dir[0 + 3][i - 1][j - 2] + hilite_dir[0 + 3][i - 1][j - 1] + hilite_dir[0 + 3][i - 1][j] + hilite_dir[0 + 3][i - 1][j + 1] + hilite_dir[0 + 3][i - 1][j + 2] + epsilon));
                }
            }
        }

        for (int j = 2; j < hfw - 2; j++) {
            if(hilite[3][hfh - 2][j] <= epsilon) {
                hilite_dir[4 + c][hfh - 1][j] += hilite_dir[0 + c][hfh - 2][j];
            }
        }
    }

    progress += 0.05;
    setProgCancel(progress);

    for (int i = hfh - 2; i > 0; i--)
        for (int j = 2; j < hfw - 2; j++) {
            //from bottom
            if (hilite[3][i][j] > epsilon) {
                hilite_dir[4 + 3][i][j] = 1.f;
            } else {
                hilite_dir[4 + 3][i][j] = (hilite_dir[4 + 3][(i + 1)][(j - 2)] + hilite_dir[4 + 3][(i + 1)][(j - 1)] + hilite_dir[4 + 3][(i + 1)][(j)] + hilite_dir[4 + 3][(i + 1)][(j + 1)] + hilite_dir[4 + 3][(i + 1)][(j + 2)]) == 0.f ? 0.f : 0.1f;
            }
        }

#ifdef _OPENMP
    #pragma omp parallel for
#endif

    for (int c = 0; c < 4; c++) {
        for (int i = hfh - 2; i > 0; i--) {
            for (int j = 2; j < hfw - 2; j++) {
                //from bottom
                if (hilite[3][i][j] > epsilon) {
                    hilite_dir[4 + c][i][j] = hilite[c][i][j] / hilite[3][i][j];
                } else {
                    hilite_dir[4 + c][i][j] = 0.1f * ((hilite_dir[4 + c][(i + 1)][(j - 2)] + hilite_dir[4 + c][(i + 1)][(j - 1)] + hilite_dir[4 + c][(i + 1)][(j)] + hilite_dir[4 + c][(i + 1)][(j + 1)] + hilite_dir[4 + c][(i + 1)][(j + 2)]) /
                                                     (hilite_dir[4 + 3][(i + 1)][(j - 2)] + hilite_dir[4 + 3][(i + 1)][(j - 1)] + hilite_dir[4 + 3][(i + 1)][(j)] + hilite_dir[4 + 3][(i + 1)][(j + 1)] + hilite_dir[4 + 3][(i + 1)][(j + 2)] + epsilon));
                }
            }
        }
    }

    progress += 0.05;
    setProgCancel(progress);

    //fill in edges
    for (int dir = 0; dir < 2; dir++) {
        for (int i = 1; i < hfh - 1; i++)
            for (int c = 0; c < 4; c++) {
                hilite_dir[dir * 4 + c][i][0] = hilite_dir[dir * 4 + c][i][1];
                hilite_dir[dir * 4 + c][i][hfw - 1] = hilite_dir[dir * 4 + c][i][hfw - 2];
            }

        for (int j = 1; j < hfw - 1; j++)
            for (int c = 0; c < 4; c++) {
                hilite_dir[dir * 4 + c][0][j] = hilite_dir[dir * 4 + c][1][j];
                hilite_dir[dir * 4 + c][hfh - 1][j] = hilite_dir[dir * 4 + c][hfh - 2][j];
            }

        for (int c = 0; c < 4; c++) {
            hilite_dir[dir * 4 + c][0][0] = hilite_dir[dir * 4 + c][1][0] = hilite_dir[dir * 4 + c][0][1] = hilite_dir[dir * 4 + c][1][1] = hilite_dir[dir * 4 + c][2][2];
            hilite_dir[dir * 4 + c][0][hfw - 1] = hilite_dir[dir * 4 + c][1][hfw - 1] = hilite_dir[dir * 4 + c][0][hfw - 2] = hilite_dir[dir * 4 + c][1][hfw - 2] = hilite_dir[dir * 4 + c][2][hfw - 3];
            hilite_dir[dir * 4 + c][hfh - 1][0] = hilite_dir[dir * 4 + c][hfh - 2][0] = hilite_dir[dir * 4 + c][hfh - 1][1] = hilite_dir[dir * 4 + c][hfh - 2][1] = hilite_dir[dir * 4 + c][hfh - 3][2];
            hilite_dir[dir * 4 + c][hfh - 1][hfw - 1] = hilite_dir[dir * 4 + c][hfh - 2][hfw - 1] = hilite_dir[dir * 4 + c][hfh - 1][hfw - 2] = hilite_dir[dir * 4 + c][hfh - 2][hfw - 2] = hilite_dir[dir * 4 + c][hfh - 3][hfw - 3];
        }
    }

    for (int i = 1; i < hfh - 1; i++)
        for (int c = 0; c < 4; c++) {
            hilite_dir0[c][0][i] = hilite_dir0[c][1][i];
            hilite_dir0[c][hfw - 1][i] = hilite_dir0[c][hfw - 2][i];
        }

    for (int j = 1; j < hfw - 1; j++)
        for (int c = 0; c < 4; c++) {
            hilite_dir0[c][j][0] = hilite_dir0[c][j][1];
            hilite_dir0[c][j][hfh - 1] = hilite_dir0[c][j][hfh - 2];
        }

    for (int c = 0; c < 4; c++) {
        hilite_dir0[c][0][0] = hilite_dir0[c][0][1] = hilite_dir0[c][1][0] = hilite_dir0[c][1][1] = hilite_dir0[c][2][2];
        hilite_dir0[c][hfw - 1][0] = hilite_dir0[c][hfw - 1][1] = hilite_dir0[c][hfw - 2][0] = hilite_dir0[c][hfw - 2][1] = hilite_dir0[c][hfw - 3][2];
        hilite_dir0[c][0][hfh - 1] = hilite_dir0[c][0][hfh - 2] = hilite_dir0[c][1][hfh - 1] = hilite_dir0[c][1][hfh - 2] = hilite_dir0[c][2][hfh - 3];
        hilite_dir0[c][hfw - 1][hfh - 1] = hilite_dir0[c][hfw - 1][hfh - 2] = hilite_dir0[c][hfw - 2][hfh - 1] = hilite_dir0[c][hfw - 2][hfh - 2] = hilite_dir0[c][hfw - 3][hfh - 3];
    }

    for (int i = 1; i < hfh - 1; i++)
        for (int c = 0; c < 4; c++) {
            hilite_dir4[c][0][i] = hilite_dir4[c][1][i];
            hilite_dir4[c][hfw - 1][i] = hilite_dir4[c][hfw - 2][i];
        }

    for (int j = 1; j < hfw - 1; j++)
        for (int c = 0; c < 4; c++) {
            hilite_dir4[c][j][0] = hilite_dir4[c][j][1];
            hilite_dir4[c][j][hfh - 1] = hilite_dir4[c][j][hfh - 2];
        }

    for (int c = 0; c < 4; c++) {
        hilite_dir4[c][0][0] = hilite_dir4[c][0][1] = hilite_dir4[c][1][0] = hilite_dir4[c][1][1] = hilite_dir4[c][2][2];
        hilite_dir4[c][hfw - 1][0] = hilite_dir4[c][hfw - 1][1] = hilite_dir4[c][hfw - 2][0] = hilite_dir4[c][hfw - 2][1] = hilite_dir4[c][hfw - 3][2];
        hilite_dir4[c][0][hfh - 1] = hilite_dir4[c][0][hfh - 2] = hilite_dir4[c][1][hfh - 1] = hilite_dir4[c][1][hfh - 2] = hilite_dir4[c][2][hfh - 3];
        hilite_dir4[c][hfw - 1][hfh - 1] = hilite_dir4[c][hfw - 1][hfh - 2] = hilite_dir4[c][hfw - 2][hfh - 1] = hilite_dir4[c][hfw - 2][hfh - 2] = hilite_dir4[c][hfw - 3][hfh - 3];
    }

    progress += 0.05;
    setProgCancel(progress);

    //free up some memory
    for(int c = 0; c < 4; c++) {
        hilite[c].free();
    }

    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    // now reconstruct clipped channels using color ratios

#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic,16)
#endif

    for (int i = 0; i < blurHeight; i++) {
        int i1 = min((i - (i % pitch)) / pitch, hfh - 1);

        for (int j = 0; j < blurWidth; j++) {

            float pixel[3] = {red[i + miny][j + minx], green[i + miny][j + minx], blue[i + miny][j + minx]};

            if (pixel[0] < max_f[0] && pixel[1] < max_f[1] && pixel[2] < max_f[2]) {
                continue;    //pixel not clipped
            }

            int j1 = min((j - (j % pitch)) / pitch, hfw - 1);

            //estimate recovered values using modified HLRecovery_blend algorithm
            float rgb[ColorCount], rgb_blend[ColorCount] = {}, cam[2][ColorCount], lab[2][ColorCount], sum[2], chratio;

            // Copy input pixel to rgb so it's easier to access in loops
            rgb[0] = pixel[0];
            rgb[1] = pixel[1];
            rgb[2] = pixel[2];

            // Initialize cam with raw input [0] and potentially clipped input [1]
            for (int c = 0; c < ColorCount; c++) {
                cam[0][c] = rgb[c];
                cam[1][c] = min(cam[0][c], clippt);
            }

            // Calculate the lightness correction ratio (chratio)
            for (int i2 = 0; i2 < 2; i2++) {
                for (int c = 0; c < ColorCount; c++) {
                    lab[i2][c] = 0;

                    for (int c1 = 0; c1 < ColorCount; c1++) {
                        lab[i2][c] += trans[c][c1] * cam[i2][c1];
                    }
                }

                sum[i2] = 0.f;

                for (int c = 1; c < ColorCount; c++) {
                    sum[i2] += SQR(lab[i2][c]);
                }
            }

            if(sum[0] == 0.f) {     // avoid division by zero
                sum[0] = epsilon;
            }

            chratio = sqrtf(sum[1] / sum[0]);


            // Apply ratio to lightness in lab space
            for (int c = 1; c < ColorCount; c++) {
                lab[0][c] *= chratio;
            }

            // Transform back from lab to RGB
            for (int c = 0; c < ColorCount; c++) {
                cam[0][c] = 0;

                for (int c1 = 0; c1 < ColorCount; c1++) {
                    cam[0][c] += itrans[c][c1] * lab[0][c1];
                }
            }

            for (int c = 0; c < ColorCount; c++) {
                rgb[c] = cam[0][c] / ColorCount;
            }

            // Copy converted pixel back
            float rfrac = max(0.f, min(1.f, medFactor[0] * (pixel[0] - blendpt)));
            float gfrac = max(0.f, min(1.f, medFactor[1] * (pixel[1] - blendpt)));
            float bfrac = max(0.f, min(1.f, medFactor[2] * (pixel[2] - blendpt)));

            if (pixel[0] > blendpt) {
                rgb_blend[0] = rfrac * rgb[0] + (1.f - rfrac) * pixel[0];
            }

            if (pixel[1] > blendpt) {
                rgb_blend[1] = gfrac * rgb[1] + (1.f - gfrac) * pixel[1];
            }

            if (pixel[2] > blendpt) {
                rgb_blend[2] = bfrac * rgb[2] + (1.f - bfrac) * pixel[2];
            }

            //end of HLRecovery_blend estimation
            //%%%%%%%%%%%%%%%%%%%%%%%

            //there are clipped highlights
            //first, determine weighted average of unclipped extensions (weighting is by 'hue' proximity)
            float totwt = 0.f;
            float clipfix[3] = {0.f, 0.f, 0.f};

            float Y = epsilon + rgb_blend[0] + rgb_blend[1] + rgb_blend[2];

            for (int c = 0; c < ColorCount; c++) {
                rgb_blend[c] /= Y;
            }

            float Yhi = 1.f / (hilite_dir0[0][j1][i1] + hilite_dir0[1][j1][i1] + hilite_dir0[2][j1][i1]);

            if (Yhi < 2.f) {
                float dirwt = 1.f / (1.f + 65535.f * (SQR(rgb_blend[0] - hilite_dir0[0][j1][i1] * Yhi) +
                                                      SQR(rgb_blend[1] - hilite_dir0[1][j1][i1] * Yhi) +
                                                      SQR(rgb_blend[2] - hilite_dir0[2][j1][i1] * Yhi)));
                totwt = dirwt;
                dirwt /= (hilite_dir0[3][j1][i1] + epsilon);
                clipfix[0] = dirwt * hilite_dir0[0][j1][i1];
                clipfix[1] = dirwt * hilite_dir0[1][j1][i1];
                clipfix[2] = dirwt * hilite_dir0[2][j1][i1];
            }

            for (int dir = 0; dir < 2; dir++) {
                float Yhil = 1.f / ( hilite_dir[dir * 4 + 0][i1][j1] + hilite_dir[dir * 4 + 1][i1][j1] + hilite_dir[dir * 4 + 2][i1][j1]);

                if (Yhil < 2.f) {
                    float dirwt = 1.f / (1.f + 65535.f * (SQR(rgb_blend[0] - hilite_dir[dir * 4 + 0][i1][j1] * Yhil) +
                                                          SQR(rgb_blend[1] - hilite_dir[dir * 4 + 1][i1][j1] * Yhil) +
                                                          SQR(rgb_blend[2] - hilite_dir[dir * 4 + 2][i1][j1] * Yhil)));
                    totwt += dirwt;
                    dirwt /= (hilite_dir[dir * 4 + 3][i1][j1] + epsilon);
                    clipfix[0] += dirwt * hilite_dir[dir * 4 + 0][i1][j1];
                    clipfix[1] += dirwt * hilite_dir[dir * 4 + 1][i1][j1];
                    clipfix[2] += dirwt * hilite_dir[dir * 4 + 2][i1][j1];
                }
            }


            Yhi = 1.f / (hilite_dir4[0][j1][i1] + hilite_dir4[1][j1][i1] + hilite_dir4[2][j1][i1]);

            if (Yhi < 2.f) {
                float dirwt = 1.f / (1.f + 65535.f * (SQR(rgb_blend[0] - hilite_dir4[0][j1][i1] * Yhi) +
                                                      SQR(rgb_blend[1] - hilite_dir4[1][j1][i1] * Yhi) +
                                                      SQR(rgb_blend[2] - hilite_dir4[2][j1][i1] * Yhi)));
                totwt += dirwt;
                dirwt /= (hilite_dir4[3][j1][i1] + epsilon);
                clipfix[0] += dirwt * hilite_dir4[0][j1][i1];
                clipfix[1] += dirwt * hilite_dir4[1][j1][i1];
                clipfix[2] += dirwt * hilite_dir4[2][j1][i1];
            }

            if(totwt == 0.f) {
                continue;
            }

            clipfix[0] /= totwt;
            clipfix[1] /= totwt;
            clipfix[2] /= totwt;

            //now correct clipped channels
            if (pixel[0] > max_f[0] && pixel[1] > max_f[1] && pixel[2] > max_f[2]) {
                //all channels clipped
                float Yl = (0.299f * clipfix[0] + 0.587f * clipfix[1] + 0.114f * clipfix[2]);

                float mult = whitept / Yl;
                red[i + miny][j + minx]   = clipfix[0] * mult; //factor;
                green[i + miny][j + minx] = clipfix[1] * mult; //factor;
                blue[i + miny][j + minx]  = clipfix[2] * mult; //factor;
            } else {//some channels clipped
                float notclipped[3] = {pixel[0] <= max_f[0] ? 1.f : 0.f, pixel[1] <= max_f[1] ? 1.f : 0.f, pixel[2] <= max_f[2] ? 1.f : 0.f};

                if (notclipped[0] == 0.f) { //red clipped
                    red[i + miny][j + minx]  = max(red[i + miny][j + minx], (clipfix[0] * ((notclipped[1] * pixel[1] + notclipped[2] * pixel[2]) /
                                                 (notclipped[1] * clipfix[1] + notclipped[2] * clipfix[2] + epsilon))));
                }

                if (notclipped[1] == 0.f) { //green clipped
                    green[i + miny][j + minx] = max(green[i + miny][j + minx], (clipfix[1] * ((notclipped[2] * pixel[2] + notclipped[0] * pixel[0]) /
                                                    (notclipped[2] * clipfix[2] + notclipped[0] * clipfix[0] + epsilon))));
                }

                if (notclipped[2] == 0.f) { //blue clipped
                    blue[i + miny][j + minx]  = max(blue[i + miny][j + minx], (clipfix[2] * ((notclipped[0] * pixel[0] + notclipped[1] * pixel[1]) /
                                                   (notclipped[0] * clipfix[0] + notclipped[1] * clipfix[1] + epsilon))));
                }
            }

            Y = (0.299f * red[i + miny][j + minx] + 0.587f * green[i + miny][j + minx] + 0.114f * blue[i + miny][j + minx]);

            if (Y > whitept) {
                float mult = whitept / Y;

                red[i + miny][j + minx]   *= mult; 
                green[i + miny][j + minx] *= mult; 
                blue[i + miny][j + minx]  *= mult; 
            }
        }
    }

    setProgCancel(1.00);

    return RP_NO_ERROR;

}// end of HLReconstruction
