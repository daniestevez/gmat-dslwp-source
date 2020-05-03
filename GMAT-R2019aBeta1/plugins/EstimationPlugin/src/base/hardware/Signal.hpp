//$Id: Sensor.hpp 67 2010-06-08 21:56:16Z  $
//------------------------------------------------------------------------------
//                         Sensor
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
// Author: Tuan Dang Nguyen
// Created: June 8, 2010
//
/**
 * The signal of a sensor.
 */
//------------------------------------------------------------------------------

#ifndef Signal_hpp
#define Signal_hpp

#include "estimation_defs.hpp"
#include "gmatdefs.hpp"

class ESTIMATION_API Signal
{
public:
   Signal();
   virtual ~Signal();
   Signal(const Signal& sig);
   Signal& operator=(const Signal& sig);


   void SetEpoch(GmatEpoch ep);
   GmatEpoch GetEpoch();
   Real GetValue();
   bool SetValue(Real v);

private:
   GmatEpoch epoch;
   Real value;
};

#endif /* Signal_hpp */
