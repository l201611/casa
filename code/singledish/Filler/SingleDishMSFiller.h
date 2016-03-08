/*
 * Scantable2MSFiller.h
 *
 *  Created on: Jan 5, 2016
 *      Author: nakazato
 */

#ifndef SINGLEDISH_FILLER_SINGLEDISHMSFILLER_H_
#define SINGLEDISH_FILLER_SINGLEDISHMSFILLER_H_

#include <string>
#include <memory>
#include <map>

#include <singledish/Filler/DataAccumulator.h>
#include <singledish/Filler/SysCalRecord.h>
#include <singledish/Filler/WeatherRecord.h>
#include <singledish/Filler/FillerUtil.h>
#include <singledish/Filler/PThreadUtil.h>

#include <casacore/casa/OS/File.h>
#include <casacore/casa/OS/Path.h>
#include <casacore/casa/BasicSL/String.h>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/ArrayIO.h>
#include <casacore/measures/Measures/MDirection.h>
#include <casacore/ms/MeasurementSets/MeasurementSet.h>
#include <casacore/ms/MeasurementSets/MSMainColumns.h>
#include <casacore/ms/MeasurementSets/MSDataDescColumns.h>
#include <casacore/ms/MeasurementSets/MSSysCalColumns.h>
#include <casacore/ms/MeasurementSets/MSPointingColumns.h>
#include <casacore/ms/MeasurementSets/MSPolColumns.h>
#include <casacore/ms/MeasurementSets/MSFeedColumns.h>
#include <casacore/ms/MeasurementSets/MSStateColumns.h>
#include <casacore/ms/MeasurementSets/MSWeatherColumns.h>
#include <casacore/tables/Tables/TableRow.h>
#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ScalarColumn.h>

namespace casa { //# NAMESPACE CASA - BEGIN

class DataAccumulator;

template<typename Reader>
class SingleDishMSFiller {
public:
  // static methods for parallel processing
  inline static void create_context();
  inline static void destroy_context();
  static void *consume(void *arg);
  static void *produce(void *arg);
  inline static void fillMainMT(SingleDishMSFiller<Reader> *filler);

  SingleDishMSFiller(std::string const &name) :
      ms_(), ms_columns_(), data_description_columns_(), feed_columns_(),
      pointing_columns_(), polarization_columns_(), syscal_columns_(),
      state_columns_(), weather_columns_(), reader_(new Reader(name)),
      is_float_(false), data_key_(), reference_feed_(-1), pointing_time_(),
      pointing_time_max_(), pointing_time_min_(), num_pointing_time_(),
      syscal_list_(), subscan_list_(), polarization_type_pool_(),
      weather_list_() {
  }

  ~SingleDishMSFiller() {
  }

  // access to reader object
  Reader const &getReader() const {
    return *reader_;
  }

  std::string const &getDataName() const {
    return reader_->getName();
  }

  // top level method to fill MS by reading input data
  void fill() {
    POST_START;

    // initialization
    initialize();

    // Fill tables that can be processed prior to main loop
    fillPreProcessTables();

    // main loop
#if 0
    SingleDishMSFiller<Reader>::fillMainMT(this);
#else
    fillMain();
#endif

    // Fill tables that must be processed after main loop
    fillPostProcessTables();

    // finalization
    finalize();

    POST_END;
  }

