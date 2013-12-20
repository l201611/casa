import pipeline.hif.tasks as hif_tasks
import pipeline.hsd.tasks as hsd_tasks
import pipeline.hifa.tasks as hifa_tasks
import pipeline.hifv.tasks as hifv_tasks

CasaTaskDict = {
                   'hif_antpos'       : 'Antpos',
                   'hif_atmflag'      : 'Atmflag',
                   'hif_applycal'     : 'Applycal',
                   'hif_bandpass'     : 'Bandpass',
                   'hif_bpflagchans'  : 'Bandpassflagchans',
		   'hif_clean'        : 'Clean', 
		   'hif_cleanlist'    : 'CleanList', 
                   'hif_exportdata'   : 'ExportData',
                   'hif_gaincal'      : 'Gaincal',
                   'hif_lowgainflag'  : 'Lowgainflag',
                   'hif_importdata'   : 'ImportData',
		   'hif_makecleanlist': 'MakeCleanList', 
                   'hif_refant'       : 'RefAnt',
		   'hif_restoredata'  : 'RestoreData',
                   'hif_setjy'        : 'Setjy',
                   'hsd_calsky'       : 'SDCalSky',
                   'hsd_caltsys'      : 'SDCalTsys',
                   'hsd_applycal'     : 'SDApplyCal',
                   'hsd_exportdata'   : 'SDExportData',
                   'hsd_importdata'   : 'SDImportData',
                   'hsd_importdata2'  : 'SDImportData2',
                   'hsd_inspectdata'  : 'SDInspectData',
                   'hsd_imaging'      : 'SDImaging',
                   'hsd_imaging2'      : 'SDImaging2',
                   'hsd_baseline'     : 'SDBaseline',
                   'hsd_flagdata'     : 'SDFlagData',
                   'hsd_flagbaseline' : 'SDFlagBaseline',
                   'hsd_plotflagbaseline': 'SDPlotFlagBaseline',
                   'hsd_reduce'       : 'SDReduction',
                   'hifa_importdata' : 'ALMAImportData',
                   'hifa_flagdata'    : 'ALMAAgentFlagger',
                   'hifa_fluxcalflag'  : 'FluxcalFlag',
                   'hifa_gfluxscale'   : 'GcorFluxscale',
                   'hifa_timegaincal' : 'TimeGaincal',
                   'hifa_tsyscal'      : 'Tsyscal',
                   'hifa_tsysflag'     : 'Tsysflag',
                   'hifa_tsysflagchans': 'Tsysflagchans',
                   'hifa_wvrgcal'      : 'Wvrgcal',
                   'hifa_wvrgcalflag'  : 'Wvrgcalflag',
                   'hifv_importdata'  : 'VLAImportData',
                   'hifv_flagdata'    : 'FlagDeterVLA',
                   'hifv_setmodel'    : 'SetModel',
                   'hifv_priorcals'   : 'Priorcals',
                   'hifv_hflag'       : 'Heuristicflag',
                   'hifv_testBPdcals' : 'testBPdcals',
                   'hifv_flagbaddef'  : 'FlagBadDeformatters',
                   'hifv_uncalspw'    : 'Uncalspw',
                   'hifv_checkflag'   : 'Checkflag',
                   'hifv_semiFinalBPdcals' : 'semiFinalBPdcals',
                   'hifv_solint'      : 'Solint',
                   'hifv_testgains'   : 'Testgains',
                   'hifv_fluxgains'   : 'Fluxgains',
                   'hifv_fluxboot'    : 'Fluxboot',
                   'hifv_finalcals'   : 'Finalcals',
                   'hifv_applycals'   : 'Applycals',
                   'hifv_targetflag'  : 'Targetflag',
                   'hifv_statwt'      : 'Statwt'
               }



