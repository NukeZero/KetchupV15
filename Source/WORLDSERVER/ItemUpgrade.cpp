#include "stdafx.h"
#include ".\itemupgrade.h"

#if __VER >= 12 // __EXT_PIERCING
#include "defineObj.h"
#include "defineSound.h"
#include "defineText.h"

#if __VER >= 11 // __SYS_COLLECTING
#include "collecting.h"
#include "definesound.h"
#include "defineitem.h"
#endif	// __SYS_COLLECTING

#include "User.h"
extern CUserMng g_UserMng;
#include "DPSrvr.h"
extern CDPSrvr g_DPSrvr;
#include "DPDatabaseClient.h"
extern CDPDatabaseClient g_dpDBClient;

#if __VER >= 15 // __PETVIS
#include "AIPet.h"
#endif // __PETVIS

CItemUpgrade::CItemUpgrade(void)
#ifdef __SYS_ITEMTRANSY
: m_nItemTransyLowLevel(1000000), m_nItemTransyHighLevel(2000000)
#endif // __SYS_ITEMTRANSY
{
	LoadScript();
}

CItemUpgrade::~CItemUpgrade(void)
{
	m_mapSuitProb.clear();
	m_mapWeaponProb.clear();
#if __VER >= 13 // __EXT_ENCHANT
	m_mapGeneralEnchant.clear();
	m_mapAttributeEnchant.clear();
#endif // __EXT_ENCHANT
}

CItemUpgrade* CItemUpgrade::GetInstance( void )
{	
	static CItemUpgrade sItemUpgrade;
	return &sItemUpgrade;
}

void CItemUpgrade::LoadScript()
{
	CLuaBase	lua;
	if( lua.RunScript( "ItemUpgrade.lua" ) != 0 )
	{
		Error( "CItemUpgrade::LoadScript() - ItemUpgrade.lua Run Failed!!!" );
		ASSERT(0);
	}

	// 规绢备 乔绢教
	lua.GetGloabal( "tSuitProb" );
	lua.PushNil();
	while( lua.TableLoop( -2 ) )
	{
		m_mapSuitProb.insert( make_pair( static_cast<int>(lua.ToNumber(-2)), static_cast<int>(lua.ToNumber(-1)) ) );
		lua.Pop( 1 );
	}
	lua.Pop(0);

	// 公扁 乔绢教
	lua.GetGloabal( "tWeaponProb" );
	lua.PushNil();
	while( lua.TableLoop( -2 ) )
	{
		m_mapWeaponProb.insert( make_pair( static_cast<int>(lua.ToNumber(-2)), static_cast<int>(lua.ToNumber(-1)) ) );
		lua.Pop( 1 );
	}
	lua.Pop(0);
	
#if __VER >= 13 // __EXT_ENCHANT
	// 老馆力访
	lua.GetGloabal( "tGeneral" );
	lua.PushNil();
	while( lua.TableLoop( -2 ) )
	{
		m_mapGeneralEnchant.insert( make_pair( static_cast<int>(lua.ToNumber(-2)), static_cast<int>(lua.ToNumber(-1)) ) );
		lua.Pop( 1 );
	}
	lua.Pop(0);

 	// 加己力访
	lua.GetGloabal( "tAttribute" );
 	lua.PushNil();
 	while( lua.TableLoop( -2 ) )
 	{
 		__ATTRIBUTE_ENCHANT attrEnchant;
		attrEnchant.nProb = static_cast<int>(lua.GetFieldToNumber( -1, "nProb" ));
		attrEnchant.nAddDamageRate = static_cast<int>(lua.GetFieldToNumber( -1, "nDamageRate" ));
#if __VER >= 14 // __EXT_ATTRIBUTE
		attrEnchant.nDefenseRate = static_cast<int>(lua.GetFieldToNumber( -1, "nDefenseRate" ));
		attrEnchant.nAddAtkDmgRate = static_cast<int>(lua.GetFieldToNumber( -1, "nAddAtkDmgRate" ));
#endif // __EXT_ATTRIBUTE
		m_mapAttributeEnchant.insert( make_pair( static_cast<int>(lua.ToNumber(-2)), attrEnchant ) );  
		lua.Pop( 1 );
#if __VER < 14 // __EXT_ATTRIBUTE
		if( ( ::GetLanguage() == LANG_FRE || ::GetLanguage() == LANG_GER ) && m_mapAttributeEnchant.size() == 10 )
			break;
#endif // __EXT_ATTRIBUTE
  	}
	lua.Pop(0);
#endif // __EXT_ENCHANT
#ifdef __SYS_ITEMTRANSY
	m_nItemTransyLowLevel = static_cast<int>( lua.GetGlobalNumber( "nItemTransyLowLevel" ) );
	m_nItemTransyHighLevel = static_cast<int>( lua.GetGlobalNumber( "nItemTransyHighLevel" ) );
	lua.Pop(0);
#endif // __SYS_ITEMTRANSY
}


void CItemUpgrade::OnPiercingSize( CUser* pUser, DWORD dwId1, DWORD dwId2, DWORD dwId3 )
{
	CItemElem* pItemElem0	= pUser->m_Inventory.GetAtId( dwId1 );
	CItemElem* pItemElem1	= pUser->m_Inventory.GetAtId( dwId2 );
	CItemElem* pItemElem2	= pUser->m_Inventory.GetAtId( dwId3 );

	if( IsUsableItem( pItemElem0 ) == FALSE || IsUsableItem( pItemElem1 ) == FALSE )
		return;

	if( pUser->m_vtInfo.GetOther() || pUser->m_vtInfo.VendorIsVendor() ) // 芭贰 吝捞搁...
		return;	
	
	if( pUser->m_Inventory.IsEquip( pItemElem0->m_dwObjId ) )
	{
		pUser->AddDefinedText( TID_GAME_EQUIPPUT );
		return;
	}	

	//////////////// 霉锅掳 酒捞袍 //////////////// 
	if( !pItemElem0->IsPierceAble( NULL_ID, TRUE ) )
	{
		pUser->AddDefinedText( TID_PIERCING_POSSIBLE_ITEM );
		return;
	}

	if( pItemElem1->GetProp()->dwID != II_GEN_MAT_MOONSTONE 
		&& pItemElem1->GetProp()->dwID != II_GEN_MAT_MOONSTONE_1 )
	{
		pUser->AddDefinedText( TID_SBEVE_NOTUSEITEM );			// 乔绢教俊 鞘夸茄 林荤困啊 酒聪搁 阂啊瓷
		return;
	}
	
	if( IsUsableItem( pItemElem2 ) && pItemElem2->m_dwItemId != II_SYS_SYS_SCR_PIEPROT )
	{
		pUser->AddDefinedText( TID_SBEVE_NOTUSEITEM );			// 惑侩酒捞袍捞 酒聪搁 阂啊瓷
		return;
	}

	LogItemInfo aLogItem;
	aLogItem.SendName = pUser->GetName();
	aLogItem.RecvName = "PIERCING";
	aLogItem.WorldId = pUser->GetWorld()->GetID();

	int nCost = 100000;

	if( 0 < nCost )
	{
		if( pUser->GetGold() < nCost )
		{
			pUser->AddDefinedText( TID_GAME_LACKMONEY , "" );
			return;
		}

		pUser->AddGold( -( nCost ) );

		aLogItem.Gold = pUser->GetGold() + nCost;
		aLogItem.Gold2 = pUser->GetGold();
		aLogItem.Action = "!";
		//aLogItem.ItemName = "SEED";
		_stprintf( aLogItem.szItemName, "%d", II_GOLD_SEED1 );
		aLogItem.itemNumber = nCost;
		g_DPSrvr.OnLogItem( aLogItem );
	}
	else
	{
		return;
	}
	aLogItem.Gold = aLogItem.Gold2 = pUser->GetGold();

	int nPersent = 0;
	if( pItemElem1->GetProp()->dwID == II_GEN_MAT_MOONSTONE
		|| pItemElem1->GetProp()->dwID == II_GEN_MAT_MOONSTONE_1 )
		nPersent = GetSizeProb( pItemElem0 );

	if( nPersent < (int)( xRandom( 10000 ) ) )
	{	// 角菩
		if( pItemElem2 )								// 惑侩拳 酒捞袍阑 荤侩窍看栏搁...
			aLogItem.RecvName = "PIERCING_PROTECTED";
		aLogItem.Action = "!";
		g_DPSrvr.OnLogItem( aLogItem, pItemElem0, pItemElem0->m_nItemNum );
		aLogItem.RecvName = "PIERCING";

		pUser->AddPlaySound( SND_INF_UPGRADEFAIL );
		g_UserMng.AddCreateSfxObj((CMover *)pUser, XI_INT_FAIL, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z);
		pUser->AddDefinedText( TID_MMI_PIERCINGFAIL , "" );
		
		if( pItemElem2 == NULL )								// 惑侩拳 酒捞袍阑 荤侩窍瘤 臼疽促搁 
			pUser->RemoveItem( (BYTE)( dwId1 ), (short)1 );	// 乔绢教 措惑 酒捞袍篮 昏力等促.			
	}
	else
	{	// 己傍			
		pUser->AddPlaySound( SND_INF_UPGRADESUCCESS );			
		g_UserMng.AddCreateSfxObj((CMover *)pUser, XI_INT_SUCCESS, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z);			
		pUser->UpdateItem( (BYTE)pItemElem0->m_dwObjId, UI_PIERCING_SIZE, pItemElem0->GetPiercingSize() + 1 );
		pUser->AddDefinedText( TID_MMI_PIERCINGSUCCESS , "" );

		aLogItem.Action = "#";
		g_DPSrvr.OnLogItem( aLogItem, pItemElem0, pItemElem0->m_nItemNum );
	}
	aLogItem.Action = "!";
	g_DPSrvr.OnLogItem( aLogItem, pItemElem1, pItemElem1->m_nItemNum );

	// 促捞胶客 蜡丰酒捞袍 昏力茄促.
	pUser->RemoveItem( (BYTE)( dwId2 ), (short)1 );

	if( dwId3 != NULL_ID )
	{
		aLogItem.Action = "!";
		if( IsUsableItem( pItemElem2 ) )
		{
			g_DPSrvr.OnLogItem( aLogItem, pItemElem2, pItemElem2->m_nItemNum );
			g_dpDBClient.SendLogSMItemUse( "2", pUser, pItemElem2, pItemElem2->GetProp() );
			pUser->RemoveItem( (BYTE)( dwId3 ), (short)1 );
		}
		else
		{
			g_DPSrvr.OnLogItem( aLogItem, NULL, 0 );
		}
	}
}