  // save
  void save(std::string const &name);

private:
  // initialization
  void initialize() {
    POST_START;

    // initialize reader
    reader_->initialize();

    // query if the data is complex or float
    is_float_ = reader_->isFloatData();
    if (is_float_) {
//      std::cout << "data column is FLOAT_DATA" << std::endl;
      data_key_ = "FLOAT_DATA";
    } else {
//      std::cout << "data column is DATA" << std::endl;
      data_key_ = "DATA";
    }

    // setup MS
    setupMS();

    // frame information
    MDirection::Types direction_frame = reader_->getDirectionFrame();
    auto mytable = ms_->pointing();
    ArrayColumn<Double> direction_column(mytable, "DIRECTION");
    TableRecord &record = direction_column.rwKeywordSet();
    Record meas_info = record.asRecord("MEASINFO");
    String ref_string = MDirection::showType(direction_frame);
    meas_info.define("Ref", ref_string);
    record.defineRecord("MEASINFO", meas_info);

    POST_END;
  }

// finalization
  void finalize() {
    POST_START;

    // finalize reader
    reader_->finalize();

    POST_END;
  }

// setup MS as Scratch table
// The table will be non-Scratch when it is saved
  void setupMS();

// fill tables that can be processed prior to main loop
  void fillPreProcessTables() {
    POST_START;

    // fill OBSERVATION table
    fillObservation();

    // fill ANTENNA table
    fillAntenna();

    // fill PROCESSOR table
    fillProcessor();

    // fill SOURCE table
    fillSource();

    // fill FIELD table
    fillField();

    // fill SPECTRAL_WINDOW table
    fillSpectralWindow();

    POST_END;
  }

// fill tables that must be processed after main loop
  void fillPostProcessTables() {
    POST_START;

    // fill HISTORY table
    fillHistory();

    // flush POINTING entry
    sortPointing();

    POST_END;
  }

// fill MAIN table
  void fillMain() {
    POST_START;

    size_t nrow = reader_->getNumberOfRows();
    DataAccumulator accumulator;
    DataRecord record;
//    std::cout << "nrow = " << nrow << std::endl;
    for (size_t irow = 0; irow < nrow; ++irow) {
      Bool status = reader_->getData(irow, record);
//      std::cout << "irow " << irow << " status " << status << std::endl;
//      std::cout << "   TIME=" << record.time << " INTERVAL=" << record.interval
//          << std::endl;
//      std::cout << "status = " << status << std::endl;
      if (status) {
        Bool is_ready = accumulator.queryForGet(record.time);
        if (is_ready) {
          flush(accumulator);
        }
        Bool astatus = accumulator.accumulate(record);
        (void) astatus;
//        std::cout << "astatus = " << astatus << std::endl;
      }
    }

    flush(accumulator);

    POST_END;
  }

  void flush(DataAccumulator &accumulator) {
    POST_START;

    size_t nchunk = accumulator.getNumberOfChunks();
//    std::cout << "nchunk = " << nchunk << std::endl;

    if (nchunk == 0) {
      return;
    }

    for (size_t ichunk = 0; ichunk < nchunk; ++ichunk) {
      Bool status = accumulator.get(ichunk, record_);
//      std::cout << "accumulator status = " << std::endl;
      if (status) {
        Double time = record_.time;
        Int antenna_id = record_.antenna_id;
        Int spw_id = record_.spw_id;
        Int feed_id = record_.feed_id;
        Int field_id = record_.field_id;
        Int scan = record_.scan;
        Int subscan = record_.subscan;
        String pol_type = record_.pol_type;
        String obs_mode = record_.intent;
        Int num_pol = record_.num_pol;
        Vector < Int > &corr_type = record_.corr_type;
        Int polarization_id = updatePolarization(corr_type, num_pol);
        updateFeed(feed_id, spw_id, pol_type);
        Int data_desc_id = updateDataDescription(polarization_id, spw_id);
        Int state_id = updateState(subscan, obs_mode);
        Matrix < Double > &direction = record_.direction;
        Double interval = record_.interval;

        // updatePointing must be called after updateFeed
        updatePointing(antenna_id, feed_id, time, interval, direction);

        updateSysCal(antenna_id, feed_id, spw_id, time, interval, record_);

        updateWeather(antenna_id, time, interval, record_);

        updateMain(antenna_id, field_id, feed_id, data_desc_id, state_id, scan,
            time, record_);
      }
    }
//    std::cout << "clear accumulator" << std::endl;
    accumulator.clear();

    POST_END;
  }

  void sortPointing();

  // Fill subtables
  // fill ANTENNA table
  void fillAntenna();

  // fill OBSERVATION table
  void fillObservation();

  // fill PROCESSOR table
  void fillProcessor();

  // fill SOURCE table
  void fillSource();

  // fill SOURCE table
  void fillField();

  // fill SPECTRAL_WINDOW table
  void fillSpectralWindow();

  // fill HISTORY table
  void fillHistory() {
    POST_START;

    // HISTORY table should be filled by upper-level
    // application command (e.g. importscantable)
//    ms_->history().addRow(1, True);
//    Vector<String> cols(2);
//    cols[0] = "APP_PARAMS";
//    cols[1] = "CLI_COMMAND";
//    TableRow row(ms_->history(), cols, True);
//    // TODO: fill HISTORY row here
//    TableRecord record = row.record();
//    record.print(std::cout);
//    row.put(0, record);

    POST_END;
  }

  // update POLARIZATION table
  // @param[in] corr_type polarization type list
  // @param[in] num_pol number of polarization components
  // @return polarization id
  Int updatePolarization(Vector<Int> const &corr_type, Int const &num_pol);

  // update DATA_DESCRIPTION table
  // @param[in] polarization_id polarization id
  // @param[in] spw_id spectral window id
  // @return data description id
  Int updateDataDescription(Int const &polarization_id, Int const &spw_id);

