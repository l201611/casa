#include "VisibilityIterator_Test.h"

#include <ms/MeasurementSets/MeasurementSet.h>
#include <tables/Tables/Table.h>
#include <tables/Tables/TableDesc.h>
#include <tables/Tables/TiledDataStMan.h>
#include <tables/Tables/TiledShapeStMan.h>
#include <ms/MeasurementSets/MSAntenna.h>
#include <ms/MeasurementSets/NewMSSimulator.h>
#include <measures/Measures/MeasTable.h>
#include <synthesis/MSVis/UtilJ.h>
#include <synthesis/MSVis/VisibilityIterator.h>
#include <synthesis/MSVis/VisBuffer.h>
#include <synthesis/MSVis/VisibilityIterator2.h>
#include <synthesis/MSVis/VisBuffer2.h>
#include <boost/tuple/tuple.hpp>

using namespace std;
using namespace casa;
using namespace casa::vi;

int
main (int /*nArgs*/, char * /*args*/ [])
{
    using namespace casa::vi::test;

    Tester tester;

    tester.doTests ();

//    MsFactory msf ("test.ms");
//
//    casa::MeasurementSet * ms;
//    int nRows;
//    boost::tie (ms, nRows) = msf.createMs ();
//
//    printMs (ms);

    return 0;
}

