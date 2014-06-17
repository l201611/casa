import contextlib
import itertools
import multiprocessing
import os
import Queue
from threading import *
import time
import re
import numpy

import libsakurapy
import _casasakura

from taskinit import gentools, ms, ssd
#import mtpy
import reductionhelper_util as rhutil

def dbgPrint(msg):
	#print(msg)
	pass

class Context(object):
	# Attributes are inQ, inCv, outQ, outCv, qLen, pendingItems
	pass

class EndOfDataException(BaseException):
	pass

EOD = EndOfDataException() # singleton instance. Use 'is' to compare.

def worker(func, context):
	try:
		while True:
			item = None
			with context.inCv:
				while True:
					try:
						item = context.inQ.get(False)
						if item is EOD:
							raise item
						break
					except Queue.Empty:
						context.inCv.wait()
			try:
				result = func(item)
			except Exception as e:
				result = e
			with context.outCv:
				context.outQ.put(result)
				context.outCv.notify()
	except EndOfDataException:
		pass
	finally:
		thr_id = current_thread().ident
		dbgPrint("{0} terminated".format(thr_id))

# out of order and parallel execution generator
def paraMap(numThreads, func, dataSource):
	assert numThreads > 0
	context = Context()
	context.qLen = int(numThreads * 1.5)
	assert context.qLen >= numThreads
	context.inQ = Queue.Queue(maxsize=context.qLen)
	context.inCv = Condition()
	context.outQ = Queue.Queue(maxsize=context.qLen)
	context.outCv = Condition()
	context.pendingItems = 0
	threads = []
	for i in range(numThreads):
		thr = Thread(target=worker, args=(func, context))
		thr.daemon = True
		thr.start()
		threads.append(thr)
	def fillInQ(context):
		try:
			while context.pendingItems < context.qLen:
				item = dataSource.next()
				with context.inCv:
					context.inQ.put(item, False)
					context.pendingItems += 1
					context.inCv.notify()
		except Queue.Full:
			assert False
	def putEODIntoInQ(context):
		try:
			with context.inCv:
				context.inQ.put(EOD, False)
				context.pendingItems += 1
				context.inCv.notify()
		except Full:
			assert False
	def getFromOutQ(context):
		assert 0 < context.pendingItems and context.pendingItems <= context.qLen
		with context.outCv:
			while True:
				try:
					item = context.outQ.get(False)
					context.pendingItems -= 1
					return item
				except Queue.Empty:
					context.outCv.wait()
	try:
		fillInQ(context)
		assert 0 < context.pendingItems and context.pendingItems <= context.qLen
		while context.pendingItems > 0:
			item = getFromOutQ(context)
			assert 0 <= context.pendingItems and context.pendingItems < context.qLen
			try:
				fillInQ(context)
				assert 0 < context.pendingItems and context.pendingItems <= context.qLen
			finally:
				yield item
	except StopIteration as e:
		while context.pendingItems > 0:
			yield getFromOutQ(context)
	assert context.pendingItems == 0
	for i in range(numThreads):
		assert context.pendingItems < context.qLen
		putEODIntoInQ(context)

