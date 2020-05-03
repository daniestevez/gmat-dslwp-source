//------------------------------------------------------------------------------
//                           FileThrust
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool
//
// Copyright (c) 2002-2015 United States Government as represented by the
// Administrator of The National Aeronautics and Space Administration.
// All Other Rights Reserved.
//
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under the FDSS II
// contract, Task Order 08
//
// Author: Darrel J. Conway, Thinking Systems, Inc.
// Created: Jan 13, 2016
/**
 * Model for thrusts applied by a thrust history file
 */
//------------------------------------------------------------------------------

#include "FileThrust.hpp"

#include "MessageInterface.hpp"
#include "TimeTypes.hpp"
#include "ODEModelException.hpp"
#include "PropagationStateManager.hpp"
#include <sstream>


//#define DEBUG_INITIALIZATION
//#define DEBUG_FILETHRUST_EXE
//#define DEBUG_INTERPOLATION
//#define DEBUG_MASS_FLOW
//#define DEBUG_SEGMENTS
//#define DEBUG_REF_OBJECTS
//#define DEBUG_TSF_SOLVEFOR

//------------------------------------------------------------------------------
// FileThrust(const std::string &name)
//------------------------------------------------------------------------------
/**
 * Constructor for forces from thrust history files.
 *
 * @param name Name of the constructed instance.
 */
//------------------------------------------------------------------------------
FileThrust::FileThrust(const std::string &name) :
   PhysicalModel           (Gmat::PHYSICAL_MODEL, "FileThrust", name),
   satCount                (0),
   cartIndex               (-1),
   fillCartesian           (true),
   dataIsThrust            (true),
   massFlowWarningNeeded   (true),
   segments                (NULL),
   mDotIndex               (-1),
   depleteMass             (false),
   coordSystem             (NULL),
   liner                   (NULL),
   spliner                 (NULL),
   warnTooFewPoints        (true),
   tsfID                   (22),          // 22 is the next entry for ODE models
                                          // @todo Fix this magic number
   estimatingTSF           (false),
   tsfEpsilonID            (-1),
   tsfEpsilonRow           (-1),
   thrustSF                (1.0),
   thrustSFinitial         (0.0)
{
   derivativeIds.push_back(Gmat::CARTESIAN_STATE);
   objectTypeNames.push_back("FileThrust");
}

//------------------------------------------------------------------------------
// ~FileThrust()
//------------------------------------------------------------------------------
/**
 * Destructor
 */
//------------------------------------------------------------------------------
FileThrust::~FileThrust()
{
   if (liner != NULL)
      delete liner;
   if (spliner != NULL)
      delete spliner;
}

//------------------------------------------------------------------------------
// FileThrust(const FileThrust& ft)
//------------------------------------------------------------------------------
/**
 * Copy constructor for forces from thrust history files.
 *
 * @param ft The original that gets copied.
 */
//------------------------------------------------------------------------------
FileThrust::FileThrust(const FileThrust& ft) :
   PhysicalModel           (ft),
   spacecraftNames         (ft.spacecraftNames),
   satCount                (ft.satCount),
   cartIndex               (ft.cartIndex),
   fillCartesian           (ft.fillCartesian),
   dataIsThrust            (ft.dataIsThrust),
   massFlowWarningNeeded   (true),
   segments                (ft.segments),
   mDotIndex               (ft.mDotIndex),
   depleteMass             (ft.depleteMass),
   activeTankName          (ft.activeTankName),
   csNames                 (ft.csNames),
   coordSystem             (NULL),
   liner                   (NULL),
   spliner                 (NULL),
   warnTooFewPoints        (true),
   tsfID                   (ft.tsfID),
   estimatingTSF           (ft.estimatingTSF),
   tsfEpsilonID            (ft.tsfEpsilonID),
   tsfEpsilonRow           (ft.tsfEpsilonRow),
   tsfInitial              (ft.tsfInitial),
   thrustSF                (ft.thrustSF),
   thrustSFinitial         (ft.thrustSFinitial)
   {
}

//------------------------------------------------------------------------------
// FileThrust& operator=(const FileThrust& ft)
//------------------------------------------------------------------------------
/**
 * Assignment operator.
 *
 * @param ft Instance that provides the parameter values.
 *
 * @return This instance, set to match the input instance.
 */
//------------------------------------------------------------------------------
FileThrust& FileThrust::operator=(const FileThrust& ft)
{
   if (this != &ft)
   {
      PhysicalModel::operator=(ft);

      spacecraftNames = ft.spacecraftNames;
      satCount      = ft.satCount;
      cartIndex     = ft.cartIndex;
      fillCartesian = ft.fillCartesian;
      dataIsThrust  = ft.dataIsThrust;
      segments      = ft.segments;
      mDotIndex     = ft.mDotIndex;
      depleteMass   = ft.depleteMass;
      activeTankName = ft.activeTankName;
      csNames       = ft.csNames;
      tsfID         = ft.tsfID;
      estimatingTSF = ft.estimatingTSF;
      tsfEpsilonID  = ft.tsfEpsilonID;
      tsfEpsilonRow = ft.tsfEpsilonRow;
      tsfInitial    = ft.tsfInitial;
      thrustSF      = ft.thrustSF;
      thrustSFinitial = ft.thrustSFinitial;
      coordSystem   = NULL;

      // Interpolator resets
      if (liner != NULL)
      {
         delete liner;
         liner = NULL;
      }
      if (spliner != NULL)
      {
         delete spliner;
         spliner = NULL;
      }

      massFlowWarningNeeded = true;
      warnTooFewPoints      = true;
   }

   return *this;
}

//------------------------------------------------------------------------------
// bool operator==(const FileThrust& ft) const
//------------------------------------------------------------------------------
/**
 * Checks to see if two FileThrusts apply the same force.
 *
 * @param ft The second FileThrust
 *
 * @return true if the segment data are the same.
 */
//------------------------------------------------------------------------------
bool FileThrust::operator==(const FileThrust& ft) const
{
   return (segments == ft.segments);
}

//------------------------------------------------------------------------------
// GmatBase* Clone() const
//------------------------------------------------------------------------------
/**
 * Overridden Clone method used to make copies from a GmatBase pointer.
 *
 * @return New instance, set to look like this one.
 */
//------------------------------------------------------------------------------
GmatBase* FileThrust::Clone() const
{
   return new FileThrust(*this);
}

//------------------------------------------------------------------------------
// void Clear(const UnsignedInt type)
//------------------------------------------------------------------------------
/**
 * Clears the arrays of elements that get set by the Propagate commands
 *
 * @param type The type of element that gets cleared.  Set to
 *             Gmat::UNKNOWN_OBJECT to clear all of the configrable arrays.
 */
//------------------------------------------------------------------------------
void FileThrust::Clear(const UnsignedInt type)
{
   if ((type == Gmat::UNKNOWN_OBJECT) || (type == Gmat::SPACECRAFT))
   {
      spacecraftNames.clear();
      spacecraft.clear();
   }
}


//------------------------------------------------------------------------------
// Integer         GetParameterID(const std::string &str) const
//------------------------------------------------------------------------------
/**
 * Returns look-up ID for thrust segment data.
 *
 * This method overrides the GmatBase method used to look up scriptable
 * parameters.  The use here is not standard, so should not be replicated
 * elsewhere.
 *
 * @param str The string identifying the look up parameter
 *
 * @return An overloaded ID for the parameter
 */
//------------------------------------------------------------------------------
Integer FileThrust::GetParameterID(const std::string &str) const
{
   Integer retval = -1, seg, id;

   UnsignedInt loc = str.find(".");
   if (loc != std::string::npos)
   {
      std::string segName, parmName;
      segName = str.substr(0, loc);
      parmName = str.substr(loc+1);

      for (UnsignedInt i = 0; i < segments->size(); ++i)
      {
         if (segments->at(i).GetName() == segName)
         {
            seg = i;
            id = segments->at(i).GetParameterID(parmName);
            retval = 100 * (i+1) + segments->at(i).GetParameterID(parmName);
            break;
         }
      }
   }

   #ifdef DEBUG_MAPPING
      MessageInterface::ShowMessage("In FileThrust: Loaded ID for %s is %d "
            "for segment %d, parameter %d\n", str.c_str(), retval, seg, id);
   #endif
   return retval;
}


//------------------------------------------------------------------------------
// bool IsParameterReadOnly(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Identifies parameters that are not written to script
 *
 * @param id The parameter checked here
 *
 * @return true for parameters that are hidden
 *
 * @note: Since this is an internal object (a force model element), this method
 * isn't really used, but included for completeness
 */
//------------------------------------------------------------------------------
bool FileThrust::IsParameterReadOnly(const Integer id) const
{
   return true;
}

//------------------------------------------------------------------------------
// bool IsParameterReadOnly(const std::string &label) const
//------------------------------------------------------------------------------
/**
 * Identifies parameters that are not written to script
 *
 * @param label The parameter checked here
 *
 * @return true for parameters that are hidden
 *
 * @note: Since this is an internal object (a force model element), this method
 * isn't really used, but included for completeness
 */
//------------------------------------------------------------------------------
bool FileThrust::IsParameterReadOnly(const std::string &label) const
{
   return true;
}

//------------------------------------------------------------------------------
// Real GetRealParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
 * Accessor for floating point parameter data
 *
 * Note that the usage here is non-standard, and used for the solve-for
 * implementation so that a common interface is used.
 *
 * @param id The ID of the parameter
 *
 * @return The parameter value
 */
//------------------------------------------------------------------------------
Real FileThrust::GetRealParameter(const Integer id) const
{
   if (id > 99)
   {
      // Break ID apart into segment number and parm id
      Integer segId, parmId;
      segId = (Integer)(id / 100) - 1;
      parmId = id - 100 * (segId + 1);

      #ifdef DEBUG_MAPPING
         MessageInterface::ShowMessage("Get %d: Segment %d, parm %d\n", id,
               segId, parmId);
      #endif

      return segments->at(segId).GetRealParameter(parmId); 
   }

   return GmatBase::GetRealParameter(id);
}

