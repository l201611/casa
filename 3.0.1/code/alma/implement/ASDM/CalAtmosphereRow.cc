
/*
 * ALMA - Atacama Large Millimeter Array
 * (c) European Southern Observatory, 2002
 * (c) Associated Universities Inc., 2002
 * Copyright by ESO (in the framework of the ALMA collaboration),
 * Copyright by AUI (in the framework of the ALMA collaboration),
 * All rights reserved.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307  USA
 *
 * Warning!
 *  -------------------------------------------------------------------- 
 * | This is generated code!  Do not modify this file.                  |
 * | If you do, all changes will be lost when the file is re-generated. |
 *  --------------------------------------------------------------------
 *
 * File CalAtmosphereRow.cpp
 */
 
#include <vector>
using std::vector;

#include <set>
using std::set;

#include <ASDM.h>
#include <CalAtmosphereRow.h>
#include <CalAtmosphereTable.h>

#include <CalReductionTable.h>
#include <CalReductionRow.h>

#include <CalDataTable.h>
#include <CalDataRow.h>
	

using asdm::ASDM;
using asdm::CalAtmosphereRow;
using asdm::CalAtmosphereTable;

using asdm::CalReductionTable;
using asdm::CalReductionRow;

using asdm::CalDataTable;
using asdm::CalDataRow;


#include <Parser.h>
using asdm::Parser;

#include <EnumerationParser.h>
 
#include <InvalidArgumentException.h>
using asdm::InvalidArgumentException;

namespace asdm {
	CalAtmosphereRow::~CalAtmosphereRow() {
	}

	/**
	 * Return the table to which this row belongs.
	 */
	CalAtmosphereTable &CalAtmosphereRow::getTable() const {
		return table;
	}
	
	void CalAtmosphereRow::isAdded() {
		hasBeenAdded = true;
	}
	
	
#ifndef WITHOUT_ACS
	/**
	 * Return this row in the form of an IDL struct.
	 * @return The values of this row as a CalAtmosphereRowIDL struct.
	 */
	CalAtmosphereRowIDL *CalAtmosphereRow::toIDL() const {
		CalAtmosphereRowIDL *x = new CalAtmosphereRowIDL ();
		
		// Fill the IDL structure.
	
		
	
  		
		
		
			
				
		x->receiverBand = receiverBand;
 				
 			
		
	

	
  		
		
		
			
				
		x->antennaName = CORBA::string_dup(antennaName.c_str());
				
 			
		
	

	
  		
		
		
			
		x->startValidTime = startValidTime.toIDLArrayTime();
			
		
	

	
  		
		
		
			
		x->endValidTime = endValidTime.toIDLArrayTime();
			
		
	

	
  		
		
		
			
				
		x->numFreq = numFreq;
 				
 			
		
	

	
  		
		
		
			
				
		x->numLoad = numLoad;
 				
 			
		
	

	
  		
		
		
			
				
		x->numReceptor = numReceptor;
 				
 			
		
	

	
  		
		
		
			
		x->forwardEffSpectrum.length(forwardEffSpectrum.size());
		for (unsigned int i = 0; i < forwardEffSpectrum.size(); i++) {
			x->forwardEffSpectrum[i].length(forwardEffSpectrum.at(i).size());			 		
		}
		
		for (unsigned int i = 0; i < forwardEffSpectrum.size() ; i++)
			for (unsigned int j = 0; j < forwardEffSpectrum.at(i).size(); j++)
					
						
				x->forwardEffSpectrum[i][j] = forwardEffSpectrum.at(i).at(j);
		 				
			 						
		
			
		
	

	
  		
		
		
			
		x->frequencyRange.length(frequencyRange.size());
		for (unsigned int i = 0; i < frequencyRange.size(); ++i) {
			
			x->frequencyRange[i] = frequencyRange.at(i).toIDLFrequency();
			
	 	}
			
		
	

	
  		
		
		
			
		x->groundPressure = groundPressure.toIDLPressure();
			
		
	

	
  		
		
		
			
		x->groundRelHumidity = groundRelHumidity.toIDLHumidity();
			
		
	

	
  		
		
		
			
		x->frequencySpectrum.length(frequencySpectrum.size());
		for (unsigned int i = 0; i < frequencySpectrum.size(); ++i) {
			
			x->frequencySpectrum[i] = frequencySpectrum.at(i).toIDLFrequency();
			
	 	}
			
		
	

	
  		
		
		
			
		x->groundTemperature = groundTemperature.toIDLTemperature();
			
		
	

	
  		
		
		
			
		x->polarizationTypes.length(polarizationTypes.size());
		for (unsigned int i = 0; i < polarizationTypes.size(); ++i) {
			
				
			x->polarizationTypes[i] = polarizationTypes.at(i);
	 			
	 		
	 	}
			
		
	

	
  		
		
		
			
		x->powerSkySpectrum.length(powerSkySpectrum.size());
		for (unsigned int i = 0; i < powerSkySpectrum.size(); i++) {
			x->powerSkySpectrum[i].length(powerSkySpectrum.at(i).size());			 		
		}
		
		for (unsigned int i = 0; i < powerSkySpectrum.size() ; i++)
			for (unsigned int j = 0; j < powerSkySpectrum.at(i).size(); j++)
					
						
				x->powerSkySpectrum[i][j] = powerSkySpectrum.at(i).at(j);
		 				
			 						
		
			
		
	

	
  		
		
		
			
		x->powerLoadSpectrum.length(powerLoadSpectrum.size());
		for (unsigned int i = 0; i < powerLoadSpectrum.size(); i++) {
			x->powerLoadSpectrum[i].length(powerLoadSpectrum.at(i).size());
			for (unsigned int j = 0; j < powerLoadSpectrum.at(i).size(); j++) {
				x->powerLoadSpectrum[i][j].length(powerLoadSpectrum.at(i).at(j).size());
			}					 		
		}
		
		for (unsigned int i = 0; i < powerLoadSpectrum.size() ; i++)
			for (unsigned int j = 0; j < powerLoadSpectrum.at(i).size(); j++)
				for (unsigned int k = 0; k < powerLoadSpectrum.at(i).at(j).size(); k++)
					
						
					x->powerLoadSpectrum[i][j][k] = powerLoadSpectrum.at(i).at(j).at(k);
		 				
			 									
			
		
	

	
  		
		
		
			
				
		x->syscalType = syscalType;
 				
 			
		
	

	
  		
		
		
			
		x->tAtmSpectrum.length(tAtmSpectrum.size());
		for (unsigned int i = 0; i < tAtmSpectrum.size(); i++) {
			x->tAtmSpectrum[i].length(tAtmSpectrum.at(i).size());			 		
		}
		
		for (unsigned int i = 0; i < tAtmSpectrum.size() ; i++)
			for (unsigned int j = 0; j < tAtmSpectrum.at(i).size(); j++)
					
				x->tAtmSpectrum[i][j]= tAtmSpectrum.at(i).at(j).toIDLTemperature();
									
		
			
		
	

	
  		
		
		
			
		x->tRecSpectrum.length(tRecSpectrum.size());
		for (unsigned int i = 0; i < tRecSpectrum.size(); i++) {
			x->tRecSpectrum[i].length(tRecSpectrum.at(i).size());			 		
		}
		
		for (unsigned int i = 0; i < tRecSpectrum.size() ; i++)
			for (unsigned int j = 0; j < tRecSpectrum.at(i).size(); j++)
					
				x->tRecSpectrum[i][j]= tRecSpectrum.at(i).at(j).toIDLTemperature();
									
		
			
		
	

	
  		
		
		
			
		x->tSysSpectrum.length(tSysSpectrum.size());
		for (unsigned int i = 0; i < tSysSpectrum.size(); i++) {
			x->tSysSpectrum[i].length(tSysSpectrum.at(i).size());			 		
		}
		
		for (unsigned int i = 0; i < tSysSpectrum.size() ; i++)
			for (unsigned int j = 0; j < tSysSpectrum.at(i).size(); j++)
					
				x->tSysSpectrum[i][j]= tSysSpectrum.at(i).at(j).toIDLTemperature();
									
		
			
		
	

	
  		
		
		
			
		x->tauSpectrum.length(tauSpectrum.size());
		for (unsigned int i = 0; i < tauSpectrum.size(); i++) {
			x->tauSpectrum[i].length(tauSpectrum.at(i).size());			 		
		}
		
		for (unsigned int i = 0; i < tauSpectrum.size() ; i++)
			for (unsigned int j = 0; j < tauSpectrum.at(i).size(); j++)
					
						
				x->tauSpectrum[i][j] = tauSpectrum.at(i).at(j);
		 				
			 						
		
			
		
	

	
  		
		
		
			
		x->tAtm.length(tAtm.size());
		for (unsigned int i = 0; i < tAtm.size(); ++i) {
			
			x->tAtm[i] = tAtm.at(i).toIDLTemperature();
			
	 	}
			
		
	

	
  		
		
		
			
		x->tRec.length(tRec.size());
		for (unsigned int i = 0; i < tRec.size(); ++i) {
			
			x->tRec[i] = tRec.at(i).toIDLTemperature();
			
	 	}
			
		
	

	
  		
		
		
			
		x->tSys.length(tSys.size());
		for (unsigned int i = 0; i < tSys.size(); ++i) {
			
			x->tSys[i] = tSys.at(i).toIDLTemperature();
			
	 	}
			
		
	

	
  		
		
		
			
		x->tau.length(tau.size());
		for (unsigned int i = 0; i < tau.size(); ++i) {
			
				
			x->tau[i] = tau.at(i);
	 			
	 		
	 	}
			
		
	

	
  		
		
		
			
		x->water.length(water.size());
		for (unsigned int i = 0; i < water.size(); ++i) {
			
			x->water[i] = water.at(i).toIDLLength();
			
	 	}
			
		
	

	
  		
		
		
			
		x->waterError.length(waterError.size());
		for (unsigned int i = 0; i < waterError.size(); ++i) {
			
			x->waterError[i] = waterError.at(i).toIDLLength();
			
	 	}
			
		
	

	
  		
		
		x->alphaSpectrumExists = alphaSpectrumExists;
		
		
			
		x->alphaSpectrum.length(alphaSpectrum.size());
		for (unsigned int i = 0; i < alphaSpectrum.size(); i++) {
			x->alphaSpectrum[i].length(alphaSpectrum.at(i).size());			 		
		}
		
		for (unsigned int i = 0; i < alphaSpectrum.size() ; i++)
			for (unsigned int j = 0; j < alphaSpectrum.at(i).size(); j++)
					
						
				x->alphaSpectrum[i][j] = alphaSpectrum.at(i).at(j);
		 				
			 						
		
			
		
	

	
  		
		
		x->forwardEfficiencyExists = forwardEfficiencyExists;
		
		
			
		x->forwardEfficiency.length(forwardEfficiency.size());
		for (unsigned int i = 0; i < forwardEfficiency.size(); ++i) {
			
				
			x->forwardEfficiency[i] = forwardEfficiency.at(i);
	 			
	 		
	 	}
			
		
	

	
  		
		
		x->forwardEfficiencyErrorExists = forwardEfficiencyErrorExists;
		
		
			
		x->forwardEfficiencyError.length(forwardEfficiencyError.size());
		for (unsigned int i = 0; i < forwardEfficiencyError.size(); ++i) {
			
				
			x->forwardEfficiencyError[i] = forwardEfficiencyError.at(i);
	 			
	 		
	 	}
			
		
	

	
  		
		
		x->sbGainExists = sbGainExists;
		
		
			
		x->sbGain.length(sbGain.size());
		for (unsigned int i = 0; i < sbGain.size(); ++i) {
			
				
			x->sbGain[i] = sbGain.at(i);
	 			
	 		
	 	}
			
		
	

	
  		
		
		x->sbGainErrorExists = sbGainErrorExists;
		
		
			
		x->sbGainError.length(sbGainError.size());
		for (unsigned int i = 0; i < sbGainError.size(); ++i) {
			
				
			x->sbGainError[i] = sbGainError.at(i);
	 			
	 		
	 	}
			
		
	

	
  		
		
		x->sbGainSpectrumExists = sbGainSpectrumExists;
		
		
			
		x->sbGainSpectrum.length(sbGainSpectrum.size());
		for (unsigned int i = 0; i < sbGainSpectrum.size(); i++) {
			x->sbGainSpectrum[i].length(sbGainSpectrum.at(i).size());			 		
		}
		
		for (unsigned int i = 0; i < sbGainSpectrum.size() ; i++)
			for (unsigned int j = 0; j < sbGainSpectrum.at(i).size(); j++)
					
						
				x->sbGainSpectrum[i][j] = sbGainSpectrum.at(i).at(j);
		 				
			 						
		
			
		
	

	
	
		
	
  	
 		
		
	 	
			
		x->calDataId = calDataId.toIDLTag();
			
	 	 		
  	

	
  	
 		
		
	 	
			
		x->calReductionId = calReductionId.toIDLTag();
			
	 	 		
  	

	
		
	

	

		
		return x;
	
	}
#endif
	

#ifndef WITHOUT_ACS
	/**
	 * Fill the values of this row from the IDL struct CalAtmosphereRowIDL.
	 * @param x The IDL struct containing the values used to fill this row.
	 */
	void CalAtmosphereRow::setFromIDL (CalAtmosphereRowIDL x){
		try {
		// Fill the values from x.
	
		
	
		
		
			
		setReceiverBand(x.receiverBand);
  			
 		
		
	

	
		
		
			
		setAntennaName(string (x.antennaName));
			
 		
		
	

	
		
		
			
		setStartValidTime(ArrayTime (x.startValidTime));
			
 		
		
	

	
		
		
			
		setEndValidTime(ArrayTime (x.endValidTime));
			
 		
		
	

	
		
		
			
		setNumFreq(x.numFreq);
  			
 		
		
	

	
		
		
			
		setNumLoad(x.numLoad);
  			
 		
		
	

	
		
		
			
		setNumReceptor(x.numReceptor);
  			
 		
		
	

	
		
		
			
		forwardEffSpectrum .clear();
		vector<float> v_aux_forwardEffSpectrum;
		for (unsigned int i = 0; i < x.forwardEffSpectrum.length(); ++i) {
			v_aux_forwardEffSpectrum.clear();
			for (unsigned int j = 0; j < x.forwardEffSpectrum[0].length(); ++j) {
				
				v_aux_forwardEffSpectrum.push_back(x.forwardEffSpectrum[i][j]);
	  			
  			}
  			forwardEffSpectrum.push_back(v_aux_forwardEffSpectrum);			
		}
			
  		
		
	

	
		
		
			
		frequencyRange .clear();
		for (unsigned int i = 0; i <x.frequencyRange.length(); ++i) {
			
			frequencyRange.push_back(Frequency (x.frequencyRange[i]));
			
		}
			
  		
		
	

	
		
		
			
		setGroundPressure(Pressure (x.groundPressure));
			
 		
		
	

	
		
		
			
		setGroundRelHumidity(Humidity (x.groundRelHumidity));
			
 		
		
	

	
		
		
			
		frequencySpectrum .clear();
		for (unsigned int i = 0; i <x.frequencySpectrum.length(); ++i) {
			
			frequencySpectrum.push_back(Frequency (x.frequencySpectrum[i]));
			
		}
			
  		
		
	

	
		
		
			
		setGroundTemperature(Temperature (x.groundTemperature));
			
 		
		
	

	
		
		
			
		polarizationTypes .clear();
		for (unsigned int i = 0; i <x.polarizationTypes.length(); ++i) {
			
			polarizationTypes.push_back(x.polarizationTypes[i]);
  			
		}
			
  		
		
	

	
		
		
			
		powerSkySpectrum .clear();
		vector<float> v_aux_powerSkySpectrum;
		for (unsigned int i = 0; i < x.powerSkySpectrum.length(); ++i) {
			v_aux_powerSkySpectrum.clear();
			for (unsigned int j = 0; j < x.powerSkySpectrum[0].length(); ++j) {
				
				v_aux_powerSkySpectrum.push_back(x.powerSkySpectrum[i][j]);
	  			
  			}
  			powerSkySpectrum.push_back(v_aux_powerSkySpectrum);			
		}
			
  		
		
	

	
		
		
			
		powerLoadSpectrum .clear();
		vector< vector<float> > vv_aux_powerLoadSpectrum;
		vector<float> v_aux_powerLoadSpectrum;
		
		for (unsigned int i = 0; i < x.powerLoadSpectrum.length(); ++i) {
			vv_aux_powerLoadSpectrum.clear();
			for (unsigned int j = 0; j < x.powerLoadSpectrum[0].length(); ++j) {
				v_aux_powerLoadSpectrum.clear();
				for (unsigned int k = 0; k < x.powerLoadSpectrum[0][0].length(); ++k) {
					
					v_aux_powerLoadSpectrum.push_back(x.powerLoadSpectrum[i][j][k]);
		  			
		  		}
		  		vv_aux_powerLoadSpectrum.push_back(v_aux_powerLoadSpectrum);
  			}
  			powerLoadSpectrum.push_back(vv_aux_powerLoadSpectrum);
		}
			
  		
		
	

	
		
		
			
		setSyscalType(x.syscalType);
  			
 		
		
	

	
		
		
			
		tAtmSpectrum .clear();
		vector<Temperature> v_aux_tAtmSpectrum;
		for (unsigned int i = 0; i < x.tAtmSpectrum.length(); ++i) {
			v_aux_tAtmSpectrum.clear();
			for (unsigned int j = 0; j < x.tAtmSpectrum[0].length(); ++j) {
				
				v_aux_tAtmSpectrum.push_back(Temperature (x.tAtmSpectrum[i][j]));
				
  			}
  			tAtmSpectrum.push_back(v_aux_tAtmSpectrum);			
		}
			
  		
		
	

	
		
		
			
		tRecSpectrum .clear();
		vector<Temperature> v_aux_tRecSpectrum;
		for (unsigned int i = 0; i < x.tRecSpectrum.length(); ++i) {
			v_aux_tRecSpectrum.clear();
			for (unsigned int j = 0; j < x.tRecSpectrum[0].length(); ++j) {
				
				v_aux_tRecSpectrum.push_back(Temperature (x.tRecSpectrum[i][j]));
				
  			}
  			tRecSpectrum.push_back(v_aux_tRecSpectrum);			
		}
			
  		
		
	

	
		
		
			
		tSysSpectrum .clear();
		vector<Temperature> v_aux_tSysSpectrum;
		for (unsigned int i = 0; i < x.tSysSpectrum.length(); ++i) {
			v_aux_tSysSpectrum.clear();
			for (unsigned int j = 0; j < x.tSysSpectrum[0].length(); ++j) {
				
				v_aux_tSysSpectrum.push_back(Temperature (x.tSysSpectrum[i][j]));
				
  			}
  			tSysSpectrum.push_back(v_aux_tSysSpectrum);			
		}
			
  		
		
	

	
		
		
			
		tauSpectrum .clear();
		vector<float> v_aux_tauSpectrum;
		for (unsigned int i = 0; i < x.tauSpectrum.length(); ++i) {
			v_aux_tauSpectrum.clear();
			for (unsigned int j = 0; j < x.tauSpectrum[0].length(); ++j) {
				
				v_aux_tauSpectrum.push_back(x.tauSpectrum[i][j]);
	  			
  			}
  			tauSpectrum.push_back(v_aux_tauSpectrum);			
		}
			
  		
		
	

	
		
		
			
		tAtm .clear();
		for (unsigned int i = 0; i <x.tAtm.length(); ++i) {
			
			tAtm.push_back(Temperature (x.tAtm[i]));
			
		}
			
  		
		
	

	
		
		
			
		tRec .clear();
		for (unsigned int i = 0; i <x.tRec.length(); ++i) {
			
			tRec.push_back(Temperature (x.tRec[i]));
			
		}
			
  		
		
	

	
		
		
			
		tSys .clear();
		for (unsigned int i = 0; i <x.tSys.length(); ++i) {
			
			tSys.push_back(Temperature (x.tSys[i]));
			
		}
			
  		
		
	

	
		
		
			
		tau .clear();
		for (unsigned int i = 0; i <x.tau.length(); ++i) {
			
			tau.push_back(x.tau[i]);
  			
		}
			
  		
		
	

	
		
		
			
		water .clear();
		for (unsigned int i = 0; i <x.water.length(); ++i) {
			
			water.push_back(Length (x.water[i]));
			
		}
			
  		
		
	

	
		
		
			
		waterError .clear();
		for (unsigned int i = 0; i <x.waterError.length(); ++i) {
			
			waterError.push_back(Length (x.waterError[i]));
			
		}
			
  		
		
	

	
		
		alphaSpectrumExists = x.alphaSpectrumExists;
		if (x.alphaSpectrumExists) {
		
		
			
		alphaSpectrum .clear();
		vector<float> v_aux_alphaSpectrum;
		for (unsigned int i = 0; i < x.alphaSpectrum.length(); ++i) {
			v_aux_alphaSpectrum.clear();
			for (unsigned int j = 0; j < x.alphaSpectrum[0].length(); ++j) {
				
				v_aux_alphaSpectrum.push_back(x.alphaSpectrum[i][j]);
	  			
  			}
  			alphaSpectrum.push_back(v_aux_alphaSpectrum);			
		}
			
  		
		
		}
		
	

	
		
		forwardEfficiencyExists = x.forwardEfficiencyExists;
		if (x.forwardEfficiencyExists) {
		
		
			
		forwardEfficiency .clear();
		for (unsigned int i = 0; i <x.forwardEfficiency.length(); ++i) {
			
			forwardEfficiency.push_back(x.forwardEfficiency[i]);
  			
		}
			
  		
		
		}
		
	

	
		
		forwardEfficiencyErrorExists = x.forwardEfficiencyErrorExists;
		if (x.forwardEfficiencyErrorExists) {
		
		
			
		forwardEfficiencyError .clear();
		for (unsigned int i = 0; i <x.forwardEfficiencyError.length(); ++i) {
			
			forwardEfficiencyError.push_back(x.forwardEfficiencyError[i]);
  			
		}
			
  		
		
		}
		
	

	
		
		sbGainExists = x.sbGainExists;
		if (x.sbGainExists) {
		
		
			
		sbGain .clear();
		for (unsigned int i = 0; i <x.sbGain.length(); ++i) {
			
			sbGain.push_back(x.sbGain[i]);
  			
		}
			
  		
		
		}
		
	

	
		
		sbGainErrorExists = x.sbGainErrorExists;
		if (x.sbGainErrorExists) {
		
		
			
		sbGainError .clear();
		for (unsigned int i = 0; i <x.sbGainError.length(); ++i) {
			
			sbGainError.push_back(x.sbGainError[i]);
  			
		}
			
  		
		
		}
		
	

	
		
		sbGainSpectrumExists = x.sbGainSpectrumExists;
		if (x.sbGainSpectrumExists) {
		
		
			
		sbGainSpectrum .clear();
		vector<float> v_aux_sbGainSpectrum;
		for (unsigned int i = 0; i < x.sbGainSpectrum.length(); ++i) {
			v_aux_sbGainSpectrum.clear();
			for (unsigned int j = 0; j < x.sbGainSpectrum[0].length(); ++j) {
				
				v_aux_sbGainSpectrum.push_back(x.sbGainSpectrum[i][j]);
	  			
  			}
  			sbGainSpectrum.push_back(v_aux_sbGainSpectrum);			
		}
			
  		
		
		}
		
	

	
	
		
	
		
		
			
		setCalDataId(Tag (x.calDataId));
			
 		
		
	

	
		
		
			
		setCalReductionId(Tag (x.calReductionId));
			
 		
		
	

	
		
	

	

		} catch (IllegalAccessException err) {
			throw ConversionException (err.getMessage(),"CalAtmosphere");
		}
	}
#endif
	
