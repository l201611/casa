'''
The qa2adapter module holds classes that regroup the results list, ordered by
stage number, into a structure ordered by task type. This regrouping is used
by the QA2 sections of the weblog.
'''
import collections
import json
import os

import pipeline.infrastructure as infrastructure
import pipeline.infrastructure.renderer.logger as logger
import pipeline.hif.tasks.bandpass as bandpass
import pipeline.hif.tasks.gaincal as gaincal
import pipeline.hif.tasks.wvrgcal as wvrgcal

LOG = infrastructure.get_logger(__name__)



class QA2Section(object):    
    """
    QA2Section is the base class for a QA2 weblog section. It should not be 
    instantiated directly.    
    """
    # text shown in weblog describing this QA2 summary section 
    description = 'Generic QA2 Section'
    
    # filename pointing to the more detailed T2-3-X sections for this QA2
    # grouping
    url = ''

    # a dictionary holding the mapping of results class to description. This
    # should be overridden by the subclass
    result_descriptions = {}

    def __init__(self, results):
        """
        Construct a new QA2Section, allocating Results to the appropriate task
        type based on the mapping help in result_descriptions.
    
        :param results: the results
        :type results: a list of :class:`~pipeline.infrastructure.api.Result`
        """
        # Within each QA2 section, results must be gathered together by type,
        # eg. all bandpass tasks under a 'Bandpass' heading). This is 
        # accomplished with the results_by_type dictionary, keyed by results
        # class and with the matching results objects as values, eg.
        # results_by_type[BandpassResults] = [bpresult1, bpresult2]
        self.results_by_type = collections.defaultdict(list)

        # the keys of the results_descriptions dictionary holds the results
        # classes we should include in this section
        results_classes = self.result_descriptions.keys()
         
        # collect each results node matching a recognised class for this
        # section, adding it to the list held as the dictionary value
        in_section = [r for r in results if r.__class__ in results_classes]
        for result in in_section:
            self.results_by_type[result.__class__].append(result)
            
        
class CalibrationQA2Section(QA2Section):
    """
    CalibrationQA2Section collects together those results generated by
    calibration tasks.
    """
    
    description = 'Calibration'
    url = 't2-3-1m.html'
    
    result_descriptions = {
        bandpass.common.BandpassResults         : 'Bandpass',
        gaincal.common.GaincalResults           : 'Gain',
        wvrgcal.resultobjects.WvrgcalflagResult : 'WVR Calibration'
    }
    
    def __init__(self, results):
        super(CalibrationQA2Section, self).__init__(results)

        
class LineFindingQA2Section(QA2Section):
    """
    LineFindingQA2Section collects together those results generated by
    line-finding tasks.
    """
    description = 'Line finding'
    url = 't2-3-2m.html'
    
    def __init__(self, results):
        super(LineFindingQA2Section, self).__init__(results)


class FlaggingQA2Section(QA2Section):
    """
    FlaggingQA2Section collects together those results generated by flagging
    tasks.
    """
    description = 'Flagging'
    url = 't2-3-3m.html'

    result_descriptions = {
        wvrgcal.resultobjects.WvrgcalflagResult : 'WVR Flagging',
    }

    def __init__(self, results):
        super(FlaggingQA2Section, self).__init__(results)


class ImagingQA2Section(QA2Section):
    """
    ImagingQA2Section collects together those results generated by imaging
    tasks.
    """
    description = 'Imaging'
    url = 't2-3-4m.html'
    
    def __init__(self, results):
        super(ImagingQA2Section, self).__init__(results)

        
class ResultsToQA2Adapter(object):
    """
    ResultsToQA2Adapter is a wrapper providing a QA2 perspective of a results
    list. It converts the results list, ordered by stage number, into a set of
    sections (QA2Sections) grouped by QA2 type.
    """
    
    # holds the QA2Sections into which we should push the results 
    section_classes = [CalibrationQA2Section,
                       LineFindingQA2Section,
                       FlaggingQA2Section,
                       ImagingQA2Section     ]
    
    def __init__(self, results):
        """
        Construct a new ResultsToQA2Adapter, allocating Results to the
        appropriate QA2 sections.  
    
        :param results: the results
        :type results: a list of :class:`~pipeline.infrastructure.api.Result`
        """
        self.sections = collections.defaultdict(list)

        for cls in self.section_classes:
            self.sections[cls] = cls(results)


def get_url(result):
    """
    Get the URL of the QA2 section appropriate to the given result.

    :param result: the task result
    :type result: :class:`~pipeline.infrastructure.api.Result`
    :rtype: the filename of the appropriate QA2 section
    """
    result_cls = result.__class__

    for section_cls in ResultsToQA2Adapter.section_classes:
        if result_cls in section_cls.result_descriptions.keys():
            return section_cls.url
    return None


#QA2Score = collections.namedtuple('QA2Score', 
#                                  'spw antenna polarization score')