  // update STATE table
  // @param[in] subscan subscan number
  // @param[in] obs_mode observing mode string
  // @return state id
  Int updateState(Int const &subscan, String const &obs_mode);

  // update FEED table
  // @param[in] feed_id feed ID
  // @param[in] spw_id spectral window ID
  // @param[in] pol_type polarization type
  // @return feed row number
  Int updateFeed(Int const &feed_id, Int const &spw_id, String const &pol_type);

  // update POINTING table
  // @param[in] antenna_id antenna id
  // @param[in] time time stamp
  // @param[in] direction pointing direction
  Int updatePointing(Int const &antenna_id, Int const &feed_id,
      Double const &time, Double const &interval,
      Matrix<Double> const &direction);

  void updateWeather(Int const &antenna_id, Double const &time,
      Double const &interval, MSDataRecord const &data_record) {
    WeatherRecord record;
    record.clear();
    record.antenna_id = antenna_id;
    record.time = time;
    record.interval = interval;
    record.temperature = data_record.temperature;
    record.pressure = data_record.pressure;
    record.rel_humidity = data_record.rel_humidity;
    record.wind_speed = data_record.wind_speed;
    record.wind_direction = data_record.wind_direction;
    auto &mytable = ms_->weather();
    auto pos = std::find(weather_list_.begin(), weather_list_.end(), record);
    if (pos == weather_list_.end()) {
      weather_list_.push_back(record);
      uInt irow = mytable.nrow();
      mytable.addRow(1, True);
      record.fill(irow, *(weather_columns_.get()));
    } else {
      auto irow = std::distance(weather_list_.begin(), pos);
      updateWeather(*(weather_columns_.get()), irow, record);
    }
  }

  void updateSysCal(Int const &antenna_id, Int const &feed_id,
      Int const &spw_id, Double const &time, Double const &interval,
      MSDataRecord const &data_record) {
    POST_START;

    SysCalRecord record;
    record.clear();
    record.antenna_id = antenna_id;
    record.feed_id = feed_id;
    record.spw_id = spw_id;
    record.time = time;
    record.interval = interval;

    //Bool tcal_empty = False;
    Bool tsys_empty = False;

    if (data_record.tcal.empty() || allEQ(data_record.tcal, 1.0f)
        || allEQ(data_record.tcal, 0.0f)) {
      //tcal_empty = True;
    } else {
//      std::cout << "tcal seems to be valid " << data_record.tcal << std::endl;
      if (data_record.float_data.shape() == data_record.tcal.shape()) {
        record.tcal_spectrum.assign(data_record.tcal);
      } else {
        Matrix < Float > tcal = data_record.tcal;
        if (!tcal.empty()) {
          record.tcal.assign(tcal.column(0));
        }
      }
    }
    if (data_record.tsys.empty() || allEQ(data_record.tsys, 1.0f)
        || allEQ(data_record.tsys, 0.0f)) {
      tsys_empty = True;
    } else {
      if (data_record.float_data.shape() == data_record.tsys.shape()) {
        record.tsys_spectrum.assign(data_record.tsys);
      } else {
        Matrix < Float > tsys = data_record.tsys;
        if (!tsys.empty()) {
          record.tsys.assign(tsys.column(0));
        }
      }
    }

    // do not add entry if Tsys is empty
    //if (tcal_empty && tsys_empty) {
    if (tsys_empty) {
      return;
    }

    auto &mytable = ms_->sysCal();
    auto pos = std::find(syscal_list_.begin(), syscal_list_.end(), record);
    if (pos == syscal_list_.end()) {
      uInt irow = mytable.nrow();
      mytable.addRow(1, True);
      record.fill(irow, *(syscal_columns_.get()));
      syscal_list_.push_back(SysCalTableRecord(ms_.get(), irow, record));
    } else {
      auto irow = std::distance(syscal_list_.begin(), pos);
      updateSysCal(*(syscal_columns_.get()), irow, record);
    }

    POST_END;
  }

  template<class _Columns, class _Record>
  void updateSubtable(_Columns &columns, uInt irow, _Record const &record) {
    // only update timestamp and interval
    Double time_org = columns.time()(irow);
    Double interval_org = columns.interval()(irow);

    Double time_min_org = time_org - interval_org / 2.0;
    Double time_max_org = time_org + interval_org / 2.0;

    Double time_min_in = record.time - record.interval / 2.0;
    Double time_max_in = record.time + record.interval / 2.0;

    Double time_min_new = min(time_min_org, time_min_in);
    Double time_max_new = max(time_max_org, time_max_in);

    if (time_min_new != time_min_org || time_max_new != time_max_org) {
      Double time_new = (time_min_new + time_max_new) / 2.0;
      Double interval_new = time_max_new - time_min_new;
      columns.time().put(irow, time_new);
      columns.interval().put(irow, interval_new);
    }
  }