	/**
	 * Return this row in the form of an XML string.
	 * @return The values of this row as an XML string.
	 */
	string CalAtmosphereRow::toXML() const {
		string buf;
		buf.append("<row> \n");
		
	
		
  	
 		
		
			buf.append(EnumerationParser::toXML("receiverBand", receiverBand));
		
		
	

  	
 		
		
		Parser::toXML(antennaName, "antennaName", buf);
		
		
	

  	
 		
		
		Parser::toXML(startValidTime, "startValidTime", buf);
		
		
	

  	
 		
		
		Parser::toXML(endValidTime, "endValidTime", buf);
		
		
	

  	
 		
		
		Parser::toXML(numFreq, "numFreq", buf);
		
		
	

  	
 		
		
		Parser::toXML(numLoad, "numLoad", buf);
		
		
	

  	
 		
		
		Parser::toXML(numReceptor, "numReceptor", buf);
		
		
	

  	
 		
		
		Parser::toXML(forwardEffSpectrum, "forwardEffSpectrum", buf);
		
		
	

  	
 		
		
		Parser::toXML(frequencyRange, "frequencyRange", buf);
		
		
	

  	
 		
		
		Parser::toXML(groundPressure, "groundPressure", buf);
		
		
	

  	
 		
		
		Parser::toXML(groundRelHumidity, "groundRelHumidity", buf);
		
		
	

  	
 		
		
		Parser::toXML(frequencySpectrum, "frequencySpectrum", buf);
		
		
	

  	
 		
		
		Parser::toXML(groundTemperature, "groundTemperature", buf);
		
		
	

  	
 		
		
			buf.append(EnumerationParser::toXML("polarizationTypes", polarizationTypes));
		
		
	

  	
 		
		
		Parser::toXML(powerSkySpectrum, "powerSkySpectrum", buf);
		
		
	

  	
 		
		
		Parser::toXML(powerLoadSpectrum, "powerLoadSpectrum", buf);
		
		
	

  	
 		
		
			buf.append(EnumerationParser::toXML("syscalType", syscalType));
		
		
	

  	
 		
		
		Parser::toXML(tAtmSpectrum, "tAtmSpectrum", buf);
		
		
	

  	
 		
		
		Parser::toXML(tRecSpectrum, "tRecSpectrum", buf);
		
		
	

  	
 		
		
		Parser::toXML(tSysSpectrum, "tSysSpectrum", buf);
		
		
	

  	
 		
		
		Parser::toXML(tauSpectrum, "tauSpectrum", buf);
		
		
	

  	
 		
		
		Parser::toXML(tAtm, "tAtm", buf);
		
		
	

  	
 		
		
		Parser::toXML(tRec, "tRec", buf);
		
		
	

  	
 		
		
		Parser::toXML(tSys, "tSys", buf);
		
		
	

  	
 		
		
		Parser::toXML(tau, "tau", buf);
		
		
	

  	
 		
		
		Parser::toXML(water, "water", buf);
		
		
	

  	
 		
		
		Parser::toXML(waterError, "waterError", buf);
		
		
	

  	
 		
		if (alphaSpectrumExists) {
		
		
		Parser::toXML(alphaSpectrum, "alphaSpectrum", buf);
		
		
		}
		
	

  	
 		
		if (forwardEfficiencyExists) {
		
		
		Parser::toXML(forwardEfficiency, "forwardEfficiency", buf);
		
		
		}
		
	

  	
 		
		if (forwardEfficiencyErrorExists) {
		
		
		Parser::toXML(forwardEfficiencyError, "forwardEfficiencyError", buf);
		
		
		}
		
	

  	
 		
		if (sbGainExists) {
		
		
		Parser::toXML(sbGain, "sbGain", buf);
		
		
		}
		
	

  	
 		
		if (sbGainErrorExists) {
		
		
		Parser::toXML(sbGainError, "sbGainError", buf);
		
		
		}
		
	

  	
 		
		if (sbGainSpectrumExists) {
		
		
		Parser::toXML(sbGainSpectrum, "sbGainSpectrum", buf);
		
		
		}
		
	

	
	
		
  	
 		
		
		Parser::toXML(calDataId, "calDataId", buf);
		
		
	

  	
 		
		
		Parser::toXML(calReductionId, "calReductionId", buf);
		
		
	

	
		
	

	

		
		buf.append("</row>\n");
		return buf;
	}

