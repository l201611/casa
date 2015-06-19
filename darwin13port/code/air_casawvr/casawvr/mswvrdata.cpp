/**
   Bojan Nikolic <b.nikolic@mrao.cam.ac.uk>, <bojan@bnikolic.co.uk>
   Initial version January 2010. 
   
   This file is part of LibAIR and is licensed under GNU Public
   License Version 2
   
   \file mswvrdata.cpp

*/

#include <set>
#include <memory>
#include <iostream>

#include "mswvrdata.hpp"
#include "msspec.hpp"
#include "msutils.hpp"

#include <casacore/ms/MeasurementSets/MeasurementSet.h>
#include <casacore/ms/MeasurementSets/MSProcessor.h>
#include <casacore/ms/MeasurementSets/MSColumns.h>
#include <casacore/casa/Utilities/GenSort.h>

#include <casa/Arrays/Vector.h>

#include "almawvr/arraydata.hpp"
#include "casawvr_errs.hpp"


namespace LibAIR {

  SPWSet
  WVRSPWIDs(const casa::MeasurementSet &ms)
  {
    const casa::MSSpectralWindow & specTable(ms.spectralWindow());
    // Not using these in present algorithm
    //const casa::MSProcessor  & proc(ms.processor());

    casa::ROScalarColumn<casa::Int> nc(specTable,
				       casa::MSSpectralWindow::columnName(casa::MSSpectralWindow::NUM_CHAN));
    
    SPWSet res;
    for(size_t i=0; i<specTable.nrow(); ++i)
    {
      if (nc(i)==4)
	res.insert(i);
    }
    return res;
  }

  std::set<size_t>
  WVRDataDescIDs(const casa::MeasurementSet &ms)
  {
    SPWSet ss=WVRSPWIDs(ms);
    std::map<size_t, size_t> ddmap=SPWDataDescMap(ms);
    std::set<size_t> res;

    for(SPWSet::const_iterator si=ss.begin();
	si!=ss.end();
	++si)
    {
      if (ddmap.count(*si))
      {
	res.insert(ddmap[*si]);
      }
    }
    return res;
  }

  size_t nWVRSPWIDs(const casa::MeasurementSet &ms)
  {
    SPWSet s=WVRSPWIDs(ms);
    return s.size();
  }

  AntSet
  WVRAntennas(const casa::MeasurementSet &ms)
  {
    AntSet res=WVRAntennasFeedTab(ms);
    if (res.size() == 0)
    {
      res=WVRAntennasMainTab(ms);
    }
    return res;
  }
  
  AntSet
  WVRAntennasFeedTab(const casa::MeasurementSet &ms)
  {
    const casa::MSFeed &feedtable=ms.feed();

    casa::ROScalarColumn<casa::Int> ant(feedtable,
					casa::MSFeed::columnName(casa::MSFeed::ANTENNA_ID));

    casa::ROScalarColumn<casa::Int> fspw(feedtable,
					 casa::MSFeed::columnName(casa::MSFeed::SPECTRAL_WINDOW_ID));

    SPWSet spws=WVRSPWIDs(ms);
    
    const size_t nfeeds=feedtable.nrow();
    AntSet res;
    for (size_t i=0; i<nfeeds; ++i)
    {
      if( spws.count(fspw(i)) )
      {
	res.insert(ant(i));
      }
    }

    return res;
    
  }

  AntSet
  WVRAntennasMainTab(const casa::MeasurementSet &ms)
  {
    std::set<size_t> dsc_ids=WVRDataDescIDs(ms);

    casa::ROScalarColumn<casa::Int> c_desc_id(ms,
					  casa::MS::columnName(casa::MS::DATA_DESC_ID));    
    casa::ROScalarColumn<casa::Int> a1(ms,
				       casa::MS::columnName(casa::MS::ANTENNA1));

    AntSet res;
    const size_t nrows=c_desc_id.nrow();    
    for(size_t i=0; i<nrows; ++i)
    {
      if (dsc_ids.count(c_desc_id(i))>0)
      {
	res.insert(a1(i));
      }
    }
    return res;
  }

