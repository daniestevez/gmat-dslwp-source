//$Id$
//------------------------------------------------------------------------------
//                           DeltaRangeAdapter
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool
//
// Copyright (c) 2020 Daniel Estevez <daniel@destevez.net>
//
// Copyright (c) 2002-2011 United States Government as represented by the
// Administrator of The National Aeronautics and Space Administration.
// All Other Rights Reserved.
//
// Author: Daniel Estevez <daniel@destevez.net>
// Created: May 2, 2020
//
// Based on GNRangeAdapter
/**
 * A measurement adapter for deltarange measurement
 */
//------------------------------------------------------------------------------

#ifndef DeltaRangeAdapter_hpp
#define DeltaRangeAdapter_hpp

#include "RangeAdapterKm.hpp"

/**
 * A measurement adapter for delta range measurement
 */
class ESTIMATION_API DeltaRangeAdapter: public RangeAdapterKm
{
public:
   DeltaRangeAdapter(const std::string& name);
   virtual ~DeltaRangeAdapter();
   DeltaRangeAdapter(const DeltaRangeAdapter& rak);
   DeltaRangeAdapter&      operator=(const DeltaRangeAdapter& rak);

   virtual GmatBase*    Clone() const;

   virtual void SetSolarSystem(SolarSystem *ss);
   
   virtual bool SetStringParameter(const Integer id, const std::string &value);
   virtual bool SetStringParameter(const Integer id, const std::string& value, const Integer index);
   virtual Integer SetIntegerParameter(const Integer id, const Integer value);
   virtual Real SetRealParameter(const Integer id, const Real value);
   virtual bool SetBooleanParameter(const Integer id, const bool value);
   virtual bool RenameRefObject(const UnsignedInt type, const std::string& oldName, const std::string& newName);
   virtual bool SetRefObject(GmatBase* obj, const UnsignedInt type, const std::string& name);
   virtual bool SetRefObject(GmatBase* obj, const UnsignedInt type, const std::string& name, const Integer index);

   virtual void SetPropagator(PropSetup* ps);
   virtual void SetTransientForces(std::vector<PhysicalModel*> *tf);
   
   virtual bool         Initialize();

   virtual const MeasurementData&
      CalculateMeasurement(bool withEvents = false,
      ObservationData* forObservation = NULL,
      std::vector<RampTableData>* rampTB = NULL,
      bool forSimulation = false);

   virtual void SetCorrection(const std::string& correctionName, const std::string& correctionType);

   RangeAdapterKm* referenceLeg; // Leg that is used to timetag the measurement at time of reception
   RangeAdapterKm* otherLeg; // Leg that is subtracted from referenceLeg to produce the measurement

protected:
   void                 ComputeMeasurementBias(const std::string biasName, const std::string measType, Integer numTrip);
   void                 ComputeMeasurementNoiseSigma(const std::string noiseSigmaName, const std::string measType, Integer numTrip);
   
private:
   // Measurement data for reference leg and other leg
   MeasurementData measDataRef;
   MeasurementData measDataOther;
};

#endif /* DeltaRangeAdapter_hpp */
