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
// contract, Task Order 28
//
// Author: Daniel Estevez <daniel@destevez.net>
// Created: May 24, 2020
//
// Based on GNDopplerAdapter
/**
 * A measurement adapter for deltarange rate measurement
 */
//------------------------------------------------------------------------------

#include "DeltaRangeRateAdapter.hpp"
#include "MeasurementException.hpp"
#include "MessageInterface.hpp"
#include "SignalBase.hpp"
#include "Transponder.hpp"
#include "PropSetup.hpp"
#include "RandomNumber.hpp"
#include "ErrorModel.hpp"
#include <sstream>

//#define DEBUG_ADAPTER_EXECUTION
//#define DEBUG_ADAPTER_DERIVATIVES
//#define DEBUG_DERIVATIVE_CALCULATION
//#define DEBUG_SET_PARAMETER
//#define DEBUG_CONSTRUCTION
//#define DEBUG_INITIALIZE
//#define DEBUG_DOPPLER_CALCULATION
//#define DEBUG_RANGERATE_CALCULATION

//------------------------------------------------------------------------------
// Static data
//------------------------------------------------------------------------------
const std::string
DeltaRangeRateAdapter::PARAMETER_TEXT[DeltaRangeRateAdapterParamCount - DeltaRangeAdapterParamCount] =
{
   "DopplerCountInterval",
};


const Gmat::ParameterType
DeltaRangeRateAdapter::PARAMETER_TYPE[DeltaRangeRateAdapterParamCount - DeltaRangeAdapterParamCount] =
{
   Gmat::REAL_TYPE,
};




//------------------------------------------------------------------------------
// DeltaRangeRateAdapter(const std::string& name)
//------------------------------------------------------------------------------
/**
 * Constructor
 *
 * @param name The name of the adapter
 */
//------------------------------------------------------------------------------
DeltaRangeRateAdapter::DeltaRangeRateAdapter(const std::string& name) :
   DeltaRangeAdapter      (name),
   adapterS               (NULL),
   dopplerCountInterval   (1.0),        // unit: 1 second
   multiplierS            (1.0),        // unit: 1/1second
   multiplierE            (1.0)         // unit: 1/1second
{
#ifdef DEBUG_CONSTRUCTION
   MessageInterface::ShowMessage("DeltaRangeRateAdapter default constructor <%p>\n", this);
#endif
   typeName = "RangeRate";  //"Doppler_RangeRate";              // change type name from "RangeKm" to ("Doppler_RangeRate") "RangeRate"
}


//------------------------------------------------------------------------------
// ~DeltaRangeRateAdapter()
//------------------------------------------------------------------------------
/**
 * Destructor
 */
//------------------------------------------------------------------------------
DeltaRangeRateAdapter::~DeltaRangeRateAdapter()
{
#ifdef DEBUG_CONSTRUCTION
   MessageInterface::ShowMessage("DeltaRangeRateAdapter default destructor <%p>\n", this);
#endif

   if (adapterS)
   {
      // prevent deletion of adapterS from deleting its calcData
      // which we share as this->calcData
      // we create and assign a new MeasureModel just to be destroyed
      adapterS->SetMeasurement(new MeasureModel("throwaway"));
      delete adapterS;
   }
}


//------------------------------------------------------------------------------
// DopplerAdapter(const DopplerAdapter& da)
//------------------------------------------------------------------------------
/**
 * Copy constructor
 *
 * @param da The adapter copied to make this one
 */
//------------------------------------------------------------------------------
DeltaRangeRateAdapter::DeltaRangeRateAdapter(const DeltaRangeRateAdapter& da) :
   DeltaRangeAdapter      (da),
   //adapterS               (NULL),
   dopplerCountInterval   (da.dopplerCountInterval),
   multiplierS            (da.multiplierS),
   multiplierE            (da.multiplierE)
{
#ifdef DEBUG_CONSTRUCTION
   MessageInterface::ShowMessage("DeltaRangeRateAdapter copy constructor   from <%p> to <%p>\n", &da, this);
#endif
   if (adapterS)
   {
      delete adapterS;
      adapterS = NULL;
   }
   if (da.adapterS)
      adapterS = (DeltaRangeAdapter*)da.adapterS->Clone();
}


//------------------------------------------------------------------------------
// DeltaRangeRateAdapter& operator=(const DeltaRangeRateAdapter& da)
//------------------------------------------------------------------------------
/**
 * Assignment operator
 *
 * @param da The adapter copied to make this one match it
 *
 * @return This adapter made to look like da
 */
//------------------------------------------------------------------------------
DeltaRangeRateAdapter& DeltaRangeRateAdapter::operator=(const DeltaRangeRateAdapter& da)
{
#ifdef DEBUG_CONSTRUCTION
   MessageInterface::ShowMessage("DeltaRangeRateAdapter operator =   set <%p> = <%p>\n", this, &da);
#endif

   if (this != &da)
   {
      DeltaRangeAdapter::operator=(da);

      dopplerCountInterval = da.dopplerCountInterval;
      multiplierS          = da.multiplierS;
      multiplierE          = da.multiplierE;

      if (adapterS)
      {
         delete adapterS;
         adapterS = NULL;
      }
      if (da.adapterS)
         adapterS = (DeltaRangeAdapter*)da.adapterS->Clone();
   }

   return *this;
}