  void WVRAddFlaggedAnts(const casa::MeasurementSet &ms,
			 std::set<int> &flaggedAnts)
  {
        // add the antennas flagged in the ANTENNA table to the set
    casa::ROScalarColumn<casa::Bool> antflagrow(ms.antenna(),
						casa::MSAntenna::columnName(casa::MSAntenna::FLAG_ROW));
    const size_t nants=ms.antenna().nrow();
    for(size_t i=0; i<nants; i++)
    {
      if(antflagrow(i)==casa::True) // i.e. flagged
      {
	flaggedAnts.insert(i);
      }
    }
  }


  void WVRTimeStatePoints(const casa::MeasurementSet &ms,
			  std::vector<double> &times,
			  std::vector<size_t> &states,
			  std::vector<size_t> &field,
			  std::vector<size_t> &source,
			  const std::vector<size_t>& sortedI)
  {
    std::set<size_t> dsc_ids=WVRDataDescIDs(ms);
    size_t dsc_id = *dsc_ids.begin();

    casa::ROScalarColumn<casa::Double> c_times(ms,
					       casa::MS::columnName(casa::MS::TIME));    
    casa::ROScalarColumn<casa::Int> c_states(ms,
					     casa::MS::columnName(casa::MS::STATE_ID));    
    casa::ROScalarColumn<casa::Int> c_field(ms,
					    casa::MS::columnName(casa::MS::FIELD_ID));    

    casa::ROScalarColumn<casa::Int> c_desc_id(ms,
					      casa::MS::columnName(casa::MS::DATA_DESC_ID));    
    casa::ROScalarColumn<casa::Int> a1(ms,
				       casa::MS::columnName(casa::MS::ANTENNA1));

    casa::ROArrayColumn<casa::Bool> c_flags(ms,
					    casa::MS::columnName(casa::MS::FLAG));

    std::map<size_t, size_t> srcmap=getFieldSrcMap(ms);

    times.resize(0);
    states.resize(0);

    double prev_time=0.;

    const size_t nrows=c_desc_id.nrow();    
    for(size_t ii=0; ii<nrows; ++ii)
    {

      size_t i = sortedI[ii];

      if (c_times(i)>prev_time and  // only one entry per time stamp
	  c_desc_id(i)==(int)dsc_id 
	  )
      {
	prev_time = c_times(i);

	bool haveUnflaggedWvrData=false;
	size_t iii=ii;
	while(iii<nrows && c_times(sortedI[iii])==prev_time)
	{
	  // while in this timestamp, check if there is unflagged WVR data
	  if(c_desc_id(sortedI[iii])==(int)dsc_id and 
	     casa::allEQ(casa::False, c_flags(sortedI[iii]))) // i.e. not flagged
	  {
	    haveUnflaggedWvrData=true;
	    break;
	  }
	  iii++;
	}
	if(haveUnflaggedWvrData)
	{
	  times.push_back(c_times(i));
	  states.push_back(c_states(i));
	  field.push_back(c_field(i));
#if __GNUC__ <= 4 and __GNUC_MINOR__ < 1
	  source.push_back(srcmap[(c_field(i))]);
#else
	  source.push_back(srcmap.at(c_field(i)));
#endif
	}
      }
    }
  }

  void loadPointing(const casa::MeasurementSet &ms,
		    std::vector<double> &time,
		    std::vector<double> &az,
		    std::vector<double> &el)
  {
    const casa::MSPointing &ptable=ms.pointing();
    const casa::ROMSPointingColumns ptablecols(ptable);
    const casa::ROArrayColumn<casa::Double> &dir=ptablecols.direction();
    const casa::ROScalarColumn<casa::Double> &ptime=ptablecols.time();

    const size_t n=ptime.nrow();
    if(n==0){
      throw LibAIR::MSInputDataError("Didn't find any POINTING data points");
    }

    time.resize(n); 
    az.resize(n); 
    el.resize(n);
    for(size_t i=0; i<n; ++i)
    {
      time[i]=ptime(i);
      casa::Array<casa::Double> a;
      dir.get(i, a,
	      casa::True);
      az[i]=a(casa::IPosition(2,0,0));
      el[i]=a(casa::IPosition(2,1,0));
    }
  }