class GenerateQueryHelper(object):
    def __init__(self, ms, data_desc_id_by_spw, data_desc_id, antenna_id):
        ms.selectinit(datadescid=data_desc_id)
	ms.selecttaql('ANTENNA1==' + str(antenna_id)+' && ANTENNA2==' + str(antenna_id))

        self.ms_name = ms.name()
        self.data_desc_id__by_spw = data_desc_id_by_spw
        self.data_desc_id = data_desc_id
        self.antenna_id = antenna_id
        self.valid_selection = True
        self.selected_idx = None

    def do_msselect(self, ms, field, spw, timerange, antenna, scan, observation, msselect):
        self.timerange = timerange
        self.msselect = msselect
        self._do_msselect(ms, 'field', field)
        self._do_msselect(ms, 'spw', spw)
        self._do_msselect(ms, 'time', timerange)
        self._do_msselect(ms, 'taql', antenna, 'ANTENNA1==')
        self._do_msselect(ms, 'scan', scan)
        self._do_msselect(ms, 'observation', observation)
        self._do_msselect(ms, 'taql', msselect)

    def _do_msselect(self, ms, name, value, value_prefix=None):
        if self.valid_selection:
            try:
                if value is not None:
                    if value_prefix is not None:
                        value = value_prefix + str(value)
                    ms.msselect({name:value})
            except:
                self.valid_selection = False

    def is_effective(self):
        return self.is_effective_id('spw', self.data_desc_id) and \
	    self.is_effective_id('antenna1', self.antenna_id) and \
	    self.is_effective_id('antenna2', self.antenna_id)

    def is_effective_id(self, key, value):
        res = True
        if len(self.selected_idx[key]) > 0:
            try:
                self.selected_idx[key].index(value)
            except:
                res = False
        return res

    def get_taql(self):
        elem = []
        self._append_taql(elem, 'DATA_DESC_ID', '==', self.data_desc_id)
        self._append_taql(elem, 'ANTENNA1', '==', self.antenna_id)
        self._append_taql(elem, 'ANTENNA2', '==', self.antenna_id)
        self._append_taql(elem, 'FIELD_ID', 'IN', 'field', True)
        taql_timerange = rhutil.select_by_timerange(self.ms_name, self.timerange) if self.timerange is not None else ''
        self._append_taql(elem, '', '', taql_timerange, True)
        self._append_taql(elem, 'SCAN_NUMBER', 'IN', 'scan', True)
        self._append_taql(elem, 'OBSERVATION_ID', 'IN', 'observationid', True)
        self._append_taql(elem, '', '', self.msselect, True)
        return ' && '.join(elem)

    def _append_taql(self, elem, keyword, operand, value, check=False):
        ope = operand.strip()
	if isinstance(value, str): value = value.strip()
        can_append = True
        if check:
            if ope == 'IN':
                can_append = len(self.selected_idx[value]) > 0
            else:
                can_append = (value is not None) and (str(value) != '')
        if can_append:
            mgn = ' ' if ope != '' else ''
            val = str(self.selected_idx[value]) if ope == 'IN' else str(value)
            res = '(' + keyword.strip().upper() + mgn + ope + mgn + val + ')'
	    elem.append(res)

def generate_query(vis, field=None, spw=None, timerange=None, antenna=None, scan=None, pol=None, observation=None, msselect=None):
    with opentable(os.path.join(vis, 'DATA_DESCRIPTION')) as tb:
        num_data_desc_id = tb.nrows()
        data_desc_id_by_spw = dict(((tb.getcell('SPECTRAL_WINDOW_ID', i), i) for i in xrange(num_data_desc_id)))

    with opentable(os.path.join(vis, 'ANTENNA')) as tb:
        num_antenna_id = tb.nrows()

    with openms(vis) as ms:
        for data_desc_id, antenna_id in itertools.product(xrange(num_data_desc_id), xrange(num_antenna_id)):
            gqh = GenerateQueryHelper(ms, data_desc_id_by_spw, data_desc_id, antenna_id)
	    gqh.do_msselect(ms, field, spw, timerange, antenna, scan, observation, msselect)
	    if gqh.valid_selection:
                gqh.selected_idx = ms.msselectedindices()
		if gqh.is_effective():
                    res = gqh.get_taql()
		    idx_channel = gqh.selected_idx['channel']
		    res_channel = str(idx_channel) if len(idx_channel) > 0 else ''
		    res_pol = pol if pol is not None else ''
		    yield res, res_channel, res_pol
	    ms.reset()

@contextlib.contextmanager
def opentable(vis):
    tb = gentools(['tb'])[0]
    tb.open(vis, nomodify=False)
    yield tb
    tb.close()

@contextlib.contextmanager
def openms(vis):
    ms = gentools(['ms'])[0]
    ms.open(vis)
    yield ms
    ms.close()
    
def optimize_thread_parameters(vis, data_desc_id, antenna_id):
    try:
        tb = gentools(['tb'])[0]
        tb.open(vis)
        query_str = 'DATA_DESC_ID==' + str(data_desc_id) + ' && ANTENNA1==' + str(antenna_id) + ' && ANTENNA2==' + str(antenna_id)
        subt = tb.query(query_str)
        num_rows = subt.nrows()
        data = subt.getcell('FLOAT_DATA', 0)
        num_pols = len(data)
        num_channels = len(data[0])
        data_size_per_record = num_pols * num_channels * (8 + 1) * 2 #dummy
        assert data_size_per_record > 0
    finally:
        tb.close()
        subt.close()

    num_cores = multiprocessing.cpu_count()
    num_threads = num_cores - 1 if (num_cores > 1) else 1
    assert num_threads > 0

    mem_size = 8*1024*1024*1024 #to be replaced with an appropriate function
    num_record = mem_size / num_threads / data_size_per_record
    if (num_record > num_rows): num_record = num_rows
    assert num_record > 0

    return num_record, num_threads

