# simulation tasks regression
#
#  multiple pointings (mosaics)
#  multiple interferometric and a total power measurements
#  no noise

import os, time, string, sys, inspect
from locatescript import locatescript

a=inspect.stack()
stacklevel=0
for k in range(len(a)):
    if (string.find(a[k][1], 'ipython console') > 0):
        stacklevel=k
        break
gl=sys._getframe(stacklevel).f_globals


# Short description
def description():
    return "Simulates ALMA-12m + ACA-7m + ALMA single-dish mosaics from a model image. No noise, imaged."


def run():
    #####locate the regression script
    lepath=locatescript('m51_3sim_regression.py')
    print 'Script used is ',lepath
    gl['regstate']=True
    execfile(lepath, gl)
    print 'regstate =', gl['regstate']
    if not gl['regstate']:
        raise Exception, 'regstate = False'
###return the images that will be templated and compared in future runs
    return ['m51c/m51c.aca.tp.sd.ms','m51c/m51c.aca.i.ms','m51c/m51c.alma_0.5arcsec.ms','m51c/m51c.alma_0.5arcsec.image']


def data():
    ### return the data files that is needed by the regression script
    return []




