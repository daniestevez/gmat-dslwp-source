//$Id: BatchEstimator.cpp 1398 2011-04-21 20:39:37Z  $
//------------------------------------------------------------------------------
//                         BatchEstimator
//------------------------------------------------------------------------------
// GMAT: General Mission Analysis Tool
//
// Copyright (c) 2002 - 2018 United States Government as represented by the
// Administrator of The National Aeronautics and Space Administration.
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
// Developed jointly by NASA/GSFC and Thinking Systems, Inc. under contract
// number NNG06CA54C
//
// Author: Darrel J. Conway, Thinking Systems, Inc.
// Created: 2009/08/04
//
/**
 * Batch least squares estimator using direct inversion
 */
//------------------------------------------------------------------------------

#include "BatchEstimator.hpp"
#include "MessageInterface.hpp"
#include "EstimatorException.hpp"
#include <sstream>
#include "StringUtil.hpp"
#include "DataWriter.hpp"
#include "SchurFactorization.hpp"
#include "CholeskyFactorization.hpp"
#include "UtilityException.hpp"

//#define DEBUG_ACCUMULATION
//#define DEBUG_ACCUMULATION_RESULTS
//#define WALK_STATE_MACHINE
//#define DEBUG_VERBOSE
//#define DEBUG_WEIGHTS
//#define DEBUG_INNER_LOOP
//#define DEBUG_O_MINUS_C
//#define DEBUG_SCHUR
//#define DEBUG_INVERSION
//#define DEBUG_STM

//------------------------------------------------------------------------------
// static data
//------------------------------------------------------------------------------

const std::string
BatchEstimator::PARAMETER_TEXT[] =
{
   "OLSEInitialRMSSigma",
   "OLSEMultiplicativeConstant",
   "OLSEAdditiveConstant",
   "OLSEUseRMSP",
   "UseInnerLoopEditing",
   "ILSEMultiplicativeConstant",
   "ILSEMaximumIterations",
};

const Gmat::ParameterType
BatchEstimator::PARAMETER_TYPE[] =
{
   Gmat::REAL_TYPE,
   Gmat::REAL_TYPE,
   Gmat::REAL_TYPE,
   Gmat::BOOLEAN_TYPE,
   Gmat::BOOLEAN_TYPE,
   Gmat::REAL_TYPE,
   Gmat::INTEGER_TYPE,
};


//------------------------------------------------------------------------------
// BatchEstimator(const std::string &name)
//------------------------------------------------------------------------------
/**
 * Default constructor
 *
 * @param name Name of the instance being constructed
 */
//------------------------------------------------------------------------------
BatchEstimator::BatchEstimator(const std::string &name):
   BatchEstimatorBase       ("BatchEstimator", name),
   maxResidualMult          (3000.0),
   constMult                (3.0),
   additiveConst            (0.0),
   chooseRMSP               (true), // true for WRMSP and false for WRMS
   useInnerLoop             (false),
   constMultIL              (3.0),
   maxIterationsIL          (15),
   iterationsTakenIL        (0),
   estimationStatusIL       (IL_UNKNOWN)
{
   objectTypeNames.push_back("BatchEstimator");
   parameterCount = BatchEstimatorParamCount;
}


//------------------------------------------------------------------------------
// ~BatchEstimator()
//------------------------------------------------------------------------------
/**
 * Class destructor
 */
//------------------------------------------------------------------------------
BatchEstimator::~BatchEstimator()
{
}


//------------------------------------------------------------------------------
// BatchEstimator(const BatchEstimator& est)
//------------------------------------------------------------------------------
/**
 * Copy constructor
 *
 * @param est The object that is being copied
 */
//------------------------------------------------------------------------------
BatchEstimator::BatchEstimator(const BatchEstimator& est) :
   BatchEstimatorBase       (est),
   maxResidualMult          (est.maxResidualMult),
   constMult                (est.constMult),
   additiveConst            (est.additiveConst),
   chooseRMSP               (est.chooseRMSP),
   useInnerLoop             (est.useInnerLoop),
   constMultIL              (est.constMultIL),
   maxIterationsIL          (est.maxIterationsIL),
   iterationsTakenIL        (est.iterationsTakenIL),
   estimationStatusIL       (est.estimationStatusIL)
{

}


//------------------------------------------------------------------------------
// BatchEstimator& operator=(const BatchEstimator& est)
//------------------------------------------------------------------------------
/**
 * Assignment operator
 *
 * @param est The object providing configuration data for this object
 *
 * @return This object, configured to match est
 */
//------------------------------------------------------------------------------
BatchEstimator& BatchEstimator::operator=(const BatchEstimator& est)
{
   if (this != &est)
   {
      BatchEstimatorBase::operator=(est);

      maxResidualMult    = est.maxResidualMult;
      constMult          = est.constMult;
      additiveConst      = est.additiveConst;
      chooseRMSP         = est.chooseRMSP;
      useInnerLoop       = est.useInnerLoop;
      constMultIL        = est.constMultIL;
      maxIterationsIL    = est.maxIterationsIL;
      iterationsTakenIL  = est.iterationsTakenIL;
      estimationStatusIL = est.estimationStatusIL;
   }

   return *this;
}


//------------------------------------------------------------------------------
// GmatBase* Clone() const
//------------------------------------------------------------------------------
/**
 * Object cloner
 *
 * @return Pointer to a new BatchEstimator configured to match this one.
 */
//------------------------------------------------------------------------------
GmatBase* BatchEstimator::Clone() const
{
   return new BatchEstimator(*this);
}


//---------------------------------------------------------------------------
//  void Copy(const GmatBase* orig)
//---------------------------------------------------------------------------
/**
 * Sets this object to match another one.
 *
 * @param orig The original that is being copied.
 */
//---------------------------------------------------------------------------
void  BatchEstimator::Copy(const GmatBase* orig)
{
   operator=(*((BatchEstimator*)(orig)));
}

//------------------------------------------------------------------------------
//  std::string GetParameterText(const Integer id) const
//------------------------------------------------------------------------------
/**
 * This method returns the parameter text, given the input parameter ID.
 *
 * @param id Id for the requested parameter text.
 *
 * @return parameter text for the requested parameter.
 *
 */
//------------------------------------------------------------------------------
std::string BatchEstimator::GetParameterText(const Integer id) const
{
   if (id >= BatchEstimatorBaseParamCount && id < BatchEstimatorParamCount)
      return PARAMETER_TEXT[id - BatchEstimatorBaseParamCount];
   return BatchEstimatorBase::GetParameterText(id);
}

//---------------------------------------------------------------------------
//  std::string GetParameterUnit(const Integer id) const
//---------------------------------------------------------------------------
/**
 * Retrieve the unit for the parameter.
 *
 * @param id The integer ID for the parameter.
 *
 * @return unit for the requested parameter.
 */
//------------------------------------------------------------------------------
std::string BatchEstimator::GetParameterUnit(const Integer id) const
{
   return BatchEstimatorBase::GetParameterUnit(id); // TBD
}

//------------------------------------------------------------------------------
//  Integer GetParameterID(const std::string &str) const
//------------------------------------------------------------------------------
/**
 * This method returns the parameter ID, given the input parameter string.
 *
 * @param str string for the requested parameter.
 *
 * @return ID for the requested parameter.
 *
 */
//------------------------------------------------------------------------------
Integer BatchEstimator::GetParameterID(const std::string &str) const
{
   for (Integer i = BatchEstimatorBaseParamCount; i < BatchEstimatorParamCount; i++)
   {
      if (str == PARAMETER_TEXT[i - BatchEstimatorBaseParamCount])
         return i;
   }

   return BatchEstimatorBase::GetParameterID(str);
}

//------------------------------------------------------------------------------
//  Gmat::ParameterType GetParameterType(const Integer id) const
//------------------------------------------------------------------------------
/**
 * This method returns the parameter type, given the input parameter ID.
 *
 * @param id ID for the requested parameter.
 *
 * @return parameter type of the requested parameter.
 *
 */
//------------------------------------------------------------------------------
Gmat::ParameterType BatchEstimator::GetParameterType(const Integer id) const
{
   if (id >= BatchEstimatorBaseParamCount && id < BatchEstimatorParamCount)
      return PARAMETER_TYPE[id - BatchEstimatorBaseParamCount];

   return BatchEstimatorBase::GetParameterType(id);
}

//------------------------------------------------------------------------------
//  std::string GetParameterTypeString(const Integer id) const
//------------------------------------------------------------------------------
/**
 * This method returns the parameter type string, given the input parameter ID.
 *
 * @param id ID for the requested parameter.
 *
 * @return parameter type string of the requested parameter.
 *
 */
//------------------------------------------------------------------------------
std::string BatchEstimator::GetParameterTypeString(const Integer id) const
{
   return BatchEstimatorBase::PARAM_TYPE_STRING[GetParameterType(id)];
}


//------------------------------------------------------------------------------
//  Real GetRealParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
 * This method returns the Real parameter value, given the input parameter ID.
 *
 * @param id ID for the requested parameter value.
 *
 * @return  Real value of the requested parameter.
 *
 */
//------------------------------------------------------------------------------
Real BatchEstimator::GetRealParameter(const Integer id) const
{
   if (id == MAX_RESIDUAL_MULTIPLIER)
      return maxResidualMult;
   if (id == CONSTANT_MULTIPLIER)
      return constMult;
   if (id == ADDITIVE_CONSTANT)
      return additiveConst;
   if (id == CONSTANT_MULTIPLIER_ILSE)
      return constMultIL;

   return BatchEstimatorBase::GetRealParameter(id);
}

//------------------------------------------------------------------------------
//  Real SetRealParameter(const Integer id, const Real value)
//------------------------------------------------------------------------------
/**
 * This method sets the Real parameter value, given the input parameter ID.
 *
 * @param id ID for the parameter whose value to change.
 * @param value Value for the parameter.
 *
 * @return  Real value of the requested parameter.
 */
//------------------------------------------------------------------------------
Real BatchEstimator::SetRealParameter(const Integer id, const Real value)
{
   if (id == MAX_RESIDUAL_MULTIPLIER)
   {
       if (value > 0.0)
           maxResidualMult = value;
      else
         throw EstimatorException("Error: "+ GetName() +"."+ GetParameterText(id) +" parameter is not a positive number\n");

       return maxResidualMult;
   }

   if (id == CONSTANT_MULTIPLIER)
   {
      if (value > 0.0)
           constMult = value;
      else
         throw EstimatorException("Error: "+ GetName() +"."+ GetParameterText(id) +" parameter is not a positive number\n");

      return constMult;
   }

   if (id == ADDITIVE_CONSTANT)
   {
      additiveConst = value;
      return additiveConst;
   }

   if (id == CONSTANT_MULTIPLIER_ILSE)
   {
      if (value > 0.0)
           constMultIL = value;
      else
         throw EstimatorException("Error: "+ GetName() +"."+ GetParameterText(id) +" parameter is not a positive number\n");

      return constMultIL;
   }


   return BatchEstimatorBase::SetRealParameter(id, value);
}


