from __future__ import absolute_import
import os
import shutil

import pipeline.infrastructure.logging as logging
import pipeline.infrastructure.pipelineqa as pqa
import pipeline.infrastructure.utils as utils
import pipeline.qa.gpcal as gpcal

from pipeline.hif.tasks.gaincal import common

LOG = logging.get_logger(__name__)

class TimegaincalQAPool(pqa.QAScorePool):
    score_types = {'XY_SCORE'   : 'X-Y phase deviation',
                   'X2X1_SCORE' : 'X2-X1 phase deviation'}

    short_msg = {'XY_SCORE'   : 'X-Y deviation',
                 'X2X1_SCORE' : 'X2-X1 deviation'}

    def __init__(self, qa_results):
        super(TimegaincalQAPool, self).__init__()
        self.qa_results = qa_results
        self._representative = self.qa_results['QASCORES']['SCORES']['TOTAL']

    def update_scores(self, phase_field_ids):
        total_score = 1.0
        longmsg = ''
        short_msg = ''
        for field_id in phase_field_ids:
            if (self.qa_results['QASCORES']['SCORES'][field_id]['XY_TOTAL'] < total_score):
                total_score = self.qa_results['QASCORES']['SCORES'][field_id]['XY_TOTAL']           
                longmsg = self.score_types['XY_SCORE']
                shortmsg  =self.short_msg['XY_SCORE']
            if (self.qa_results['QASCORES']['SCORES'][field_id]['X2X1_TOTAL'] < total_score):
                total_score = self.qa_results['QASCORES']['SCORES'][field_id]['X2X1_TOTAL']           
                longmsg = self.score_types['X2X1_SCORE']
                shortmsg  =self.short_msg['X2X1_SCORE']

        self.pool = [pqa.QAScore(total_score, longmsg=longmsg, shortmsg=shortmsg)]

class TimegaincalQAHandler(pqa.QAResultHandler):
    """
    QA handler for an uncontained TimegaincalResult.
    """
    result_cls = common.GaincalResults
    child_cls = None

    def handle(self, context, result):
        vis = result.inputs['vis']
        ms = context.observing_run.get_ms(vis)
        phase_field_ids = [field.id for field in ms.get_fields(intent='PHASE')]

        qa_dir = os.path.join(context.report_dir,
                               'stage%s' % result.stage_number,
                               'qa')

        if not os.path.exists(qa_dir):
            os.makedirs(qa_dir)

        foundIntP = False
        for calapp in result.final:
            try:
                solint = calapp.origin.inputs['solint']
                calmode = calapp.origin.inputs['calmode']
                if ((not foundIntP) and (solint == 'int') and (calmode == 'p')):
                    foundIntP = True
                    try:
                        qa_results = gpcal.gpcal(calapp.gaintable)
                        result.qa = TimegaincalQAPool(qa_results)
                    except Exception as e2:
                        print 'Timegaincal QA error:', e2
                    result.qa.update_scores(phase_field_ids)
            except Exception as e:
                LOG.error('Problem occurred running QA analysis. QA '
                          'results will not be available for this task')
                LOG.exception(e)


class TimegaincalListQAHandler(pqa.QAResultHandler):
    """
    QA handler for a list containing TimegaincalResults.
    """
    result_cls = list
    child_cls = common.GaincalResults

    def handle(self, context, result):
        # collate the QAScores from each child result, pulling them into our
        # own QAscore list
        collated = utils.flatten([r.qa.pool for r in result])
        result.qa.pool[:] = collated
