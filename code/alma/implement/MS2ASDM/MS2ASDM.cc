//# MS2ASDM.cc 
//#
//#  ALMA - Atacama Large Millimeter Array
//#  (c) European Southern Observatory, 2002
//#  (c) Associated Universities Inc., 2002
//#  Copyright by ESO (in the framework of the ALMA collaboration),
//#  Copyright by AUI (in the framework of the ALMA collaboration),
//#  All rights reserved.
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
#include <alma/MS2ASDM/MS2ASDM.h>
#include <tables/Tables/ExprNode.h>
#include <tables/Tables/RefRows.h>
#include <ms/MeasurementSets/MSColumns.h>
#include <casa/Quanta/MVBaseline.h>
#include <casa/Arrays/Matrix.h>
#include <casa/Arrays/Cube.h>
#include <casa/Arrays/ArrayMath.h>
#include <casa/Arrays/ArrayOpsDiffShapes.h>
#include <casa/Arrays/ArrayLogical.h>
#include <casa/Arrays/ArrayUtil.h>
#include <casa/Arrays/IPosition.h>
#include <casa/Arrays/Slice.h>
#include <casa/Logging/LogIO.h>
#include <casa/OS/File.h>
#include <casa/OS/HostInfo.h>
#include <casa/Containers/Record.h>
#include <casa/BasicSL/String.h>
#include <casa/Utilities/Assert.h>
#include <casa/Utilities/GenSort.h>
#include <casa/System/AppInfo.h>
#include <tables/Tables/IncrementalStMan.h>
#include <tables/Tables/ScalarColumn.h>
#include <tables/Tables/ScaColDesc.h>
#include <tables/Tables/SetupNewTab.h>
#include <tables/Tables/StandardStMan.h>
#include <tables/Tables/Table.h>
#include <tables/Tables/TableDesc.h>
#include <tables/Tables/TableInfo.h>
#include <tables/Tables/TableLock.h>
#include <tables/Tables/TableRecord.h>
#include <tables/Tables/TableCopy.h>
#include <tables/Tables/TableRow.h>
#include <tables/Tables/TiledColumnStMan.h>
#include <tables/Tables/TiledShapeStMan.h>
#include <tables/Tables/TiledDataStMan.h>
#include <tables/Tables/TiledStManAccessor.h>
#include <casa/sstream.h>
#include <casa/iomanip.h>

#include <algorithm>
#include <casa/OS/Directory.h>
#include <alma/ASDM/ASDMAll.h>
#include <alma/ASDMBinaries/SDMDataObjectWriter.h>
#include <alma/ASDMBinaries/SDMDataObject.h>

using asdm::ASDM;
using asdm::TagType;
using namespace asdmbinaries;

namespace casa {
  
  MS2ASDM::MS2ASDM(MeasurementSet& ms) :
    ROMSColumns(ms),
    ms_p(ms),
    baseUid_p("uid://X0/X0/X"),
    runningId_p(0),
    currentUid_p("uid://X0/X0/X0"),
    telName_p(""),
    // the Id to Tag maps
    asdmStationId_p(Tag()),
    asdmAntennaId_p(Tag()),
    asdmSpectralWindowId_p(Tag()),
    asdmPolarizationId_p(Tag()),
    asdmProcessorId_p(Tag()),
    asdmFieldId_p(Tag()),
    asdmEphemerisId_p(Tag()),
    asdmDataDescriptionId_p(Tag()),
    asdmStateId_p(Tag()),
    asdmConfigDescriptionId_p(Tag()),
    asdmSBSummaryId_p(Tag()),
    asdmExecBlockId_p(Tag()),
    // other maps
    asdmFeedId_p(-1),
    asdmSourceId_p(-1),
    asdmPointingModelId_p(-1)
  {
    ASDM_p = new ASDM();
    asdmVersion_p = String((ASDM_p->getEntity()).getEntityVersion());
    setSBDuration(); // set to default values
    setSubScanDuration(); 
    setDataAPCorrected();
    setVerbosity();
  }
  
  MS2ASDM::~MS2ASDM()
  {
    delete ASDM_p;
  }
  
  const String& MS2ASDM::showversion()
  {

    static String rstr = String(asdmVersion_p);
    if(rstr==""){
      ASDM* myASDM = new ASDM();
      rstr = String((myASDM->getEntity()).getEntityVersion());
      delete myASDM;
    }
    return rstr;
  }
  
  void MS2ASDM::setBaseUid(const String& baseuid){ baseUid_p = baseuid; }

  const String& MS2ASDM::getBaseUid(){ return baseUid_p; }

  const std::string& MS2ASDM::getCurrentUid(){ 
    static std::string str;
    str = string(currentUid_p.c_str());
    return str; 
  }

  const std::string& MS2ASDM::getCurrentUidAsFileName(){ 
    static std::string str;
    str = string(currentUid_p.c_str());
    std::replace( str.begin(), str.end(), ':', '_' );
    std::replace( str.begin(), str.end(), '/', '_' );

    return str; 
  }

  Bool MS2ASDM::incrementUid()
  {
    static char cstr[20];
    runningId_p++;
    if(sprintf(cstr,"%x", runningId_p) > 0){
      currentUid_p = baseUid_p + String(cstr);
      return True;
    }
    else{
      runningId_p--;
      return False;
    }
  }

  Bool MS2ASDM::writeASDM(const String& asdmfile, const String& datacolumn, 
			  const String& archiveid, const String& rangeid, Bool verbose,
			  const Double subscanDuration, const Double schedBlockDuration, 
			  const Bool msDataIsAPCorrected)
  {

    LogIO os(LogOrigin("MS2ASDM", "writeASDM()"));

    setBaseUid("uid://"+archiveid+"/"+rangeid+"/X");

    if(!incrementUid()){// need to increment before first use
      os << LogIO::SEVERE << "Error generating UID"
	 << LogIO::POST;      
      return False;
    }
      
    // initialize observatory name
    if(observation().nrow()==0){
      os << LogIO::SEVERE << "MS Observation table is empty." << LogIO::POST;
      return False;
    }

    setObservatoryName(observation().telescopeName()(0)); // get name of observatory from first row of observation table

    if(subscanDuration<1.){
      os << LogIO::SEVERE << "Input error: Unreasonably short sub scan duration limit: " << subscanDuration  
	 << " seconds." << LogIO::POST;
      return False;
    }
     
    if(schedBlockDuration<60.){
      os << LogIO::SEVERE << "Input error: Unreasonably short scheduling block duration limit: " << schedBlockDuration  
	 << " seconds. (Should be >= 60 s)" << LogIO::POST;
      return False;
    }
     
    setSubScanDuration(subscanDuration);

    setSBDuration(schedBlockDuration);

    setDataAPCorrected(msDataIsAPCorrected);

    if(verbose){
      //      setVerbosity(1);
      setVerbosity(2); // set to 1 before releasing
    }
    else{
      setVerbosity(0);
    }

    if(verbosity_p>0){
      os << LogIO::NORMAL << "Converting " << ms_p.tableName() << " to ASDM " << asdmfile
	 << LogIO::POST;
    }

    if(verbosity_p>1){
      cout << "Base uid is " << getBaseUid() << endl;
      cout << " UID is " << getCurrentUid() << endl;
    }

    // write the ASDM tables

    if(!writeStation()){
      return False;
    }

    if(!writeAntenna()){
      return False;
    }

    if(!writeSpectralWindow()){
      return False;
    }

    if(!writeSource()){
      return False;
    }

    if(!writePolarization()){
      return False;
    }

    if(!writeProcessor()){
      return False;
    }

    if(!writeField()){
      return False;
    }

    if(!writeReceiver()){
      return False;
    }

    if(!writeFeed()){
      return False;
    }

    if(!writeDataDescription()){
      return False;
    }

    if(!writeSwitchCycle()){
      return False;
    }

    if(!writeState()){
      return False;
    }

    if(!writeSysCal()){
      return False;
    }

    if(!writeConfigDescription()){
      return False;
    }

    if(!writeSBSummaryAndExecBlockStubs()){ 
      return False;
    }

    // prepare the writing of binary data
    if(!setDirectory(asdmfile)){
       return False;
    }

    if(!writeMainAndScanAndSubScan(datacolumn)){ 
      return False;
    }

    if(!writePointingModel()){
      return False;
    }

    if(!writePointing()){
      return False;
    }

    // finish writing the ASDM non-binary data
    try{
      ASDM_p->toFile(asdmfile);
    }
    catch(ConversionException x){
      os << LogIO::SEVERE << "Error writing ASDM non-binary data:" << x.getMessage()
	 << LogIO::POST;      
      return False;
    }     

    return True;

  }

  Bool MS2ASDM::setDirectory(const String& asdmfile){

    LogIO os(LogOrigin("MS2ASDM", "setDirectory()"));

    // create ASDM output directories
    Directory asdmDir(asdmfile);
    // create if it doesn't exist
    try{
      asdmDir.isEmpty(); // throws if dir doesn't exist
    }
    catch(AipsError x){
      try{
	asdmDir.create(False); // do not overwrite
      }
      catch(AipsError y){
	os << LogIO::SEVERE << "Error creating ASDM top directory: " << y.getMesg()
	   << LogIO::POST;      
	return False;
      }
    }
    // create ASDM binary output directory
    Directory asdmDir2(asdmfile+"/ASDMBinary");
    try{
      asdmDir2.create(False); // do not overwrite
    }
    catch(AipsError x){
      os << LogIO::SEVERE << "Error creating ASDMBinary directory: " << x.getMesg()
	 << LogIO::POST;      
      return False;
    }
    asdmDir_p = asdmfile;
    return True;
  }