//------------------------------------------------------------------------------
//  Integer GetRealParameter(const std::string &label) const
//------------------------------------------------------------------------------
/**
* This method gets value of an real parameter specified by parameter name.
*
* @param label    name of parameter.
*
* @return         value of a real parameter.
*/
//------------------------------------------------------------------------------
Real BatchEstimator::GetRealParameter(const std::string &label) const
{
   return GetRealParameter(GetParameterID(label));
}


//------------------------------------------------------------------------------
//  Integer SetRealParameter(const std::string &label, const Integer value)
//------------------------------------------------------------------------------
/**
* This method sets value to a real parameter specified by the input parameter name.
*
* @param label    name for the requested parameter.
* @param value    real value used to set to the request parameter.
*
* @return value set to the requested parameter.
*/
//------------------------------------------------------------------------------
Real BatchEstimator::SetRealParameter(const std::string &label, const Real value)
{
   return SetRealParameter(GetParameterID(label), value);
}


//------------------------------------------------------------------------------
//  Integer GetIntegerParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
 * This method returns value of an integer parameter given the input parameter ID.
 *
 * @param id ID for the requested parameter.
 *
 * @return value of the requested parameter.
 */
//------------------------------------------------------------------------------
Integer BatchEstimator::GetIntegerParameter(const Integer id) const
{
   if (id == MAX_ITERATIONS_ILSE)
      return maxIterationsIL;

   return BatchEstimatorBase::GetIntegerParameter(id);
}


//------------------------------------------------------------------------------
//  Integer SetIntegerParameter(const Integer id, const Integer value)
//------------------------------------------------------------------------------
/**
 * This method sets value to an integer parameter specified by the input parameter ID.
 *
 * @param id       ID for the requested parameter.
 * @param value    integer value used to set to the request parameter. 
 *
 * @return value set to the requested parameter.
 */
//------------------------------------------------------------------------------
Integer BatchEstimator::SetIntegerParameter(const Integer id, const Integer value)
{
   if (id == MAX_ITERATIONS_ILSE)
   {
      if (value > 0)
         maxIterationsIL = value;
      else
         throw SolverException(
            "The value entered for the maximum ILSE iterations on " + instanceName +
            " is not an allowed value. The allowed value is: [Integer > 0].");
      return maxIterationsIL;
   }

   return BatchEstimatorBase::SetIntegerParameter(id, value);
}


//------------------------------------------------------------------------------
//  Integer GetIntegerParameter(const std::string &label) const
//------------------------------------------------------------------------------
/**
* This method gets value of an integer parameter specified by parameter name.
*
* @param label    name of parameter.
*
* @return         value of an integer parameter.
*/
//------------------------------------------------------------------------------
Integer BatchEstimator::GetIntegerParameter(const std::string &label) const
{
   return GetIntegerParameter(GetParameterID(label));
}


//------------------------------------------------------------------------------
//  Integer SetIntegerParameter(const std::string &label, const Integer value)
//------------------------------------------------------------------------------
/**
* This method sets value to an integer parameter specified by the input parameter name.
*
* @param label    name for the requested parameter.
* @param value    integer value used to set to the request parameter.
*
* @return value set to the requested parameter.
*/
//------------------------------------------------------------------------------
Integer BatchEstimator::SetIntegerParameter(const std::string &label, const Integer value)
{
   return SetIntegerParameter(GetParameterID(label), value);
}


//------------------------------------------------------------------------------
// bool GetBooleanParameter(const Integer id) const
//------------------------------------------------------------------------------
/**
* Gets value of a boolean parameter
*
* @param id       The id of parameter
*
* @return         value of boolean parameter
*/
//------------------------------------------------------------------------------
bool BatchEstimator::GetBooleanParameter(const Integer id) const
{
   if (id == USE_RMSP)
      return chooseRMSP;
   if (id == ENABLE_ILSE)
      return useInnerLoop;

   return BatchEstimatorBase::GetBooleanParameter(id);
}


//------------------------------------------------------------------------------
// bool SetBooleanParameter(const Integer id, const bool value)
//------------------------------------------------------------------------------
/**
* Sets value to a boolean parameter
*
* @param id       The id of parameter
* @param value    The value used to set to a boolean parameter
*
* @return         true if the setting is successed, false otherwise
*/
//------------------------------------------------------------------------------
bool BatchEstimator::SetBooleanParameter(const Integer id, const bool value)
{
   if (id == USE_RMSP)
   {
      chooseRMSP = value;
      return true;
   }

   if (id == ENABLE_ILSE)
   {
      useInnerLoop = value;
      return true;
   }

   return BatchEstimatorBase::SetBooleanParameter(id, value);
}


//------------------------------------------------------------------------------
// Methods used in the finite state machine
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  void CompleteInitialization()
//------------------------------------------------------------------------------
/**
 * This method completes initialization for the Simulator object, initializing
 * its MeasurementManager, retrieving the epoch and setting the state.
 */
//------------------------------------------------------------------------------
void BatchEstimator::CompleteInitialization()
{
   BatchEstimatorBase::CompleteInitialization();

   writeMeasurmentsAtEnd = useInnerLoop;

   iterationsTakenIL  = 0;
   estimationStatusIL = IL_UNKNOWN;
}


//------------------------------------------------------------------------------
// void WriteReportFileHeaderPart6()
//------------------------------------------------------------------------------
/**
* This function is used to write estimation options to the report file.
*/
//------------------------------------------------------------------------------
void BatchEstimator::WriteReportFileHeaderPart6()
{
   // 1. Write estimation options header
   textFile << "*********************************************************************  ESTIMATION OPTIONS  *********************************************************************\n";
   textFile << "\n";

   std::stringstream ss;
   StringArray sa1, sa2, sa3;


   // 2. Write data to the first and second columns
   ss.str("");
   if ((maxResidualMult == 0.0) || ((GmatMathUtil::Abs(maxResidualMult) < 1.0e6) && (GmatMathUtil::Abs(maxResidualMult) > 1.0e-2)))
      ss << maxResidualMult;
   else
      ss << GmatStringUtil::RealToString(maxResidualMult, false, true);
   sa1.push_back("OLSE Initial RMS Sigma"); sa2.push_back(ss.str());

   ss.str("");
   if ((constMult == 0.0) || ((GmatMathUtil::Abs(constMult) < 1.0e6) && (GmatMathUtil::Abs(constMult) > 1.0e-2)))
      ss << constMult;
   else
      ss << GmatStringUtil::RealToString(constMult, false, true);
   sa1.push_back("OLSE Multiplicative Constant"); sa2.push_back(ss.str());

   ss.str("");
   if ((additiveConst == 0.0) || ((GmatMathUtil::Abs(additiveConst) < 1.0e6) && (GmatMathUtil::Abs(additiveConst) > 1.0e-2)))
      ss << additiveConst;
   else
      ss << GmatStringUtil::RealToString(additiveConst, false, true);
   sa1.push_back("OLSE Additive Constant"); sa2.push_back(ss.str());

   ss.str(""); ss << GetRealParameter("AbsoluteTol"); sa1.push_back("Absolute Tolerance for Convergence"); sa2.push_back(ss.str());
   ss.str(""); ss << GetRealParameter("RelativeTol"); sa1.push_back("Relative Tolerance for Convergence"); sa2.push_back(ss.str());
   ss.str(""); ss << GetIntegerParameter("MaximumIterations"); sa1.push_back("Maximum Iterations"); sa2.push_back(ss.str());
   ss.str(""); ss << GetIntegerParameter("MaxConsecutiveDivergences"); sa1.push_back("Maximum Consecutive Divergences"); sa2.push_back(ss.str());
   ss.str(""); ss << (useInnerLoop ? "Yes" : "No"); sa1.push_back("Inner Loop Editing"); sa2.push_back(ss.str());

   if (useInnerLoop)
   {
      ss.str("");
      if ((constMultIL == 0.0) || ((GmatMathUtil::Abs(constMultIL) < 1.0e6) && (GmatMathUtil::Abs(constMultIL) > 1.0e-2)))
         ss << constMultIL;
      else
         ss << GmatStringUtil::RealToString(constMultIL, false, true);
      sa1.push_back("ILSE Multiplicative Constant"); sa2.push_back(ss.str());
      ss.str(""); ss << GetIntegerParameter("ILSEMaximumIterations"); sa1.push_back("Inner Loop Maximum Iterations"); sa2.push_back(ss.str());
   }

   if (freezeEditing)
   {
      ss.str(""); ss << GetIntegerParameter("FreezeIteration"); sa1.push_back("Freeze Editing on Iteration"); sa2.push_back(ss.str());
   }


   // 3. Write the 3rd column
   GmatTime taiMjdEpoch, utcMjdEpoch;
   std::string utcEpoch;

   sa3.push_back("Estimation Epoch :");
   sa3.push_back("");

   ss.precision(15);
   if (estEpochFormat != "FromParticipants")
   {
      ss.str(""); ss << "   " << estEpoch << " " << estEpochFormat; sa3.push_back(ss.str());
      sa3.push_back("");
      sa3.push_back("");
   }
   else
   {
      bool handleLeapSecond;
      taiMjdEpoch = theTimeConverter->Convert(estimationEpochGT, TimeSystemConverter::A1MJD, TimeSystemConverter::TAIMJD);
      utcMjdEpoch = theTimeConverter->Convert(estimationEpochGT, TimeSystemConverter::A1MJD, TimeSystemConverter::UTCMJD,
            GmatTimeConstants::JD_JAN_5_1941, &handleLeapSecond);
      utcEpoch = theTimeConverter->ConvertMjdToGregorian(utcMjdEpoch.GetMjd(), handleLeapSecond);

      ss.str(""); ss << utcEpoch << " UTCG"; sa3.push_back(ss.str());
      ss.str(""); ss << estimationEpochGT.ToString() << " A.1 Mod. Julian"; sa3.push_back(ss.str());
      ss.str(""); ss << taiMjdEpoch.ToString() << " TAI Mod. Julian"; sa3.push_back(ss.str());

   }
   sa3.push_back("");
   sa3.push_back("");
   sa3.push_back("");
   if (useInnerLoop)
   {
      sa3.push_back("");
      sa3.push_back("");
   }
   if (freezeEditing)
      sa3.push_back("");

   // 4. Write to text file
   Integer nameLen = 0;
   for (Integer i = 0; i < sa1.size(); ++i)
      nameLen = (Integer)GmatMathUtil::Max(nameLen, sa1[i].size());

   for (Integer i = 0; i < sa1.size(); ++i)
   {
      textFile << GmatStringUtil::GetAlignmentString("", 33)
         << GmatStringUtil::GetAlignmentString(sa1[i], nameLen + 2, GmatStringUtil::LEFT)
         << GmatStringUtil::GetAlignmentString(sa2[i], 95-(35 + nameLen), GmatStringUtil::LEFT)
         << sa3[i] << "\n";
   }
   textFile << "\n";

   textFile.flush();
}