  void updateSysCal(MSSysCalColumns &columns, uInt irow,
      SysCalRecord const &record) {
    updateSubtable(columns, irow, record);
  }

  void updateWeather(MSWeatherColumns &columns, uInt irow,
      WeatherRecord const &record) {
    updateSubtable(columns, irow, record);
  }

  // update MAIN table
  // @param[in] fieldId field id
  // @param[in] feedId feed id
  // @param[in] dataDescriptionId data description id
  // @param[in] stateId state id
  // @param[in] mainSpec main table row specification except id
  void updateMain(Int const &antenna_id, Int field_id, Int feedId,
      Int dataDescriptionId, Int stateId, Int const &scan_number,
      Double const &time, MSDataRecord const &dataRecord) {
    POST_START;

    // constant stuff
    static Vector<Double> const uvw(3, 0.0);
    static Array<Bool> const flagCategory(IPosition(3, 0, 0, 0));

    // target row id
    uInt irow = ms_->nrow();

    // add new row
    //ms_->addRow(1, True);
    ms_->addRow(1, False);

    ms_columns_->uvw().put(irow, uvw);
    ms_columns_->flagCategory().put(irow, flagCategory);
    ms_columns_->antenna1().put(irow, antenna_id);
    ms_columns_->antenna2().put(irow, antenna_id);
    ms_columns_->fieldId().put(irow, field_id);
    ms_columns_->feed1().put(irow, feedId);
    ms_columns_->feed2().put(irow, feedId);
    ms_columns_->dataDescId().put(irow, dataDescriptionId);
    ms_columns_->stateId().put(irow, stateId);
    ms_columns_->scanNumber().put(irow, scan_number);
    ms_columns_->time().put(irow, time);
    ms_columns_->timeCentroid().put(irow, time);
    Double const &interval = dataRecord.interval;
    ms_columns_->interval().put(irow, interval);
    ms_columns_->exposure().put(irow, interval);

    if (is_float_) {
      Matrix < Float > floatData;
      if (dataRecord.isFloat()) {
        floatData.reference(dataRecord.float_data);
      } else {
        floatData.assign(real(dataRecord.complex_data));
      }
      ms_columns_->floatData().put(irow, floatData);
    } else {
      Matrix < Complex > data;
      if (dataRecord.isFloat()) {
        data.assign(
            makeComplex(dataRecord.float_data,
                Matrix < Float > (dataRecord.float_data.shape(), 0.0f)));
      } else {
        data.reference(dataRecord.complex_data);
      }
      ms_columns_->data().put(irow, data);
    }

    ms_columns_->flag().put(irow, dataRecord.flag);
    ms_columns_->flagRow().put(irow, dataRecord.flag_row);
    ms_columns_->sigma().put(irow, dataRecord.sigma);
    ms_columns_->weight().put(irow, dataRecord.weight);

    POST_END;
  }

  std::unique_ptr<MeasurementSet> ms_;
  std::unique_ptr<MSMainColumns> ms_columns_;
  std::unique_ptr<MSDataDescColumns> data_description_columns_;
  std::unique_ptr<MSFeedColumns> feed_columns_;
  std::unique_ptr<MSPointingColumns> pointing_columns_;
  std::unique_ptr<MSPolarizationColumns> polarization_columns_;
  std::unique_ptr<MSSysCalColumns> syscal_columns_;
  std::unique_ptr<MSStateColumns> state_columns_;
  std::unique_ptr<MSWeatherColumns> weather_columns_;
  std::unique_ptr<Reader> reader_;
  bool is_float_;
  String data_key_;

  // for POINTING table
  Int reference_feed_;
  std::map<Int, Vector<Double>> pointing_time_;
  std::map<Int, Double> pointing_time_max_;
  std::map<Int, Double> pointing_time_min_;
  Vector<uInt> num_pointing_time_;

  // for SYSCAL table
  std::vector<SysCalTableRecord> syscal_list_;

  // for STATE table
  std::vector<Int> subscan_list_;

  // for FEED table
  std::vector<Vector<String> *> polarization_type_pool_;

  // for WEATHER table
  std::vector<WeatherRecord> weather_list_;

  // Data storage to interact with DataAccumulator
  MSDataRecord record_;

}
;

} //# NAMESPACE CASA - END

#include <singledish/Filler/SingleDishMSFiller.tcc>

#endif /* SINGLEDISH_FILLER_SINGLEDISHMSFILLER_H_ */
