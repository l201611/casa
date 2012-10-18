//# Copyright (C) 2005
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

#include <display/QtPlotter/QtPlotSettings.h>
#include <display/QtPlotter/conversion/Converter.h>
#include <assert.h>
#include <QtGui>
namespace casa { 


QtPlotSettings::QtPlotSettings()
{
	const int TICK_MIN = 0;
	const int TICK_MAX = 10;
	const int TICK_COUNT = 5;
	for ( int i = 0; i < QtPlotSettings::END_AXIS_INDEX; i++ ){
		AxisIndex axisIndex = static_cast<AxisIndex>(i);
		minX[axisIndex] = TICK_MIN;
		maxX[axisIndex] = TICK_MAX;
	}
	numXTicks = TICK_COUNT;
    minY = TICK_MIN;
    maxY = TICK_MAX;
    numYTicks = TICK_COUNT;
}

void QtPlotSettings::scroll(int dx, int dy){
	for ( int i = 0; i < QtPlotSettings::END_AXIS_INDEX; i++ ){
		AxisIndex axisIndex = static_cast<AxisIndex>(i);
		double stepX = spanX( axisIndex ) / numXTicks/*[i]*/;
		minX[i] += dx * stepX;
		maxX[i] += dx * stepX;
		if ( axisIndex == QtPlotSettings::xBottom ){
			originalMinX = minX[i];
			originalMaxX = maxX[i];
		}
	}

    double stepY = spanY() / numYTicks;
    minY += dy * stepY;
    maxY += dy * stepY;
}

pair<double,double> QtPlotSettings::getZoomOutY( double zoomFactor ) const {
	double prevSpanY = spanY();
	double minY = this->minY - zoomFactor * prevSpanY;
	double maxY = this->maxY + zoomFactor * prevSpanY;
	pair<double,double> percentageSpan( minY, maxY );
	return percentageSpan;
}

void QtPlotSettings::zoomOut( double zoomFactor, const QString& topUnits, const QString& bottomUnits ){
	for ( int i = 0; i < END_AXIS_INDEX; i++ ){
		AxisIndex axisIndex = static_cast<AxisIndex>(i);
		double prevSpanX = spanX(axisIndex);
		minX[i] = minX[i] - zoomFactor * prevSpanX;
		maxX[i] = maxX[i] + zoomFactor * prevSpanX;
		if ( axisIndex == QtPlotSettings::xBottom ){
			originalMinX = minX[i];
			originalMaxX = maxX[i];
		}
	}
	adjust( topUnits, bottomUnits );
}

pair<double,double> QtPlotSettings::getZoomInY( double zoomFactor ) const {
	double prevSpanY = spanY();
	double minY = this->minY + zoomFactor * prevSpanY;
	double maxY = this->maxY - zoomFactor * prevSpanY;
	pair<double,double> percentageSpan( minY,maxY);
	return percentageSpan;
}

void QtPlotSettings::zoomY( double minY, double maxY ){
	this->minY = minY;
	this->maxY = maxY;
	adjustAxis( minY, maxY, numYTicks );
}

void QtPlotSettings::zoomIn( double zoomFactor, const QString& topUnits, const QString& bottomUnits ){
	for ( int i = 0; i < END_AXIS_INDEX; i++ ){
		AxisIndex axisIndex = static_cast<AxisIndex>(i);
		double prevSpanX = spanX( axisIndex );
		minX[i] = minX[i] + zoomFactor * prevSpanX;
		maxX[i] = maxX[i] - zoomFactor * prevSpanX;
		if ( axisIndex == QtPlotSettings::xBottom ){
			originalMinX = minX[i];
			originalMaxX = maxX[i];
		}
	}
	adjust( topUnits, bottomUnits );
}



void QtPlotSettings::adjust( const QString& topUnits, const QString& bottomUnits){
	//Adjust the bottom axis allowing it to set the number of ticks.
	adjustAxis( minX[0], maxX[0], numXTicks);

	//Adjust the top axis using the same number of ticks.  Use a
	//converter to get its min and max based on the min and max of
	//the bottom axis.
	adjustAxisTop( minX[1], maxX[1], topUnits, bottomUnits);

	//Now adjust the y-axis
    adjustAxis(minY, maxY, numYTicks);
}

void QtPlotSettings::adjustAxis(double &min, double &max,
                                   int &numTicks ){
    const int MinTicks = 4;
    double grossStep = fabs(max - min) / MinTicks;
    double step = std::pow(10, floor(log10(grossStep)));
    if (5 * step < grossStep){
        step *= 5;
    }
    else if (2 * step < grossStep){
        step *= 2;
    }
    numTicks = (int)fabs(ceil(max / step) - floor(min / step));
    double newMin = floor(min / step) * step;
    double newMax = ceil(max / step) * step;
    minPercentage = (min - newMin) / (max - min);
    maxPercentage = (newMax - max) / (max - min );
    min = newMin;
    max = newMax;
}



void QtPlotSettings::adjustAxisTop(double &min, double &max,
		const QString& topUnits, const QString& bottomUnits)
{
	Converter* converter = Converter::getConverter( bottomUnits, topUnits);
	//Top axis is not channels
	if ( topUnits != "" ){
		min = converter->convert( minX[QtPlotSettings::xBottom] );
		max = converter->convert( maxX[QtPlotSettings::xBottom] );
	}
	//Top axis is channels, but bottom axis is not
	else if ( topUnits == "" && bottomUnits != "" ){
		//Run fake values through adjust so we have accurate percentages
		double fakeMin = originalMinX;
		double fakeMax = originalMaxX;
		int fakeTicks = 0;
		adjustAxis( fakeMin, fakeMax, fakeTicks );
		double newMin = min - minPercentage * (max - min);
		double newMax = max + maxPercentage * (max - min);
		min = newMin;
		max = newMax;
	}
	//Both axis are using channels
	else {
		min = minX[QtPlotSettings::xBottom];
		max = maxX[QtPlotSettings::xBottom];
	}
	delete converter;
	if ( bottomUnits == ""){
		minPercentage = 0;
		maxPercentage = 0;
	}
}

void QtPlotSettings::setMinX( AxisIndex index, double value ){
 	minX[static_cast<int>(index)] = value;
 	if ( index == QtPlotSettings::xBottom ){
 		originalMinX = value;
 	}
}

void QtPlotSettings::setMaxX( AxisIndex index, double value ){
     maxX[static_cast<int>(index)] = value;
     if ( index == QtPlotSettings::xBottom ){
    	 originalMaxX = value;
     }
}

void QtPlotSettings::setMinY( double value ){
     minY = value;
}

void QtPlotSettings::setMaxY( double value ){
  	maxY = value;
}



}