//------------------------------------------------------------------------------
// void SetSolarSystem(SolarSystem *ss)
//------------------------------------------------------------------------------
/**
 * Sets the solar system pointer
 *
 * @param ss The pointer
 */
//------------------------------------------------------------------------------
void DeltaRangeRateAdapter::SetSolarSystem(SolarSystem *ss)
{
#ifdef DEBUG_SET_PARAMETER
   MessageInterface::ShowMessage("DeltaRangeRateAdapter<%p>::SetSolarSystem('%s')\n", this, ss->GetName().c_str()); 
#endif

   adapterS->SetSolarSystem(ss);

   DeltaRangeAdapter::SetSolarSystem(ss);
}


//------------------------------------------------------------------------------
// GmatBase* Clone() const
//------------------------------------------------------------------------------
/**
 * Creates a new adapter that matches this one
 *
 * @return A new adapter set to match this one
 */
//------------------------------------------------------------------------------
GmatBase* DeltaRangeRateAdapter::Clone() const
{
#ifdef DEBUG_CONSTRUCTION
   MessageInterface::ShowMessage("DeltaRangeRateAdapter::Clone()   clone this <%p>\n", this);
#endif

   return new DeltaRangeRateAdapter(*this);
}


//------------------------------------------------------------------------------
// std::string GetParameterText(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Returns the script name for the parameter
 *
 * @param id The id of the parameter
 *
 * @return The script name
 */
//------------------------------------------------------------------------------
std::string DeltaRangeRateAdapter::GetParameterText(const Integer id) const
{
   if (id >= DeltaRangeAdapterParamCount && id < DeltaRangeRateAdapterParamCount)
      return PARAMETER_TEXT[id - DeltaRangeAdapterParamCount];
   return DeltaRangeAdapter::GetParameterText(id);
}


//------------------------------------------------------------------------------
// Integer GetParameterID(const std::string& str) const
//------------------------------------------------------------------------------
/**
 * Retrieves the ID for a scriptable parameter
 *
 * @param str The string used for the parameter
 *
 * @return The parameter ID
 */
//------------------------------------------------------------------------------
Integer DeltaRangeRateAdapter::GetParameterID(const std::string& str) const
{
   for (Integer i = DeltaRangeAdapterParamCount; i < DeltaRangeRateAdapterParamCount; i++)
   {
      if (str == PARAMETER_TEXT[i - DeltaRangeAdapterParamCount])
         return i;
   }
   return DeltaRangeAdapter::GetParameterID(str);
}


//------------------------------------------------------------------------------
// Gmat::ParameterType GetParameterType(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Retrieves the type for a specified parameter
 *
 * @param id The ID for the parameter
 *
 * @return The parameter's type
 */
//------------------------------------------------------------------------------
Gmat::ParameterType DeltaRangeRateAdapter::GetParameterType(const Integer id) const
{
   if (id >= DeltaRangeAdapterParamCount && id < DeltaRangeRateAdapterParamCount)
      return PARAMETER_TYPE[id - DeltaRangeAdapterParamCount];

   return DeltaRangeAdapter::GetParameterType(id);
}


//------------------------------------------------------------------------------
// std::string GetParameterTypeString(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Retrieves a text description for a parameter
 *
 * @param id The ID for the parameter
 *
 * @return The description string
 */
//------------------------------------------------------------------------------
std::string DeltaRangeRateAdapter::GetParameterTypeString(const Integer id) const
{
   return DeltaRangeAdapter::PARAM_TYPE_STRING[GetParameterType(id)];
}


//------------------------------------------------------------------------------
// bool SetStringParameter(const Integer id, const std::string& value)
//------------------------------------------------------------------------------
/**
 * Sets the value for a string parameter
 *
 * @param id The ID for the parameter
 * @param value The value for the parameter
 *
 * @return true if the paramter was set, false if not
 */
//------------------------------------------------------------------------------
bool DeltaRangeRateAdapter::SetStringParameter(const Integer id, const std::string& value)
{
   // Note that: measurement type of adapter is always ("Doppler_RangeRate") "RangeRate", so it does not need to change
   bool retval = true;
   MeasureModel* tempCalcData;
   MeasureModel* oldCalcData;

   if (id == SIGNAL_PATH)
   {
      // put temporarily a cloned calcData in adapterS to prevent
      // adding to the strand
      // (adapterS and this share the same calcData)
      tempCalcData = (MeasureModel*)adapterS->GetMeasurementModel()->Clone();
      oldCalcData = adapterS->GetMeasurementModel();
      adapterS->SetMeasurement(tempCalcData);
   }
      
   if ((id != MEASUREMENT_TYPE))
      retval = adapterS->SetStringParameter(id, value);

   if (id == SIGNAL_PATH)
   {
      // restore things
      adapterS->SetMeasurement(oldCalcData);
      delete tempCalcData;
   }

   retval = DeltaRangeAdapter::SetStringParameter(id, value) && retval;

   return retval; 
}


//------------------------------------------------------------------------------
// bool SetStringParameter(const Integer id, const std::string& value,
//       const Integer index)
//------------------------------------------------------------------------------
/**
 * Sets a string parameter in an array of strings
 *
 * @param id The ID for the parameter
 * @param value The new value for the parameter
 * @index The desired location of the parameter in the array
 *
 * @return true if the parameter was set, false if not
 */
