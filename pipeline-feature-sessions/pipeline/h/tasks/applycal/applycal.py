from __future__ import absolute_import

import collections
import copy
import os
import types

import pipeline.infrastructure as infrastructure
from pipeline.infrastructure import casa_tasks
import pipeline.infrastructure.basetask as basetask
import pipeline.infrastructure.callibrary as callibrary
import pipeline.infrastructure.utils as utils
import pipeline.infrastructure.vdp as vdp

from ...heuristics.fieldnames import IntentFieldnames

__all__ = [
    'Applycal',
    'ApplycalInputs',
    'ApplycalResults'
]

LOG = infrastructure.get_logger(__name__)


class ApplycalInputs(vdp.StandardInputs):
    """
    ApplycalInputs defines the inputs for the Applycal pipeline task.
    """
    @vdp.VisDependentProperty
    def antenna(self):
        return ''

    @antenna.convert
    def antenna(self, value):
        antennas = self.ms.get_antenna(value)
        # if all antennas are selected, return ''
        if len(antennas) == len(self.ms.antennas):
            return ''
        return utils.find_ranges([a.id for a in antennas])

    applymode = vdp.VisDependentProperty(default='calflagstrict')

    @vdp.VisDependentProperty
    def field(self):
        # this will give something like '0542+3243,0343+242'
        field_finder = IntentFieldnames()
        intent_fields = field_finder.calculate(self.ms, self.intent)

        # run the answer through a set, just in case there are duplicates
        fields = set()
        fields.update(utils.safe_split(intent_fields))

        return ','.join(fields)

    flagbackup = vdp.VisDependentProperty(default=True)
    flagdetailedsum = vdp.VisDependentProperty(default=False)
    flagsum = vdp.VisDependentProperty(default=True)
    intent = vdp.VisDependentProperty(default='TARGET,PHASE,BANDPASS,AMPLITUDE,CHECK')

    @vdp.VisDependentProperty
    def spw(self):
        science_spws = self.ms.get_spectral_windows(with_channels=True)
        return ','.join([str(spw.id) for spw in science_spws])

    @basetask.log_equivalent_CASA_call
    def __init__(self, context, output_dir=None, vis=None, field=None, spw=None, antenna=None, intent=None,
                 opacity=None, parang=None, applymode=None, flagbackup=None, flagsum=None, flagdetailedsum=None):
        # pipeline inputs
        self.context = context
        # vis must be set first, as other properties may depend on it
        self.vis = vis
        self.output_dir = output_dir

        # data selection arguments
        self.field = field
        self.spw = spw
        self.antenna = antenna
        self.intent = intent

        # solution parameters
        self.opacity = opacity
        self.parang = parang
        self.applymode = applymode
        self.flagbackup = flagbackup
        self.flagsum = flagsum
        self.flagdetailedsum = flagdetailedsum

    def to_casa_args(self):
        casa_args = super(ApplycalInputs, self).to_casa_args()
        del casa_args['flagsum']
        del casa_args['flagdetailedsum']
        return casa_args


class ApplycalResults(basetask.Results):
    """
    ApplycalResults is the results class for the pipeline Applycal task.     
    """
    
    def __init__(self, applied=None):
        """
        Construct and return a new ApplycalResults.
        
        The resulting object should be initialized with a list of
        CalibrationTables corresponding to the caltables applied by this task.

        :param applied: caltables applied by this task
        :type applied: list of :class:`~pipeline.domain.caltable.CalibrationTable`
        """
        if applied is None:
            applied = []

        super(ApplycalResults, self).__init__()
        self.applied = set()
        self.applied.update(applied)

    def merge_with_context(self, context):
        """
        Merges these results with the given context by examining the context
        and marking any applied caltables, so removing them from subsequent
        on-the-fly calibration calculations.

        See :method:`~pipeline.Results.merge_with_context`
        """
        if not self.applied:
            LOG.error('No results to merge')

        for calapp in self.applied:
            LOG.trace('Marking %s as applied' % calapp.as_applycal())
            context.callibrary.mark_as_applied(calapp.calto, calapp.calfrom)

    def __repr__(self):
        for caltable in self.applied:
            s = 'ApplycalResults:\n'
            if type(caltable.gaintable) is types.ListType:
                basenames = [os.path.basename(x) for x in caltable.gaintable]
                s += '\t{name} applied to {vis} spw #{spw}\n'.format(
                    spw=caltable.spw, vis=os.path.basename(caltable.vis),
                    name=','.join(basenames))
            else:
                s += '\t{name} applied to {vis} spw #{spw}\n'.format(
                    name=caltable.gaintable, spw=caltable.spw, 
                    vis=os.path.basename(caltable.vis))
        return s


