//# TSMCubeBuff.cc: Tiled Hypercube Storage Manager for tables using buff
//# Copyright (C) 2009
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
//# $Id: TSMCubeBuff.cc 20874 2010-03-30 06:28:34Z gervandiepen $


//# Includes
#include <casa/aips.h>
#include <tables/Tables/TSMCubeBuff.h>
#include <tables/Tables/TiledStMan.h>
#include <tables/Tables/TSMFile.h>
#include <tables/Tables/TSMDataColumn.h>
#include <tables/Tables/DataManError.h>
#include <casa/Arrays/ArrayUtil.h>
#include <casa/Containers/Record.h>
#include <casa/Containers/RecordField.h>
#include <casa/Containers/Block.h>
#include <casa/BasicMath/Math.h>
#include <casa/IO/BucketBuffered.h>
#include <casa/IO/AipsIO.h>
#include <casa/OS/Conversion.h>
#include <casa/OS/HostInfo.h>
#include <casa/string.h>                           // for memcpy
#include <casa/iostream.h>


namespace casa { //# NAMESPACE CASA - BEGIN


TSMCubeBuff::TSMCubeBuff (TiledStMan* stman, TSMFile* file,
                          const IPosition& cubeShape,
                          const IPosition& tileShape,
                          const Record& values,
                          Int64 fileOffset,
                          uInt bufferSize)
  : TSMCube (stman, file, cubeShape, tileShape, values, fileOffset, True),
    cache_p (0),
    bufferSize_p (bufferSize)
{
  // Note that the TSMCube constructor calls setShape.
  // However, because it is in the constructor TSMCube's setShape is called.
  // Hence we have to make the cache here.
  if (fileOffset < 0  &&  nrTiles_p > 0) {
    makeCache();
  }
}

TSMCubeBuff::TSMCubeBuff (TiledStMan* stman, AipsIO& ios, uInt bufferSize)
  : TSMCube (stman, ios, True),
    cache_p (0),
    bufferSize_p (bufferSize)
{}

TSMCubeBuff::~TSMCubeBuff()
{
    delete cache_p;
}


void TSMCubeBuff::showCacheStatistics (ostream& os) const
{
  if (cache_p != 0) {
    os << ">>> No TSMCube cache statistics (uses mmap)" << endl;
    os << "<<<" << endl;
  }
}


void TSMCubeBuff::makeCache()
{
    // If there is no cache, make one.
    if (cache_p == 0) {
        cache_p = new BucketBuffered (filePtr_p->bucketFile(), fileOffset_p,
                                      bucketSize_p, nrTiles_p);
    }
}

void TSMCubeBuff::flushCache()
{
    if (cache_p != 0) {
	cache_p->flush();
    }
}

void TSMCubeBuff::resyncCache()
{
    if (cache_p != 0) {
	cache_p->resync (nrTiles_p);
    }
}

void TSMCubeBuff::deleteCache()
{
    delete cache_p;
    cache_p = 0;
}

void TSMCubeBuff::setShape (const IPosition& cubeShape,
                            const IPosition& tileShape)
{
  TSMCube::setShape (cubeShape, tileShape);
  makeCache();
}

void TSMCubeBuff::extend (uInt nr, const Record& coordValues,
                          const TSMColumn* lastCoordColumn)
{
    if (!extensible_p) {
        throw (TSMError ("Hypercube is not extensible"));
    }
    // Make the cache here, otherwise nrTiles_p is too high.
    makeCache();
    uInt lastDim = nrdim_p - 1;
    uInt nrold = nrTiles_p;
    cubeShape_p(lastDim) += nr;
    tilesPerDim_p(lastDim) = (cubeShape_p(lastDim) + tileShape_p(lastDim) - 1)
                             / tileShape_p(lastDim);
    nrTiles_p = nrTilesSubCube_p * tilesPerDim_p(lastDim);
    // Extend the cache which extends the file and remaps.
    // Note that extending TSMFile only means updating its length.
    getCache()->extend (nrTiles_p - nrold);
    filePtr_p->extend ((nrTiles_p - nrold) * bucketSize_p);
    // Update the last coordinate (if there).
    if (lastCoordColumn != 0) {
        extendCoordinates (coordValues, lastCoordColumn->columnName(),
                           cubeShape_p(lastDim));
    }
}

void TSMCubeBuff::setCacheSize (uInt, Bool, Bool)
{}

void TSMCubeBuff::setCacheSize (const IPosition&,
                                const IPosition&,
                                const IPosition&,
                                const IPosition&,
                                Bool, Bool)
{}

void TSMCubeBuff::accessSection (const IPosition& start, const IPosition& end,
                                 char* section, uInt colnr,
                                 uInt localPixelSize, uInt externalPixelSize,
                                 Bool writeFlag)
{
  // A tile can contain more than one data column.
  // Get the offset of the column's data array in the tile.
  uInt tileOffset = externalOffset_p[colnr];
  // Get convert function and nr of elements per value to convert.
  const TSMDataColumn* dataColumn = stmanPtr_p->getDataColumn(colnr);
  Conversion::ValueFunction* convertFunc =
    dataColumn->getConvertFunction (writeFlag);
  uInt nrConvElem = dataColumn->getNrConvert();
  // A Bool column is stored as bits and has to be treated differently.
  uInt dataPixelSize = externalPixelSize;
  Bool useBool = False;
  if (dataPixelSize == 0) {
    useBool = True;
    dataPixelSize = 1;
  }
  // Set flag if writing.
  if (writeFlag) {
    stmanPtr_p->setDataChanged();
  }

  // Initialize the various variables and determine the number of
  // tiles needed (which will determine the cache size).
  // Also determine if the slice happens to be an entire tile
  // or if it is a line (these cases occur quite often and can be
  // handled in a faster way).
  Bool oneEntireTile = True;
  for (uInt i=0; i<nrdim_p; i++) {
    startTile_p(i) = start(i) / tileShape_p(i);
    endTile_p(i)   = end(i) / tileShape_p(i);
    nrTileSection_p(i)         = 1 + endTile_p(i) - startTile_p(i);
    startPixelInFirstTile_p(i) = start(i) - startTile_p(i)*tileShape_p(i);
    endPixelInLastTile_p(i)    = end(i) - endTile_p(i) * tileShape_p(i);
    endPixelInFirstTile_p(i)   = tileShape_p(i) - 1;
    if (nrTileSection_p(i) == 1) {
      endPixelInFirstTile_p(i) = endPixelInLastTile_p(i);
      if (startPixelInFirstTile_p(i) != 0
      ||  endPixelInFirstTile_p(i) != tileShape_p(i) - 1) {
        oneEntireTile = False;
      }
    }else{
      oneEntireTile = False;
    }
  }
  // Get the cache.
  BucketBuffered* cachePtr = getCache();
    
//    cout << "nrTileSection_p=" << nrTileSection_p << endl;
//    cout << "startTile_p=" << startTile_p << endl;
//    cout << "endTile_p=" << endTile_p << endl;
//    cout << "startPixelInFirstTile_p" << startPixelInFirstTile_p << endl;
//    cout << "endPixelInFirstTile_p" << endPixelInFirstTile_p << endl;
//    cout << "endPixelInLastTile_p" << endPixelInLastTile_p << endl;


  // If the section matches the tile shape, we can simply
  // copy all values and do not have to do difficult iterations.
  if (oneEntireTile) {
    // Get the tile from the cache.
    uInt tileNr = expandedTilesPerDim_p.offset (startTile_p);
    // If writing, set cache slot to dirty.
    if (writeFlag) {
      convertFunc (cachePtr->getBuffer(), section, tileSize_p*nrConvElem);
      cachePtr->write (tileNr, 0, bucketSize_p);
    } else {
      cachePtr->read (tileNr, 0, bucketSize_p);
      convertFunc (section, cachePtr->getBuffer(), tileSize_p*nrConvElem);
    }
    return;
  }

  // At this point we start looping through all tiles.
  // startPixel and endPixel will contain the first and last pixels
  // needed in the current tile.
  // tilePos contains the position of the current tile.
  IPosition startSection (start);            // start of section in cube
  IPosition sectionShape (end - start + 1);  // section shape
  TSMShape expandedSectionShape (sectionShape);
  IPosition startPixel (startPixelInFirstTile_p);
  IPosition endPixel   (endPixelInFirstTile_p);
  IPosition tilePos    (startTile_p);
  IPosition tileIncr = 
    expandedTilesPerDim_p.offsetIncrement (nrTileSection_p);
  IPosition dataLength(nrdim_p);
  IPosition dataPos   (nrdim_p);
  IPosition sectionPos(nrdim_p);
  uInt dataOffset;
  size_t sectionOffset;
  uInt tileNr = expandedTilesPerDim_p.offset (tilePos);

  // Loop over all tiles.
  while (True) {
//      cout << "tilePos=" << tilePos << endl;
//      cout << "tileNr=" << tileNr << endl;
//      cout << "start=" << startPixel << endl;
//      cout << "end=" << endPixel << endl;
    // At this point we start looping through all pixels in the tile.
    // We do a vector at a time.
    // Calculate the start and end pixel in the tile.
    // Initialize the pixel position in the data and section.
    // Note that for Bools it counts external in bits.
    for (uInt i=0; i<nrdim_p; i++) {
      dataLength(i) = 1 + endPixel(i) - startPixel(i);
      dataPos(i)    = startPixel(i);
      sectionPos(i) = tilePos(i) * tileShape_p(i)
        + startPixel(i) - startSection(i);
    }
    dataOffset    = expandedTileShape_p.offset (startPixel);
    sectionOffset = localPixelSize * expandedSectionShape.offset (sectionPos);
    IPosition dataIncr    = dataPixelSize *
      expandedTileShape_p.offsetIncrement (dataLength);
    IPosition sectionIncr = localPixelSize *
      expandedSectionShape.offsetIncrement (dataLength);

    // Calculate the largest number of pixels
    // that are consequtive in data and in section
    uInt n = dataLength(0);
    uInt incrDim = 1;
    for (uInt j = 1; j < nrdim_p; j++) {
        if (dataLength(j-1) == tileShape_p(j-1)  &&
            dataLength(j-1) == sectionShape(j-1)) {

            n *= dataLength(j);
            incrDim = j+1;
        }
        else {
            break;
        }
    }

    uInt nrval     = n * nrConvElem;
    uInt localSize = n * localPixelSize;
    uInt dataSize  = n * dataPixelSize;

    // Loop through the data in the tile. Handle Bool specifically.
    // On read, it converts the data from the external to the local format.
    // On write it does the opposite.
    if (useBool) {
      while (True) {
        // Determine start and length in the bucket.
        uInt offset = tileOffset + dataOffset/8;
        uInt stBit = dataOffset % 8;             // bit to start
        uInt nBytes = (stBit+nrval+7) / 8;
        if (writeFlag) {
          // Read first and/or last byte if no full byte is used.
          if (stBit != 0) {
            cachePtr->read (tileNr, offset, 1);
          }
          if (nBytes > 1  &&  (stBit+nrval) % 8 != 0) {
            cachePtr->read (tileNr, offset+nBytes-1, 1, nBytes-1);
          }
          Conversion::boolToBit (cachePtr->getBuffer(), section+sectionOffset,
                                 stBit, nrval);
          cachePtr->write (tileNr, offset, nBytes);
        }else{
          cachePtr->read (tileNr, offset, nBytes);
          Conversion::bitToBool (section+sectionOffset, cachePtr->getBuffer(),
                                 stBit, nrval);
        }
        dataOffset    += dataSize;
        sectionOffset += localSize;
        uInt j;
        for (j=incrDim; j<nrdim_p; j++) {
          dataOffset    += dataIncr(j);
          sectionOffset += sectionIncr(j);
          if (++dataPos(j) <= endPixel(j)) {
            break;
          }
          dataPos(j) = startPixel(j);
        }
        if (j == nrdim_p) {
          break;
        }
      }
    } else {
      dataOffset *= dataPixelSize;
      dataOffset += tileOffset;
      while (True) {
        if (writeFlag) {
          convertFunc (cachePtr->getBuffer(), section+sectionOffset, nrval);
          cachePtr->write (tileNr, dataOffset, dataSize);
        } else {
          cachePtr->read (tileNr, dataOffset, dataSize);
          convertFunc (section+sectionOffset, cachePtr->getBuffer(), nrval);
        }
        dataOffset    += dataSize;
        sectionOffset += localSize;
        uInt j;
        for (j=incrDim; j<nrdim_p; j++) {
          dataOffset    += dataIncr(j);
          sectionOffset += sectionIncr(j);
          if (++dataPos(j) <= endPixel(j)) {
            break;
          }
          dataPos(j) = startPixel(j);
        }
        if (j == nrdim_p) {
          break;
        }
      }
    }

    // Determine the next tile to access and the starting and
    // ending pixels in it.
    // We increase the tile position in a dimension.
    uInt i;
    for (i=0; i<nrdim_p; i++) {
      tileNr += tileIncr(i);
      startPixel(i) = 0;
      if (++tilePos(i) < endTile_p(i)) {
        break;                                 // not at last tile
      }
      if (tilePos(i) == endTile_p(i)) {
        endPixel(i) = endPixelInLastTile_p(i);   // last tile
        break;
      }
      // Past last tile in this dimension.
      // Reset start and end.
      tilePos(i) = startTile_p(i);
      startPixel(i) = startPixelInFirstTile_p(i);
      endPixel(i)   = endPixelInFirstTile_p(i);
    }
    if (i == nrdim_p) {
      break;                                     // ready
    }
  }
}


void TSMCubeBuff::accessStrided (const IPosition& start, const IPosition& end,
                                 const IPosition& stride,
                                 char* section, uInt colnr,
                                 uInt localPixelSize, uInt externalPixelSize,
                                 Bool writeFlag)
{
  // If no strides, use accessSection.
  if (stride.allOne()) {
    accessSection (start, end, section, colnr,
                   localPixelSize, externalPixelSize, writeFlag);
    return;
  }
  // Get the data by getting the array part and stride it thereafter.
  // When writing it is the opposite.
  // Handle the arrays as chars to be type-agnostic, so add an axis for it.
  // It is less efficient than doing a proper accessStrided, but much easier.
  // Since strided access is used seldomly, the price is not too high.
  IPosition sectShape ((end - start + stride) / stride);
  IPosition fullShape (end - start + 1);
  IPosition incr(stride);
  // Add the first axis to handle as chars.
  if (localPixelSize != 1) {
    sectShape.prepend (IPosition(1, localPixelSize));
    fullShape.prepend (IPosition(1, localPixelSize));
    incr.prepend (IPosition(1,1));
  }
  IPosition fst(incr.size(), 0);
  IPosition fend(fullShape - 1);
  Array<char> fullArr(fullShape);
  Array<char> partArr = fullArr(fst, fend, incr);
  Array<char> sectArr(sectShape, section, SHARE);
  // Read the data of the full array.
  // Thereafter copy the part needed.
  accessSection (start, end, fullArr.data(), colnr,
                 localPixelSize, externalPixelSize, False);
  if (writeFlag) {
    partArr = sectArr;
    accessSection (start, end, fullArr.data(), colnr,
                   localPixelSize, externalPixelSize, True);
  } else {
    sectArr = partArr;
  }
}


} //# NAMESPACE CASA - END