//------------------------------------------------------------------------------
bool DeltaRangeRateAdapter::SetStringParameter(const Integer id,
      const std::string& value, const Integer index)
{
   bool retval = adapterS->SetStringParameter(id, value, index);
   retval = DeltaRangeAdapter::SetStringParameter(id, value, index) && retval;

   return retval;
}


//------------------------------------------------------------------------------
// bool SetStringParameter(const std::string& label, const std::string& value)
//------------------------------------------------------------------------------
/**
 * Sets the value for a string parameter
 *
 * @param label The scriptable name of the parameter
 * @param value The value for the parameter
 *
 * @return true if the paramter was set, false if not
 */
//------------------------------------------------------------------------------
bool DeltaRangeRateAdapter::SetStringParameter(const std::string& label,
      const std::string& value)
{
   return SetStringParameter(GetParameterID(label), value);
}


//------------------------------------------------------------------------------
// bool SetStringParameter(const std::string& label, const std::string& value,
//       const Integer index)
//------------------------------------------------------------------------------
/**
 * Sets a string parameter in an array of strings
 *
 * @param label The scriptable name of the parameter
 * @param value The new value for the parameter
 * @index The desired location of the parameter in the array
 *
 * @return true if the parameter was set, false if not
 */
//------------------------------------------------------------------------------
bool DeltaRangeRateAdapter::SetStringParameter(const std::string& label,
      const std::string& value, const Integer index)
{
   return SetStringParameter(GetParameterID(label), value, index);
}


//------------------------------------------------------------------------------
// bool SetIntegerParameter(const Integer id, const Integer value)
//------------------------------------------------------------------------------
/**
 * Sets the value for an integer parameter
 *
 * @param id The ID for the parameter
 * @param value The value for the parameter
 *
 * @return setting value
 */
//------------------------------------------------------------------------------
Integer DeltaRangeRateAdapter::SetIntegerParameter(const Integer id, const Integer value)
{
   adapterS->SetIntegerParameter(id, value);
   return DeltaRangeAdapter::SetIntegerParameter(id, value);
}


//------------------------------------------------------------------------------
// bool SetIntegerParameter(const std::string &label, const Integer value)
//------------------------------------------------------------------------------
/**
 * Sets the value for an integer parameter
 *
 * @param label The name for the parameter
 * @param value The value for the parameter
 *
 * @return setting value
 */
//------------------------------------------------------------------------------
Integer DeltaRangeRateAdapter::SetIntegerParameter(const std::string &label, const Integer value)
{
   return SetIntegerParameter(GetParameterID(label), value);
}


//------------------------------------------------------------------------------
// std::string GetRealParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Retrieves the value of a real parameter
 *
 * @param id The ID for the parameter
 *
 * @return The value of the parameter
 */
//------------------------------------------------------------------------------
Real DeltaRangeRateAdapter::GetRealParameter(const Integer id) const
{
   if (id == DOPPLER_COUNT_INTERVAL)
      return dopplerCountInterval;

   return DeltaRangeAdapter::GetRealParameter(id);
}


//------------------------------------------------------------------------------
// bool SetRealParameter(const Integer id, const Real value)
//------------------------------------------------------------------------------
/**
 * Sets the value for a real parameter
 *
 * @param id The ID for the parameter
 * @param value The value for the parameter
 *
 * @return setting value
 */
//------------------------------------------------------------------------------
Real DeltaRangeRateAdapter::SetRealParameter(const Integer id, const Real value)
{
   if (id == DOPPLER_COUNT_INTERVAL)
   {
      if (value <= 0.0)
         throw MeasurementException("Error: Doppler count interval has a nonpositive value\n");

      dopplerCountInterval = value;
      return dopplerCountInterval;
   }

   bool retval = adapterS->SetRealParameter(id, value);
   retval = DeltaRangeAdapter::SetRealParameter(id, value) && retval;

   return retval;
}


//------------------------------------------------------------------------------
// std::string GetRealParameter(const std::string &label) const
//------------------------------------------------------------------------------
/**
 * Retrieves the value of a real parameter
 *
 * @param label The name for the parameter
 *
 * @return The value of the parameter
 */
//------------------------------------------------------------------------------
Real DeltaRangeRateAdapter::GetRealParameter(const std::string &label) const
{
   return GetRealParameter(GetParameterID(label));
}


//------------------------------------------------------------------------------
// bool SetRealParameter(const std::string &label, const Real value)
//------------------------------------------------------------------------------
/**
 * Sets the value for a real parameter
 *
 * @param label The name for the parameter
 * @param value The value for the parameter
 *
 * @return setting value
 */
//------------------------------------------------------------------------------
Real DeltaRangeRateAdapter::SetRealParameter(const std::string &label, const Real value)
{
   return SetRealParameter(GetParameterID(label), value);
}


bool DeltaRangeRateAdapter::SetBooleanParameter(const Integer id, const bool value)
{
   // Note: for Start path, AddNoise always is set to false due to it calculation
   bool retval = true;
   if (id == ADD_NOISE)
      retval = adapterS->SetBooleanParameter(id, false);
   else
      retval = adapterS->SetBooleanParameter(id, value);

   retval = DeltaRangeAdapter::SetBooleanParameter(id, value) && retval;

   return retval;
}


bool DeltaRangeRateAdapter::SetBooleanParameter(const std::string &label, const bool value)
{
   return SetBooleanParameter(GetParameterID(label), value);
}


