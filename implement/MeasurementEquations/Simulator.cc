//# newsimulator.cc: Simulation  program
//# Copyright (C) 1996,1997,1998,1999,2000,2001,2002,2003
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
//# $Id: Simulator.cc,v 1.1.2.4 2006/10/06 21:03:19 kgolap Exp $

#include <stdexcept>
#include <casa/Arrays/Matrix.h>
#include <casa/Arrays/ArrayMath.h>
#include <casa/Exceptions/Error.h>
#include <casa/iostream.h>

#include <casa/Logging.h>
#include <casa/Logging/LogIO.h>
#include <casa/OS/File.h>
#include <casa/Containers/Record.h>

#include <tables/Tables/TableParse.h>
#include <tables/Tables/TableRecord.h>
#include <tables/Tables/TableDesc.h>
#include <tables/Tables/TableLock.h>
#include <tables/Tables/ExprNode.h>

#include <casa/BasicSL/String.h>
#include <casa/Utilities/Assert.h>
#include <casa/Utilities/Fallible.h>

#include <casa/BasicSL/Constants.h>

#include <casa/Logging/LogSink.h>
#include <casa/Logging/LogMessage.h>

#include <casa/Arrays/ArrayMath.h>

#include <msvis/MSVis/VisSet.h>
#include <msvis/MSVis/VisSetUtil.h>
#include <synthesis/MeasurementComponents/VisCal.h>
#include <synthesis/MeasurementComponents/VisCalGlobals.h>
#include <ms/MeasurementSets/NewMSSimulator.h>

#include <measures/Measures/Stokes.h>
#include <casa/Quanta/UnitMap.h>
#include <casa/Quanta/UnitVal.h>
#include <casa/Quanta/MVAngle.h>
#include <measures/Measures/MDirection.h>
#include <measures/Measures/MPosition.h>
#include <casa/Quanta/MVEpoch.h>
#include <measures/Measures/MEpoch.h>

#include <ms/MeasurementSets/MeasurementSet.h>
#include <ms/MeasurementSets/MSColumns.h>

#include <ms/MeasurementSets/MSSummary.h>
#include <synthesis/MeasurementEquations/SkyEquation.h>
#include <synthesis/MeasurementComponents/ImageSkyModel.h>
#include <synthesis/MeasurementComponents/SimACohCalc.h>
#include <synthesis/MeasurementComponents/SimACoh.h>
//#include <synthesis/MeasurementComponents/SimVisJones.h>
#include <synthesis/MeasurementComponents/VPSkyJones.h>
#include <synthesis/MeasurementEquations/StokesImageUtil.h>
#include <lattices/Lattices/LatticeExpr.h> 

#include <synthesis/MeasurementEquations/Simulator.h>
#include <synthesis/MeasurementComponents/CleanImageSkyModel.h>
#include <synthesis/MeasurementComponents/GridBoth.h>
#include <synthesis/MeasurementComponents/WProjectFT.h>
#include <synthesis/MeasurementComponents/GridBoth.h>
#include <synthesis/MeasurementComponents/MosaicFT.h>
#include <synthesis/MeasurementComponents/SimpleComponentFTMachine.h>
#include <casa/OS/HostInfo.h>
#include <images/Images/PagedImage.h>
#include <casa/Arrays/Cube.h>
#include <casa/Arrays/Vector.h>
#include <casa/sstream.h>

#include <casa/namespace.h>

#define RI_DEBUG


Simulator::Simulator(): 
  msname_p(String("")), ms_p(0), mssel_p(0), vs_p(0), 
  seed_p(11111),
  ve_p(),
  vc_p(),
  ac_p(0), vp_p(0), gvp_p(0), 
  sim_p(0),
  // epJ_p(0),
  epJTableName_p()
{
}


Simulator::Simulator(String& msname) 
  : msname_p(msname), ms_p(0), mssel_p(0), vs_p(0), seed_p(11111),
    ve_p(),
    vc_p(),
    ac_p(0), vp_p(0), gvp_p(0), 
    sim_p(0),
    // epJ_p(0),
    epJTableName_p()
{
  LogIO os(LogOrigin("simulator", "simulator(String& msname)", WHERE));

  defaults();

  if(!sim_p) {
    sim_p= new NewMSSimulator(msname);
  }
  
  // Make a MeasurementSet object for the disk-base MeasurementSet that we just
  // created
  ms_p = new MeasurementSet(msname, TableLock(TableLock::AutoNoReadLocking), 
			    Table::Update);
  AlwaysAssert(ms_p, AipsError);

}


Simulator::Simulator(MeasurementSet &theMs)
  : msname_p(""), ms_p(0), mssel_p(0), vs_p(0), seed_p(11111),
    ve_p(),
    vc_p(),
    ac_p(0), vp_p(0), gvp_p(0), 
    sim_p(0),
    // epJ_p(0),
    epJTableName_p()
{
  LogIO os(LogOrigin("simulator", "simulator(MeasurementSet& theMs)", WHERE));

  defaults();

  msname_p=theMs.tableName();

  if(!sim_p) {
    sim_p= new NewMSSimulator(theMs);
  }

  ms_p = new MeasurementSet(theMs);
  AlwaysAssert(ms_p, AipsError);

  // get info from the MS into Simulator:
  if (!getconfig()) os << "Can't find antenna information for loaded MS" << LogIO::WARN;
}


Simulator::Simulator(const Simulator &other)
  : msname_p(""), ms_p(0), vs_p(0), seed_p(11111),
    ve_p(),
    vc_p(),
    ac_p(0), vp_p(0), gvp_p(0),
    sim_p(0),
    // epJ_p(0),
    epJTableName_p()
{
  defaults();
  ms_p = new MeasurementSet(*other.ms_p);
  if(other.mssel_p) {
    mssel_p = new MeasurementSet(*other.mssel_p);
  }
}

Simulator &Simulator::operator=(const Simulator &other)
{
  if (ms_p && this != &other) {
    *ms_p = *(other.ms_p);
  }
  if (mssel_p && this != &other && other.mssel_p) {
    *mssel_p = *(other.mssel_p);
  }
  if (vs_p && this != &other) {
    *vs_p = *(other.vs_p);
  }
  if (ac_p && this != &other) {
    *ac_p = *(other.ac_p);
  }

  // TBD VisEquation/VisCal stuff

  if (vp_p && this != &other) {
    *vp_p = *(other.vp_p);
  }
  if (gvp_p && this != &other) {
    *gvp_p = *(other.gvp_p);
  }
  if (sim_p && this != &other) {
    *sim_p = *(other.sim_p);
  }
  //  if (epJ_p && this != &other) *epJ_p = *(other.epJ_p);
  return *this;
}

Simulator::~Simulator()
{
  if (ms_p) {
    ms_p->relinquishAutoLocks();
    ms_p->unlock();
    delete ms_p;
  }
  ms_p = 0;
  if (mssel_p) {
    mssel_p->relinquishAutoLocks();
    mssel_p->unlock();
    delete mssel_p;
  }
  mssel_p = 0;
  if (vs_p) {
    delete vs_p;
  }
  vs_p = 0;

  // Delete all vis-plane calibration corruption terms
  resetviscal();

  // Delete all im-plane calibration corruption terms
  resetimcal();

  if(sim_p) delete sim_p; sim_p = 0;

  if(sm_p) delete sm_p; sm_p = 0;
  if(ft_p) delete ft_p; ft_p = 0;
  if(cft_p) delete cft_p; cft_p = 0;

}


void Simulator::defaults()
{
  UnitMap::putUser("Pixel", UnitVal(1.0), "Pixel solid angle");
  UnitMap::putUser("Beam", UnitVal(1.0), "Beam solid angle");
  gridfunction_p="SF";
  // Use half the machine memory as cache. The user can override
  // this via the setoptions function().
  cache_p=(HostInfo::memoryTotal()/8)*1024;

  tile_p=16;
  ftmachine_p="gridft";
  padding_p=1.3;
  facets_p=1;
  sm_p = 0;
  ft_p = 0;
  cft_p = 0;
  vp_p = 0;
  gvp_p = 0;
  sim_p = 0;
  images_p = 0;
  nmodels_p = 1;
  // info for configurations
  areStationCoordsSet_p = False;
  telescope_p = "UNSET";
  nmodels_p = 0;

  // info for fields and schedule:
  sourceName_p="UNSET";
  calCode_p="";

  // info for spectral windows
  spWindowName_p="UNSET";
  nChan_p=1;
  startFreq_p=Quantity(50., "GHz");
  freqInc_p=Quantity(0.1, "MHz");
  freqRes_p=Quantity(0.1, "MHz");
  stokesString_p="RR RL LR LL";

  // feeds
  feedMode_p = "perfect R L";
  nFeeds_p = 1;
  feedsHaveBeenSet = False;
  feedsInitialized = False;

  // times
  integrationTime_p = Quantity(10.0, "s");
  useHourAngle_p=True;
  refTime_p = MEpoch(Quantity(0.0, "s"), MEpoch::UTC);
  timesHaveBeenSet_p=False;

  // VP stuff
  doVP_p=False;
  doDefaultVP_p = True;

};


