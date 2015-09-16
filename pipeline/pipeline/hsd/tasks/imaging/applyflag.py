from __future__ import absolute_import

import os
import numpy

import pipeline.infrastructure as infrastructure
import pipeline.infrastructure.casatools as casatools
import pipeline.infrastructure.sdfilenamer as filenamer
from pipeline.infrastructure import casa_tasks
from .. import common

LOG = infrastructure.get_logger(__name__)

class SDApplyFlagInputs(common.SingleDishInputs):
    """
    Inputs for applying flags to each scantable 
    """
    def __init__(self, context, infiles=None, iflist=None, pollist=None):
        self._init_properties(vars())
        self._to_list(['infiles', 'iflist', 'pollist'])

class SDApplyFlagResults(common.SingleDishResults):
    def __init__(self, task=None, success=None, outcome=None):
        super(SDApplyFlagResults, self).__init__(task, success, outcome)

    def merge_with_context(self, context):
        super(SDApplyFlagResults, self).merge_with_context(context)

    def _outcome_name(self):
        return ''


class SDApplyFlag(common.SingleDishTaskTemplate):
    Inputs = SDApplyFlagInputs
    
    @common.datatable_setter
    def prepare(self):
        # for each data
        context = self.inputs.context
        reduction_group = context.observing_run.reduction_group
        infiles = self.inputs.infiles

        index_for_infiles = [context.observing_run.st_names.index(v) 
                             for v in infiles]
        # flag all WVR data and off-source data  
        for index in index_for_infiles:
            data = context.observing_run[index]
            self._apply_apriori_flags(data)

        namer = filenamer.BaselineSubtractedTable()

        # loop over reduction group
        for (group_id, group_desc) in reduction_group.items():
            
            # for each group member
            for member in group_desc:
                index = member.antenna
                if index not in index_for_infiles:
                    continue

                # apply baseline flags to the data
                data = context.observing_run[index]
                spwid = member.spw
                namer.spectral_window(spwid)
                namer.asdm(common.asdm_name(data))
                namer.antenna_name(data.antenna.name)
                bltable_name = namer.get_filename()
                if not os.path.exists(bltable_name):
                    continue
                filename = data.work_data
#                 filename = data.baselined_name
#                 if not os.path.exists(filename):
#                     filename = data.name
                srctype = data.calibration_strategy['srctype']
                self._apply_baseline_flags(filename, index, spwid, srctype)

            
        result = SDApplyFlagResults(task=self.__class__,
                                 success=True,
                                 outcome=None)
        result.task = self.__class__

        if self.inputs.context.subtask_counter is 0: 
            result.stage_number = self.inputs.context.task_counter - 1
        else:
            result.stage_number = self.inputs.context.task_counter 

        return result
    
    def analyse(self, result):
        return result

    def _apply_apriori_flags(self, data):
        # infile
        filename = data.work_data
#         filename = data.baselined_name
#         if not os.path.exists(filename):
#             filename = data.name
            
        # flag by spw id (exclude WVR, SQLD, POINTING, ATMCAL, ...)
        spws = data.spectral_window
        nonscience = common.list_to_selection(list(common.nonscience_spw(spws)))
        if len(nonscience) > 0:
            args = {'infile': filename,
                    'mode': 'manual',
                    'spw': nonscience}
            job = casa_tasks.sdflag(**args)
            self._executor.execute(job, merge=False)

        # flag by intents
        srctype = data.calibration_strategy['srctype']
        science = list(common.science_spw(spws))
        with common.TableSelector(filename, query='SRCTYPE != %s && IFNO IN %s'%(srctype,science)) as tb:
            rows = tb.rownumbers()

        if len(rows) > 0:
            args = {'infile': filename,
                    'mode': 'rowid',
                    'row': common.list_to_selection(rows)}
            job = casa_tasks.sdflag(**args)
            self._executor.execute(job, merge=False)

    def _apply_baseline_flags(self, filename, antenna, spwid, on_source):
        context = self.inputs.context
        datatable = context.observing_run.datatable_instance
        datatable_name = datatable.plaintable
        tb = datatable.tb1
        taqlstring = 'USING STYLE PYTHON SELECT RO.ROW FROM "%s" RO, "%s" RW WHERE ANTENNA==%s && IF==%s && SRCTYPE==%s && RW.FLAG_SUMMARY==0'\
            % (os.path.join(datatable_name, 'RO'), os.path.join(datatable_name, 'RW'), antenna, spwid, on_source)
        tx = tb.taql(taqlstring)
        rows = tx.getcol('ROW')
        tx.close()
        
        if len(rows) > 0:
            LOG.info('Apply flags determined by hsd_blflag')
            args = {'infile': filename,
                    'mode': 'rowid',
                    'row': common.list_to_selection(rows)}
            job = casa_tasks.sdflag(**args)
            self._executor.execute(job, merge=False)
        else:
            LOG.info('No flagged rows for %s (spw %s)'%(os.path.basename(filename.rstrip('/')),spwid)) 