//------------------------------------------------------------------------------
// Real SetRealParameter(const Integer id, const Real value)
//------------------------------------------------------------------------------
/**
 * Accessor for floating point parameter data
 *
 * Note that the usage here is non-standard, and used for the solve-for
 * implementation so that a common interface is used.
 *
 * @param id The ID of the parameter
 * @param value The new parameter value
 *
 * @return The parameter value
 */
//------------------------------------------------------------------------------
Real FileThrust::SetRealParameter(const Integer id, const Real value)
{
   if (id > 99)
   {
      // Break ID apart into segment number and parm id
      Integer segId, parmId;
      segId = (Integer)(id / 100) - 1;
      parmId = id - 100 * (segId + 1);

      return segments->at(segId).SetRealParameter(parmId, value);
   }

   return PhysicalModel::SetRealParameter(id, value);
}

//------------------------------------------------------------------------------
// const ObjectTypeArray& GetRefObjectTypeArray()
//------------------------------------------------------------------------------
/**
 * Retrieves the list of ref object types used by this class.
 *
 * @return the list of object types.
 *
 */
//------------------------------------------------------------------------------
const ObjectTypeArray& FileThrust::GetRefObjectTypeArray()
{
   refObjectTypes.clear();
   refObjectTypes.push_back(Gmat::COORDINATE_SYSTEM);
   refObjectTypes.push_back(Gmat::SPACECRAFT);
   return refObjectTypes;
}

//------------------------------------------------------------------------------
// bool SetRefObjectName(const UnsignedInt type, const std::string& name)
//------------------------------------------------------------------------------
/**
 * Sets the names for referenced objects.
 *
 * FileThrust instances use Spacecraft and (set separately) thrust segment
 * objects.  This method sets the names for the Spacecraft objects.
 *
 * @param type The type of object that has the name.
 * @param name The object's name
 *
 * @return true on success, false otherwise.
 */
//------------------------------------------------------------------------------
bool FileThrust::SetRefObjectName(const UnsignedInt type,
      const std::string& name)
{
   if (type == Gmat::SPACECRAFT)
   {
      if (find(spacecraftNames.begin(), spacecraftNames.end(), name) ==
            spacecraftNames.end())
         spacecraftNames.push_back(name);
      return true;
   }

   if (type == Gmat::COORDINATE_SYSTEM)
   {
      if (find(csNames.begin(), csNames.end(), name) == csNames.end())
         csNames.push_back(name);
      return true;
   }

   return PhysicalModel::SetRefObjectName(type, name);
}

//------------------------------------------------------------------------------
// const StringArray& GetRefObjectNameArray(const UnsignedInt type)
//------------------------------------------------------------------------------
/**
 * Accesses the names for referenced objects.
 *
 * @param type The type of object requested.
 *
 * @return The vector of names.
 */
//------------------------------------------------------------------------------
const StringArray& FileThrust::GetRefObjectNameArray(
      const UnsignedInt type)
{
   #ifdef DEBUG_REF_OBJECTS
      MessageInterface::ShowMessage("FileThrust::GetRefObjectNameArray called "
            "for type %d\n", type);
   #endif
   PhysicalModel::GetRefObjectNameArray(type);

   if ((type == Gmat::SPACECRAFT) || (type == Gmat::UNKNOWN_OBJECT))
      refObjectNames.insert(refObjectNames.begin(), spacecraftNames.begin(),
            spacecraftNames.end());

   if ((type == Gmat::COORDINATE_SYSTEM) || (type == Gmat::UNKNOWN_OBJECT))
      refObjectNames.insert(refObjectNames.begin(), csNames.begin(),
            csNames.end());

   #ifdef DEBUG_REF_OBJECTS
      MessageInterface::ShowMessage("  Returning %d names\n",
            refObjectNames.size());
   #endif

   return refObjectNames;
}

//------------------------------------------------------------------------------
// bool SetRefObject(GmatBase *obj, const UnsignedInt type,
//                   const std::string &name)
//------------------------------------------------------------------------------
/**
 * Sets referenced object pointers.
 *
 * @param obj The object.
 * @param type The type of the object.
 * @param name The object's name.
 *
 * @return true if the object is set, false if not.
 */
//------------------------------------------------------------------------------
bool FileThrust::SetRefObject(GmatBase* obj, const UnsignedInt type,
      const std::string& name)
{
   if (type == Gmat::COORDINATE_SYSTEM)
   {
      #ifdef DEBUG_REF_OBJECTS
         MessageInterface::ShowMessage("Setting cs named %s\n", name.c_str());
      #endif

      bool retval = false;

      if (obj->IsOfType("CoordinateSystem") == false)
         throw ODEModelException(
            "FileThrust::SetRefObject cannot use objects of type " +
            obj->GetTypeName());
      if (find(csNames.begin(), csNames.end(), name) != csNames.end())
      {
         // Pass the CS to each segment that needs it
         for (UnsignedInt i = 0; i < segments->size(); ++i)
         {
            if (segments->at(i).segData.csName == name)
            {
               segments->at(i).segData.cs = (CoordinateSystem*)obj;
               retval = true;
            }
         }
      }
      return retval;
   }

   return PhysicalModel::SetRefObject(obj, type, name);
}

//------------------------------------------------------------------------------
// bool SetRefObject(GmatBase *obj, const UnsignedInt type,
//       const std::string &name, const Integer index)
//------------------------------------------------------------------------------
/**
 * Sets referenced object pointers in an object array.
 *
 * @param obj The object.
 * @param type The type of the object.
 * @param name The object's name.
 * @param index The index of the object in the array
 *
 * @return true if the object is set, false if not.
 */
//------------------------------------------------------------------------------
bool FileThrust::SetRefObject(GmatBase* obj, const UnsignedInt type,
      const std::string& name, const Integer index)
{
   MessageInterface::ShowMessage("Setting %s\n", name.c_str());
   return PhysicalModel::SetRefObject(obj, type, name, index);
}


//------------------------------------------------------------------------------
// bool RenameRefObject(const UnsignedInt type, const std::string &oldName,
//       const std::string &newName)
//------------------------------------------------------------------------------
/**
 * Changes the name for reference objects
 *
 * @param type The type of the object.
 * @param oldName The object's name before the change.
 * @param newName The object's proposed name after the change.
 *
 * @return true if a change was made, false if not
 */
//------------------------------------------------------------------------------
bool FileThrust::RenameRefObject(const UnsignedInt type,
      const std::string& oldName, const std::string& newName)
{
   if (type == Gmat::SPACECRAFT)
   {
      for (UnsignedInt i = 0; i < spacecraftNames.size(); ++i)
      {
         if (spacecraftNames[i] == oldName)
            spacecraftNames[i] = newName;
      }
      return true;
   }

   if (type == Gmat::COORDINATE_SYSTEM)
   {
      for (UnsignedInt i = 0; i < csNames.size(); ++i)
      {
         if (csNames[i] == oldName)
            csNames[i] = newName;
      }
      return true;
   }

   return PhysicalModel::RenameRefObject(type, oldName, newName);
}

//------------------------------------------------------------------------------
// GmatBase* GetRefObject(const UnsignedInt type, const std::string &name)
//------------------------------------------------------------------------------
/**
 * Retrieves a pointer to a reference object
 *
 * @param type The type of the object
 * @param name The object's name
 *
 * @return The object pointer
 */
//------------------------------------------------------------------------------
GmatBase* FileThrust::GetRefObject(const UnsignedInt type,
      const std::string& name)
{
   if (type == Gmat::SPACECRAFT)
   {
      if (name == "")
         if (spacecraft.size() > 0)
            return spacecraft[0];
      for (UnsignedInt i = 0; i < spacecraft.size(); ++i)
         if (spacecraft[i]->GetName() == name)
            return spacecraft[i];
      return NULL;
   }

   return PhysicalModel::GetRefObject(type, name);
}

//------------------------------------------------------------------------------
// GmatBase* GetRefObject(const UnsignedInt type, const std::string &name,
//       const Integer index)
//------------------------------------------------------------------------------
/**
 * Retrieves a pointer to a reference object from an array
 *
 * @param type The type of the object
 * @param name The object's name
 * @param index The index of the object in the array
 *
 * @return The object pointer
 */
//------------------------------------------------------------------------------
GmatBase* FileThrust::GetRefObject(const UnsignedInt type,
      const std::string& name, const Integer index)
{
   return PhysicalModel::GetRefObject(type, name, index);
}

//------------------------------------------------------------------------------
// bool IsTransient()
//------------------------------------------------------------------------------
/**
 * Specifies whether the force is transient.
 *
 * @return true for all FiniteThrust forces.
 */
//------------------------------------------------------------------------------
bool FileThrust::IsTransient()
{
   return true;
}

//------------------------------------------------------------------------------
// bool DepletesMass()
//------------------------------------------------------------------------------
/**
 * Detects mass depletion from a PhysicalModel
 *
 * @return true if the model depletes mass, false if it does not
 */
//------------------------------------------------------------------------------
bool FileThrust::DepletesMass()
{
   return depleteMass;
}

//------------------------------------------------------------------------------
// void FileThrust::SetSegmentList(std::vector<ThrustSegment> *segs)
//------------------------------------------------------------------------------
/**
 * Sets the list of thrust file segments used by the force
 *
 * @param segs The segment list
 */
