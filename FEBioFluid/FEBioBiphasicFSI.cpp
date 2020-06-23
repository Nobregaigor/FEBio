/*This file is part of the FEBio source code and is licensed under the MIT license
listed below.

See Copyright-FEBio.txt for details.

Copyright (c) 2020 University of Utah, The Trustees of Columbia University in 
the City of New York, and others.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#include "stdafx.h"
#include "FEBioBiphasicFSI.h"
#include <FECore/FECoreKernel.h>
#include "FEBiphasicFSISolver.h"
#include "FEBiphasicFSI.h"
#include "FEBiphasicFSIDomain3D.h"
#include "FEBiphasicFSIDomainFactory.h"
#include "FEBiphasicFSITraction.h"

//-----------------------------------------------------------------------------
const char* FEBioBiphasicFSI::GetVariableName(FEBioBiphasicFSI::BIPHASIC_FSI_VARIABLE var)
{
    switch (var)
    {
        case DISPLACEMENT                : return "displacement"               ; break;
        case VELOCITY                    : return "velocity"                   ; break;
        case SHELL_ROTATION              : return "shell rotation"             ; break;
        case SHELL_DISPLACEMENT          : return "shell displacement"         ; break;
        case SHELL_VELOCITY              : return "shell velocity"             ; break;
        case SHELL_ACCELERATION          : return "shell acceleration"         ; break;
        case RIGID_ROTATION              : return "rigid rotation"             ; break;
        case RELATIVE_FLUID_VELOCITY     : return "relative fluid velocity"    ; break;
        case RELATIVE_FLUID_ACCELERATION : return "relative fluid acceleration"; break;
        case FLUID_VELOCITY              : return "fluid velocity"             ; break;
        case FLUID_ACCELERATION          : return "fluid acceleration"         ; break;
        case FLUID_DILATATION            : return "fluid dilation"             ; break;
        case FLUID_DILATATION_TDERIV     : return "fluid dilation tderiv"      ; break;
    }
    assert(false);
    return nullptr;
}

void FEBioBiphasicFSI::InitModule()
{
    FECoreKernel& febio = FECoreKernel::GetInstance();
    
    // register domain
    febio.RegisterDomain(new FEBiphasicFSIDomainFactory);
    
    // define the fsi module
    febio.CreateModule("biphasic-FSI");
    febio.SetModuleDependency("fluid-FSI");
    febio.SetModuleDependency("fluid");
    
    REGISTER_FECORE_CLASS(FEBiphasicFSISolver, "biphasic-FSI");
    
    REGISTER_FECORE_CLASS(FEBiphasicFSI, "biphasic-FSI");
    
    REGISTER_FECORE_CLASS(FEBiphasicFSIDomain3D, "biphasic-FSI-3D");
    
    REGISTER_FECORE_CLASS(FEBiphasicFSITraction, "biphasic-FSI traction");
    
    febio.SetActiveModule(0);
}