  Int MS2ASDM::writeMainBinSubScanForOneDDIdFIdPair(const Int theDDId, const Int theFieldId, 
						    const String& datacolumn, 
						    const uInt theScan, const uInt theSubScan,
						    const uInt startRow, const uInt endRow,
						    const Tag eBlockId,
						    int& datasize, asdm::EntityRef& dataOid, 
						    vector< asdm::Tag >& stateIdV){

    LogIO os(LogOrigin("MS2ASDM", "writeMainBinForOneDDIdAndSubScan()"));

    // return values
    Int numIntegrations = 0;
    datasize = 0;
    dataOid = EntityRef();
    stateIdV.resize(0);

    asdm::ExecBlockRow* eBlockRow = (ASDM_p->getExecBlock()).getRowByKey(eBlockId);
    string eBlockUID = eBlockRow->getExecBlockUID().toString();
    int eBlockNum = eBlockRow->getExecBlockNum();

    Bool warned=False; // use later to avoid repetitive warnings

    try{

      uInt nMainTabRows = ms_p.nrow();

      unsigned int subscanNum = theSubScan;

      if(startRow>=nMainTabRows){
	os << LogIO::SEVERE << "Internal error: startRow " << startRow << " exceeds end of MS."
	   << LogIO::POST;
	return -1;
      }

      if(endRow>=nMainTabRows){
	os << LogIO::SEVERE << "Internal error: endRow " << endRow << " exceeds end of MS."
	   << LogIO::POST;
	return -1;
      }

      Int DDId = dataDescId()(startRow);
      if(DDId != theDDId){ // check Data Description Id
	os << LogIO::SEVERE << "Internal error: input parameters to this routine are inconsistent.\n"
	   << " DDId in start row should be as given in input parameters ==" << theDDId << LogIO::POST;
	return -1;
      }
      
      Int FId = fieldId()(startRow);
      if(FId != theFieldId){ // check Field Id
	os << LogIO::SEVERE << "Internal error: input parameters to this routine are inconsistent.\n"
	   << " FieldId in start row should be as given in input parameters ==" << theFieldId << LogIO::POST;
	return -1;
      }

      if(verbosity_p>1){
	os << LogIO::NORMAL << "Writing Main table entries for DataDescId " << DDId 
	   << ", Field Id " << FId << ", ExecBlock " << eBlockNum << ", Scan number " << theScan
	   << ", SubScan number " << theSubScan << LogIO::POST;
      }

      //////////////////////
      // Construct subscan == SDMDataObject
      
      // Assume MS main table is sorted in time. 
      
      // Get first timestamp.
      Double subScanStartTime = time()(startRow);
      Double subScanEndTime = time()(endRow); 
      
      // determine number of different timestamps in this subscan 
      unsigned int numTimestampsCorr = 0;
      unsigned int numTimestampsAuto = 0;
      for(uInt i=startRow; i<=endRow; i++){

	DDId = dataDescId()(i);
	if(DDId != theDDId){ // skip all other Data Description Ids
	  continue;
	}
	FId = fieldId()(i);
	if(FId != theFieldId){ // skip all other Field Ids
	  continue;
	}
	
	if(antenna1()(i)==antenna2()(i)){
	  numTimestampsAuto++;
	}
	else{
	  numTimestampsCorr++;
	}
      } // end for

      if(verbosity_p>1){
	cout << " subscan number is " << subscanNum << endl;
	cout << " DDId " << theDDId << " FId " << theFieldId << endl;
	cout << " start row " << startRow << " end row " << endRow << endl;
	cout << "  subscan number of timestamps with crosscorrelations is " << numTimestampsCorr << endl;
	cout << "  subscan number of timestamps with autocorrelations is " << numTimestampsAuto << endl;
	cout << "  subscan end time is " << subScanEndTime << endl;
      }
      
      // open disk file for subscan
      String subscanFileName = asdmDir_p+"/ASDMBinary/"+String(getCurrentUidAsFileName());
      if(verbosity_p>1){
	cout << "  subscan filename is " << subscanFileName << endl;
      }
      try{
	dataOid = EntityRef(getCurrentUid(),"","ASDM",asdmVersion_p); 
      }
      catch(asdm::InvalidArgumentException x){
	os << LogIO::SEVERE << "Error creating ASDM:  UID \"" << getCurrentUid() 
	   << "\" (intended for a BLOB) is not a valid Entity reference: " <<  x.getMessage()
	   << LogIO::POST;      
	return -1;
      }
	
      // make sure that this file doesn't exist yet
      ofstream ofs(subscanFileName.c_str());
      
      // set up SDMDataObjectWriter
      
      SDMDataObjectWriter sdmdow(&ofs, getCurrentUid()); // use default title
      
      unsigned long long startTime = (unsigned long long) floor(subScanStartTime);
      unsigned int execBlockNum = eBlockNum; // constant for all scans
      unsigned int scanNum = theScan; // ASDM scan numbering starts at 1
      if(scanNum == 0 && !warned){
	os << LogIO::WARN << "Scan Number is 0. Note that by convention scan numbers in ASDMs should start at 1." << LogIO::POST;
	warned = True;
      }
      
      // determine actual number of baselines and antennas
      // assume that the first timestamp has complete information
      // (at the same time also fill the stateId vector)
      unsigned int numAutoCorrs = 0;
      unsigned int numBaselines = 0;
      unsigned int numAntennas = 0;
      vector<Int> ant;
      {
	uInt i = startRow;
	Double thisTStamp = time()(i); 
	while(i<nMainTabRows && time()(i)== thisTStamp){
	  
	  DDId = dataDescId()(i);
	  if(DDId != theDDId){ // skip all other Data Description Ids
	    i++;
	    continue;
	  }
	  FId = fieldId()(i);
	  if(FId != theFieldId){ // skip all other Field Ids
	    i++;
	    continue;
	  }
	  
	  if(antenna1()(i) == antenna2()(i)){
	    numAutoCorrs++;
	  }
	  else{
	    numBaselines++;
	  }
	  
	  Bool found = False;
	  for(uInt j=0;j<ant.size();j++){
	    if(antenna1()(i) == ant[j]){
	      found = True;
	      break;
	    }
	  }
	  if(!found){
	    ant.push_back(antenna1()(i));
	    stateIdV.push_back( asdmStateId_p(stateId()(i)) );
	  }
	  found = False;
	  for(uInt j=0;j<ant.size();j++){
	    if(antenna2()(i) == ant[j]){
	      found = True;
	      break;
	    }
	  }
	  if(!found){
	    ant.push_back(antenna2()(i));
	    stateIdV.push_back( asdmStateId_p(stateId()(i)) );
	  }
	  i++;
	}
      }
      numAntennas = ant.size();
      
      CorrelationMode correlationMode;
      if(numAutoCorrs==0){
	correlationMode = CorrelationModeMod::CROSS_ONLY;
      }
      else if(numBaselines>0){
	correlationMode = CorrelationModeMod::CROSS_AND_AUTO;
      }
      else{
	correlationMode = CorrelationModeMod::AUTO_ONLY;
      }
      // if datacolumn is FLOAT_DATA make correlation mode == AUTO as well!
      
      
      vector< AtmPhaseCorrection >  apc;
      vector< SDMDataObject::Baseband > basebands; // ???
      
      // construct spectral window and basedband vectors
      
      vector<SDMDataObject::SpectralWindow> spectralWindows;
      
      // for creating a Spectral Window
      uInt PolId = dataDescription().polarizationId()(theDDId);
      uInt numStokesMS = polarization().numCorr()(PolId);

      asdm::PolarizationRow* PR = (ASDM_p->getPolarization()).getRowByKey(asdmPolarizationId_p(PolId));
      uInt numStokes = PR->getNumCorr();
      Array<Int> corrT = polarization().corrType()(PolId);
      vector<StokesParameter> crossPolProducts = PR->getCorrType();

      vector<StokesParameter> sdPolProduct;
      float scaleFactor = 1.;
      uInt spwId = dataDescription().spectralWindowId()(theDDId);
      unsigned int numSpectralPoint = spectralWindow().numChan()(spwId);

      OptionalSpectralResolutionType spectralResolution = SpectralResolutionTypeMod::FULL_RESOLUTION; 
      if(numSpectralPoint<5){
	spectralResolution = SpectralResolutionTypeMod::CHANNEL_AVERAGE;
	if(verbosity_p>0){
	  os << LogIO::NORMAL << "    Less than 5 channels. Assuming data is of spectral resolution type \"CHANNEL_AVERAGE\"." << LogIO::POST;      
	}
      } 

      unsigned int numBin = 1; // number of switch cycles
      NetSideband sideband = CNetSideband::from_int(spectralWindow().netSideband()(spwId));
      
      SDMDataObject::SpectralWindow spw(crossPolProducts,
					sdPolProduct,
					scaleFactor,
					numSpectralPoint,
					numBin,
					sideband);
      
      spectralWindows.push_back(spw);
      
      
      SDMDataObject::Baseband bband(BasebandNameMod::NOBB, spectralWindows); // how to select name ???? -> Francois
      // note: for ALMA only four basebands exist, not easy to find baseband for a given SPW
      
      basebands.push_back(bband);
      
      // construct binary parts for dataStruct
      unsigned int bpFlagsSize = numSpectralPoint * numStokes * (numBaselines+numAutoCorrs);
      vector<AxisName> bpFlagsAxes;
      bpFlagsAxes.push_back(AxisNameMod::TIM); // order: inner part of loop should be last!!!!!!
      bpFlagsAxes.push_back(AxisNameMod::SPP); 
      bpFlagsAxes.push_back(AxisNameMod::POL);
      SDMDataObject::BinaryPart bpFlags(bpFlagsSize, bpFlagsAxes);
      if(verbosity_p>1){
	cout << "FlagsSize " << bpFlagsSize << " numStokes " << numStokes << endl;
      }
      
      unsigned int bpTimesSize = 0; // only needed for data blanking
      //unsigned int bpTimesSize = numTimestampsCorr+numTimestampsAuto; 
      vector<AxisName> bpTimesAxes;
      bpTimesAxes.push_back(AxisNameMod::TIM);
      SDMDataObject::BinaryPart bpActualTimes(bpTimesSize, bpTimesAxes);
      //      cout << "TimesSize " << bpTimesSize << endl;
      
      unsigned int bpDurSize = 0; // only needed for data blanking
      //	unsigned int bpDurSize = numTimestampsCorr+numTimestampsAuto;
      vector<AxisName> bpDurAxes;
      bpDurAxes.push_back(AxisNameMod::TIM);
      SDMDataObject::BinaryPart bpActualDurations(bpDurSize, bpDurAxes);
      //      cout << "DurSize " << bpDurSize << endl;
      
      unsigned int bpLagsSize = 0; // not filled for the moment (only useful if LAG_DATA column present) -> Francois 
      vector<AxisName> bpLagsAxes;
      bpLagsAxes.push_back(AxisNameMod::SPP); // ******
      SDMDataObject::ZeroLagsBinaryPart bpZeroLags(bpLagsSize, bpLagsAxes, CorrelatorTypeMod::FXF); // ALMA default
      if(telName_p != "ALMA"){
	if(telName_p == "ACA" || telName_p == "VLBA" || telName_p == "EVLA"){
	  bpZeroLags = SDMDataObject::ZeroLagsBinaryPart(bpLagsSize, bpLagsAxes, CorrelatorTypeMod::FX);	  
	}
	else{
	  bpZeroLags = SDMDataObject::ZeroLagsBinaryPart(bpLagsSize, bpLagsAxes, CorrelatorTypeMod::XF);
	}
      }
      //      cout << "LagsSize " << bpLagsSize << endl;
      
      unsigned int bpCrossSize = numSpectralPoint * numStokes * numBaselines * 2; // real + imag
      vector<AxisName> bpCrossAxes;
      bpCrossAxes.push_back(AxisNameMod::TIM);
      bpCrossAxes.push_back(AxisNameMod::BAL);
      bpCrossAxes.push_back(AxisNameMod::SPP); 
      bpCrossAxes.push_back(AxisNameMod::POL);
      SDMDataObject::BinaryPart bpCrossData(bpCrossSize, bpCrossAxes);
      //      cout << "CrossSize " << bpCrossSize << endl;
      
      unsigned int bpAutoSize = numSpectralPoint * numStokes * numAutoCorrs;
      vector<AxisName> bpAutoAxes;
      bpAutoAxes.push_back(AxisNameMod::TIM);
      bpAutoAxes.push_back(AxisNameMod::ANT);
      bpAutoAxes.push_back(AxisNameMod::SPP); 
      bpAutoAxes.push_back(AxisNameMod::POL);
      SDMDataObject::AutoDataBinaryPart bpAutoData(bpAutoSize, bpAutoAxes, False); // not normalised
      //      cout << "AutoSize " << bpAutoSize << endl;
      
      SDMDataObject::DataStruct dataStruct( apc, basebands, bpFlags, bpActualTimes, bpActualDurations, 
					    bpZeroLags, bpCrossData, bpAutoData);	 
      
      // Parameters of dataStruct:
      //     	apc 	        a vector of AtmPhaseCorrection. If apc is not relevant pass an empty vector.
      //     	basebands 	a vector of Baseband.
      //     	flags 	        a BinaryPart object describing the flags. If flags is not relevant pass an 
      //                      empty BinaryPart object.
      //     	actualTimes 	a BinaryPart object describing the actual times. If actualTimes is not 
      //                      relevant pass an empty BinaryPart object.
      //     	actualDurations a BinaryPart object describing the actual durations. If actualDurations is 
      //                      not relevant pass an empty BinaryPart object.
      //     	zeroLags 	a ZeroLagsBinaryPart object describing the zero lags. If zeroLags is not 
      //                      relevant pass an empty ZeroLagsBinaryPart object.
      //     	crossData 	a BinaryPart object describing the cross data. If crossData is not relevant 
      //                      pass an empty BinaryPart object.
      //     	autoData 	an AutoDataBinaryPart object describing the auto data. If autoData is not 
      //                      relevant pass an empty AutoDataBinaryPart object. 
      
      
      // Write the global header.
      sdmdow.corrDataHeader(startTime,
			    eBlockUID,
			    execBlockNum,
			    scanNum,
			    subscanNum,
			    numAntennas,        
			    correlationMode,    // the correlation mode.
			    spectralResolution, // the spectral resolution.
			    dataStruct);        // the description of the structure of the binary data.
      
      //////////////////////////////////////////////////////
      // write the integrations until timestamp exceeds limit
      unsigned int integrationNum = 1;
      uInt mainTabRow=startRow; 

      while(mainTabRow <= endRow){
	
	DDId = dataDescId()(mainTabRow);
	if(DDId != theDDId){ // skip all other Data Description Ids
	  mainTabRow++;
	  continue;
	}
	FId = fieldId()(mainTabRow);
	if(FId != theFieldId){ // skip all other Field Ids
	  mainTabRow++;
	  continue;
	}
	
	unsigned long long timev = (unsigned long long) floor((time()(mainTabRow))*1E9); // what units? nanoseconds
	unsigned long long intervalv = (unsigned long long) floor(interval()(mainTabRow)*1E9);
	vector< unsigned long > flags;
	vector< long long > actualTimes;
	vector< long long > actualDurations;
	vector< float > zeroLags; // LAG_DATA, optional column, not used for the moment
	vector< float > crossData;
	// vector< short > crossData;
	// vector< int > crossData; // standard case for ALMA
	vector< float > autoData;	 
	
	////////////////////////////////////////////////////////
	// fill data, time, and flag vectors for this timestamp
	Double theTStamp = time()(mainTabRow);
	
	// SORT the data by baseline and antenna resp.!!!!!!!!!!!!
	
	while(mainTabRow < nMainTabRows 
	      && time()(mainTabRow)==theTStamp){
	  
	  DDId = dataDescId()(mainTabRow);
	  if(DDId != theDDId){ // skip all other Data Description Ids
	    mainTabRow++;
	    continue;
	  }
	  FId = fieldId()(mainTabRow);
	  if(FId != theFieldId){ // skip all other Field Ids
	    mainTabRow++;
	    continue;
	  }
	  
	  Matrix<Complex> dat;
	  Matrix<Bool> flagsm;
	  if(datacolumn == "MODEL"){
	    dat.reference(modelData()(mainTabRow));
	  }
	  else if(datacolumn == "CORRECTED"){
	    dat.reference(correctedData()(mainTabRow));
	  }
	  else{
	    dat.reference(data()(mainTabRow));
	  }
	  flagsm.reference(flag()(mainTabRow));
	  
	  if(antenna1()(mainTabRow) == antenna2()(mainTabRow)){
	    Complex x;  
	    float a;
	    for(uInt i=0; i<numSpectralPoint; i++){
	      for(uInt j=0; j<numStokesMS; j++){
		if(skipCorr_p[j][PolId]){
		  continue;
		}
		else{
		  x = dat(j,i);
		  a = x.real();
		  autoData.push_back( a );
		}
	      }
	    }
	  }
	  else{
	    Complex x;  
	    float a,b;
	    // int a,b;
	    for(uInt i=0; i<numSpectralPoint; i++){
	      for(uInt j=0; j<numStokesMS; j++){
		if(skipCorr_p[j][PolId]){
		  continue;
		}
		else{
		  x = dat(j,i);
		  a = x.real();
		  b = x.imag();
		  crossData.push_back( a );
		  crossData.push_back( b );
		}
	      }
	    }
	  }	
	  unsigned long ul;
	  for(uInt i=0; i<numSpectralPoint; i++){
	    for(uInt j=0; j<numStokesMS; j++){
	      if(skipCorr_p[j][PolId]){
		continue;
	      }
	      else{
		if(flagRow()(mainTabRow)){
		  ul = 1;
		}
		else{
		  ul = flagsm(j,i);
		}
		flags.push_back( ul );
	      }
	    }
	  }
	  // the following two lines only needed for data blanking
	  //	    actualTimes.push_back((long long)floor(time()(mainTabRow))*1000.);
	  //	    actualDurations.push_back((long long)floor(interval()(mainTabRow))*1000.);
	  mainTabRow++;
	}// end while same timestamp
	
	if(verbosity_p>1){
	  cout << "Sizes: " << endl;
	  cout << "   flags " << flags.size() << endl;
	  cout << "   actualTimes " << actualTimes.size() << endl;
	  cout << "   actualDurations " << actualDurations.size() << endl;
	  cout << "   zeroLags " << zeroLags.size() << endl;
	  cout << "   crossData " << crossData.size() << endl;
	  cout << "   autoData " << autoData.size() << endl;
	}

	sdmdow.addIntegration(integrationNum,    // integration's index.
			      timev,             // midpoint
			      intervalv,         // time interval
			      flags,             // flags binary data 
			      actualTimes,       // actual times binary data      
			      actualDurations,   // actual durations binary data          
			      zeroLags,          // zero lags binary data                 
			      crossData,    // cross data (can be short or int)  
			      autoData);         // single dish data.  

	integrationNum++;
	datasize += flags.size() * sizeof(unsigned long)
	  + actualTimes.size() * sizeof( long long )
	  + actualDurations.size() * sizeof( long long )
	  + zeroLags.size() * sizeof( float )
	  + crossData.size() * sizeof( float )
	  + autoData.size() * sizeof( float );
	numIntegrations++;
	
	// (Note: subintegrations are used only for channel averaging to gain time res. by sacrificing spec. res.)
	
      } // end while 
      
      sdmdow.done();
      
      ofs.close();
      
      // end write subscan
      
      incrementUid();
            
    }
    catch(asdmbinaries::SDMDataObjectWriterException x){
      os << LogIO::SEVERE << "Error creating ASDM: " << x.getMessage()
	 << LogIO::POST;      
      return -1;
    }
    catch(asdmbinaries::SDMDataObjectException x){
      os << LogIO::SEVERE << "Error creating ASDM: " << x.getMessage()
	 << LogIO::POST;      
      return -1;
    }
    catch(AipsError y){
      os << LogIO::SEVERE << "Error creating ASDM: " << y.getMesg()
	 << LogIO::POST;      
      return -1;
    }      
    catch(std::string z){
      os << LogIO::SEVERE << "Error creating ASDM: " << z
	 << LogIO::POST;      
      return -1;
    }      
    catch(std::exception zz){
      os << LogIO::SEVERE << "Error creating ASDM: " << zz.what()
	 << LogIO::POST;      
      return -1;
    }      
    
    return numIntegrations;

  } // end writeMainBinForOneDDIdAndSubScan


  /////////////////////////////////////
  //   // Pattern for wrting a non-binary table:
  //   Bool MS2ASDM::writeAntenna(){ // create asdm antenna table and fill antenna id map
  //     LogIO os(LogOrigin("MS2ASDM", "writeAntenna()"));
  
  //     Bool rstat = True;
  
  //     asdm::AntennaTable& tT = ASDM_p->getAntenna();
  
  //     asdm::AntennaRow* tR = 0;
  
  //     //    tR = tT.newRow();
  
  //     tT.add(tR);
  
  //     EntityId theUid(getCurrentUid());
  //     Entity ent = tT.getEntity();
  //     ent.setEntityId(theUid);
  //     tT.setEntity(ent);
  //     if(verbosity_p>0){
  //       os << LogIO::NORMAL << "Filled Antenna table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
  //     }
  //     incrementUid();
  
  //     return rstat;
  //   }

  Bool MS2ASDM::writeStation(){ // create asdm station table and fill station id map
    LogIO os(LogOrigin("MS2ASDM", "writeStation()"));

    Bool rstat = True;

    asdm::StationTable& tT = ASDM_p->getStation();

    asdm::StationRow* tR = 0;

    // MS has no station table; get information from the antenna table

    for(uInt irow=0; irow<antenna().nrow(); irow++){

      // parameters for the new station row
      StationTypeMod::StationType type = StationTypeMod::ANTENNA_PAD; 
      // (alternatives are MAINTENANCE_PAD and WEATHER_STATION)

      // parameters of the new row
      string name = antenna().station()(irow).c_str();

      vector<Length > position; // take the antenna position (!), (later set the ASDM antenna postion to 0,0,0)
      Vector<Double > v; // aux. vector
      v.reference(antenna().position()(irow));
      for(uInt i=0; i<v.size(); i++){
	position.push_back(Length(v[i]));
      }

      Tag stationId;
      String sId = antenna().station()(irow);
      tR = tT.newRow(name, position, type);
      
      asdm::StationRow* tR2 = 0;

      tR2 = tT.add(tR);
      if(tR2 == tR){ // adding this row caused a new tag to be defined
	// enter tag into the map
	asdmStationId_p.define(String(name), tR->getStationId());
      }
      else{
	os << LogIO::SEVERE << "Duplicate station in MS Antenna table: " << irow << LogIO::POST;
	return False;
      }

    } // end for irow

    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled Station table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();

    return rstat;
  }

  Bool MS2ASDM::writeAntenna(){ // create asdm antenna table and fill antenna id map
    LogIO os(LogOrigin("MS2ASDM", "writeAntenna()"));

    Bool rstat = True;

    asdm::AntennaTable& tT = ASDM_p->getAntenna();

    asdm::AntennaRow* tR = 0;

    for(uInt irow=0; irow<antenna().nrow(); irow++){

      // parameters of the new row
      string name = antenna().name()(irow).c_str();
      AntennaMakeMod::AntennaMake antennaMake = AntennaMakeMod::UNDEFINED; // no info in MS (presently)
      AntennaTypeMod::AntennaType antennaType = ASDMAntennaType( antenna().type()(irow) );
      Length dishDiameter = Length( antenna().dishDiameter()(irow) );

      vector<Length > position;
      for(uInt i=0; i<3; i++){
	position.push_back(Length(0.)); // the antenna position in the ASDM is the sum of station pos and antenna pos.
      }

      vector<Length > offset;
      Vector<Double > v; // aux. vector
      v.reference(antenna().offset()(irow));
      for(uInt i=0; i<v.size(); i++){
	offset.push_back(Length(v[i]));
      }

      ArrayTime atime = ASDMArrayTime(time()(0)); // use start time of MS 

      Tag stationId;
      String sId = antenna().station()(irow); // the station name
      if(asdmStationId_p.isDefined(sId)){
	stationId = asdmStationId_p(sId);
      }
      else{
	os << LogIO::SEVERE << "Internal error: No station ID defined for station name " << sId << LogIO::POST;
	return False;
      }
      
      tR = tT.newRow(name, antennaMake, antennaType, dishDiameter, position, offset, atime, stationId);
      
      asdm::AntennaRow* tR2 = 0;

      tR2 = tT.add(tR);
      if(tR2 == tR){ // adding this row caused a new tag to be defined
	// enter tag into the map
	asdmAntennaId_p.define(irow, tR->getAntennaId());
      }
      else{
	os << LogIO::WARN << "Duplicate row in MS Antenna table :" << irow << LogIO::POST;
      }
    }

    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled Antenna table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();

    return rstat;
  }

  Bool MS2ASDM::writeSpectralWindow(){
    LogIO os(LogOrigin("MS2ASDM", "writeSpectralWindow()"));

    Bool rstat = True;

    asdm::SpectralWindowTable& tT = ASDM_p->getSpectralWindow();

    asdm::SpectralWindowRow* tR = 0;

    for(uInt irow=0; irow<spectralWindow().nrow(); irow++){

      // parameters of the new row
      BasebandNameMod::BasebandName basebandName = BasebandNameMod::NOBB;
      if(!spectralWindow().bbcNo().isNull()){ // BBC_NO is an optional MS column
	basebandName = ASDMBBName( spectralWindow().bbcNo()(irow) );
      }
      NetSidebandMod::NetSideband netSideband = ASDMNetSideBand( spectralWindow().netSideband()(irow) );
      int numChan = spectralWindow().numChan()(irow);

      Frequency refFreq  = Frequency( spectralWindow().refFrequencyQuant()(irow).getValue(unitASDMFreq()) );
      SidebandProcessingModeMod::SidebandProcessingMode sidebandProcessingMode = SidebandProcessingModeMod::NONE;
      Frequency totBandwidth = Frequency( spectralWindow().totalBandwidthQuant()(irow).getValue(unitASDMFreq()) );
      WindowFunctionMod::WindowFunction windowFunction = WindowFunctionMod::UNIFORM;

      tR = tT.newRow(basebandName, netSideband, numChan, refFreq, sidebandProcessingMode, totBandwidth, windowFunction);

      Vector< Quantum<Double> > v;
      vector< Frequency > chanFreqArray;
      v.reference( spectralWindow().chanFreqQuant()(irow) );
      for(uInt i=0; i<v.nelements(); i++){ 
	chanFreqArray.push_back( Frequency( v[i].getValue(unitASDMFreq()) ) );
      }
      tR->setChanFreqArray(chanFreqArray);

      vector< Frequency > chanWidthArray;
      v.reference( spectralWindow().chanWidthQuant()(irow) );
      for(uInt i=0; i<v.nelements(); i++){ 
	chanWidthArray.push_back( Frequency( v[i].getValue(unitASDMFreq()) ) );
      }
      tR->setChanWidthArray(chanWidthArray);

      vector< Frequency > effectiveBwArray;
      v.reference( spectralWindow().effectiveBWQuant()(irow) );
      for(uInt i=0; i<v.nelements(); i++){ 
	effectiveBwArray.push_back( Frequency( v[i].getValue(unitASDMFreq()) ) );
      }
      tR->setEffectiveBwArray(effectiveBwArray);

      vector< Frequency > resolutionArray;
      v.reference( spectralWindow().resolutionQuant()(irow) );
      for(uInt i=0; i<v.nelements(); i++){ 
	resolutionArray.push_back( Frequency( v[i].getValue(unitASDMFreq()) ) );
      }      
      tR->setResolutionArray( resolutionArray );

      MFrequency::Types refFrame = MFrequency::castType(spectralWindow().measFreqRef()(irow));
      FrequencyReferenceCodeMod::FrequencyReferenceCode measFreqRef = ASDMFreqRefCode( refFrame );
      tR->setMeasFreqRef( measFreqRef );

      tR->setFreqGroup( spectralWindow().freqGroup()(irow) );

      tR->setFreqGroupName( string( spectralWindow().freqGroupName()(irow).c_str() ) );	

      if(!spectralWindow().dopplerId().isNull()){ // DOPPLER_ID is an optional MS column
	tR->setDopplerId( spectralWindow().dopplerId()(irow) );
      }

      // add the row to the table
      asdm::SpectralWindowRow* tR2 = 0;
      tR2 = tT.add(tR);

      if(tR2 == tR){ // adding this row caused a new tag to be defined
	// enter tag into the map
	asdmSpectralWindowId_p.define(irow, tR->getSpectralWindowId());
      }
      else{
	os << LogIO::WARN << "Duplicate row in MS Spectral Window table :" << irow << LogIO::POST;
      }
    } // end loop over MS SPW table

    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled SpectralWindow table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();

    return rstat;
  }

