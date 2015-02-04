//# Copyright (C) 2000,2001
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This library is free software; you can redistribute it and/or modify it
//# under the terms of the GNU Library General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or (at your
//# option) any later version.
//#
//# This library is distributed in the hope that it will be useful, but WITHOUT
//# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
//# License for more details.
//#
//# You should have received a copy of the GNU Library General Public License
//# along with this library; if not, write to the Free Software Foundation,
//# Inc., 675 Massachusetts Ave, Cambridge, MA 02139, USA.
//#
//# Correspondence concerning AIPS++ should be addressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#

#ifndef SCIMATH_ZSCORECALCULATOR_H
#define SCIMATH_ZSCORECALCULATOR_H

#include <casa/aips.h>

#include <map>
#include <set>
#include <math.h>

namespace casa {

/*
 * This class contains static methods related to z-scores. A z-score is the number of
 * standard deviations from the mean in a normal distribution.
 */

class ZScoreCalculator {
public:

	// compute the maximum expected zscore given the number of points
	// in a sample.
	static Double getMaxZScore(uInt64 npts);

	// Get the minimum number of points in a Gaussian distribution, such that the
	// probability that the maximum of the distribution having the specified zscore
	// is 0.5. <src>zscore</src> should be non-negative.
	static inline uInt64 zscoreToNpts(Double zscore) {
		return (uInt64)(0.5/erfc(zscore/sqrt(2)));
	}

private:
	static std::map<uInt64, Double> _nptsToMaxZScore;

};

}

#endif
