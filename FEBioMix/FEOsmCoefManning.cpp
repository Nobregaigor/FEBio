//
//  FEOsmCoefManning.cpp
//  FEBioMix
//
//  Created by Gerard Ateshian on 1/8/16.
//  Copyright © 2016 febio.org. All rights reserved.
//

#include "FEOsmCoefManning.h"
#include "FECore/FEModel.h"

//-----------------------------------------------------------------------------
// define the material parameters
BEGIN_PARAMETER_LIST(FEOsmCoefManning, FEOsmoticCoefficient)
    ADD_PARAMETER2(m_ksi, FE_PARAM_DOUBLE, FE_RANGE_GREATER_OR_EQUAL(0.0), "ksi"  );
    ADD_PARAMETER (m_sol, FE_PARAM_INT, "co_ion");
    ADD_PARAMETER2(m_osmc, FE_PARAM_DOUBLE, FE_RANGE_GREATER_OR_EQUAL(0.0), "osmc");
END_PARAMETER_LIST();

//-----------------------------------------------------------------------------
//! Constructor.
FEOsmCoefManning::FEOsmCoefManning(FEModel* pfem) : FEOsmoticCoefficient(pfem)
{
    m_ksi = 1;
    m_sol = -1;
    m_lsol = -1;
    m_lc = -1;
    m_osmc = 1;
    m_pMP = nullptr;
    m_pLC = nullptr;
}

//-----------------------------------------------------------------------------
bool FEOsmCoefManning::Init()
{
    // get the ancestor material which must be a multiphasic material
    m_pMP = dynamic_cast<FEMultiphasic*> (GetAncestor());
    if (m_pMP == nullptr) return MaterialError("Ancestor material must be multiphasic");
    
    // extract the local id of the solute from the global id
    m_lsol = m_pMP->FindLocalSoluteID(m_sol-1); // m_sol must be zero-based
    if (m_lsol == -1) return MaterialError("Invalid value for sol");
    
    // extract optional load curve for Wells analysis
    if (m_lc >= 0 && m_pLC == nullptr)
    {
        m_pLC = GetFEModel()->GetLoadCurve(m_lc);
        if (m_pLC == 0) return false;
    }
    
    return true;
}

//-----------------------------------------------------------------------------
//! Osmotic coefficient
double FEOsmCoefManning::OsmoticCoefficient(FEMaterialPoint& mp)
{
    double phiPM = OsmoticCoefficient_Manning(mp);
    double phiMM = OsmoticCoefficient_Wells(mp);
    
    return phiPM + phiMM - 1;
}

//-----------------------------------------------------------------------------
//! Tangent of osmotic coefficient with respect to strain
double FEOsmCoefManning::Tangent_OsmoticCoefficient_Strain(FEMaterialPoint &mp)
{
    double dphiPMdJ = Tangent_OsmoticCoefficient_Strain_Manning(mp);
    double dphiMMdJ = Tangent_OsmoticCoefficient_Strain_Wells(mp);
    
    return dphiPMdJ + dphiMMdJ;
}

//-----------------------------------------------------------------------------
//! Tangent of osmotic coefficient with respect to concentration
double FEOsmCoefManning::Tangent_OsmoticCoefficient_Concentration(FEMaterialPoint &mp, const int isol)
{
    double dphiPMdc = Tangent_OsmoticCoefficient_Concentration_Manning(mp,isol);
    double dphiMMdc = Tangent_OsmoticCoefficient_Concentration_Wells(mp,isol);
    
    return dphiPMdc + dphiMMdc;
}

//-----------------------------------------------------------------------------
//! Osmotic coefficient
double FEOsmCoefManning::OsmoticCoefficient_Manning(FEMaterialPoint& mp)
{
    FESolutesMaterialPoint& spt = *mp.ExtractData<FESolutesMaterialPoint>();
    
    // evaluate X = FCD/co-ion actual concentration
    double ca = spt.m_ca[m_lsol];
    double cF = fabs(spt.m_cF);
    double X = 0;
    if (ca > 0) X = cF/ca;
    
    // --- Manning osmotic coefficient ---
    double osmcoef;
    if (m_ksi <= 1)
        osmcoef = 1 - 0.5*m_ksi*X/(X+2);
    else
        osmcoef = (0.5*X/m_ksi+2)/(X+2);

    assert(osmcoef>0);
    
    return osmcoef;
}