Bool Simulator::close()
{
  LogIO os(LogOrigin("Simulator", "close()", WHERE));
  os << "Closing MeasurementSet and detaching from Simulator"
     << LogIO::POST;

  // Delete all im-plane calibration corruption terms
  resetimcal();
  // Delete all vis-plane calibration corruption terms
  resetviscal();

  ms_p->unlock();
  if(mssel_p) mssel_p->unlock();
  if(vs_p) delete vs_p; vs_p = 0;
  if(mssel_p) delete mssel_p; mssel_p = 0;
  if(ms_p) delete ms_p; ms_p = 0;
  if(sm_p) delete sm_p; sm_p = 0;
  if(ft_p) delete ft_p; ft_p = 0;
  if(cft_p) delete cft_p; cft_p = 0;

  return True;
}

Bool Simulator::resetviscal() {
  LogIO os(LogOrigin("Simulator", "reset()", WHERE));
  try {

    os << "Resetting all visibility corruption components" << LogIO::POST;
    
    // The noise term (for now)
    if(ac_p) delete ac_p; ac_p=0;

    // Delete all VisCals
    for (uInt i=0;i<vc_p.nelements();++i)
      if (vc_p[i]) delete vc_p[i];
    vc_p.resize(0,True);

    // reset the VisEquation (by sending an empty vc_p)
    ve_p.setapply(vc_p);

  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;
    return False;
  } 
  return True;
}


Bool Simulator::resetimcal() {
  LogIO os(LogOrigin("Simulator", "reset()", WHERE));
  try {

    os << "Reset all image-plane corruption components" << LogIO::POST;
    
    if(vp_p) delete vp_p; vp_p=0;
    if(gvp_p) delete gvp_p; gvp_p=0;
    /*
    //    if(epJ_p) delete epJ_p; epJ_p=0;
    */

  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;
    return False;
  } 
  return True;
}


Bool Simulator::reset() {
  LogIO os(LogOrigin("Simulator", "reset()", WHERE));
  try {
    
    // reset vis-plane cal terms
    resetviscal();

    // reset im-plane cal terms
    resetimcal();

  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;
    return False;
  } 
  return True;
}


String Simulator::name() const
{
  if (detached()) {
    return "none";
  }
  return msname_p;
}

String Simulator::state()
{
  ostringstream os;
  os << "Need to write the state() method!" << LogIO::POST;
  if(doVP_p) {
    os << "  Primary beam correction is enabled" << endl;
  }
  return String(os);
}

Bool Simulator::summary()
{
  LogIO os(LogOrigin("Simulator", "summary()", WHERE));
  createSummary(os);
  predictSummary(os);
  corruptSummary(os);

  return True;
}


Bool Simulator::createSummary(LogIO& os) 
{
  Bool configResult = configSummary(os);
  Bool fieldResult = fieldSummary(os);
  Bool windowResult = spWindowSummary(os);
  Bool feedResult = feedSummary(os);

  if (!configResult && !fieldResult && !windowResult && !feedResult) {
    os << "=======================================" << LogIO::POST;
    os << "No create-type information has been set" << LogIO::POST;
    os << "=======================================" << LogIO::POST;
    return False;
  } else {
    // user has set at least ONE, so we report on each
    if (!configResult) {
      os << "No configuration information set yet, but other create-type info HAS been set" << LogIO::POST;
    }
    if (!fieldResult) {
      os << "No field information set yet, but other create-type info HAS been set" << LogIO::POST;
    }
    if (!windowResult) {
      os << "No window information set yet, but other create-type info HAS been set" << LogIO::POST;
    }
    if (!feedResult) {
      os << "No feed information set yet, but other create-type info HAS been set" << LogIO::POST;
      os << "(feeds will default to perfect R-L feeds if not set)" << LogIO::POST;
    }
    os << "======================================================================" << LogIO::POST;
  }
  return True;
}



Bool Simulator::configSummary(LogIO& os)
{
  if ( ! areStationCoordsSet_p ) {
    return False;
  } else {
    os << "----------------------------------------------------------------------" << LogIO::POST;
    os << "Generating (u,v,w) using this configuration: " << LogIO::POST;
    os << "   x     y     z     diam     mount " << LogIO::POST;
    for (uInt i=0; i< x_p.nelements(); i++) {
      os << x_p(i)
	 << "  " << y_p(i)
	 << "  " << z_p(i)
	 << "  " << diam_p(i)
	 << "  " << mount_p(i)
	 << LogIO::POST;
    }
    os << " Coordsystem = " << coordsystem_p << LogIO::POST;
    os << " RefLocation = " << 
      mRefLocation_p.getAngle("deg").getValue("deg") << LogIO::POST;
  }
  return True;

}



Bool Simulator::fieldSummary(LogIO& os)
{
  os << "----------------------------------------------------------------------" << LogIO::POST;
  os << " Field information: " << LogIO::POST;
  os << " Name  direction  calcode" << LogIO::POST; 
  os << sourceName_p 
     << "  " << formatDirection(sourceDirection_p)
     << "  " << calCode_p
     << LogIO::POST;
  return True;
}



Bool Simulator::timeSummary(LogIO& os)
{
  if(integrationTime_p.getValue("s") <= 0.0) {
    return False;
  } else {
    os << "----------------------------------------------------------------------" << LogIO::POST;
    os << " Time information: " << LogIO::POST;
    os << " integration time = " << integrationTime_p.getValue("s") 
       << " s" << LogIO::POST;
    os << " reference time = " << MVTime(refTime_p.get("s").getValue("d")).string()
       << LogIO::POST;
  }
  return True;
}



Bool Simulator::spWindowSummary(LogIO& os)
{
  os << "----------------------------------------------------------------------" << LogIO::POST;
  os << " Spectral Windows information: " << LogIO::POST;
  os << " Name  nchan  freq[GHz]  freqInc[MHz]  freqRes[MHz]  stokes" << LogIO::POST;
  os << spWindowName_p 	 
     << "  " << nChan_p
     << "  " << startFreq_p.getValue("GHz")
     << "  " << freqInc_p.getValue("MHz")
     << "  " << freqRes_p.getValue("MHz")
     << "  " << stokesString_p
     << LogIO::POST;
  return True;
}


Bool Simulator::feedSummary(LogIO& os)
{
  if (!feedsHaveBeenSet) {
    return False;
  } else {
    os << "----------------------------------------------------------------------" << LogIO::POST;
    os << " Feed information: " << LogIO::POST;
    os << feedMode_p << LogIO::POST;
  }
  return True;
}


Bool Simulator::predictSummary(LogIO& os)
{
  Bool vpResult = vpSummary(os);
  Bool optionsResult = optionsSummary(os);

  // keep compiler happy
  if (!vpResult && !optionsResult) {}
  return True;
}


Bool Simulator::vpSummary(LogIO& os)
{
  if (vp_p) {
    vp_p->summary();
    return True;
  } else {
    return False;
  }
}


Bool Simulator::optionsSummary(LogIO& os)
{
  return True;
}
 

Bool Simulator::corruptSummary(LogIO& os)
{
  if (vc_p.nelements()<1 && !ac_p) {
    os << "===========================================" << LogIO::POST;
    os << "No corrupting-type information has been set" << LogIO::POST;
    os << "===========================================" << LogIO::POST;
    return False;
  }
  else {
    os << "Visibilities will be CORRUPTED with the following terms:" << LogIO::POST;

    Int napp(vc_p.nelements());
    for (Int iapp=0;iapp<napp;++iapp)
      os << LogIO::NORMAL << ".   "
	 << vc_p[iapp]->siminfo()
	 << LogIO::POST;
    
    // Report also on the noise settings
    noiseSummary(os);  

  }
  return True;
}


Bool Simulator::noiseSummary(LogIO& os)
{
  if (!ac_p) {
   return False;
  } else {
    os << "Thermal noise corruption activated" << LogIO::POST;
    os << "Thermal noise mode: " << noisemode_p << LogIO::POST;
  }
  return True;
}













//========================================================================
//       SETUP OBSERVATION


Bool Simulator::settimes(const Quantity& integrationTime, 
			 const Bool      useHourAngle,
			 const MEpoch&   refTime)
{
  
  LogIO os(LogOrigin("simulator", "settimes()", WHERE));
  try {
    
    integrationTime_p=integrationTime;
    useHourAngle_p=useHourAngle;
    refTime_p=refTime;

    os << "Times " << endl
       <<  "     Integration time " << integrationTime.getValue("s") << "s" << LogIO::POST;
    if(useHourAngle) {
      os << "     Times will be interpreted as hour angles for first source" << LogIO::POST;
    }

    sim_p->settimes(integrationTime, useHourAngle, refTime);
    
    timesHaveBeenSet_p=True;
    
    return True;
  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;
    return False;
  } 
  return True;
  
}



Bool Simulator::setseed(const Int seed) {
  seed_p = seed;
  return True;
}