//------------------------------------------------------------------------------
// bool RenameRefObject(const UnsignedInt type, const std::string& oldName,
//       const std::string& newName)
//------------------------------------------------------------------------------
/**
 * Method used to rename reference objects
 *
 * @param type The type of the object that is renamed
 * @param oldName The old object name
 * @param newName The new name
 *
 * @return true if a rename happened, false if not
 */
//------------------------------------------------------------------------------
bool DeltaRangeRateAdapter::RenameRefObject(const UnsignedInt type,
      const std::string& oldName, const std::string& newName)
{
   // Handle additional renames specific to this adapter
   bool retval = adapterS->RenameRefObject(type, oldName, newName);
   retval = DeltaRangeAdapter::RenameRefObject(type, oldName, newName) && retval;

   return retval;
}


//------------------------------------------------------------------------------
// bool SetRefObject(GmatBase* obj, const UnsignedInt type,
//       const std::string& name)
//------------------------------------------------------------------------------
/**
 * Sets pointers to the model's reference objects
 *
 * @param obj The object pointer
 * @param type The type of the object (not used)
 * @param name The name of the object  (not used)
 *
 * @return true if the pointer was set, false if not
 */
//------------------------------------------------------------------------------
bool DeltaRangeRateAdapter::SetRefObject(GmatBase* obj,
      const UnsignedInt type, const std::string& name)
{
   bool retval = adapterS->SetRefObject(obj, type, name);
   retval = DeltaRangeAdapter::SetRefObject(obj, type, name) && retval;

   return retval; 
}

//------------------------------------------------------------------------------
// bool SetRefObject(GmatBase* obj, const UnsignedInt type,
//       const std::string& name, const Integer index)
//------------------------------------------------------------------------------
/**
 * Sets the pointers for the reference object
 *
 * @param obj The object pointer
 * @param type The type of the object (not used)
 * @param name The name of the object  (not used)
 * @param index Index for the object's location
 *
 * @return true if the pointer was set, false if not
 */
//------------------------------------------------------------------------------
bool DeltaRangeRateAdapter::SetRefObject(GmatBase* obj,
      const UnsignedInt type, const std::string& name, const Integer index)
{
   bool retval = adapterS->SetRefObject(obj, type, name, index);
   retval = DeltaRangeAdapter::SetRefObject(obj, type, name, index) && retval;

   return retval;
}

//------------------------------------------------------------------------------
// bool SetMeasurement(MeasureModel* meas)
//------------------------------------------------------------------------------
/**
 * Sets the measurement model pointer
 *
 * @param meas The pointer
 *
 * @return true if set, false if not
 */
//------------------------------------------------------------------------------
bool DeltaRangeRateAdapter::SetMeasurement(MeasureModel* meas)
{
   bool retval = adapterS->SetMeasurement(meas);
   retval = DeltaRangeAdapter::SetMeasurement(meas) && retval;

   return retval;
}


//------------------------------------------------------------------------------
// void SetPropagator(PropSetup* ps)
//------------------------------------------------------------------------------
/**
 * Passes a propagator to the adapter for use in light time iterations.  The
 * propagator is cloned so that propagation of single spacecraft can be
 * performed.
 *
 * @param ps The PropSetup that is being set
 *
 * @todo The current call takes a single propagator.  Once the estimation system
 *       supports multiple propagators, this should be changed to a vector of
 *       PropSetup objects.
 */
//------------------------------------------------------------------------------
void DeltaRangeRateAdapter::SetPropagator(PropSetup* ps)
{
   #ifdef DEBUG_INITIALIZATION
      MessageInterface::ShowMessage("Setting propagator to %p in "
            "DeltaRangeRateAdapter\n", ps);
   #endif

   adapterS->SetPropagator(ps);
   DeltaRangeAdapter::SetPropagator(ps);
}


//------------------------------------------------------------------------------
//  void SetTransientForces(std::vector<PhysicalModel*> *tf)
//------------------------------------------------------------------------------
/**
* Passes the transient force vector into the adapter
*
* The transient force vector is a set of models used in GMAT's ODEModel for
* affects that are turned on and off over the course of a mission.  An example
* of a transient force is a finite burn, which is toggled by the
* BeginFiniteBurn and EndFiniteBurn commands.  These components are only used
* by commands that need them.  Typical usage is found in the propagation
* enabled commands.
*
* @param tf The vector of transient forces
*/
//------------------------------------------------------------------------------
void DeltaRangeRateAdapter::SetTransientForces(std::vector<PhysicalModel*> *tf)
{
   DeltaRangeAdapter::SetTransientForces(tf);
   adapterS->SetTransientForces(tf);
}


//------------------------------------------------------------------------------
// bool Initialize()
//------------------------------------------------------------------------------
/**
 * Prepares the adapter for use
 *
 * @return true if the initialization succeeds, false if it fails
 */
//------------------------------------------------------------------------------
bool DeltaRangeRateAdapter::Initialize()
{
#ifdef DEBUG_INITIALIZE
   MessageInterface::ShowMessage("DeltaRangeRateAdapter::Initialize() <%p> start\n", this);
#endif

   bool retval = false;

   if (DeltaRangeAdapter::Initialize())
   {
      // @todo: initialize all needed variables

      retval = adapterS->Initialize();
   }

#ifdef DEBUG_INITIALIZE
   MessageInterface::ShowMessage("DeltaRangeRateAdapter::Initialize() <%p> exit\n", this);
#endif
   return retval;
}


