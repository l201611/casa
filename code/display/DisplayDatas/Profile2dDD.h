//# Profile2dDD.h: 2D Profile DisplayData
//# Copyright (C) 2003,2004
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
//# $Id$

#ifndef TRIALDISPLAY_PROFILE2DDD_H
#define TRIALDISPLAY_PROFILE2DDD_H

#include <casa/aips.h>
#include <casa/Arrays/Array.h>
#include <casa/Arrays/Vector.h>
#include <casa/Arrays/Matrix.h>
#include <display/Display/DParameterColorChoice.h>
#include <display/Display/DParameterFontChoice.h>
#include <display/Display/DParameterMapKeyChoice.h>
#include <display/Display/DParameterRange.h>
#include <display/Display/DParameterSwitch.h>
#include <display/DisplayDatas/ActiveCaching2dDD.h>
#include <display/DisplayCanvas/WCCSNLAxisLabeller.h>
#include <display/DisplayEvents/DisplayEH.h>
#include <lattices/Lattices/LatticeStatsBase.h>

namespace casa { //# NAMESPACE CASA - BEGIN

	template <class T> class LatticePADisplayData;
	template <class T> class DParameterRange;

	class WorldCanvas;
	class Profile2dDM;
	class DParameterColorChoice;
	class DParameterMapKeyChoice;
	class DParameterSwitch;

// <summary>
// A DisplayData to draw Profiles
// </summary>
//
// <use visibility=export>
//
// <reviewed reviewer="" date="yyyy/mm/dd" tests="" demos="">
// </reviewed>
//
// <prerequisite>
//   <li> DisplayData
//   <li> CachingDisplayData
//   <li> ActiveCaching2dDD
// </prerequisite>
//
// <etymology> </etymology>

// <synopsis> This Display Data attaches to itself, another Display
// Data whos profile (at a point) is to be extracted and drawn.  The
// Display Data attached to Profile2dDD must have atleast 3 world Axes
// and atleast 2 pixels on it's profile axis (3rd axis).  An axis from
// a Linear Coordinate or Stokes Coordinate presently can not be on
// the profile axis.  All other AIPS++ coordinate types are supported.
//
// Profile2dDD is an implements WCMotionEH and WCPositionEH and
// listens to motion and position events generated by the attached
// Display Data. A motion event comes with a new world position on
// the attached Display Data. Profile2dDD uses this world position to
// extract and draw a profile on its world canvas. A Position event
// (key press) switches the profiling on and off. The default switch
// is the space bar.
//
// Profile2dDD is a DisplayEH (all DisplayDatas are) and listens to
// DisplayEvents sent out by the attached DisplayData. This is so it
// can listen for Tool events such as Crosshair event.
//
// Each time the profile is refreshed (with new data), Profile2dDD
// sends out a DDModEvent to all listening DisplayEHs, to indicate
// that the data has been modified.
//
// Since Profile2dDD inherit's from ActiveCaching2dDD and uses
// WCCSNLAxisLabeller, all the options such as position tracking and
// axis labelling are available.  Options specific to Profilng, such
// as profile color, line width, line style and autoscaling are also
// available. </synopsis>
//
// <example>
// </example>
//
// <motivation>
// Existing Glish implementation is too slow and limited.
// </motivation>
//
// <todo>
// <li> Mixed axes sometimes result in slow and messy axis labelling
// <li> Update Profile2dDD in real time when axis of attached dd
// changes
// <li> Option to start Profile from a specific channel.
// <li> Add support for Display Datas with profile of a linear
// coordinate axis.
// <li> Flux and integrated flux calculations
// <li> Ability to add multiple display datas and overlay their
// profiles

// </todo>