Bool Simulator::setconfig(const String& telname,
			     const Vector<Double>& x, 
			     const Vector<Double>& y, 
			     const Vector<Double>& z,
			     const Vector<Double>& dishDiameter,
			     const Vector<Double>& offset,
			     const Vector<String>& mount,
			     const Vector<String>& antName,
			     const String& coordsystem,
			     const MPosition& mRefLocation) 
{

  telescope_p = telname;
  x_p.resize(x.nelements());
  x_p = x;
  y_p.resize(y.nelements());
  y_p = y;
  z_p.resize(z.nelements());
  z_p = z;
  diam_p.resize(dishDiameter.nelements());
  diam_p = dishDiameter;
  offset_p.resize(offset.nelements());
  offset_p = offset;
  mount_p.resize(mount.nelements());
  mount_p = mount;
  antName_p.resize(antName.nelements());
  antName_p = antName;
  coordsystem_p = coordsystem;
  mRefLocation_p = mRefLocation;

  uInt nn = x_p.nelements();

  if (diam_p.nelements() == 1) {
    diam_p.resize(nn);
    diam_p.set(dishDiameter(0));
  }
  if (mount_p.nelements() == 1) {
    mount_p.resize(nn);
    mount_p.set(mount(0));
  }
  if (mount_p.nelements() == 0) {
    mount_p.resize(nn);
    mount_p.set("alt-az");
  }
  if (offset_p.nelements() == 1) {
    offset_p.resize(nn);
    offset_p.set(offset(0));
  }
  if (offset_p.nelements() == 0) {
    offset_p.resize(nn);
    offset_p.set(0.0);
  }
  if (antName_p.nelements() == 1) {
    antName_p.resize(nn);
    antName_p.set(antName(0));
  }
  if (antName_p.nelements() == 0) {
    antName_p.resize(nn);
    antName_p.set("UNKNOWN");
  }

  AlwaysAssert( (nn == y_p.nelements())  , AipsError);
  AlwaysAssert( (nn == z_p.nelements())  , AipsError);
  AlwaysAssert( (nn == diam_p.nelements())  , AipsError);
  AlwaysAssert( (nn == offset_p.nelements())  , AipsError);
  AlwaysAssert( (nn == mount_p.nelements())  , AipsError);

  areStationCoordsSet_p = True;
  
  sim_p->initAnt(telescope_p, x_p, y_p, z_p, diam_p, offset_p, mount_p, antName_p, 
		 coordsystem_p, mRefLocation_p);
  
  return True;  
}



Bool Simulator::getconfig() {
  // get it from NewMSSimulator
  Matrix<Double> xyz_p;
  Int nAnt;
  if (sim_p->getAnt(telescope_p, nAnt, &xyz_p, diam_p, offset_p, mount_p, antName_p, 
		    coordsystem_p, mRefLocation_p)) {
    x_p.resize(nAnt);
    y_p.resize(nAnt);
    z_p.resize(nAnt);
    for (Int i=0;i<nAnt;i++) {
      x_p(i)=xyz_p(0,i);
      y_p(i)=xyz_p(1,i);
      z_p(i)=xyz_p(2,i);
    }
    areStationCoordsSet_p = True;
    return True;
  } else {
    return False;
  }
}



Bool Simulator::setfield(const String& sourceName,           
			 const MDirection& sourceDirection,  
			 const String& calCode,
			 const Quantity& distance)
{
  LogIO os(LogOrigin("Simulator", "setfield()", WHERE));

  try {
    if (sourceName == "") {
      os << LogIO::SEVERE << "must provide a source name" << LogIO::POST;  
      return False;
    }

    distance_p=distance;
    sourceName_p=sourceName;
    sourceDirection_p=sourceDirection;
    calCode_p=calCode;

    sim_p->initFields(sourceName, sourceDirection, calCode);

  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;
    return False;
  } 
  return True;
};




Bool Simulator::setmosaicfield(const String& sourcename, const String& calcode, const MDirection& fieldcenter, const Int xmosp, const Int ymosp, const Quantity& mosspacing, const Quantity& distance)
{

        Int nx = xmosp;
        Int ny = ymosp;          
        Double roffset = mosspacing.getValue("rad");
        Double newraval, newdecval;
        uInt k=0;
        for (Int i=0; i< nx; ++i) {
          for(Int j=0; j < ny; ++j) {
            if((nx/2)!=floor(nx/2)) { // odd number of fields in x direction(ra)
              newraval = fieldcenter.getAngle().getValue("rad")(0) + 
		(i-ceil(nx/2.0))*roffset/cos(fieldcenter.getAngle().getValue("rad")(1));
            }
            else { //even case
              newraval = fieldcenter.getAngle().getValue("rad")(0) + 
		((i-ceil(nx/2)) - 0.5)*roffset/cos(fieldcenter.getAngle().getValue("rad")(1));
            }
            if((ny/2)!=floor(ny/2)) {
              newdecval = fieldcenter.getAngle().getValue("rad")(1) + 
		(j-ceil(ny/2.0))*roffset;
            }
            else {
              newdecval = fieldcenter.getAngle().getValue("rad")(1) + 
		((j-ceil(ny/2.0)) - 0.5)*roffset;
            }
            if(newraval >2.0*C::pi) {
              newraval = newraval - 2.0*C::pi;
            }
	    Int sign;
            if(abs(newdecval) >C::pi/2) {
              if(newdecval<0) {
                 sign = -1;
              }
              else {
		sign = 1;
              }
              newdecval =  sign*(C::pi - abs(newdecval));
              newraval = abs(C::pi - newraval); 
            } 
            Quantity newdirra(newraval, "rad");
            Quantity newdirdec(newdecval, "rad");
            MDirection newdir(newdirra, newdirdec);
	    newdir.setRefString(fieldcenter.getRefString());
	    ostringstream oos;
	    oos << sourcename << "_" << k ;
  

           setfield(String(oos), newdir, calcode, distance); 
    
            ++k;
          }
	}


	return True;
}



Bool Simulator::setspwindow(const String& spwName,           
			    const Quantity& freq,
			    const Quantity& deltafreq,
			    const Quantity& freqresolution,
			    const Int nChan,
			    const String& stokes) 

{
  LogIO os(LogOrigin("Simulator", "setspwindow()", WHERE));

  try {
    if (nChan == 0) {
      os << LogIO::SEVERE << "must provide nchannels" << LogIO::POST;  
      return False;
    }

    spWindowName_p = spwName;   
    nChan_p = nChan;          
    startFreq_p = freq;      
    freqInc_p = deltafreq;        
    freqRes_p = freqresolution;        
    stokesString_p = stokes;   

    sim_p->initSpWindows(spWindowName_p, nChan_p, startFreq_p, freqInc_p, 
			 freqRes_p, stokesString_p);

  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;
    return False;
  } 
  return True;
};


Bool Simulator::setfeed(const String& mode,
			   const Vector<Double>& x,
			   const Vector<Double>& y,
			   const Vector<String>& pol)
{
  LogIO os(LogOrigin("Simulator", "setfeed()", WHERE));
  
  if (mode != "perfect R L" && mode != "perfect X Y" && mode != "list") {
    os << LogIO::SEVERE << 
      "Currently, only perfect R L or perfect X Y feeds or list are recognized" 
       << LogIO::POST;
    return False;
  }
  sim_p->initFeeds(feedMode_p, x, y, pol);

  feedMode_p = mode;
  nFeeds_p = x.nelements();
  feedsHaveBeenSet = True;

  return True;
};




Bool Simulator::setvp(const Bool dovp,
			 const Bool doDefaultVPs,
			 const String& vpTable,
			 const Bool doSquint,
			 const Quantity &parAngleInc,
			 const Quantity &skyPosThreshold,
			 const Float &pbLimit)
{
  LogIO os(LogOrigin("Simulatore", "setvp()", WHERE));
  
  os << "Setting voltage pattern parameters" << LogIO::POST;
  
  doVP_p=dovp;
  doDefaultVP_p = doDefaultVPs;
  vpTableStr_p = vpTable;
  if (doSquint) {
    squintType_p = BeamSquint::GOFIGURE;
  } else {
    squintType_p = BeamSquint::NONE;
  }
  parAngleInc_p = parAngleInc;
  skyPosThreshold_p = skyPosThreshold;

  if (doDefaultVP_p) {
    os << "Using system default voltage patterns for each telescope"  << LogIO::POST;
  } else {
    os << "Using user defined voltage patterns in Table "<<  vpTableStr_p 
       << LogIO::POST;
  }
  if (doSquint) {
    os << "Beam Squint will be included in the VP model" <<  LogIO::POST;
    os << "and the parallactic angle increment is " 
       << parAngleInc_p.getValue("deg") << " degrees"  << LogIO::POST;
  }
  pbLimit_p = pbLimit;
  return True;
};













//========================================================================
//       Create corruption terms - top level specialized routines


// NEW NOISE WITH ANoise

