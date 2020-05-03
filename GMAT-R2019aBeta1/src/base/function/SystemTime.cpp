//$Id$
//------------------------------------------------------------------------------
//                           GetSystemTime
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool
//
// Copyright (c) 2002 - 2015 United States Government as represented by the
// Administrator of the National Aeronautics and Space Administration.
// All Other Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// You may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
// http://www.apache.org/licenses/LICENSE-2.0.
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied.   See the License for the specific language
// governing permissions and limitations under the License.
//
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under the FDSS II
// contract, Task Order 08
//
// Author: Claire R. Conway, Thinking Systems, Inc.
// Created: Jan 30, 2018
/**
 * 
 */
//------------------------------------------------------------------------------

#include "SystemTime.hpp"
#include "FunctionException.hpp"
#include "RealUtilities.hpp"       // for SystemTime()
#include "MessageInterface.hpp"
#include <chrono>
#include "StringWrapper.hpp"
#include "NumberWrapper.hpp"

//#define DEBUG_FUNCTION_INIT
//#define DEBUG_FUNCTION_EXEC


//#ifndef DEBUG_MEMORY
//#define DEBUG_MEMORY
//#endif

#ifdef DEBUG_MEMORY
#include "MemoryTracker.hpp"
#endif

//---------------------------------
// static data
//---------------------------------


//------------------------------------------------------------------------------
//  SystemTime(std::string typeStr, std::string name)
//------------------------------------------------------------------------------
/**
 * Constructs the SystemTime object (default constructor).
 *
 * @param <typeStr> String text identifying the object type
 * @param <name>   Name for the object
 */
//------------------------------------------------------------------------------
SystemTime::SystemTime(const std::string &typeStr, const std::string &name) :
   BuiltinGmatFunction  (typeStr, name),
   outputWrapper        (NULL)
{
   objectTypeNames.push_back(typeStr);
   objectTypeNames.push_back("SystemTime");

   // Build input and output arrays. Function interface is:
   // SystemTime(time)

   // Add dummy input names
   inputNames.push_back("__SystemTime_input_1_time__");
   inputArgMap.insert(std::make_pair("__SystemTime_input_1_time__", (ElementWrapper*) NULL));

   // Add dummy output names
   outputNames.push_back("__SystemTime_output_1_time__");
   outputArgMap.insert(std::make_pair("__SystemTime_output_1_time__", (ElementWrapper*) NULL));
   outputWrapperTypes.push_back(Gmat::STRING_OBJECT_WT);
   outputRowCounts.push_back(1);
   outputColCounts.push_back(1);
}

//------------------------------------------------------------------------------
//  ~SystemTime(void)
//------------------------------------------------------------------------------
/**
 * Destroys the SystemTime object (destructor).
 */
//------------------------------------------------------------------------------
SystemTime::~SystemTime()
{
}


//------------------------------------------------------------------------------
//  SystemTime(const SystemTime &f)
//------------------------------------------------------------------------------
/**
 * Constructs the SystemTime object (copy constructor).
 *
 * @param <f> Object that is copied
 */
//------------------------------------------------------------------------------
SystemTime::SystemTime(const SystemTime &f) :
   BuiltinGmatFunction  (f),
   outputWrapper        (NULL)
{
}


//------------------------------------------------------------------------------
//  SystemTime& operator=(const SystemTime &f)
//------------------------------------------------------------------------------
/**
 * Sets one SystemTime object to match another (assignment operator).
 *
 * @param <f> The object that is copied.
 *
 * @return this object, with the parameters set as needed.
 */
//------------------------------------------------------------------------------
SystemTime& SystemTime::operator=(const SystemTime &f)
{
   if (this == &f)
      return *this;

   BuiltinGmatFunction::operator=(f);
   if (outputWrapper)
      delete outputWrapper;
   outputWrapper = NULL;

   return *this;
}


//------------------------------------------------------------------------------
// virtual WrapperTypeArray GetOutputTypes(IntegerArray &rowCounts,
//                                         IntegeArrayr &colCounts) const
//------------------------------------------------------------------------------
WrapperTypeArray SystemTime::GetOutputTypes(IntegerArray &rowCounts,
                                          IntegerArray &colCounts) const
{
   rowCounts = outputRowCounts;
   colCounts = outputColCounts;
   return outputWrapperTypes;
}

