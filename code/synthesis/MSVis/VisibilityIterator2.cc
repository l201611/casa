/*
 * VisibilityIterator2.cc
 *
 *  Created on: Jun 4, 2012
 *      Author: jjacobs
 */

#include <casa/Arrays/Cube.h>
#include <casa/Arrays/Matrix.h>
#include <casa/Arrays/Slicer.h>
#include <casa/BasicSL/String.h>
#include <casa/Containers/Stack.h>
#include <casa/Quanta/MVDoppler.h>
#include <casa/aips.h>
#include <casa/System/AipsrcValue.h>
#include <measures/Measures/MCDoppler.h>
#include <measures/Measures/MDoppler.h>
#include <measures/Measures/MeasConvert.h>
#include <measures/Measures/Stokes.h>
#include <ms/MeasurementSets/MSDerivedValues.h>
#include <ms/MeasurementSets/MSIter.h>
#include <ms/MeasurementSets/MeasurementSet.h>
#include <scimath/Mathematics/RigidVector.h>
#include <scimath/Mathematics/SquareMatrix.h>
#include <synthesis/MSVis/StokesVector.h>
#include <synthesis/MSVis/VisBufferComponents2.h>
#include <synthesis/MSVis/VisImagingWeight.h>
#include <synthesis/MSVis/VisibilityIterator2.h>
#include <synthesis/MSVis/VisibilityIteratorImpl2.h>
#include <synthesis/MSVis/VisibilityIteratorImplAsync2.h>
#include <synthesis/MSVis/UtilJ.h>
#include <tables/Tables/ArrayColumn.h>
#include <tables/Tables/ScalarColumn.h>

#include <map>
#include <set>
#include <utility>
#include <vector>

using namespace std;

#define CheckImplementationPointerR() Assert (readImpl_p != NULL);
#define CheckImplementationPointerW() Assert (writeImpl_p != NULL);