	/**
	 * Fill the values of this row from an XML string 
	 * that was produced by the toXML() method.
	 * @param x The XML string being used to set the values of this row.
	 */
	void CalAtmosphereRow::setFromXML (string rowDoc) {
		Parser row(rowDoc);
		string s = "";
		try {
	
		
	
		
		
		
		receiverBand = EnumerationParser::getReceiverBand("receiverBand","CalAtmosphere",rowDoc);
		
		
		
	

	
  		
			
	  	setAntennaName(Parser::getString("antennaName","CalAtmosphere",rowDoc));
			
		
	

	
  		
			
	  	setStartValidTime(Parser::getArrayTime("startValidTime","CalAtmosphere",rowDoc));
			
		
	

	
  		
			
	  	setEndValidTime(Parser::getArrayTime("endValidTime","CalAtmosphere",rowDoc));
			
		
	

	
  		
			
	  	setNumFreq(Parser::getInteger("numFreq","CalAtmosphere",rowDoc));
			
		
	

	
  		
			
	  	setNumLoad(Parser::getInteger("numLoad","CalAtmosphere",rowDoc));
			
		
	

	
  		
			
	  	setNumReceptor(Parser::getInteger("numReceptor","CalAtmosphere",rowDoc));
			
		
	

	
  		
			
					
	  	setForwardEffSpectrum(Parser::get2DFloat("forwardEffSpectrum","CalAtmosphere",rowDoc));
	  			
	  		
		
	

	
  		
			
					
	  	setFrequencyRange(Parser::get1DFrequency("frequencyRange","CalAtmosphere",rowDoc));
	  			
	  		
		
	

	
  		
			
	  	setGroundPressure(Parser::getPressure("groundPressure","CalAtmosphere",rowDoc));
			
		
	

	
  		
			
	  	setGroundRelHumidity(Parser::getHumidity("groundRelHumidity","CalAtmosphere",rowDoc));
			
		
	

	
  		
			
					
	  	setFrequencySpectrum(Parser::get1DFrequency("frequencySpectrum","CalAtmosphere",rowDoc));
	  			
	  		
		
	

	
  		
			
	  	setGroundTemperature(Parser::getTemperature("groundTemperature","CalAtmosphere",rowDoc));
			
		
	

	
		
		
		
		polarizationTypes = EnumerationParser::getPolarizationType1D("polarizationTypes","CalAtmosphere",rowDoc);			
		
		
		
	

	
  		
			
					
	  	setPowerSkySpectrum(Parser::get2DFloat("powerSkySpectrum","CalAtmosphere",rowDoc));
	  			
	  		
		
	

	
  		
			
					
	  	setPowerLoadSpectrum(Parser::get3DFloat("powerLoadSpectrum","CalAtmosphere",rowDoc));
	  			
	  		
		
	

	
		
		
		
		syscalType = EnumerationParser::getSyscalMethod("syscalType","CalAtmosphere",rowDoc);
		
		
		
	

	
  		
			
					
	  	setTAtmSpectrum(Parser::get2DTemperature("tAtmSpectrum","CalAtmosphere",rowDoc));
	  			
	  		
		
	

	
  		
			
					
	  	setTRecSpectrum(Parser::get2DTemperature("tRecSpectrum","CalAtmosphere",rowDoc));
	  			
	  		
		
	

	
  		
			
					
	  	setTSysSpectrum(Parser::get2DTemperature("tSysSpectrum","CalAtmosphere",rowDoc));
	  			
	  		
		
	

	
  		
			
					
	  	setTauSpectrum(Parser::get2DFloat("tauSpectrum","CalAtmosphere",rowDoc));
	  			
	  		
		
	

	
  		
			
					
	  	setTAtm(Parser::get1DTemperature("tAtm","CalAtmosphere",rowDoc));
	  			
	  		
		
	

	
  		
			
					
	  	setTRec(Parser::get1DTemperature("tRec","CalAtmosphere",rowDoc));
	  			
	  		
		
	

	
  		
			
					
	  	setTSys(Parser::get1DTemperature("tSys","CalAtmosphere",rowDoc));
	  			
	  		
		
	

	
  		
			
					
	  	setTau(Parser::get1DFloat("tau","CalAtmosphere",rowDoc));
	  			
	  		
		
	

	
  		
			
					
	  	setWater(Parser::get1DLength("water","CalAtmosphere",rowDoc));
	  			
	  		
		
	

	
  		
			
					
	  	setWaterError(Parser::get1DLength("waterError","CalAtmosphere",rowDoc));
	  			
	  		
		
	

	
  		
        if (row.isStr("<alphaSpectrum>")) {
			
								
	  		setAlphaSpectrum(Parser::get2DFloat("alphaSpectrum","CalAtmosphere",rowDoc));
	  			
	  		
		}
 		
	

	
  		
        if (row.isStr("<forwardEfficiency>")) {
			
								
	  		setForwardEfficiency(Parser::get1DFloat("forwardEfficiency","CalAtmosphere",rowDoc));
	  			
	  		
		}
 		
	

	
  		
        if (row.isStr("<forwardEfficiencyError>")) {
			
								
	  		setForwardEfficiencyError(Parser::get1DDouble("forwardEfficiencyError","CalAtmosphere",rowDoc));
	  			
	  		
		}
 		
	

	
  		
        if (row.isStr("<sbGain>")) {
			
								
	  		setSbGain(Parser::get1DFloat("sbGain","CalAtmosphere",rowDoc));
	  			
	  		
		}
 		
	

	
  		
        if (row.isStr("<sbGainError>")) {
			
								
	  		setSbGainError(Parser::get1DFloat("sbGainError","CalAtmosphere",rowDoc));
	  			
	  		
		}
 		
	

	
  		
        if (row.isStr("<sbGainSpectrum>")) {
			
								
	  		setSbGainSpectrum(Parser::get2DFloat("sbGainSpectrum","CalAtmosphere",rowDoc));
	  			
	  		
		}
 		
	

	
	
		
	
  		
			
	  	setCalDataId(Parser::getTag("calDataId","CalData",rowDoc));
			
		
	

	
  		
			
	  	setCalReductionId(Parser::getTag("calReductionId","CalReduction",rowDoc));
			
		
	

	
		
	

	

		} catch (IllegalAccessException err) {
			throw ConversionException (err.getMessage(),"CalAtmosphere");
		}
	}
	
