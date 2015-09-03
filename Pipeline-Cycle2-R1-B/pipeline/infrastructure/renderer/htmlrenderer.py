from __future__ import absolute_import, division
import atexit
import collections
import contextlib
import datetime
import decimal
import itertools
import json
import math
import operator
import os
import pydoc
import re
import shutil
import StringIO
import sys
import tempfile
import zipfile

import casadef
import mako
import mako.lookup as lookup
import numpy as np

import pipeline as pipeline
import pipeline.domain.measures as measures
import pipeline.infrastructure as infrastructure
from pipeline.infrastructure import casa_tasks
import pipeline.infrastructure.casatools as casatools
import pipeline.infrastructure.casataskdict as casataskdict
import pipeline.infrastructure.displays.applycal as applycal
import pipeline.infrastructure.displays.setjy as setjy
import pipeline.infrastructure.displays.gfluxscale as gfluxscale
import pipeline.infrastructure.displays.bandpass as bandpass
import pipeline.infrastructure.displays.clean as clean
import pipeline.infrastructure.displays.flagging as flagging
import pipeline.infrastructure.displays.gaincal as gaincal
import pipeline.infrastructure.displays.image as image
import pipeline.infrastructure.displays.summary as summary
import pipeline.infrastructure.displays.slice as slicedisplay
import pipeline.infrastructure.displays.tsys as tsys
import pipeline.infrastructure.displays.wvr as wvr
import pipeline.infrastructure.displays.singledish as sddisplay
import pipeline.infrastructure.displays.vla.finalcalsdisplay as finalcalsdisplay
import pipeline.infrastructure.displays.vla.testBPdcalsdisplay as testBPdcalsdisplay
import pipeline.infrastructure.displays.vla.semifinalBPdcalsdisplay as semifinalBPdcalsdisplay
import pipeline.infrastructure.displays.vla.testgainsdisplay as testgainsdisplay
import pipeline.infrastructure.displays.vla.fluxbootdisplay as fluxbootdisplay
import pipeline.infrastructure.displays.vla.targetflagdisplay as targetflagdisplay
import pipeline.infrastructure.displays.vla.opacitiesdisplay as opacitiesdisplay
import pipeline.infrastructure.basetask as basetask
import pipeline.infrastructure.filenamer as filenamer
import pipeline.infrastructure.logging as logging
import pipeline.infrastructure.renderer.rendererutils as rendererutils
from pipeline.infrastructure.renderer import templates
from pipeline.infrastructure.renderer.templates import resources
from . import logger
from . import qaadapter
from .. import utils
import pipeline.hif as hif
import pipeline.hsd as hsd
import pipeline.hifa as hifa
import pipeline.hifv as hifv
from pipeline.hif.tasks.common import calibrationtableaccess as caltableaccess

LOG = infrastructure.get_logger(__name__)


def get_task_description(result_obj):
    if not isinstance(result_obj, collections.Iterable):
        return get_task_description([result_obj, ])

    if len(result_obj) is 0:
        msg = 'Cannot get description for zero-length results list'
        LOG.error(msg)
        return msg

    task_cls = getattr(result_obj[0], 'task', None)
    if task_cls is None:
        results_cls = result_obj[0].__class__.__name__
        msg = 'No task registered on results of type %s' % results_cls
        LOG.warning(msg)
        return msg

    d = {'description' : _get_task_description_for_class(task_cls),
         'task_name'   : get_task_name(result_obj, include_stage=False),
         'stage'       : get_stage_number(result_obj)}
    return '{stage}. <strong>{task_name}</strong>: {description}'.format(**d)

def _get_task_description_for_class(task_cls):
    if task_cls is hif.tasks.AgentFlagger:
        return 'Deterministic Flagging'

    if task_cls is hif.tasks.Applycal:
        return 'Apply calibrations from context'

    if task_cls is hif.tasks.Atmflag:
        return 'Flag on atmospheric transmission'

    if task_cls is hif.tasks.Bandpassflagchans:
        return 'Flag channels of bandpass calibration'

    if task_cls is hif.tasks.Clean:
        return 'Produce a cleaned image'

    if task_cls is hif.tasks.CleanList:
        return 'Calculate clean products'

    if task_cls is hif.tasks.ExportData:
        return 'Export data products'

    if task_cls is hif.tasks.Flagchans:
        return 'Flag channels in raw data'

    if task_cls in (hifa.tasks.FlagDeterALMA, hifa.tasks.ALMAAgentFlagger):
        return 'ALMA deterministic flagging'

    if task_cls is hifa.tasks.FluxcalFlag:
        return 'Flag spectral features in solar system flux calibrators'

    if task_cls is hifa.tasks.GcorFluxscale:
        return 'Transfer fluxscale from amplitude calibrator'

    if task_cls is hif.tasks.GTypeGaincal:
        return 'G-type gain calibration'

    if task_cls in (hifa.tasks.ALMAImportData, hif.tasks.ImportData, 
                    hsd.tasks.SDImportData, hsd.tasks.SDImportData2,
                    hifv.tasks.importdata.importdata.VLAImportData):
        return 'Register measurement sets with the pipeline'

    if task_cls is hifa.tasks.Linpolcal:
        return 'Linear polarization calibration'

    if task_cls is hif.tasks.Lowgainflag:
        return 'Flag antennas with low gain'

    if task_cls is hif.tasks.MakeCleanList:
        return 'Compile a list of cleaned images to be calculated'

    if task_cls is hif.tasks.NormaliseFlux:
        return 'Calculate mean fluxes of calibrators'

    if task_cls is hif.tasks.PhcorBandpass:
        return 'Bandpass calibration'

    if task_cls is hif.tasks.RefAnt:
        return 'Select reference antennas'

    if task_cls is hif.tasks.Setjy:
        return 'Set calibrator model visibilities'

    if task_cls is hsd.tasks.SDConvertData:
        return 'Convert Data'

    if task_cls is hsd.tasks.SDReduction:
        return 'Single-dish end-to-end reduction'

    if task_cls is hsd.tasks.SDExportData:
        return 'Single-dish SDExportData'

    if task_cls in (hifa.tasks.TimeGaincal, hif.tasks.GaincalMode,
                    hif.tasks.GTypeGaincal, hif.tasks.GSplineGaincal):
        return 'Gain calibration'

    if task_cls is hifa.tasks.Tsyscal:
        return 'Calculate Tsys calibration'

    if task_cls is hifa.tasks.Tsysflag:
        return 'Flag Tsys calibration'

    if task_cls is hifa.tasks.Tsysflagchans:
        return 'Flag channels of Tsys calibration'

    if task_cls is hifa.tasks.Tsysflagspectra:
        return 'Flag spectra in Tsys calibration'

    if task_cls is hifa.tasks.Wvrgcal:
        return 'Calculate WVR calibration'

    if task_cls is hifa.tasks.Wvrgcalflag:
        return 'Calculate and flag WVR calibration'

    if task_cls is hsd.tasks.SDInspectData:
        return 'Inspect data'

    if task_cls is hsd.tasks.SDCalTsys:
        return 'Generate Tsys calibration table'

    if task_cls is hsd.tasks.SDCalSky:
        return 'Generate Sky calibration table'

    if task_cls is hsd.tasks.SDApplyCal:
        return 'Apply calibration tables'

    if task_cls in (hsd.tasks.SDImaging, hsd.tasks.SDImagingOld):
        return 'Image single dish data'

    if task_cls is hsd.tasks.SDBaselineOld:
        return 'Subtract spectral baseline'

    if task_cls is hsd.tasks.SDBaseline:
        return 'Generate Baseline tables and subtract spectral baseline'

    if task_cls is hsd.tasks.SDFlagData:
        return 'Flag data by Tsys, weather, and statistics of spectra'
    
    if task_cls is hsd.tasks.SDFlagBaseline:
        return 'Iterative execution of baseline subtraction and flagging'

    if task_cls is hsd.tasks.SDPlotFlagBaseline:
        return 'Plot whole spectra with baseline fit and flag result'

    if task_cls is hsd.tasks.SDConvertData:
        return 'Convert MSs into Scantables'
    
    if task_cls is hifv.tasks.flagging.vlaagentflagger.VLAAgentFlagger:
        return 'VLA deterministic flagging'
    
    if task_cls is hifv.tasks.setmodel.SetModel:
        return 'Set model: Prepare for calibrations'
        
    if task_cls is hifv.tasks.setmodel.VLASetjy:
        return 'Set model: Prepare for calibrations'
    
    if task_cls is hifv.tasks.priorcals.priorcals.Priorcals:
        return 'Priorcals (gaincurves, opacities, and rq gains)'
    
    if task_cls is hifv.tasks.flagging.hflag.Heuristicflag:
        return 'VLA heuristic flagging'
    
    if task_cls is hifv.tasks.testBPdcals:
        return 'Initial test calibrations'
        
    if task_cls is hifv.tasks.Hanning:
        return 'Hanning smoothing'
        
    if task_cls is hifv.tasks.flagging.flagbaddeformatters.FlagBadDeformatters:
        return 'Flag bad deformatters'
        
    if task_cls is hifv.tasks.flagging.uncalspw.Uncalspw:
        return 'Flag spws that have no calibration'
        
    if task_cls is hifv.tasks.flagging.checkflag.Checkflag:
        return 'Flag possible RFI on BP calibrator using rflag'
    
    if task_cls is hifv.tasks.semiFinalBPdcals:
        return 'Semi-final delay and bandpass calibrations'
        
    if task_cls is hifv.tasks.fluxscale.solint.Solint:
        return 'Determine solint'
    
    if task_cls is hifv.tasks.fluxscale.testgains.Testgains:
        return 'Test gain calibrations'
        
    if task_cls is hifv.tasks.setmodel.fluxgains.Fluxgains:
        return 'Flux density bootstrapping'
    
    if task_cls is hifv.tasks.fluxscale.fluxboot.Fluxboot:
        return 'Fit spectral index'
        
    if task_cls is hifv.tasks.Finalcals:
        return 'Final calibration tables'
    
    if task_cls is hifv.tasks.Applycals:
        return 'Apply all calibrations'
    
    if task_cls is hifv.tasks.flagging.targetflag.Targetflag:
        return 'Targetflag (all data through rflag)'
    
    if task_cls is hifv.tasks.Statwt:
        return 'Reweight visibilities'

    if LOG.isEnabledFor(LOG.todo):
        LOG.todo('No task description for \'%s\'' % task_cls.__name__)
        return ('\'%s\' (developers should add a task description)'
                '' % task_cls.__name__)

    return ('\'%s\'' % task_cls.__name__)


def get_task_name(result_obj, include_stage=True):
    if not isinstance(result_obj, collections.Iterable):
        return get_task_name([result_obj, ])

    if len(result_obj) is 0:
        msg = 'Cannot get task name for zero-length results list'
        LOG.error(msg)
        return msg

    task_cls = result_obj[0].task
    if task_cls is None:
        results_cls = result_obj[0].__class__.__name__
        msg = 'No task registered on results of type %s' % results_cls
        LOG.warning(msg)
        return msg
    
    stage = '%s. ' % get_stage_number(result_obj) if include_stage else ''
    return '%s%s' % (stage,
                     casataskdict.classToCASATask.get(task_cls, task_cls.__name__))


def get_stage_number(result_obj):
    if not isinstance(result_obj, collections.Iterable):
        return get_stage_number([result_obj, ])

    if len(result_obj) is 0:
        msg = 'Cannot get stage number for zero-length results list'
        LOG.error(msg)
        return msg

    return result_obj[0].stage_number


def get_plot_dir(context, stage_number):
    stage_dir = os.path.join(context.report_dir, 'stage%d' % stage_number)
    plots_dir = os.path.join(stage_dir, 'plots')
    return plots_dir


class StdOutFile(StringIO.StringIO):
    def close(self):
        sys.stdout.write(self.getvalue())
        StringIO.StringIO.close(self)


class Session(object):
    def __init__(self, mses=[], name='Unnamed Session'):
        self.mses = mses
        self.name = name

    @staticmethod
    def get_sessions(context):
        d = collections.defaultdict(list)
        for ms in context.observing_run.measurement_sets:
            d[ms.session].append(ms)

        return [Session(v, k) for k, v in d.items()]


class RendererBase(object):
    """
    Base renderer class.
    """
    @classmethod
    def rerender(cls, context):
        LOG.todo('RendererBase: I think I\'m rerendering all pages!')
        return True

    @classmethod
    def get_path(cls, context):
        return os.path.join(context.report_dir, cls.output_file)

    @classmethod
    def get_file(cls, context, hardcopy):
        path = cls.get_path(context)
        file_obj = open(path, 'w') if hardcopy else StdOutFile()
        return contextlib.closing(file_obj)

    @classmethod
    def render(cls, context, hardcopy=False):
        # give the implementing class a chance to bypass rendering. This is
        # useful when the page has not changed, eg. MS description pages when
        # no subsequent ImportData has been performed
        path = cls.get_path(context)
        if os.path.exists(path) and not cls.rerender(context):
            return

        with cls.get_file(context, hardcopy) as fileobj:
            template = TemplateFinder.get_template(cls.template)
            display_context = cls.get_display_context(context)
            fileobj.write(template.render(**display_context))


class TemplateFinder(object):
    _templates_dir = os.path.dirname(templates.__file__)
    _tmp_directory = tempfile.mkdtemp()
    LOG.trace('Mako working directory: %s' % _tmp_directory)
    # Remove temporary Mako codegen directory on termination of Python process
    atexit.register(lambda: shutil.rmtree(TemplateFinder._tmp_directory,
                                          ignore_errors=True))
    _lookup = lookup.TemplateLookup(directories=[_templates_dir],
                                    module_directory=_tmp_directory)

    @staticmethod
    def get_template(filename, **kwargs):
        return TemplateFinder._lookup.get_template(filename)


class T1_1Renderer(RendererBase):
    """
    T1-1 OUS Splash Page renderer
    """
    output_file = 't1-1.html'
    template = 't1-1.html'
    
    
    # named tuple holding values for each row in the main summary table
    TableRow = collections.namedtuple(
                'Tablerow', 
                'ousstatus_entity_id schedblock_id session '
                'execblock_id ms href filesize ' 
                'receivers '
                'num_antennas beamsize_min beamsize_max '
                'time_start time_end time_on_source '
                'baseline_min baseline_max baseline_rms')
    

    @staticmethod
    def get_display_context(context):
        obs_start = context.observing_run.start_datetime
        obs_end = context.observing_run.end_datetime

        project_uids = ', '.join(context.observing_run.project_ids)
        schedblock_uids = ', '.join(context.observing_run.schedblock_ids)
        execblock_uids = ', '.join(context.observing_run.execblock_ids)
        observers = ', '.join(context.observing_run.observers)

        array_names = set([ms.antenna_array.name
                           for ms in context.observing_run.measurement_sets])

        # pipeline execution start, end and duration
        exec_start = context.results[0].timestamps.start
        exec_end = context.results[-1].timestamps.end
        # remove unnecessary precision for execution duration
        dt = exec_end - exec_start
        exec_duration = datetime.timedelta(days=dt.days, seconds=dt.seconds)

#         qaresults = qaadapter.ResultsToQAAdapter(context.results)

        out_fmt = '%Y-%m-%d %H:%M:%S'
        
        
        #Observation Summary (formerly the T1-2 page)
        ms_summary_rows = []
        for ms in context.observing_run.measurement_sets:
            href = os.path.join('t2-1.html?ms=%s' % ms.basename)

            num_antennas = len(ms.antennas)
            # times should be passed as Python datetimes
            time_start = utils.get_epoch_as_datetime(ms.start_time)
            time_start = utils.format_datetime(time_start)
            time_end = utils.get_epoch_as_datetime(ms.end_time)
            time_end = utils.format_datetime(time_end)

            target_scans = [s for s in ms.scans if 'TARGET' in s.intents]
            time_on_source = utils.total_time_on_source(target_scans)
            time_on_source = utils.format_timedelta(time_on_source)
           
            baseline_min = ms.antenna_array.min_baseline.length
            baseline_max = ms.antenna_array.max_baseline.length
            
            # compile a list of primitive numbers representing the baseline 
            # lengths in metres..
            bls = [bl.length.to_units(measures.DistanceUnits.METRE)
                   for bl in ms.antenna_array.baselines]
            # .. so that we can calculate the RMS baseline length with 
            # consistent units
            baseline_rms = math.sqrt(sum(bl**2 for bl in bls)/len(bls))
            baseline_rms = measures.Distance(baseline_rms,
                                             units=measures.DistanceUnits.METRE)
 
            science_spws = ms.get_spectral_windows(science_windows_only=True)
            receivers = sorted(set(spw.band for spw in science_spws))

            row = T1_1Renderer.TableRow(ousstatus_entity_id=context.project_structure.ousstatus_entity_id,
                                        schedblock_id=ms.schedblock_id,
                                        session=ms.session,
                                        execblock_id=ms.execblock_id,
                                        ms=ms.basename,
                                        href=href,
                                        filesize=ms.filesize,
                                        receivers=receivers,                           
                                        num_antennas=num_antennas,
                                        beamsize_min='TODO',
                                        beamsize_max='TODO',
                                        time_start=time_start,
                                        time_end=time_end,
                                        time_on_source=time_on_source,
                                        baseline_min=baseline_min,
                                        baseline_max=baseline_max,
                                        baseline_rms=baseline_rms)
        
            ms_summary_rows.append(row)
        
        
        
        return {'pcontext'          : context,
                'casa_version'      : casadef.casa_version,
                'casa_revision'     : casadef.subversion_revision,
                'pipeline_revision' : pipeline.revision,
                'obs_start'         : obs_start.strftime(out_fmt),
                'obs_end'           : obs_end.strftime(out_fmt),
                'array_names'       : utils.commafy(array_names),
                'exec_start'        : exec_start.strftime(out_fmt),
                'exec_end'          : exec_end.strftime(out_fmt),
                'exec_duration'     : str(exec_duration),
                'project_uids'      : project_uids,
                'schedblock_uids'   : schedblock_uids,
                'execblock_uids'    : execblock_uids,
                'ous_uid'           : context.project_structure.ous_entity_id,
                'ousstatus_entity_id'     : context.project_structure.ousstatus_entity_id,
                'ppr_uid'           : None,
                'observers'         : observers,
#                 'qaadapter'        : qaresults,
                'ms_summary_rows'   : ms_summary_rows}


class T1_2Renderer(RendererBase):
    """
    T1-2 Observation Summary renderer
    """
    output_file = 't1-2.html'
    template = 't1-2.html'

    # named tuple holding values for each row in the main summary table
    TableRow = collections.namedtuple(
                'Tablerow', 
                'ms href filesize ' 
                'receivers '
                'num_antennas beamsize_min beamsize_max '
                'time_start time_end time_on_source '
                'baseline_min baseline_max baseline_rms')

    @staticmethod
    def get_display_context(context):
        ms_summary_rows = []
        for ms in context.observing_run.measurement_sets:
            href = os.path.join('t2-1.html?ms=%s' % ms.basename)

            num_antennas = len(ms.antennas)
            # times should be passed as Python datetimes
            time_start = utils.get_epoch_as_datetime(ms.start_time)
            time_start = utils.format_datetime(time_start)
            time_end = utils.get_epoch_as_datetime(ms.end_time)
            time_end = utils.format_datetime(time_end)

            target_scans = [s for s in ms.scans if 'TARGET' in s.intents]
            time_on_source = utils.total_time_on_source(target_scans)
            time_on_source = utils.format_timedelta(time_on_source)
           
            baseline_min = ms.antenna_array.min_baseline.length
            baseline_max = ms.antenna_array.max_baseline.length
            
            # compile a list of primitive numbers representing the baseline 
            # lengths in metres..
            bls = [bl.length.to_units(measures.DistanceUnits.METRE)
                   for bl in ms.antenna_array.baselines]
            # .. so that we can calculate the RMS baseline length with 
            # consistent units
            baseline_rms = math.sqrt(sum(bl**2 for bl in bls)/len(bls))
            baseline_rms = measures.Distance(baseline_rms,
                                             units=measures.DistanceUnits.METRE)
 
            science_spws = ms.get_spectral_windows(science_windows_only=True)
            receivers = sorted(set(spw.band for spw in science_spws))

            row = T1_2Renderer.TableRow(ms=ms.basename,
                                        href=href,
                                        filesize=ms.filesize,
                                        receivers=receivers,                           
                                        num_antennas=num_antennas,
                                        beamsize_min='TODO',
                                        beamsize_max='TODO',
                                        time_start=time_start,
                                        time_end=time_end,
                                        time_on_source=time_on_source,
                                        baseline_min=baseline_min,
                                        baseline_max=baseline_max,
                                        baseline_rms=baseline_rms)
        
            ms_summary_rows.append(row)
        
        return {'pcontext'        : context,
                'ms_summary_rows' : ms_summary_rows}


class T1_3MRenderer(RendererBase):
    """
    T1-3M renderer
    """
    output_file = 't1-3.html'
    template = 't1-3m.html'
    
    MsgTableRow = collections.namedtuple('MsgTableRow', 'stage task type message')
    
    @classmethod
    def get_display_context(cls, context):
        registry = qaadapter.registry
        # distribute results between topics
        registry.assign_to_topics(context.results)

        scores = {}
        tablerows = []
        results_list = []
        flagtables = {}
        for result in context.results:
            scores[result.stage_number] = result.qa.representative
            results_list = result
        
            qa_errors = cls._filter_qascores(results_list, -0.1, 0.1)
            tablerows.extend(cls._qascores_to_tablerows(qa_errors,
                                                            results_list,
                                                            'QA Error'))
        
            qa_warnings = cls._filter_qascores(results_list, 0.1, 0.5)
            tablerows.extend(cls._qascores_to_tablerows(qa_warnings,
                                                            results_list,
                                                            'QA Warning'))

            error_msgs = utils.get_logrecords(results_list, logging.ERROR)
            tablerows.extend(cls._logrecords_to_tablerows(error_msgs,
                                                              results_list,
                                                              'Error'))

            warning_msgs = utils.get_logrecords(results_list, logging.WARNING)
            tablerows.extend(cls._logrecords_to_tablerows(warning_msgs,
                                                              results_list,
                                                              'Warning'))
            
            if 'applycal' in get_task_description(result):
                try:
                    for resultitem in result:
                        ms_name = os.path.basename(resultitem.inputs['vis'])
                        ms = [i for i in context.observing_run.get_measurement_sets() if ms_name in i.name][0]
                        flagtable = {}
			for field in resultitem.flagsummary.keys():
			    #each field
			    intents = ','.join([f.intents for f in ms.get_fields(intent='BANDPASS,PHASE,AMPLITUDE,CHECK,TARGET') if field in f.name][0])
			    
			    flagsummary = resultitem.flagsummary[field]
			
			    fieldtable = {}
			    for k,v in flagsummary.iteritems():
				myname = v['name']
				myspw = v['spw']
				myant = v['antenna']
				antkeys = myant.keys()
				spwkey = myspw.keys()[0]
			    
				fieldtable.update({myname:{spwkey:myant}})
				
			    flagtable['Source name: '+ field + ', Intents: ' + intents] = fieldtable
			
			flagtables[ms_name] = flagtable
			
                except:
                    LOG.debug("No flag summary table available yet from applycal")
                 

        return {'pcontext' : context,
                'registry' : registry,
                'scores'   : scores,
                'tablerows': tablerows,
                'flagtables': flagtables}


    @classmethod
    def _filter_qascores(cls, results_list, lo, hi):
        qa_pool = results_list.qa.pool
        #print qa_pool
        with_score = [s for s in qa_pool if s.score not in ('', 'N/A', None)]
        return [s for s in with_score if s.score > lo and s.score <= hi]

    @classmethod
    def _create_tablerow(cls, results, message, msgtype):
        return cls.MsgTableRow(stage=results.stage_number,
                               task=get_task_name(results, False),
                               type=msgtype,
                               message=message)

    @classmethod
    def _qascores_to_tablerows(cls, qascores, results, msgtype='ERROR'):
        return [cls._create_tablerow(results, qascore.longmsg, msgtype)
                for qascore in qascores]

    @classmethod
    def _logrecords_to_tablerows(cls, records, results, msgtype='ERROR'):
        return [cls._create_tablerow(results, record.msg, msgtype)
                for record in records]

class T1_4MRenderer(RendererBase):
    """
    T1-4M renderer
    """
    output_file = 't1-4.html'
    # TODO get template at run-time
    template = 't1-4m.html'

    @staticmethod
    def get_display_context(context):
        scores = {}
        for result in context.results:
            scores[result.stage_number] = result.qa.representative
                
        return {'pcontext' : context,
                'results'  : context.results,
                'scores'   : scores}
        

class T2_1Renderer(RendererBase):
    """
    T2-4M renderer
    """
    output_file = 't2-1.html'
    template = 't2-1.html'

    @staticmethod
    def get_display_context(context):
        sessions = Session.get_sessions(context)
        return {'pcontext' : context,
                'sessions' : sessions}


class T2_1DetailsRenderer(object):
    output_file = 't2-1_details.html'
    template = 't2-1_details.html'

    @classmethod
    def get_file(cls, context, session, ms, hardcopy):
        ms_dir = os.path.join(context.report_dir, 
                              'session%s' % session.name,
                              ms.basename)
        if hardcopy and not os.path.exists(ms_dir):
            os.makedirs(ms_dir)
        filename = os.path.join(ms_dir, cls.output_file)
        file_obj = open(filename, 'w') if hardcopy else StdOutFile()
        return contextlib.closing(file_obj)

    @staticmethod
    def write_listobs(context, ms):
        listfile = os.path.join(context.report_dir, 
                                'session%s' % ms.session,
                                ms.basename,
                                'listobs.txt')

        if not os.path.exists(listfile):
            LOG.debug('Writing listobs output to %s' % listfile)
            task = infrastructure.casa_tasks.listobs(vis=ms.name,
                                                     listfile=listfile)
            task.execute(dry_run=False)

    @staticmethod
    def get_display_context(context, ms):
        T2_1DetailsRenderer.write_listobs(context, ms)
        
        inputs = summary.IntentVsTimeChart.Inputs(context, vis=ms.basename)
        task = summary.IntentVsTimeChart(inputs)
        intent_vs_time = task.plot()

        inputs = summary.FieldVsTimeChart.Inputs(context, vis=ms.basename)
        task = summary.FieldVsTimeChart(inputs)
        field_vs_time = task.plot()

        science_spws = ms.get_spectral_windows(science_windows_only=True)
        all_bands = sorted(set([spw.band for spw in ms.spectral_windows]))
        science_bands = sorted(set([spw.band for spw in science_spws]))
        
        science_sources = sorted(set([source.name for source in ms.sources 
                                      if 'TARGET' in source.intents]))

        calibrators = sorted(set([source.name for source in ms.sources 
                                  if 'TARGET' not in source.intents]))

        baseline_min = ms.antenna_array.min_baseline.length
        baseline_max = ms.antenna_array.max_baseline.length

        time_start = utils.get_epoch_as_datetime(ms.start_time)
        time_end = utils.get_epoch_as_datetime(ms.end_time)

        time_on_source = utils.total_time_on_source(ms.scans) 
        science_scans = [scan for scan in ms.scans if 'TARGET' in scan.intents]
        time_on_science = utils.total_time_on_source(science_scans)
        
#         dirname = os.path.join(context.report_dir, 
#                                'session%s' % ms.session,
#                                ms.basename)

        task = summary.WeatherChart(context, ms)
        weather_plot = task.plot()


        dirname = os.path.join('session%s' % ms.session,
                               ms.basename)
        
        return {
            'pcontext'        : context,
            'ms'              : ms,
            'science_sources' : utils.commafy(science_sources),
            'calibrators'     : utils.commafy(calibrators),
            'all_bands'       : utils.commafy(all_bands),
            'science_bands'   : utils.commafy(science_bands),
            'baseline_min'    : baseline_min,
            'baseline_max'    : baseline_max,
            'time_start'      : utils.format_datetime(time_start),
            'time_end'        : utils.format_datetime(time_end),
            'time_on_source'  : utils.format_timedelta(time_on_source),
            'time_on_science' : utils.format_timedelta(time_on_science),
            'intent_vs_time'  : intent_vs_time,
            'field_vs_time'   : field_vs_time,
            'dirname'         : dirname,
            'weather_plot'    : weather_plot
        }

    @classmethod
    def render(cls, context, hardcopy=False):
        for session in Session.get_sessions(context):
            for ms in session.mses:
                with cls.get_file(context, session, ms, hardcopy) as fileobj:
                    template = TemplateFinder.get_template(cls.template)
                    display_context = cls.get_display_context(context, ms)
                    fileobj.write(template.render(**display_context))


class MetadataRendererBase(RendererBase):
    @classmethod
    def rerender(cls, context):
        # TODO: only rerender when a new ImportData result is queued
        if cls in DEBUG_CLASSES:
            LOG.warning('Always rerendering %s' % cls.__name__)
            return True
        return False