int CItemUpgrade::GetSizeProb( CItemElem* pItemElem )
{
	// 祈过栏肺...IK3_SOCKETCARD啊 沥惑捞搁 酱飘...
	if( pItemElem->IsPierceAble( IK3_SOCKETCARD ) )
	{
		//return m_vecSuitProb.size() >= pItemElem->GetPiercingSize() ? m_vecSuitProb[pItemElem->GetPiercingSize()] : 0;
		map<int, int>::iterator it = m_mapSuitProb.find( pItemElem->GetPiercingSize()+1 );
		if( it != m_mapSuitProb.end() )
			return it->second;
	}
	
	// 祈过栏肺...IK3_SOCKETCARD2啊 沥惑捞搁 公扁率...
	if( pItemElem->IsPierceAble( IK3_SOCKETCARD2 ) )
	{
		//return m_vecWeaponProb.size() >= pItemElem->GetPiercingSize() ? m_vecWeaponProb[pItemElem->GetPiercingSize()] : 0;
		map<int, int>::iterator it = m_mapWeaponProb.find( pItemElem->GetPiercingSize()+1 );
		if( it != m_mapWeaponProb.end() )
			return it->second;
	}

	return 0;
}

void CItemUpgrade::OnPiercing( CUser* pUser, DWORD dwItemId, DWORD dwSocketCard )
{
	// 牢亥配府俊 乐绰瘤 厘馒登绢 乐绰瘤 犬牢阑 秦具 窃
	CItemElem* pItemElem0	= pUser->m_Inventory.GetAtId( dwItemId );
	CItemElem* pItemElem1	= pUser->m_Inventory.GetAtId( dwSocketCard );
	if( IsUsableItem( pItemElem0 ) == FALSE || IsUsableItem( pItemElem1 ) == FALSE )
		return;

	// 厘馒登绢 乐绰 酒捞袍篮 乔绢教 给窃
	if( pUser->m_Inventory.IsEquip( dwItemId ) )
	{
		pUser->AddDefinedText( TID_GAME_EQUIPPUT , "" );
		return;
	}			

	// 墨靛啊 甸绢哎 酒捞袍捞 乔绢教 啊瓷茄瘤 八荤
	if( !pItemElem0->IsPierceAble() )
	{
		pUser->AddDefinedText(  TID_PIERCING_POSSIBLE_ITEM, "" );
		return;
	}

	//  IK3_SOCKETCARD?啊 酒聪搁 乔绢教 给窃
	if( !pItemElem0->IsPierceAble( pItemElem1->GetProp()->dwItemKind3 ) )
	{
		pUser->AddDefinedText( TID_UPGRADE_ERROR_WRONGUPLEVEL , "" );			
		return;					
	}

	// 醚 乔绢教等荐客 傈眉 荐甫 厚背茄促.
	int nSize = pItemElem0->GetPiercingSize();

	int nCount = 0;
	for( int j = 0; j < nSize; j++ )
	{
		if( pItemElem0->GetPiercingItem( j ) != 0 )
			nCount++;
	}

	// 后镑捞 绝栏搁 吝窜
	if( nCount == nSize )
	{
		pUser->AddDefinedText( TID_PIERCING_ERROR_NOPIERCING, "" );
		return;
	}

	// 爽
	if( pUser->m_vtInfo.GetOther() )	// 芭贰吝牢 措惑捞 乐栏搁?
		return;
	if( pUser->m_vtInfo.VendorIsVendor() )		// 郴啊 迫绊 乐栏搁?
		return;

	LogItemInfo aLogItem;
	aLogItem.SendName = pUser->GetName();
	aLogItem.RecvName = "PIERCING";
	aLogItem.WorldId = pUser->GetWorld()->GetID();
	aLogItem.Gold = aLogItem.Gold2 = pUser->GetGold();

	if( pUser->Pierce( pItemElem0, pItemElem1->m_dwItemId ) )
	{
		aLogItem.Action = "$";
		g_DPSrvr.OnLogItem( aLogItem, pItemElem0, pItemElem0->m_nItemNum );
		aLogItem.Action = "!";
		g_DPSrvr.OnLogItem( aLogItem, pItemElem1, pItemElem1->m_nItemNum );

		// 酒捞袍 冠扁 己傍~
		pUser->AddPlaySound( SND_INF_UPGRADESUCCESS );
		g_UserMng.AddCreateSfxObj((CMover *)pUser, XI_INT_SUCCESS, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z);
		// 犁丰 酒捞袍 昏力
		pUser->RemoveItem( (BYTE)( dwSocketCard ), (short)1 );
	}

}

void CItemUpgrade::OnPiercingRemove( CUser* pUser, DWORD objId )
{
	CItemElem* pItemElem = pUser->m_Inventory.GetAtId( objId );
	if( !IsUsableItem( pItemElem ) || !pItemElem->IsPierceAble() )
		return;

	if( pUser->m_Inventory.IsEquip( objId ) )
		return;

	// 乔绢教 可记捞 绝绰 版快
	if( pItemElem->GetPiercingSize() == 0 || pItemElem->GetPiercingItem( 0 ) == 0 )
	{
		pUser->AddDefinedText( TID_GAME_REMOVE_PIERCING_ERROR );
		return;
	}

	int nPayPenya = 1000000; // 瘤阂且 其衬
	if( pUser->GetGold() < nPayPenya )	// 其衬啊 何练窍促.
	{
		pUser->AddDefinedText( TID_GAME_LACKMONEY );
		return;
	}

	for( int i=pItemElem->GetPiercingSize()-1; i>=0; i-- )
	{
		if( pItemElem->GetPiercingItem( i ) != 0 )
		{
			pUser->AddGold( -nPayPenya );	// 其衬 瘤阂
			pUser->AddDefinedText( TID_GAME_REMOVE_PIERCING_SUCCESS );
			pUser->UpdateItem( (BYTE)( pItemElem->m_dwObjId ), UI_PIERCING, MAKELONG( i, 0 ) );

			LogItemInfo aLogItem;
			aLogItem.Action = "$";
			aLogItem.SendName = pUser->GetName();
			aLogItem.RecvName = "PIERCING_REMOVE";
			aLogItem.WorldId = pUser->GetWorld()->GetID();
			aLogItem.Gold = pUser->GetGold() + nPayPenya;
			aLogItem.Gold2 = pUser->GetGold();
			aLogItem.Gold_1 = -nPayPenya;
			g_DPSrvr.OnLogItem( aLogItem, pItemElem, 1 );
			break;
		}
	}
}
#endif // __EXT_PIERCING

#if __VER >= 13 // __EXT_ENCHANT
void	CItemUpgrade::OnEnchant( CUser* pUser, CItemElem* pItemMain, CItemElem* pItemMaterial )
{
#if __VER >= 14 // __SMELT_SAFETY
	if( !IsUsableItem( pItemMain ) || !IsUsableItem( pItemMaterial ) )
		return;
	// 措惑捞 厘馒吝牢啊?
	if( pUser->m_Inventory.IsEquip( pItemMain->m_dwObjId ) )
	{
		pUser->AddDefinedText( TID_GAME_EQUIPPUT , "" );
		return;
	}
#endif // __SMELT_SAFETY
	
	switch( pItemMaterial->GetProp()->dwItemKind3 )
	{
		case IK3_ELECARD:	
			EnchantAttribute( pUser, pItemMain, pItemMaterial );
			break;

		case IK3_ENCHANT:	
			EnchantGeneral( pUser, pItemMain, pItemMaterial );
			break;

		default:
#if __VER >= 14 // __SMELT_SAFETY
			if( pItemMain->IsAccessory() )
				RefineAccessory( pUser, pItemMain, pItemMaterial );

			else if( pItemMain->IsCollector() )
				RefineCollector( pUser, pItemMain, pItemMaterial );
#endif // __SMELT_SAFETY
			break;
	}
}

#if __VER >= 14 // __SMELT_SAFETY
BYTE	CItemUpgrade::OnSmeltSafety( CUser* pUser, CItemElem* pItemMain, CItemElem* pItemMaterial, CItemElem* pItemProtScr, CItemElem* pItemSmeltScr )
{
	// 犁丰俊 蝶扼 盒扁
	switch( pItemMaterial->GetProp()->dwItemKind3 )
	{
		//	坷府漠鸟老锭
		case IK3_ENCHANT:
			//	老馆力访(老馆 or 倔磐岗傀迄牢瘤 犁八荤)
			return SmeltSafetyNormal( pUser, pItemMain, pItemMaterial, pItemProtScr, pItemSmeltScr );

		//	巩胶沛老锭
		case IK3_PIERDICE:
			//	厩技辑府牢啊
			if( pItemMain->IsAccessory() )
				return SmeltSafetyAccessory( pUser, pItemMain, pItemMaterial, pItemProtScr );
			//	乔绢教牢啊
			else if( pItemMain->IsPierceAble( NULL_ID, TRUE ) )
				return SmeltSafetyPiercingSize( pUser, pItemMain, pItemMaterial, pItemProtScr );

#if __VER >= 15 // __15_5TH_ELEMENTAL_SMELT_SAFETY
		// 加己墨靛 老锭
		case IK3_ELECARD:
			return SmeltSafetyAttribute( pUser, pItemMain, pItemMaterial, pItemProtScr, pItemSmeltScr );
#endif // __15_5TH_ELEMENTAL_SMELT_SAFETY

		default:
			break;
	}
	return 0;
}

