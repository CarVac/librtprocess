////////////////////////////////////////////////////////////////
//
//  White Balance application on raw bayer cfa data
//
//  copyright (c) 2019 Glenn Butcher <glenn.butcher@gmail.com>
//
//  code dated: February 7, 2019
//
//  WB_apply.cc is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////

#include "librtprocess.h"
#include "bayerhelper.h"
#include "StopWatch.h"

using namespace librtprocess;

rpError WB_apply(int width, int height, float ** rawData, float redmult, float green1mult, float green2mult, float bluemult, const unsigned cfarray[2][2], const std::function<bool(double)> &setProgCancel)
{
	BENCHFUN
	
	float cfamults[2][2] = 	
	{
		{ 1.0, 1.0 },
		{ 1.0, 1.0 }
	};
	
	if (!validateBayerCfa(4, cfarray)) {
        return RP_WRONG_CFA;
    }
	
	for (int x=0; x<2; x++) {
		for (int y=0; y<2; y++) {
			if (cfarray[x][y] == 0) cfamults[x][y] = redmult;
			if (cfarray[x][y] == 1) cfamults[x][y] = green1mult;
			if (cfarray[x][y] == 2) cfamults[x][y] = bluemult;
			if (cfarray[x][y] == 3) cfamults[x][y] = green2mult;
		}
	}
	
	double progress = 0.0;
    setProgCancel(progress);

	#pragma omp parallel for 
	for (int y=0; y<height; y+=2) {
		for (int x=0; x<width; x+=2) {
			for (int i=0; i<2; i++) {
				for (int j=0; j<2; j++) {
					rawData[y+i][x+j] *= cfamults[i][j]; 
				}
			}
		}
	}

	setProgCancel(1.0);
	
	return RP_NO_ERROR;
	
}