class T2_2_XRendererBase(object):
    """
    Base renderer for T2-2-X series of pages.
    """
    @classmethod
    def get_file(cls, filename):
        ms_dir = os.path.dirname(filename)

        if not os.path.exists(ms_dir):
            os.makedirs(ms_dir)

        file_obj = open(filename, 'w')
        return contextlib.closing(file_obj)

    @classmethod
    def get_filename(cls, context, ms):
        return os.path.join(context.report_dir,
                            'session%s' % ms.session,
                            ms.basename,
                            cls.output_file)

    @classmethod
    def render(cls, context, hardcopy=False):
        for ms in context.observing_run.measurement_sets:
            filename = cls.get_filename(context, ms)
            # now that the details pages are written per MS rather than having
            # tabs for each MS, we don't need to write them each time as
            # importdata will not affect their content.
            if not os.path.exists(filename):
                with cls.get_file(filename) as fileobj:
                    template = TemplateFinder.get_template(cls.template)
                    display_context = cls.get_display_context(context, ms)
                    fileobj.write(template.render(**display_context))


class T2_2_1Renderer(T2_2_XRendererBase):
    """
    T2-2-1 renderer - spatial setup
    """
    output_file = 't2-2-1.html'
    template = 't2-2-1.html'

    @staticmethod
    def get_display_context(context, ms):
        mosaics = []
        for source in ms.sources:
            num_pointings = len([f for f in ms.fields 
                                 if f.source_id == source.id])
            if num_pointings > 1:
                task = summary.MosaicChart(context, ms, source)
                mosaics.append((source, task.plot()))

        return {'pcontext' : context,
                'ms'       : ms,
                'mosaics'  : mosaics}


class T2_2_2Renderer(T2_2_XRendererBase):
    """
    T2-2-2 renderer
    """
    output_file = 't2-2-2.html'
    template = 't2-2-2.html'

    @staticmethod
    def get_display_context(context, ms):
        return {'pcontext' : context,
                'ms'       : ms}


class T2_2_3Renderer(T2_2_XRendererBase):
    """
    T2-2-3 renderer
    """
    output_file = 't2-2-3.html'
    template = 't2-2-3.html'

    @staticmethod
    def get_display_context(context, ms):
        task = summary.PlotAntsChart2(context, ms)
        plot_ants = task.plot()

        dirname = os.path.join('session%s' % ms.session,
                               ms.basename)
        
        return {'pcontext'  : context,
                'plot_ants' : plot_ants,
                'ms'        : ms,
                'dirname'   : dirname}


class T2_2_4Renderer(T2_2_XRendererBase):
    """
    T2-2-4 renderer
    """
    output_file = 't2-2-4.html'
    template = 't2-2-4.html'

    @staticmethod
    def get_display_context(context, ms):
        task = summary.AzElChart(context, ms)
        azel_plot = task.plot()

        task = summary.ElVsTimeChart(context, ms)
        el_vs_time_plot = task.plot()

        dirname = os.path.join('session%s' % ms.session,
                               ms.basename)

        return {'pcontext'        : context,
                'ms'              : ms,
                'azel_plot'       : azel_plot,
                'el_vs_time_plot' : el_vs_time_plot,
                'dirname'         : dirname}


class T2_2_5Renderer(T2_2_XRendererBase):
    """
    T2-2-5 renderer - weather page
    """
    output_file = 't2-2-5.html'
    template = 't2-2-5.html'

    @staticmethod
    def get_display_context(context, ms):
        task = summary.WeatherChart(context, ms)
        weather_plot = task.plot()
        dirname = os.path.join('session%s' % ms.session,
                               ms.basename)

        return {'pcontext'     : context,
                'ms'           : ms,
                'weather_plot' : weather_plot,
                'dirname'      : dirname}


class T2_2_6Renderer(T2_2_XRendererBase):
    """
    T2-2-6 renderer - scans page
    """
    output_file = 't2-2-6.html'
    template = 't2-2-6.html'

    TableRow = collections.namedtuple(
        'TableRow', 
        'id time_start time_end duration intents fields spws'
    )

    @staticmethod
    def get_display_context(context, ms):
        tablerows = []
        for scan in ms.scans:
            scan_id = scan.id
            epoch_start = utils.get_epoch_as_datetime(scan.start_time)
            time_start = utils.format_datetime(epoch_start)
            epoch_end = utils.get_epoch_as_datetime(scan.end_time)
            time_end = utils.format_datetime(epoch_end)
            duration = utils.format_timedelta(scan.time_on_source)
            intents = sorted(scan.intents)
            fields = utils.commafy(sorted([f.name for f in scan.fields]))
            
            spw_ids = sorted([spw.id for spw in scan.spws])
            spws = ', '.join([str(spw_id) for spw_id in spw_ids])

            row = T2_2_6Renderer.TableRow(
                id=scan_id,
                time_start=time_start,
                time_end=time_end,
                duration=duration,
                intents=intents,
                fields=fields,
                spws=spws
            )

            tablerows.append(row)
            
        return {'pcontext'     : context,
                'ms'           : ms,
                'tablerows'    : tablerows}


class T2_3_XMBaseRenderer(RendererBase):
    # the filename to which output will be directed
    output_file = 'overrideme'
    # the template file for this renderer
    template = 'overrideme'

    MsgTableRow = collections.namedtuple('MsgTableRow', 'stage task type message')

    @classmethod
    def get_display_context(cls, context):
        topic = cls.get_topic()

        scores = {}
        for result in context.results:
            scores[result.stage_number] = result.qa.representative

        tablerows = []
        for list_of_results_lists in topic.results_by_type.values():
            if not list_of_results_lists:
                continue
            
            for results_list in list_of_results_lists:
                qa_errors = cls._filter_qascores(results_list, -0.1, 0.1)
                tablerows.extend(cls._qascores_to_tablerows(qa_errors,
                                                            results_list,
                                                            'QA Error'))
                    
                qa_warnings = cls._filter_qascores(results_list, 0.1, 0.5)
                tablerows.extend(cls._qascores_to_tablerows(qa_warnings,
                                                            results_list,
                                                            'QA Warning'))

                error_msgs = utils.get_logrecords(results_list, logging.ERROR)
                tablerows.extend(cls._logrecords_to_tablerows(error_msgs,
                                                              results_list,
                                                              'Error'))

                warning_msgs = utils.get_logrecords(results_list, logging.WARNING)
                tablerows.extend(cls._logrecords_to_tablerows(warning_msgs,
                                                              results_list,
                                                              'Warning'))
                
        return {'pcontext'  : context,
                'scores'    : scores,
                'tablerows' : tablerows,
                'topic'     : topic     }
                
    @classmethod
    def _filter_qascores(cls, results_list, lo, hi):
        qa_pool = results_list.qa.pool
        with_score = [s for s in qa_pool if s.score not in ('', 'N/A', None)]
        return [s for s in with_score if s.score > lo and s.score <= hi]

    @classmethod
    def _create_tablerow(cls, results, message, msgtype):
        return cls.MsgTableRow(stage=results.stage_number,
                               task=get_task_name(results, False),
                               type=msgtype,
                               message=message)

    @classmethod
    def _qascores_to_tablerows(cls, qascores, results, msgtype='ERROR'):
        return [cls._create_tablerow(results, qascore.longmsg, msgtype)
                for qascore in qascores]

    @classmethod
    def _logrecords_to_tablerows(cls, records, results, msgtype='ERROR'):
        return [cls._create_tablerow(results, record.msg, msgtype)
                for record in records]


class T2_3_1MRenderer(T2_3_XMBaseRenderer):
    """
    Renderer for T2-3-1M, the data set topic.
    """
    # the filename to which output will be directed
    output_file = 't2-3-1m.html'
    # the template file for this renderer
    template = 't2-3-1m.html'

    @classmethod
    def get_topic(cls):
        return qaadapter.registry.get_dataset_topic()        


class T2_3_2MRenderer(T2_3_XMBaseRenderer):
    """
    Renderer for T2-3-2M: the QA calibration section.
    """
    # the filename to which output will be directed
    output_file = 't2-3-2m.html'
    # the template file for this renderer
    template = 't2-3-2m.html'

    @classmethod
    def get_topic(cls):
        return qaadapter.registry.get_calibration_topic()        


class T2_3_3MRenderer(T2_3_XMBaseRenderer):
    """
    Renderer for T2-3-3M: the QA flagging section.
    """
    # the filename to which output will be directed
    output_file = 't2-3-3m.html'
    # the template file for this renderer
    template = 't2-3-3m.html'

    @classmethod
    def get_topic(cls):
        return qaadapter.registry.get_flagging_topic()        


class T2_3_4MRenderer(T2_3_XMBaseRenderer):
    """
    Renderer for T2-3-4M: the QA line finding section.
    """
    # the filename to which output will be directed
    output_file = 't2-3-4m.html'
    # the template file for this renderer
    template = 't2-3-4m.html'

    @classmethod
    def get_topic(cls):
        return qaadapter.registry.get_linefinding_topic()        


class T2_3_5MRenderer(T2_3_XMBaseRenderer):
    """
    Renderer for T2-3-5M: the imaging topic
    """
    # the filename to which output will be directed
    output_file = 't2-3-5m.html'
    # the template file for this renderer
    template = 't2-3-5m.html'

    @classmethod
    def get_topic(cls):
        return qaadapter.registry.get_imaging_topic()        


class T2_3_6MRenderer(T2_3_XMBaseRenderer):
    """
    Renderer for T2-3-6M: the miscellaneous topic
    """
    # the filename to which output will be directed
    output_file = 't2-3-6m.html'
    # the template file for this renderer
    template = 't2-3-6m.html'

    @classmethod
    def get_topic(cls):
        return qaadapter.registry.get_miscellaneous_topic()        


class T2_3MDetailsDefaultRenderer(object):
    def __init__(self, template='t2-3m_details.html', always_rerender=False):
        self.template = template
        self.always_rerender = always_rerender
        
    def get_display_context(self, context, result):
        return {'pcontext' : context,
                'result'   : result}

    def render(self, context, result):
        display_context = self.get_display_context(context, result)
        t = TemplateFinder.get_template(self.template)
        return t.render(**display_context)


class T2_3MDetailsRenderer(object):
    # the filename component of the output file. While this is the same for
    # all results, the directory is stage-specific, so there's no risk of
    # collisions  
    output_file = 't2-3m_details.html'
    
    # the default renderer used should the task:renderer mapping not specify a
    # specialised renderer
    _default_renderer = T2_3MDetailsDefaultRenderer()

    """
    Get the file object for this renderer.

    The returned file object will be either direct output to a file or to
    stdout, depending on whether hardcopy is set to true or false
    respectively. 

    :param context: the pipeline Context
    :type context: :class:`~pipeline.infrastructure.launcher.Context`
    :param result: the task results object to render
    :type result: :class:`~pipeline.infrastructure.api.Result`
    :param hardcopy: render to disk (true) or to stdout (false). Default is
        true
    :type hardcopy: boolean
    :rtype: a file object
    """
    @classmethod
    def get_file(cls, context, result, hardcopy):
        # construct the relative filename, eg. 'stageX/t2-3m_details.html'
        path = cls.get_path(context, result)

        # to avoid any subsequent file not found errors, create the directory
        # if a hard copy is requested and the directory is missing
        stage_dir = os.path.dirname(path)
        if hardcopy and not os.path.exists(stage_dir):
            os.makedirs(stage_dir)
        
        # create a file object that writes to a file if a hard copy is 
        # requested, otherwise return a file object that flushes to stdout
        file_obj = open(path, 'w') if hardcopy else StdOutFile()
        
        # return the file object wrapped in a context manager, so we can use
        # it with the autoclosing 'with fileobj as f:' construct
        return contextlib.closing(file_obj)

    """
    Get the path to which the template will be written.
    
    :param context: the pipeline Context
    :type context: :class:`~pipeline.infrastructure.launcher.Context`
    :param result: the task results object to render
    :type result: :class:`~pipeline.infrastructure.api.Result`
    :rtype: string
    """
    @classmethod
    def get_path(cls, context, result):
        # HTML output will be written to the directory 'stageX' 
        stage = 'stage%s' % result.stage_number
        stage_dir = os.path.join(context.report_dir, stage)

        # construct the relative filename, eg. 'stageX/t2-4m_details.html'
        return os.path.join(stage_dir, cls.output_file)

    """
    Render the detailed QA perspective of each Results in the given context.
    
    This renderer creates detailed T2_3M output for each Results. Each Results
    in the context is passed to a specialised renderer, which generates
    custom output and plots for the Result in question.
    
    :param context: the pipeline Context
    :type context: :class:`~pipeline.infrastructure.launcher.Context`
    :param hardcopy: render to disk (true) or to stdout (false). Default is true.
    :type hardcopy: boolean
    """
    @classmethod
    def render(cls, context, hardcopy=False):
        # get the map of t2_3m renderers from the dictionary
        t2_3m_renderers = renderer_map[T2_3MDetailsRenderer]
        
        # for each result accepted and stored in the context..
        for result in context.results:
            # we only handle lists of results, so wrap single objects in a
            # list if necessary
            if not isinstance(result, collections.Iterable):
                l = basetask.ResultsList()
                l.append(result)
                l.timestamps = result.timestamps
                l.inputs = result.inputs
                l.stage_number = result.stage_number
                result = l
            task = result[0].task

            # find the renderer appropriate to the task..
            renderer = t2_3m_renderers.get(task, cls._default_renderer)
            LOG.trace('Using %s to render %s result' % (
                renderer.__class__.__name__, task.__name__))
            
            # details pages do not need to be updated once written
            path = cls.get_path(context, result)
            force_rerender = getattr(renderer, 'always_rerender', False)
            if os.path.exists(path) and not force_rerender:
                continue
            
            # .. get the file object to which we'll render the result
            with cls.get_file(context, result, hardcopy) as fileobj:
                # .. and write the renderer's interpretation of this result to
                # the file object  
                fileobj.write(renderer.render(context, result))


class T2_3MDetailsWvrgcalflagRenderer(T2_3MDetailsDefaultRenderer):
    """
    T2_43DetailsBandpassRenderer generates the QA output specific to the
    Wvrgcalflag task.
    """
    
    def __init__(self, template='t2-3m_details-wvrgcalflag.html',
                 always_rerender=False):
        # set the name of our specialised Mako template via the superclass
        # constructor 
        super(T2_3MDetailsWvrgcalflagRenderer, self).__init__(template,
                                                              always_rerender)

    """
    Get the Mako context appropriate to the results created by a Bandpass
    task.
    
    :param context: the pipeline Context
    :type context: :class:`~pipeline.infrastructure.launcher.Context`
    :param results: the Wvrgcalflag results to describe
    :type results: 
        :class:`~pipeline.infrastructure.tasks.wvrgcal.resultobjects.WvrgcalflagResults`
    :rtype a dictionary that can be passed to the matching bandpass Mako 
        template
    """
    def get_display_context(self, context, results):
        # get the standard Mako context from the superclass implementation 
        super_cls = super(T2_3MDetailsWvrgcalflagRenderer, self)
        ctx = super_cls.get_display_context(context, results)

        plots_dir = os.path.join(context.report_dir, 
                                 'stage%d' % results.stage_number)
        if not os.path.exists(plots_dir):
            os.mkdir(plots_dir)

        plotter = image.ImageDisplay()
        plots = []
        for result in results:
            if result.qa_wvr.view:
                plot = plotter.plot(context, result.qa_wvr, reportdir=plots_dir, 
                                    prefix='qa', change='WVR')
                plots.append(plot)
    
        # Group the Plots by axes and plot types; each logical grouping will
        # be contained in a PlotGroup  
        plot_groups = logger.PlotGroup.create_plot_groups(plots)
        for plot_group in plot_groups:
            # Write the thumbnail pages for each plot grouping to disk 
            renderer = PlotGroupRenderer(context, results, plot_group, 'qa')
            plot_group.filename = renderer.filename
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())

        # add the PlotGroups to the Mako context. The Mako template will parse
        # these objects in order to create links to the thumbnail pages we
        # just created
        ctx.update({'plot_groups' : plot_groups})
        return ctx


class T2_3MDetailsBandpassRenderer(T2_3MDetailsDefaultRenderer):
    """
    T2_43DetailsBandpassRenderer generates the QA output specific to the
    bandpass task.
    """
    
    def __init__(self, template='t2-3m_details-bandpass.html',
                 always_rerender=False):
        # set the name of our specialised Mako template via the superclass
        # constructor 
        super(T2_3MDetailsBandpassRenderer, self).__init__(template,
                                                           always_rerender)

    def get_display_context(self, context, results):
        """
        Get the Mako context appropriate to the results created by a Bandpass
        task.

        :param context: the pipeline Context
        :type context: :class:`~pipeline.infrastructure.launcher.Context`
        :param results: the bandpass results to describe
        :type results:
            :class:`~pipeline.infrastructure.tasks.bandpass.common.BandpassResults`
        :rtype a dictionary that can be passed to the matching bandpass Mako
            template
        """
        # get the standard Mako context from the superclass implementation
        super_cls = super(T2_3MDetailsBandpassRenderer, self)
        ctx = super_cls.get_display_context(context, results)

        plots = []
        flagged = []
        num_flagged_feeds = 0
        for result in results:
            # return early if there are no QA results
            if not result.qa:
                return ctx
            adapter = qaadapter.QABandpassAdapter(context, result)
            plots.append(adapter.amplitude_plots)
            plots.append(adapter.phase_plots)
            
            for antenna, feeds in adapter.flagged_feeds.items():
                flagged.append((os.path.basename(result.inputs['vis']),
                                antenna.identifier,
                                ', '.join(feeds)))
                num_flagged_feeds += len(feeds)

        # Group the Plots by axes and plot types; each logical grouping will
        # be contained in a PlotGroup  
        plot_groups = logger.PlotGroup.create_plot_groups(plots)
        # Write the thumbnail pages for each plot grouping to disk 
        for plot_group in plot_groups:
            renderer = QAPlotRenderer(context, results, plot_group, 'qa')
            plot_group.filename = renderer.filename 
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())

        # add the PlotGroups to the Mako context. The Mako template will parse
        # these objects in order to create links to the thumbnail pages we
        # just created
        ctx.update({'plot_groups'       : plot_groups,
                    'flagged'           : flagged,
                    'num_flagged_feeds' : num_flagged_feeds})

        return ctx


class T2_4MRenderer(RendererBase):
    """
    T2-4M renderer
    """
    output_file = 't2-4m.html'
    template = 't2-4m.html'

    @staticmethod
    def get_display_context(context):
        return {'pcontext' : context,
                'results'  : context.results }


class T2_4MDetailsDefaultRenderer(object):
    def __init__(self, template='t2-4m_details-generic.html', 
                 always_rerender=False):
        self.template = template
        self.always_rerender = always_rerender

    def get_display_context(self, context, result):
        return {'pcontext' : context,
                'result'   : result,
                'stagelog' : self._get_stagelog(context, result),
                'taskhelp' : self._get_help(context, result),
                'dirname'  : 'stage%s' % result.stage_number}

    def render(self, context, result):
        display_context = self.get_display_context(context, result)
        t = TemplateFinder.get_template(self.template)
        return t.render(**display_context)

    def _get_stagelog(self, context, result):
        """
        Read in the CASA log extracts from the file in the stage directory.
        """
        stagelog_path = os.path.join(context.report_dir,
                                     'stage%s' % result.stage_number,
                                     'casapy.log')

        if not os.path.exists(stagelog_path):
            return None
        
        with open(stagelog_path, 'r') as f:
            return ''.join([l.expandtabs() for l in f.readlines()])

    def _get_help(self, context, result):
        try:
            # get hif-prefixed taskname from the result from which we can
            # retrieve the XML documentation, otherwise fall back to the
            # Python class documentation              
            taskname = getattr(result, 'taskname', result[0].task)

            obj, _ = pydoc.resolve(taskname, forceload=0)
            page = pydoc.render_doc(obj)
            return '<pre>%s</pre>' % re.sub('\x08.', '', page)
        except Exception:
            return None


class CleanPlotsRenderer(object):
    template = 'cleanplots.html'

    def __init__(self, context, result, plots_dict, field, spw, pol):
        self.context = context
        self.result = result
        self.plots_dict = plots_dict
        self.field = field
        self.spw = spw
        self.pol = pol
        
    @property
    def dirname(self):
        stage = 'stage%s' % self.result.stage_number
        return os.path.join(self.context.report_dir, stage)
    
    @property
    def filename(self):
        filename = 'field%s-spw%s-pol%s-cleanplots.html' % (
          self.field, 
          self.spw,
          self.pol)
        filename = filenamer.sanitize(filename)
        return filename
    
    @property
    def path(self):
        return os.path.join(self.dirname, self.filename)
    
    def get_file(self, hardcopy=True):
        if hardcopy and not os.path.exists(self.dirname):
            os.makedirs(self.dirname)
        file_obj = open(self.path, 'w') if hardcopy else StdOutFile()
        return contextlib.closing(file_obj)
    
    def _get_display_context(self):
        return {'pcontext'   : self.context,
                'result'     : self.result,
                'plots_dict' : self.plots_dict,
                'dirname'    : self.dirname,
                'field'      : self.field,
                'spw'        : self.spw}

    def render(self):
        display_context = self._get_display_context()
        t = TemplateFinder.get_template(self.template)
        return t.render(**display_context)


class PlotGroupRenderer(object):
    template = 'plotgroup.html'

    def __init__(self, context, result, plot_group, prefix=''):
        self.context = context
        self.result = result
        self.plot_group = plot_group
        self.prefix = prefix
        if self.prefix != '':
            self.prefix = '%s-' % prefix
        
    @property
    def dirname(self):
        stage = 'stage%s' % self.result.stage_number
        return os.path.join(self.context.report_dir, stage)
    
    @property
    def filename(self):
        filename = '%s%s-%s-thumbnails.html' % (self.plot_group.x_axis, 
                                                self.plot_group.y_axis,
                                                self.prefix)
        filename = filenamer.sanitize(filename)
        return filename
    
    @property
    def path(self):
        return os.path.join(self.dirname, self.filename)
    
    def get_file(self, hardcopy=True):
        if hardcopy and not os.path.exists(self.dirname):
            os.makedirs(self.dirname)
            
        file_obj = open(self.path, 'w') if hardcopy else StdOutFile()
        return contextlib.closing(file_obj)
    
    def _get_display_context(self):
        return {'pcontext'   : self.context,
                'result'     : self.result,
                'plot_group' : self.plot_group,
                'dirname'    : self.dirname}

    def render(self):
        display_context = self._get_display_context()
        t = TemplateFinder.get_template(self.template)
        return t.render(**display_context)


class QAPlotRenderer(PlotGroupRenderer):
    template = 'qa.html'

    def __init__(self, context, result, plot_group, prefix=''):
        super(QAPlotRenderer, self).__init__(context, result, plot_group, 
                                                   prefix)
        json_path = os.path.join(context.report_dir, 
                                 'stage%s' % result.stage_number, 
                                 'qa.json')
        
        if os.path.exists(json_path):
            with open(json_path, 'r') as json_file:
                self.json = json_file.readlines()[0]
        else:
            self.json = '{}'
         
    def _get_display_context(self):
        ctx = super(QAPlotRenderer, self)._get_display_context()
        ctx.update({'json' : self.json});
        return ctx


class T2_4MDetailsWvrgcalflagRenderer(T2_4MDetailsDefaultRenderer):
    """
    T2_4MDetailsWvrgcalflagRenderer generates the detailed T2_4M-level plots
    and output specific to the wvrgcalflag task.
    """
    
    WvrApplication = collections.namedtuple('WvrApplication', 
                                            'ms gaintable interpolated applied') 
    
    def __init__(self, template='t2-4m_details-hif_wvrgcalflag.html', 
                 always_rerender=False):
        # set the name of our specialised Mako template via the superclass
        # constructor 
        super(T2_4MDetailsWvrgcalflagRenderer, self).__init__(template,
                                                              always_rerender)

    """
    Get the Mako context appropriate to the results created by a Wvrgcalflag
    task.
    
    :param context: the pipeline Context
    :type context: :class:`~pipeline.infrastructure.launcher.Context`
    :param results: the bandpass results to describe
    :type results: 
        :class:`~pipeline.infrastructure.tasks.wvrgcalflag.resultobjects.WvrgcalflagResults`
    :rtype a dictionary that can be passed to the matching Mako template
    """
    def get_display_context(self, context, results):
        # get the standard Mako context from the superclass implementation 
        super_cls = super(T2_4MDetailsWvrgcalflagRenderer, self)
        ctx = super_cls.get_display_context(context, results)

        plots_dir = os.path.join(context.report_dir, 
                                 'stage%d' % results.stage_number)
        if not os.path.exists(plots_dir):
            os.mkdir(plots_dir)

        applications = []
        flag_plots = {}
        metric_plots = {}
        phase_offset_summary_plots = {}
        baseline_summary_plots = {}
        wvrinfos = {}
        ms_non12 = []
        for result in results:
            vis = os.path.basename(result.inputs['vis'])
            ms = context.observing_run.get_ms(vis)

            # if there's no WVR data, the pool will be empty
            if not result.pool:
                # check if this MS is all 7m data. 
                if all([a for a in ms.antennas if a.diameter != 12.0]):
                    ms_non12.append(os.path.basename(vis))
                continue

            applications.extend(self.get_wvr_applications(result))
            try:
                wvrinfos[vis] = result.wvr_infos
            except:
                pass

            # collect flagging metric plots from result
            if result.qa_wvr.view:
                plotter = image.ImageDisplay()
                plots = plotter.plot(context, result.qa_wvr, reportdir=plots_dir, 
                                     prefix='qa', change='WVR')
                plots = sorted(plots, key=lambda p: int(p.parameters['spw']))
                
                # render flagging metric plots to their own HTML page
                renderer = WvrcalflagMetricPlotsRenderer(context, result, plots)
                with renderer.get_file() as fileobj:
                    fileobj.write(renderer.render())        
    
                metric_plots[vis] = plots[0]

            if result.view:
                flag_plotter = image.ImageDisplay()
                plots = flag_plotter.plot(context, result, reportdir=plots_dir, 
                                          prefix='flag', change='Flagging')
                # sort plots by spw
                flag_plots[vis] = sorted(plots, 
                                         key=lambda p: int(p.parameters['spw']))

            # generate the phase offset summary plots
            phase_offset_summary_plotter = wvr.WVRPhaseOffsetSummaryPlot(context, result)
            phase_offset_summary_plots[vis] = phase_offset_summary_plotter.plot()

            # generate the per-antenna phase offset plots
            phase_offset_plotter = wvr.WVRPhaseOffsetPlot(context, result)            
            phase_offset_plots = phase_offset_plotter.plot() 
            # write the html for each MS to disk
            renderer = WvrgcalflagPhaseOffsetPlotRenderer(context, result, 
                                                          phase_offset_plots)
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())        

            baseline_plotter = wvr.WVRPhaseVsBaselineChart(context, result)
            baseline_plots = baseline_plotter.plot()
            # write the html for each MS to disk
            renderer = WvrgcalflagPhaseOffsetVsBaselinePlotRenderer(context, result, 
                                                                    baseline_plots)

            # get the first scan for the QA intent(s)
            qa_intent = set(result.inputs['qa_intent'].split(','))
            qa_scan = sorted([scan.id for scan in ms.scans 
                               if not qa_intent.isdisjoint(scan.intents)])[0]                               
            # scan parameter on plot is comma-separated string 
            qa_scan = str(qa_scan)            
            LOG.trace('Using scan %s for phase vs baseline summary '
                      'plots' % qa_scan)
            baseline_summary_plots[vis] = [p for p in baseline_plots
                                           if qa_scan in set(p.parameters['scan'].split(','))]
            
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())        

        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % results.stage_number)

        # add the PlotGroups to the Mako context. The Mako template will parse
        # these objects in order to create links to the thumbnail pages we
        # just created
        ctx.update({'applications' : applications,
                    'wvrinfos'     : wvrinfos,
                    'flag_plots' : flag_plots,
                    'metric_plots' : metric_plots,
                    'phase_offset_summary_plots' : phase_offset_summary_plots,
                    'baseline_summary_plots' : baseline_summary_plots,
                    'dirname' : weblog_dir})

        # tell the user not to panic for non-12m dishes missing WVR 
        if ms_non12:
            msg = ('WVR data are not expected for %s, which %s not observed '
                   'using 12m antennas.'
                   '' % (utils.commafy(ms_non12, quotes=False, conjunction='or'),
                         'was' if len(ms_non12) is 1 else 'were'))
            ctx['alerts_info'] = [msg,]

        return ctx

        # Phase vs time for the overview plot should be for the widest window
        # at the highest frequency