//------------------------------------------------------------------------------
// const MeasurementData& CalculateMeasurement(bool withEvents,
//       ObservationData* forObservation, std::vector<RampTableData>* rampTB)
//------------------------------------------------------------------------------
/**
 * Computes the measurement associated with this adapter
 *
 * @note: The parameters associated with this call will probably be removed;
 * they are here to support compatibility with the old measurement models
 *
 * @param withEvents Flag indicating is the light time solution should be
 *                   included
 * @param forObservation The observation data associated with this measurement
 * @param rampTB Ramp table for a ramped measurement
 *
 * @return The computed measurement data
 */
//------------------------------------------------------------------------------
const MeasurementData& DeltaRangeRateAdapter::CalculateMeasurement(bool withEvents,
      ObservationData* forObservation, std::vector<RampTableData>* rampTable,
      bool forSimulation)
{
   #ifdef DEBUG_RANGERATE_CALCULATION
      MessageInterface::ShowMessage("DeltaRangeRateAdapter::CalculateMeasurement(%s, "
            "<%p>, <%p>) called\n", (withEvents ? "true" : "false"), forObservation,
            rampTable);
   #endif

   obsData = forObservation;
   // Reset value for doppler count interval
   if (obsData)
      dopplerCountInterval = obsData->dopplerCountInterval;          // unit: second

   // Compute for End path
   #ifdef DEBUG_RANGERATE_CALCULATION
      MessageInterface::ShowMessage("Compute range for E-Path...\n");
   #endif
   bool addNoiseOption  = addNoise;
   bool addBiasOption   = addBias;
   bool rangeOnlyOption = rangeOnly; 

   addNoise = false;
   addBias = false;
   rangeOnly = true;
   DeltaRangeAdapter::CalculateMeasurement(withEvents, forObservation, rampTB, forSimulation);
   measDataE = cMeasurement;
   // Ionosphere phase delay is opposite from group delay
   measDataE.value[0] = measDataE.value[0] - 2 * GetIonoCorrection();
   measDataE.correction[0] = measDataE.correction[0] - 2 * GetIonoCorrection();
   
   addNoise = addNoiseOption;
   addBias = addBiasOption;
   rangeOnly = rangeOnlyOption;
   
   // Compute for Start path
   #ifdef DEBUG_RANGERATE_CALCULATION
      MessageInterface::ShowMessage("Compute range for S-Path...\n");
   #endif
   // Measurement time isthe same one as for the End path
   GmatTime tm = cMeasurement.epochGT;       // Get measurement time
   ObservationData* obData = NULL;
   if (forObservation)
      obData = new ObservationData(*forObservation);
   else
      obData = new ObservationData();
   obData->epochGT = tm;
   obData->epoch   = tm.GetMjd();

   // Set Doppler count interval
   // Start path is measured earlier by number of seconds shown in Doppler count interval
   adapterS->referenceLeg->GetMeasurementModel()->SetCountInterval(dopplerCountInterval);
   // adapterS->otherLeg does not need dopplerCountInterval because its transmission
   // time comes determined by the transmission time of adapterS->referenceLeg
   
   // For Start-path, range calculation does not add bias and noise to calculated value
   // Note that: default option is no adding noise
   adapterS->AddBias(false);
   adapterS->AddNoise(false);
   adapterS->SetRangeOnly(true);
   
   adapterS->CalculateMeasurement(withEvents, obData, rampTB, forSimulation);
   
   if (obData)
      delete obData;
   
   measDataS = adapterS->GetMeasurement();
   measDataS.value[0] = measDataS.value[0] - 2 * adapterS->GetIonoCorrection();
   measDataS.correction[0] = measDataS.correction[0] - 2 * adapterS->GetIonoCorrection();

   // Set value for isFeasible, feasibilityValue, and unfeasibleReason for measurement
   if ((measDataE.unfeasibleReason.at(0) == 'B') || (measDataS.unfeasibleReason.at(0) == 'B'))
   {
      if (measDataE.unfeasibleReason.at(0) == 'B')
         cMeasurement.unfeasibleReason = measDataE.unfeasibleReason + "E";
      else
      {
         cMeasurement.unfeasibleReason = measDataS.unfeasibleReason + "S";
         cMeasurement.isFeasible = false;
         cMeasurement.feasibilityValue = measDataS.feasibilityValue;
      }
   }

   Real dtS, dtE, dtdt;

   std::vector<SignalBase*> paths = calcData->GetSignalPaths();
   for (UnsignedInt i = 0; i < paths.size(); ++i)          // In the current version of GmatEstimation plugin, it has only 1 signal path. The code has to be modified for multiple signal paths.
   {
      dtS  = measDataS.value[0];              // real travel length for S-path   (unit: Km)
      dtE  = measDataE.value[0];              // real travel length for E-path   (unit: Km)

      // TODO determine if we need something like
      // USE_TAYLOR_SERIES or USE_CHEBYSHEV_DIFFERENCE
      // in GNDopplerAdapter
      dtdt = dtE - dtS;                       // real travel difference          (unit: Km)

      cMeasurement.value[i] = dtdt/dopplerCountInterval;  // deltarange rate     (unit: km/s)

      // Update media corrections
      cMeasurement.ionoCorrectValue = -(GetIonoCorrection() - adapterS->GetIonoCorrection()) / dopplerCountInterval;
      cMeasurement.tropoCorrectValue = (GetTropoCorrection() - adapterS->GetTropoCorrection()) / dopplerCountInterval;

      cMeasurement.dopplerCountInterval = dopplerCountInterval;

      // TODO: implement noise and bias
      //  Add noise and bias if possible
      Real C_idealVal = cMeasurement.value[i];

      if (measurementType == "DeltaRangeRate")
      {         
         ComputeMeasurementBias("Bias", "DeltaRangeRate", 1);
         ComputeMeasurementNoiseSigma("NoiseSigma", "DeltaRangeRate", 1);
         ComputeMeasurementErrorCovarianceMatrix();

         // if need range value only, skip this section, otherwise add noise and bias as possible
         if (!rangeOnly)
         {
            // Add noise to measurement value
            if (addNoise)
            {
               // Add noise here
               if (cMeasurement.unfeasibleReason != "R")
               {
                  RandomNumber* rn = RandomNumber::Instance();
                  Real val = rn->Gaussian(cMeasurement.value[i], noiseSigma[i]);
                  cMeasurement.value[i] = val;
               }
            }
         
            //Add bias to measurement value only after noise had been added in order to avoid adding bias' noise 
            if (addBias)
            {
               #ifdef DEBUG_RANGERATE_CALCULATION
                  MessageInterface::ShowMessage("      . Add bias...\n");
               #endif
               cMeasurement.value[i] = cMeasurement.value[i] + measurementBias[i];
            }
         }
      }

      #ifdef DEBUG_RANGERATE_CALCULATION
         MessageInterface::ShowMessage("===================================================================\n");
         MessageInterface::ShowMessage("====  DeltaRangeRateAdapter: Range Calculation for Measurement Data %dth\n", i);
         MessageInterface::ShowMessage("===================================================================\n");
         MessageInterface::ShowMessage("      . Measurement epoch             : %.12lf\n", cMeasurement.epochGT.GetMjd());
         MessageInterface::ShowMessage("      . Measurement type              : <%s>\n", measurementType.c_str());
         MessageInterface::ShowMessage("      . Noise adding option           : %s\n", (addNoise?"true":"false"));
         MessageInterface::ShowMessage("      . Doppler count interval        : %.12lf seconds\n", dopplerCountInterval);
         MessageInterface::ShowMessage("      . Real travel lenght for S-path : %.12lf Km\n", dtS);
         MessageInterface::ShowMessage("      . Real travel lenght for E-path : %.12lf Km\n", dtE);
         MessageInterface::ShowMessage("      . Travel length difference dtdt : %.12lf Km\n", dtdt);
         MessageInterface::ShowMessage("      . Ramp table is %s used\n", (rampTable?"":"not"));
         MessageInterface::ShowMessage("      . Multiplier factor for S-path  : %.12lf\n", multiplierS);
         MessageInterface::ShowMessage("      . Multiplier factor for E-path  : %.12lf\n", multiplierE);
         MessageInterface::ShowMessage("      . C-value w/o noise and bias    : %.12lf Km/s\n", C_idealVal);
	 MessageInterface::ShowMessage("      . DeltaRangeRate noise sigma      : %.12lf Km/s \n", noiseSigma[i]);
	 MessageInterface::ShowMessage("      . DeltaRangeRate bias             : %.12lf Km/s \n", measurementBias[i]);
         MessageInterface::ShowMessage("      . C-value with noise and bias   : %.12lf Km/s\n", cMeasurement.value[i]);
         MessageInterface::ShowMessage("      . Measurement epoch A1Mjd       : %.12lf\n", cMeasurement.epoch); 
         MessageInterface::ShowMessage("      . Measurement is %s\n", (cMeasurement.isFeasible?"feasible":"unfeasible"));
         MessageInterface::ShowMessage("      . Feasibility reason            : %s\n", cMeasurement.unfeasibleReason.c_str());
         MessageInterface::ShowMessage("      . Elevation angle               : %.12lf degree\n", cMeasurement.feasibilityValue);
         MessageInterface::ShowMessage("      . Covariance matrix             : <%p>\n", cMeasurement.covariance);
         if (cMeasurement.covariance)
         {
            MessageInterface::ShowMessage("      . Covariance matrix size = %d\n", cMeasurement.covariance->GetDimension());
            MessageInterface::ShowMessage("     [ ");
            for (UnsignedInt i = 0; i < cMeasurement.covariance->GetDimension(); ++i)
            {
               if ( i > 0)
                  MessageInterface::ShowMessage("\n");
               for (UnsignedInt j = 0; j < cMeasurement.covariance->GetDimension(); ++j)
                  MessageInterface::ShowMessage("%le   ", cMeasurement.covariance->GetCovariance()->GetElement(i,j));
            }
            MessageInterface::ShowMessage("]\n");
         }
         MessageInterface::ShowMessage("===================================================================\n");
      #endif

   }

   #ifdef DEBUG_RANGERATE_CALCULATION
      MessageInterface::ShowMessage("DeltaRangeRateAdapter::CalculateMeasurement(%s, "
            "<%p>, <%p>) exit\n", (withEvents ? "true" : "false"), forObservation,
            rampTable);
   #endif

   return cMeasurement;
}