//----------------------------------------------------------------------
// void WriteReportFileSummaryPart1(Solver::SolverState sState)
//----------------------------------------------------------------------
/**
* Write statistics summary for estimation
*/
//----------------------------------------------------------------------
void BatchEstimator::WriteReportFileSummaryPart1(Solver::SolverState sState)
{
   if (sState == ESTIMATING)
   {
      // 1. Write summary part 1 header:
      textFile0 << "\n";
      textFile0 << "***********************************************************  ITERATION " << GmatStringUtil::ToString(iterationsTaken, 3) << ":  MEASUREMENT STATISTICS  ***********************************************************\n";
      textFile0 << "\n";
      textFile0.flush();
   }

   if (sState == FINISHED)
   {
      std::stringstream ss;

      /// 1.1. Write estimation status
      ss << "***  Estimation ";
      switch (estimationStatus)
      {
      case ABSOLUTETOL_CONVERGED:
      case RELATIVETOL_CONVERGED:
      case ABS_AND_REL_TOL_CONVERGED:
         ss << "converged!";
         break;
      case MAX_CONSECUTIVE_DIVERGED:
      case MAX_ITERATIONS_DIVERGED:
      case CONVERGING:
      case DIVERGING:
         ss << "did not converge!";
         break;
      case UNKNOWN:
         break;
      };
      ss << "  ***";
      textFile0 << GmatStringUtil::GetAlignmentString(ss.str(), 160, GmatStringUtil::CENTER) << "\n";

      // 1.2. Write reason for convergence
      textFile0 << GmatStringUtil::GetAlignmentString(GmatStringUtil::Trim(convergenceReason.substr(0,convergenceReason.size()-1), GmatStringUtil::BOTH), 160, GmatStringUtil::CENTER) << "\n";

      // 1.3. Write number of iterations was run for estimation
      ss.str("");
      ss << "Estimation completed in " << iterationsTaken << " iterations";
      textFile0 << GmatStringUtil::GetAlignmentString(ss.str(), 160, GmatStringUtil::CENTER) << "\n";
      textFile0 << "\n";

      std::vector<ObservationData> *obsList = GetMeasurementManager()->GetObservationDataList();
      std::string firstObsEpoch, lastObsEpoch;
      Integer lastIndex = obsList->size() - 1;
      Real temp;
      theTimeConverter->Convert("A1ModJulian", (*obsList)[0].epoch, "", "UTCGregorian", temp, firstObsEpoch, 1);         // @todo: It needs to changes (*obsList)[0].epoch to (*obsList)[0].epochGT
      theTimeConverter->Convert("A1ModJulian", (*obsList)[lastIndex].epoch, "", "UTCGregorian", temp, lastObsEpoch, 1);  // @todo: It needs to changes (*obsList)[lastIndex].epoch to (*obsList)[lastIndex].epochGT

      textFile0 << "                                                      Time of First Observation : " << firstObsEpoch << "\n";
      textFile0 << "                                                      Time of Last Observation  : " << lastObsEpoch << "\n";
      textFile0 << "\n";
      textFile0.flush();
   }

   if (sState == ESTIMATING)
   {
      std::stringstream ss;
      ss << "Total Number Of Observations              : " << GetMeasurementManager()->GetObservationDataList()->size();
      textFile1 << GmatStringUtil::GetAlignmentString("", 33) << GmatStringUtil::GetAlignmentString(ss.str(), 60) << "Current WRMS Residuals   : " << GmatStringUtil::RealToString(newResidualRMS, false, false, true, 6) << "\n";
      ss.str(""); ss << "Observations Used For Estimation          : " << numRemovedRecords["N"];
      textFile1 << GmatStringUtil::GetAlignmentString("", 33) << GmatStringUtil::GetAlignmentString(ss.str(), 60) << "Predicted WRMS Residuals : " << GmatStringUtil::RealToString(predictedRMS, false, false, true, 6) << "\n";

      ss.str(""); ss << "No Computed Value Configuration Available : " << numRemovedRecords["U"];
      textFile1 << GmatStringUtil::GetAlignmentString("", 33) << GmatStringUtil::GetAlignmentString(ss.str(), 60);
      if (iterationsTaken != 0)
         textFile1 << "Previous WRMS Residuals  : " << GmatStringUtil::RealToString(oldResidualRMS, false, false, true, 6) << "\n";
      else
      {
         if ((maxResidualMult == 0.0) || ((GmatMathUtil::Abs(maxResidualMult) < 1.0e6) && (GmatMathUtil::Abs(maxResidualMult) > 1.0e-2)))
            textFile1 << "OLSE Initial RMS Sigma   : " << GmatStringUtil::RealToString(maxResidualMult, false, false, true, 6) << "\n";
         else
            textFile1 << "OLSE Initial RMS Sigma   : " << GmatStringUtil::RealToString(maxResidualMult, false, true, true, 6) << "\n";
      }

      
      ss.str(""); ss << "Out of Ramp Table Range                   : " << numRemovedRecords["R"] << " ";
      textFile1 << GmatStringUtil::GetAlignmentString("", 33) << GmatStringUtil::GetAlignmentString(ss.str(), 60) << "Smallest WRMS Residuals  : " << GmatStringUtil::RealToString(bestResidualRMS, false, false, true, 6) << "\n";

      // 2. Write data records usage summary:
      ss.str("");  ss << "Signal Blocked                            : " << numRemovedRecords["B"];
      textFile1 << GmatStringUtil::GetAlignmentString("", 33) << GmatStringUtil::GetAlignmentString(ss.str(), 60);
      
      //ss.str(""); ss << "Sigma Editing                             : " << ((iterationsTaken == 0) ? numRemovedRecords["IRMS"] : numRemovedRecords["OLSE"]);
      ss.str(""); ss << "Sigma Editing                             : " << ((numRemovedRecords["IRMS"] != 0) ? numRemovedRecords["IRMS"] : numRemovedRecords["OLSE"]);
      textFile1_1 << GmatStringUtil::GetAlignmentString("", 33) << GmatStringUtil::GetAlignmentString(ss.str(), 60) << "\n";
      
      ss.str(""); ss << "Inner Loop Sigma Editing                  : " << numRemovedRecords["ILSE"];
      textFile1_1 << GmatStringUtil::GetAlignmentString("", 33) << GmatStringUtil::GetAlignmentString(ss.str(), 60);

      if (useInnerLoop)
         textFile1_1 << "Inner Loop Iterations    : " << iterationsTakenIL;
      textFile1_1 << "\n";
      
      ss.str(""); ss << "User Editing                              : " << numRemovedRecords["USER"];
      textFile1_1 << GmatStringUtil::GetAlignmentString("", 33) << GmatStringUtil::GetAlignmentString(ss.str(), 60);

      if (useInnerLoop)
      {
         ss.str("");
         switch (estimationStatusIL)
         {
         case IL_SAME_EDITS_CONVERGED:
            ss << "Converged";
            break;
         case IL_MAX_ITERATIONS_DIVERGED:
            ss << "Maximum Iterations";
            break;
         case IL_UNKNOWN:
         default:
            ss << "UNKNOWN";
            break;
         }
         textFile1_1 << "Inner Loop Termination   : " << ss.str();
      }
      textFile1_1 << "\n";
      textFile1_1 << "\n";
      textFile1_1.flush();

      textFile1.flush();
   }
   
   if ((sState == CHECKINGRUN) || (sState == FINISHED))
   {
      // 4. Convergence status summary:
      textFile1 << "DC Status                : ";
      switch (estimationStatus)
      {
      case ABSOLUTETOL_CONVERGED:
         textFile1 << "Absolute Tolerance Converged";
         break;
      case RELATIVETOL_CONVERGED:
         textFile1 << "Relative Tolerance Converged";
         break;
      case ABS_AND_REL_TOL_CONVERGED:
         textFile1 << "Absolute and Relative Tolerance Converged";
         break;
      case MAX_CONSECUTIVE_DIVERGED:
         textFile1 << "Maximum Consecutive Diverged";
         break;
      case MAX_ITERATIONS_DIVERGED:
         textFile1 << "Maximum Iterations Diverged";
         break;
      case CONVERGING:
         textFile1 << "Converging";
         break;
      case DIVERGING:
         textFile1 << "Diverging";
         break;
      case UNKNOWN:
         textFile1 << "Unknown";
         break;
      }
      textFile1 << "\n";

      textFile1.flush();
   }
}


//------------------------------------------------------------------------------
//  void Accumulate()
//------------------------------------------------------------------------------
/**
 * This method collects the data needed for estimation.
 */
