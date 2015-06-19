//# FlagAgentElevation.h: This file contains the interface definition of the FlagAgentElevation class.
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

#ifndef FlagAgentElevation_H_
#define FlagAgentElevation_H_

#include <flagging/Flagging/FlagAgentBase.h>

namespace casa { //# NAMESPACE CASA - BEGIN

class FlagAgentElevation : public FlagAgentBase {

public:

	FlagAgentElevation(FlagDataHandler *dh, Record config, Bool writePrivateFlagCube = false, Bool flag = true);
	~FlagAgentElevation();

protected:

	// Common functionality for each visBuffer (don't repeat at the row level)
	void preProcessBuffer(const VisBuffer &visBuffer);

	// Common preProcessing code for the single/multiple agent cases
	void preProcessBufferCore(const VisBuffer &visBuffer);

	// Compute flags afor a given mapped visibility point
	bool computeRowFlags(const VisBuffer &visBuffer, FlagMapper &flags, uInt row);

	// Parse configuration parameters
	void setAgentParameters(Record config);

private:

	/// Input parameters ///
	Double lowerlimit_p;
	Double upperlimit_p;

	// Declaration of static members for common pre-processing
	uShort agentNumber_p;
	static vector< vector<Double> > antennaPointingMap_p;
	static casa::async::Mutex staticMembersMutex_p;
	static vector<bool> startedProcessing_p;
	static bool preProcessingDone_p;
	static uShort nAgents_p;
};


} //# NAMESPACE CASA - END

#endif /* FlagAgentElevation_H_ */