Bool Simulator::setnoise2(const String& mode, 
			 const Quantity& simplenoise,
			 const String& caltable,
			 const Float antefficiency=0.80,
			 const Float correfficiency=0.85,
			 const Float spillefficiency=0.85,
			 const Float tau=0.0,
			 const Float trx=150.0, 
			 const Float tatmos=250.0, 
			 const Float tground=270.0, 
			 const Float tcmb=2.7) {
  
  LogIO os(LogOrigin("Simulator", "setnoise2()", WHERE));
#ifndef RI_DEBUG
  try {
#endif
    
    os << "In Simulator::setnoise2() " << endl;
    noisemode_p = mode;

    RecordDesc simparDesc;
    simparDesc.addField ("type", TpString);
    simparDesc.addField ("caltable", TpString);
    simparDesc.addField ("mode", TpString);
    simparDesc.addField ("amplitude", TpFloat);  // for constant scale
    simparDesc.addField ("scale", TpFloat);  // for fractional fluctuations

    simparDesc.addField ("antefficiency"  ,TpFloat);
    simparDesc.addField ("correfficiency" ,TpFloat);
    simparDesc.addField ("spillefficiency",TpFloat);
    simparDesc.addField ("tau"		  ,TpFloat);
    simparDesc.addField ("trx"		  ,TpFloat);
    simparDesc.addField ("tatmos"	  ,TpFloat);
    simparDesc.addField ("tground"	  ,TpFloat);
    simparDesc.addField ("tcmb"           ,TpFloat);

    String caltbl(caltable);
    if (String(caltbl).matches (Regex(Regex::fromPattern("cal$")))) {      
      caltbl.resize(caltable.length()-3);
    }
    
    Record simpar(simparDesc);
    simpar.define ("type", "A Noise");
    simpar.define ("caltable", caltbl+".A.cal");
    simpar.define ("mode", mode);

    if (mode=="simple") {
      os << "Using simple noise model with noise level of " << simplenoise.getValue("Jy")
	 << " Jy" << LogIO::POST;
      simpar.define ("amplitude", Float(simplenoise.getValue("Jy")) );
      simpar.define ("mode", "simple");

    } else if (mode=="calculate") {
      os << "adding noise with unity amplitude" << LogIO::POST;
      // do be scaled in a minute by a Tsys-derived M below
      simpar.define ("amplitude", Float(1.0) );
      simpar.define ("mode", "calc");

    } else {
      throw(AipsError("unsupported mode "+mode+" in setnoise2()"));
    }
        
    // create the A
    SolvableVisCal *svc = create_corrupt(simpar);
    
    // set combination parameters in a way that make sense
    svc->combine()="";
    
    // calculate the A
    calc_corrupt(svc,simpar);
    
    if (mode=="calculate") {

      simpar.define ("antefficiency"  ,antefficiency  ); 
      simpar.define ("correfficiency" ,correfficiency );
      simpar.define ("spillefficiency",spillefficiency);
      simpar.define ("tau"	      ,tau	      );
      simpar.define ("trx"	      ,trx	      );
      simpar.define ("tatmos"	      ,tatmos	      );
      simpar.define ("tground"	      ,tground	      );
      simpar.define ("tcmb"           ,tcmb           );


      // simpar.define ("amplitude", tsys );
      simpar.define ("type", "M");
      simpar.define ("caltable", caltbl+".M.cal");
      simpar.define ("mode", "tsys");  
      
      SolvableVisCal *svc = create_corrupt(simpar);
      
      // set combination parameters in a way that make sense
      svc->combine()="SCAN,FIELD,SPW";
      svc->combine()="";
      
      // calculate the M 
      calc_corrupt(svc,simpar);

      // RI TODO Sim::setnoise2: change M,Topac to use an anoisecorruptor?
      // tau from ATM with user unput pwv or user can input a freq-indep tau
      // need mode="calculate-atm" and calculate-tau ?

      // RI TODO Sim::setnoise2 create a Topac based in tau, or need a TOpacf that knows to use ATM and pwv
     
    } 

    return True;
#ifndef RI_DEBUG
  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;
    return False;
  } 
  return True;
#endif
}





Bool Simulator::setgain(const String& mode, 
			const String& caltable,
			const Float timescale,
			const Float rms) {
  
  LogIO os(LogOrigin("Simulator", "setgain()", WHERE));

#ifndef RI_DEBUG
  try {
#endif
        
    if(mode=="table") {      
      os << LogIO::SEVERE << "Cannot yet read from table" << LogIO::POST;
      return False;
    }
    else {
      // RI TODO Sim::setgain add mode=simple
      if(mode=="calculate") {

	// set record format for calibration table simulation information
	RecordDesc simparDesc;
	simparDesc.addField ("type", TpString);
	simparDesc.addField ("caltable", TpString);
	simparDesc.addField ("timescale", TpFloat);
	simparDesc.addField ("amplitude", TpFloat);
    
	// Create record with the requisite field values
	Record simpar(simparDesc);
	simpar.define ("type", "G JONES");
	simpar.define ("timescale", timescale);
	simpar.define ("amplitude", rms);
	simpar.define ("caltable", caltable);
	
	// create the VC and set basic stuff
	SolvableVisCal *svc = create_corrupt(simpar);
	
	// set combination parameters in a way that make sense
	svc->combine()="SCAN,FIELD,SPW";
	svc->combine()="";
	
	// actually calculate the corruptions
	calc_corrupt(svc,simpar);
	
      } else {
	throw(AipsError("unsupported mode "+mode+" in setgain()"));
      }
    }
    return True;
#ifndef RI_DEBUG
  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;
    return False;
  } 
  return True;
#endif
}







Bool Simulator::settrop(const String& mode, 
			const String& caltable,   // output
			const Float pwv,
			const Float deltapwv,
			const Float beta,
			const Float windspeed) {
  
  LogIO os(LogOrigin("Simulator", "settrop()", WHERE));

#ifndef RI_DEBUG
  try {
#endif
        
    if (mode=="test"||mode=="individual"||mode=="screen") {
      
      // set record format for calibration table simulation information
      RecordDesc simparDesc;
      simparDesc.addField ("type", TpString);
      simparDesc.addField ("caltable", TpString);
      simparDesc.addField ("mean_pwv", TpFloat);
      simparDesc.addField ("mode", TpString);
      simparDesc.addField ("delta_pwv", TpFloat);
      simparDesc.addField ("beta", TpFloat);
      simparDesc.addField ("windspeed", TpFloat);
            
      // create record with the requisite field values
      Record simpar(simparDesc);
      simpar.define ("type", "TF");
      simpar.define ("caltable", caltable);
      simpar.define ("mean_pwv", pwv);
      simpar.define ("mode", mode);
      simpar.define ("delta_pwv", deltapwv);
      simpar.define ("beta", beta);
      simpar.define ("windspeed", deltapwv);

      // RI TODO Sim:settrop check timescale and set to smaller if required

      // create the VC and set basic stuff
      SolvableVisCal *svc = create_corrupt(simpar);
      
      // set combination parameters in a way that make sense
      svc->combine()="SCAN,FIELD,SPW";
      svc->combine()="";
    
      // actually calculate the corruptions
      calc_corrupt(svc,simpar);

    } else {
      throw(AipsError("unsupported mode "+mode+" in settrop()"));
    }

    return True;
#ifndef RI_DEBUG
  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;
    return False;
  } 
  return True;
#endif
}






Bool Simulator::setleakage(const String& mode, const String& table,
			      const Quantity& interval, const Double amplitude) 
{
  
  LogIO os(LogOrigin("Simulator", "setleakage()", WHERE));
  
#ifndef RI_DEBUG
  try {
#endif

    // set record format for calibration table simulation information
    RecordDesc simparDesc;
    simparDesc.addField ("type", TpString);
    simparDesc.addField ("caltable", TpString);
    simparDesc.addField ("amplitude", TpFloat);
            
    // create record with the requisite field values
    Record simpar(simparDesc);
    simpar.define ("type", "D");
    simpar.define ("caltable", table);
    simpar.define ("amplitude", amplitude);

    // create the VC and set basic stuff
    SolvableVisCal *svc = create_corrupt(simpar);
    
    // set combination parameters in a way that make sense
    svc->combine()="SCAN,FIELD,SPW";
    svc->combine()="";
    
    // actually calculate the corruptions
    calc_corrupt(svc,simpar);

    return True;
#ifndef RI_DEBUG
  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;
    return False;
  } 
  return True;
#endif
}



















//========================================================================
//        OLD as yet unconverted corruption generation routines

// OLD NOISE WITH ACoh

Bool Simulator::setnoise(const String& mode, 
			 const Quantity& simplenoise,
			 const String& table,
			 const Float antefficiency=0.80,
			 const Float correfficiency=0.85,
			 const Float spillefficiency=0.85,
			 const Float tau=0.0,
			 const Float trx=50.0, 
			 const Float tatmos=250.0, 
			 const Float tcmb=2.7) {
                         // const Quantity& trx=50.0, 
                         // const Quantity& tatmos=250.0, 
                         // const Quantity& tcmb=2.7) {
  
  LogIO os(LogOrigin("Simulator", "setnoise()", WHERE));
  try {
    
    os << "In Simulator::setnoise() " << endl;
    noisemode_p = mode;

    if(mode=="table") {
      os << LogIO::SEVERE << "Cannot yet read from table" << LogIO::POST;
      return False;
    }
    else if (mode=="simplenoise") {
      os << "Using simple noise model with noise level of " << simplenoise.getValue("Jy")
	 << " Jy" << LogIO::POST;
	if(ac_p) delete ac_p; ac_p = 0;
	ac_p = new SimACoh(seed_p, simplenoise.getValue("Jy") );
    }
    else {
      os << "Using the Brown calculated noise model" << LogIO::POST;
      os << "  eta_ant=" << antefficiency << " eta_corr=" << correfficiency << " eta_spill=" << spillefficiency << LogIO::POST;
      os << "  tau=" << tau << " trx=" << trx << " tatmos=" << tatmos << " tcmb=" << tcmb << LogIO::POST;
	if(ac_p) delete ac_p; ac_p = 0;
	ac_p = new SimACohCalc(seed_p, antefficiency, correfficiency,
			       spillefficiency, tau, Quantity(trx, "K"), 
			       Quantity(tatmos, "K"), Quantity(tcmb, "K"));
    }

    return True;
  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;
    return False;
  } 
  return True;
  
}





Bool Simulator::setpa(const String& mode, const String& table,
		      const Quantity& interval) {
  
  LogIO os(LogOrigin("Simulator", "setpa()", WHERE));
  
  try {
    
    throw(AipsError("Corruption by simulated errors temporarily disabled (06Nov20 gmoellen)"));
 /*
    if(mode=="table") {
      os << LogIO::SEVERE << "Cannot yet read from table" << LogIO::POST;
      return False;
    }
    else {
      makeVisSet();
      if(pj_p) delete pj_p; pj_p = 0;
      pj_p = new PJones (*vs_p, interval.get("s").getValue());
      os <<"Using parallactic angle correction"<< LogIO::POST;
    }
 */
    return True;
  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;
    return False;
  } 
  return True;
};