BYTE	CItemUpgrade::SmeltSafetyNormal( CUser* pUser, CItemElem* pItemMain, CItemElem* pItemMaterial, CItemElem* pItemProtScr, CItemElem* pItemSmeltScr )
{
	//	坷府漠鸟牢啊, 蝴唱绰 坷府漠鸟牢啊
	switch( pItemMaterial->GetProp()->dwID )
	{
		//	坷府漠鸟老锭 老馆力访
		case II_GEN_MAT_ORICHALCUM01:
		case II_GEN_MAT_ORICHALCUM01_1:
			return SmeltSafetyGeneral( pUser, pItemMain, pItemMaterial, pItemProtScr, pItemSmeltScr );

		//	蝴唱绰 坷府漠鸟老锭 倔磐岗傀迄 力访
		case II_GEN_MAT_ORICHALCUM02:
			return prj.m_UltimateWeapon.SmeltSafetyUltimate( pUser, pItemMain, pItemMaterial, pItemProtScr );

		default:
			break;
	}
	return 0;
}

BYTE	CItemUpgrade::SmeltSafetyGeneral( CUser* pUser, CItemElem* pItemMain, CItemElem* pItemMaterial, CItemElem* pItemProtScr, CItemElem* pItemSmeltScr )
{
	//	力访啊瓷茄 酒捞袍捞 酒匆版快 府畔
	if( !CItemElem::IsDiceRefineryAble(pItemMain->GetProp()) )
	{
		//pUser->AddDefinedText( TID_GAME_NOTEQUALITEM );
		return 0;
	}
	
	//	倔磐岗傀迄 捞芭唱 老馆焊龋狼 滴风付府啊 酒匆版快 府畔
	if( pItemMain->GetProp()->dwReferStat1 == WEAPON_ULTIMATE || pItemProtScr->GetProp()->dwID != II_SYS_SYS_SCR_SMELPROT )
	{
		//pUser->AddDefinedText( TID_GAME_NOTEQUALITEM );
		return 0;
	}
	
	//	力访荐摹啊 max摹甫 逞菌阑锭 府畔
	if( pItemMain->GetAbilityOption() >= GetMaxGeneralEnchantSize() )
	{
		//pUser->AddDefinedText( TID_UPGRADE_MAXOVER );
		return 3;
	}
	
	// 1000窜困狼 己傍 欺季飘
	int nPercent = GetGeneralEnchantProb( pItemMain->GetAbilityOption() );
	
	//	力访狼 滴风付府甫 荤侩沁促搁
	if( pItemSmeltScr != NULL )
	{
		//	力访狼 滴风付府啊 嘎绰瘤 犬牢
		if( IsUsableItem( pItemSmeltScr ) && pItemSmeltScr->GetProp()->dwID == II_SYS_SYS_SCR_SMELTING )
		{
			//	力访狼 滴风付府 荤侩啊瓷 荐摹牢啊
			if( pItemMain->GetAbilityOption() < 7 )
			{
				nPercent += 1000;
				ItemProp* pItemProp = pItemSmeltScr->GetProp();
				if( pItemProp)
				{
					g_dpDBClient.SendLogSMItemUse( "1", pUser, pItemSmeltScr, pItemProp );
					g_dpDBClient.SendLogSMItemUse( "2", pUser, NULL, pItemProp );
				}
				pUser->RemoveItem( (BYTE)( pItemSmeltScr->m_dwObjId ), 1 );
			}
		}
		//	力访狼 滴风付府啊 酒匆锭
		else
			return 0;
	}

	LogItemInfo aLogItem;
	aLogItem.Action = "N";
	aLogItem.SendName = pUser->GetName();
	aLogItem.RecvName = "UPGRADEITEM_SMELTSAFETY";
	aLogItem.WorldId = pUser->GetWorld()->GetID();
	aLogItem.Gold = pUser->GetGold();
	aLogItem.Gold2 = pUser->GetGold();

	g_DPSrvr.OnLogItem( aLogItem, pItemMaterial, pItemMaterial->m_nItemNum );
	// 力访酒袍 昏力 - 己傍捞带, 角菩带...
	pUser->RemoveItem( (BYTE)( pItemMaterial->m_dwObjId ), 1 );
	//	焊龋狼 滴风付府 肺弊巢辨巴
	ItemProp* pItemProp = pItemProtScr->GetProp();
	if( pItemProp )
	{
		g_dpDBClient.SendLogSMItemUse( "1", pUser, pItemProtScr, pItemProp );
		g_dpDBClient.SendLogSMItemUse( "2", pUser, NULL, pItemProp );
	}
	pUser->RemoveItem( (BYTE)( pItemProtScr->m_dwObjId ), 1 );

	// 秦寸 酒捞袍狼 加己, 老馆 饭骇阑 掘绢 犬啦阑 波辰促.
	if( (int)( xRandom( 10000 ) ) > nPercent )
	{
		// 角菩 皋技瘤 免仿
		//pUser->AddDefinedText( TID_UPGRADE_FAIL );
		pUser->AddPlaySound( SND_INF_UPGRADEFAIL );
		
		if((pUser->IsMode( TRANSPARENT_MODE ) ) == 0)
			g_UserMng.AddCreateSfxObj( (CMover *)pUser, XI_INT_FAIL, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z );
		
		aLogItem.Action = "F";
		g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );

		return 2;
	}
	else
	{
		// 己傍
		//pUser->AddDefinedText( TID_UPGRADE_SUCCEEFUL );
		pUser->AddPlaySound( SND_INF_UPGRADESUCCESS );
		
		if((pUser->IsMode( TRANSPARENT_MODE ) ) == 0)
			g_UserMng.AddCreateSfxObj( (CMover *)pUser, XI_INT_SUCCESS, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z );
		
		pUser->UpdateItem( (BYTE)pItemMain->m_dwObjId, UI_AO,  pItemMain->GetAbilityOption() + 1 );
		aLogItem.Action = "H";
		g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );

		return 1;
	}
	return 0;
}

BYTE	CItemUpgrade::SmeltSafetyAccessory(CUser* pUser, CItemElem* pItemMain, CItemElem* pItemMaterial, CItemElem* pItemProtScr )
{
	// 犁丰啊 巩胶沛牢啊?
	if( pItemMaterial->GetProp()->dwID != II_GEN_MAT_MOONSTONE && pItemMaterial->GetProp()->dwID != II_GEN_MAT_MOONSTONE_1 )
	{
		//pUser->AddDefinedText( TID_GAME_NOTEQUALITEM );
		return 0;
	}
	//	厩技辑府 焊龋狼 滴风付府啊 酒匆版快 府畔
	if( pItemProtScr->GetProp()->dwID != II_SYS_SYS_SCR_SMELPROT4 )
	{
		//pUser->AddDefinedText( TID_GAME_NOTEQUALITEM );
		return 0;
	}
	//	力访荐摹啊 max摹甫 逞菌阑锭 府畔
	if( pItemMain->GetAbilityOption() >= MAX_AAO )	// 20
	{
		//pUser->AddDefinedText( TID_GAME_ACCESSORY_MAX_AAO );
		return 3;
	}

	// log
	LogItemInfo aLogItem;
	aLogItem.SendName	= pUser->GetName();
	aLogItem.RecvName	= "UPGRADEITEM_SMELTSAFETY";
	aLogItem.WorldId	= pUser->GetWorld()->GetID();
	aLogItem.Gold	= pUser->GetGold();
	aLogItem.Gold2	= pUser->GetGold();

	aLogItem.Action	= "N";
	g_DPSrvr.OnLogItem( aLogItem, pItemMaterial, pItemMaterial->m_nItemNum );
	pUser->RemoveItem( (BYTE)( pItemMaterial->m_dwObjId ), 1 );
	// 咀技辑府 焊龋狼 滴风付府 肺弊 巢辨巴
	ItemProp* pItemProp = pItemProtScr->GetProp();
	if( pItemProp )
	{
		g_dpDBClient.SendLogSMItemUse( "1", pUser, pItemProtScr, pItemProp );
		g_dpDBClient.SendLogSMItemUse( "2", pUser, NULL, pItemProp );
	}
	pUser->RemoveItem( (BYTE)( pItemProtScr->m_dwObjId ), 1 );

	// 力访 犬伏
	DWORD dwProbability = CAccessoryProperty::GetInstance()->GetProbability( pItemMain->GetAbilityOption() );

	if( xRandom( 10000 ) > dwProbability )	// 角菩
	{
		pUser->AddPlaySound( SND_INF_UPGRADEFAIL );
		if( pUser->IsMode( TRANSPARENT_MODE ) == 0 )
			g_UserMng.AddCreateSfxObj( (CMover *)pUser, XI_INT_FAIL, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z );

		aLogItem.Action	= "L";
		g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );

		return 2;
	}
	else	// 己傍
	{
		pUser->AddPlaySound( SND_INF_UPGRADESUCCESS );
			
		if( pUser->IsMode( TRANSPARENT_MODE ) == 0)
			g_UserMng.AddCreateSfxObj( (CMover *)pUser, XI_INT_SUCCESS, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z);

		pUser->UpdateItem( (BYTE)pItemMain->m_dwObjId, UI_AO,  pItemMain->GetAbilityOption()+1 );

		aLogItem.Action		= "H";
		g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );

		return 1;
	}
	return 0;
}