	void CalAtmosphereRow::toBin(EndianOSStream& eoss) {
	
	
	
	
		
						
			eoss.writeString(antennaName);
				
		
	

	
	
		
					
			eoss.writeInt(receiverBand);
				
		
	

	
	
		
	calDataId.toBin(eoss);
		
	

	
	
		
	calReductionId.toBin(eoss);
		
	

	
	
		
	startValidTime.toBin(eoss);
		
	

	
	
		
	endValidTime.toBin(eoss);
		
	

	
	
		
						
			eoss.writeInt(numFreq);
				
		
	

	
	
		
						
			eoss.writeInt(numLoad);
				
		
	

	
	
		
						
			eoss.writeInt(numReceptor);
				
		
	

	
	
		
		
			
		eoss.writeInt((int) forwardEffSpectrum.size());
		eoss.writeInt((int) forwardEffSpectrum.at(0).size());
		for (unsigned int i = 0; i < forwardEffSpectrum.size(); i++) 
			for (unsigned int j = 0;  j < forwardEffSpectrum.at(0).size(); j++) 
							 
				eoss.writeFloat(forwardEffSpectrum.at(i).at(j));
				
	
						
		
	

	
	
		
	Frequency::toBin(frequencyRange, eoss);
		
	

	
	
		
	groundPressure.toBin(eoss);
		
	

	
	
		
	groundRelHumidity.toBin(eoss);
		
	

	
	
		
	Frequency::toBin(frequencySpectrum, eoss);
		
	

	
	
		
	groundTemperature.toBin(eoss);
		
	

	
	
		
		
			
		eoss.writeInt((int) polarizationTypes.size());
		for (unsigned int i = 0; i < polarizationTypes.size(); i++)
				
			eoss.writeInt(polarizationTypes.at(i));
				
				
						
		
	

	
	
		
		
			
		eoss.writeInt((int) powerSkySpectrum.size());
		eoss.writeInt((int) powerSkySpectrum.at(0).size());
		for (unsigned int i = 0; i < powerSkySpectrum.size(); i++) 
			for (unsigned int j = 0;  j < powerSkySpectrum.at(0).size(); j++) 
							 
				eoss.writeFloat(powerSkySpectrum.at(i).at(j));
				
	
						
		
	

	
	
		
		
			
		eoss.writeInt((int) powerLoadSpectrum.size());
		eoss.writeInt((int) powerLoadSpectrum.at(0).size());		
		eoss.writeInt((int) powerLoadSpectrum.at(0).at(0).size());
		for (unsigned int i = 0; i < powerLoadSpectrum.size(); i++) 
			for (unsigned int j = 0;  j < powerLoadSpectrum.at(0).size(); j++)
				for (unsigned int k = 0; k <  powerLoadSpectrum.at(0).at(0).size(); k++)	
							 
					eoss.writeFloat(powerLoadSpectrum.at(i).at(j).at(k));
						
						
		
	

	
	
		
					
			eoss.writeInt(syscalType);
				
		
	

	
	
		
	Temperature::toBin(tAtmSpectrum, eoss);
		
	

	
	
		
	Temperature::toBin(tRecSpectrum, eoss);
		
	

	
	
		
	Temperature::toBin(tSysSpectrum, eoss);
		
	

	
	
		
		
			
		eoss.writeInt((int) tauSpectrum.size());
		eoss.writeInt((int) tauSpectrum.at(0).size());
		for (unsigned int i = 0; i < tauSpectrum.size(); i++) 
			for (unsigned int j = 0;  j < tauSpectrum.at(0).size(); j++) 
							 
				eoss.writeFloat(tauSpectrum.at(i).at(j));
				
	
						
		
	

	
	
		
	Temperature::toBin(tAtm, eoss);
		
	

	
	
		
	Temperature::toBin(tRec, eoss);
		
	

	
	
		
	Temperature::toBin(tSys, eoss);
		
	

	
	
		
		
			
		eoss.writeInt((int) tau.size());
		for (unsigned int i = 0; i < tau.size(); i++)
				
			eoss.writeFloat(tau.at(i));
				
				
						
		
	

	
	
		
	Length::toBin(water, eoss);
		
	

	
	
		
	Length::toBin(waterError, eoss);
		
	


	
	
	eoss.writeBoolean(alphaSpectrumExists);
	if (alphaSpectrumExists) {
	
	
	
		
		
			
		eoss.writeInt((int) alphaSpectrum.size());
		eoss.writeInt((int) alphaSpectrum.at(0).size());
		for (unsigned int i = 0; i < alphaSpectrum.size(); i++) 
			for (unsigned int j = 0;  j < alphaSpectrum.at(0).size(); j++) 
							 
				eoss.writeFloat(alphaSpectrum.at(i).at(j));
				
	
						
		
	

	}

	eoss.writeBoolean(forwardEfficiencyExists);
	if (forwardEfficiencyExists) {
	
	
	
		
		
			
		eoss.writeInt((int) forwardEfficiency.size());
		for (unsigned int i = 0; i < forwardEfficiency.size(); i++)
				
			eoss.writeFloat(forwardEfficiency.at(i));
				
				
						
		
	

	}

	eoss.writeBoolean(forwardEfficiencyErrorExists);
	if (forwardEfficiencyErrorExists) {
	
	
	
		
		
			
		eoss.writeInt((int) forwardEfficiencyError.size());
		for (unsigned int i = 0; i < forwardEfficiencyError.size(); i++)
				
			eoss.writeDouble(forwardEfficiencyError.at(i));
				
				
						
		
	

	}

	eoss.writeBoolean(sbGainExists);
	if (sbGainExists) {
	
	
	
		
		
			
		eoss.writeInt((int) sbGain.size());
		for (unsigned int i = 0; i < sbGain.size(); i++)
				
			eoss.writeFloat(sbGain.at(i));
				
				
						
		
	

	}

	eoss.writeBoolean(sbGainErrorExists);
	if (sbGainErrorExists) {
	
	
	
		
		
			
		eoss.writeInt((int) sbGainError.size());
		for (unsigned int i = 0; i < sbGainError.size(); i++)
				
			eoss.writeFloat(sbGainError.at(i));
				
				
						
		
	

	}

	eoss.writeBoolean(sbGainSpectrumExists);
	if (sbGainSpectrumExists) {
	
	
	
		
		
			
		eoss.writeInt((int) sbGainSpectrum.size());
		eoss.writeInt((int) sbGainSpectrum.at(0).size());
		for (unsigned int i = 0; i < sbGainSpectrum.size(); i++) 
			for (unsigned int j = 0;  j < sbGainSpectrum.at(0).size(); j++) 
							 
				eoss.writeFloat(sbGainSpectrum.at(i).at(j));
				
	
						
		
	

	}

	}
	
void CalAtmosphereRow::antennaNameFromBin(EndianISStream& eiss) {
		
	
	
		
			
		antennaName =  eiss.readString();
			
		
	
	
}
void CalAtmosphereRow::receiverBandFromBin(EndianISStream& eiss) {
		
	
	
		
			
		receiverBand = CReceiverBand::from_int(eiss.readInt());
			
		
	
	
}
void CalAtmosphereRow::calDataIdFromBin(EndianISStream& eiss) {
		
	
		
		
		calDataId =  Tag::fromBin(eiss);
		
	
	
}
void CalAtmosphereRow::calReductionIdFromBin(EndianISStream& eiss) {
		
	
		
		
		calReductionId =  Tag::fromBin(eiss);
		
	
	
}
void CalAtmosphereRow::startValidTimeFromBin(EndianISStream& eiss) {
		
	
		
		
		startValidTime =  ArrayTime::fromBin(eiss);
		
	
	
}
void CalAtmosphereRow::endValidTimeFromBin(EndianISStream& eiss) {
		
	
		
		
		endValidTime =  ArrayTime::fromBin(eiss);
		
	
	
}
void CalAtmosphereRow::numFreqFromBin(EndianISStream& eiss) {
		
	
	
		
			
		numFreq =  eiss.readInt();
			
		
	
	
}
void CalAtmosphereRow::numLoadFromBin(EndianISStream& eiss) {
		
	
	
		
			
		numLoad =  eiss.readInt();
			
		
	
	
}
void CalAtmosphereRow::numReceptorFromBin(EndianISStream& eiss) {
		
	
	
		
			
		numReceptor =  eiss.readInt();
			
		
	
	
}
void CalAtmosphereRow::forwardEffSpectrumFromBin(EndianISStream& eiss) {
		
	
	
		
			
	
		forwardEffSpectrum.clear();
		
		unsigned int forwardEffSpectrumDim1 = eiss.readInt();
		unsigned int forwardEffSpectrumDim2 = eiss.readInt();
		vector <float> forwardEffSpectrumAux1;
		for (unsigned int i = 0; i < forwardEffSpectrumDim1; i++) {
			forwardEffSpectrumAux1.clear();
			for (unsigned int j = 0; j < forwardEffSpectrumDim2 ; j++)			
			
			forwardEffSpectrumAux1.push_back(eiss.readFloat());
			
			forwardEffSpectrum.push_back(forwardEffSpectrumAux1);
		}
	
	

		
	
	
}
void CalAtmosphereRow::frequencyRangeFromBin(EndianISStream& eiss) {
		
	
		
		
			
	
	frequencyRange = Frequency::from1DBin(eiss);	
	

		
	
	
}
void CalAtmosphereRow::groundPressureFromBin(EndianISStream& eiss) {
		
	
		
		
		groundPressure =  Pressure::fromBin(eiss);
		
	
	
}
void CalAtmosphereRow::groundRelHumidityFromBin(EndianISStream& eiss) {
		
	
		
		
		groundRelHumidity =  Humidity::fromBin(eiss);
		
	
	
}
void CalAtmosphereRow::frequencySpectrumFromBin(EndianISStream& eiss) {
		
	
		
		
			
	
	frequencySpectrum = Frequency::from1DBin(eiss);	
	

		
	
	
}
void CalAtmosphereRow::groundTemperatureFromBin(EndianISStream& eiss) {
		
	
		
		
		groundTemperature =  Temperature::fromBin(eiss);
		
	
	
}
void CalAtmosphereRow::polarizationTypesFromBin(EndianISStream& eiss) {
		
	
	
		
			
	
		polarizationTypes.clear();
		
		unsigned int polarizationTypesDim1 = eiss.readInt();
		for (unsigned int  i = 0 ; i < polarizationTypesDim1; i++)
			
			polarizationTypes.push_back(CPolarizationType::from_int(eiss.readInt()));
			
	

		
	
	
}
void CalAtmosphereRow::powerSkySpectrumFromBin(EndianISStream& eiss) {
		
	
	
		
			
	
		powerSkySpectrum.clear();
		
		unsigned int powerSkySpectrumDim1 = eiss.readInt();
		unsigned int powerSkySpectrumDim2 = eiss.readInt();
		vector <float> powerSkySpectrumAux1;
		for (unsigned int i = 0; i < powerSkySpectrumDim1; i++) {
			powerSkySpectrumAux1.clear();
			for (unsigned int j = 0; j < powerSkySpectrumDim2 ; j++)			
			
			powerSkySpectrumAux1.push_back(eiss.readFloat());
			
			powerSkySpectrum.push_back(powerSkySpectrumAux1);
		}
	
	

		
	
	
}
void CalAtmosphereRow::powerLoadSpectrumFromBin(EndianISStream& eiss) {
		
	
	
		
			
	
		powerLoadSpectrum.clear();
			
		unsigned int powerLoadSpectrumDim1 = eiss.readInt();
		unsigned int powerLoadSpectrumDim2 = eiss.readInt();
		unsigned int powerLoadSpectrumDim3 = eiss.readInt();
		vector <vector<float> > powerLoadSpectrumAux2;
		vector <float> powerLoadSpectrumAux1;
		for (unsigned int i = 0; i < powerLoadSpectrumDim1; i++) {
			powerLoadSpectrumAux2.clear();
			for (unsigned int j = 0; j < powerLoadSpectrumDim2 ; j++) {
				powerLoadSpectrumAux1.clear();
				for (unsigned int k = 0; k < powerLoadSpectrumDim3; k++) {
			
					powerLoadSpectrumAux1.push_back(eiss.readFloat());
			
				}
				powerLoadSpectrumAux2.push_back(powerLoadSpectrumAux1);
			}
			powerLoadSpectrum.push_back(powerLoadSpectrumAux2);
		}	
	

		
	
	
}
void CalAtmosphereRow::syscalTypeFromBin(EndianISStream& eiss) {
		
	
	
		
			
		syscalType = CSyscalMethod::from_int(eiss.readInt());
			
		
	
	
}
void CalAtmosphereRow::tAtmSpectrumFromBin(EndianISStream& eiss) {
		
	
		
		
			
	
	tAtmSpectrum = Temperature::from2DBin(eiss);		
	

		
	
	
}
void CalAtmosphereRow::tRecSpectrumFromBin(EndianISStream& eiss) {
		
	
		
		
			
	
	tRecSpectrum = Temperature::from2DBin(eiss);		
	

		
	
	
}
void CalAtmosphereRow::tSysSpectrumFromBin(EndianISStream& eiss) {
		
	
		
		
			
	
	tSysSpectrum = Temperature::from2DBin(eiss);		
	

		
	
	
}
void CalAtmosphereRow::tauSpectrumFromBin(EndianISStream& eiss) {
		
	
	
		
			
	
		tauSpectrum.clear();
		
		unsigned int tauSpectrumDim1 = eiss.readInt();
		unsigned int tauSpectrumDim2 = eiss.readInt();
		vector <float> tauSpectrumAux1;
		for (unsigned int i = 0; i < tauSpectrumDim1; i++) {
			tauSpectrumAux1.clear();
			for (unsigned int j = 0; j < tauSpectrumDim2 ; j++)			
			
			tauSpectrumAux1.push_back(eiss.readFloat());
			
			tauSpectrum.push_back(tauSpectrumAux1);
		}
	
	

		
	
	
}
void CalAtmosphereRow::tAtmFromBin(EndianISStream& eiss) {
		
	
		
		
			
	
	tAtm = Temperature::from1DBin(eiss);	
	

		
	
	
}
void CalAtmosphereRow::tRecFromBin(EndianISStream& eiss) {
		
	
		
		
			
	
	tRec = Temperature::from1DBin(eiss);	
	

		
	
	
}
void CalAtmosphereRow::tSysFromBin(EndianISStream& eiss) {
		
	
		
		
			
	
	tSys = Temperature::from1DBin(eiss);	
	

		
	
	
}
void CalAtmosphereRow::tauFromBin(EndianISStream& eiss) {
		
	
	
		
			
	
		tau.clear();
		
		unsigned int tauDim1 = eiss.readInt();
		for (unsigned int  i = 0 ; i < tauDim1; i++)
			
			tau.push_back(eiss.readFloat());
			
	

		
	
	
}
void CalAtmosphereRow::waterFromBin(EndianISStream& eiss) {
		
	
		
		
			
	
	water = Length::from1DBin(eiss);	
	

		
	
	
}
void CalAtmosphereRow::waterErrorFromBin(EndianISStream& eiss) {
		
	
		
		
			
	
	waterError = Length::from1DBin(eiss);	
	

		
	
	
}

void CalAtmosphereRow::alphaSpectrumFromBin(EndianISStream& eiss) {
		
	alphaSpectrumExists = eiss.readBoolean();
	if (alphaSpectrumExists) {
		
	
	
		
			
	
		alphaSpectrum.clear();
		
		unsigned int alphaSpectrumDim1 = eiss.readInt();
		unsigned int alphaSpectrumDim2 = eiss.readInt();
		vector <float> alphaSpectrumAux1;
		for (unsigned int i = 0; i < alphaSpectrumDim1; i++) {
			alphaSpectrumAux1.clear();
			for (unsigned int j = 0; j < alphaSpectrumDim2 ; j++)			
			
			alphaSpectrumAux1.push_back(eiss.readFloat());
			
			alphaSpectrum.push_back(alphaSpectrumAux1);
		}
	
	

		
	

	}
	
}
void CalAtmosphereRow::forwardEfficiencyFromBin(EndianISStream& eiss) {
		
	forwardEfficiencyExists = eiss.readBoolean();
	if (forwardEfficiencyExists) {
		
	
	
		
			
	
		forwardEfficiency.clear();
		
		unsigned int forwardEfficiencyDim1 = eiss.readInt();
		for (unsigned int  i = 0 ; i < forwardEfficiencyDim1; i++)
			
			forwardEfficiency.push_back(eiss.readFloat());
			
	

		
	

	}
	
}
void CalAtmosphereRow::forwardEfficiencyErrorFromBin(EndianISStream& eiss) {
		
	forwardEfficiencyErrorExists = eiss.readBoolean();
	if (forwardEfficiencyErrorExists) {
		
	
	
		
			
	
		forwardEfficiencyError.clear();
		
		unsigned int forwardEfficiencyErrorDim1 = eiss.readInt();
		for (unsigned int  i = 0 ; i < forwardEfficiencyErrorDim1; i++)
			
			forwardEfficiencyError.push_back(eiss.readDouble());
			
	

		
	

	}
	
}
void CalAtmosphereRow::sbGainFromBin(EndianISStream& eiss) {
		
	sbGainExists = eiss.readBoolean();
	if (sbGainExists) {
		
	
	
		
			
	
		sbGain.clear();
		
		unsigned int sbGainDim1 = eiss.readInt();
		for (unsigned int  i = 0 ; i < sbGainDim1; i++)
			
			sbGain.push_back(eiss.readFloat());
			
	

		
	

	}
	
}
void CalAtmosphereRow::sbGainErrorFromBin(EndianISStream& eiss) {
		
	sbGainErrorExists = eiss.readBoolean();
	if (sbGainErrorExists) {
		
	
	
		
			
	
		sbGainError.clear();
		
		unsigned int sbGainErrorDim1 = eiss.readInt();
		for (unsigned int  i = 0 ; i < sbGainErrorDim1; i++)
			
			sbGainError.push_back(eiss.readFloat());
			
	

		
	

	}
	
}
void CalAtmosphereRow::sbGainSpectrumFromBin(EndianISStream& eiss) {
		
	sbGainSpectrumExists = eiss.readBoolean();
	if (sbGainSpectrumExists) {
		
	
	
		
			
	
		sbGainSpectrum.clear();
		
		unsigned int sbGainSpectrumDim1 = eiss.readInt();
		unsigned int sbGainSpectrumDim2 = eiss.readInt();
		vector <float> sbGainSpectrumAux1;
		for (unsigned int i = 0; i < sbGainSpectrumDim1; i++) {
			sbGainSpectrumAux1.clear();
			for (unsigned int j = 0; j < sbGainSpectrumDim2 ; j++)			
			
			sbGainSpectrumAux1.push_back(eiss.readFloat());
			
			sbGainSpectrum.push_back(sbGainSpectrumAux1);
		}
	
	

		
	

	}
	
}
	
	
	CalAtmosphereRow* CalAtmosphereRow::fromBin(EndianISStream& eiss, CalAtmosphereTable& table, const vector<string>& attributesSeq) {
		CalAtmosphereRow* row = new  CalAtmosphereRow(table);
		
		map<string, CalAtmosphereAttributeFromBin>::iterator iter ;
		for (unsigned int i = 0; i < attributesSeq.size(); i++) {
			iter = row->fromBinMethods.find(attributesSeq.at(i));
			if (iter == row->fromBinMethods.end()) {
				throw ConversionException("There is not method to read an attribute '"+attributesSeq.at(i)+"'.", "CalAtmosphereTable");
			}
			(row->*(row->fromBinMethods[ attributesSeq.at(i) ] ))(eiss);
		}				
		return row;
	}
	