namespace casa {

namespace vi {

void
FrequencySelection::filterByWindow (Int windowId) const
{
    filterWindowId_p = windowId;
}

Int
FrequencySelection::filterWindow() const
{
    return filterWindowId_p;
}

String
FrequencySelection::frameName (Int referenceFrame)
{
    String result;

    if (referenceFrame >= 0 && referenceFrame < MFrequency::N_Types){

        result = MFrequency::showType (referenceFrame);
    }
    else if (referenceFrame == ByChannel){
    }
    else{

        ThrowIf (True, utilj::format ("Unknown frame of reference: id=%d", referenceFrame));
    }

    return result;
}

Int
FrequencySelection::getFrameOfReference () const
{
    return referenceFrame_p;
}

void
FrequencySelectionUsingChannels::add (Int spectralWindow, Int firstChannel,
                                      Int nChannels, Int increment)
{
    Assert (spectralWindow >= 0);
    Assert (firstChannel >= 0);
    Assert (nChannels > 0);
    Assert (increment != 0 || nChannels == 1);

    elements_p.push_back (Element (spectralWindow, firstChannel, nChannels, increment));
}

FrequencySelectionUsingChannels::const_iterator
FrequencySelectionUsingChannels::begin () const
{
    filtered_p.clear();

    for (Elements::const_iterator i = elements_p.begin();
         i != elements_p.end();
         i ++){

        if (filterWindow () < 0 || i->spectralWindow_p == filterWindow()){
            filtered_p.push_back (* i);
        }
    }

    return filtered_p.begin();
}


FrequencySelection *
FrequencySelectionUsingChannels::clone () const
{
    return new FrequencySelectionUsingChannels (* this);
}

FrequencySelectionUsingChannels::const_iterator
FrequencySelectionUsingChannels::end () const
{
    return filtered_p.end();
}

set<int>
FrequencySelectionUsingChannels::getSelectedWindows () const
{
    set <int> result;
    for (Elements::const_iterator i = elements_p.begin();
         i != elements_p.end();
         i ++){

        result.insert (i->spectralWindow_p);

    }

    return result;
}


String
FrequencySelectionUsingChannels::toString () const
{
    String s = utilj::format ("{frame='%s' {", frameName (getFrameOfReference()).c_str());

    for (Elements::const_iterator e = elements_p.begin();
         e != elements_p.end();
         e ++){

        s += utilj::format ("(spw=%d, 1st=%d, n=%d, inc=%d)",
                            e->spectralWindow_p,
                            e->firstChannel_p,
                            e->nChannels_p,
                            e->increment_p);
    }
    s += "}}";

    return s;
}

FrequencySelectionUsingFrame::FrequencySelectionUsingFrame (MFrequency::Types frameOfReference)
: FrequencySelection (frameOfReference)
{}

void
FrequencySelectionUsingFrame::add (Int spectralWindow, Double bottomFrequency,
                                   Double topFrequency)
{
    elements_p.push_back (Element (spectralWindow, bottomFrequency, topFrequency));
}

//void
//FrequencySelectionUsingFrame::add (Int spectralWindow, Double bottomFrequency,
//                                   Double topFrequency, Double increment)
//{
//    elements_p.push_back (Elements (spectralWindow, bottomFrequency, topFrequency, increment));
//}


FrequencySelectionUsingFrame::const_iterator
FrequencySelectionUsingFrame::begin () const
{
    filtered_p.clear();

    for (Elements::const_iterator i = elements_p.begin();
         i != elements_p.end();
         i ++){

        if (filterWindow () < 0 || i->spectralWindow_p == filterWindow()){
            filtered_p.push_back (* i);
        }
    }

    return filtered_p.begin();
}

FrequencySelection *
FrequencySelectionUsingFrame::clone () const
{
    return new FrequencySelectionUsingFrame (* this);
}

FrequencySelectionUsingFrame::const_iterator
FrequencySelectionUsingFrame::end () const
{
    return filtered_p.end();
}

set<int>
FrequencySelectionUsingFrame::getSelectedWindows () const
{
    set<int> result;
    for (Elements::const_iterator i = elements_p.begin();
         i != elements_p.end();
         i ++){

        result.insert (i->spectralWindow_p);

    }

    return result;
}

Double
FrequencySelectionUsingFrame::Element::getBeginFrequency () const
{
    return beginFrequency_p;
}

Double
FrequencySelectionUsingFrame::Element::getEndFrequency () const
{
    return endFrequency_p;
}

String
FrequencySelectionUsingFrame::toString () const
{
    String s = utilj::format ("{frame='%s' {", frameName (getFrameOfReference()).c_str());

    for (Elements::const_iterator e = elements_p.begin();
         e != elements_p.end();
         e ++){

        s += utilj::format ("(spw=%d, 1st=%g, n=%g, inc=%g)",
                            e->spectralWindow_p,
                            e->beginFrequency_p,
                            e->endFrequency_p,
                            e->increment_p);
    }

    s += "}}";

    return s;
}

FrequencySelections::FrequencySelections ()
{}

FrequencySelections::FrequencySelections (const FrequencySelections & other)
{
    for (Selections::const_iterator s = other.selections_p.begin();
         s != other.selections_p.end(); s++){
        selections_p.push_back ((* s)->clone());
    }

    filterWindow_p = other.filterWindow_p;
    selectedWindows_p = other.selectedWindows_p;
}

FrequencySelections::~FrequencySelections ()
{
    for (Selections::const_iterator s = selections_p.begin();
         s != selections_p.end(); s++){
        delete (* s);
    }
}

void
FrequencySelections::add (const FrequencySelection & selection)
{
    if (! selections_p.empty()){
        ThrowIf (getFrameOfReference() != selection.getFrameOfReference(),
                 utilj::format ("Frequency selection #%d has incompatible frame of reference %d:%s "
                                "(!= %d:%s)",
                                selections_p.size() + 1,
                                selection.getFrameOfReference(),
                                FrequencySelection::frameName (selection.getFrameOfReference()).c_str(),
                                getFrameOfReference(),
                                FrequencySelection::frameName (getFrameOfReference()).c_str()));
    }

    selections_p.push_back (selection.clone());
    Int msIndex = selections_p.size() - 1;
    set<int> windows = selection.getSelectedWindows();

    for (set<int>::const_iterator w = windows.begin(); w != windows.end(); w++){
        selectedWindows_p.insert (make_pair (msIndex, * w));
    }

}

FrequencySelections *
FrequencySelections::clone () const
{
    return new FrequencySelections (* this);
}

const FrequencySelection &
FrequencySelections::get (Int msIndex) const
{
    if (selections_p.empty()){
        return defaultSelection_p;
    }

    ThrowIf (msIndex < 0 || msIndex >= (int) selections_p.size(),
             String::format ("MS index, %d, out of bounds [0,%d]", msIndex, selections_p.size() - 1));

    return * selections_p [msIndex];
}


Int
FrequencySelections::getFrameOfReference () const
{
    if (selections_p.empty()){
        return FrequencySelection::ByChannel;
    }
    else {
        return selections_p.front()->getFrameOfReference();
    }
}

Bool
FrequencySelections::isSpectralWindowSelected (Int msIndex, Int spectralWindowId) const
{
    // Empty selections means everything is selected

    if (selections_p.empty()){
        return True;
    }

    SelectedWindows::const_iterator swi =
        selectedWindows_p.find (make_pair (msIndex, spectralWindowId));

    return swi != selectedWindows_p.end();
}


Int
FrequencySelections::size () const
{
    return (Int) selections_p.size();
}


ROVisibilityIterator2::ROVisibilityIterator2(const MeasurementSet& ms,
                                             const Block<Int>& sortColumns,
                                             const VisBufferComponents2 * prefetchColumns,
                                             const Bool addDefaultSortCols,
                                             Double timeInterval)
{
    construct (prefetchColumns, Block<MeasurementSet> (1, ms), sortColumns,
               addDefaultSortCols, timeInterval, False);
}

ROVisibilityIterator2::ROVisibilityIterator2 (const Block<MeasurementSet>& mss,
                                              const Block<Int>& sortColumns,
                                              const VisBufferComponents2 * prefetchColumns,
                                              const Bool addDefaultSortCols,
                                              Double timeInterval)
{
    construct (prefetchColumns, mss, sortColumns, addDefaultSortCols,
               timeInterval, False);
}

ROVisibilityIterator2::ROVisibilityIterator2 (const VisBufferComponents2 * prefetchColumns,
                                              const Block<MeasurementSet>& mss,
                                              const Block<Int>& sortColumns,
                                              const Bool addDefaultSortCols,
                                              Double timeInterval,
                                              Bool writable)
{
    construct (prefetchColumns, mss, sortColumns, addDefaultSortCols,
               timeInterval, writable);
}



void
ROVisibilityIterator2::construct (const VisBufferComponents2 * prefetchColumns,
                                  const Block<MeasurementSet>& mss,
                                  const Block<Int>& sortColumns,
                                  const Bool addDefaultSortCols,
                                  Double timeInterval,
                                  Bool writable)
{

    // Factory didn't create the read implementation so decide whether to create a
    // synchronous or asynchronous read implementation.

    Bool createAsAsynchronous = prefetchColumns != NULL && isAsynchronousIoEnabled ();

    if (createAsAsynchronous){
        //            readImpl_p = new ViReadImplAsync2 (mss, * prefetchColumns, sortColumns,
        //                                               addDefaultSortCols, timeInterval, writable);
    }
    else{
        readImpl_p = new VisibilityIteratorReadImpl2 (this, mss, sortColumns,
                                                      addDefaultSortCols, timeInterval, True, writable);
    }

    nMS_p = mss.nelements();

}

ROVisibilityIterator2::~ROVisibilityIterator2 ()
{
    delete readImpl_p;
}

ROVisibilityIterator2 &
ROVisibilityIterator2::operator++ (int)
{
    advance ();

    return * this;
}

ROVisibilityIterator2 &
ROVisibilityIterator2::operator++ ()
{
    advance ();

    return * this;
}

void
ROVisibilityIterator2::advance ()
{
    CheckImplementationPointerR ();
    readImpl_p->advance ();
}

Bool
ROVisibilityIterator2::allBeamOffsetsZero () const
{
    CheckImplementationPointerR ();
    return readImpl_p->allBeamOffsetsZero ();
}

//void
//ROVisibilityIterator2::allSelectedSpectralWindows (Vector<Int>& spws, Vector<Int>& nvischan)
//{
//    CheckImplementationPointerR ();
//    readImpl_p->allSelectedSpectralWindows (spws, nvischan);
//}

void
ROVisibilityIterator2::antenna1 (Vector<Int>& ant1) const
{
    CheckImplementationPointerR ();
    readImpl_p->antenna1 (ant1);
}

void
ROVisibilityIterator2::antenna2 (Vector<Int>& ant2) const
{
    CheckImplementationPointerR ();
    readImpl_p->antenna2 (ant2);
}

const Vector<String>&
ROVisibilityIterator2::antennaMounts () const
{
    CheckImplementationPointerR ();
    return readImpl_p->antennaMounts ();
}

const Block<Int>&
ROVisibilityIterator2::getSortColumns() const
{
  CheckImplementationPointerR();
  return readImpl_p->getSortColumns();
}

const MeasurementSet &
ROVisibilityIterator2::getMeasurementSet () const
{
    CheckImplementationPointerR ();
    return readImpl_p->getMeasurementSet ();
}


const MeasurementSet&
ROVisibilityIterator2::ms () const
{
    CheckImplementationPointerR ();
    return readImpl_p->ms ();
}

Int
ROVisibilityIterator2::arrayId () const
{
    CheckImplementationPointerR ();
    return readImpl_p->arrayId ();
}


void
ROVisibilityIterator2::attachColumns (const Table &t)
{
    CheckImplementationPointerR ();
    readImpl_p->attachColumns (t);
}

const Table
ROVisibilityIterator2::attachTable () const
{
    CheckImplementationPointerR ();
    return readImpl_p->attachTable ();
}

Vector<MDirection>
ROVisibilityIterator2::azel (Double time) const
{
    CheckImplementationPointerR ();
    return readImpl_p->azel (time);
}

MDirection
ROVisibilityIterator2::azel0 (Double time) const
{
    CheckImplementationPointerR ();
    return readImpl_p->azel0 (time);
}

void
ROVisibilityIterator2::azelCalculate (Double time, MSDerivedValues & msd, Vector<MDirection> & azel,
                                     Int nAnt, const MEpoch & mEpoch0)
{
    VisibilityIteratorReadImpl2::azelCalculate (time, msd, azel, nAnt, mEpoch0);
}

void
ROVisibilityIterator2::azel0Calculate (Double time, MSDerivedValues & msd,
		                      MDirection & azel0, const MEpoch & mEpoch0)
{
    VisibilityIteratorReadImpl2::azel0Calculate (time, msd, azel0, mEpoch0);
}

void
ROVisibilityIterator2::jonesC (Vector<SquareMatrix<Complex,2> >& cjones) const
{
    CheckImplementationPointerR ();
    readImpl_p->jonesC (cjones);
}


void
ROVisibilityIterator2::corrType (Vector<Int>& corrTypes) const
{
    CheckImplementationPointerR ();
    readImpl_p->corrType (corrTypes);
}

Int
ROVisibilityIterator2::dataDescriptionId () const
{
    CheckImplementationPointerR ();
    return readImpl_p->dataDescriptionId ();
}

Bool
ROVisibilityIterator2::existsColumn (VisBufferComponent2 id) const
{
    CheckImplementationPointerR ();

    return readImpl_p->existsColumn (id);
}

Bool
ROVisibilityIterator2::existsFlagCategory() const
{
  CheckImplementationPointerR ();
  return readImpl_p->existsFlagCategory();
}

Bool
ROVisibilityIterator2::existsWeightSpectrum () const
{
    CheckImplementationPointerR ();
    return readImpl_p->existsWeightSpectrum ();
}

void
ROVisibilityIterator2::exposure (Vector<Double>& expo) const
{
    CheckImplementationPointerR ();
    readImpl_p->exposure (expo);
}

Vector<Float>
ROVisibilityIterator2::feed_paCalculate(Double time, MSDerivedValues & msd,
  									    Int nAntennas, const MEpoch & mEpoch0,
									    const Vector<Float> & receptor0Angle)
{
    return VisibilityIteratorReadImpl2::feed_paCalculate (time, msd, nAntennas, mEpoch0, receptor0Angle);
}

void
ROVisibilityIterator2::feed1 (Vector<Int>& fd1) const
{
    CheckImplementationPointerR ();
    readImpl_p->feed1 (fd1);
}

void
ROVisibilityIterator2::feed2 (Vector<Int>& fd2) const
{
    CheckImplementationPointerR ();
    readImpl_p->feed2 (fd2);
}

Vector<Float>
ROVisibilityIterator2::feed_pa (Double time) const
{
    CheckImplementationPointerR ();
    return readImpl_p->feed_pa (time);
}

Int
ROVisibilityIterator2::fieldId () const
{
    CheckImplementationPointerR ();
    return readImpl_p->fieldId ();
}

String
ROVisibilityIterator2::fieldName () const
{
    CheckImplementationPointerR ();
    return readImpl_p->fieldName ();
}

void
ROVisibilityIterator2::flag (Cube<Bool>& flags) const
{
    CheckImplementationPointerR ();
    readImpl_p->flag (flags);
}

void
ROVisibilityIterator2::flag (Matrix<Bool>& flags) const
{
    CheckImplementationPointerR ();
    readImpl_p->flag (flags);
}

void
ROVisibilityIterator2::flagCategory (Array<Bool>& flagCategories) const
{
    CheckImplementationPointerR ();
    readImpl_p->flagCategory (flagCategories);
}

void
ROVisibilityIterator2::flagRow (Vector<Bool>& rowflags) const
{
    CheckImplementationPointerR ();
    readImpl_p->flagRow (rowflags);
}

void
ROVisibilityIterator2::floatData (Cube<Float>& fcube) const
{
    CheckImplementationPointerR ();
    readImpl_p->floatData (fcube);
}

const Cube<RigidVector<Double, 2> >&
ROVisibilityIterator2::getBeamOffsets () const
{
    CheckImplementationPointerR ();
    return readImpl_p->getBeamOffsets ();
}

Int
ROVisibilityIterator2::getDataDescriptionId () const
{
    CheckImplementationPointerR ();
    return readImpl_p->getDataDescriptionId ();
}

MEpoch
ROVisibilityIterator2::getEpoch () const
{
    CheckImplementationPointerR ();
    return readImpl_p->getEpoch ();
}

const MSDerivedValues &
ROVisibilityIterator2::getMSD () const
{
    CheckImplementationPointerR ();
    return readImpl_p->getMSD ();
}

Int
ROVisibilityIterator2::getMeasurementSetId () const
{
    CheckImplementationPointerR ();
    return readImpl_p->getMeasurementSetId ();
}

std::vector<MeasurementSet>
ROVisibilityIterator2::getMeasurementSets () const
{
    CheckImplementationPointerR ();
    return readImpl_p->getMeasurementSets ();
}

Int
ROVisibilityIterator2::getNAntennas () const
{
    CheckImplementationPointerR ();
    return readImpl_p->getNAntennas ();
}

//MFrequency::Types
//ROVisibilityIterator2::getObservatoryFrequencyType () const
//{
//    CheckImplementationPointerR ();
//    return readImpl_p->getObservatoryFrequencyType ();
//}

//MPosition
//ROVisibilityIterator2::getObservatoryPosition () const
//{
//    CheckImplementationPointerR ();
//
//    return readImpl_p->getObservatoryPosition ();
//}

//MDirection
//ROVisibilityIterator2::getPhaseCenter () const
//{
//    CheckImplementationPointerR ();
//
//    return readImpl_p->getPhaseCenter ();
//}

VisibilityIteratorReadImpl2 *
ROVisibilityIterator2::getReadImpl () const
{
    return readImpl_p;
}

//void
//ROVisibilityIterator2::getSpwInFreqRange (Block<Vector<Int> >& spw,
//                                         Block<Vector<Int> >& start,
//                                         Block<Vector<Int> >& nchan,
//                                         Double freqStart,
//                                         Double freqEnd,
//                                         Double freqStep,
//                                         MFrequency::Types freqFrame)
//{
//    CheckImplementationPointerR ();
//    readImpl_p->getSpwInFreqRange (spw, start, nchan, freqStart, freqEnd, freqStep, freqFrame);
//}
//
//void
//ROVisibilityIterator2::getFreqInSpwRange(Double& freqStart, Double& freqEnd, MFrequency::Types freqframe){
//  CheckImplementationPointerR ();
//  readImpl_p->getFreqInSpwRange(freqStart, freqEnd, freqframe);
//}

Int
ROVisibilityIterator2::getReportingFrameOfReference () const
{
    return readImpl_p->getReportingFrameOfReference ();
}

void
ROVisibilityIterator2::setReportingFrameOfReference (Int frame)
{
    readImpl_p->setReportingFrameOfReference (frame);
}


SubChunkPair2
ROVisibilityIterator2::getSubchunkId () const
{
    CheckImplementationPointerR ();
    return readImpl_p->getSubchunkId ();
}


VisBuffer2 *
ROVisibilityIterator2::getVisBuffer ()
{
    CheckImplementationPointerR ();
    return readImpl_p->getVisBuffer ();
}

Double
ROVisibilityIterator2::hourang (Double time) const
{
    CheckImplementationPointerR ();
    return readImpl_p->hourang (time);
}

Double
ROVisibilityIterator2::hourangCalculate (Double time, MSDerivedValues & msd, const MEpoch & mEpoch0)
{
    return VisibilityIteratorReadImpl2::hourangCalculate (time, msd, mEpoch0);
}

//Matrix<Float>&
//ROVisibilityIterator2::imagingWeight (Matrix<Float>& wt) const
//{
//    CheckImplementationPointerR ();
//    return readImpl_p->imagingWeight (wt);
//}

Vector<Double>
ROVisibilityIterator2::getFrequencies (Double time, Int frameOfReference) const
{
    CheckImplementationPointerR ();
    return readImpl_p->getFrequencies (time, frameOfReference);
}

Vector<Int>
ROVisibilityIterator2::getChannels (Double time, Int frameOfReference) const
{
    CheckImplementationPointerR ();
    return readImpl_p->getChannels (time, frameOfReference);
}


const VisImagingWeight &
ROVisibilityIterator2::getImagingWeightGenerator () const
{
    CheckImplementationPointerR ();
    return readImpl_p->getImagingWeightGenerator ();
}

Bool
ROVisibilityIterator2::isAsynchronous () const
{
//    Bool isAsync = readImpl_p != NULL && dynamic_cast<const ViReadImplAsync2 *> (readImpl_p) != NULL;
//
//    return isAsync;

    return False; // for now
}


Bool
ROVisibilityIterator2::isAsynchronousIoEnabled()
{
    // Determines whether asynchronous I/O is enabled by looking for the
    // expected AipsRc value.  If not found then async i/o is disabled.

    Bool isEnabled;
    AipsrcValue<Bool>::find (isEnabled, getAipsRcBase () + ".enabled", False);

    return isEnabled;
}

//Bool
//ROVisibilityIterator2::isInSelectedSPW (const Int& spw)
//{
//    CheckImplementationPointerR ();
//    return readImpl_p->isInSelectedSPW (spw);
//}

Bool
ROVisibilityIterator2::isWritable () const
{
    CheckImplementationPointerR ();
    return readImpl_p->isWritable ();
}

Bool
ROVisibilityIterator2::more () const
{
    CheckImplementationPointerR ();
    return readImpl_p->more ();
}

Bool
ROVisibilityIterator2::moreChunks () const
{
    CheckImplementationPointerR ();
    return readImpl_p->moreChunks ();
}

Int
ROVisibilityIterator2::msId () const
{
    CheckImplementationPointerR ();
    return readImpl_p->msId ();
}

Int
ROVisibilityIterator2::nPolarizations () const
{
    CheckImplementationPointerR ();
    return readImpl_p->nPolarizations ();
}

Int
ROVisibilityIterator2::nRows () const
{
    CheckImplementationPointerR ();
    return readImpl_p->nRows ();
}

Int
ROVisibilityIterator2::nRowsInChunk () const
{
    CheckImplementationPointerR ();
    return readImpl_p->nRowsInChunk ();
}

Bool
ROVisibilityIterator2::newArrayId () const
{
    CheckImplementationPointerR ();
    return readImpl_p->newArrayId ();
}

Bool
ROVisibilityIterator2::newFieldId () const
{
    CheckImplementationPointerR ();
    return readImpl_p->newFieldId ();
}

Bool
ROVisibilityIterator2::isNewMS () const
{
    CheckImplementationPointerR ();
    return readImpl_p->isNewMs ();
}

Bool
ROVisibilityIterator2::newSpectralWindow () const
{
    CheckImplementationPointerR ();
    return readImpl_p->newSpectralWindow ();
}

ROVisibilityIterator2&
ROVisibilityIterator2::nextChunk ()
{
    CheckImplementationPointerR ();
    readImpl_p->nextChunk ();

    return * this;
}

Int
ROVisibilityIterator2::numberAnt ()
{
    CheckImplementationPointerR ();
    return readImpl_p->numberAnt ();
}

Int
ROVisibilityIterator2::numberCoh ()
{
    CheckImplementationPointerR ();
    return readImpl_p->numberCoh ();
}

Int
ROVisibilityIterator2::numberDDId ()
{
    CheckImplementationPointerR ();
    return readImpl_p->numberDDId ();
}

Int
ROVisibilityIterator2::numberPol ()
{
    CheckImplementationPointerR ();
    return readImpl_p->numberPol ();
}

Int
ROVisibilityIterator2::numberSpw ()
{
    CheckImplementationPointerR ();
    return readImpl_p->numberSpw ();
}

void
ROVisibilityIterator2::observationId (Vector<Int>& obsids) const
{
    CheckImplementationPointerR ();
    readImpl_p->observationId (obsids);
}

void
ROVisibilityIterator2::origin ()
{
    CheckImplementationPointerR ();
    readImpl_p->origin ();
}

void
ROVisibilityIterator2::originChunks ()
{
    CheckImplementationPointerR ();
    readImpl_p->originChunks ();
}

void
ROVisibilityIterator2::originChunks (Bool forceRewind)
{
    CheckImplementationPointerR ();
    readImpl_p->originChunks (forceRewind);
}

Vector<Float>
ROVisibilityIterator2::parang (Double time) const
{
    CheckImplementationPointerR ();
    return readImpl_p->parang (time);
}

const Float&
ROVisibilityIterator2::parang0 (Double time) const
{
    CheckImplementationPointerR ();
    return readImpl_p->parang0 (time);
}


Float
ROVisibilityIterator2::parang0Calculate (Double time, MSDerivedValues & msd, const MEpoch & epoch0)
{
    return VisibilityIteratorReadImpl2::parang0Calculate (time, msd, epoch0);
}

Vector<Float>
ROVisibilityIterator2::parangCalculate (Double time, MSDerivedValues & msd,
                                       int nAntennas, const MEpoch mEpoch0)
{
    return VisibilityIteratorReadImpl2::parangCalculate (time, msd, nAntennas, mEpoch0);
}

const MDirection&
ROVisibilityIterator2::phaseCenter () const
{
    CheckImplementationPointerR ();
    return readImpl_p->phaseCenter ();
}

Int
ROVisibilityIterator2::polFrame () const
{
    CheckImplementationPointerR ();
    return readImpl_p->polFrame ();
}

Int
ROVisibilityIterator2::polarizationId () const
{
    CheckImplementationPointerR ();
    return readImpl_p->polarizationId ();
}

void
ROVisibilityIterator2::processorId (Vector<Int>& procids) const
{
    CheckImplementationPointerR ();
    readImpl_p->processorId (procids);
}

const Cube<Double>&
ROVisibilityIterator2::receptorAngles () const
{
    CheckImplementationPointerR ();
    return readImpl_p->receptorAngles ();
}

Vector<Float>
ROVisibilityIterator2::getReceptor0Angle ()
{
    CheckImplementationPointerR ();
    return readImpl_p->getReceptor0Angle();
}

void
ROVisibilityIterator2::getRowIds (Vector <uInt> & value) const
{
    CheckImplementationPointerR();

    readImpl_p->getRowIds (value);
}

void
ROVisibilityIterator2::scan (Vector<Int>& scans) const
{
    CheckImplementationPointerR ();
    readImpl_p->scan (scans);
}

void
ROVisibilityIterator2::setFrequencySelection (const FrequencySelection & selection)
{
    FrequencySelections selections;
    selections.add (selection);
    setFrequencySelection (selections);
}

void
ROVisibilityIterator2::setFrequencySelection (const FrequencySelections & selections)
{
    ThrowIf (selections.size () != nMS_p,
             utilj::format ("Frequency selection size, %d, does not VisibilityIterator # of MSs, %d.",
                     nMS_p, selections.size()));

    CheckImplementationPointerR ();
    readImpl_p->setFrequencySelections (selections);
}


Double
ROVisibilityIterator2::getInterval() const
{
    CheckImplementationPointerR ();
    return readImpl_p->getInterval();
}

void
ROVisibilityIterator2::setInterval (Double timeInterval)
{
    CheckImplementationPointerR ();
    readImpl_p->setInterval (timeInterval);
}

void
ROVisibilityIterator2::setRowBlocking (Int nRows) // for use by Async I/O *ONLY
{
    CheckImplementationPointerR ();
    readImpl_p->setRowBlocking (nRows);
}

void
ROVisibilityIterator2::sigma (Vector<Float>& sig) const
{
    CheckImplementationPointerR ();
    readImpl_p->sigma (sig);
}

void
ROVisibilityIterator2::sigmaMat (Matrix<Float>& sigmat) const
{
    CheckImplementationPointerR ();
    readImpl_p->sigmaMat (sigmat);
}

void
ROVisibilityIterator2::slurp () const
{
    CheckImplementationPointerR ();
    readImpl_p->slurp ();
}

String
ROVisibilityIterator2::sourceName () const
{
    CheckImplementationPointerR ();
    return readImpl_p->sourceName ();
}

Int
ROVisibilityIterator2::spectralWindow () const
{
    CheckImplementationPointerR ();
    return readImpl_p->spectralWindow ();
}

void
ROVisibilityIterator2::stateId (Vector<Int>& stateids) const
{
    CheckImplementationPointerR ();
    readImpl_p->stateId (stateids);
}

const vi::SubtableColumns &
ROVisibilityIterator2::subtableColumns () const
{
    CheckImplementationPointerR ();
    return readImpl_p->subtableColumns ();
}

void
ROVisibilityIterator2::time (Vector<Double>& t) const
{
    CheckImplementationPointerR ();
    readImpl_p->time (t);
}

void
ROVisibilityIterator2::timeCentroid (Vector<Double>& t) const
{
    CheckImplementationPointerR ();
    readImpl_p->timeCentroid (t);
}

void
ROVisibilityIterator2::timeInterval (Vector<Double>& ti) const
{
    CheckImplementationPointerR ();
    readImpl_p->timeInterval (ti);
}

void
ROVisibilityIterator2::useImagingWeight (const VisImagingWeight& imWgt)
{
    CheckImplementationPointerR ();
    readImpl_p->useImagingWeight (imWgt);
}

void
ROVisibilityIterator2::uvw (Matrix<Double>& uvw) const
{
    CheckImplementationPointerR ();
    readImpl_p->uvw (uvw);
}

void
ROVisibilityIterator2::visibilityCorrected (Cube<Complex>& vis) const
{
    CheckImplementationPointerR ();
    readImpl_p->visibilityCorrected (vis);
}

void
ROVisibilityIterator2::visibilityModel (Cube<Complex>& vis) const
{
    CheckImplementationPointerR ();
    readImpl_p->visibilityModel (vis);
}

void
ROVisibilityIterator2::visibilityObserved (Cube<Complex>& vis) const
{
    CheckImplementationPointerR ();
    readImpl_p->visibilityObserved (vis);
}

void
ROVisibilityIterator2::visibilityCorrected (Matrix<CStokesVector>& vis) const
{
    CheckImplementationPointerR ();
    readImpl_p->visibilityCorrected (vis);
}

void
ROVisibilityIterator2::visibilityModel (Matrix<CStokesVector>& vis) const
{
    CheckImplementationPointerR ();
    readImpl_p->visibilityModel (vis);
}

void
ROVisibilityIterator2::visibilityObserved (Matrix<CStokesVector>& vis) const
{
    CheckImplementationPointerR ();
    readImpl_p->visibilityObserved (vis);
}

IPosition
ROVisibilityIterator2::visibilityShape () const
{
    CheckImplementationPointerR ();
    return readImpl_p->visibilityShape ();
}

void
ROVisibilityIterator2::weight (Vector<Float>& wt) const
{
    CheckImplementationPointerR ();
    readImpl_p->weight (wt);
}

void
ROVisibilityIterator2::weightMat (Matrix<Float>& wtmat) const
{
    CheckImplementationPointerR ();
    readImpl_p->weightMat (wtmat);
}

void
ROVisibilityIterator2::weightSpectrum (Cube<Float>& wtsp) const
{
    CheckImplementationPointerR ();
    readImpl_p->weightSpectrum (wtsp);
}

VisibilityIterator2::VisibilityIterator2 (const MeasurementSet & ms,
                                          const Block<Int>& sortColumns,
                                          const VisBufferComponents2 * prefetchColumns,
                                          const Bool addDefaultSortCols,
                                          Double timeInterval)
: ROVisibilityIterator2 (prefetchColumns, Block<MeasurementSet> (1, ms), sortColumns,
                         addDefaultSortCols, timeInterval, True)
{
    construct ();
}

VisibilityIterator2::VisibilityIterator2 (const Block<MeasurementSet>& mss,
                                          const Block<Int>& sortColumns,
                                          const VisBufferComponents2 * prefetchColumns,
                                          const Bool addDefaultSortCols,
                                          Double timeInterval)
: ROVisibilityIterator2 (prefetchColumns, mss, sortColumns,
                         addDefaultSortCols, timeInterval, True)
{
    construct ();
}

VisibilityIterator2::~VisibilityIterator2 ()
{
    delete writeImpl_p;
}

VisibilityIterator2 &
VisibilityIterator2::operator++ (int)
{
    advance ();

    return * this;
}

VisibilityIterator2 &
VisibilityIterator2::operator++ ()
{
    advance();

    return * this;
}

void
VisibilityIterator2::attachColumns (const Table &t)
{
    CheckImplementationPointerW ();

    writeImpl_p->attachColumns (t);
}

void
VisibilityIterator2::construct ()
{
    if (isAsynchronous ()){
/////////        writeImpl_p = new ViWriteImplAsync2 (this);
    }
    else{
        writeImpl_p = new VisibilityIteratorWriteImpl2 (this);
    }

    /////////////////  readImpl_p->originChunks();
}

VisibilityIteratorWriteImpl2 *
VisibilityIterator2::getWriteImpl () const
{
    return writeImpl_p;
}

Bool
VisibilityIterator2::isWritable () const
{
    return True;
}

void
VisibilityIterator2::writeFlag (const Matrix<Bool>& flag)
{
    CheckImplementationPointerW ();
    writeImpl_p->writeFlag (flag);
}

void
VisibilityIterator2::writeFlag (const Cube<Bool>& flag)
{
    CheckImplementationPointerW ();
    writeImpl_p->writeFlag (flag);
}
void
VisibilityIterator2::writeFlagCategory(const Array<Bool>& flagCategory)
{
    CheckImplementationPointerW ();
    writeImpl_p->writeFlagCategory (flagCategory);
}

void
VisibilityIterator2::writeFlagRow (const Vector<Bool>& rowflags)
{
    CheckImplementationPointerW ();
    writeImpl_p->writeFlagRow (rowflags);
}

void
VisibilityIterator2::writeSigma (const Vector<Float>& sig)
{
    CheckImplementationPointerW ();
    writeImpl_p->writeSigma (sig);
}

void
VisibilityIterator2::writeSigmaMat (const Matrix<Float>& sigmat)
{
    CheckImplementationPointerW ();
    writeImpl_p->writeSigmaMat (sigmat);
}

void
VisibilityIterator2::writeVisCorrected (const Matrix<CStokesVector>& vis)
{
    CheckImplementationPointerW ();
    writeImpl_p->writeVisCorrected (vis);
}

void
VisibilityIterator2::writeVisModel (const Matrix<CStokesVector>& vis)
{
    CheckImplementationPointerW ();
    writeImpl_p->writeVisModel (vis);
}

void
VisibilityIterator2::writeVisObserved (const Matrix<CStokesVector>& vis)
{
    CheckImplementationPointerW ();
    writeImpl_p->writeVisObserved (vis);
}

void
VisibilityIterator2::writeVisCorrected (const Cube<Complex>& vis)
{
    CheckImplementationPointerW ();
    writeImpl_p->writeVisCorrected (vis);
}

void
VisibilityIterator2::writeVisModel (const Cube<Complex>& vis)
{
    CheckImplementationPointerW ();
    writeImpl_p->writeVisModel (vis);
}

void
VisibilityIterator2::writeVisObserved (const Cube<Complex>& vis)
{
    CheckImplementationPointerW ();
    writeImpl_p->writeVisObserved (vis);
}

void
VisibilityIterator2::writeWeight (const Vector<Float>& wt)
{
    CheckImplementationPointerW ();
    writeImpl_p->writeWeight (wt);
}

void
VisibilityIterator2::writeWeightMat (const Matrix<Float>& wtmat)
{
    CheckImplementationPointerW ();
    writeImpl_p->writeWeightMat (wtmat);
}

void
VisibilityIterator2::writeWeightSpectrum (const Cube<Float>& wtsp)
{
    CheckImplementationPointerW ();
    writeImpl_p->writeWeightSpectrum (wtsp);
}

void
VisibilityIterator2::putModel(const RecordInterface& rec, Bool iscomponentlist, Bool incremental){
  CheckImplementationPointerW ();
  writeImpl_p->putModel(rec, iscomponentlist, incremental);

}

void
VisibilityIterator2::writeBackChanges (VisBuffer2 * vb)
{
    CheckImplementationPointerW ();
    writeImpl_p->writeBackChanges (vb);
}

SubtableColumns::SubtableColumns (const MSIter & msIter)
: msIter_p (msIter)
{}

const ROMSAntennaColumns&
SubtableColumns::antenna() const
{
    return msIter_p.msColumns().antenna();
}

const ROMSDataDescColumns&
SubtableColumns::dataDescription() const
{
    return msIter_p.msColumns().dataDescription();
}

const ROMSFeedColumns&
SubtableColumns::feed() const
{
    return msIter_p.msColumns().feed();
}

const ROMSFieldColumns&
SubtableColumns::field() const
{
    return msIter_p.msColumns().field();
}

const ROMSFlagCmdColumns&
SubtableColumns::flagCmd() const
{
    return msIter_p.msColumns().flagCmd();
}

const ROMSHistoryColumns&
SubtableColumns::history() const
{
    return msIter_p.msColumns().history();
}

const ROMSObservationColumns&
SubtableColumns::observation() const
{
    return msIter_p.msColumns().observation();
}

const ROMSPointingColumns&
SubtableColumns::pointing() const
{
    return msIter_p.msColumns().pointing();
}

const ROMSPolarizationColumns&
SubtableColumns::polarization() const
{

    return msIter_p.msColumns().polarization();
}

const ROMSProcessorColumns&
SubtableColumns::processor() const
{
    return msIter_p.msColumns().processor();
}

const ROMSSpWindowColumns&
SubtableColumns::spectralWindow() const
{

    return msIter_p.msColumns().spectralWindow();
}

const ROMSStateColumns&
SubtableColumns::state() const
{
    return msIter_p.msColumns().state();
}

const ROMSDopplerColumns&
SubtableColumns::doppler() const
{
    return msIter_p.msColumns().doppler();
}

const ROMSFreqOffsetColumns&
SubtableColumns::freqOffset() const
{
    return msIter_p.msColumns().freqOffset();
}

const ROMSSourceColumns&
SubtableColumns::source() const
{
    return msIter_p.msColumns().source();
}

const ROMSSysCalColumns&
SubtableColumns::sysCal() const
{
    return msIter_p.msColumns().sysCal();
}

const ROMSWeatherColumns&
SubtableColumns::weather() const
{
    return msIter_p.msColumns().weather();
}


} // end namespace vi


} // end namespace casa

