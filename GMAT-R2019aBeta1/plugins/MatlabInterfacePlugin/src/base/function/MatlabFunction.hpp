//$Id$
//------------------------------------------------------------------------------
//                                  MatlabFunction
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
// number S-67573-G
//
// Author: Allison Greene
// Created: 2004/09/22
//
/**
 * Definition for the MatlabFunction class.
 */
//------------------------------------------------------------------------------


#ifndef MatlabFunction_hpp
#define MatlabFunction_hpp

#include "matlabinterface_defs.hpp"
#include "Function.hpp"

class MATLAB_API MatlabFunction : public Function
{
public:
   MatlabFunction(const std::string &nomme);
   virtual ~MatlabFunction();
   MatlabFunction(const MatlabFunction &copy);
   MatlabFunction& operator=(const MatlabFunction &right);
   
   // inherited from GmatBase
   virtual GmatBase*    Clone() const;
   virtual void         Copy(const GmatBase* orig);
   
   virtual bool         SetStringParameter(const Integer id, 
                                           const std::string &value);
   virtual bool         SetStringParameter(const std::string &label,
                                           const std::string &value);
   
protected:
   bool SetMatlabFunctionPath(const std::string &path);   
};

#endif