//------------------------------------------------------------------------------
void BatchEstimator::Accumulate()
{
#ifdef WALK_STATE_MACHINE
   MessageInterface::ShowMessage("BatchEstimator state is ACCUMULATING\n");
#endif

#ifdef DEBUG_ACCUMULATION
   MessageInterface::ShowMessage("Entered BatchEstimator::Accumulate()\n");
#endif
   // Measurements are possible!
   const MeasurementData *calculatedMeas = NULL;
   std::vector<RealArray> stateDeriv;

   for (UnsignedInt i = 0; i < hTilde.size(); ++i)
      hTilde[i].clear();
   hTilde.clear();

   // Get state map, measurement models, and measurement data
   const std::vector<ListItem*> *stateMap = esm.GetStateMap();
   modelsToAccess = measManager.GetValidMeasurementList();            // Get valid measurement models
   const ObservationData *currentObs = measManager.GetObsData();      // Get current observation data
   
   // Perform second level data editing on observation data
   Integer recNum = measManager.GetCurrentRecordNumber();
   if (recNum == editedRecords.size())
   {
      Integer dataFilterIndex = -1;
      ObservationData* obData = FilteringData(measManager.GetObsDataObject(), recNum, dataFilterIndex);
      if (obData)
         editedRecords.push_back(NORMAL_FLAG);
      else
      {
         editedRecords.push_back(USER_FLAG);
         ++numRemovedRecords["USER"];
      }
   }
   else
   {
      editedRecords[recNum] = editedRecords[recNum] & USER_FLAG;      // keep USER flag and clear all other flags
      if (editedRecords[recNum] == USER_FLAG)
         ++numRemovedRecords["USER"];                                 // recount due to this variable was reset to 0
   }

   // Get ground station and measurement type from current observation data
   std::string gsName = currentObs->participantIDs[0];
   std::string typeName = currentObs->typeName;

   // Build vectors of each unique ground station and measurement type
   bool found = false;
   for (UnsignedInt indexKey = 0; indexKey < stationsList.size(); ++indexKey)
   {
      if (stationsList[indexKey] == gsName)
      {
         found = true;
         break;
      }
   }
   if (!found)
      stationsList.push_back(gsName);

   found = false;
   for (UnsignedInt indexKey = 0; indexKey < measTypesList.size(); ++indexKey)
   {
      if (measTypesList[indexKey] == typeName)
      {
         found = true;
         break;
      }
   }
   if (!found)
      measTypesList.push_back(typeName);

   // Prepare measurement statistics
   MeasurementInfoType measStat;
   measStat.epoch     = currentEpochGT;
   measStat.recNum    = recNum;
   measStat.modelSize = modelsToAccess.size();
   measStat.station   = gsName;
   measStat.type      = typeName;

   // Add default alues that aren't assigned on all logic branches
   measStat.isCalculated     = false;
   measStat.uniqueID         = 0;
   measStat.feasibilityValue = 0.0;

#ifdef DEBUG_ACCUMULATION
   MessageInterface::ShowMessage("StateMap size is %d\n", stateMap->size());
   MessageInterface::ShowMessage("Found %d models\n", modelsToAccess.size());

   MessageInterface::ShowMessage("Observation data O: epoch = %.12lf   type = <%s>", currentObs->epoch, currentObs->typeName.c_str());
   for(UnsignedInt i=0; i < currentObs->participantIDs.size(); ++i)
      MessageInterface::ShowMessage("   %s", currentObs->participantIDs[i].c_str());

   MessageInterface::ShowMessage("   O = [");
   for (UnsignedInt i = 0; i < currentObs->value.size(); ++i)
      MessageInterface::ShowMessage("%.12lf   ", currentObs->value[i]);
   MessageInterface::ShowMessage("]\n");

   if (currentObs->typeName == "DSN_SeqRange")
      MessageInterface::ShowMessage("   range modulo = %lf\n", currentObs->rangeModulo);
   MessageInterface::ShowMessage("\n");
#endif

   std::string times;
   Real temp;

   // Verify measurement epoch is inside of EOP time range
   if ((currentObs->epoch < eopTimeMin) || (currentObs->epoch > eopTimeMax))
   {
      if (warningCount == 0)
         MessageInterface::ShowMessage("Warning: measurement epoch %.12lf A1Mjd is outside EOP time range [%.12lf A1Mjd, %.12lf A1Mjd]\n", currentObs->epoch, eopTimeMin, eopTimeMax);
      ++warningCount;
   }

   if (modelsToAccess.size() == 0)
   {
      // Count number of records removed by measurement model unmatched 
      numRemovedRecords["U"]++;
      measManager.GetObsDataObject()->inUsed = false;
      measManager.GetObsDataObject()->removedReason = "U";
      editedRecords[recNum] = editedRecords[recNum] | U_FLAG;          // adding Unmatched flag
   }
   else
   {
      int count = measManager.CountFeasibleMeasurements(modelsToAccess[0]);
      calculatedMeas = measManager.GetMeasurement(modelsToAccess[0]);

      // verify media correction to be in acceptable range. It is [0m, 60m] for troposphere correction and [0m, 20m] for ionosphere correction
      ValidateMediaCorrection(calculatedMeas);

      std::string ss = measManager.GetObsDataObject()->removedReason;
      if (!((freezeEditing) && (iterationsTaken >= freezeIteration) && ((ss == "IRMS") || (ss == "OLSE") || (ss == "ILSE"))))
         ss = measManager.GetObsDataObject()->removedReason = calculatedMeas->unfeasibleReason;

      if (count == 0)
      {
         // count = 0 when calulation is out of ramp table
         if (ss == "R")
         {
            numRemovedRecords[ss]++;
            editedRecords[recNum] = editedRecords[recNum] | RAMP_FLAG;
         }
      }
      else // (count >= 1)
      {
         // Currently assuming uniqueness; modify if more than 1 possible here
         // Case: ((modelsToAccess.size() > 0) && (measManager.Calculate(modelsToAccess[0], true) >= 1))

         // It has to make correction for observation value before running data filter
         //if ((iterationsTaken == 0)&&((currentObs->typeName == "DSNTwoWayRange")||(currentObs->typeName == "DSN_SeqRange")))
         if ((iterationsTaken == 0) && (currentObs->typeName == "DSN_SeqRange"))
         {
            // value correction is only applied for DSN_SeqRange and it is only performed at the first time
            for (Integer index = 0; index < currentObs->value.size(); ++index)
               measManager.GetObsDataObject()->value[index] = ObservationDataCorrection(calculatedMeas->value[index], currentObs->value[index], currentObs->rangeModulo);
         }

         // Peform Outloop Sigma Editing. It generates IRMS and OLSE flags
         if (ss.substr(0, 1) != "B")
         {
            if ((!freezeEditing) || 
               ((freezeEditing) && (iterationsTaken <= (freezeIteration - 1))))
                DataFilter();
         }

         // It needs to get "IRMS" or "OLSE" flag
         ss = measManager.GetObsDataObject()->removedReason;

         #ifdef DEBUG_ACCUMULATION
         MessageInterface::ShowMessage("iterationsTaken = %d    inUsed = %s\n", iterationsTaken, (measManager.GetObsDataObject()->inUsed ? "true" : "false"));
         #endif

         if (ss.substr(0, 1) == "B")
         {
            numRemovedRecords["B"]++;
            editedRecords[recNum] = editedRecords[recNum] | BLOCKED_FLAG;           // Adding blocked flag
         }
         else if (ss == "IRMS")
         {
            numRemovedRecords["IRMS"]++;
            editedRecords[recNum] = editedRecords[recNum] | IRMS_FLAG;              // Adding IRMS flag
         }
         else if (ss == "OLSE")
         {
            numRemovedRecords["OLSE"]++;
            editedRecords[recNum] = editedRecords[recNum] | OLSE_FLAG;              // Adding OSLE flag
         }
         else if (!(editedRecords[recNum] == USER_FLAG))
            numRemovedRecords[ss]++;                    // Count number of records with Normal flag

         RealArray hTrow;
         hTrow.assign(stateSize, 0.0);
         UnsignedInt rowCount = calculatedMeas->value.size();

         for (UnsignedInt i = 0; i < rowCount; ++i)
            hTilde.push_back(hTrow);

         // Now walk the state vector and get elements of H-tilde for each piece
         for (UnsignedInt i = 0; i < stateMap->size(); ++i)
         {
            if ((*stateMap)[i]->subelement == 1)
            {
               #ifdef DEBUG_ACCUMULATION
                  MessageInterface::ShowMessage(
                     "   Calculating ddstate(%d) for %s, subelement %d of %d, "
                     "id = %d\n",
                     i, (*stateMap)[i]->elementName.c_str(),
                     (*stateMap)[i]->subelement, (*stateMap)[i]->length,
                     (*stateMap)[i]->elementID);
                  MessageInterface::ShowMessage("object = <%p '%s'>\n", (*stateMap)[i]->object, (*stateMap)[i]->object->GetName().c_str());
               #endif
               // Partial derivatives at measurement time tm
               stateDeriv = measManager.CalculateDerivatives(
                  (*stateMap)[i]->object, (*stateMap)[i]->elementID,
                  modelsToAccess[0]);
               
               // Fill in the corresponding elements of hTilde
               for (UnsignedInt j = 0; j < rowCount; ++j)
                  for (UnsignedInt k = 0; k < (*stateMap)[i]->length; ++k)
                     hTilde[j][i+k] = stateDeriv[j][k];                                          // hTilde is partial derivates at measurement time tm (not at aprioi time t0)

               #ifdef DEBUG_ACCUMULATION
                  MessageInterface::ShowMessage("      Result:\n");
                  for (UnsignedInt l = 0; l < stateDeriv.size(); ++l)
                  {
                     for (UnsignedInt m = 0; m < (*stateMap)[i]->length; ++m)
                        MessageInterface::ShowMessage("%.12le   ",
                           stateDeriv[l][m]);
                     MessageInterface::ShowMessage("\n");
                  }
                  MessageInterface::ShowMessage("\n");

                  MessageInterface::ShowMessage("      hTilde:\n");
                  for (UnsignedInt row = 0; row < hTilde.size(); ++row)
                  {
                     for (UnsignedInt col = 0; col < hTilde[row].size(); ++col)
                     {
                        MessageInterface::ShowMessage("%.12le   ", hTilde[row][col]);
                     }
                     MessageInterface::ShowMessage("\n");
                  }
                  MessageInterface::ShowMessage("\n");
               #endif
            }
         }


         // Apply the STM
         #ifdef DEBUG_ACCUMULATION
            MessageInterface::ShowMessage("Applying the STM\n");
         #endif
         RealArray hRow;

         // Temporary buffer for non-scalar measurements; hMeas is used to build
         // the information matrix below.
         std::vector<RealArray> hMeas;

         #ifdef DEBUG_STM
            MessageInterface::ShowMessage("STM:\n");
            for (UnsignedInt j = 0; j < stateMap->size(); ++j)
            {
               MessageInterface::ShowMessage("      [");
               for (UnsignedInt k = 0; k < stateMap->size(); ++k)
               {
                  MessageInterface::ShowMessage("  %le  ", (*stm)(j,k));
               }
               MessageInterface::ShowMessage("]\n");
            }
         #endif

         Real entry;
         for (UnsignedInt i = 0; i < hTilde.size(); ++i)
         {
            hRow.assign(stateMap->size(), 0.0);

            // hRow is partial derivaties at apriori time t0
            for (UnsignedInt j = 0; j < stateMap->size(); ++j)
            {
               entry = 0.0;
               for (UnsignedInt k = 0; k < stateMap->size(); ++k)
               {
                  entry += hTilde[i][k] * (*stm)(k, j);
               }
               hRow[j] = entry;
            }

            // Convert hRow into solve-for coordinates
            RealArray hRowSolveFor(hRow.size(), 0.0);
            for (UnsignedInt ii = 0; ii < hRow.size(); ii++)
            {
               // Specify hAccum vector in Solve-for state
               for (UnsignedInt jj = 0; jj < cart2SolvMatrix.GetNumColumns(); ++jj)
                  hRowSolveFor[ii] += hRow[jj] * cart2SolvMatrix(jj, ii);
            }

            hMeas.push_back(hRowSolveFor);
            measStat.hAccum.push_back(hRowSolveFor);

            #ifdef DEBUG_ACCUMULATION_RESULTS
            // derivatives at time tm in Internal Cartesian coordinate system (such as Keplerian coordinate system)
            MessageInterface::ShowMessage("   Htilde  = [");
            for (UnsignedInt l = 0; l < stateMap->size(); ++l)
               MessageInterface::ShowMessage(  " %.12le ", hTilde[i][l]);
            MessageInterface::ShowMessage(  "]\n");

            // derivatives at time t0 in Internal Cartesian coordinate system (such as Keplerian coordinate system)
            MessageInterface::ShowMessage("   hRow = [");
            for (UnsignedInt l = 0; l < stateMap->size(); ++l)
               MessageInterface::ShowMessage(" %.12le ", hRow[l]);
            MessageInterface::ShowMessage("]\n");

            // matrix conversion from Internal Cartesian coordinate system to solve-for variables coordinate system
            MessageInterface::ShowMessage("cart2SolvMatrix = \n");
            for (UnsignedInt row = 0; row < cart2SolvMatrix.GetNumRows(); row++)
            {
               MessageInterface::ShowMessage("[");
               for (UnsignedInt col = 0; col < cart2SolvMatrix.GetNumColumns(); ++col)
                  MessageInterface::ShowMessage("%.12le   ", cart2SolvMatrix(row, col));
               MessageInterface::ShowMessage("]\n");
            }
            MessageInterface::ShowMessage("\n");

            // derivatives at time t0 in solve-for variables coordinate system (such as Keplerian coordinate system)
            MessageInterface::ShowMessage("   hRowSolveFor = [");
            for (UnsignedInt l = 0; l < stateMap->size(); ++l)
               MessageInterface::ShowMessage(  " %.12le ", hRowSolveFor[l]);
            MessageInterface::ShowMessage(  "]\n");

            MessageInterface::ShowMessage("   hMeas has %d rows (number of observables used for estimation)\n",
               hMeas.size());
            #endif
         }

         Real ocDiff;          // It is residual value = O - C
         RealArray ocDiffVec;  // It is residual vector = O_vector - C_vector
         Real weight;
         #ifdef DEBUG_ACCUMULATION
            MessageInterface::ShowMessage("Accumulating the O-C differences\n");
            MessageInterface::ShowMessage("   Obs size = %d, calc size = %d\n",
               currentObs->value.size(), calculatedMeas->value.size());
         #endif

         for (UnsignedInt k = 0; k < currentObs->value.size(); ++k)
         {
            // Calculate residual O-C
            if (calculatedMeas->isPeriodic) {
               ocDiff = currentObs->value[k] - calculatedMeas->value[k];
               if (fabs(ocDiff) > calculatedMeas->period / 2.0)
                  ocDiff = (calculatedMeas->period - fabs(ocDiff)) * (ocDiff < 0 ? 1 : -1);
            }
            else
               ocDiff = currentObs->value[k] - calculatedMeas->value[k];
            #ifdef DEBUG_O_MINUS_C
               Real OD_Epoch = theTimeConverter->Convert(currentObs->epoch,
                                    TimeSystemConverter::A1MJD, TimeSystemConverter::TAIMJD,
                                    GmatTimeConstants::JD_JAN_5_1941);
               Real C_Epoch = theTimeConverter->Convert(calculatedMeas->epoch,
                                    TimeSystemConverter::A1MJD, TimeSystemConverter::TAIMJD,
                                    GmatTimeConstants::JD_JAN_5_1941);
               MessageInterface::ShowMessage("Observation data: %s  %s  %s  Epoch = %.12lf  O = %.12le;         Calculated measurement: %s  %s  Epoch = %.12lf  C = %.12le;       O-C = %.12le     frequency = %.12le\n",
                  currentObs->typeName.c_str(), currentObs->participantIDs[0].c_str(), currentObs->participantIDs[1].c_str(), OD_Epoch, currentObs->value[k], 
                  calculatedMeas->participantIDs[0].c_str(), calculatedMeas->participantIDs[1].c_str(), C_Epoch, calculatedMeas->value[k], 
                  ocDiff, calculatedMeas->uplinkFreq);
            #endif


            // Calculate weight
            weight = GetMeasurementWeight(k);
            #ifdef DEBUG_WEIGHTS
            MessageInterface::ShowMessage("Covariance(%d %d) "
               "is %le\n", k, k, GetMeasurementCovariance(k,k));
            MessageInterface::ShowMessage("editedRecords[recNum = %d] = %d   NORMAL_FLAG = %d\n", recNum, editedRecords[recNum], NORMAL_FLAG);
            #endif

            // Accummulate information matrix and residuals based on observation 
            // data which is selected for estimation calculation
            if (editedRecords[recNum] == NORMAL_FLAG)
            {
               for (UnsignedInt i = 0; i < stateSize; ++i)
               {
                  for (UnsignedInt j = 0; j < stateSize; ++j)
                     information(i, j) += hMeas[k][i] * hMeas[k][j] * weight;   // the first term in open-close square bracket of equation 8-57 in GTDS MathSpec
                                                                                // this is actually hMeas[k][i] * weight * hMeas[k][j],
                                                                                // but rearranged for numerical precision reasons
                                                                                // to preserve the symmetry of the information matrix

                  residuals[i] += hMeas[k][i] * weight * ocDiff;               // the first term in open-close parenthesis of equation 8-57 in GTDS MathSpec
               }
            }
         }

         #ifdef DEBUG_ACCUMULATION_RESULTS
            MessageInterface::ShowMessage("Observed measurement value:\n");
            for (UnsignedInt k = 0; k < currentObs->value.size(); ++k)
               MessageInterface::ShowMessage("   %.12lf", currentObs->value[k]);
            MessageInterface::ShowMessage("\n");

            MessageInterface::ShowMessage("Calculated measurement value:\n");
            for (UnsignedInt k = 0; k < calculatedMeas->value.size(); ++k)
               MessageInterface::ShowMessage("   %.12lf",calculatedMeas->value[k]);
            MessageInterface::ShowMessage("\n");

            MessageInterface::ShowMessage("   O - C = ");
            for (UnsignedInt k = 0; k < calculatedMeas->value.size(); ++k)
               MessageInterface::ShowMessage("   %.12lf",
                  currentObs->value[k] - calculatedMeas->value[k]);
            MessageInterface::ShowMessage("\n");

            MessageInterface::ShowMessage("Derivatives w.r.t. state (H-tilde):\n");
            for (UnsignedInt k = 0; k < hTilde.size(); ++k)
            {
               for (UnsignedInt l = 0; l < hTilde[k].size(); ++l)
                  MessageInterface::ShowMessage("   %.12le",hTilde[k][l]);
               MessageInterface::ShowMessage("\n");
            }
            MessageInterface::ShowMessage("   Information Matrix = \n");
            for (Integer i = 0; i < estimationState->GetSize(); ++i)
            {
               MessageInterface::ShowMessage("      [");
               for (Integer j = 0; j < estimationState->GetSize(); ++j)
               {
                  MessageInterface::ShowMessage(" %.12lf ", information(i, j));
               }
               MessageInterface::ShowMessage("]\n");
            }
            MessageInterface::ShowMessage("   Residuals = [");
            for (Integer i = 0; i < residuals.GetSize(); ++i)
               MessageInterface::ShowMessage(" %.12lf ", residuals[i]);
            MessageInterface::ShowMessage("]\n");
         #endif

         // Get measurement statistics
         for (Integer k = 0; k < currentObs->value.size(); ++k)
         {
            Real ocDiff;
            if (calculatedMeas->isPeriodic) {
               ocDiff = currentObs->value[k] - calculatedMeas->value[k];
               if (fabs(ocDiff) > calculatedMeas->period / 2.0)
                  ocDiff = (calculatedMeas->period - fabs(ocDiff)) * (ocDiff < 0 ? 1 : -1);
            }
            else
               ocDiff = currentObs->value[k] - calculatedMeas->value[k];
            measStat.measValue.push_back(calculatedMeas->value[k]);
            measStat.residual.push_back(ocDiff);
            measStat.weight.push_back(GetMeasurementWeight(k));
         }

         measStat.tropoCorrectValue = calculatedMeas->tropoCorrectValue;
         measStat.ionoCorrectValue = calculatedMeas->ionoCorrectValue;

      } // end of if (count >= 1)

      measStat.isCalculated     = (count >= 1);
      measStat.uniqueID         = calculatedMeas->uniqueID;
      measStat.frequency        = calculatedMeas->uplinkFreqAtRecei;
      measStat.feasibilityValue = calculatedMeas->feasibilityValue;

   }  // end of if (modelsToAccess.size() == 0)

   // Write user edited flag
   if (editedRecords[recNum] & USER_FLAG)
      measManager.GetObsDataObject()->removedReason = "USER";

   measStat.editFlag      = editedRecords[recNum];
   measStat.removedReason = measManager.GetObsDataObject()->removedReason;
   measStats.push_back(measStat);

   if (!writeMeasurmentsAtEnd)
   {
      BuildMeasurementLine(measStat);
      AddMatlabData(measStat);
      WriteToTextFile(currentState);
   }

   // GMAT only resets when measurement editing is not freezed
   if ((!freezeEditing) || 
         ((freezeEditing) && (iterationsTaken < (freezeIteration-1))))
   {
      measManager.GetObsDataObject()->inUsed = true;
      measManager.GetObsDataObject()->removedReason = "N";
   }

   // Accumulate the processed data
   #ifdef RUN_SINGLE_PASS
      #ifdef SHOW_STATE_TRANSITIONS
         MessageInterface::ShowMessage("BatchEstimator state is ESTIMATING at "
               "epoch %.12lf\n", currentEpoch);
      #endif
   #endif

   #ifdef DEBUG_ACCUMULATION
      MessageInterface::ShowMessage("Advancing to the next measurement\n");
   #endif

   // Advance to the next measurement and get its epoch
   bool isEndOfTable = measManager.AdvanceObservation();
   if (isEndOfTable)
      currentState = ESTIMATING;
   else
   {
      nextMeasurementEpochGT = measManager.GetEpochGT();
      FindTimeStep();

      //if (currentEpoch <= (nextMeasurementEpoch + 5.0e-12))                  // It needs to add 5.0e-12 in order to avoid accuracy limit of double
      if ((currentEpochGT - nextMeasurementEpochGT).GetTimeInSec() <= 5.0e-7)
         currentState = PROPAGATING;
      else
         currentState = ESTIMATING;
   }

   #ifdef DEBUG_ACCUMULATION
      MessageInterface::ShowMessage("Exit BatchEstimator::Accumulate()\n");
   #endif

}