BYTE	CItemUpgrade::SmeltSafetyPiercingSize(CUser* pUser, CItemElem* pItemMain, CItemElem* pItemMaterial, CItemElem* pItemProtScr )
{
	// 犁丰啊 巩胶沛牢啊?
	if( pItemMaterial->GetProp()->dwID != II_GEN_MAT_MOONSTONE && pItemMaterial->GetProp()->dwID != II_GEN_MAT_MOONSTONE_1 )
	{
		//pUser->AddDefinedText( TID_GAME_NOTEQUALITEM );
		return 0;
	}
	//	乔绢教 焊龋狼 滴风付府牢啊
	if( pItemProtScr->m_dwItemId != II_SYS_SYS_SCR_PIEPROT )
	{
		//pUser->AddDefinedText( TID_SBEVE_NOTUSEITEM );			// 惑侩酒捞袍捞 酒聪搁 阂啊瓷
		return 0;
	}

	LogItemInfo aLogItem;
	aLogItem.SendName = pUser->GetName();
	aLogItem.RecvName = "PIERCING_SMELTSAFETY";
	aLogItem.WorldId = pUser->GetWorld()->GetID();

	int nCost = 100000;

	if( 0 < nCost )
	{
		if( pUser->GetGold() < nCost )
		{
			pUser->AddDefinedText( TID_GAME_LACKMONEY );
			return 0;
		}

		pUser->AddGold( -( nCost ) );

		aLogItem.Gold = pUser->GetGold() + nCost;
		aLogItem.Gold2 = pUser->GetGold();
		aLogItem.Action = "!";
		//aLogItem.ItemName = "SEED";
		_stprintf( aLogItem.szItemName, "%d", II_GOLD_SEED1 );
		aLogItem.itemNumber = nCost;
		g_DPSrvr.OnLogItem( aLogItem );
	}
	else
	{
		return 0;
	}
	aLogItem.Gold = aLogItem.Gold2 = pUser->GetGold();

	// 犁丰客 滴风付府 昏力.
	aLogItem.Action = "!";
	g_DPSrvr.OnLogItem( aLogItem, pItemMaterial, pItemMaterial->m_nItemNum );
	pUser->RemoveItem( (BYTE)( pItemMaterial->m_dwObjId ), 1 );
	//	乔绢教 焊龋狼 滴风付府 肺弊 巢辨巴
	ItemProp* pItemProp = pItemProtScr->GetProp();
	if( pItemProp )
	{
		g_dpDBClient.SendLogSMItemUse( "1", pUser, pItemProtScr, pItemProp );
		g_dpDBClient.SendLogSMItemUse( "2", pUser, NULL, pItemProp );
	}
	pUser->RemoveItem( (BYTE)( pItemProtScr->m_dwObjId ), 1 );
	
	int nPercent = GetSizeProb( pItemMain );

	if( nPercent < (int)( xRandom( 10000 ) ) )
	{	// 角菩
		//pUser->AddDefinedText( TID_MMI_PIERCINGFAIL );
		pUser->AddPlaySound( SND_INF_UPGRADEFAIL );
		if( pUser->IsMode( TRANSPARENT_MODE ) == 0)
			g_UserMng.AddCreateSfxObj( (CMover *)pUser, XI_INT_FAIL, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z);
		
		aLogItem.Action = "!";
		g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );

		return 2;
	}
	else
	{	// 己傍
		//pUser->AddDefinedText( TID_MMI_PIERCINGSUCCESS );
		pUser->AddPlaySound( SND_INF_UPGRADESUCCESS );
		if( pUser->IsMode( TRANSPARENT_MODE ) == 0)
			g_UserMng.AddCreateSfxObj( (CMover *)pUser, XI_INT_SUCCESS, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z );
		pUser->UpdateItem( (BYTE)pItemMain->m_dwObjId, UI_PIERCING_SIZE, pItemMain->GetPiercingSize() + 1 );

		aLogItem.Action = "#";
		g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );

		return 1;
	}
	return 0;
}

void	CItemUpgrade::RefineAccessory( CUser* pUser, CItemElem* pItemMain, CItemElem* pItemMaterial )
{
	// 犁丰啊 巩胶沛牢啊?
	if( pItemMaterial->GetProp()->dwID != II_GEN_MAT_MOONSTONE && pItemMaterial->GetProp()->dwID != II_GEN_MAT_MOONSTONE_1 )
	{
		pUser->AddDefinedText( TID_GAME_NOTEQUALITEM );
		return;
	}
	if( pItemMain->GetAbilityOption() >= MAX_AAO )	// 20
	{
		pUser->AddDefinedText( TID_GAME_ACCESSORY_MAX_AAO );
		return;
	}

	// log
	LogItemInfo aLogItem;
	aLogItem.SendName	= pUser->GetName();
	aLogItem.RecvName	= "UPGRADEITEM";
	aLogItem.WorldId	= pUser->GetWorld()->GetID();
	aLogItem.Gold	= pUser->GetGold();
	aLogItem.Gold2	= pUser->GetGold();

	DWORD dwProbability		= CAccessoryProperty::GetInstance()->GetProbability( pItemMain->GetAbilityOption() );
	// 咀技辑府 焊龋狼 滴风付府
	BOOL bSmelprot	= FALSE;
	if( pUser->HasBuff( BUFF_ITEM, II_SYS_SYS_SCR_SMELPROT4 ) )
	{
		bSmelprot	= TRUE;
		pUser->RemoveBuff( BUFF_ITEM, II_SYS_SYS_SCR_SMELPROT4 );
		ItemProp* pItemProp = prj.GetItemProp( II_SYS_SYS_SCR_SMELPROT4 );
		if( pItemProp )
			g_dpDBClient.SendLogSMItemUse( "2", pUser, NULL, pItemProp );
	}

	if( xRandom( 10000 ) < dwProbability )	// 己傍
	{
		pUser->AddDefinedText( TID_UPGRADE_SUCCEEFUL );
		pUser->AddPlaySound( SND_INF_UPGRADESUCCESS );
			
		if( pUser->IsMode( TRANSPARENT_MODE ) == 0)
			g_UserMng.AddCreateSfxObj( (CMover *)pUser, XI_INT_SUCCESS, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z);

		pUser->UpdateItem( (BYTE)pItemMain->m_dwObjId, UI_AO,  pItemMain->GetAbilityOption()+1 );

		aLogItem.Action		= "H";
		g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );
	}
	else	// 角菩
	{
		pUser->AddDefinedText( TID_UPGRADE_FAIL );
		pUser->AddPlaySound( SND_INF_UPGRADEFAIL );
		if( pUser->IsMode( TRANSPARENT_MODE ) == 0 )
			g_UserMng.AddCreateSfxObj( (CMover *)pUser, XI_INT_FAIL, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z );

		if( !bSmelprot )
		{
			if( pItemMain->GetAbilityOption() >= 3 )		// 昏力
			{
				aLogItem.Action	= "L";
				g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );
				pUser->UpdateItem( (BYTE)pItemMain->m_dwObjId, UI_NUM, 0 );
			}
		}
	}
	aLogItem.Action	= "N";
	g_DPSrvr.OnLogItem( aLogItem, pItemMaterial, pItemMaterial->m_nItemNum );
	pUser->UpdateItem( (BYTE)pItemMaterial->m_dwObjId, UI_NUM, pItemMaterial->m_nItemNum - 1 );
}

void	CItemUpgrade::RefineCollector( CUser* pUser, CItemElem* pItemMain, CItemElem* pItemMaterial )
{
	// 犁丰啊 巩胶沛牢啊?
	if( pItemMaterial->GetProp()->dwID != II_GEN_MAT_MOONSTONE && pItemMaterial->GetProp()->dwID != II_GEN_MAT_MOONSTONE_1 )
	{
		pUser->AddDefinedText( TID_GAME_NOTEQUALITEM );
		return;
	}

	CCollectingProperty* pProperty	= CCollectingProperty::GetInstance();
	if( pItemMain->GetAbilityOption() >= pProperty->GetMaxCollectorLevel() )
	{
		pUser->AddDefinedText( TID_GAME_MAX_COLLECTOR_LEVEL );
		return;
	}
	
	int nProb	= pProperty->GetEnchantProbability( pItemMain->GetAbilityOption() );
	if( nProb == 0 )
		return;

	// log
	LogItemInfo aLogItem;
	aLogItem.SendName	= pUser->GetName();
	aLogItem.RecvName	= "UPGRADEITEM";
	aLogItem.WorldId	= pUser->GetWorld()->GetID();
	aLogItem.Gold	= pUser->GetGold();
	aLogItem.Gold2	= pUser->GetGold();

	DWORD dwRand	= xRandom( 1000 );	// 0 - 999
	if( (int)( dwRand ) < nProb )
	{
		pUser->AddDefinedText( TID_UPGRADE_SUCCEEFUL );
		pUser->AddPlaySound( SND_INF_UPGRADESUCCESS );
		if( pUser->IsMode( TRANSPARENT_MODE ) == 0 )
			g_UserMng.AddCreateSfxObj( (CMover *)pUser, XI_INT_SUCCESS, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z );
		pUser->UpdateItem( (BYTE)pItemMain->m_dwObjId, UI_AO,  pItemMain->GetAbilityOption()+1 );
		aLogItem.Action		= "H";
		g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );
	}
	else
	{
		pUser->AddDefinedText( TID_UPGRADE_FAIL );
		pUser->AddPlaySound( SND_INF_UPGRADEFAIL );
		if( pUser->IsMode( TRANSPARENT_MODE ) == 0 )
			g_UserMng.AddCreateSfxObj( (CMover *)pUser, XI_INT_FAIL, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z );
		// log
	}
	aLogItem.Action	= "N";
	g_DPSrvr.OnLogItem( aLogItem, pItemMaterial, pItemMaterial->m_nItemNum );
	pUser->UpdateItem( (BYTE)pItemMaterial->m_dwObjId, UI_NUM, pItemMaterial->m_nItemNum - 1 );
}