Bool Simulator::setbandpass(const String& mode, const String& table,
			    const Quantity& interval,
			    const Vector<Double>& amplitude) {
  
  LogIO os(LogOrigin("Simulator", "setbandpass()", WHERE));
  
  try {

    throw(AipsError("Corruption by simulated errors temporarily disabled (06Nov20 gmoellen)"));

    /*    
    if(mode=="table") {
      os << LogIO::SEVERE << "Cannot yet read from table" << LogIO::POST;
      return False;
    }
    else {
      os << LogIO::SEVERE << "Cannot yet calculate bandpass" << LogIO::POST;
      return False;
    }
    return True;
    */
  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;

    return False;
  } 
  return True;
}




Bool Simulator::setpointingerror(const String& epJTableName,
				    const Bool applyPointingOffsets,
				    const Bool doPBCorrection)
{
  LogIO os(LogOrigin("Simulator", "close()", WHERE));
  epJTableName_p = epJTableName;
  //  makeVisSet();
  try {
    throw(AipsError("Corruption by simulated errors temporarily disabled (06Nov20 gmoellen)"));
    /*    
    if (epJ_p) delete epJ_p;epJ_p=0;
    {
      epJ_p = new EPJones(*vs_p);
      epJ_p->load(epJTableName_p,"","diagonal");
    }
    */
  }
  catch (AipsError x)
    {
      os << LogIO::SEVERE << "Caught exception: "
	 << x.getMesg() << LogIO::POST;
      return False;
    }

  applyPointingOffsets_p = applyPointingOffsets;
  doPBCorrection_p = doPBCorrection;
  return True;
}











//========================================================================
//       CORRUPTION - GENERIC VISCAL FUNCTIONS



SolvableVisCal *Simulator::create_corrupt(const Record& simpar)
{
  LogIO os(LogOrigin("Simulator", "create_corrupt()", WHERE));

  SolvableVisCal *svc(NULL);

  // RI TODO sim::create_corrupt assert that ms has certain structure - 
  // either has been predict()ed or sm opened from a real ms.  
    
  try {

    // if VisSet goes away we'll have to change this.
    makeVisSet();
    
    String upType=simpar.asString("type");
    upType.upcase();
    
    os << LogIO::NORMAL
       << "Arranging to CORRUPT with:"
       << LogIO::POST;
    
    svc = createSolvableVisCal(upType,*vs_p);

    // debugging info
    svc->setPrtlev(3);

    // Generic VisCal setSimulate will throw an exception -- 
    //   each VC needs to have its own.
    // specializations should call SolvableVisCal::setSimulate though
    svc->setSimulate(simpar);
    // note that when setSimulate creates the CalSet it doesn't know 
    // nChan(Spw()) yet so the calSet needs to be inflated for each 
    // Spw, using 
    // void SolvableVisCal::inflate(const Vector<Int>& nChan,
    // const Vector<Int>& startChan,
    // const Vector<Int>& nSlot) {

    // 2009 oct - setsimulate doesn't make a calset anymore - that's done in 
    // calc_corrupt

    // makes a calset, but spwOK starts out F for the shape-based constructor
    // used here and in the solve context, in contrast to the caltable-based
    // constructor used in setapply
    // do we need VC->setsimulate to do VC.spwOK=T like VC::setapply? 
    // no - we'll set that in calc_corrupt
    
    os << LogIO::NORMAL << ".   "
       << svc->siminfo()
       << LogIO::POST;

    uInt napp=vc_p.nelements();
    vc_p.resize(napp+1,False,True);
    vc_p[napp] = (VisCal*) svc;
    // svc=NULL;

    // Maintain apply/corrupt list and sort the vc_p list
    ve_p.setapply(vc_p);

    return svc;
    
  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg()
       << LogIO::POST;
    if (svc) delete svc;
    throw(AipsError("Error in Simulator::createcorrupt"));
    return NULL;
  }
  // return NULL;
}







Bool Simulator::calc_corrupt(SolvableVisCal *svc, const Record& simpar)
{
  LogIO os(LogOrigin("Simulator", "calc_corrupt()", WHERE));

#ifndef RI_DEBUG
    try {
#endif

    AlwaysAssert((svc->isSimulated()),AipsError);

    // we could make this work without a VisSet, but if someone wants 
    // to corrupt an existing MS, this is the easiest way to get the 
    // shape of the MS, and if we're making a new MS, this will also work.
    makeVisSet();
    AlwaysAssert(vs_p, AipsError);

    // adapted from Calibrater::standardSolve3()
    // but here we need to setup the entire observation at once 
    // in each VC's simcorrupter, so that it can ensure continuity
    // across time in a physics-specific way.   

    // we made sure this was in seconds above
    Double &interval = svc->interval();  
    
    // enforce that the caltable not be on any finer cadence 
    // than the actual data / integration time
    // RI TODO relax min sim_interval ?     
    if (interval < integrationTime_p.getValue("s"))
      interval = integrationTime_p.getValue("s");
        
    // sizeUpSim also deals with nCorr for us
    // sizeUpSim does inflate the CalSet - did we make one?
    // sizeUpSim does setSolveChannelization() and initSolvePar();
    // initSolvePar does solveCPar().resize(nPar(),1,nAnt());
    
    Vector<Int> nChunkPerSim;
    Vector<Double> solTimes;
    Int nSim = svc->setupSim(*vs_p,simpar,nChunkPerSim,solTimes);

    // setupSim (sizeUpSim) is supposed to be inflating the calset
    // after running setSolveChannelization, so why is nChanPar
    // not correct in the cs?
    
    // setupSim might be a good place to set the VI sort order and 
    // reset the VI (in which case it needs a pointer to the VisSet)
    
    if (!(svc->corruptor_p))
      throw(AipsError("Error in Simulator::calc_corrupt: corruptor doesn't exist!"));
    
    // GM sez: organize calibration correction/corruption according to 
    // multi-spw consistency; e.g. move time ahead of data_desc_id so that 
    // data_desc_id (spw) changes faster than time, even within scans.
    
    // Arrange for iteration over data
    Block<Int> columns;
    // include scan iteration
    columns.resize(5);
    columns[0]=MS::ARRAY_ID;
    columns[1]=MS::SCAN_NUMBER;
    columns[2]=MS::FIELD_ID;
    columns[3]=MS::DATA_DESC_ID;
    columns[4]=MS::TIME;
    vs_p->resetVisIter(columns,0.0);
    VisIter& vi(vs_p->iter());
    //    VisBuffer vb(vi);
    
    Int nSpw=vs_p->numberSpw();
    // same as cs_p->nSpw() ?    
    //AlwaysAssert(nSpw == svc->cs().nSpw(), AipsError);

    Vector<Int> slotidx(nSpw,-1);
    
    Int nGood(0);
    vi.originChunks();
    Double t0(0.);

    // debug:
    cout << "nChunkPerSim = ";
    cout << 0 << " | " << nChunkPerSim[0] << " ; " << nSim-1 << " | " << nChunkPerSim[nSim-1] << endl;

    for (Int isim=0;isim<nSim && vi.moreChunks();++isim) {      
      Int thisSpw=svc->spwMap()(vi.spectralWindow());
      slotidx(thisSpw)++;
      
      // I'd really like to do all channels at once but George has 
      // a lot of stuff coded for a SVJ that requires one channel 
      // at a time - we'd need to change SolvableVisCal SVJ:initSolvePar()
      for (Int ich=((const SolvableVisCal*)svc)->nChanPar()-1;ich>-1;--ich) {
	svc->focusChan()=ich;

// need to inflate calset to right shape (is this a function of spw - probably)
// since it was created without Chan info:
// new CalSet<Complex>(nSpw(),nPar(),Vector<Int>(1,1),nElem(),Vector<Int>(1,1));

//			     const Vector<Int>& startChan,
//			     const Vector<Int>& nSlot) {
// 	cs().resize(nPar(),nChan,nElem(),nSlot);
//
// this call assumes all channels want to be simulated:      
// we should be able to do this for all spw together, right?  
// but that requires figuring out the spwmap and nchan per spw - 
// does the VisSet know this already or is there some way to do it?
// George probably knows....
// also, do we want to inflate completely to nSim, or just to current slotidx?
// the cast to SVC is annoying but nChanPar() is private for some reason...
//      svc->inflate(Vector<Int>(1,((const SolvableVisCal*)svc)->nChanPar()),
//		   Vector<Int>(1,0), 
//		   Vector<Int>(1,slotidx(thisSpw)));
      
	if (!svc->simPar(vi,nChunkPerSim[isim])) 
	  throw(AipsError("Error calculating simulated VC")); 
	svc->keep(slotidx(thisSpw));
      }
    }
    
    // this doesn't seem to be getting all spws... is nSpw() set? 
    svc->setSpwOK();
    cout << "Sim::calc_corr vc.SpwOK = " << svc->spwOK() << endl;
    // calls these protected methods:   
    //svc->cs().setSpwOK();  // checks if nTime()!=0;  hopefully nTime is getting set
    //svc->ci().setSpwOK();  // gets that from cs()
    

    if (svc->calTableName()!="<none>") {      
      // RI TODO Sim::create_corrupt check if user wants to overwrite calTable
      os << LogIO::NORMAL 
	 << "Writing calTable = "+svc->calTableName()+" ("+svc->typeName()+")" 
	 << endl << LogIO::POST;      
      // write the table
      // append()=False set by setSimulate()
      svc->store();
    } else {
      os << LogIO::NORMAL 
	 << "calTable name not set - not writing to disk." 
	 << endl << LogIO::POST;
    }

    return True;
#ifndef RI_DEBUG    
  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg()
       << LogIO::POST;
    if (svc) delete svc;
    throw(AipsError("Error in Simulator::calc_corrupt"));
    return False;
  }
  return False;
#endif
}