//------------------------------------------------------------------------------
void FileThrust::SetSegmentList(std::vector<ThrustSegment> *segs)
{
   segments = segs;
   depleteMass = false;

   // Activate mass flow if any segment needs it and collect ref objects
   csNames.clear();
   for (UnsignedInt i = 0; i < segments->size(); ++i)
   {
      depleteMass |= segments->at(i).DepletesMass();

      // Collect the names of all of the coordinate systems needed
      std::string name = segments->at(i).segData.csName;
      if (find(csNames.begin(), csNames.end(), name) == csNames.end())
         csNames.push_back(name);
 
      // Set initial TSF
      if (i < tsfInitial.size())
      {
          segments->at(i).SetRealParameter("TSF_Epsilon", 0.0);
          thrustSF = segments->at(i).GetRealParameter("ThrustScaleFactor");
          if (tsfInitial[i] == -99999999.9999)
              tsfInitial[i] = segments->at(i).GetRealParameter("ThrustScaleFactor");
      }
      else
      {
          segments->at(i).SetRealParameter("TSF_Epsilon", 0.0);
          thrustSF = segments->at(i).GetRealParameter("ThrustScaleFactor");
          tsfInitial.push_back(segments->at(i).GetRealParameter("ThrustScaleFactor"));
      }
   }

   #ifdef DEBUG_SEGMENTS
      // Dump seg list here
      MessageInterface::ShowMessage("%d segments in the list\n", segments->size());
      for (UnsignedInt i = 0; i < segments->size(); ++i)
      {
         MessageInterface::ShowMessage("%d:  %s\n", i,
               segments->at(i).GetName().c_str());
         MessageInterface::ShowMessage("     TSF:   %lf\n",
               segments->at(i).GetRealParameter("ThrustScaleFactor"));
         MessageInterface::ShowMessage("     MFSF:  %lf\n",
               segments->at(i).GetRealParameter("MassFlowScaleFactor"));
         MessageInterface::ShowMessage("     Tanks: %d\n",
               segments->at(i).GetStringArrayParameter("MassSource").size());
         MessageInterface::ShowMessage("     CS: %s\n",
               segments->at(i).segData.csName.c_str());
      }
   #endif
}

//------------------------------------------------------------------------------
// void SetPropList(ObjectArray *soList)
//------------------------------------------------------------------------------
/**
 * Sets the list of propagated space objects for transient forces.
 */
//------------------------------------------------------------------------------
void FileThrust::SetPropList(ObjectArray *soList)
{
   #ifdef DEBUG_INITIALIZATION
      MessageInterface::ShowMessage("FileThrust::SetPropList called with %d "
            "objects\n", soList->size());
   #endif
   spacecraft.clear();
   for (ObjectArray::iterator i = soList->begin(); i != soList->end(); ++i)
      spacecraft.push_back(*i);
}

//------------------------------------------------------------------------------
// bool Initialize()
//------------------------------------------------------------------------------
/**
 * Set up data structures to manage the file based burns
 *
 * @return true if the call succeeds, false on failure.
 */
//------------------------------------------------------------------------------
bool FileThrust::Initialize()
{
   #ifdef DEBUG_INITIALIZATION
      MessageInterface::ShowMessage("Initializing FileThrust\n");
   #endif

   bool retval = false;

   isInitialized = PhysicalModel::Initialize();

   // Zero the data containers
   for (Integer i = 0; i < 5; ++i)
   {
      dataBlock[i] = 0.0;
      dataSet[i][0] = dataSet[i][1] = dataSet[i][2] =
      dataSet[i][3] = dataSet[i][4] = 0.0;
   }

   if (isInitialized)
   {
      if ((segments != NULL) && (spacecraftNames.size() > 0))
      {
         // set up the indices into the state vector that match spacecraft
         Integer satIndex, stateIndex;
         for (StringArray::iterator satName = spacecraftNames.begin();
              satName != spacecraftNames.end(); ++satName)
         {
            satIndex = 0;
            stateIndex = 0;

            for (ObjectArray::iterator propSat = spacecraft.begin();
                 propSat != spacecraft.end(); ++propSat)
            {
               if ((*propSat)->IsOfType(Gmat::SPACEOBJECT))
               {
                  stateIndex += ((SpaceObject*)(*propSat))->GetState().GetSize();
                  ++satIndex;
               }
            }
         }

         massFlowWarningNeeded = true;
         warnTooFewPoints      = true;
         indexPair[0]          = -1;
         retval                = true;
      }
      else
         isInitialized = false;
   }

   if (!isInitialized)
      throw ODEModelException("Unable to initialize FileThrust base");

   interpolatorData[0] = interpolatorData[1] = interpolatorData[2] =
   interpolatorData[3] = interpolatorData[4] = -1;

   return retval;
}

//------------------------------------------------------------------------------
// bool GetDerivatives(Real * state, Real dt, Integer order)
//------------------------------------------------------------------------------
/**
 * Method invoked to calculate derivatives
 *
 * This method is invoked to fill the deriv array with derivative information
 * for the system that is being integrated.  It uses the state and elapsedTime
 * parameters, along with the time interval dt passed in as a parameter, to
 * calculate the derivative information at time \f$t=t_0+t_{elapsed}+dt\f$.
 *
 * @param state         Pointer to the current state data.  This can differ
 *                      from the PhysicalModel state if the subscribing
 *                      integrator samples other state values during
 *                      propagation.  (For example, the Runge-Kutta integrators
 *                      do this during the stage calculations.)
 * @param dt            Additional time increment for the derivative
 *                      calculation; defaults to 0.
 * @param order         The order of the derivative to be taken (first
 *                      derivative, second derivative, etc)
 *
 * @return              true if the call succeeds, false on failure.
 */