//------------------------------------------------------------------------------
// virtual void SetOutputTypes(WrapperTypeArray &outputTypes,
//                             IntegerArray &rowCounts, IntegerArray &colCounts)
//------------------------------------------------------------------------------
/*
 * Sets function output types. This method is called when parsing the function
 * file from the Interpreter.
 */
//------------------------------------------------------------------------------
void SystemTime::SetOutputTypes(WrapperTypeArray &outputTypes,
                              IntegerArray &rowCounts, IntegerArray &colCounts)
{
   #ifdef DEBUG_FUNCTION_IN_OUT
   MessageInterface::ShowMessage
      ("SystemTime::SetOutputTypes() setting %d outputTypes\n", outputTypes.size());
   #endif

   // Set output wrapper type for SystemTime
   outputWrapperTypes = outputTypes;
   outputRowCounts = rowCounts;
   outputColCounts = colCounts;
}


void SystemTime::SetOutputName(const std::string &outputName)
{
   outputWrapperTypes.clear();
   if (outputName.find("ModJulian") == std::string::npos)
   {
      outputWrapperTypes.push_back(Gmat::STRING_OBJECT_WT);
   }
   else
   {
      outputWrapperTypes.push_back(Gmat::NUMBER_WT);
   }
}

bool SystemTime::SetStringParameter(const Integer id,
      const std::string &value)
{
   if (id == FUNCTION_OUTPUT)
   {
      if (value.find("ModJulian") == std::string::npos)
      {
         outputWrapperTypes.push_back(Gmat::STRING_OBJECT_WT);
      }
      else
      {
         outputWrapperTypes.push_back(Gmat::NUMBER_WT);
      }
   }

   return BuiltinGmatFunction::SetStringParameter(id, value);
}

bool SystemTime::SetStringParameter(const std::string &label,
                                        const std::string &value)
{
   return SetStringParameter(GetParameterID(label), value);
}

//------------------------------------------------------------------------------
// bool Initialize(ObjectInitializer *objInit, bool reinitialize)
//------------------------------------------------------------------------------
bool SystemTime::Initialize(ObjectInitializer *objInit, bool reinitialize)
{
   #ifdef DEBUG_FUNCTION_INIT
   MessageInterface::ShowMessage
      ("SystemTime::Initialize() <%p>'%s' entered\n", this, GetName().c_str());
   #endif

   BuiltinGmatFunction::Initialize(objInit);

   #ifdef DEBUG_FUNCTION_INIT
   MessageInterface::ShowMessage
      ("SystemTime::Initialize() <%p>'%s' returning true\n", this, GetName().c_str());
   #endif
   return true;
}


