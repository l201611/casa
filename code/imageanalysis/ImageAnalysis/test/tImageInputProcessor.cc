//# tImageFitter.cc:  test the PagedImage clas
//# Copyright (C) 1994,1995,1998,1999,2000,2001,2002
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This program is free software; you can redistribute it and/or modify it
//# under the terms of the GNU General Public License as published by the Free
//# Software Foundation; either version 2 of the License, or(at your option)
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
//# $Id: $

#include <imageanalysis/ImageAnalysis/ImageInputProcessor.h>
#include <images/Images/FITSImage.h>
#include <casa/OS/EnvVar.h>

#include <casa/namespace.h>
#include <casa/iomanip.h>

using namespace casa;

void writeTestString(const String& test) {
    LogIO log;
    log << LogIO::NORMAL << "*** " << test << " ***" << LogIO::POST;
}

void _checkCorner(const Record& gotRecord, const Vector<Double>& expected) {
	for (uInt i=0; i<expected.size(); i++) {
		uInt fieldNumber = i+1;
		Double got = gotRecord.asRecord(RecordFieldId(
				"*" + String::toString(fieldNumber))
			).asDouble(RecordFieldId("value"));
		/*
		cout << setprecision(10);
		cout << "got " << got << endl;
		cout << "expected " << expected << endl;
		*/
	    AlwaysAssert(fabs((got-expected[i])/expected[i]) < 3e-9, AipsError);
	}
}

void testException(
	const String& test,
	vector<OutputDestinationChecker::OutputStruct> *outputStruct,
	const String& imagename, const Record *regionPtr,
	const String& regionName, const String& box,
	const String& chans, String& stokes,
	const CasacRegionManager::StokesControl& stokesControl,
	const Bool allowMultipleBoxes
) {
	ImageInputProcessor processor;
	ImageInterface<Float> *image = 0;
	Record region;
	String diagnostics;
	Bool fail = true;
	try {
		writeTestString(test);
		processor.process(
			image, region, diagnostics, outputStruct,
            stokes, imagename, regionPtr, regionName,
            box, chans, stokesControl,
            allowMultipleBoxes, 0
		);
		// should not get here
		fail = false;
		delete image;
		AlwaysAssert(false, AipsError);
	}
	catch (AipsError) {
		// should get here with fail = true
		AlwaysAssert(fail, AipsError);
	}
}

void testSuccess(
	const String& test,
	vector<OutputDestinationChecker::OutputStruct> *outputStruct,
	const String& imagename, const Record *regionPtr,
	const String& regionName, const String& box,
	const String& chans, String& stokes,
	const CasacRegionManager::StokesControl& stokesControl,
	const Bool allowMultipleBoxes,
	const Vector<Double>& expectedBlc,
	const Vector<Double>& expectedTrc
) {
	ImageInputProcessor processor;
	ImageInterface<Float> *image = 0;
	Record region;
	String diagnostics;
	writeTestString(test);
	processor.process(
		image, region, diagnostics, outputStruct, stokes,
        imagename, regionPtr, regionName, box, chans,
        stokesControl, allowMultipleBoxes, 0
	);
	_checkCorner(region.asRecord(RecordFieldId("blc")), expectedBlc);
	_checkCorner(region.asRecord(RecordFieldId("trc")), expectedTrc);
	AlwaysAssert(processor.nSelectedChannels() == 1, AipsError);
}

void runProcess(
	const String& test,
	vector<OutputDestinationChecker::OutputStruct> *outputStruct,
	const String& imagename, const Record *regionPtr,
	const String& regionName, const String& box,
	const String& chans, String& stokes,
	const CasacRegionManager::StokesControl& stokesControl,
	const Bool allowMultipleBoxes
) {
	ImageInputProcessor processor;
	ImageInterface<Float> *image = 0;
	Record region;
	String diagnostics;
	writeTestString(test);
	processor.process(
		image, region, diagnostics, outputStruct, stokes,
        imagename, regionPtr, regionName, box, chans,
        stokesControl, allowMultipleBoxes, 0
	);
}


