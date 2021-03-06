//# CTSelection.h: Class to represent a selection on a CASA CalTable
//# Copyright (C) 1996,1997,1998,1999,2001
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
//#
//# $Id$

#ifndef SYNTHESIS_CTSELECTION_H
#define SYNTHESIS_CTSELECTION_H

#include <casa/aips.h>
#include <casa/BasicSL/String.h>
#include <casa/Arrays/Vector.h>
#include <measures/Measures/MEpoch.h>
#include <measures/Measures/MRadialVelocity.h>
#include <tables/TaQL/ExprNode.h>
#include <ms/MeasurementSets/MeasurementSet.h>
#include <ms/MSSel/MSSelection.h>
#include <casa/Arrays/Matrix.h>
#include <casa/Arrays/Cube.h>
#include <ms/MSSel/MSSelectionError.h>
#include <ms/MSSel/MSSelectionErrorHandler.h>
#include <ms/MSSel/MSSelectableTable.h>
#include <casa/Containers/OrderedMap.h>
#include <casa/Containers/MapIO.h>
#include <synthesis/CalTables/NewCalTable.h>

namespace casa { //# NAMESPACE CASA - BEGIN

// <summary> 
// CTSelection: Class to represent a selection on a CASA CalTable
// </summary>

// <use visibility=export>

// <reviewed reviewer="" date="" tests="" demos="">

// <prerequisite>
//   <li> <linkto class="casacore::MeasurementSet">casacore::MeasurementSet</linkto> module
// </prerequisite>
//
// <etymology>
// From "CalTable" and "selection".
// </etymology>
//
// <synopsis>
// The CTSelection class represents a selection on a CASA CalTable (CT).
//
// The purpose of this class is to provides a simple expression based
// selection mechanism to both the end-user and developer wishing to
// perform query operations over a measurement set.  This class is a
// specialization of the CASACORE casacore::MSSelection class.  The
// specialization this provides is that this workes with CT and uses
// parsers specialization for CT where necessary.  For a complete list
// of the STaQL interface refer to the casacore::MeasurementSet Selection Syntax
// document at: <a href="http://casa.nrao.edu/other_doc.shtml">Data
// Selection</a>
//
// The sub-expressions are interpreted in the order which they were
// set.  The order however in not important - any dependency on the
// order in which the expressions are evaluated is handled internally.
// The result of parsing the expressions is casacore::TableExprNode (TEN).  All
// TENs from sub-expressions are finally ANDed and the resultant TEN
// is used to select the rows of the casacore::MS table.
//
// </synopsis>
//
// <example>
// <srcblock>
// </srcblock>
// </example>
//
// <motivation>
// </motivation>
//
// <todo asof="Aug/14/2009">
// </todo>

  class CTSelection: public MSSelection
  {
  public:

    // Default null constructor, and destructor
    CTSelection();
    virtual ~CTSelection();
    
    casacore::TableExprNode toTableExprNode(casacore::MSSelectableTable* msLike);
    // Construct using an casacore::MS and the various selection expressions to
    // be applied to the given MS.  By default, the expressions will
    // be parsed immediately.  With mode=PARSE_LATE, the parsing will
    // be done with a call to toTableExprNode().
    CTSelection(const NewCalTable& ct,
		const MSSMode& mode= PARSE_NOW,
		const casacore::String& timeExpr="",
		const casacore::String& antennaExpr="",
		const casacore::String& fieldExpr="",
		const casacore::String& spwExpr="",
		const casacore::String& uvDistExpr="",
		const casacore::String& taqlExpr="",
		const casacore::String& polnExpr="",
		const casacore::String& scanExpr="",
		const casacore::String& arrayExpr="",
		const casacore::String& stateExpr="",
		const casacore::String& observationExpr="");
    
    // Construct from a record representing a selection item at the
    // CLI or user interface level.  This is functionally same as the
    // constructor above with mode=PARSE_LATE.
    CTSelection(const casacore::Record& selectionItem);
    
    // Copy constructor
    CTSelection(const CTSelection& other);
    
    // Assignment operator
    CTSelection& operator=(const CTSelection& other);
    
    // Accessor for result of parsing all of the selection
    // expressions.  The final casacore::TableExprNode (TEN) is the result of
    // ANDing the TENs for the individual expressions.
    //    casacore::TableExprNode getTEN(const casacore::MeasurementSet*ms = NULL);

    // Accessor for the list of antenna-1 of the selected baselines.
    // Antennas affected by the baseline negation operator have the
    // antenna IDs multiplied by -1.

    // // inline casacore::Vector<casacore::Int> getAntenna1List(const casacore::MeasurementSet* ms=NULL) 
    // // {getTEN(ms); return antenna1IDs_p;}
    
    // // Accessor for the list of antenna-2 of the selected baselines.
    // // Antennas affected by the baseline negation operator have the
    // // antenna IDs multiplied by -1.
    // // inline casacore::Vector<casacore::Int> getAntenna2List(const casacore::MeasurementSet* ms=NULL) 
    // // {getTEN(ms); return antenna2IDs_p;}
    
    // // inline casacore::Matrix<casacore::Int> getBaselineList(const casacore::MeasurementSet* ms=NULL) 
    // // {getTEN(ms); return baselineIDs_p;}
    
    // // // Accessor for the list of selected field IDs.
    // // inline casacore::Vector<casacore::Int> getFieldList(const casacore::MeasurementSet* ms=NULL) 
    // // {// if (fieldIDs_p.nelements() <= 0) 
    // // 	getTEN(ms); return fieldIDs_p;}

