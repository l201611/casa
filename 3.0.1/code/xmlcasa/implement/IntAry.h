/*
 * This file was automatically generated by 
 * <http://ccmtools.sourceforge.net/>
 * DO NOT EDIT !
 */

// ===========================================================================
// struct definition for IntAry
// ===========================================================================

#ifndef __STRUCT__CCM_Local_casac_IntAry__H__
#define __STRUCT__CCM_Local_casac_IntAry__H__

#include<string>
#include <vector>

namespace casac {

struct IntAry
{
  IntAry( ) { }
  IntAry(std::vector<int> arg0, std::vector<int> arg1) : value(arg0), shape(arg1) { }
  std::vector<int> value;
  std::vector<int> shape;

};

} // / namespace casac

#endif // __STRUCT__CCM_Local_casac_IntAry__H__

