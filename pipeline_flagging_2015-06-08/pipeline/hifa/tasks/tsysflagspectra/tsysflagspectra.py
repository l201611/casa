from __future__ import absolute_import

import collections
import numpy as np 
import re

from pipeline.infrastructure import casa_tasks
import pipeline.infrastructure as infrastructure
import pipeline.infrastructure.basetask as basetask
import pipeline.infrastructure.callibrary as callibrary
import pipeline.infrastructure.utils as utils

from pipeline.hif.tasks.flagging.flagdatasetter import FlagdataSetter
from pipeline.hif.tasks.common import commonresultobjects
from pipeline.hif.tasks.common import calibrationtableaccess as caltableaccess
from pipeline.hif.tasks.common import commonhelpermethods
from pipeline.hif.tasks.common import viewflaggers

from .resultobjects import TsysflagspectraResults

LOG = infrastructure.get_logger(__name__)

def intent_ids(intent, ms):
    """Get the fieldids associated with the given intents.
    """  
    # translate intents to regex form, i.e. '*PHASE*+*TARGET*' or
    # '*PHASE*,*TARGET*' to '.*PHASE.*|.*TARGET.*'
    re_intent = intent.replace(' ', '')
    re_intent = re_intent.replace('*', '.*')
    re_intent = re_intent.replace('+', '|')
    re_intent = re_intent.replace(',', '|')
    re_intent = re_intent.replace("'", "")

    # translate intents to fieldids - have to be careful that
    # PHASE ids have PHASE intent and no other
    ids = []
    if re.search(pattern=re_intent, string='AMPLITUDE'):
        ids += [field.id for field in ms.fields if 'AMPLITUDE'
          in field.intents]
    if re.search(pattern=re_intent, string='BANDPASS'):
        ids += [field.id for field in ms.fields if 'BANDPASS'
          in field.intents]
    if re.search(pattern=re_intent, string='PHASE'):
        ids += [field.id for field in ms.fields if 
          'PHASE' in field.intents and
          'BANDPASS' not in field.intents and
          'AMPLITUDE' not in field.intents]
    if re.search(pattern=re_intent, string='TARGET'):
        ids += [field.id for field in ms.fields if 'TARGET'
          in field.intents]
    if re.search(pattern=re_intent, string='ATMOSPHERE'):
        ids += [field.id for field in ms.fields if 'ATMOSPHERE'
          in field.intents]

    return ids


class TsysflagspectraInputs(basetask.StandardInputs):
    @basetask.log_equivalent_CASA_call
    def __init__(self, context, output_dir=None, vis=None, caltable=None, 
      intentgroups=None, refintent=None, metric=None, flagcmdfile=None,
      flag_nmedian=None, fnm_limit=None,
      flag_hi=None, fhi_limit=None, fhi_minsample=None,
      flag_tmf1=None, tmf1_axis=None, tmf1_limit=None,
      flag_maxabs=None, fmax_limit=None,
      niter=None, prepend=''):

        # set the properties to the values given as input arguments
        self._init_properties(vars())

    @property
    def caltable(self):
        if self._caltable is None:
            caltables = self.context.callibrary.active.get_caltable(
              caltypes='tsys')

            # return just the tsys table that matches the vis being handled
            result = None
            for name in caltables:
                # Get the tsys table name
                tsystable_vis = \
                  caltableaccess.CalibrationTableDataFiller._readvis(name)
                if tsystable_vis in self.vis:
                    result = name
                    break

            return result

        return self._caltable

    @caltable.setter
    def caltable(self, value):
        self._caltable = value

    @property
    def intentgroups(self):
        if self._intentgroups is None:
            return ['ATMOSPHERE']
        else:
            # intentgroups is set by the user as a single string, needs
            # converting to a list of strings, i.e.
            # "['a,b,c', 'd,e,f']" becomes ['a,b,c', 'd,e,f']
            intentgroups = self._intentgroups
            intentgroups = intentgroups.replace('[', '')
            intentgroups = intentgroups.replace(']', '')
            intentgroups = intentgroups.replace(' ', '')
            intentgroups = intentgroups.replace("','", "'|'")
            intentgroups = intentgroups.replace('","', '"|"')
            intentgroups = intentgroups.split('|')
            intentgroups = [intentgroup.replace('"', '') for intentgroup in \
              intentgroups]
            intentgroups = [intentgroup.replace("'", "") for intentgroup in \
              intentgroups]

        return intentgroups

    @intentgroups.setter
    def intentgroups(self, value):
        self._intentgroups = value

    @property
    def refintent(self):
        if self._refintent is None:
            return 'BANDPASS'
        return self._refintent

    @refintent.setter
    def refintent(self, value):
        self._refintent = value

    @property
    def metric(self):
        if self._metric is None:
            return 'median'
        return self._metric

    @metric.setter
    def metric(self, value):
        self._metric = value

    # flag n * median
    @property
    def flag_nmedian(self):
        if self._flag_nmedian is None:
            return True
        return self._flag_nmedian

    @flag_nmedian.setter
    def flag_nmedian(self, value):
        self._flag_nmedian = value

    @property
    def fnm_limit(self):
        if self._fnm_limit is None:
            return 2.0
        return self._fnm_limit

    @fnm_limit.setter
    def fnm_limit(self, value):
        self._fnm_limit = value

    # flag high outlier
    @property
    def flag_hi(self):
        if self._flag_hi is None:
            return False
        return self._flag_hi

    @flag_hi.setter
    def flag_hi(self, value):
        if value is None:
            value = False
        self._flag_hi = value

    @property
    def fhi_limit(self):
        if self._fhi_limit is None:
            return 5
        return self._fhi_limit

    @fhi_limit.setter
    def fhi_limit(self, value):
        if value is None:
            value = 5
        self._fhi_limit = value

    @property
    def fhi_minsample(self):
        if self._fhi_minsample is None:
            return 5
        return self._fhi_minsample

    @fhi_minsample.setter
    def fhi_minsample(self, value):
        if value is None:
            value = 5
        self._fhi_minsample = value

    @property
    def flag_maxabs(self):
        if self._flag_maxabs is None:
            return False
        return self._flag_maxabs

    @flag_maxabs.setter
    def flag_maxabs(self, value):
        if value is None:
            value = False
        self._flag_maxabs = value

    @property
    def fmax_limit(self):
        if self._fmax_limit is None:
            return 5
        return self._fmax_limit

    @fmax_limit.setter
    def fmax_limit(self, value):
        self._fmax_limit = value

    @property
    def flag_tmf1(self):
        if self._flag_tmf1 is None:
            return False
        return self._flag_tmf1

    @flag_tmf1.setter
    def flag_tmf1(self, value):
        if value is None:
            value = False
        self._flag_tmf1 = value

    @property
    def tmf1_limit(self):
        if self._tmf1_limit is None:
            return 0.5
        return self._tmf1_limit

    @tmf1_limit.setter
    def tmf1_limit(self, value):
        if value is None:
            value = 0.5
        self._tmf1_limit = value

    @property
    def tmf1_axis(self):
        if self._tmf1_axis is None:
            return 'Time'
        return self._tmf1_axis

    @tmf1_axis.setter
    def tmf1_axis(self, value):
        if value is None:
            value = 'Time'
        self._tmf1_axis = value

    @property
    def niter(self):
        if self._niter is None:
            return 1
        return self._niter

    @niter.setter
    def niter(self, value):
        self._niter = value