//------------------------------------------------------------------------------
// void Estimate()
//------------------------------------------------------------------------------
/**
 * This method updates the state estimate, covariance, and estimation statistics
 */
//------------------------------------------------------------------------------
void BatchEstimator::Estimate()
{
   #ifdef WALK_STATE_MACHINE
      MessageInterface::ShowMessage("BatchEstimator state is ESTIMATING\n");
   #endif

   // Plot all residuals
   if (showAllResiduals)
      PlotResiduals();

   // Display number of removed records for each type of filters
   if (!numRemovedRecords.empty())
   {
      if (!dataFilterStrings.empty())
         MessageInterface::ShowMessage("Number records edited by Second Level Data Editor : %d\n", numRemovedRecords["USER"]);

      MessageInterface::ShowMessage("Number of Records Removed Due To:\n");
      MessageInterface::ShowMessage("   . No Computed Value Configuration Available : %d\n", numRemovedRecords["U"]);
      MessageInterface::ShowMessage("   . Out of Ramp Table Range   : %d\n", numRemovedRecords["R"]);
      MessageInterface::ShowMessage("   . Signal Blocked : %d\n", numRemovedRecords["B"]);
      MessageInterface::ShowMessage("   . Initial RMS Sigma Filter  : %d\n", numRemovedRecords["IRMS"]);
      MessageInterface::ShowMessage("   . Outer-Loop Sigma Editor : %d\n", numRemovedRecords["OLSE"]);
   }
   MessageInterface::ShowMessage("Number of records used for estimation: %d\n", numRemovedRecords["N"]);

   UnsignedInt countMeasurements = 0;
   for (UnsignedInt ii = 0; ii < measStats.size(); ii++)
      if (measStats[ii].editFlag == NORMAL_FLAG)
         countMeasurements += measStats[ii].residual.size();

   if (countMeasurements < esm.GetStateMap()->size())
   {
      std::stringstream ss;
      ss << "Error: For Batch estimator " << GetName() 
         << ", there are " << esm.GetStateMap()->size() 
         << " solve-for parameters, and only " << countMeasurements 
         //<< " solve-for parameters, and only " << numRemovedRecords["N"]
         << " valid observable records remaining after editing. Please modify data editing criteria or provide a better a-priori estimate.\n";
      throw EstimatorException(ss.str());
   }

   // Apriori state (initial state for 0th iteration) and initial state for current iteration: 
   if (iterationsTaken == 0)
   {
      initialEstimationState = (*estimationState);
      initialEstimationStateS = estimationStateS;
   }
   oldEstimationState = (*estimationState);
   oldEstimationStateS = estimationStateS;

   // Convert previous state from GMAT internal coordinate system to participants' coordinate system
   previousSolveForState = esm.GetEstimationStateForReport();
   previousSolveForStateC = esm.GetEstimationCartesianStateForReport();
   previousSolveForStateK = esm.GetEstimationKeplerianStateForReport("MA");

   // Specify information matrix and residual vector in Solve-for state
#ifdef DEBUG_VERBOSE
   MessageInterface::ShowMessage("   Information matrix in Solve-for state:\n");
   for (UnsignedInt i = 0; i < information.GetNumRows(); ++i)
   {
      MessageInterface::ShowMessage("      [");
      for (UnsignedInt j = 0; j < information.GetNumColumns(); ++j)
      {
         MessageInterface::ShowMessage(" %.12lf ", information(i, j));
      }
      MessageInterface::ShowMessage("]\n");
   }
#endif

   if (useApriori)
   {
      Rmatrix Pdx0_inv;
      InvertApriori(Pdx0_inv);

      // adding a priori to information matrix
      information = information + Pdx0_inv;

      // adding a priori to residual
      for (Integer i = 0; i < Pdx0_inv.GetNumRows(); ++i)
      {
         for (UnsignedInt j = 0; j < stateSize; ++j)
         {
            residuals[i] += Pdx0_inv(i, j) * x0bar[j];      // At the beginning of each iteration, [Lambda] = ([Px0]^-1).delta_XTile(i)  the last term in open-close square bracket in euqation 8-57 GTDS MathSpec
         }
      }
   }


   // Solve normal equation
   #ifdef DEBUG_VERBOSE
      MessageInterface::ShowMessage("Accumulation complete; now solving the "
            "normal equations!\n");

      MessageInterface::ShowMessage("\nEstimating changes for iteration %d\n\n",
            iterationsTaken+1);

      MessageInterface::ShowMessage("   Presolution estimation state:\n      "
            "epoch = %s\n     Cartesian state: [", estimationState->GetEpochGT().ToString().c_str());
      for (UnsignedInt i = 0; i < stateSize; ++i)
         MessageInterface::ShowMessage("  %.12lf  ", (*estimationState)[i]);
      MessageInterface::ShowMessage("]\n");
      MessageInterface::ShowMessage("   Keplerian state: [");
      for (UnsignedInt i = 0; i < stateSize; ++i)
         MessageInterface::ShowMessage("  %.12lf  ", estimationStateS[i]);
      MessageInterface::ShowMessage("]\n");
   #endif

   SolveNormalEquations(information, informationInverse);

   IntegerArray normalMatrixIndexesSaved = removedNormalMatrixIndexes;  // save indexes which will be overwritten by InnerLoop()

   #ifdef DEBUG_VERBOSE
      MessageInterface::ShowMessage(" residuals: [");
      for (UnsignedInt i = 0; i < stateSize; ++i)
         MessageInterface::ShowMessage("  %.12lf  ", residuals(i));
      MessageInterface::ShowMessage("]\n");

      MessageInterface::ShowMessage("   covariance matrix:\n");
      for (UnsignedInt i = 0; i < informationInverse.GetNumRows(); ++i)
      {
         MessageInterface::ShowMessage("      [");
         for (UnsignedInt j = 0; j < informationInverse.GetNumColumns(); ++j)
         {
            MessageInterface::ShowMessage(" %.12lf ", informationInverse(i, j));
         }
         MessageInterface::ShowMessage("]\n");
      }

      MessageInterface::ShowMessage("   Before solving normal equation, estimationStateS = [\n");
      for (UnsignedInt i = 0; i < estimationStateS.GetSize(); ++i)
         MessageInterface::ShowMessage(" %.12lf   ", estimationStateS[i]);
      MessageInterface::ShowMessage("]\n");
   #endif

   // Calculate state change dx in equation 8-57 in GTDS MathSpec
   dx.clear();
   Real delta;
   for (UnsignedInt i = 0; i < stateSize; ++i)
   {
      delta = 0.0;
      for (UnsignedInt j = 0; j < stateSize; ++j)
         delta += informationInverse(i, j) * residuals(j);
      dx.push_back(delta);
      //estimationStateS[i] += delta;                                // Equation 8-24 GTSD MathSpec
   }

   // Specify previous, current, and the best weighted RMS:
   // Calculate RMSOLD:
   if (iterationsTaken > 0)
      oldResidualRMS = newResidualRMS;                       // old value is only valid from 1st iteration

   UnsignedIntArray measurementList;
   for (UnsignedInt ii = 0; ii < measStats.size(); ii++)
      if (measStats[ii].editFlag == NORMAL_FLAG)
         measurementList.push_back(ii);

   newResidualRMS = CalculateWRMS(measurementList);
   predictedRMS = CalculateWRMS(measurementList, dx);

   // Check Inner Loop if not yet frozen
   if (!freezeEditing || (freezeEditing && (iterationsTaken < freezeIteration)))
      InnerLoop();

   // Calculate RMSB:
   if (iterationsTaken == 0)
      bestResidualRMS = newResidualRMS;
   else
      bestResidualRMS = GmatMathUtil::Min(bestResidualRMS, newResidualRMS);

   for (UnsignedInt i = 0; i < stateSize; ++i)
      estimationStateS[i] += dx[i];                                // Equation 8-24 GTSD MathSpec

   // Convert estimation state from Keplerian to Cartesian
   esm.SetEstimationState(estimationStateS);                       // update the value of estimation state

   esm.RestoreObjects(&outerLoopBuffer);                           // Restore solver-object initial state
   esm.MapVectorToObjects();                                       // update objects state to current state
   removedNormalMatrixIndexes = normalMatrixIndexesSaved;          // restore indexes that were overwritten by InnerLoop()

   // Convert current estimation state from GMAT internal coordinate system to participants' coordinate system
   currentSolveForState = esm.GetEstimationStateForReport();
   currentSolveForStateC = esm.GetEstimationCartesianStateForReport();
   currentSolveForStateK = esm.GetEstimationKeplerianStateForReport("MA");

   /// Recalculate all conversion derivative matrixes for the new estimation state
   // Calculate conversion derivative matrix [dX/dS] from Cartesian to Solve-for state
   cart2SolvMatrix = esm.CartToSolveForStateConversionDerivativeMatrix();
   // Calculate conversion derivative matrix [dS/dK] from solve-for state to Keplerian
   solv2KeplMatrix = esm.SolveForStateToKeplConversionDerivativeMatrix();

   #ifdef DEBUG_VERBOSE
      MessageInterface::ShowMessage("   State vector change (dx):\n      [");
      for (UnsignedInt i = 0; i < stateSize; ++i)
         MessageInterface::ShowMessage("  %.12lf  ", dx[i]);
      MessageInterface::ShowMessage("]\n");

      MessageInterface::ShowMessage("   New estimation state:\n      "
            "epoch = %.12lf\n      [", estimationState->GetEpoch());
      for (UnsignedInt i = 0; i < stateSize; ++i)
         MessageInterface::ShowMessage("  %.12lf  ", (*estimationState)[i]);
      MessageInterface::ShowMessage("]\n");

      MessageInterface::ShowMessage("   After solving normal equation, estimationStateS = [\n");
      for (UnsignedInt i = 0; i < estimationStateS.GetSize(); ++i)
         MessageInterface::ShowMessage(" %.12lf   ", estimationStateS[i]);
      MessageInterface::ShowMessage("]\n");

   #endif

   // Write to report initial state for current iteration
   WriteToTextFile(currentState);

   currentState = CHECKINGRUN;
}