#if __VER >= 15 // __15_5TH_ELEMENTAL_SMELT_SAFETY
BYTE	CItemUpgrade::SmeltSafetyAttribute(CUser* pUser, CItemElem* pItemMain, CItemElem* pItemMaterial, CItemElem* pItemProtScr, CItemElem* pItemSmeltScr )
{
	// 加己 力访捞 啊瓷茄啊
	if( !CItemElem::IsEleRefineryAble( pItemMain->GetProp() ) )
		return 0;

	// 加己篮 茄啊瘤父
	if( pItemMain->m_bItemResist != SAI79::NO_PROP )
	{
		if( pItemMain->m_bItemResist != pItemMaterial->GetProp()->eItemType )
			return 0;
	}

	// 加己 寸 窍唱狼 加己 力访 墨靛甫 荤侩窍档废 荐沥
	if( pItemMaterial->GetProp()->dwID != WhatEleCard( pItemMaterial->GetProp()->eItemType ) )
		return 0;

	// 弥措 蔼阑 逞绰 版快 吝窜
	if( pItemMain->m_nResistAbilityOption >= GetMaxAttributeEnchantSize() )
		return 3;
	
	// 10000窜困狼 己傍 欺季飘
	int nPercent = GetAttributeEnchantProb( pItemMain->m_nResistAbilityOption );

	//	加己 力访狼 滴风付府甫 荤侩沁促搁
	if( pItemSmeltScr != NULL )
	{
		//	加己 力访狼 滴风付府啊 嘎绰瘤 犬牢
		if( IsUsableItem( pItemSmeltScr ) && pItemSmeltScr->GetProp()->dwID == II_SYS_SYS_SCR_SMELTING2 )
		{
			//	加己 力访狼 滴风付府 荤侩啊瓷 荐摹牢啊
			if( pItemMain->m_nResistAbilityOption < 10 )
			{
				nPercent	+= 1000;
				//	加己 力访狼 滴风付府 肺弊 巢辨巴
				ItemProp* pItemProp = pItemSmeltScr->GetProp();
				if( pItemProp )
				{
					g_dpDBClient.SendLogSMItemUse( "1", pUser, pItemSmeltScr, pItemProp );
					g_dpDBClient.SendLogSMItemUse( "2", pUser, NULL, pItemProp );
				}
				pUser->RemoveItem( (BYTE)( pItemSmeltScr->m_dwObjId ), 1 );
			}
		}
		//	加己 力访狼 滴风付府啊 酒匆锭
		else
			return 0;
	}

	LogItemInfo aLogItem;
	aLogItem.Action = "N";
	aLogItem.SendName = pUser->GetName();
	aLogItem.RecvName = "UPGRADEITEM_SMELTSAFETY";
	aLogItem.WorldId = pUser->GetWorld()->GetID();
	aLogItem.Gold = pUser->GetGold();
	aLogItem.Gold2 = pUser->GetGold();

	g_DPSrvr.OnLogItem( aLogItem, pItemMaterial, pItemMaterial->m_nItemNum );
	DWORD dwValue = pItemMaterial->GetProp()->eItemType;
	pUser->RemoveItem( (BYTE)( pItemMaterial->m_dwObjId ), (short)1 );

	ItemProp* pItemProp = pItemProtScr->GetProp();
	if( pItemProp )
	{
		g_dpDBClient.SendLogSMItemUse( "1", pUser, pItemProtScr, pItemProp );
		g_dpDBClient.SendLogSMItemUse( "2", pUser, NULL, pItemProp );
	}
	pUser->RemoveItem( (BYTE)( pItemProtScr->m_dwObjId ), 1 );

	// 秦寸 酒捞袍狼 加己, 老馆 饭骇阑 掘绢 犬啦阑 波辰促.
	if( (int)( xRandom( 10000 ) ) > nPercent )
	{
		// 角菩
		pUser->AddPlaySound( SND_INF_UPGRADEFAIL );

		if( !pUser->IsMode( TRANSPARENT_MODE ) )
			g_UserMng.AddCreateSfxObj( (CMover *)pUser, XI_INT_FAIL, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z );

		aLogItem.Action = "J";
		g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );

		return 2;
	}
	else
	{
		// 己傍
		pUser->AddPlaySound( SND_INF_UPGRADESUCCESS );

		if( !pUser->IsMode( TRANSPARENT_MODE ) )
			g_UserMng.AddCreateSfxObj( (CMover *)pUser, XI_INT_SUCCESS, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z );

		pUser->UpdateItem( (BYTE)pItemMain->m_dwObjId, UI_IR, dwValue );
		pUser->UpdateItem( (BYTE)pItemMain->m_dwObjId, UI_RAO,  pItemMain->m_nResistAbilityOption + 1 );
		aLogItem.Action = "O";
		g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );

		return 1;
	}
	return 0;
}
#endif // __15_5TH_ELEMENTAL_SMELT_SAFETY
#endif // __SMELT_SAFETY

void	CItemUpgrade::EnchantGeneral( CUser* pUser, CItemElem* pItemMain, CItemElem* pItemMaterial )
{	
	int*  pAbilityOption = pItemMain->GetAbilityOptionPtr();
	if( pAbilityOption == NULL )
		return;

	// 老馆力访篮 规绢备, 公扁
#if __VER >= 9 // __ULTIMATE
	if( pItemMain->GetProp()->dwReferStat1 == WEAPON_ULTIMATE )
	{
		pUser->AddDefinedText( TID_GAME_NOTEQUALITEM );
		return;
	}

	if( pItemMaterial->GetProp()->dwID != II_GEN_MAT_ORICHALCUM01
		&& pItemMaterial->GetProp()->dwID != II_GEN_MAT_ORICHALCUM01_1 )
	{
		pUser->AddDefinedText( TID_GAME_NOTEQUALITEM );
		return;
	}
#endif // __ULTIMATE

	if( !CItemElem::IsDiceRefineryAble(pItemMain->GetProp()) )
	{
		pUser->AddDefinedText( TID_GAME_NOTEQUALITEM );			
		return;								
	}

	if( *pAbilityOption >= GetMaxGeneralEnchantSize() )
	{
		pUser->AddDefinedText( TID_UPGRADE_MAXOVER );			
		return;
	}

	// 1000窜困狼 己傍 欺季飘 
	int nPercent = GetGeneralEnchantProb( *pAbilityOption );

	LogItemInfo aLogItem;
	aLogItem.SendName = pUser->GetName();
	aLogItem.RecvName = "UPGRADEITEM";
	aLogItem.WorldId = pUser->GetWorld()->GetID();
	aLogItem.Gold = pUser->GetGold();
	aLogItem.Gold2 = pUser->GetGold();

	BOOL bSmelprot	= FALSE;
	if( pUser->HasBuff( BUFF_ITEM, II_SYS_SYS_SCR_SMELPROT ) )
	{
		bSmelprot	= TRUE;
		pUser->RemoveBuff( BUFF_ITEM, II_SYS_SYS_SCR_SMELPROT );

		ItemProp* pItemProp = prj.GetItemProp( II_SYS_SYS_SCR_SMELPROT );
		if( pItemProp )
			g_dpDBClient.SendLogSMItemUse( "2", pUser, NULL, pItemProp );
	}

	if( pUser->HasBuff( BUFF_ITEM, II_SYS_SYS_SCR_SMELTING ) )
	{
		if( *pAbilityOption < 7 )
		{
			nPercent	+= 1000;
			pUser->RemoveBuff( BUFF_ITEM, II_SYS_SYS_SCR_SMELTING );

			ItemProp* pItemProp = prj.GetItemProp( II_SYS_SYS_SCR_SMELTING );
			if( pItemProp )
				g_dpDBClient.SendLogSMItemUse( "2", pUser, NULL, pItemProp );
		}
	}
#ifdef __SM_ITEM_2ND_EX
	BOOL bSmelprot2	= FALSE;
	if( !bSmelprot && pUser->HasBuff( BUFF_ITEM, II_SYS_SYS_SCR_SMELPROT2 ) )
	{
		bSmelprot2	= TRUE;
		pUser->RemoveBuff( BUFF_ITEM, II_SYS_SYS_SCR_SMELPROT2 );
	}
#endif	// __SM_ITEM_2ND_EX

	if( pUser->HasBuff( BUFF_ITEM, II_SYS_SYS_SCR_SUPERSMELTING ) )
	{
		if( *pAbilityOption < 7 )
		{
			nPercent	+= 10000;
			pUser->RemoveBuff( BUFF_ITEM, II_SYS_SYS_SCR_SUPERSMELTING );

			ItemProp* pItemProp = prj.GetItemProp( II_SYS_SYS_SCR_SUPERSMELTING );
			if( pItemProp )
				g_dpDBClient.SendLogSMItemUse( "2", pUser, NULL, pItemProp );
		}				
	}

	// 秦寸 酒捞袍狼 加己, 老馆 饭骇阑 掘绢 犬啦阑 波辰促.
	if( (int)( xRandom( 10000 ) ) > nPercent )
	{
		// 角菩 皋技瘤 免仿
		pUser->AddDefinedText( TID_UPGRADE_FAIL );
		pUser->AddPlaySound( SND_INF_UPGRADEFAIL );

		if((pUser->IsMode( TRANSPARENT_MODE ) ) == 0)
			g_UserMng.AddCreateSfxObj((CMover *)pUser, XI_INT_FAIL, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z);

		// 角菩窍搁 3捞惑捞搁 酒捞袍 昏力
		if( *pAbilityOption >= 3 )
		{
			if( !bSmelprot )
			{	// 荤侩救窍搁 甸绢咳.. 措惑 酒捞袍 昏力 角菩 肺弊
#ifdef __SM_ITEM_2ND_EX
				if( bSmelprot2 )
				{
					pUser->UpdateItem( (BYTE)pItemMain->m_dwObjId, UI_AO,  pItemMain->GetAbilityOption() - 1 );
					aLogItem.Action = "9";
					g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );
				}
				else
#endif	// __SM_ITEM_2ND_EX
				{
					aLogItem.Action = "L";
					g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );
					pUser->RemoveItem( (BYTE)( pItemMain->m_dwObjId ), (short)1 );
				}
			}
			else
			{
				aLogItem.Action = "F";
				g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );
			}
		}
	}
	else
	{
		// 己傍
		pUser->AddDefinedText( TID_UPGRADE_SUCCEEFUL );
		pUser->AddPlaySound( SND_INF_UPGRADESUCCESS );

		if((pUser->IsMode( TRANSPARENT_MODE ) ) == 0)
			g_UserMng.AddCreateSfxObj((CMover *)pUser, XI_INT_SUCCESS, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z);

		pUser->UpdateItem( (BYTE)pItemMain->m_dwObjId, UI_AO,  pItemMain->GetAbilityOption()+1 );
		aLogItem.Action = "H";
		g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );
	}

	aLogItem.Action = "N";
	g_DPSrvr.OnLogItem( aLogItem, pItemMaterial, pItemMaterial->m_nItemNum );
	// 力访酒袍 昏力 - 己傍捞带, 角菩带...
	pUser->RemoveItem( (BYTE)( pItemMaterial->m_dwObjId ), (short)1 );
}

int		CItemUpgrade::GetGeneralEnchantProb( int nAbilityOption )
{
	int nProb = 0;
	
	map<int, int>::iterator it = m_mapGeneralEnchant.find( nAbilityOption + 1 );
	if( it != m_mapGeneralEnchant.end() )
		nProb = it->second;

	if( ::GetLanguage() != LANG_KOR && nAbilityOption >= 3 )	// 力访 4何磐 10% 犬伏 皑家(秦寇父)
		nProb = static_cast<int>(static_cast<float>(nProb) * 0.9f);
	
	return nProb;
}