#         spws = sorted(ms.spectral_windows, 
#                       key=operator.attrgetter('bandwidth', 'centre_frequency'))
#         overview_spw = spws[-1]

    def get_wvr_applications(self, result):
        applications = []

        interpolated = utils.commafy(result.wvrflag, False)

        # define a closure that adds a wvrapplication for each calapplication
        # unless the applications list already contains one for that 
        # ms/caltable combination
        def collect(calapps, accept):
            for calapp in calapps:
                ms = os.path.basename(calapp.vis)
                gaintable = os.path.basename(calapp.gaintable)
    
                a = T2_4MDetailsWvrgcalflagRenderer.WvrApplication(ms, 
                                                                   gaintable, 
                                                                   interpolated,
                                                                   accept)            
    
                if not any([r for r in applications 
                            if r.ms == ms and r.gaintable == gaintable]):
                    applications.append(a)

        collect(result.final, True)
        collect(result.pool, False)

        return applications


class WvrcalflagMetricPlotsRenderer(object):
    # take a look at WvrgcalflagPhaseOffsetVsBaselinePlotRenderer when we have
    # scores and histograms to generate. there should be a common base class. 
    template = 't2-4m_details-hif_wvrgcalflag-metric_view.html'

    def __init__(self, context, result, plots):
        self.context = context
        self.result = result
        self.plots = plots
        self.ms = os.path.basename(self.result.inputs['vis'])

        # all values set on this dictionary will be written to the JSON file
        d = {}
        for plot in plots:
            # calculate the relative pathnames as seen from the browser
            thumbnail_relpath = os.path.relpath(plot.thumbnail,
                                                self.context.report_dir)
            image_relpath = os.path.relpath(plot.abspath,
                                            self.context.report_dir)
            spw_id = plot.parameters['spw']
            field = plot.parameters['field']

            # Javascript JSON parser doesn't like Javascript floating point 
            # constants (NaN, Infinity etc.), so convert them to null. We  
            # do not omit the dictionary entry so that the plot is hidden
            # by the filters.
#             if math.isnan(ratio) or math.isinf(ratio):
#                 ratio = 'null'

            d[image_relpath] = {'spw'       : str(spw_id),
                                'field'     : field,
                                'thumbnail' : thumbnail_relpath}

        # Javascript parser requires \" -> \\" conversion 
        self.json = json.dumps(d).replace('\"', '\\"')
         
    def _get_display_context(self):
        return {'pcontext'   : self.context,
                'result'     : self.result,
                'plots'      : self.plots,
                'dirname'    : self.dirname,
                'json'       : self.json,
                'plot_title' : 'WVR flagging metric view for %s' % self.ms}

    @property
    def dirname(self):
        stage = 'stage%s' % self.result.stage_number
        return os.path.join(self.context.report_dir, stage)
    
    @property
    def filename(self):        
        filename = filenamer.sanitize('flagging_metric-%s.html' % self.ms)
        return filename
    
    @property
    def path(self):
        return os.path.join(self.dirname, self.filename)
    
    def get_file(self, hardcopy=True):
        if hardcopy and not os.path.exists(self.dirname):
            os.makedirs(self.dirname)
            
        file_obj = open(self.path, 'w') if hardcopy else StdOutFile()
        return contextlib.closing(file_obj)
    
    def render(self):
        display_context = self._get_display_context()
        t = TemplateFinder.get_template(self.template)
        return t.render(**display_context)


class WvrgcalflagPhaseOffsetPlotRenderer(object):
    template = 'wvr_phase_offset_plots.html'

    def __init__(self, context, result, plots):
        self.context = context
        self.result = result
        self.plots = plots
        self.ms = os.path.basename(self.result.inputs['vis'])

        # put this code here to save overcomplicating the template
        antenna_names = set()
        for plot in plots:
            antenna_names.update(set(plot.parameters['ant']))
        antenna_names = sorted(list(antenna_names))
        self.antenna_names = antenna_names
        
        # all values set on this dictionary will be written to the JSON file
        d = {}
        for plot in plots:
            # calculate the relative pathnames as seen from the browser
            thumbnail_relpath = os.path.relpath(plot.thumbnail,
                                                self.context.report_dir)
            image_relpath = os.path.relpath(plot.abspath,
                                            self.context.report_dir)
            antenna_name = plot.parameters['ant']
            spw_id = plot.parameters['spw']

            ratio = 1.0 / plot.qa_score

            # Javascript JSON parser doesn't like Javascript floating point 
            # constants (NaN, Infinity etc.), so convert them to null. We  
            # do not omit the dictionary entry so that the plot is hidden
            # by the filters.
            if math.isnan(ratio) or math.isinf(ratio):
                ratio = 'null'

            d[image_relpath] = {'antenna'   : antenna_name,
                                'spw'       : spw_id,
                                'ratio'     : ratio,
                                'thumbnail' : thumbnail_relpath}

        self.json = json.dumps(d)
         
    def _get_display_context(self):
        return {'pcontext'   : self.context,
                'result'     : self.result,
                'plots'      : self.plots,
                'antennas'   : self.antenna_names,
                'dirname'    : self.dirname,
                'json'       : self.json,
                'plot_title' : 'WVR Phase Offset Plots for %s' % self.ms}

    @property
    def dirname(self):
        stage = 'stage%s' % self.result.stage_number
        return os.path.join(self.context.report_dir, stage)
    
    @property
    def filename(self):        
        filename = filenamer.sanitize('phase_offsets-%s.html' % self.ms)
        return filename
    
    @property
    def path(self):
        return os.path.join(self.dirname, self.filename)
    
    def get_file(self, hardcopy=True):
        if hardcopy and not os.path.exists(self.dirname):
            os.makedirs(self.dirname)
            
        file_obj = open(self.path, 'w') if hardcopy else StdOutFile()
        return contextlib.closing(file_obj)
    
    def render(self):
        display_context = self._get_display_context()
        t = TemplateFinder.get_template(self.template)
        return t.render(**display_context)


class WvrgcalflagPhaseOffsetVsBaselinePlotRenderer(object):
    template = 'wvr_phase_offset_vs_baseline_plots.html'

    def __init__(self, context, result, plots):
        self.context = context
        self.result = result
        self.plots = plots
        self.ms = os.path.basename(self.result.inputs['vis'])

        # all values set on this dictionary will be written to the JSON file
        d = {}
        for plot in plots:
            # calculate the relative pathnames as seen from the browser
            thumbnail_relpath = os.path.relpath(plot.thumbnail,
                                                self.context.report_dir)
            image_relpath = os.path.relpath(plot.abspath,
                                            self.context.report_dir)
            spw_id = plot.parameters['spw']
            scan_id = plot.parameters['scan']

            # Javascript JSON parser doesn't like Javascript floating point 
            # constants (NaN, Infinity etc.), so convert them to null. We  
            # do not omit the dictionary entry so that the plot is hidden
            # by the filters.
#             if math.isnan(ratio) or math.isinf(ratio):
#                 ratio = 'null'

            d[image_relpath] = {'spw'       : spw_id,
                                'scan'      : scan_id,
                                'thumbnail' : thumbnail_relpath}

        self.json = json.dumps(d)
         
    def _get_display_context(self):
        return {'pcontext'   : self.context,
                'result'     : self.result,
                'plots'      : self.plots,
                'dirname'    : self.dirname,
                'json'       : self.json,
                'plot_title' : 'Phase offset vs average baseline for %s' % self.ms}

    @property
    def dirname(self):
        stage = 'stage%s' % self.result.stage_number
        return os.path.join(self.context.report_dir, stage)
    
    @property
    def filename(self):        
        filename = filenamer.sanitize('phase_offsets_vs_baseline-%s.html' % self.ms)
        return filename
    
    @property
    def path(self):
        return os.path.join(self.dirname, self.filename)
    
    def get_file(self, hardcopy=True):
        if hardcopy and not os.path.exists(self.dirname):
            os.makedirs(self.dirname)
            
        file_obj = open(self.path, 'w') if hardcopy else StdOutFile()
        return contextlib.closing(file_obj)
    
    def render(self):
        display_context = self._get_display_context()
        t = TemplateFinder.get_template(self.template)
        return t.render(**display_context)


class T2_4MDetailsGaincalRenderer(T2_4MDetailsDefaultRenderer):
    GaincalApplication = collections.namedtuple('GaincalApplication', 
                                                'ms gaintable calmode solint intent spw') 

    def __init__(self, template='t2-4m_details-hif_gaincal.html', 
                 always_rerender=False):
        # set the name of our specialised Mako template via the superclass
        # constructor 
        super(T2_4MDetailsGaincalRenderer, self).__init__(template,
                                                              always_rerender)

    def get_display_context(self, context, results):
        # get the standard Mako context from the superclass implementation 
        super_cls = super(T2_4MDetailsGaincalRenderer, self)
        ctx = super_cls.get_display_context(context, results)

        stage_dir = os.path.join(context.report_dir, 
                                 'stage%d' % results.stage_number)
        if not os.path.exists(stage_dir):
            os.mkdir(stage_dir)

        applications = []
        structure_plots = {}
        amp_vs_time_summaries = {}
        phase_vs_time_summaries = {}
        amp_vs_time_details = {}
        phase_vs_time_details = {}
        diagnostic_amp_vs_time_summaries = {}
        diagnostic_phase_vs_time_summaries = {}
        diagnostic_amp_vs_time_details = {}
        diagnostic_phase_vs_time_details = {}
        
        for result in results:
            vis = os.path.basename(result.inputs['vis'])
            ms = context.observing_run.get_ms(vis)

            applications.extend(self.get_gaincal_applications(result, ms))

            # generate the phase structure plots
            plotter = gaincal.RMSOffsetVsRefAntDistanceChart(context, result)
            structure_plots[vis] = plotter.plot()

            # result.final calapps contains p solution for solint=int,inf and a
            # solution for solint=inf.
            calapps = result.final

            # generate amp vs time plots
            plotter = gaincal.GaincalAmpVsTimeSummaryChart(context, result, 
                                                           calapps, 'TARGET')
            amp_vs_time_summaries[vis] = plotter.plot()

            # generate phase vs time plots
            plotter = gaincal.GaincalPhaseVsTimeSummaryChart(context, result,
                                                             calapps, 'TARGET')
            phase_vs_time_summaries[vis] = plotter.plot()

            # generate diagnostic phase vs time plots for bandpass solution, i.e. 
            # with solint=int
            plotter = gaincal.GaincalPhaseVsTimeSummaryChart(context, result,
                                                             calapps, 'BANDPASS')
            diagnostic_phase_vs_time_summaries[vis] = plotter.plot()

            # generate diagnostic amp vs time plots for bandpass solution, 
            # pointing to the diagnostic calapps outside result.final
            calapps = result.calampresult.final
            plotter = gaincal.GaincalAmpVsTimeSummaryChart(context, result, 
                                                           calapps, '')
            diagnostic_amp_vs_time_summaries[vis] = plotter.plot()

            if infrastructure.generate_detail_plots(result):
                calapps = result.final

                # phase vs time plots
                plotter = gaincal.GaincalPhaseVsTimeDetailChart(context, result,
                                                                calapps, 'TARGET')
                phase_vs_time_details[vis] = plotter.plot()
                renderer = GaincalPhaseVsTimePlotRenderer(context, result, 
                                                          phase_vs_time_details[vis])
                with renderer.get_file() as fileobj:
                    fileobj.write(renderer.render())        

                # amp vs time plots
                plotter = gaincal.GaincalAmpVsTimeDetailChart(context, result,
                                                               calapps, 'TARGET')
                amp_vs_time_details[vis] = plotter.plot()
                renderer = GaincalAmpVsTimePlotRenderer(context, result, 
                                                        amp_vs_time_details[vis])
                with renderer.get_file() as fileobj:
                    fileobj.write(renderer.render())        

                # phase vs time for solint=int
                plotter = gaincal.GaincalPhaseVsTimeDetailChart(context, result,
                                                                calapps, 'BANDPASS')
                diagnostic_phase_vs_time_details[vis] = plotter.plot()
                renderer = GaincalPhaseVsTimeDiagnosticPlotRenderer(context,
                        result, diagnostic_phase_vs_time_details[vis])
                with renderer.get_file() as fileobj:
                    fileobj.write(renderer.render())        

                # amp vs time plots for solint=int
                calapps = result.calampresult.final
                plotter = gaincal.GaincalAmpVsTimeDetailChart(context, result,
                                                              calapps, '')
                diagnostic_amp_vs_time_details[vis] = plotter.plot()
                renderer = GaincalAmpVsTimeDiagnosticPlotRenderer(context, 
                        result, diagnostic_amp_vs_time_details[vis])
                with renderer.get_file() as fileobj:
                    fileobj.write(renderer.render())        

            # get the first scan for the PHASE intent(s)
#             first_phase_scan = ms.get_scans(scan_intent='PHASE')[0]
#             scan_id = first_phase_scan.id
#             LOG.trace('Using scan %s for phase structure summary '
#                       'plots' % first_phase_scan.id)
#             structure_summary_plots[vis] = [p for p in structure_plots
#                                             if scan_id in set(p.parameters['scan'].split(','))]
            
        # add the PlotGroups to the Mako context. The Mako template will parse
        # these objects in order to create links to the thumbnail pages we
        # just created
        ctx.update({'applications'        : applications,
                    'structure_plots'     : structure_plots,
                    'amp_vs_time_plots'   : amp_vs_time_summaries,
                    'phase_vs_time_plots' : phase_vs_time_summaries,
                    'diagnostic_amp_vs_time_plots'   : diagnostic_amp_vs_time_summaries,
                    'diagnostic_phase_vs_time_plots' : diagnostic_phase_vs_time_summaries,
                    'dirname'             : stage_dir})

        return ctx
    
    def get_gaincal_applications(self, result, ms):
        applications = []
        
        calmode_map = {'p':'Phase only',
                       'a':'Amplitude only',
                       'ap':'Phase and amplitude'}
        
        for calapp in result.final:
            solint = calapp.origin.inputs['solint']

            if solint == 'inf':
                solint = 'Infinite'
            
            # Convert solint=int to a real integration time. 
            # solint is spw dependent; science windows usually have the same
            # integration time, though that's not guaranteed by the MS.
            if solint == 'int':
                from_intent = calapp.origin.inputs['intent']
                
                # from_intent is given in CASA intents, ie. *AMPLI*, *PHASE*
                # etc. We need this in pipeline intents.
                pipeline_intent = utils.to_pipeline_intent(ms, from_intent)
                scans = ms.get_scans(scan_intent=pipeline_intent)

                # let CASA parse spw arg in case it contains channel spec
                spw_ids = [spw_id for (spw_id, _, _, _) 
                           in utils.spw_arg_to_id(calapp.vis, calapp.spw)]
                
                all_solints = set()
                for spw_id in spw_ids:                    
                    spw_solints = set([scan.mean_interval(spw_id) 
                                       for scan in scans])
                    all_solints.update(spw_solints)
                
                in_secs = ['%0.2fs' % (dt.seconds + dt.microseconds * 1e-6) 
                           for dt in all_solints]  
                solint = 'Per integration (%s)' % utils.commafy(in_secs, quotes=False, conjunction='or')
            
            gaintable = os.path.basename(calapp.gaintable)
            spw = ', '.join(calapp.spw.split(','))

            to_intent = ', '.join(calapp.intent.split(','))
            if to_intent == '':
                to_intent = 'ALL'

            calmode = calapp.origin.inputs['calmode']
            calmode = calmode_map.get(calmode, calmode)
            a = T2_4MDetailsGaincalRenderer.GaincalApplication(ms.basename,
                                                               gaintable,
                                                               solint,
                                                               calmode,
                                                               to_intent,
                                                               spw)
            applications.append(a)

        return applications


class GenericPlotsRenderer(object):
    # take a look at WvrgcalflagPhaseOffsetVsBaselinePlotRenderer when we have
    # scores and histograms to generate. there should be a common base class. 
    template = 'generic_x_vs_y_detail_plots.html'

    def __init__(self, context, result, plots):
        self.context = context
        self.result = result
        self.plots = plots
        self.ms = os.path.basename(self.result.inputs['vis'])

        # all values set on this dictionary will be written to the JSON file
        d = {}
        for plot in plots:
            # calculate the relative pathnames as seen from the browser
            thumbnail_relpath = os.path.relpath(plot.thumbnail,
                                                self.context.report_dir)
            image_relpath = os.path.relpath(plot.abspath,
                                            self.context.report_dir)
            spw_id = plot.parameters['spw']
            ant_id = plot.parameters['ant']

            # Javascript JSON parser doesn't like Javascript floating point 
            # constants (NaN, Infinity etc.), so convert them to null. We  
            # do not omit the dictionary entry so that the plot is hidden
            # by the filters.
#             if math.isnan(ratio) or math.isinf(ratio):
#                 ratio = 'null'

            d[image_relpath] = {'spw'       : str(spw_id),
                                'ant'       : ant_id,
                                'thumbnail' : thumbnail_relpath}

        self.json = json.dumps(d)
         
    def _get_display_context(self):
        return {'pcontext'   : self.context,
                'result'     : self.result,
                'plots'      : self.plots,
                'dirname'    : self.dirname,
                'json'       : self.json,
                'plot_title' : 'Phase vs time for %s' % self.ms}

    @property
    def dirname(self):
        stage = 'stage%s' % self.result.stage_number
        return os.path.join(self.context.report_dir, stage)
    
    @property
    def filename(self):        
        filename = filenamer.sanitize('phase_vs_time-%s.html' % self.ms)
        return filename
    
    @property
    def path(self):
        return os.path.join(self.dirname, self.filename)
    
    def get_file(self, hardcopy=True):
        if hardcopy and not os.path.exists(self.dirname):
            os.makedirs(self.dirname)
            
        file_obj = open(self.path, 'w') if hardcopy else StdOutFile()
        return contextlib.closing(file_obj)
    
    def render(self):
        display_context = self._get_display_context()
        t = TemplateFinder.get_template(self.template)
        return t.render(**display_context)


class GaincalPhaseVsTimePlotRenderer(GenericPlotsRenderer):
    @property
    def filename(self):        
        filename = filenamer.sanitize('phase_vs_time-%s.html' % self.ms)
        return filename

    def _get_display_context(self):
        d = super(GaincalPhaseVsTimePlotRenderer, self)._get_display_context()
        d['plot_title'] = 'Phase vs time for %s' % self.ms
        return d


class GaincalPhaseVsTimeDiagnosticPlotRenderer(GaincalPhaseVsTimePlotRenderer):
    template = 'diagnostic_phase_vs_time_plots.html'

    def __init__(self, context, result, plots):
        super(GaincalPhaseVsTimeDiagnosticPlotRenderer, self).__init__(context, result, plots)

        # The basic json dictionary is set in __init__. Now add the scores.
        d = json.loads(self.json)
        score_types = ('PHASE_SCORE_XY', 'PHASE_SCORE_X2X1')
        # First check if we have scores for this MS
        table_names = [name for name in self.result.qa.qa_results_dict.iterkeys() if name.find(self.ms) != -1]
        if (table_names != []):
            for key in d.iterkeys():
                spw = d[key]['spw']
                ant = d[key]['ant']
                for score_type in score_types:
                    average_score = 0.0
                    num_scores = 0
                    for table_name in table_names:
                        spw_id = int(spw)
                        ant_id_dict = dict((value, key) for (key, value) in self.result.qa.qa_results_dict[table_name]['QASCORES']['ANTENNAS'].iteritems())
                        ant_id = ant_id_dict[ant]
                        phase_field_ids = self.result.qa.qa_results_dict[table_name]['PHASE_FIELDS']
                        if (phase_field_ids != []):
                            for field_id in phase_field_ids:
                                score = self.result.qa.qa_results_dict[table_name]['QASCORES']['SCORES'][field_id][spw_id][ant_id][score_type]
                                if (score == 'C/C'):
                                    average_score += -0.1
                                else:
                                    average_score += score
                                num_scores += 1
                        else:
                            average_score += 1.0
                            num_scores += 1
                    if (num_scores != 0):
                        average_score /= num_scores
                    d[key][score_type] = average_score

        else:
            # We don't have scores
            for key in d.iterkeys():
                for score_type in score_types:
                    d[key][score_type] = 1.0

        self.json = json.dumps(d)

    @property
    def filename(self):        
        filename = filenamer.sanitize('diagnostic_phase_vs_time-%s.html' % self.ms)
        return filename
    

class GaincalAmpVsTimePlotRenderer(GenericPlotsRenderer):
    # take a look at WvrgcalflagPhaseOffsetVsBaselinePlotRenderer when we have
    # scores and histograms to generate. there should be a common base class. 
    template = 'generic_x_vs_y_detail_plots.html'

    @property
    def filename(self):        
        filename = filenamer.sanitize('amp_vs_time-%s.html' % self.ms)
        return filename

    def _get_display_context(self):
        d = super(GaincalAmpVsTimePlotRenderer, self)._get_display_context()
        d['plot_title'] = 'Amplitude vs time for %s' % self.ms
        return d


class GaincalAmpVsTimeDiagnosticPlotRenderer(GaincalAmpVsTimePlotRenderer):
    @property
    def filename(self):        
        filename = filenamer.sanitize('diagnostic_amp_vs_time-%s.html' % self.ms)
        return filename


class T2_4MDetailsBandpassRenderer(T2_4MDetailsDefaultRenderer):
    """
    T2_4MDetailsBandpassRenderer generates the detailed T2_4M-level plots and
    output specific to the bandpass calibration task.
    """
    BandpassApplication = collections.namedtuple('BandpassApplication', 
                                                 'ms gaintable bandtype solint intent spw') 
    PhaseupApplication = collections.namedtuple('PhaseupApplication', 
                                                'ms calmode solint minblperant minsnr flagged phaseupbw') 
    
    def __init__(self, template='t2-4m_details-bandpass.html', 
                 always_rerender=False):
        # set the name of our specialised Mako template via the superclass
        # constructor 
        super(T2_4MDetailsBandpassRenderer, self).__init__(template, 
                                                           always_rerender)

    """
    Get the Mako context appropriate to the results created by a Bandpass
    task.
    
    This routine triggers the creation of the plots specific to the bandpass
    task, creating thumbnail pages as necessary. The returned dictionary  
    
    :param context: the pipeline Context
    :type context: :class:`~pipeline.infrastructure.launcher.Context`
    :param results: the bandpass results to describe
    :type results: 
        :class:`~pipeline.infrastructure.tasks.bandpass.common.BandpassResults`
    :rtype a dictionary that can be passed to the matching bandpass Mako 
        template
    """
    def get_display_context(self, context, results):
        # get the standard Mako context from the superclass implementation 
        super_cls = super(T2_4MDetailsBandpassRenderer, self)
        ctx = super_cls.get_display_context(context, results)

        stage_dir = os.path.join(context.report_dir, 
                                 'stage%d' % results.stage_number)
        if not os.path.exists(stage_dir):
            os.mkdir(stage_dir)

        # generate the bandpass-specific plots, collecting the Plot objects
        # returned by the plot generator 
        applications = []
        phaseup_applications = []
        amp_refant = {}
        amp_mode = {}
        amp_details = {}
        amp_vs_time_subpages = {}

        phase_refant = {}
        phase_mode = {}
        phase_details = {}
        phase_vs_time_subpages = {}

        for result in results:
            vis = os.path.basename(result.inputs['vis'])
            ms = context.observing_run.get_ms(vis)
            applications.extend(self.get_bandpass_applications(result, ms))
            phaseup_applications.extend(self.get_phaseup_applications(result, ms))
            ms_refant = ms.reference_antenna.split(',')[0]

            # need two summary plots: one for the refant, one for the mode
            plotter = bandpass.BandpassAmpVsFreqSummaryChart(context, result)
            summaries = plotter.plot()
            for_refant = [p for p in summaries 
                          if p.parameters['ant'] == ms_refant]
            amp_refant[vis] = for_refant[0] if for_refant else None

            # replace mode with first non-refant plot until we have scores
            LOG.todo('Replace bp summary plot with mode when scores are in place')
            non_refants = [p for p in summaries
                           if p.parameters['ant'] != ms_refant]
            amp_mode[vis] = non_refants[0] if non_refants else None

            # need two summary plots: one for the refant, one for the mode
            plotter = bandpass.BandpassPhaseVsFreqSummaryChart(context, result)
            summaries = plotter.plot()
            for_refant = [p for p in summaries 
                          if p.parameters['ant'] == ms_refant]
            phase_refant[vis] = for_refant[0] if for_refant else None
            
            non_refants = [p for p in summaries
                           if p.parameters['ant'] != ms_refant]
            phase_mode[vis] = non_refants[0] if non_refants else None

            # make phase vs freq plots for all data 
            plotter = bandpass.BandpassPhaseVsFreqDetailChart(context, result)
            phase_details[vis] = plotter.plot()
            renderer = BandpassPhaseVsFreqPlotRenderer(context, result, 
                                                       phase_details[vis])            
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())    
                phase_vs_time_subpages[ms.basename] = renderer.filename

            plotter = bandpass.BandpassAmpVsFreqDetailChart(context, result)
            amp_details[vis] = plotter.plot()            
            renderer = BandpassAmpVsFreqPlotRenderer(context, result, 
                                                     amp_details[vis])            
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())    
                # the filename is sanitised - the MS name is not. We need to
                # map MS to sanitised filename for link construction.
                amp_vs_time_subpages[ms.basename] = renderer.filename

        # add the PlotGroups to the Mako context. The Mako template will parse
        # these objects in order to create links to the thumbnail pages we
        # just created
        ctx.update({'applications'         : applications,
                    'phaseup_applications' : phaseup_applications,
                    'amp_mode'             : amp_mode,
                    'amp_refant'           : amp_refant,
                    'phase_mode'           : phase_mode,
                    'phase_refant'         : phase_refant,
                    'amp_subpages'         : amp_vs_time_subpages,
                    'phase_subpages'       : phase_vs_time_subpages,
                    'dirname'              : stage_dir})

        return ctx

    def get_phaseup_applications(self, result, ms):
        # return early if phase-up was not activated
        if result.inputs.get('phaseup', False) != True:
            return []
        
        calmode_map = {'p':'Phase only',
                       'a':'Amplitude only',
                       'ap':'Phase and amplitude'}
        
        # identify phaseup from 'preceding' list attached to result
        phaseup_calapps = [] 
        for previous_result in result.preceding:
            for calapp in previous_result:
                l = [cf for cf in calapp.calfrom if cf.caltype == 'gaincal']
                if l and calapp not in phaseup_calapps:
                    phaseup_calapps.append(calapp)
                
        applications = []
        for calapp in phaseup_calapps:
            solint = calapp.origin.inputs['solint']

            if solint == 'inf':
                solint = 'Infinite'
            
            # Convert solint=int to a real integration time. 
            # solint is spw dependent; science windows usually have the same
            # integration time, though that's not guaranteed by the MS.
            if solint == 'int':
                from_intent = calapp.origin.inputs['intent']
                
                # from_intent is given in CASA intents, ie. *AMPLI*, *PHASE*
                # etc. We need this in pipeline intents.
                pipeline_intent = utils.to_pipeline_intent(ms, from_intent)
                scans = ms.get_scans(scan_intent=pipeline_intent)

                # let CASA parse spw arg in case it contains channel spec
                spw_ids = [spw_id for (spw_id, _, _, _) 
                           in utils.spw_arg_to_id(calapp.vis, result.inputs['spw'])]
                
                all_solints = set()
                for spw_id in spw_ids:                    
                    spw_solints = set([scan.mean_interval(spw_id) 
                                       for scan in scans])
                    all_solints.update(spw_solints)
                
                in_secs = ['%0.2fs' % (dt.seconds + dt.microseconds * 1e-6) 
                           for dt in all_solints]  
                solint = 'Per integration (%s)' % utils.commafy(in_secs, quotes=False, conjunction='or')
            
            calmode = calapp.origin.inputs.get('calmode', 'N/A')
            calmode = calmode_map.get(calmode, calmode)
            minblperant = calapp.origin.inputs.get('minblperant', 'N/A')
            minsnr = calapp.origin.inputs.get('minsnr', 'N/A')
            flagged = 'TODO'
            phaseupbw = result.inputs.get('phaseupbw', 'N/A')

            a = T2_4MDetailsBandpassRenderer.PhaseupApplication(ms.basename,
                                                                calmode,
                                                                solint,
                                                                minblperant,
                                                                minsnr,
                                                                flagged,
                                                                phaseupbw)
            applications.append(a)

        return applications
    
    def get_bandpass_applications(self, result, ms):
        applications = []
        
        bandtype_map = {'B'    :'Channel',
                        'BPOLY':'Polynomial'}                       
        
        for calapp in result.final:
            solint = calapp.origin.inputs['solint']

            if solint == 'inf':
                solint = 'Infinite'
            
            # Convert solint=int to a real integration time. 
            # solint is spw dependent; science windows usually have the same
            # integration time, though that's not guaranteed by the MS.
            if solint == 'int':
                from_intent = calapp.origin.inputs['intent']
                
                # from_intent is given in CASA intents, ie. *AMPLI*, *PHASE*
                # etc. We need this in pipeline intents.
                pipeline_intent = utils.to_pipeline_intent(ms, from_intent)
                scans = ms.get_scans(scan_intent=pipeline_intent)

                # let CASA parse spw arg in case it contains channel spec
                spw_ids = [spw_id for (spw_id, _, _, _) 
                           in utils.spw_arg_to_id(calapp.vis, calapp.spw)]
                
                all_solints = set()
                for spw_id in spw_ids:                    
                    spw_solints = set([scan.mean_interval(spw_id) 
                                       for scan in scans])
                    all_solints.update(spw_solints)
                
                in_secs = ['%0.2fs' % (dt.seconds + dt.microseconds * 1e-6) 
                           for dt in all_solints]  
                solint = 'Per integration (%s)' % utils.commafy(in_secs, quotes=False, conjunction='or')
            
            gaintable = os.path.basename(calapp.gaintable)
            spw = ', '.join(calapp.spw.split(','))

            to_intent = ', '.join(calapp.intent.split(','))
            if to_intent == '':
                to_intent = 'ALL'

            # TODO get this from the calapp rather than the top-level inputs?
            bandtype = calapp.origin.inputs['bandtype']
            bandtype = bandtype_map.get(bandtype, bandtype)
            a = T2_4MDetailsBandpassRenderer.BandpassApplication(ms.basename,
                                                                 gaintable,
                                                                 bandtype,
                                                                 solint,
                                                                 to_intent,
                                                                 spw)
            applications.append(a)

        return applications