//------------------------------------------------------------------------------
bool FileThrust::GetDerivatives(Real* state, Real dt, Integer order,
      const Integer id)
{
   bool retval = true;

   #ifdef DEBUG_FILETHRUST_EXE
      MessageInterface::ShowMessage("Supplying derivative data from a THF, %s\n",
            (depleteMass ? "including mass flow" : "without mass flow"));
   #endif

   Integer index = psm->GetSTMIndex(tsfID);

   #ifdef DEBUG_TSF_SOLVEFOR
      MessageInterface::ShowMessage("File TSF from psm %p (id %d) index in the STM "
            "is %d, dim %d\n", psm, tsfID, index, dimension);
   #endif

   // Thrust Scale Factor Solve For settings
   if (index >= 0)
   {
      estimatingTSF = true;
      tsfEpsilonRow = index;
      fillSTM = true;
   }

   #ifdef DEBUG_TSF_SOLVEFOR
      MessageInterface::ShowMessage("===> %sEstimating TSF, Entries go in row %d\n",
            (estimatingTSF ? "" : "NOT"), tsfEpsilonRow);
   #endif

   if (fillCartesian)
   {
      Real accel[3], mDot, burnData[4];
      Integer i6, mloc = -1;
      Integer i = 0, j = 0;
      SpaceObject *sat;

      // Loop through the spacecraft list, building accels for active sats
      for (ObjectArray::iterator sc = spacecraft.begin();
           sc != spacecraft.end(); ++sc)
      {
         i6 = cartIndex + i * 6;

         if (depleteMass)
         {
            mloc = mDotIndex + j;
            ++j;
         }
         else
            mloc = -1;

         if ((*sc)->IsOfType(Gmat::SPACEOBJECT) == false)
            continue;

         // Just a convenience
         sat = (SpaceObject*)(*sc);

         #ifdef DEBUG_FILETHRUST_EXE
            MessageInterface::ShowMessage("   Checking spacecraft %s\n",
               sat->GetName().c_str());
         #endif

         if (find(spacecraftNames.begin(), spacecraftNames.end(), sat->GetName()) !=
             spacecraftNames.end())
         {

            #ifdef DEBUG_FILETHRUST_EXE
               MessageInterface::ShowMessage("   Maneuvering %s\n",
                     (*sc)->GetName().c_str());
            #endif

            if (sat->GetType() != Gmat::SPACECRAFT)
               throw ODEModelException("FileThrust::GetDerivatives Finite "
               "burns cannot maneuver " + sat->GetTypeName() + " objects");

            // Start with zero thrust
            mDot = accel[0] = accel[1] = accel[2] = 0.0;
            if (HasPrecisionTime()) {
               epochGT = theState->GetEpochGT();
               GmatTime nowGT = epochGT;
               nowGT.AddSeconds(dt);
               ComputeAccelerationMassFlow(nowGT, burnData);
            }
            else {
               epoch = theState->GetEpoch();
               Real now = epoch + dt / GmatTimeConstants::SECS_PER_DAY;
               ComputeAccelerationMassFlow(now, burnData);
            }

            // Factor used to convert m/s^2 to km/s^2, and to divide out mass
            // if modeling thrust
            Real factor = 0.001;
            if (dataIsThrust)
            {
               // todo: replace string label with an int ID
               Real mass = sat->GetRealParameter("TotalMass");
               factor /= mass;
            }

            accel[0] += burnData[0] * factor;
            accel[1] += burnData[1] * factor;
            accel[2] += burnData[2] * factor;

            #ifdef DEBUG_MASS_FLOW
               MessageInterface::ShowMessage(
                     "   --> direction = [%.12le %.12le %.12le]",
                     accel[0], accel[1], accel[2]);
            #endif

            if (depleteMass)
            {
               // Set the mass flow flag on sat's tanks
               FuelTank *tank = (FuelTank*)(sat->GetRefObject(
                     Gmat::FUEL_TANK, activeTankName));
               if (tank != NULL)
                  tank->SetFlowWithoutThruster(true);

               if (order != 1)
                  throw ODEModelException("Mass depletion cannot be "
                        "performed with the selected propagator.");
               mDot -= burnData[3];
               #ifdef DEBUG_MASS_FLOW
                  MessageInterface::ShowMessage("  mDot =  %.12lf\n",
                        mDot);
               #endif
            }
            #ifdef DEBUG_MASS_FLOW
            else
               MessageInterface::ShowMessage("  mDot =  0.0\n");
            #endif

            // Apply the burns to the state vector
            if (order == 1)
            {
               // dr/dt = v
               deriv[i6]     =
               deriv[1 + i6] =
               deriv[2 + i6] = 0.0;
               deriv[3 + i6] = accel[0];
               deriv[4 + i6] = accel[1];
               deriv[5 + i6] = accel[2];

               if (mloc >= 0)
               {
                  #ifdef DEBUG_MASS_FLOW
                     MessageInterface::ShowMessage("Setting mdot in deriv[%d] "
                           "to %le\n", mloc+i, mDot);
                  #endif
                  deriv[mloc+i] = mDot;
               }
            }
            else
            {
               deriv[ i6 ] = accel[0];
               deriv[i6+1] = accel[1];
               deriv[i6+2] = accel[2];
               deriv[i6+3] = 0.0;
               deriv[i6+4] = 0.0;
               deriv[i6+5] = 0.0;
            }
         }
         else
         {
            deriv[ i6 ] =
            deriv[i6+1] =
            deriv[i6+2] =
            deriv[i6+3] =
            deriv[i6+4] =
            deriv[i6+5] = 0.0;
         }
         ++i;
      }
   }

   #ifdef DEBUG_FILETHRUST_EXE
      //ShowDerivative("FiniteThrust::GetDerivatives() AFTER compute", state,
      //               satCount);
      MessageInterface::ShowMessage
         ("     deriv[1:3] = [%18le %18le %18le]\n"
          "     deriv[4:6] = [%18le %18le %18le]\n"
          "     mdot       = %18le\n",
          deriv[0], deriv[1], deriv[2], deriv[3], deriv[4], deriv[5],
          (depleteMass ? deriv[mDotIndex] : 0.0));
      MessageInterface::ShowMessage("FiniteThrust::GetDerivatives finished\n");
   #endif

   Integer i, ix;
   if (fillSTM || fillAMatrix)
   {
      Real *aTilde;
      Real now;
      Real burnData[4];
      Integer stmSize = stmRowCount * stmRowCount;
      aTilde = new Real[stmSize];

      #ifdef DEBUG_TSF_SOLVEFOR
         MessageInterface::ShowMessage("FileThrust STM row count = %d\n",
               stmRowCount);
      #endif

      for (i = 0; i < satCount; ++i)
      {
         for (Integer j = 0; j < stmRowCount; ++j)
         {
            ix = j * stmRowCount;
            for (Integer k = 0; k < stmRowCount; ++k)
               aTilde[ix + k] = 0.0;
         }
         if (estimatingTSF)
         {
            // probably don't need this block
            if (!fillCartesian) {
                // in case ComputeAccelerationMassFlow() wasn't called
                Real scaleFactors[2];
                Real tt = epoch + dt / GmatTimeConstants::SECS_PER_DAY;
                for (UnsignedInt i = 0; i < segments->size(); ++i)
                {
                    if (((*segments)[i].segData.startEpoch <= tt) &&
                        ((*segments)[i].segData.endEpoch >= tt))
                    {
                        (*segments)[i].GetScaleFactors(scaleFactors);
                        scaleFactors[0] *= (1.0 + (*segments)[i].GetRealParameter("TSF_Epsilon"));
                        thrustSF = scaleFactors[0];
                        thrustSFinitial = tsfInitial[i];
                        break;
                    }
                }
            }

            #ifdef DEBUG_TSF_SOLVEFOR
               MessageInterface::ShowMessage("In FileThrust: ix = %d tsfEpsilonRow = %d, i = %d\n", ix, tsfEpsilonRow, i);
            #endif
            for (UnsignedInt jj = 0; jj < 3; ++jj)
            {
               ix = stmRowCount * (3 + jj);
               aTilde[ix + tsfEpsilonRow] = deriv[i * 6 + 3 + jj] * thrustSFinitial / thrustSF;
            }
         }

         Integer iStart = stmStart + i * stmSize;
         Integer element;
         for (Integer j = 0; j < stmRowCount; ++j)
         {
            for (Integer k = 0; k < stmRowCount; ++k)
            {
               element = j * stmRowCount + k;
               deriv[iStart + element] = aTilde[element];
            }
         }

         #ifdef DUMP_DERIVATIVE
            MessageInterface::ShowMessage("A-matrix:\n");
            for (Integer m = 0; m < stmRowCount; ++m)
            {
               MessageInterface::ShowMessage("      ");
               for (Integer n = 0; n < stmRowCount; ++n)
               {
                  if (n > 0)
                     MessageInterface::ShowMessage(", ");
                  MessageInterface::ShowMessage("%le", aTilde[m*stmRowCount + n]);
               }
               MessageInterface::ShowMessage("\n");
            }
            MessageInterface::ShowMessage("--------------------------------------"
               "-----------------------------------------------------------\n");
         #endif

         #ifdef DEBUG_FINITEDIFF
            MessageInterface::ShowMessage("Drag derivatives:\n   State: [");
            Integer p, q;
            for (p = 0; p < 6; ++p)
              MessageInterface::ShowMessage(" %le ", deriv[p]);
            MessageInterface::ShowMessage("]\n\n   A-Matrix: [");
            for (p = 0; p < stmRowCount; ++p)
            {
               if (p > 0)
                  MessageInterface::ShowMessage(";\n              ");
               for (q = 0; q < stmRowCount; ++q)
               {
                  MessageInterface::ShowMessage(" %le ", deriv[6+p*stmRowCount+q]);
               }
            }
            MessageInterface::ShowMessage("]\n\n");
         #endif
      }

      delete[] aTilde;
   }

   return retval;
}


//------------------------------------------------------------------------------
// Rvector6 GetDerivativesForSpacecraft(Spacecraft* sc)
//------------------------------------------------------------------------------
/**
 * Retrieves derivative data used in the force model Parameter code
 *
 * @param sc The spacecraft that supplies the needed state data
 *
 * @return The derivative data as a 6-vector
 *
 * @note: Not yet implemented for the Thrust History File reader code.
 */
//------------------------------------------------------------------------------
Rvector6 FileThrust::GetDerivativesForSpacecraft(Spacecraft* sc)
{
   Rvector6 dv;
   return dv;
}

//------------------------------------------------------------------------------
// bool SupportsDerivative(Gmat::StateElementId id)
//------------------------------------------------------------------------------
/**
 * Function used to determine if the physical model supports derivative
 * information for a specified type.
 *
 * @param id State Element ID for the derivative type
 *
 * @return true if the type is supported, false otherwise.
 */
//------------------------------------------------------------------------------
bool FileThrust::SupportsDerivative(Gmat::StateElementId id)
{
   #ifdef DEBUG_REGISTRATION
      MessageInterface::ShowMessage(
            "FileThrust checking for support for id %d\n", id);
   #endif

   if (id == Gmat::CARTESIAN_STATE)
      return true;

   if (id == Gmat::MASS_FLOW)
   {
      return true;
   }

   if (id == Gmat::ORBIT_STATE_TRANSITION_MATRIX)
      return true;

   if (id == Gmat::ORBIT_A_MATRIX)
      return true;

   return PhysicalModel::SupportsDerivative(id);
}

//------------------------------------------------------------------------------
// bool SetStart(Gmat::StateElementId id, Integer index, Integer quantity)
//------------------------------------------------------------------------------
/**
 * Function used to set the start point and size information for the state
 * vector, so that the derivative information can be placed in the correct place
 * in the derivative vector.
 *
 * @param id State Element ID for the derivative type
 * @param index Starting index in the state vector for this type of derivative
 * @param quantity Number of objects that supply this type of data
 *
 * @return true if the type is supported, false otherwise.
 */
//------------------------------------------------------------------------------
bool FileThrust::SetStart(Gmat::StateElementId id, Integer index,
      Integer quantity, Integer sizeOfType)
{
   #ifdef DEBUG_REGISTRATION
      MessageInterface::ShowMessage("FileThrust setting start data for id = %d"
            " to index %d; %d objects identified, size is %d\n", id, index,
            quantity, sizeOfType);
   #endif

   bool retval = false;

   switch (id)
   {
      case Gmat::CARTESIAN_STATE:
         satCount = quantity;
         cartIndex = index;
         fillCartesian = true;
         retval = true;
         break;

      case Gmat::MASS_FLOW:
         // todo: add mass flow bits here
//         satThrustCount = quantity;
         mDotIndex = index;
         depleteMass = true;
         retval = true;
         break;

	  case Gmat::ORBIT_STATE_TRANSITION_MATRIX:
		  stmCount = quantity;
		  stmStart = index;
		  fillSTM = true;
		  stmRowCount = Integer(sqrt((Real)sizeOfType));
		  retval = true;
		  break;

	  case Gmat::ORBIT_A_MATRIX:
		  aMatrixCount = quantity;
		  aMatrixStart = index;
		  fillAMatrix = true;
		  stmRowCount = Integer(sqrt((Real)sizeOfType));
		  retval = true;
		  break;

      default:
         break;
   }

   return retval;
}


//------------------------------------------------------------------------------
// StringArray GetSolveForList()
//------------------------------------------------------------------------------
/**
 * Retrieves solve-for parameters supported for the FileThrust force
 *
 * @return The list of object/solve-for names
 */
//------------------------------------------------------------------------------
StringArray FileThrust::GetSolveForList()
{
   solveForList.clear();

   StringArray sfors;
   std::string segName;

   for (UnsignedInt i = 0; i < segments->size(); ++i)
   {
      segName = segments->at(i).GetName();
      segName += ".";
      sfors = segments->at(i).GetStringArrayParameter("SolveFors");
      for (UnsignedInt j=0; j < sfors.size(); ++j)
         solveForList.push_back(segName + sfors[j]);
   }

   return solveForList;
}