classToCASATask = {
    # ALMA interferometry tasks ---------------------------------------------
    hifa_tasks.ALMAImportData : 'hifa_importdata',
    hifa_tasks.ALMAAgentFlagger : 'hifa_flagdata',
    hifa_tasks.FluxcalFlag : 'hifa_fluxcalflag',
    hifa_tasks.GcorFluxscale : 'hifa_gfluxscale',
    hifa_tasks.TimeGaincal : 'hifa_timegaincal',
    hifa_tasks.Tsyscal : 'hifa_tsyscal',
    hifa_tasks.Tsysflag : 'hifa_tsysflag',
    hifa_tasks.Tsysflagchans : 'hifa_tsysflagchans',
    hifa_tasks.Wvrgcal : 'hifa_wvrgcal',
    hifa_tasks.Wvrgcalflag : 'hifa_wvrgcalflag',
    # Interferometry tasks ---------------------------------------------------
    hif_tasks.Antpos : 'hif_antpos',
    hif_tasks.Applycal : 'hif_applycal',    
    hif_tasks.Atmflag : 'hif_atmflag',
    hif_tasks.Bandpass : 'hif_bandpass',
    hif_tasks.Bandpassflagchans : 'hif_bpflagchans',
    hif_tasks.Clean : 'hif_clean',
    hif_tasks.CleanList : 'hif_cleanlist',
    hif_tasks.ExportData : 'hif_exportdata',
    hif_tasks.Fluxcal : 'hif_fluxcal',
    hif_tasks.Fluxscale : 'hif_fluxscale',
    hif_tasks.Gaincal : 'hif_gaincal',
    hif_tasks.ImportData : 'hif_importdata',
    hif_tasks.Lowgainflag : 'hif_lowgainflag',
    hif_tasks.MakeCleanList : 'hif_makecleanlist',
    hif_tasks.RefAnt : 'hif_refant',
    hif_tasks.RestoreData : 'hif_restoredata',
    hif_tasks.Setjy : 'hif_setjy',
    # Single dish tasks ------------------------------------------------------
    hsd_tasks.SDCalSky : 'hsd_calsky',
    hsd_tasks.SDCalTsys : 'hsd_caltsys',
    hsd_tasks.SDApplyCal : 'hsd_applycal',
    hsd_tasks.SDExportData : 'hsd_exportdata',
    hsd_tasks.SDImportData : 'hsd_importdata',
    hsd_tasks.SDImportData2 : 'hsd_importdata2',
    hsd_tasks.SDInspectData : 'hsd_inspectdata',
    hsd_tasks.SDImaging : 'hsd_imaging',
    hsd_tasks.SDImaging2 : 'hsd_imaging2',
    hsd_tasks.SDBaseline : 'hsd_baseline',
    hsd_tasks.SDFlagData : 'hsd_flagdata',
    hsd_tasks.SDFlagBaseline : 'hsd_flagbaseline',
    hsd_tasks.SDPlotFlagBaseline : 'hsd_plotflagbaseline',
    hsd_tasks.SDReduction : 'hsd_reduce',
    #VLA tasks
    hifv_tasks.VLAImportData       : 'hifv_importdata',
    hifv_tasks.FlagDeterVLA        : 'hifv_flagdata',
    hifv_tasks.SetModel            : 'hifv_setmodel', 
    hifv_tasks.Priorcals           : 'hifv_priorcals',
    hifv_tasks.Heuristicflag       : 'hifv_hflag', 
    hifv_tasks.testBPdcals         : 'hifv_testBPdcals',
    hifv_tasks.FlagBadDeformatters : 'hifv_flagbaddef',
    hifv_tasks.Uncalspw            : 'hifv_uncalspw',
    hifv_tasks.Checkflag           : 'hifv_checkflag',
    hifv_tasks.semiFinalBPdcals    : 'hifv_semiFinalBPdcals',
    hifv_tasks.Solint              : 'hifv_solint', 
    hifv_tasks.Testgains           : 'hifv_testgains',
    hifv_tasks.Fluxgains           : 'hifv_fluxgains',
    hifv_tasks.Fluxboot            : 'hifv_fluxboot', 
    hifv_tasks.Finalcals           : 'hifv_finalcals',
    hifv_tasks.Applycals           : 'hifv_applycals',
    hifv_tasks.Targetflag          : 'hifv_targetflag',
    hifv_tasks.Statwt              : 'hifv_statwt' 
}