class Tsysflagspectra(basetask.StandardTaskTemplate):
    Inputs = TsysflagspectraInputs

    def prepare(self):
        inputs = self.inputs

        # Construct the task that will read the data and create the
        # view of the data that is the basis for flagging.
        datainputs = TsysflagspectraWorkerInputs(context=inputs.context,
          output_dir=inputs.output_dir, caltable=inputs.caltable,
          vis=inputs.vis, intentgroups=inputs.intentgroups, 
          refintent=inputs.refintent, metric=inputs.metric)
        datatask = TsysflagspectraWorker(datainputs)

        # Construct the task that will set any flags raised in the
        # underlying data.
        flagsetterinputs = FlagdataSetter.Inputs(context=inputs.context,
          vis=inputs.vis, table=inputs.caltable, inpfile=[])
        flagsettertask = FlagdataSetter(flagsetterinputs)

        # Translate the input flagging parameters to a more compact
        # list of rules.
        rules = viewflaggers.MatrixFlagger.make_flag_rules (
          flag_hilo=False,
          flag_hi=inputs.flag_hi,
          fhi_limit=inputs.fhi_limit, 
          fhi_minsample=inputs.fhi_minsample,
          flag_maxabs=inputs.flag_maxabs,
          fmax_limit=inputs.fmax_limit,
          flag_lo=False,
          flag_tmf1=inputs.flag_tmf1,
          tmf1_axis=inputs.tmf1_axis,
          tmf1_limit=inputs.tmf1_limit,
          flag_tmf2=False,
          flag_nmedian = inputs.flag_nmedian,
          fnm_hi_limit = inputs.fnm_limit,
          fnm_lo_limit = 0.0)

        # Construct the flagger task around the data view task and the
        # flagsetter task.
        matrixflaggerinputs = viewflaggers.MatrixFlaggerInputs(
          context=inputs.context, output_dir=inputs.output_dir,
          vis=inputs.vis, datatask=datatask, flagsettertask=flagsettertask,
          rules=rules, niter=inputs.niter, prepend=inputs.prepend)
        flaggertask = viewflaggers.MatrixFlagger(matrixflaggerinputs)

        # Create a "before" summary of the flagging state
        summary_job = casa_tasks.flagdata(vis=inputs.caltable, mode='summary')
        stats_before = self._executor.execute(summary_job)
        
        # Execute the flagger task
        result = self._executor.execute(flaggertask)

        # Create an "after" summary of the flagging state
        summary_job = casa_tasks.flagdata(vis=inputs.caltable, mode='summary')
        stats_after = self._executor.execute(summary_job)
        
        # Copy flagging summaries to final result        
        result.summaries = [stats_before, stats_after]

        return result

    def analyse(self, result):
        return result


