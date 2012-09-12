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

#ifndef SEARCHMOLECULESWIDGET_QO_H_1
#define SEARCHMOLECULESWIDGET_QO_H_1

#include <QtGui/QWidget>
#include <QMap>
#include <QThread>
#include <QProgressDialog>
#include <display/QtPlotter/SearchMoleculesWidget.ui.h>
#include <measures/Measures/MRadialVelocity.h>
#include <measures/Measures/MDoppler.h>
#include <display/QtPlotter/conversion/Converter.h>
#include <spectrallines/Splatalogue/Searcher.h>
#include <spectrallines/Splatalogue/SearcherFactory.h>
#include <display/QtPlotter/SearchMoleculesResultDisplayer.h>

namespace casa {


/**
 * Responsible for running the search algorithm in
 * the background so that we don't freeze the GUI.
 */
class SearchThread : public QThread {
public:
	SearchThread( Searcher* searcher, int offset ){
		this->searcher= searcher;
		this->offset = offset;
	}
	String getErrorMessage() const {
		return errorMsg;
	}
	String getErrorMessageCount() const {
		return errorMsgCount;
	}

	long getResultsCount() const {
		return searchResultsCount;
	}
	vector<SplatResult> getResults() const {
		return searchResults;
	}
	void run(){
		if ( offset == 0 ){
			searchResultsCount = searcher->doSearchCount( errorMsgCount );
		}
		searchResults = searcher->doSearch( errorMsg, offset );
	}
	~SearchThread(){
	}
private:
	Searcher* searcher;
	int searchResultsCount;
	int offset;
	vector<SplatResult> searchResults;
	string errorMsg;
	string errorMsgCount;
};


class SearchMoleculesWidget : public QWidget
{
    Q_OBJECT

public:
    SearchMoleculesWidget(QWidget *parent = 0);
    QString getUnit() const;
    bool isLocal() const;
    //bool isPrevResults() const;
    //bool isNextResults() const;
    void setRange( float min, float max, QString units );
    void updateReferenceFrame();
    static void setInitialReferenceFrame( QString initialReferenceStr );
    void setResultDisplay( SearchMoleculesResultDisplayer* resultDisplay );
    double getRedShiftedValue( bool reverseRedshift, double value ) const;
    //String getDatabasePath() const;
    vector<SplatResult> getSearchResults() const;
    MDoppler::Types getDopplerType() const;
    MRadialVelocity::Types getReferenceFrame() const;
    ~SearchMoleculesWidget();
    static const QString SPLATALOGUE_UNITS;

signals:
	void searchCompleted();

private slots:
    void search();
    void searchUnitsChanged( const QString& searchUnits );
    void dopplerShiftChanged();
    void dopplerVelocityUnitsChanged();
    void searchFinished();
    void prevResults();
    void nextResults();

private:

    static QString initialReferenceStr;

    void setAstronomicalFilters( Searcher* searcher );
    void convertRangeLineEdit( QLineEdit* lineEdit, Converter* converter );
    void initializeSearchRange( QLineEdit* lineEdit, Double& value );
    void startSearchThread();

    MDoppler getRedShiftAdjustment( bool reverseRedshift) const;

    enum AstroFilters { NONE, TOP_20, PLANETARY_ATMOSPHERE,HOT_CORES,
		DARK_CLOUDS,DIFFUSE_CLOUDS,COMETS, AGB_PPN_PN,EXTRAGALACTIC };

    Ui::SearchMoleculesWidget ui;
    //String defaultDatabasePath;
    //String databasePath;
    QString unitStr;
    QString dopplerVelocityUnitStr;
    vector<SplatResult> searchResults;
    QList<QString> velocityUnitsList;
    QMap<QString, MRadialVelocity::Types> radialVelocityTypeMap;
    QMap<QString, MDoppler::Types> dopplerTypeMap;
    bool dopplerInVelocity;
    SearchThread* searchThread;
    Searcher* searcher;
    QProgressDialog progressBar;

    //Scrolling support
    int searchResultCount;
    int searchResultOffset;
    int searchResultLimit;

    static const double SPLATALOGUE_DEFAULT_MIN;
    static const double SPLATALOGUE_DEFAULT_MAX;
    SearchMoleculesResultDisplayer* resultDisplay;
};

}

#endif // SEARCHMOLECULESWIDGET_H