//-----------------------------------------------------------------------------
//! Tangent of osmotic coefficient with respect to strain
double FEOsmCoefManning::Tangent_OsmoticCoefficient_Strain_Manning(FEMaterialPoint &mp)
{
    FEElasticMaterialPoint& pt = *mp.ExtractData<FEElasticMaterialPoint>();
    FEBiphasicMaterialPoint& bpt = *mp.ExtractData<FEBiphasicMaterialPoint>();
    FESolutesMaterialPoint& spt = *mp.ExtractData<FESolutesMaterialPoint>();
    
    // evaluate X = FCD/co-ion actual concentration
    double ca = spt.m_ca[m_lsol];
    double cF = fabs(spt.m_cF);
    double X = 0;
    if (ca > 0) X = cF/ca;
    
    // evaluate dX/dJ
    double J = pt.m_J;
    double phisr = bpt.m_phi0;
    double kt = spt.m_k[m_lsol];
    double dktdJ = spt.m_dkdJ[m_lsol];
    double dXdJ = -(1./(J-phisr)+dktdJ/kt)*X;
    
    double dosmdX;
    if (m_ksi <= 1)
        dosmdX = -m_ksi/pow(X+2, 2);
    else
        dosmdX = (1-2*m_ksi)/m_ksi/pow(X+2, 2);
    
    return dosmdX*dXdJ;
}

//-----------------------------------------------------------------------------
//! Tangent of osmotic coefficient with respect to concentration
double FEOsmCoefManning::Tangent_OsmoticCoefficient_Concentration_Manning(FEMaterialPoint &mp, const int isol)
{
    FESolutesMaterialPoint& spt = *mp.ExtractData<FESolutesMaterialPoint>();
    
    // evaluate X = FCD/co-ion actual concentration
    double ca = spt.m_ca[m_lsol];
    double cF = fabs(spt.m_cF);
    double X = 0;
    if (ca > 0) X = cF/ca;
    
    // evaluate dX/dc
    double kta = spt.m_k[m_lsol];
    double kt = spt.m_k[isol];
    int zt = m_pMP->GetSolute(isol)->ChargeNumber();
    double dXdc = -zt*kt/ca;
    if (isol == m_lsol) dXdc -= kta*X/ca;
    
    double dosmdX;
    if (m_ksi <= 1)
        dosmdX = -m_ksi/pow(X+2, 2);
    else
        dosmdX = (1./m_ksi-2)/pow(X+2, 2);
    
    return dosmdX*dXdc;
}

//-----------------------------------------------------------------------------
//! Osmotic coefficient
double FEOsmCoefManning::OsmoticCoefficient_Wells(FEMaterialPoint& mp)
{
    FESolutesMaterialPoint& spt = *mp.ExtractData<FESolutesMaterialPoint>();
    
    double ca = spt.m_ca[m_lsol];
    double osmc = m_osmc;
    
    if (m_pLC != nullptr)
        osmc *= m_pLC->Value(ca);
    
    assert(osmc>0);
    
    return osmc;
}

//-----------------------------------------------------------------------------
//! Tangent of osmotic coefficient with respect to strain
double FEOsmCoefManning::Tangent_OsmoticCoefficient_Strain_Wells(FEMaterialPoint &mp)
{
    FESolutesMaterialPoint& spt = *mp.ExtractData<FESolutesMaterialPoint>();
    
    double ca = spt.m_ca[m_lsol];
    double dosmc = 0;
    
    if (m_pLC != nullptr)
        dosmc = m_osmc*m_pLC->Deriv(ca);
    
    double f = spt.m_dkdJ[m_lsol]*spt.m_c[m_lsol];
    
    return dosmc*f;
}

//-----------------------------------------------------------------------------
//! Tangent of osmotic coefficient with respect to concentration
double FEOsmCoefManning::Tangent_OsmoticCoefficient_Concentration_Wells(FEMaterialPoint &mp, const int isol)
{
    FESolutesMaterialPoint& spt = *mp.ExtractData<FESolutesMaterialPoint>();
    
    double ca = spt.m_ca[m_lsol];
    double dosmc = 0;
    
    if (m_pLC != nullptr)
        dosmc = m_osmc*m_pLC->Deriv(ca);
    
    double f = spt.m_dkdc[m_lsol][isol]*spt.m_c[m_lsol];
    if (isol == m_lsol) f += spt.m_k[m_lsol];
    
    return dosmc*f;
}

//-----------------------------------------------------------------------------
void FEOsmCoefManning::Serialize(DumpStream& ar)
{
    if (ar.IsSaving())
    {
        ar << m_osmc << m_lc;
    }
    else
    {
        ar >> m_osmc >> m_lc;
        m_pLC = ar.GetFEModel().GetLoadCurve(m_lc);

    }
}

//-----------------------------------------------------------------------------
bool FEOsmCoefManning::SetParameterAttribute(FEParam& p, const char* szatt, const char* szval)
{
    if (strcmp(p.m_szname, "osmc") == 0)
    {
        if (strcmp(szatt, "lc") == 0)
        {
            m_lc = atoi(szval) - 1;
            return true;
        }
    }
    return false;
}