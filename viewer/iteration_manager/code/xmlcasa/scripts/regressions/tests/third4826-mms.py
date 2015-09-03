import sys
import os
import string
from locatescript import locatescript
import inspect

a=inspect.stack()
stacklevel=0
for k in range(len(a)):
    if (string.find(a[k][1], 'ipython console') > 0):
        stacklevel=k
        break
gl=sys._getframe(stacklevel).f_globals

def description():
    return "Based on ngc4826_tutorial_regression.py"

def run():
    #####locate the regression script
    lepath=locatescript('ngc4826-mms_tutorial_regression.py')
    print 'Script used is ',lepath
    gl['regstate']=True
    execfile(lepath, gl)
    print 'regstate =', gl['regstate']
    if not gl['regstate']:
        raise Exception, 'regstate = False'

###return the images that will be templated and compared in future runs
    return ['ngc4826.tutorial.16apr98.src.clean.image']

def data():
    ### return the data files that is needed by the regression script
    return ['fitsfiles']

def doCopy():
    return [0]