void	CItemUpgrade::EnchantAttribute( CUser* pUser, CItemElem* pItemMain, CItemElem* pItemMaterial )
{
	int*  pAbilityOption = &(pItemMain->m_nResistAbilityOption);
	if( pAbilityOption == NULL )
		return;

	// 2啊瘤 加己篮 力访且荐 绝澜
	if( pItemMain->m_bItemResist != SAI79::NO_PROP )
	{
		if( pItemMain->m_bItemResist != pItemMaterial->GetProp()->eItemType )
		{
			pUser->AddDefinedText( TID_UPGRADE_ERROR_TWOELEMENT );								
			return;
		}
	}

	if( !CItemElem::IsEleRefineryAble(pItemMain->GetProp()) )
	{
		pUser->AddDefinedText( TID_GAME_NOTEQUALITEM );			
		return;								
	}

#if __VER >= 12 // __J12_0
	// 加己 寸 窍唱狼 加己 力访 墨靛甫 荤侩窍档废 荐沥
	DWORD dwReqCard	= WhatEleCard( pItemMaterial->GetProp()->eItemType );
#else	// __J12_0
	DWORD dwReqCard = WhatEleCard( *pAbilityOption, pItemMaterial->GetProp()->eItemType );
#endif	// __J12_0

	if( pItemMaterial->GetProp()->dwID != dwReqCard )
	{
		pUser->AddDefinedText( TID_UPGRADE_ERROR_WRONGUPLEVEL );			
		return;					
	}

	// 弥措 蔼阑 逞绰 版快 吝窜
	if( *pAbilityOption >= GetMaxAttributeEnchantSize() )
	{
		pUser->AddDefinedText( TID_UPGRADE_MAXOVER );			
		return;
	}
	// 10000窜困狼 己傍 欺季飘 
	int nPercent = GetAttributeEnchantProb( *pAbilityOption );

	LogItemInfo aLogItem;
	aLogItem.SendName = pUser->GetName();
	aLogItem.RecvName = "UPGRADEITEM";
	aLogItem.WorldId = pUser->GetWorld()->GetID();
	aLogItem.Gold = pUser->GetGold();
	aLogItem.Gold2 = pUser->GetGold();


	BOOL bSmelprot	= FALSE;
	if( pUser->HasBuff( BUFF_ITEM, II_SYS_SYS_SCR_SMELPROT ) )
	{
		bSmelprot	= TRUE;
		pUser->RemoveBuff( BUFF_ITEM, II_SYS_SYS_SCR_SMELPROT );

		ItemProp* pItemProp = prj.GetItemProp( II_SYS_SYS_SCR_SMELPROT );
		if( pItemProp )
			g_dpDBClient.SendLogSMItemUse( "2", pUser, NULL, pItemProp );
	}
	
#if __VER >= 14 // __EXT_ATTRIBUTE
	if( pUser->HasBuff( BUFF_ITEM, II_SYS_SYS_SCR_SMELTING2 ) )	// 加己 力访狼 滴风付府
#else // __EXT_ATTRIBUTE
	if( pUser->HasBuff( BUFF_ITEM, II_SYS_SYS_SCR_SMELTING ) ) // 力访狼 滴风付府
#endif // __EXT_ATTRIBUTE
	{
#if __VER >= 14 // __EXT_ATTRIBUTE
		if( *pAbilityOption < 10 )
#else // __EXT_ATTRIBUTE
		if( *pAbilityOption < 7 )
#endif // __EXT_ATTRIBUTE
		{
			nPercent	+= 1000;
#if __VER >= 14 // __EXT_ATTRIBUTE
			pUser->RemoveBuff( BUFF_ITEM, II_SYS_SYS_SCR_SMELTING2 );
			ItemProp* pItemProp = prj.GetItemProp( II_SYS_SYS_SCR_SMELTING2 );
#else // __EXT_ATTRIBUTE
			pUser->RemoveBuff( BUFF_ITEM, II_SYS_SYS_SCR_SMELTING );
			ItemProp* pItemProp = prj.GetItemProp( II_SYS_SYS_SCR_SMELTING );
#endif // __EXT_ATTRIBUTE
			if( pItemProp )
				g_dpDBClient.SendLogSMItemUse( "2", pUser, NULL, pItemProp );
		}
	}
#ifdef __SM_ITEM_2ND_EX
	BOOL bSmelprot2	= FALSE;
	if( !bSmelprot && pUser->HasBuff( BUFF_ITEM, II_SYS_SYS_SCR_SMELPROT2 ) )
	{
		bSmelprot2	= TRUE;
		pUser->RemoveBuff( BUFF_ITEM, II_SYS_SYS_SCR_SMELPROT2 );
	}
#endif	// __SM_ITEM_2ND_EX

	if( pUser->HasBuff( BUFF_ITEM, II_SYS_SYS_SCR_SUPERSMELTING ) )
	{
		if( *pAbilityOption < 7 )
		{
			nPercent	+= 10000;
			pUser->RemoveBuff( BUFF_ITEM, II_SYS_SYS_SCR_SUPERSMELTING );

			ItemProp* pItemProp = prj.GetItemProp( II_SYS_SYS_SCR_SUPERSMELTING );
			if( pItemProp )
				g_dpDBClient.SendLogSMItemUse( "2", pUser, NULL, pItemProp );
		}				
	}

	// 秦寸 酒捞袍狼 加己, 老馆 饭骇阑 掘绢 犬啦阑 波辰促.
	if( (int)( xRandom( 10000 ) ) > nPercent )
	{
		// 角菩 皋技瘤 免仿
		pUser->AddDefinedText( TID_UPGRADE_FAIL );
		pUser->AddPlaySound( SND_INF_UPGRADEFAIL );

		if((pUser->IsMode( TRANSPARENT_MODE ) ) == 0)
			g_UserMng.AddCreateSfxObj((CMover *)pUser, XI_INT_FAIL, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z);

		// 角菩窍搁 nLevDown捞惑捞搁 酒捞袍 昏力
		if( *pAbilityOption >= 3 )
		{
			if( !bSmelprot )
			{	// 荤侩救窍搁 甸绢咳.. 措惑 酒捞袍 昏力 角菩 肺弊
#ifdef __SM_ITEM_2ND_EX
				if( bSmelprot2  )
				{
					pUser->UpdateItem( (BYTE)pItemMain->m_dwObjId, UI_RAO, pItemMain->m_nResistAbilityOption - 1 );
					aLogItem.Action = "8";
					g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );
				}
				else
#endif	// __SM_ITEM_2ND_EX
				{
					aLogItem.Action = "L";
					g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );
					pUser->RemoveItem( (BYTE)( pItemMain->m_dwObjId ), (short)1 );
				}
			}
		}
		else
		{	// 荤侩阑 窍搁 角菩 肺弊
			aLogItem.Action = "J";
			g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );
		}
	}
	else
	{
		// 己傍
		pUser->AddDefinedText( TID_UPGRADE_SUCCEEFUL );
		pUser->AddPlaySound( SND_INF_UPGRADESUCCESS );

		if((pUser->IsMode( TRANSPARENT_MODE ) ) == 0)
			g_UserMng.AddCreateSfxObj((CMover *)pUser, XI_INT_SUCCESS, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z);

		pUser->UpdateItem( (BYTE)pItemMain->m_dwObjId, UI_IR,  pItemMaterial->GetProp()->eItemType );
		pUser->UpdateItem( (BYTE)pItemMain->m_dwObjId, UI_RAO,  pItemMain->m_nResistAbilityOption+1 );
		aLogItem.Action = "O";
		g_DPSrvr.OnLogItem( aLogItem, pItemMain, pItemMain->m_nItemNum );
	}

	aLogItem.Action = "N";
	g_DPSrvr.OnLogItem( aLogItem, pItemMaterial, pItemMaterial->m_nItemNum );
	// 力访酒袍 昏力 - 己傍捞带, 角菩带...
	pUser->RemoveItem( (BYTE)( pItemMaterial->m_dwObjId ), (short)1 );
}

void CItemUpgrade::ChangeAttribute( CUser* pUser, OBJID dwTargetItem, OBJID dwUseItem , SAI79::ePropType nAttribute )
{
	CItemElem* pTargetItemElem	= pUser->m_Inventory.GetAtId( dwTargetItem );
	CItemElem* pUseItemElem = pUser->m_Inventory.GetAtId( dwUseItem );		

	if( pUser->m_vtInfo.GetOther() )	// 芭贰吝牢 措惑捞 乐栏搁?
		return;
	if( pUser->m_vtInfo.VendorIsVendor() )		// 郴啊 迫绊 乐栏搁?
		return;

#if __VER >= 11 // __SYS_COLLECTING
#if __VER < 14 // __SMELT_SAFETY
	if( pUser->PreRefine( dwTargetItem, dwUseItem ) )
		return;
#endif // __SMELT_SAFETY
#endif	// __SYS_COLLECTING
	if( !IsUsableItem( pTargetItemElem ) || !IsUsableItem( pUseItemElem ) )
		return;

	// 措惑捞 厘馒吝牢啊?
	if( pUser->m_Inventory.IsEquip( dwTargetItem ) )
	{
		pUser->AddDefinedText( TID_GAME_EQUIPPUT , "" );
		return;
	}
	
	if( !CItemElem::IsEleRefineryAble( pTargetItemElem->GetProp() ) )	// 加己力访 啊瓷茄 酒捞袍捞 酒聪夸..
		return;

	if( pUseItemElem->m_dwItemId != II_SYS_SYS_SCR_SOKCHANG )	// 加己函版 酒捞袍捞 酒聪匙...
		return;

	if( nAttribute >= SAI79::END_PROP || nAttribute <= SAI79::NO_PROP )
		return;

	if( pTargetItemElem->m_bItemResist == nAttribute )	// 鞍篮 加己牢 版快 加己函版 阂啊!!
	{
		pUser->AddDefinedText( TID_GAME_NOCHANGE_SAME_ATTRIBUTE );
		return;
	}
	
	if( (pTargetItemElem->m_bItemResist != SAI79::NO_PROP) && (pTargetItemElem->m_nResistAbilityOption > 0) )
	{
		pUser->AddPlaySound( SND_INF_UPGRADESUCCESS );
		if( ( pUser->IsMode( TRANSPARENT_MODE ) ) == 0 )
			g_UserMng.AddCreateSfxObj((CMover *)pUser, XI_INT_SUCCESS, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z);
		pUser->UpdateItem( (BYTE)pTargetItemElem->m_dwObjId, UI_IR,  nAttribute );	// 加己 函版

		// 加己力访 函版 己傍 肺弊
		LogItemInfo aLogItem;
		aLogItem.SendName = pUser->GetName();
		aLogItem.RecvName = "CHANGE_ATTRIBUTE_TARGET";
		aLogItem.WorldId = pUser->GetWorld()->GetID();
		aLogItem.Gold = pUser->GetGold();
		aLogItem.Gold2 = pUser->GetGold();
		aLogItem.Action = "O";
		g_DPSrvr.OnLogItem( aLogItem, pTargetItemElem );
		
		aLogItem.RecvName = "CHANGE_ATTRIBUTE_MATERIAL";
		g_DPSrvr.OnLogItem( aLogItem, pUseItemElem );
		pUser->RemoveItem( (BYTE)( dwUseItem ), 1 );	// 蜡丰 酒捞袍 昏力
	}
	else
		pUser->AddDefinedText( TID_GAME_NOTELEMENT );
}