//------------------------------------------------------------------------------
//  Real CalculateWRMS(const UnsignedIntArray &measurementList) const
//------------------------------------------------------------------------------
/**
 * This method calculates the WRMS value.
 *
 * @param measurementList  List of measurement indicies to include in the
 *                         WRMS calculation.
 *
 * @return The WRMS value.
 */
//------------------------------------------------------------------------------
Real BatchEstimator::CalculateWRMS(const UnsignedIntArray &measurementList) const
{
   RealArray dx;
   dx.assign(stateSize, 0.0);

   return CalculateWRMS(measurementList, dx);
}


//------------------------------------------------------------------------------
//  Real CalculateWRMS(const UnsignedIntArray &measurementList, const RealArray &dx) const
//------------------------------------------------------------------------------
/**
 * This method calculates the predicted WRMS value for a change in the initial state.
 *
 * @param measurementList  List of measurement indicies to include in the
 *                         WRMS calculation.
 * @param dx               Change in the initial state to calculate the
 *                         predicted WRMS.
 *
 * @return The WRMS value.
 */
//------------------------------------------------------------------------------
Real BatchEstimator::CalculateWRMS(const UnsignedIntArray &measurementList, const RealArray &dx) const
{
   UnsignedInt count = 0;
   Real value = 0.0; // Accumulate the weighted squares of the residuals

   if (useApriori)
   {
      // The last term of RMSP in equation 8-185 in GTDS MathSpec
      GmatState currentEstimationStateS = estimationStateS;

      for (UnsignedInt ii = 0; ii < stateSize; ii++)
         currentEstimationStateS[ii] += dx[ii];

      Rmatrix Pdx0_inv;
      InvertApriori(Pdx0_inv);

      for (UnsignedInt i = 0; i < stateSize; ++i)
         for (UnsignedInt j = 0; j < stateSize; ++j)
            value += (currentEstimationStateS[i] - initialEstimationStateS[i])*Pdx0_inv(i, j)*(currentEstimationStateS[j] - initialEstimationStateS[j]);     // The second term inside square brackets of equation 8-185 GTDS MathSpec

      count++;
   }

   for (UnsignedInt ii = 0; ii < measurementList.size(); ii++)
   {
      MeasurementInfoType measStat = measStats[measurementList[ii]];
      count += measStat.residual.size();

      for (UnsignedInt jj = 0; jj < measStat.hAccum.size(); jj++)
      {
         Real residualChange = CalculateResidualChange(measStat.hAccum[jj], dx);

         // The first term in equation 8-185 in GTDS MathSpec
         value += (measStat.residual[jj] - residualChange) * (measStat.residual[jj] - residualChange) * measStat.weight[jj];
      }
   }

   // Thake the root of the mean squares
   value = sqrt(value / count);
   return value;
}


//------------------------------------------------------------------------------
//  Real CalculateResidualChange(const std::vector<RealArray> &hAccum, const RealArray &dx) const
//------------------------------------------------------------------------------
/**
 * This method calculates the change in the residual based on a delta in the
 * initial state and the linearized measurement derivatives for one measurement.
 *
 * @param hAccum   Linearized measurement derivatives.
 * @param dx       Change in the initial state to calculate the
 *                 linearized residial change.
 *
 * @return The linearized change in the residual.
 */
//------------------------------------------------------------------------------
Real BatchEstimator::CalculateResidualChange(const RealArray &hAccum, const RealArray &dx) const
{
   Real residualChange = 0;
   for (UnsignedInt ii = 0; ii < hAccum.size(); ii++)
      residualChange += hAccum[ii] * dx[ii];

   return residualChange;
}


