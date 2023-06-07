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

#ifndef _LIBRTPROCESS_
#define _LIBRTPROCESS_

#include <stddef.h>

#include <functional>
#include <cstddef>

#ifndef LIBRTPROCESS_STATIC
// DLL interface export/import macros are only available for MSVC for now, 
// in order to keep compatibility with previous versions of librtprocess 
//#   if defined( __WIN32__ ) || defined( _WIN32 )
#   ifdef _MSC_VER
#       if defined( rtprocess_EXPORTS ) 
#           define RTPROCESS_API __declspec( dllexport )
#       else
#           define RTPROCESS_API __declspec( dllimport )
#       endif
#   else
//#     define RTPROCESS_API  __attribute__ ((visibility("default")))
#       define RTPROCESS_API
#   endif
#else 
#   define RTPROCESS_API
#endif

enum rpError {RP_NO_ERROR, RP_MEMORY_ERROR, RP_WRONG_CFA, RP_CACORRECT_ERROR};
RTPROCESS_API rpError bayerborder_demosaic(int winw, int winh, int lborders, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2]);
RTPROCESS_API void xtransborder_demosaic(int winw, int winh, int border, const float * const *rawData, float **red, float **green, float **blue, const unsigned xtrans[6][6]);
RTPROCESS_API rpError ahd_demosaic (int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const float rgb_cam[3][4], const std::function<bool(double)> &setProgCancel);
RTPROCESS_API rpError amaze_demosaic(int raw_width, int raw_height, int winx, int winy, int winw, int winh, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel, double initGain, int border, float inputScale, float outputScale, std::size_t chunkSize = 2, bool measure = false);
RTPROCESS_API rpError bayerfast_demosaic(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel, double initGain);
RTPROCESS_API rpError dcb_demosaic(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel, int iterations, bool dcb_enhance);
RTPROCESS_API rpError hphd_demosaic(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel);
RTPROCESS_API rpError rcd_demosaic(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel, std::size_t chunkSize = 2, bool measure = false, bool multiThread = true);
RTPROCESS_API rpError markesteijn_demosaic(int width, int height, const float * const *rawdata, float **red, float **green, float **blue, const unsigned xtrans[6][6], const float rgb_cam[3][4], const std::function<bool(double)> &setProgCancel, const int passes, const bool useCieLab, std::size_t chunkSize = 2, bool measure = false);
RTPROCESS_API rpError xtransfast_demosaic(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned xtrans[6][6], const std::function<bool(double)> &setProgCancel);
RTPROCESS_API rpError vng4_demosaic (int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel);
RTPROCESS_API rpError igv_demosaic(int winw, int winh, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel);
RTPROCESS_API rpError lmmse_demosaic(int width, int height, const float * const *rawData, float **red, float **green, float **blue, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel, int iterations);
// for CA_correct rawDataIn and rawDataOut may point to the same buffer. That's handled fine inside CA_correct
RTPROCESS_API rpError CA_correct(int winx, int winy, int winw, int winh, const bool autoCA, std::size_t autoIterations, const double cared, const double cablue, bool avoidColourshift, const float * const *rawDataIn, float **rawDataOut, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel, double fitParams[2][2][16], bool fitParamsIn, float inputScale = 65535.f, float outputScale = 65535.f, size_t chunkSize = 2, bool measure = false);
RTPROCESS_API rpError HLRecovery_inpaint(const int width, const int height, float **red, float **green, float **blue, const float chmax[3], const float clmax[3], const std::function<bool(double)> &setProgCancel);

#endif
