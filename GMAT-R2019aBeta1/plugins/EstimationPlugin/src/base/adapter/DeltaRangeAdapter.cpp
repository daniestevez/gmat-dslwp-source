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

#include "DeltaRangeAdapter.hpp"
#include "RandomNumber.hpp"
#include "MeasurementException.hpp"
#include "MessageInterface.hpp"
#include "SignalBase.hpp"
#include "ErrorModel.hpp"
#include "BodyFixedPoint.hpp"
#include "SpaceObject.hpp"
#include "PropSetup.hpp"
#include "ODEModel.hpp"
#include "GroundstationInterface.hpp"
#include <sstream>


//#define DEBUG_ADAPTER_EXECUTION
//#define DEBUG_ADAPTER_DERIVATIVES
//#define DEBUG_CONSTRUCTION
//#define DEBUG_SET_PARAMETER
//#define DEBUG_INITIALIZATION
#define DEBUG_RANGE_CALCULATION
//#define DEBUG_INITIALIZE

//------------------------------------------------------------------------------
// DeltaRangeAdapter(const std::string& name)
//------------------------------------------------------------------------------
/**
 * Constructor
 *
 * @param name The name of the adapter
 */
//------------------------------------------------------------------------------
DeltaRangeAdapter::DeltaRangeAdapter(const std::string& name) :
   RangeAdapterKm(name),
   referenceLeg(NULL),
   otherLeg(NULL)
{
#ifdef DEBUG_CONSTRUCTION
   MessageInterface::ShowMessage("DeltaRangeAdapter default constructor <%p>\n", this);
#endif

   typeName = "Range";           // change measurement type from "RangeKm" to "Range" for GN Range 
}


//------------------------------------------------------------------------------
// ~DeltaRangeAdapter()
//------------------------------------------------------------------------------
/**
 * Destructor
 */
//------------------------------------------------------------------------------
DeltaRangeAdapter::~DeltaRangeAdapter()
{
#ifdef DEBUG_CONSTRUCTION
   MessageInterface::ShowMessage("DeltaRangeAdapter default destructor  <%p>\n", this);
#endif

   if (referenceLeg)
     delete referenceLeg;
   if (otherLeg)
      delete otherLeg;
}


//------------------------------------------------------------------------------
// DeltaRangeAdapter(const DeltaRangeAdapter& rak)
//------------------------------------------------------------------------------
/**
 * Copy constructor
 *
 * @param rak The adapter copied to make this one
 */
//------------------------------------------------------------------------------
DeltaRangeAdapter::DeltaRangeAdapter(const DeltaRangeAdapter& rak) :
   RangeAdapterKm      (rak),
   referenceLeg       (NULL),
   otherLeg           (NULL)
{
#ifdef DEBUG_CONSTRUCTION
   MessageInterface::ShowMessage("DeltaRangeAdapter copy constructor   from <%p> to <%p>\n", &rak, this);
#endif

}


//------------------------------------------------------------------------------
// DeltaRangeAdapter& operator=(const DeltaRangeAdapter& rak)
//------------------------------------------------------------------------------
/**
 * Assignment operator
 *
 * @param rak The adapter copied to make this one match it
 *
 * @return This adapter made to look like rak
 */
