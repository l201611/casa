//# SIMapperImageMosaic.cc: Implementation of SIMapperImageMosaic.h
//# Copyright (C) 1997-2008
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This program is free software; you can redistribute it and/or modify it
//# under the terms of the GNU General Public License as published by the Free
//# Software Foundation; either version 2 of the License, or (at your option)
//# any later version.
//#
//# This program is distributed in the hope that it will be useful, but WITHOUT
//# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//# more details.
//#
//# You should have received a copy of the GNU General Public License along
//# with this program; if not, write to the Free Software Foundation, Inc.,
//# 675 Massachusetts Ave, Cambridge, MA 02139, USA.
//#
//# Correspondence concerning AIPS++ should be addressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
//# $Id$

#include <casa/Exceptions/Error.h>
#include <casa/iostream.h>
#include <casa/sstream.h>

#include <casa/Arrays/Matrix.h>
#include <casa/Arrays/ArrayMath.h>
#include <casa/Arrays/ArrayLogical.h>

#include <casa/Logging.h>
#include <casa/Logging/LogIO.h>
#include <casa/Logging/LogMessage.h>
#include <casa/Logging/LogSink.h>
#include <casa/Logging/LogMessage.h>

#include <casa/OS/DirectoryIterator.h>
#include <casa/OS/File.h>
#include <casa/OS/Path.h>

#include <casa/OS/HostInfo.h>

#include <ms/MeasurementSets/MSHistoryHandler.h>
#include <ms/MeasurementSets/MeasurementSet.h>

#include <synthesis/TransformMachines2/BeamSkyJones.h>
#include <synthesis/TransformMachines2/SkyJones.h>
#include <synthesis/TransformMachines2/SimpleComponentFTMachine.h>
#include <synthesis/TransformMachines/SimpCompGridMachine.h>

#include <synthesis/ImagerObjects2/SIMapperImageMosaic.h>


#include <sys/types.h>
#include <unistd.h>
using namespace std;

using namespace casacore;
namespace casa { //# NAMESPACE CASA - BEGIN
namespace refim {//# namespace refactor imaging
  
using namespace casacore;
using namespace casa;
using namespace casacore;
using namespace casa::refim;
using namespace casacore;
using namespace casa::vi;
  
  SIMapperImageMosaic::SIMapperImageMosaic( CountedPtr<SIImageStore>& imagestore, 
					    CountedPtr<FTMachine>& ftm, 
					    CountedPtr<FTMachine>& iftm)
    //		      CountedPtr<VPSkyJones>& vp)
    : SIMapper( imagestore, ftm, iftm ), ovb_p(NULL)
    //    vb_p (vi::VisBuffer2::factory (vi::VbPlain, vi::VbRekeyable))
  {
    LogIO os( LogOrigin("SIMapperImageMosaic","Constructor",WHERE) );

    /*
    if( !vp.null() ) {
      ejgrid_p=vp;
      ejdegrid_p=vp;
    }
    else {
      ejgrid_p=NULL;
      ejdegrid_p=NULL;
    }
    */

    firstaccess_p = true;

  }
  
  SIMapperImageMosaic::SIMapperImageMosaic(const ComponentList& cl, 
					   String& whichMachine)
    //		     CountedPtr<VPSkyJones>& vp)
    : SIMapper(cl, whichMachine )
    //      vb_p (vi::VisBuffer2::factory (vi::VbPlain, vi::VbRekeyable))
  {

    /*
    if( !vp.null() ) {
      ejgrid_p=vp;
      ejdegrid_p=vp;
    }
    else {
      ejgrid_p=NULL;
      ejdegrid_p=NULL;
    }
    */
    clCorrupted_p=cl;

    firstaccess_p = true;
      
  }
  
  SIMapperImageMosaic::~SIMapperImageMosaic() 
  {
  }
  
  // #############################################
  // #############################################
  // #######  Gridding / De-gridding functions ###########
  // #############################################
  // #############################################