//------------------------------------------------------------------------------
//  void InnerLoop()
//------------------------------------------------------------------------------
/**
 * This method performs the Inner Loop editing procedure.
 */
//------------------------------------------------------------------------------
void BatchEstimator::InnerLoop()
{
   #ifdef DEBUG_INNER_LOOP
      MessageInterface::ShowMessage("\nBatchEstimator %s check the inner loop.\n", useInnerLoop ? "will" : "will not");
   #endif

   if(useInnerLoop)
   {
      UnsignedIntArray indexUsedRecordsOL;
      UnsignedIntArray indexUsedRecords, indexUsedRecordsLast;
      UnsignedIntArray editedRecordsIL, editedRecordsILLast;
      bool convergedIL = false;
      estimationStatusIL = IL_UNKNOWN;

      Rmatrix informationIL(stateSize,stateSize);
      Rvector residualsIL(stateSize);
      RealArray dxIL, dxILLast;

      // Initialize inner loop with values from outer loop
      dxIL = dx;
      dxILLast = dxIL;

      GmatState currentEstimationStateIL = estimationStateS;

      Real newResidualRMSIL = newResidualRMS;
      Real predictedRMSIL = predictedRMS;

      #ifdef DEBUG_INNER_LOOP
         MessageInterface::ShowMessage("   Initial Inner Loop delta = [");
         for (UnsignedInt ii = 0; ii < dxIL.size(); ii++)
         {
            MessageInterface::ShowMessage("%lf", dxIL[ii]);
            if (ii + 1 < dxIL.size())
               MessageInterface::ShowMessage(", ");
         }
         MessageInterface::ShowMessage("]\n");

         MessageInterface::ShowMessage("   Initial Inner Loop state = [");
         for (UnsignedInt ii = 0; ii < dxIL.size(); ii++)
         {
            MessageInterface::ShowMessage("%lf", currentEstimationStateIL[ii] + dxIL[ii]);
            if (ii + 1 < dxIL.size())
               MessageInterface::ShowMessage(", ");
         }
         MessageInterface::ShowMessage("]\n");

         MessageInterface::ShowMessage("   Initial Inner Loop RMS = %lf\n", newResidualRMSIL);
         MessageInterface::ShowMessage("   Initial Inner Loop predicted RMS = %lf\n", predictedRMSIL);
      #endif

      // The first term of RMSP in equation 8-185 in GTDS MathSpec
      for (UnsignedInt ii = 0; ii < measStats.size(); ii++)
         if (measStats[ii].editFlag == NORMAL_FLAG)
            indexUsedRecordsOL.push_back(ii);

      indexUsedRecordsLast = indexUsedRecordsOL;

      MessageInterface::ShowMessage("\n");

      for (iterationsTakenIL = 0; iterationsTakenIL < maxIterationsIL; iterationsTakenIL++)
      {
         MessageInterface::ShowMessage("Performing Inner Loop iteration %d\n", iterationsTakenIL+1);

         // Initialize values for this loop iteration
         Real sigmaVal = (chooseRMSP ? predictedRMSIL : newResidualRMSIL);

         indexUsedRecords.clear();
         editedRecordsIL.clear();

         for (UnsignedInt ii = 0; ii < stateSize; ii++)
         {
            for (UnsignedInt jj = 0; jj < stateSize; jj++)
               informationIL(ii, jj) = 0.0;

            residualsIL[ii] = 0.0;
         }

         // Find change in residuals due to dxIL and determine if it should be edited by IL
         for (UnsignedInt ii = 0; ii < indexUsedRecordsOL.size(); ii++)
         {
            MeasurementInfoType measStat = measStats[indexUsedRecordsOL[ii]];
            bool removed = false;

            for (UnsignedInt vIndex = 0; vIndex < measStat.hAccum.size(); vIndex++) // Index for each value
            {
               Real residualChange = CalculateResidualChange(measStat.hAccum[vIndex], dxIL);

               if (sqrt(measStat.weight[vIndex])*GmatMathUtil::Abs(measStat.residual[vIndex] - residualChange) > (constMultIL*sigmaVal))
               {
                  removed = true;
                  break;
               }
            }

            if (removed)
            {
               editedRecordsIL.push_back(indexUsedRecordsOL[ii]); // List of IL edited measurements

               // Update IL information
               for (UnsignedInt vIndex = 0; vIndex < measStat.hAccum.size(); vIndex++)
               {
                  for (UnsignedInt i = 0; i < stateSize; ++i)
                  {
                     for (UnsignedInt j = 0; j < stateSize; ++j)
                        informationIL(i, j) += measStat.hAccum[vIndex][i] * measStat.hAccum[vIndex][j] * measStat.weight[vIndex];

                     residualsIL[i] += measStat.hAccum[vIndex][i] * measStat.weight[vIndex] * measStat.residual[vIndex];
                  }
               }
            }
            else
            {
               // Add to list of kept measurements
               indexUsedRecords.push_back(indexUsedRecordsOL[ii]);
            }
         }

         MessageInterface::ShowMessage("   The Inner Loop edited %d record(s).\n", editedRecordsIL.size());

         #ifdef DEBUG_INNER_LOOP
            MessageInterface::ShowMessage("   Edited records: [");
            for (UnsignedInt ii = 0; ii < editedRecordsIL.size(); ii++)
            {
               MessageInterface::ShowMessage("%d", editedRecordsIL[ii]);
               if (ii + 1 < editedRecordsIL.size())
                  MessageInterface::ShowMessage(", ");
            }
            MessageInterface::ShowMessage("]\n");
         #endif

         newResidualRMSIL = CalculateWRMS(indexUsedRecords);

         #ifdef DEBUG_INNER_LOOP
            MessageInterface::ShowMessage("   New Inner Loop RMS = %lf\n", newResidualRMSIL);
         #endif

         informationIL = information - informationIL;
         residualsIL = residuals - residualsIL;

         // Solve normal equations
         Rmatrix cov;
         SolveNormalEquations(informationIL, cov);

         // Calculate state change dx in equation 8-57 in GTDS MathSpec
         Real delta;
         for (UnsignedInt i = 0; i < stateSize; ++i)
         {
            delta = 0.0;
            for (UnsignedInt j = 0; j < stateSize; ++j)
               delta += cov(i,j) * residualsIL(j);
            dxIL[i] = delta;
            currentEstimationStateIL[i] = estimationStateS[i] + delta;
         }

         #ifdef DEBUG_INNER_LOOP
            MessageInterface::ShowMessage("   New Inner Loop delta = [");
            for (UnsignedInt ii = 0; ii < dxIL.size(); ii++)
            {
               MessageInterface::ShowMessage("%lf", dxIL[ii]);
               if (ii + 1 < dxIL.size())
                  MessageInterface::ShowMessage(", ");
            }
            MessageInterface::ShowMessage("]\n");

            MessageInterface::ShowMessage("   New Inner Loop state = [");
            for (UnsignedInt ii = 0; ii < dxIL.size(); ii++)
            {
               MessageInterface::ShowMessage("%lf", currentEstimationStateIL[ii]);
               if (ii + 1 < dxIL.size())
                  MessageInterface::ShowMessage(", ");
            }
            MessageInterface::ShowMessage("]\n");
         #endif

         predictedRMSIL = CalculateWRMS(indexUsedRecords, dxIL);

         #ifdef DEBUG_INNER_LOOP
            MessageInterface::ShowMessage("   New Inner Loop predicted RMS = %lf\n", predictedRMSIL);
         #endif

         // Check convergence criteria
         if (editedRecordsIL.size() == editedRecordsILLast.size())
         {
            bool allMatch = true;

            for (UnsignedInt ii = 0; ii < editedRecordsIL.size(); ii++)
            {
               if (editedRecordsIL[ii] != editedRecordsILLast[ii])
               {
                  allMatch = false;
                  break;
               }
            }

            if (allMatch)
            {
               convergedIL = true;
               estimationStatusIL = IL_SAME_EDITS_CONVERGED;
               iterationsTakenIL++;
               break;
            }
         }

         // Prepare for next loop
         indexUsedRecordsLast = indexUsedRecords;
         editedRecordsILLast = editedRecordsIL;
         dxILLast = dxIL;
      }

      if (estimationStatusIL == IL_UNKNOWN && iterationsTakenIL == maxIterationsIL)
         estimationStatusIL = IL_MAX_ITERATIONS_DIVERGED; // If not converged on last iteration

      MessageInterface::ShowMessage("Inner Loop Iterations : %d\n", iterationsTakenIL);

      MessageInterface::ShowMessage("Inner Loop Termination : ");
      switch (estimationStatusIL)
      {
      case IL_SAME_EDITS_CONVERGED:
         MessageInterface::ShowMessage("Identical edited records");
         break;
      case IL_MAX_ITERATIONS_DIVERGED:
         MessageInterface::ShowMessage("Maximum iterations");
         break;
      case IL_UNKNOWN:
      default:
         MessageInterface::ShowMessage("UNKNOWN");
         break;
      }
      MessageInterface::ShowMessage("\n");

      MessageInterface::ShowMessage("Inner Loop edited records: %d\n", editedRecordsIL.size());
      MessageInterface::ShowMessage("Number of records used for estimation: %d\n", numRemovedRecords["N"]);

      UnsignedInt numEditedIL = editedRecordsIL.size();

      numRemovedRecords["N"] -= numEditedIL;
      numRemovedRecords["ILSE"] = numEditedIL;

      // Mark IL edited measurements
      for (UnsignedInt ii = 0; ii < editedRecordsIL.size(); ii++)
      {
         UnsignedInt index = editedRecordsIL[ii];
         measStats[index].editFlag      = ILSE_FLAG;
         measStats[index].removedReason = "ILSE";

         // Change measurement editing if frozen
         if (freezeEditing  && (iterationsTaken >= (freezeIteration-1)))
         {
            measManager.GetObsDataObject(measStats[index].recNum)->inUsed = false;
            measManager.GetObsDataObject(measStats[index].recNum)->removedReason = "ILSE";
         }
      }

      newResidualRMS = newResidualRMSIL;
      predictedRMS = predictedRMSIL;

      dx = dxIL;
   }
}


//------------------------------------------------------------------------------
// void SolveNormalEquations(const Rmatrix &infMatrix, Rmatrix &covMatrix)
//------------------------------------------------------------------------------
/**
 * This method solves the normal equations using the selected inversionType
 */