	////////////////////////////////
	// Intrinsic Table Attributes //
	////////////////////////////////
	
	

	
 	/**
 	 * Get receiverBand.
 	 * @return receiverBand as ReceiverBandMod::ReceiverBand
 	 */
 	ReceiverBandMod::ReceiverBand CalAtmosphereRow::getReceiverBand() const {
	
  		return receiverBand;
 	}

 	/**
 	 * Set receiverBand with the specified ReceiverBandMod::ReceiverBand.
 	 * @param receiverBand The ReceiverBandMod::ReceiverBand value to which receiverBand is to be set.
 	 
 	
 		
 	 * @throw IllegalAccessException If an attempt is made to change this field after is has been added to the table.
 	 	
 	 */
 	void CalAtmosphereRow::setReceiverBand (ReceiverBandMod::ReceiverBand receiverBand)  {
  	
  	
  		if (hasBeenAdded) {
 		
			throw IllegalAccessException("receiverBand", "CalAtmosphere");
		
  		}
  	
 		this->receiverBand = receiverBand;
	
 	}
	
	

	

	
 	/**
 	 * Get antennaName.
 	 * @return antennaName as string
 	 */
 	string CalAtmosphereRow::getAntennaName() const {
	
  		return antennaName;
 	}

 	/**
 	 * Set antennaName with the specified string.
 	 * @param antennaName The string value to which antennaName is to be set.
 	 
 	
 		
 	 * @throw IllegalAccessException If an attempt is made to change this field after is has been added to the table.
 	 	
 	 */
 	void CalAtmosphereRow::setAntennaName (string antennaName)  {
  	
  	
  		if (hasBeenAdded) {
 		
			throw IllegalAccessException("antennaName", "CalAtmosphere");
		
  		}
  	
 		this->antennaName = antennaName;
	
 	}
	
	

	

	
 	/**
 	 * Get startValidTime.
 	 * @return startValidTime as ArrayTime
 	 */
 	ArrayTime CalAtmosphereRow::getStartValidTime() const {
	
  		return startValidTime;
 	}

 	/**
 	 * Set startValidTime with the specified ArrayTime.
 	 * @param startValidTime The ArrayTime value to which startValidTime is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setStartValidTime (ArrayTime startValidTime)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->startValidTime = startValidTime;
	
 	}
	
	

	

	
 	/**
 	 * Get endValidTime.
 	 * @return endValidTime as ArrayTime
 	 */
 	ArrayTime CalAtmosphereRow::getEndValidTime() const {
	
  		return endValidTime;
 	}

 	/**
 	 * Set endValidTime with the specified ArrayTime.
 	 * @param endValidTime The ArrayTime value to which endValidTime is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setEndValidTime (ArrayTime endValidTime)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->endValidTime = endValidTime;
	
 	}
	
	

	

	
 	/**
 	 * Get numFreq.
 	 * @return numFreq as int
 	 */
 	int CalAtmosphereRow::getNumFreq() const {
	
  		return numFreq;
 	}

 	/**
 	 * Set numFreq with the specified int.
 	 * @param numFreq The int value to which numFreq is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setNumFreq (int numFreq)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->numFreq = numFreq;
	
 	}
	
	

	

	
 	/**
 	 * Get numLoad.
 	 * @return numLoad as int
 	 */
 	int CalAtmosphereRow::getNumLoad() const {
	
  		return numLoad;
 	}

 	/**
 	 * Set numLoad with the specified int.
 	 * @param numLoad The int value to which numLoad is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setNumLoad (int numLoad)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->numLoad = numLoad;
	
 	}
	
	

	

	
 	/**
 	 * Get numReceptor.
 	 * @return numReceptor as int
 	 */
 	int CalAtmosphereRow::getNumReceptor() const {
	
  		return numReceptor;
 	}

 	/**
 	 * Set numReceptor with the specified int.
 	 * @param numReceptor The int value to which numReceptor is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setNumReceptor (int numReceptor)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->numReceptor = numReceptor;
	
 	}
	
	

	

	
 	/**
 	 * Get forwardEffSpectrum.
 	 * @return forwardEffSpectrum as vector<vector<float > >
 	 */
 	vector<vector<float > > CalAtmosphereRow::getForwardEffSpectrum() const {
	
  		return forwardEffSpectrum;
 	}

 	/**
 	 * Set forwardEffSpectrum with the specified vector<vector<float > >.
 	 * @param forwardEffSpectrum The vector<vector<float > > value to which forwardEffSpectrum is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setForwardEffSpectrum (vector<vector<float > > forwardEffSpectrum)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->forwardEffSpectrum = forwardEffSpectrum;
	
 	}
	
	

	

	
 	/**
 	 * Get frequencyRange.
 	 * @return frequencyRange as vector<Frequency >
 	 */
 	vector<Frequency > CalAtmosphereRow::getFrequencyRange() const {
	
  		return frequencyRange;
 	}

 	/**
 	 * Set frequencyRange with the specified vector<Frequency >.
 	 * @param frequencyRange The vector<Frequency > value to which frequencyRange is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setFrequencyRange (vector<Frequency > frequencyRange)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->frequencyRange = frequencyRange;
	
 	}
	
	

	

	
 	/**
 	 * Get groundPressure.
 	 * @return groundPressure as Pressure
 	 */
 	Pressure CalAtmosphereRow::getGroundPressure() const {
	
  		return groundPressure;
 	}

 	/**
 	 * Set groundPressure with the specified Pressure.
 	 * @param groundPressure The Pressure value to which groundPressure is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setGroundPressure (Pressure groundPressure)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->groundPressure = groundPressure;
	
 	}
	
	

	

	
 	/**
 	 * Get groundRelHumidity.
 	 * @return groundRelHumidity as Humidity
 	 */
 	Humidity CalAtmosphereRow::getGroundRelHumidity() const {
	
  		return groundRelHumidity;
 	}

 	/**
 	 * Set groundRelHumidity with the specified Humidity.
 	 * @param groundRelHumidity The Humidity value to which groundRelHumidity is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setGroundRelHumidity (Humidity groundRelHumidity)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->groundRelHumidity = groundRelHumidity;
	
 	}
	
	

	

	
 	/**
 	 * Get frequencySpectrum.
 	 * @return frequencySpectrum as vector<Frequency >
 	 */
 	vector<Frequency > CalAtmosphereRow::getFrequencySpectrum() const {
	
  		return frequencySpectrum;
 	}

 	/**
 	 * Set frequencySpectrum with the specified vector<Frequency >.
 	 * @param frequencySpectrum The vector<Frequency > value to which frequencySpectrum is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setFrequencySpectrum (vector<Frequency > frequencySpectrum)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->frequencySpectrum = frequencySpectrum;
	
 	}
	
	

	

	
 	/**
 	 * Get groundTemperature.
 	 * @return groundTemperature as Temperature
 	 */
 	Temperature CalAtmosphereRow::getGroundTemperature() const {
	
  		return groundTemperature;
 	}

 	/**
 	 * Set groundTemperature with the specified Temperature.
 	 * @param groundTemperature The Temperature value to which groundTemperature is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setGroundTemperature (Temperature groundTemperature)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->groundTemperature = groundTemperature;
	
 	}
	
	

	

	
 	/**
 	 * Get polarizationTypes.
 	 * @return polarizationTypes as vector<PolarizationTypeMod::PolarizationType >
 	 */
 	vector<PolarizationTypeMod::PolarizationType > CalAtmosphereRow::getPolarizationTypes() const {
	
  		return polarizationTypes;
 	}

 	/**
 	 * Set polarizationTypes with the specified vector<PolarizationTypeMod::PolarizationType >.
 	 * @param polarizationTypes The vector<PolarizationTypeMod::PolarizationType > value to which polarizationTypes is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setPolarizationTypes (vector<PolarizationTypeMod::PolarizationType > polarizationTypes)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->polarizationTypes = polarizationTypes;
	
 	}
	
	

	

	
 	/**
 	 * Get powerSkySpectrum.
 	 * @return powerSkySpectrum as vector<vector<float > >
 	 */
 	vector<vector<float > > CalAtmosphereRow::getPowerSkySpectrum() const {
	
  		return powerSkySpectrum;
 	}

 	/**
 	 * Set powerSkySpectrum with the specified vector<vector<float > >.
 	 * @param powerSkySpectrum The vector<vector<float > > value to which powerSkySpectrum is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setPowerSkySpectrum (vector<vector<float > > powerSkySpectrum)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->powerSkySpectrum = powerSkySpectrum;
	
 	}
	
	

	

	
 	/**
 	 * Get powerLoadSpectrum.
 	 * @return powerLoadSpectrum as vector<vector<vector<float > > >
 	 */
 	vector<vector<vector<float > > > CalAtmosphereRow::getPowerLoadSpectrum() const {
	
  		return powerLoadSpectrum;
 	}

 	/**
 	 * Set powerLoadSpectrum with the specified vector<vector<vector<float > > >.
 	 * @param powerLoadSpectrum The vector<vector<vector<float > > > value to which powerLoadSpectrum is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setPowerLoadSpectrum (vector<vector<vector<float > > > powerLoadSpectrum)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->powerLoadSpectrum = powerLoadSpectrum;
	
 	}
	
	

	

	
 	/**
 	 * Get syscalType.
 	 * @return syscalType as SyscalMethodMod::SyscalMethod
 	 */
 	SyscalMethodMod::SyscalMethod CalAtmosphereRow::getSyscalType() const {
	
  		return syscalType;
 	}

 	/**
 	 * Set syscalType with the specified SyscalMethodMod::SyscalMethod.
 	 * @param syscalType The SyscalMethodMod::SyscalMethod value to which syscalType is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setSyscalType (SyscalMethodMod::SyscalMethod syscalType)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->syscalType = syscalType;
	
 	}
	
	

	

	
 	/**
 	 * Get tAtmSpectrum.
 	 * @return tAtmSpectrum as vector<vector<Temperature > >
 	 */
 	vector<vector<Temperature > > CalAtmosphereRow::getTAtmSpectrum() const {
	
  		return tAtmSpectrum;
 	}

 	/**
 	 * Set tAtmSpectrum with the specified vector<vector<Temperature > >.
 	 * @param tAtmSpectrum The vector<vector<Temperature > > value to which tAtmSpectrum is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setTAtmSpectrum (vector<vector<Temperature > > tAtmSpectrum)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->tAtmSpectrum = tAtmSpectrum;
	
 	}
	
	

	

	
 	/**
 	 * Get tRecSpectrum.
 	 * @return tRecSpectrum as vector<vector<Temperature > >
 	 */
 	vector<vector<Temperature > > CalAtmosphereRow::getTRecSpectrum() const {
	
  		return tRecSpectrum;
 	}

