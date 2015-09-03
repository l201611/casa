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
#ifndef SPECFITSETTINGSWIDGETRADIO_QO_H
#define SPECFITSETTINGSWIDGETRADIO_QO_H

#include <QtGui/QWidget>
#include <QProgressDialog>
#include <display/QtPlotter/SpecFitSettingsWidgetRadio.ui.h>
#include <display/QtPlotter/ProfileTaskFacilitator.h>
#include <casa/Containers/Record.h>
#include <coordinates/Coordinates/CoordinateSystem.h>

namespace casa {

class ImageProfileFitter;
class SpectralList;
class SpectralElement;
class SpecFit;
class SpecFitThread;

class SpecFitSettingsWidgetRadio : public QWidget, public ProfileTaskFacilitator
{
    Q_OBJECT

public:
    SpecFitSettingsWidgetRadio(QWidget *parent = 0);
    ~SpecFitSettingsWidgetRadio();
    void setUnits( QString units );
    void setRange(float start, float end );
    void reset();
    void pixelsChanged( int pixX, int pixY );

private slots:
	void polyFitChanged( int state );
	void adjustTableRowCount( int count );
	void clean();
	void specLineFit();
	void setOutputLogFile();
	void viewOutputLogFile();
	void saveOutputChanged( int state );
	void specFitEstimateSpecified(double xValue,double yValue, bool centerPeak);
	void fitDone();
	void cancelFit();

private:
	/**
	 * Places the passed in Gaussian estimate val into the indicated row and
	 * column of the initial Gaussian estimate table.
	 */
	void setEstimateValue( int row, int col, double val );
	bool isValidEstimate( QString& peakStr, QString& centerStr,
			QString& fwhmStr, QString& fixedStr, int rowIndex );
	SpectralList buildSpectralList( int nGauss, Bool& validList );
	bool isValidFitSpecification( int gaussCount, bool polyFit );
	void setCanvas( QtCanvas* canvas );
	void doFit( float startVal, float endVal, uint gaussCount, bool fitPoly, int polyN );
	String getChannels( float startVal, float endVal, const Vector<Float>& specValues ) const;
	void getConversion( const String& unitStr, Bool& velocity, Bool& wavelength ) const;
	void clearEstimates();
	void clear();
	void resolveOutputLogFile( );
	void processFitResults(Vector<float>& xValues, Vector<float>& xValuesPix);
	bool processFitResultGaussian( const SpectralElement* solution,
			int index, QList<SpecFit*>& curves);
	bool processFitResultPolynomial( const SpectralElement* solution,
				QList<SpecFit*>& curves);
	double toPixels( double val) const;
	void drawCurves( int pixelX, int pixelY );

	enum TableHeaders {PEAK,CENTER,FWHM,FIXED,END_COLUMN};
    Ui::SpecFitSettingsWidgetRadio ui;
    ImageProfileFitter* fitter;
    SpecFitThread* specFitThread;
    QProgressDialog progressDialog;
    QString outputLogPath;
    QList<QList<SpecFit*> > curveList;
    int POINT_COUNT;
    int SUM_FIT_INDEX;

    volatile bool fitCancelled;
};
}
#endif // SPECFITSETTINGSWIDGETRADIO_H