	class Profile2dDD : public ActiveCaching2dDD, public WCMotionEH,
		public WCPositionEH {

	public:
		// <group>
		// (Required) default constructor.
		Profile2dDD();

		// Constructor taking a pointer to an already constructed
		// ImageInterface.The Display Data dd must have atleast 3 World Axes
		// and atleast 2 elements in the profile (3rd) world axis.
		Profile2dDD(LatticePADisplayData<Float>* dd);

		// Destructor.
		virtual ~Profile2dDD();
		// </group>

		// <group>
		// Attach a Display Data to this Profile2dDD. The Display Data dd
		// must have atleast 3 World Axes and atleast 2 elements in the
		// profile (3rd) world axis. If any of the above conditions are not
		// met or a Display Data is already attached, False is
		// returned. Otherwise True is returned.
		virtual Bool attachDD(LatticePADisplayData<Float>* dd);

		// Detach the currently attached Display Data.
		virtual void detachDD();
		// </group>

		// Overloading ActiveCaching2dDD::sizeControl. Zooming is modified
		// for autoscaling feature.
		virtual Bool sizeControl(WorldCanvasHolder &wcHolder,
		                         AttributeBuffer &holderBuf);

		// <group>
		// Store the data to be drawn in the data Matrix. If world is false,
		// pixel coordinates are used. If world is true, then world
		// coordinates are used.
		virtual void getDrawData(Matrix<Double> &data,
		                         const Bool world=False);

		// Store the mask in mask vector
		virtual void getMaskData(Vector<Bool> &mask);

		// Store the details of the current profile in rec
		// Record structure similar to position event structure.
		virtual void getProfileAsRecord(Record &rec);
		// </group>


		// Return the data unit.
		virtual const Unit dataUnit();

		// Returns an empty string.
		virtual String showValue(const Vector<Double> &world);

		// Motion Event Handler
		virtual void operator()(const WCMotionEvent &ev);

		// Position Event Handler
		virtual void operator()(const WCPositionEvent &ev);

		// Display Event Handler
		virtual void handleEvent(DisplayEvent &ev);

		// Send out DDModEvents to all DisplayEHs listening
		virtual void sendDDModEvent();

		// Draws and labels the axes based on the refresh event
		virtual Bool labelAxes(const WCRefreshEvent &ev);

		// <group>
		// Install the default options for this DisplayData.
		virtual void setDefaultOptions();

		// Apply options stored in <src>rec</src> to the DisplayData. A
		// return value of <src>True</src> means a refresh is needed.
		// <src>recOut</src> contains any fields which were implicitly
		// changed as a result of the call to this function.
		virtual Bool setOptions(Record &rec, Record &recOut);

		// Retrieve the current and default options and parameter types.
		virtual Record getOptions();
		// </group>

		// Return the type of this DisplayData.
		virtual Display::DisplayDataType classType() {
			return Display::Vector;
		}

		// Create a new CachingDisplayMethod for drawing on the given
		// WorldCanvas when the AttributeBuffers are suitably matched to the
		// current state of this DisplayData and of the WorldCanvas/Holder.
		// The tag is a unique number used to identify the age of the newly
		// constructed CachingDisplayMethod.
		virtual CachingDisplayMethod *newDisplayMethod(WorldCanvas *worldCanvas,
		        AttributeBuffer *wchAttributes,
		        AttributeBuffer *ddAttributes,
		        CachingDisplayData *dd);

		// Return the current options of this DisplayData as an
		// AttributeBuffer.
		virtual AttributeBuffer optionsAsAttributes();

		// Take actions on removal from WC[H] (notably, deletion of drawlists).
		virtual void notifyUnregister(WorldCanvasHolder& wcHolder,
		                              Bool ignoreRefresh = False);

		// <group>
		// Return Profile Color
		virtual const String profileColor() const {
			return itsParamColor->value();
		}
		// Return Profile Line Width
		virtual const Float profileLineWidth() const {
			return itsParamLineWidth->value();
		}
		// Return Profile LineStyle
		virtual const Display::LineStyle profileLineStyle() {
			return static_cast<Display::LineStyle>(itsParamLineStyle->keyValue());
		}

		// Return True if the last requested profile was for a
		// region. Return False if the last requested profile was for a
		// single point
		virtual const Bool isRegionProfile() const {
			return itsIsRegionProfile;
		}

		// get the region dimensions, in pixels, of the last region
		// submitted to Profile2dDD.
		virtual void regionDimensions(Vector<Double> &regionBlc,
		                              Vector<Double> &regionTrc);

		// Return Minimum Y value
		virtual const Double profileYMin() const {
			return itsCurrentBlc(1);
		}
		// Return Maximum Y value
		virtual const Double profileYMax() const {
			return itsCurrentTrc(1);
		}
		// Return Minimum X value
		virtual const Double profileXMin() const {
			return itsCurrentBlc(0);
		}
		// Return Maximum X value
		virtual const Double profileXMax() const {
			return itsCurrentTrc(0);
		}
		// Return the autoscale status (On or Off)
		virtual const Bool profileAutoscale() const {
			return itsParamAutoscale->value();
		}
		// Return the rest frequency display status (to draw or not to draw)
		virtual const Bool showRestFrequency()  const {
			return itsParamShowRestFrequency->value();
		}
		// Return rest frequency.
		virtual const Double restFrequency()  const {
			return itsRestFrequency;
		}

		// Return the statistics used for region calculations.
		virtual const LatticeStatsBase::StatisticsTypes  regionStatType()  const {
			return static_cast<LatticeStatsBase::StatisticsTypes>
			       (itsParamRegionStatType->keyValue());
		}

		// Return the x value added to the pixel at a pixel position to
		// create a region
		virtual const Int regionXRadius()  const {
			return itsParamRegionXRadius->value();
		}

		// Return the y value added to the pixel at a pixel position to
		// create a region
		virtual const Int regionYRadius()  const {
			return itsParamRegionYRadius->value();
		}
		// Return the profile axis number (from the original image)
		virtual const Int profileAxis();

		// </group>

	protected:

		friend class Profile2dDM;

		// (Required) copy constructor.
		Profile2dDD(const Profile2dDD &other);

		// (Required) copy assignment.
		void operator=(const Profile2dDD &other);

	private:

		// Helper function. Initialise Profile2dDD with a CoordinateSystem
		// put together from the parent DD
		Bool createCoordinateSystem();

		// Update the coordinate system of this Display Data and the axis
		// labeller. Set new minimum and maximum Y values if necessary
		Bool updateCoordinateSys(CoordinateSystem &cs);

		// Extract the profile data from the provided pixel region and place
		// it into itsData. The statistics used is determined by the
		// options. Return True if new profile data has been extracted
		// (i.e. a refresh is needed) otherwise return False
		Bool getRegionProfile(Vector<Double> &fpixelBlc,
		                      Vector<Double> &fpixelTrc);

		// Extract the profile data from the provided world position and
		// place it into itsData. Also extract the Mask data (if it exists)
		// and place it into itsMask. Return True if new profile data has
		// been extracted (i.e. a refresh is needed) otherwise return False
		Bool getPointProfile(const Vector<Double> &world);

		// Crop the region so that it does not define areas outside the
		// data. Returns False if the entire region is outside image data
		Bool cropRegion(Vector<Double> &fpixelBlc,
		                Vector<Double> &fpixelTrc);

		// Construct the parameters for getOptions and setOptions
		virtual void constructParameters();
		// Delete the parameters for getOptions() and setOptions()
		virtual void destructParameters();

		// A pointer to the attached DisplayData
		LatticePADisplayData<Float>* itsDD;

		// Flag to indicate whether the last drawn profile was for a region
		// or for a single point.
		Bool itsIsRegionProfile;

		// A flag to indicate whether tracking is on. If True, a new profile
		// is extracted each time a new motion event is received.
		Bool itsTrackingState;

		// The dependent (world) axis of itsDD. For example, if a RA axis is
		// the profile axis, then the DEC axis will be the dependent axis.
		Int itsDependentAxis;

		// Increment value for the linear coordinate on the Y axis
		Double itsYAxisInc;

		// Rest Frequency
		Double itsRestFrequency;

		// Minimum and maximum X/Y values
		Vector<Double> itsCurrentBlc;
		Vector<Double> itsCurrentTrc;

		// <group> The pixel position on the world canvas when the last
		// motion' event was received. This position is in the format of
		// itsDD, NOT the original image.
		Vector<Double> itsPixelPosition;
		// The world position on the world canvas when the last motion'
		// event was received. This position is in the format of
		// itsDD, NOT the original image.
		Vector<Double> itsWorldPosition;

		// The region dimensions, in pixels, of the last region event
		// received by Profile2dDD
		Vector<Double> itsRegionBlc;
		Vector<Double> itsRegionTrc;

		// </group>
		// The Profile Data
		Matrix<Double> itsData;
		// The Mask Data
		Vector<Bool> itsMask;

		// The axis map between input DD's Coordinate system and
		// itsCompleteCS
		Vector<Int> itsWorldAxisMap;
		Vector<Int> itsPixelAxisMap;

		// The choices for region calculations. eg, mean, median, etc
		Vector<String> itsRegionCalcChoices;

		// The default x and y axis labels. The x and y labels change to
		// indicate region profiles.
		Vector<String> itsDefaultAxisLabels;

		// The Axis Labeller used to draw Axes
		WCCSNLAxisLabeller itsAxisLabeller;

		// Display Parameters for
		DParameterColorChoice* itsParamColor;
		DParameterRange<Float>* itsParamLineWidth;
		DParameterMapKeyChoice* itsParamLineStyle;
		DParameterSwitch* itsParamAutoscale;
		DParameterSwitch* itsParamShowRestFrequency;
		DParameterMapKeyChoice* itsParamRegionStatType;
		DParameterRange<Int>* itsParamRegionXRadius;
		DParameterRange<Int>* itsParamRegionYRadius;
	};

} //# NAMESPACE CASA - END

#endif