 	/**
 	 * Set tRecSpectrum with the specified vector<vector<Temperature > >.
 	 * @param tRecSpectrum The vector<vector<Temperature > > value to which tRecSpectrum is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setTRecSpectrum (vector<vector<Temperature > > tRecSpectrum)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->tRecSpectrum = tRecSpectrum;
	
 	}
	
	

	

	
 	/**
 	 * Get tSysSpectrum.
 	 * @return tSysSpectrum as vector<vector<Temperature > >
 	 */
 	vector<vector<Temperature > > CalAtmosphereRow::getTSysSpectrum() const {
	
  		return tSysSpectrum;
 	}

 	/**
 	 * Set tSysSpectrum with the specified vector<vector<Temperature > >.
 	 * @param tSysSpectrum The vector<vector<Temperature > > value to which tSysSpectrum is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setTSysSpectrum (vector<vector<Temperature > > tSysSpectrum)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->tSysSpectrum = tSysSpectrum;
	
 	}
	
	

	

	
 	/**
 	 * Get tauSpectrum.
 	 * @return tauSpectrum as vector<vector<float > >
 	 */
 	vector<vector<float > > CalAtmosphereRow::getTauSpectrum() const {
	
  		return tauSpectrum;
 	}

 	/**
 	 * Set tauSpectrum with the specified vector<vector<float > >.
 	 * @param tauSpectrum The vector<vector<float > > value to which tauSpectrum is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setTauSpectrum (vector<vector<float > > tauSpectrum)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->tauSpectrum = tauSpectrum;
	
 	}
	
	

	

	
 	/**
 	 * Get tAtm.
 	 * @return tAtm as vector<Temperature >
 	 */
 	vector<Temperature > CalAtmosphereRow::getTAtm() const {
	
  		return tAtm;
 	}

 	/**
 	 * Set tAtm with the specified vector<Temperature >.
 	 * @param tAtm The vector<Temperature > value to which tAtm is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setTAtm (vector<Temperature > tAtm)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->tAtm = tAtm;
	
 	}
	
	

	

	
 	/**
 	 * Get tRec.
 	 * @return tRec as vector<Temperature >
 	 */
 	vector<Temperature > CalAtmosphereRow::getTRec() const {
	
  		return tRec;
 	}

 	/**
 	 * Set tRec with the specified vector<Temperature >.
 	 * @param tRec The vector<Temperature > value to which tRec is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setTRec (vector<Temperature > tRec)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->tRec = tRec;
	
 	}
	
	

	

	
 	/**
 	 * Get tSys.
 	 * @return tSys as vector<Temperature >
 	 */
 	vector<Temperature > CalAtmosphereRow::getTSys() const {
	
  		return tSys;
 	}

 	/**
 	 * Set tSys with the specified vector<Temperature >.
 	 * @param tSys The vector<Temperature > value to which tSys is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setTSys (vector<Temperature > tSys)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->tSys = tSys;
	
 	}
	
	

	

	
 	/**
 	 * Get tau.
 	 * @return tau as vector<float >
 	 */
 	vector<float > CalAtmosphereRow::getTau() const {
	
  		return tau;
 	}

 	/**
 	 * Set tau with the specified vector<float >.
 	 * @param tau The vector<float > value to which tau is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setTau (vector<float > tau)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->tau = tau;
	
 	}
	
	

	

	
 	/**
 	 * Get water.
 	 * @return water as vector<Length >
 	 */
 	vector<Length > CalAtmosphereRow::getWater() const {
	
  		return water;
 	}

 	/**
 	 * Set water with the specified vector<Length >.
 	 * @param water The vector<Length > value to which water is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setWater (vector<Length > water)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->water = water;
	
 	}
	
	

	

	
 	/**
 	 * Get waterError.
 	 * @return waterError as vector<Length >
 	 */
 	vector<Length > CalAtmosphereRow::getWaterError() const {
	
  		return waterError;
 	}

 	/**
 	 * Set waterError with the specified vector<Length >.
 	 * @param waterError The vector<Length > value to which waterError is to be set.
 	 
 	
 		
 	 */
 	void CalAtmosphereRow::setWaterError (vector<Length > waterError)  {
  	
  	
  		if (hasBeenAdded) {
 		
  		}
  	
 		this->waterError = waterError;
	
 	}
	
	

	
	/**
	 * The attribute alphaSpectrum is optional. Return true if this attribute exists.
	 * @return true if and only if the alphaSpectrum attribute exists. 
	 */
	bool CalAtmosphereRow::isAlphaSpectrumExists() const {
		return alphaSpectrumExists;
	}
	

	
 	/**
 	 * Get alphaSpectrum, which is optional.
 	 * @return alphaSpectrum as vector<vector<float > >
 	 * @throw IllegalAccessException If alphaSpectrum does not exist.
 	 */
 	vector<vector<float > > CalAtmosphereRow::getAlphaSpectrum() const  {
		if (!alphaSpectrumExists) {
			throw IllegalAccessException("alphaSpectrum", "CalAtmosphere");
		}
	
  		return alphaSpectrum;
 	}

 	/**
 	 * Set alphaSpectrum with the specified vector<vector<float > >.
 	 * @param alphaSpectrum The vector<vector<float > > value to which alphaSpectrum is to be set.
 	 
 	
 	 */
 	void CalAtmosphereRow::setAlphaSpectrum (vector<vector<float > > alphaSpectrum) {
	
 		this->alphaSpectrum = alphaSpectrum;
	
		alphaSpectrumExists = true;
	
 	}
	
	
	/**
	 * Mark alphaSpectrum, which is an optional field, as non-existent.
	 */
	void CalAtmosphereRow::clearAlphaSpectrum () {
		alphaSpectrumExists = false;
	}
	

	
	/**
	 * The attribute forwardEfficiency is optional. Return true if this attribute exists.
	 * @return true if and only if the forwardEfficiency attribute exists. 
	 */
	bool CalAtmosphereRow::isForwardEfficiencyExists() const {
		return forwardEfficiencyExists;
	}
	

	
 	/**
 	 * Get forwardEfficiency, which is optional.
 	 * @return forwardEfficiency as vector<float >
 	 * @throw IllegalAccessException If forwardEfficiency does not exist.
 	 */
 	vector<float > CalAtmosphereRow::getForwardEfficiency() const  {
		if (!forwardEfficiencyExists) {
			throw IllegalAccessException("forwardEfficiency", "CalAtmosphere");
		}
	
  		return forwardEfficiency;
 	}

 	/**
 	 * Set forwardEfficiency with the specified vector<float >.
 	 * @param forwardEfficiency The vector<float > value to which forwardEfficiency is to be set.
 	 
 	
 	 */
 	void CalAtmosphereRow::setForwardEfficiency (vector<float > forwardEfficiency) {
	
 		this->forwardEfficiency = forwardEfficiency;
	
		forwardEfficiencyExists = true;
	
 	}
	
	
	/**
	 * Mark forwardEfficiency, which is an optional field, as non-existent.
	 */
	void CalAtmosphereRow::clearForwardEfficiency () {
		forwardEfficiencyExists = false;
	}
	

	
	/**
	 * The attribute forwardEfficiencyError is optional. Return true if this attribute exists.
	 * @return true if and only if the forwardEfficiencyError attribute exists. 
	 */
	bool CalAtmosphereRow::isForwardEfficiencyErrorExists() const {
		return forwardEfficiencyErrorExists;
	}
	

	
 	/**
 	 * Get forwardEfficiencyError, which is optional.
 	 * @return forwardEfficiencyError as vector<double >
 	 * @throw IllegalAccessException If forwardEfficiencyError does not exist.
 	 */
 	vector<double > CalAtmosphereRow::getForwardEfficiencyError() const  {
		if (!forwardEfficiencyErrorExists) {
			throw IllegalAccessException("forwardEfficiencyError", "CalAtmosphere");
		}
	
  		return forwardEfficiencyError;
 	}

 	/**
 	 * Set forwardEfficiencyError with the specified vector<double >.
 	 * @param forwardEfficiencyError The vector<double > value to which forwardEfficiencyError is to be set.
 	 
 	
 	 */
 	void CalAtmosphereRow::setForwardEfficiencyError (vector<double > forwardEfficiencyError) {
	
 		this->forwardEfficiencyError = forwardEfficiencyError;
	
		forwardEfficiencyErrorExists = true;
	
 	}
	
	
	/**
	 * Mark forwardEfficiencyError, which is an optional field, as non-existent.
	 */
	void CalAtmosphereRow::clearForwardEfficiencyError () {
		forwardEfficiencyErrorExists = false;
	}
	

	
	/**
	 * The attribute sbGain is optional. Return true if this attribute exists.
	 * @return true if and only if the sbGain attribute exists. 
	 */
	bool CalAtmosphereRow::isSbGainExists() const {
		return sbGainExists;
	}
	

	
 	/**
 	 * Get sbGain, which is optional.
 	 * @return sbGain as vector<float >
 	 * @throw IllegalAccessException If sbGain does not exist.
 	 */
 	vector<float > CalAtmosphereRow::getSbGain() const  {
		if (!sbGainExists) {
			throw IllegalAccessException("sbGain", "CalAtmosphere");
		}
	
  		return sbGain;
 	}

 	/**
 	 * Set sbGain with the specified vector<float >.
 	 * @param sbGain The vector<float > value to which sbGain is to be set.
 	 
 	
 	 */
 	void CalAtmosphereRow::setSbGain (vector<float > sbGain) {
	
 		this->sbGain = sbGain;
	
		sbGainExists = true;
	
 	}
	
	
	/**
	 * Mark sbGain, which is an optional field, as non-existent.
	 */
	void CalAtmosphereRow::clearSbGain () {
		sbGainExists = false;
	}
	

	
	/**
	 * The attribute sbGainError is optional. Return true if this attribute exists.
	 * @return true if and only if the sbGainError attribute exists. 
	 */
	bool CalAtmosphereRow::isSbGainErrorExists() const {
		return sbGainErrorExists;
	}
	

	
 	/**
 	 * Get sbGainError, which is optional.
 	 * @return sbGainError as vector<float >
 	 * @throw IllegalAccessException If sbGainError does not exist.
 	 */
 	vector<float > CalAtmosphereRow::getSbGainError() const  {
		if (!sbGainErrorExists) {
			throw IllegalAccessException("sbGainError", "CalAtmosphere");
		}
	
  		return sbGainError;
 	}

 	/**
 	 * Set sbGainError with the specified vector<float >.
 	 * @param sbGainError The vector<float > value to which sbGainError is to be set.
 	 
 	
 	 */
 	void CalAtmosphereRow::setSbGainError (vector<float > sbGainError) {
	
 		this->sbGainError = sbGainError;
	
		sbGainErrorExists = true;
	
 	}
	
	
	/**
	 * Mark sbGainError, which is an optional field, as non-existent.
	 */
	void CalAtmosphereRow::clearSbGainError () {
		sbGainErrorExists = false;
	}
	

	
	/**
	 * The attribute sbGainSpectrum is optional. Return true if this attribute exists.
	 * @return true if and only if the sbGainSpectrum attribute exists. 
	 */
	bool CalAtmosphereRow::isSbGainSpectrumExists() const {
		return sbGainSpectrumExists;
	}
	

	
 	/**
 	 * Get sbGainSpectrum, which is optional.
 	 * @return sbGainSpectrum as vector<vector<float > >
 	 * @throw IllegalAccessException If sbGainSpectrum does not exist.
 	 */
 	vector<vector<float > > CalAtmosphereRow::getSbGainSpectrum() const  {
		if (!sbGainSpectrumExists) {
			throw IllegalAccessException("sbGainSpectrum", "CalAtmosphere");
		}
	
  		return sbGainSpectrum;
 	}

 	/**
 	 * Set sbGainSpectrum with the specified vector<vector<float > >.
 	 * @param sbGainSpectrum The vector<vector<float > > value to which sbGainSpectrum is to be set.
 	 
 	
 	 */
 	void CalAtmosphereRow::setSbGainSpectrum (vector<vector<float > > sbGainSpectrum) {
	
 		this->sbGainSpectrum = sbGainSpectrum;
	
		sbGainSpectrumExists = true;
	
 	}
	
	
	/**
	 * Mark sbGainSpectrum, which is an optional field, as non-existent.
	 */
	void CalAtmosphereRow::clearSbGainSpectrum () {
		sbGainSpectrumExists = false;
	}
	

	
	////////////////////////////////
	// Extrinsic Table Attributes //
	////////////////////////////////
	
	

	
 	/**
 	 * Get calDataId.
 	 * @return calDataId as Tag
 	 */
 	Tag CalAtmosphereRow::getCalDataId() const {
	
  		return calDataId;
 	}