//------------------------------------------------------------------------------
// void ComputeAccelerationMassFlow(const GmatEpoch atEpoch, Real burnData[4])
//------------------------------------------------------------------------------
/**
 * Retrieves dv data at a specified epoch from data in the thrust history file
 *
 * @param atEpoch The epoch of the data request
 * @param burnData The container receiving the acceleration and mass flow
 */
//------------------------------------------------------------------------------
void FileThrust::ComputeAccelerationMassFlow(const GmatEpoch atEpoch,
      Real burnData[4])
{
   // Start from nothin'
   burnData[0] = burnData[1] = burnData[2] = burnData[3] = 0.0;

   Real scaleFactors[2];

   // Find the segment with data covering the input epoch.  Note that if
   // segments overlap, we use the data in the first segment covering the epoch
   Integer index = -1;
   for (UnsignedInt i = 0; i < segments->size(); ++i)
   {
      if (((*segments)[i].segData.startEpoch <= atEpoch) &&
            ((*segments)[i].segData.endEpoch >= atEpoch))
      {
         index = i;
         // Factor used to convert m/s^2 to km/s^2, and to divide out mass if
         // modeling thrust
         dataIsThrust = (*segments)[i].segData.modelThrust;
         (*segments)[i].GetScaleFactors(scaleFactors);
         coordSystem = (*segments)[i].segData.cs;

         // Thrust Scale Factor Solve For
         scaleFactors[0] *= (1.0 + (*segments)[i].GetRealParameter("TSF_Epsilon"));

         thrustSF = scaleFactors[0];
         thrustSFinitial = tsfInitial[i];

         break;
      }
   }

   if (index != -1)
   {
      dataBlock[5] = (*segments)[index].segData.accelIntType;
      dataBlock[6] = (*segments)[index].segData.massIntType;

      // Interpolate the thrust/acceleration
      GetSegmentData(index, atEpoch);
      switch ((Integer)dataBlock[5])
      {
      case ThfDataSegment::LINEAR:
         LinearInterpolate(index, atEpoch);
         #ifdef DEBUG_INTERPOLATION
            MessageInterface::ShowMessage("Thrust/Acceleration: Linear Interpolation "
                     "-> VXdot = %.12le\n", dataBlock[0] * scaleFactors[0]);
         #endif
         burnData[0] = dataBlock[0] * scaleFactors[0];
         burnData[1] = dataBlock[1] * scaleFactors[0];
         burnData[2] = dataBlock[2] * scaleFactors[0];
         break;

      case ThfDataSegment::SPLINE:
         SplineInterpolate(index, atEpoch);
         #ifdef DEBUG_INTERPOLATION
            MessageInterface::ShowMessage("Thrust/Acceleration: Spline Interpolation "
                     "-> VXdot = %.12le\n", dataBlock[0] * scaleFactors[0]);
         #endif
         burnData[0] = dataBlock[0] * scaleFactors[0];
         burnData[1] = dataBlock[1] * scaleFactors[0];
         burnData[2] = dataBlock[2] * scaleFactors[0];
         break;

      case ThfDataSegment::NONE:
      default:
         #ifdef DEBUG_INTERPOLATION
            MessageInterface::ShowMessage("Thrust/Acceleration: No Interpolation "
                     "-> VXdot = %.12le\n", dataBlock[0] * scaleFactors[0]);
         #endif
         burnData[0] = dataBlock[0] * scaleFactors[0];
         burnData[1] = dataBlock[1] * scaleFactors[0];
         burnData[2] = dataBlock[2] * scaleFactors[0];
         break;
      }

      if (dataBlock[5] == dataBlock[6])
      {
         burnData[3] = dataBlock[3] * scaleFactors[1];
      }
      else
      {
         // Interpolate the mass flow, using data already collected
         switch ((Integer)dataBlock[6])
         {
         case ThfDataSegment::LINEAR:
            LinearInterpolate(index, atEpoch);
            #ifdef DEBUG_INTERPOLATION
               MessageInterface::ShowMessage("Mass Flow: Linear Interpolation "
                     "-> Mdot = %.12le\n", dataBlock[3] * scaleFactors[1]);
            #endif
            burnData[3] = dataBlock[3] * scaleFactors[1];
            break;

         case ThfDataSegment::SPLINE:
            SplineInterpolate(index, atEpoch);
            #ifdef DEBUG_INTERPOLATION
               MessageInterface::ShowMessage("Mass Flow: Spline Interpolation "
                     "-> Mdot = %.12le\n", dataBlock[3] * scaleFactors[1]);
            #endif
            burnData[3] = dataBlock[3] * scaleFactors[1];
            break;

         case ThfDataSegment::NONE:
         default:
            #ifdef DEBUG_INTERPOLATION
               MessageInterface::ShowMessage("Mass Flow: No Interpolation "
                     "-> Mdot = %.12le\n", dataBlock[3] * scaleFactors[1]);
            #endif
            burnData[3] = dataBlock[3] * scaleFactors[1];
            break;
         }
      }

      if (burnData[3] != 0.0)
      {
         #ifdef DEBUG_MASS_FLOW
            MessageInterface::ShowMessage("Accessing segment %d at epoch "
                  "%.12lf; [3] = %le\n", index, atEpoch, burnData[3]);
            MessageInterface::ShowMessage("   %d mass sources\n",
                  (*segments)[index].massSource.size());
         #endif
         if (segments->size() > index)
         {
            if ((*segments)[index].massSource.size() > 0)
               activeTankName = (*segments)[index].massSource[0];
            else
            {
               if (massFlowWarningNeeded)
               {
                  MessageInterface::ShowMessage("Warning: The Thrust History "
                        "File force %s cannot deplete mass: no mass source "
                        "is identified\n", instanceName.c_str());
                  massFlowWarningNeeded = false;
               }
               burnData[3] = 0.0;
               depleteMass = false;
               massFlowWarningNeeded = false;
            }
         }
      }

      if (coordSystem != NULL)
         ConvertDirectionToInertial(burnData, burnData, atEpoch);
   }
}


//------------------------------------------------------------------------------
// void ComputeAccelerationMassFlow(const GmatEpoch atEpoch, Real burnData[4])
//------------------------------------------------------------------------------
/**
* Retrieves dv data at a specified epoch from data in the thrust history file
*
* @param atEpoch The epoch of the data request
* @param burnData The container receiving the acceleration and mass flow
*/
//------------------------------------------------------------------------------
void FileThrust::ComputeAccelerationMassFlow(const GmatTime &atEpoch,
   Real burnData[4])
{
   // Start from nothin'
   burnData[0] = burnData[1] = burnData[2] = burnData[3] = 0.0;

   Real scaleFactors[2];

   // Find the segment with data covering the input epoch.  Note that if
   // segments overlap, we use the data in the first segment covering the epoch
   Integer index = -1;
   for (UnsignedInt i = 0; i < segments->size(); ++i)
   {
      if (((*segments)[i].segData.startEpochGT <= atEpoch) &&
         ((*segments)[i].segData.endEpochGT >= atEpoch))
      {
         index = i;
         // Factor used to convert m/s^2 to km/s^2, and to divide out mass if
         // modeling thrust
         dataIsThrust = (*segments)[i].segData.modelThrust;
         (*segments)[i].GetScaleFactors(scaleFactors);
         coordSystem = (*segments)[i].segData.cs;

         // Thrust Scale Factor Solve For
         scaleFactors[0] *= (1.0 + (*segments)[i].GetRealParameter("TSF_Epsilon"));

         thrustSF = scaleFactors[0];
         thrustSFinitial = tsfInitial[i];

         break;
      }
   }

   if (index != -1)
   {
      dataBlock[5] = (*segments)[index].segData.accelIntType;
      dataBlock[6] = (*segments)[index].segData.massIntType;

      // Interpolate the thrust/acceleration
      GetSegmentData(index, atEpoch);
      switch ((Integer)dataBlock[5])
      {
      case ThfDataSegment::LINEAR:
         LinearInterpolate(index, atEpoch);
#ifdef DEBUG_INTERPOLATION
         MessageInterface::ShowMessage("Thrust/Acceleration: Linear Interpolation "
            "-> VXdot = %.12le\n", dataBlock[0] * scaleFactors[0]);
#endif
         burnData[0] = dataBlock[0] * scaleFactors[0];
         burnData[1] = dataBlock[1] * scaleFactors[0];
         burnData[2] = dataBlock[2] * scaleFactors[0];
         break;

      case ThfDataSegment::SPLINE:
         SplineInterpolate(index, atEpoch);
#ifdef DEBUG_INTERPOLATION
         MessageInterface::ShowMessage("Thrust/Acceleration: Spline Interpolation "
            "-> VXdot = %.12le\n", dataBlock[0] * scaleFactors[0]);
#endif
         burnData[0] = dataBlock[0] * scaleFactors[0];
         burnData[1] = dataBlock[1] * scaleFactors[0];
         burnData[2] = dataBlock[2] * scaleFactors[0];
         break;

      case ThfDataSegment::NONE:
      default:
#ifdef DEBUG_INTERPOLATION
         MessageInterface::ShowMessage("Thrust/Acceleration: No Interpolation "
            "-> VXdot = %.12le\n", dataBlock[0] * scaleFactors[0]);
#endif
         burnData[0] = dataBlock[0] * scaleFactors[0];
         burnData[1] = dataBlock[1] * scaleFactors[0];
         burnData[2] = dataBlock[2] * scaleFactors[0];
         break;
      }

      if (dataBlock[5] == dataBlock[6])
      {
         burnData[3] = dataBlock[3] * scaleFactors[1];
      }
      else
      {
         // Interpolate the mass flow, using data already collected
         switch ((Integer)dataBlock[6])
         {
         case ThfDataSegment::LINEAR:
            LinearInterpolate(index, atEpoch);
#ifdef DEBUG_INTERPOLATION
            MessageInterface::ShowMessage("Mass Flow: Linear Interpolation "
               "-> Mdot = %.12le\n", dataBlock[3] * scaleFactors[1]);
#endif
            burnData[3] = dataBlock[3] * scaleFactors[1];
            break;

         case ThfDataSegment::SPLINE:
            SplineInterpolate(index, atEpoch);
#ifdef DEBUG_INTERPOLATION
            MessageInterface::ShowMessage("Mass Flow: Spline Interpolation "
               "-> Mdot = %.12le\n", dataBlock[3] * scaleFactors[1]);
#endif
            burnData[3] = dataBlock[3] * scaleFactors[1];
            break;

         case ThfDataSegment::NONE:
         default:
#ifdef DEBUG_INTERPOLATION
            MessageInterface::ShowMessage("Mass Flow: No Interpolation "
               "-> Mdot = %.12le\n", dataBlock[3] * scaleFactors[1]);
#endif
            burnData[3] = dataBlock[3] * scaleFactors[1];
            break;
         }
      }

      if (burnData[3] != 0.0)
      {
#ifdef DEBUG_MASS_FLOW
         MessageInterface::ShowMessage("Accessing segment %d at epoch "
            "%s; [3] = %le\n", index, atEpoch.ToString(), burnData[3]);
         MessageInterface::ShowMessage("   %d mass sources\n",
            (*segments)[index].massSource.size());
#endif
         if (segments->size() > index)
         {
            if ((*segments)[index].massSource.size() > 0)
               activeTankName = (*segments)[index].massSource[0];
            else
            {
               if (massFlowWarningNeeded)
               {
                  MessageInterface::ShowMessage("Warning: The Thrust History "
                     "File force %s cannot deplete mass: no mass source "
                     "is identified\n", instanceName.c_str());
                  massFlowWarningNeeded = false;
               }
               burnData[3] = 0.0;
               depleteMass = false;
               massFlowWarningNeeded = false;
            }
         }
      }

      if (coordSystem != NULL)
         ConvertDirectionToInertial(burnData, burnData, atEpoch);
   }
}


