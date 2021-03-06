//# t_subImage.cc: Test program for class _subImage
//# Copyright (C) 1998,1999,2000,2001,2003
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
//# $Id: $

#include <imageanalysis/ImageAnalysis/ImageStatsCalculator.h>

#include <casa/BasicSL/String.h>
#include <images/Images/ImageUtilities.h>
#include <imageanalysis/ImageAnalysis/SubImageFactory.h>

#include <iomanip>

using namespace casacore;
namespace casa {

const String ImageStatsCalculator::_class = "ImageStatsCalculator";

const String ImageStatsCalculator::SIGMA = "sigma";

ImageStatsCalculator::ImageStatsCalculator(
    const SPCIIF image,
    const Record *const &regionPtr,
    const String& maskInp,
    Bool beVerboseDuringConstruction
) : ImageStatsConfigurator(
        image, regionPtr, maskInp
    ), _oldStatsRegion(0), _oldStatsMask(0),
    _axes(), _includepix(), _excludepix(), _list(false),
    _disk(false), _robust(false), _verbose(false),
    _subImage() {
    _construct(beVerboseDuringConstruction);
    // _algConf.algorithm = StatisticsData::CLASSICAL;
}

ImageStatsCalculator::~ImageStatsCalculator() {}

Record ImageStatsCalculator::calculate() {
    *_getLog() << LogOrigin(_class, __func__);
    std::unique_ptr<vector<String> > messageStore( _getLogFile() ? new vector<String>() : nullptr );
    Record retval = statistics(messageStore.get());
    Bool writeFile = _openLogfile();
    if (_verbose || writeFile) {
        if (writeFile) {
            for (
                vector<String>::const_iterator iter=messageStore->begin();
                iter != messageStore->end(); ++iter
            ) {
                _writeLogfile("# " + *iter, false, false);
            }
        }
        IPosition shape = _axes.empty() ? IPosition(_subImage->ndim(), 1)
            : _subImage->shape();
        for (const auto& axis: _axes) {
            shape[axis] = 1;
        }
        Record r;
        auto csys = _subImage->coordinates();
        csys.save(r, "");
        try {
            SPIIF tempIm = ImageFactory::floatImageFromShape("", shape.asVector(), r);
            _reportDetailedStats(tempIm, retval);
        }
        catch (const AipsError& x) {
            *_getLog() << LogIO::WARN << "Unable to collapse image "
                << "so detailed per plane statistics reporting is not "
                << "possible. The exception message was " << x.getMesg()
                << LogIO::POST;
        }
    }
    return retval;
}

void ImageStatsCalculator::setVerbose(Bool v) {
    if (_verbose != v) {
        _resetStats();
    }
    _verbose = v;
}

void ImageStatsCalculator::setDisk(Bool d) {
    if (_disk != d) {
        _resetStats();
    }
    _disk = d;
}

void ImageStatsCalculator::_reportDetailedStats(
    const SPCIIF tempIm, const Record& retval
) {
    const CoordinateSystem& csys = tempIm->coordinates();
    auto worldAxes = csys.worldAxisNames();
    auto imShape = tempIm->shape();
    vector<uInt> colwidth;
    Int stokesCol = -1;
    Int freqCol = -1;
    Int raCol = -1;
    Int decCol = -1;
    IPosition otherCol;
    for (Int i=worldAxes.size()-1; i>=0; i--) {
        auto gg = worldAxes[i];
        gg.upcase();
        if (gg == "RIGHT ASCENSION") {
            raCol = i;
        }
        else if (gg == "DECLINATION") {
            decCol = i;
        }
        else if (gg == "FREQUENCY") {
            freqCol = i;
        }
        else if (gg == "STOKES") {
            stokesCol = i;
        }
        else {
            otherCol.append(IPosition(1, i));
        }
    }
    IPosition idx(worldAxes.size(), 0);
    uInt myloc = 0;
    IPosition reportAxes;
    if (stokesCol >= 0) {
        idx[myloc] = stokesCol;
        if (imShape[stokesCol] > 1) {
            reportAxes.prepend(IPosition(1, stokesCol));
        }
        ++myloc;
    }
    if (freqCol >= 0) {
        idx[myloc] = freqCol;
        if (imShape[freqCol] > 1) {
            reportAxes.prepend(IPosition(1, freqCol));
        }
        ++myloc;
    }
    if (decCol >= 0) {
        idx[myloc] = decCol;
        if (imShape[decCol] > 1) {
            reportAxes.prepend(IPosition(1, decCol));
        }
        ++myloc;
    }
    if (raCol >= 0) {
        idx[myloc] = raCol;
        if (imShape[raCol] > 1) {
            reportAxes.prepend(IPosition(1, raCol));
        }
        myloc++;
    }
    if (otherCol.size() > 0) {
        for (uInt i=0; i<otherCol.nelements(); ++i) {
            idx[myloc] = otherCol[i];
            ++myloc;
            if (imShape[otherCol[i]] > 1) {
                reportAxes.append(IPosition(1, otherCol[i]));
            }
        }
    }
    Bool doVelocity = csys.hasSpectralAxis()
        && csys.spectralCoordinate().restFrequency() > 0;
    ostringstream oss;
    // CSSC wants "#" in log file but not in logger output, sigh
    for (auto ax : reportAxes) {
        if (ax == freqCol) {
            if (doVelocity) {
                oss << "VELOCITY column unit = "
                    << csys.spectralCoordinate().velocityUnit() << endl;
            }
            else {
                oss << "FREQUENCY column unit = "
                    << csys.spectralCoordinate().worldAxisUnits()[0] << endl;
            }
            if (_verbose) {
                *_getLog() << LogIO::NORMAL << oss.str() << LogIO::POST;
            }
            if (_getLogFile()) {
                _writeLogfile("#" + oss.str(), false, false);
            }
            oss.str("");
        }
    }
    auto bUnit = _getImage()->units().getName();
    if (_verbose) {
        oss.str("");
        oss << "Sum column unit = " << bUnit << endl;
        oss << "Mean column unit = " << bUnit << endl;
        oss << "Std_dev column unit = " << bUnit << endl;
        oss << "Minimum column unit = " << bUnit << endl;
        oss << "Maximum column unit = " << bUnit << endl;
        *_getLog() << LogIO::NORMAL << oss.str() << LogIO::POST;
    }
    if (_getLogFile()) {
        oss.str("");
        oss << "#Sum column unit = " << bUnit << endl;
        oss << "#Mean column unit = " << bUnit << endl;
        oss << "#Std_dev column unit = " << bUnit << endl;
        oss << "#Minimum column unit = " << bUnit << endl;
        oss << "#Maximum column unit = " << bUnit << endl;
        _writeLogfile(oss.str(), false, false);
        oss.str("");
    }
    for (auto ax : reportAxes) {
        String gg = worldAxes[ax];
        gg.upcase();
        uInt width = gg == "STOKES" ? 6 : gg == "FREQUENCY"?  16: 15;
        if (
            gg == "FREQUENCY" && doVelocity
        ) {
            gg = "VELOCITY";
        }
        colwidth.push_back(width);
        oss << setw(width) << gg << "  "
            << gg << "(Plane)" << " ";
        width = gg.size() + 8;
        colwidth.push_back(width);
    }
    Vector<Int> axesMap = reportAxes.asVector();
    GenSort<Int>::sort(axesMap);
    oss << "Npts          Sum           Mean          Rms           Std_dev       Minimum       Maximum     ";
    std::map<String, uInt> chauvIters;
    const auto alg = _getAlgConf().algorithm;
    const auto& stats = _getImageStats();
    if (alg == StatisticsData::CHAUVENETCRITERION) {
        chauvIters = stats->getChauvenetNiter();
        oss << "  N Iter";
    }
    oss << endl;
    if (_verbose) {
        *_getLog() << LogIO::NORMAL << oss.str() << LogIO::POST;
    }
    if (_getLogFile()) {
        _writeLogfile("#" + oss.str(), false, false);
    }
    oss.str("");
    for (uInt i=0; i<7; ++i) {
        colwidth.push_back(12);
    }
    if (alg == StatisticsData::CHAUVENETCRITERION) {
        colwidth.push_back(6);
    }
    TileStepper ts(
        tempIm->niceCursorShape(),
        IPosition(tempIm->ndim(), 1), idx
    );
    RO_MaskedLatticeIterator<Float> inIter(
        *tempIm, ts
    );
    Vector<Double> world;
    IPosition arrayIndex(axesMap.nelements(), 0);
    IPosition blc = stats->getBlc();
    IPosition position(tempIm->ndim());
    uInt width = 13;
    Vector<Vector<String> > coords(reportAxes.size());
    auto i = 0;
    for (const auto& axis: reportAxes) {
        Vector<Double> indices = indgen(imShape[axis], 0.0, 1.0);
        uInt prec = axis == freqCol ? 9 : 5;
        if (doVelocity && reportAxes[i] == freqCol) {
            const SpectralCoordinate& spc = csys.spectralCoordinate();
            Vector<Double> vels;
            spc.pixelToVelocity(vels, indices);
            vector<String> sv;
            for (const auto& v : vels) {
                ostringstream oss;
                oss << setprecision(prec) << v;
                sv.push_back(oss.str());
            }
            coords[i] = Vector<String>(sv);
        }
        else {
            ImageUtilities::pixToWorld(
                coords[i], csys, axis, _axes,
                IPosition(imShape.size(),0), imShape-1, indices, prec,
                true
            );
        }
        ++i;
    }
    uInt count = 0;
    for (inIter.reset(); ! inIter.atEnd(); ++inIter) {
        oss << std::scientific;
        uInt colNum = 0;
        position = inIter.position();
        for (uInt i=0; i<reportAxes.nelements(); ++i) {
            oss << setw(colwidth[colNum]);
            oss    << coords[i][position[reportAxes[i]]];
            ++colNum;
            oss << " " << setw(colwidth[colNum])
                << (position[reportAxes[i]] + blc[reportAxes[i]]) << " ";
            ++colNum;
        }
        csys.toWorld(world, position);
        if (axesMap.empty()) {
            arrayIndex = IPosition(1, 0);
        }
        else {
            auto n = axesMap.nelements();
            for (uInt i=0; i<n; ++i) {
                arrayIndex[i] = position[axesMap[i]];
            }
        }
        if (retval.asArrayDouble("npts").size() == 0) {
            oss << "NO VALID POINTS FOR WHICH TO DETERMINE STATISTICS" << endl;
        }
        else {
            oss << std::setw(width) << retval.asArrayDouble("npts")(arrayIndex) << " "
                << std::setw(width) << retval.asArrayDouble("sum")(arrayIndex) << " "
                << std::setw(width) << retval.asArrayDouble("mean")(arrayIndex) << " "
                << std::setw(width) << retval.asArrayDouble("rms")(arrayIndex) << " "
                << std::setw(width) << retval.asArrayDouble(SIGMA)(arrayIndex) << " "
                << std::setw(width) << retval.asArrayDouble("min")(arrayIndex) << " "
                << std::setw(width) << retval.asArrayDouble("max")(arrayIndex);
            if (alg == StatisticsData::CHAUVENETCRITERION) {
                ostringstream pos;
                pos << position;
                oss << std::setw(6) << " " << chauvIters[pos.str()];
                ++count;
            }
            oss << endl;
        }
        if (_verbose) {
            *_getLog() << LogIO::NORMAL << oss.str() << LogIO::POST;
        }
        // add a space at the beginning of the line to account for the
        // "#" in the column header
        _writeLogfile(" " + oss.str(), false, false);
        oss.str("");
    }
    _closeLogfile();
}

Record ImageStatsCalculator::statistics(
    vector<String> *const &messageStore
) {
    LogOrigin myOrigin(_class, __func__);
    *_getLog() << myOrigin;
    CountedPtr<ImageRegion> region, mask;
    String mtmp = _getMask();
    if (mtmp == "false" || mtmp == "[]") {
        mtmp = "";
    }
    _subImage = SubImageFactory<Float>::createSubImageRO(
        region, mask, *_getImage(), *_getRegion(), mtmp,
        (_verbose ? _getLog().get() : 0), AxesSpecifier(),
        _getStretch()
    );
    *_getLog() << myOrigin;
    // Find BLC of _subImage in pixels and world coords, and output the
    // information to the logger.
    // NOTE: ImageStatitics can't do this because it only gets the _subImage
    //       not a region and the full image.
    IPosition shape = _subImage->shape();
    IPosition blc(_subImage->ndim(), 0);
    IPosition trc(shape - 1);
    if (region) {
        LatticeRegion latRegion = region->toLatticeRegion(
            _getImage()->coordinates(), _getImage()->shape()
        );
        Slicer sl = latRegion.slicer();
        blc = sl.start();
        trc = sl.end();
    }
    // for precision
    CoordinateSystem csys = _getImage()->coordinates();
    Int precis = -1;
    if (csys.hasDirectionCoordinate()) {
        DirectionCoordinate dirCoord = csys.directionCoordinate();
        Vector<String> dirUnits = dirCoord.worldAxisUnits();
        Vector<Double> dirIncs = dirCoord.increment();
        for (uInt i=0; i< dirUnits.size(); ++i) {
            Quantity inc(dirIncs[i], dirUnits[i]);
            inc.convert("s");
            Int newPrecis = abs(int(floor(log10(inc.getValue()))));
            precis = (newPrecis > 2 && newPrecis > precis) ? newPrecis : precis;
        }
    }
    String blcf, trcf;
    blcf = CoordinateUtil::formatCoordinate(blc, csys, precis);
    trcf = CoordinateUtil::formatCoordinate(trc, csys, precis);
    auto& stats = _getImageStats();
    if (! stats) {
        _resetStats(
            _verbose
            ? new ImageStatistics<Float> (*_subImage, *_getLog(), true, _disk)
            : new ImageStatistics<Float> (*_subImage, true, _disk)
        );
    }
    else {
        stats->resetError();
        if (
            _haveRegionsChanged(
                region.get(), mask.get(),
                _oldStatsRegion.get(), _oldStatsMask.get()
            )
        ) {
            stats->setNewImage(*_subImage);
        }
    }
    // prevent the table of stats we no longer use from being logged
    stats->setListStats(false);
    String myAlg = _configureAlgorithm();
    if (_list) {
        *_getLog() << myOrigin << LogIO::NORMAL;
        String algInfo = "Statistics calculated using "
            + myAlg + " algorithm";
        *_getLog() << algInfo << LogIO::POST;
        if (messageStore) {
            messageStore->push_back(algInfo + "\n");
        }
        // Only write to the logger if the user wants it displayed.
        Vector<String> x(5);
        ostringstream y;
        x[0] = "Regions --- ";
        y << "         -- bottom-left corner (pixel) [blc]:  " << blc;
        x[1] = y.str();
        y.str("");
        y << "         -- top-right corner (pixel) [trc]:    " << trc;
        x[2] = y.str();
        y.str("");
        y << "         -- bottom-left corner (world) [blcf]: " << blcf;
        x[3] = y.str();
        y.str("");
        y << "         -- top-right corner (world) [trcf]:   " << trcf;
        x[4] = y.str();
        for (uInt i=0; i<x.size(); ++i) {
            *_getLog() << x[i] << LogIO::POST;
            if (messageStore != 0) {
                messageStore->push_back(x[i] + "\n");
            }
        }
    }
    if (messageStore != NULL) {
        stats->recordMessages(true);
    }
    stats->setPrecision(precis);
    stats->setBlc(blc);

    // Assign old regions to current regions
    _oldStatsMask.reset(0);

    _oldStatsRegion = region;
    _oldStatsMask = mask;
    //_oldStatsStorageForce = _disk;
    // Set cursor axes
    *_getLog() << myOrigin;
    ThrowIf(! stats->setAxes(_axes), stats->errorMessage());
    ThrowIf(
        !stats->setInExCludeRange(_includepix, _excludepix, false),
        stats->errorMessage()
    );
    // Tell what to list
    ThrowIf(
        !stats->setList(_list),
        stats->errorMessage()
    );
    // Recover statistics
    Array<Double> npts, sum, sumsquared, min, max, mean, sigma;
    Array<Double> rms, fluxDensity, med, medAbsDevMed, quartile, q1, q3;
    Bool ok = true;
    Bool doFlux = true;
    if (_getImage()->imageInfo().hasMultipleBeams()) {
        if (csys.hasSpectralAxis() || csys.hasPolarizationCoordinate()) {
            Int spAxis = csys.spectralAxisNumber();
            Int poAxis = csys.polarizationAxisNumber();
            for (Int i=0; i<(Int)_axes.size(); ++i) {
                if (_axes[i] == spAxis || _axes[i] == poAxis) {
                    *_getLog() << LogIO::WARN << "At least one cursor axis contains multiple beams. "
                        << "You should thus use care in interpreting these statistics. Flux densities "
                        << "will not be computed." << LogIO::POST;
                    doFlux = false;
                    break;
                }
            }
        }
    }
    if (_robust) {
        ok = stats->getStatistic(med, LatticeStatsBase::MEDIAN)
            && stats->getStatistic(
                medAbsDevMed, LatticeStatsBase::MEDABSDEVMED
            )
            && stats->getStatistic(
                quartile, LatticeStatsBase::QUARTILE
            )
            && stats->getStatistic(
                q1, LatticeStatsBase::Q1
            )
            && stats->getStatistic(
                q3, LatticeStatsBase::Q3
            );
    }
    if (ok) {
        ok = stats->getStatistic(npts, LatticeStatsBase::NPTS)
            && stats->getStatistic(sum, LatticeStatsBase::SUM)
            && stats->getStatistic(sumsquared, LatticeStatsBase::SUMSQ)
            && stats->getStatistic(min, LatticeStatsBase::MIN)
            && stats->getStatistic(max, LatticeStatsBase::MAX)
            && stats->getStatistic(mean, LatticeStatsBase::MEAN)
            && stats->getStatistic(sigma, LatticeStatsBase::SIGMA)
            && stats->getStatistic(rms, LatticeStatsBase::RMS);
    }
    ThrowIf(! ok, stats->errorMessage());
    Record statsout;
    statsout.define("npts", npts);
    statsout.define("sum", sum);
    statsout.define("sumsq", sumsquared);
    statsout.define("min", min);
    statsout.define("max", max);
    statsout.define("mean", mean);
    if (_robust) {
        statsout.define("median", med);
        statsout.define("medabsdevmed", medAbsDevMed);
        statsout.define("quartile", quartile);
        statsout.define("q1", q1);
        statsout.define("q3", q3);
    }
    statsout.define(SIGMA, sigma);
    statsout.define("rms", rms);
    if (
        doFlux
        && stats->getStatistic(
            fluxDensity, LatticeStatsBase::FLUX
        )
    ) {
        statsout.define("flux", fluxDensity);
    }
    statsout.define("blc", blc.asVector());
    statsout.define("blcf", blcf);
    statsout.define("trc", trc.asVector());
    statsout.define("trcf", trcf);
    String tmp;
    IPosition minPos, maxPos;
    if (stats->getMinMaxPos(minPos, maxPos)) {
        if (minPos.nelements() > 0) {
            statsout.define("minpos", (blc + minPos).asVector());
            tmp = CoordinateUtil::formatCoordinate(blc + minPos, csys, precis);
            statsout.define("minposf", tmp);
        }
        if (maxPos.nelements() > 0) {
            statsout.define("maxpos", (blc + maxPos).asVector());
            tmp = CoordinateUtil::formatCoordinate(blc + maxPos, csys, precis);
            statsout.define("maxposf", tmp);
        }
    }
    if (_list) {
        stats->showRobust(_robust);
        ThrowIf(
            ! stats->display(),
            stats->errorMessage()
        );
    }
    if (messageStore != 0) {
        vector<String> messages = stats->getMessages();
        for (
            vector<String>::const_iterator iter=messages.begin();
            iter!=messages.end(); ++iter
        ) {
            messageStore->push_back(*iter + "\n");
        }
        stats->clearMessages();
    }
    return statsout;
}

void ImageStatsCalculator::setRobust(Bool b) {
    _robust = b;
}

Bool ImageStatsCalculator::_haveRegionsChanged(
    ImageRegion* newRegion,
    ImageRegion* newMask, ImageRegion* oldRegion,
    ImageRegion* oldMask
) {
    Bool regionChanged = (
            newRegion != 0 && oldRegion != 0
            && (*newRegion) != (*oldRegion)
        )
        || (newRegion == 0 && oldRegion != 0)
        || (newRegion != 0 && oldRegion == 0
    );
    Bool maskChanged = (
            newMask != 0 && oldMask != 0
            && (*newMask) != (*oldMask)
        )
        || (newMask == 0 && oldMask != 0)
        || (newMask != 0 && oldMask == 0
    );
    return (regionChanged || maskChanged);
}

}