 	/**
 	 * Set calDataId with the specified Tag.
 	 * @param calDataId The Tag value to which calDataId is to be set.
 	 
 	
 		
 	 * @throw IllegalAccessException If an attempt is made to change this field after is has been added to the table.
 	 	
 	 */
 	void CalAtmosphereRow::setCalDataId (Tag calDataId)  {
  	
  	
  		if (hasBeenAdded) {
 		
			throw IllegalAccessException("calDataId", "CalAtmosphere");
		
  		}
  	
 		this->calDataId = calDataId;
	
 	}
	
	

	

	
 	/**
 	 * Get calReductionId.
 	 * @return calReductionId as Tag
 	 */
 	Tag CalAtmosphereRow::getCalReductionId() const {
	
  		return calReductionId;
 	}

 	/**
 	 * Set calReductionId with the specified Tag.
 	 * @param calReductionId The Tag value to which calReductionId is to be set.
 	 
 	
 		
 	 * @throw IllegalAccessException If an attempt is made to change this field after is has been added to the table.
 	 	
 	 */
 	void CalAtmosphereRow::setCalReductionId (Tag calReductionId)  {
  	
  	
  		if (hasBeenAdded) {
 		
			throw IllegalAccessException("calReductionId", "CalAtmosphere");
		
  		}
  	
 		this->calReductionId = calReductionId;
	
 	}
	
	

	///////////
	// Links //
	///////////
	
	
	
	
		

	/**
	 * Returns the pointer to the row in the CalReduction table having CalReduction.calReductionId == calReductionId
	 * @return a CalReductionRow*
	 * 
	 
	 */
	 CalReductionRow* CalAtmosphereRow::getCalReductionUsingCalReductionId() {
	 
	 	return table.getContainer().getCalReduction().getRowByKey(calReductionId);
	 }
	 

	

	
	
	
		

	/**
	 * Returns the pointer to the row in the CalData table having CalData.calDataId == calDataId
	 * @return a CalDataRow*
	 * 
	 
	 */
	 CalDataRow* CalAtmosphereRow::getCalDataUsingCalDataId() {
	 
	 	return table.getContainer().getCalData().getRowByKey(calDataId);
	 }
	 

	

	
	/**
	 * Create a CalAtmosphereRow.
	 * <p>
	 * This constructor is private because only the
	 * table can create rows.  All rows know the table
	 * to which they belong.
	 * @param table The table to which this row belongs.
	 */ 
	CalAtmosphereRow::CalAtmosphereRow (CalAtmosphereTable &t) : table(t) {
		hasBeenAdded = false;
		
	
	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	
		alphaSpectrumExists = false;
	

	
		forwardEfficiencyExists = false;
	

	
		forwardEfficiencyErrorExists = false;
	

	
		sbGainExists = false;
	

	
		sbGainErrorExists = false;
	

	
		sbGainSpectrumExists = false;
	

	
	

	

	
	
	
	
// This attribute is scalar and has an enumeration type. Let's initialize it to some valid value (the 1st of the enumeration).		
receiverBand = CReceiverBand::from_int(0);
	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	
// This attribute is scalar and has an enumeration type. Let's initialize it to some valid value (the 1st of the enumeration).		
syscalType = CSyscalMethod::from_int(0);
	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	
	
	 fromBinMethods["antennaName"] = &CalAtmosphereRow::antennaNameFromBin; 
	 fromBinMethods["receiverBand"] = &CalAtmosphereRow::receiverBandFromBin; 
	 fromBinMethods["calDataId"] = &CalAtmosphereRow::calDataIdFromBin; 
	 fromBinMethods["calReductionId"] = &CalAtmosphereRow::calReductionIdFromBin; 
	 fromBinMethods["startValidTime"] = &CalAtmosphereRow::startValidTimeFromBin; 
	 fromBinMethods["endValidTime"] = &CalAtmosphereRow::endValidTimeFromBin; 
	 fromBinMethods["numFreq"] = &CalAtmosphereRow::numFreqFromBin; 
	 fromBinMethods["numLoad"] = &CalAtmosphereRow::numLoadFromBin; 
	 fromBinMethods["numReceptor"] = &CalAtmosphereRow::numReceptorFromBin; 
	 fromBinMethods["forwardEffSpectrum"] = &CalAtmosphereRow::forwardEffSpectrumFromBin; 
	 fromBinMethods["frequencyRange"] = &CalAtmosphereRow::frequencyRangeFromBin; 
	 fromBinMethods["groundPressure"] = &CalAtmosphereRow::groundPressureFromBin; 
	 fromBinMethods["groundRelHumidity"] = &CalAtmosphereRow::groundRelHumidityFromBin; 
	 fromBinMethods["frequencySpectrum"] = &CalAtmosphereRow::frequencySpectrumFromBin; 
	 fromBinMethods["groundTemperature"] = &CalAtmosphereRow::groundTemperatureFromBin; 
	 fromBinMethods["polarizationTypes"] = &CalAtmosphereRow::polarizationTypesFromBin; 
	 fromBinMethods["powerSkySpectrum"] = &CalAtmosphereRow::powerSkySpectrumFromBin; 
	 fromBinMethods["powerLoadSpectrum"] = &CalAtmosphereRow::powerLoadSpectrumFromBin; 
	 fromBinMethods["syscalType"] = &CalAtmosphereRow::syscalTypeFromBin; 
	 fromBinMethods["tAtmSpectrum"] = &CalAtmosphereRow::tAtmSpectrumFromBin; 
	 fromBinMethods["tRecSpectrum"] = &CalAtmosphereRow::tRecSpectrumFromBin; 
	 fromBinMethods["tSysSpectrum"] = &CalAtmosphereRow::tSysSpectrumFromBin; 
	 fromBinMethods["tauSpectrum"] = &CalAtmosphereRow::tauSpectrumFromBin; 
	 fromBinMethods["tAtm"] = &CalAtmosphereRow::tAtmFromBin; 
	 fromBinMethods["tRec"] = &CalAtmosphereRow::tRecFromBin; 
	 fromBinMethods["tSys"] = &CalAtmosphereRow::tSysFromBin; 
	 fromBinMethods["tau"] = &CalAtmosphereRow::tauFromBin; 
	 fromBinMethods["water"] = &CalAtmosphereRow::waterFromBin; 
	 fromBinMethods["waterError"] = &CalAtmosphereRow::waterErrorFromBin; 
		
	
	 fromBinMethods["alphaSpectrum"] = &CalAtmosphereRow::alphaSpectrumFromBin; 
	 fromBinMethods["forwardEfficiency"] = &CalAtmosphereRow::forwardEfficiencyFromBin; 
	 fromBinMethods["forwardEfficiencyError"] = &CalAtmosphereRow::forwardEfficiencyErrorFromBin; 
	 fromBinMethods["sbGain"] = &CalAtmosphereRow::sbGainFromBin; 
	 fromBinMethods["sbGainError"] = &CalAtmosphereRow::sbGainErrorFromBin; 
	 fromBinMethods["sbGainSpectrum"] = &CalAtmosphereRow::sbGainSpectrumFromBin; 
	
	}
	
	CalAtmosphereRow::CalAtmosphereRow (CalAtmosphereTable &t, CalAtmosphereRow &row) : table(t) {
		hasBeenAdded = false;
		
		if (&row == 0) {
	
	
	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	

	
		alphaSpectrumExists = false;
	

	
		forwardEfficiencyExists = false;
	

	
		forwardEfficiencyErrorExists = false;
	

	
		sbGainExists = false;
	

	
		sbGainErrorExists = false;
	

	
		sbGainSpectrumExists = false;
	

	
	

	
		
		}
		else {
	
		
			antennaName = row.antennaName;
		
			receiverBand = row.receiverBand;
		
			calDataId = row.calDataId;
		
			calReductionId = row.calReductionId;
		
		
		
		
			startValidTime = row.startValidTime;
		
			endValidTime = row.endValidTime;
		
			numFreq = row.numFreq;
		
			numLoad = row.numLoad;
		
			numReceptor = row.numReceptor;
		
			forwardEffSpectrum = row.forwardEffSpectrum;
		
			frequencyRange = row.frequencyRange;
		
			groundPressure = row.groundPressure;
		
			groundRelHumidity = row.groundRelHumidity;
		
			frequencySpectrum = row.frequencySpectrum;
		
			groundTemperature = row.groundTemperature;
		
			polarizationTypes = row.polarizationTypes;
		
			powerSkySpectrum = row.powerSkySpectrum;
		
			powerLoadSpectrum = row.powerLoadSpectrum;
		
			syscalType = row.syscalType;
		
			tAtmSpectrum = row.tAtmSpectrum;
		
			tRecSpectrum = row.tRecSpectrum;
		
			tSysSpectrum = row.tSysSpectrum;
		
			tauSpectrum = row.tauSpectrum;
		
			tAtm = row.tAtm;
		
			tRec = row.tRec;
		
			tSys = row.tSys;
		
			tau = row.tau;
		
			water = row.water;
		
			waterError = row.waterError;
		
		
		
		
		if (row.alphaSpectrumExists) {
			alphaSpectrum = row.alphaSpectrum;		
			alphaSpectrumExists = true;
		}
		else
			alphaSpectrumExists = false;
		
		if (row.forwardEfficiencyExists) {
			forwardEfficiency = row.forwardEfficiency;		
			forwardEfficiencyExists = true;
		}
		else
			forwardEfficiencyExists = false;
		
		if (row.forwardEfficiencyErrorExists) {
			forwardEfficiencyError = row.forwardEfficiencyError;		
			forwardEfficiencyErrorExists = true;
		}
		else
			forwardEfficiencyErrorExists = false;
		
		if (row.sbGainExists) {
			sbGain = row.sbGain;		
			sbGainExists = true;
		}
		else
			sbGainExists = false;
		
		if (row.sbGainErrorExists) {
			sbGainError = row.sbGainError;		
			sbGainErrorExists = true;
		}
		else
			sbGainErrorExists = false;
		
		if (row.sbGainSpectrumExists) {
			sbGainSpectrum = row.sbGainSpectrum;		
			sbGainSpectrumExists = true;
		}
		else
			sbGainSpectrumExists = false;
		
		}
		
		 fromBinMethods["antennaName"] = &CalAtmosphereRow::antennaNameFromBin; 
		 fromBinMethods["receiverBand"] = &CalAtmosphereRow::receiverBandFromBin; 
		 fromBinMethods["calDataId"] = &CalAtmosphereRow::calDataIdFromBin; 
		 fromBinMethods["calReductionId"] = &CalAtmosphereRow::calReductionIdFromBin; 
		 fromBinMethods["startValidTime"] = &CalAtmosphereRow::startValidTimeFromBin; 
		 fromBinMethods["endValidTime"] = &CalAtmosphereRow::endValidTimeFromBin; 
		 fromBinMethods["numFreq"] = &CalAtmosphereRow::numFreqFromBin; 
		 fromBinMethods["numLoad"] = &CalAtmosphereRow::numLoadFromBin; 
		 fromBinMethods["numReceptor"] = &CalAtmosphereRow::numReceptorFromBin; 
		 fromBinMethods["forwardEffSpectrum"] = &CalAtmosphereRow::forwardEffSpectrumFromBin; 
		 fromBinMethods["frequencyRange"] = &CalAtmosphereRow::frequencyRangeFromBin; 
		 fromBinMethods["groundPressure"] = &CalAtmosphereRow::groundPressureFromBin; 
		 fromBinMethods["groundRelHumidity"] = &CalAtmosphereRow::groundRelHumidityFromBin; 
		 fromBinMethods["frequencySpectrum"] = &CalAtmosphereRow::frequencySpectrumFromBin; 
		 fromBinMethods["groundTemperature"] = &CalAtmosphereRow::groundTemperatureFromBin; 
		 fromBinMethods["polarizationTypes"] = &CalAtmosphereRow::polarizationTypesFromBin; 
		 fromBinMethods["powerSkySpectrum"] = &CalAtmosphereRow::powerSkySpectrumFromBin; 
		 fromBinMethods["powerLoadSpectrum"] = &CalAtmosphereRow::powerLoadSpectrumFromBin; 
		 fromBinMethods["syscalType"] = &CalAtmosphereRow::syscalTypeFromBin; 
		 fromBinMethods["tAtmSpectrum"] = &CalAtmosphereRow::tAtmSpectrumFromBin; 
		 fromBinMethods["tRecSpectrum"] = &CalAtmosphereRow::tRecSpectrumFromBin; 
		 fromBinMethods["tSysSpectrum"] = &CalAtmosphereRow::tSysSpectrumFromBin; 
		 fromBinMethods["tauSpectrum"] = &CalAtmosphereRow::tauSpectrumFromBin; 
		 fromBinMethods["tAtm"] = &CalAtmosphereRow::tAtmFromBin; 
		 fromBinMethods["tRec"] = &CalAtmosphereRow::tRecFromBin; 
		 fromBinMethods["tSys"] = &CalAtmosphereRow::tSysFromBin; 
		 fromBinMethods["tau"] = &CalAtmosphereRow::tauFromBin; 
		 fromBinMethods["water"] = &CalAtmosphereRow::waterFromBin; 
		 fromBinMethods["waterError"] = &CalAtmosphereRow::waterErrorFromBin; 
			
	
		 fromBinMethods["alphaSpectrum"] = &CalAtmosphereRow::alphaSpectrumFromBin; 
		 fromBinMethods["forwardEfficiency"] = &CalAtmosphereRow::forwardEfficiencyFromBin; 
		 fromBinMethods["forwardEfficiencyError"] = &CalAtmosphereRow::forwardEfficiencyErrorFromBin; 
		 fromBinMethods["sbGain"] = &CalAtmosphereRow::sbGainFromBin; 
		 fromBinMethods["sbGainError"] = &CalAtmosphereRow::sbGainErrorFromBin; 
		 fromBinMethods["sbGainSpectrum"] = &CalAtmosphereRow::sbGainSpectrumFromBin; 
			
	}

	
	bool CalAtmosphereRow::compareNoAutoInc(string antennaName, ReceiverBandMod::ReceiverBand receiverBand, Tag calDataId, Tag calReductionId, ArrayTime startValidTime, ArrayTime endValidTime, int numFreq, int numLoad, int numReceptor, vector<vector<float > > forwardEffSpectrum, vector<Frequency > frequencyRange, Pressure groundPressure, Humidity groundRelHumidity, vector<Frequency > frequencySpectrum, Temperature groundTemperature, vector<PolarizationTypeMod::PolarizationType > polarizationTypes, vector<vector<float > > powerSkySpectrum, vector<vector<vector<float > > > powerLoadSpectrum, SyscalMethodMod::SyscalMethod syscalType, vector<vector<Temperature > > tAtmSpectrum, vector<vector<Temperature > > tRecSpectrum, vector<vector<Temperature > > tSysSpectrum, vector<vector<float > > tauSpectrum, vector<Temperature > tAtm, vector<Temperature > tRec, vector<Temperature > tSys, vector<float > tau, vector<Length > water, vector<Length > waterError) {
		bool result;
		result = true;
		
	
		
		result = result && (this->antennaName == antennaName);
		
		if (!result) return false;
	

	
		
		result = result && (this->receiverBand == receiverBand);
		
		if (!result) return false;
	

	
		
		result = result && (this->calDataId == calDataId);
		
		if (!result) return false;
	

	
		
		result = result && (this->calReductionId == calReductionId);
		
		if (!result) return false;
	

	
		
		result = result && (this->startValidTime == startValidTime);
		
		if (!result) return false;
	

	
		
		result = result && (this->endValidTime == endValidTime);
		
		if (!result) return false;
	

	
		
		result = result && (this->numFreq == numFreq);
		
		if (!result) return false;
	

	
		
		result = result && (this->numLoad == numLoad);
		
		if (!result) return false;
	

	
		
		result = result && (this->numReceptor == numReceptor);
		
		if (!result) return false;
	

	
		
		result = result && (this->forwardEffSpectrum == forwardEffSpectrum);
		
		if (!result) return false;
	

	
		
		result = result && (this->frequencyRange == frequencyRange);
		
		if (!result) return false;
	

	
		
		result = result && (this->groundPressure == groundPressure);
		
		if (!result) return false;
	

	
		
		result = result && (this->groundRelHumidity == groundRelHumidity);
		
		if (!result) return false;
	

	
		
		result = result && (this->frequencySpectrum == frequencySpectrum);
		
		if (!result) return false;
	

	
		
		result = result && (this->groundTemperature == groundTemperature);
		
		if (!result) return false;
	

	
		
		result = result && (this->polarizationTypes == polarizationTypes);
		
		if (!result) return false;
	

	
		
		result = result && (this->powerSkySpectrum == powerSkySpectrum);
		
		if (!result) return false;
	

	
		
		result = result && (this->powerLoadSpectrum == powerLoadSpectrum);
		
		if (!result) return false;
	

	
		
		result = result && (this->syscalType == syscalType);
		
		if (!result) return false;
	

	
		
		result = result && (this->tAtmSpectrum == tAtmSpectrum);
		
		if (!result) return false;
	

	
		
		result = result && (this->tRecSpectrum == tRecSpectrum);
		
		if (!result) return false;
	

	
		
		result = result && (this->tSysSpectrum == tSysSpectrum);
		
		if (!result) return false;
	

	
		
		result = result && (this->tauSpectrum == tauSpectrum);
		
		if (!result) return false;
	

	
		
		result = result && (this->tAtm == tAtm);
		
		if (!result) return false;
	

	
		
		result = result && (this->tRec == tRec);
		
		if (!result) return false;
	

	
		
		result = result && (this->tSys == tSys);
		
		if (!result) return false;
	

	
		
		result = result && (this->tau == tau);
		
		if (!result) return false;
	

	
		
		result = result && (this->water == water);
		
		if (!result) return false;
	

	
		
		result = result && (this->waterError == waterError);
		
		if (!result) return false;
	

		return result;
	}	
	
	
	