  void SIMapperImageMosaic::initializeGrid(vi::VisBuffer2& vb, Bool dopsf, Bool firstaccess)
  {
    
    LogIO os( LogOrigin("SIMapperImageMosaic","initializeGrid",WHERE) );
    //Componentlist FTM has nothing to do
    if(ift_p.null())
      return;
    
    ift_p->initializeToSkyNew( dopsf, vb, itsImages);
    
    Bool dirDep= ift_p->isSkyJonesSet(); //  (!ejgrid_p.null());
    dirDep= dirDep || ((ift_p->name()) == "MosaicFT");

    //    ovb_p.assign(vb, false);
    //    ovb_p.updateCoordInfo(&vb, dirDep);
    ovb_p = vi::VisBuffer2::factory(vi::VbPlain, vi::VbRekeyable);
    ovb_p->copy(vb,true);
    
    firstaccess_p = firstaccess;
    
  }
  

  /////////////////OLD vi/vb version
  void SIMapperImageMosaic::grid(vi::VisBuffer2& vb, Bool dopsf, FTMachine::Type col)
   {
     LogIO os( LogOrigin("SIMapperImageMosaic","grid",WHERE) );
     //Componentlist FTM has no gridding to do
     if(ift_p.null())
       return;
     
      Int nRow=vb.nRows();
      Bool internalChanges=false;  // Does this VB change inside itself?
      Bool firstOneChanges=false;  // Has this VB changed from the previous one?
      if((ift_p->name() != "MosaicFT")    && (ift_p->name() != "PBWProjectFT") &&
	 (ift_p->name() != "AWProjectFT") && (ift_p->name() != "AWProjectWBFT")) {
	ift_p->changedSkyJonesLogic(vb, firstOneChanges, internalChanges);
      }
      //First ft machine change should be indicative
      //anyways right now we are allowing only 1 ftmachine for GridBoth
      Bool IFTChanged=ift_p->changed(vb);

      // cout << "gridCoreMos : internalChanges : " << internalChanges << "  firstchange : " << firstOneChanges << endl;
      
      if(internalChanges) {
	// Yes there are changes: go row by row.
	for (Int row=0; row<nRow; row++) 
	  {
	    ////	  if(IFTChanged||ejgrid_p->changed(vb,row)) 
	    if(IFTChanged|| ift_p->isSkyJonesChanged(vb,row)) 
	      {
		// Need to apply the SkyJones from the previous row
		// and finish off before starting with this row

		finalizeGrid( *ovb_p, dopsf );
		initializeGrid( vb, dopsf );
		
	      }
	    ift_p->put(vb, row, dopsf, col);
	    //gridCore( vb, dopsf, col, ftm, row );
	  }
      } else if (IFTChanged || firstOneChanges) {
	//IMPORTANT:We need to finalize here by checking that we are not at the begining of the iteration
	if( !firstaccess_p )
	  {
	    finalizeGrid( *ovb_p, dopsf );
	    firstaccess_p=false;
	  }
	initializeGrid( vb, dopsf );
	ift_p->put(vb, -1, dopsf, col);
      } else  {
	ift_p->put(vb, -1, dopsf, col);
      }
     
   }


  //////////////OLD VI/VB version
  void SIMapperImageMosaic::finalizeGrid(vi::VisBuffer2& vb, Bool dopsf)
    {
      LogIO os( LogOrigin("SIMapperImageMosaic","finalizeGrid",WHERE) );
    
      if(ift_p.null())
      	return;

      ift_p->finalizeToSkyNew( dopsf, vb, itsImages );
    }

  //////////////////OLD vi/vb version
  void SIMapperImageMosaic::initializeDegrid(vi::VisBuffer2& vb, const Int /*row*/)
  {
    LogIO os( LogOrigin("SIMapperImageMosaic", "initializeDegrid",WHERE) );
    if(ft_p.null() && cft_p.null())
      return;

    ft_p->initializeToVisNew(vb, itsImages);

    /// Add a call here, to do corrupt cl_p and create clCorrupted_p;
    /// This should happen inside the SimpleComponentMachine, which will be init'd with the SkyJones.
    /// For now, it's just clCorrupted_p = cl_p.

    /*
      if(!cftm.null()) {
	clCorrupted_p=ComponentList();
	for (uInt k=0; k < cl.nelements(); ++k){
	  SkyComponent comp=cl.component(k).copy();
	  if(vb.polFrame()==MSIter::Linear) {
	    if(comp.flux().pol()==ComponentType::STOKES) {
	      comp.flux().convertPol(ComponentType::LINEAR);
	    }
	  }
	  else {
	    if(comp.flux().pol()==ComponentType::STOKES) {
	      comp.flux().convertPol(ComponentType::CIRCULAR);
	    }
	  }
	  ////We might have to deal with the right row here if the visbuffer is has changed internally
	  ejdegrid_p->apply(comp, comp, vb,row, true);
	  clCorrupted_p.add(comp);
	}
      }
    */

  }


