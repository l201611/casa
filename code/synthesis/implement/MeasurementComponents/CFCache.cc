// -*- C++ -*-
//# CFCache.cc: Implementation of the CFCache class
//# Copyright (C) 1997,1998,1999,2000,2001,2002,2003
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
//# $Id$
#include <synthesis/MeasurementComponents/SynthesisError.h>
#include <synthesis/MeasurementComponents/CFCache.h>
#include <synthesis/MeasurementComponents/Utils.h>
#include <lattices/Lattices/LatticeExpr.h>
#include <casa/Exceptions/Error.h>
#include <casa/OS/Directory.h>
#include <fstream>

namespace casa{
  CFCache::~CFCache()
  {
    // Int n=memCache_p.nelements();
    // for (Int i=0;i<n;i++)
    //   delete memCache_p[i].data;
    memCache_p.resize(0);
  }
  //
  //-------------------------------------------------------------------------
  // Load just the axillary info. if found.  The convolution functions
  // are loaded on-demand.
  //
  void CFCache::initCache()
  {
    ostringstream name;
    String line;
    Directory dirObj(Dir);

    if (Dir.length() == 0) 
      throw(SynthesisFTMachineError("Got null string for disk cache dir. "
				    "in CFCache::initCache()"));
    //
    // If the directory does not exist, create it
    //
    if (!dirObj.exists()) dirObj.create();
    else if ((!dirObj.isWritable()) || (!dirObj.isReadable()))
      {
	throw(SynthesisFTMachineError(String("Directory \"")+Dir+String("\"")+
				      String(" for convolution function cache"
					     " exists but is unreadable/unwriteable")));
      }

    try
      {
	name << Dir << "/" << aux;
	File file(name);
	Int Npa=0,Nw=0;
	ifstream aux;
	Bool readFromFile=False;
	if (file.exists() && file.isRegular()) 
	  {
	    readFromFile=True;
	    aux.open(name.str().c_str());
	    if (readFromFile && aux.good()) aux >> Npa >> Nw;
	    else
	      throw(SynthesisFTMachineError(String("Error while reading convolution "
						   "function cache file ")+name));
	  }

	if (Npa > 0)
	  {
	    paList.resize(Npa,True);

	    IPosition s(2,Nw,Npa);
	    XSup.resize(s,True);
	    YSup.resize(s,True);
	    Sampling.resize(Npa,True);
	    for(Int i=0;i<Npa;i++)
	      {
		Float pa, S;
		Int XS, YS;
		s[2]=i;
		aux >> pa;
		for(Int iw=0;iw<Nw;iw++)
		  {
		    s[0]=iw;
		    aux >> XS >> YS;
		    YS = XS;
		    paList[i] = pa*M_PI/180.0;
		    XSup(iw,i)=XS;
		    YSup(iw,i)=YS;
		  }
		aux >> S;
		Sampling[i]=S;
	      }
	  }
      }
    catch(AipsError& x)
      {
	throw(SynthesisFTMachineError(String("Error while initializing CF disk cache: ")
				      +x.getMesg()));
      }
  }
  CFCache& CFCache::operator=(const CFCache& other)
  {
    paList = other.paList;
    Sampling = other.Sampling;
    XSup = other.XSup;
    YSup = other.YSup;
    Dir = other.Dir;
    cfPrefix = other.cfPrefix;
    aux = other.aux;
    return *this;
  };
  void CFCache::makeFTCoordSys(const CoordinateSystem& coords,
			       const Int& convSize,
			       const Vector<Double>& ftRef,
			       CoordinateSystem& ftCoords)
  {
    Int directionIndex;

    ftCoords = coords;
    directionIndex=ftCoords.findCoordinate(Coordinate::DIRECTION);
    //  The following line follows the (lame) logic that if a
    //  DIRECTION axis was not found, the coordinate system must be of
    //  the FT domain already
    if (directionIndex == -1) return;

    DirectionCoordinate dc;//=coords.directionCoordinate(directionIndex);
    //	AlwaysAssert(directionIndex>=0, AipsError);
    dc=coords.directionCoordinate(directionIndex);
    Vector<Bool> axes(2); axes(0)=axes(1)=True;//axes(2)=True;
    Vector<Int> shape(2,convSize);

    Vector<Double>ref(4);
    ref(0)=ref(1)=ref(2)=ref(3)=0;
    dc.setReferencePixel(ref);
    Coordinate* ftdc=dc.makeFourierCoordinate(axes,shape);
    Vector<Double> refVal;
    refVal=ftdc->referenceValue();
    refVal(0)=refVal(1)=0;
    ftdc->setReferenceValue(refVal);
    ref(0)=ftRef(0);
    ref(1)=ftRef(1);
    ftdc->setReferencePixel(ref);
    
    ftCoords.replaceCoordinate(*ftdc, directionIndex);
    delete ftdc; ftdc=0;
  }
  //
  //-------------------------------------------------------------------------
  //
  Int CFCache::addToMemCache(Float pa, CFType& cf, 
			     CoordinateSystem& coords,
			     Vector<Int>& xConvSupport,
			     Vector<Int>& yConvSupport,
			     Float convSampling)
  {
    Float dPA=paCD_p.getParAngleTolerance().getValue("rad");
    Int where=-1, N=memCache_p.nelements(), wConvSize = cf.shape()(2);
    Bool found=searchConvFunction(where, pa, dPA);
    //
    // If the PA value was not found, the return value in "where" is
    // the negative of the location in which the PA value should be
    // found.  Convert it to positive value to be used for resizing
    // etc.
    //
    where=abs(where);
    //
    // Resize the arrays if the CF for the relevant PA was not in the
    // MEM cache.  Note that if the arrays are already of size
    // where+1, Array<>::resize() is a no-op.
    //
    if (N <= where)
      {
	IPosition s(2,wConvSize,where+1);
	memCache_p.resize(where+1, True);
	paList.resize(where+1,True);
	XSup.resize(s,True);	YSup.resize(s,True);
	Sampling.resize(where+1,True);
      }
    //
    // If the PA was not found, enter the aux. values in the internal
    // arrays.
    //
    if (!found)
      {
	paList[where] = pa;
	for(Int iw=0;iw<wConvSize;iw++)
	  {
	    YSup(iw,N) = xConvSupport(iw);
	    XSup(iw,N) = yConvSupport(iw);
	  }
	Sampling[N]=convSampling;
      }
    //
    // If the CF was in the mem. cache, add it.
    //
    if (memCache_p[where].null())
      {
	memCache_p[where].data = new CFType(cf);
	memCache_p[where].sampling.resize(1);
	memCache_p[where].sampling = convSampling;
	memCache_p[where].xSupport = xConvSupport;
	memCache_p[where].ySupport = yConvSupport;
	memCache_p[where].coordSys = coords;
      }
    
    return where;
  }
  //-------------------------------------------------------------------------
  // Write the conv. functions from the mem. cache to the disk cache.
  //
  void CFCache::cacheConvFunction(Int which,  const Float& pa, CFType& cf, 
				  CoordinateSystem& coords,
				  CoordinateSystem& ftCoords,
				  Int &convSize,
				  Vector<Int> &xConvSupport, 
				  Vector<Int> &yConvSupport, 
				  Float convSampling, String nameQualifier,
				  Bool savePA)
  {
    if (Dir.length() == 0) return;
    Int N=paList.nelements();
    if (which < 0) which = N-1;
    try
      {
	IPosition newConvShape = cf.shape();
	Int wConvSize = newConvShape(2);
	for(Int iw=0;iw<wConvSize;iw++)
	  {
	    IPosition sliceStart(4,0,0,iw,0), 
	      sliceLength(4,newConvShape(0),newConvShape(1),1,newConvShape(3));

	    Vector<Double> ftRef(2);
	    ftRef(0)=newConvShape(0)/2-1;
	    ftRef(1)=newConvShape(1)/2-1;
	    makeFTCoordSys(coords, convSize, ftRef, ftCoords);
	    ostringstream name;
	    name << Dir << "/" << cfPrefix << nameQualifier << iw << "_" << which;
	    const CFType tmpArr=cf(Slicer(sliceStart,sliceLength));
	    storeArrayAsImage(name, ftCoords,tmpArr);
	  }
	addToMemCache(pa,cf, ftCoords, xConvSupport, yConvSupport, convSampling);
	
	// if (savePA)
	//   {
	//     IPosition s(2,wConvSize,N+1);
	//     paList.resize(N+1,True);
	//     // 	XSup.resize(N+1,True); 
	//     // 	YSup.resize(N+1,True); 
	//     XSup.resize(s,True);
	//     YSup.resize(s,True);
	//     Sampling.resize(N+1,True);
	//     paList[N] = pa;
	//     for(Int iw=0;iw<wConvSize;iw++)
	//       {
	// 	YSup(iw,N) = convSupport(iw);
	// 	XSup(iw,N) = convSupport(iw);
	//       }
	//     Sampling[N]=convSampling;
	//   }
      }
    catch (AipsError& x)
      {
	throw(SynthesisFTMachineError("Error while caching CF to disk in "
				      "CFCache::cacheConvFunction(): "
				      +x.getMesg()));
      }
  }
  //
  //-------------------------------------------------------------------------
  // Write the weight functions from the mem. cache to the disk cache.
  //
  void CFCache::cacheWeightsFunction(Int which, Float pa, 
					 Array<Complex>& cfWt, 
					 CoordinateSystem& coords,
					 Int &convSize,
					 Vector<Int> &convSupport, 
					 Float convSampling)
  {
//     Int N=paList.nelements();
//     if (Dir.length() == 0) return;

//     try
//       {
// 	IPosition newConvShape = cfWt.shape();
// 	Int wConvSize = newConvShape(2);
// 	for(Int iw=0;iw<wConvSize;iw++)
// 	  {
// 	    IPosition sliceStart(4,0,0,iw,0), 
// 	      sliceLength(4,newConvShape(0),newConvShape(1),1,newConvShape(3));

// 	    Vector<Double> ftRef(2);
// 	    ftRef(0)=newConvShape(0)/2-1;
// 	    ftRef(1)=newConvShape(1)/2-1;
// 	    makeFTCoordSys(coords, convSize, ftRef, ftCoords);
// 	    ostringstream name;
// 	    name << Dir << "/" << cfPrefix << "WT" << iw << "_" << which;
// 	    storeArrayAsImage(name, ftCoords,((cfWt(Slicer(sliceStart,sliceLength)).nonDegenerate())));
// 	  }
// 	IPosition s(3,wConvSize,1,N+1);
// 	paList.resize(N+1,True);
// // 	XSup.resize(N+1,True); 
// // 	YSup.resize(N+1,True); 
// 	XSup.resize(s,True);
// 	YSup.resize(s,True);
// 	Sampling.resize(N+1,True);
// 	paList[N] = pa;
// 	for(Int iw=0;iw<wConvSize;iw++)
// 	  {
// 	    YSup(iw,N) = convSupport(iw,0,which);
// 	    XSup(iw,N) = convSupport(iw,0,which);
// 	  }
// 	Sampling[N]=convSampling;
//       }
//     catch (AipsError& x)
//       {
// 	throw(SynthesisFTMachineError("Error while caching CFWT to disk in "
// 				      "CFCache::cacheCFtion(): "
// 				      +x.getMesg()));
//       }
  }
  //
  //-------------------------------------------------------------------------
  //  
  Bool CFCache::searchConvFunction(const VisBuffer& vb, 
					    VPSkyJones& vpSJ, 
					    Int& which,
					    Float &pa)
  {
    Int i,NPA=paList.nelements(); Bool paFound=False;
    Float iPA, dPA;
    dPA = vpSJ.getPAIncrement().getValue("rad");
    /*
    Vector<Float> antPA = vb.feed_pa(getCurrentTimeStamp(vb));
    pa = sum(antPA)/(antPA.nelements()-1);
    */
    pa = getPA(vb);
    //    cout << dPA*57.295 << " " << pa*57.295 << endl;
    //    pa = 0;
    //    cout << "######CFDC::search: " << pa << " " << getPA(vb) << endl;
    //    if (NPA == 0) return -1;
    
    for(i=0;i<NPA;i++)
      {
	iPA = paList[i];
	if (fabs(iPA - pa) <= dPA)
	  {
	    paFound = True;
	    break;
	  }
      }
    if (paFound) which = i; else which = -i;
    return paFound;
  }
  //
  //-------------------------------------------------------------------------
  //  
  Bool CFCache::searchConvFunction(Int& which,
				   const Float pa, const Float dPA)
  {
    if (paList.nelements()==0) initCache();
    Int i,NPA=paList.nelements(); Bool paFound=False;
    Float iPA;
    
    Float paDiff=2*dPA;
    Int saveNdx=-1;

    saveNdx = -1;
    for(i=0;i<NPA;i++)
      {
	iPA = paList[i];
	if (fabs(iPA - pa) < paDiff)
	  {
	    saveNdx = i;
	    paDiff = fabs(iPA-pa);
	  }
      }
    if (saveNdx > -1)
      {
	iPA = paList[saveNdx];
	if (fabs(iPA - pa) <= dPA)
	  {
	    i = saveNdx;
	    paFound=True;
	  }
      }
    if (paFound) which = i; 
    else which = -i;
    return paFound;
  }
  //
  //-------------------------------------------------------------------------
  //Write the aux. info. also in the disk cache (wonder if this should
  //be automatically be called from cacheCFtion() method).
  //
  void CFCache::flush()
  {
    if (Dir.length() == 0) return;
    ostringstream name;
    
    name << Dir << "/aux.dat";
    Int n=memCache_p.nelements(),nw;
    if (n>0)
      try
	{
	  nw=memCache_p[0].xSupport.nelements();
	  ofstream aux(name.str().c_str());
	  aux << n << " " << nw << endl;
	  for(Int ipa=0;ipa<n;ipa++)
	    {
	      aux << paList[ipa]*180.0/M_PI << " ";
	      for(int iw=0;iw<nw;iw++)
		aux << memCache_p[ipa].xSupport(iw) << " " << memCache_p[ipa].ySupport(iw) << " ";
	      aux << " " << Sampling[ipa] <<endl;
	    }
	}
      catch(AipsError &x)
	{
	  throw(SynthesisFTMachineError(String("Error while writing ")
					+ name + x.getMesg()));
	}
  }
  void CFCache::flushold()
  {
    if (Dir.length() == 0) return;
    ostringstream name;
    
    name << Dir << "/aux.dat";
    try
      {
	//    cout << "Writing to " << name.str() << endl;
	IPosition supportShape=XSup.shape();
	ofstream aux(name.str().c_str());
	aux << paList.nelements() << " " << supportShape[0] << endl;
	for(uInt ipa=0;ipa<paList.nelements();ipa++)
	  {
	    aux << paList[ipa]*180.0/M_PI << " ";
	    for(int iw=0;iw<supportShape[0];iw++)
	      aux << XSup(iw,ipa) << " " << YSup(iw,ipa) << " ";
	    aux << " " << Sampling[ipa] <<endl;
	  }
      }
    catch(AipsError &x)
      {
	throw(SynthesisFTMachineError(String("Error while writing ")
				      + name + x.getMesg()));
      }
  }
  //
  //-------------------------------------------------------------------------
  //Along with the aux. info., also save the average PB in the disk cache.
  //
  void CFCache::flush(ImageInterface<Float>& avgPB)
  {
    if (Dir.length() == 0) return;
    flush();
    ostringstream Name;
    Name << Dir <<"/avgPB";
    try
      {
	storeImg(Name, avgPB);
      }
    catch(AipsError &x)
      {
	throw(SynthesisFTMachineError(String("Error while writing ")
				      + Name + x.getMesg()));
      }
  }
  //
  //-------------------------------------------------------------------------
  //Load the average PB from the disk cache.
  //
  void CFCache::loadAvgPB(ImageInterface<Float>& avgPB)
  {
    if (Dir.length() == 0) return;
    ostringstream name;
    name << Dir << "/avgPB";
    //    cout << name.str() << endl;
    try
      {
	PagedImage<Float> tmp(name.str().c_str());
	avgPB.resize(tmp.shape());
	avgPB.put(tmp.get());
      }
    catch(AipsError& x) // Just rethrowing the exception for now.
                        // Ultimately, this should be used to make
			// the state of this object consistant.
      {
	throw(SynthesisFTMachineError(String("Error while loading \"")+
				      name + String("\": ") + x.getMesg()));
      }
  }
  //
  //-------------------------------------------------------------------------
  //Load a conv. func. from the disk.  This is non-optimal due to the
  //data structure used for the conv. func. in-memory cache (it's an
  //array of pointers where it should really be a List of pointers).
  //The conf. func. index, which is also used as a key to located them
  //in the mem. cache, are not assured to be contiguous.  As a result,
  //in the current implementation there can be gaps in the
  //convFuncCache array.  These gaps are initialized to NULL pointers.
  //It's not much of a memory waste, but still non-optimal!  Leaving
  //it like this for now.
  //
  // Return TRUE if loaded from disk and FLASE if found in the mem. cache.
  //
  Int CFCache::loadFromDisk(Int where, Float pa, Float dPA,
			     Int Nw, CFStoreCacheType &convFuncCache,
			     CFStore& cfs, String prefix)
  {
    Vector<Int> xconvSupport,yconvSupport;;
    Vector<Float> convSampling;
    Double cfRefFreq; CoordinateSystem coordSys;
    Array<Complex> cfBuf;
    
    if (Dir.length() == 0) 
      throw(SynthesisFTMachineError("CFCache::loadFromDisk(): Cache dir. name not set"));
      
    if (where < (Int)convFuncCache.nelements() && (!convFuncCache[where].data.null())) 
      return MEMCACHE;

    Int wConvSize, polInUse=2;
    Int N=convFuncCache.nelements();

    //
    // Re-size the conv. func. memory cache if required, and set the
    // new members of the resized cache to NULL.  This is used in the
    // loop below to make a decision about allocating new memory or
    // not.
    //
    convFuncCache.resize(max(where+1,N), True);
    //    for(Int i=N;i<=where;i++) convFuncCache[i].data=NULL;
    //
    // Each w-plan is in a separate disk file.  Each file contains all
    // polarization planes. Memory cache holds all w-planes and
    // poln-planes in a single complex array.  The loop below read
    // each w-plane image from the disk, and fills in the 3D
    // mem. cache for each computed PA.
    //
    wConvSize = Nw;
    for(Int iw=0;iw<Nw;iw++)
      {
	ostringstream name;
	name << Dir << prefix << iw << "_" << where;
	try
	  {
	    PagedImage<Complex> tmp(name.str().c_str());
	    Int index= tmp.coordinates().findCoordinate(Coordinate::SPECTRAL);
	    coordSys = tmp.coordinates();
	    SpectralCoordinate spCS = coordSys.spectralCoordinate(index);

	    cfRefFreq=spCS.referenceValue()(0);
	
	    polInUse = tmp.shape()(2);
	    IPosition ts=tmp.shape(),ndx(4,0,0,0,0),ts2(4,0,0,0,0);
	    Array<Complex> imBuf=tmp.get();
	    if (convFuncCache[where].data.null())
	      cfBuf.resize(IPosition(4,ts(0),ts(1), wConvSize,polInUse));
	
	    ndx(2)=iw;                              // The w-axis
	    for(ndx(3)=0;ndx(3)<polInUse;ndx(3)++)  // The Poln. axis.
	      for(ndx(0)=0;ndx(0)<ts(0);ndx(0)++)   
	    	for(ndx(1)=0;ndx(1)<ts(1);ndx(1)++)
	    	  {
	    	    ts2(0)=ndx(0);ts2(1)=ndx(1);
	    	    ts2(2)=ndx(3); // The Poln. axis of the disk-cache
	    	    ts2(3)=0;      // The freq. axis of the disk-cache
	    	    cfBuf(ndx)=imBuf(ts2);
	    	  }
	  }
	catch(AipsError &x)
	  {
	    throw(SynthesisFTMachineError(String("Error while loading \"")+
					  name + String("\": ") + x.getMesg()));
	  }
      }
    xconvSupport.resize(wConvSize,True);
    yconvSupport.resize(wConvSize,True);
    for(Int i=0;i<wConvSize;i++)
      {
	xconvSupport(i) = XSup(i,where);
	yconvSupport(i) = YSup(i,where);
      }

    convSampling = Sampling[where];
    
    where=addToMemCache(pa, cfBuf, coordSys, xconvSupport, yconvSupport, Sampling[where]);
    cfs=convFuncCache[where];
    
    return DISKCACHE;
  };
  // Locate a convlution function in either mem. or disk cache.  
  // Return CFCache::DISKCACHE (=1) if found in the disk cache.
  //        CFCache::MEMCACHE (=2)  if found in the mem. cache.
  //       <0 if not found in either cache.  In this case, absolute of
  //          the return value corresponds to the index in the list of
  //          conv. funcs. where this conv. func. should be filled
  //
  Int CFCache::locateConvFunction(const Int Nw, const Float pa, const Float dPA,
				  CFStore& cfs)
  {
    Int paKey,retVal=NOTCACHED; 
    Bool found;
    // Search for the PA corresponding to the supplied VB to find a
    // paKey in memCache_p which has a Conv. Func. within dPA (dPA is
    // given by paCD).  If found, return the key in paKey.
    found = searchConvFunction(paKey,pa, dPA);
    if (found)
      {
	retVal=loadFromDisk(paKey,pa,dPA,Nw,memCache_p,cfs);
	switch (retVal)
	  {
	  case DISKCACHE:
	    {
	      //*****************	    convWeights.reference(*convWeightsCache[PAIndex]);
	      if (paKey < (Int)memCache_p.nelements())
		logIO() << "Loaded from disk cache: Conv. func. # "
			<< paKey << LogIO::POST;
	      else
		throw(SynthesisFTMachineError("CFCache::locateConvFunction(): "
					      "Internal error: paKey out of range"));
	      break;
	    }
	  case MEMCACHE:{break;}
	  case NOTCACHED: {return retVal;}
	  };
	cfs=(memCache_p[paKey]);
	//**************	convWeights.reference(*convWeightsCache[paKey]);
      }
    return retVal;
  }

} // end casa namespace