  Bool MS2ASDM::writeSource(){ // create asdm source table and fill source id map
    LogIO os(LogOrigin("MS2ASDM", "writeSource()"));
    
    Bool rstat = True;
    
    asdm::SourceTable& tT = ASDM_p->getSource();
    
    asdm::SourceRow* tR = 0;
    
    if(source().isNull()){// the source table doesn't exist (it is optional)
      os << LogIO::WARN << "MS Source table doesn't exist (it is optional). Cannot create ASDM Source table."
	 << LogIO::POST;
      rstat = True; // not an error
    }
    else {
      for(uInt irow=0; irow<source().nrow(); irow++){

	Int sId = source().sourceId()(irow);

	// parameters of the new row

	ArrayTimeInterval timeInterval( ASDMTimeInterval(source().timeQuant()(irow), source().intervalQuant()(irow)) );
	Int spwId = source().spectralWindowId()(irow);
	Tag spectralWindowId;
	if(!asdmSpectralWindowId_p.isDefined(spwId)){
	  os << LogIO::SEVERE << "Undefined SPW id " << spwId << " in MS Source table row "<< irow << LogIO::POST;
	  return False;
	}
	else{
	  spectralWindowId = asdmSpectralWindowId_p(spwId);
	}
	string code = source().code()(irow).c_str();
	vector< Angle > direction;
	MDirection theSourceDir = source().directionMeas()(irow);
	direction.push_back( Angle( theSourceDir.getAngle( unitASDMAngle() ).getValue()(0) ) ); // RA
	direction.push_back( Angle( theSourceDir.getAngle( unitASDMAngle() ).getValue()(1) ) ); // DEC
	
	vector< AngularRate > properMotion;
	if(!source().properMotionQuant().isNull()){
	  Vector< Quantity > pMV;
	  pMV.reference(source().properMotionQuant()(irow));
	  try{
	    properMotion.push_back( AngularRate( pMV[0].getValue(unitASDMAngularRate()) ) );
	    properMotion.push_back( AngularRate( pMV[1].getValue(unitASDMAngularRate()) ) );
	  }
	  catch(AipsError x){
	    os << LogIO::SEVERE << "Error accessing proper motion parameters in MS Source table row " << irow 
	       << ":\n   " << x.getMesg() << LogIO::POST;
	  }
	}
	else{ 
	  properMotion.push_back( AngularRate( 0. ) );
	  properMotion.push_back( AngularRate( 0. ) );
	}	  
	
	string sourceName = (source().name()(irow)).c_str();

	tR = tT.newRow(timeInterval, spectralWindowId, code, direction, properMotion, sourceName);
    
	asdm::SourceRow* tR2;
	tR2 = tT.add(tR);
	if(tR2 != tR){ // did not lead to the creation of a new tag
	  os << LogIO::WARN << "Duplicate MS Source table row " << irow << ", source id " << sId << LogIO::POST;
	}
	if(!asdmSourceId_p.isDefined(sId)){
	  int souId = tR2->getSourceId();
	  asdmSourceId_p.define(sId, souId);
	}
      } // end loop over source table
    } // id if source table exists

    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled Source table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();
    
    return rstat;
  }



  Bool MS2ASDM::writePolarization(){
    LogIO os(LogOrigin("MS2ASDM", "writePolarization()"));

    Bool rstat = True;
    asdm::PolarizationTable& tT = ASDM_p->getPolarization();

    asdm::PolarizationRow* tR = 0;

    skipCorr_p.resize(0); 

    for(uInt irow=0; irow<polarization().nrow(); irow++){

      vector< Bool > skipCorr;

      // parameters of the new row

      vector< StokesParameterMod::StokesParameter > corrTypeV;
      vector< vector< PolarizationTypeMod::PolarizationType > > corrProduct;
      
      Vector< Int > v; // aux. vector
      v.reference( polarization().corrType()(irow) );
      for(uInt i=0; i<v.nelements(); i++){
	Bool skip = True;
	Stokes::StokesTypes st = static_cast<Stokes::StokesTypes>(v[i]);
	if(st == Stokes::LR){ // only add if RL is not also present
	  if(!stokesTypePresent(v, Stokes::RL)){
	    corrTypeV.push_back( ASDMStokesParameter( st ) );
	    skip = False;
	  }
	}
	else if(st == Stokes::YX){ // only add if XY is not also present
	  if(!stokesTypePresent(v, Stokes::XY)){
	    corrTypeV.push_back( ASDMStokesParameter( st ) );
	    skip = False;
	  }
	}
	else{
	  corrTypeV.push_back( ASDMStokesParameter( st ) );
	  skip = False;
	}
	skipCorr.push_back(skip);
      }
      int numCorr = corrTypeV.size(); 
      skipCorr_p.push_back(skipCorr);

      // now read the just created corrTypeV and write the correlation products accordingly
      for(uInt i=0; i<corrTypeV.size(); i++){
	vector< PolarizationTypeMod::PolarizationType > w;
	switch(corrTypeV[i]){
	case StokesParameterMod::RR:
	  w.push_back(PolarizationTypeMod::R);
	  w.push_back(PolarizationTypeMod::R);
	  break;
	case StokesParameterMod::RL:
	  w.push_back(PolarizationTypeMod::R);
	  w.push_back(PolarizationTypeMod::L);
	  break;
	case StokesParameterMod::LR:
	  w.push_back(PolarizationTypeMod::L);
	  w.push_back(PolarizationTypeMod::R);
	  break;
	case StokesParameterMod::LL:
	  w.push_back(PolarizationTypeMod::L);
	  w.push_back(PolarizationTypeMod::L);
	  break;
	case StokesParameterMod::XX:
	  w.push_back(PolarizationTypeMod::X);
	  w.push_back(PolarizationTypeMod::X);
	  break;
	case StokesParameterMod::XY:
	  w.push_back(PolarizationTypeMod::X);
	  w.push_back(PolarizationTypeMod::Y);
	  break;
	case StokesParameterMod::YX:
	  w.push_back(PolarizationTypeMod::Y);
	  w.push_back(PolarizationTypeMod::X);
	  break;
	case StokesParameterMod::YY:
	  w.push_back(PolarizationTypeMod::Y);
	  w.push_back(PolarizationTypeMod::Y);
	  break;
	case StokesParameterMod::RX:
	  w.push_back(PolarizationTypeMod::R);
	  w.push_back(PolarizationTypeMod::X);
	  break;
	case StokesParameterMod::RY:
	  w.push_back(PolarizationTypeMod::R);
	  w.push_back(PolarizationTypeMod::Y);
	  break;
	case StokesParameterMod::LX:
	  w.push_back(PolarizationTypeMod::L);
	  w.push_back(PolarizationTypeMod::X);
	  break;
	case StokesParameterMod::LY:
	  w.push_back(PolarizationTypeMod::L);
	  w.push_back(PolarizationTypeMod::Y);
	  break;
	case StokesParameterMod::XR:
	  w.push_back(PolarizationTypeMod::X);
	  w.push_back(PolarizationTypeMod::R);
	  break;
	case StokesParameterMod::XL:
	  w.push_back(PolarizationTypeMod::X);
	  w.push_back(PolarizationTypeMod::L);
	  break;
	case StokesParameterMod::YR:
	  w.push_back(PolarizationTypeMod::Y);
	  w.push_back(PolarizationTypeMod::R);
	  break;
	case StokesParameterMod::YL:
	  w.push_back(PolarizationTypeMod::Y);
	  w.push_back(PolarizationTypeMod::L);
	  break;
	default:
	  os << LogIO::SEVERE << "Cannot store correlation product for stokes parameter " << CStokesParameter::name(corrTypeV[i]) << LogIO::POST;
	  rstat = False;
	  break;
	}	  
	corrProduct.push_back(w);
      } // end loop over corrTypeV

      tR = tT.newRow(numCorr, corrTypeV, corrProduct);

      //cout << "ASDM numCorr is " << numCorr << endl;

      bool flagRow = polarization().flagRow()(irow);
      tR->setFlagRow(flagRow);

      // add the row to the table
      asdm::PolarizationRow* tR2 = 0;
      tR2 = tT.add(tR);

      if(tR2 == tR){ // adding this row caused a new tag to be defined
	// enter tag into the map
	asdmPolarizationId_p.define(irow, tR->getPolarizationId());
      }
      else{
	os << LogIO::WARN << "Duplicate row in MS Polarization table :" << irow << LogIO::POST;
      }
    } // end loop over MS Pol table

    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled Polarization table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();

    return rstat;
  }

  Bool MS2ASDM::writeCorrelatorMode(){
    LogIO os(LogOrigin("MS2ASDM", "writeCorrelatorMode()"));

    Bool rstat = True;
    asdm::CorrelatorModeTable& tT = ASDM_p->getCorrelatorMode();

    asdm::CorrelatorModeRow* tR = 0;

    // create only one row

    int numBaseband = 1;
    vector<BasebandNameMod::BasebandName > basebandNames;
    basebandNames.push_back(BasebandNameMod::NOBB);
    vector<int > basebandConfig;
    basebandConfig.push_back(0);
    AccumModeMod::AccumMode accumMode = AccumModeMod::NORMAL;
    int binMode = 1; // standard setting for interferometry
    int numAxes = 4; // time, antenna, stokes, pol 
    vector<AxisNameMod::AxisName > axesOrderArray;
    axesOrderArray.push_back(AxisNameMod::TIM);
    axesOrderArray.push_back(AxisNameMod::ANT);
    axesOrderArray.push_back(AxisNameMod::SPP); 
    axesOrderArray.push_back(AxisNameMod::POL);
    vector<FilterModeMod::FilterMode > filterMode;
    filterMode.push_back(FilterModeMod::UNDEFINED);

    CorrelatorNameMod::CorrelatorName correlatorName = CorrelatorNameMod::ALMA_BASELINE; // the default
    if(telName_p == "ALMA"){
      correlatorName = CorrelatorNameMod::ALMA_BASELINE; //???
    }
    else if(telName_p == "ACA"){
      correlatorName = CorrelatorNameMod::ALMA_ACA; //???
    }
    else if(telName_p == "VLA"){
      correlatorName = CorrelatorNameMod::NRAO_VLA; //???
    }
    else if(telName_p == "EVLA"){
      correlatorName = CorrelatorNameMod::NRAO_WIDAR; //???
    }
    else if(telName_p == "PDB"){
      correlatorName = CorrelatorNameMod::IRAM_PDB; //???
    }
    else{
      os << LogIO::WARN << "Unknown telescope name: " << telName_p << ". Assuming ALMA_BASELINE." << LogIO::POST;
    }      

    tR = tT.newRow(numBaseband, basebandNames, basebandConfig, accumMode, binMode, numAxes, axesOrderArray, filterMode, correlatorName);

    tT.add(tR);

    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled CorrelatorMode table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();

    return rstat;
  }

  Bool MS2ASDM::writeAlmaRadiometer(){
    LogIO os(LogOrigin("MS2ASDM", "writeAlmaRadiometer()"));

    Bool rstat = True;
    asdm::AlmaRadiometerTable& tT = ASDM_p->getAlmaRadiometer();

    asdm::AlmaRadiometerRow* tR = 0;

    //    tR = tT.newRow();

    tT.add(tR);

    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled AlmaRadiometer table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();

    return rstat;
  }

  Bool MS2ASDM::writeHolography(){
    LogIO os(LogOrigin("MS2ASDM", "writeHolography()"));

    Bool rstat = True;
    asdm::HolographyTable& tT = ASDM_p->getHolography();

    asdm::HolographyRow* tR = 0;

    //    tR = tT.newRow();

    tT.add(tR);

    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled Holography table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();

    return rstat;
  }

  Bool MS2ASDM::writeProcessor(){
    LogIO os(LogOrigin("MS2ASDM", "writeProcessor()"));

    Bool rstat = True;
    asdm::ProcessorTable& tT = ASDM_p->getProcessor();

    asdm::ProcessorRow* tR = 0;

    uInt nPRows = processor().nrow();

    if(nPRows > 0){ // need to test because MS processor table is obligatory but may be empty
      for(uInt irow=0; irow<nPRows; irow++){
	// parameters of the new row
	Tag modeId;
	ProcessorTypeMod::ProcessorType processorType;
	ProcessorSubTypeMod::ProcessorSubType processorSubType;
	if(processor().type()(irow) == "CORRELATOR"){
	  processorType = ProcessorTypeMod::CORRELATOR; 
	  if(processor().subType()(irow) == "ALMA_CORRELATOR_MODE"){
	    processorSubType = ProcessorSubTypeMod::ALMA_CORRELATOR_MODE; 
	  }
	  else{
	    os << LogIO::WARN << "Unsupported processor subType for type CORRELATOR " << processor().subType()(irow)
	       << " assuming ALMA_CORRELATOR" << LogIO::POST;
	    processorSubType = ProcessorSubTypeMod::ALMA_CORRELATOR_MODE; 
	  }	    
	}
	else if(processor().type()(irow) == "SPECTROMETER"){
	  processorType = ProcessorTypeMod::SPECTROMETER; 
	  if(processor().subType()(irow) == "HOLOGRAPHY"){
	    processorSubType = ProcessorSubTypeMod::HOLOGRAPHY; 
	  }
	  else{
	    os << LogIO::WARN << "Unsupported processor subType for type SPECTROMETER " << processor().subType()(irow)
	       << " assuming HOLOGRAPHY" << LogIO::POST;
	    processorSubType = ProcessorSubTypeMod::HOLOGRAPHY; 
	  }	    
	}
	else if(processor().type()(irow) == "RADIOMETER"){
	  processorType = ProcessorTypeMod::RADIOMETER; 
	  if(processor().subType()(irow) == "ALMA_RADIOMETER"){
	    processorSubType = ProcessorSubTypeMod::ALMA_RADIOMETER; 
	  }
	  else{
	    os << LogIO::WARN << "Unsupported processor type " << processor().subType()(irow)
	       << " assuming ALMA_RADIOMETER" << LogIO::POST;
	    processorSubType = ProcessorSubTypeMod::ALMA_RADIOMETER;
	  }	    
	}
	else{
	  os << LogIO::WARN << "Unsupported processor type " << processor().type()(irow)
	     << " assuming CORRELATOR" << LogIO::POST;
	  processorType = ProcessorTypeMod::CORRELATOR; 
	  processorSubType = ProcessorSubTypeMod::ALMA_CORRELATOR_MODE; 
	}

	// the following three vectors need to be declared outside the switch statement below;
	// otherwise the compiler complains
	vector< asdm::CorrelatorModeRow* > corrModeRows;
	vector< asdm::HolographyRow* > holographyRows;
	vector< asdm::AlmaRadiometerRow* > almaRadiometerRows;
	  
	switch(processorSubType){
	case ProcessorSubTypeMod::ALMA_CORRELATOR_MODE:
	  if(!writeCorrelatorMode()){ // create the optional CorrelatorMode table
	    return False;
	  }
	  corrModeRows = (ASDM_p->getCorrelatorMode()).get();
	  if(corrModeRows.size()==0){
	    os << LogIO::SEVERE << "Internal error: ASDM CorrelatorMode table is empty." << LogIO::POST;
	    return False;
	  }
	  modeId = corrModeRows[0]->getCorrelatorModeId(); // get tag from first row of CorrelatorMode table (there is only one) ??? 
	  break;
	case ProcessorSubTypeMod::HOLOGRAPHY:
	  if(!writeHolography()){ // create the optional Holography table
	    return False;
	  }
	  holographyRows = (ASDM_p->getHolography()).get();
	  if(holographyRows.size()==0){
	    os << LogIO::SEVERE << "Internal error: ASDM Holography table is empty." << LogIO::POST;
	    return False;
	  }
	  modeId = holographyRows[0]->getHolographyId(); // get tag from first row of Holography table (there is only one) ??? 
	  break;
	case ProcessorSubTypeMod::ALMA_RADIOMETER:
	  if(!writeAlmaRadiometer()){ // create the optional AlmaRadiometer table
	    return False;
	  }
	  almaRadiometerRows = (ASDM_p->getAlmaRadiometer()).get();
	  if(almaRadiometerRows.size()==0){
	    os << LogIO::SEVERE << "Internal error: ASDM AlmaRadiometer table is empty." << LogIO::POST;
	    return False;
	  }
	  modeId = almaRadiometerRows[0]->getAlmaRadiometerId(); // get tag from first row of AlmaRadiometer table (there is only one) ??? 
	  break;
	default:
	  os << LogIO::SEVERE << "Internal error: unsupported processor sub type." 
	     << CProcessorSubType::name(processorSubType) << LogIO::POST;
	  return False;
	}
	  
	tR = tT.newRow(modeId, processorType, processorSubType);
	// add the row to the table
	asdm::ProcessorRow* tR2 = 0;
	tR2 = tT.add(tR);

	if(tR2 == tR){ // adding this row caused a new tag to be defined
	  // enter tag into the map
	  asdmProcessorId_p.define(irow, tR->getProcessorId());
	}
	else{
	  os << LogIO::WARN << "Duplicate row in MS Processor table :" << irow << LogIO::POST;
	}
      } // end loop over MS processor table
    }
    else{ // MS processor table is empty
      os << LogIO::WARN << "MS Processor table is empty. Will try to proceed assuming a standard ALMA Correlator." << LogIO::POST;
      ProcessorTypeMod::ProcessorType processorType = ProcessorTypeMod::CORRELATOR;
      ProcessorSubTypeMod::ProcessorSubType processorSubType = ProcessorSubTypeMod::ALMA_CORRELATOR_MODE;
      if(!writeCorrelatorMode()){ // create the optional CorrelatorMode table
	return False;
      }
      vector< asdm::CorrelatorModeRow* > corrModeRows = ASDM_p->getCorrelatorMode().get();
      if(corrModeRows.size()==0){
	os << LogIO::SEVERE << "Internal error: ASDM CorrelatorMode table is empty." << LogIO::POST;
	return False;
      }
      Tag modeId = corrModeRows[0]->getCorrelatorModeId(); // get tag from first row of CorrelatorMode table (there is only one) ??? 
      tR = tT.newRow(modeId, processorType, processorSubType);
      tT.add(tR);
      // enter tag into the map connecting it to "-1"
      asdmProcessorId_p.define(-1, tR->getProcessorId());
    } // end if

    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled Processor table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();

    return rstat;
  }


