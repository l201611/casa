import sys
import os
import string
from locatescript import copydata
from locatescript import locatescript
from casa_stack_manip import stack_frame_find

gl=stack_frame_find( )

# Short description
def description():
    return "Test of a complete ALMA SV data analysis (2 ASDMs from Sept 2011)"

pass_on = { "asdm_dataset_name" : "uid___A002_X2a5c2f_X54",
            "asdm_dataset2_name" : "uid___A002_X2a5c2f_X220",
            "tsys_table" : 'cal-tsys_X54.fdm',
            "tsys_table2" : 'cal-tsys_X220.fdm',
            "mask1" : 'M100cont-orig.mask',
            "mask2" : 'M100line-orig.mask',
            "mask3" : 'test-M100line-orig.mask'
            }


def data():
    ### return the data files that are needed by the regression script
    return [pass_on["asdm_dataset_name"],
            pass_on["asdm_dataset2_name"],
            pass_on["tsys_table"],
            pass_on["tsys_table2"],
            pass_on["mask1"],
            pass_on["mask2"],
            pass_on["mask3"]
            ]

def doCopy():
    ### return a list of the same length as that returned by data()
    ###   containing 0 if the corresponding file should be just linked
    ###   (i.e. is read-only)
    ###   or 1 if the corresponding file should be really copied to
    ###   the work directory

    return [0,0,1,1,1,1,1]

def run( fetch=False ):

    #####fetch data
    if fetch:
        for f in data( ):
            copydata( f, os.getcwd( ) )
    
    #####locate the regression script
    try: 
        lepath=locatescript('alma-m100-analysis-regression.py')
        print 'Script used is ',lepath
        execfile(lepath, gl, pass_on)
    except:
        print 'execution failed: ', sys.exc_info()
        raise
###return the images that will be templated and compared in future runs
    return []
