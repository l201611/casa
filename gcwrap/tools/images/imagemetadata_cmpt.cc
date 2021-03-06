#include <imagemetadata_cmpt.h>

#include <casa/Containers/ValueHolder.h>
#include <casa/Utilities/PtrHolder.h>
#include <imageanalysis/ImageAnalysis/ImageMetaDataRW.h>
#include <imageanalysis/ImageAnalysis/ImageFactory.h>
#include <images/Images/ImageOpener.h>

#include <stdcasa/version.h>

#include <casa/namespace.h>

using namespace std;

using namespace casacore;
using namespace casa;

namespace casac {

const String imagemetadata::_class = "imagemetadata";

imagemetadata::imagemetadata() :
_log(new LogIO()), _header(0) {}

imagemetadata::~imagemetadata() {}

bool imagemetadata::close() {
	_header.reset(0);
	return true;
}

bool imagemetadata::add(const string& key, const variant& value) {
	try {
		_exceptIfDetached();
		std::auto_ptr<const ValueHolder> vh(casa::toValueHolder(value));
		return _header->add(key, *vh);
	}
	catch (const AipsError& x) {
		*_log << LogIO::SEVERE << "Exception Reported: " << x.getMesg()
			<< LogIO::POST;
		RETHROW(x);
	}
}

bool imagemetadata::done() {
	return close();
}

variant* imagemetadata::get(const string& key) {
	try {
		_exceptIfDetached();
		return casa::fromValueHolder(
			_header->getFITSValue(key)
		);
	}
	catch (const AipsError& x) {
		*_log << LogIO::SEVERE << "Exception Reported: "
			<< x.getMesg() << LogIO::POST;
		RETHROW(x);
	}
}

record* imagemetadata::list(bool verbose) {
	try {
		_exceptIfDetached();
		return casa::fromRecord(_header->toRecord(verbose));
	}
	catch (const AipsError& x) {
		*_log << LogIO::SEVERE << "Exception Reported: "
			<< x.getMesg() << LogIO::POST;
		RETHROW(x);
	}
}

bool imagemetadata::open(const std::string& infile) {
	try {
		if (_log.get() == 0) {
			_log.reset(new LogIO());
		}
		auto mypair = ImageFactory::fromFile(infile);
		if (mypair.first) {
			_header.reset(new ImageMetaDataRW(mypair.first));
		}
		else {
			_header.reset(new ImageMetaDataRW(mypair.second));
		}
		return true;
	}
	catch (const AipsError& x) {
		*_log << LogIO::SEVERE << "Exception Reported: "
			<< x.getMesg() << LogIO::POST;
		RETHROW(x);
	}
}

bool imagemetadata::remove(
	const string& key, const variant& value
) {
	try {
		_exceptIfDetached();
		if (String(key) == ImageMetaDataBase::MASKS) {
			return _header->removeMask(value.toString());
		}
		else {
			return _header->remove(key);
		}
	}
	catch (const AipsError& x) {
		*_log << LogIO::SEVERE << "Exception Reported: "
			<< x.getMesg() << LogIO::POST;
		RETHROW(x);
	}
}

bool imagemetadata::set(const string& key, const variant& value) {
	try {
		_exceptIfDetached();
		std::auto_ptr<const ValueHolder> vh(toValueHolder(value));
		return _header->set(key, *vh);
	}
	catch (const AipsError& x) {
		*_log << LogIO::SEVERE << "Exception Reported: "
			<< x.getMesg() << LogIO::POST;
		RETHROW(x);
	}
}

void imagemetadata::_exceptIfDetached() const {
	if (_header.get() == 0) {
		throw AipsError("Tool is not attached to a metadata object. Call open() first.");
	}
}


} // casac namespace
