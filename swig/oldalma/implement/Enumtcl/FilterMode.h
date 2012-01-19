/*
 *
 * /////////////////////////////////////////////////////////////////
 * // WARNING!  DO NOT MODIFY THIS FILE!                          //
 * //  ---------------------------------------------------------  //
 * // | This is generated code using a C++ template function!   | //
 * // ! Do not modify this file.                                | //
 * // | Any changes will be lost when the file is re-generated. | //
 * //  ---------------------------------------------------------  //
 * /////////////////////////////////////////////////////////////////
 *
 */


#if     !defined(_FILTERMODE_H)

#include <CFilterMode.h>
#define _FILTERMODE_H
#endif 

#if     !defined(_FILTERMODE_HH)

#include "Enum.hpp"

using namespace FilterModeMod;

template<>
 struct enum_set_traits<FilterMode> : public enum_set_traiter<FilterMode,3,FilterModeMod::FILTER_TFB> {};

template<>
class enum_map_traits<FilterMode,void> : public enum_map_traiter<FilterMode,void> {
public:
  static bool   init_;
  static string typeName_;
  static string enumerationDesc_;
  static string order_;
  static string xsdBaseType_;
  static bool   init(){
    EnumPar<void> ep;
    m_.insert(pair<FilterMode,EnumPar<void> >
     (FilterModeMod::FILTER_NA,ep((int)FilterModeMod::FILTER_NA,"FILTER_NA"," Not Applicable (2 antenna prototype). The Tunable Filter Banks are not implemented")));
    m_.insert(pair<FilterMode,EnumPar<void> >
     (FilterModeMod::FILTER_TDM,ep((int)FilterModeMod::FILTER_TDM,"FILTER_TDM","Time Division Mode. In this mode the Tunable Filter banks are bypassed")));
    m_.insert(pair<FilterMode,EnumPar<void> >
     (FilterModeMod::FILTER_TFB,ep((int)FilterModeMod::FILTER_TFB,"FILTER_TFB","The Tunable Filter Bank is implemented and used")));
    return true;
  }
  static map<FilterMode,EnumPar<void> > m_;
};
#define _FILTERMODE_HH
#endif