  /** Get the nearest pointing record to each WVR observation
   */
  void WVRNearestPointing(const casa::MeasurementSet &ms,
			  const std::vector<double> &time,
			  std::vector<double> &az,
			  std::vector<double> &el)
  {

    std::vector<double> ptime, paz, pel;
    bool noPointing = false;
    try{
      loadPointing(ms,
		   ptime,
		   paz,
		   pel);
    }
    catch(const std::runtime_error rE){
      std::cerr << std::endl << "WARNING: problem while accessing POINTING table:"
		<< std::endl << "         LibAIR::WVRNearestPointing: " << rE.what() << std::endl;
      std::cout << std::endl << "WARNING: problem while accessing POINTING table:"
		<< std::endl << "         LibAIR::WVRNearestPointing: " << rE.what() << std::endl;
      std::cerr << "Will try to continue using constant AZ=0, EL= 60 deg." << std::endl;
      std::cout << "Will try to continue using constant AZ=0, EL= 60 deg." << std::endl;
      noPointing = true;
    }

    size_t wrows=time.size();
    size_t prows=ptime.size();
    
    az.resize(wrows);  
    el.resize(wrows);

    size_t pi=0;

    for (size_t wi=0; wi<wrows; ++wi)
    {
      if(noPointing){
	az[wi]=0.;
	el[wi]=60./180.*3.141592;
      }
      else{
	while(pi<(prows-1) and  ptime[pi]<time[wi])
	  ++pi;
	az[wi]=paz[pi];
	el[wi]=pel[pi];
      }
    }

  }
			  