//========================================================================
//       currupt and setapply, for actually changing visibilities 


/// this can be used to load any table, just that it has to have the right form

Bool Simulator::setapply(const String& type,
			 const Double& t,
			 const String& table,
			 const String& spw,
			 const String& field,
			 const String& interp,
			 const Bool& calwt,
			 const Vector<Int>& spwmap,
			 const Float& opacity)
{
  LogIO os(LogOrigin("Simulator", "setapply()", WHERE));

  // First try to create the requested VisCal object
  VisCal *vc(NULL);
    
  try {

    makeVisSet();

    // Set record format for calibration table application information
    RecordDesc applyparDesc;
    applyparDesc.addField ("t", TpDouble);
    applyparDesc.addField ("table", TpString);
    applyparDesc.addField ("interp", TpString);
    applyparDesc.addField ("spw", TpArrayInt);
    applyparDesc.addField ("field", TpArrayInt);
    applyparDesc.addField ("calwt",TpBool);
    applyparDesc.addField ("spwmap",TpArrayInt);
    applyparDesc.addField ("opacity",TpFloat);
    
    // Create record with the requisite field values
    Record applypar(applyparDesc);
    applypar.define ("t", t);
    applypar.define ("table", table);
    applypar.define ("interp", interp);
    applypar.define ("spw",Vector<Int>());
    applypar.define ("field",Vector<Int>());
    //    applypar.define ("spw",getSpwIdx(spw));
    //    applypar.define ("field",getFieldIdx(field));
    applypar.define ("calwt",calwt);
    applypar.define ("spwmap",spwmap);
    applypar.define ("opacity", opacity);
    
    String upType=type;
    if (upType=="")
      // Get type from table
      upType = calTableType(table);

    // Must be upper case
    upType.upcase();
    
    os << LogIO::NORMAL
       << "Arranging to CORRUPT with:"
       << LogIO::POST;
    
    // Add a new VisCal to the apply list
    vc = createVisCal(upType,*vs_p);
    
    vc->setApply(applypar);

    os << LogIO::NORMAL << ".   "
       << vc->applyinfo()
       << LogIO::POST;
    
  } catch (AipsError x) {
    os << LogIO::SEVERE << x.getMesg()
       << " Check inputs and try again."
       << LogIO::POST;
    if (vc) delete vc;
    throw(AipsError("Error in Simulator::setapply."));
    return False;
  }

  // Creation apparently successful, so add to the apply list
  // TBD: consolidate with above?
  try {

    uInt napp=vc_p.nelements();
    vc_p.resize(napp+1,False,True);
    vc_p[napp] = vc;
    vc=NULL;

    // Maintain sort of apply list
    ve_p.setapply(vc_p);

    return True;

  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg()
       << LogIO::POST;
    if (vc) delete vc;
    throw(AipsError("Error in Simulator::setapply."));
    return False;
  }
  return False;
}






Bool Simulator::corrupt() {

  // VIS-plane (only) corruption
  
  LogIO os(LogOrigin("Simulator", "corrupt()", WHERE));

  try {
    
    ms_p->lock();
    if(mssel_p) mssel_p->lock();

//    makeVisSet();
//    AlwaysAssert(vs_p, AipsError);

    // Arrange the sort for efficient cal apply
    Block<Int> columns;
    // include scan iteration
    columns.resize(5);
    columns[0]=MS::ARRAY_ID;
    columns[1]=MS::SCAN_NUMBER;
    columns[2]=MS::FIELD_ID;
    columns[3]=MS::DATA_DESC_ID;
    columns[4]=MS::TIME;

    Matrix<Int> noselection;
    if(mssel_p) {
      vs_p = new VisSet(*mssel_p,columns,noselection);
    }
    else {
      vs_p = new VisSet(*ms_p,columns,noselection);
    }
    AlwaysAssert(vs_p, AipsError);

    // initCalSet() happens in VS creation unless there is a stored channel selection 
    // in the ms, and it equals the channel selection passed from here in mssel_p
    // vs_p->initCalSet();
    
    VisIter& vi=vs_p->iter();
    VisBuffer vb(vi);

    // Ensure VisEquation is ready - this sorts the VCs
    // if we want a different order for corruption we will either need to 
    // implement the sort here or create a VE::setcorrupt(vc_p)
    ve_p.setapply(vc_p);

    // set to corrupt Model down to B (T,D,G,etc) and correct Observed with AN,M,K
    ve_p.setPivot(VisCal::B); 
    
    // Apply 
    if (vc_p.nelements()>0) {
      os << LogIO::NORMAL << "Doing visibility corruption." 
	 << LogIO::POST;
      for (Int i=0;i<vc_p.nelements();i++) {
	//	os << vc_p[i]->longTypeName() << endl << vc_p[i]->siminfo() << endl <<
	//	  "spwok = " << vc_p[i]->spwOK() << LogIO::POST;
	os << vc_p[i]->siminfo() << "spwok = " << vc_p[i]->spwOK();
	if (vc_p[i]->type() >= ve_p.pivot()) 
	  os << " in corrupt mode." << endl << LogIO::POST;
	else 
	  os << " in correct mode." << endl << LogIO::POST;
      }
      for (vi.originChunks();vi.moreChunks();vi.nextChunk()) {
	// Only if 
	if (ve_p.spwOK(vi.spectralWindow())) {
	  for (vi.origin(); vi.more(); vi++) {

	    // corrupt model to pivot, correct data up to pivot
	    ve_p.collapseForSim(vb);	    

	    // Deposit corrupted visibilities into DATA
	    // vi.setVis(vb.modelVisCube(), VisibilityIterator::Observed);
	    vi.setVis(vb.visCube(), VisibilityIterator::Observed);

	    // RI TODO is this 100% right?
	    vi.setWeightMat(vb.weightMat());

	  }
	}
	else 
	  cout << "Encountered data spw " << vi.spectralWindow() << " for which there is no (simulated) calibration." << endl;
      }
    }

    // Old-fashioned noise, for now
//    if(ac_p != NULL){
//      os << LogIO::NORMAL << "Doing noise corruption " 
//	 << LogIO::POST;
//      for (vi.originChunks();vi.moreChunks();vi.nextChunk()) {
//	for (vi.origin(); vi.more(); vi++) {
//
//	  ac_p->apply(vb);
//	  vi.setVis(vb.visibility(), VisibilityIterator::Observed);
//	  vi.setVis(vb.visibility(), VisibilityIterator::Corrected);
//	}
//      }
//    }

    // Clear scratch columns - this is private, should it be?
    // vs_p->removeCalSet(*ms_p);

    // Flush to disk
    vs_p->flush();

    ms_p->relinquishAutoLocks();
    ms_p->unlock();
    if(mssel_p) mssel_p->unlock();

  } catch (std::exception& x) {
    ms_p->relinquishAutoLocks();
    ms_p->unlock();
    if(mssel_p) mssel_p->unlock();
    throw(AipsError(x.what()));
    return False;
  } 
  return True;
}




















Bool Simulator::observe(const String&   sourcename,
			   const String&   spwname,
			   const Quantity& startTime, 
			   const Quantity& stopTime)
{
  LogIO os(LogOrigin("Simulator", "observe()", WHERE));
  

  try {
    
    if(!feedsHaveBeenSet && !feedsInitialized) {
      os << "Feeds have not been set - using default " << feedMode_p << LogIO::WARN;
      sim_p->initFeeds(feedMode_p);
      feedsInitialized = True;
    }
    if(!timesHaveBeenSet_p) {
      os << "Times have not been set - using defaults " << endl
	 << "     Times will be interpreted as hour angles for first source"
	 << LogIO::WARN;
    }

    sim_p->observe(sourcename, spwname, startTime, stopTime);

    if(ms_p) delete ms_p; ms_p=0;
    if(mssel_p) delete mssel_p; mssel_p=0;
    ms_p = new MeasurementSet(msname_p, 
			      TableLock(TableLock::AutoNoReadLocking), 
			      Table::Update);

    ms_p->flush();

    ms_p->unlock();

  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;
    return False;
  } 
  return True;
}