  Bool MS2ASDM::writeField(){
    LogIO os(LogOrigin("MS2ASDM", "writeField()"));

    Bool rstat = True;
    asdm::FieldTable& tT = ASDM_p->getField();

    asdm::FieldRow* tR = 0;

    for(uInt irow=0; irow<field().nrow(); irow++){

      // parameters of the new row
      string fieldName = field().name()(irow).c_str();
      string code = field().code()(irow).c_str();
      int numPoly = field().numPoly()(irow);
      if(numPoly>0){
	os << LogIO::SEVERE << "Internal error: NUM_POLY > 0 not yet supported." << LogIO::POST;
	return False;
      }
      Int numpol = 0;

      String msAngUnit = "rad"; // MS uses radians here
      Matrix< Double > mdir; // aux. matrix

      vector< vector< Angle > > delayDirV;
      mdir.reference(field().delayDir()(irow));
      {
	vector< Angle > dirV;
	Quantity angle0(mdir(0,numpol), msAngUnit);
	dirV.push_back(Angle(angle0.getValue(unitASDMAngle())));
	Quantity angle1(mdir(1,numpol), msAngUnit);
	dirV.push_back(Angle(angle1.getValue(unitASDMAngle())));
	delayDirV.push_back(dirV);
      }

      vector< vector< Angle > > phaseDirV;
      mdir.reference(field().phaseDir()(irow));
      {
	vector< Angle > dirV;
	Quantity angle0(mdir(0,numpol), msAngUnit);
	dirV.push_back(Angle(angle0.getValue(unitASDMAngle())));
	Quantity angle1(mdir(1,numpol), msAngUnit);
	dirV.push_back(Angle(angle1.getValue(unitASDMAngle())));
	phaseDirV.push_back(dirV);
      }

      vector< vector< Angle > > referenceDirV;
      mdir.reference(field().referenceDir()(irow));
      {
	vector< Angle > dirV;
	Quantity angle0(mdir(0,numpol), msAngUnit);
	dirV.push_back(Angle(angle0.getValue(unitASDMAngle())));
	Quantity angle1(mdir(0,numpol), msAngUnit);
	dirV.push_back(Angle(angle1.getValue(unitASDMAngle())));
	referenceDirV.push_back(dirV);
      }

      tR = tT.newRow(fieldName, code, numPoly, delayDirV, phaseDirV, referenceDirV);

      tR->setTime(ASDMArrayTime(field().timeQuant()(irow).getValue("s")));

      Int sId = field().sourceId()(irow);
      if(asdmSourceId_p.isDefined(sId)){
	tR->setSourceId(asdmSourceId_p(sId));	
      }
      else if(sId!=-1){ // -1 means "no source"
	os << LogIO::WARN << "Undefined source id " << sId << " in MS field table row " << irow << LogIO::POST;
      }

      if(!field().ephemerisId().isNull()){
	Int eid = field().ephemerisId()(irow);
	if(asdmEphemerisId_p.isDefined(eid)){
	  tR->setEphemerisId(asdmEphemerisId_p(eid));
	}
	else{
	  os << LogIO::WARN << "Undefined ephemeris id " << eid << " in MS field table row " << irow << LogIO::POST;
	}
      }

      // add the row to the table
      asdm::FieldRow* tR2 = 0;
      tR2 = tT.add(tR);
      if(tR2 == tR){ // adding this row caused a new tag to be defined
	// enter tag into the map
	asdmFieldId_p.define(irow, tR->getFieldId());
      }
      else{
	os << LogIO::WARN << "Duplicate row in MS Field table :" << irow << LogIO::POST;
      }
    } // end loop over MS field table

    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled Field table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();

    return rstat;
  }


  Bool MS2ASDM::writeReceiver(){
    LogIO os(LogOrigin("MS2ASDM", "writeReceiver()"));

    Bool rstat = True;
    asdm::ReceiverTable& tT = ASDM_p->getReceiver();

    asdm::ReceiverRow* tR = 0;

    // create one row for each spectral window Id and time interval (i.e. receiverId is always == 0) 

    // loop over ASDM SPW table
    vector< asdm::SpectralWindowRow* > SPWRows = (ASDM_p->getSpectralWindow()).get();

    Bool informed = False;

    for(uInt irow=0; irow<SPWRows.size(); irow++){
   
      // parameters for the new Recevier table row
      Tag spectralWindowId = SPWRows[irow]->getSpectralWindowId();
      if(!informed){
	if(verbosity_p>0){
	  os << LogIO::NORMAL << "Taking validity time interval for all ASDM Receiver table entries from row 0 of MS Feed table." 
	     << LogIO::POST;
	}
	informed = True;
      }
      asdm::ArrayTimeInterval timeInterval( ASDMTimeInterval(feed().timeQuant()(0), feed().intervalQuant()(0)) ); 
      string name = "unspec. frontend";
      ReceiverBandMod::ReceiverBand frequencyBand;
      ReceiverSidebandMod::ReceiverSideband receiverSideband;
      setRecBands(SPWRows[irow]->getRefFreq(), frequencyBand, receiverSideband); 
      int numLO = 0; // no information in the MS ???
      vector<Frequency > freqLO;
      //freqLO.push_back(Frequency(1.)); // ???
      vector<NetSidebandMod::NetSideband > sidebandLO;
      //sidebandLO.push_back(SPWRows[irow]->getNetSideband()); // ???

      tR = tT.newRow(spectralWindowId, timeInterval, name, numLO, frequencyBand, freqLO, receiverSideband, sidebandLO);

      // add the row to the table
      tT.add(tR);
    } // end loop over SPW rows

    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled Receiver table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();

    return rstat;
  }

  Bool MS2ASDM::writeFeed(){
    LogIO os(LogOrigin("MS2ASDM", "writeFeed()"));

    Bool rstat = True;
    asdm::FeedTable& tT = ASDM_p->getFeed();

    asdm::FeedRow* tR = 0;

    Bool warned = False;
    Bool warned2 = False;
    Bool warned3 = False;

    for(uInt irow=0; irow<feed().nrow(); irow++){

      // parameters of the new feed row
      Tag antennaId;
      Int aid = feed().antennaId()(irow);
      if(asdmAntennaId_p.isDefined(aid)){
	antennaId = asdmAntennaId_p(aid);
      }
      else{
	os << LogIO::SEVERE << "Undefined antenna id " << aid << " in MS feed table row "<< irow << LogIO::POST;
	return False;
      }
      ArrayTimeInterval timeInterval( ASDMTimeInterval(feed().timeQuant()(irow), feed().intervalQuant()(irow)) );
      
      int numReceptor = feed().numReceptors()(irow);

      vector< vector< double > > beamOffsetV;
      vector< vector< Length > > focusReferenceV;
      vector< PolarizationTypeMod::PolarizationType > polarizationTypesV;
      vector< vector< asdm::Complex > > polResponseV;
      vector< Angle > receptorAngleV;
      vector< int > receiverIdV;

      Matrix< Double > mboffset; // aux. matrix
      mboffset.reference(feed().beamOffset()(irow));

      Vector< String > polT; // aux. vector
      polT.reference(feed().polarizationType()(irow));

      Matrix< Complex > polR;
      polR.reference(feed().polResponse()(irow));

      Vector< Quantity > receptA;
      receptA.reference(feed().receptorAngleQuant()(irow));

      for(uInt rnum=0; rnum<(uInt)numReceptor; rnum++){ // loop over all receptors
	vector< double > abo;
	abo.push_back(mboffset(0,rnum));
	abo.push_back(mboffset(1,rnum));
	beamOffsetV.push_back(abo);

	vector< Length > afr;
	afr.push_back(0); // x
	afr.push_back(0); // y
	if(!feed().focusLength().isNull()){ // the FOCUS_LENGTH column is optional
	  afr.push_back(Length(feed().focusLength()(irow))); // z  ???
	}
	else{
	  afr.push_back(0); // z
	} 
	focusReferenceV.push_back(afr);

	try{
	  polarizationTypesV.push_back(CPolarizationType::literal(polT[rnum].c_str()));
	}
	catch(std::string z){
	  os << LogIO::SEVERE << "Polarization type " << polT[rnum].c_str() << " for receptor " 
	     << rnum << " in MS Feed table row  " << irow << " not defined in ASDM: " 
	     << z << LogIO::POST;      
	  return False;
	}      	
	
	vector< asdm::Complex > apr;
	for(uInt rnum2=0; rnum2<(uInt)numReceptor; rnum2++){
	  apr.push_back( ASDMComplex( polR(rnum,rnum2) ) );
	}
	polResponseV.push_back(apr);

	receptorAngleV.push_back( Angle( receptA[rnum].getValue( unitASDMAngle() ) ) );

	if(telName_p=="ALMA" || telName_p=="ACA"){
	  receiverIdV.push_back(0); // always zero for ALMA
	}
	else{
	  receiverIdV.push_back(0);
	  if(!warned){
	    os << LogIO::WARN << "Setting receiver ID to zero (ALMA convention)." << LogIO::POST;
	    warned = True;
	  }
	}
      } // end loop over receptors

      // look at SPW Id parameter last!
      Tag spectralWindowId;
      vector< Tag > spwIdV; // the spw ids fow which to insert rows for this feed
      Int spwid = feed().spectralWindowId()(irow);
      if(spwid == -1){ // this means the MS Feed row is valid for all SPWs => need to insert same row for each SPW!
	// loop over all SPWs
	for(Int ispw=0; ispw<(Int)spectralWindow().nrow(); ispw++){
	  if(asdmSpectralWindowId_p.isDefined(ispw)){
	    spectralWindowId = asdmSpectralWindowId_p(ispw);
	    spwIdV.push_back(spectralWindowId); 
	  }
	}  
      }
      else if(asdmSpectralWindowId_p.isDefined(spwid)){
	spectralWindowId = asdmSpectralWindowId_p(spwid);
	spwIdV.push_back(spectralWindowId); // just one entry
      }
      else{
	os << LogIO::SEVERE << "Undefined SPW id " << spwid << " in MS feed table row "<< irow << LogIO::POST;
	return False;
      }

      // create the row for the first of the SPW Ids
      tR = tT.newRow(antennaId, spwIdV[0], timeInterval, numReceptor, beamOffsetV, focusReferenceV, 
		     polarizationTypesV, polResponseV, receptorAngleV, receiverIdV);

      if(telName_p=="ALMA" || telName_p=="ACA"){
	tR->setFeedNum(1); // always 1 for ALMA
      }
      else{
	tR->setFeedNum(1); 
	if(!warned2){
	  os << LogIO::WARN << "Assuming single-feed receivers. Setting FeedNum to 1 (as for ALMA)." << LogIO::POST;
	  warned2 = True;
	}
      }

      vector< Length > positionV;
      Vector< Quantity > pos;
      pos.reference(feed().positionQuant()(irow));
      for(uInt i=0; i<3; i++){
	positionV.push_back( Length( pos[0].getValue( unitASDMLength() ) ) );
      }
      tR->setPosition(positionV);

      if(feed().beamId()(irow)!=-1 && !warned3){ // there should be a beam table in the MS, but presently it is not implemented!!!
	os << LogIO::WARN << "MS Feed table contains reference to a Beam table row " << feed().beamId()(irow)
	   << " but a Beam table is not implemented in CASA. Ignoring." << LogIO::POST;
      }

      // add the row to the table
      tT.add(tR);	

      int asdmFId = tR->getFeedId(); // the determination of the feed id is done internally by the add() method
      Int fId = feed().feedId()(irow);
      if(asdmFeedId_p.isDefined(fId)){ // there is already a mapping
	if(asdmFId!=asdmFeedId_p(fId)){ // but it doesn't agree with the newly defined id
	  os << LogIO::WARN << "Internal problem: field id map inconsistent for MS feed table row:" << irow 
	     << ". MS FId " << fId << " is already mapped to ASDM FId" << asdmFeedId_p(fId)
	     << " but should also be mapped to ASDM FId " << asdmFId << LogIO::POST;
	}
      }
      else{
	// enter id into the map
	asdmFeedId_p.define(fId, asdmFId);
      }

      // add the same row for the remaining SPW Ids in the vector accumulated above
      for(uint i=1; i<spwIdV.size(); i++){
	tR->setSpectralWindowId(spwIdV[i]);
	tT.add(tR);	
      }	

    } // end loop over MS feed table

    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled Feed table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();

    return rstat;
  }


  Bool MS2ASDM::writeDataDescription(){
    LogIO os(LogOrigin("MS2ASDM", "writeDataDescription()"));

    Bool rstat = True;
    asdm::DataDescriptionTable& tT = ASDM_p->getDataDescription();

    asdm::DataDescriptionRow* tR = 0;

    // loop over MS data description table
    for(uInt irow=0; irow<dataDescription().nrow(); irow++){
      // parameters for the new row
      Tag polOrHoloId;
      Tag spectralWindowId;
      Int polId = dataDescription().polarizationId()(irow);
      Int spwId = dataDescription().spectralWindowId()(irow);
      if(asdmPolarizationId_p.isDefined(polId)){
	polOrHoloId = asdmPolarizationId_p(polId);
      }
      else{
	os << LogIO::SEVERE << "Inconsistent MS: undefined polarization id " << polId 
	   << " in row " << irow << " of the DataDesc table." << LogIO::POST;
	return False;
      }
      if(asdmSpectralWindowId_p.isDefined(spwId)){
	spectralWindowId = asdmSpectralWindowId_p(spwId);
      }
      else{
	os << LogIO::SEVERE << "Inconsistent MS: undefined SPW id " << spwId 
	   << " in row " << irow << " of the DataDesc table." << LogIO::POST;
	return False;
      }

      tR = tT.newRow(polOrHoloId, spectralWindowId);

      asdm::DataDescriptionRow* tR2 = 0;

      tR2 = tT.add(tR);
      if(tR2 == tR){ // adding this row caused a new tag to be defined
	// enter tag into the map
	asdmDataDescriptionId_p.define(irow, tR->getDataDescriptionId());
      }
      else{
	os << LogIO::WARN << "Duplicate row in MS DataDesc table :" << irow << LogIO::POST;
      }
    } // end loop over MS DD table

    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled DataDescription table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();

    return rstat;
  }

  Bool MS2ASDM::writeSwitchCycle(){ 
    LogIO os(LogOrigin("MS2ASDM", "writeSwitchCycle()"));
    
    Bool rstat = True;
    
    asdm::SwitchCycleTable& tT = ASDM_p->getSwitchCycle();
    
    asdm::SwitchCycleRow* tR = 0;
    
    // switch cycle table will only be dummy ? -> Francois
    // dummy if PHASE_ID column doesn't exist in MS main
    // PHASE_ID identifies bin in switch cycle
    //   otherwise, e.g. PHASE_ID = 0 and 1 => numStep == 2

    if(phaseId().isNull()){ // phaseId column doesn't exist

      // parameters of the new row
      int numStep = 1;
      vector< float > weightArray;
      weightArray.push_back(1.);
      vector< vector< Angle > > dirOffsetArray;
      vector< Angle > aV;
      aV.push_back(Angle(0.));
      aV.push_back(Angle(0.));
      dirOffsetArray.push_back(aV);
      vector< Frequency > freqOffsetArray;
      freqOffsetArray.push_back(Frequency(0.));
      vector< Interval > stepDurationArray;
      stepDurationArray.push_back(Interval(0)); // set to zero ???
      
      tR = tT.newRow(numStep, weightArray, dirOffsetArray, freqOffsetArray, stepDurationArray);
      
      tT.add(tR);
      
      EntityId theUid(getCurrentUid());
      Entity ent = tT.getEntity();
      ent.setEntityId(theUid);
      tT.setEntity(ent);
      if(verbosity_p>0){
	os << LogIO::NORMAL << "PHASE_ID column doesn't exist in MS Main table.\n Filled ASDM SwitchCycle table " 
	   << getCurrentUid() << " with one dummy entry." << LogIO::POST;
      }
      incrementUid();
    }
    else{ // phaseId column exists
      os << LogIO::SEVERE 
	 << "PHASE_ID column exists in MS Main table but proper creation of ASDM SwitchCycle table not yet implemented." 
	 << LogIO::POST;
      rstat = False;
    }      

    return rstat;
  }

  Bool MS2ASDM::writeState(){ 
    LogIO os(LogOrigin("MS2ASDM", "writeState()"));

    Bool rstat = True;

    asdm::StateTable& tT = ASDM_p->getState();

    asdm::StateRow* tR = 0;

    if(state().nrow()<1){ // State table not filled

      os << LogIO::WARN << "MS State table is empty. Creating ASDM State table with one on-source entry." 
	 << LogIO::POST;
     
      // parameters of the new row
      bool bSig = True;
      bool bRef = False;
      bool bOnSky = True; 
      CalibrationDeviceMod::CalibrationDevice calDeviceName = CalibrationDeviceMod::NONE;

      tR = tT.newRow(calDeviceName, bSig, bRef, bOnSky);
      
      tR->setWeight(1.); // optional column

      // add the new row to the table
      tT.add(tR);
      // enter tag into the map
      asdmStateId_p.define(-1, tR->getStateId());
    }
    else{ // MS State table exists
      for(uInt irow=0; irow<state().nrow(); irow++){
	// parameters of the new row
	bool bSig = state().sig()(irow);
	bool bRef = state().ref()(irow);
	Double loadT = state().load()(irow);
	Double noiseT = state().cal()(irow);
	bool bOnSky = bSig || bRef; 
	CalibrationDeviceMod::CalibrationDevice calDeviceName;
	if(0.<loadT && loadT<270.){
	  calDeviceName = CalibrationDeviceMod::COLD_LOAD;
	}
	else if(270<=loadT && loadT<303.){
	  calDeviceName = CalibrationDeviceMod::AMBIENT_LOAD;
	}
	else if(303.<=loadT){
	  calDeviceName = CalibrationDeviceMod::HOT_LOAD;
	}
	else if(0.<noiseT){
	  calDeviceName = CalibrationDeviceMod::NOISE_TUBE_LOAD;
	}
	else{
	  calDeviceName = CalibrationDeviceMod::NONE;
	  if(!bSig){
	    os << LogIO::WARN << "Trouble determining Cal Device for row " << irow << " in MS State table."
	       << "Assuming NONE." << LogIO::POST;
	  }
	}
	
	tR = tT.newRow(calDeviceName, bSig, bRef, bOnSky);
	
	tR->setWeight(1.); // optional column
	
	// add the new row to the table
	asdm::StateRow* tR2 = 0;
	
	tR2 = tT.add(tR);
	if(tR2 == tR){ // adding this row caused a new tag to be defined
	  // enter tag into the map
	  asdmStateId_p.define(irow, tR->getStateId());
	}
	else{
	  os << LogIO::WARN << "Duplicate row in MS State table :" << irow << LogIO::POST;
	}
      } // end loop over MS state table
    } // end else

    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled State table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();

    return rstat;
  }