//------------------------------------------------------------------------------
// bool Execute(ObjectInitializer *objInit, bool reinitialize)
//------------------------------------------------------------------------------
bool SystemTime::Execute(ObjectInitializer *objInit, bool reinitialize)
{
   //=================================================================
   // Do some validation here
   //=================================================================
   // Check for input info, there should be 1 inputs
   if (inputArgMap.size() != 1)
   {
      #ifdef DEBUG_FUNCTION_EXEC
      MessageInterface::ShowMessage
         ("GetSystemTime::Execute() returning false, size of inputArgMap:(%d) "
          "is not 1\n", inputArgMap.size());
      #endif
      return false;
   }

   // Check for output info, there shoud be 1 output
   // It is an internal coding error if not 1
   if ((outputArgMap.size() != outputWrapperTypes.size()) &&
       outputWrapperTypes.size() != 1)
   {
      if (outputArgMap.size() != 1)
      {
         #ifdef DEBUG_FUNCTION_EXEC
         MessageInterface::ShowMessage
            ("GetSystemTime::Execute() returning false, size of outputArgMap: %d or "
             "outputWrapperTypes: %d are not 4\n",outputArgMap.size(),  outputWrapperTypes.size());
         #endif
      }
      return false;
   }

   // Check for output row and col counts
   if (outputRowCounts.empty() || outputColCounts.empty())
   {
      #ifdef DEBUG_FUNCTION_EXEC
      MessageInterface::ShowMessage
         ("GetSystemTime::Execute() returning false, size of outputRowCounts: &d or "
          "outputColCounts: %d are zero\n",outputRowCounts.size(),  outputColCounts.size());
      #endif
      return false;
   }

   // Check if input names are in the objectStore
   std::string msg;
   GmatBase *obj = NULL;
   GmatBase *input1_formatString = NULL;

   for (unsigned int i = 0; i < inputNames.size(); i++)
   {
      std::string objName = inputNames[i];
      ObjectMap::iterator objIter = objectStore->find(objName);
      if (objIter != objectStore->end())
      {
         obj = objIter->second;
         #ifdef DEBUG_FUNCTION_EXEC
         MessageInterface::ShowMessage
            ("   input[%d] = <%p><%s>'%s'\n", i, obj, obj ? obj->GetTypeName().c_str() : "NULL",
             obj ? obj->GetName().c_str() : "NULL");
         #endif
         if (obj == NULL)
         {
            msg = msg + "Cannot find the object '" + objName + "' in the objectStore\n";
         }
         else
         {
            if (obj->IsOfType(Gmat::STRING))
            {
               input1_formatString = obj;
            }
            else
            {
               msg = "Input to SystemTime() function requires String value";
            }
         }
      }
   }


   if (msg != "")
   {
      #ifdef DEBUG_FUNCTION_EXEC
      MessageInterface::ShowMessage
         ("SystemTime::Execute() returning false, one or more inputs "
          "not found in the objectStore or wrong type to operate on\n");
      #endif
      throw FunctionException(msg + " in \"" + callDescription + "\"");
   }

   //Check that the format string has been set
   if (input1_formatString == NULL) // if inputNames.size() == 0
   {
         throw FunctionException
            ("SystemTime::Execute()  input1_formatString remains unset\n");
   }

   // Set objects to passed in input values
   #ifdef DEBUG_FUNCTION_EXEC
   MessageInterface::ShowMessage("   Evaluating input value\n");
   #endif
   try
   {
      formatString = inputArgMap[input1_formatString->GetName()]->EvaluateString();

      #ifdef DEBUG_FUNCTION_EXEC
      MessageInterface::ShowMessage
         ("   formatString = %f, checking valid input\n", formatString);
      #endif

      std::list<std::string> validFormats{"A1ModJulian", "TAIModJulian",
         "UTCModJulian", "TDBModJulian", "TTModJulian", "A1Gregorian",
         "TAIGregorian", "UTCGregorian", "TDBGregorian", "TTGregorian"};

      //Error if the format string is not in the list of valid formats.
      if (std::find(validFormats.begin(), validFormats.end(), formatString) == validFormats.end())
      {
         throw FunctionException
            ("Input to SystemTime() function is not a recognized format");
      }
   }
   catch (BaseException &be)
   {
      #ifdef DEBUG_FUNCTION_EXEC
      MessageInterface::ShowMessage
         ("==> Caught Exception: '%s'\n", be.GetFullMessage().c_str());
      #endif
      std::string msg = be.GetFullMessage();
      be.SetDetails("");
      be.SetMessage(msg + " in \"" + callDescription + "\"");
      throw;
   }

   Integer year, month, day, hour, min, sec;
   Real msec;

   #ifdef _WIN32
      SYSTEMTIME theTime;
      GetSystemTime(&theTime);

      year = theTime.wYear;
      month = theTime.wMonth;
      day = theTime.wDay;
      hour = theTime.wHour;
      min = theTime.wMinute;
      msec = theTime.wSecond + theTime.wMilliseconds / 1000.0;
   #else
      //Retrieve the system time. This result is UTCGregorian, but not in the accepted
      //GMAT format.
      std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
      std::time_t now_t = std::chrono::system_clock::to_time_t(now);

      long int millis =
            std::chrono::duration_cast<std::chrono::milliseconds>
            (now.time_since_epoch()).count();

      #ifdef DEBUG_TIME_SYSTEM
         // What is the epoch time?
         std::chrono::time_point<std::chrono::system_clock> p1;
         std::time_t epoch_time = std::chrono::system_clock::to_time_t(p1);
         std::string epstr = std::ctime(&epoch_time);
      #endif

      msec = millis / 1000.0 - (Integer)(millis / 1000);

      //Convert the retrieved system time to a proper formatted UTCModJulian time.
      struct tm *now_struct = gmtime(&now_t);

      year = now_struct->tm_year + 1900;
      month = now_struct->tm_mon + 1;
      day = now_struct->tm_mday;
      hour = now_struct->tm_hour;
      min = now_struct->tm_min;
      msec = now_struct->tm_sec + msec;
   #endif

   #ifdef DEBUG_SYSTEM_TIME
      MessageInterface::ShowMessage("SystemTime is %02d:%02d:%02d.%03d\n",
         hour, min, sec, msec);
   #endif

   Real MJD = -1;
   MJD = ModifiedJulianDate
         (year, month, day, hour, min, msec, GmatTimeConstants::JULIAN_DATE_OF_010541);

   #ifdef DEBUG_TIME_SYSTEM
      MessageInterface::ShowMessage
        ("System Time: %s\nYear: %d\nMon:%d\nDay:%d\nHour:%d\nMin:%d\nSec:%d\nMJD: %.12lf\n",
              now_s.c_str(),
              now_struct->tm_year, now_struct->tm_mon, now_struct->tm_mday,
              now_struct->tm_hour, now_struct->tm_min, now_struct->tm_sec,
              MJD);
   #endif


   // Output
   std::map<std::string, ElementWrapper *>::iterator ewi = outputArgMap.begin();

   Real toMjd;
   std::string epochStr;

   TimeSystemConverter::Instance()->Convert("UTCModJulian", MJD, "", formatString, toMjd, epochStr, 1);
   CreateOutputEpochWrapper(toMjd, epochStr, ewi->first);

   #ifdef DEBUG_FUNCTION_EXEC
   MessageInterface::ShowMessage("   Setting outWrapper1 to outputWrapperMap\n");
   #endif
   ewi->second = outputWrapper;

   return true;
}



