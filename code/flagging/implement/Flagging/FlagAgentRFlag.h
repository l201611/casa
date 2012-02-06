//# FlagAgentRFlag.h: This file contains the interface definition of the FlagAgentRFlag class.
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

#ifndef FlagAgentRFlag_H_
#define FlagAgentRFlag_H_

#include <flagging/Flagging/FlagAgentBase.h>

namespace casa { //# NAMESPACE CASA - BEGIN

class FlagAgentRFlag : public FlagAgentBase {

public:

	FlagAgentRFlag(FlagDataHandler *dh, Record config, Bool writePrivateFlagCube = false, Bool flag = true);
	~FlagAgentRFlag();

protected:

	// Compute flags for a given (time,freq) map
	bool computeAntennaPairFlags(const VisBuffer &visBuffer, VisMapper &visibilities,FlagMapper &flags,Int antenna1,Int antenna2,vector<uInt> &rows);

	// Parse configuration parameters
	void setAgentParameters(Record config);

private:

	/// Input parameters ///

	Int half_nchan_p;
	Int half_ntime_p;


};


} //# NAMESPACE CASA - END

#endif /* FlagAgentRFlag_H_ */