    // // // Accessor for the list of the specified time range(s) as the
    // // // start and end MJD values.  The time ranges are stored as columns,
    // // // i.e. the output casacore::Matrix is 2 x n_ranges.
    // // inline casacore::Matrix<casacore::Double> getTimeList(const casacore::MeasurementSet* ms=NULL)
    // // {getTEN(ms); return selectedTimesList_p;}
    
    // // // Accessor for the list of the selected Spectral Window IDs.
    // // inline casacore::Vector<casacore::Int> getSpwList(const casacore::MeasurementSet* ms=NULL) 
    // // {// if (spwIDs_p.nelements() <= 0) 
    // // 	getTEN(ms); return spwIDs_p;}


    // // casacore::Matrix<casacore::Int> getChanList(const casacore::MeasurementSet* ms=NULL, 
    // // 			    const casacore::Int defaultStep=1,
    // // 			    const casacore::Bool sorted=false);

    // // //
    // // // Same as getChanList, except that the channels and steps are in Hz.
    // // //    
    // // casacore::Matrix<casacore::Double> getChanFreqList(const casacore::MeasurementSet* ms=NULL, 
    // // 				   const casacore::Bool sorted=false);

    // // // Accessor for the list of the selected casacore::Data Description IDs from
    // // // the SPW expression parsing.  The actual
    // // // selected DDIDs would be an intersection of the DDIDs selected
    // // // from polarization and SPW expressions parsing (see
    // // // getDDIDList() above).
    // // inline casacore::Vector<casacore::Int> getSPWDDIDList(const casacore::MeasurementSet* ms=NULL) 
    // // {if (spwDDIDs_p.nelements() <= 0) getTEN(ms); return spwDDIDs_p;}

    // // //
    // // // The key in the ordered map returned by getPolMap() is the Data
    // // // Description ID (DDID). The value is a vector containing the
    // // // list of in-row indices to pick out the selected polarizations
    // // // (or equivalently, the list of indices for the vector in the
    // // // corrType column of the POLARIZATION sub-table). These are also
    // // // what the user intended (i.e., e.g. not all DD IDs due to user
    // // // POL expression might be selected due to SPW expressions).
    // // //
    // // inline casacore::OrderedMap<casacore::Int, casacore::Vector<casacore::Int> > getPolMap(const casacore::MeasurementSet* ms=NULL) 
    // // {getTEN(ms); return selectedPolMap_p;};

    // // //
    // // // The key in the ordered map returned by getCorrMap() is the
    // // // pol. is the casacore::Data Description ID (DDID).  The value is a set of
    // // // two vectors.  The first vector is the list of the in-row
    // // // indices to pick out the selected polarizations (or
    // // // equivalently, the list of indices for the vector in the
    // // // corrType column of the POLARIZATION sub-table).
    // // //
    // // inline casacore::OrderedMap<casacore::Int, casacore::Vector<Vector<casacore::Int> > > getCorrMap(const casacore::MeasurementSet* ms=NULL) 
    // // {getTEN(ms); return selectedSetupMap_p;};

    // // // Methods to convert the maps return by getChanList and
    // // // getCorrMap to a list of casacore::Slice which can be directly used by
    // // // casacore::Table system for in-row selection of frequency channels and
    // // // polarizations.
    // // void getChanSlices(casacore::Vector<Vector<casacore::Slice> >& chanslices, 
    // // 		       const casacore::MeasurementSet* ms=NULL, 
    // // 		       const casacore::Int defaultChanStep=1);

    // // void getCorrSlices(casacore::Vector<Vector<casacore::Slice> >& corrslices,
    // // 		       const casacore::MeasurementSet* ms=NULL);
    
    // This version of reset() works with generic MSSeletableTable
    // object.  Accessing the services of the CTSelection module via
    // this interface is recommended over the version of reset() that
    // uses MeasurementSet.
    void reset(casacore::MSSelectableTable& msLike,
	       const MSSMode& mode           = PARSE_NOW,
	       const casacore::String& timeExpr        = "",
	       const casacore::String& antennaExpr     = "",
	       const casacore::String& fieldExpr       = "",
	       const casacore::String& spwExpr         = "",
	       const casacore::String& uvDistExpr      = "",
	       const casacore::String& taqlExpr        = "",
	       const casacore::String& polnExpr        = "",
	       const casacore::String& scanExpr        = "",
	       const casacore::String& arrayExpr       = "",
	       const casacore::String& stateExpr       = "",
	       const casacore::String& observationExpr = "");
    
    // // // Set the error handler to be used for reporting errors while
    // // // parsing the type of expression give by the first argument.
    // // void setErrorHandler(const MSExprType type, casacore::MSSelectionErrorHandler* mssEH,
    // // 			 const casacore::Bool overRide=false);
    
    // // // Initialize the error handler.  This is set the error-handler to
    // // // the user supplied error handler via setErrorHandler() or to the
    // // // default built-in error handler.
    // // void initErrorHandler(const MSExprType tye=NO_EXPR);

    // // // Execute the handleError() method of the error-handlers.  This
    // // // is called in the catch code for any exceptions emitted from any
    // // // of the parsers. It is also called at the end of the
    // // // parsing cycle.
    // // void runErrorHandler();
  };
  
} //# NAMESPACE CASA - END

#endif