int CItemUpgrade::GetAttributeEnchantProb( int nAbilityOption )
{
	map<int, __ATTRIBUTE_ENCHANT>::iterator it = m_mapAttributeEnchant.find( nAbilityOption + 1 );
	if( it != m_mapAttributeEnchant.end() )
		return it->second.nProb;

	return 0;
}

int CItemUpgrade::GetAttributeDamageFactor( int nAbilityOption )
{
	if( nAbilityOption > GetMaxAttributeEnchantSize() )
		nAbilityOption = GetMaxAttributeEnchantSize();

	map<int, __ATTRIBUTE_ENCHANT>::iterator it = m_mapAttributeEnchant.find( nAbilityOption );
	if( it != m_mapAttributeEnchant.end() )
		return it->second.nAddDamageRate;

	return 0;
}

#if __VER >= 14 // __EXT_ATTRIBUTE
int CItemUpgrade::GetAttributeDefenseFactor( int nAbilityOption )
{
	if( nAbilityOption > GetMaxAttributeEnchantSize() )
		nAbilityOption = GetMaxAttributeEnchantSize();

	map<int, __ATTRIBUTE_ENCHANT>::iterator it = m_mapAttributeEnchant.find( nAbilityOption );
	if( it != m_mapAttributeEnchant.end() )
		return it->second.nDefenseRate;

	return 0;
}

int CItemUpgrade::GetAttributeAddAtkDmgFactor( int nAbilityOption )
{
	if( nAbilityOption > GetMaxAttributeEnchantSize() )
		nAbilityOption = GetMaxAttributeEnchantSize();

	map<int, __ATTRIBUTE_ENCHANT>::iterator it = m_mapAttributeEnchant.find( nAbilityOption );
	if( it != m_mapAttributeEnchant.end() )
		return it->second.nAddAtkDmgRate;

	return 0;
}
#endif // __EXT_ATTRIBUTE

DWORD CItemUpgrade::WhatEleCard( DWORD dwItemType )
{	// 加己 力访 侩 墨靛狼 辆幅啊 
	// 加己 寸 窍唱肺 烹钦凳
	switch( dwItemType )
	{
	case SAI79::FIRE:
		return II_GEN_MAT_ELE_FLAME;
	case SAI79::WATER:
		return II_GEN_MAT_ELE_RIVER;
	case SAI79::ELECTRICITY:
		return II_GEN_MAT_ELE_GENERATOR;
	case SAI79::EARTH:
		return II_GEN_MAT_ELE_DESERT;
	case SAI79::WIND:
		return II_GEN_MAT_ELE_CYCLON;
	default:
		return 0;
	}
}
#endif // __EXT_ENCHANT

#ifdef __SYS_ITEMTRANSY
void CItemUpgrade::OnItemTransy( CUser* pUser, OBJID objidTarget, OBJID objidTransy, DWORD dwChangeId, BOOL bCash )
{
	CItemElem* pItemElemTarget = pUser->m_Inventory.GetAtId( objidTarget );	
	if( !IsUsableItem( pItemElemTarget ) )
		return;

	// 厘馒登绢 乐绰 酒捞袍捞搁 府畔( 坷扼捞~~~ せせ )
	if( pUser->m_Inventory.IsEquip( objidTarget ) )
	{
		pUser->AddDefinedText( TID_GAME_EQUIPPUT , "" );
		return;
	}

	if( bCash )
	{
		CItemElem* pItemElemTransy = pUser->m_Inventory.GetAtId( objidTransy );
		if( !IsUsableItem( pItemElemTransy ) )
			return;

		// 犁丰啊 飘罚瘤(ITM)牢瘤 八荤
		if( pItemElemTransy->GetProp()->dwID != II_CHR_SYS_SCR_ITEMTRANSY_A && pItemElemTransy->GetProp()->dwID != II_CHR_SYS_SCR_ITEMTRANSY_B )
			return;
		
		// 饭骇 八荤
		if( pItemElemTransy->GetProp()->dwID == II_CHR_SYS_SCR_ITEMTRANSY_A )
		{
			if( pItemElemTarget->GetProp()->dwLimitLevel1 > 60 )
				return;
		}
		else
		{
			if( pItemElemTarget->GetProp()->dwLimitLevel1 < 61 )
				return;
		}

		if( RunItemTransy( pUser, pItemElemTarget, dwChangeId ) )
		{
			g_dpDBClient.SendLogSMItemUse( "1", pUser, pItemElemTransy, pItemElemTransy->GetProp(), "RemoveItem" );	
			pUser->RemoveItem( (BYTE)( objidTransy ), (short)1 );		// 扁粮 酒捞袍 犁丰 昏力
		}
	}
	else
	{
		//	mulcom	BEGIN100312	其衬肺 酒捞袍 飘坊瘤 陛瘤.
		#ifdef	__ITEMTRANSY_PENYA
			int nPayPenya = 0;
			if( pItemElemTarget->GetProp()->dwLimitLevel1 < 61 )
				nPayPenya = m_nItemTransyLowLevel;
			else
				nPayPenya = m_nItemTransyHighLevel;

			if( pUser->GetGold() < nPayPenya || objidTransy != NULL_ID )
			{
				pUser->AddDefinedText( TID_GAME_LACKMONEY );
				return;
			}

			if( RunItemTransy( pUser, pItemElemTarget, dwChangeId ) )
			{
				pUser->AddGold( -nPayPenya );
				g_DPSrvr.PutPenyaLog( pUser, "O", "TRANSYITEM_PAY", nPayPenya );
			}
		#endif
		//	mulcom	END100312	其衬肺 酒捞袍 飘坊瘤 陛瘤.
	}
}

BOOL CItemUpgrade::RunItemTransy( CUser* pUser, CItemElem* pItemElemTarget, DWORD dwChangeId )
{
	ItemProp* pItemProp = pItemElemTarget->GetProp();
	ItemProp* pItemPropChange = prj.GetItemProp( dwChangeId );
	
	// 函版瞪 酒捞袍狼 炼扒捞 嘎绰瘤 八荤.
	if( !pItemProp || !pItemPropChange || pItemProp->dwID == pItemPropChange->dwID 
		|| ( pItemProp->dwItemKind2 != IK2_ARMOR && pItemProp->dwItemKind2 != IK2_ARMORETC )
		|| ( pItemProp->dwItemSex != SEX_MALE && pItemProp->dwItemSex != SEX_FEMALE )
		|| ( pItemPropChange->dwItemKind2 != IK2_ARMOR && pItemPropChange->dwItemKind2 != IK2_ARMORETC )
		|| ( pItemPropChange->dwItemSex != SEX_MALE && pItemPropChange->dwItemSex != SEX_FEMALE )
		|| pItemProp->dwItemSex == pItemPropChange->dwItemSex
		|| pItemProp->dwItemKind1 != pItemPropChange->dwItemKind1 || pItemProp->dwItemKind2 != pItemPropChange->dwItemKind2
		|| pItemProp->dwItemKind3 != pItemPropChange->dwItemKind3 || pItemProp->dwItemJob != pItemPropChange->dwItemJob 
		|| pItemProp->dwHanded != pItemPropChange->dwHanded || pItemProp->dwParts != pItemPropChange->dwParts  
		|| pItemProp->dwItemLV != pItemPropChange->dwItemLV || pItemProp->dwAbilityMin != pItemPropChange->dwAbilityMin
		|| pItemProp->dwAbilityMax != pItemPropChange->dwAbilityMax || pItemProp->fAttackSpeed != pItemPropChange->fAttackSpeed
		)
		return FALSE;


	// 酒捞袍 飘罚瘤 己傍
	pUser->AddPlaySound( SND_INF_UPGRADESUCCESS );			
	g_UserMng.AddCreateSfxObj((CMover *)pUser, XI_INT_SUCCESS, pUser->GetPos().x, pUser->GetPos().y, pUser->GetPos().z);			

	// 扁粮 酒捞袍狼 Elem 沥焊甫 历厘 窍绊 勒澜...
	CItemElem ItemElemSend;
	ItemElemSend = *pItemElemTarget;
	ItemElemSend.m_dwItemId = pItemPropChange->dwID;
	ItemElemSend.m_nHitPoint	= pItemPropChange->dwEndurance;		// 郴备仿 100%

	g_dpDBClient.SendLogSMItemUse( "1", pUser, pItemElemTarget, pItemElemTarget->GetProp(), "RemoveItem" );	
	g_dpDBClient.SendLogSMItemUse( "1", pUser, &ItemElemSend, ItemElemSend.GetProp(), "CreateItem" );	
	pUser->AddDefinedText( TID_GAME_ITEM_TRANSY_SUCCESS, "\"%s\" \"%s\"", pItemElemTarget->GetProp()->szName, ItemElemSend.GetProp()->szName );

	// 扁粮 酒捞袍 犁丰 昏力
	pUser->RemoveItem( (BYTE)( pItemElemTarget->m_dwObjId ), (short)1 );
	
	// 货肺款 酒捞袍 瘤鞭
	pUser->CreateItem( &ItemElemSend );
	
	return TRUE;
}
#endif // __SYS_ITEMTRANSY