//------------------------------------------------------------------------------
// const std::vector<RealArray>& CalculateMeasurementDerivatives(GmatBase* obj,
//       Integer id)
//------------------------------------------------------------------------------
/**
 * Computes measurement derivatives for a given parameter on a given object
 *
 * @param obj The object that has the w.r.t. parameter
 * @param id  The ID of the w.r.t. parameter
 *
 * @return The derivative vector
 */
//------------------------------------------------------------------------------
const std::vector<RealArray>& DeltaRangeRateAdapter::CalculateMeasurementDerivatives(
      GmatBase* obj, Integer id)
{
   if (!calcData)
      throw MeasurementException("Measurement derivative data was requested "
            "for " + instanceName + " before the measurement was set");

   Integer parmId = GetParmIdFromEstID(id, obj);
   #ifdef DEBUG_DERIVATIVE_CALCULATION
      MessageInterface::ShowMessage("Enter DeltaRangeRateAdapter::CalculateMeasurementDerivatives(%s, %d) called; parm ID is %d; Epoch %.12lf\n", obj->GetFullName().c_str(), id, parmId, cMeasurement.epoch);
   #endif
   
   // Get parameter name specified by id
   Integer parameterID;
   if (id > 250)
      parameterID = id - obj->GetType() * 250; // GetParmIdFromEstID(id, obj);
   else
      parameterID = id;
   std::string paramName = obj->GetParameterText(parameterID);

   #ifdef DEBUG_DERIVATIVE_CALCULATION
      MessageInterface::ShowMessage("Solve-for parameter: %s\n", paramName.c_str());
   #endif

   // Clear derivative variable
   for (UnsignedInt i = 0; i < theDataDerivatives.size(); ++i)
      theDataDerivatives[i].clear();
   theDataDerivatives.clear();

   if (paramName == "Bias")
   {
      //if (((ErrorModel*)obj)->GetStringParameter("Type") == "Doppler_RangeRate")
      if (((ErrorModel*)obj)->GetStringParameter("Type") == "RangeRate")
         theDataDerivatives = calcData->CalculateMeasurementDerivatives(obj, id);
      else
      {
         Integer size = obj->GetEstimationParameterSize(id);
         RealArray oneRow;
         oneRow.assign(size, 0.0);
         theDataDerivatives.push_back(oneRow);
      }
   }
   else
   {
      // Perform the calculations
      const std::vector<RealArray> *derivativeDataE =
         &(DeltaRangeAdapter::CalculateMeasurementDerivatives(obj, id));

      const std::vector<RealArray> *derivativeDataS =
         &(adapterS->CalculateMeasurementDerivatives(obj, id));

      #ifdef DEBUG_ADAPTER_DERIVATIVES
      MessageInterface::ShowMessage("   Derivatives E-path: [");
      for (UnsignedInt i = 0; i < derivativeDataE->size(); ++i)
      {
         if (i > 0)
            MessageInterface::ShowMessage("]\n                [");
         for (UnsignedInt j = 0; j < derivativeDataE->at(i).size(); ++j)
         {
            if (j > 0)
               MessageInterface::ShowMessage(", ");
            MessageInterface::ShowMessage("%.12le", (derivativeDataE->at(i))[j]);
         }
      }
      MessageInterface::ShowMessage("]\n");
      MessageInterface::ShowMessage("   Haft of Derivatives S-path: [");
      for (UnsignedInt i = 0; i < derivativeDataS->size(); ++i)
      {
         if (i > 0)
            MessageInterface::ShowMessage("]\n                [");
         for (UnsignedInt j = 0; j < derivativeDataS->at(i).size(); ++j)
         {
            if (j > 0)
               MessageInterface::ShowMessage(", ");
            MessageInterface::ShowMessage("%.12le", (derivativeDataS->at(i))[j]);
         }
      }
      MessageInterface::ShowMessage("]\n");
      #endif

      // copy S and E paths' derivatives
      UnsignedInt size = derivativeDataE->at(0).size();
      std::vector<RealArray> derivativesE;
      for (UnsignedInt i = 0; i < derivativeDataE->size(); ++i)
      {
         RealArray oneRow;
         oneRow.assign(size, 0.0);
         derivativesE.push_back(oneRow);

         if (derivativeDataE->at(i).size() != size)
            throw MeasurementException("Derivative data size is a different size "
               "than expected");

         for (UnsignedInt j = 0; j < size; ++j)
         {
            derivativesE[i][j] = (derivativeDataE->at(i))[j];
         }
      }
   
      size = derivativeDataS->at(0).size();
      std::vector<RealArray> derivativesS;
      for (UnsignedInt i = 0; i < derivativeDataS->size(); ++i)
      {
         RealArray oneRow;
         oneRow.assign(size, 0.0);
         derivativesS.push_back(oneRow);

         if (derivativeDataS->at(i).size() != size)
            throw MeasurementException("Derivative data size is a different size "
               "than expected");

         for (UnsignedInt j = 0; j < size; ++j)
         {
	    derivativesS[i][j] = (derivativeDataS->at(i))[j];
         }
      }

      #ifdef DEBUG_ADAPTER_DERIVATIVES
      MessageInterface::ShowMessage("   Derivatives E-path: [");
      for (UnsignedInt i = 0; i < derivativesE.size(); ++i)
      {
         if (i > 0)
            MessageInterface::ShowMessage("]\n                [");
         for (UnsignedInt j = 0; j < derivativesE[i].size(); ++j)
         {
            if (j > 0)
               MessageInterface::ShowMessage(", ");
            MessageInterface::ShowMessage("%.12le", derivativesE[i][j]);
         }
      }
      MessageInterface::ShowMessage("] * multiplierE = %.12le\n", multiplierE);
      MessageInterface::ShowMessage("   Derivatives S-path: [");
      for (UnsignedInt i = 0; i < derivativesS.size(); ++i)
      {
         if (i > 0)
            MessageInterface::ShowMessage("]\n                [");
         for (UnsignedInt j = 0; j < derivativesS[i].size(); ++j)
         {
            if (j > 0)
               MessageInterface::ShowMessage(", ");
            MessageInterface::ShowMessage("%.12le", derivativesS[i][j]);
         }
      }
      MessageInterface::ShowMessage("] * multiplierS = %.12le\n", multiplierS);
      #endif

      // Now assemble the derivative data into the requested derivative
      size = derivativesE[0].size();

      //theDataDerivatives.clear();
      for (UnsignedInt i = 0; i < derivativesE.size(); ++i)
      {
         RealArray oneRow;
         oneRow.assign(size, 0.0);
         theDataDerivatives.push_back(oneRow);

         if (derivativesE[i].size() != size)
            throw MeasurementException("Derivative data size for E path is a different size "
               "than expected");
         if (derivativesS[i].size() != size)
            throw MeasurementException("Derivative data size for S path is a different size "
               "than expected");

         for (UnsignedInt j = 0; j < size; ++j)
         {
            if ((paramName == "Position")||(paramName == "Velocity")||(paramName == "CartesianX"))
            {
               // Convert measurement derivatives from km/s to Hz for velocity and position 
               theDataDerivatives[i][j] = derivativesE[i][j] * multiplierE - derivativesS[i][j] * multiplierS;
            }
            else
            {
               // set the same E path 's derivatives for Bias an other solve-for variables
               theDataDerivatives[i][j] = derivativesE[i][j];
            }
         }
      }
   }

   #ifdef DEBUG_DERIVATIVE_CALCULATION
      for (UnsignedInt i = 0; i < theDataDerivatives.size(); ++i)
      {
         MessageInterface::ShowMessage("Derivative for path %dth:\n", i);
         MessageInterface::ShowMessage("[");
         for (UnsignedInt j = 0; j < theDataDerivatives[i].size(); ++j)
         {
            MessageInterface::ShowMessage("    %.12lf", theDataDerivatives[i][j]);
            MessageInterface::ShowMessage("%s", ((j == theDataDerivatives[i].size()-1)?"":","));
         }
         MessageInterface::ShowMessage("]\n");
      }
      
   #endif

   #ifdef DEBUG_DERIVATIVE_CALCULATION
      MessageInterface::ShowMessage("Exit DeltaRangeRateAdapter::CalculateMeasurementDerivatives():\n");
   #endif

   return theDataDerivatives;
}