//------------------------------------------------------------------------------
void BatchEstimator::SolveNormalEquations(const Rmatrix &infMatrix, Rmatrix &covMatrix)
{
   #ifdef DEBUG_VERBOSE
      MessageInterface::ShowMessage("   Information matrix:\n");
      for (UnsignedInt i = 0; i < infMatrix.GetNumRows(); ++i)
      {
         MessageInterface::ShowMessage("      [");
         for (UnsignedInt j = 0; j < infMatrix.GetNumColumns(); ++j)
         {
            MessageInterface::ShowMessage(" %.12lf ", infMatrix(i,j));
         }
         MessageInterface::ShowMessage("]\n");
      }
   #endif

   Integer iSize = infMatrix.GetNumColumns();
   if (iSize != infMatrix.GetNumRows())
   {
      if (inversionType == "Schur")
         throw EstimatorException("Schur inversion requires a square information "
            "matrix");
      else if (inversionType == "Cholesky")
         throw EstimatorException("Cholesky inversion requires a symmetric positive "
           "definite information matrix");
      else
         throw EstimatorException("Inversion requires a square information matrix");
   }

   Integer numRemoved;
   IntegerArray auxVector;
   Rmatrix reducedCovMatrix;

   // copy infMatrix to reducedInfMatrix, remove 0 rows/columns
   Rmatrix reducedInfMatrix = MatrixFactorization::CompressNormalMatrix(infMatrix,
      removedNormalMatrixIndexes, auxVector, numRemoved);
   if (numRemoved == iSize)
      throw EstimatorException("Error: Normal matrix has no rows/columns after "
         "removing all rows/columns of zeros.\n");

   if (numRemoved > 0) {
      //MessageInterface::ShowMessage("Matrix reduction was performed on the normal matrix\n");

      const std::vector<ListItem*> *map = esm.GetStateMap();
      for (int i = 0; i < removedNormalMatrixIndexes.size(); i++)
      {
         // *** Performed normal matrix reduction for EstSat.EarthMJ2000Eq.VZ
         int index = removedNormalMatrixIndexes.at(i);
         std::stringstream ss;
         ss << "*** Performed normal matrix reduction for ";
         if (((*map)[index]->object->IsOfType(Gmat::MEASUREMENT_MODEL)) &&
            ((*map)[index]->elementName == "Bias"))
         {
            //MeasurementModel* mm = (MeasurementModel*)((*map)[index]->object);
            TrackingDataAdapter* mm = (TrackingDataAdapter*)((*map)[index]->object);
            StringArray sa = mm->GetStringArrayParameter("Participants");
            ss << mm->GetStringParameter("Type") << " ";
            for (UnsignedInt j = 0; j < sa.size(); ++j)
               ss << sa[j] << (((j + 1) != sa.size()) ? "," : " Bias.");
            ss << (*map)[index]->subelement;
         }
         else
            ss << GetElementFullName((*map)[index], false);
         ss << "\n";
         MessageInterface::ShowMessage(ss.str());
      }
   }

   #ifdef DEBUG_VERBOSE
      if (numRemoved > 0)
      {
         MessageInterface::ShowMessage("   Information matrix (zero row/cols removed):\n");
         for (UnsignedInt i = 0; i < reducedInfMatrix.GetNumRows(); ++i)
         {
            MessageInterface::ShowMessage("      [");
            for (UnsignedInt j = 0; j < reducedInfMatrix.GetNumColumns(); ++j)
            {
               MessageInterface::ShowMessage(" %.12lf ", reducedInfMatrix(i, j));
            }
            MessageInterface::ShowMessage("]\n");
         }
      }
   #endif

   reducedCovMatrix.SetSize(iSize - numRemoved, iSize - numRemoved);

   if (inversionType == "Schur")
   {
      // first use Cholesky to determine if matrix is invertible - Schur will invert poorly conditioned matrices,
      // whereas Cholesky will throw an exception.  If the matrix is poorly conditioned, we want GMAT to stop, rather
      // than to continue processing and give the user a bad result
      Rmatrix testMatrix;
      CholeskyFactorization cf;
      testMatrix = reducedInfMatrix;
      try {
         cf.Invert(testMatrix);
      }
      catch (BaseException &ex) {
         throw EstimatorException("Cholesky algorithm used for error checking only:  " + ex.GetDetails());
      }

      // now invert the matrix using the Schur inversion that the user requested
      SchurFactorization sf;

      reducedCovMatrix = reducedInfMatrix;

      sf.Invert(reducedCovMatrix);
   }
   else if (inversionType == "Cholesky")
   {
      CholeskyFactorization cf;

      reducedCovMatrix = reducedInfMatrix;

      cf.Invert(reducedCovMatrix);
   }
   else
   {
      try
      {
         reducedCovMatrix = reducedInfMatrix.Inverse();
      } 
      catch (...)
      {
         #ifdef DEBUG_INVERSION
            MessageInterface::ShowMessage("Information matrix:\n");
            for (UnsignedInt i = 0; i < reducedInfMatrix.GetNumRows(); ++i)
            {
               MessageInterface::ShowMessage("      [");
               for (UnsignedInt j = 0; j < reducedInfMatrix.GetNumColumns(); ++j)
               {
                  MessageInterface::ShowMessage(" %.12lf ", reducedInfMatrix(i, j));
               }
               MessageInterface::ShowMessage("]\n");
            }
         #endif
         throw EstimatorException("Error: Normal matrix is singular.\n");
      }
   }

   #ifdef DEBUG_VERBOSE
      MessageInterface::ShowMessage("   Covariance matrix%s:\n", numRemoved > 0 ? " (reduced)" : "");
      for (UnsignedInt i = 0; i < reducedCovMatrix.GetNumRows(); ++i)
      {
         MessageInterface::ShowMessage("      [");
         for (UnsignedInt j = 0; j < reducedCovMatrix.GetNumColumns(); ++j)
         {
            MessageInterface::ShowMessage(" %.12lf ", reducedCovMatrix(i, j));
         }
         MessageInterface::ShowMessage("]\n");
      }
   #endif

   covMatrix = MatrixFactorization::ExpandNormalMatrixInverse(reducedCovMatrix,
      auxVector, numRemoved);

   #ifdef DEBUG_VERBOSE
      if (numRemoved > 0)
      {
         MessageInterface::ShowMessage("   Covariance matrix (zero row/cols restored):\n");
         for (UnsignedInt i = 0; i < covMatrix.GetNumRows(); ++i)
         {
            MessageInterface::ShowMessage("      [");
            for (UnsignedInt j = 0; j < covMatrix.GetNumColumns(); ++j)
            {
               MessageInterface::ShowMessage(" %.12lf ", covMatrix(i, j));
            }
            MessageInterface::ShowMessage("]\n");
         }
      }
   #endif
}


//-------------------------------------------------------------------------
// bool DataFilter()
//-------------------------------------------------------------------------
/**
* This function is used to filter bad observation data records. It has
*   1. Data filter based on OLSEInitialRMSSigma
*   2. Data filter based on outer-loop sigma editting
*/
//-------------------------------------------------------------------------
bool BatchEstimator::DataFilter()
{
   const ObservationData *currentObs =  measManager.GetObsData();                              // Get observation measurement data O
   const MeasurementData *calculatedMeas = measManager.GetMeasurement(modelsToAccess[0]);      // Get calculated measurement data C

   if (iterationsTaken == 0)
   {
      for (Integer i=0; i < currentObs->value.size(); ++i)
      {
         // 1. Data filtered based on OLSEInitialRMSSigma
         // 1.1. Specify Weight
         Real weight = GetMeasurementWeight(i);

         // 1.2. Filter based on maximum residual multiplier
#ifdef DEBUG_DATA_FILTER
         MessageInterface::ShowMessage("Epoch = %.12lf A1Mjd   O = %.6lf   C = %.6lf   w = %le    sqrt(w)*Abs(O-C) = %.6lf   maxResidualMult = %lf\n", calculatedMeas->epoch, currentObs->value[i], calculatedMeas->value[i], weight, sqrt(weight)*GmatMathUtil::Abs(currentObs->value[i] - calculatedMeas->value[i]), maxResidualMult);
#endif
         if (sqrt(weight)*GmatMathUtil::Abs(currentObs->value[i] - calculatedMeas->value[i]) > maxResidualMult)   // if (Wii*GmatMathUtil::Abs(O-C) > maximum residual multiplier) then throw away this data record
         {
            measManager.GetObsDataObject()->inUsed = false;
            measManager.GetObsDataObject()->removedReason = "IRMS";            // "IRMS": represent for OLSEInitialRMSSigma
            std::string filterName = "IRMS";
#ifdef DEBUG_DATA_FILTER
            MessageInterface::ShowMessage("This record is fillted.\n");
#endif
            break;
         }
      }
   }
   else
   {
     for (Integer i=0; i < currentObs->value.size(); ++i)
     {
         // 2. Data filtered based on outer-loop sigma editting
         // 2.1. Specify Weight
         Real weight = GetMeasurementWeight(i);

         // 2.2. Filter based on n-sigma
         Real sigmaVal = (chooseRMSP ? predictedRMS : newResidualRMS);
         if (sqrt(weight)*GmatMathUtil::Abs(currentObs->value[i] - calculatedMeas->value[i]) > (constMult*sigmaVal + additiveConst))   // if (Wii*GmatMathUtil::Abs(O-C) > k*sigma+ K) then throw away this data record
         {
            measManager.GetObsDataObject()->inUsed = false;
            measManager.GetObsDataObject()->removedReason = "OLSE";                     // "OLSE": represent for outer-loop sigma filter
            break;
         }
      }
   }

   return measManager.GetObsDataObject()->inUsed;
}


Real BatchEstimator::ObservationDataCorrection(Real cValue, Real oValue, Real moduloConstant)
{
   Real delta = cValue - oValue;
   int N = (int)(delta/moduloConstant + 0.5);

   return (oValue + N*moduloConstant);
}


void BatchEstimator::ValidateMediaCorrection(const MeasurementData* measData)
{
   if (measData->isIonoCorrectWarning)
   {
      // Get measurement pass:
      std::stringstream ss1;
      ss1 << "{{";
      for (Integer i = 0; i < measData->participantIDs.size(); ++i)
      {
         ss1 << measData->participantIDs[i] << (((i + 1) < measData->participantIDs.size()) ? "," : "");
      }
      ss1 << "}," << measData->typeName << "}";

      // if the pass is not in warning list, then display warning message
      if (find(ionoWarningList.begin(), ionoWarningList.end(), ss1.str()) == ionoWarningList.end())
      {
         // generate warning message
         MessageInterface::ShowMessage("Warning: When running estimator '%s', ionosphere correction is %lf m for measurement %s at measurement time tag %.12lf A1Mjd. Media corrections to the computed measurement may be inaccurate.\n", GetName().c_str(), measData->ionoCorrectRawValue * 1000.0, ss1.str().c_str(), measData->epoch);

         // add pass to the list
         ionoWarningList.push_back(ss1.str());
      }

   }

   if (measData->isTropoCorrectWarning)
   {
      // Get measurement path:
      std::stringstream ss1;
      ss1 << "{{";
      for (Integer i = 0; i < measData->participantIDs.size(); ++i)
      {
         ss1 << measData->participantIDs[i] << (((i + 1) < measData->participantIDs.size()) ? "," : "");
      }
      ss1 << "}," << measData->typeName << "}";

      // if the pass is not in warning list, then display warning message
      if (find(tropoWarningList.begin(), tropoWarningList.end(), ss1.str()) == tropoWarningList.end())
      {
         // generate warning message
         MessageInterface::ShowMessage("Warning: When running estimator '%s', troposphere correction is %lf m for measurement %s at measurement time tag %.12lf A1Mjd. Media corrections to the computed measurement may be inaccurate.\n", GetName().c_str(), measData->tropoCorrectRawValue * 1000.0, ss1.str().c_str(), measData->epoch);

         // add pass to the list
         tropoWarningList.push_back(ss1.str());
      }
   }
}