//------------------------------------------------------------------------------
// void GetSegmentData(Integer atIndex, GmatEpoch atEpoch)
//------------------------------------------------------------------------------
/**
 * Retrieves the segment data for the segment containing the input epoch
 *
 * @param atIndex Index of the segment containing the data
 * @param atEpoch The epoch of the requested data
 */
//------------------------------------------------------------------------------
void FileThrust::GetSegmentData(Integer atIndex, GmatEpoch atEpoch)
{
   #ifdef DEBUG_INTERPOLATION
      MessageInterface::ShowMessage("Entered GetSegmentData(%d, %.12lf)\n",
            atIndex, atEpoch);
   #endif

   dataBlock[0] = dataBlock[1] = dataBlock[2] = dataBlock[3] = dataBlock[4] = 0.0;

   // If at the end point; use its data
   if ((*segments)[atIndex].segData.endEpoch == atEpoch)
   {
      Integer i = (*segments)[atIndex].segData.profile.size()-1;
      dataBlock[0] = (*segments)[atIndex].segData.profile[i].vector[0];
      dataBlock[1] = (*segments)[atIndex].segData.profile[i].vector[1];
      dataBlock[2] = (*segments)[atIndex].segData.profile[i].vector[2];
      dataBlock[3] = (*segments)[atIndex].segData.profile[i].mdot;
      dataBlock[4] = (*segments)[atIndex].segData.profile[i].time;
   }
   else
   {
      Real offset = atEpoch - (*segments)[atIndex].segData.startEpoch;

      switch ((Integer)dataBlock[5])
      {
      case ThfDataSegment::NONE:
      case ThfDataSegment::LINEAR:
         for (UnsignedInt i = 0; i < (*segments)[atIndex].segData.profile.size()-1; ++i)
         {
            if (((*segments)[atIndex].segData.profile[i].time <= offset) &&
                ((*segments)[atIndex].segData.profile[i+1].time > offset))
            {
               dataBlock[0] = (*segments)[atIndex].segData.profile[i].vector[0];
               dataBlock[1] = (*segments)[atIndex].segData.profile[i].vector[1];
               dataBlock[2] = (*segments)[atIndex].segData.profile[i].vector[2];
               if ((dataBlock[6] == ThfDataSegment::LINEAR) ||
                   (dataBlock[6] == ThfDataSegment::NONE))
                  dataBlock[3] = (*segments)[atIndex].segData.profile[i].mdot;
               dataBlock[4] = (*segments)[atIndex].segData.profile[i].time;
               break;
            }
         }
         break;

      case ThfDataSegment::SPLINE:
         {
            bool reload = false;
            if (indexPair[0] != atIndex)
               reload = true;
            indexPair[0] = atIndex;
            Integer proIndex = -1;

            // Look up the profile index
            for (UnsignedInt i = 0;
                  i < (*segments)[atIndex].segData.profile.size()-1; ++i)
            {
               if (((*segments)[atIndex].segData.profile[i].time <= offset) &&
                   ((*segments)[atIndex].segData.profile[i+1].time > offset))
               {
                  proIndex = i;
                  break;
               }
            }

            // Now set the indices for the buffer.  This code selects the region
            // between the 2nd and 3rd points when possible.
            if (proIndex == 0)
               proIndex = 1;
            else if (proIndex >
                     (Integer)((*segments)[atIndex].segData.profile.size()) - 4)
               proIndex = (*segments)[atIndex].segData.profile.size() - 4;

            interpolatorData[0] = proIndex - 1;
            interpolatorData[1] = proIndex;
            interpolatorData[2] = proIndex + 1;
            interpolatorData[3] = proIndex + 2;
            interpolatorData[4] = proIndex + 3;
         }
         break;

      default:
         break;
      }

      // Setup for mass flow
      switch ((Integer)dataBlock[6])
      {
      case ThfDataSegment::NONE:
      case ThfDataSegment::LINEAR:
         for (UnsignedInt i = 0;
               i < (*segments)[atIndex].segData.profile.size()-1; ++i)
         {
            if (((*segments)[atIndex].segData.profile[i].time <= offset) &&
                ((*segments)[atIndex].segData.profile[i+1].time > offset))
            {
               dataBlock[3] = (*segments)[atIndex].segData.profile[i].mdot;
               break;
            }
         }
         break;

      case ThfDataSegment::SPLINE:
         if ((*segments)[atIndex].segData.accelIntType != ThfDataSegment::SPLINE)
         {
            bool reload = false;
            if (indexPair[0] != atIndex)
               reload = true;
            indexPair[0] = atIndex;
            Integer proIndex = -1;

            // Look up the profile index
            for (UnsignedInt i = 0;
                  i < (*segments)[atIndex].segData.profile.size()-1; ++i)
            {
               if (((*segments)[atIndex].segData.profile[i].time <= offset) &&
                   ((*segments)[atIndex].segData.profile[i+1].time > offset))
               {
                  proIndex = i;
                  break;
               }
            }

            // Now set the indices for the buffer.  This code selects the region
            // between the 2nd and 3rd points when possible.
            if (proIndex == 0)
               proIndex = 1;
            else if (proIndex >
                     (Integer)((*segments)[atIndex].segData.profile.size()) - 4)
               proIndex = (*segments)[atIndex].segData.profile.size() - 4;

            interpolatorData[0] = proIndex - 1;
            interpolatorData[1] = proIndex;
            interpolatorData[2] = proIndex + 1;
            interpolatorData[3] = proIndex + 2;
            interpolatorData[4] = proIndex + 3;
         }
         break;

      default:
         break;
      }
   }
}