	bool CalAtmosphereRow::compareRequiredValue(ArrayTime startValidTime, ArrayTime endValidTime, int numFreq, int numLoad, int numReceptor, vector<vector<float > > forwardEffSpectrum, vector<Frequency > frequencyRange, Pressure groundPressure, Humidity groundRelHumidity, vector<Frequency > frequencySpectrum, Temperature groundTemperature, vector<PolarizationTypeMod::PolarizationType > polarizationTypes, vector<vector<float > > powerSkySpectrum, vector<vector<vector<float > > > powerLoadSpectrum, SyscalMethodMod::SyscalMethod syscalType, vector<vector<Temperature > > tAtmSpectrum, vector<vector<Temperature > > tRecSpectrum, vector<vector<Temperature > > tSysSpectrum, vector<vector<float > > tauSpectrum, vector<Temperature > tAtm, vector<Temperature > tRec, vector<Temperature > tSys, vector<float > tau, vector<Length > water, vector<Length > waterError) {
		bool result;
		result = true;
		
	
		if (!(this->startValidTime == startValidTime)) return false;
	

	
		if (!(this->endValidTime == endValidTime)) return false;
	

	
		if (!(this->numFreq == numFreq)) return false;
	

	
		if (!(this->numLoad == numLoad)) return false;
	

	
		if (!(this->numReceptor == numReceptor)) return false;
	

	
		if (!(this->forwardEffSpectrum == forwardEffSpectrum)) return false;
	

	
		if (!(this->frequencyRange == frequencyRange)) return false;
	

	
		if (!(this->groundPressure == groundPressure)) return false;
	

	
		if (!(this->groundRelHumidity == groundRelHumidity)) return false;
	

	
		if (!(this->frequencySpectrum == frequencySpectrum)) return false;
	

	
		if (!(this->groundTemperature == groundTemperature)) return false;
	

	
		if (!(this->polarizationTypes == polarizationTypes)) return false;
	

	
		if (!(this->powerSkySpectrum == powerSkySpectrum)) return false;
	

	
		if (!(this->powerLoadSpectrum == powerLoadSpectrum)) return false;
	

	
		if (!(this->syscalType == syscalType)) return false;
	

	
		if (!(this->tAtmSpectrum == tAtmSpectrum)) return false;
	

	
		if (!(this->tRecSpectrum == tRecSpectrum)) return false;
	

	
		if (!(this->tSysSpectrum == tSysSpectrum)) return false;
	

	
		if (!(this->tauSpectrum == tauSpectrum)) return false;
	

	
		if (!(this->tAtm == tAtm)) return false;
	

	
		if (!(this->tRec == tRec)) return false;
	

	
		if (!(this->tSys == tSys)) return false;
	

	
		if (!(this->tau == tau)) return false;
	

	
		if (!(this->water == water)) return false;
	

	
		if (!(this->waterError == waterError)) return false;
	

		return result;
	}
	
	
	/**
	 * Return true if all required attributes of the value part are equal to their homologues
	 * in x and false otherwise.
	 *
	 * @param x a pointer on the CalAtmosphereRow whose required attributes of the value part 
	 * will be compared with those of this.
	 * @return a boolean.
	 */
	bool CalAtmosphereRow::equalByRequiredValue(CalAtmosphereRow* x) {
		
			
		if (this->startValidTime != x->startValidTime) return false;
			
		if (this->endValidTime != x->endValidTime) return false;
			
		if (this->numFreq != x->numFreq) return false;
			
		if (this->numLoad != x->numLoad) return false;
			
		if (this->numReceptor != x->numReceptor) return false;
			
		if (this->forwardEffSpectrum != x->forwardEffSpectrum) return false;
			
		if (this->frequencyRange != x->frequencyRange) return false;
			
		if (this->groundPressure != x->groundPressure) return false;
			
		if (this->groundRelHumidity != x->groundRelHumidity) return false;
			
		if (this->frequencySpectrum != x->frequencySpectrum) return false;
			
		if (this->groundTemperature != x->groundTemperature) return false;
			
		if (this->polarizationTypes != x->polarizationTypes) return false;
			
		if (this->powerSkySpectrum != x->powerSkySpectrum) return false;
			
		if (this->powerLoadSpectrum != x->powerLoadSpectrum) return false;
			
		if (this->syscalType != x->syscalType) return false;
			
		if (this->tAtmSpectrum != x->tAtmSpectrum) return false;
			
		if (this->tRecSpectrum != x->tRecSpectrum) return false;
			
		if (this->tSysSpectrum != x->tSysSpectrum) return false;
			
		if (this->tauSpectrum != x->tauSpectrum) return false;
			
		if (this->tAtm != x->tAtm) return false;
			
		if (this->tRec != x->tRec) return false;
			
		if (this->tSys != x->tSys) return false;
			
		if (this->tau != x->tau) return false;
			
		if (this->water != x->water) return false;
			
		if (this->waterError != x->waterError) return false;
			
		
		return true;
	}	
	
/*
	 map<string, CalAtmosphereAttributeFromBin> CalAtmosphereRow::initFromBinMethods() {
		map<string, CalAtmosphereAttributeFromBin> result;
		
		result["antennaName"] = &CalAtmosphereRow::antennaNameFromBin;
		result["receiverBand"] = &CalAtmosphereRow::receiverBandFromBin;
		result["calDataId"] = &CalAtmosphereRow::calDataIdFromBin;
		result["calReductionId"] = &CalAtmosphereRow::calReductionIdFromBin;
		result["startValidTime"] = &CalAtmosphereRow::startValidTimeFromBin;
		result["endValidTime"] = &CalAtmosphereRow::endValidTimeFromBin;
		result["numFreq"] = &CalAtmosphereRow::numFreqFromBin;
		result["numLoad"] = &CalAtmosphereRow::numLoadFromBin;
		result["numReceptor"] = &CalAtmosphereRow::numReceptorFromBin;
		result["forwardEffSpectrum"] = &CalAtmosphereRow::forwardEffSpectrumFromBin;
		result["frequencyRange"] = &CalAtmosphereRow::frequencyRangeFromBin;
		result["groundPressure"] = &CalAtmosphereRow::groundPressureFromBin;
		result["groundRelHumidity"] = &CalAtmosphereRow::groundRelHumidityFromBin;
		result["frequencySpectrum"] = &CalAtmosphereRow::frequencySpectrumFromBin;
		result["groundTemperature"] = &CalAtmosphereRow::groundTemperatureFromBin;
		result["polarizationTypes"] = &CalAtmosphereRow::polarizationTypesFromBin;
		result["powerSkySpectrum"] = &CalAtmosphereRow::powerSkySpectrumFromBin;
		result["powerLoadSpectrum"] = &CalAtmosphereRow::powerLoadSpectrumFromBin;
		result["syscalType"] = &CalAtmosphereRow::syscalTypeFromBin;
		result["tAtmSpectrum"] = &CalAtmosphereRow::tAtmSpectrumFromBin;
		result["tRecSpectrum"] = &CalAtmosphereRow::tRecSpectrumFromBin;
		result["tSysSpectrum"] = &CalAtmosphereRow::tSysSpectrumFromBin;
		result["tauSpectrum"] = &CalAtmosphereRow::tauSpectrumFromBin;
		result["tAtm"] = &CalAtmosphereRow::tAtmFromBin;
		result["tRec"] = &CalAtmosphereRow::tRecFromBin;
		result["tSys"] = &CalAtmosphereRow::tSysFromBin;
		result["tau"] = &CalAtmosphereRow::tauFromBin;
		result["water"] = &CalAtmosphereRow::waterFromBin;
		result["waterError"] = &CalAtmosphereRow::waterErrorFromBin;
		
		
		result["alphaSpectrum"] = &CalAtmosphereRow::alphaSpectrumFromBin;
		result["forwardEfficiency"] = &CalAtmosphereRow::forwardEfficiencyFromBin;
		result["forwardEfficiencyError"] = &CalAtmosphereRow::forwardEfficiencyErrorFromBin;
		result["sbGain"] = &CalAtmosphereRow::sbGainFromBin;
		result["sbGainError"] = &CalAtmosphereRow::sbGainErrorFromBin;
		result["sbGainSpectrum"] = &CalAtmosphereRow::sbGainSpectrumFromBin;
			
		
		return result;	
	}
*/	
} // End namespace asdm
 
