//$Id$
//------------------------------------------------------------------------------
//                           DeltaRangeRateAdapter
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool
//
// Copyright (c) 2020 Daniel Estevez <daniel@destevez.net>
//
// Copyright (c) 2002-2011 United States Government as represented by the
// Administrator of The National Aeronautics and Space Administration.
// All Other Rights Reserved.
//
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under the FDSS 
// contract
//
// Author: Daniel Estevez <daniel@destevez.net>
// Created: May 24, 2020
//
// Based on DeltaRangeRateAdapter
/**
 * A measurement adapter for GN Doppler
 */
//------------------------------------------------------------------------------

#ifndef DeltaRangeRateAdapter_hpp
#define DeltaRangeRateAdapter_hpp

#include "DeltaRangeAdapter.hpp"


/**
 * A measurement adapter for DeltaRangeRate
 */
class ESTIMATION_API DeltaRangeRateAdapter: public DeltaRangeAdapter
{
public:
   DeltaRangeRateAdapter(const std::string& name);
   virtual ~DeltaRangeRateAdapter();
   DeltaRangeRateAdapter(const DeltaRangeRateAdapter& da);
   DeltaRangeRateAdapter&      operator=(const DeltaRangeRateAdapter& da);

   virtual void SetSolarSystem(SolarSystem *ss);

   virtual GmatBase*    Clone() const;

   virtual std::string  GetParameterText(const Integer id) const;
   virtual Integer GetParameterID(const std::string &str) const;
   virtual Gmat::ParameterType
                        GetParameterType(const Integer id) const;
   virtual std::string  GetParameterTypeString(const Integer id) const;



   virtual Real         GetRealParameter(const Integer id) const;
   virtual Real         SetRealParameter(const Integer id,
                                         const Real value);
   virtual Real         GetRealParameter(const std::string &label) const;
   virtual Real         SetRealParameter(const std::string &label,
                                         const Real value);

   virtual bool         SetStringParameter(const Integer id, const std::string& value);
   virtual bool         SetStringParameter(const Integer id, const std::string &value, const Integer index);
   virtual bool         SetStringParameter(const std::string &label, const std::string &value);
   virtual bool         SetStringParameter(const std::string &label, const std::string &value, const Integer index);

   virtual Integer      SetIntegerParameter(const Integer id, const Integer value);
   virtual Integer      SetIntegerParameter(const std::string &label, const Integer value);

   virtual bool         SetBooleanParameter(const Integer id, const bool value);
   virtual bool         SetBooleanParameter(const std::string &label, const bool value);

   virtual bool         RenameRefObject(const UnsignedInt type,
                                        const std::string &oldName,
                                        const std::string &newName);
   virtual bool         SetRefObject(GmatBase* obj,
                                     const UnsignedInt type,
                                     const std::string& name);
   virtual bool         SetRefObject(GmatBase* obj,
                                     const UnsignedInt type,
                                     const std::string& name, 
                                     const Integer index);

   virtual bool         SetMeasurement(MeasureModel* meas);
   virtual void         SetPropagator(PropSetup* ps);

   virtual void         SetCorrection(const std::string& correctionName,
                                      const std::string& correctionType);


   virtual bool         Initialize();
   virtual void         SetTransientForces(std::vector<PhysicalModel*> *tf);

   // Preserve interfaces in the older measurement model code
   virtual const MeasurementData&
                        CalculateMeasurement(bool withEvents = false,
                              ObservationData* forObservation = NULL,
                              std::vector<RampTableData>* rampTB = NULL,
                              bool forSimulation = false);
   virtual const std::vector<RealArray>&
                        CalculateMeasurementDerivatives(GmatBase *obj,
                              Integer id);
   virtual bool         WriteMeasurements();
   virtual bool         WriteMeasurement(Integer id);

   // Covariance handling code
   virtual Integer      HasParameterCovariances(Integer parameterId);

   virtual Integer      GetEventCount();


   /// DeltaRangeAdapter object for S-path
   DeltaRangeAdapter* adapterS;

   DEFAULT_TO_NO_CLONES

protected:
   /// Doppler count interval
   Real                 dopplerCountInterval;             // unit: second
   /// Multiplier for S-path and E-path
   Real                 multiplierS;
   Real                 multiplierE;

   /// Parameter IDs for the DopplerAdapter
   enum
   {
      DOPPLER_COUNT_INTERVAL = DeltaRangeAdapterParamCount,
      DeltaRangeRateAdapterParamCount,
   };

   /// Strings describing the DeltaRangeRateAdapter parameters
   static const std::string PARAMETER_TEXT[DeltaRangeRateAdapterParamCount -
                                           DeltaRangeAdapterParamCount];
   /// Types of the DeltaRangeRateAdapter parameters
   static const Gmat::ParameterType PARAMETER_TYPE[DeltaRangeRateAdapterParamCount -
                                                   DeltaRangeAdapterParamCount];

private:
   /// MeasurementData for Start path
   MeasurementData measDataS;
   /// MeasurementData for End path
   MeasurementData measDataE;
};

#endif /* DeltaRangeRateAdapter_hpp */