class Applycal(basetask.StandardTaskTemplate):
    """
    Applycal executes CASA applycal tasks for the current active context
    state, applying calibrations registered with the pipeline context to the
    target measurement set.
    
    Applying the results from this task to the context marks the referred
    tables as applied. As a result, they will not be included in future
    on-the-fly calibration arguments.
    """
    Inputs = ApplycalInputs

    def prepare(self):
        inputs = self.inputs

        # Get the target data selection for this task as a CalTo object
        calto = callibrary.get_calto_from_inputs(inputs)

        # Now get the calibration state for that CalTo data selection. The
        # returned dictionary of CalTo:CalFroms specifies the calibrations to
        # be applied and the data selection to apply them to.
        #
        # Note that no 'ignore' argument is given to get_calstate
        # (specifically, we don't say ignore=['calwt'] like many other tasks)
        # as applycal is a task that can handle calwt and so different values
        # of calwt should in this case result in different tasks.
        calstate = inputs.context.callibrary.get_calstate(calto)
        merged = calstate.merged()

        merged = callibrary.fix_cycle0_data_selection(inputs.context, merged)

        jobs = []
        for calto, calfroms in merged.iteritems():
            # if there's nothing to apply for this data selection, continue
            if not calfroms:
                continue

            # arrange a calibration job for the unique data selection
            inputs.spw = calto.spw
            inputs.field = calto.field
            inputs.intent = calto.intent

            task_args = inputs.to_casa_args()

            # set the on-the-fly calibration state for the data selection.
            calapp = callibrary.CalApplication(calto, calfroms)
            task_args['gaintable'] = calapp.gaintable
            task_args['gainfield'] = calapp.gainfield
            task_args['spwmap'] = calapp.spwmap
            task_args['interp'] = calapp.interp
            task_args['calwt'] = calapp.calwt
            task_args['applymode'] = inputs.applymode

            ### Note this is a temporary workaround ###
            task_args['antenna'] = '*&*'

            jobs.append(casa_tasks.applycal(**task_args))

        # if requested, schedule additional flagging tasks to determine
        # statistics
        if inputs.flagsum:
            # schedule a flagdata summary jobs either side of the applycal jobs
            jobs.insert(0, casa_tasks.flagdata(vis=inputs.vis, mode='summary', name='before'))
            jobs.append(casa_tasks.flagdata(vis=inputs.vis, mode='summary', name='applycal'))

            if inputs.flagdetailedsum:
                # Schedule a flagdata job to determine flagging stats per spw
                # and per field
                ms = inputs.context.observing_run.get_ms(inputs.vis)
                flagkwargs = ['spw="{!s}" fieldcnt=True mode="summary" name="AntSpw{:0>3}"'.format(spw.id, spw.id)
                              for spw in ms.get_spectral_windows()]
                jobs.append(casa_tasks.flagdata(vis=inputs.vis, mode='list', inpfile=flagkwargs, flagbackup=False))

        # execute the jobs and capture the output
        job_results = [self._executor.execute(job) for job in jobs]
        flagdata_results = [job_result for job, job_result in zip(jobs, job_results) if job.fn.__name__ == 'flagdata']

        applied_calapps = [callibrary.CalApplication(calto, calfroms) for calto, calfroms in merged.iteritems()]
        result = ApplycalResults(applied_calapps)

        # extract the flagging results if required
        if inputs.flagsum:
            result.summaries = [flagdata_results[0], flagdata_results[1]]
            if inputs.flagdetailedsum:
                result.flagsummary = reshape_flagdata_summary(flagdata_results[2])

        return result

    def analyse(self, result):
        return result


def reshape_flagdata_summary(flagdata_result):
    """
    Reshape a flagdata result so that results are grouped by field.

    :param flagdata_result:
    :return:
    """
    # Set into single dictionary report (single spw) if only one dict returned
    if not all([key.startswith('report') for key in flagdata_result]):
        flagdata_result = {'report0': flagdata_result}

    flagsummary = collections.defaultdict(dict)
    for report_level, report in flagdata_result.iteritems():
        report_name = report['name']
        report_type = report['type']
        # report keys are all fieldnames with the exception of 'name' and
        # 'type', which are in there too.
        for field_name in [key for key in report if key not in ('name', 'type')]:
            # deepcopy to avoid modifying the results dict
            flagsummary[field_name][report_level] = copy.deepcopy(report[field_name])
            flagsummary[field_name][report_level]['name'] = '{!s}Field_{!s}'.format(report_name, field_name)
            flagsummary[field_name][report_level]['type'] = report_type

    return flagsummary
