#include "StdAfx.h"
#include "ModelInfoSA.h"

WRAPPER void CBaseModelInfo::Shutdown() { EAXJMP(0x4C4D50); }

WRAPPER RwTexture* CCustomCarPlateMgr::CreatePlateTexture(const char* pText, signed char nDesign) { EAXJMP(0x6FDEA0); }
WRAPPER bool CCustomCarPlateMgr::GeneratePlateText(char* pBuf, int nLen) { EAXJMP(0x6FD5B0); }
WRAPPER signed char CCustomCarPlateMgr::GetMapRegionPlateDesign() { EAXJMP(0x6FD7A0); }
WRAPPER void CCustomCarPlateMgr::SetupMaterialPlatebackTexture(RpMaterial* pMaterial, signed char nDesign) { EAXJMP(0x6FDE50); }

void CVehicleModelInfo::Shutdown()
{
	CBaseModelInfo::Shutdown();

	delete m_apPlateMaterials;
}

void CVehicleModelInfo::FindEditableMaterialList()
{
	std::pair<CVehicleModelInfo*,int>		MatsPair = std::make_pair(this, 0);

	RpClumpForAllAtomics(reinterpret_cast<RpClump*>(pRwObject), GetEditableMaterialListCB, &MatsPair);

	if ( m_pVehicleStruct->m_nNumExtras > 0 )
	{
		for ( int i = 0; i < m_pVehicleStruct->m_nNumExtras; i++ )
			GetEditableMaterialListCB(m_pVehicleStruct->m_apExtras[i], &MatsPair);
	}

	m_nPrimaryColor = -1;
	m_nSecondaryColor = -1;
	m_nTertiaryColor = -1;
	m_nQuaternaryColor = -1;
}

void CVehicleModelInfo::SetCarCustomPlate()
{
	m_plateText[0] = '\0';
	m_nPlateType = -1;

	m_apPlateMaterials = reinterpret_cast<RpMaterial**>(new DWORD_PTR[2*NUM_MAX_PLATES]);
	
	for ( int i = 0; i < 2*NUM_MAX_PLATES; i++ )
		m_apPlateMaterials[i] = nullptr;
CCustomCarPlateMgr::SetupClump(reinterpret_cast<RpClump*>(pRwObject), m_apPlateMaterials);
}

RpAtomic* CVehicleModelInfo::GetEditableMaterialListCB(RpAtomic* pAtomic, void* pData)
{
	RpGeometryForAllMaterials(RpAtomicGetGeometry(pAtomic), GetEditableMaterialListCB, pData);
	return pAtomic;
}

RpMaterial* CVehicleModelInfo::GetEditableMaterialListCB(RpMaterial* pMaterial, void* pData)
{
	if ( RpMaterialGetTexture(pMaterial) )
	{
		if ( !strncmp(RwTextureGetName(RpMaterialGetTexture(pMaterial)), "vehiclegrunge256", 16) )
		{
			std::pair<CVehicleModelInfo*,int>*	pMats = static_cast<std::pair<CVehicleModelInfo*,int>*>(pData);

			if ( pMats->second < 32 )
				pMats->first->m_apDirtMaterials[pMats->second++] = pMaterial;
		}
	}
	return pMaterial;
}

static RpMaterial* PollPlateData(RpMaterial* pMaterial, void* pData)
{
	if ( RwTexture* pTexture = RpMaterialGetTexture(pMaterial) )
	{
		if ( RwTextureGetName(pTexture) )
		{
			if ( !_strnicmp(RwTextureGetName(pTexture), "carplate", 8) )
			{
				auto&		pCallbackData = *static_cast<std::tuple<RpMaterial**,RpMaterial**,unsigned char,unsigned char>*>(pData);

				assert(std::get<2>(pCallbackData) < NUM_MAX_PLATES);
				if ( std::get<2>(pCallbackData)++ < NUM_MAX_PLATES )
					*(std::get<0>(pCallbackData)++) = pMaterial;
			}
			else if ( !_strnicmp(RwTextureGetName(pTexture), "carpback", 8) )
			{
				auto&		pCallbackData = *static_cast<std::tuple<RpMaterial**,RpMaterial**,unsigned char,unsigned char>*>(pData);

				assert(std::get<3>(pCallbackData) < NUM_MAX_PLATES);
				if ( std::get<3>(pCallbackData)++ < NUM_MAX_PLATES )
					*(std::get<1>(pCallbackData)++) = pMaterial;
			}
		}
	}


	return pMaterial;
}

static RpAtomic* PollPlateData(RpAtomic* pAtomic, void* pData)
{
	RpGeometryForAllMaterials(RpAtomicGetGeometry(pAtomic), PollPlateData, pData);
	return pAtomic;
}

void CCustomCarPlateMgr::SetupClump(RpClump* pClump, RpMaterial** pMatsArray)
{
	// Split pMatsArray
	std::tuple<RpMaterial**,RpMaterial**,unsigned char,unsigned char>	CallbackData = std::make_tuple(pMatsArray, pMatsArray+NUM_MAX_PLATES, 0, 0);

	RpClumpForAllAtomics(pClump, PollPlateData, &CallbackData);
}