//------------------------------------------------------------------------------
// void GetSegmentData(Integer atIndex, GmatTime atEpoch)
//------------------------------------------------------------------------------
/**
* Retrieves the segment data for the segment containing the input epoch
*
* @param atIndex Index of the segment containing the data
* @param atEpoch The epoch of the requested data
*/
//------------------------------------------------------------------------------
void FileThrust::GetSegmentData(Integer atIndex, const GmatTime &atEpoch)
{
#ifdef DEBUG_INTERPOLATION
   MessageInterface::ShowMessage("Entered GetSegmentData(%d, %s)\n",
      atIndex, atEpoch.ToString());
#endif

   dataBlock[0] = dataBlock[1] = dataBlock[2] = dataBlock[3] = dataBlock[4] = 0.0;

   // If at the end point; use its data
   if ((*segments)[atIndex].segData.endEpochGT == atEpoch)
   {
      Integer i = (*segments)[atIndex].segData.profile.size() - 1;
      dataBlock[0] = (*segments)[atIndex].segData.profile[i].vector[0];
      dataBlock[1] = (*segments)[atIndex].segData.profile[i].vector[1];
      dataBlock[2] = (*segments)[atIndex].segData.profile[i].vector[2];
      dataBlock[3] = (*segments)[atIndex].segData.profile[i].mdot;
      dataBlock[4] = (*segments)[atIndex].segData.profile[i].time;
   }
   else
   {
      GmatTime offsetGT = atEpoch - (*segments)[atIndex].segData.startEpochGT;
      Real offset = offsetGT.GetTimeInSec() / GmatTimeConstants::SECS_PER_DAY;

      switch ((Integer)dataBlock[5])
      {
      case ThfDataSegment::NONE:
      case ThfDataSegment::LINEAR:
         for (UnsignedInt i = 0; i < (*segments)[atIndex].segData.profile.size() - 1; ++i)
         {
            if (((*segments)[atIndex].segData.profile[i].time <= offset) &&
               ((*segments)[atIndex].segData.profile[i + 1].time > offset))
            {
               dataBlock[0] = (*segments)[atIndex].segData.profile[i].vector[0];
               dataBlock[1] = (*segments)[atIndex].segData.profile[i].vector[1];
               dataBlock[2] = (*segments)[atIndex].segData.profile[i].vector[2];
               if ((dataBlock[6] == ThfDataSegment::LINEAR) ||
                  (dataBlock[6] == ThfDataSegment::NONE))
                  dataBlock[3] = (*segments)[atIndex].segData.profile[i].mdot;
               dataBlock[4] = (*segments)[atIndex].segData.profile[i].time;
               break;
            }
         }
         break;

      case ThfDataSegment::SPLINE:
      {
         bool reload = false;
         if (indexPair[0] != atIndex)
            reload = true;
         indexPair[0] = atIndex;
         Integer proIndex = -1;

         // Look up the profile index
         for (UnsignedInt i = 0;
            i < (*segments)[atIndex].segData.profile.size() - 1; ++i)
         {
            if (((*segments)[atIndex].segData.profile[i].time <= offset) &&
               ((*segments)[atIndex].segData.profile[i + 1].time > offset))
            {
               proIndex = i;
               break;
            }
         }

         // Now set the indices for the buffer.  This code selects the region
         // between the 2nd and 3rd points when possible.
         if (proIndex == 0)
            proIndex = 1;
         else if (proIndex >
            (Integer)((*segments)[atIndex].segData.profile.size()) - 4)
            proIndex = (*segments)[atIndex].segData.profile.size() - 4;

         interpolatorData[0] = proIndex - 1;
         interpolatorData[1] = proIndex;
         interpolatorData[2] = proIndex + 1;
         interpolatorData[3] = proIndex + 2;
         interpolatorData[4] = proIndex + 3;
      }
      break;

      default:
         break;
      }

      // Setup for mass flow
      switch ((Integer)dataBlock[6])
      {
      case ThfDataSegment::NONE:
      case ThfDataSegment::LINEAR:
         for (UnsignedInt i = 0;
            i < (*segments)[atIndex].segData.profile.size() - 1; ++i)
         {
            if (((*segments)[atIndex].segData.profile[i].time <= offset) &&
               ((*segments)[atIndex].segData.profile[i + 1].time > offset))
            {
               dataBlock[3] = (*segments)[atIndex].segData.profile[i].mdot;
               break;
            }
         }
         break;

      case ThfDataSegment::SPLINE:
         if ((*segments)[atIndex].segData.accelIntType != ThfDataSegment::SPLINE)
         {
            bool reload = false;
            if (indexPair[0] != atIndex)
               reload = true;
            indexPair[0] = atIndex;
            Integer proIndex = -1;

            // Look up the profile index
            for (UnsignedInt i = 0;
               i < (*segments)[atIndex].segData.profile.size() - 1; ++i)
            {
               if ((offset >= (*segments)[atIndex].segData.profile[i].time) &&
                  (offset < (*segments)[atIndex].segData.profile[i + 1].time))
               {
                  proIndex = i;
                  break;
               }
            }

            // Now set the indices for the buffer.  This code selects the region
            // between the 2nd and 3rd points when possible.
            if (proIndex == 0)
               proIndex = 1;
            else if (proIndex >
               (Integer)((*segments)[atIndex].segData.profile.size()) - 4)
               proIndex = (*segments)[atIndex].segData.profile.size() - 4;

            interpolatorData[0] = proIndex - 1;
            interpolatorData[1] = proIndex;
            interpolatorData[2] = proIndex + 1;
            interpolatorData[3] = proIndex + 2;
            interpolatorData[4] = proIndex + 3;
         }
         break;

      default:
         break;
      }
   }
}


//------------------------------------------------------------------------------
// void LinearInterpolate(Integer atIndex, GmatEpoch atEpoch)
//------------------------------------------------------------------------------
/**
 * Retrieves linearly interpolated segment data for the input epoch
 *
 * @param atIndex Index of the segment containing the data
 * @param atEpoch The epoch of the requested data
 */
//------------------------------------------------------------------------------
void FileThrust::LinearInterpolate(Integer atIndex, GmatEpoch atEpoch)
{
   GetSegmentData(atIndex, atEpoch);
   bool includeMass = false;

   if ((*segments)[atIndex].segData.endEpoch == atEpoch)
   {
      return;
   }

   Real offset = atEpoch - (*segments)[atIndex].segData.startEpoch;
   for (UnsignedInt i = 0; i < (*segments)[atIndex].segData.profile.size()-1; ++i)
   {
      if (((*segments)[atIndex].segData.profile[i].time <= offset) &&
          ((*segments)[atIndex].segData.profile[i+1].time > offset))
      {
         dataSet[0][0] = dataBlock[0];
         dataSet[0][1] = dataBlock[1];
         dataSet[0][2] = dataBlock[2];
         dataSet[0][3] = dataBlock[3];
         dataSet[0][4] = (*segments)[atIndex].segData.profile[i].time;

         dataSet[1][0] = (*segments)[atIndex].segData.profile[i+1].vector[0];
         dataSet[1][1] = (*segments)[atIndex].segData.profile[i+1].vector[1];
         dataSet[1][2] = (*segments)[atIndex].segData.profile[i+1].vector[2];
         dataSet[1][3] = (*segments)[atIndex].segData.profile[i+1].mdot;
         dataSet[1][4] = (*segments)[atIndex].segData.profile[i+1].time;
         break;
      }
   }

   Real pct = 0.0;
   if ((dataSet[1][4] != dataSet[0][4]))
      pct = (offset - dataSet[0][4]) / (dataSet[1][4] - dataSet[0][4]);

   if (dataBlock[5] == ThfDataSegment::LINEAR)
   {
      dataBlock[0] = dataSet[0][0] + pct * (dataSet[1][0] - dataSet[0][0]);
      dataBlock[1] = dataSet[0][1] + pct * (dataSet[1][1] - dataSet[0][1]);
      dataBlock[2] = dataSet[0][2] + pct * (dataSet[1][2] - dataSet[0][2]);
   }
   if (dataBlock[6] == ThfDataSegment::LINEAR)
      dataBlock[3] = dataSet[0][3] + pct * (dataSet[1][3] - dataSet[0][3]);

   #ifdef DEBUG_INTERPOLATION
      MessageInterface::ShowMessage("Linear Interpolating to offset %.12lf "
            "using t, vector, mdot:\n", offset);
      MessageInterface::ShowMessage("   %.12lf [%.12le  %.12le  %.12le] "
            "%.12le\n   %.12lf [%.12le  %.12le  %.12le] %.12le\n",
            dataSet[0][4], dataSet[0][0], dataSet[0][1], dataSet[0][2],
            dataSet[0][3], dataSet[1][4], dataSet[1][0], dataSet[1][1],
            dataSet[1][2], dataSet[1][3]);

      MessageInterface::ShowMessage("-> %.12lf [%.12le  %.12le  %.12le] "
            "%.12le\n", offset, dataBlock[0], dataBlock[1], dataBlock[2], dataBlock[3]);
   #endif
}


//------------------------------------------------------------------------------
// void LinearInterpolate(Integer atIndex, GmatTime atEpoch)
//------------------------------------------------------------------------------
/**
* Retrieves linearly interpolated segment data for the input epoch
*
* @param atIndex Index of the segment containing the data
* @param atEpoch The epoch of the requested data
*/
//------------------------------------------------------------------------------
void FileThrust::LinearInterpolate(Integer atIndex, const GmatTime &atEpoch)
{
   GetSegmentData(atIndex, atEpoch);
   bool includeMass = false;

   if ((*segments)[atIndex].segData.endEpochGT == atEpoch)
   {
      return;
   }
   GmatTime offsetGT1 = atEpoch - (*segments)[atIndex].segData.startEpochGT;
   Real offset = offsetGT1.GetTimeInSec() / GmatTimeConstants::SECS_PER_DAY;
   for (UnsignedInt i = 0; i < (*segments)[atIndex].segData.profile.size() - 1; ++i)
   {
      if (((*segments)[atIndex].segData.profile[i].time <= offset) &&
         ((*segments)[atIndex].segData.profile[i + 1].time > offset))
      {
         dataSet[0][0] = dataBlock[0];
         dataSet[0][1] = dataBlock[1];
         dataSet[0][2] = dataBlock[2];
         dataSet[0][3] = dataBlock[3];
         dataSet[0][4] = (*segments)[atIndex].segData.profile[i].time;

         dataSet[1][0] = (*segments)[atIndex].segData.profile[i + 1].vector[0];
         dataSet[1][1] = (*segments)[atIndex].segData.profile[i + 1].vector[1];
         dataSet[1][2] = (*segments)[atIndex].segData.profile[i + 1].vector[2];
         dataSet[1][3] = (*segments)[atIndex].segData.profile[i + 1].mdot;
         dataSet[1][4] = (*segments)[atIndex].segData.profile[i + 1].time;
         break;
      }
   }

   Real pct = 0.0;
   if ((dataSet[1][4] != dataSet[0][4]))
      pct = (offset - dataSet[0][4]) / (dataSet[1][4] - dataSet[0][4]);

   if (dataBlock[5] == ThfDataSegment::LINEAR)
   {
      dataBlock[0] = dataSet[0][0] + pct * (dataSet[1][0] - dataSet[0][0]);
      dataBlock[1] = dataSet[0][1] + pct * (dataSet[1][1] - dataSet[0][1]);
      dataBlock[2] = dataSet[0][2] + pct * (dataSet[1][2] - dataSet[0][2]);
   }
   if (dataBlock[6] == ThfDataSegment::LINEAR)
      dataBlock[3] = dataSet[0][3] + pct * (dataSet[1][3] - dataSet[0][3]);

#ifdef DEBUG_INTERPOLATION
   MessageInterface::ShowMessage("Linear Interpolating to offset %s "
      "using t, vector, mdot:\n", offsetGT.ToString());
   MessageInterface::ShowMessage("   %.12lf [%.12le  %.12le  %.12le] "
      "%.12le\n   %.12lf [%.12le  %.12le  %.12le] %.12le\n",
      dataSet[0][4], dataSet[0][0], dataSet[0][1], dataSet[0][2],
      dataSet[0][3], dataSet[1][4], dataSet[1][0], dataSet[1][1],
      dataSet[1][2], dataSet[1][3]);

   MessageInterface::ShowMessage("-> %s [%.12le  %.12le  %.12le] "
      "%.12le\n", offsetGT, dataBlock[0], dataBlock[1], dataBlock[2], dataBlock[3]);
#endif
}