#if __VER >= 15 // __PETVIS
void CItemUpgrade::PetVisSize( CUser* pUser, OBJID objIdMaterial )
{
	if( !IsValidObj( pUser ) )
		return;

	CItemElem* pItemElemPet = pUser->GetVisPetItem();
	CItemElem* pItemElemMaterial = pUser->m_Inventory.GetAtId( objIdMaterial );

	if( !IsUsableItem( pItemElemPet ) || !pItemElemPet->IsVisPet() )
	{
		pUser->AddDefinedText( TID_GAME_BUFFPET_NOSUMMON02 );	
		return;
	}

	if( !IsUsableItem( pItemElemMaterial ) )
		return;

	if( pItemElemMaterial->m_dwItemId != II_SYS_SYS_VIS_KEY01 )
		return;


	if( !pItemElemPet->IsPierceAble( NULL_ID, TRUE ) ) // 厚胶 浇吩 犬厘 啊瓷 八荤
	{
		pUser->AddDefinedText( TID_GAME_BUFFPET_EXPANSION );
		return;
	}
		
	g_DPSrvr.PutItemLog( pUser, "!", "VIS_SLOT_MATERIAL", pItemElemMaterial );
	if( pItemElemMaterial->m_bCharged )
		g_dpDBClient.SendLogSMItemUse( "1", pUser, pItemElemMaterial, pItemElemMaterial->GetProp() );	
	pUser->UpdateItem( (BYTE)( pItemElemPet->m_dwObjId ), UI_PETVIS_SIZE, pItemElemPet->GetPiercingSize() + 1 );
	g_DPSrvr.PutItemLog( pUser, "#", "VIS_SLOT_SIZE", pItemElemPet );
	pUser->RemoveItem( (BYTE)( objIdMaterial ), 1 );
}

void CItemUpgrade::SetPetVisItem( CUser* pUser, OBJID objIdVis )
{
	if( !IsValidObj( pUser ) )
		return;

	CItemElem* pItemElemPet = pUser->GetVisPetItem();
	CItemElem* pItemElemVis = pUser->m_Inventory.GetAtId( objIdVis );

	if( !IsUsableItem( pItemElemPet ) )
	{
		pUser->AddDefinedText( TID_GAME_BUFFPET_NOSUMMON01 );
		return;
	}
		
	if( !IsUsableItem( pItemElemVis ) )
		return;

	ItemProp* pVisProp = prj.GetItemProp( pItemElemVis->m_dwItemId );
	if( !pVisProp )
		return;

	if( !pItemElemPet->IsPierceAble( pVisProp->dwItemKind3 ) )	// 厚胶 厘馒 啊瓷 八荤.
		return;

	int nFirstEmptySlot = NULL_ID;
	for( int i=0; i<pItemElemPet->GetPiercingSize(); i++ )
	{
		DWORD dwVisId = pItemElemPet->GetPiercingItem( i );
		if( dwVisId == pVisProp->dwID )	// 捞固 鞍篮 辆幅狼 厚胶啊 厘馒登绢 乐促.
		{
			pUser->AddDefinedText( TID_GAME_BUFFPET_EQUIPVIS );
			return;
		}

		if( nFirstEmptySlot == NULL_ID && dwVisId == 0 )
			nFirstEmptySlot = i;
	}

	if( nFirstEmptySlot == NULL_ID )	// 厚绢乐绰 厚胶 浇吩捞 绝促.
	{
		pUser->AddDefinedText( TID_GAME_BUFFPET_LACKSLOT );
		return;
	}

	if( pUser->IsSatisfyNeedVis( pItemElemPet, pVisProp ) != SUCCSESS_NEEDVIS )
	{
		pUser->AddDefinedText( TID_GAME_BUFFPET_REQVIS );
		return;
	}

	
	pUser->ResetPetVisDST( pItemElemPet );
	pUser->UpdateItem( (BYTE)( pItemElemPet->m_dwObjId ), UI_PETVIS_ITEM, MAKELONG( nFirstEmptySlot, pItemElemVis->m_dwItemId ), pVisProp->dwAbilityMin );
	PutPetVisItemLog( pUser, "!", "VIS_MATERIAL", pItemElemPet, nFirstEmptySlot );
	if( pItemElemVis->m_bCharged )		// 惑侩拳 酒捞袍 肺弊
		g_dpDBClient.SendLogSMItemUse( "1", pUser, pItemElemVis, pVisProp );		
	g_DPSrvr.PutItemLog( pUser, "$", "VIS_PIERCING", pItemElemPet );
	pUser->RemoveItem( (BYTE)( objIdVis ), 1 );
	pUser->SetPetVisDST( pItemElemPet );
	ChangeVisPetSfx( pUser, pItemElemPet );
}

void CItemUpgrade::RemovePetVisItem( CUser* pUser, int nPosition, BOOL bExpired )
{
	if( !IsValidObj( pUser ) )
		return;

	CItemElem* pItemElemPet = pUser->GetVisPetItem();
	if( !IsUsableItem( pItemElemPet ) )
		return;

	DWORD dwItemId = pItemElemPet->GetPiercingItem( nPosition );
	if( dwItemId  == 0 )	// 捞固 厚绢乐绰 浇吩
		return;

	pUser->ResetPetVisDST( pItemElemPet );
	if( bExpired )
		PutPetVisItemLog( pUser, "$", "VIS_REMOVE_EXPIRED", pItemElemPet, nPosition );
	else
		PutPetVisItemLog( pUser, "$", "VIS_REMOVE_BYUSER", pItemElemPet, nPosition );
	pUser->UpdateItem( (BYTE)( pItemElemPet->m_dwObjId ), UI_PETVIS_ITEM, MAKELONG( nPosition, 0 ), 0 ); // 秦寸 浇吩阑 厚款促.
	ItemProp* pItemProp = prj.GetItemProp( dwItemId );
	if( pItemProp )
		pUser->AddDefinedText( TID_GAME_BUFFPET_REMOVEVIS, "\"%s\"", pItemProp->szName );
	pUser->SetPetVisDST( pItemElemPet );
	ChangeVisPetSfx( pUser, pItemElemPet );
}

void CItemUpgrade::PutPetVisItemLog( CUser* pUser, const char* szAction, const char* szContext, CItemElem* pItem, int nPosition )
{	// 酒捞袍捞 力芭等 捞饶俊 龋免登瘤 臼档废 林狼秦具 窃
	LogItemInfo	log;
	log.Action	=  szAction;
	log.SendName	= pUser->GetName();
	log.RecvName	= szContext;
	log.WorldId		= pUser->GetWorld() ? pUser->GetWorld()->GetID() : WI_WORLD_NONE;	// chipi_090623 荐沥 - 霉 立加矫 父丰等 滚橇牢 版快 岿靛啊 绝绰 惑怕肺 甸绢柯促. 
	log.Gold	= pUser->GetGold();
	log.Gold2	= pItem->GetVisKeepTime( nPosition ) - time_null();
	g_DPSrvr.OnLogItem( log, pItem, pItem->m_nItemNum );
}

void CItemUpgrade::SwapVis( CUser* pUser, int nPos1, int nPos2 )
{
	if( !IsValidObj( pUser ) )
		return;

	CItemElem* pItemElemPet = pUser->GetVisPetItem();
	if( !IsUsableItem( pItemElemPet ) )
		return;
	
	pUser->UpdateItem( (BYTE)( pItemElemPet->m_dwObjId ), UI_PETVIS_ITEMSWAP, MAKELONG( nPos1, nPos2 ) );
}

// 弥绊饭骇 厚胶俊 函拳啊 积变 版快 SFX甫 函拳矫挪促.
void CItemUpgrade::ChangeVisPetSfx( CUser* pUser, CItemElem* pItemElemPet )
{
	CMover* pVisPet = prj.GetMover( pUser->GetEatPetId() );
	if( IsValidObj( pUser ) && IsValidObj( pVisPet ) )
	{
		DWORD dwSfxId = pItemElemPet->GetVisPetSfxId();
		if( pVisPet->m_dwMoverSfxId != dwSfxId )
		{
			pVisPet->m_dwMoverSfxId = dwSfxId;
			g_UserMng.AddChangeMoverSfxId( pVisPet );
		}
	}
}

void CItemUpgrade::TransFormVisPet( CUser* pUser, OBJID objIdMaterial )
{
	CMover* pEatPet = prj.GetMover( pUser->GetEatPetId() );
	if( IsValidObj( pEatPet ) )
	{
		CAIPet* pAI = static_cast<CAIPet*>( pEatPet->m_pAIInterface );
		if( pAI )
		{
			CItemElem* pItemEatPet = pUser->m_Inventory.GetAtId( pAI->GetPetItemId() );
			CItemElem* pItemMaterial = pUser->m_Inventory.GetAtId( objIdMaterial );
			if( IsUsableItem( pItemEatPet ) && IsUsableItem( pItemMaterial ) )
			{
				if( pItemEatPet->IsVisPet() )
				{
					pUser->AddDefinedText( TID_GAME_PET_TRAN_FAILURE );
					return;
				}
								
				if( pItemMaterial->m_bCharged )		// 惑侩拳 酒捞袍 肺弊
					g_dpDBClient.SendLogSMItemUse( "1", pUser, pItemMaterial, pItemMaterial->GetProp() );
				pUser->RemoveItem( (BYTE)( objIdMaterial ), 1 );
				pUser->UpdateItem( (BYTE)( pItemEatPet->m_dwObjId ), UI_TRANSFORM_VISPET, TRUE );
				g_DPSrvr.PutItemLog( pUser, "!", "TRANSFORM_VISPET", pItemEatPet );
				pUser->AddDefinedText( TID_GAME_PET_TRAN_SUCCESS, "\"%s\"", pItemEatPet->GetProp()->szName );
			}
		}
	}
	else
	{
		pUser->AddDefinedText( TID_GAME_PET_TRAN_FAILURE );
	}
}
#endif // __PETVIS