Bool Simulator::predict(const Vector<String>& modelImage, 
			   const String& compList,
			   const Bool incremental) {
  
  LogIO os(LogOrigin("Simulator", "predict()", WHERE));
  
  // Note that incremental here does not apply to se_p->predict(False),
  // Rather it means: add the calculated model visibility to the data visibility.
  // We return a MS with Data, Model, and Corrected columns identical

  try {

    os << "Predicting visibilities using model: " << modelImage << 
      " and componentList: " << compList << LogIO::POST;
    if (incremental) {
      os << "The data column will be incremented" <<  LogIO::POST;
    } else {
      os << "The data column will be replaced" <<  LogIO::POST;
    }
    if(!ms_p) {
      os << "MeasurementSet pointer is null : logic problem!"
	 << LogIO::EXCEPTION;
    }
    ms_p->lock();   
    if(mssel_p) mssel_p->lock();   
    if (!createSkyEquation( modelImage, compList)) {
      os << LogIO::SEVERE << "Failed to create SkyEquation" << LogIO::POST;
      return False;
    }
    if (incremental) {
      se_p->predict(False,MS::MODEL_DATA);  
    } else {
      se_p->predict(False,MS::DATA);   //20091030 RI changed SE::predict to use DATA
    }
    destroySkyEquation();

    // Copy the predicted visibilities over to the observed and 
    // the corrected data columns
    makeVisSet();

    VisIter& vi = vs_p->iter();
    VisBuffer vb(vi);
    vi.origin();
    vi.originChunks();

    //os << "Copying predicted visibilities from MODEL_DATA to DATA and CORRECTED_DATA" << LogIO::POST;
      
    for (vi.originChunks();vi.moreChunks();vi.nextChunk()){
      for (vi.origin(); vi.more(); vi++) {
	//	vb.setVisCube(vb.modelVisCube());

	if (incremental) {
	  vi.setVis( (vb.modelVisCube() + vb.visCube()),
		     VisibilityIterator::Corrected);
	  vi.setVis(vb.correctedVisCube(),VisibilityIterator::Observed);

	  // This is to make things more consistent with VS::initCalSet 
	  // vi.setVis(vb.correctedVisCube(),VisibilityIterator::Model);
	  vb.setModelVisCube(Complex(1.0,0.0));
	  vi.setVis(vb.modelVisCube(),VisibilityIterator::Model);
	} else {
	  // from above, the prediction is now already in Observed.
	  //	  vi.setVis(vb.modelVisCube(),VisibilityIterator::Observed);
	  //	  vi.setVis(vb.modelVisCube(),VisibilityIterator::Corrected);
	}
      }
    }
    ms_p->unlock();     
    if(mssel_p) mssel_p->lock();   

  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;
    ms_p->unlock();     
    if(mssel_p) mssel_p->lock();   
    return False;
  } 
  return True;
}

Bool Simulator::createSkyEquation(const Vector<String>& image,
				     const String complist)
{

  LogIO os(LogOrigin("Simulator", "createSkyEquation()", WHERE));

  try {
    if(sm_p==0) {
      sm_p = new CleanImageSkyModel();
    }
    AlwaysAssert(sm_p, AipsError);
    
    // Add the componentlist
    if(complist!="") {
      if(!Table::isReadable(complist)) {
	os << LogIO::SEVERE << "ComponentList " << complist
	   << " not readable" << LogIO::POST;
	return False;
      }
      componentList_p=new ComponentList(complist, True);
      if(componentList_p==0) {
	os << LogIO::SEVERE << "Cannot create ComponentList from " << complist
	   << LogIO::POST;
	return False;
      }
      if(!sm_p->add(*componentList_p)) {
	os << LogIO::SEVERE << "Cannot add ComponentList " << complist
	   << " to SkyModel" << LogIO::POST;
	return False;
      }
    } else {
      componentList_p=0;
    }
    
    nmodels_p = image.nelements();
    if (nmodels_p == 1 && image(0) == "") nmodels_p = 0;
    
    if (nmodels_p > 0) {
      images_p.resize(nmodels_p); 
      
      for (Int model=0;model<Int(nmodels_p);model++) {
	if(image(model)=="") {
	  os << LogIO::SEVERE << "Need a name for model "  << model+1
	     << LogIO::POST;
	  return False;
	} else {
	  if(!Table::isReadable(image(model))) {
	    os << LogIO::SEVERE << image(model) << " is unreadable" << LogIO::POST;
	  } else {
	    images_p[model]=0;
	    os << "About to open model " << model+1 << " named "
	       << image(model) << LogIO::POST;
	    images_p[model]=new PagedImage<Float>(image(model));

	    AlwaysAssert(images_p[model], AipsError);
	    // Add distance
	    if(abs(distance_p.get().getValue())>0.0) {
	      os << "  Refocusing to distance " << distance_p.get("km").getValue()
		 << " km" << LogIO::POST;
	    }
	    Record info(images_p[model]->miscInfo());
	    info.define("distance", distance_p.get("m").getValue());
	    images_p[model]->setMiscInfo(info);
	    if(sm_p->add(*images_p[model])!=model) {
	      os << LogIO::SEVERE << "Error adding model " << model+1 << LogIO::POST;
	      return False;
	    }
	  }
	}
      }
    }
    
    
    if(vs_p) {
      delete vs_p; vs_p=0;
    }
    makeVisSet();
    
    cft_p = new SimpleComponentFTMachine();
    
    MeasurementSet *ams=0;
    
    if(mssel_p) {
      ams=mssel_p;
    }
    else {
      ams=ms_p;
    }
    if((ftmachine_p=="sd")||(ftmachine_p=="both")||(ftmachine_p=="mosaic")) {
      if(!gvp_p) {
	os << "Using default primary beams for gridding" << LogIO::POST;
	gvp_p=new VPSkyJones(*ams, True, parAngleInc_p, squintType_p);
      }
      if(ftmachine_p=="sd") {
	os << "Single dish gridding " << LogIO::POST;
	if(gridfunction_p=="pb") {
	  ft_p = new SDGrid(*gvp_p, cache_p/2, tile_p, gridfunction_p);
	}
	else {
	  ft_p = new SDGrid(cache_p/2, tile_p, gridfunction_p);
	}
      }
      else if(ftmachine_p=="mosaic") {
	os << "Performing Mosaic gridding" << LogIO::POST;
	ft_p = new MosaicFT(gvp_p, mLocation_p, stokesString_p, cache_p/2, tile_p, True);
      }
      else if(ftmachine_p=="both") {
	os << "Performing single dish gridding with convolution function "
	   << gridfunction_p << LogIO::POST;
	os << "and interferometric gridding with convolution function SF"
	   << LogIO::POST;
	
	ft_p = new GridBoth(*gvp_p, cache_p/2, tile_p,
			    mLocation_p, 
			    gridfunction_p, "SF", padding_p);
      }
      
      VisIter& vi(vs_p->iter());
      // Get bigger chunks o'data: this should be tuned some time
      // since it may be wrong for e.g. spectral line
      vi.setRowBlocking(100);
    }
    else {
      os << "Synthesis gridding " << LogIO::POST;
      // Now make the FTMachine
      //    if(wprojPlanes_p>1) {
      if (ftmachine_p=="wproject") {
	os << "Fourier transforms will use specified common tangent point:" << LogIO::POST;
	os << formatDirection(sourceDirection_p) << LogIO::POST;
	//      ft_p = new WProjectFT(*ams, facets_p, cache_p/2, tile_p, False);
	ft_p = new WProjectFT(wprojPlanes_p, mLocation_p, cache_p/2, tile_p, False);
      }
      else if (ftmachine_p=="pbwproject") {
	os << "Fourier transfroms will use specified common tangent point and PBs" 
	   << LogIO::POST;
	os << formatDirection(sourceDirection_p) << LogIO::POST;
	
	//	if (!epJ_p)
	  os << "Antenna pointing related term (EPJones) not set.  "
	     << "This is required when using pbwproject FTMachine." 
	     << "(gmoellen 06Nov20: pointing errors temporarily disabled)"
	     << LogIO::EXCEPTION;

   /*
	doVP_p = False; // Since this FTMachine includes PB
	if (wprojPlanes_p<=1)
	  {
	    os << LogIO::NORMAL
	       << "You are using wprojplanes=1. Doing co-planar imaging "
	       << "(no w-projection needed)" 
	       << LogIO::POST;
	    os << "Performing pb-projection"
	       << LogIO::POST;
	  }
	if((wprojPlanes_p>1)&&(wprojPlanes_p<64)) 
	  {
	    os << LogIO::WARN
	       << "No. of w-planes set too low for W projection - recommend at least 128"
	       << LogIO::POST;
	    os << "Performing pb + w-plane projection"
	       << LogIO::POST;
	  }
//	  epJ_p = new EPJones(*vs_p);
//	  epJ_p->load(epJTableName_p,"","diagonal");
	if(!gvp_p) 
	  {
	    os << "Using defaults for primary beams used in gridding" << LogIO::POST;
	    gvp_p=new VPSkyJones(*ms_p, True, parAngleInc_p, squintType_p);
	  }
//	  ft_p = new PBWProjectFT(*ms_p, epJ, gvp_p, facets_p, cache_p/2, 
//	  doPointing, tile_p, paStep_p, 
//	  pbLimit_p, True);

	String cfCacheDirName = "cache";
	if (mssel_p)
	  ft_p = new PBWProjectFT(*mssel_p, epJ_p, 
	  // gvp_p,
				  wprojPlanes_p, cache_p/2, 
				  cfCacheDirName,
				  applyPointingOffsets_p, doPBCorrection_p, 
				  tile_p, 
				  0.0, // Not required here. parAngleInc_p is used in gvp_p 
				  pbLimit_p, True);
	else
	  ft_p = new PBWProjectFT(*ms_p, epJ_p, 
				  // gvp_p, 
				  wprojPlanes_p, cache_p/2, 
				  cfCacheDirName,
				  applyPointingOffsets_p, doPBCorrection_p, 
				  tile_p, 
				  0.0, // Not required here. parAngleInc_p is used in gvp_p 
				  pbLimit_p, True);
	AlwaysAssert(ft_p, AipsError);
	cft_p = new SimpleComponentFTMachine();
	AlwaysAssert(cft_p, AipsError);

   */
      }
      else {
	os << "Fourier transforms will use image centers as tangent points" << LogIO::POST;
	ft_p = new GridFT(cache_p/2, tile_p, gridfunction_p, mLocation_p, padding_p);
      }
    }
    AlwaysAssert(ft_p, AipsError);
    
    se_p = new SkyEquation ( *sm_p, *vs_p, *ft_p, *cft_p );
    
    // Now add any SkyJones that are needed
    if(doVP_p) {
      if (doDefaultVP_p) {
	os << "Using default primary beams for mosaicing (use setvp to change)" << LogIO::POST;
	vp_p=new VPSkyJones(*ams, True, parAngleInc_p, squintType_p, skyPosThreshold_p);
      } else {
	Table vpTable( vpTableStr_p );
	vp_p=new VPSkyJones(*ams, vpTable, parAngleInc_p, squintType_p);
      }
      vp_p->summary();
      se_p->setSkyJones(*vp_p);
    }
    else {
      vp_p=0;
    }
  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;
    ms_p->unlock();     
    if(mssel_p) mssel_p->lock();   
    return False;
  } 
  return True;
};