QA2Score = collections.namedtuple('Score', 'phase amplitude total')
PhaseScore = collections.namedtuple('PhaseScore', 'delay rms flag total')
AmplitudeScore = collections.namedtuple('AmplitudeScore', 'rms total sn flag') 
                                    
FlaggedFeed = collections.namedtuple('FlaggedFeed', 'antenna polarization')
 
class QA2BandpassAdapter(object):
    TableEntry = collections.namedtuple('TableEntry', 
                                        'vis spw antenna polarizations')

    def __init__(self, context, result):
        self._context = context
        self._result = result
        self._qa2 = result.qa2

        vis = self._result.inputs['vis']
        self._ms = self._context.observing_run.get_ms(vis)

        self.scores = {}
        self.amplitude_plots = self._get_plots('amp')
        self.phase_plots = self._get_plots('phase')

        self.flagged_feeds = self._get_flagged_feeds() 

        self._write_json(context, result)


    def _write_json(self, context, result):
        json_file = os.path.join(context.report_dir, 
                                 'stage%s' % result.stage_number, 
                                 'qa2.json')
        
        LOG.trace('Writing QA2 data to %s' % json_file)
        with open(json_file, 'w') as fp:
            json.dump(self.scores, fp)

    def _get_flagged_feeds(self):
        flagged = collections.defaultdict(list)
        
        # completely flagged antennas are present in the QA2 plot dictionaries
        # but missing from the QA2 score dictionaries
        for spw_id, qa2_plots in self._qa2['QA2PLOTS']['PHASE_PLOT'].items():
            if spw_id not in self._qa2['QA2SCORES']['PHASE_SCORE_RMS']:
                continue
            
            all_ids = qa2_plots.keys()
            unflagged_ids = self._qa2['QA2SCORES']['PHASE_SCORE_RMS'][spw_id].keys()
            flagged_ids = [i for i in all_ids if i not in unflagged_ids]

            for qa2_id in flagged_ids:
                spw = self._ms.get_spectral_window(spw_id)
                dd = self._ms.get_data_description(spw=spw)
                
                ant_id = int(qa2_id) / dd.num_polarizations
                feed_id = int(qa2_id) % dd.num_polarizations

                polarization = dd.polarizations[feed_id]
                antenna = self._ms.get_antenna(ant_id)[0]

                flagged[antenna].append(polarization)

        return flagged

    def _get_score(self, spw, key, y_axis):
        if y_axis == 'phase':
            return self._get_phase_score(spw, key)
        else:
            return self._get_amplitude_score(spw, key)

    def _get_phase_score(self, spw, key):        
        f = lambda k : self._qa2['QA2SCORES'][k][spw].get(key, None) 
        return {
            'total' : f('PHASE_SCORE_TOTAL'),
            'rms'   : f('PHASE_SCORE_RMS'),
            'flag'  : f('PHASE_SCORE_FLAG'),
            'delay' : f('PHASE_SCORE_DELAY')
        }

    def _get_amplitude_score(self, spw, key):
        f = lambda k : self._qa2['QA2SCORES'][k][spw].get(key, None) 
        return {
            'total' : f('AMPLITUDE_SCORE_TOTAL'),
            'rms'   : f('AMPLITUDE_SCORE_RMS'),
            'flag'  : f('AMPLITUDE_SCORE_FLAG'),
            'sn'    : f('AMPLITUDE_SCORE_SN')
        }

    def _get_plots(self, y_axis):
        y_axis = 'amp' if y_axis == 'amp' else 'phase'
        plot_key = 'AMPLITUDE_PLOT' if y_axis == 'amp' else 'PHASE_PLOT'

        plots = []
        for spw_id, spw_plots in self._qa2['QA2PLOTS'][plot_key].items():
            spw = self._ms.get_spectral_window(spw_id)
            dd = self._ms.get_data_description(spw=spw)

            # Some windows, such as ALMA window averages, do not have a data
            # description              
            if dd is None:
                continue
            
            for k, filename in spw_plots.items():
                # QA2 dictionary keys are peculiar, in that their index is a
                # function of both antenna and feed.
                ant_id = int(k) / dd.num_polarizations
                feed_id = int(k) % dd.num_polarizations
                
                polarization = dd.polarizations[feed_id]
                antenna = self._ms.get_antenna(ant_id)[0]

                # while we have the spw index and qa2 feed idx at hand,
                # populate the scores for this plot
                score = self._get_score(spw_id, k, y_axis)
                self.scores[os.path.basename(filename)] = score
                
                plot = logger.Plot(filename,
                                   x_axis='freq',
                                   y_axis=y_axis,
                                   field=self._result.inputs['field'],
                                   parameters={'ant' : antenna.identifier,
                                               'spw' : spw.id,
                                               'pol' : polarization})
                plots.append(plot)

        return plots