//------------------------------------------------------------------------------
DeltaRangeAdapter& DeltaRangeAdapter::operator=(const DeltaRangeAdapter& rak)
{
#ifdef DEBUG_CONSTRUCTION
   MessageInterface::ShowMessage("DeltaRangeAdapter operator =   set <%p> = <%p>\n", this, &rak);
#endif

   if (this != &rak)
   {
      TrackingDataAdapter::operator=(rak);

      if (referenceLeg)
      {
	 delete referenceLeg;
	 referenceLeg = NULL;
      }
      if (rak.referenceLeg)
	 referenceLeg = (RangeAdapterKm*)rak.referenceLeg->Clone();
	 
      if (otherLeg)
      {
	 delete otherLeg;
	 otherLeg = NULL;
      }
      if (rak.otherLeg)
	 otherLeg = (RangeAdapterKm*)rak.otherLeg->Clone();
   }

   return *this;
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
GmatBase* DeltaRangeAdapter::Clone() const
{
#ifdef DEBUG_CONSTRUCTION
   MessageInterface::ShowMessage("DeltaRangeAdapter::Clone() clone this <%p>\n", this);
#endif

   return new DeltaRangeAdapter(*this);
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
void DeltaRangeAdapter::SetSolarSystem(SolarSystem *ss)
{
#ifdef DEBUG_SET_PARAMETER
   MessageInterface::ShowMessage("DeltaRangeAdapter<%p>::SetSolarSystem('%s')\n", this, ss->GetName().c_str()); 
#endif

   referenceLeg->SetSolarSystem(ss);
   otherLeg->SetSolarSystem(ss);

   RangeAdapterKm::SetSolarSystem(ss);
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
bool DeltaRangeAdapter::SetStringParameter(const Integer id, const std::string& value)
{
   // Note that: measurement type of adapter is always the same
   bool retval = true;
   if ((id != MEASUREMENT_TYPE)&&(id != SIGNAL_PATH))
   {
      retval = referenceLeg->SetStringParameter(id, value) && retval;
      retval = otherLeg->SetStringParameter(id, value) && retval;
   }

   retval = RangeAdapterKm::SetStringParameter(id, value) && retval;

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
bool DeltaRangeAdapter::SetStringParameter(const Integer id,
      const std::string& value, const Integer index)
{
   bool retval = true;
   retval = referenceLeg->SetStringParameter(id, value, index) && retval;
   retval = otherLeg->SetStringParameter(id, value, index) && retval;

   retval = RangeAdapterKm::SetStringParameter(id, value, index) && retval;

   return retval;
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
Integer DeltaRangeAdapter::SetIntegerParameter(const Integer id, const Integer value)
{
   referenceLeg->SetIntegerParameter(id, value);
   otherLeg->SetIntegerParameter(id, value);

   return RangeAdapterKm::SetIntegerParameter(id, value);
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
Real DeltaRangeAdapter::SetRealParameter(const Integer id, const Real value)
{
   referenceLeg->SetRealParameter(id, value);
   otherLeg->SetRealParameter(id, value);

   Real retval = RangeAdapterKm::SetRealParameter(id, value);

   return retval;
}

//------------------------------------------------------------------------------
// bool SetBooleanParameter(const Integer id, const bool value)
//------------------------------------------------------------------------------
/**
 * This method sets value to a bool parameter.
 *
 * @param id      The ID for the parameter
 * @param value   bool value used to set
 *
 * @return The parameter value.
 */
//------------------------------------------------------------------------------
bool DeltaRangeAdapter::SetBooleanParameter(const Integer id, const bool value)
{
   bool retval = true;

   retval = referenceLeg->SetBooleanParameter(id, value) && retval;
   retval = otherLeg->SetBooleanParameter(id, value) && retval;

   retval = RangeAdapterKm::SetBooleanParameter(id, value) && retval;

   return retval;
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
bool DeltaRangeAdapter::RenameRefObject(const UnsignedInt type,
      const std::string& oldName, const std::string& newName)
{
   bool retval = true;
   retval = referenceLeg->RenameRefObject(type, oldName, newName) && retval;
   retval = otherLeg->RenameRefObject(type, oldName, newName) && retval;

   retval = RangeAdapterKm::RenameRefObject(type, oldName, newName) && retval;

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
bool DeltaRangeAdapter::SetRefObject(GmatBase* obj,
      const UnsignedInt type, const std::string& name)
{
   bool retval = true;
   retval = referenceLeg->SetRefObject(obj, type, name) && retval;
   retval = otherLeg->SetRefObject(obj, type, name) && retval;

   retval = RangeAdapterKm::SetRefObject(obj, type, name) && retval;

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
bool DeltaRangeAdapter::SetRefObject(GmatBase* obj,
      const UnsignedInt type, const std::string& name, const Integer index)
{
   bool retval = true;
   retval = referenceLeg->SetRefObject(obj, type, name, index) && retval;
   retval = otherLeg->SetRefObject(obj, type, name, index) && retval;

   retval = RangeAdapterKm::SetRefObject(obj, type, name, index) && retval;

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
void DeltaRangeAdapter::SetPropagator(PropSetup* ps)
{
   #ifdef DEBUG_INITIALIZATION
      MessageInterface::ShowMessage("Setting propagator to %p in "
            "DeltaRangeAdapter\n", ps);
   #endif

   referenceLeg->SetPropagator(ps);
   otherLeg->SetPropagator(ps);

   RangeAdapterKm::SetPropagator(ps);
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
void DeltaRangeAdapter::SetTransientForces(std::vector<PhysicalModel*> *tf)
{
    GetMeasurementModel()->SetTransientForces(tf);
    referenceLeg->SetTransientForces(tf);
    otherLeg->SetTransientForces(tf);
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
bool DeltaRangeAdapter::Initialize()
{
#ifdef DEBUG_INITIALIZE
   MessageInterface::ShowMessage("DeltaRangeAdapter::Initialize() <%p> start\n", this);
#endif

   bool retval = true;

   // setting measurementType = "DeltaRange" on reference and other legs prevents
   // noise and bias calculation in GNRangeAdapter::CalculateMeasurement()
   
   retval = retval && RangeAdapterKm::Initialize();
   retval = retval && referenceLeg->SetStringParameter(MEASUREMENT_TYPE, "DeltaRange");
   retval = retval && referenceLeg->Initialize();
   retval = retval && otherLeg->SetStringParameter(MEASUREMENT_TYPE, "DeltaRange");
   retval = retval && otherLeg->Initialize();

#ifdef DEBUG_INITIALIZE
   MessageInterface::ShowMessage("DeltaRangeAdapter::Initialize() <%p> exit\n", this);
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
const MeasurementData& DeltaRangeAdapter::CalculateMeasurement(bool withEvents,
   ObservationData* forObservation, std::vector<RampTableData>* rampTB,
   bool forSimulation)
{
#ifdef DEBUG_ADAPTER_EXECUTION
   MessageInterface::ShowMessage("DeltaRangeAdapter::CalculateMeasurement(%s, "
      "<%p>, <%p>) called\n", (withEvents ? "true" : "false"), forObservation,
      rampTB);
#endif
   
   if (!calcData)
      throw MeasurementException("Measurement data was requested for " +
      instanceName + " before the measurement was set");

   // Compute range for reference leg
   referenceLeg->CalculateMeasurement(withLighttime, forObservation, rampTB, forSimulation);
   measDataRef = referenceLeg->GetMeasurement();

   // Compute range for other leg
   
   // For the other leg, the transmission time is fixed to be the
   // transmission time corresponding to the reference leg
   ObservationData* otherOb = NULL;
   if (forObservation)
      otherOb = new ObservationData(*forObservation);
   else
      otherOb = new ObservationData();
   otherOb->epochGT = measDataRef.tPrecTimes[0]; // take transmission time
   otherOb->epoch = measDataRef.tPrecTimes[0].GetMjd();

   otherLeg->GetMeasurementModel()->SetTimeTagFlag(false); // transmission time is fixed
   otherLeg->CalculateMeasurement(withLighttime, otherOb, rampTB, forSimulation);
   measDataOther = otherLeg->GetMeasurement();

   // Clear cMeasurement
   // TODO determine if we need to set any of these variables after clearing them
   cMeasurement.rangeVecs.clear();
   cMeasurement.tBodies.clear();
   cMeasurement.rBodies.clear();
   cMeasurement.tPrecTimes.clear();
   cMeasurement.rPrecTimes.clear();
   cMeasurement.tLocs.clear();
   cMeasurement.rLocs.clear();

   cMeasurement.isFeasible = measDataRef.isFeasible && measDataOther.isFeasible;
   if (cMeasurement.isFeasible)
      cMeasurement.unfeasibleReason = "N";
   else if (measDataOther.isFeasible)
      cMeasurement.unfeasibleReason = "B1";
   else if (measDataRef.isFeasible)
      cMeasurement.unfeasibleReason = "B2";
   else
      cMeasurement.unfeasibleReason = "B12";
   cMeasurement.feasibilityValue = std::min(measDataRef.feasibilityValue,
					    measDataOther.feasibilityValue);
   
   cMeasurement.value.clear();
   cMeasurement.value.push_back(measDataRef.value[0] - measDataOther.value[0]);
   cMeasurement.correction.clear();
   cMeasurement.correction.push_back(measDataRef.correction[0] - measDataOther.correction[0]);
   
   cMeasurement.epochGT = measDataRef.epochGT; // this includes reference receiver rDelay
   cMeasurement.epoch = measDataRef.epoch;

   ComputeMeasurementBias("Bias", measurementType, 1);
   ComputeMeasurementNoiseSigma("NoiseSigma", measurementType, 1);
   ComputeMeasurementErrorCovarianceMatrix();
   cMeasurement.covariance = &measErrorCovariance;
   
#ifdef DEBUG_RANGE_CALCULATION
   MessageInterface::ShowMessage("===================================================================\n");
   MessageInterface::ShowMessage("====  DeltaRangeAdapter (%s): DeltaRange Calculation\n", GetName().c_str());
   MessageInterface::ShowMessage("===================================================================\n");

   MessageInterface::ShowMessage("      . Path : ");
   MessageInterface::ShowMessage("[ %s -> %s ] - [ %s -> %s ]\n",
				 participantLists[0]->at(1).c_str(),
				 participantLists[0]->at(0).c_str(),
				 participantLists[0]->at(1).c_str(),
				 participantLists[0]->at(2).c_str());

   MessageInterface::ShowMessage("      . Measurement epoch          : %.12lf\n", cMeasurement.epochGT.GetMjd());
   MessageInterface::ShowMessage("      . Measurement type           : <%s>\n", measurementType.c_str());
   MessageInterface::ShowMessage("      . Reference leg C-value w/o noise and bias : %.12lf km \n", measDataRef.value[0]);
   MessageInterface::ShowMessage("      . Other leg C-value w/o noise and bias : %.12lf km \n", measDataOther.value[0]);
   MessageInterface::ShowMessage("      . C-value w/o noise and bias : %.12lf km \n", cMeasurement.value[0]);
   MessageInterface::ShowMessage("      . Reference leg corrections : %.12lf km \n", measDataRef.correction[0]);
   MessageInterface::ShowMessage("      . Other leg corrections : %.12lf km \n", measDataOther.correction[0]);
   MessageInterface::ShowMessage("      . Corrections : %.12lf km \n", cMeasurement.correction[0]);
   MessageInterface::ShowMessage("      . Noise adding option        : %s\n", (addNoise ? "true" : "false"));
   MessageInterface::ShowMessage("      . Bias adding option        : %s\n", (addBias ? "true" : "false"));
   
   MessageInterface::ShowMessage("      . Range noise sigma          : %.12lf km \n", noiseSigma[0]);
   MessageInterface::ShowMessage("      . Range bias                 : %.12lf km \n", measurementBias[0]);
   MessageInterface::ShowMessage("      . Multiplier                 : %.12lf \n", multiplier);
#endif

   if (addNoise)
   {
      RandomNumber* rn = RandomNumber::Instance();
      Real val = rn->Gaussian(0.0, noiseSigma[0]);

      cMeasurement.value[0] += val;
      cMeasurement.correction[0] += val;
   }

   if (addBias)
   {
      cMeasurement.value[0] += measurementBias[0];
      cMeasurement.correction[0] += measurementBias[0];
   }
      
#ifdef DEBUG_RANGE_CALCULATION
   MessageInterface::ShowMessage("      . C-value with noise and bias : %.12lf km\n", cMeasurement.value[0]);
   MessageInterface::ShowMessage("      . Measurement epoch A1Mjd     : %.12lf\n", cMeasurement.epoch);
   MessageInterface::ShowMessage("      . Transmit frequency at receive epoch  : %.12le Hz\n", cMeasurement.uplinkFreqAtRecei);
   MessageInterface::ShowMessage("      . Transmit frequency at transmit epoch : %.12le Hz\n", cMeasurement.uplinkFreq);
   MessageInterface::ShowMessage("      . Measurement is %s\n", (cMeasurement.isFeasible ? "feasible" : "unfeasible"));
   MessageInterface::ShowMessage("      . Feasibility reason          : %s\n", cMeasurement.unfeasibleReason.c_str());
   MessageInterface::ShowMessage("      . Elevation angle             : %.12lf degree\n", cMeasurement.feasibilityValue);
   MessageInterface::ShowMessage("      . Covariance matrix           : <%p>\n", cMeasurement.covariance);
   if (cMeasurement.covariance)
      {
	 MessageInterface::ShowMessage("      . Covariance matrix size = %d\n", cMeasurement.covariance->GetDimension());
	 MessageInterface::ShowMessage("     [ ");
	 for (UnsignedInt i = 0; i < cMeasurement.covariance->GetDimension(); ++i)
            {
               if (i > 0)
                  MessageInterface::ShowMessage("\n");
               for (UnsignedInt j = 0; j < cMeasurement.covariance->GetDimension(); ++j)
                  MessageInterface::ShowMessage("%le   ", cMeasurement.covariance->GetCovariance()->GetElement(i, j));
            }
	 MessageInterface::ShowMessage("]\n");
      }

   MessageInterface::ShowMessage("===================================================================\n");
#endif
   
#ifdef DEBUG_ADAPTER_EXECUTION
   MessageInterface::ShowMessage("Computed measurement\n   Type:  %d\n   "
				 "Type:  %s\n   UID:   %d\n   Epoch:%.12lf\n   Participants:\n",
				 cMeasurement.type, cMeasurement.typeName.c_str(),
				 cMeasurement.uniqueID, cMeasurement.epoch);
   for (UnsignedInt k = 0; k < cMeasurement.participantIDs.size(); ++k)
      MessageInterface::ShowMessage("      %s\n",
				    cMeasurement.participantIDs[k].c_str());
   MessageInterface::ShowMessage("   Values:\n");
   for (UnsignedInt k = 0; k < cMeasurement.value.size(); ++k)
      MessageInterface::ShowMessage("      %.12lf\n",
				    cMeasurement.value[k]);
   
   MessageInterface::ShowMessage("   Valid: %s\n",
				 (cMeasurement.isFeasible ? "true" : "false"));
#endif

#ifdef DEBUG_ADAPTER_EXECUTION
   MessageInterface::ShowMessage("DeltaRangeAdapter::CalculateMeasurement(%s, "
				 "<%p>, <%p>) exit\n", (withEvents ? "true" : "false"), forObservation,
				 rampTB);
#endif

   return cMeasurement;
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
void DeltaRangeAdapter::SetCorrection(const std::string& correctionName,
      const std::string& correctionType)
{
   referenceLeg->SetCorrection(correctionName, correctionType);
   otherLeg->SetCorrection(correctionName, correctionType);
   RangeAdapterKm::SetCorrection(correctionName, correctionType);
}

//------------------------------------------------------------------------------
// void ComputeMeasurementBias(const std::string biasName, 
//                             const std::string measType, Integer numTrip)
//------------------------------------------------------------------------------
/**
* Get measurement bias
*
* @param biasName      Name of the measurement bias. In this case is "Bias"      
* @param measType      Measurement type of this tracking data
* @param numTrip       Number of ways signal travel such as 1-way, 2-ways, 
*                      or 3-ways 
*
* @return              Value of bias
*/
//------------------------------------------------------------------------------
void DeltaRangeAdapter::ComputeMeasurementBias(const std::string biasName, const std::string measType, Integer numTrip)
{
   // Get signal data
   std::vector<SignalData*> data = calcData->GetSignalData();
   UnsignedInt measurementSize = data.size();

   // Calculate bias for each measurement (signal path)
   measurementBias.clear();
   if (rangeOnly)
   {
      for (UnsignedInt i = 0; i < measurementSize; ++i)
         measurementBias.push_back(0.0);
      return;
   }

   Real bias;
   for (UnsignedInt i = 0; i < measurementSize; ++i)
   {
      SpacePoint* gs1 = data[i]->tNode; // reference groundstation
      SpacePoint* gs2 = data[i]->next->rNode; // other groundstation
      GmatBase* sc = data[i]->rNode; // spacecraft

      // Search for ErrorModel associated with measType and spacecraft (It has to search in GroundStation::errorModelMap) that matches both groundstations
      std::map<std::string,ObjectArray> errmodelMap1 = ((GroundstationInterface*)gs1)->GetErrorModelMap();
      std::map<std::string,ObjectArray> errmodelMap2 = ((GroundstationInterface*)gs2)->GetErrorModelMap();
      ErrorModel* errmodel = NULL;
      for (std::map<std::string,ObjectArray>::iterator mapIndex1 = errmodelMap1.begin(); mapIndex1 != errmodelMap1.end(); ++mapIndex1)
      {
	 for (std::map<std::string,ObjectArray>::iterator mapIndex2 = errmodelMap2.begin(); mapIndex2 != errmodelMap2.end(); ++mapIndex2)
	 {
	    if ((sc->GetName() == mapIndex1->first) &&
		(sc->GetName() == mapIndex2->first))
	    {
	       for(UnsignedInt j = 0; j < mapIndex1->second.size(); ++j)
               {
		  for (UnsignedInt k = 0; k < mapIndex2->second.size(); ++k)
	          {
		     if ((mapIndex1->second.at(j)->GetStringParameter("Type") == measType)
			 && (mapIndex2->second.at(k)->GetName() == mapIndex1->second.at(j)->GetName()))
		     {
			errmodel = (ErrorModel*) mapIndex1->second.at(j);
			break;
		     }
		      
		     if (errmodel != NULL)
			break;
		  }
		  
		  if (errmodel != NULL)
		     break;
	       }
	    }
            if (errmodel != NULL)
               break;
         }

	 if (errmodel != NULL)
	    break;
      }

      // if not found, throw an error message
      if (errmodel == NULL)
      {
         std::stringstream ss;
         ss << "Error: ErrorModel mismatched. No error model with Type = '" << measType << "' was set to GroundStation " << gs1->GetName() << ".ErrorModels and Groundstation " << gs2->GetName() << ".ErrorModels\n";
         throw MeasurementException(ss.str());
      }


      bias = 0.0;
      if (forObjects.size() > 0)
      {
         // This is case for running estimation: solve-for objects are stored in forObjects array
         // Search for object with the same name with errmodels[k]
         UnsignedInt j;
         for (j = 0; j < forObjects.size(); ++j)
         {
            if (forObjects[j]->GetFullName() == errmodel->GetFullName())
               break;
         }

         // Get Bias from that error model
         if (j >= forObjects.size())
            bias = errmodel->GetRealParameter(biasName);              // bias is a consider parameter
         else
            bias = forObjects[j]->GetRealParameter(biasName);         // bias is a solve-for parameter
      }
      else
      {
         // This is case for running simulation. no solve-for objects are stored in forObjects
         // For this case, bias value is gotten from GroundStation.ErrorModels parameter
         bias = errmodel->GetRealParameter(biasName);             // bias is consider parameter
      }
      
      measurementBias.push_back(bias);
   }
   
   // Clean up memory
   data.clear();
}

//------------------------------------------------------------------------------
// void ComputeMeasurementNoiseSigma(const std::string noiseSigmaName, 
//                                   const std::string measType, Integer numTrip)
//------------------------------------------------------------------------------
/**
* Get measurement noise sigma
*
* @param noisSigmaName      Name of the measurement noise sigma. In this case is 
*                           "NoisSigma"
* @param measType           Measurement type of this tracking data
* @param numTrip            NumTrip shown number of ways signal travel such as 
*                           1-way, 2-ways, or 3-ways
*
* @return                   Value of noise sigma
*/
//------------------------------------------------------------------------------
void DeltaRangeAdapter::ComputeMeasurementNoiseSigma(const std::string noiseSigmaName, const std::string measType, Integer numTrip)
{
   // Get signal data
   std::vector<SignalData*> data = calcData->GetSignalData();
   UnsignedInt measurementSize = data.size();

   // Calculate noise sigma for each signal path
   noiseSigma.clear();
   if (rangeOnly)
   {
      for (UnsignedInt i = 0; i < measurementSize; ++i)
         noiseSigma.push_back(0.0);
      return;
   }

   Real noise;
   for (UnsignedInt i = 0; i < measurementSize; ++i)
   {
      SpacePoint* gs1 = data[i]->tNode; // reference groundstation
      SpacePoint* gs2 = data[i]->next->rNode; // other groundstation

      // Search for ErrorModel associated with measType and numTrip in both groundstations
      ObjectArray errmodels1 = ((GroundstationInterface*)gs1)->GetRefObjectArray("ErrorModel");
      ObjectArray errmodels2 = ((GroundstationInterface*)gs1)->GetRefObjectArray("ErrorModel");
      GmatBase* errmodel = NULL;
      for (UnsignedInt j = 0; j < errmodels1.size(); ++j)
      {
	 for (UnsignedInt k = 0; k < errmodels2.size(); ++k)
	 {
	    if ((errmodels1[j]->GetStringParameter("Type") == measType)
		&& (errmodels2[k]->GetName() == errmodels1[k]->GetName()))
	    {
	       errmodel = errmodels1[j];
	       break;
	    }
	 }
	 if (errmodel)
	    break;
      }
      if (errmodel == NULL)
      {
         std::stringstream ss;
         ss << "Error: ErrorModel mismatched. No error model with Type = '" << measType << "' was set to GroundStation " << gs1->GetName() << ".ErrorModels and Groundstation " << gs2->GetName() << ".ErrorModels\n";
         throw MeasurementException(ss.str());
      }

      // Get NoiseSigma from that error model
      noise = errmodel->GetRealParameter(noiseSigmaName);

      noiseSigma.push_back(noise);
   }// for i

   // Clean up memory
   data.clear();
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
const std::vector<RealArray>& DeltaRangeAdapter::CalculateMeasurementDerivatives(
      GmatBase* obj, Integer id)
{
   if (!calcData)
      throw MeasurementException("Measurement derivative data was requested "
            "for " + instanceName + " before the measurement was set");

   Integer parmId = GetParmIdFromEstID(id, obj);
   #ifdef DEBUG_DERIVATIVE_CALCULATION
      MessageInterface::ShowMessage("Enter DeltaRangeAdapter::CalculateMeasurementDerivatives(%s, %d) called; parm ID is %d; Epoch %.12lf\n", obj->GetFullName().c_str(), id, parmId, cMeasurement.epoch);
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

   // Clear derivative variable:
   for (UnsignedInt i = 0; i < theDataDerivatives.size(); ++i)
      theDataDerivatives[i].clear();
   theDataDerivatives.clear();

   if (paramName == "Bias")
   {
      // TODO: what is this? do we need it in DeltaRangeAdapter?
      //if (((ErrorModel*)obj)->GetStringParameter("Type") == "TDRSDoppler_HZ")
      if (((ErrorModel*)obj)->GetStringParameter("Type") == "SN_Doppler")
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
      // Derivative for reference leg
      const std::vector<RealArray> *derivativeDataRef =
         &(referenceLeg->CalculateMeasurementDerivatives(obj, id));

      // Derivative for other leg

      // First we need to set up the STM for the other leg:
      // it must coincide with the STM for the reference leg
      otherLeg->GetMeasurementModel()->GetSignalData()[0]->tSTM =
	referenceLeg->GetMeasurementModel()->GetSignalData()[0]->tSTM;
      otherLeg->GetMeasurementModel()->GetSignalData()[0]->tSTMtm =
	referenceLeg->GetMeasurementModel()->GetSignalData()[0]->tSTMtm;

      // now we can compte the derivative as usual
      const std::vector<RealArray> *derivativeDataOther =
         &(otherLeg->CalculateMeasurementDerivatives(obj, id));

      // copy reference and other paths' derivatives
      UnsignedInt size = derivativeDataRef->at(0).size();
      std::vector<RealArray> derivativesRef;
      for (UnsignedInt i = 0; i < derivativeDataRef->size(); ++i)
      {
         RealArray oneRow;
         oneRow.assign(size, 0.0);
         derivativesRef.push_back(oneRow);

         if (derivativeDataRef->at(i).size() != size)
            throw MeasurementException("Derivative data size is a different size "
               "than expected");

         for (UnsignedInt j = 0; j < size; ++j)
         {
            derivativesRef[i][j] = (derivativeDataRef->at(i))[j];
         }
      }
   
      size = derivativeDataOther->at(0).size();
      std::vector<RealArray> derivativesOther;
      for (UnsignedInt i = 0; i < derivativeDataOther->size(); ++i)
      {
         RealArray oneRow;
         oneRow.assign(size, 0.0);
         derivativesOther.push_back(oneRow);

         if (derivativeDataOther->at(i).size() != size)
            throw MeasurementException("Derivative data size is a different size "
               "than expected");

         for (UnsignedInt j = 0; j < size; ++j)
         {
            derivativesOther[i][j] = (derivativeDataOther->at(i))[j];
         }
      }

      #ifdef DEBUG_ADAPTER_DERIVATIVES
      MessageInterface::ShowMessage("   Derivatives Reference path: [");
      for (UnsignedInt i = 0; i < derivativeDataRef->size(); ++i)
      {
         if (i > 0)
            MessageInterface::ShowMessage("]\n                [");
         for (UnsignedInt j = 0; j < derivativeDataRef->at(i).size(); ++j)
         {
            if (j > 0)
               MessageInterface::ShowMessage(", ");
            MessageInterface::ShowMessage("%.12le", (derivativeDataRef->at(i))[j]);
         }
      }
      MessageInterface::ShowMessage("]\n");
      MessageInterface::ShowMessage("   Derivatives Other path: [");
      for (UnsignedInt i = 0; i < derivativeDataOther->size(); ++i)
      {
         if (i > 0)
            MessageInterface::ShowMessage("]\n                [");
         for (UnsignedInt j = 0; j < derivativeDataOther->at(i).size(); ++j)
         {
            if (j > 0)
               MessageInterface::ShowMessage(", ");
            MessageInterface::ShowMessage("%.12le", (derivativeDataOther->at(i))[j]);
         }
      }
      MessageInterface::ShowMessage("]\n");
      #endif

      // Now assemble the derivative data into the requested derivative
      size = derivativesRef[0].size();               // This is the size of derivative vector for signal path 0
      
      for (UnsignedInt i = 0; i < derivativesRef.size(); ++i)
      {
         // For each signal path, do the following steps:
         RealArray oneRow;
         oneRow.assign(size, 0.0);
         theDataDerivatives.push_back(oneRow);

         if (derivativesRef[i].size() != size)
            throw MeasurementException("Derivative data size for Reference path is a different size "
               "than expected");
         if (derivativesOther[i].size() != size)
            throw MeasurementException("Derivative data size for Other path is a different size "
               "than expected");

         for (UnsignedInt j = 0; j < size; ++j)
         {
            if ((paramName == "Position")||(paramName == "Velocity")||(paramName == "CartesianX"))
            {
               theDataDerivatives[i][j] = derivativesRef[i][j] - derivativesOther[i][j];
            }
            else
            {
	       // TODO: check if this is correct
               theDataDerivatives[i][j] =  derivativesRef[i][j] - derivativesOther[i][j];
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
      MessageInterface::ShowMessage("Exit DeltaRangeAdapter::CalculateMeasurementDerivatives():\n");
   #endif

   return theDataDerivatives;
}