void Simulator::destroySkyEquation() 
{
  if(se_p) delete se_p; se_p=0;
  if(sm_p) delete sm_p; sm_p=0;
  if(vp_p) delete vp_p; vp_p=0;
  if(componentList_p) delete componentList_p; componentList_p=0;

  for (Int model=0;model<Int(nmodels_p);model++) {
    if(images_p[model]) delete images_p[model]; images_p[model]=0;
  }
};














Bool Simulator::setlimits(const Double shadowLimit,
			  const Quantity& elevationLimit)
{
  
  LogIO os(LogOrigin("Simulator", "setlimits()", WHERE));
  
  try {
    
    sim_p->setFractionBlockageLimit( shadowLimit );
    sim_p->setElevationLimit( elevationLimit );
  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;
    return False;
  } 
  return True;
}
      
Bool Simulator::setauto(const Double autocorrwt) 
{
  
  LogIO os(LogOrigin("Simulator", "setauto()", WHERE));
  
  try {
    
    sim_p->setAutoCorrelationWt(autocorrwt);

  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg() << LogIO::POST;
    return False;
  } 
  return True;
}
      
void Simulator::makeVisSet() {

  if(vs_p) return;
  
  Block<int> sort(0);
  sort.resize(5);
  sort[0] = MS::FIELD_ID;
  sort[1] = MS::FEED1;
  sort[2] = MS::ARRAY_ID;
  sort[3] = MS::DATA_DESC_ID;
  sort[4] = MS::TIME;
  Matrix<Int> noselection;
  if(mssel_p) {
    vs_p = new VisSet(*mssel_p,sort,noselection);
  }
  else {
    if (ms_p) 
      vs_p = new VisSet(*ms_p,sort,noselection);
    else
      throw(AipsError("No measurement set open in Simulator."));
  }
  AlwaysAssert(vs_p, AipsError);
  
}



Bool Simulator::setoptions(const String& ftmachine, const Int cache, const Int tile,
			      const String& gridfunction, const MPosition& mLocation,
			      const Float padding, const Int facets, const Double maxData,
			      const Int wprojPlanes)
{
  LogIO os(LogOrigin("Simulator", "setoptions()", WHERE));
  
  os << "Setting processing options" << LogIO::POST;
  
  sim_p->setMaxData(maxData*1024.0*1024.0);

  ftmachine_p=downcase(ftmachine);
  if(cache>0) cache_p=cache;
  if(tile>0) tile_p=tile;
  gridfunction_p=downcase(gridfunction);
  mLocation_p=mLocation;
  if(padding>=1.0) {
    padding_p=padding;
  }
  facets_p=facets;
  wprojPlanes_p = wprojPlanes;
  // Destroy the FTMachine
  if(ft_p) {delete ft_p; ft_p=0;}
  if(cft_p) {delete cft_p; cft_p=0;}

  return True;
}


Bool Simulator::detached() const
{
  return False;
}

String Simulator::formatDirection(const MDirection& direction) {
  MVAngle mvRa=direction.getAngle().getValue()(0);
  MVAngle mvDec=direction.getAngle().getValue()(1);
  ostringstream oss;
  oss.setf(ios::left, ios::adjustfield);
  oss.width(14);
  oss << mvRa(0.0).string(MVAngle::TIME,8);
  oss.width(14);
  oss << mvDec.string(MVAngle::DIG2,8);
  oss << "     " << MDirection::showType(direction.getRefPtr()->getType());
  return String(oss);
}

String Simulator::formatTime(const Double time) {
  MVTime mvtime(Quantity(time, "s"));
  return mvtime.string(MVTime::DMY,7);
}



Bool Simulator::setdata(const Vector<Int>& spectralwindowids,
			   const Vector<Int>& fieldids,
			   const String& msSelect)
  
{

  
  LogIO os(LogOrigin("Simulator", "setdata()", WHERE));

  if(!ms_p) {
    os << LogIO::SEVERE << "Program logic error: MeasurementSet pointer ms_p not yet set"
       << LogIO::POST;
    return False;
  }

  try {
    
    os << "Selecting data" << LogIO::POST;
    
   // Map the selected spectral window ids to data description ids
    MSDataDescColumns dataDescCol(ms_p->dataDescription());
    Vector<Int> ddSpwIds=dataDescCol.spectralWindowId().getColumn();

    Vector<Int> datadescids(0);
    for (uInt row=0; row<ddSpwIds.nelements(); row++) {
      Bool found=False;
      for (uInt j=0; j<spectralwindowids.nelements(); j++) {
	if (ddSpwIds(row)==spectralwindowids(j)) found=True;
      };
      if (found) {
	datadescids.resize(datadescids.nelements()+1,True);
	datadescids(datadescids.nelements()-1)=row;
      };
    };

    if(vs_p) delete vs_p; vs_p=0;
    if(mssel_p) delete mssel_p; mssel_p=0;
      
    // If a selection has been made then close the current MS
    // and attach to a new selected MS. We do this on the original
    // MS. 
    if(fieldids.nelements()>0||datadescids.nelements()>0) {
      os << "Performing selection on MeasurementSet" << LogIO::POST;
      Table& original=*ms_p;
      
      // Now we make a condition to do the old FIELD_ID, SPECTRAL_WINDOW_ID
      // selection
      TableExprNode condition;
      String colf=MS::columnName(MS::FIELD_ID);
      String cols=MS::columnName(MS::DATA_DESC_ID);
      if(fieldids.nelements()>0&&datadescids.nelements()>0){
	condition=original.col(colf).in(fieldids)&&original.col(cols).in(datadescids);
        os << "Selecting on field and spectral window ids" << LogIO::POST;
      }
      else if(datadescids.nelements()>0) {
	condition=original.col(cols).in(datadescids);
        os << "Selecting on spectral window id" << LogIO::POST;
      }
      else if(fieldids.nelements()>0) {
	condition=original.col(colf).in(fieldids);
        os << "Selecting on field id" << LogIO::POST;
      }
      
      // Now remake the original ms
      mssel_p = new MeasurementSet(original(condition));

      //AlwaysAssert(mssel_p, AipsError);
      //mssel_p->rename(msname_p+"/SELECTED_TABLE", Table::Scratch);
      if(mssel_p->nrow()==0) {
	delete mssel_p; mssel_p=0;
	os << LogIO::WARN
	   << "Selection is empty: reverting to original MeasurementSet"
	   << LogIO::POST;
	mssel_p=new MeasurementSet(original);
      }
      else {
	mssel_p->flush();
      }

    }
    else {
      mssel_p=new MeasurementSet(*ms_p);
    }
    {
      Int len = msSelect.length();
      Int nspace = msSelect.freq (' ');
      Bool nullSelect=(msSelect.empty() || nspace==len);
      if (!nullSelect) {
	os << "Now applying selection string " << msSelect << LogIO::POST;
	MeasurementSet* mssel_p2;
	// Apply the TAQL selection string, to remake the original MS
	String parseString="select from $1 where " + msSelect;
	mssel_p2=new MeasurementSet(tableCommand(parseString,*mssel_p));
	AlwaysAssert(mssel_p2, AipsError);
	// Rename the selected MS as */SELECTED_TABLE2
	//mssel_p2->rename(msname_p+"/SELECTED_TABLE2", Table::Scratch); 
	if (mssel_p2->nrow()==0) {
	  os << LogIO::WARN
	     << "Selection string results in empty MS: "
	     << "reverting to original MeasurementSet"
	     << LogIO::POST;
	  delete mssel_p2;
	} else {
	  if (mssel_p) {
	    delete mssel_p; 
	    mssel_p=mssel_p2;
	    mssel_p->flush();
	  }
	}
      } else {
	os << "No selection string given" << LogIO::POST;
      }

      if(mssel_p->nrow()!=ms_p->nrow()) {
	os << "By selection " << ms_p->nrow() << " rows are reduced to "
	   << mssel_p->nrow() << LogIO::POST;
      }
      else {
	os << "Selection did not drop any rows" << LogIO::POST;
      }
    }
    
    // Now create the VisSet
    if(vs_p) delete vs_p; vs_p=0;
    makeVisSet();
    //Now assign the source direction to something selected or sensible
    {
      Int fieldsel=0;
      if(fieldids.nelements() >0)
	fieldsel=fieldids(0);
      sourceDirection_p=(vs_p->iter()).msColumns().field().phaseDirMeas(fieldsel); 
    }
    return True;
  } catch (AipsError x) {
    os << LogIO::SEVERE << "Caught exception: " << x.getMesg()
       << LogIO::POST;
    return False;
  } 
  return True;
}