class BaseJsonPlotRenderer(object):
    template = None

    def __init__(self, context, result, plots, scores):
        self.context = context
        self.result = result
        self.plots = plots
        self.vis = os.path.basename(self.result.inputs['vis']) 
        self.json = json.dumps(scores)
         
    def _get_display_context(self):
        return {'pcontext'   : self.context,
                'result'     : self.result,
                'plots'      : self.plots,
                'dirname'    : self.dirname,
                'vis'        : self.vis,
                'json'       : self.json}

    @property
    def dirname(self):
        stage = 'stage%s' % self.result.stage_number
        return os.path.join(self.context.report_dir, stage)
    
    @property
    def filename(self):        
        raise NotImplementedError
    
    @property
    def path(self):
        return os.path.join(self.dirname, self.filename)
    
    def get_file(self, hardcopy=True):
        if hardcopy and not os.path.exists(self.dirname):
            os.makedirs(self.dirname)
            
        file_obj = open(self.path, 'w') if hardcopy else StdOutFile()
        return contextlib.closing(file_obj)
    
    def render(self):
        display_context = self._get_display_context()
        t = TemplateFinder.get_template(self.template)
        return t.render(**display_context)


class BandpassAmpVsFreqPlotRenderer(BaseJsonPlotRenderer):
    template = 't2-4m_details-bandpass-amp_vs_freq_plots.html'

    def __init__(self, context, result, plots):
        vis = os.path.basename(result.inputs['vis']) 
        ms = context.observing_run.get_ms(vis)
        scores = get_bandpass_amp_qa_scores(ms, result, plots,
                                             context.report_dir)
                
        super(BandpassAmpVsFreqPlotRenderer, self).__init__(context, result, plots, scores) 
         
    @property
    def filename(self):        
        filename = filenamer.sanitize('amp_vs_freq-%s.html' % self.vis)
        return filename


class BandpassPhaseVsFreqPlotRenderer(BaseJsonPlotRenderer):
    template = 't2-4m_details-bandpass-phase_vs_freq_plots.html'

    def __init__(self, context, result, plots):
        vis = os.path.basename(result.inputs['vis']) 
        ms = context.observing_run.get_ms(vis)
        scores = get_bandpass_phase_qa_scores(ms, result, plots,
                                               context.report_dir)

        super(BandpassPhaseVsFreqPlotRenderer, self).__init__(context, result, plots, scores) 

    @property
    def filename(self):        
        filename = filenamer.sanitize('phase_vs_freq-%s.html' % self.vis)
        return filename
        

class T2_4MDetailsBandpassFlagRenderer(T2_4MDetailsDefaultRenderer):
    '''
    Renders detailed HTML output for the Tsysflag task.
    '''
    def __init__(self, template='t2-4m_details-hif_bandpassflagchans.html',
            always_rerender=False):
        super(T2_4MDetailsBandpassFlagRenderer, self).__init__(template,
                always_rerender)

    def get_display_context(self, context, results):
        super_cls = super(T2_4MDetailsBandpassFlagRenderer, self)
        ctx = super_cls.get_display_context(context, results)

        htmlreports = self.get_htmlreports(context, results)
        
        ctx.update({'htmlreports' : htmlreports})
        return ctx

    def get_htmlreports(self, context, results):
        report_dir = context.report_dir
        weblog_dir = os.path.join(report_dir,
                                  'stage%s' % results.stage_number)

        htmlreports = {}
        for result in results:
            if not hasattr(result, 'table'):
                # empty result
                continue

            flagcmd_abspath = self.write_flagcmd_to_disk(weblog_dir, result)
            flagcmd_relpath = os.path.relpath(flagcmd_abspath, report_dir)
            table_basename = os.path.basename(result.table)
            htmlreports[table_basename] = (flagcmd_relpath,)

        return htmlreports

    def write_flagcmd_to_disk(self, weblog_dir, result):
        tablename = os.path.basename(result.table)
        filename = os.path.join(weblog_dir, '%s.html' % tablename)
        if os.path.exists(filename):
            return filename

        rendererutils.renderflagcmds(result.flagcmds(), filename)
        return filename


class T2_4MDetailsFlagchansRenderer(T2_4MDetailsDefaultRenderer):
    '''
    Renders detailed HTML output for the Flagchans task.
    '''
    def __init__(self, template='t2-4m_details-hif_flagchans.html',
            always_rerender=False):
        super(T2_4MDetailsFlagchansRenderer, self).__init__(template,
                always_rerender)

    def get_display_context(self, context, results):
        super_cls = super(T2_4MDetailsFlagchansRenderer, self)
        ctx = super_cls.get_display_context(context, results)

        htmlreports = self.get_htmlreports(context, results)
        
        ctx.update({'htmlreports' : htmlreports})
        return ctx

    def get_htmlreports(self, context, results):
        report_dir = context.report_dir
        weblog_dir = os.path.join(report_dir,
                                  'stage%s' % results.stage_number)

        htmlreports = {}
        for result in results:
#            if not hasattr(result, 'flagcmdfile'):
#                continue

            flagcmd_abspath = self.write_flagcmd_to_disk(weblog_dir, result)
            flagcmd_relpath = os.path.relpath(flagcmd_abspath, report_dir)
            table_basename = os.path.basename(result.table)
            htmlreports[table_basename] = (flagcmd_relpath,)

        return htmlreports

    def write_flagcmd_to_disk(self, weblog_dir, result):
        tablename = os.path.basename(result.table)
        filename = os.path.join(weblog_dir, '%s.html' % tablename)
        if os.path.exists(filename):
            return filename

        rendererutils.renderflagcmds(result.flagcmds(), filename)
        return filename


class T2_4MDetailsImportDataRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hif_importdata.html', 
                 always_rerender=False):
        super(T2_4MDetailsImportDataRenderer, self).__init__(template,
                                                             always_rerender)
        
    def get_display_context(self, context, result):
        super_cls = super(T2_4MDetailsImportDataRenderer, self)        
        ctx = super_cls.get_display_context(context, result)

        setjy_results = []
        for r in result:
            setjy_results.extend(r.setjy_results)

        measurements = []        
        for r in setjy_results:
            measurements.extend(r.measurements)

        num_mses = reduce(operator.add, [len(r.mses) for r in result])

        ctx.update({'flux_imported' : True if measurements else False,
                    'setjy_results' : setjy_results,
                    'num_mses'      : num_mses})

        return ctx

class T2_4MDetailsVLAImportDataRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hifv_importdata.html', 
                 always_rerender=False):
        super(T2_4MDetailsVLAImportDataRenderer, self).__init__(template,
                                                             always_rerender)
        
    def get_display_context(self, context, result):
        super_cls = super(T2_4MDetailsVLAImportDataRenderer, self)        
        ctx = super_cls.get_display_context(context, result)

        setjy_results = []
        for r in result:
            setjy_results.extend(r.setjy_results)

        measurements = []        
        for r in setjy_results:
            measurements.extend(r.measurements)

        num_mses = reduce(operator.add, [len(r.mses) for r in result])

        ctx.update({'flux_imported' : True if measurements else False,
                    'setjy_results' : setjy_results,
                    'num_mses'      : num_mses})

        return ctx


class T2_4MDetailsSetjyRenderer(T2_4MDetailsDefaultRenderer):

    def __init__(self, template='t2-4m_details-hif_setjy.html', 
                 always_rerender=False):
        super(T2_4MDetailsSetjyRenderer, self).__init__(template,
                                                           always_rerender)

    def get_display_context(self, context, result):
        super_cls = super(T2_4MDetailsSetjyRenderer, self)
        ctx = super_cls.get_display_context(context, result)
        
        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % result.stage_number)
                                  
        ctx.update({'dirname'  : weblog_dir})
        
        '''
        amp_vs_uv_summary_plots = self.create_plots(context, 
                                                    result, 
                                                    setjy.AmpVsUVSummaryChart, 
                                                    'AMPLITUDE')
        '''
        
        amp_vs_uv_summary_plots = collections.defaultdict(dict)
        for intents in ['AMPLITUDE']:
            plots = self.create_plots(context, 
                                      result, 
                                      setjy.AmpVsUVSummaryChart, 
                                      intents)
            self.sort_plots_by_baseband(plots)

            key = intents
            for vis, vis_plots in plots.items():
                amp_vs_uv_summary_plots[vis][key] = vis_plots

        ctx.update({'amp_vs_uv_plots'     : amp_vs_uv_summary_plots})
        
        return ctx

    def sort_plots_by_baseband(self, d):
        for vis, plots in d.items():
            plots = sorted(plots, 
                           key=lambda plot: plot.parameters['baseband'])
            d[vis] = plots

    def create_plots(self, context, results, plotter_cls, intents, renderer_cls=None):
        """
        Create plots and return a dictionary of vis:[Plots].
        """
        d = {}
        for result in results:
            plots = self.plots_for_result(context, result, plotter_cls, intents, renderer_cls)
            d = utils.dict_merge(d, plots)
        return d

    def plots_for_result(self, context, result, plotter_cls, intents, renderer_cls=None):
        vis = os.path.basename(result.inputs['vis'])
        
        plotter = plotter_cls(context, result, intents)
        plots = plotter.plot()

        d = collections.defaultdict(dict)
        d[vis] = plots

        if renderer_cls is not None:
            renderer = renderer_cls(context, result, plots)
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())        

        return d



class T2_4MDetailsGFluxscaleRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hifv_gfluxscale.html', 
                 always_rerender=False):
        super(T2_4MDetailsGFluxscaleRenderer, self).__init__(template,
                                                          always_rerender)
                                                          
    def get_display_context(self, context, result):
        super_cls = super(T2_4MDetailsGFluxscaleRenderer, self)
        ctx = super_cls.get_display_context(context, result)
        
        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % result.stage_number)
                                  
        ctx.update({'dirname'  : weblog_dir})
        
        '''
        amp_vs_uv_summary_plots = self.create_plots(context, 
                                                    result, 
                                                    setjy.AmpVsUVSummaryChart, 
                                                    'AMPLITUDE')
        '''
        #All antenna, sort by baseband
        ampuv_allant_plots = collections.defaultdict(dict)
        for intents in ['AMPLITUDE']:
            plots = self.create_plots(context, 
                                      result, 
                                      gfluxscale.GFluxscaleSummaryChart, 
                                      intents)
            self.sort_plots_by_baseband(plots)

            key = intents
            for vis, vis_plots in plots.items():
                ampuv_allant_plots[vis][key] = vis_plots
                
        #List of antenna for the fluxscale result, sorted by baseband
        ampuv_ant_plots = collections.defaultdict(dict)

        for intents in ['AMPLITUDE']:
            plots = self.create_plots_ants(context, 
                                      result, 
                                      gfluxscale.GFluxscaleSummaryChart, 
                                      intents)
            self.sort_plots_by_baseband(plots)

            key = intents
            for vis, vis_plots in plots.items():
                ampuv_ant_plots[vis][key] = vis_plots

        table_rows = self.make_flux_table(context, result)

        ctx.update({'ampuv_allant_plots' : ampuv_allant_plots,
                    'ampuv_ant_plots'    : ampuv_ant_plots,
                    'table_rows'         : table_rows})
        
        return ctx

    def sort_plots_by_baseband(self, d):
        for vis, plots in d.items():
            plots = sorted(plots, 
                           key=lambda plot: plot.parameters['baseband'])
            d[vis] = plots

    def create_plots(self, context, results, plotter_cls, intents, renderer_cls=None):
        """
        Create plots and return a dictionary of vis:[Plots].
        """
        d = {}
        for result in results:
            plots = self.plots_for_result(context, result, plotter_cls, intents, renderer_cls)
            d = utils.dict_merge(d, plots)
        return d
        
    def create_plots_ants(self, context, results, plotter_cls, intents, renderer_cls=None):
        """
        Create plots and return a dictionary of vis:[Plots].
        """
        d = {}
        for result in results:
            plots = self.plots_for_result(context, result, plotter_cls, intents, renderer_cls, ant=result.resantenna)
            d = utils.dict_merge(d, plots)
        return d

    def plots_for_result(self, context, result, plotter_cls, intents, renderer_cls=None, ant=''):
        vis = os.path.basename(result.inputs['vis'])
        
        
        plotter = plotter_cls(context, result, intents, ant=ant)
        plots = plotter.plot()

        d = collections.defaultdict(dict)
        d[vis] = plots

        if renderer_cls is not None:
            renderer = renderer_cls(context, result, plots)
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())        

        return d

    FluxTR = collections.namedtuple('FluxTR', 'vis field spw i q u v')

    def make_flux_table(self, context, results):
        # will hold all the flux stat table rows for the results
        rows = []
    
        for single_result in results:
            ms_for_result = context.observing_run.get_ms(single_result.vis)
            vis_cell = os.path.basename(single_result.vis)
    
            # measurements will be empty if fluxscale derivation failed
            if len(single_result.measurements) is 0:
                continue
                
            for field_arg, measurements in single_result.measurements.items():
                field = ms_for_result.get_fields(field_arg)[0]
                field_cell = '%s (#%s)' % (field.name, field.id)
    
                for measurement in sorted(measurements, key=lambda m: int(m.spw_id)):
                    fluxes = collections.defaultdict(lambda: 'N/A')
                    for stokes in ['I', 'Q', 'U', 'V']:
                        try:                        
                            flux = getattr(measurement, stokes)
                            unc = getattr(measurement.uncertainty, stokes)
                            flux_jy = flux.to_units(measures.FluxDensityUnits.JANSKY)
                            unc_jy = unc.to_units(measures.FluxDensityUnits.JANSKY)
                            
                            if flux_jy != 0 and unc_jy != 0:
                                unc_ratio = decimal.Decimal('100')*(unc_jy/flux_jy)
                                uncertainty = ' &#177; %s (%0.1f%%)' % (str(unc),
                                                                        unc_ratio)
                            else:
                                uncertainty = ''
                                
                            fluxes[stokes] = '%s%s' % (flux, uncertainty)
                        except:
                            pass
                                        
                    tr = T2_4MDetailsGFluxscaleRenderer.FluxTR(vis_cell, 
                                field_cell, measurement.spw_id, fluxes['I'],
                                fluxes['Q'], fluxes['U'], fluxes['V'])
                    rows.append(tr)
    
        return utils.merge_td_columns(rows)

    
class T2_4MDetailsApplycalRenderer(T2_4MDetailsDefaultRenderer):
    FlagTotal = collections.namedtuple('FlagSummary', 'flagged total')

    def __init__(self, template='t2-4m_details-hif_applycal.html', 
                 always_rerender=False):
        super(T2_4MDetailsApplycalRenderer, self).__init__(template,
                                                           always_rerender)

    def get_display_context(self, context, result):
        super_cls = super(T2_4MDetailsApplycalRenderer, self)
        ctx = super_cls.get_display_context(context, result)

        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % result.stage_number)

        flag_totals = {}
        for r in result:
            flag_totals = utils.dict_merge(flag_totals, 
                                           self.flags_for_result(r, context))

        calapps = {}
        for r in result:
            calapps = utils.dict_merge(calapps,
                                       self.calapps_for_result(r))

        caltypes = {}
        for r in result:
            caltypes = utils.dict_merge(caltypes,
                                        self.caltypes_for_result(r))

        # return all agents so we get ticks and crosses against each one
        agents = ['before', 'applycal']

        ctx.update({'flags'    : flag_totals,
                    'calapps'  : calapps,
                    'caltypes' : caltypes,
                    'agents'   : agents,
                    'dirname'  : weblog_dir})

        amp_vs_time_summary_plots = self.create_plots(context, 
                                                      result, 
                                                      applycal.AmpVsTimeSummaryChart, 
                                                      ['PHASE', 'BANDPASS', 'AMPLITUDE', 'TARGET'])

        phase_vs_time_summary_plots = self.create_plots(context, 
                                                      result, 
                                                      applycal.PhaseVsTimeSummaryChart, 
                                                      ['PHASE', 'BANDPASS', 'AMPLITUDE', 'TARGET'])

#         amp_vs_freq_phase_summary_plots = self.create_plots(context, 
#                                                             result, 
#                                                             applycal.AmpVsFrequencySummaryChart, 
#                                                             ['PHASE'])
# 
#         phase_vs_freq_phase_summary_plots = self.create_plots(context, 
#                                                               result, 
#                                                               applycal.PhaseVsFrequencySummaryChart, 
#                                                               ['PHASE'])
# 
#         amp_vs_freq_bandpass_summary_plots = self.create_plots(context, 
#                                                             result, 
#                                                             applycal.AmpVsFrequencySummaryChart, 
#                                                             ['BANDPASS'])
# 
#         phase_vs_freq_bandpass_summary_plots = self.create_plots(context, 
#                                                                  result, 
#                                                                  applycal.PhaseVsFrequencySummaryChart, 
#                                                                  ['BANDPASS'])
# 
#         self.sort_plots_by_baseband(amp_vs_freq_phase_summary_plots)
#         self.sort_plots_by_baseband(phase_vs_freq_phase_summary_plots)
#         self.sort_plots_by_baseband(amp_vs_freq_bandpass_summary_plots)
#         self.sort_plots_by_baseband(phase_vs_freq_bandpass_summary_plots)

        amp_vs_freq_summary_plots = utils.OrderedDefaultdict(utils.OrderedDefaultdict)
        for intents in [['PHASE'],['BANDPASS']]:
            plots = self.create_plots(context, 
                                      result, 
                                      applycal.AmpVsFrequencySummaryChart, 
                                      intents)
            self.sort_plots_by_baseband(plots)

            key = utils.commafy(intents, quotes=False)
            for vis, vis_plots in plots.items():
                amp_vs_freq_summary_plots[vis][key] = vis_plots

        phase_vs_freq_summary_plots = utils.OrderedDefaultdict(utils.OrderedDefaultdict)
        for intents in [['PHASE'],['BANDPASS']]:
            plots = self.create_plots(context, 
                                      result, 
                                      applycal.PhaseVsFrequencySummaryChart, 
                                      intents)
            self.sort_plots_by_baseband(plots)

            key = utils.commafy(intents, quotes=False)
            for vis, vis_plots in plots.items():
                phase_vs_freq_summary_plots[vis][key] = vis_plots

        amp_vs_uv_summary_plots = self.create_plots(context, 
                                                    result, 
                                                    applycal.AmpVsUVSummaryChart, 
                                                    ['AMPLITUDE'])

        phase_vs_uv_summary_plots = self.create_plots(context, 
                                                      result, 
                                                      applycal.PhaseVsUVSummaryChart, 
                                                      ['AMPLITUDE'])

        # CAS-5970: add science target plots to the applycal page
        (science_amp_vs_freq_summary_plots, 
         science_phase_vs_freq_summary_plots,
         science_amp_vs_uv_summary_plots,
         uv_max) = self.create_science_plots(context, result) 

        if pipeline.infrastructure.generate_detail_plots(result):
            # detail plots. Don't need the return dictionary, but make sure a
            # renderer is passed so the detail page is written to disk
            self.create_plots(context, 
                              result, 
                              applycal.AmpVsFrequencyDetailChart, 
                              ['BANDPASS', 'PHASE'],
                              ApplycalAmpVsFreqPlotRenderer)
    
            self.create_plots(context, 
                              result, 
                              applycal.PhaseVsFrequencyDetailChart, 
                              ['BANDPASS', 'PHASE'],
                              ApplycalPhaseVsFreqPlotRenderer)
    
            self.create_plots(context, 
                              result, 
                              applycal.AmpVsUVDetailChart, 
                              ['AMPLITUDE'],
                              ApplycalAmpVsUVPlotRenderer)
    
            self.create_plots(context, 
                              result, 
                              applycal.PhaseVsUVDetailChart, 
                              ['AMPLITUDE'],
                              ApplycalPhaseVsUVPlotRenderer)
    
            self.create_plots(context, 
                              result, 
                              applycal.AmpVsTimeDetailChart, 
                              ['AMPLITUDE','PHASE','BANDPASS','TARGET'],
                              ApplycalAmpVsTimePlotRenderer)
    
            self.create_plots(context, 
                              result, 
                              applycal.PhaseVsTimeDetailChart, 
                              ['AMPLITUDE','PHASE','BANDPASS','TARGET'],
                              ApplycalPhaseVsTimePlotRenderer)

        ctx.update({'amp_vs_freq_plots'   : amp_vs_freq_summary_plots,
                    'phase_vs_freq_plots' : phase_vs_freq_summary_plots,
                    'amp_vs_time_plots'   : amp_vs_time_summary_plots,
                    'amp_vs_uv_plots'     : amp_vs_uv_summary_plots,
                    'phase_vs_uv_plots'   : phase_vs_uv_summary_plots,
                    'phase_vs_time_plots' : phase_vs_time_summary_plots,
                    'science_amp_vs_freq_plots'   : science_amp_vs_freq_summary_plots,
                    'science_phase_vs_freq_plots' : science_phase_vs_freq_summary_plots,
                    'science_amp_vs_uv_plots' : science_amp_vs_uv_summary_plots,
                    'uv_max' : uv_max})
        
        return ctx

    def create_science_plots(self, context, results):
        """
        Create plots for the science targets, returning two dictionaries of 
        vis:[Plots].
        """
        amp_vs_freq_summary_plots = collections.defaultdict(dict)
        phase_vs_freq_summary_plots = collections.defaultdict(dict)
        amp_vs_uv_summary_plots = collections.defaultdict(dict)
        max_uvs = collections.defaultdict(dict)
        
        by_source_id = lambda field: field.source.id
        for result in results:
            # Plot for 1 science field (either 1 science target or for a mosaic 1 
            # pointing). The science field that should be chosen is the one with
            # the brightest average amplitude over all spws
            vis = os.path.basename(result.inputs['vis'])
            ms = context.observing_run.get_ms(vis)
            
            brightest_fields = T2_4MDetailsApplycalRenderer.get_brightest_fields(ms)        
            for source_id, brightest_field in brightest_fields.items():
                # Ideally, the uvmax of the spectrum (plots 1 and 2) 
                # would be set by the appearance of plot 3; that is, if there is 
                # no obvious drop in amplitude with uvdist, then use all the data. 
                # A simpler compromise would be to use a uvrange that captures the
                # inner half the data. 
                baselines = sorted(ms.antenna_array.baselines,
                                   key=operator.attrgetter('length'))
                # take index as midpoint + 1 so we include the midpoint in the
                # constraint
                half_baselines = baselines[0:(len(baselines)//2)+1]
                uv_max = half_baselines[-1].length.to_units(measures.DistanceUnits.METRE)
                uv_range = '<%s' % uv_max
                LOG.debug('Setting UV range to %s for %s', uv_range, vis)
                max_uvs[vis] = half_baselines[-1].length
    
                plots = self.science_plots_for_result(context, 
                                                      result, 
                                                      applycal.AmpVsFrequencySummaryChart,
                                                      [brightest_field.id],
                                                      uv_range)
                amp_vs_freq_summary_plots[vis][source_id] = plots
    
                plots = self.science_plots_for_result(context, 
                                                      result, 
                                                      applycal.PhaseVsFrequencySummaryChart,
                                                      [brightest_field.id],
                                                      uv_range)
                phase_vs_freq_summary_plots[vis][source_id] = plots
    
                plots = self.science_plots_for_result(context, 
                                                      result, 
                                                      applycal.AmpVsUVBasebandSummaryChart,
                                                      [brightest_field.id])
                amp_vs_uv_summary_plots[vis][source_id] = plots

            if pipeline.infrastructure.generate_detail_plots(result):
                scans = ms.get_scans(scan_intent='TARGET')
                fields = set()
                for scan in scans:
                    fields.update([field.id for field in scan.fields])
                
                # Science target detail plots. Note that summary plots go onto the
                # detail pages; we don't create plots per spw or antenna
                self.science_plots_for_result(context, 
                                              result, 
                                              applycal.AmpVsFrequencySummaryChart, 
                                              fields,
                                              uv_range,
                                              ApplycalAmpVsFreqSciencePlotRenderer)

                self.science_plots_for_result(context, 
                                              result, 
                                              applycal.PhaseVsFrequencySummaryChart,
                                              fields,
                                              uv_range,
                                              ApplycalPhaseVsFreqSciencePlotRenderer)

                self.science_plots_for_result(context, 
                                              result, 
                                              applycal.AmpVsUVBasebandSummaryChart,
                                              fields,
                                              renderer_cls=ApplycalAmpVsUVSciencePlotRenderer)

        # sort plots by baseband so that the summary plots appear in baseband order
        for vis, source_plots in amp_vs_freq_summary_plots.items():
            self.sort_plots_by_baseband(source_plots)
        for vis, source_plots in phase_vs_freq_summary_plots.items():
            self.sort_plots_by_baseband(source_plots)
        for vis, source_plots in amp_vs_uv_summary_plots.items():
            self.sort_plots_by_baseband(source_plots)

        return (amp_vs_freq_summary_plots, phase_vs_freq_summary_plots, 
                amp_vs_uv_summary_plots, max_uvs)

    def science_plots_for_result(self, context, result, plotter_cls, fields, 
                                 uvrange=None, renderer_cls=None):
        # override field when plotting amp/phase vs frequency, as otherwise
        # the field is resolved to a list of all field IDs  
        overrides = {'coloraxis' : 'spw'}
        if uvrange is not None:
            overrides['uvrange'] = uvrange
        
        plots = []
        for field in fields:
            # override field when plotting amp/phase vs frequency, as otherwise
            # the field is resolved to a list of all field IDs  
            overrides['field'] = field
            
            plotter = plotter_cls(context, result, ['TARGET'], **overrides)
            plots.extend(plotter.plot())

        for plot in plots:
            plot.parameters['intent'] = ['TARGET']

        if renderer_cls is not None:
            renderer = renderer_cls(context, result, plots)
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())        

        return plots

    @staticmethod
    def get_brightest_fields(ms, intent='TARGET'):
        """
        
        """
        # get IDs for all science spectral windows
        spw_ids = set()
        for scan in ms.get_scans(scan_intent=intent):
            scan_spw_ids = set([dd.spw.id for dd in scan.data_descriptions])
            spw_ids.update(scan_spw_ids)

        if intent == 'TARGET':
            science_ids = set([spw.id for spw in ms.get_spectral_windows()])
            spw_ids = spw_ids.intersection(science_ids)

        result = collections.OrderedDict()

        by_source_id = lambda field: field.source.id
        fields_by_source_id = sorted(ms.get_fields(intent=intent), 
                                     key=by_source_id)
        for source_id, source_fields in itertools.groupby(fields_by_source_id,
                                                          by_source_id):
            fields = list(source_fields)
            
            # give the sole science target name if there's only one science target
            # in this ms.
            if len(fields) is 1:
                LOG.info('Only one %s target for Source #%s. Bypassing '
                         'brightest target selection.', intent, source_id)
                result[source_id] = fields[0]
                continue
            
            field_ids = set([(f.id, f.name) for f in fields])
    
            field = fields[0]
            LOG.warning('Bypassing brightest field selection due to problem '
                        'with visstat. Using Field #%s (%s) for Source #%s'
                        '', field.id, field.name, source_id)
            result[source_id] = field
            continue
    
            # holds the mapping of field name to mean flux 
            average_flux = {}
        
            # defines the parameters for the visstat job
            job_params = {
                'vis'        : ms.name,
                'axis'       : 'amp',
                'datacolumn' : 'corrected',
                'spw'        : ','.join(map(str, spw_ids)),
            }
        
            LOG.info('Calculating which %s field has the highest mean flux '
                     'for Source #%s', intent, source_id)
            # run visstat for each scan selection for the target
            for field_id, field_name in field_ids:
                job_params['field'] = str(field_id)
                job = casa_tasks.visstat(**job_params)
                LOG.debug('Calculating statistics for %r (#%s)', field_name, field_id)
                result = job.execute(dry_run=False)
                
                average_flux[(field_id, field_name)] = float(result['CORRECTED']['mean'])
                
            LOG.debug('Mean flux for %s targets:', intent)
            for (field_id, field_name), v in average_flux.items():
                LOG.debug('\t%r (%s): %s', field_name, field_id, v)
        
            # find the ID of the field with the highest average flux
            sorted_by_flux = sorted(average_flux.iteritems(), 
                                    key=operator.itemgetter(1),
                                    reverse=True)
            (brightest_id, brightest_name), highest_flux = sorted_by_flux[0]
            
            LOG.info('%s field %r (%s) has highest mean flux (%s)', intent,
                     brightest_name, brightest_id, highest_flux)
            result[source_id] = brightest_id

        return result
    
    def sort_plots_by_baseband(self, d):
        for vis, plots in d.items():
            plots = sorted(plots, 
                           key=lambda plot: plot.parameters['baseband'])
            d[vis] = plots

    def create_plots(self, context, results, plotter_cls, intents, renderer_cls=None):
        """
        Create plots and return a dictionary of vis:[Plots].
        """
        d = {}
        for result in results:
            plots = self.plots_for_result(context, result, plotter_cls, intents, renderer_cls)
            d = utils.dict_merge(d, plots)
        return d

    def plots_for_result(self, context, result, plotter_cls, intents, renderer_cls=None):
        vis = os.path.basename(result.inputs['vis'])

        plotter = plotter_cls(context, result, intents)
        plots = plotter.plot()
        for plot in plots:
            plot.parameters['intent'] = intents

        d = collections.defaultdict(dict)
        d[vis] = plots

        if renderer_cls is not None:
            renderer = renderer_cls(context, result, plots)
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())        

        return d

    def calapps_for_result(self, result):
        calapps = collections.defaultdict(list)
        for calapp in result.applied:
            vis = os.path.basename(calapp.vis)
            calapps[vis].append(calapp)
        return calapps

    def caltypes_for_result(self, result):
        type_map = {
            'bandpass' : 'Bandpass',
            'gaincal'  : 'Gain',
            'tsys'     : 'T<sub>sys</sub>',
            'wvr'      : 'WVR',
        }
        
        d = {}
        for calapp in result.applied:
            for calfrom in calapp.calfrom:
                caltype = type_map.get(calfrom.caltype, calfrom.caltype)

                if calfrom.caltype == 'gaincal':
                    # try heuristics to detect phase-only and amp-only 
                    # solutions 
                    caltype += self.get_gain_solution_type(calfrom.gaintable)
                    
                d[calfrom.gaintable] = caltype

        return d
                
    def get_gain_solution_type(self, gaintable):
        # get stats on amp solution of gaintable 
        calstat_job = casa_tasks.calstat(caltable=gaintable, axis='amp', 
                                         datacolumn='CPARAM', useflags=True)
        calstat_result = calstat_job.execute(dry_run=False)        
        stats = calstat_result['CPARAM']

        # amp solutions of unity imply phase-only was requested
        tol = 1e-3
        no_amp_soln = all([utils.approx_equal(stats['sum'], stats['npts'], tol),
                           utils.approx_equal(stats['min'], 1, tol),
                           utils.approx_equal(stats['max'], 1, tol)])

        # same again for phase solution
        calstat_job = casa_tasks.calstat(caltable=gaintable, axis='phase', 
                                         datacolumn='CPARAM', useflags=True)
        calstat_result = calstat_job.execute(dry_run=False)        
        stats = calstat_result['CPARAM']

        # phase solutions ~ 0 implies amp-only solution
        tol = 1e-5
        no_phase_soln = all([utils.approx_equal(stats['sum'], 0, tol),
                             utils.approx_equal(stats['min'], 0, tol),
                             utils.approx_equal(stats['max'], 0, tol)])

        if no_phase_soln and not no_amp_soln:
            return ' (amplitude only)'
        if no_amp_soln and not no_phase_soln:
            return ' (phase only)'
        return ''

    def flags_for_result(self, result, context):
        ms = context.observing_run.get_ms(result.inputs['vis'])
        summaries = result.summaries

        by_intent = self.flags_by_intent(ms, summaries)
        by_spw = self.flags_by_science_spws(ms, summaries)

        return {ms.basename : utils.dict_merge(by_intent, by_spw)}

    def flags_by_intent(self, ms, summaries):
        # create a dictionary of scans per observing intent, eg. 'PHASE':[1,2,7]
        intent_scans = {}
        for intent in ('BANDPASS', 'PHASE', 'AMPLITUDE', 'TARGET'):
            # convert IDs to strings as they're used as summary dictionary keys
            intent_scans[intent] = [str(s.id) for s in ms.scans
                                    if intent in s.intents]

        # while we're looping, get the total flagged by looking in all scans 
        intent_scans['TOTAL'] = [str(s.id) for s in ms.scans]

        total = collections.defaultdict(dict)

        previous_summary = None
        for summary in summaries:

            for intent, scan_ids in intent_scans.items():
                flagcount = 0
                totalcount = 0
    
                for i in scan_ids:
                    flagcount += int(summary['scan'][i]['flagged'])
                    totalcount += int(summary['scan'][i]['total'])
        
                    if previous_summary:
                        flagcount -= int(previous_summary['scan'][i]['flagged'])
    
                ft = T2_4MDetailsApplycalRenderer.FlagTotal(flagcount, 
                                                                totalcount)
                total[summary['name']][intent] = ft
                
            previous_summary = summary
                
        return total 
    
    def flags_by_science_spws(self, ms, summaries):
        science_spws = ms.get_spectral_windows(science_windows_only=True)
    
        total = collections.defaultdict(dict)
    
        previous_summary = None
        for summary in summaries:
    
            flagcount = 0
            totalcount = 0
    
            for spw in science_spws:
                spw_id = str(spw.id)
                flagcount += int(summary['spw'][spw_id]['flagged'])
                totalcount += int(summary['spw'][spw_id]['total'])
        
                if previous_summary:
                    flagcount -= int(previous_summary['spw'][spw_id]['flagged'])

            ft = T2_4MDetailsApplycalRenderer.FlagTotal(flagcount, 
                                                            totalcount)
            total[summary['name']]['SCIENCE SPWS'] = ft
                
            previous_summary = summary
                
        return total