  Bool MS2ASDM::writeSysCal(){ 
    LogIO os(LogOrigin("MS2ASDM", "writeSysCal()"));

    Bool rstat = True;

    asdm::SysCalTable& tT = ASDM_p->getSysCal();

    asdm::SysCalRow* tR = 0;

    uInt nSysCalRows = sysCal().nrow();

    if(nSysCalRows<1){ // SysCal table not filled
      os << LogIO::WARN << "MS SysCal table doesn't exist or is empty. Creating ASDM SysCal table with default entries." 
	 << LogIO::POST;
    
      // loop over the main table
      uInt nMainTabRows = ms_p.nrow();
      for(uInt mainTabRow=0; mainTabRow<nMainTabRows; mainTabRow++){

	// get DDId and feed id
	Int f1Id = feed1()(mainTabRow);
	Int f2Id = feed2()(mainTabRow);
	Int DDId = dataDescId()(mainTabRow);	
	// get start time stamp
	Double startTime = timestampStartSecs(mainTabRow);
	// create vectors of info for each antenna
	vector< Int > aIdV;
	vector< Int > SPWIdV;
	vector< Int > feedIdV;
	vector< int > nRecV;
	vector< int > nChanV;
	SimpleOrderedMap <Int, Int> antennaDone(-1);

	uInt irow = mainTabRow;
	// while ddid and feed id remain the same
	while(irow<nMainTabRows &&
	      dataDescId()(irow) == DDId &&
	      feed1()(irow) == f1Id &&
	      feed2()(irow) == f2Id
	      ){
	  Int aId = antenna1()(irow);
	  //   if info for given antenna not yet filled
	  if(!antennaDone.isDefined(aId)){
	    //       get info for antenna and fill vectors
	    aIdV.push_back(aId);
	    Int spwId = dataDescription().spectralWindowId()(DDId);
	    SPWIdV.push_back(spwId);
	    feedIdV.push_back(f1Id);
	    nRecV.push_back(feed().numReceptors()(f1Id)); 
	    nChanV.push_back(spectralWindow().numChan()(spwId));
	    antennaDone.define(aId, f1Id);
	  }
	  aId = antenna2()(irow);
	  //   if info for given antenna not yet filled
	  if(!antennaDone.isDefined(aId)){
	    //       get info for antenna and fill vectors
	    aIdV.push_back(aId);
	    Int spwId = dataDescription().spectralWindowId()(DDId);
	    SPWIdV.push_back(spwId);
	    feedIdV.push_back(f2Id);
	    nRecV.push_back(feed().numReceptors()(f2Id)); 
	    nChanV.push_back(spectralWindow().numChan()(spwId));
	    antennaDone.define(aId, f2Id);
	  }
	  irow++;
	} // end while
	// get end timestamp
	Double endTime = timestampEndSecs(irow-1);
	// create ArrayTimeInterval
	asdm::ArrayTimeInterval timeInterval( ASDMArrayTime(startTime),
					      ASDMInterval(endTime-startTime) );
	// create new rows in syscal table based on the vectors
	for(uInt i=0; i<aIdV.size(); i++){
	  // parameters of the new SysCal row
	  Tag antennaId = asdmAntennaId_p(aIdV[i]);
	  Tag spectralWindowId = asdmSpectralWindowId_p(SPWIdV[i]);
	  int feedId = asdmFeedId_p(feedIdV[i]);
	  int numReceptor = nRecV[i];
	  int numChan = nChanV[i];

	  tR = tT.newRow(antennaId, spectralWindowId, timeInterval, feedId, numReceptor, numChan);
      
	  // add the new row to the table
	  tT.add(tR);
	}
	mainTabRow = irow;
      } // end loop over main table
    }
    else{ // MS SysCal table exists
      for(uInt irow=0; irow<nSysCalRows; irow++){
	// parameters of the new row
	Tag antennaId = asdmAntennaId_p( sysCal().antennaId()(irow) );
	Int spwId = sysCal().spectralWindowId()(irow);
	Tag spectralWindowId = asdmSpectralWindowId_p( spwId );
	int feedId = sysCal().feedId()(irow);
	ArrayTimeInterval timeInterval( ASDMTimeInterval(sysCal().timeQuant()(irow), sysCal().intervalQuant()(irow)) );

	uInt numReceptor = feed().numReceptors()(feedId); 
	uInt nChan = spectralWindow().numChan()(spwId); 

	tR = tT.newRow(antennaId, spectralWindowId, timeInterval, feedId, (int)numReceptor, (int)nChan);


	// now set the optional columns if they exist in the MS
	
	// Tant 
	if(!sysCal().tantFlag().isNull()){
	  tR->setTantFlag(sysCal().tantFlag()(irow));
	}
	if(!sysCal().tantSpectrumQuant().isNull()){
	  Matrix< Quantum< Float > > sM;
	  sM.reference(sysCal().tantSpectrumQuant()(irow));
	  if(sM.shape()(0) != (Int)numReceptor){
	    os << LogIO::SEVERE << "Inconsistent MS: TANT spectrum in syscal table row " << irow
	       << " should have number of receptors as in Feed table: " << numReceptor << LogIO::POST;
	    return False;
	  } 
	  if(sM.shape()(1) != (Int)nChan){
	    os << LogIO::SEVERE << "Inconsistent MS: TANT spectrum in syscal table row " << irow
	       << " should have second dimension as referenced SPW." << LogIO::POST;
	    return False;
	  } 
	  vector< vector< float > > fVV; // presently a float but should be a Temperature ???
	  for(uInt i=0; i<numReceptor; i++){
	    vector< float > fV;
	    for(uInt j=0; j<nChan; j++){
	      fV.push_back( sM(i,j).getValue(unitASDMTemp()) ); 
	    }
	    fVV.push_back(fV);
	  }
	  tR->setTantSpectrum(fVV);
	}
	else if(!sysCal().tantQuant().isNull()){
	  Vector< Quantum< Float > > sV;
	  sV.reference(sysCal().tantQuant()(irow));
	  vector< vector< float > > fVV; // presently a float but should be a Temperature ???
	  if(sV.size() != numReceptor){
	    os << LogIO::SEVERE << "Inconsistent MS: TANT in syscal table row " << irow
	       << " should have number of receptors as in Feed table: " << numReceptor << LogIO::POST;
	    return False;
	  } 
	  for(uInt i=0; i<numReceptor; i++){
	    vector< float > fV;
	    for(uInt j=0; j<nChan; j++){
	      fV.push_back( sV[i].getValue(unitASDMTemp()) );
	    }
	    fVV.push_back(fV);
	  }
	  tR->setTantSpectrum(fVV);
	}

	// Tant/Tsys
	if(!sysCal().tantTsysFlag().isNull()){
	  tR->setTantTsysFlag(sysCal().tantTsysFlag()(irow));
	}
	if(!sysCal().tantTsysSpectrum().isNull()){
	  Matrix< Float > sM;
	  sM.reference(sysCal().tantTsysSpectrum()(irow));
	  if(sM.shape()(0) != (Int)numReceptor){
	    os << LogIO::SEVERE << "Inconsistent MS: TANT_TSYS spectrum in syscal table row " << irow
	       << " should have number of receptors as in Feed table: " << numReceptor << LogIO::POST;
	    return False;
	  } 
	  if(sM.shape()(1) != (Int)nChan){
	    os << LogIO::SEVERE << "Inconsistent MS: TANT_TSYS spectrum in syscal table row " << irow
	       << " should have second dimension as referenced SPW." << LogIO::POST;
	    return False;
	  } 
	  vector< vector< float > > fVV; 
	  for(uInt i=0; i<numReceptor; i++){
	    vector< float > fV;
	    for(uInt j=0; j<nChan; j++){
	      fV.push_back( sM(i,j) ); 
	    }
	    fVV.push_back(fV);
	  }
	  tR->setTantTsysSpectrum(fVV);
	}
	else if(!sysCal().tantTsys().isNull()){
	  Vector< Float > sV;
	  sV.reference(sysCal().tantTsys()(irow));
	  vector< vector< float > > fVV; 
	  if(sV.size() != numReceptor){
	    os << LogIO::SEVERE << "Inconsistent MS: TANT_TSYS in syscal table row " << irow
	       << " should have number of receptors as in Feed table: " << numReceptor << LogIO::POST;
	    return False;
	  } 
	  for(uInt i=0; i<numReceptor; i++){
	    vector< float > fV;
	    for(uInt j=0; j<nChan; j++){
	      fV.push_back( sV[i] );
	    }
	    fVV.push_back(fV);
	  }
	  tR->setTantTsysSpectrum(fVV);
	}

	// Tcal
	if(!sysCal().tcalFlag().isNull()){
	  tR->setTcalFlag(sysCal().tcalFlag()(irow));
	}
	if(!sysCal().tcalSpectrumQuant().isNull()){
	  Matrix< Quantum< Float > > sM;
	  sM.reference(sysCal().tcalSpectrumQuant()(irow));
	  if(sM.shape()(0) != (Int)numReceptor){
	    os << LogIO::SEVERE << "Inconsistent MS: TCAL spectrum in syscal table row " << irow
	       << " should have number of receptors as in Feed table: " << numReceptor << LogIO::POST;
	    return False;
	  } 
	  if(sM.shape()(1) != (Int)nChan){
	    os << LogIO::SEVERE << "Inconsistent MS: TCAL spectrum in syscal table row " << irow
	       << " should have second dimension as referenced SPW." << LogIO::POST;
	    return False;
	  } 
	  vector< vector< Temperature > > fVV; 
	  for(uInt i=0; i<numReceptor; i++){
	    vector< Temperature > fV;
	    for(uInt j=0; j<nChan; j++){
	      fV.push_back( Temperature(sM(i,j).getValue(unitASDMTemp())) ); 
	    }
	    fVV.push_back(fV);
	  }
	  tR->setTcalSpectrum(fVV);
	}
	else if(!sysCal().tcalQuant().isNull()){
	  Vector< Quantum< Float > > sV;
	  sV.reference(sysCal().tcalQuant()(irow));
	  vector< vector< Temperature > > fVV; 
	  if(sV.size() != numReceptor){
	    os << LogIO::SEVERE << "Inconsistent MS: TCAL in syscal table row " << irow
	       << " should have number of receptors as in Feed table: " << numReceptor << LogIO::POST;
	    return False;
	  } 
	  for(uInt i=0; i<numReceptor; i++){
	    vector< Temperature > fV;
	    for(uInt j=0; j<nChan; j++){
	      fV.push_back( Temperature(sV[i].getValue(unitASDMTemp())) );
	    }
	    fVV.push_back(fV);
	  }
	  tR->setTcalSpectrum(fVV);
	}

	// Trx
	if(!sysCal().trxFlag().isNull()){
	  tR->setTrxFlag(sysCal().trxFlag()(irow));
	}
	if(!sysCal().trxSpectrumQuant().isNull()){
	  Matrix< Quantum< Float > > sM;
	  sM.reference(sysCal().trxSpectrumQuant()(irow));
	  if(sM.shape()(0) != (Int)numReceptor){
	    os << LogIO::SEVERE << "Inconsistent MS: TRX spectrum in syscal table row " << irow
	       << " should have number of receptors as in Feed table: " << numReceptor << LogIO::POST;
	    return False;
	  } 
	  if(sM.shape()(1) != (Int)nChan){
	    os << LogIO::SEVERE << "Inconsistent MS: TRX spectrum in syscal table row " << irow
	       << " should have second dimension as referenced SPW." << LogIO::POST;
	    return False;
	  } 
	  vector< vector< Temperature > > fVV; 
	  for(uInt i=0; i<numReceptor; i++){
	    vector< Temperature > fV;
	    for(uInt j=0; j<nChan; j++){
	      fV.push_back( Temperature(sM(i,j).getValue(unitASDMTemp())) ); 
	    }
	    fVV.push_back(fV);
	  }
	  tR->setTrxSpectrum(fVV);
	}
	else if(!sysCal().trxQuant().isNull()){
	  Vector< Quantum< Float > > sV;
	  sV.reference(sysCal().trxQuant()(irow));
	  vector< vector< Temperature > > fVV; 
	  if(sV.size() != numReceptor){
	    os << LogIO::SEVERE << "Inconsistent MS: TRX in syscal table row " << irow
	       << " should have number of receptors as in Feed table: " << numReceptor << LogIO::POST;
	    return False;
	  } 
	  for(uInt i=0; i<numReceptor; i++){
	    vector< Temperature > fV;
	    for(uInt j=0; j<nChan; j++){
	      fV.push_back( Temperature(sV[i].getValue(unitASDMTemp())) );
	    }
	    fVV.push_back(fV);
	  }
	  tR->setTrxSpectrum(fVV);
	}

	// Tsky
	if(!sysCal().tskyFlag().isNull()){
	  tR->setTskyFlag(sysCal().tskyFlag()(irow));
	}
	if(!sysCal().tskySpectrumQuant().isNull()){
	  Matrix< Quantum< Float > > sM;
	  sM.reference(sysCal().tskySpectrumQuant()(irow));
	  if(sM.shape()(0) != (Int)numReceptor){
	    os << LogIO::SEVERE << "Inconsistent MS: TSKY spectrum in syscal table row " << irow
	       << " should have number of receptors as in Feed table: " << numReceptor << LogIO::POST;
	    return False;
	  } 
	  if(sM.shape()(1) != (Int)nChan){
	    os << LogIO::SEVERE << "Inconsistent MS: TSKY spectrum in syscal table row " << irow
	       << " should have second dimension as referenced SPW." << LogIO::POST;
	    return False;
	  } 
	  vector< vector< Temperature > > fVV; 
	  for(uInt i=0; i<numReceptor; i++){
	    vector< Temperature > fV;
	    for(uInt j=0; j<nChan; j++){
	      fV.push_back( Temperature(sM(i,j).getValue(unitASDMTemp())) ); 
	    }
	    fVV.push_back(fV);
	  }
	  tR->setTskySpectrum(fVV);
	}
	else if(!sysCal().tskyQuant().isNull()){
	  Vector< Quantum< Float > > sV;
	  sV.reference(sysCal().tskyQuant()(irow));
	  vector< vector< Temperature > > fVV; 
	  if(sV.size() != numReceptor){
	    os << LogIO::SEVERE << "Inconsistent MS: TSKY in syscal table row " << irow
	       << " should have number of receptors as in Feed table: " << numReceptor << LogIO::POST;
	    return False;
	  } 
	  for(uInt i=0; i<numReceptor; i++){
	    vector< Temperature > fV;
	    for(uInt j=0; j<nChan; j++){
	      fV.push_back( Temperature(sV[i].getValue(unitASDMTemp())) );
	    }
	    fVV.push_back(fV);
	  }
	  tR->setTskySpectrum(fVV);
	}

	// Tsys
	if(!sysCal().tsysFlag().isNull()){
	  tR->setTsysFlag(sysCal().tsysFlag()(irow));
	}
	if(!sysCal().tsysSpectrumQuant().isNull()){
	  Matrix< Quantum< Float > > sM;
	  sM.reference(sysCal().tsysSpectrumQuant()(irow));
	  if(sM.shape()(0) != (Int)numReceptor){
	    os << LogIO::SEVERE << "Inconsistent MS: TSYS spectrum in syscal table row " << irow
	       << " should have number of receptors as in Feed table: " << numReceptor << LogIO::POST;
	    return False;
	  } 
	  if(sM.shape()(1) != (Int)nChan){
	    os << LogIO::SEVERE << "Inconsistent MS: TSYS spectrum in syscal table row " << irow
	       << " should have second dimension as referenced SPW." << LogIO::POST;
	    return False;
	  } 
	  vector< vector< Temperature > > fVV; 
	  for(uInt i=0; i<numReceptor; i++){
	    vector< Temperature > fV;
	    for(uInt j=0; j<nChan; j++){
	      fV.push_back( Temperature(sM(i,j).getValue(unitASDMTemp())) ); 
	    }
	    fVV.push_back(fV);
	  }
	  tR->setTsysSpectrum(fVV);
	}
	else if(!sysCal().tsysQuant().isNull()){
	  Vector< Quantum< Float > > sV;
	  sV.reference(sysCal().tsysQuant()(irow));
	  vector< vector< Temperature > > fVV; 
	  if(sV.size() != numReceptor){
	    os << LogIO::SEVERE << "Inconsistent MS: TSYS in syscal table row " << irow
	       << " should have number of receptors as in Feed table: " << numReceptor << LogIO::POST;
	    return False;
	  } 
	  for(uInt i=0; i<numReceptor; i++){
	    vector< Temperature > fV;
	    for(uInt j=0; j<nChan; j++){
	      fV.push_back( Temperature(sV[i].getValue(unitASDMTemp())) );
	    }
	    fVV.push_back(fV);
	  }
	  tR->setTsysSpectrum(fVV);
	}

	// phase diff
	if(!sysCal().phaseDiffFlag().isNull()){
	  tR->setPhaseDiffFlag(sysCal().phaseDiffFlag()(irow));
	}
	if(!sysCal().phaseDiffQuant().isNull()){
	  float pD = sysCal().phaseDiffQuant()(irow).getValue(unitASDMAngle());
	  vector< vector< float > > fVV;
	  for(uInt i=0; i<numReceptor; i++){
	    vector< float > fV;
	    for(uInt j=0; j<nChan; j++){
	      fV.push_back(pD); 
	    }
	    fVV.push_back(fV);
	  }
	  tR->setPhaseDiffSpectrum(fVV);
	}
	
	// finally add the completed new row to the table
	asdm::SysCalRow* tR2 = 0;
	tR2 = tT.add(tR);
	if(tR2 != tR){ 
	  os << LogIO::WARN << "Duplicate row in MS SysCal table :" << irow << LogIO::POST;
	}	

      } // end loop over MS syscal table
    } // end else

    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled SysCal table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();

    return rstat;
  }