def readchunk(table, criteria, nrecord):
    tb = table.query(criteria)
    nrow = tb.nrows()
    rownumbers = tb.rownumbers()
    tb.close()
    nchunk = nrow / nrecord 
    for ichunk in xrange(nchunk):
        start = ichunk * nrecord
        end = start + nrecord
        chunk =  _readchunk(table, rownumbers[start:end])
        #print 'readchunk:',chunk
        yield chunk

    # residuals
    residual = nrow % nrecord
    if residual > 0:
        start = nrow - residual
        end = nrow
        chunk = _readchunk(table, rownumbers[start:end])
        #print 'readchunk:',chunk
        yield chunk
        
def _readchunk(table, rows):
    print '_readchunk: reading rows %s...'%(rows)
    return tuple((_readrow(table, irow) for irow in rows))

def _readrow(table, row):
    get = lambda col: table.getcell(col, row)
    return (row, get('FLOAT_DATA'), get('FLAG'), get('TIME_CENTROID'))
            
def reducechunk(chunk, context):
    return tuple((reducerecord(record) for record in chunk))

def reducerecord(record):
    print 'reducing row %s'%(record[0])
    data, flag, stats = reducedata(record[0], record[1], record[2], record[3])
    return (record[0], data, flag, record[3], stats)

def reducerecord2(record):
    data, mask = tosakura(record[1], record[2])
    data, mask = calibratedata(data, mask, record[3])
    mask = masknanorinf(data, mask)
    mask = maskedge(data, mask)
    data, mask = baselinedata(data, mask)
    mask = clipdata(data, mask)
    data = smoothdata(data, mask)
    stats = calcstats(data, mask)
    data, flag = tocasa(data, mask)
    yield (record[0], data, flag, record[3], stats)
    
def writechunk(table, results):
    put = lambda row, col, val: table.putcell(col, row, val)
    for record in results:
        row = int(record[0])
        data = record[1]
        flag = record[2]
        print 'writing result to table %s at row %s...'%(table.name(), row)
        put(row, 'FLOAT_DATA', data)
        put(row, 'FLAG', flag)

###
def reducedata(row, data, flag, timestamp):
    data[:] = float(row)
    print 'reducing row %s...'%(row)
    #mtpy.wait_for(5, 'row%s'%(row))
    print 'done reducing row %s...'%(row)
    return data, flag, {'statistics': data.real.mean()}

BASELINE_TYPEMAP = {'poly': libsakurapy.BASELINE_TYPE_POLYNOMIAL,
                    'chebyshev': libsakurapy.BASELINE_TYPE_CHEBYSHEV,
                    'cspline': libsakurapy.BASELINE_TYPE_CUBIC_SPLINE,
                    'sinusoid': libsakurapy.BASELINE_TYPE_SINUSOID}
CONVOLVE1D_TYPEMAP = {'hanning': libsakurapy.CONVOLVE1D_KERNEL_TYPE_HANNING,
                      'gaussian': libsakurapy.CONVOLVE1D_KERNEL_TYPE_GAUSSIAN,
                      'boxcar': libsakurapy.CONVOLVE1D_KERNEL_TYPE_BOXCAR}
CALIBRATION_TYPEMAP = {'nearest': libsakurapy.INTERPOLATION_METHOD_NEAREST,
                       'linear': libsakurapy.INTERPOLATION_METHOD_LINEAR,
                       'cspline': libsakurapy.INTERPOLATION_METHOD_SPLINE}

def sakura_typemap(typemap, key):
    try:
        return typemap[key.lower()]
    except KeyError, e:
        raise RuntimeError('Invalid type: %s'%(key))

def calibration_typemap(key):
    try:
        return CALIBRATION_TYPEMAP[key.lower()], -1
    except KeyError, e:
        if key.isdigit():
            return libsakurapy.INTERPOLATION_METHOD_POLYNOMIAL, int(key)
        else:
            raise RuntimeError('Invalid type: %s'%(key))
        