  InterpArrayData *loadWVRData(const casa::MeasurementSet &ms, std::vector<size_t>& sortedI, 
			       std::set<int>& flaggedantsInMain, double requiredUnflaggedFraction)
  {
    std::set<size_t> dsc_ids=WVRDataDescIDs(ms);
    AntSet wvrants=WVRAntennas(ms);
    const size_t nWVRs=wvrants.size();

    if(requiredUnflaggedFraction<0.){
      std::cout << "WARNING: Negative required fraction of unflagged data points. Will assume 0." << std::endl;
      requiredUnflaggedFraction=0.;
    }
    if(requiredUnflaggedFraction>1.){
      std::cout << "WARNING: Required fraction of unflagged data points > 1. Will assume 1." << std::endl;
      requiredUnflaggedFraction=1.;
    }

    casa::ROScalarColumn<casa::Double> maintime(ms, 
						casa::MS::columnName(casa::MS::TIME)); 

    const size_t nrows=maintime.nrow();

    sortedI.resize(nrows);
    flaggedantsInMain.clear();

    {
      casa::Vector<casa::uInt> sortedIV(nrows);
      casa::Vector<casa::Double> mainTimesV = maintime.getColumn();
      casa::GenSortIndirect<casa::Double>::sort(sortedIV,mainTimesV);
      for(casa::uInt i=0; i<nrows; i++){ // necessary for type conversion 
	sortedI[i] = (size_t) sortedIV(i); 
      }
    }
    
    std::cout << "Multi-MS (MMS) capable version using time sorted access and respecting flags." << std::endl;

    std::vector<double> times, az, el;
    std::vector<size_t> states, fields, source;
    WVRTimeStatePoints(ms,
		       times,
		       states,
		       fields,
		       source,
		       sortedI); 

    if (times.size() == 0)
      throw LibAIR::MSInputDataError("Didn't find any WVR data points");
    
    WVRNearestPointing(ms, times, az, el);
      

    std::auto_ptr<InterpArrayData> 
      res(new InterpArrayData(times, 
			      el,
			      az,
			      states,
			      fields,
			      source,
			      nWVRs));


    // This holds how far we've filled in for each of the antennas

    //std::vector<size_t> counters(nWVRs, 0);
    int counter = -1;

    std::vector<size_t> nunflagged(nWVRs, 0);
    std::vector<size_t> ntotal(nWVRs, 0);

    casa::ROArrayColumn<casa::Complex> indata(ms, 
					      casa::MS::columnName(casa::MS::DATA));
    casa::ROScalarColumn<casa::Int> indsc_id(ms,
					      casa::MS::columnName(casa::MS::DATA_DESC_ID));
    casa::ROScalarColumn<casa::Int> a1(ms,
				       casa::MS::columnName(casa::MS::ANTENNA1));
    casa::ROScalarColumn<casa::Int> a2(ms,
				       casa::MS::columnName(casa::MS::ANTENNA2));

    casa::ROArrayColumn<casa::Bool> inflags(ms,
					    casa::MS::columnName(casa::MS::FLAG));

    casa::ROScalarColumn<casa::Double> c_times(ms,
					       casa::MS::columnName(casa::MS::TIME));  

    double prevtime=0;

    for(size_t ii=0; ii<nrows; ++ii)
    {
      size_t i = sortedI[ii];

      if (a1(i) == a2(i) and 
	  dsc_ids.count(indsc_id(i)) > 0)
      {

	int newtimestamp = 0;
	if(c_times(i)>prevtime)
	{
	  prevtime = c_times(i); 
	  newtimestamp = 1;
	}
	
	if(c_times(i)==times[counter+newtimestamp]) // there is data for this timestamp
	{

	  if(newtimestamp==1)
	  {
	    counter++;
	  }

	  ntotal[a1(i)]++;

	  casa::Array<casa::Bool> fl;
	  inflags.get(i, fl, ::casa::True);
 
	  if(casa::allEQ(casa::False, inflags(i))) // i.e. not flagged
	  {
	    casa::Array<std::complex<float> > a;
	    indata.get(i,a, casa::True);

	    for(size_t k=0; k<4; ++k)
	    {
	      res->set(counter,
		       a1(i),
		       k,
		       a(casa::IPosition(2,k,0)).real());
	    }
	    nunflagged[a1(i)]++;
	  }
	  else
	  {
	    for(size_t k=0; k<4; ++k)
	    {
	      res->set(counter,
		       a1(i),
		       k,
		       0.);
	    }
	  }
	} // end if c_times ...
      }
    } // end for ii


    bool allFlagged=true;
    for(size_t i=0; i<nWVRs; i++)
    {
      if(nunflagged[i]>0)
      {
	allFlagged=false;
	break;
      }
    }
    if(!allFlagged)
    {
      allFlagged = true;
      for(size_t i=0; i<nWVRs; i++)
      {
	if(nunflagged[i]==0)
	{
	  std::cout << "All WVR data points for antenna " << i << " are flagged." << std::endl;
	  flaggedantsInMain.insert(i);
	}
	else if(nunflagged[i]<requiredUnflaggedFraction * ntotal[i])
	{
	  std::cout << "The fraction of good (unflagged) WVR data points for antenna " << i
		    << " is " << nunflagged[i] << " out of " << ntotal[i]
		    << ". This is below the required " << requiredUnflaggedFraction*100.
		    << "%. Antenna will be flagged." << std::endl;
	  flaggedantsInMain.insert(i);
	}
	else{
	  allFlagged=false;
	}
      }
      if(allFlagged)
      {
	throw LibAIR::MSInputDataError("All antennas needed to be flagged.");
      }
    }
    else
    {
      throw LibAIR::MSInputDataError("All WVR data points are flagged.");
    }


    return res.release();
    
  }
  


}