class GenericFieldSpwAntPlotsRenderer(object):
    # take a look at WvrgcalflagPhaseOffsetVsBaselinePlotRenderer when we have
    # scores and histograms to generate. there should be a common base class. 
    template = 'generic_x_vs_y_field_spw_ant_detail_plots.html'

    def __init__(self, context, result, plots):
        self.context = context
        self.result = result
        self.plots = plots
        self.ms = os.path.basename(self.result.inputs['vis'])

        # all values set on this dictionary will be written to the JSON file
        d = {}
        for plot in plots:
            # calculate the relative pathnames as seen from the browser
            thumbnail_relpath = os.path.relpath(plot.thumbnail,
                                                self.context.report_dir)
            image_relpath = os.path.relpath(plot.abspath,
                                            self.context.report_dir)
            field = plot.parameters['field']
            spw_id = plot.parameters['spw']
            ant_id = plot.parameters['ant']

            # Javascript JSON parser doesn't like Javascript floating point 
            # constants (NaN, Infinity etc.), so convert them to null. We  
            # do not omit the dictionary entry so that the plot is hidden
            # by the filters.
#             if math.isnan(ratio) or math.isinf(ratio):
#                 ratio = 'null'

            d[image_relpath] = {'spw'       : str(spw_id),
                                'ant'       : ant_id,
                                'field'     : field,
                                'thumbnail' : thumbnail_relpath}

        # Javascript parser requires \" -> \\" conversion 
        self.json = json.dumps(d).replace('\"', '\\"')
         
    def _get_display_context(self):
        return {'pcontext'   : self.context,
                'result'     : self.result,
                'plots'      : self.plots,
                'dirname'    : self.dirname,
                'json'       : self.json,
                'plot_title' : 'Phase vs time for %s' % self.ms}

    @property
    def dirname(self):
        stage = 'stage%s' % self.result.stage_number
        return os.path.join(self.context.report_dir, stage)
    
    @property
    def filename(self):        
        filename = filenamer.sanitize('phase_vs_time-%s.html' % self.ms)
        return filename
    
    @property
    def path(self):
        return os.path.join(self.dirname, self.filename)
    
    def get_file(self, hardcopy=True):
        if hardcopy and not os.path.exists(self.dirname):
            os.makedirs(self.dirname)
            
        file_obj = open(self.path, 'w') if hardcopy else StdOutFile()
        return contextlib.closing(file_obj)
    
    def render(self):
        display_context = self._get_display_context()
        t = TemplateFinder.get_template(self.template)
        return t.render(**display_context)


class GenericFieldBasebandPlotsRenderer(GenericFieldSpwAntPlotsRenderer):
    # take a look at WvrgcalflagPhaseOffsetVsBaselinePlotRenderer when we have
    # scores and histograms to generate. there should be a common base class. 
    template = 'generic_x_vs_y_field_baseband_detail_plots.html'

    def __init__(self, context, result, plots):
        self.context = context
        self.result = result
        self.plots = plots
        self.ms = os.path.basename(self.result.inputs['vis'])

        # all values set on this dictionary will be written to the JSON file
        d = {}
        for plot in plots:
            # calculate the relative pathnames as seen from the browser
            thumbnail_relpath = os.path.relpath(plot.thumbnail,
                                                self.context.report_dir)
            image_relpath = os.path.relpath(plot.abspath,
                                            self.context.report_dir)
            field = plot.parameters['field']
            baseband_id = plot.parameters['baseband']

            # Javascript JSON parser doesn't like Javascript floating point 
            # constants (NaN, Infinity etc.), so convert them to null. We  
            # do not omit the dictionary entry so that the plot is hidden
            # by the filters.
#             if math.isnan(ratio) or math.isinf(ratio):
#                 ratio = 'null'

            d[image_relpath] = {'baseband'  : str(baseband_id),
                                'field'     : field,
                                'thumbnail' : thumbnail_relpath}

        # Javascript parser requires \" -> \\" conversion 
        self.json = json.dumps(d).replace('\"', '\\"')


class ApplycalAmpVsFreqPlotRenderer(GenericFieldSpwAntPlotsRenderer):
    @property
    def filename(self):        
        filename = filenamer.sanitize('amp_vs_freq-%s.html' % self.ms)
        return filename

    def _get_display_context(self):
        d = super(ApplycalAmpVsFreqPlotRenderer, self)._get_display_context()
        d['plot_title'] = 'Calibrated amplitude vs frequency for %s' % self.ms
        return d


class ApplycalPhaseVsFreqPlotRenderer(GenericFieldSpwAntPlotsRenderer):
    @property
    def filename(self):        
        filename = filenamer.sanitize('phase_vs_freq-%s.html' % self.ms)
        return filename

    def _get_display_context(self):
        d = super(ApplycalPhaseVsFreqPlotRenderer, self)._get_display_context()
        d['plot_title'] = 'Calibrated phase vs frequency for %s' % self.ms
        return d


class ApplycalAmpVsFreqSciencePlotRenderer(GenericFieldBasebandPlotsRenderer):
    @property
    def filename(self):        
        filename = filenamer.sanitize('science_amp_vs_freq-%s.html' % self.ms)
        return filename

    def _get_display_context(self):
        d = super(ApplycalAmpVsFreqSciencePlotRenderer, self)._get_display_context()
        d['plot_title'] = 'Calibrated amplitude vs frequency for %s' % self.ms
        return d


class ApplycalPhaseVsFreqSciencePlotRenderer(GenericFieldBasebandPlotsRenderer):
    @property
    def filename(self):        
        filename = filenamer.sanitize('science_phase_vs_freq-%s.html' % self.ms)
        return filename

    def _get_display_context(self):
        d = super(ApplycalPhaseVsFreqSciencePlotRenderer, self)._get_display_context()
        d['plot_title'] = 'Calibrated phase vs frequency for %s' % self.ms
        return d


class ApplycalAmpVsUVSciencePlotRenderer(GenericFieldBasebandPlotsRenderer):
    @property
    def filename(self):        
        filename = filenamer.sanitize('science_amp_vs_uv-%s.html' % self.ms)
        return filename

    def _get_display_context(self):
        d = super(ApplycalAmpVsUVSciencePlotRenderer, self)._get_display_context()
        d['plot_title'] = 'Calibrated amplitude vs UV distance for %s' % self.ms
        return d


class ApplycalAmpVsUVPlotRenderer(GenericPlotsRenderer):
    @property
    def filename(self):        
        filename = filenamer.sanitize('amp_vs_uv-%s.html' % self.ms)
        return filename

    def _get_display_context(self):
        d = super(ApplycalAmpVsUVPlotRenderer, self)._get_display_context()
        d['plot_title'] = 'Calibrated amplitude vs UV distance for %s' % self.ms
        return d


class ApplycalPhaseVsUVPlotRenderer(GenericPlotsRenderer):
    @property
    def filename(self):        
        filename = filenamer.sanitize('amp_vs_phase-%s.html' % self.ms)
        return filename

    def _get_display_context(self):
        d = super(ApplycalPhaseVsUVPlotRenderer, self)._get_display_context()
        d['plot_title'] = 'Calibrated phase vs UV distance for %s' % self.ms
        return d


class ApplycalAmpVsTimePlotRenderer(GenericFieldSpwAntPlotsRenderer):
    @property
    def filename(self):        
        filename = filenamer.sanitize('amp_vs_time-%s.html' % self.ms)
        return filename

    def _get_display_context(self):
        d = super(ApplycalAmpVsTimePlotRenderer, self)._get_display_context()
        d['plot_title'] = 'Calibrated phase vs time for %s' % self.ms
        return d


class ApplycalPhaseVsTimePlotRenderer(GenericFieldSpwAntPlotsRenderer):
    @property
    def filename(self):        
        filename = filenamer.sanitize('phase_vs_time-%s.html' % self.ms)
        return filename

    def _get_display_context(self):
        d = super(ApplycalPhaseVsTimePlotRenderer, self)._get_display_context()
        d['plot_title'] = 'Calibrated phase vs time for %s' % self.ms
        return d


class T2_4MDetailsLowgainFlagRenderer(T2_4MDetailsDefaultRenderer):
    '''
    Renders detailed HTML output for the Lowgainflag task.
    '''
    def __init__(self, template='t2-4m_details-hif_lowgainflag.html',
            always_rerender=False):
        super(T2_4MDetailsLowgainFlagRenderer, self).__init__(template,
                always_rerender)

    def get_display_context(self, context, results):
        super_cls = super(T2_4MDetailsLowgainFlagRenderer, self)
        ctx = super_cls.get_display_context(context, results)

        htmlreports = self.get_htmlreports(context, results)
        
        ctx.update({'htmlreports' : htmlreports})
        return ctx

    def get_htmlreports(self, context, results):
        report_dir = context.report_dir
        weblog_dir = os.path.join(report_dir,
                                  'stage%s' % results.stage_number)

        htmlreports = {}
        for result in results:
#            if not hasattr(result, 'flagcmdfile'):
#                continue

            flagcmd_abspath = self.write_flagcmd_to_disk(weblog_dir, result)
            flagcmd_relpath = os.path.relpath(flagcmd_abspath, report_dir)
            table_basename = os.path.basename(result.table)
            htmlreports[table_basename] = (flagcmd_relpath,)

        return htmlreports

    def write_flagcmd_to_disk(self, weblog_dir, result):
        tablename = os.path.basename(result.table)
        filename = os.path.join(weblog_dir, '%s.html' % tablename)
        if os.path.exists(filename):
            return filename

        rendererutils.renderflagcmds(result.flagcmds(), filename)
        return filename


class T2_4MDetailsTsysflagchansRenderer(T2_4MDetailsDefaultRenderer):
    '''
    Renders detailed HTML output for the Tsysflagchans task.
    '''
    def __init__(self, template='t2-4m_details-hif_tsysflagchans.html',
                 always_rerender=False):
        super(T2_4MDetailsTsysflagchansRenderer, self).__init__(template,
                                                                   always_rerender)

    def get_display_context(self, context, results):
        super_cls = super(T2_4MDetailsTsysflagchansRenderer, self)
        ctx = super_cls.get_display_context(context, results)

        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % results.stage_number)

        htmlreports = self.get_html_reports(context, results)
        plot_groups = self.get_plots(context, results)

        summary_plots = {}
        subpages = {}
        slplots = []
        for result in results:
            if result.view:
                plotter = slicedisplay.SliceDisplay()
                slplots.append(plotter.plot(context, result, weblog_dir, 
                                            plotbad=False,
                                            plot_only_flagged=True))
            
            if result.flagged() is False:
                continue

            plotter = tsys.TsysSummaryChart(context, result)
            plots = plotter.plot()
            ms = os.path.basename(result.inputs['vis'])
            summary_plots[ms] = plots

            # generate the per-antenna charts and JSON file
            plotter = tsys.ScoringTsysPerAntennaChart(context, result)
            plots = plotter.plot() 
            json_path = plotter.json_filename

            # write the html for each MS to disk
            renderer = TsyscalPlotRenderer(context, result, plots, json_path)
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                # the filename is sanitised - the MS name is not. We need to
                # map MS to sanitised filename for link construction.
                subpages[ms] = renderer.filename


        # add the PlotGroups to the Mako context. The Mako template will parse
        # these objects in order to create links to the thumbnail pages we
        # just created
        ctx.update({'plot_groups'     : plot_groups,
                    'summary_plots'   : summary_plots,
                    'summary_subpage' : subpages,
                    'dirname'         : weblog_dir,
                    'htmlreports'     : htmlreports})

        return ctx

    def get_plots(self, context, results):
        report_dir = context.report_dir
        weblog_dir = os.path.join(report_dir,
                                  'stage%s' % results.stage_number)

        plots = []
        for result in results:
            if result.view:
                plots.append(slicedisplay.SliceDisplay().plot(
                    context=context, results=result, reportdir=weblog_dir,
                    plotbad=False, plot_only_flagged=True))

        # Group the Plots by axes and plot types; each logical grouping will
        # be contained in a PlotGroup
        plot_groups = logger.PlotGroup.create_plot_groups(plots)
        # Write the thumbnail pages for each plot grouping to disk
        for plot_group in plot_groups:
            renderer = PlotGroupRenderer(context, results, plot_group)
            plot_group.filename = renderer.filename
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                
        return plot_groups


    def get_html_reports(self, context, results):
        report_dir = context.report_dir
        weblog_dir = os.path.join(report_dir,
                                  'stage%s' % results.stage_number)

        htmlreports = {}
        for result in results:
#            if not hasattr(result, 'flagcmdfile'):
#                continue

            flagcmd_abspath = self.write_flagcmd_to_disk(weblog_dir, result)
            report_abspath = self.write_report_to_disk(weblog_dir, result)

            flagcmd_relpath = os.path.relpath(flagcmd_abspath, report_dir)
            report_relpath = os.path.relpath(report_abspath, report_dir)

            table_basename = os.path.basename(result.table)
            htmlreports[table_basename] = (flagcmd_relpath, report_relpath)

        return htmlreports

    def write_flagcmd_to_disk(self, weblog_dir, result):
        tablename = os.path.basename(result.table)
        filename = os.path.join(weblog_dir, '%s.html' % tablename)
        if os.path.exists(filename):
            return filename

        rendererutils.renderflagcmds(result.flagcmds(), filename)
        return filename

    def write_report_to_disk(self, weblog_dir, result):
        # now write printTsysFlags output to a report file
        tablename = os.path.basename(result.table)
        filename = os.path.join(weblog_dir, '%s.report.html' % tablename)
        if os.path.exists(filename):
            return filename

        rendererutils.printTsysFlags(result.table, filename)
        return filename


class T2_4MDetailsTsysflagRenderer(T2_4MDetailsDefaultRenderer):
    '''
    Renders detailed HTML output for the Tsysflag task.
    '''
    def __init__(self, template='t2-4m_details-hif_tsysflag.html',
                 always_rerender=False):
        super(T2_4MDetailsTsysflagRenderer, self).__init__(template,
                                                              always_rerender)

    def get_display_context(self, context, results):
        super_cls = super(T2_4MDetailsTsysflagRenderer, self)
        ctx = super_cls.get_display_context(context, results)

        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % results.stage_number)

        flag_totals = {}
        components = ['nmedian', 'derivative', 'edgechans', 'fieldshape', 'birdies']
        for msresult in results:
            table = os.path.basename(msresult.inputs['caltable'])
            flag_totals[table] = {}

            # summarise flag state on entry
            flag_totals[table]['before'] = self.flags_for_result(msresult, context,
              summary='first')

            # summarise flagging by each step
            for component in components:
                if component not in msresult.components.keys():
                    flag_totals[table][component] = None
                else:
                    flag_totals[table][component] = self.flags_for_result(
                      msresult.components[component], context)

            # summarise flag state on exit
            flag_totals[table]['after'] = self.flags_for_result(msresult, context,
              summary='last')

        htmlreports = self.get_htmlreports(context, results)
        
        summary_plots = {}
        subpages = {}
        for msresult in results:
            # summary plots at end of flagging sequence, beware empty
            # sequence
            lastflag = msresult.components.keys()
            if lastflag:
                lastflag = lastflag[-1]
            lastresult = msresult.components[lastflag]

            plotter = tsys.TsysSummaryChart(context, lastresult)
            plots = plotter.plot()
            ms = os.path.basename(lastresult.inputs['vis'])
            summary_plots[ms] = plots

            # generate the per-antenna charts and JSON file
            plotter = tsys.ScoringTsysPerAntennaChart(context, lastresult)
            plots = plotter.plot() 
            json_path = plotter.json_filename

            # write the html for each MS to disk
            renderer = TsyscalPlotRenderer(context, lastresult, plots, 
              json_path)
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                # the filename is sanitised - the MS name is not. We need to
                # map MS to sanitised filename for link construction.
                subpages[ms] = renderer.filename

        ctx.update({'flags'           : flag_totals,
                    'components'      : components,
                    'summary_plots'   : summary_plots,
                    'summary_subpage' : subpages,
                    'dirname'         : weblog_dir,
                    'htmlreports'     : htmlreports})

        return ctx

    def get_htmlreports(self, context, results):
        report_dir = context.report_dir
        weblog_dir = os.path.join(report_dir,
                                  'stage%s' % results.stage_number)

        r = results[0]
        components = r.components.keys()

        htmlreports = {}

        for component in components:
            htmlreports[component] = {}

            for msresult in results:
                flagcmd_abspath = self.write_flagcmd_to_disk(weblog_dir, 
                  msresult.components[component], component)
                report_abspath = self.write_report_to_disk(weblog_dir, 
                  msresult.components[component], component)

                flagcmd_relpath = os.path.relpath(flagcmd_abspath, report_dir)
                report_relpath = os.path.relpath(report_abspath, report_dir)

                table_basename = os.path.basename(
                  msresult.components[component].table)
                htmlreports[component][table_basename] = \
                  (flagcmd_relpath, report_relpath)

        return htmlreports

    def write_flagcmd_to_disk(self, weblog_dir, result, component=None):
        tablename = os.path.basename(result.table)
        if component:
            filename = os.path.join(weblog_dir, '%s%s.html' % (tablename, component))
        else:
            filename = os.path.join(weblog_dir, '%s.html' % (tablename))

        rendererutils.renderflagcmds(result.flagcmds(), filename)
        return filename

    def write_report_to_disk(self, weblog_dir, result, component=None):
        # now write printTsysFlags output to a report file
        tablename = os.path.basename(result.table)
        if component:
            filename = os.path.join(weblog_dir, '%s%s.report.html' % (tablename, 
              component))
        else:
            filename = os.path.join(weblog_dir, '%s.report.html' % (tablename))
        if os.path.exists(filename):
            return filename
        
        rendererutils.printTsysFlags(result.table, filename)
        return filename

    def flags_for_result(self, result, context, summary=None):
        name = result.inputs['caltable']
        tsystable = caltableaccess.CalibrationTableDataFiller.getcal(name)
        ms = context.observing_run.get_ms(name=tsystable.vis) 

        summaries = result.summaries
        if summary=='first':
            summaries = summaries[:1]
        elif summary=='last':
            summaries = summaries[-1:]

        by_intent = self.flags_by_intent(ms, summaries)
        by_spw = self.flags_by_spws(ms, summaries)

        return utils.dict_merge(by_intent, by_spw)

    def flags_by_intent(self, ms, summaries):
        # create a dictionary of fields per observing intent, eg. 'PHASE':['3C273']
        intent_fields = {}
        for intent in ('BANDPASS', 'PHASE', 'AMPLITUDE', 'TARGET', 'ATMOSPHERE'):
            # use _name from field as we do want the raw name here as used
            # in the summaries dict (not sometimes enclosed in "..."). Better
            # perhaps to fix the summaries dict.
            intent_fields[intent] = [f._name for f in ms.fields
                                    if intent in f.intents]

        # while we're looping, get the total flagged by looking in all scans 
        intent_fields['TOTAL'] = [f._name for f in ms.fields]

        total = collections.defaultdict(dict)

        previous_summary = None
        for summary in summaries:

            for intent, fields in intent_fields.items():
                flagcount = 0
                totalcount = 0
    
                for field in fields:
                    if field in summary['field'].keys():
                        flagcount += int(summary['field'][field]['flagged'])
                        totalcount += int(summary['field'][field]['total'])
        
                    if previous_summary:
                        if field in previous_summary['field'].keys():
                            flagcount -= int(previous_summary['field'][field]['flagged'])

                ft = T2_4MDetailsAgentFlaggerRenderer.FlagTotal(flagcount,
                                                                totalcount)
                total[summary['name']][intent] = ft
    
            previous_summary = summary
                
        return total 
    
    def flags_by_spws(self, ms, summaries):
        total = collections.defaultdict(dict)
    
        previous_summary = None
        for summary in summaries:
            tsys_spws = summary['spw'].keys()
    
            flagcount = 0
            totalcount = 0
    
            for spw in tsys_spws:
                try:
                    flagcount += int(summary['spw'][spw]['flagged'])
                    totalcount += int(summary['spw'][spw]['total'])
                except:
                    print spw, summary['spw'].keys()

                if previous_summary:
                    flagcount -= int(previous_summary['spw'][spw]['flagged'])

            ft = T2_4MDetailsAgentFlaggerRenderer.FlagTotal(flagcount,
                                                            totalcount)
            total[summary['name']]['TSYS SPWS'] = ft

            previous_summary = summary
                
        return total


class T2_4MDetailsTsysflagspectraRenderer(T2_4MDetailsDefaultRenderer):
    '''
    Renders detailed HTML output for the Tsysflagspectra task.
    '''
    def __init__(self, template='t2-4m_details-hif_tsysflagspectra.html',
                 always_rerender=False):
        super(T2_4MDetailsTsysflagspectraRenderer, self).__init__(template,
                                                              always_rerender)

    def get_display_context(self, context, results):
        super_cls = super(T2_4MDetailsTsysflagspectraRenderer, self)
        ctx = super_cls.get_display_context(context, results)

        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % results.stage_number)

        htmlreports = self.get_htmlreports(context, results)
        
        summary_plots = {}
        subpages = {}
        for result in results:
            if result.flagged() is False:
                continue
            
            plotter = tsys.TsysSummaryChart(context, result)
            plots = plotter.plot()
            ms = os.path.basename(result.inputs['vis'])
            summary_plots[ms] = plots

            # generate the per-antenna charts and JSON file
            plotter = tsys.ScoringTsysPerAntennaChart(context, result)
            plots = plotter.plot() 
            json_path = plotter.json_filename

            # write the html for each MS to disk
            renderer = TsyscalPlotRenderer(context, result, plots, json_path)
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                # the filename is sanitised - the MS name is not. We need to
                # map MS to sanitised filename for link construction.
                subpages[ms] = renderer.filename

        ctx.update({'summary_plots'   : summary_plots,
                    'summary_subpage' : subpages,
                    'dirname'         : weblog_dir,
                    'htmlreports'     : htmlreports})

        return ctx

    def get_htmlreports(self, context, results):
        report_dir = context.report_dir
        weblog_dir = os.path.join(report_dir,
                                  'stage%s' % results.stage_number)

        htmlreports = {}
        for result in results:
