//# FlagAgentTimeFreqCrop.h: This file contains the interface definition of the FlagAgentTimeFreqCrop class.
//#
//#  CASA - Common Astronomy Software Applications (http://casa.nrao.edu/)
//#  Copyright (C) Associated Universities, Inc. Washington DC, USA 2011, All rights reserved.
//#  Copyright (C) European Southern Observatory, 2011, All rights reserved.
//#
//#  This library is free software; you can redistribute it and/or
//#  modify it under the terms of the GNU Lesser General Public
//#  License as published by the Free software Foundation; either
//#  version 2.1 of the License, or (at your option) any later version.
//#
//#  This library is distributed in the hope that it will be useful,
//#  but WITHOUT ANY WARRANTY, without even the implied warranty of
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//#  Lesser General Public License for more details.
//#
//#  You should have received a copy of the GNU Lesser General Public
//#  License along with this library; if not, write to the Free Software
//#  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
//#  MA 02111-1307  USA
//# $Id: $

#ifndef FlagAgentTimeFreqCrop_H_
#define FlagAgentTimeFreqCrop_H_

#include <flagging/Flagging/FlagAgentBase.h>

namespace casa { //# NAMESPACE CASA - BEGIN

class FlagAgentTimeFreqCrop : public FlagAgentBase {

public:

	FlagAgentTimeFreqCrop(FlagDataHandler *dh, Record config, Bool writePrivateFlagCube = false);
	~FlagAgentTimeFreqCrop();

protected:

	// Compute flags for a given (time,freq) map
	void computeAntennaPairFlags(VisMapper &visibilities,FlagMapper &flag,Int antenna1,Int antenna2);

	// Parse configuration parameters
	void setAgentParameters(Record config);

private:

	/// Input parameters ///

	// Flag threshold in time (flag all data-points further than N-stddev from the fit).
	Double T_TOL_p;
    // Flag threshold in frequency. Flag all data-points further than N-stddev from the fit.
	Double F_TOL_p;
    // Maximum number of pieces to allow in the piecewise-polynomial fits (1-9)
	Int MaxNPieces_p;
    // Fitting function for the time direction  ('line' or 'poly')
	String timeFitType_p;
    // Fitting function for the frequency direction  ('line' or 'poly')
	String freqFitType_p;
    // Choose the directions along which to perform flagging ('time', 'freq', 'timefreq', 'freqtime')
	String flagDimension_p;
    // Half width of sliding window to use with 'usewindowstats' (1,2,3 for 3-point, 5-point or 7-point window sizes)
    Int halfWin_p;
    // Use sliding-window statistics to find additional flags ('none', 'sum', 'std', 'both' )
    String winStats_p;

};


} //# NAMESPACE CASA - END

#endif /* FlagAgentTimeFreqCrop_H_ */

