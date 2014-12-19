//# Copyright (C) 2000,2001
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

#ifndef SCIMATH_HINGESFENCESSTATISTICS_H
#define SCIMATH_HINGESFENCESSTATISTICS_H

#include <casa/aips.h>

#include <scimath/Mathematics/ClassicalStatistics.h>

#include <set>
#include <vector>
#include <utility>

namespace casa {

// Class to calculate statistics using the so-called hinges and fences algorithm. In this
// algorithm, the data on which the statistics are computed from is limited to the range
// of values between Q1 - f*D and Q3 + f*D, inclusive, where D = Q3 - Q1 and Q1 and Q3 are
// the first and third quartiles, respectively.

template <class AccumType, class InputIterator, class MaskIterator=const Bool*> class HingesFencesStatistics
	: public ClassicalStatistics<AccumType, InputIterator, MaskIterator> {
public:

	// If <src>f</src> is negative, the full dataset is used; ie the object has the same
	// behavior as a ClassicalStatistics object
	HingesFencesStatistics(Double f=-1.0);

	virtual ~HingesFencesStatistics();

	// copy semantics
	HingesFencesStatistics<AccumType, InputIterator, MaskIterator>& operator=(
		const HingesFencesStatistics<AccumType, InputIterator, MaskIterator>& other
	);

	// <group>
	// In the following group of methods, if the size of the composite dataset
	// is smaller than
	// <src>binningThreshholdSizeBytes</src>, the composite dataset
	// will be (perhaps partially) sorted and persisted in memory during the
	// call. In that case, and if <src>persistSortedArray</src> is True, this
	// sorted array will remain in memory after the call and will be used on
	// subsequent calls of this method when <src>binningThreshholdSizeBytes</src>
	// is greater than the size of the composite dataset. If
	// <src>persistSortedArray</src> is False, the sorted array will not be
	// stored after this call completes and so any subsequent calls for which the
	// dataset size is less than <src>binningThreshholdSizeBytes</src>, the
	// dataset will be sorted from scratch. Values which are not included due to
	// non-unity strides, are not included in any specified ranges, are masked,
	// or have associated weights of zero are not considered as dataset members
	// for quantile computations.
	// If one has a priori information regarding
	// the number of points (npts) and/or the minimum and maximum values of the data
	// set, these can be supplied to improve performance. Note however, that if these
	// values are not correct, the resulting median
	// and/or quantile values will also not be correct (although see the following notes regarding
	// max/min). Note that if this object has already had getStatistics()
	// called, and the min and max were calculated, there is no need to pass these values in
	// as they have been stored internally and used (although passing them in shouldn't hurt
	// anything). If provided, npts, the number of points falling in the specified ranges which are
	// not masked and have weights > 0, should be exactly correct. <src>min</src> can be less than
	// the true minimum, and <src>max</src> can be greater than the True maximum, but for best
	// performance, these should be as close to the actual min and max as possible.
	// In order for quantile computations to occur over multiple datasets, all datasets
	// must be available. This means that if setCalculateAsAdded()
	// was previously called by passing in a value of True, these methods will throw
	// an exception as the previous call indicates that there is no guarantee that
	// all datasets will be available. If one uses a data provider (by having called
	// setDataProvider()), then this should not be an issue.

	// get the median of the distribution.
	// For a dataset with an odd number of good points, the median is just the value
	// at index int(N/2) in the equivalent sorted dataset, where N is the number of points.
	// For a dataset with an even number of points, the median is the mean of the values at
	// indices int(N/2)-1 and int(N/2) in the sorted dataset.

	AccumType getMedian(
		CountedPtr<uInt64> knownNpts=NULL, CountedPtr<AccumType> knownMin=NULL,
		CountedPtr<AccumType> knownMax=NULL, uInt binningThreshholdSizeBytes=4096*4096,
		Bool persistSortedArray=False
	);

	// If one needs to compute both the median and quantile values, it is better to call
	// getMedianAndQuantiles() rather than getMedian() and getQuantiles() seperately, as the
	// first will scan large data sets fewer times than calling the seperate methods.
	// The return value is the median; the quantiles are returned in the <src>quantileToValue</src> map.
	AccumType getMedianAndQuantiles(
		std::map<Double, AccumType>& quantileToValue, const std::set<Double>& quantiles,
		CountedPtr<uInt64> knownNpts=NULL, CountedPtr<AccumType> knownMin=NULL,
		CountedPtr<AccumType> knownMax=NULL,
		uInt binningThreshholdSizeBytes=4096*4096, Bool persistSortedArray=False
	);

	// get the median of the absolute deviation about the median of the data.
	AccumType getMedianAbsDevMed(
		CountedPtr<uInt64> knownNpts=NULL,
		CountedPtr<AccumType> knownMin=NULL, CountedPtr<AccumType> knownMax=NULL,
		uInt binningThreshholdSizeBytes=4096*4096, Bool persistSortedArray=False
	);

	// Get the specified quantiles. <src>quantiles</src> must be between 0 and 1,
	// noninclusive.
	std::map<Double, AccumType> getQuantiles(
		const std::set<Double>& quantiles, CountedPtr<uInt64> knownNpts=NULL,
		CountedPtr<AccumType> knownMin=NULL, CountedPtr<AccumType> knownMax=NULL,
		uInt binningThreshholdSizeBytes=4096*4096, Bool persistSortedArray=False
	);
	// </group>

	// scan the dataset(s) that have been added, and find the min and max.
	// This method may be called even if setStatsToCaclulate has been called and
	// MAX and MIN has been excluded. If setCalculateAsAdded(True) has previously been
	// called after this object has been (re)initialized, an exception will be thrown.
	void getMinMax(AccumType& mymin, AccumType& mymax);

	// scan the dataset(s) that have been added, and find the number of good points.
	// This method may be called even if setStatsToCaclulate has been called and
	// NPTS has been excluded. If setCalculateAsAdded(True) has previously been
	// called after this object has been (re)initialized, an exception will be thrown.
	uInt64 getNPts();

	// see base class description
	std::pair<uInt, uInt> getStatisticIndex(StatisticsData::STATS stat);

	// reset object to initial state. Clears all private fields including data,
	// accumulators, global range. It does not affect the fence factor (_f), which was
	// set at object construction.
	virtual void reset();

	// This class does not allow statistics to be calculated as datasets are added, so
	// an exception will be thrown if <src>c</src> is True.
	void setCalculateAsAdded(Bool c);

protected:
	// <group>
	// scan through the data set to determine the number of good (unmasked, weight > 0,
	// within range) points. The first with no mask, no
	// ranges, and no weights is trivial with npts = nr in this class, but is implemented here
	// so that derived classes may override it.
	inline void _accumNpts(
		uInt64& npts,
		const InputIterator& dataStart, Int64 nr, uInt dataStride
	) const;

	void _accumNpts(
		uInt64& npts,
		const InputIterator& dataStart, Int64 nr, uInt dataStride,
		const DataRanges& ranges, Bool isInclude
	) const;

	void _accumNpts(
		uInt64& npts,
		const InputIterator& dataBegin, Int64 nr, uInt dataStride,
		const MaskIterator& maskBegin, uInt maskStride
	) const;

	void _accumNpts(
		uInt64& npts,
		const InputIterator& dataBegin, Int64 nr, uInt dataStride,
		const MaskIterator& maskBegin, uInt maskStride, const DataRanges& ranges,
		Bool isInclude
	) const;

	void _accumNpts(
		uInt64& npts,
		const InputIterator& dataBegin, const InputIterator& weightsBegin,
		Int64 nr, uInt dataStride
	) const;

	void _accumNpts(
		uInt64& npts,
		const InputIterator& dataBegin, const InputIterator& weightsBegin,
		Int64 nr, uInt dataStride, const DataRanges& ranges, Bool isInclude
	) const;

	void _accumNpts(
		uInt64& npts,
		const InputIterator& dataBegin, const InputIterator& weightsBegin,
		Int64 nr, uInt dataStride, const MaskIterator& maskBegin, uInt maskStride,
		const DataRanges& ranges, Bool isInclude
	) const;

	void _accumNpts(
		uInt64& npts,
		const InputIterator& dataBegin, const InputIterator& weightBegin,
		Int64 nr, uInt dataStride, const MaskIterator& maskBegin, uInt maskStride
	) const;
	// </group>

	// <group>
	virtual void _findBins(
		vector<uInt64>& binCounts,
		CountedPtr<AccumType>& sameVal, Bool& allSame,
		const InputIterator& dataStart, Int64 nr, uInt dataStride,
		const typename ClassicalStatistics<AccumType, InputIterator, MaskIterator>::BinDesc& binDesc, AccumType maxLimit
	) const ;

	virtual void _findBins(
		vector<uInt64>& binCounts,
		CountedPtr<AccumType>& sameVal, Bool& allSame,
		const InputIterator& dataStart, Int64 nr, uInt dataStride,
		const DataRanges& ranges, Bool isInclude,
		const typename ClassicalStatistics<AccumType, InputIterator, MaskIterator>::BinDesc& binDesc, AccumType maxLimit
	) const;

	virtual void _findBins(
		vector<uInt64>& binCounts,
		CountedPtr<AccumType>& sameVal, Bool& allSame,
		const InputIterator& dataBegin, Int64 nr, uInt dataStride,
		const MaskIterator& maskBegin, uInt maskStride,
		const typename ClassicalStatistics<AccumType, InputIterator, MaskIterator>::BinDesc& binDesc, AccumType maxLimit
	) const;

	virtual void _findBins(
		vector<uInt64>& binCounts,
		CountedPtr<AccumType>& sameVal, Bool& allSame,
		const InputIterator& dataBegin, Int64 nr, uInt dataStride,
		const MaskIterator& maskBegin, uInt maskStride, const DataRanges& ranges,
		Bool isInclude,
		const typename ClassicalStatistics<AccumType, InputIterator, MaskIterator>::BinDesc& binDesc, AccumType maxLimit
	) const;

	virtual void _findBins(
		vector<uInt64>& binCounts,
		CountedPtr<AccumType>& sameVal, Bool& allSame,
		const InputIterator& dataBegin, const InputIterator& weightsBegin,
		Int64 nr, uInt dataStride,
		const typename ClassicalStatistics<AccumType, InputIterator, MaskIterator>::BinDesc& binDesc, AccumType maxLimit
	) const ;

	virtual void _findBins(
		vector<uInt64>& binCounts,
		CountedPtr<AccumType>& sameVal, Bool& allSame,
		const InputIterator& dataBegin, const InputIterator& weightsBegin,
		Int64 nr, uInt dataStride, const DataRanges& ranges, Bool isInclude,
		const typename ClassicalStatistics<AccumType, InputIterator, MaskIterator>::BinDesc& binDesc, AccumType maxLimit
	) const;

	virtual void _findBins(
		vector<uInt64>& binCounts,
		CountedPtr<AccumType>& sameVal, Bool& allSame,
		const InputIterator& dataBegin, const InputIterator& weightsBegin,
		Int64 nr, uInt dataStride, const MaskIterator& maskBegin, uInt maskStride,
		const DataRanges& ranges, Bool isInclude,
		const typename ClassicalStatistics<AccumType, InputIterator, MaskIterator>::BinDesc& binDesc, AccumType maxLimit
	) const;

	virtual void _findBins(
		vector<uInt64>& binCounts,
		CountedPtr<AccumType>& sameVal, Bool& allSame,
		const InputIterator& dataBegin, const InputIterator& weightBegin,
		Int64 nr, uInt dataStride, const MaskIterator& maskBegin, uInt maskStride,
		const typename ClassicalStatistics<AccumType, InputIterator, MaskIterator>::BinDesc& binDesc, AccumType maxLimit
	) const;
	// </group>

	AccumType _getStatistic(StatisticsData::STATS stat);

	Record _getStatistics();

	// <group>
	virtual void _minMax(
		CountedPtr<AccumType>& mymin, CountedPtr<AccumType>& mymax,
		const InputIterator& dataBegin, Int64 nr, uInt dataStride
	) const;

	virtual void _minMax(
		CountedPtr<AccumType>& mymin, CountedPtr<AccumType>& mymax,
		const InputIterator& dataBegin, Int64 nr, uInt dataStride,
		const DataRanges& ranges, Bool isInclude
	) const;

	virtual void _minMax(
		CountedPtr<AccumType>& mymin, CountedPtr<AccumType>& mymax,
		const InputIterator& dataBegin, Int64 nr, uInt dataStride,
		const MaskIterator& maskBegin, uInt maskStride
	) const;

	virtual void _minMax(
		CountedPtr<AccumType>& mymin, CountedPtr<AccumType>& mymax,
		const InputIterator& dataBegin, Int64 nr, uInt dataStride,
		const MaskIterator& maskBegin, uInt maskStride, const DataRanges& ranges,
		Bool isInclude
	) const;

	virtual void _minMax(
		CountedPtr<AccumType>& mymin, CountedPtr<AccumType>& mymax,
		const InputIterator& dataBegin, const InputIterator& weightsBegin,
		Int64 nr, uInt dataStride
	) const;

	virtual void _minMax(
		CountedPtr<AccumType>& mymin, CountedPtr<AccumType>& mymax,
		const InputIterator& dataBegin, const InputIterator& weightsBegin,
		Int64 nr, uInt dataStride, const DataRanges& ranges, Bool isInclude
	) const;

	virtual void _minMax(
		CountedPtr<AccumType>& mymin, CountedPtr<AccumType>& mymax,
		const InputIterator& dataBegin, const InputIterator& weightsBegin,
		Int64 nr, uInt dataStride, const MaskIterator& maskBegin, uInt maskStride,
		const DataRanges& ranges, Bool isInclude
	) const;

	virtual void _minMax(
		CountedPtr<AccumType>& mymin, CountedPtr<AccumType>& mymax,
		const InputIterator& dataBegin, const InputIterator& weightBegin,
		Int64 nr, uInt dataStride, const MaskIterator& maskBegin, uInt maskStride
	) const;
	// </group>

	//<group>
	// populate an unsorted array with valid data. If <src>includeLimits</src> is defined,
	// then restrict values that are entered in the array to those limits (inclusive of the
	// minimum, exclusive of the maximum). <src>maxCount</src> and <src>currentCount</src> are
	// used only if <src>includeLimits</src> is defined. In this case, the method will return
	// when currentCount == maxCount, thus avoiding scanning remaining data unnecessarily.

	// no weights, no mask, no ranges
	void _populateArray(
		vector<AccumType>& ary, uInt& currentCount, const InputIterator& dataBegin, Int64 nr, uInt dataStride,
		const std::pair<AccumType, AccumType> *const &includeLimits, uInt maxCount
	) const;

	// ranges
	void _populateArray(
		vector<AccumType>& ary, uInt& currentCount, const InputIterator& dataBegin, Int64 nr,
		uInt dataStride, const DataRanges& ranges, Bool isInclude,
		const std::pair<AccumType, AccumType> *const &includeLimits, uInt maxCount
	) const;

	void _populateArray(
		vector<AccumType>& ary, uInt& currentCount, const InputIterator& dataBegin,
		Int64 nr, uInt dataStride, const MaskIterator& maskBegin,
		uInt maskStride,
		const std::pair<AccumType, AccumType> *const &includeLimits, uInt maxCount
	) const;

	// mask and ranges
	void _populateArray(
		vector<AccumType>& ary, uInt& currentCount, const InputIterator& dataBegin, Int64 nr,
		uInt dataStride, const MaskIterator& maskBegin, uInt maskStride,
		const DataRanges& ranges, Bool isInclude,
		const std::pair<AccumType, AccumType> *const &includeLimits, uInt maxCount
	) const;

	// weights
	void _populateArray(
		vector<AccumType>& ary, uInt& currentCount, const InputIterator& dataBegin,
		const InputIterator& weightsBegin, Int64 nr, uInt dataStride,
		const std::pair<AccumType, AccumType> *const &includeLimits, uInt maxCount
	) const;

	// weights and ranges
	void _populateArray(
		vector<AccumType>& ary, uInt& currentCount, const InputIterator& dataBegin,
		const InputIterator& weightsBegin, Int64 nr, uInt dataStride,
		const DataRanges& ranges, Bool isInclude,
		const std::pair<AccumType, AccumType> *const &includeLimits, uInt maxCount
	) const;

	// weights and mask
	void _populateArray(
		vector<AccumType>& ary, uInt& currentCount, const InputIterator& dataBegin,
		const InputIterator& weightBegin, Int64 nr, uInt dataStride,
		const MaskIterator& maskBegin, uInt maskStride,
		const std::pair<AccumType, AccumType> *const &includeLimits, uInt maxCount
	) const;

	// weights, mask, ranges
	void _populateArray(
		vector<AccumType>& ary, uInt& currentCount, const InputIterator& dataBegin, const InputIterator& weightBegin,
		Int64 nr, uInt dataStride, const MaskIterator& maskBegin, uInt maskStride,
		const DataRanges& ranges, Bool isInclude,
		const std::pair<AccumType, AccumType> *const &includeLimits, uInt maxCount
	) const;
	// </group>

	// <group>
		// no weights, no mask, no ranges
		Bool _populateTestArray(
			vector<AccumType>& ary, const InputIterator& dataBegin,
			Int64 nr, uInt dataStride, uInt maxElements
		) const;

		// ranges
		Bool _populateTestArray(
			vector<AccumType>& ary, const InputIterator& dataBegin, Int64 nr,
			uInt dataStride, const DataRanges& ranges, Bool isInclude,
			uInt maxElements
		) const;

		// mask
		Bool _populateTestArray(
			vector<AccumType>& ary, const InputIterator& dataBegin,
			Int64 nr, uInt dataStride, const MaskIterator& maskBegin,
			uInt maskStride, uInt maxElements
		) const;

		// mask and ranges
		Bool _populateTestArray(
			vector<AccumType>& ary, const InputIterator& dataBegin, Int64 nr,
			uInt dataStride, const MaskIterator& maskBegin, uInt maskStride,
			const DataRanges& ranges, Bool isInclude, uInt maxElements
		) const;

		// weights
		Bool _populateTestArray(
			vector<AccumType>& ary, const InputIterator& dataBegin,
			const InputIterator& weightBegin, Int64 nr, uInt dataStride,
			uInt maxElements
		) const;

		// weights and ranges
		Bool _populateTestArray(
			vector<AccumType>& ary, const InputIterator& dataBegin,
			const InputIterator& weightsBegin, Int64 nr, uInt dataStride,
			const DataRanges& ranges, Bool isInclude, uInt maxElements
		) const;

		// weights and mask
		Bool _populateTestArray(
			vector<AccumType>& ary, const InputIterator& dataBegin,
			const InputIterator& weightBegin, Int64 nr,
			uInt dataStride, const MaskIterator& maskBegin,
			uInt maskStride, uInt maxElements
		) const;

		// weights, mask, ranges
		Bool _populateTestArray(
			vector<AccumType>& ary, const InputIterator& dataBegin, const InputIterator& weightBegin,
			Int64 nr, uInt dataStride, const MaskIterator& maskBegin, uInt maskStride,
			const DataRanges& ranges, Bool isInclude,
			uInt maxElements
		) const;
		// </group>

		// <group>
		// no weights, no mask, no ranges
		void _unweightedStats(
			uInt64& ngood, AccumType& mymin, AccumType& mymax,
			Int64& minpos, Int64& maxpos,
			const InputIterator& dataBegin, Int64 nr, uInt dataStride
		);

		// no weights, no mask
		void _unweightedStats(
			uInt64& ngood, AccumType& mymin, AccumType& mymax,
			Int64& minpos, Int64& maxpos,
			const InputIterator& dataBegin, Int64 nr, uInt dataStride,
			const DataRanges& ranges, Bool isInclude
		);

		void _unweightedStats(
			uInt64& ngood, AccumType& mymin, AccumType& mymax,
			Int64& minpos, Int64& maxpos,
			const InputIterator& dataBegin, Int64 nr, uInt dataStride,
			const MaskIterator& maskBegin, uInt maskStride
		);

		void _unweightedStats(
			uInt64& ngood, AccumType& mymin, AccumType& mymax,
			Int64& minpos, Int64& maxpos,
			const InputIterator& dataBegin, Int64 nr, uInt dataStride,
			const MaskIterator& maskBegin, uInt maskStride,
			const DataRanges& ranges, Bool isInclude
		);
		// </group>

		// <group>
		// has weights, but no mask, no ranges
		void _weightedStats(
			AccumType& mymin, AccumType& mymax,
			Int64& minpos, Int64& maxpos,
			const InputIterator& dataBegin, const InputIterator& weightsBegin,
			Int64 nr, uInt dataStride
		);

		void _weightedStats(
			AccumType& mymin, AccumType& mymax,
			Int64& minpos, Int64& maxpos,
			const InputIterator& dataBegin, const InputIterator& weightsBegin,
			Int64 nr, uInt dataStride, const DataRanges& ranges, Bool isInclude
		);

		void _weightedStats(
			AccumType& mymin, AccumType& mymax,
			Int64& minpos, Int64& maxpos,
			const InputIterator& dataBegin, const InputIterator& weightBegin,
			Int64 nr, uInt dataStride, const MaskIterator& maskBegin, uInt maskStride
		);

		void _weightedStats(
			AccumType& mymin, AccumType& mymax,
			Int64& minpos, Int64& maxpos,
			const InputIterator& dataBegin, const InputIterator& weightBegin,
			Int64 nr, uInt dataStride, const MaskIterator& maskBegin, uInt maskStride,
			const DataRanges& ranges, Bool isInclude
		);
		// </group>

private:

	// _f defined in inclusion range between Q1 - _f*D and Q3 + _f*D, where D = Q3 - Q1 and
	// Q1 and Q3 are the first and third quartiles, respectively
	Double _f;
	Bool _doMedAbsDevMed, _rangeIsSet, _hasRange, _settingRange;
	CountedPtr<std::pair<AccumType, AccumType> > _range;
	CountedPtr<AccumType> _median;

	inline Bool _isInRange(const AccumType& datum) const;

	void _setRange();


};

}

#ifndef CASACORE_NO_AUTO_TEMPLATES
#include <scimath/Mathematics/HingesFencesStatistics.tcc>
#endif //# CASACORE_NO_AUTO_TEMPLATES

#endif
