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


#if     !defined(_HOLOGRAPHYCHANNELTYPE_H)

#include <CHolographyChannelType.h>
#define _HOLOGRAPHYCHANNELTYPE_H
#endif 

#if     !defined(_HOLOGRAPHYCHANNELTYPE_HH)

#include "Enum.hpp"

using namespace HolographyChannelTypeMod;

template<>
 struct enum_set_traits<HolographyChannelType> : public enum_set_traiter<HolographyChannelType,6,HolographyChannelTypeMod::S2> {};

template<>
class enum_map_traits<HolographyChannelType,void> : public enum_map_traiter<HolographyChannelType,void> {
public:
  static bool   init_;
  static string typeName_;
  static string enumerationDesc_;
  static string order_;
  static string xsdBaseType_;
  static bool   init(){
    EnumPar<void> ep;
    m_.insert(pair<HolographyChannelType,EnumPar<void> >
     (HolographyChannelTypeMod::Q2,ep((int)HolographyChannelTypeMod::Q2,"Q2","Quadrature channel auto-product")));
    m_.insert(pair<HolographyChannelType,EnumPar<void> >
     (HolographyChannelTypeMod::QR,ep((int)HolographyChannelTypeMod::QR,"QR","Quadrature channel times Reference channel cross-product")));
    m_.insert(pair<HolographyChannelType,EnumPar<void> >
     (HolographyChannelTypeMod::QS,ep((int)HolographyChannelTypeMod::QS,"QS","Quadrature channel times Signal channel cross-product")));
    m_.insert(pair<HolographyChannelType,EnumPar<void> >
     (HolographyChannelTypeMod::R2,ep((int)HolographyChannelTypeMod::R2,"R2","Reference channel auto-product")));
    m_.insert(pair<HolographyChannelType,EnumPar<void> >
     (HolographyChannelTypeMod::RS,ep((int)HolographyChannelTypeMod::RS,"RS","Reference channel times Signal channel cross-product")));
    m_.insert(pair<HolographyChannelType,EnumPar<void> >
     (HolographyChannelTypeMod::S2,ep((int)HolographyChannelTypeMod::S2,"S2","Signal channel auto-product")));
    return true;
  }
  static map<HolographyChannelType,EnumPar<void> > m_;
};
#define _HOLOGRAPHYCHANNELTYPE_HH
#endif