int main() {
    try {
    	String *parts = new String[2];
    	split(EnvironmentVariable::get("CASAPATH"), parts, 2, String(" "));
    	String datadir = parts[0] + "/data/regression/unittest/imageanalysis/ImageAnalysis/";
    	delete [] parts;
    	String goodImage = datadir + "image_input_processor.im";
    	ImageInputProcessor processor;
    	ImageInterface<Float> *image = 0;
    	Record region;
    	String diagnostics;
    	Bool fail = true;
        String none = "";
    	testException("Bad image name throws exception",
    		0, "bogus_image", 0, "", "", "", none,
    		CasacRegionManager::USE_ALL_STOKES, true
    	);
    	testException("Bad region name throws exception", 0, goodImage, 0, "bogus.rgn",
    		"", "", none, CasacRegionManager::USE_ALL_STOKES, true
    	);
    	testException("Bad region name in another image throws exception",
    		0, goodImage, 0, "bogus.im:bogus.rgn",
    		"", "", none, CasacRegionManager::USE_ALL_STOKES, true
    	);
    	testException(
    		"Bad box spec #1 throws exception",
    		0, goodImage, 0, "", "-1,0,10,10",
    		"", none, CasacRegionManager::USE_ALL_STOKES, true
    	);
    	testException(
    		"Bad box spec #2 throws exception",
    		0, goodImage, 0, "", "0,-1,10,10",
    		"", none, CasacRegionManager::USE_ALL_STOKES, true
    	);
    	testException("Bad box spec #3 throws exception",
    		0, goodImage, 0, "", "0,0,100 ,10",
    		"", none, CasacRegionManager::USE_ALL_STOKES, true
    	);
    	testException(
    		"Bad box spec #4 throws exception",
    		0, goodImage, 0, "", "0, 0,10 ,100",
    		"", none, CasacRegionManager::USE_ALL_STOKES, true
    	);
    	testException(
    		"Bad box spec #5 throws exception",
    		0, goodImage, 0, "", "5, 0, 0,10 ,10",
    		"", none, CasacRegionManager::USE_ALL_STOKES, true
    	);
    	testException(
    		"Bad box spec #6 throws exception",
    		0, goodImage, 0, "", "a, 0,10 ,10",
    		"", none, CasacRegionManager::USE_ALL_STOKES, true
    	);
    	testException("Bad box spec #7 throws exception",
    		0, goodImage, 0, "", "1a, 0,10 ,10",
        	"", none, CasacRegionManager::USE_ALL_STOKES, true
        );
    	testException(
    		"Valid box spec with invalid channel spec #1 throws exception",
			0, goodImage, 0, "", "0, 0,10 ,10",
        	"1", none, CasacRegionManager::USE_ALL_STOKES, true
        );
    	testException(
    		"Valid box spec with invalid channel spec #2 throws exception",
        	0, goodImage, 0, "", "0, 0,10 ,10",
        	"a", none, CasacRegionManager::USE_ALL_STOKES, true
        );
    	testException(
    		"Valid box spec with invalid channel spec #3 throws exception",
        	0, goodImage, 0, "", "0, 0,10 ,10",
        	"a-b", none, CasacRegionManager::USE_ALL_STOKES, true
        );
    	testException(
    		"Valid box spec with invalid channel spec #4 throws exception",
        	0, goodImage, 0, "", "0, 0,10 ,10",
        	"0-b", none, CasacRegionManager::USE_ALL_STOKES, true
        );
    	testException(
    		"Valid box spec with invalid channel spec #5 throws exception",
        	0, goodImage, 0, "", "0, 0,10 ,10",
        	">0", none, CasacRegionManager::USE_ALL_STOKES, true
        );
    	testException(
    		"Valid box spec with invalid channel spec #6 throws exception",
        	0, goodImage, 0, "", "0, 0,10 ,10",
        	"-1", none, CasacRegionManager::USE_ALL_STOKES, true
        );
    	testException(
    		"Valid box spec with invalid channel spec #7 throws exception",
        	0, goodImage, 0, "", "0, 0,10 ,10",
        	"<5", none, CasacRegionManager::USE_ALL_STOKES, true
        );
        String stokes = "b";
    	testException(
    		"Valid box spec with invalid stokes spec #1 throws exception",
        	0, goodImage, 0, "", "0, 0,10 ,10",
        	"", stokes, CasacRegionManager::USE_ALL_STOKES, true
        );
        stokes = "yy";
    	testException(
    		"Valid box spec with invalid stokes spec #2 throws exception",
        	0, goodImage, 0, "", "0, 0,10 ,10",
        	"", stokes, CasacRegionManager::USE_ALL_STOKES, true
        );
    	try {
    		writeTestString("Calling nSelectedChannels() before process() throws an exception");
    		ImageInputProcessor processor;
		    processor.nSelectedChannels();
        	// should not get here
        	fail = false;
        	AlwaysAssert(false, AipsError);
        }
        catch (AipsError) {
        	// should get here with fail = true
        	AlwaysAssert(fail, AipsError);
        }
       	{
        	OutputDestinationChecker::OutputStruct output;
        	String out = "/cannot_create";
        	output.label = "file";
        	output.outputFile = &out;
        	output.required = true;
        	output.replaceable = true;
        	vector<OutputDestinationChecker::OutputStruct> outs(1, output);
        	testException(
        		"Non-createable output file throws exception",
        		&outs, goodImage, 0, "", "0, 0,  10,10",
        		"", none, CasacRegionManager::USE_ALL_STOKES, true
        	);
       	}
       	{
        	writeTestString("Non-overwriteable output file throws exception");
        	OutputDestinationChecker::OutputStruct output;
        	String out = "/usr";
        	output.label = "file";
        	output.outputFile = &out;
        	output.required = true;
        	output.replaceable = true;
        	vector<OutputDestinationChecker::OutputStruct> outs(1, output);
        	testException(
        		"Non-overwriteable output file throws exception",
        		&outs, goodImage, 0, "", "0, 0,  10,10",
        		"", none, CasacRegionManager::USE_ALL_STOKES, true
        	);
        }

       	{
        	writeTestString("Non-replaceable output file throws exception");
        	OutputDestinationChecker::OutputStruct output;
        	String out = datadir + "input_image_processor_dont_replace_me";
        	output.label = "file";
        	output.outputFile = &out;
        	output.required = true;
        	output.replaceable = false;
        	vector<OutputDestinationChecker::OutputStruct> outs(1, output);
        	testException(
        		"Non-replaceable output file throws exception",
        		&outs, goodImage, 0, "", "0, 0,  10,10",
        		"", none, CasacRegionManager::USE_ALL_STOKES, true
        	);
        }
       	testException(
       		"Multiple boxes with allowMultipleRegions = false throws exception",
        	0, goodImage, 0, "", "0, 0,10 ,10, 20,20,30,30",
        	"", none, CasacRegionManager::USE_ALL_STOKES, false
        );
        stokes = "iu";
       	testException(
       		"Multiple stokes ranges with allowMultipleRegions = false throws exception",
        	0, goodImage, 0, "", "0, 0,10 ,10",
        	"", stokes, CasacRegionManager::USE_ALL_STOKES, false
        );
       	stokes = "";
       	testException(
       		"Bad region name throws exception",
        	0, goodImage, 0, "mybox", "",
        	"", stokes, CasacRegionManager::USE_ALL_STOKES, false
        );
        {
            stokes = "iu";
        	writeTestString("Multiple stokes ranges with allowMultipleRegions = true succeeds");
        	processor.process(
        		image, region, diagnostics, 0, stokes, goodImage, 0, "", "0, 0,10 ,10",
        		none, CasacRegionManager::USE_ALL_STOKES, true, 0
        	);
        	delete image;
        	// FIXME just checks that no excpetion is thrown at this point, need to
        	// do region checking.
        }
    	Vector<Double> expectedBlc(4);
    	expectedBlc[0] = 1.24795230e+00;
    	expectedBlc[1] = 7.82549990e-01;
    	expectedBlc[2] = 4.73510000e+09;
    	expectedBlc[3] = 1;
    	Vector<Double> expectedTrc(4);
    	expectedTrc[0] = 1.24784989e+00;
    	expectedTrc[1] = 7.82622817e-01;
    	expectedTrc[2] = 4.73510000e+09;
    	expectedTrc[3] = 4;

    	testSuccess(
    		"Nothing specified gives entire image as region",
    		0, goodImage, 0, "", "", "", none,
    		CasacRegionManager::USE_ALL_STOKES, true,
        	expectedBlc, expectedTrc
    	);
    	expectedTrc[0] = 1.24793182e+00;
    	expectedTrc[1] = 7.82564556e-01;
    	expectedTrc[2] = 4.73510000e+09;
    	expectedTrc[3] = 4;
    	testSuccess(
    		"Valid box specification succeeds",
        	0, goodImage, 0, "", "0, 0,  10,10",
        	"", none, CasacRegionManager::USE_ALL_STOKES, true,
        	expectedBlc, expectedTrc
        );
    	testSuccess(
    		"Valid box specification with valid channel specification #1 succeeds",
        	0, goodImage, 0, "", "0, 0,  10,10",
        	"0~0", none, CasacRegionManager::USE_ALL_STOKES, true,
        	expectedBlc, expectedTrc
        );
    	testSuccess(
    		"Valid box specification with valid channel specification #2 succeeds",
        	0, goodImage, 0, "", "0, 0,  10,10",
        	"0", none, CasacRegionManager::USE_ALL_STOKES, true,
        	expectedBlc, expectedTrc
        );
    	testSuccess(
    		"Valid box specification with valid channel specification #3 succeeds",
        	0, goodImage, 0, "", "0, 0,  10,10",
        	"0,0,0", none, CasacRegionManager::USE_ALL_STOKES, true,
        	expectedBlc, expectedTrc
        );
    	testSuccess(
    		"Valid box specification with valid channel specification #4 succeeds",
        	0, goodImage, 0, "", "0, 0,  10,10",
        	"0;0;0", none, CasacRegionManager::USE_ALL_STOKES, true,
        	expectedBlc, expectedTrc
        );
    	testSuccess(
    		"Valid box specification with valid channel specification #5 succeeds",
        	0, goodImage, 0, "", "0, 0,  10,10",
        	"0,0;0", none, CasacRegionManager::USE_ALL_STOKES, true,
        	expectedBlc, expectedTrc
        );
        stokes = "QVIU";
        testSuccess(
        	"Valid box specification with valid stokes specification #1 succeeds",
        	 0, goodImage, 0, "", "0, 0,  10,10",
        	"", stokes, CasacRegionManager::USE_ALL_STOKES, true,
        	expectedBlc, expectedTrc
        );
        {
        	expectedTrc[3] = 3;
            stokes = "QIU";
            testSuccess(
        		"Valid box specification with valid stokes specification #2 succeeds",
        		0, goodImage, 0, "", "0, 0,  10,10", "",
        		stokes, CasacRegionManager::USE_ALL_STOKES, true,
            	expectedBlc, expectedTrc
        	);
        }
        {
            stokes = "Q";
        	expectedBlc[3] = expectedTrc[3] = 2;
        	testSuccess(
        		"Valid box specification with valid stokes specification #3 succeeds",
        		0, goodImage, 0, "", "0, 0,  10,10",
        		"", stokes, CasacRegionManager::USE_ALL_STOKES, true,
            	expectedBlc, expectedTrc
        	);
        }
        {
        	expectedBlc[3] = 1;
        	expectedTrc[3] = 3;
            stokes = "Q,I,U";
            testSuccess(
        		"Valid box specification with valid stokes specification #4 succeeds",
        		0, goodImage, 0, "", "0, 0,  10,10", "",
        		stokes, CasacRegionManager::USE_ALL_STOKES, true,
            	expectedBlc, expectedTrc
        	);
        }
        {
        	expectedBlc[3] = 1;
        	expectedTrc[3] = 3;
            stokes = "Q;I;U";
            testSuccess(
        		"Valid box specification with valid stokes specification #5 succeeds",
        		0, goodImage, 0, "", "0, 0,  10,10", "",
        		stokes, CasacRegionManager::USE_ALL_STOKES, true,
            	expectedBlc, expectedTrc
        	);
        }
        {
        	expectedBlc[3] = 1;
        	expectedTrc[3] = 3;
            stokes = "Q,I;U";
            testSuccess(
        		"Valid box specification with valid stokes specification #6 succeeds",
        		0, goodImage, 0, "", "0, 0,  10,10", "",
        		stokes, CasacRegionManager::USE_ALL_STOKES, true,
            	expectedBlc, expectedTrc
        	);
        }
        {
        	expectedBlc[3] = 1;
        	expectedTrc[3] = 4;
        	testSuccess(
        		"Valid box specification using all polarizations for blank stokes",
        		0, goodImage, 0, "", "0, 0,  10,10",
        		"", none, CasacRegionManager::USE_ALL_STOKES, true,
            	expectedBlc, expectedTrc
            );
        }
        {
        	expectedTrc[3] = 1;
        	expectedBlc[3] = 1;
            stokes = "  ";
        	testSuccess(
        		"Valid box specification using first polarizations for blank stokes",
        		0, goodImage, 0, "", "0, 0,  10,10",
        		"", stokes, CasacRegionManager::USE_FIRST_STOKES, true,
            	expectedBlc, expectedTrc
        	);
        }
        {
        	// these are one relative
        	expectedBlc.set(1.0);
        	expectedTrc[0] = 21;
        	expectedTrc[1] = 21;
        	expectedTrc[2] = 1;
        	expectedTrc[3] = 3;
            stokes = "";
            String regionFile = goodImage + "/mybox.rgn";
        	testSuccess(
        		"Valid region file",
        		0, goodImage, 0, regionFile, "",
        		"", stokes, CasacRegionManager::USE_FIRST_STOKES, true,
            	expectedBlc, expectedTrc
        	);
        }
        {
        	// these are one relative
        	expectedBlc.set(1.0);
        	expectedTrc[0] = 21;
        	expectedTrc[1] = 21;
        	expectedTrc[2] = 1;
        	expectedTrc[3] = 3;
            stokes = "";
            String region = goodImage + ":" + "mybox2";
        	testSuccess(
        		"Valid region description from image table",
        		0, goodImage, 0, region, "",
        		"", stokes, CasacRegionManager::USE_FIRST_STOKES, true,
            	expectedBlc, expectedTrc
        	);
        }
       	{
        	writeTestString("Non-required, non-overwriteable output file is set to blank");
        	OutputDestinationChecker::OutputStruct output;
        	String out = "/usr";
        	output.label = "file";
        	output.outputFile = &out;
        	output.required = false;
        	output.replaceable = true;
        	vector<OutputDestinationChecker::OutputStruct> outs(1, output);
        	runProcess(
        		"Non-required, non-overwriteable output file is set to blank",
        		&outs, goodImage, 0, "", "0, 0,  10,10",
        		"", none, CasacRegionManager::USE_ALL_STOKES, true
        	);
        	AlwaysAssert(out.empty(), AipsError);
       	}
       	{
        	OutputDestinationChecker::OutputStruct output;
        	String out = "/cannot_write_me";
        	output.label = "file";
        	output.outputFile = &out;
        	output.required = false;
        	output.replaceable = true;
        	vector<OutputDestinationChecker::OutputStruct> outs(1, output);
        	runProcess(
        		"Non-required, non-createable output file is set to blank",
        		&outs, goodImage, 0, "", "0, 0,  10,10",
        		"", none, CasacRegionManager::USE_ALL_STOKES, true
        	);
        	AlwaysAssert(out.empty(), AipsError);
       	}
       	{
        	OutputDestinationChecker::OutputStruct output;
        	String out = datadir + "input_image_processor_dont_replace_me";
        	output.required = false;
        	output.label = "file";
        	output.outputFile = &out;
        	output.required = false;
        	output.replaceable = false;
        	vector<OutputDestinationChecker::OutputStruct> outs(1, output);
        	runProcess(
        		"Non-required, non-replaceable output file is set to blank",
        		&outs, goodImage, 0, "", "0, 0,  10,10",
        		"", none, CasacRegionManager::USE_ALL_STOKES, true
        	);
        	AlwaysAssert(out.empty(), AipsError);
       	}
       	{
        	OutputDestinationChecker::OutputStruct output;
        	output.label = "file";
        	String name = "youcanwritemedddslsl";
        	String save = name;
        	output.outputFile = &name;
        	output.required = true;
        	output.replaceable = false;
        	vector<OutputDestinationChecker::OutputStruct> outs(1, output);
        	runProcess(
        		"Writeable file is not reset",
        		&outs, goodImage, 0, "", "0, 0,  10,10",
        		"", none, CasacRegionManager::USE_ALL_STOKES, true
        	);
        	AlwaysAssert(name == save, AipsError);
       	}
    }
    catch (AipsError x) {
        cerr << "Exception caught: " << x.getMesg() << endl;
        return 1;
    } 
    cout << "ok" << endl;
    return 0;
}