#            if not hasattr(result, 'flagcmdfile'):
#                continue

            flagcmd_abspath = self.write_flagcmd_to_disk(weblog_dir, result)
            report_abspath = self.write_report_to_disk(weblog_dir, result)

            flagcmd_relpath = os.path.relpath(flagcmd_abspath, report_dir)
            report_relpath = os.path.relpath(report_abspath, report_dir)

            table_basename = os.path.basename(result.table)
            htmlreports[table_basename] = (flagcmd_relpath, report_relpath)

        return htmlreports

    def write_flagcmd_to_disk(self, weblog_dir, result):
        tablename = os.path.basename(result.table)
        filename = os.path.join(weblog_dir, '%s.html' % tablename)
        if os.path.exists(filename):
            return filename

        rendererutils.renderflagcmds(result.flagcmds(), filename)
        return filename

    def write_report_to_disk(self, weblog_dir, result):
        # now write printTsysFlags output to a report file
        tablename = os.path.basename(result.table)
        filename = os.path.join(weblog_dir, '%s.report.html' % tablename)
        if os.path.exists(filename):
            return filename
        
        rendererutils.printTsysFlags(result.table, filename)
        return filename


class T2_4MDetailsTsyscalRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hif_tsyscal.html', 
                 always_rerender=False):
        super(T2_4MDetailsTsyscalRenderer, self).__init__(template,
                                                          always_rerender)

    def get_display_context(self, context, results):
        super_cls = super(T2_4MDetailsTsyscalRenderer, self)
        ctx = super_cls.get_display_context(context, results)

        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % results.stage_number)

        summary_plots = {}
        subpages = {}
        for result in results:
            plotter = tsys.TsysSummaryChart(context, result)
            plots = plotter.plot()
            ms = os.path.basename(result.inputs['vis'])
            summary_plots[ms] = plots

            # generate the per-antenna charts and JSON file
            plotter = tsys.ScoringTsysPerAntennaChart(context, result)
            plots = plotter.plot() 
            json_path = plotter.json_filename

            # write the html for each MS to disk
            renderer = TsyscalPlotRenderer(context, result, plots, json_path)
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                # the filename is sanitised - the MS name is not. We need to
                # map MS to sanitised filename for link construction.
                subpages[ms] = renderer.filename

        ctx.update({'summary_plots'   : summary_plots,
                    'summary_subpage' : subpages,
                    'dirname'         : weblog_dir})

        return ctx


class TsyscalPlotRenderer(object):
    template = 'tsyscal_plots.html'

    def __init__(self, context, result, plots, json_path):
        self.context = context
        self.result = result
        self.plots = plots
        self.ms = os.path.basename(self.result.inputs['vis'])

        if os.path.exists(json_path):
            with open(json_path, 'r') as json_file:
                self.json = json_file.readlines()[0]
        else:
            self.json = '{}'
         
    def _get_display_context(self):
        return {'pcontext'   : self.context,
                'result'     : self.result,
                'plots'      : self.plots,
                'dirname'    : self.dirname,
                'json'       : self.json,
                'plot_title' : 'T<sub>sys</sub> plots for %s' % self.ms}

    @property
    def dirname(self):
        stage = 'stage%s' % self.result.stage_number
        return os.path.join(self.context.report_dir, stage)
    
    @property
    def filename(self):        
        filename = filenamer.sanitize('tsys-%s.html' % self.ms)
        return filename
    
    @property
    def path(self):
        return os.path.join(self.dirname, self.filename)
    
    def get_file(self, hardcopy=True):
        if hardcopy and not os.path.exists(self.dirname):
            os.makedirs(self.dirname)
            
        file_obj = open(self.path, 'w') if hardcopy else StdOutFile()
        return contextlib.closing(file_obj)
    
    def render(self):
        display_context = self._get_display_context()
        t = TemplateFinder.get_template(self.template)
        return t.render(**display_context)

#-----------------------------------------------------------------------

class T2_4MDetailsfinalcalsRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hifv_finalcals.html', 
                 always_rerender=False):
        super(T2_4MDetailsfinalcalsRenderer, self).__init__(template,
                                                          always_rerender)
    
    def get_display_context(self, context, results):
        super_cls = super(T2_4MDetailsfinalcalsRenderer, self)
        ctx = super_cls.get_display_context(context, results)
        
        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % results.stage_number)
        
        summary_plots = {}
        finaldelay_subpages = {}
        phasegain_subpages = {}
        bpsolamp_subpages = {}
        bpsolphase_subpages = {}
        bpsolphaseshort_subpages = {}
        finalamptimecal_subpages = {}
        finalampfreqcal_subpages = {}
        finalphasegaincal_subpages = {}
        
        for result in results:
            
            plotter = finalcalsdisplay.finalcalsSummaryChart(context, result)
            plots = plotter.plot()
            ms = os.path.basename(result.inputs['vis'])
            summary_plots[ms] = plots
            
            # generate testdelay plots and JSON file
            plotter = finalcalsdisplay.finalDelaysPerAntennaChart(context, result)
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'finalcals_plots.html', 'finaldelays')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                finaldelay_subpages[ms] = renderer.filename
            
                
            # generate phase Gain plots and JSON file
            plotter = finalcalsdisplay.finalphaseGainPerAntennaChart(context, result)
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'finalcals_plots.html', 'phasegain')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                phasegain_subpages[ms] = renderer.filename
                
            # generate amp bandpass solution plots and JSON file
            plotter = finalcalsdisplay.finalbpSolAmpPerAntennaChart(context, result)
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'finalcals_plots.html', 'bpsolamp')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                bpsolamp_subpages[ms] = renderer.filename
                
            # generate phase bandpass solution plots and JSON file
            plotter = finalcalsdisplay.finalbpSolPhasePerAntennaChart(context, result)
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'finalcals_plots.html', 'bpsolphase')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                bpsolphase_subpages[ms] = renderer.filename
                
            # generate phase short bandpass solution plots and JSON file
            plotter = finalcalsdisplay.finalbpSolPhaseShortPerAntennaChart(context, result)
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'finalcals_plots.html', 'bpsolphaseshort')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                bpsolphaseshort_subpages[ms] = renderer.filename
            
            # generate final amp time cal solution plots and JSON file
            plotter = finalcalsdisplay.finalAmpTimeCalPerAntennaChart(context, result)
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'finalcals_plots.html', 'finalamptimecal')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                finalamptimecal_subpages[ms] = renderer.filename
                
            # generate final amp freq cal solution plots and JSON file
            plotter = finalcalsdisplay.finalAmpFreqCalPerAntennaChart(context, result)
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'finalcals_plots.html', 'finalampfreqcal')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                finalampfreqcal_subpages[ms] = renderer.filename
        
            # generate final phase gain cal solution plots and JSON file
            plotter = finalcalsdisplay.finalPhaseGainCalPerAntennaChart(context, result)
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'finalcals_plots.html', 'finalphasegaincal')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                finalphasegaincal_subpages[ms] = renderer.filename
        
        ctx.update({'summary_plots'   : summary_plots,
                    'finaldelay_subpages' : finaldelay_subpages,
                    'phasegain_subpages' : phasegain_subpages,
                    'bpsolamp_subpages'  : bpsolamp_subpages,
                    'bpsolphase_subpages' : bpsolphase_subpages,
                    'bpsolphaseshort_subpages' : bpsolphaseshort_subpages,
                    'finalamptimecal_subpages' : finalamptimecal_subpages,
                    'finalampfreqcal_subpages' : finalampfreqcal_subpages,
                    'finalphasegaincal_subpages' : finalphasegaincal_subpages,
                    'dirname'         : weblog_dir})
                
        return ctx


class T2_4MDetailsHeuristicFlagRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hifv_semifinalbpdcals.html', 
                 always_rerender=False):
        super(T2_4MDetailsHeuristicFlagRenderer, self).__init__(template,
                                                          always_rerender)
    
    def get_display_context(self, context, results):
        super_cls = super(T2_4MDetailsHeuristicFlagRenderer, self)
        ctx = super_cls.get_display_context(context, results)
        
        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % results.stage_number)
        
        testsummary_plots = {}
        testdelay_subpages = {}
        testampgain_subpages = {}
        testphasegain_subpages = {}
        testbpsolamp_subpages = {}
        testbpsolphase_subpages = {}
        
        semifinalsummary_plots1 = {}
        semifinaldelay_subpages1 = {}
        semifinalampgain_subpages1 = {}
        semifinalphasegain_subpages1 = {}
        semifinalbpsolamp_subpages1 = {}
        bpsolphase_subpages1 = {}
        
        semifinalsummary_plots2 = {}
        semifinaldelay_subpages2 = {}
        semifinalampgain_subpages2 = {}
        semifinalphasegain_subpages2 = {}
        semifinalbpsolamp_subpages2 = {}
        bpsolphase_subpages2 = {}
        
        for result in results:
            
            plotter = semifinalBPdcalsdisplay.semifinalBPdcalsSummaryChart(context, result, suffix='1')
            plots = plotter.plot()
            ms = os.path.basename(result.inputs['vis'])
            summary_plots[ms] = plots
            
            # generate testdelay plots and JSON file
            plotter = semifinalBPdcalsdisplay.DelaysPerAntennaChart(context, result, suffix='1')
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'semifinalcals_plots.html', 'delays')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                delay_subpages[ms] = renderer.filename
            
                
            # generate phase Gain plots and JSON file
            plotter = semifinalBPdcalsdisplay.semifinalphaseGainPerAntennaChart(context, result, suffix='1')
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'semifinalcals_plots.html', 'phasegain')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                phasegain_subpages[ms] = renderer.filename
                
            # generate amp bandpass solution plots and JSON file
            plotter = semifinalBPdcalsdisplay.semifinalbpSolAmpPerAntennaChart(context, result, suffix='1')
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'semifinalcals_plots.html', 'bpsolamp')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                bpsolamp_subpages[ms] = renderer.filename
                
            # generate phase bandpass solution plots and JSON file
            plotter = semifinalBPdcalsdisplay.semifinalbpSolPhasePerAntennaChart(context, result, suffix='1')
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'semifinalcals_plots.html', 'bpsolphase')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                bpsolphase_subpages[ms] = renderer.filename
        
        ctx.update({'summary_plots'   : summary_plots,
                    'delay_subpages' : delay_subpages,
                    'phasegain_subpages' : phasegain_subpages,
                    'bpsolamp_subpages'  : bpsolamp_subpages,
                    'bpsolphase_subpages' : bpsolphase_subpages,
                    'dirname'         : weblog_dir})
                
        return ctx







class T2_4MDetailssemifinalBPdcalsRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hifv_semifinalbpdcals.html', 
                 always_rerender=False):
        super(T2_4MDetailssemifinalBPdcalsRenderer, self).__init__(template,
                                                          always_rerender)
    
    def get_display_context(self, context, results):
        super_cls = super(T2_4MDetailssemifinalBPdcalsRenderer, self)
        ctx = super_cls.get_display_context(context, results)
        
        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % results.stage_number)
        
        summary_plots = {}
        delay_subpages = {}
        ampgain_subpages = {}
        phasegain_subpages = {}
        bpsolamp_subpages = {}
        bpsolphase_subpages = {}
        
        suffix=''
        
        for result in results:
            
            plotter = semifinalBPdcalsdisplay.semifinalBPdcalsSummaryChart(context, result, suffix=suffix)
            plots = plotter.plot()
            ms = os.path.basename(result.inputs['vis'])
            summary_plots[ms] = plots
            
            # generate testdelay plots and JSON file
            plotter = semifinalBPdcalsdisplay.DelaysPerAntennaChart(context, result, suffix=suffix)
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'semifinalcals_plots.html', 'delays')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                delay_subpages[ms] = renderer.filename
            
                
            # generate phase Gain plots and JSON file
            plotter = semifinalBPdcalsdisplay.semifinalphaseGainPerAntennaChart(context, result, suffix=suffix)
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'semifinalcals_plots.html', 'phasegain')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                phasegain_subpages[ms] = renderer.filename
                
            # generate amp bandpass solution plots and JSON file
            plotter = semifinalBPdcalsdisplay.semifinalbpSolAmpPerAntennaChart(context, result, suffix=suffix)
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'semifinalcals_plots.html', 'bpsolamp')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                bpsolamp_subpages[ms] = renderer.filename
                
            # generate phase bandpass solution plots and JSON file
            plotter = semifinalBPdcalsdisplay.semifinalbpSolPhasePerAntennaChart(context, result, suffix=suffix)
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'semifinalcals_plots.html', 'bpsolphase')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                bpsolphase_subpages[ms] = renderer.filename
        
        ctx.update({'summary_plots'   : summary_plots,
                    'delay_subpages' : delay_subpages,
                    'phasegain_subpages' : phasegain_subpages,
                    'bpsolamp_subpages'  : bpsolamp_subpages,
                    'bpsolphase_subpages' : bpsolphase_subpages,
                    'dirname'         : weblog_dir})
                
        return ctx


class T2_4MDetailstestBPdcalsRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hifv_testbpdcals.html', 
                 always_rerender=False):
        super(T2_4MDetailstestBPdcalsRenderer, self).__init__(template,
                                                          always_rerender)
    
    def get_display_context(self, context, results):
        super_cls = super(T2_4MDetailstestBPdcalsRenderer, self)
        ctx = super_cls.get_display_context(context, results)
        
        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % results.stage_number)
        
        summary_plots = {}
        testdelay_subpages = {}
        ampgain_subpages = {}
        phasegain_subpages = {}
        bpsolamp_subpages = {}
        bpsolphase_subpages = {}
        
        for result in results:
            
            plotter = testBPdcalsdisplay.testBPdcalsSummaryChart(context, result)
            plots = plotter.plot()
            ms = os.path.basename(result.inputs['vis'])
            summary_plots[ms] = plots
            
            # generate testdelay plots and JSON file
            plotter = testBPdcalsdisplay.testDelaysPerAntennaChart(context, result)
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'testdelays_plots.html', 'testdelays')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                testdelay_subpages[ms] = renderer.filename
            
            # generate amp Gain plots and JSON file
            plotter = testBPdcalsdisplay.ampGainPerAntennaChart(context, result)
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'ampgain_plots.html', 'ampgain')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                ampgain_subpages[ms] = renderer.filename
                
            # generate phase Gain plots and JSON file
            plotter = testBPdcalsdisplay.phaseGainPerAntennaChart(context, result)
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'phasegain_plots.html', 'phasegain')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                phasegain_subpages[ms] = renderer.filename
                
            # generate amp bandpass solution plots and JSON file
            plotter = testBPdcalsdisplay.bpSolAmpPerAntennaChart(context, result)
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'bpsolamp_plots.html', 'bpsolamp')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                bpsolamp_subpages[ms] = renderer.filename
                
            # generate phase bandpass solution plots and JSON file
            plotter = testBPdcalsdisplay.bpSolPhasePerAntennaChart(context, result)
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'bpsolphase_plots.html', 'bpsolphase')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                bpsolphase_subpages[ms] = renderer.filename
        
        ctx.update({'summary_plots'   : summary_plots,
                    'testdelay_subpages' : testdelay_subpages,
                    'ampgain_subpages'   : ampgain_subpages,
                    'phasegain_subpages' : phasegain_subpages,
                    'bpsolamp_subpages'  : bpsolamp_subpages,
                    'bpsolphase_subpages' : bpsolphase_subpages,
                    'dirname'         : weblog_dir})
                
        return ctx



class T2_4MDetailspriorcalsRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hifv_priorcals.html', 
                 always_rerender=False):
        super(T2_4MDetailspriorcalsRenderer, self).__init__(template,
                                                          always_rerender)
    
    def get_display_context(self, context, results):
        super_cls = super(T2_4MDetailspriorcalsRenderer, self)
        ctx = super_cls.get_display_context(context, results)
        
        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % results.stage_number)
        
        opacity_plots = {}
        spw = {}
        center_frequencies = {}
        opacities = {}
        
        for result in results:
            
            ms = os.path.basename(result.inputs['vis'])
            spw[ms] = result.oc_result[0].spw.split(',')
            center_frequencies[ms] = result.oc_result[0].center_frequencies
            opacities[ms] = result.oc_result[0].opacities
            
            plotter = opacitiesdisplay.opacitiesSummaryChart(context, result)
            plots = plotter.plot()
            opacity_plots[ms] = plots
            
            # generate testdelay plots and JSON file
            #plotter = testgainsdisplay.testgainsPerAntennaChart(context, result, 'amp')
            #plots = plotter.plot() 
            #json_path = plotter.json_filename
            
             # write the html for each MS to disk
            #renderer = VLASubPlotRenderer(context, result, plots, json_path, 'testgains_plots.html', 'amp')
            #with renderer.get_file() as fileobj:
            #    fileobj.write(renderer.render())
            #    testgainsamp_subpages[ms] = renderer.filename
           
        
        ctx.update({'opacity_plots'        : opacity_plots,
                    'spw'                  : spw,
                    'center_frequencies'   : center_frequencies,
                    'opacities'            : opacities,
                    'dirname'              : weblog_dir})
                
        return ctx




class T2_4MDetailsSolintRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hifv_testgains.html', 
                 always_rerender=False):
        super(T2_4MDetailsSolintRenderer, self).__init__(template,
                                                          always_rerender)
    
    def get_display_context(self, context, results):
        super_cls = super(T2_4MDetailsSolintRenderer, self)
        ctx = super_cls.get_display_context(context, results)
        
        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % results.stage_number)
        
        summary_plots = {}
        testgainsamp_subpages = {}
        testgainsphase_subpages = {}
        
        longsolint = {}
        gain_solint2 = {}
        
        shortsol2 = {}
        short_solint = {}
        new_gain_solint1 = {}
        
        
        for result in results:
            
            plotter = testgainsdisplay.testgainsSummaryChart(context, result)
            plots = plotter.plot()
            ms = os.path.basename(result.inputs['vis'])
            summary_plots[ms] = plots
            
            # generate testdelay plots and JSON file
            plotter = testgainsdisplay.testgainsPerAntennaChart(context, result, 'amp')
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'testgains_plots.html', 'amp')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                testgainsamp_subpages[ms] = renderer.filename
            
            # generate amp Gain plots and JSON file
            plotter = testgainsdisplay.testgainsPerAntennaChart(context, result, 'phase')
            plots = plotter.plot() 
            json_path = plotter.json_filename
            
             # write the html for each MS to disk
            renderer = VLASubPlotRenderer(context, result, plots, json_path, 'testgains_plots.html', 'phase')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
                testgainsphase_subpages[ms] = renderer.filename
           
            #String type
            new_gain_solint1[ms] = result.new_gain_solint1
            longsolint[ms]       = result.longsolint
            

        ctx.update({'summary_plots'   : summary_plots,
                    'testgainsamp_subpages' : testgainsamp_subpages,
                    'testgainsphase_subpages'   : testgainsphase_subpages,
                    'new_gain_solint1'           : new_gain_solint1,
                    'longsolint'                : longsolint,
                    'dirname'         : weblog_dir})
                
        return ctx
        

class T2_4MDetailsfluxbootRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hifv_fluxboot.html', 
                 always_rerender=False):
        super(T2_4MDetailsfluxbootRenderer, self).__init__(template,
                                                          always_rerender)
    
    def get_display_context(self, context, results):
        super_cls = super(T2_4MDetailsfluxbootRenderer, self)
        ctx = super_cls.get_display_context(context, results)
        
        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % results.stage_number)
        
        summary_plots = {}
        weblog_results = {}
        spindex_results = {}

        for result in results:
            
            plotter = fluxbootdisplay.fluxbootSummaryChart(context, result)
            plots = plotter.plot()
            ms = os.path.basename(result.inputs['vis'])
            summary_plots[ms] = plots
            weblog_results[ms] = result.weblog_results
            spindex_results[ms] = result.spindex_results
            
        ctx.update({'summary_plots'   : summary_plots,
                    'weblog_results'  : weblog_results,
                    'spindex_results' : spindex_results,
                    'dirname'         : weblog_dir})
                
        return ctx

class T2_4MDetailstargetflagRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hifv_targetflag.html', 
                 always_rerender=False):
        super(T2_4MDetailstargetflagRenderer, self).__init__(template,
                                                          always_rerender)
    
    def get_display_context(self, context, results):
        super_cls = super(T2_4MDetailstargetflagRenderer, self)
        ctx = super_cls.get_display_context(context, results)
        
        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % results.stage_number)
        
        summary_plots = {}

        for result in results:
            
            plotter = targetflagdisplay.targetflagSummaryChart(context, result)
            plots = plotter.plot()
            ms = os.path.basename(result.inputs['vis'])
            summary_plots[ms] = plots
            
        ctx.update({'summary_plots'   : summary_plots,
                    'dirname'         : weblog_dir})
                
        return ctx

class VLASubPlotRenderer(object):
    #template = 'testdelays_plots.html'
    
    def __init__(self, context, result, plots, json_path, template, filename_prefix):
        self.context = context
        self.result = result
        self.plots = plots
        self.ms = os.path.basename(self.result.inputs['vis'])
        self.template = template
        self.filename_prefix=filename_prefix

        if os.path.exists(json_path):
            with open(json_path, 'r') as json_file:
                self.json = json_file.readlines()[0]
        else:
            self.json = '{}'
            
    def _get_display_context(self):
        return {'pcontext'   : self.context,
                'result'     : self.result,
                'plots'      : self.plots,
                'dirname'    : self.dirname,
                'json'       : self.json}

    @property
    def dirname(self):
        stage = 'stage%s' % self.result.stage_number
        return os.path.join(self.context.report_dir, stage)
    
    @property
    def filename(self):        
        filename = filenamer.sanitize(self.filename_prefix + '-%s.html' % self.ms)
        return filename
    
    @property
    def path(self):
        return os.path.join(self.dirname, self.filename)
    
    def get_file(self, hardcopy=True):
        if hardcopy and not os.path.exists(self.dirname):
            os.makedirs(self.dirname)
            
        file_obj = open(self.path, 'w') if hardcopy else StdOutFile()
        return contextlib.closing(file_obj)
    
    def render(self):
        display_context = self._get_display_context()
        t = TemplateFinder.get_template(self.template)
        return t.render(**display_context)


#----------------------------------------------------------------------



class T2_4MDetailsCleanRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hif_cleanlist.html',
                 always_rerender=False):
        super(T2_4MDetailsCleanRenderer, self).__init__(template,
                                                        always_rerender)

    def get_display_context(self, context, results):
        super_cls = super(T2_4MDetailsCleanRenderer, self)
        ctx = super_cls.get_display_context(context, results)

        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % results.stage_number)

        # There is only ever one CleanListResult in the ResultsList as it
        # operates over multiple measurement sets, so we can set the result to
        # the first item in the list

        # Get results info
        info_dict = {}

        if results[0]:
            cresults = results[0].results

            for r in cresults:
                if r.empty():
                    continue
                if not r.iterations:
                    continue

                maxiter = max(r.iterations.keys())

                with casatools.ImageReader(r.iterations[maxiter]['image']) as image:
                   info = image.miscinfo()
                   spw = pol = field = None
                   if info.has_key('spw'): 
                       spw = info['spw']
                   if info.has_key('field'):
                       field = '%s (%s)' % (info['field'], r.intent)

                   coordsys = image.coordsys()
                   coord_names = np.array(coordsys.names())
                   coord_refs = coordsys.referencevalue(format='s')
                   pol = coord_refs['string'][coord_names=='Stokes'][0]
                   frequency = coord_refs['string'][coord_names=='Frequency'][0]
                   frequency = casatools.quanta.convert(frequency, 'GHz')
                   info_dict[(field,spw,pol,'frequency')] = frequency

                   info_dict[(field,spw,pol,'image name')] = image.name(strippath=True)
                   stats = image.statistics(robust=False)
                   info_dict[(field,spw,pol,'max')] = stats.get('max')[0]
                   beam = image.restoringbeam()
                   major = casatools.quanta.convert(beam['major'], 'arcsec')
                   info_dict[(field,spw,pol,'beam major')] = major
                   minor = casatools.quanta.convert(beam['minor'], 'arcsec')
                   info_dict[(field,spw,pol,'beam minor')] = minor
                   pa = casatools.quanta.convert(beam['positionangle'], 'deg')
                   info_dict[(field,spw,pol,'beam pa')] = pa
                   info_dict[(field,spw,pol,'brightness unit')] = image.brightnessunit()

                   stats = image.statistics(robust=False)
                   info_dict[(field,spw,pol,'image rms')] = stats.get('rms')[0]

                with casatools.ImageReader(r.iterations[maxiter]['residual']) as residual:
                   stats = image.statistics(robust=False)
                   info_dict[(field,spw,pol,'residual rms')] = stats.get('rms')[0]

        # Make the plots
        plotter = clean.CleanSummary(context, results[0])
        plots = plotter.plot()        

        fields = sorted(set([p.parameters['field'] for p in plots]))
        spws = sorted(set([p.parameters['spw'] for p in plots]))
        iterations = sorted(set([p.parameters['iter'] for p in plots]))
        types = sorted(set([p.parameters['type'] for p in plots]))
        
        iteration_dim = lambda : collections.defaultdict(dict)
        spw_dim = lambda : collections.defaultdict(iteration_dim)
        plots_dict = collections.defaultdict(spw_dim)
        for field in fields:
            for spw in spws:
                for iteration in iterations:
                    for t in types:
                        matching = [p for p in plots
                                    if p.parameters['field'] == field
                                    and p.parameters['spw'] == spw
                                    and p.parameters['iter'] == iteration
                                    and p.parameters['type'] == t]
                        if matching:
                            plots_dict[field][spw][iteration][t] = matching[0]
                    
        ctx.update({'fields'     : fields,
                    'spws'       : spws,
                    'iterations' : iterations,
                    'plots'      : plots,
                    'plots_dict' : plots_dict,
                    'info_dict'  : info_dict,
                    'dirname'    : weblog_dir})

#         import pprint
#         d = {}
#         d.update(plots_dict)
#         pprint.pprint(d)

        return ctx




class T2_4MDetailsVLAAgentFlaggerRenderer(T2_4MDetailsDefaultRenderer):
    FlagTotal = collections.namedtuple('FlagSummary', 'flagged total')

    def __init__(self, template='t2-4m_details-hifv_flagdata.html', 
                 always_rerender=False):
        super(T2_4MDetailsVLAAgentFlaggerRenderer, self).__init__(template,
                                                               always_rerender)

    def get_display_context(self, context, result):
        super_cls = super(T2_4MDetailsVLAAgentFlaggerRenderer, self)
        ctx = super_cls.get_display_context(context, result)

        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % result.stage_number)

        flag_totals = {}
        for r in result:
            flag_totals = utils.dict_merge(flag_totals, 
                                           self.flags_for_result(r, context))

            # copy template files across to weblog directory
            toggle_to_filenames = {'online'   : 'fileonline',
                                   'template' : 'filetemplate'}
            inputs = r.inputs
            for toggle, filenames in toggle_to_filenames.items():
                src = inputs[filenames]
                if inputs[toggle] and os.path.exists(src):
                    LOG.trace('Copying %s to %s' % (src, weblog_dir))
                    shutil.copy(src, weblog_dir)

        flagcmd_files = {}
        for r in result:
            # write final flagcmds to a file
            ms = context.observing_run.get_ms(r.inputs['vis'])
            flagcmds_filename = '%s-agent_flagcmds.txt' % ms.basename
            flagcmds_path = os.path.join(weblog_dir, flagcmds_filename)
            with open(flagcmds_path, 'w') as flagcmds_file:
                terminated = '\n'.join(r.flagcmds())
                flagcmds_file.write(terminated)

            flagcmd_files[ms.basename] = flagcmds_path