namespace casa {
namespace vi {
namespace test {

MsFactory::MsFactory (const String & msName)
 : simulator_p (new NewMSSimulator (msName)),
   timeStart_p (-1)
{
    ms_p = new MeasurementSet (* simulator_p->getMs ()); //
}

MsFactory::~MsFactory ()
{
    delete ms_p;
}

void
MsFactory::addAntennas (Int nAntennas)
{
    Vector<Double> x (nAntennas), y (nAntennas), z (nAntennas),
                   diameter (nAntennas), offset (nAntennas);

    x [0] = 0;
    y [0] = 0;
    z [0] = 0;

    Vector<String> mount (nAntennas), name (nAntennas), pad (nAntennas);

    for (Int i = 1; i < nAntennas; i++){

        Double angle = ((i - 1) % 3) * (2 * 3.14159 / 3.0);
        Double radius = (i - 1) / 3.0 * 100;

        x [i] = radius * cos (angle);
        y [i] = radius * sin (angle);
        z [0] = 0;

        name [i] = String::format ("a%02d", i);
        pad [i] = String::format ("p%02d", i);
    }

    diameter = 10;
    offset = 0;
    mount = "ALT-AZ";

    MPosition vlaPosition;
    MeasTable::Observatory(vlaPosition, "VLA");

    simulator_p->initAnt ("Simulated", x, y, z, diameter, offset,
                          mount, name, pad, "local", vlaPosition);

    nAntennas_p = nAntennas;

}


//void
//MsFactory::addColumn (MSMainEnums::PredefinedColumns columnId)
//{
//    columnIds_p.insert (columnId);
//}


void
MsFactory::addDefaults ()
{
    // Configure fields if not present.

    Int nFields;
    Vector<String> sourceName;
    Vector<MDirection> sourceDirection;
    Vector<String> calCode;

    simulator_p->getFields (nFields, sourceName, sourceDirection, calCode);

    if (nFields == 0){

        Quantity ra (85.25, "deg");   // 05 41.0  -02 25 (horsehead nebula)
        Quantity dec (-2.417, "deg");
        MDirection direction (ra, dec);

        addField ("HorseHeadNebula", direction);
    }

    // Configure antennas if not present

    String telescope;
    Int nAntennas;
    Matrix<Double> xyz;
    Vector<Double> diameter;
    Vector<Double> offset;
    Vector<String> mount;
    Vector<String> name;
    Vector<String> pad;
    String coordinateSystem;
    MPosition referenceLocation;

    Bool ok = simulator_p->getAnt (telescope, nAntennas, & xyz, diameter, offset,
                                   mount, name, pad, coordinateSystem, referenceLocation);

    if (! ok){
        addAntennas (4);
    }

    // Configure feeds

    Vector<Double> x (2) , y (2);
    Vector<String> polarization (2);

    x[0] = 0;
    y[0] = .005;
    polarization [0] = "R";

    x[1] = 0;
    y[1] = .005;
    polarization [1] = "L";

    simulator_p->initFeeds ("", x, y, polarization);

    Int nSpectralWindows;
    Vector<String> spWindowNames;
    Vector<Int> nChannels;
    Vector<Quantity> startFrequencies;
    Vector<Quantity> frequencyDeltas;
    Vector<String> stokesString;

    ok = simulator_p->getSpWindows(nSpectralWindows,
                                   spWindowNames,
                                   nChannels,
                                   startFrequencies,
                                   frequencyDeltas,
                                   stokesString);

    if (! ok || nSpectralWindows == 0){

        addSpectralWindows (4);
    }

    if (timeStart_p < 0){
        timeStart_p = 0;
        timeInterval_p = 1;
        timeEnd_p = 15;
    }

}

void
MsFactory::addField (const String & name,
                     const MDirection & direction)
{
    simulator_p->initFields (name, direction, "");
}

void
MsFactory::addSpectralWindows (int nSpectralWindows)
{
    for (Int i = 0; i < nSpectralWindows; i++){

        String name = String::format ("sp%d", i);

        addSpectralWindow (name,
                           10 + i,
                           1e9 * (i + 1),
                           1e6 * (i + 1),
                           "LL RR RL LR");
    }
}

void
MsFactory::addSpectralWindow (const String & name,
                              Int nChannels,
                              Double frequency,
                              Double frequencyDelta,
                              const String & stokes)
{
    simulator_p->initSpWindows (name,
                                nChannels,
                                Quantity (frequency, "Hz"),
                                Quantity (frequencyDelta, "Hz"),
                                Quantity (frequencyDelta, "Hz"),
                                MFrequency::TOPO,
                                stokes);

//    ThrowIf (! ok, String::format ("Failed to add spectral window "
//                                   "(name=%s, nChannels=%d, fr=%f, dFr=%f, stokes=%s",
//                                   name.c_str(), nChannels, frequency, frequencyDelta,
//                                   stokes));
}

void
MsFactory::addColumns ()
{
    TableDesc tableDescription;// = ms_p->actualTableDesc ();

    MS::addColumnToDesc (tableDescription, MS::WEIGHT_SPECTRUM, 2);
    IPosition tileShape (3, 4, 100, 100);

    TiledShapeStMan storageManager ("WeightSpectrumTiled", tileShape);

    ms_p->addColumn (tableDescription, storageManager);

    ms_p->flush();
}

void
MsFactory::attachColumns ()
{
    const ColumnDescSet & cds = ms_p->tableDesc ().columnDescSet ();

    columns_p.antenna1_p.attach (* ms_p, MS::columnName (MS::ANTENNA1));
    columns_p.antenna2_p.attach (* ms_p, MS::columnName (MS::ANTENNA2));

    if (cds.isDefined ("CORRECTED_DATA")) {
        columns_p.corrVis_p.attach (* ms_p, "CORRECTED_DATA");
    }

    columns_p.dataDescriptionId_p.attach (* ms_p, MS::columnName (MS::DATA_DESC_ID));
    columns_p.exposure_p.attach (* ms_p, MS::columnName (MS::EXPOSURE));
    columns_p.field_p.attach (* ms_p, MS::columnName (MS::FIELD_ID));
    columns_p.feed1_p.attach (* ms_p, MS::columnName (MS::FEED1));
    columns_p.feed2_p.attach (* ms_p, MS::columnName (MS::FEED2));
    columns_p.flag_p.attach (* ms_p, MS::columnName (MS::FLAG));
    columns_p.flagCategory_p.attach (* ms_p, MS::columnName (MS::FLAG_CATEGORY));
    columns_p.flagRow_p.attach (* ms_p, MS::columnName (MS::FLAG_ROW));

    if (cds.isDefined (MS::columnName (MS::FLOAT_DATA))) {
        columns_p.floatVis_p.attach (* ms_p, MS::columnName (MS::FLOAT_DATA));
        //floatDataFound_p = True;
    } else {
        //floatDataFound_p = False;
    }

    if (cds.isDefined ("MODEL_DATA")) {
        columns_p.modelVis_p.attach (* ms_p, "MODEL_DATA");
    }

    columns_p.observation_p.attach (* ms_p, MS::columnName (MS::OBSERVATION_ID));
    columns_p.processor_p.attach (* ms_p, MS::columnName (MS::PROCESSOR_ID));
    columns_p.scan_p.attach (* ms_p, MS::columnName (MS::SCAN_NUMBER));
    columns_p.sigma_p.attach (* ms_p, MS::columnName (MS::SIGMA));
    columns_p.state_p.attach (* ms_p, MS::columnName (MS::STATE_ID));
    columns_p.time_p.attach (* ms_p, MS::columnName (MS::TIME));
    columns_p.timeCentroid_p.attach (* ms_p, MS::columnName (MS::TIME_CENTROID));
    columns_p.timeInterval_p.attach (* ms_p, MS::columnName (MS::INTERVAL));
    columns_p.uvw_p.attach (* ms_p, MS::columnName (MS::UVW));

    if (cds.isDefined (MS::columnName (MS::DATA))) {
        columns_p.vis_p.attach (* ms_p, MS::columnName (MS::DATA));
    }

    columns_p.weight_p.attach (* ms_p, MS::columnName (MS::WEIGHT));

    if (cds.isDefined ("WEIGHT_SPECTRUM")) {
        columns_p.weightSpectrum_p.attach (* ms_p, "WEIGHT_SPECTRUM");
    }
}

pair<MeasurementSet *, Int>
MsFactory::createMs ()
{
    addColumns ();

    fillData ();

    ms_p->flush();

    pair<MeasurementSet *, Int> result = make_pair (ms_p, nRows_p);

    ms_p = 0; // give up all ownership and access

    return result;
}

void
MsFactory::fillData ()
{
    addDefaults ();

    attachColumns ();

    FillState fillState;

    fillState.rowNumber_p = 0;
    fillState.nAntennas_p = nAntennas_p;
    fillState.nFlagCategories_p = 3;
    fillState.timeDelta_p = timeInterval_p;

    Double time = timeStart_p;

    Int nSpectralWindows;
    Vector<String> spWindowNames;
    Vector<Int> nChannels;
    Vector<Quantity> startFrequencies;
    Vector<Quantity> frequencyDeltas;
    Vector<String> stokesString;

    simulator_p->getSpWindows(nSpectralWindows,
                              spWindowNames,
                              nChannels,
                              startFrequencies,
                              frequencyDeltas,
                              stokesString);

    while (time < timeEnd_p){

        fillState.time_p = time;

        printf ("t=%010.1f\n", time);

        for (Int j = 0; j < nSpectralWindows; j++){

            fillState.spectralWindow_p = j;
            fillState.nChannels_p = nChannels [j];
            vector<String> stokesComponents = utilj::split (stokesString [j], " ", True);
            fillState.nCorrelations_p = stokesComponents.size();

            fillRows (fillState);
        }

        time += timeInterval_p;
    }

    printf ("\n---Total of %d rows filled\n", fillState.rowNumber_p);

    nRows_p = fillState.rowNumber_p;
}

void
MsFactory::fillRows (FillState & fillState)
{
    // Extend the MS to have one row per every unique pairing
    // of antennas.

    Int n = fillState.nAntennas_p;
    Int nNewRows = (n * (n - 1)) / 2;
    ms_p->addRow(nNewRows);

    // Fill in a row for every unique antenna pairing.

    for (Int a1 = 0; a1 < fillState.nAntennas_p - 1; a1 ++){

        fillState.antenna1_p = a1;

        for (Int a2 = a1 + 1; a2 < fillState.nAntennas_p; a2 ++){

            fillState.antenna2_p = a2;

            fillCubes (fillState);

            fillFlagCategories (fillState);

            fillCollections (fillState);

            fillScalars (fillState);

            fillState.rowNumber_p ++;
        }
    }
}

template <typename T>
void
MsFactory::fillCube (ArrayColumn<T> & column, const FillState & fillState,
                     const GeneratorBase * generatorBase)
{

    const Generator<T> * generator = dynamic_cast <const Generator<T> *> (generatorBase);

    ThrowIf (generator == 0, "Bad return type on generator");

    Matrix <T> cell (IPosition (2, fillState.nCorrelations_p, fillState.nChannels_p));

    for (Int channel = 0; channel < fillState.nChannels_p; channel ++){

        for (Int correlation = 0;
             correlation < fillState.nCorrelations_p;
             correlation ++){

            cell (correlation, channel) = (* generator) (fillState, channel, correlation);

        }
    }

    column.put (fillState.rowNumber_p, cell);
}

template <typename T>
void
MsFactory::fillScalar (ScalarColumn<T> & column, const FillState & fillState,
                       const GeneratorBase * generatorBase)
{
    const Generator<T> * generator = dynamic_cast <const Generator<T> *> (generatorBase);

    ThrowIf (generator == 0, "Bad return type on generator");

    ThrowIf (column.isNull(),
             String::format ("Column is not attached (%s)", typeid (column).name()));

    T value = (* generator) (fillState, -1, -1);

    column.put (fillState.rowNumber_p, value);
}


void
MsFactory::fillCubes (FillState & fillState)
{
    fillVisCubeCorrected (fillState);
    fillVisCubeModel (fillState);
    fillVisCubeObserved (fillState);

    fillWeightSpectrumCube (fillState);
    fillFlagCube (fillState);
}

void
MsFactory::fillFlagCategories (const FillState & fillState)
{

    const Generator<Vector<Bool> > * generator =
            dynamic_cast <const Generator<Vector <Bool> > *> (generators_p.get(MSMainEnums::FLAG_CATEGORY));

    ThrowIf (generator == 0, "Bad return type on generator");

    Cube <Bool> cell (IPosition (3, fillState.nCorrelations_p, fillState.nChannels_p,
                                  fillState.nFlagCategories_p));

    for (Int channel = 0; channel < fillState.nChannels_p; channel ++){

        for (Int correlation = 0;
             correlation < fillState.nCorrelations_p;
             correlation ++){

            Vector<Bool> categories = (* generator) (fillState, channel, correlation);

            for (Int category = 0;
                 category < fillState.nFlagCategories_p;
                 category ++){

                cell (correlation, channel, category) = categories (category);
            }
        }
    }

    columns_p.flagCategory_p.put (fillState.rowNumber_p, cell);
}


void
MsFactory::fillUvw (FillState & fillState)
{
    const GeneratorBase * generatorBase = generators_p.get (MSMainEnums::UVW);
    const Generator<Vector <Double> > * generator =
            dynamic_cast<const Generator<Vector <Double> > *> (generatorBase);

    Vector<Double> uvw (3);
    uvw = (* generator) (fillState, -1, -1);

    columns_p.uvw_p.put (fillState.rowNumber_p, uvw);
}


void
MsFactory::fillVisCubeCorrected (FillState & fillState)
{
    if (! columns_p.corrVis_p.isNull ()){

        fillCube (columns_p.corrVis_p, fillState, generators_p.get(MSMainEnums::CORRECTED_DATA));
    }
}

void
MsFactory::fillWeightSpectrumCube (FillState & fillState)
{
    if (! columns_p.weightSpectrum_p.isNull()){

        fillCube (columns_p.weightSpectrum_p, fillState, generators_p.get(MSMainEnums::WEIGHT_SPECTRUM));
    }
}

void
MsFactory::fillFlagCube (FillState & fillState)
{
    fillCube (columns_p.flag_p, fillState, generators_p.get(MSMainEnums::FLAG));
}


void
MsFactory::fillVisCubeModel (FillState & fillState)
{
    if (! columns_p.modelVis_p.isNull ()){

        fillCube (columns_p.modelVis_p, fillState, generators_p.get(MSMainEnums::MODEL_DATA));
    }
}

void
MsFactory::fillVisCubeObserved (FillState & fillState)
{
    fillCube (columns_p.vis_p, fillState, generators_p.get (MSMainEnums::DATA));
}

void
MsFactory::fillCollections (FillState & fillState)
{
    fillWeight (fillState);
    fillSigma (fillState);
    fillUvw (fillState);
}

void
MsFactory::fillScalars (FillState & fillState)
{
    fillScalar (columns_p.antenna1_p, fillState, generators_p.get (MSMainEnums::ANTENNA1));
    fillScalar (columns_p.antenna2_p, fillState, generators_p.get (MSMainEnums::ANTENNA2));
    fillScalar (columns_p.dataDescriptionId_p, fillState, generators_p.get (MSMainEnums::DATA_DESC_ID));
    fillScalar (columns_p.exposure_p, fillState, generators_p.get (MSMainEnums::EXPOSURE));
    fillScalar (columns_p.feed1_p, fillState, generators_p.get (MSMainEnums::FEED1));
    fillScalar (columns_p.feed2_p, fillState, generators_p.get (MSMainEnums::FEED2));
    fillScalar (columns_p.field_p, fillState, generators_p.get (MSMainEnums::FIELD_ID));
    fillScalar (columns_p.flagRow_p, fillState, generators_p.get (MSMainEnums::FLAG_ROW));
    fillScalar (columns_p.observation_p, fillState, generators_p.get (MSMainEnums::OBSERVATION_ID));
    fillScalar (columns_p.processor_p, fillState, generators_p.get (MSMainEnums::PROCESSOR_ID));
    fillScalar (columns_p.scan_p, fillState, generators_p.get (MSMainEnums::SCAN_NUMBER));
    fillScalar (columns_p.state_p, fillState, generators_p.get (MSMainEnums::STATE_ID));
    fillScalar (columns_p.timeCentroid_p, fillState, generators_p.get (MSMainEnums::TIME_CENTROID));
    fillScalar (columns_p.timeInterval_p, fillState, generators_p.get (MSMainEnums::INTERVAL));
    fillScalar (columns_p.time_p, fillState, generators_p.get (MSMainEnums::TIME));
}

void
MsFactory::fillSigma (FillState & fillState)
{
    Vector<Float> sigmas (fillState.nCorrelations_p);
    const Generator<Float> * generator =
            dynamic_cast <const Generator<Float> *> (generators_p.get (MSMainEnums::SIGMA));

    for (Int i = 0; i < fillState.nCorrelations_p; i ++){
        sigmas (i) = (* generator) (fillState, -1, i);
    }


    columns_p.sigma_p.put (fillState.rowNumber_p, sigmas);

}


void
MsFactory::fillWeight (FillState & fillState)
{
    Vector<Float> weights (fillState.nCorrelations_p);
    const GeneratorBase * generatorBase = generators_p.get (MSMainEnums::WEIGHT);
    const Generator<Float> * generator =
        dynamic_cast <const Generator<Float> *> (generatorBase);

    for (Int i = 0; i < fillState.nCorrelations_p; i ++){
        weights (i) = (* generator) (fillState, -1, i);
    }

    columns_p.weight_p.put (fillState.rowNumber_p, weights);
}

void
MsFactory::setDataGenerator (MSMainEnums::PredefinedColumns column, GeneratorBase * generator)
{
    generators_p.set (column, generator);
}

void
MsFactory::setTimeInfo (Double startingTime, Double endingTime, Double interval)
{
    timeStart_p = startingTime;
    timeEnd_p = endingTime;
    timeInterval_p = interval;
}


template <typename T>
void
printMsCubePart (Int row, const Cube<T> & array, const String & name)
{
    IPosition shape = array.shape();

    for (Int i = 0; i < shape (1); i++){
        printf ("%s [0, %2d]: ", name.c_str(), i);
        for (Int j = 0; j < shape (0); j++){

            printf ("%8.0f", array (j, i, row));

        }
        printf ("\n");
    }

    if (array.nelements () == 0){
        printf ("%s is empty\n", name.c_str());
    }
    printf ("   ----------\n");
}

template <typename>
void
printMsCubePart (Int row, const Cube<Bool> & array, const String & name)
{
    IPosition shape = array.shape();

    for (Int i = 0; i < shape (1); i++){
        printf ("%s [0, %2d]: ", name.c_str(), i);
        for (Int j = 0; j < shape (0); j++){

            printf ("%2s", array (j, i, row) ? "T" : "F");

        }
        printf ("\n");
    }

    if (array.nelements () == 0){
        printf ("%s is empty\n", name.c_str());
    }
    printf ("   ----------\n");
}

template <>
void
printMsCubePart (Int row, const Cube<Complex> & array, const String & name)
{
    IPosition shape = array.shape();

    for (Int i = 0; i < shape (1); i++){
        printf ("%s [0, %2d]: ", name.c_str(), i);
        for (Int j = 0; j < shape (0); j++){

            Complex z = array (j, i, row);

            printf ("(%8.0f,%8.0f) ", z.real(), z.imag());

        }
        printf ("\n");
    }

    if (array.nelements () == 0){
        printf ("%s is empty\n", name.c_str());
    }
    printf ("   ----------\n");
}


void
printMs (MeasurementSet * ms)
{
    Block<Int> noSortColumns;
    VisibilityIterator vi (* ms, noSortColumns);
    VisBuffer vb;
    vb.attachToVisIter (vi);

    for (vi.originChunks (); vi.moreChunks(); vi.nextChunk()){

        for (vi.origin(); vi.more (); vi ++){

            printf ("\n");

            for (int i = 0; i < vb.nRow(); i++){

                printf ("r=%d ", vb.rowIds () [i]);
                printf ("t=%.0f ", vb.time () [i]);
                printf ("a1=%d ", vb.antenna1() [i]);
                printf ("a2=%d ", vb.antenna2 () [i]);
                printf ("dd=%d ", vb.dataDescriptionId());
                printf ("sp=%d ", vb.spectralWindow());
                printf ("fld=%d ", vb.fieldId());
                printf ("tc=%3.1f ", vb.timeCentroid() [i]);
                printf ("\n");
                printMsCubePart (i, vb.visCube(), "vis");
                printMsCubePart (i, vb.weightCube(), "wtSpec");
                printMsCubePart<Bool> (i, vb.flagCube(), "flag");
                //printf ("", vb. () [i]);

            }
        }
    }
}

std::pair<Bool,Bool>
Tester::sweepMs (TestWidget & tester)
{
    MeasurementSet * ms;
    pair<Bool,Bool> result;
    Bool moreSweeps = False;

    try {

        Int nRows;
        Bool writableVi;
        boost::tie (ms, nRows, writableVi) = tester.createMs ();

        do {

            Block<Int> noSortColumns;
            auto_ptr <ROVisibilityIterator2> vi (writableVi ? new VisibilityIterator2 (* ms, noSortColumns)
                                                            : new ROVisibilityIterator2 (* ms, noSortColumns));
            VisBuffer2 * vb = vi->getVisBuffer ();
            Int nRowsProcessed = 0;

            tester.startOfData (* vi, vb);

            for (vi->originChunks (); vi->moreChunks(); vi->nextChunk()){

                tester.nextChunk (* vi, vb);

                for (vi->origin(); vi->more (); (* vi) ++){

                    nRowsProcessed += vb->nRows();

                    tester.nextSubchunk (* vi, vb);

                }
                tester.endOfChunk (* vi, vb);
            }

            moreSweeps = tester.noMoreData (* vi, vb, nRowsProcessed);

        } while (moreSweeps);


        result = make_pair (True, False);
    }
    catch (TestError & e){

        fprintf (stderr, "*** TestError while executing test %s:\n-->%s\n",
                 tester.name ().c_str(), e.what());
        result = make_pair (False, False);
    }
    catch (AipsError & e){

        fprintf (stderr, "*** AipsError while executing test %s:\n-->%s\n",
                 tester.name ().c_str(), e.what());

        result = make_pair (False, False);
    }
    catch (...){

        fprintf (stderr, "*** Unknown exception while executing test %s\n***\n*** Exiting ***\n",
                 tester.name ().c_str());
        result = make_pair (False, True);
    }

    fflush (stderr); // just in case things are really bad and we croak

    delete ms;
    return result;
}


template <typename T>
bool
Tester::doTest ()
{
    T t;

    nTestsAttempted_p ++;

    bool ok;
    bool fatal;

    printf ("Performing %s ...\n", t.name().c_str());
    fflush (stdout);
    boost::tie (ok, fatal) = sweepMs (t);
    String result = (ok ? "Passed" : (fatal ? "FATAL ERROR" : "FAILED"));
    printf ("... %s test %s\n", result.c_str(), t.name().c_str());
    fflush (stdout);

    if (ok){
        nTestsPassed_p ++;
    }

    TestErrorIf (fatal, "No message");

    return ok;
}

bool
Tester::doTests ()
{
    nTestsAttempted_p = 0;
    nTestsPassed_p = 0;

    try {

        //// doTest<BasicChannelSelection> ();

        //// doTest<BasicMutation> ();

        //// doTest<FrequencyChannelSelection> ();

        PerformanceComparator pc ("3c391_ctm_mosaic_spw0.ms");
        pc.compare();
    }
    catch (TestError & e){

        fprintf (stderr, "\n\n*** Last error was fatal; ending test!\n");
        fflush (stderr);
        exit (1);
    }

    bool allPassed = nTestsAttempted_p == nTestsPassed_p;
    printf ("Completed testing: ");

    if (allPassed){
        printf ("All %d tests passed ;-)\n", nTestsAttempted_p);
    }
    else{
        printf ("FAILED %d of %d tests attempted ;-0\n", nTestsAttempted_p - nTestsPassed_p, nTestsAttempted_p);
    }

    return allPassed;
}

BasicChannelSelection::BasicChannelSelection ()
: TestWidget ("BasicChannelSelection"),
  factor_p (1),
  msf_p (0),
  nAntennas_p (4),
  nFlagCategories_p (3),
  nSweeps_p (0)
{}

BasicChannelSelection::~BasicChannelSelection ()
{
    delete msf_p;
}

void
BasicChannelSelection::checkRowScalars (VisBuffer2 * vb)
{
    // Check out the non-cube data for each row

    const Vector<uInt> & rowIds = vb->rowIds ();
    Int nRows = vb->nRows();

    Int expectedAntenna1 = 0;
    Int expectedAntenna2 = 1;
    Double previousTime = -1;
    Bool newTimeExpected = True;

    for (Int row = 0; row < nRows; row ++){

        Int rowId = rowIds (row);

        if (newTimeExpected){
            TestErrorIf (vb->time()(row) + 0.5 < previousTime,
                          String::format ("Backwards time=%f (< %f) at msRow=%d",
                                          vb->time()(row), previousTime, rowId));
            previousTime = vb->time()(row);
            newTimeExpected = False;
        }

        TestErrorIf (vb->time()(row) != previousTime,
                      String::format ("Expected time=%f, got %f at msRow=%d",
                                      previousTime, vb->time()(row), rowId));

        TestErrorIf (vb->antenna1 ()(row) != expectedAntenna1,
                      String::format ("Expected %d for antenna1 (%d); got %d",
                                      expectedAntenna1, rowId, vb->antenna1()(rowId)));

        TestErrorIf (vb->antenna2 ()(row) != expectedAntenna2,
                      String::format ("Expected %d for antenna2 (%d); got %d",
                                      expectedAntenna2, rowId, vb->antenna2()(rowId)));

        expectedAntenna2 ++;

        if (expectedAntenna2 >= nAntennas_p){

            expectedAntenna1 ++;
            expectedAntenna2 = expectedAntenna1 + 1;
            newTimeExpected = True;
        }

        // Check UVW

        TestErrorIf (vb->uvw().shape().nelements() != 2 ||
                     vb->uvw().shape()(0) != 3 ||
                     vb->uvw().shape()(1) != nRows,
                     String::format ("Bad uvw shape: expected [3,%d] got [%d,%d]",
                                     nRows, vb->uvw().shape()(0), vb->uvw().shape()(1)));

        for (int i = 0; i < 3; i++){

            Double expected = rowId * 10 + i;

            TestErrorIf (expected != vb->uvw()(i, row),
                          String::format ("Expected %f for uvw (%d, %d); got %f",
                                          expected, rowId, i, vb->uvw()(row, i)));
        }

        Bool flagRowExpected = (rowId % 3 == 0) || (rowId % 5 == 0);
        if (factor_p != 1){
            flagRowExpected = ! flagRowExpected;
        }

        TestErrorIf (vb->flagRow () (row) != flagRowExpected,
                      String::format ("Bad flag row value for msRow=%d; expected %d, got %d",
                                      rowId, flagRowExpected, vb->flagRow () (row) ));

        checkRowScalar (vb->exposure()(row), 8, rowId, "scan");
        checkRowScalar (vb->observationId()(row), 7, rowId, "scan");
        checkRowScalar (vb->scan() (row), 5, rowId, "scan");
        checkRowScalar (vb->timeCentroid ()(row), 1, rowId, "timeCentroid");
        checkRowScalar (vb->timeInterval () (row), 2, rowId, "timeInterval");
        checkRowScalar (vb->sigma () (row), 3, rowId, "sigma", factor_p);
        checkRowScalar (vb->weight () (row), 4, rowId, "weight", factor_p);
    }
}

void
BasicChannelSelection::checkRowScalar (Double value, Double offset, Int rowId, const char * name,
                                       Int factor)
{
    Double expected = rowId * 100 + offset;
    expected *= factor;

    ThrowIf (value != expected,
                 String::format ("Expected %d for %s (%d); got %d",
                                   expected, name, rowId, value));
}


boost::tuple <MeasurementSet *, Int, Bool>
BasicChannelSelection::createMs ()
{
    system ("rm -r BasicChannelSelection.ms");

    msf_p = new MsFactory ("BasicChannelSelection.ms");

    pair<MeasurementSet *, Int> p = msf_p->createMs ();
    return boost::make_tuple (p.first, p.second, False);
}


/*void
BasicChannelSelection::endOfChunk (ROVisibilityIterator2 & vi, VisBuffer2 * vb, Int nRowsProcessed)
{

}*/

/* void
BasicChannelSelection::nextChunk (ROVisibilityIterator2 & vi, VisBuffer2 * vb)
{
}*/

void
BasicChannelSelection::nextSubchunk (ROVisibilityIterator2 & /*vi*/, VisBuffer2 * vb)
{
    // Check out that the subchunk has the appropriate data

    Int spectralWindow = vb->spectralWindow();
    Int nRows = vb->nRows();

    // The expected channels are 0..4 for spw 1, 6..10 for spw 2 and 3..8 for spw 3

    static const Int info [][3] = {{0, 5, 1 }, {6, 5, 1}, {3, 6, 1}, {0, 5, 2}};

    const Cube<Complex> & visibility = vb->visCube();
    const Cube<Complex> & visibilityCorrected = vb->visCubeCorrected();
    const Cube<Complex> & visibilityModel = vb->visCubeModel();

    const Array<Bool> & flagCategories = vb->flagCategory();

    const Vector<uInt> & rowIds = vb->rowIds ();

    Int channelOffset = info [spectralWindow][0];
    Int nChannels = info [spectralWindow][1];
    Int channelIncrement = info [spectralWindow][2];

    // Check visibility shapes

    IPosition expectedShape = IPosition (3, 4, nChannels, nRows);
    TestErrorIf (! visibility.shape().isEqual (expectedShape),
                 String::format("Bad visibility shape; expected %s, got %s; "
                                "at spw=%d, msRow=%d",
                                expectedShape.toString().c_str(),
                                visibility.shape().toString().c_str(),
                                spectralWindow,
                                rowIds (0)));

    TestErrorIf (! visibilityCorrected.shape().isEqual (expectedShape),
                 String::format("Bad corrected visibility shape; expected %s, got %s; "
                                "at spw=%d, msRow=%d",
                                expectedShape.toString().c_str(),
                                visibilityCorrected.shape().toString().c_str(),
                                spectralWindow,
                                rowIds (0)));

    TestErrorIf (! visibilityModel.shape().isEqual (expectedShape),
                 String::format("Bad model visibility shape; expected %s, got %s; "
                                "at spw=%d, msRow=%d",
                                expectedShape.toString().c_str(),
                                visibilityModel.shape().toString().c_str(),
                                spectralWindow,
                                rowIds (0)));

    TestErrorIf (! vb->weightSpectrum ().shape().isEqual (expectedShape),
                 String::format("Bad weight spectrum shape; expected %s, got %s; "
                         "at spw=%d, msRow=%d",
                         expectedShape.toString().c_str(),
                         vb->weightSpectrum ().shape().toString().c_str(),
                         spectralWindow,
                         rowIds (0)));


    // Test flag cube shapes

    IPosition expectedShape2 (IPosition (4, 4, nChannels, nFlagCategories_p, nRows));
    TestErrorIf (! flagCategories.shape ().isEqual (expectedShape2),
                 String::format("Bad flag category shape; expected %s, got %s; "
                                "spw=%d, msRow=%d",
                                flagCategories.shape().toString().c_str(),
                                expectedShape2.toString().c_str(),
                                spectralWindow,
                                rowIds (0)))

    checkRowScalars (vb);

    for (Int row = 0; row < nRows; row ++){

        const Vector<Int> & channels = vb->getChannelNumbers(row);

        for (Int channel = 0; channel < nChannels; channel ++){

            for (Int correlation = 0; correlation < 4; correlation ++){

                // Test to see if the default fill pattern for the three types of
                // visibility cubes.

                checkVisCube (rowIds (row), spectralWindow, row, channel, correlation,
                                     visibility, "", channelOffset, channelIncrement, 0);

                checkVisCube (rowIds (row), spectralWindow, row, channel, correlation,
                                     visibilityCorrected, "corrected", channelOffset, channelIncrement, 1);

                checkVisCube (rowIds (row), spectralWindow, row, channel, correlation,
                                     visibilityModel, "model", channelOffset, channelIncrement, 2);

                checkFlagCube (rowIds (row), spectralWindow, row, channel, correlation,
                               channelOffset, channelIncrement, vb);

                checkWeightSpectrum (rowIds (row), spectralWindow, row, channel, correlation,
                                     channelOffset, channelIncrement, vb);

                checkChannelAndFrequency (rowIds (row), row, channel, channelIncrement, channelOffset,
                                          spectralWindow, vb);

                // See if the getChannels method is returning the correct channel number

                Int expectedChannelNumber = channel * channelIncrement + channelOffset;

                TestErrorIf (expectedChannelNumber != channels [channel],
                             String::format ("vb->getChannels()[%d] returned %d; expected %d",
                                             channel, channels [channel], expectedChannelNumber));

                // Now check out the flag categories array [nC, nF, nCat, nR]

                Bool expected = (rowIds (row) % 2) ^ (channels [channel] % 2) ^ (correlation % 2);
                if (factor_p != 1){
                    expected = ! expected;
                }

                for (int category = 0; category < nFlagCategories_p; category ++){

                    Bool value = flagCategories (IPosition (4, correlation, channel, category, row));

                    TestErrorIf (value != expected,
                                 String::format("Expected %d, got %d for flagCategory at "
                                                "spw=%d, vbRow=%d, msRow=%d, ch=%d, corr=%d, cat=%d",
                                                expected,
                                                value,
                                                spectralWindow,
                                                row,
                                                rowIds (row),
                                                channel,
                                                correlation,
                                                category));

                    expected = ! expected;
                }
            }
        }
    }
}

void
BasicChannelSelection::checkChannelAndFrequency (Int rowId, Int row, Int channel, Int channelIncrement, Int channelOffset,
                                                 Int spectralWindow, const VisBuffer2 * vb)
{
    const Vector<Int> & channels = vb->getChannelNumbers(row);

    // See if the getChannels method is returning the correct channel number

    Int expectedChannelNumber = channel * channelIncrement + channelOffset;

    TestErrorIf (expectedChannelNumber != channels [channel],
                 String::format ("vb->getChannels()[%d] returned %d; expected %d",
                                 channel, channels [channel], expectedChannelNumber));

    // Now check the frequency lookup.  Use topo since it should return the same frequency that the
    // bin was recorded at.  The default simulated base frequency should conform is 1E9 * spectral window
    // and the increment is 1E6 * spectral window.  Accept any deviation that is less than a 1/2 of the
    // increment.

    Double frequency = vb->getFrequency(row, channel, MFrequency::TOPO);

    Double expectedFrequency = 1e9 * (spectralWindow + 1) + 1e6 * (spectralWindow + 1) * expectedChannelNumber;

    ThrowIf (abs (frequency - expectedFrequency) > 0.5e6 * (spectralWindow + 1),
             String::format ("TOPO frequency mismatch: expected %12.5e, got %12.5e at msRow=%d, ch=%d",
                             expectedFrequency, frequency, rowId, channel, channels [channel]));
}

void
BasicChannelSelection::checkVisCube (Int rowId, Int spectralWindow, Int row, Int channel, Int correlation,
                                     const Cube<Complex> & cube, const String & tag,
                                     Int channelOffset, Int channelIncrement, Int cubeDelta)
{
    Float real = 10 * rowId + spectralWindow;
    Int expectedChannelNumber = channel * channelIncrement + channelOffset;
    Float imag = 100 * (expectedChannelNumber) + correlation * 10 + cubeDelta;
    Complex z0 (real, imag);
    z0 *= factor_p;
    Complex z = cube (correlation, channel, row);

    TestErrorIf (z != z0,
                 String::format("Expected (%f,%f), got (%f,%f) for %s vis at "
                         "spw=%d, vbRow=%d, msRow=%d, ch=%d, corr=%d",
                         z0.real(), z0.imag(),
                         z.real(), z.imag(),
                         tag.c_str(),
                         spectralWindow,
                         row,
                         rowId,
                         channel,
                         correlation));

}

void
BasicChannelSelection::checkFlagCube (Int rowId, Int spectralWindow, Int row, Int channel, Int correlation,
                                      Int channelOffset, Int channelIncrement, VisBuffer2 * vb)
{
    Int expectedChannelNumber = channel * channelIncrement + channelOffset;

    Bool expectedValue = (expectedChannelNumber % 2 == 0) == (correlation % 2 == 0);
    if (factor_p != 1){
        expectedValue = ! expectedValue;
    }

    Bool value = vb->flagCube ()(correlation, channel, row);

    TestErrorIf (expectedValue != value,
                 String::format("Expected %d, got %d for flag cube at "
                         "spw=%d, vbRow=%d, msRow=%d, ch=%d, corr=%d",
                         expectedValue,
                         value,
                         spectralWindow,
                         row,
                         rowId,
                         channel,
                         correlation));
}

void
BasicChannelSelection::checkWeightSpectrum (Int rowId, Int spectralWindow, Int row, Int channel,
                                            Int correlation, Int channelOffset, Int channelIncrement,
                                            const VisBuffer2 * vb)
{
    Int expectedChannelNumber = channel * channelIncrement + channelOffset;

    Float expectedValue = rowId * 1000 + spectralWindow * 100 + expectedChannelNumber * 10 + correlation;
    expectedValue *= factor_p;

    Float actualValue = vb->weightSpectrum() (correlation, channel, row);

    TestErrorIf (expectedValue != actualValue,
                 String::format ("Expected %f for weight spectrum (%d,%d,%d); got %f "
                                 "spw=%d, msRow=%d",
                                 expectedValue,
                                 correlation,
                                 channel,
                                 row,
                                 actualValue,
                                 spectralWindow,
                                 rowId));
}

Bool
BasicChannelSelection::noMoreData (ROVisibilityIterator2 & /*vi*/, VisBuffer2 * /*vb*/, int nRowsProcessed)
{
    TestErrorIf (nRowsProcessed != 360,
                 String::format ("Expected to process 360 rows, but did %d instead.",
                                 nRowsProcessed));

    nSweeps_p ++;

    return nSweeps_p < 2;
}

void
BasicChannelSelection::startOfData (ROVisibilityIterator2 & vi, VisBuffer2 * /*vb*/)
{
    // Apply channel selections

    FrequencySelectionUsingChannels selection;
    selection.add (0, 0, 5);
    selection.add (1, 6, 5);
    selection.add (2, 3, 6);
    selection.add (3, 0, 5, 2);

    vi.setFrequencySelection (selection);
}

BasicMutation::BasicMutation ()
: BasicChannelSelection (),
  firstPass_p (True)
{
}

BasicMutation::~BasicMutation ()
{
}

boost::tuple <MeasurementSet *, Int, Bool>
BasicMutation::createMs ()
{
    MeasurementSet * ms;
    Int nRows;
    Bool toss;

    boost::tie (ms, nRows, toss) = BasicChannelSelection::createMs ();

    return boost::make_tuple (ms, nRows, True);
}

class LogicalNot {
public:

    Bool operator() (Bool b) { return ! b;}
};


void
BasicMutation::nextSubchunk (ROVisibilityIterator2 & vi, VisBuffer2 * vb)
{
    // Perform the standard value check

    BasicChannelSelection::nextSubchunk (vi, vb);

    // On the first pass through, toggle the data: (x -> - x and b -> ~b)

    if (firstPass_p){

        Cube<Complex> cube;

        cube = vb->visCube();
        cube = - cube;
        vb->setVisCube (cube);

        cube = vb->visCubeCorrected();
        cube = - cube;
        vb->setVisCubeCorrected (cube);

        cube = vb->visCubeModel();
        cube = - cube;
        vb->setVisCubeModel (cube);

        Cube<Float> cubeF;

        cubeF = vb->weightSpectrum ();
        cubeF = - cubeF;
        vb->setWeightSpectrum (cubeF);

        Cube<Bool> cubeB;

        cubeB = vb->flagCube();
        cubeB = arrayTransformResult (cubeB, LogicalNot());
        vb->setFlagCube (cubeB);

        Array<Bool> flagCategory = vb->flagCategory();
        flagCategory = arrayTransformResult (flagCategory, LogicalNot());
        vb->setFlagCategory (flagCategory);

        Vector<Float> v;

        v = vb->weight();
        v = - v;
        vb->setWeight (v);

        v = vb->sigma();
        v = - v;
        vb->setSigma (v);

        Vector<Bool> vB;
        vB = vb->flagRow();
        vB = arrayTransformResult (vB, LogicalNot());
        vb->setFlagRow (vB);

        vb->writeChangesBack();
    }
}

Bool
BasicMutation::noMoreData (ROVisibilityIterator2 & /*vi*/, VisBuffer2 * /*vb*/, int nRows)
{
    Bool moreSweeps = firstPass_p;
    firstPass_p = False;
    setFactor (-1);

    return moreSweeps;
}


void
FrequencyChannelSelection::startOfData (ROVisibilityIterator2 & vi, VisBuffer2 * /*vb*/)
{
    // Apply channel selections

    FrequencySelectionUsingFrame selection (MFrequency::TOPO);

    selection.add (0, 1e9, 1.0043e9);
    selection.add (1, 2.012E9, 2.0203e9);
    selection.add (2, 3.009e9, 3.024e9);

    vi.setFrequencySelection (selection);
}

Bool
FrequencyChannelSelection::noMoreData (ROVisibilityIterator2 & /*vi*/, VisBuffer2 * /*vb*/, int nRowsProcessed)
{
    TestErrorIf (nRowsProcessed != 270,
                 String::format ("Expected to process 270 rows, but did %d instead.",
                                 nRowsProcessed));

    return False;
}



////////////////  Template for new TestWidget Subclasses //////////////////

//TheWidget::TheWidget () : TestWidget ("TheWidget") {}

/*pair<MeasurementSet * ms, Int>
TheWidget::createMs ()
{
}*/


/*void
TheWidget::endOfChunk (ROVisibilityIterator2 & vi, VisBuffer2 * vb)
{
}*/

/* void
TheWidget::nextChunk (ROVisibilityIterator2 & vi, VisBuffer2 * vb)
{
}*/

/* void
TheWidget::nextSubchunk (ROVisibilityIterator2 & vi, VisBuffer2 * vb)
{
}*/

/* void
TheWidget::noMoreData (ROVisibilityIterator2 & vi, VisBuffer2 * vb, int nRows)
{
}*/

/* void
TheWidget::startOfData (ROVisibilityIterator2 & vi, VisBuffer2 * vb)
{
}*/

PerformanceComparator::PerformanceComparator (const String & ms)
: ms_p (ms)
{}

void
PerformanceComparator::compare ()
{
    printf ("\n========== Performance Comparison Begin =========\n");

    MeasurementSet ms (ms_p);

    printf ("--- MeasurementSet: %s\n", ms_p.c_str());

    Block<int> sortColumns;

    ROVisibilityIterator oldVi (ms, sortColumns, True);

    ROVisibilityIterator2 newVi (ms, sortColumns);

    printf ("\n--- Default channel selection ---\n");

    //////compareOne (& oldVi, & newVi, 11);

    printf ("\n--- First channel selection ---\n");

    Block< Vector<Int> > groupSize (1, Vector<Int> (1, 1));
    Block< Vector<Int> > spectralWindow (1, Vector<Int> (1, 0));
    Block< Vector<Int> > start (1, Vector<Int> (1, 0));
    Block< Vector<Int> > increment (1, Vector<Int> (1, 1));
    Block< Vector<Int> > count (1, Vector<Int> (1, 1));

    oldVi.selectChannel (groupSize, start, count, increment, spectralWindow);

    FrequencySelections fs;
    FrequencySelectionUsingChannels fsuc;
    fsuc.add (0, 0, 1);
    fs.add (fsuc);

    newVi.setFrequencySelection (fs);

    compareOne (& oldVi, & newVi, 1);

    printf ("\n========== Performance Comparison Complete =========\n");

}


void
PerformanceComparator::compareOne (ROVisibilityIterator * oldVi,
                                   ROVisibilityIterator2 * newVi,
                                   int nSweeps)
{
    using namespace utilj;

    IoStatistics oldIo1;
    ThreadTimes oldTime1 = ThreadTimes::getTime ();

    try {

        for (int i = 0; i < nSweeps; i++){
            Double d = sweepViOld (* oldVi);
            printf ("... old sweep %d completed. (%f)\n", i, d);
        }
    }
    catch (AipsError & e){
        printf ("*** Caught exception while sweeping oldVi: %s\n", e.what());
        throw;
    }

    ThreadTimes oldTime2 = ThreadTimes::getTime ();
    IoStatistics oldIo2;

    IoStatistics newIo1;
    ThreadTimes newTime1 = ThreadTimes::getTime ();

    try{
        for (int i = 0; i < nSweeps; i++){
            Double d = sweepViNew (* newVi);
            printf ("... new sweep %d completed. (%f)\n", i, d);
        }
    }
    catch (AipsError & e){
        printf ("*** Caught exception while sweeping newVi: %s\n", e.what());
        throw;
    }

    ThreadTimes newTime2 = ThreadTimes::getTime ();
    IoStatistics newIo2;

    IoStatistics oldIo = oldIo2 - oldIo1;
    IoStatistics newIo = newIo2 - newIo1;
    DeltaThreadTimes oldDt = oldTime2 - oldTime1;
    DeltaThreadTimes newDt = newTime2 - newTime1;

    printf ("--- Stats ---  nSweeps=%d\n\n", nSweeps);
    printf ("Old VI: %s; %s\n", oldIo.report ().c_str(), oldDt.formatStats().c_str());
    printf ("New VI: %s; %s\n", newIo.report ().c_str(), newDt.formatStats().c_str());
    IoStatistics ratioIo = newIo / oldIo;
    printf ("old/new: %s ; elapsed: %6.2f %% cpu: %6.2f %%\n", ratioIo.report (100, "%").c_str(),
            newDt.elapsed () / oldDt.elapsed() * 100,
            newDt.cpu() / oldDt.cpu() * 100);

}

Double
PerformanceComparator::sweepViNew (ROVisibilityIterator2 & vi)
{
    VisBuffer2 * vb = vi.getVisBuffer();
    Double sum = 0;

    for (vi.originChunks (); vi.moreChunks(); vi.nextChunk()){

        for (vi.origin(); vi.more(); vi++){

            Int nRows = vb->nRows();
            Int nChannels = vb->nChannels();
            Int nCorrelations = vb->nCorrelations();

            for (Int row = 0; row < nRows; row ++){
                for (Int channel = 0; channel < nChannels; channel ++){
                    for (Int correlation = 0; correlation < nCorrelations; correlation ++){
                        Complex c = vb->visCube ()(correlation, channel, row);
                        c = c * c;
                        sum += c.real();
                    }
                }
            }
        }
    }

    return sum;
}

Double
PerformanceComparator::sweepViOld (ROVisibilityIterator & vi)
{
    VisBuffer vb (vi);
    Double sum = 0;

    for (vi.originChunks (); vi.moreChunks(); vi.nextChunk()){

        for (vi.origin(); vi.more(); vi++){

            Int nRows = vb.nRow();
            Int nChannels = vb.nChannel();
            Int nCorrelations = vb.nCorr();

            for (Int row = 0; row < nRows; row ++){
                for (Int channel = 0; channel < nChannels; channel ++){
                    for (Int correlation = 0; correlation < nCorrelations; correlation ++){
                        Complex c = vb.visCube ()(correlation, channel, row);
                        c = c * c;
                        sum += c.real();
                    }
                }
            }
        }
    }

    return sum;
}

} // end namespace casa
} // end namespace vi
} // end namespace test