  Bool MS2ASDM::writeConfigDescription(){

   LogIO os(LogOrigin("MS2ASDM", "writeConfigDesc()"));

    Bool rstat = True;

    asdm::ConfigDescriptionTable& tT = ASDM_p->getConfigDescription();

    asdm::ProcessorTable& procT = ASDM_p->getProcessor();

    asdm::SwitchCycleTable& swcT = ASDM_p->getSwitchCycle();
    vector< asdm::SwitchCycleRow * > swcRV = swcT.get();
    Tag swcTag = swcRV[0]->getSwitchCycleId(); // preliminary implementation: get tag from first (and only) row

    uInt nProcTabRows =  processor().nrow();
    if(nProcTabRows<1){ // processor table not filled, all data will have proc id == -1
      os <<  LogIO::WARN << "MS Processor table is empty. Will assume processor type is CORRELATOR." << LogIO::POST;
      nProcTabRows = 1;
    }

    // loop over MS processor table (typically, this loop will only be executed once)
    for(uInt uprocId=0; uprocId<nProcTabRows; uprocId++){

      Int procId = uprocId;

      if(processor().nrow()<1){
	procId  = -1;
      }

      if(verbosity_p>0){
	os << LogIO::NORMAL << "Processor Id: " << procId << LogIO::POST;
      }

      asdm::ConfigDescriptionRow* tR = 0;

      Tag procIdTag;
      if(asdmProcessorId_p.isDefined(procId)){
	procIdTag = asdmProcessorId_p(procId);
      }
      else{
	os << LogIO::SEVERE << "Internal error: undefined mapping for processor id " << procId << LogIO::POST;
	return False;
      }

      // get processor type from already existing ASDM processor table
      ProcessorTypeMod::ProcessorType processorType = procT.getRowByKey(procIdTag)->getProcessorType();

      SpectralResolutionTypeMod::SpectralResolutionType spectralType = SpectralResolutionTypeMod::FULL_RESOLUTION; 
      // alternatives: BASEBAND_WIDE, CHANNEL_AVERAGE
      if(verbosity_p>0){
	os << LogIO::NORMAL << "Assuming data is of spectral resolution type \"FULL RESOLUTION\"." << LogIO::POST;
      }


      // loop over MS Main table
      Tag previousTag = Tag();
      uInt nMainTabRows = ms_p.nrow();
      for(uInt mainTabRow=0; mainTabRow<nMainTabRows; mainTabRow++){

	vector<Int> msAntennaIdV;
	vector<Int> msDDIdV;
	vector<Int> msFeedIdV;

	vector<Tag> antennaId;
	vector<Tag> dataDId;
	vector<int> feedId;
	vector<Tag> switchCycleId;
	vector<AtmPhaseCorrectionMod::AtmPhaseCorrection> atmPhaseCorrection;
	int numAntenna = 0;
	int numDD = 0;
	int numFeed = 0;
	CorrelationModeMod::CorrelationMode correlationMode;
	
	// loop over MS main table and find for this proc id and timestamp
	//  a) all used antennas
	//  b) all used DD IDs
	//  c) all used feed IDs
	uInt numAutoCorrs = 0;
	uInt numBaselines = 0;

	if(processorId()(mainTabRow)==procId){

	  //	  cout << "proc id " << procId << " used at main table row " << mainTabRow << endl;

	  uInt irow = mainTabRow;
	  Double thisTStamp = time()(irow); 

	  while(time()(irow)==thisTStamp && irow<nMainTabRows){
	    
	    // for the later determination of the correlation mode
	    if(antenna1()(irow) == antenna2()(irow)){
	      numAutoCorrs++;
	    }
	    else{
	      numBaselines++;
	    }
	    
	    // antenna ids

	    Int aId = antenna1()(irow); 
	    Bool found = false;
	    for(uInt j=0; j<msAntennaIdV.size(); j++){
	      if(aId == msAntennaIdV[j]){
		found = True;
		break;
	      }
	    }
	    if(!found){
	      msAntennaIdV.push_back(aId);
	      if(!asdmAntennaId_p.isDefined(aId)){
		os << LogIO::SEVERE << "Internal error: undefined mapping for antenna1 id " << aId 
		   << " in main table row " << mainTabRow << LogIO::POST;
		return False;
	      }
	    }
	    aId = antenna2()(irow); 
	    found = false;
	    for(uInt j=0; j<msAntennaIdV.size(); j++){
	      if(aId == msAntennaIdV[j]){
		found = True;
		break;
	      }
	    }
	    if(!found){
	      msAntennaIdV.push_back(aId);
	      if(!asdmAntennaId_p.isDefined(aId)){
		os << LogIO::SEVERE << "Internal error: undefined mapping for antenna2 id " << aId 
		   << " in main table row " << mainTabRow << LogIO::POST;
		return False;
	      }
	    }

	    
	    // DD IDs
	    Int dDIdi = dataDescId()(irow);
	    found = False;
	    for(uInt j=0;j<msDDIdV.size();j++){
	      if(dDIdi == msDDIdV[j]){
		found = True;
		break;
	      }
	    }
	    if(!found){
	      msDDIdV.push_back(dDIdi);
	      if(!asdmDataDescriptionId_p.isDefined(dDIdi)){
		os << LogIO::SEVERE << "Internal error: undefined mapping for data desc. id " << dDIdi
		   << " in main table row " << mainTabRow << LogIO::POST;
		return False;
	      }
	    } 
	    
	    // feed ids
	    Int fIdi = feed1()(irow);
	    found = False;	      
	    for(uInt j=0;j<msFeedIdV.size();j++){
	      if(fIdi == msFeedIdV[j]){
		found = True;
		break;
	      }
	    }
	    if(!found){
	      msFeedIdV.push_back(fIdi);
	      if(!asdmFeedId_p.isDefined(fIdi)){
		os << LogIO::SEVERE << "Internal error: undefined mapping for feed1 id " << fIdi
		   << " in main table row " << mainTabRow << LogIO::POST;
		return False;
	      }
	    }
	    fIdi = feed2()(irow);
	    found = False;	      
	    for(uInt j=0;j<msFeedIdV.size();j++){
	      if(fIdi == msFeedIdV[j]){
		found = True;
		break;
	      }
	    }
	    if(!found){
	      msFeedIdV.push_back(fIdi);
	      if(!asdmFeedId_p.isDefined(fIdi)){
		os << LogIO::SEVERE << "Internal error: undefined mapping for feed2 id " << fIdi
		   << " in main table row " << mainTabRow << LogIO::POST;
		return False;
	      }
	    }
	    
	    irow++;
	  } // end while

	   // sort the  antenna ids before entering them into the ConfigDescription table
	  std::sort(msAntennaIdV.begin(), msAntennaIdV.end());
	  for(uInt i=0; i<msAntennaIdV.size(); i++){
	    antennaId.push_back(asdmAntennaId_p(msAntennaIdV[i]));
	  }
	  numAntenna = antennaId.size();
	   // sort the DD ids before entering them into the ConfigDescription table
	  std::sort(msDDIdV.begin(), msDDIdV.end());
	  for(uInt i=0; i<msDDIdV.size(); i++){
	    dataDId.push_back(asdmDataDescriptionId_p(msDDIdV[i]));
	  }
	  numDD = dataDId.size();
	  // sort the feed ids before entering them into the ConfigDescription table
	  std::sort(msFeedIdV.begin(), msFeedIdV.end());
	  for(uInt i=0; i<msFeedIdV.size(); i++){
	    feedId.push_back(asdmFeedId_p(msFeedIdV[i]));
	  }
	  numFeed = feedId.size();

	  if(numAutoCorrs==0){
	    correlationMode = CorrelationModeMod::CROSS_ONLY;
	  }
	  else if(numBaselines>0){
	    correlationMode = CorrelationModeMod::CROSS_AND_AUTO;
	  }
	  else{
	    correlationMode = CorrelationModeMod::AUTO_ONLY;
	  }
	  
	  switchCycleId.push_back(swcTag); // switch cycle table will only be dummy ? -> Francois
	  // dummy if PHASE_ID column doesn't exist in MS main
          // PHASE_ID identifies bin in switch cycle
	  //   otherwise, e.g. PHASE_ID = 0 and 1 => numStep == 2

	  if(dataIsAPCorrected()){
	    atmPhaseCorrection.push_back(AtmPhaseCorrectionMod::AP_CORRECTED); 
	  }
	  else{
	    atmPhaseCorrection.push_back(AtmPhaseCorrectionMod::AP_UNCORRECTED); 
	  }	    
    
	  // create a new row with its mandatory attributes.
	  tR = tT.newRow (numAntenna,
			  numDD,
			  numFeed,
			  correlationMode, 
			  atmPhaseCorrection.size(),
			  atmPhaseCorrection, 
			  processorType, 
			  spectralType, 
			  antennaId, 
			  feedId, 
			  switchCycleId, 
			  dataDId, 
			  procIdTag);
	  
	  // optional attributes.
	  //vector<Tag> assocConfigDescriptionId(1);
	  //assocConfigDescriptionId[0] = Tag(1, TagType::ConfigDescription);
	  //vector<int> phasedArrayList(2);
	  //phasedArrayList[0] = 0;
	  //phasedArrayList[1] = 1;
	  //vector<bool> flagAnt(2, false);
	  //	  vector<SpectralResolutionTypeMod::SpectralResolutionType> assocNature(1);
	  //assocNature[0] = SpectralResolutionTypeMod::FULL_RESOLUTION;
	  //tR->setAssocConfigDescriptionId(assocConfigDescriptionId);
	  //tR->setPhasedArrayList(phasedArrayList);
	  //tR->setFlagAnt(flagAnt);
	  //tR->setAssocNature(assocNature);	  
	  
	  // add this row to to the config description table.
	  //  note that this will check for uniqueness
	  asdm::ConfigDescriptionRow* tR2 = 0;

	  tR2 = tT.add(tR);
	  Tag newTag = tR2->getConfigDescriptionId();
	  asdmConfigDescriptionId_p.define(mainTabRow, newTag); // memorize tag for every row (to keep it simple)
	  if(verbosity_p>1){
	    cout << "Defined conf desc id for main table row " << mainTabRow << endl;
	  }

	  mainTabRow = irow-1;

	} // end if proc id

      } // end loop over MS main table

    } // end loop over MS processor table

    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled ConfigDescription table " << getCurrentUid() << " with " << tT.size() << " rows ... " << LogIO::POST;
    }
    incrementUid();