class TsysflagspectraWorkerInputs(basetask.StandardInputs):
    def __init__(self, context, output_dir=None, vis=None, caltable=None,
      intentgroups=None, refintent=None, metric=None):
        self._init_properties(vars())


class TsysflagspectraWorker(basetask.StandardTaskTemplate):
    
    # This task assumes that 'tsyscal' has been called and its Tsys table
    # has been accepted into the context 
    
    Inputs = TsysflagspectraWorkerInputs

    def __init__(self, inputs):
        super(TsysflagspectraWorker, self).__init__(inputs)
        self.result = TsysflagspectraResults()

    def prepare(self):
        
        inputs = self.inputs

        # Initialize the final result
        final = []

        # Get the tsys table name
        name = inputs.caltable

        # Load the tsystable from file into memory, store vis in result
        tsystable = caltableaccess.CalibrationTableDataFiller.getcal(name)
        self.result.vis = tsystable.vis

        # Get the MS object from the context
        ms = inputs.context.observing_run.get_ms(name=tsystable.vis)

        # Get the Tsys spw map by retrieving it from the first tsys CalFrom 
        # that is present in the callibrary. 
        tsys_calfrom = utils.get_calfroms(inputs, 'tsys')[0]
        spwmap = tsys_calfrom.spwmap

        # Get the spws from the tsystable.
        tsysspws = set()
        for row in tsystable.rows:
            tsysspws.update([row.get('SPECTRAL_WINDOW_ID')])
       
        # Construct a callibrary entry for the results that are to be
        # merged back into the context, and append it to the final result.
        calto = callibrary.CalTo(vis=tsystable.vis)
        calfrom = callibrary.CalFrom(name, caltype='tsys', spwmap=spwmap)
        calapp = callibrary.CalApplication(calto, calfrom)
        final.append(calapp)

        # Get ids of fields for intent groups of interest
        intentgroupids = {}
        for intentgroup in self.inputs.intentgroups:
            groupids = intent_ids(intentgroup, ms) 
            intentgroupids[intentgroup] = groupids

        # Compute the flagging view for every spw and every intentgroup
        LOG.info ('Computing flagging metrics for caltable %s ' % (name))
        for tsysspwid in tsysspws:
            for intentgroup in self.inputs.intentgroups:
                # calculate view for each group of fieldids
                self.calculate_view(tsystable, tsysspwid, intentgroup,
                  intentgroupids[intentgroup], inputs.metric,
                  inputs.refintent)

        # Store the final result into the class result structure
        self.result.final = final[:]

        return self.result

    def analyse(self, result):
        return result

    def calculate_view(self, tsystable, spwid, intent, fieldids, metric,
      refintent=None):
        """
        tsystable -- CalibrationTableData object giving access to the tsys
                     caltable.
        spwid     -- view will be calculated using data for this spw id.
        fieldids  -- view will be calculated using data for all field_ids in
                     this list.
        metric    -- the name of the view metric:
                        'shape' gives an image where each pixel is a measure 
                          of the departure of the shape for that antenna/scan 
                          from the median over all antennas/scans in the 
                          selected fields in the SpW. 
                        'fieldshape' gives an image where each pixel is a
                          measure of the departure of the shape for that 
                          antenna/scan from the median over all
                          scans for that antenna in the selected fields
                          in the SpW. 
                        'derivative' gives an image where each pixel is
                          the MAD of the channel to channel derivative
                          across the spectrum for that antenna/scan.
                        'median' gives an image where each pixel is the
                          tsys median for that antenna/scan.
        refintent -- intent whose data are to be used as 'reference'
                     for comparison in some views.  
        """
        if metric == 'shape':
            self.calculate_shape_view(tsystable, spwid, intent, fieldids)
        elif metric == 'fieldshape':
            self.calculate_fieldshape_view(tsystable, spwid, intent, fieldids,
              refintent)
        elif metric == 'median':
            self.calculate_median_view(tsystable, spwid, intent,
              fieldids)
        elif metric == 'derivative':
            self.calculate_derivative_view(tsystable, spwid, intent,
              fieldids)

    def calculate_fieldshape_view(self, tsystable, spwid, intent, fieldids,
      refintent):
        """
        tsystable -- CalibrationTableData object giving access to the tsys
                     caltable.
        spwid     -- view will be calculated using data for this spw id.
        fieldids  -- view will be calculated using data for all field_ids in
                     this list.
        refintent -- data with this intent will be used to
                     calculate the 'reference' Tsys shape to which
                     other data will be compared.
 
        Data of the specified spwid, intent and range of fieldids are
        read from the given tsystable object. Two data 'views' will be
        created, one for each polarization. Each 'view' is a matrix with
        axes antenna_id v time. Each point in the matrix is a measure of
        the difference of the tsys spectrum there from the median of all
        the tsys spectra for that antenna/spw in the 'reference' fields
        (those with refintent). The shape metric is calculated
        using the formula:

        metric = 100 * mean(abs(normalised tsys - reference normalised tsys))

        where a 'normalised' array is defined as:

                         array
        normalised = -------------
                     median(array)

        """

        # Get the MS object from the context
        ms = self.inputs.context.observing_run.get_ms(name=self.inputs.vis)

        # Get antenna names, ids
        antenna_name, antenna_ids = commonhelpermethods.get_antenna_names(ms)

        # Get names of polarisations, and create polarisation index 
        corr_type = commonhelpermethods.get_corr_axis(ms, spwid)
        pols = range(len(corr_type))

        # Initialize tsysspectra and corresponding times.
        tsysspectra = collections.defaultdict(TsysflagspectraResults)
        times = set()

        # Select rows from tsystable that match the specified spw and fields,
        # store a Tsys spectrum for each polarisation in the tsysspectra results
        # and store the corresponding time.
        for row in tsystable.rows:
            if row.get('SPECTRAL_WINDOW_ID') == spwid and \
              row.get('FIELD_ID') in fieldids:

                for pol in pols:          
                    tsysspectrum = commonresultobjects.SpectrumResult(
                      data=row.get('FPARAM')[pol,:,0],
                      flag=row.get('FLAG')[pol,:,0],
                      datatype='Normalised Tsys', filename=tsystable.name,
                      field_id=row.get('FIELD_ID'),
                      spw=row.get('SPECTRAL_WINDOW_ID'),
                      ant=(row.get('ANTENNA1'),
                      antenna_name[row.get('ANTENNA1')]),
                      pol=corr_type[pol][0],
                      time=row.get('TIME'), normalise=True)
                    tsysspectra[pol].addview(tsysspectrum.description,
                      tsysspectrum)
                    times.update([row.get('TIME')])

        # Get ids of fields for reference spectra
        referencefieldids = intent_ids(refintent, ms)

        # Create a flagging view for each antenna
        for pol in pols:
            
            # Initialize results
            tsysrefs = TsysflagspectraResults()

            # For each antenna, create a "reference" Tsys spectrum, 
            # by creating a stack of Tsys spectra that belong to the 
            # reference field ids, and calculating the median Tsys
            # spectrum for this spectrum stack:

            for antenna_id in antenna_ids:
                
                # Create a stack of spectra, and corresponding flags, 
                # for the fields that are listed as reference field.
                spectrumstack = None
                for description in tsysspectra[pol].descriptions():
                    tsysspectrum = tsysspectra[pol].last(description)
                    if tsysspectrum.pol==corr_type[pol][0] and \
                      tsysspectrum.ant[0]==antenna_id and \
                      tsysspectrum.field_id in referencefieldids:
                        if spectrumstack is None:
                            spectrumstack = tsysspectrum.data
                            flagstack = tsysspectrum.flag
                        else:
                            spectrumstack = np.vstack((tsysspectrum.data,
                              spectrumstack))
                            flagstack = np.vstack((tsysspectrum.flag,
                              flagstack))

                if spectrumstack is not None:
                    
                    # From the stack of Tsys spectra, create a median Tsys 
                    # spectrum and corresponding flag list.
                    
                    # Ensure that the spectrum stack is 2D
                    if np.ndim(spectrumstack) == 1:
                        # need to reshape to 2d array
                        dim = np.shape(spectrumstack)[0]
                        spectrumstack = np.reshape(spectrumstack, (1,dim))
                        flagstack = np.reshape(flagstack, (1,dim))

                    # Initialize median spectrum and corresponding flag list
                    stackmedian = np.zeros(np.shape(spectrumstack)[1])
                    stackmedianflag = np.ones(np.shape(spectrumstack)[1], np.bool)

                    # Calculate median Tsys spectrum from spectrum stack
                    for j in range(np.shape(spectrumstack)[1]):
                        valid_data = spectrumstack[:,j][np.logical_not(flagstack[:,j])]
                        if len(valid_data):
                            stackmedian[j] = np.median(valid_data)
                            stackmedianflag[j] = False

                    # In the median Tsys reference spectrum, look for channels
                    # that may cover lines, and flag these channels:

                    LOG.debug('looking for atmospheric lines')
                    
                    # Calculate first derivative, and propagate flags
                    diff = abs(stackmedian[1:] - stackmedian[:-1])
                    diff_flag = np.logical_or(stackmedianflag[1:], stackmedianflag[:-1])

                    # Determine where first derivative is larger 0.05
                    # and not flagged, and create a new list of flags 
                    # that include these channels
                    newflag = (diff>0.05) & np.logical_not(diff_flag)
                    flag_chan = np.zeros([len(newflag)+1], np.bool)
                    flag_chan[:-1] = newflag
                    flag_chan[1:] = np.logical_or(flag_chan[1:], newflag)
                    channels_flagged = np.arange(len(newflag)+1)[flag_chan]

                    # Flag lines in the median Tsys reference spectrum
                    if len(flag_chan) > 0:
                        LOG.debug('possible lines flagged in channels: %s', 
                          channels_flagged)
                        stackmedianflag[flag_chan] = True
           
                    # Create a SpectrumResult from the median Tsys reference
                    # spectrum, and add it as a view to tsysrefs
                    tsysref = commonresultobjects.SpectrumResult(
                      data=stackmedian,
                      flag=stackmedianflag,  
                      datatype='Median Normalised Tsys',
                      filename=tsystable.name, spw=spwid,
                      pol=corr_type[pol][0],
                      ant=(antenna_id, antenna_name[antenna_id]),
                      intent=intent)
                    tsysrefs.addview(tsysref.description, tsysref)

            # Initialize the data and flagging state for the flagging view,
            # and get values for the 'times' axis.
            times = np.sort(list(times))
            data = np.zeros([antenna_ids[-1]+1, len(times)])
            flag = np.ones([antenna_ids[-1]+1, len(times)], np.bool)

            # Populate the flagging view based on the flagging metric
            for description in tsysspectra[pol].descriptions():

                # Get Tsys spectrum
                tsysspectrum = tsysspectra[pol].last(description)

                # Get the 'reference' median Tsys spectrum for this antenna
                for ref_description in tsysrefs.descriptions():
                    tsysref = tsysrefs.last(ref_description)
                    if tsysref.ant[0] != tsysspectrum.ant[0]:
                        continue

                    # Calculate the metric
                    # (100 * mean absolute deviation from reference)
                    diff = abs(tsysspectrum.data - tsysref.data)
                    diff_flag = (tsysspectrum.flag | tsysref.flag)
                    if not np.all(diff_flag):
                        metric = 100.0 * np.mean(diff[diff_flag==0])
                        metricflag = 0
                    else:
                        metric = 0.0
                        metricflag = 1

                    ant = tsysspectrum.ant
                    caltime = tsysspectrum.time
                    data[ant[0], caltime==times] = metric
                    flag[ant[0], caltime==times] = metricflag

            # Create axes for flagging view
            axes = [commonresultobjects.ResultAxis(name='Antenna1',
              units='id', data=np.arange(antenna_ids[-1]+1)),
              commonresultobjects.ResultAxis(name='Time', units='',
              data=times)]

            # Convert flagging view into an ImageResult
            viewresult = commonresultobjects.ImageResult(
              filename=tsystable.name, data=data,
              flag=flag, axes=axes, datatype='Shape Metric * 100',
              spw=spwid, intent=intent, pol=corr_type[pol][0])

            # Store the spectra contributing to this view as 'children'
            viewresult.children['tsysmedians'] = tsysrefs
            viewresult.children['tsysspectra'] = tsysspectra[pol]

            # Add the view results to the class result structure
            self.result.addview(viewresult.description, viewresult)

    def calculate_shape_view(self, tsystable, spwid, intent, fieldids):
        """
        tsystable -- CalibrationTableData object giving access to the tsys
                     caltable.
        spwid     -- view will be calculated using data for this spw id.
        fieldids  -- view will be calculated using data for all field_ids in
                     this list.

        Data of the specified spwid, intent and range of fieldids are
        read from the given tsystable object. Two data 'views' will be
        created, one for each polarization. Each 'view' is a matrix with
        axes antenna_id v time. Each point in the matrix is a measure of the
        difference of the tsys spectrum there from the median of all the 
        tsys spectra for that spw in the selected fields. The shape metric
        is calculated using the formula:

        metric = 100 * mean(abs(normalised tsys - median normalised tsys))

        where a 'normalised' array is defined as:

                         array
        normalised = -------------
                     median(array)

        """

        ms = self.inputs.context.observing_run.get_ms(name=self.inputs.vis)

        # Get antenna names, ids
        antenna_name, antenna_ids = commonhelpermethods.get_antenna_names(ms)

        # Get names of polarisations, and create polarisation index 
        corr_type = commonhelpermethods.get_corr_axis(ms, spwid)
        pols = range(len(corr_type))

        # Initialize the tsysspectra and corresponding times
        tsysspectra = collections.defaultdict(TsysflagspectraResults)
        times = set()

        # Select rows from tsystable that match the specified spw and fields,
        # store a Tsys spectrum for each polarisation in the tsysspectra results
        # and store the corresponding time.
        for row in tsystable.rows:
            if row.get('SPECTRAL_WINDOW_ID') == spwid and \
              row.get('FIELD_ID') in fieldids:

                for pol in pols:          
                    tsysspectrum = commonresultobjects.SpectrumResult(
                      data=row.get('FPARAM')[pol,:,0],
                      flag=row.get('FLAG')[pol,:,0],
                      datatype='Normalised Tsys', filename=tsystable.name,
                      field_id=row.get('FIELD_ID'),
                      spw=row.get('SPECTRAL_WINDOW_ID'),
                      ant=(row.get('ANTENNA1'),
                      antenna_name[row.get('ANTENNA1')]),
                      pol=corr_type[pol][0],
                      time=row.get('TIME'), normalise=True)

                    tsysspectra[pol].addview(tsysspectrum.description,
                      tsysspectrum)
                    times.update([row.get('TIME')])

        # Create separate flagging views for each polarisation
        for pol in pols:
            
            # Initialize median Tsys spectra, and spectrum stack
            tsysmedians = TsysflagspectraResults()
            spectrumstack = None

            # Create a stack of all spectra for the specified polarisation
            for description in tsysspectra[pol].descriptions():
                tsysspectrum = tsysspectra[pol].last(description)
                if tsysspectrum.pol==corr_type[pol][0]:
                    if spectrumstack is None:
                        spectrumstack = tsysspectrum.data
                        flagstack = tsysspectrum.flag
                    else:
                        spectrumstack = np.vstack((tsysspectrum.data,
                          spectrumstack))
                        flagstack = np.vstack((tsysspectrum.flag,
                          flagstack))

            # From the stack of Tsys spectra, create a median Tsys
            # spectrum and corresponding flagging state, convert to a
            # SpectrumResult, and store this as a view in tsysmedians:
            if spectrumstack is not None:
                
                # Initialize median spectrum and corresponding flag list
                # and populate with valid data
                stackmedian = np.zeros(np.shape(spectrumstack)[1])
                stackmedianflag = np.ones(np.shape(spectrumstack)[1], np.bool)
                for j in range(np.shape(spectrumstack)[1]):
                    valid_data = spectrumstack[:,j][np.logical_not(flagstack[:,j])]
                    if len(valid_data):
                        stackmedian[j] = np.median(valid_data)
                        stackmedianflag[j] = False

                tsysmedian = commonresultobjects.SpectrumResult(
                  data=stackmedian, 
                  datatype='Median Normalised Tsys',
                  filename=tsystable.name, spw=spwid, 
                  pol=corr_type[pol][0], intent=intent)
                tsysmedians.addview(tsysmedian.description, tsysmedian)

            # Initialize the data and flagging state for the flagging view,
            # and get values for the 'times' axis.
            times = np.sort(list(times))
            data = np.zeros([antenna_ids[-1]+1, len(times)])
            flag = np.ones([antenna_ids[-1]+1, len(times)], np.bool)

            # Populate the flagging view based on the flagging metric
            for description in tsysspectra[pol].descriptions():
                tsysspectrum = tsysspectra[pol].last(description)
                validdata = tsysspectrum.data\
                  [np.logical_not(tsysspectrum.flag)]
                validmedian = stackmedian\
                  [np.logical_not(tsysspectrum.flag)]
                if len(validdata) > 0:
                    metric = abs(validdata - validmedian)
                    metric = 100.0 * np.mean(metric)
                    metricflag = 0
                else:
                    metric = 0.0
                    metricflag = 1

                ant = tsysspectrum.ant
                caltime = tsysspectrum.time
                data[ant[0], caltime==times] = metric
                flag[ant[0], caltime==times] = metricflag

            # Create axes for flagging view
            axes = [commonresultobjects.ResultAxis(name='Antenna1',
              units='id', data=np.arange(antenna_ids[-1]+1)),
              commonresultobjects.ResultAxis(name='Time', units='',
              data=times)]

            # Convert flagging view into an ImageResult
            viewresult = commonresultobjects.ImageResult(
              filename=tsystable.name, data=data,
              flag=flag, axes=axes, datatype='Shape Metric * 100',
              spw=spwid, intent=intent, pol=corr_type[pol][0])

            # Store the spectra contributing to this view as 'children'
            viewresult.children['tsysmedians'] = tsysmedians
            viewresult.children['tsysspectra'] = tsysspectra[pol]

            # Add the view results to the class result structure
            self.result.addview(viewresult.description, viewresult)


    def calculate_median_view(self, tsystable, spwid, intent, fieldids):
        """
        tsystable -- CalibrationTableData object giving access to the tsys
                     caltable.
        spwid     -- view will be calculated using data for this spw id.
        fieldids  -- view will be calculated using data for all field_ids in
                     this list.

        Data of the specified spwid, intent and range of fieldids are
        read from the given tsystable object. Two data 'views' will be
        created, one for each polarization. Each 'view' is a matrix with
        axes antenna_id v time. Each point in the matrix is the median
        value of the tsys spectrum for that antenna/time.
        """

        # Get the MS object from the context
        ms = self.inputs.context.observing_run.get_ms(name=self.inputs.vis)

        # Get antenna names, ids
        antenna_name, antenna_ids = commonhelpermethods.get_antenna_names(ms)

        # Get names of polarisations, and create polarisation index 
        corr_type = commonhelpermethods.get_corr_axis(ms, spwid)
        pols = range(len(corr_type))

        # Initialize a dictionary of Tsys spectra results and corresponding times
        tsysspectra = collections.defaultdict(TsysflagspectraResults)
        times = set()

        # Select rows from tsystable that match the specified spw and fields,
        # store a Tsys spectrum for each polarisation in the tsysspectra results
        # and store the corresponding time.
        for row in tsystable.rows:
            if row.get('SPECTRAL_WINDOW_ID') == spwid and \
              row.get('FIELD_ID') in fieldids:

                for pol in pols:
                    tsysspectrum = commonresultobjects.SpectrumResult(
                      data=row.get('FPARAM')[pol,:,0],
                      flag=row.get('FLAG')[pol,:,0],
                      datatype='Tsys', filename=tsystable.name,
                      field_id=row.get('FIELD_ID'),
                      spw=row.get('SPECTRAL_WINDOW_ID'),
                      ant=(row.get('ANTENNA1'),
                      antenna_name[row.get('ANTENNA1')]), units='K',
                      pol=corr_type[pol][0],
                      time=row.get('TIME'), normalise=False)

                    tsysspectra[pol].addview(tsysspectrum.description,
                      tsysspectrum)
                    times.update([row.get('TIME')])

        # Create separate flagging views for each polarisation
        for pol in pols:
            
            # Initialize the median Tsys spectra results, and spectrum stack
            tsysmedians = TsysflagspectraResults()
            spectrumstack = None

            # Create a stack of all Tsys spectra for specified spw and pol
            for description in tsysspectra[pol].descriptions():
                tsysspectrum = tsysspectra[pol].last(description)
                if tsysspectrum.pol==corr_type[pol][0]:
                    if spectrumstack is None:
                        spectrumstack = np.vstack((tsysspectrum.data,))
                        flagstack = np.vstack((tsysspectrum.flag,))
                    else:
                        spectrumstack = np.vstack((tsysspectrum.data,
                          spectrumstack))
                        flagstack = np.vstack((tsysspectrum.flag,
                          flagstack))
            
            # From the stack of all Tsys spectra for the specified spw and pol,
            # create a median Tsys spectrum and corresponding flagging state,
            # convert to a SpectrumResult, and store this as a view in tsysmedians:
            if spectrumstack is not None:

                # Initialize median spectrum and corresponding flag list
                # and populate with valid data
                stackmedian = np.zeros(np.shape(spectrumstack)[1])
                stackmedianflag = np.ones(np.shape(spectrumstack)[1], np.bool)
                for j in range(np.shape(spectrumstack)[1]):
                    valid_data = spectrumstack[:,j][np.logical_not(flagstack[:,j])]
                    if len(valid_data):
                        stackmedian[j] = np.median(valid_data)
                        stackmedianflag[j] = False

                tsysmedian = commonresultobjects.SpectrumResult(
                  data=stackmedian, 
                  datatype='Median Normalised Tsys',
                  filename=tsystable.name, spw=spwid, 
                  pol=corr_type[pol][0],
                  intent=intent)
                tsysmedians.addview(tsysmedian.description, tsysmedian)

            # Initialize the data and flagging state for the flagging view,
            # and get values for the 'times' axis.
            times = np.sort(list(times))
            data = np.zeros([antenna_ids[-1]+1, len(times)])
            flag = np.ones([antenna_ids[-1]+1, len(times)], np.bool)

            # Populate the flagging view based on the flagging metric
            for description in tsysspectra[pol].descriptions():
                tsysspectrum = tsysspectra[pol].last(description)
                metric = tsysspectrum.median
                metricflag = np.all(tsysspectrum.flag)

                ant = tsysspectrum.ant
                caltime = tsysspectrum.time

                data[ant[0], caltime==times] = metric
                flag[ant[0], caltime==times] = metricflag

            # Create axes for flagging view
            axes = [commonresultobjects.ResultAxis(name='Antenna1',
              units='id', data=np.arange(antenna_ids[-1]+1)),
              commonresultobjects.ResultAxis(name='Time', units='',
              data=times)]

            # Convert flagging view into an ImageResult
            viewresult = commonresultobjects.ImageResult(
              filename=tsystable.name, data=data,
              flag=flag, axes=axes, datatype='Median Tsys',
              spw=spwid, intent=intent, pol=corr_type[pol][0])

            # Store the spectra contributing to this view as 'children'
            viewresult.children['tsysmedians'] = tsysmedians
            viewresult.children['tsysspectra'] = tsysspectra[pol]

            # Add the view results to the class result structure
            self.result.addview(viewresult.description, viewresult)

    def calculate_derivative_view(self, tsystable, spwid, intent, fieldids):
        """
        tsystable -- CalibrationTableData object giving access to the tsys
                     caltable.
        spwid     -- view will be calculated using data for this spw id.
        fieldids  -- view will be calculated using data for all field_ids in
                     this list.

        Data of the specified spwid, intent and range of fieldids are
        read from the given tsystable object. Two data 'views' will be
        created, one for each polarization. Each 'view' is a matrix with
        axes antenna_id v time. Each point in the matrix is the median
        absolute deviation (MAD) of the channel to channel derivative
        across the Tsys spectrum for that antenna/time.
        """

        # Get the MS object from the context
        ms = self.inputs.context.observing_run.get_ms(name=self.inputs.vis)

        # Get antenna names, ids
        antenna_name, antenna_ids = commonhelpermethods.get_antenna_names(ms)

        # Get names of polarisations, and create polarisation index 
        corr_type = commonhelpermethods.get_corr_axis(ms, spwid)
        pols = range(len(corr_type))

        # Initialize a dictionary of Tsys spectra results and corresponding times
        tsysspectra = collections.defaultdict(TsysflagspectraResults)
        times = set()

        # Select rows from tsystable that match the specified spw and fields,
        # store a Tsys spectrum for each polarisation in the tsysspectra results
        # and store the corresponding time.
        # Note: the SpectrumResult is normalised (i.e. divided by its median).
        for row in tsystable.rows:
            if row.get('SPECTRAL_WINDOW_ID') == spwid and \
              row.get('FIELD_ID') in fieldids:

                for pol in pols:
                    tsysspectrum = commonresultobjects.SpectrumResult(
                      data=row.get('FPARAM')[pol,:,0],
                      flag=row.get('FLAG')[pol,:,0],
                      datatype='Tsys', filename=tsystable.name,
                      field_id=row.get('FIELD_ID'),
                      spw=row.get('SPECTRAL_WINDOW_ID'),
                      ant=(row.get('ANTENNA1'),
                      antenna_name[row.get('ANTENNA1')]), units='K',
                      pol=corr_type[pol][0], time=row.get('TIME'),
                      normalise=True)

                    tsysspectra[pol].addview(tsysspectrum.description,
                      tsysspectrum)
                    times.update([row.get('TIME')])

        # Create separate flagging views for each polarisation
        for pol in pols:
            
            # Initialize the median Tsys spectra results, and spectrum stack
            tsysmedians = TsysflagspectraResults()
            spectrumstack = None

            # Create a stack of all Tsys spectra for specified spw and pol
            for description in tsysspectra[pol].descriptions():
                tsysspectrum = tsysspectra[pol].last(description)
                if tsysspectrum.pol==corr_type[pol][0]:
                    if spectrumstack is None:
                        spectrumstack = np.vstack((tsysspectrum.data,))
                        flagstack = np.vstack((tsysspectrum.flag,))
                    else:
                        spectrumstack = np.vstack((tsysspectrum.data,
                          spectrumstack))
                        flagstack = np.vstack((tsysspectrum.flag,
                          flagstack))

            # From the stack of all Tsys spectra for the specified spw and pol,
            # create a median Tsys spectrum and corresponding flagging state,
            # convert to a SpectrumResult, and store this as a view in tsysmedians:
            if spectrumstack is not None:

                # Initialize median spectrum and corresponding flag list
                # and populate with valid data
                stackmedian = np.zeros(np.shape(spectrumstack)[1])
                stackmedianflag = np.ones(np.shape(spectrumstack)[1], np.bool)
                for j in range(np.shape(spectrumstack)[1]):
                    valid_data = spectrumstack[:,j][np.logical_not(flagstack[:,j])]
                    if len(valid_data):
                        stackmedian[j] = np.median(valid_data)
                        stackmedianflag[j] = False

                tsysmedian = commonresultobjects.SpectrumResult(
                  data=stackmedian, 
                  datatype='Median Normalised Tsys',
                  filename=tsystable.name, spw=spwid, pol=pol,
                  intent=intent)
                tsysmedians.addview(tsysmedian.description, tsysmedian)

            # Initialize the data and flagging state for the flagging view,
            # and get values for the 'times' axis.
            times = np.sort(list(times))
            data = np.zeros([antenna_ids[-1]+1, len(times)])
            flag = np.ones([antenna_ids[-1]+1, len(times)], np.bool)

            # Populate the flagging view based on the flagging metric:
            # Get MAD of channel by channel derivative of Tsys
            # for each antenna, time/scan.
            for description in tsysspectra[pol].descriptions():

                tsysspectrum = tsysspectra[pol].last(description)

                tsysdata = tsysspectrum.data
                tsysflag = tsysspectrum.flag

                deriv = tsysdata[1:] - tsysdata[:-1]
                deriv_flag = np.logical_or(tsysflag[1:], tsysflag[:-1])
                
                valid = deriv[np.logical_not(deriv_flag)]
                if len(valid):
                    metric = np.median(np.abs(valid - np.median(valid))) * 100
                    
                    ant = tsysspectrum.ant
                    caltime = tsysspectrum.time
                    data[ant[0], caltime==times] = metric
                    flag[ant[0], caltime==times] = 0
            
            # Create axes for flagging view
            axes = [commonresultobjects.ResultAxis(name='Antenna1',
              units='id', data=np.arange(antenna_ids[-1]+1)),
              commonresultobjects.ResultAxis(name='Time', units='',
              data=times)]
            
            # Convert flagging view into an ImageResult
            viewresult = commonresultobjects.ImageResult(
              filename=tsystable.name, data=data,
              flag=flag, axes=axes, datatype='100 * MAD of normalised derivative',
              spw=spwid, intent=intent, pol=corr_type[pol][0])

            # Store the spectra contributing to this view as 'children'
            viewresult.children['tsysmedians'] = tsysmedians
            viewresult.children['tsysspectra'] = tsysspectra[pol]

            # Add the view results to the class result structure
            self.result.addview(viewresult.description, viewresult)