//------------------------------------------------------------------------------
// bool WriteMeasurements()
//------------------------------------------------------------------------------
/**
 * Method to write measurements
 *
 * @todo Implement this method
 *
 * @return true if written, false if not
 */
//------------------------------------------------------------------------------
bool DeltaRangeRateAdapter::WriteMeasurements()
{
   bool retval = false;

   return retval;
}


//------------------------------------------------------------------------------
// bool WriteMeasurement(Integer id)
//------------------------------------------------------------------------------
/**
 * Method to write a specific measurement
 *
 * @todo Implement this method
 *
 * @param id ID of the parameter to write
 *
 * @return true if written, false if not
 */
//------------------------------------------------------------------------------
bool DeltaRangeRateAdapter::WriteMeasurement(Integer id)
{
   bool retval = false;

   return retval;
}


//------------------------------------------------------------------------------
// Integer HasParameterCovariances(Integer parameterId)
//------------------------------------------------------------------------------
/**
 * Checks to see if adapter has covariance data for a specified parameter ID
 *
 * @param paramID The parameter ID
 *
 * @return Size of the covariance data that is available
 */
//------------------------------------------------------------------------------
Integer DeltaRangeRateAdapter::HasParameterCovariances(Integer parameterId)
{
   Integer retval = 0;

   return retval;
}