    return rstat;

  }

  Bool MS2ASDM::writeSBSummaryAndExecBlockStubs(){
    LogIO os(LogOrigin("MS2ASDM", "writeSBSummaryAndExecBlockStubs()"));
    
    Bool rstat = True;
    
    asdm::SBSummaryTable& tT = ASDM_p->getSBSummary();
    
    asdm::SBSummaryRow* tR = 0;

    asdm::ExecBlockTable& tET = ASDM_p->getExecBlock();
    
    asdm::ExecBlockRow* tER = 0;

    SimpleOrderedMap <asdm::Tag, Double> execBlockStartTime(-1.); // map from SBSummaryID to current execblock start time
    SimpleOrderedMap <asdm::Tag, Double> execBlockEndTime(-1.); // map from SBSummaryID to current execblock start time
    SimpleOrderedMap <asdm::Tag, asdm::ConfigDescriptionRow*> correspConfigDescRow(0); // map from SBSummaryID to the 
                                                                             // ConfigDescription row of current exec block
    SimpleOrderedMap <asdm::Tag, Int> execBlockNumber(0); // map from SBSummaryID to current execblock number
    SimpleOrderedMap <asdm::Tag, Int> obsIdFromSBSum(-1); // map from SBSummaryID to current obs ID
    SimpleOrderedMap <asdm::Tag, Double> minBaseline(0.); // map from SBSummaryID to minimum baseline of current exec block
    SimpleOrderedMap <asdm::Tag, Double> maxBaseline(0.); // map from SBSummaryID to maximum baseline of current exec block

    SimpleOrderedMap <Int, Int> firstFieldIdFromObsId(-1); // map from obsId to first field id (for the representative direction)

    // unfortunately, we have to loop over the main table to get the information

    // but beforehand we calculate the position of the array center for later reference

    MSAntenna anttable = ms_p.antenna();
    ROMSAntennaColumns ANTCols(anttable);
    ROScalarMeasColumn<MPosition> ANTPositionMeasCol = ANTCols.positionMeas(); 
    Int nAnt = 0;
    Vector<Double> pos(3); 
    pos=0;
    for (uInt i=0; i<anttable.nrow(); i++) {
      pos+=ANTPositionMeasCol(i).getValue().get();
      nAnt++;
    }
    if(nAnt>0){
      pos /= Double(nAnt);
    }
    else {
      os << LogIO::SEVERE << "No antennas in this MS. Cannot proceed ..." 
	 << LogIO::POST;
      return False; 
    }
    MVPosition mObsPos = MVPosition(pos);
    Length siteAltitude = Length(mObsPos.getLength().getValue(unitASDMLength()));
    Angle siteLongitude = Angle(mObsPos.getLong("rad").getValue(unitASDMAngle()));
    Angle siteLatitude = Angle(mObsPos.getLat("rad").getValue(unitASDMAngle()));

    // loop over main table
    Bool warned = False; // aux. var. to avoid warning repetition  
    uInt nMainTabRows = ms_p.nrow();
    for(uInt mainTabRow=0; mainTabRow<nMainTabRows; mainTabRow++){

      // Step 1: determine SBSummary ID and fill SBSummary table row
      // (duplicate rows will automatically be taken care of by the row adding method)

      Int obsId = observationId()(mainTabRow);

      // parameters of the new SBSummary row
      Int ddId = dataDescId()(mainTabRow);
      Int spwId = dataDescription().spectralWindowId()(ddId);
      Int sbKey = obsId+10000*spwId;
      String sbsUid("uid://SAFFEC0C0/X1/X1");
      ostringstream oss;
      oss << sbKey;
      sbsUid = sbsUid + String(oss);
      EntityRef sbSummaryUID(sbsUid.c_str(), "", "ASDM", asdmVersion_p); // will be reset later when linking the ASDM to an APDM 
      EntityRef projectUID("uid://SAFFEC0C0/X1/X2", "", "ASDM", asdmVersion_p); // dto.
      EntityRef obsUnitSetId("uid://SAFFEC0C0/X1/X3", "", "ASDM", asdmVersion_p); // dto. ???
      double frequency = (spectralWindow().refFrequencyQuant()(spwId)).getValue(unitASDMFreq()); 
      ReceiverBandMod::ReceiverBand frequencyBand; // get from frequency
      ReceiverSidebandMod::ReceiverSideband rSB;
      setRecBands(Frequency(frequency), frequencyBand, rSB); 
      
      SBTypeMod::SBType sbType = SBTypeMod::OBSERVATORY; //???
      Vector< Quantum< Double > > tRange;
      tRange.reference(observation().timeRangeQuant()(obsId)); 
      Double durationSecs = tRange[1].getValue("s") - tRange[0].getValue("s"); 
      if(durationSecs == 0){ // try to derive the sched block duration in a different way
	if(!warned){
	  os << LogIO::WARN << "Observation time range is zero length for obs ID "
	     << obsId << " in MS Observation table.\n Will try to proceed ..."
	     <<LogIO::POST;
	  warned = True;
	}
	if((Int)observation().nrow()-1 == obsId){ // there is no next observation
	  durationSecs = timestampEndSecs(nMainTabRows-1) - tRange[0].getValue("s");
	}
	else{
	  Vector< Quantum< Double > > tRange2;
	  tRange2.reference(observation().timeRangeQuant()(obsId+1)); 
	  durationSecs = tRange2[0].getValue("s") - tRange[0].getValue("s");
	}
      } 
      Interval sbDuration = ASDMInterval(durationSecs);
      int numberRepeats = 1;
      // limit the scheduling block duration (not the same as the observation duration)
      if(durationSecs > schedBlockDuration_p){
	sbDuration = ASDMInterval(schedBlockDuration_p);
	numberRepeats = (int) ceil(durationSecs/schedBlockDuration_p);
	durationSecs = schedBlockDuration_p;
      }
      vector< Angle > centerDirection;
      Int fId = fieldId()(mainTabRow);
      // an observation (and an SB) can have many fields. use the first one as representative direction
      if(firstFieldIdFromObsId.isDefined(obsId)){
	fId = firstFieldIdFromObsId(obsId);
      }
      else{
	firstFieldIdFromObsId.define(obsId,fId);
      } 
      MDirection theFieldDir = field().phaseDirMeas(fId,0);
      centerDirection.push_back( theFieldDir.getAngle( unitASDMAngle() ).getValue()(0) ); // RA
      centerDirection.push_back( theFieldDir.getAngle( unitASDMAngle() ).getValue()(1) ); // DEC
      
      int numObservingMode = 1;
      vector< string > observingMode;
      observingMode.push_back("observing mode t.b.d.");
      int numScienceGoal = 1;
      vector< string > scienceGoal;
      scienceGoal.push_back("science goal t.b.d.");
      int numWeatherConstraint = 1;
      vector< string > weatherConstraint;
      weatherConstraint.push_back("weather constraint t.b.d.");
      
      tR = tT.newRow(sbSummaryUID, projectUID, obsUnitSetId, frequency, frequencyBand, sbType, sbDuration, 
		     centerDirection, numObservingMode, observingMode, numberRepeats, numScienceGoal, 
		     scienceGoal, numWeatherConstraint, weatherConstraint);
      
      asdm::SBSummaryRow* tR2 = 0;
      tR2 = tT.add(tR);
      Tag sBSummaryTag = tR2->getSBSummaryId();
      if(tR2 == tR){ // adding the row led to the creation of a new tag
	if(verbosity_p>1){
	  cout << "New SBSummary tag created: " << tR2 << endl;
	}
	if(asdmSBSummaryId_p.isDefined(sbKey)){
	  os << LogIO::WARN << "There is more than one scheduling block necessary for the obsid - spwid pair (" 
	     << obsId << ", " << spwId << ").\n This can presently not yet be handled properly.\n" 
	     << "(MS Main table row " << mainTabRow << ")" << LogIO::POST;
	}
	else{
	  asdmSBSummaryId_p.define(sbKey, sBSummaryTag);
	}
      }

      // now have a valid SBSummaryID for the execblock

      // Step 2: write exec block table

      // has the exec block been started already?
      if(execBlockStartTime.isDefined(sBSummaryTag)){ // yes

	// continue accumulation of min and max baseline
	Double baseLine = MVBaseline( (antenna().positionMeas()(antenna1()(mainTabRow))).getValue(),
				      (antenna().positionMeas()(antenna2()(mainTabRow))).getValue()
				      ).getLength().getValue(unitASDMLength());
	if(baseLine>maxBaseline(sBSummaryTag)){
	  maxBaseline.remove(sBSummaryTag);
	  maxBaseline.define(sBSummaryTag, baseLine);
	}
	else if(baseLine<minBaseline(sBSummaryTag)){
	  minBaseline.remove(sBSummaryTag);
	  minBaseline.define(sBSummaryTag, baseLine);
	}

	// is the exec block complete?
	Double endT = timestampEndSecs(mainTabRow);
	execBlockEndTime.remove(sBSummaryTag);
	execBlockEndTime.define(sBSummaryTag, endT);
	if(endT - execBlockStartTime(sBSummaryTag) >= durationSecs){ // yes, it is complete
	  
	  // parameters for a new row
  
	  ArrayTime startTime = ASDMArrayTime(execBlockStartTime(sBSummaryTag));
	  ArrayTime endTime = ASDMArrayTime(endT);
	  int execBlockNum = execBlockNumber(sBSummaryTag);
	  EntityRef execBlockUID;
	  try{
	    execBlockUID = EntityRef(getCurrentUid(), "", "ASDM", asdmVersion_p);
	  }
	  catch(asdm::InvalidArgumentException x){
	    os << LogIO::SEVERE << "Error creating ASDM:  UID \"" << getCurrentUid() 
	       << "\" (intended for an exec block) is not a valid Entity reference: " <<  x.getMessage()
	       << LogIO::POST;      
	    return False;
	  }
	  incrementUid();
	  EntityRef projectId = projectUID;
	  string configName = "configName t.b.d."; // ???
	  string telescopeName = telName_p;
	  string observerName = observation().observer()(obsId).c_str();
	  string observingLog = "";
	  Vector< String > sV;
	  if(observation().log().isDefined(obsId)){// log string array not empty
	    sV.reference(observation().log()(obsId)); // the observation log is an array of strings
	    for(uInt i=0; i<sV.size(); i++){
	      if(i>0){
		observingLog += "\n";
	      }
	      observingLog += string(sV[i].c_str());
	    }
	  }
	  string sessionReference = "sessionReference t.b.d."; // ???
	  EntityRef sbSummary = sbSummaryUID;
	  string schedulerMode = "CASA exportasdm"; //???
	  Length baseRangeMin = Length( minBaseline(sBSummaryTag) );
	  Length baseRangeMax = Length( maxBaseline(sBSummaryTag) );
	  Length baseRmsMinor = Length(0); // ???
	  Length baseRmsMajor = Length(0); // ???
	  Angle basePa = Angle(0); // ???
	  bool aborted = False;
	  asdm::ConfigDescriptionRow* cDR = correspConfigDescRow(sBSummaryTag);
	  int numAntenna = cDR->getNumAntenna();
	  vector< Tag > antennaId = cDR->getAntennaId();

	  tER = tET.newRow(startTime, endTime, execBlockNum, execBlockUID, projectId, configName, telescopeName,
			   observerName, observingLog, sessionReference, sbSummary, schedulerMode, baseRangeMin,
			   baseRangeMax, baseRmsMinor, baseRmsMajor, basePa, siteAltitude, siteLongitude, siteLatitude, 
			   aborted, numAntenna, antennaId, sBSummaryTag);
	  
	  asdm::ExecBlockRow* tER2;

	  tER2 = tET.add(tER);
	  if(tER2 != tER){
	    os << LogIO::SEVERE << "Internal error: attempt to store duplicate exec block row." << LogIO::POST;
	    return False;
	  }
	  asdmExecBlockId_p.define(execBlockStartTime(sBSummaryTag), tER->getExecBlockId());

	  // undefine the mapping for this Tag since the ExecBlock was completed
	  execBlockStartTime.remove(sBSummaryTag);
	  execBlockEndTime.remove(sBSummaryTag);
	  correspConfigDescRow.remove(sBSummaryTag);
	  obsIdFromSBSum.remove(sBSummaryTag);
	  minBaseline.remove(sBSummaryTag);
	  maxBaseline.remove(sBSummaryTag);
	  
	}
	//	else{ // no, it is not complete	 
	//	}
      } 
      else{// no, it has not been started, yet

	execBlockStartTime.define(sBSummaryTag, timestampStartSecs(mainTabRow));
	execBlockEndTime.define(sBSummaryTag, timestampEndSecs(mainTabRow)); // will be updated
	Int oldNum = 0;
	if(execBlockNumber.isDefined(sBSummaryTag)){ 
	  // increment exec block number
	  oldNum = execBlockNumber(sBSummaryTag);
	  execBlockNumber.remove(sBSummaryTag);
	}
	execBlockNumber.define(sBSummaryTag, oldNum + 1); // sequential numbering starting at 1

	obsIdFromSBSum.define(sBSummaryTag, obsId); // remember the obsId for this exec block
	
	if(!asdmConfigDescriptionId_p.isDefined(mainTabRow)){
	  os << LogIO::SEVERE << "Internal error: undefined config description id for MS main table row "
	     << mainTabRow << LogIO::POST;
	  return False;
	}  
	asdm::ConfigDescriptionRow* cDR = (ASDM_p->getConfigDescription()).getRowByKey(asdmConfigDescriptionId_p(mainTabRow));
	if(cDR ==0){
	  os << LogIO::SEVERE << "Internal error: no row in ASDM ConfigDesc Table for ConfigDescriptionId stored for main table row "
	     << mainTabRow << LogIO::POST;
	  return False;
	}
	correspConfigDescRow.define(sBSummaryTag, cDR); // remember the config description row for this exec block

	// start accumulation of min and max baseline
	Double bLine = MVBaseline( (antenna().positionMeas()(antenna1()(mainTabRow))).getValue(),
				   (antenna().positionMeas()(antenna2()(mainTabRow))).getValue()
				   ).getLength().getValue(unitASDMLength());
	minBaseline.define(sBSummaryTag, bLine);
	maxBaseline.define(sBSummaryTag, bLine);
      }

      // skip rest of this timestamp
      Double tStamp = time()(mainTabRow);
      while(mainTabRow<nMainTabRows 
	    && time()(mainTabRow)==tStamp
	    && dataDescId()(mainTabRow)==ddId){
	mainTabRow++;
      }
      mainTabRow--;// we are inside a for loop which will perform the last mainTabRow++ 
    } // end loop over main table

    // are there pending exec blocks?
    while(execBlockStartTime.ndefined()>0){ // yes
      Tag sBSummaryTag = execBlockStartTime.getKey(0); 
      tR = tT.getRowByKey(sBSummaryTag);
      Int obsId = obsIdFromSBSum(sBSummaryTag);
      // parameters for a new row      
      ArrayTime startTime = ASDMArrayTime(execBlockStartTime(sBSummaryTag));
      ArrayTime endTime = ASDMArrayTime(execBlockEndTime(sBSummaryTag));
      int execBlockNum = execBlockNumber(sBSummaryTag);
      EntityRef execBlockUID;
      try{
	execBlockUID = EntityRef(getCurrentUid(), "", "ASDM", asdmVersion_p);
      }
      catch(asdm::InvalidArgumentException x){
	os << LogIO::SEVERE << "Error creating ASDM:  UID \"" << getCurrentUid() 
	   << "\" (intended for an exec block) not a valid Entity reference: " <<  x.getMessage()
	   << LogIO::POST;      
	return False;
      }
      incrementUid();
      EntityRef projectId = tR->getProjectUID();
      string configName = "configName t.b.d."; // ???
      string telescopeName = telName_p;
      string observerName = observation().observer()(obsId).c_str();
      string observingLog = "";
      Vector< String > sV;
      if(observation().log().isDefined(obsId)){ // string array column not empty
	sV.reference(observation().log()(obsId)); // the observation log is an array of strings
	for(uInt i=0; i<sV.size(); i++){
	  if(i>0){
	    observingLog += "\n";
	  }
	  observingLog += string(sV[i].c_str());
	}
      }
      string sessionReference = "sessionReference t.b.d."; // ???
      EntityRef sbSummary = tR->getSbSummaryUID();
      string schedulerMode = "CASA exportasdm"; //???
      Length baseRangeMin = Length( minBaseline(sBSummaryTag) );
      Length baseRangeMax = Length( maxBaseline(sBSummaryTag) );
      Length baseRmsMinor = Length(0); // ???
      Length baseRmsMajor = Length(0); // ???
      Angle basePa = Angle(0); // ???
      bool aborted = False;
      asdm::ConfigDescriptionRow* cDR = correspConfigDescRow(sBSummaryTag);
      int numAntenna = cDR->getNumAntenna();
      vector< Tag > antennaId = cDR->getAntennaId();
      
      tER = tET.newRow(startTime, endTime, execBlockNum, execBlockUID, projectId, configName, telescopeName,
		       observerName, observingLog, sessionReference, sbSummary, schedulerMode, baseRangeMin,
		       baseRangeMax, baseRmsMinor, baseRmsMajor, basePa, siteAltitude, siteLongitude, siteLatitude, 
		       aborted, numAntenna, antennaId, sBSummaryTag);
      
      asdm::ExecBlockRow* tER2;
      
      tER2 = tET.add(tER);
      if(tER2 != tER){
	os << LogIO::SEVERE << "Internal error: attempt to store duplicate exec block row." << LogIO::POST;
	return False;
      }
      asdmExecBlockId_p.define(execBlockStartTime(sBSummaryTag), tER->getExecBlockId());

      // undefine the mapping for this Tag since the ExecBlock was completed
      execBlockStartTime.remove(sBSummaryTag); // need only remove from the map which is tested

    }

    // finish the SBSummary table
    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled SBSummary table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();

    // finish the ExecBlock table
    theUid = getCurrentUid();
    ent = tET.getEntity();
    ent.setEntityId(theUid);
    tET.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled ExecBlock table " << getCurrentUid() << " with " << tET.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();
    
    return rstat;
    
  }

  Bool MS2ASDM::writeMainAndScanAndSubScan(const String& datacolumn){
    LogIO os(LogOrigin("MS2ASDM", "writeMainAndScanAndSubScan()"));
    
    Bool rstat = True;
    
    // check if datacolumn exists
    
    if( (datacolumn == "CORRECTED" && !ms_p.tableDesc().isColumn("CORRECTED_DATA"))
	|| (datacolumn == "MODEL" && !ms_p.tableDesc().isColumn("MODEL_DATA"))
	|| (datacolumn == "DATA" && !ms_p.tableDesc().isColumn("DATA"))
	){
      os << LogIO::SEVERE << "Error: column " << datacolumn << " does not exist in "
	 << ms_p.tableName() << LogIO::POST;      
      return False;
    }
    else{
      if(verbosity_p>0){
	os << LogIO::NORMAL << "Using column " << datacolumn << "." << LogIO::POST;      
      }
    }      
    
    asdm::MainTable& tT = ASDM_p->getMain();
    asdm::MainRow* tR = 0;
    
    asdm::ScanTable& tST = ASDM_p->getScan();
    asdm::ScanRow* tSR = 0;
    
    asdm::SubscanTable& tSST = ASDM_p->getSubscan();
    asdm::SubscanRow* tSSR = 0;
    
    
    // Scheme
    // loop over main table
    //   asdmExecBlockId_p(time()(mainTabRow) defined?
    //   if so, a new exec block has started
    //      is there a previous exec block?
    //        if so, finish it
    //      set up new exec block
    //      set scanNumber to 1
    //      set subscanNumber to 1
    //   end if
    //   while(scan not finished)
    //      find all DDIds in the time between now and now+subscanduration or until Scan ends
    //        and memorize their start rows
    //      for each DDId found
    //        find all FieldIds in the time between now and now+subscanduration or until Scan ends
    //        for each FieldId
    //           Construct subscan == SDMDataObject
    //              using the ConfigDescription table row asdmConfigDescriptionId_p(startRow)
    //           and write it
    //           write corresponding Main table row and subscan table row
    //           subscanNumber++
    //        end for
    //      end for
    //      next mainTabRow
    //      error if asdmExecBlockId_p(time()(mainTabRow) defined: 
    //   end while
    //   write scan table row
    //   scanNumber++
    // end loop (i.e. next main table row)  
    
    
    ////////////////////////////////////////////////
    // Prepare loop over main table
    Double scanStart;
    
    Tag execBlockId = Tag();
    int subscanNumber = 0;
    // parameters for the new Scan table row
    int scanNumber = 0;
    ArrayTime scanStartTime;
    ArrayTime scanEndTime;
    int scanNumIntent = 0;
    int numSubScan = 0;
    vector< ScanIntentMod::ScanIntent > scanIntent;
    vector< CalDataOriginMod::CalDataOrigin > scanCalDataType;
    vector< bool > scanCalibrationOnLine;
    bool scanFlagRow = False; // always false ???
    
    uInt nMainTabRows = ms_p.nrow();
    for(uInt mainTabRow=0; mainTabRow<nMainTabRows; mainTabRow++){
      
      Double rowTime = timestampStartSecs(mainTabRow);
      
      //   asdmExecBlockId_p defined for this timestamp?
      if(asdmExecBlockId_p.isDefined(timestampStartSecs(mainTabRow))){ // a new exec block has started
	// is there a previous exec block?
	if(execBlockId != Tag()){
	  //  finish the old exec block
	}
	// set up new exec block
	// parameters for the first scan
	execBlockId = asdmExecBlockId_p(timestampStartSecs(mainTabRow));
	asdm::ExecBlockRow* EBR = (ASDM_p->getExecBlock()).getRowByKey(execBlockId);
	scanNumber = 1; // ASDM scan numbering starts at 1
        subscanNumber = 1; // dito for subscans
	scanStart = rowTime;
	scanStartTime = ASDMArrayTime( scanStart );
	scanEndTime = EBR->getEndTime(); // preset to the end of the execblock
	scanNumIntent = 0;
	numSubScan = 0;
	scanIntent.resize(0);
	scanCalDataType.resize(0);
	scanCalibrationOnLine.resize(0);
	scanFlagRow = False;
      } // end if a new exec block has started
      else if(execBlockId == Tag()){
	cout << "Encoutered main tab row " << mainTabRow << " which is not part of an execblock." << endl;
	continue;
      }
      
      // while(scan not finished)
      while( mainTabRow<nMainTabRows && 
	     (rowTime = timestampStartSecs(mainTabRow)) < MSTimeSecs(scanEndTime) ){ // presently one scan per exec block ???
	
	// parameters for the new SubScan table row
	Double subScanEnd = rowTime + subscanDuration_p; 
	if(subScanEnd > MSTimeSecs(scanEndTime)){
	  subScanEnd = MSTimeSecs(scanEndTime);
	}
	
	// find all DDIds in the time between now and end of subscan
	uInt irow = mainTabRow;
	SimpleOrderedMap< Int, uInt > subScanDDIdStartRows(0);
	while(irow<nMainTabRows &&
	      timestampStartSecs(irow) < subScanEnd){
	  Int ddId = dataDescId()(irow);
	  if(!subScanDDIdStartRows.isDefined(ddId)){
	    subScanDDIdStartRows.define(ddId, irow);    // memorize their start rows
	  }
	  irow++;
	}
	
	// for each DDId found
	for(uInt ddIndex=0; ddIndex < subScanDDIdStartRows.ndefined(); ddIndex++){
	  Int theDDId = subScanDDIdStartRows.getKey(ddIndex);

	  // find all FieldIds in the time between now and now+subscanduration or until Scan ends	  
	  uInt irow2 = mainTabRow;
	  SimpleOrderedMap< Int, uInt > subScanStartRows(0);
	  SimpleOrderedMap< Int, uInt > subScanEndRows(0);
	  while(irow2<nMainTabRows &&
		timestampStartSecs(irow2)< subScanEnd){
	    Int ddId = dataDescId()(irow2);
	    Int fId = fieldId()(irow2);
	    if(ddId == theDDId){
	      if(subScanEndRows.isDefined(fId)){
		subScanEndRows.remove(fId);
		subScanEndRows.define(fId, irow2);    // update end row
	      }
	      if(!subScanStartRows.isDefined(fId)){
		subScanStartRows.define(fId, irow2);    // memorize their start rows
		subScanEndRows.define(fId, irow2);    // and end rows
	      }
	    }
	    irow2++;
	  }
	  // for each FieldId
	  for(uInt fIndex=0; fIndex < subScanStartRows.ndefined(); fIndex++){
	    Int theFId = subScanStartRows.getKey(fIndex);
	    uInt startRow = subScanStartRows(theFId);
	    uInt endRow = subScanEndRows(theFId);
	    // write subscan
	    // parameters for the new SubScan table row
	    ArrayTime subScanStartArrayTime = ASDMArrayTime(timestampStartSecs(startRow)); 
	    ArrayTime subScanEndArrayTime = ASDMArrayTime(timestampEndSecs(endRow)); 
	    string fieldName = field().name()(fieldId()(startRow)).c_str(); 
	    SubscanIntentMod::SubscanIntent subscanIntent = SubscanIntentMod::ON_SOURCE;
	    vector< int > numberSubintegration;
	    bool flagRow = False;

	    // parameters for the corresponding new Main table row
	    ArrayTime mainTime = subScanStartArrayTime;

	    if(!asdmConfigDescriptionId_p.isDefined(startRow)){
	      os << LogIO::SEVERE << "Internal error: undefined config description id for MS main table row "
		 << startRow << LogIO::POST;
	      return False;
	    }  
	    Tag configDescriptionId = asdmConfigDescriptionId_p(startRow);
	    asdm::ConfigDescriptionRow* CDR = (ASDM_p->getConfigDescription()).getRowByKey(configDescriptionId);
	    if(CDR ==0){
	      os << LogIO::SEVERE << "Internal error: no row in ASDM ConfigDesc Table for ConfigDescriptionId stored for main table row "
		 << startRow << LogIO::POST;
	      return False;
	    }

	    Tag fieldIdTag = asdmFieldId_p(theFId);
	    int numAntenna = CDR->getNumAntenna();
	    TimeSamplingMod::TimeSampling timeSampling = TimeSamplingMod::INTEGRATION;
	    Interval interval = ASDMInterval(timestampEndSecs(endRow) - timestampStartSecs(startRow));
	    int numIntegration; // to be set by the following method call
	    int dataSize; // to be set by the following method call
	    EntityRef dataOid; // to be set by the following method call
	    vector< Tag > stateIdV; // "
	  
	    numIntegration = writeMainBinSubScanForOneDDIdFIdPair(theDDId, theFId, 
								  datacolumn, 
								  scanNumber, subscanNumber,
								  startRow, endRow,
								  execBlockId,
								  dataSize, dataOid, stateIdV);
	    if(numIntegration<0){ // error!
	      os << LogIO::SEVERE << "Error writing Subscan starting at main table row " 
		 << startRow << LogIO::POST;
	      return False;
	    }

	    for(uInt i=0; i<(uInt)numIntegration; i++){
	      numberSubintegration.push_back(0); // no subintegrations for the moment, no channel averaging (???)
	    }
	  
	    // end write subscan
	  
	    // write corresponding Main table row
	    tR = tT.newRow(mainTime, configDescriptionId, fieldIdTag, numAntenna, timeSampling, interval, numIntegration, 
			   scanNumber, subscanNumber, dataSize, dataOid, stateIdV, execBlockId);
	    tT.add(tR);
	  
	    // write corresponding Subscan table row
	    
	    tSSR = tSST.newRow(execBlockId, scanNumber, subscanNumber, subScanStartArrayTime, subScanEndArrayTime, 
			       fieldName, subscanIntent, numIntegration, numberSubintegration, flagRow);
	    
	    tSST.add(tSSR);
	    
	    subscanNumber++;
	    numSubScan++;

	  } // end loop over Field indices         

	} // end loop over DD indices
	// update mainTabRow
	mainTabRow = irow;
	//cout << "mainTabRow " << mainTabRow << endl;
      } // end while scan continues
      // scan finished
      // complete and write scan table row
      mainTabRow--; // return to last row of the scan
      scanEndTime = ArrayTime( timestampEndSecs(mainTabRow) );	  
      scanNumIntent = 1; // hardwired (???)
      for(uInt i=0; i<(uInt)scanNumIntent; i++){
	scanIntent.push_back(ScanIntentMod::OBSERVE_TARGET); // hardwired for the moment (???)
	scanCalDataType.push_back(CalDataOriginMod::FULL_RESOLUTION_CROSS); // hardwired for the moment (???)
	scanCalibrationOnLine.push_back(False); // hardwired for the moment (???)
      }

      tSR = tST.newRow(execBlockId, scanNumber, scanStartTime, scanEndTime, scanNumIntent, numSubScan, scanIntent, 
		       scanCalDataType, scanCalibrationOnLine, scanFlagRow);
      
      tST.add(tSR);
      
      scanNumber++;
      
    }//  end for

    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled Main table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();
    
    theUid = getCurrentUid();
    ent = tST.getEntity();
    ent.setEntityId(theUid);
    tST.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled Scan table " << getCurrentUid() << " with " << tST.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();
    
    theUid = getCurrentUid();
    ent = tSST.getEntity();
    ent.setEntityId(theUid);
    tSST.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled SubScan table " << getCurrentUid() << " with " << tSST.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();
    
    return rstat;
  }

  Bool MS2ASDM::writePointingModel(){ // create asdm PointingModel table 
    LogIO os(LogOrigin("MS2ASDM", "writePointingModel()"));
    
    Bool rstat = True;
    
    asdm::PointingModelTable& tT = ASDM_p->getPointingModel();
    
    asdm::PointingModelRow* tR = 0;
    
    asdm::FeedTable& tFT = ASDM_p->getFeed();
    
    vector< asdm::FeedRow * > feedRowV = tFT.get();

    // loop over ASDM Feed table and create Pointing model rows for each
    // receptor of each Feed table row 

    for(uInt feedRow=0; feedRow<feedRowV.size(); feedRow++){
      

      uInt numRec = feedRowV[feedRow]->getNumReceptor();
      vector< PolarizationTypeMod::PolarizationType > polTypeV = feedRowV[feedRow]->getPolarizationTypes();
      vector< int > receivIdV = feedRowV[feedRow]->getReceiverId(); 

      for(uInt iRec=0; iRec<numRec; iRec++){ // loop over all receptors

	const vector< asdm::ReceiverRow * > recRowV = feedRowV[feedRow]->getReceivers(receivIdV[iRec]);
	
	// parameters for new PointingModel row

	Tag antennaId = feedRowV[feedRow]->getAntennaId();
	int numCoeff = 2; // seems to be the dummy value ???, to be confirmed
	vector< string > coeffName;
	coeffName.push_back("IA");
	coeffName.push_back("IE");
	vector< float > coeffVal;
	coeffVal.push_back(0.);
	coeffVal.push_back(0.);
	PolarizationTypeMod::PolarizationType polarizationType;
	try{
	  polarizationType = polTypeV[iRec];
	}
	catch(std::string z){
	  os << LogIO::SEVERE << "Internal error: invalid polarization type in Feed table row  " << z
	     << LogIO::POST;      
	  return False;
	}      
	  
	ReceiverBandMod::ReceiverBand receiverBand = recRowV[0]->getFrequencyBand(); // take from the first receiver 
	string assocNature = "NOT_SET";
	int assocPointingModelId = -1;

        tR = tT.newRow(antennaId, numCoeff, coeffName, coeffVal, polarizationType, 
		       receiverBand, assocNature, assocPointingModelId);
    
	asdm::PointingModelRow* tR2;

	tR2 = tT.add(tR);
	if(!asdmPointingModelId_p.isDefined(antennaId)){
	  asdmPointingModelId_p.define(antennaId, tR2->getPointingModelId() );
	}

      } // end loop over receptors

    } // end loop over ASDM Feed table
    
    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled PointingModel table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();
    
    return rstat;
  }

  Bool MS2ASDM::writePointing(){ // create asdm pointing table
    LogIO os(LogOrigin("MS2ASDM", "writePointing()"));
    
    Bool rstat = True;
    
    asdm::PointingTable& tT = ASDM_p->getPointing();
    
    asdm::PointingRow* tR = 0;

    uInt nPointingRows = pointing().nrow();
   
    if(nPointingRows==0){
      os << LogIO::WARN << "MS Pointing table doesn't exist or is empty." << LogIO::POST;	
      return True; // not an error
    }

    Bool warned = False; // aux. var. to avoid repetition of warnings

    // loop over MS antenna table
    for(Int aId=0; aId<(Int)antenna().nrow(); aId++){

      if(!asdmAntennaId_p.isDefined(aId)){
	os << LogIO::SEVERE << "Internal error: no tag defined for antenna id " 
	   << aId << LogIO::POST;
	return False;
      }

      uInt irow=0;
      uInt totNumRows=0; // total of MS Pointing table rows for this antenna

      while(irow<nPointingRows){

	// find next pointing entry for this antenna in MS pointing table (== beginning of a new ASDM pointing table entry)
	Int firstRow=-1;
	while(irow<nPointingRows){
	  if(pointing().antennaId()(irow) == aId){
	    firstRow = irow;
	    break;
	  }
	  irow++;
	}

	if(firstRow==-1){ // no further data for this antenna
	  break;
	}

	uInt numRows=1; // number of rows with contiguous timestamps for this antenna 
	
	// parameters for the next pointing table row
	Tag antennaId = asdmAntennaId_p(aId);
	ArrayTimeInterval timeInterval( ASDMTimeInterval( pointing().timeQuant()(firstRow), pointing().intervalQuant()(firstRow)) );

	bool pointingTracking = pointing().tracking()(firstRow);

	bool usePolynomials = False;
	int numTerm = 0; // to be updated later

	if(pointing().numPoly()(firstRow)>0){
	  usePolynomials = True;
	  numTerm = pointing().numPoly()(irow)+1;
	}

	ArrayTime timeOrigin = ASDMArrayTime( pointing().timeOriginQuant()(firstRow).getValue("s") );
	
	vector< vector< Angle > > pointingDirection;
	Vector< MDirection > dirV; // aux. vector to access array column
	dirV.reference(pointing().directionMeasCol()(firstRow));
	if(numTerm == 0){
	  pointingDirection.push_back(ASDMAngleV(dirV[0]));
	}
	else{
	  if(numTerm != (Int)dirV.size()){
	    os << LogIO::SEVERE << "Inconsistent MS: in pointing table row " << firstRow
	       << ": numpoly + 1 should be == dimension of array DIRECTION." << LogIO::POST;	
	    return False;
	  }	    
	  for(uInt i=0; i<(uInt)numTerm; i++){
	    pointingDirection.push_back(ASDMAngleV(dirV[i]));
	  }
	}	     
	
	vector< vector< Angle > > encoder;
	if(pointing().encoderMeas().isNull()){ // encoder column is optional in MS but not in ASDM
	  if(numTerm == 0){
	    // use pointing Direction instead
	    if(!warned){
	      os << LogIO::WARN << "No ENCODER column in MS Pointing table. Will use DIRECTION instead." 
		 << LogIO::POST;	
	      warned = True;
	    }
	    dirV.reference(pointing().directionMeasCol()(firstRow));
	    encoder.push_back(ASDMAngleV(dirV[0]));
	  }
	  else{ // cannot use pointing direction because it contains only polynomial terms
	    vector< Angle > angV;
	    angV.push_back(Angle(0.));
	    angV.push_back(Angle(0.));
	    if(!warned){
	      os << LogIO::WARN << "No ENCODER column in MS Pointing table. Will fill with zeros." 
		 << LogIO::POST;	
	      warned = True;
	    }
	    encoder.push_back(angV);
	  }
	}
	else{
	  encoder.push_back(ASDMAngleV(pointing().encoderMeas()(firstRow)));
	}
	
	vector< vector< Angle > > target;
	dirV.reference(pointing().targetMeasCol()(firstRow));
	if(numTerm == 0){
	  target.push_back(ASDMAngleV(dirV[0]));
	}
	else{
	  if(numTerm != (Int)dirV.size()){
	    os << LogIO::SEVERE << "Inconsistent MS: in pointing table row " << firstRow
	       << ": numpoly + 1 should be == dimension of array TARGET." << LogIO::POST;	
	    return False;
	  }	    
	  for(uInt i=0; i<(uInt)numTerm; i++){
	    target.push_back(ASDMAngleV(dirV[i]));
	  }
	}	     
	
	vector< vector< asdm::Angle > > offset;
	// source offset column is optional in the MS but not in the ASDM
	if(pointing().pointingOffsetMeasCol().isNull()){ // no MS source offset column 
	  vector< Angle > angV;
	  angV.push_back(Angle(0.));
	  angV.push_back(Angle(0.));
	  offset.push_back(angV);
	  if(numTerm>0){
	    for(uInt i=1; i<(uInt)numTerm; i++){
	      offset.push_back(angV);
	    }
	  }
	}
	else{
	  dirV.reference(pointing().pointingOffsetMeasCol()(firstRow));
	  if(numTerm==0){
	    offset.push_back(ASDMAngleV(dirV[0]));
	  }
	  else{
	    if(numTerm != (Int)dirV.size()){
	      os << LogIO::SEVERE << "Inconsistent MS: in pointing table row " << firstRow
		 << ": numpoly + 1 should be == dimension of array POINTING_OFFSET." << LogIO::POST;	
	      return False;
	    }	    
	    for(uInt i=0; i<(uInt)numTerm; i++){
	      offset.push_back(ASDMAngleV(dirV[i]));
	    }
	  }
	}	
	
	vector< vector< Angle > > sourceOffset; // optional in the ASDM and in the MS
	if(!pointing().sourceOffsetMeasCol().isNull()){ // sourceOffset column is present
	  dirV.reference(pointing().sourceOffsetMeasCol()(firstRow));
	  if(numTerm==0){
	    sourceOffset.push_back(ASDMAngleV(dirV[0]));
	  }
	  else{
	    if(numTerm != (Int)dirV.size()){
	      os << LogIO::SEVERE << "Inconsistent MS: in pointing table row " << firstRow
		 << ": numpoly + 1 should be == dimension of array SOURCE_OFFSET." << LogIO::POST;	
	      return False;
	    }	    
	    for(uInt i=0; i<(uInt)numTerm; i++){
	      sourceOffset.push_back(ASDMAngleV(dirV[i]));
	    }
	  }
	} 
	
	int pointingModelId = asdmPointingModelId_p(antennaId);

	// check if there are more rows for this antenna with adjacent time intervals

	Double endTime = pointing().time()(firstRow) + pointing().interval()(firstRow)/2.;

	irow++;
	
	while(irow<nPointingRows && numTerm==0){ // while we find more adjacent rows and don't use polynomials

	  if(pointing().antennaId()(irow) != aId){
	    irow++;
	    continue;
	  }

	  if( (endTime != pointing().time()(irow) - pointing().interval()(irow)/2.) // row irow is adjacent in time to previous row
	      || pointingTracking != pointing().tracking()(irow) 
	      || (usePolynomials == False && pointing().numPoly()(irow)>0)
	      ){
	    break; // there will be no further samples for this ASDM pointing table row
	  }

	  // found a scond pointing row for the antenna ID, now accumulate directions
	  // until time ranges not contiguous or end of table

	  numRows++;
	  endTime = pointing().time()(irow) + pointing().interval()(irow)/2.;

	  // update time interval
	  timeInterval.setDuration( timeInterval.getDuration() + ASDMInterval(pointing().intervalQuant()(irow).getValue("s")) );

	  dirV.reference(pointing().directionMeasCol()(irow));
	  pointingDirection.push_back(ASDMAngleV(dirV[0]));
	
	  if(pointing().encoderMeas().isNull()){ // encoder column is optional
	    // use pointing Direction instead
	    encoder.push_back(ASDMAngleV(dirV[0]));
	  }
	  else{
	    encoder.push_back(ASDMAngleV(pointing().encoderMeas()(irow)));
	  }
	
	  dirV.reference(pointing().targetMeasCol()(irow));
	  target.push_back(ASDMAngleV(dirV[0]));
	
	  if(pointing().pointingOffsetMeasCol().isNull()){ // source offset column is optional
	    vector< Angle > angV;
	    angV.push_back(Angle(0.));
	    angV.push_back(Angle(0.));
	    offset.push_back(angV);
	  }
	  else{
	    dirV.reference(pointing().pointingOffsetMeasCol()(irow));
	    offset.push_back(ASDMAngleV(dirV[0]));
	  }	
	
	  if(!pointing().sourceOffsetMeasCol().isNull()){ // source offset column is optional
	    dirV.reference(pointing().sourceOffsetMeasCol()(irow));
	    sourceOffset.push_back(ASDMAngleV(dirV[0]));
	  }	
	
	} // end while accumulating for started  entry

	// finish the ASDM table row and add it to the table
	int numSample = encoder.size(); 
	if(!usePolynomials){
	  numTerm = numSample;
	}
	
	tR = tT.newRow(antennaId, timeInterval, numSample, encoder, pointingTracking, usePolynomials, timeOrigin, 
		       numTerm, pointingDirection, target, offset, pointingModelId);
	
	if(sourceOffset.size()>0){
	  tR->setSourceOffset(sourceOffset);
	} 

	asdm::PointingRow* tR2;
	
	tR2 = tT.add(tR);
	if(tR2 != tR){// no new row was inserted
	  os << LogIO::WARN << "Internal error: duplicate row in Pointing table for antenna id " 
	     << aId << LogIO::POST;	
	}
	else{
	  if(verbosity_p>0){
	    os << LogIO::NORMAL << "Combined " << numRows 
	       << " MS Pointing table rows into one ASDM Pointing table row for antenna id " 
	       << aId << LogIO::POST;	
	  }
	}
	totNumRows += numRows;
	
      } // end while rows left in pointing table
      
      if(totNumRows==0){
	os << LogIO::WARN << "No MS Pointing table rows found for antenna id " 
	    << aId << LogIO::POST;	
      }	

    } // end loop over antenna ids
    
    EntityId theUid(getCurrentUid());
    Entity ent = tT.getEntity();
    ent.setEntityId(theUid);
    tT.setEntity(ent);
    if(verbosity_p>0){
      os << LogIO::NORMAL << "Filled Pointing table " << getCurrentUid() << " with " << tT.size() << " rows ..." << LogIO::POST;
    }
    incrementUid();
    
    return rstat;
  }

  StokesParameterMod::StokesParameter MS2ASDM::ASDMStokesParameter( Stokes::StokesTypes s) {
    switch (s) {
    case Stokes::I:     return StokesParameterMod::I;         
    case Stokes::Q: 	return StokesParameterMod::Q;        
    case Stokes::U: 	return StokesParameterMod::U;        
    case Stokes::V: 	return StokesParameterMod::V;        
    case Stokes::RR: 	return StokesParameterMod::RR;       
    case Stokes::RL: 	return StokesParameterMod::RL;       
    case Stokes::LR: 	return StokesParameterMod::LR;       
    case Stokes::LL: 	return StokesParameterMod::LL;       
    case Stokes::XX: 	return StokesParameterMod::XX;       
    case Stokes::XY: 	return StokesParameterMod::XY;       
    case Stokes::YX: 	return StokesParameterMod::YX;       
    case Stokes::YY: 	return StokesParameterMod::YY;       
    case Stokes::RX: 	return StokesParameterMod::RX;       
    case Stokes::RY: 	return StokesParameterMod::RY;       
    case Stokes::LX: 	return StokesParameterMod::LX;       
    case Stokes::LY: 	return StokesParameterMod::LY;       
    case Stokes::XR: 	return StokesParameterMod::XR;       
    case Stokes::XL: 	return StokesParameterMod::XL;       
    case Stokes::YR: 	return StokesParameterMod::YR;       
    case Stokes::YL: 	return StokesParameterMod::YL;       
    case Stokes::PP: 	return StokesParameterMod::PP;       
    case Stokes::PQ: 	return StokesParameterMod::PQ;       
    case Stokes::QP: 	return StokesParameterMod::QP;       
    case Stokes::QQ: 	return StokesParameterMod::QQ;       
    case Stokes::RCircular: return StokesParameterMod::RCIRCULAR;
    case Stokes::LCircular: return StokesParameterMod::LCIRCULAR;
    case Stokes::Linear: 	return StokesParameterMod::LINEAR;   
    case Stokes::Ptotal: 	return StokesParameterMod::PTOTAL;   
    case Stokes::Plinear: 	return StokesParameterMod::PLINEAR;  
    case Stokes::PFtotal: 	return StokesParameterMod::PFTOTAL;  
    case Stokes::PFlinear: 	return StokesParameterMod::PFLINEAR; 
    case Stokes::Pangle:  	return StokesParameterMod::PANGLE; 
    case Stokes::Undefined:
    default:    
      throw(AipsError("Undefined stokes parameter."+String((Int)s)) );
    }
  }

  AntennaTypeMod::AntennaType MS2ASDM::ASDMAntennaType( const String& type ){
    LogIO os(LogOrigin("MS2ASDM", "(ASDMAntennaType)"));
    if(type == "GROUND-BASED"){
      return AntennaTypeMod::GROUND_BASED;
    }
    else if(type == "SPACE-BASED"){
      return AntennaTypeMod::SPACE_BASED;
    }      
    else{
      os << LogIO::WARN << "Unknown antenna type " << type << " assuming GROUND-BASED." << LogIO::POST;
      return AntennaTypeMod::GROUND_BASED;
    }      
  } 

  ArrayTimeInterval MS2ASDM::ASDMTimeInterval( const Quantity midpoint, const Quantity interval){
    Double sTime = midpoint.getValue("s");  
    Double sInterval = interval.getValue("s");
    if(sInterval >= sTime){ // a very large value was set to express "always valid"
      sTime = timeQuant()(0).getValue("s") - intervalQuant()(0).getValue("s")/2.;
      sInterval =  timeQuant()(ms_p.nrow()-1).getValue("s") - sTime 
	+ intervalQuant()(ms_p.nrow()-1).getValue("s")/2.;
    }
    else{ // still need to make sTime the interval start
      sTime -= sInterval/2.;
    } 
    
    ArrayTimeInterval timeInterval( ASDMArrayTime(sTime),
				    ASDMInterval(sInterval) );
    return timeInterval;
  }

  vector< Angle > MS2ASDM::ASDMAngleV(const MDirection mDir){
    vector< Angle > angV;
    angV.push_back( mDir.getAngle( unitASDMAngle() ).getValue()(0) ); 
    angV.push_back( mDir.getAngle( unitASDMAngle() ).getValue()(1) ); 
    return angV;
  }
    


  BasebandNameMod::BasebandName MS2ASDM::ASDMBBName( Int BBCNo ){
    switch(BBCNo){
    case 1: return BasebandNameMod::BB_1;
    case 2: return BasebandNameMod::BB_2;
    case 3: return BasebandNameMod::BB_3;
    case 4: return BasebandNameMod::BB_4;
    case 5: return BasebandNameMod::BB_5;
    case 6: return BasebandNameMod::BB_6;
    case 7: return BasebandNameMod::BB_7;
    case 8: return BasebandNameMod::BB_8;
    case 0: 
    default:
      return BasebandNameMod::NOBB;
    }
  }

  NetSidebandMod::NetSideband MS2ASDM::ASDMNetSideBand( Int netSideB ){
    switch(netSideB){
    case 1: return NetSidebandMod::LSB;
    case 2: return NetSidebandMod::USB;
    case 3: return NetSidebandMod::DSB;
    case 0:
    default:
      return NetSidebandMod::NOSB;
    }
  }

  FrequencyReferenceCodeMod::FrequencyReferenceCode MS2ASDM::ASDMFreqRefCode( const MFrequency::Types refFrame ){
    LogIO os(LogOrigin("MS2ASDM", "(ASDMFreqRefCode)"));
    switch( refFrame ){
    case MFrequency::LSRD: return FrequencyReferenceCodeMod::LSRD;
    case MFrequency::LSRK: return FrequencyReferenceCodeMod::LSRK;
    case MFrequency::BARY: return FrequencyReferenceCodeMod::BARY;
    case MFrequency::REST: return FrequencyReferenceCodeMod::REST;
    case MFrequency::GEO: return FrequencyReferenceCodeMod::GEO;
    case MFrequency::GALACTO: return FrequencyReferenceCodeMod::GALACTO;
    case MFrequency::TOPO: return FrequencyReferenceCodeMod::TOPO;
    default:
      os << LogIO::SEVERE << "Unsupported CASA reference frame " << MFrequency::showType(refFrame) 
	 << ", assuming TOPO." << LogIO::POST;
      return FrequencyReferenceCodeMod::TOPO; 
    }
  }

  Bool MS2ASDM::stokesTypePresent( const Vector< Int > corrT, 
				   const Stokes::StokesTypes st ){
    Bool rval = False;
    for(uInt j=0; j<corrT.size(); j++){
      if(corrT[j]==static_cast<Int>(st)){
	 rval = True;
      }
    }
    return rval;
  }

  Bool MS2ASDM::setRecBands( const asdm::Frequency refFreq, 
			     ReceiverBandMod::ReceiverBand& frequencyBand,
			     ReceiverSidebandMod::ReceiverSideband& receiverSideband){

    Quantity repFreq( refFreq.get(), String(Frequency::unit()) );
    Double repFreqGHz = repFreq.getValue("GHz");
    
    frequencyBand = ReceiverBandMod::UNSPECIFIED;
    receiverSideband = ReceiverSidebandMod::NOSB;

    // implementation of the ALMA freq bands !!!
    if(telName_p == "ALMA"){
      if(31.<=repFreqGHz &&  repFreqGHz<45.){
	frequencyBand = ReceiverBandMod::ALMA_RB_01;
	receiverSideband = ReceiverSidebandMod::SSB; 
      }
      if(67.<=repFreqGHz &&  repFreqGHz<90.){
	frequencyBand = ReceiverBandMod::ALMA_RB_02;
	receiverSideband = ReceiverSidebandMod::SSB; 
      }
      if(84.<=repFreqGHz &&  repFreqGHz<116.){
	frequencyBand = ReceiverBandMod::ALMA_RB_03;
	receiverSideband = ReceiverSidebandMod::TSB; 
      }
      else if(125.<=repFreqGHz &&  repFreqGHz<163.){
	frequencyBand = ReceiverBandMod::ALMA_RB_04;
	receiverSideband = ReceiverSidebandMod::TSB; 
      }	  
      else if(163.<=repFreqGHz &&  repFreqGHz<211.){
	frequencyBand = ReceiverBandMod::ALMA_RB_05;
	receiverSideband = ReceiverSidebandMod::TSB; 
      }	  
      else if(211.<=repFreqGHz &&  repFreqGHz<275.){
	frequencyBand = ReceiverBandMod::ALMA_RB_06;
	receiverSideband = ReceiverSidebandMod::TSB; 
      }	  
      else if(275.<=repFreqGHz &&  repFreqGHz<373.){
	frequencyBand = ReceiverBandMod::ALMA_RB_07;
	receiverSideband = ReceiverSidebandMod::TSB; 
      }	  
      else if(385.<=repFreqGHz &&  repFreqGHz<500.){
	frequencyBand = ReceiverBandMod::ALMA_RB_08;
	receiverSideband = ReceiverSidebandMod::TSB; 
      }	  
      else if(602.<=repFreqGHz &&  repFreqGHz<720.){
	frequencyBand = ReceiverBandMod::ALMA_RB_09;
	receiverSideband = ReceiverSidebandMod::DSB; 
      }	  
      else if(787.<=repFreqGHz &&  repFreqGHz<950.){
	frequencyBand = ReceiverBandMod::ALMA_RB_10;
	receiverSideband = ReceiverSidebandMod::DSB; 
      }
      return True;
    }
    return False;
  }  


} //#End casa namespace