#         # collect the agent names
#         agent_names = set()
#         for r in result:
#             agent_names.update([s['name'] for s in r.summaries])
# 
#         # get agent names in execution order
#         order = ['before', 'online', 'template', 'autocorr', 'shadow', 
#                  'intents', 'edgespw']
#         agents = [s for s in order if s in agent_names]

        # return all agents so we get ticks and crosses against each one
        agents = ['before', 'online', 'template', 'autocorr', 'shadow', 
                  'intents', 'edgespw', 'clip', 'quack', 'baseband']

        flagplots = [self.flagplot(r, context) for r in result]
        # plot object may be None if plot failed to generate
        flagplots = [f for f in flagplots if f is not None]

        ctx.update({'flags'     : flag_totals,
                    'agents'    : agents,
                    'dirname'   : weblog_dir,
                    'flagcmds'  : flagcmd_files,
                    'flagplots' : flagplots})

        return ctx

    def flagplot(self, result, context):
        plotter = flagging.PlotAntsChart(context, result)
        return plotter.plot()

    def flags_for_result(self, result, context):
        ms = context.observing_run.get_ms(result.inputs['vis'])
        summaries = result.summaries

        by_intent = self.flags_by_intent(ms, summaries)
        by_spw = self.flags_by_science_spws(ms, summaries)

        return {ms.basename : utils.dict_merge(by_intent, by_spw)}

    def flags_by_intent(self, ms, summaries):
        # create a dictionary of scans per observing intent, eg. 'PHASE':[1,2,7]
        intent_scans = {}
        for intent in ('BANDPASS', 'PHASE', 'AMPLITUDE', 'TARGET'):
            # convert IDs to strings as they're used as summary dictionary keys
            intent_scans[intent] = [str(s.id) for s in ms.scans
                                    if intent in s.intents]

        # while we're looping, get the total flagged by looking in all scans 
        intent_scans['TOTAL'] = [str(s.id) for s in ms.scans]

        total = collections.defaultdict(dict)

        previous_summary = None
        for summary in summaries:

            for intent, scan_ids in intent_scans.items():
                flagcount = 0
                totalcount = 0
    
                for i in scan_ids:
                    flagcount += int(summary['scan'][i]['flagged'])
                    totalcount += int(summary['scan'][i]['total'])
        
                    if previous_summary:
                        flagcount -= int(previous_summary['scan'][i]['flagged'])
    
                ft = T2_4MDetailsVLAAgentFlaggerRenderer.FlagTotal(flagcount, 
                                                                totalcount)
                total[summary['name']][intent] = ft
                
            previous_summary = summary
                
        return total 
    
    def flags_by_science_spws(self, ms, summaries):
        science_spws = ms.get_spectral_windows(science_windows_only=True)
    
        total = collections.defaultdict(dict)
    
        previous_summary = None
        for summary in summaries:
    
            flagcount = 0
            totalcount = 0
    
            for spw in science_spws:
                spw_id = str(spw.id)
                flagcount += int(summary['spw'][spw_id]['flagged'])
                totalcount += int(summary['spw'][spw_id]['total'])
        
                if previous_summary:
                    flagcount -= int(previous_summary['spw'][spw_id]['flagged'])

            ft = T2_4MDetailsVLAAgentFlaggerRenderer.FlagTotal(flagcount, 
                                                            totalcount)
            total[summary['name']]['SCIENCE SPWS'] = ft
                
            previous_summary = summary
                
        return total










class T2_4MDetailsAgentFlaggerRenderer(T2_4MDetailsDefaultRenderer):
    FlagTotal = collections.namedtuple('FlagSummary', 'flagged total')

    def __init__(self, template='t2-4m_details-hif_flagdeteralma.html', 
                 always_rerender=False):
        super(T2_4MDetailsAgentFlaggerRenderer, self).__init__(template,
                                                               always_rerender)

    def get_display_context(self, context, result):
        super_cls = super(T2_4MDetailsAgentFlaggerRenderer, self)
        ctx = super_cls.get_display_context(context, result)

        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % result.stage_number)

        flag_totals = {}
        for r in result:
            flag_totals = utils.dict_merge(flag_totals, 
                                           self.flags_for_result(r, context))

            # copy template files across to weblog directory
            toggle_to_filenames = {'online'   : 'fileonline',
                                   'template' : 'filetemplate'}
            inputs = r.inputs
            for toggle, filenames in toggle_to_filenames.items():
                src = inputs[filenames]
                if inputs[toggle] and os.path.exists(src):
                    LOG.trace('Copying %s to %s' % (src, weblog_dir))
                    shutil.copy(src, weblog_dir)

        flagcmd_files = {}
        for r in result:
            # write final flagcmds to a file
            ms = context.observing_run.get_ms(r.inputs['vis'])
            flagcmds_filename = '%s-agent_flagcmds.txt' % ms.basename
            flagcmds_path = os.path.join(weblog_dir, flagcmds_filename)
            with open(flagcmds_path, 'w') as flagcmds_file:
                terminated = '\n'.join(r.flagcmds())
                flagcmds_file.write(terminated)

            flagcmd_files[ms.basename] = flagcmds_path

#         # collect the agent names
#         agent_names = set()
#         for r in result:
#             agent_names.update([s['name'] for s in r.summaries])
# 
#         # get agent names in execution order
#         order = ['before', 'online', 'template', 'autocorr', 'shadow', 
#                  'intents', 'edgespw']
#         agents = [s for s in order if s in agent_names]

        # return all agents so we get ticks and crosses against each one
        agents = ['before', 'online', 'template', 'autocorr', 'shadow', 
                  'intents', 'edgespw']

        flagplots = [self.flagplot(r, context) for r in result]
        # plot object may be None if plot failed to generate
        flagplots = [f for f in flagplots if f is not None]

        ctx.update({'flags'     : flag_totals,
                    'agents'    : agents,
                    'dirname'   : weblog_dir,
                    'flagcmds'  : flagcmd_files,
                    'flagplots' : flagplots})

        return ctx

    def flagplot(self, result, context):
        plotter = flagging.PlotAntsChart(context, result)
        return plotter.plot()

    def flags_for_result(self, result, context):
        ms = context.observing_run.get_ms(result.inputs['vis'])
        summaries = result.summaries

        by_intent = self.flags_by_intent(ms, summaries)
        by_spw = self.flags_by_science_spws(ms, summaries)

        return {ms.basename : utils.dict_merge(by_intent, by_spw)}

    def flags_by_intent(self, ms, summaries):
        # create a dictionary of scans per observing intent, eg. 'PHASE':[1,2,7]
        intent_scans = {}
        for intent in ('BANDPASS', 'PHASE', 'AMPLITUDE', 'TARGET'):
            # convert IDs to strings as they're used as summary dictionary keys
            intent_scans[intent] = [str(s.id) for s in ms.scans
                                    if intent in s.intents]

        # while we're looping, get the total flagged by looking in all scans 
        intent_scans['TOTAL'] = [str(s.id) for s in ms.scans]

        total = collections.defaultdict(dict)

        previous_summary = None
        for summary in summaries:

            for intent, scan_ids in intent_scans.items():
                flagcount = 0
                totalcount = 0
    
                for i in scan_ids:
                    flagcount += int(summary['scan'][i]['flagged'])
                    totalcount += int(summary['scan'][i]['total'])
        
                    if previous_summary:
                        flagcount -= int(previous_summary['scan'][i]['flagged'])
    
                ft = T2_4MDetailsAgentFlaggerRenderer.FlagTotal(flagcount, 
                                                                totalcount)
                total[summary['name']][intent] = ft
                
            previous_summary = summary
                
        return total 
    
    def flags_by_science_spws(self, ms, summaries):
        science_spws = ms.get_spectral_windows(science_windows_only=True)
    
        total = collections.defaultdict(dict)
    
        previous_summary = None
        for summary in summaries:
    
            flagcount = 0
            totalcount = 0
    
            for spw in science_spws:
                spw_id = str(spw.id)
                flagcount += int(summary['spw'][spw_id]['flagged'])
                totalcount += int(summary['spw'][spw_id]['total'])
        
                if previous_summary:
                    flagcount -= int(previous_summary['spw'][spw_id]['flagged'])

            ft = T2_4MDetailsAgentFlaggerRenderer.FlagTotal(flagcount, 
                                                            totalcount)
            total[summary['name']]['SCIENCE SPWS'] = ft
                
            previous_summary = summary
                
        return total
      
class T2_4MDetailsSingleDishInspectDataRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hsd_inspectdata.html',
                 always_rerender=False):
        super(T2_4MDetailsSingleDishInspectDataRenderer, self).__init__(template, 
                                                                       always_rerender)
        
    def get_display_context(self, context, results):
        ctx = super(T2_4MDetailsSingleDishInspectDataRenderer, self).get_display_context(context, results)
        
        stage_dir = os.path.join(context.report_dir,'stage%d'%(results[0].stage_number))
        if not os.path.exists(stage_dir):
            os.mkdir(stage_dir)
   
        plot_types = [{'type': 'pointing',
                       'renderer': sddisplay.SDPointingDisplay,
                       'plot_title': 'Telescope Pointing on the Sky'},
                      {'type': 'azel',
                       'renderer': sddisplay.SDAzElDisplay,
                       'plot_title': 'Azimuth/Elevation vs Time'},
                      {'type': 'weather',
                       'renderer': sddisplay.SDWeatherDisplay,
                       'plot_title': 'Weather vs Time'},
                      {'type': 'wvr',
                       'renderer': sddisplay.SDWvrDisplay,
                       'plot_title': 'WVR Reading vs Time'}]
        
        summary_plots = {} 
        subpages = {}
        for _types in plot_types:
            renderer_cls = _types['renderer']
            inputs = renderer_cls.Inputs(context, results[0])
            task = renderer_cls(inputs)
            plots = task.plot()
            plot_group = self._group_by_vis(plots)
            for vis in plot_group.keys():
                if not summary_plots.has_key(vis):
                    summary_plots[vis] = dict([(p['type'], None) for p in plot_types])
                summary_plots[vis][_types['type']] = self._get_summary_plot(_types['type'], 
                                                                            plot_group, 
                                                                            vis)#plot_group[vis][0]
            
            plot_list = {}
            for (name, _plots) in plot_group.items():
                if _types['type'] == 'pointing':
                    renderer = SingleDishPointingPlotsRenderer(context, results, name, _plots,
                                                               _types['plot_title'])
                else:
                    renderer = SingleDishInspectDataPlotsRenderer(context, results, name, _plots,
                                                                  _types['plot_title'])
                with renderer.get_file() as fileobj:
                    fileobj.write(renderer.render())
                plot_list[name] = renderer.filename
                if not subpages.has_key(name):
                    subpages[name] = {}
                subpages[name][_types['type']] = renderer.filename
   
        ctx.update({'subplot': subpages,
                    'summary': summary_plots,
                    'dirname': stage_dir})
        
        return ctx
    
    def _group_by_vis(self, plots):
        plot_group = {}
        for p in [p for _p in plots for p in _p]:
            key = p.parameters['vis']
            if plot_group.has_key(key):
                plot_group[key].append(p)
            else:
                plot_group[key] = [p]
        return plot_group
    
    def _get_summary_plot(self, plot_type, plot_group, vis):
        if plot_type == 'pointing':
            for plot in plot_group[vis]:
                if plot.parameters['type'] == 'on source pointing':
                    return plot
        return plot_group[vis][0]        
    
        
class T2_4MDetailsSingleDishCalSkyRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hsd_calsky.html', 
                 always_rerender=False):
        super(T2_4MDetailsSingleDishCalSkyRenderer, self).__init__(template,
                                                                   always_rerender)
    def get_display_context(self, context, results):
        ctx = super(T2_4MDetailsSingleDishCalSkyRenderer, self).get_display_context(context, results)
        
        stage_dir = os.path.join(context.report_dir,'stage%d'%(results.stage_number))
        if not os.path.exists(stage_dir):
            os.mkdir(stage_dir)
              
        inputs = sddisplay.SDSkyDisplay.Inputs(context,results)
        task = sddisplay.SDSkyDisplay(inputs)
        # plots is list-of-list of plot instances
        plots = task.plot()
        
        # plot_group is a dictionary of (MS names, associating plots) 
        plot_group = self._group_by_vis(plots)
        
        # summary_plots is a dictionary of (MS names, list of typical plots for each spw) 
        # at the moment typical plot is a first plot of each spectral window
        summary_plots = self._summary_plots(plot_group)
        
        plot_list = {}
        for (name, _plots) in plot_group.items():
            #renderer = SingleDishCalSkyPlotsRenderer(context, results, name, _plots)
            renderer = SingleDishGenericPlotsRenderer(context, results, name, _plots,
                                                      'Sky Level vs Frequency')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
            plot_list[name] = renderer.filename
                
        # default dirname is relative path so replacing it with absolute path 
        ctx.update({'summary_subpage': plot_list,
                    'summary_plots': summary_plots,
                    'dirname': stage_dir})
            
        return ctx
    
    def _group_by_vis(self, plots):
        plot_group = {}
        for p in [p for _p in plots for p in _p]:
            key = p.parameters['vis']
            if plot_group.has_key(key):
                plot_group[key].append(p)
            else:
                plot_group[key] = [p]
        return plot_group
                
    def _summary_plots(self, plot_group):
        summary_plots = {}
        for (vis, plots) in plot_group.items():
            spw_list = set()
            summary_plots[vis]= []
            for plot in plots:
                spw = plot.parameters['spw']
                if spw not in spw_list:
                    spw_list.add(spw)
                    summary_plots[vis].append(plot)
        return summary_plots


class T2_4MDetailsSingleDishCalTsysRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hsd_caltsys.html', 
                 always_rerender=False):
        super(T2_4MDetailsSingleDishCalTsysRenderer, self).__init__(template,
                                                                    always_rerender)
        
    def get_display_context(self, context, results):
        ctx = super(T2_4MDetailsSingleDishCalTsysRenderer, self).get_display_context(context, results)
        
        stage_dir = os.path.join(context.report_dir,'stage%d'%(results.stage_number))
        if not os.path.exists(stage_dir):
            os.mkdir(stage_dir)

        inputs = sddisplay.SDTsysDisplay.Inputs(context,results)
        task = sddisplay.SDTsysDisplay(inputs)
        plots = task.plot()
        plots_per_type = self._plots_per_type(plots, None)
        summary_plots = {}
        plot_list = {}
        plot_group = self._group_by_vis(plots_per_type)
        for (name, _plots) in plot_group.items():
            summary_plots[name] = _plots['summary']
            individual_plots = _plots['individual']
            renderer = SingleDishGenericPlotsRenderer(context, results, name, individual_plots,
                                                      'Tsys vs Frequency')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
            plot_list[name] = renderer.filename

        ctx.update({'subpage': plot_list,
                    'summary': summary_plots,
                    'dirname': stage_dir})

        return ctx

    def _group_by_vis(self, plots):
        plot_group = {}
        for p in plots[0]:
            key = p.parameters['vis']
            if plot_group.has_key(key):
                plot_group[key]['summary'].append(p)
            else:
                plot_group[key] = {'summary': [p],
                                   'individual': []}
        for p in plots[1]:
            plot_group[p.parameters['vis']]['individual'].append(p)
        return plot_group

        
        
    def _plots_per_type(self, plots, xaxis_list):
        plot_group = [[], []] #[summary plots, individual plots]
        for _plots in plots:
            for p in _plots:
                spw = p.parameters['spw']
                if spw == 'all':
                    plot_group[0].append(p)
                else:
                    plot_group[1].append(p)
        return plot_group

class T2_4MDetailsSingleDishImagingRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hsd_imaging.html', 
                 always_rerender=False):
        super(T2_4MDetailsSingleDishImagingRenderer, self).__init__(template,
                                                                    always_rerender)
        
    def get_display_context(self, context, results):
        ctx = super(T2_4MDetailsSingleDishImagingRenderer, self).get_display_context(context, results)
        
        stage_dir = os.path.join(context.report_dir,'stage%d'%(results.stage_number))
        if not os.path.exists(stage_dir):
            os.mkdir(stage_dir)
            
        plots = []
        #for image_item in result.outcome:
        for r in results:
            image_item = r.outcome['image']
            antenna_list = r.outcome['file_index']
            spwid = image_item.spwlist
            spw_type = context.observing_run[antenna_list[0]].spectral_window[spwid[0]].type
            task_cls = sddisplay.SDImageDisplayFactory(spw_type)
            inputs = task_cls.Inputs(context,result=r)
            task = task_cls(inputs)
            plots.append(task.plot())
            
        map_types = {'sparsemap': {'type': 'sd_sparse_map',
                                   'plot_title': 'Sparse Profile Map'},
                     'profilemap': {'type': 'sd_spectral_map',
                                    'plot_title': 'Detailed Profile Map'},
                     'channelmap': {'type': 'channel_map',
                                    'plot_title': 'Channel Map'},
                     'rmsmap': {'type': 'rms_map',
                                'plot_title': 'Baseline RMS Map'},
                     'integratedmap': {'type': 'sd_integrated_map',
                                       'plot_title': 'Integrated Intensity Map'}}
        for (key, value) in map_types.items():
            plot_list = self._plots_per_field_with_type(plots, value['type'])
            summary = self._summary_plots(plot_list)
            subpage = {}
            for (name, _plots) in plot_list.items():
                #renderer = value['renderer'](context, results, name, _plots)
                renderer = SingleDishGenericPlotsRenderer(context, results, name, _plots, 
                                                          value['plot_title'])
                with renderer.get_file() as fileobj:
                    fileobj.write(renderer.render())
                subpage[name] = renderer.filename
            ctx.update({'%s_subpage'%(key): subpage,
                        '%s_plots'%(key): summary})
            if key == 'sparsemap':
                profilemap_entries = {}
                for (field, _plots) in plot_list.items():
                    _ap = {}
                    for p in _plots:
                        ant = p.parameters['ant']
                        pol = p.parameters['pol']
                        if not _ap.has_key(ant):
                            _ap[ant] = [pol]
                        elif pol not in _ap[ant]:
                            _ap[ant].append(pol)
                    profilemap_entries[field] = _ap
                ctx.update({'profilemap_entries': profilemap_entries})

        ctx.update({'dirname': stage_dir})

        return ctx
    
    def _plots_per_field_with_type(self, plots, type_string):
        plot_group = {}
        for p in [p for _p in plots for p in _p]:
            if p.parameters['type'] == type_string:
                key = p.field
                if plot_group.has_key(key):
                    plot_group[key].append(p)
                else:
                    plot_group[key] = [p]
        return plot_group
        
    def _summary_plots(self, plot_group):
        summary_plots = {}
        for (field_name, plots) in plot_group.items():
            spw_list = []
            summary_plots[field_name]= []
            for plot in plots:
                spw = plot.parameters['spw']
                if spw not in spw_list:
                    spw_list.append(spw)
                    summary_plots[field_name].append(plot)
                if plot.parameters['ant'] == 'COMBINED':
                    idx = spw_list.index(spw)
                    summary_plots[field_name][idx] = plot
        return summary_plots

class T2_4MDetailsSingleDishPlotFlagBaselineRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hsd_plotflagbaseline.html',
                 always_rerender=False):
        super(T2_4MDetailsSingleDishPlotFlagBaselineRenderer, self).__init__(template,
                                                                             always_rerender)
    def get_display_context(self, context, results):
        ctx = super(T2_4MDetailsSingleDishPlotFlagBaselineRenderer, self).get_display_context(context, 
                                                                                              results)
        stage_number = get_stage_number(results)
        stage_dir = os.path.join(context.report_dir,'stage%d'%(stage_number))
        if not os.path.exists(stage_dir):
            os.mkdir(stage_dir)

        plots = []
        for r in results:
            inputs = sddisplay.SDBaselineAllDisplay.Inputs(context,result=r)
            task = sddisplay.SDBaselineAllDisplay(inputs)
            plots.extend(task.plot())
            
        plot_group = self._group_by_vis(plots)
        summary_plots = self._summary_plots(plot_group)

        plot_list = {}
        for (name, _plots) in plot_group.items():
            renderer = SingleDishPlotFlagBaselinePlotsRenderer(context, results, name, _plots,
                                                               'Intensity vs Frequency')
            with renderer.get_file() as fileobj:
                fileobj.write(renderer.render())
            plot_list[name] = renderer.filename
                
        # default dirname is relative path so replacing it with absolute path 
        ctx.update({'summary_subpage': plot_list,
                    'summary_plots': summary_plots,
                    'dirname': stage_dir})
    
        return ctx

    def _group_by_vis(self, plots):
        plot_group = {}
        for p in [p for _p in plots for p in _p]:
            key = p.parameters['vis']
            if plot_group.has_key(key):
                plot_group[key].append(p)
            else:
                plot_group[key] = [p]
        return plot_group
    
    def _summary_plots(self, plot_group):
        summary_plots = {}
        for (vis, plots) in plot_group.items():
            spw_list = set()
            summary_plots[vis]= []
            for plot in plots:
                spw = plot.parameters['spw']
                if spw not in spw_list:
                    spw_list.add(spw)
                    summary_plots[vis].append(plot)
        return summary_plots

class T2_4MDetailsSingleDishImportDataRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hsd_importdata.html', 
                 always_rerender=False):
        super(T2_4MDetailsSingleDishImportDataRenderer, self).__init__(template,
                                                             always_rerender)
        
    def get_display_context(self, context, result):
        super_cls = super(T2_4MDetailsSingleDishImportDataRenderer, self)        
        ctx = super_cls.get_display_context(context, result)

        num_mses = reduce(operator.add, [len(r.mses) for r in result])

        ctx.update({'num_mses'      : num_mses})

        return ctx

class T2_4MDetailsSingleDishApplycalRenderer(T2_4MDetailsDefaultRenderer):
    #FlagTotal = collections.namedtuple('FlagSummary', 'flagged total')

    def __init__(self, template='t2-4m_details-hsd_applycal.html', 
                 always_rerender=False):
        super(T2_4MDetailsSingleDishApplycalRenderer, self).__init__(template,
                                                                     always_rerender)

    def get_display_context(self, context, result):
        super_cls = super(T2_4MDetailsSingleDishApplycalRenderer, self)
        ctx = super_cls.get_display_context(context, result)

        weblog_dir = os.path.join(context.report_dir,
                                  'stage%s' % result.stage_number)

        calapps = {}
        for r in result:
            calapps = utils.dict_merge(calapps,
                                       self.calapps_for_result(r))

        caltypes = {}
        for r in result:
            caltypes = utils.dict_merge(caltypes,
                                        self.caltypes_for_result(r))

        # return all agents so we get ticks and crosses against each one
        agents = ['before', 'applycal']

        ctx.update({'calapps'  : calapps,
                    'caltypes' : caltypes,
                    'agents'   : agents,
                    'dirname'  : weblog_dir})
        return ctx

    def calapps_for_result(self, result):
        calapps = collections.defaultdict(list)
        #for calapp in result.applied:
        #for calapp in result.outcome:
        calapplist = result.outcome
        for calapp in calapplist:
            infile = os.path.basename(calapp.infile)
            calapps[infile].append(calapp)
        return calapps

    def caltypes_for_result(self, result):
        type_map = {
            'ps' : 'Sky',
            'otf' : 'Sky',
            'otfraster'  : 'Sky',
            'tsys'     : 'T<sub>sys</sub>',
        }
        
        d = {}
        #for calapp in result.applied:
        calapplist = result.outcome
        for calapp in calapplist:
            for calfrom in calapp.calfrom:
                caltype = type_map.get(calfrom.caltype, calfrom.caltype)
                
                caltype += self.get_gain_solution_type(calfrom)
                
                d[calfrom.gaintable] = caltype

        return d
                
    def get_gain_solution_type(self, calfrom):
        if calfrom.caltype != 'tsys':
            return ' (%s)'%(calfrom.caltype)
        return ''


class SingleDishGenericPlotsRenderer(object):
    template = 'sd_generic_plots.html'

    def __init__(self, context, result, name, plots, plot_title):
        self.context = context
        self.result = result
        self.plots = plots
        self.name = name
        self.plot_title = str(plot_title)

        # all values set on this dictionary will be written to the JSON file
        d = {}
        for plot in plots:
            # calculate the relative pathnames as seen from the browser
            thumbnail_relpath = os.path.relpath(plot.thumbnail,
                                                self.context.report_dir)
            image_relpath = os.path.relpath(plot.abspath,
                                            self.context.report_dir)
            spw_id = plot.parameters['spw']
            ant_id = plot.parameters['ant']
            pol_id = plot.parameters['pol']

            # Javascript JSON parser doesn't like Javascript floating point 
            # constants (NaN, Infinity etc.), so convert them to null. We  
            # do not omit the dictionary entry so that the plot is hidden
            # by the filters.
#             if math.isnan(ratio) or math.isinf(ratio):
#                 ratio = 'null'

            d[image_relpath] = {'spw'       : str(spw_id),
                                'ant'       : ant_id,
                                'pol'       : pol_id,
                                'thumbnail' : thumbnail_relpath}

        self.json = json.dumps(d)
         
    def _get_display_context(self):
        return {'pcontext'   : self.context,
                'result'     : self.result,
                'plots'      : self.plots,
                'dirname'    : self.dirname,
                'json'       : self.json,
                'plot_title' : '%s for %s' % (self.plot_title, self.name)}

    @property
    def dirname(self):
        stage = 'stage%s' % self.result.stage_number
        return os.path.join(self.context.report_dir, stage)
    
    @property
    def filename(self):        
        filename = filenamer.sanitize('%s-%s.html' % (self.plot_title.lower(), self.name))
        return filename
    
    @property
    def path(self):
        return os.path.join(self.dirname, self.filename)
    
    def get_file(self, hardcopy=True):
        if hardcopy and not os.path.exists(self.dirname):
            os.makedirs(self.dirname)
            
        file_obj = open(self.path, 'w') if hardcopy else StdOutFile()
        return contextlib.closing(file_obj)
    
    def render(self):
        display_context = self._get_display_context()
        t = TemplateFinder.get_template(self.template)
        return t.render(**display_context)
    
class SingleDishInspectDataPlotsRenderer(SingleDishGenericPlotsRenderer):
    template = 'sd_inspectdata_plots.html'   

class SingleDishPointingPlotsRenderer(SingleDishGenericPlotsRenderer):
    template = 'sd_pointing_plots.html'
    
    def __init__(self, context, result, name, plots, plot_title):
        self.context = context
        self.result = result
        self.plots = plots
        self.name = name
        self.plot_title = str(plot_title)

        # all values set on this dictionary will be written to the JSON file
        d = {}
        for plot in plots:
            # calculate the relative pathnames as seen from the browser
            thumbnail_relpath = os.path.relpath(plot.thumbnail,
                                                self.context.report_dir)
            image_relpath = os.path.relpath(plot.abspath,
                                            self.context.report_dir)
            spw_id = plot.parameters['spw']
            ant_id = plot.parameters['ant']
            pol_id = plot.parameters['pol']
            type_str = plot.parameters['type']

            # Javascript JSON parser doesn't like Javascript floating point 
            # constants (NaN, Infinity etc.), so convert them to null. We  
            # do not omit the dictionary entry so that the plot is hidden
            # by the filters.
#             if math.isnan(ratio) or math.isinf(ratio):
#                 ratio = 'null'

            d[image_relpath] = {'spw'       : str(spw_id),
                                'ant'       : ant_id,
                                'pol'       : pol_id,
                                'type'      : type_str,
                                'thumbnail' : thumbnail_relpath}

        self.json = json.dumps(d)

class SingleDishPlotFlagBaselinePlotsRenderer(SingleDishGenericPlotsRenderer):
    template = 'generic_x_vs_y_detail_plots.html'    

class T2_4MDetailsSingleDishBaselineRenderer(T2_4MDetailsDefaultRenderer):
    # Renderer class for stage summary
    def __init__(self, template='t2-4m_details-hsd_baseline.html',
                 always_rerender=False):
        super(T2_4MDetailsSingleDishBaselineRenderer, self).__init__(template,
                                                                   always_rerender)
        
    def get_display_context(self, context, results):
        ctx = super(T2_4MDetailsSingleDishBaselineRenderer, self).get_display_context(context, results)
        
        stage_dir = os.path.join(context.report_dir,'stage%d'%(results.stage_number))
        if not os.path.exists(stage_dir):
            os.mkdir(stage_dir)
        plots = []
        for r in results:
            inputs = sddisplay.ClusterDisplay.Inputs(context,result=r)
            task = sddisplay.ClusterDisplay(inputs)
            plots.append(task.plot())

        plot_group = self._group_by_axes(plots)
        plot_detail = [] # keys are 'title', 'html', 'cover_plots'
        plot_cover = [] # keys are 'title', 'cover_plots'
        # Render stage details pages
        details_title = ["R.A. vs Dec."]
        for (name, _plots) in plot_group.items():
            group_desc = {'title': name}
            if name in details_title:
                renderer = SingleDishClusterPlotsRenderer(context, results, name, _plots)
                with renderer.get_file() as fileobj:
                    fileobj.write(renderer.render())
                group_desc['html'] = renderer.filename
                group_desc['cover_plots'] = self._get_a_plot_per_spw(_plots)
                plot_detail.append(group_desc)
            else:
                group_desc['cover_plots'] = _plots
                plot_cover.append(group_desc)
                
        ctx.update({'detail': plot_detail,
                    'cover_only': plot_cover})
        
        return ctx
    
    def _group_by_axes(self, plots):
        plot_group = {}
        for p in [p for _p in plots for p in _p]:
            key = "%s vs %s" % (p.x_axis, p.y_axis)
            if plot_group.has_key(key): plot_group[key].append(p)
            else: plot_group[key] = [p]
        return plot_group
    
    def _get_a_plot_per_spw(self, plots):
        known_spw = []
        plot_list = []
        for p in plots:
            if p.parameters['type'] == 'clustering_final' and p.parameters['spw'] not in known_spw:
                known_spw.append(p.parameters['spw'])
                plot_list.append(p)
        return plot_list
    
