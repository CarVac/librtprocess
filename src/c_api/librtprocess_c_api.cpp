/*
 * This file is part of librtprocess.
 *
 * Copyright (c) 2018 Carlo Vaccari
 *
 * librtprocess is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the license, or
 * (at your option) any later version.
 *
 * librtprocess is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with librtprocess.  If not, see <http://www.gnu.org/licenses/>.
 */

// C API Wrapper for librtprocess

#include "librtprocess.h"

extern "C"
{

rpError rt_process_bayerborder_demosaic_c_api(int winw, int winh, int lborders, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2])
{
    return bayerborder_demosaic(winw, winh, lborders, rawData, red, green, blue, cfarray);
}

void rt_process_xtransborder_demosaic_c_api(int winw, int winh, int border, const float * const *rawData, float **red, float **green, float **blue, const unsigned xtrans[6][6])
{
    return xtransborder_demosaic(winw, winh, border, rawData, red, green, blue, xtrans);
}

rpError rt_process_ahd_demosaic_c_api(int width, int height, const float* const* rawData, float** red, float** green, float** blue, const unsigned cfarray[2][2], const float rgb_cam[3][4], bool(* setProgCancel)(double))
{
    return ahd_demosaic(width, height, rawData, red, green, blue, cfarray, rgb_cam, setProgCancel);
}

rpError rt_process_amaze_demosaic_c_api(int raw_width, int raw_height, int winx, int winy, int winw, int winh, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], bool(* setProgCancel)(double), double initGain, int border, float inputScale, float outputScale, size_t chunkSize, bool measure)
{
    return amaze_demosaic(raw_width, raw_height, winx, winy, winw, winh, rawData, red, green, blue, cfarray, setProgCancel, initGain, border, inputScale, outputScale, chunkSize ? chunkSize : 2, measure);
}

rpError rt_process_bayerfast_demosaic_c_api(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], bool(* setProgCancel)(double), double initGain)
{
    return bayerfast_demosaic(width, height,  rawData, red, green, blue, cfarray, setProgCancel, initGain);
}

rpError rt_process_dcb_demosaic_c_api(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], bool(* setProgCancel)(double), int iterations, bool dcb_enhance)
{
    return dcb_demosaic(width, height,  rawData, red, green, blue, cfarray, setProgCancel, iterations, dcb_enhance);
}

rpError rt_process_hphd_demosaic_c_api(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], bool(* setProgCancel)(double))
{
    return hphd_demosaic(width, height,  rawData, red, green, blue, cfarray, setProgCancel);
}

rpError rt_process_rcd_demosaic_c_api(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], bool(* setProgCancel)(double), size_t chunkSize, bool measure, bool multiThread)
{
    return rcd_demosaic(width, height,  rawData, red, green, blue, cfarray, setProgCancel, chunkSize ? chunkSize : 2, measure, multiThread);
}

rpError rt_process_markesteijn_demosaic_c_api(int width, int height, const float * const *rawdata, float **red, float **green, float **blue, const unsigned xtrans[6][6], const float rgb_cam[3][4], bool(* setProgCancel)(double), const int passes, const bool useCieLab, size_t chunkSize, bool measure)
{
    return markesteijn_demosaic(width, height,  rawdata, red, green, blue, xtrans, rgb_cam, setProgCancel, passes, useCieLab, chunkSize ? chunkSize: 2, measure);
}

rpError rt_process_xtransfast_demosaic_c_api(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned xtrans[6][6], bool(* setProgCancel)(double))
{
    return xtransfast_demosaic(width, height,  rawData, red, green, blue, xtrans, setProgCancel);
}

rpError rt_process_vng4_demosaic_c_api (int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], bool(* setProgCancel)(double))
{
    return vng4_demosaic(width, height,  rawData, red, green, blue, cfarray, setProgCancel);
}

rpError rt_process_igv_demosaic_c_api(int winw, int winh, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], bool(* setProgCancel)(double))
{
    return igv_demosaic(winw, winh,  rawData, red, green, blue, cfarray, setProgCancel);
}

rpError rt_process_lmmse_demosaic_c_api(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], bool(* setProgCancel)(double), int iterations)
{
    return lmmse_demosaic(width, height,  rawData, red, green, blue, cfarray, setProgCancel, iterations);
}

rpError rt_process_CA_correct_c_api(int winx, int winy, int winw, int winh, const bool autoCA, size_t autoIterations, const double cared, const double cablue, bool avoidColourshift, const float * const *rawDataIn, float **rawDataOut, const unsigned cfarray[2][2], bool(* setProgCancel)(double), double fitParams[2][2][16], bool fitParamsIn, float inputScale, float outputScale, size_t chunkSize, bool measure)
{
    return CA_correct(winx, winy, winw, winh, autoCA, autoIterations, cared, cablue, avoidColourshift,  rawDataIn, rawDataOut, cfarray, setProgCancel, fitParams, fitParamsIn, inputScale != 0 ? inputScale : (float)65535, outputScale != 0 ? outputScale : (float)65535, chunkSize ? chunkSize: 2, measure);
}

rpError rt_process_HLRecovery_inpaint_c_api(const int width, const int height, float **red, float **green, float **blue, const float chmax[3], const float clmax[3], bool(* setProgCancel)(double))
{
    return HLRecovery_inpaint(width, height, red, green, blue, chmax, clmax,  setProgCancel);
}

} // extern "C"