//------------------------------------------------------------------------------
// Integer GetEventCount()
//------------------------------------------------------------------------------
/**
 * Returns the number of light time solution events in the measurement
 *
 * @return The event count
 */
//------------------------------------------------------------------------------
Integer DeltaRangeRateAdapter::GetEventCount()
{
   Integer retval = 0;

   return retval;
}


//------------------------------------------------------------------------------
// void SetCorrection(const std::string& correctionName,
//       const std::string& correctionType)
//------------------------------------------------------------------------------
/**
 * Passes a correction name into the owned CoreMeasurement
 *
 * @param correctionName The name of the correction
 * @param correctionType The type of correction
 *
 * @note This information originally was not passed via SetStringParameter
 *       because it wasn't managed by scripting on MeasurementModels.  It was
 *       managed in the TrackingSystem code.  Since it is now scripted on the
 *       measurement -- meaning on the adapter -- this code should move into the
 *       Get/SetStringParameter methods.  It is included here !!TEMPORARILY!!
 *       to get a scripted adapter functioning in GMAT Nav.
 */
//------------------------------------------------------------------------------
void DeltaRangeRateAdapter::SetCorrection(const std::string& correctionName,
      const std::string& correctionType)
{
   adapterS->SetCorrection(correctionName, correctionType);
   DeltaRangeAdapter::SetCorrection(correctionName, correctionType);
}

