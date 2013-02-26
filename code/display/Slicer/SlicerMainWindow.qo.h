//# Copyright (C) 1994,1995,1996,1997,1998,1999,2000
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

#ifndef SLICER_MAINWINDOW_QO_H
#define SLICER_MAINWINDOW_QO_H

#include <QtGui/QMainWindow>
#include <display/Slicer/SlicerMainWindow.ui.h>
#include <display/Slicer/SlicePlot.qo.h>

class QCursor;

namespace casa {

class SliceZoomer;
class SliceColorPreferences;
class StatisticsRegion;

class SlicerMainWindow : public QMainWindow {

    Q_OBJECT

public:
    SlicerMainWindow(QWidget *parent = 0);
    void updateChannel( int channel );
    void updatePolyLine(  int regionId, viewer::region::RegionChanges regionChanges,
		const QList<double> & worldX, const QList<double> & worldY,
		const QList<int> &pixelX, const QList<int> & pixelY );
    void addPolyLine(  int regionId, viewer::region::RegionChanges regionChanges,
    		const QList<double> & worldX, const QList<double> & worldY,
    		const QList<int> &pixelX, const QList<int> & pixelY, const QString& colorName );
    void setImage( ImageInterface<float>* img );
    void setCurveColor( int id, const QString& color );
    ~SlicerMainWindow();

private slots:
	void clearCurves();
	void autoCountChanged( bool selected );
	void sampleCountChanged();
	void interpolationMethodChanged( const QString& method );
	void accumulateSlicesChanged(bool accumulate);
	void exportSlice();
	void showColorDialog();
	void resetColors();
	void zoomIn();
	void zoomNeutral();
	void zoomOut();
	bool checkZoom();

private:
	void initializeZooming();
	SlicerMainWindow( const SlicerMainWindow& mainWindow );
	SlicerMainWindow& operator=( const SlicerMainWindow&  other);

	int populateSampleCount() const;

	//Persistence
	bool toImageFormat( const QString& fileName, const QString& format );
	bool toASCII( const QString& fileName );

	//Statistics
	void updateStatisticsLayout();

	SliceColorPreferences* colorPreferences;
    SliceZoomer* plotZoomer;

    SlicePlot slicePlot;

    QStringList methodList;
    QStringList xAxisList;

    Ui::SlicerMainWindowClass ui;

};
}

#endif // SLICERMAINWINDOW_QO_H