  ////////////////////Old vi/Vb version

  void SIMapperImageMosaic::degrid(vi::VisBuffer2& vb)
    {
      LogIO os( LogOrigin("SIMapperImageMosaic","degrid",WHERE) );
      ///This should not be called even but heck let's ignore
      if(ft_p.null() and cft_p.null())
      	return;

      Cube<Complex> origCube;
      origCube.assign(vb.visCubeModel()); 

      Int nRow=vb.nRows();
      Bool internalChanges=false;  // Does this VB change inside itself?
      Bool firstOneChanges=false;  // Has this VB changed from the previous one?
      
      if((!ft_p.null() && (ft_p->name() != "MosaicFT")    && (ft_p->name() != "PBWProjectFT") &&
	  (ft_p->name() != "AWProjectFT") && (ft_p->name() != "AWProjectWBFT")) || (!cft_p.null())) {
	ft_p->changedSkyJonesLogic(vb, firstOneChanges, internalChanges);
      }
      //anyways right now we are allowing only 1 ftmachine for GridBoth
      Bool FTChanged=ft_p->changed(vb);
      
      if(internalChanges)
	{
	  // Yes there are changes within this buffer: go row by row.
	  // This will automatically catch a change in the FTMachine so
	  // we don't have to check for that.
	  for (Int row=0; row<nRow; row++)
	    {
	      ////	      if(FTChanged||ejdegrid_p->changed(vb,row))
	      if(FTChanged||  ft_p->isSkyJonesChanged(vb,row) )
		{
		  // Need to apply the SkyJones from the previous row
		  // and finish off before starting with this row
		  finalizeDegrid();
		  initializeDegrid(vb,row);
		}
	      ft_p.null() ? cft_p->get(vb, clCorrupted_p, row) : ft_p->get(vb, row);
	      
	    }
	  
	}
      else if (FTChanged||firstOneChanges) {
	// This buffer has changed wrt the previous buffer, but
	// this buffer has no changes within it. Again we don't need to
	// check for the FTMachine changing.
	finalizeDegrid();
	initializeDegrid(vb, 0);

	ft_p.null() ? cft_p->get(vb, clCorrupted_p) : ft_p->get(vb);
      }
      else {
	ft_p.null() ? cft_p->get(vb, clCorrupted_p) : ft_p->get(vb);
      }
      
      //      vb.modelVisCube()+=origCube;
      vb.setVisCubeModel( vb.visCubeModel() + origCube );

    }


  /*
  Bool SIMapperImageMosaic::changedSkyJonesLogic(const vi::VisBuffer2& vb, Bool& firstRow, Bool& internalRow, const Bool grid){
      firstRow=false;
      internalRow=false;
      CountedPtr<VPSkyJones> ej= grid ? ejgrid_p : ejdegrid_p;
      if(ej.null())
    	  return false;
	  if(ej->changed(vi::VisBuffer2Adapter(&vb),0))
		  firstRow=true;
	  Int row2temp=0;
	  if(ej->changedBuffer(vi::VisBuffer2Adapter(&vb),0,row2temp)) {
	     internalRow=true;
	   }
	   return (firstRow || internalRow) ;
  }
  ////////////Old VB version
  Bool SIMapperImageMosaic::changedSkyJonesLogic(const VisBuffer& vb, Bool& firstRow, Bool& internalRow, const Bool grid){
        firstRow=false;
        internalRow=false;
        CountedPtr<VPSkyJones> ej= grid ? ejgrid_p : ejdegrid_p;
        if(ej.null())
      	  return false;
  	  if(ej->changed(vb,0))
  		  firstRow=true;
  	  Int row2temp=0;
  	  if(ej->changedBuffer(vb,0,row2temp)) {
  	     internalRow=true;
  	   }
  	   return (firstRow || internalRow) ;
    }
  */

}//# namespace refim
} //# NAMESPACE CASA - END