//------------------------------------------------------------------------------
// void SplineInterpolate(Integer atIndex, GmatEpoch atEpoch)
//------------------------------------------------------------------------------
/**
 * Retrieves spline interpolated segment data for the input epoch
 *
 * @param atIndex Index of the segment containing the data
 * @param atEpoch The epoch of the requested data
 */
//------------------------------------------------------------------------------
void FileThrust::SplineInterpolate(Integer atIndex, GmatEpoch atEpoch)
{
   // Handle case of too few points by falling back to linear interpolation
   if ((*segments)[atIndex].segData.profile.size() < 5)
   {
      if (warnTooFewPoints)
      {
         MessageInterface::ShowMessage("Cannot perform spline interpolation: "
               "the thrust history data segment contains %d points, but spline "
               "interpolation requires at least 5.  Linear interpolation will "
               "be applied instead.\n",
               (*segments)[atIndex].segData.profile.size());
         warnTooFewPoints = false;
      }
      LinearInterpolate(atIndex, atEpoch);
   }

   if (spliner == NULL)
      spliner = new NotAKnotInterpolator("SplineInterpolator", 4);

   if (spliner == NULL)
      throw ODEModelException("The cubic spline interpolator failed to build");

   Real data[4];
   if (interpolatorData[1] == -1)
   {
      // Ouside of the span; do nothing
      data[0] =
      data[1] =
      data[2] =
      data[3] = 0.0;
   }
   else
   {
      // Reload the interpolator.  For now, this is done at each call.
      spliner->Clear();
      for (UnsignedInt i = 0; i < 5; ++i)
      {
         data[0] = (*segments)[atIndex].segData.profile[interpolatorData[i]].vector[0];
         data[1] = (*segments)[atIndex].segData.profile[interpolatorData[i]].vector[1];
         data[2] = (*segments)[atIndex].segData.profile[interpolatorData[i]].vector[2];
         data[3] = (*segments)[atIndex].segData.profile[interpolatorData[i]].mdot;

         spliner->AddPoint((*segments)[atIndex].segData.profile[interpolatorData[i]].time, data);
      }

      Real offset = atEpoch - (*segments)[atIndex].segData.startEpoch;
      spliner->Interpolate(offset, data);
   }

   if (dataBlock[5] == ThfDataSegment::SPLINE)
   {
      dataBlock[0] = data[0];
      dataBlock[1] = data[1];
      dataBlock[2] = data[2];
   }
   if (dataBlock[6] == ThfDataSegment::SPLINE)
      dataBlock[3] = data[3];
}


//------------------------------------------------------------------------------
// void SplineInterpolate(Integer atIndex, GmatTime atEpoch)
//------------------------------------------------------------------------------
/**
* Retrieves spline interpolated segment data for the input epoch
*
* @param atIndex Index of the segment containing the data
* @param atEpoch The epoch of the requested data
*/
//------------------------------------------------------------------------------
void FileThrust::SplineInterpolate(Integer atIndex, const GmatTime &atEpoch)
{
   // Handle case of too few points by falling back to linear interpolation
   if ((*segments)[atIndex].segData.profile.size() < 5)
   {
      if (warnTooFewPoints)
      {
         MessageInterface::ShowMessage("Cannot perform spline interpolation: "
            "the thrust history data segment contains %d points, but spline "
            "interpolation requires at least 5.  Linear interpolation will "
            "be applied instead.\n",
            (*segments)[atIndex].segData.profile.size());
         warnTooFewPoints = false;
      }
      LinearInterpolate(atIndex, atEpoch);
   }

   if (spliner == NULL)
      spliner = new NotAKnotInterpolator("SplineInterpolator", 4);

   if (spliner == NULL)
      throw ODEModelException("The cubic spline interpolator failed to build");

   Real data[4];
   if (interpolatorData[1] == -1)
   {
      // Ouside of the span; do nothing
      data[0] =
         data[1] =
         data[2] =
         data[3] = 0.0;
   }
   else
   {
      // Reload the interpolator.  For now, this is done at each call.
      spliner->Clear();
      for (UnsignedInt i = 0; i < 5; ++i)
      {
         data[0] = (*segments)[atIndex].segData.profile[interpolatorData[i]].vector[0];
         data[1] = (*segments)[atIndex].segData.profile[interpolatorData[i]].vector[1];
         data[2] = (*segments)[atIndex].segData.profile[interpolatorData[i]].vector[2];
         data[3] = (*segments)[atIndex].segData.profile[interpolatorData[i]].mdot;

         spliner->AddPoint((*segments)[atIndex].segData.profile[interpolatorData[i]].time, data);
      }

      GmatTime offsetGT = atEpoch - (*segments)[atIndex].segData.startEpochGT;
      Real offset = offsetGT.GetTimeInSec() / GmatTimeConstants::SECS_PER_DAY;
      spliner->Interpolate(offset, data);
   }

   if (dataBlock[5] == ThfDataSegment::SPLINE)
   {
      dataBlock[0] = data[0];
      dataBlock[1] = data[1];
      dataBlock[2] = data[2];
   }
   if (dataBlock[6] == ThfDataSegment::SPLINE)
      dataBlock[3] = data[3];
}


//------------------------------------------------------------------------------
// void ConvertDirectionToInertial(Real *dir, Real *dirInertial, Real epoch)
//------------------------------------------------------------------------------
/*
 * Converts thrust direction to inertial frame
 *
 * @param dir  Thrust/accel direction in internal frame
 * @param dirInertial  Thrust direction in inertial frame
 * @param epoch  Epoch to be used for conversion
 */
//------------------------------------------------------------------------------
void FileThrust::ConvertDirectionToInertial(Real *dir, Real *dirInertial, Real epoch)
{
   #ifdef DEBUG_FILETHRUST_EXE
      MessageInterface::ShowMessage("Rotating into a new frame\n");
   #endif

   Real inDir[6], outDir[6];
   for (Integer i=0; i<3; i++)
      inDir[i] = dir[i];
   for (Integer i=3; i<6; i++)
      inDir[i] = 0.0;

   // Now rotate to base system axes, we don't want to translate so
   // set coincident to true
   coordSystem->ToBaseSystem(A1Mjd(epoch), inDir, outDir, true);

   dirInertial[0] = outDir[0];
   dirInertial[1] = outDir[1];
   dirInertial[2] = outDir[2];
}


//------------------------------------------------------------------------------
// void ConvertDirectionToInertial(Real *dir, Real *dirInertial, GmatTime epoch)
//------------------------------------------------------------------------------
/*
* Converts thrust direction to inertial frame
*
* @param dir  Thrust/accel direction in internal frame
* @param dirInertial  Thrust direction in inertial frame
* @param epoch  Epoch to be used for conversion
*/
//------------------------------------------------------------------------------
void FileThrust::ConvertDirectionToInertial(Real *dir, Real *dirInertial,
   const GmatTime &epoch)
{
#ifdef DEBUG_FILETHRUST_EXE
   MessageInterface::ShowMessage("Rotating into a new frame\n");
#endif

   Real inDir[6], outDir[6];
   for (Integer i = 0; i<3; i++)
      inDir[i] = dir[i];
   for (Integer i = 3; i<6; i++)
      inDir[i] = 0.0;

   // Now rotate to base system axes, we don't want to translate so
   // set coincident to true
   coordSystem->ToBaseSystem(epoch, inDir, outDir, true);

   dirInertial[0] = outDir[0];
   dirInertial[1] = outDir[1];
   dirInertial[2] = outDir[2];
}


//------------------------------------------------------------------------------
// const std::vector<ThrustSegment> GetAllThrustSegments()
//------------------------------------------------------------------------------
/**
 * Accessor for the ThrustSegment array
 *
 * @return The ThrustSegment array
 */
//------------------------------------------------------------------------------
const std::vector<ThrustSegment> FileThrust::GetAllThrustSegments()
{
	return *segments;
}


//------------------------------------------------------------------------------
// bool IsEstimationParameterValid(const Integer item)
//------------------------------------------------------------------------------
/**
 * Checks to see if a parameter can be estimated
 *
 * @param item The ID for the parameter, as known in the estimation system
 *
 * @return true if estimation is available, false if not
 */
//------------------------------------------------------------------------------
bool FileThrust::IsEstimationParameterValid(const Integer item)
{
   // convert Estimation ID to object parameter ID
   Integer id = item - type * ESTIMATION_TYPE_ALLOCATION;

   #ifdef DEBUG_TSF_SOLVEFOR
      MessageInterface::ShowMessage("Testing estimation capability for "
            "ID %d (%s)\n", id, GetParameterText(id).c_str());
   #endif

   if (id == tsfID)
      return true;

   return GmatBase::IsEstimationParameterValid(id);
}


//------------------------------------------------------------------------------
// bool SetPrecisionTimeFlag(bool onOff)
//------------------------------------------------------------------------------
/**
* Set whether the thrust segment is using precision time or not
*
* @param onOff Flag indicating whether the thrust segment uses precision time
*
* @return Returns the value of the onOff flag passed in
*/
//------------------------------------------------------------------------------
bool FileThrust::SetPrecisionTimeFlag(bool onOff)
{
   hasPrecisionTime = onOff;

   if (segments != NULL)
      for (ThrustSegment seg : *segments)
         seg.SetPrecisionTimeFlag(onOff);

   return hasPrecisionTime;
}