def data_desc_id_map(vis):
    with opentable(os.path.join(vis, 'DATA_DESCRIPTION')) as tb:
        ddidmap = dict(((tb.getcell('SPECTRAL_WINDOW_ID',i),i) for i in xrange(tb.nrows())))
    return ddidmap
    
    
def initcontext(vis, spw, antenna, gaintable, interp, spwmap,
                maskmode, thresh, avg_limit, edge, blmask,
                blfunc, order, npiece, applyfft, fftmethod,
                fftthresh, addwn, rejwn, clipthresh, clipniter,
                bloutput, blformat, clipminmax,
                kernel, kwidth, usefft, interpflag,
                statmask, stoutput, stformat):
    ssd.initialize_sakura()

    context_dict = {}
    
    # get nchan for each spw
    #with opentable(os.path.join(vis, 'DATA_DESCRIPTION')) as tb:
    #    ddidmap = dict(((tb.getcell('SPECTRAL_WINDOW_ID',i),i) for i in xrange(tb.nrows())))
    ddidmap = data_desc_id_map(vis)

    # spw selection to index
    selection = ms.msseltoindex(vis=vis, spw=spw, baseline='%s&&&'%(antenna))
    spwid_list = selection['spw']
    antennaid_list = selection['baselines'][:,0]
    
    # nchan
    with opentable(os.path.join(vis, 'SPECTRAL_WINDOW')) as tb:
        nchanmap = dict(((i,tb.getcell('NUM_CHAN',i)) for i in xrange(tb.nrows())))
    
    # create calibration context (base data for interpolation)
    sky_tables = _select_sky_tables(gaintable)
    tsys_tables = _select_tsys_tables(gaintable)

    ## def _tsysspw(spwmap, spwid):
    ##     for (k,v) in spwmap.items():
    ##         if spwid in v:
    ##             return k
    ##     return None

    for spwid in spwid_list:
        nchan = nchanmap[spwid]

        # create calibration context
        #tsysspw = _tsysspw(spwmap, spwid)
        tsysspw = spwmap[spwid] # interferometry style spwmap
        calibration_context = create_calibration_context(vis,
                                                         sky_tables,
                                                         tsys_tables,
                                                         spwid,
                                                         tsysspw,
                                                         antennaid_list,
                                                         interp)
        
        # create baseline context
        baseline_type = sakura_typemap(BASELINE_TYPEMAP, blfunc)
        baseline_context = libsakurapy.create_baseline_context(baseline_type,
                                                               order,
                                                               nchan)
        
        # create convolve1D context
        convolve1d_type = sakura_typemap(CONVOLVE1D_TYPEMAP, kernel)
        convolve1d_context = libsakurapy.create_convolve1D_context(nchan,
                                                                   convolve1d_type,
                                                                   kwidth,
                                                                   usefft)
        context_dict[spwid] = (calibration_context, baseline_context, convolve1d_context,)

    return context_dict

