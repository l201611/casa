/***
 * Framework independent implementation file for flag...
 *
 * Implement the flag component here.
 * 
 * // TODO: WRITE YOUR DESCRIPTION HERE! 
 *
 * @author
 * @version 
 ***/

#include <iostream>
#include <flagger_cmpt.h>

#include <casa/Logging/LogIO.h>
#include <casa/Logging/LogOrigin.h>
#include <casa/Exceptions/Error.h>
#include <ms/MeasurementSets/MeasurementSet.h>
#include <flagging/Flagging/Flagger.h>
#include <flagging/Flagging/RFCommon.h>
#include <casa/Containers/RecordInterface.h>
#include <casa/Arrays/Array.h>
#include <casa/Arrays/Vector.h>
#include <casa/Arrays/IPosition.h>
#include <casa/sstream.h>
#include <unistd.h>
#include <xmlcasa/StdCasa/CasacSupport.h>


using namespace std;
using namespace casa;

namespace casac {

flagger::flagger()
{
    try
    {
        logger_p = new LogIO();
	ms_p = NULL;
	flagger_p = new Flagger();
	
    } catch (AipsError x) {
            *logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
            RETHROW(x);
    }

}

flagger::~flagger()
{
    try
    {
        done();
    } catch (AipsError x) {
            *logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
            RETHROW(x);
    }
}

bool
flagger::open(const std::string& msname)
{
    try
    {
	if(ms_p) delete ms_p;
	ms_p = new MeasurementSet(String(msname),Table::Update);

	if ( !flagger_p ) {
	    flagger_p = new Flagger();
	}
        if ( flagger_p ) {
          return flagger_p->attach(*ms_p);
        }
	return new ::casac::record();
    } catch (AipsError x) {
            *logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
            RETHROW(x);
    }
}

bool
flagger::setdata(
    const std::string& field, 
    const std::string& spw, 
    const std::string& array, 
    const std::string& feed, 
    const std::string& scan, 
    const std::string& baseline, 
    const std::string& uvrange, 
    const std::string& time, 
    const std::string& correlation)
{
    try {
	if (flagger_p) {
	    return flagger_p->setdata(
		String(field),String(spw),String(array),
		String(feed),String(scan),String(baseline),
		String(uvrange),String(time),String(correlation));
        }
	return false;
    } catch (AipsError x) {
	*logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
	RETHROW(x);
    }
}

bool
flagger::setmanualflags(
    const std::string& field,
    const std::string& spw, 
    const std::string& array, 
    const std::string& feed, 
    const std::string& scan, 
    const std::string& baseline, 
    const std::string& uvrange, 
    const std::string& time, 
    const std::string& correlation, 
    const bool autocorrelation, 
    const bool unflag, 
    const std::string& clipexpr, 
    const std::vector<double>& cliprange,
    const std::string& clipcolumn, 
    const bool outside, 
    const bool channelavg,
    const double quackinterval, 
    const std::string& quackmode, 
    const bool quackincrement)
{
    try	{
	Vector<Double> l_cliprange(cliprange.size());
	for(uInt i=0;
	    i < cliprange.size();
	    i++)  {
	    l_cliprange[i]=cliprange[i];
	}
	
	if(flagger_p) {
	    Bool ret;
	    ret = flagger_p->selectdata(
		False, 
		String(field), String(spw), String(array),
		String(feed), String(scan), String(baseline),
		String(uvrange), String(time), String(correlation));
	    
	    if(ret) {
		ret = flagger_p->setmanualflags(
		    Bool(autocorrelation), Bool(unflag),
		    String(clipexpr), l_cliprange, String(clipcolumn), 
                    Bool(outside), Bool(channelavg),
		    quackinterval, String(quackmode), Bool(quackincrement),
                    String("FLAG"));
	    }
	    
	    return ret;
	}
	
	return false;
	
    } catch (AipsError x) {
	*logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
	RETHROW(x);
    }
}

bool
flagger::printflagselection()
{
    try
    {
	if(flagger_p)
        {
                return flagger_p->printflagselections();
        }
        return false;
    } catch (AipsError x) {
            *logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
            RETHROW(x);
    }
}

bool
flagger::clearflagselection(const int index)
{
    try
    {

	if(flagger_p)
        {
                return flagger_p->clearflagselections(index);
        }
        return false;
    } catch (AipsError x) {
            *logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
            RETHROW(x);
    }
}

::casac::record*
flagger::getautoflagparams(const std::string& algorithm)
{
    try
    {
        casac::record* rstat(0);
	if(flagger_p)
        {
	        Record defparams = flagger_p->getautoflagparams(String(algorithm));
                rstat = fromRecord(defparams);
        }
        return rstat;
    } catch (AipsError x) {
            *logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
            RETHROW(x);
    }
}

bool
flagger::setautoflag(const std::string& algorithm, const ::casac::record& parameters)
{
    try
    {
        Record params = *toRecord(parameters);
	if(flagger_p)
        {
	        return flagger_p->setautoflagparams(String(algorithm), params);
        }
        return false;
    } catch (AipsError x) {
            *logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
            RETHROW(x);
    }
}

bool
flagger::setflagsummary(const std::string& field, const std::string& spw, const std::string& array, const std::string& feed, const std::string& scan, const std::string& baseline, const std::string& uvrange, const std::string& time, const std::string& correlation)
{
    try {
	if(flagger_p) {
	    Bool ret;
	    ret = flagger_p->selectdata(
		False, String(field), String(spw), String(array),
		String(feed), String(scan), String(baseline),
		String(uvrange), String(time), String(correlation));

	    if(ret) {
		ret = flagger_p->setmanualflags(False,False,
						String(""),Vector<Double>(),String(""),
						False, False,
                                                0.0, String("beg"), False,
                                                String("SUMMARY"));
	    }
	    
	    return true;
        }

        return false;

    } catch (AipsError x) {
	*logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
	RETHROW(x);
    }
}

bool
flagger::setshadowflags(const std::string& field,
			const std::string& spw,
			const std::string& array,
			const std::string& feed,
			const std::string& scan,
			const std::string& baseline, 
			const std::string& uvrange,
			const std::string& time, 
			const std::string& correlation,
                        double diameter)
{
    try {
	if(flagger_p) {
	    Bool ret;
	    ret = flagger_p->selectdata(
		False, 
		String(field), String(spw), String(array),
		String(feed), String(scan), String(baseline),
		String(uvrange), String(time), String(correlation));

	    if(ret) {
		ret = flagger_p->setmanualflags(
		    False, False,
		    String(""), Vector<Double>(), String(""),
		    False, False,
                    0.0, String("beg"), False,
                    String("SHADOW"), diameter);
	    }
	    
	    return true;
        }

        return false;

    } catch (AipsError x) {
	*logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
	RETHROW(x);
    }
}

bool
flagger::setelevationflags(const std::string& field,
                           const std::string& spw,
                           const std::string& array,
                           const std::string& feed,
                           const std::string& scan,
                           const std::string& baseline, 
                           const std::string& uvrange,
                           const std::string& time, 
                           const std::string& correlation,
                           double lowerlimit,
                           double upperlimit)
{
    try {
	if(flagger_p) {
	    Bool ret;
	    ret = flagger_p->selectdata(
		False, 
		String(field), String(spw), String(array),
		String(feed), String(scan), String(baseline),
		String(uvrange), String(time), String(correlation));

	    if(ret) {
		ret = flagger_p->setmanualflags(
		    False, False,
		    String(""), Vector<Double>(), String(""),
		    False, False,
                    0.0, String("beg"), False,
                    String("ELEVATION"), 0, lowerlimit, upperlimit);
	    }
	    
	    return true;
        }

        return false;

    } catch (AipsError x) {
	*logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
	RETHROW(x);
    }
}


bool
flagger::setqueryflag(const std::string& field, const std::string& spw, const std::string& array, const std::string& feed, const std::string& scan, const std::string& baseline, const std::string& uvrange, const std::string& time, const std::string& correlation,const std::string& what, const double fractionthreshold, const int nflagsthreshold, const bool morethan)
{
    try
    {

    // TODO : IMPLEMENT ME HERE !
        cout << "Not Implemented Yet !!! " << endl;
	if(flagger_p)
        {
	        return true;
        }
        return false;
    } catch (AipsError x) {
            *logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
            RETHROW(x);
    }
}

bool
flagger::setextendflag(const std::string& field, const std::string& spw, const std::string& array, const std::string& feed, const std::string& scan, const std::string& baseline, const std::string& uvrange, const std::string& time, const std::string& correlation,const std::string& along, const int width)
{
    try
    {

    // TODO : IMPLEMENT ME HERE !
        cout << "Not Implemented Yet !!! " << endl;
	if(flagger_p)
        {
	        return true;
        }
        return false;
    } catch (AipsError x) {
            *logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
            RETHROW(x);
    }
}

::casac::record*
flagger::run(const bool trial, const bool reset)
{
    try {
        if(flagger_p){
            return fromRecord(flagger_p->run(Bool(trial),Bool(reset)));
        }

        return fromRecord(Record());
    } catch (AipsError x) {
        *logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
        RETHROW(x);
    }
}

bool
flagger::writeflagstodisk()
{
    try
    {

    // TODO : IMPLEMENT ME HERE !
        cout << "Not Implemented Yet !!! " << endl;
	if(flagger_p)
        {
	        return true;
        }
        return false;
    } catch (AipsError x) {
            *logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
            RETHROW(x);
    }
}

bool
flagger::help(const std::string& names)
{
    try
    {

    // TODO : IMPLEMENT ME HERE !
        cout << "Not Implemented Yet !!! " << endl;
	if(flagger_p)
        {
	        return true;
        }
        return false;
    } catch (AipsError x) {
            *logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
            RETHROW(x);
    }
}

bool
flagger::done()
{
    try
    {
	if(flagger_p) 
        {
                delete flagger_p;
                flagger_p = NULL;
        }
	if(ms_p) 
        {
                delete ms_p;
                ms_p = NULL;
        }
        return true;
    } catch (AipsError x) {
            *logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
            RETHROW(x);
    }
}

bool
flagger::saveflagversion(const std::string& versionname, const std::string& comment, const std::string& merge)
{
    try
    {
        if( flagger_p )
        {
                return flagger_p->saveFlagVersion(String(versionname), String(comment),String(merge));
        }
        return False;
    } catch (AipsError x) {
            *logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
            RETHROW(x);
    }
}

bool
flagger::restoreflagversion(const std::vector<std::string>& versionname, const std::string& merge)
{
    try
    {
        if( flagger_p )
        {	
                Vector<String> verlist;
		verlist.resize(0);
		verlist = toVectorString(versionname);
                return flagger_p->restoreFlagVersion(verlist, String(merge));
        }
        return False;
    } catch (AipsError x) {
            *logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
            RETHROW(x);
    }
}

bool
flagger::deleteflagversion(const std::vector<std::string>& versionname)
{
    try
    {
        if( flagger_p )
        {
                Vector<String> verlist;
		verlist.resize(0);
		verlist = toVectorString(versionname);
                return flagger_p->deleteFlagVersion(verlist);
        }
        return False;
    } catch (AipsError x) {
            *logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
            RETHROW(x);
    }
}

std::vector<std::string>
flagger::getflagversionlist(const bool printflags)
{
    try
    {
        std::vector<std::string> result;

        if( flagger_p )
        {
                Vector<String> versionlist(0);
		flagger_p->getFlagVersionList(versionlist);
		for(uInt i = 0; i < versionlist.nelements(); i++) {
		    if (printflags) *logger_p << versionlist[i] << LogIO::POST;
		    result.push_back(versionlist[i]);
		}
		
        }
        return result;
    } catch (AipsError x) {
            *logger_p << LogIO::SEVERE << "Exception Reported: " << x.getMesg() << LogIO::POST;
            RETHROW(x);
    }
}


} // casac namespace