class SingleDishClusterPlotsRenderer(object):
    # take a look at WvrgcalflagPhaseOffsetVsBaselinePlotRenderer when we have
    # scores and histograms to generate. there should be a common base class. 
    template = 'sd_cluster_plots.html'
    
    def __init__(self, context, result, xytitle, plots):
        self.context = context
        self.result = result
        self.plots = plots
        self.xy_title = "Clustering: %s" % xytitle
        self.json = json.dumps(self._get_plot_info(plots))

    def _get_plot_info(self, plots): 
        d = {}
        for plot in plots:
            # calculate the relative pathnames as seen from the browser
            thumbnail_relpath = os.path.relpath(plot.thumbnail,
                                                self.context.report_dir)
            image_relpath = os.path.relpath(plot.abspath,
                                            self.context.report_dir)
            d[image_relpath] = {'thumbnail': thumbnail_relpath}
            for key, val in plot.parameters.items():
                d[image_relpath][key] = val
        return d

    def _get_display_context(self):
        return {'pcontext'   : self.context,
                'result'     : self.result,
                'plots'      : self.plots,
                'dirname'    : self.dirname,
                'json'       : self.json,
                'plot_title' : self.xy_title}

    @property
    def dirname(self):
        stage = 'stage%s' % self.result.stage_number
        return os.path.join(self.context.report_dir, stage)
    
    @property
    def filename(self):        
        filename = filenamer.sanitize('%s.html' % (self.xy_title.lower().replace(" ", "_")))
        return filename
    
    @property
    def path(self):
        return os.path.join(self.dirname, self.filename)
    
    def get_file(self, hardcopy=True):
        if hardcopy and not os.path.exists(self.dirname):
            os.makedirs(self.dirname)
            
        file_obj = open(self.path, 'w') if hardcopy else StdOutFile()
        return contextlib.closing(file_obj)
    
    def render(self):
        display_context = self._get_display_context()
        t = TemplateFinder.get_template(self.template)
        return t.render(**display_context)

class T2_4MDetailsSingleDishFlagBaselineRenderer(T2_4MDetailsDefaultRenderer):
    def __init__(self, template='t2-4m_details-hsd_flagbaseline.html',
                 always_rerender=False):
        super(T2_4MDetailsSingleDishFlagBaselineRenderer, self).__init__(template,
                                                                         always_rerender)
        self.baseline_renderer = T2_4MDetailsSingleDishBaselineRenderer(always_rerender=False)        
        
    def get_display_context(self, context, results):
        ctx = super(T2_4MDetailsSingleDishFlagBaselineRenderer, self).get_display_context(context, 
                                                                                          results)
        
        stage_number = get_stage_number(results)
        stage_dir = os.path.join(context.report_dir,'stage%d'%(stage_number))
        if not os.path.exists(stage_dir):
            os.mkdir(stage_dir)
   
        plot_groups_list = []
        flag_html_list = []
        for r in results:

            baseline_result = r.outcome['baseline']
            flagdata_result = r.outcome['flagdata']
           
            plots = []
            inputs = sddisplay.ClusterDisplay.Inputs(context,result=baseline_result)
            task = sddisplay.ClusterDisplay(inputs)
            plots.append(task.plot())
    
            plot_groups = logger.PlotGroup.create_plot_groups(plots)
            for plot_group in plot_groups:
                renderer = PlotGroupRenderer(context, results, plot_group)
                plot_group.filename = renderer.filename
                with renderer.get_file() as fileobj:
                    fileobj.write(renderer.render())
            plot_groups_list.append(plot_groups)
        
            rel_path = os.path.basename(stage_dir)   ### stage#

            html_names = []
            summaries = flagdata_result.outcome['summary']
            for summary in summaries:
                html_names.append(summary['html'])
                flag_html_list.append(html_names)
                
        baseline_context = []
        for r in results:
            baseline_results = basetask.ResultsList()
            baseline_results.append(r.outcome['baseline'])
            baseline_results.stage_number = results.stage_number
            baseline_context.append(self.baseline_renderer.get_display_context(context, baseline_results))
    
        ctx.update({'dirname': stage_dir,
                    'plot_groups_list': plot_groups_list,
                    'flag_html_list': flag_html_list,
                    'baseline_context': baseline_context})
    
        return ctx

class T2_4MDetailsRenderer(object):
    # the filename component of the output file. While this is the same for
    # all results, the directory is stage-specific, so there's no risk of
    # collisions  
    output_file = 't2-4m_details.html'
    
    # the default renderer should the task:renderer mapping not specify a
    # specialised renderer
    _default_renderer = T2_4MDetailsDefaultRenderer()

    """
    Get the file object for this renderer.

    The returned file object will be either direct output to a file or to
    stdout, depending on whether hardcopy is set to true or false
    respectively. 

    :param context: the pipeline Context
    :type context: :class:`~pipeline.infrastructure.launcher.Context`
    :param result: the task results object to render
    :type result: :class:`~pipeline.infrastructure.api.Result`
    :param hardcopy: render to disk (true) or to stdout (false). Default is
        true
    :type hardcopy: boolean
    :rtype: a file object
    """
    @classmethod
    def get_file(cls, context, result, hardcopy):
        # construct the relative filename, eg. 'stageX/t2-4m_details.html'
        path = cls.get_path(context, result)

        # to avoid any subsequent file not found errors, create the directory
        # if a hard copy is requested and the directory is missing
        stage_dir = os.path.dirname(path)
        if hardcopy and not os.path.exists(stage_dir):
            os.makedirs(stage_dir)
        
        # create a file object that writes to a file if a hard copy is 
        # requested, otherwise return a file object that flushes to stdout
        file_obj = open(path, 'w') if hardcopy else StdOutFile()
        
        # return the file object wrapped in a context manager, so we can use
        # it with the autoclosing 'with fileobj as f:' construct
        return contextlib.closing(file_obj)

    """
    Get the path to which the template will be written.
    
    :param context: the pipeline Context
    :type context: :class:`~pipeline.infrastructure.launcher.Context`
    :param result: the task results object to render
    :type result: :class:`~pipeline.infrastructure.api.Result`
    :rtype: string
    """
    @classmethod
    def get_path(cls, context, result):
        # HTML output will be written to the directory 'stageX' 
        stage = 'stage%s' % result.stage_number
        stage_dir = os.path.join(context.report_dir, stage)

        # construct the relative filename, eg. 'stageX/t2-4m_details.html'
        return os.path.join(stage_dir, cls.output_file)

    """
    Render the detailed task-centric view of each Results in the given
    context.
    
    This renderer creates detailed, T2_4M output for each Results. Each
    Results in the context is passed to a specialised renderer, which 
    generates customised output and plots for the Result in question.
    
    :param context: the pipeline Context
    :type context: :class:`~pipeline.infrastructure.launcher.Context`
    :param hardcopy: render to disk (true) or to stdout (false). Default is true.
    :type hardcopy: boolean
    """
    @classmethod
    def render(cls, context, hardcopy=False):
        # get the map of t2_4m renderers from the dictionary
        t2_4m_renderers = renderer_map[T2_4MDetailsRenderer]

        # for each result accepted and stored in the context..
        for result in context.results:
            # we only handle lists of results, so wrap single objects in a
            # list if necessary
            if not isinstance(result, collections.Iterable):
                l = basetask.ResultsList()
                l.append(result)
                l.timestamps = result.timestamps
                l.stage_number = result.stage_number
                l.inputs = result.inputs
                if hasattr(result, 'taskname'):
                    l.taskname = result.taskname
                result = l
            task = result[0].task
            
            # find the renderer appropriate to the task..
            renderer = t2_4m_renderers.get(task, cls._default_renderer)
            LOG.trace('Using %s to render %s result' % (
                renderer.__class__.__name__, task.__name__))

            # details pages do not need to be updated once written unless the
            # renderer specifies that an update is required
            path = cls.get_path(context, result)
            force_rerender = getattr(renderer, 'always_rerender', False)
            if os.path.exists(path) and not force_rerender:
                continue
            
            # .. get the file object to which we'll render the result
            with cls.get_file(context, result, hardcopy) as fileobj:
                # .. and write the renderer's interpretation of this result to
                # the file object  
                try:
                    fileobj.write(renderer.render(context, result))
                except:
                    LOG.warning('Template generation failed for '
                                '%s.' % cls.__name__)
                    LOG.debug(mako.exceptions.text_error_template().render())
                    fileobj.write(mako.exceptions.html_error_template().render())


class WebLogGenerator(object):
    renderers = [T1_1Renderer,         # OUS splash page
                 T1_2Renderer,         # observation summary
                 T1_3MRenderer,        # by topic page
                 T2_1Renderer,         # session tree
                 T2_1DetailsRenderer,  # session details
                 T2_2_1Renderer,       # spatial setup
                 T2_2_2Renderer,       # spectral setup
                 T2_2_3Renderer,       # antenna setup
                 T2_2_4Renderer,       # sky setup
                 T2_2_5Renderer,       # weather
                 T2_2_6Renderer,       # scans
                 T2_3_1MRenderer,      # data set topic
                 T2_3_2MRenderer,      # calibration topic
                 T2_3_3MRenderer,      # flagging topic
                 # disable unused line finding topic for July 2014 release
#                  T2_3_4MRenderer,      # line finding topic
                 T2_3_5MRenderer,      # imaging topic
                 T2_3_6MRenderer,      # miscellaneous topic
                 T2_3MDetailsRenderer, # QA details pages
                 T2_4MRenderer,        # task tree
                 T2_4MDetailsRenderer, # task details
                 # some summary renderers are placed last for access to scores
                 T1_4MRenderer]        # task summary

    @staticmethod
    def copy_resources(context):
        outdir = os.path.join(context.report_dir, 'resources')
        
        # shutil.copytree complains if the output directory exists
        if os.path.exists(outdir):
            shutil.rmtree(outdir)
        
        # copy all uncompressed non-python resources to output directory
        src = os.path.dirname(resources.__file__)
        dst = outdir
        ignore_fn = shutil.ignore_patterns('*.zip', '*.py', '*.pyc', 'CVS*',
                                           '.svn')
        shutil.copytree(src, 
                        dst, 
                        symlinks=False, 
                        ignore=ignore_fn)

        # unzip fancybox to output directory
        infile = os.path.join(src, 'fancybox.zip')
        z = zipfile.ZipFile(infile, 'r')        
        z.extractall(outdir)

    @staticmethod
    def render(context, hardcopy=True):
        # copy CSS, javascript etc. to weblog directory
        WebLogGenerator.copy_resources(context)

        # We could seriously optimise the rendering process by only unpickling
        # those objects that we need to render.  
        LOG.todo('Add results argument to renderer interfaces!')
        proxies = context.results
        
        try:
            # unpickle the results objects ready for rendering
            context.results = [proxy.read() for proxy in context.results]

            for renderer in WebLogGenerator.renderers:
                try:
                    LOG.trace('%s rendering...' % renderer.__name__)
                    renderer.render(context, hardcopy)
                except Exception as e:
                    LOG.exception('Error generating weblog: %s', e)

            # create symlink to t1-1.html 
            link_relsrc = T1_1Renderer.output_file
            link_abssrc = os.path.join(context.report_dir, link_relsrc)
            link_dst = os.path.join(context.report_dir, 'index.html')
            if os.path.exists(link_abssrc) and not os.path.exists(link_dst):
                os.symlink(link_relsrc, link_dst)
        finally:
            context.results = proxies

class LogCopier(object):
    """
    LogCopier copies and handles the CASA logs so that they may be referenced
    by the pipeline web logs. 
    
    Capturing the CASA log gives us a few problems:
    The previous log is renamed upon starting a new session. To be reliably
    referenced from the web log, we must give it an immutable name and copy it
    to a safe location within the web log directory.

    The user may want to view the web log at any time during a pipeline
    session. To avoid broken links to the CASA log, the log should be copied
    across to the web log location at the end of each task.

    Pipeline sessions may be interrupted and restored, resulting in multiple
    CASA logs for such sessions. These logs must be consolidated into one file
    alongside any previous log information.

    Adding HTML tags such as '<pre>' and HTML anchors causes the CASA log
    reader to render such entries as empty entries at the bottom of the log.
    The result is that you must scroll up to find the last log entry. To
    prevent this, we need to output anchors as CASA log comments, possibly
    timestamps, and then use javascript to navigate to the log location.
    """
    
    # Thanks to the unique timestamps in the CASA log, the implementation
    # turns out to be quite simple. Is a class overkill?
    
    @staticmethod
    def copy(context):
        output_file = os.path.join(context.report_dir, 'casapy.log')

        existing_entries = []
        if os.path.exists(output_file):
            with open(output_file, 'r') as weblog:
                existing_entries.extend(weblog.readlines())
                    
        # read existing log, appending any non-duplicate entries to our casapy
        # web log. This is Python 2.6 so we can't define the context managers
        # on the same line
        with open(output_file, 'a') as weblog:
            with open(casatools.log.logfile(), 'r') as casalog:
                to_append = [entry for entry in casalog 
                             if entry not in existing_entries]
            weblog.writelines(to_append)

#     @staticmethod    
#     def write_stage_logs(context):
#         """
#         Take the CASA log snippets attached to each result and write them to
#         the appropriate weblog directory. The log snippet is deleted from the
#         result after a successful write to keep the pickle size down. 
#         """
#         for result in context.results:
#             if not hasattr(result, 'casalog'):
#                 continue
# 
#             stage_dir = os.path.join(context.report_dir,
#                                      'stage%s' % result.stage_number)
#             if not os.path.exists(stage_dir):                
#                 os.makedirs(stage_dir)
# 
#             stagelog_entries = result.casalog
#             start = result.timestamps.start
#             end = result.timestamps.end
# 
#             stagelog_path = os.path.join(stage_dir, 'casapy.log')
#             with open(stagelog_path, 'w') as stagelog:
#                 LOG.debug('Writing CASA log entries for stage %s (%s -> %s)' %
#                           (result.stage_number, start, end))                          
#                 stagelog.write(stagelog_entries)
#                 
#             # having written the log entries, the CASA log entries have no 
#             # further use. Remove them to keep the size of the pickle small
#             delattr(result, 'casalog')
#
#    @staticmethod    
#    def write_stage_logs(context):
#        casalog = os.path.join(context.report_dir, 'casapy.log')
#
#        for result in context.results:
#            stage_dir = os.path.join(context.report_dir,
#                                     'stage%s' % result.stage_number)
#            stagelog_path = os.path.join(stage_dir, 'casapy.log')
#            if os.path.exists(stagelog_path):
#                LOG.trace('CASA log exists for stage %s, continuing..' 
#                          % result.stage_number)
##                continue
#
#            if not os.path.exists(stage_dir):                
#                os.makedirs(stage_dir)
#
#            # CASA log timestamps have seconds resolution, whereas our task
#            # timestamps have microsecond resolution. Cast down to seconds 
#            # resolution to make a comparison, taking care to leave the 
#            # original timestamp unaltered
#            start = result.timestamps.start.replace(microsecond=0)
#            end = result.timestamps.end.replace(microsecond=0)
#            end += datetime.timedelta(seconds=1)
#            
#            # get the hif_XXX command from the task attribute if possible,
#            # otherwise fall back to the Python class name accessible at
#            # taskname
#            task = result.taskname
#             
#            stagelog_entries = LogCopier._extract(casalog, start, end, task)
#            with open(stagelog_path, 'w') as stagelog:
#                LOG.debug('Writing CASA log entries for stage %s (%s -> %s)' %
#                          (result.stage_number, start, end))                          
#                stagelog.writelines(stagelog_entries)
#
#    @staticmethod
#    def _extract(filename, start, end, task=None):
#        with open(filename, 'r') as logfile:
#            rows = logfile.readlines()
#
#        # find the indices of the log entries recorded just after and just 
#        # before the end of task execution. We do this so that our subsequent
#        # search can begin these times, giving a more optimal search
#        pattern = re.compile('^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}')
#        timestamps = [pattern.match(r).group(0) for r in rows]
#        datetimes = [datetime.datetime.strptime(t, '%Y-%m-%d %H:%M:%S')
#                     for t in timestamps]
#        within_timestamps = [n for n, elem in enumerate(datetimes) 
#                             if elem > start and elem < end]            
#        start_idx, end_idx = within_timestamps[0], within_timestamps[-1]
#
#        # Task executions are bookended by statements log entries like this:
#        #
#        # 2013-02-15 13:55:47 INFO    hif_importdata::::casa+ ##########################################
#        #
#        # This regex matches this pattern, and therefore the start and end
#        # sections of the CASA log for this task 
#        pattern = re.compile('^.*?%s::::casa\+\t\#{42}$' % task)
#
#        # Rewinding from the starting timestamp, find the index of the task
#        # start log entry
#        for idx in range(within_timestamps[0], 0, -1):
#            if pattern.match(rows[idx]):
#                start_idx = idx
#                break
#
#        # looking forward from the end timestamp, find the index of the task
#        # end log entry
#        for idx in range(within_timestamps[-1], len(rows)-1):
#            if pattern.match(rows[idx]):
#                end_idx = min(idx+1, len(rows))
#                break
#
#        return rows[start_idx:end_idx]
          
          
def get_bandpass_amp_qa_scores(ms, result, plots, rootdir):
    # .. and because the caltable name is a heuristic and resolved at child 
    # taske execution time, the stage number doesn't map to the input! 
    #caltable = result.inputs['caltable']
    caltable = result.final[0].gaintable
    num_pols = utils.get_num_caltable_polarizations(caltable)
    score_types = ['AMPLITUDE_SCORE_DD',
                   'AMPLITUDE_SCORE_FN',
                   'AMPLITUDE_SCORE_SNR']
    return get_bandpass_qa_scores(ms, result, plots, score_types, rootdir, num_pols)


def get_bandpass_phase_qa_scores(ms, result, plots, rootdir):
    # .. and because the caltable name is a heuristic and resolved at child 
    # taske execution time, the stage number doesn't map to the input! 
    #caltable = result.inputs['caltable']
    caltable = result.final[0].gaintable
    num_pols = utils.get_num_caltable_polarizations(caltable)
    score_types = ['PHASE_SCORE_DD',
                   'PHASE_SCORE_FN',
                   'PHASE_SCORE_RMS']
    return get_bandpass_qa_scores(ms, result, plots, score_types, rootdir, num_pols)    

def get_bandpass_qa_scores(ms, result, plots, score_types, rootdir, num_polarizations):
    qa_data = result.qa.rawdata
    
    scores = {}
    for plot in plots:
        spw = ms.get_spectral_window(plot.parameters['spw'])
        spw_str = str(spw.id)        
        dd = ms.get_data_description(spw=spw)
        if dd is None:
            continue

        antennas = ms.get_antenna(plot.parameters['ant'])
        assert len(antennas) is 1, 'plot antennas != 1'
        antenna = antennas[0]

        pol = plot.parameters['pol']
        pol_id = dd.get_polarization_id(pol)

        png = os.path.relpath(plot.abspath, rootdir)
        thumbnail = os.path.relpath(plot.thumbnail, rootdir)
        
        png_scores = {'antenna'   : antenna.name,
                      'spw'       : spw.id,
                      'pol'       : pol,
                      'thumbnail' : thumbnail}
        scores[png] = png_scores

        # QA dictionary keys are peculiar, in that their index is a
        # function of both antenna and feed.
        qa_id = int(antenna.id) * num_polarizations + pol_id
        qa_str = str(qa_id)

        for score_type in score_types:            
            if 'QASCORES' in qa_data:
                try:
                    score = qa_data['QASCORES'][score_type][spw_str][qa_str]
                except KeyError:
                    LOG.error('Could not get %s score for %s (%s) spw %s pol %s (id=%s)' % (score_type, antenna.name, antenna.id, spw_str, pol, qa_str))
                    score = None 
            else:
                score = 1.0
            png_scores[score_type] = score
        
    return scores


# renderer_map holds the mapping of tasks to result renderers for
# task-dependent weblog sections. This lets us write customised output for
# each task type.
renderer_map = {
    T2_3MDetailsRenderer : {
        hifa.tasks.Wvrgcalflag    : T2_3MDetailsWvrgcalflagRenderer(),
        # hif.tasks.Bandpass       : T2_3MDetailsBandpassRenderer(),
    },
    T2_4MDetailsRenderer : {
        hif.tasks.Applycal       : T2_4MDetailsApplycalRenderer(),                        
        hif.tasks.AgentFlagger   : T2_4MDetailsAgentFlaggerRenderer(),
        hifa.tasks.ALMAAgentFlagger : T2_4MDetailsAgentFlaggerRenderer(),
        hifa.tasks.FlagDeterALMA : T2_4MDetailsAgentFlaggerRenderer(),
        hif.tasks.Atmflag        : T2_4MDetailsDefaultRenderer('t2-4m_details-hif_atmflag.html'),
        hif.tasks.Bandpass       : T2_4MDetailsBandpassRenderer(),
        hif.tasks.Bandpassflagchans: T2_4MDetailsBandpassFlagRenderer(),
        hif.tasks.Clean          : T2_4MDetailsCleanRenderer(),
        hif.tasks.CleanList      : T2_4MDetailsCleanRenderer(),
        hif.tasks.ExportData     : T2_4MDetailsDefaultRenderer('t2-4m_details-hif_exportdata.html'),
        hif.tasks.Flagchans      : T2_4MDetailsFlagchansRenderer(),
        hifa.tasks.FluxcalFlag   : T2_4MDetailsDefaultRenderer('t2-4m_details-hif_fluxcalflag.html'),
        hif.tasks.Fluxscale      : T2_4MDetailsDefaultRenderer('t2-4m_details-fluxscale.html'),
        hif.tasks.Gaincal        : T2_4MDetailsGaincalRenderer(),
        hifa.tasks.GcorFluxscale : T2_4MDetailsGFluxscaleRenderer('t2-4m_details-hif_gfluxscale.html'),
        hif.tasks.ImportData     : T2_4MDetailsImportDataRenderer(),
        hifa.tasks.ALMAImportData   : T2_4MDetailsImportDataRenderer(),
        hif.tasks.Lowgainflag    : T2_4MDetailsLowgainFlagRenderer(),
        hif.tasks.MakeCleanList  : T2_4MDetailsDefaultRenderer('t2-4m_details-hif_makecleanlist.html'),
        hif.tasks.NormaliseFlux  : T2_4MDetailsDefaultRenderer('t2-4m_details-hif_normflux.html'),
        hif.tasks.RefAnt         : T2_4MDetailsDefaultRenderer('t2-4m_details-hif_refant.html'),
        hif.tasks.Setjy          : T2_4MDetailsSetjyRenderer('t2-4m_details-hif_setjy.html'),
        hifa.tasks.TimeGaincal   : T2_4MDetailsGaincalRenderer(),
        hifa.tasks.Tsyscal       : T2_4MDetailsTsyscalRenderer(),
        hifa.tasks.Tsysflag      : T2_4MDetailsTsysflagRenderer(),
        hifa.tasks.Tsysflagchans : T2_4MDetailsTsysflagchansRenderer(),
        hifa.tasks.Tsysflagspectra: T2_4MDetailsTsysflagspectraRenderer(),
        hifa.tasks.Wvrgcal       : T2_4MDetailsDefaultRenderer('t2-4m_details-hif_wvrgcal.html'),
        hifa.tasks.Wvrgcalflag   : T2_4MDetailsWvrgcalflagRenderer(),
        hsd.tasks.SDReduction    : T2_4MDetailsDefaultRenderer('t2-4-singledish.html'),
        hsd.tasks.SDImportData   : T2_4MDetailsSingleDishImportDataRenderer(always_rerender=False),
        hsd.tasks.SDInspectData  : T2_4MDetailsSingleDishInspectDataRenderer(always_rerender=False),
        hsd.tasks.SDCalTsys      : T2_4MDetailsSingleDishCalTsysRenderer(always_rerender=False),
        hsd.tasks.SDCalSky       : T2_4MDetailsSingleDishCalSkyRenderer(always_rerender=False),
        hsd.tasks.SDApplyCal     : T2_4MDetailsSingleDishApplycalRenderer(always_rerender=False),
        hsd.tasks.SDBaselineOld  : T2_4MDetailsSingleDishBaselineRenderer(always_rerender=False),
        hsd.tasks.SDBaseline     : T2_4MDetailsSingleDishBaselineRenderer(always_rerender=False),
        hsd.tasks.SDFlagData     : T2_4MDetailsDefaultRenderer('t2-4m_details-hsd_flagdata.html', always_rerender=False),
        hsd.tasks.SDImaging      : T2_4MDetailsSingleDishImagingRenderer(always_rerender=False),
        hsd.tasks.SDImagingOld   : T2_4MDetailsSingleDishImagingRenderer(always_rerender=False),
        hsd.tasks.SDFlagBaseline : T2_4MDetailsSingleDishFlagBaselineRenderer(always_rerender=False),
        hsd.tasks.SDPlotFlagBaseline : T2_4MDetailsSingleDishPlotFlagBaselineRenderer(always_rerender=False),
        hsd.tasks.SDImportData2  : T2_4MDetailsImportDataRenderer(),
        hifv.tasks.importdata.VLAImportData : T2_4MDetailsVLAImportDataRenderer(),
        hifv.tasks.flagging.vlaagentflagger.VLAAgentFlagger : T2_4MDetailsVLAAgentFlaggerRenderer(template='t2-4m_details-hifv_flagdata.html', always_rerender=False),
        hifv.tasks.flagging.flagdetervla.FlagDeterVLA : T2_4MDetailsVLAAgentFlaggerRenderer(template='t2-4m_details-hifv_flagdata.html', always_rerender=False),
        hifv.tasks.setmodel.SetModel : T2_4MDetailsDefaultRenderer('t2-4m_details-hifv_setmodel.html', always_rerender=False),
        hifv.tasks.setmodel.VLASetjy : T2_4MDetailsSetjyRenderer('t2-4m_details-hif_setjy.html', always_rerender=False),
        hifv.tasks.priorcals.priorcals.Priorcals : T2_4MDetailspriorcalsRenderer('t2-4m_details-hifv_priorcals.html', always_rerender=False),
        hifv.tasks.flagging.hflag.Heuristicflag : T2_4MDetailsDefaultRenderer('t2-4m_details-hifv_hflag.html', always_rerender=False),
        hifv.tasks.testBPdcals                   : T2_4MDetailstestBPdcalsRenderer('t2-4m_details-hifv_testbpdcals.html', always_rerender=False),
        hifv.tasks.Hanning                       :T2_4MDetailsDefaultRenderer('t2-4m_details-hifv_hanning.html', always_rerender=False),
        hifv.tasks.flagging.flagbaddeformatters.FlagBadDeformatters : T2_4MDetailsDefaultRenderer('t2-4m_details-hifv_flagbaddef.html', always_rerender=False),
        hifv.tasks.flagging.uncalspw.Uncalspw    : T2_4MDetailsDefaultRenderer('t2-4m_details-hifv_uncalspw.html', always_rerender=False),
        hifv.tasks.flagging.checkflag.Checkflag  : T2_4MDetailsDefaultRenderer('t2-4m_details-hifv_checkflag.html', always_rerender=False),
        hifv.tasks.semiFinalBPdcals              : T2_4MDetailssemifinalBPdcalsRenderer('t2-4m_details-hifv_semifinalbpdcals.html', always_rerender=False),
        hifv.tasks.fluxscale.solint.Solint       : T2_4MDetailsSolintRenderer('t2-4m_details-hifv_solint.html', always_rerender=False),
        hifv.tasks.setmodel.fluxgains.Fluxgains  : T2_4MDetailsDefaultRenderer('t2-4m_details-hifv_fluxgains.html', always_rerender=False),
        hifv.tasks.fluxscale.fluxboot.Fluxboot   : T2_4MDetailsfluxbootRenderer('t2-4m_details-hifv_fluxboot.html', always_rerender=False),
        hifv.tasks.Finalcals                     : T2_4MDetailsfinalcalsRenderer('t2-4m_details-hifv_finalcals.html', always_rerender=False),
        #hifv.tasks.Applycals                     : T2_4MDetailsDefaultRenderer('t2-4m_details-hifv_applycals.html', always_rerender=True),
        hifv.tasks.Applycals                     :T2_4MDetailsApplycalRenderer(always_rerender=False),
        hifv.tasks.flagging.targetflag.Targetflag : T2_4MDetailstargetflagRenderer('t2-4m_details-hifv_targetflag.html', always_rerender=False),
        hifv.tasks.Statwt                         : T2_4MDetailsDefaultRenderer('t2-4m_details-hifv_statwt.html', always_rerender=False)

    }
}

# adding classes to this tuple always rerenders their content, bypassing the
# cache or 'existing file' checks. This is useful for developing and debugging
# as you can just call WebLogGenerator.render(context) 
DEBUG_CLASSES = ()

    