def create_calibration_context(vis, sky_tables, tsys_tables, spwid, tsysspw, antennaid_list, interp):
    context = {}
    # context must be prepared for each antenna
    for antennaid in antennaid_list:

        # collect sky data for given antenna id and spw id
        timestamp = None
        data = None
        for sky_table in sky_tables:
            ddidmap = data_desc_id_map(sky_table)
            if ddidmap.has_key(spwid):
                ddid = ddidmap[spwid]
                with opentable(sky_table) as tb:
                    datacol = colname(tb)
                    tsel = tb.query('DATA_DESC_ID==%s && ANTENNA1 == ANTENNA2 && ANTENNA1 == %s'%(ddid,antennaid))
                    if tsel.nrows() > 0:
                        if timestamp is None:
                            timestamp = tsel.getcol('TIME')
                        else:
                            timestamp = numpy.concatenate([timestamp, tsel.getcol('TIME')], axis=0)
                        if data is None:
                            data = tsel.getcol(datacol)
                        else:
                            data = numpy.concatenate([data, tsel.getcol(datacol)], axis=1)
                    tsel.close()
                    
        if timestamp is None or data is None:
            raise RuntimeError('Empty sky data for antenna %s spw %s. Cannot proceed.'%(antennaid, spwid))

        # sort by time
        sorted_time, sort_index = numpy.unique(timestamp, return_index=True)
        print 'sort_index', sort_index
        print 'data.shape', data.shape
        sorted_data = data.take(sort_index, axis=2)
        
        # create aligned buffer for sky
        # these are base data for interpolation so that it has to
        # encapsulate flattened array for each polarization
        time_sky = _casasakura.tosakura_double(sorted_time)[0][0]
        npol, nchan, nrow_sky = sorted_data.shape
        if datacol == 'DATA':
            func = _casasakura.tosakura_complex
        else:
            func = _casasakura.tosakura_float
        sky = tuple((func(sorted_data[i].flatten(order='F'))[0][0] for i in xrange(npol)))

        # collect Tsys data for given antenna id and spw id
        timestamp = None
        data = None
        for tsys_table in tsys_tables:
            with opentable(os.path.join(tsys_table, 'SYSCAL')) as tb:
                datacol = 'TSYS_SPECTRUM'
                if not datacol in tb.colnames():
                    datacol = 'TSYS'
                tsel = tb.query('ANTENNA_ID==%s && SPECTRAL_WINDOW_ID==%s'%(antennaid,tsysspw))
                if tsel.nrows() > 0:
                    if timestamp is None:
                        timestamp = tsel.getcol('TIME')
                    else:
                        timestamp = numpy.concatenate([timestamp, tsel.getcol('TIME')], axis=0)
                    if data is None:
                        data = tsel.getcol(datacol)
                    else:
                        data = numpy.concatenate([data, tsel.getcol(datacol)], axis=1)
                tsel.close()
        
        if timestamp is None or data is None:
            raise RuntimeError('Empty Tsys data for antenna %s spw %s. Cannot proceed.'%(antennaid, spwid))
                    
        # sort by time
        sorted_time, sort_index = numpy.unique(timestamp, return_index=True)
        sorted_data = data.take(sort_index, axis=2)

        # interpolate along spectral axis
        # frequency labels are taken from vis
        with opentable(os.path.join(vis, 'SPECTRAL_WINDOW')) as tb:
            freq = tb.getcell('CHAN_FREQ', spwid)
            freq_tsys = tb.getcell('CHAN_FREQ', tsysspw)

        # interpolation method
        if len(interp) > 0:
            interpolation_method, poly_order = calibration_typemap(interp.split(',')[-1])
        else:
            interpolation_method, poly_order = libsakurapy.INTERPOLATION_METHOD_LINEAR


        # create aligned buffer for interpolation
        def gen_interpolation():
            interpolated_freq = _casasakura.tosakura_double(freq)
            base_freq = _casasakura.tosakura_double(freq_tsys)
            nchan = len(interpolated_freq)
            npol, nchan_base, nrow = sorted_data.shape
            for ipol in xrange(npol):
                base_data = _casasakura.tosakura_float(sorted_data[ipol].flatten(order='F'))[0][0]
                interpolated_data = libsakurapy.new_aligned_buffer(libsakurapy.TYPE_FLOAT, (nchan * nrow,))
    
                # perform interpolation
                ## libsakurapy.interpolate_float_xaxis(interpolation_method,
                ##                                     poly_order,
                ##                                     nchan_base,
                ##                                     base_freq,
                ##                                     nrow,
                ##                                     base_data,
                ##                                     nchan,
                ##                                     interpolated_freq,
                ##                                     interpolated_data)
                yield interpolated_data

        # data for context
        time_tsys = _casasakura.tosakura_double(sorted_time)[0][0]
        tsys = tuple(gen_interpolation())
        nrow_tsys = sorted_data.shape[2]

        context[antennaid] = {'nchan': nchan,
                              'nrow_sky': nrow_sky,
                              'nrow_tsys': nrow_tsys,
                              'time_sky': time_sky,
                              'time_tsys': time_tsys,
                              'sky': sky,
                              'tsys': tsys}
    return context
            

def _select_sky_tables(gaintable):
    return list(_select_match(gaintable, 'sky'))

def _select_tsys_tables(gaintable):
    return list(_select_match(gaintable, 'tsys'))

def _select_match(gaintable, tabletype):
    pattern = '_%s(.ms)?$'%(tabletype.lower())
    print pattern
    for caltable in gaintable:
        print caltable
        if re.search(pattern, caltable):
            yield caltable
                

def colname(tb):
    colnames = tb.colnames()
    if 'FLOAT_DATA' in colnames:
        return 'FLOAT_DATA'
    else:
        return 'DATA'