ElementWrapper* SystemTime::CreateOutputEpochWrapper(Real MjdEpoch, std::string epochStr,
                                                         const std::string &outName)
{
   #ifdef DEBUG_WRAPPERS
   MessageInterface::ShowMessage
      ("GetSystemTime::CreateOutputEpochWrapper() entered, a1MjdEpoch=%.12f, outName='%s'\n",
       a1MjdEpoch, outName.c_str());
   #endif

   if (outName.find("ModJulian") == std::string::npos)
   {
//      if (outputWrapper == NULL)
      {
         // Create StringObjectWrapper
         outputWrapper = new StringWrapper();
         outputWrapper->SetDescription(outName);
      }
      ((StringWrapper*)outputWrapper)->SetString(epochStr);
   }
   else
   {
//      if (outputWrapper == NULL)
      {
         outputWrapper = new NumberWrapper();
         outputWrapper->SetDescription(outName);

      }
      ((NumberWrapper*)outputWrapper)->SetReal(MjdEpoch);
   }

   #ifdef DEBUG_WRAPPERS
   MessageInterface::ShowMessage
      ("GetEphemStates::CreateOutputEpochWrapper() returning wrapper <%p>\n",
       outWrapper);
   #endif

   return outputWrapper;
}


//------------------------------------------------------------------------------
// void SystemTime::Finalize(bool cleanUp)
//------------------------------------------------------------------------------
void SystemTime::Finalize(bool cleanUp)
{
   #ifdef DEBUG_FUNCTION_FINALIZE
   MessageInterface::ShowMessage
      ("SystemTime::Finalize() <%p>'%s' entered, nothing to do here?\n",
       this, GetName().c_str());
   #endif
}


//------------------------------------------------------------------------------
//  GmatBase* Clone() const
//------------------------------------------------------------------------------
/**
 * Clone of the SystemTime.
 *
 * @return clone of the SystemTime.
 *
 */
//------------------------------------------------------------------------------
GmatBase* SystemTime::Clone() const
{
   return (new SystemTime(*this));
}


//---------------------------------------------------------------------------
// void Copy(const GmatBase* orig)
//---------------------------------------------------------------------------
/**
 * Sets this object to match another one.
 *
 * @param orig The original that is being copied.
 */
//---------------------------------------------------------------------------
void SystemTime::Copy(const GmatBase* orig)
{
   operator=(*((SystemTime *)(orig)));
}



