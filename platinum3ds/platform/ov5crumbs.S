@ generated: crumb trampolines for every cross-object call made by overlay005 (field map) code
.syntax unified
.arm
.section .rodata.crumbtags,"a"
.Ltag0: .asciz "ARNG_Next"
.Ltag1: .asciz "ApplicationManager_Args"
.Ltag2: .asciz "AreaDataManager_Alloc"
.Ltag3: .asciz "AreaDataManager_Free"
.Ltag4: .asciz "AreaDataManager_GetAreaLightArchiveID"
.Ltag5: .asciz "AreaDataManager_GetMapPropMaterialShape"
.Ltag6: .asciz "AreaDataManager_GetMapPropModelFile"
.Ltag7: .asciz "AreaDataManager_GetMapPropModelID"
.Ltag8: .asciz "AreaDataManager_GetMapPropTexture"
.Ltag9: .asciz "AreaDataManager_GetMapTexture"
.Ltag10: .asciz "AreaDataManager_HasMapPropModelFile"
.Ltag11: .asciz "AreaDataManager_IsOutdoorsLighting"
.Ltag12: .asciz "AreaDataManager_Load"
.Ltag13: .asciz "AreaLightManager_Free"
.Ltag14: .asciz "AreaLightManager_New"
.Ltag15: .asciz "AreaLightManager_UpdateActiveTemplate"
.Ltag16: .asciz "AreaLight_UseGlobalModelAttributes"
.Ltag17: .asciz "Ascii_ConvertToInt"
.Ltag18: .asciz "Ascii_CopyToTerminator"
.Ltag19: .asciz "BDHC_Free"
.Ltag20: .asciz "BDHC_KillLoad"
.Ltag21: .asciz "BDHC_LazyLoad"
.Ltag22: .asciz "BDHC_Load"
.Ltag23: .asciz "BDHC_MarkNotLoaded"
.Ltag24: .asciz "BDHC_New"
.Ltag25: .asciz "BDHC_Reset"
.Ltag26: .asciz "Bag_CanRemoveItem"
.Ltag27: .asciz "Bag_TryAddItem"
.Ltag28: .asciz "Bag_TryRemoveItem"
.Ltag29: .asciz "BattleFrontierSave_ClearFriendStatsAndSh"
.Ltag30: .asciz "BattleFrontierStats_GetHallRecordStreakI"
.Ltag31: .asciz "BattleHallWinRecords_Get"
.Ltag32: .asciz "BattleHallWinRecords_GetRecordForSpecies"
.Ltag33: .asciz "BerryPatchGraphicsManager_Free"
.Ltag34: .asciz "BerryPatchGraphicsManager_FreeEffectCoun"
.Ltag35: .asciz "BerryPatchGraphicsManager_New"
.Ltag36: .asciz "BerryPatchGraphicsManager_NewEffectCount"
.Ltag37: .asciz "BerryPatchGraphics_FreeGraphics"
.Ltag38: .asciz "BerryPatchGraphics_GetCurrentGraphicsRes"
.Ltag39: .asciz "BerryPatchGraphics_GetGraphicsObject"
.Ltag40: .asciz "BerryPatchGraphics_IsBerryPatch"
.Ltag41: .asciz "BerryPatchGraphics_NewGraphics"
.Ltag42: .asciz "BerryPatchGraphics_PauseGraphics"
.Ltag43: .asciz "BerryPatchGraphics_ResumeGraphics"
.Ltag44: .asciz "BerryPatchGraphics_UpdateGraphics"
.Ltag45: .asciz "BerryPatchManager_Free"
.Ltag46: .asciz "BerryPatchManager_New"
.Ltag47: .asciz "BerryPatches_GetGrowthStage"
.Ltag48: .asciz "BerryPatches_GetMoisture"
.Ltag49: .asciz "BerryPatches_UpdateGrowthStates"
.Ltag50: .asciz "BgConfig_New"
.Ltag51: .asciz "Bg_ChangeTilemapRectPalette"
.Ltag52: .asciz "Bg_ClearTilemap"
.Ltag53: .asciz "Bg_ClearTilesRange"
.Ltag54: .asciz "Bg_CopyTilemapBufferRangeToVRAM"
.Ltag55: .asciz "Bg_CopyTilemapBufferToVRAM"
.Ltag56: .asciz "Bg_FillTilemap"
.Ltag57: .asciz "Bg_FillTilemapRect"
.Ltag58: .asciz "Bg_FillTilesRange"
.Ltag59: .asciz "Bg_FreeTilemapBuffer"
.Ltag60: .asciz "Bg_GetYOffset"
.Ltag61: .asciz "Bg_InitFromTemplate"
.Ltag62: .asciz "Bg_LoadPalette"
.Ltag63: .asciz "Bg_LoadTilemapBuffer"
.Ltag64: .asciz "Bg_LoadTiles"
.Ltag65: .asciz "Bg_LoadToTilemapRect"
.Ltag66: .asciz "Bg_MaskPalette"
.Ltag67: .asciz "Bg_RunScheduledUpdates"
.Ltag68: .asciz "Bg_ScheduleScroll"
.Ltag69: .asciz "Bg_ScheduleTilemapTransfer"
.Ltag70: .asciz "Bg_SetOffset"
.Ltag71: .asciz "Bg_SetPriority"
.Ltag72: .asciz "BillboardGfxSequence_SetData"
.Ltag73: .asciz "BillboardList_Append"
.Ltag74: .asciz "BillboardList_Delete"
.Ltag75: .asciz "BillboardList_IsRedrawing"
.Ltag76: .asciz "BillboardList_New"
.Ltag77: .asciz "BillboardList_ResetRedraw"
.Ltag78: .asciz "BillboardLists_Create"
.Ltag79: .asciz "BillboardLists_Delete"
.Ltag80: .asciz "BillboardLists_Draw"
.Ltag81: .asciz "BillboardResources_SetWithoutKeys"
.Ltag82: .asciz "Billboard_AdvanceAnim"
.Ltag83: .asciz "Billboard_Delete"
.Ltag84: .asciz "Billboard_GetAnimFrameNum"
.Ltag85: .asciz "Billboard_GetAnimNum"
.Ltag86: .asciz "Billboard_GetDrawFlag"
.Ltag87: .asciz "Billboard_GetFrameNum"
.Ltag88: .asciz "Billboard_GetModel"
.Ltag89: .asciz "Billboard_GetPos"
.Ltag90: .asciz "Billboard_GetRotMatrix"
.Ltag91: .asciz "Billboard_GetState"
.Ltag92: .asciz "Billboard_SetAnimFrameNum"
.Ltag93: .asciz "Billboard_SetAnimNum"
.Ltag94: .asciz "Billboard_SetCallback"
.Ltag95: .asciz "Billboard_SetDrawFlag"
.Ltag96: .asciz "Billboard_SetFrameNum"
.Ltag97: .asciz "Billboard_SetPos"
.Ltag98: .asciz "Billboard_SetRotMatrix"
.Ltag99: .asciz "Billboard_SetScale"
.Ltag100: .asciz "Billboard_TryRequestVRAMTransfer"
.Ltag101: .asciz "BlendPalette"
.Ltag102: .asciz "BlendPalettes"
.Ltag103: .asciz "BoxPokemon_Copy"
.Ltag104: .asciz "BoxPokemon_FromPokemon"
.Ltag105: .asciz "BoxPokemon_GetGender"
.Ltag106: .asciz "BoxPokemon_GetLevel"
.Ltag107: .asciz "BoxPokemon_GetValue"
.Ltag108: .asciz "BoxPokemon_Init"
.Ltag109: .asciz "BoxPokemon_SetShayminForm"
.Ltag110: .asciz "BoxPokemon_SetValue"
.Ltag111: .asciz "BrightnessController_IsTransitionComplet"
.Ltag112: .asciz "BrightnessController_SetScreenBrightness"
.Ltag113: .asciz "BrightnessController_StartTransition"
.Ltag114: .asciz "BufferManager_Delete"
.Ltag115: .asciz "BufferManager_GetReadBuffer"
.Ltag116: .asciz "BufferManager_GetWriteBuffer"
.Ltag117: .asciz "BufferManager_New"
.Ltag118: .asciz "BufferManager_SwapBuffers"
.Ltag119: .asciz "CalcCosineDegrees"
.Ltag120: .asciz "CalcLinearFov"
.Ltag121: .asciz "CalcSineDegrees"
.Ltag122: .asciz "CalcTextureDataSizeFromResourceName"
.Ltag123: .asciz "Camera_AdjustFOV"
.Ltag124: .asciz "Camera_Alloc"
.Ltag125: .asciz "Camera_ClearActive"
.Ltag126: .asciz "Camera_ComputeViewMatrix"
.Ltag127: .asciz "Camera_ComputeViewMatrixWithRoll"
.Ltag128: .asciz "Camera_Delete"
.Ltag129: .asciz "Camera_DeleteHistory"
.Ltag130: .asciz "Camera_GetAngle"
.Ltag131: .asciz "Camera_GetDistance"
.Ltag132: .asciz "Camera_GetFOV"
.Ltag133: .asciz "Camera_GetPosition"
.Ltag134: .asciz "Camera_GetTarget"
.Ltag135: .asciz "Camera_InitHistory"
.Ltag136: .asciz "Camera_InitWithTarget"
.Ltag137: .asciz "Camera_ReleaseTarget"
.Ltag138: .asciz "Camera_SetAngleAroundTarget"
.Ltag139: .asciz "Camera_SetAsActive"
.Ltag140: .asciz "Camera_SetClipping"
.Ltag141: .asciz "Camera_SetDistance"
.Ltag142: .asciz "Camera_SetFOV"
.Ltag143: .asciz "CanPokemonFormLearnTM"
.Ltag144: .asciz "CanalaveGym_CheckIfPlayerOnPlatform"
.Ltag145: .asciz "CatchingShow_CheckWildEncounter"
.Ltag146: .asciz "CatchingShow_GetBattleDataTransfer"
.Ltag147: .asciz "CharTransfer_ClearBuffers"
.Ltag148: .asciz "CharTransfer_Free"
.Ltag149: .asciz "CharTransfer_InitWithVramModes"
.Ltag150: .asciz "ChatotCry_ResetStatus"
.Ltag151: .asciz "ClearUnusedSystemCache"
.Ltag152: .asciz "Coins_GetValue"
.Ltag153: .asciz "CommInfo_TrainerInfo"
.Ltag154: .asciz "CommManager_IsInitializedNotPoketch"
.Ltag155: .asciz "CommManager_PauseUnionClient"
.Ltag156: .asciz "CommManager_PauseUnionServer"
.Ltag157: .asciz "CommManager_SetCommError"
.Ltag158: .asciz "CommPlayerMan_ForcePos"
.Ltag159: .asciz "CommPlayer_GetMovementTimer"
.Ltag160: .asciz "CommSys_ConnectedCount"
.Ltag161: .asciz "CommSys_CurNetId"
.Ltag162: .asciz "DaycareMail_GetMail"
.Ltag163: .asciz "DaycareMon_AddSteps"
.Ltag164: .asciz "DaycareMon_CopyToDaycareMon"
.Ltag165: .asciz "DaycareMon_GetBoxMon"
.Ltag166: .asciz "DaycareMon_GetDaycareMail"
.Ltag167: .asciz "DaycareMon_GetSteps"
.Ltag168: .asciz "DaycareMon_Init"
.Ltag169: .asciz "DaycareMon_SetSteps"
.Ltag170: .asciz "Daycare_AreParentLanguagesDifferent"
.Ltag171: .asciz "Daycare_GetDaycareMon"
.Ltag172: .asciz "Daycare_GetOffspringPersonality"
.Ltag173: .asciz "Daycare_GetStepCounter"
.Ltag174: .asciz "Daycare_HasEgg"
.Ltag175: .asciz "Daycare_SetOffspringPersonality"
.Ltag176: .asciz "Daycare_SetStepCounter"
.Ltag177: .asciz "Daycare_Update"
.Ltag178: .asciz "Direction_GetOpposite"
.Ltag179: .asciz "DisableHBlank"
.Ltag180: .asciz "DistWorldFallingBoulder_New"
.Ltag181: .asciz "DistWorldFallingBoulder_Tick"
.Ltag182: .asciz "DistWorldSurfMountRenderer_ClearFlags"
.Ltag183: .asciz "DistWorldSurfMountRenderer_Free"
.Ltag184: .asciz "DistWorldSurfMountRenderer_HandleSurfBeg"
.Ltag185: .asciz "DistWorldSurfMountRenderer_New"
.Ltag186: .asciz "DistWorldSurfMountRenderer_SetFlags"
.Ltag187: .asciz "DistWorld_ApplyGiratinaSpritePalette"
.Ltag188: .asciz "DistWorld_ArePersistedFeaturesInit"
.Ltag189: .asciz "DistWorld_BindMapObjectRotator"
.Ltag190: .asciz "DistWorld_CheckMapTransition"
.Ltag191: .asciz "DistWorld_GetMapObjectRotatorAnimFrame"
.Ltag192: .asciz "DistWorld_HandlePlayerMoved"
.Ltag193: .asciz "DistWorld_HandlePlayerMovementEnd"
.Ltag194: .asciz "DistWorld_HandlePlayerPositionChanged"
.Ltag195: .asciz "DistWorld_IsBoulderAtFallLocation"
.Ltag196: .asciz "DistWorld_LoadFloorOffsets"
.Ltag197: .asciz "DistWorld_RenderInactiveFloor"
.Ltag198: .asciz "DistWorld_TickInactiveFloor"
.Ltag199: .asciz "DistWorld_UnbindMapObjectRotator"
.Ltag200: .asciz "DistWorld_UpdateCameraAngle"
.Ltag201: .asciz "DistWorld_WillBoulderBeAtFallLocation"
.Ltag202: .asciz "DynamicMapFeatures_Free"
.Ltag203: .asciz "DynamicMapFeatures_Init"
.Ltag204: .asciz "DynamicTerrainHeightManager_Free"
.Ltag205: .asciz "DynamicTerrainHeightManager_New"
.Ltag206: .asciz "Easy3D_BindTextureToResource"
.Ltag207: .asciz "Easy3D_DrawRenderObj"
.Ltag208: .asciz "Easy3D_Init"
.Ltag209: .asciz "Easy3D_IsTextureUploadedToVRAM"
.Ltag210: .asciz "Easy3D_Shutdown"
.Ltag211: .asciz "EasyChatSentence_Init"
.Ltag212: .asciz "EasyChatSentence_SetTypeAndID"
.Ltag213: .asciz "EasyChatSentence_SetWord"
.Ltag214: .asciz "EasyChatSentence_ToString"
.Ltag215: .asciz "EncounterEffect_BlendTrainerSpritePltt"
.Ltag216: .asciz "EncounterEffect_Cave_HigherLevel"
.Ltag217: .asciz "EncounterEffect_Cave_LowerLevel"
.Ltag218: .asciz "EncounterEffect_ChampionCynthia"
.Ltag219: .asciz "EncounterEffect_Double"
.Ltag220: .asciz "EncounterEffect_EliteFourAaron"
.Ltag221: .asciz "EncounterEffect_EliteFourBertha"
.Ltag222: .asciz "EncounterEffect_EliteFourFlint"
.Ltag223: .asciz "EncounterEffect_EliteFourLucian"
.Ltag224: .asciz "EncounterEffect_Finish"
.Ltag225: .asciz "EncounterEffect_Flash"
.Ltag226: .asciz "EncounterEffect_Frontier"
.Ltag227: .asciz "EncounterEffect_GalacticBoss"
.Ltag228: .asciz "EncounterEffect_GalacticGrunt"
.Ltag229: .asciz "EncounterEffect_GetHBlankFlag"
.Ltag230: .asciz "EncounterEffect_Grass_HigherLevel"
.Ltag231: .asciz "EncounterEffect_Grass_LowerLevel"
.Ltag232: .asciz "EncounterEffect_InitSpriteCollection"
.Ltag233: .asciz "EncounterEffect_LeaderByron"
.Ltag234: .asciz "EncounterEffect_LeaderCandice"
.Ltag235: .asciz "EncounterEffect_LeaderFantina"
.Ltag236: .asciz "EncounterEffect_LeaderGardenia"
.Ltag237: .asciz "EncounterEffect_LeaderMaylene"
.Ltag238: .asciz "EncounterEffect_LeaderRoark"
.Ltag239: .asciz "EncounterEffect_LeaderVolkner"
.Ltag240: .asciz "EncounterEffect_LeaderWake"
.Ltag241: .asciz "EncounterEffect_Legendary"
.Ltag242: .asciz "EncounterEffect_Mythical"
.Ltag243: .asciz "EncounterEffect_ScreenSlice"
.Ltag244: .asciz "EncounterEffect_ScreenSplit"
.Ltag245: .asciz "EncounterEffect_Trainer_Cave_HigherLevel"
.Ltag246: .asciz "EncounterEffect_Trainer_Cave_LowerLevel"
.Ltag247: .asciz "EncounterEffect_Trainer_Grass_HigherLeve"
.Ltag248: .asciz "EncounterEffect_Trainer_Grass_LowerLevel"
.Ltag249: .asciz "EncounterEffect_Trainer_Water_HigherLeve"
.Ltag250: .asciz "EncounterEffect_Trainer_Water_LowerLevel"
.Ltag251: .asciz "EncounterEffect_Water_HigherLevel"
.Ltag252: .asciz "EncounterEffect_Water_LowerLevel"
.Ltag253: .asciz "Encounter_NewVsPalParkTransfer"
.Ltag254: .asciz "Encounter_StartVsWild"
.Ltag255: .asciz "ErrorHandling_AssertFail"
.Ltag256: .asciz "FS_CloseFile"
.Ltag257: .asciz "FS_InitFile"
.Ltag258: .asciz "FS_OpenFile"
.Ltag259: .asciz "FS_ReadFile"
.Ltag260: .asciz "FieldBGM_GetEffective"
.Ltag261: .asciz "FieldBGM_GetForMapHeader"
.Ltag262: .asciz "FieldBGM_PlayEffectiveForMapHeader"
.Ltag263: .asciz "FieldBGM_SetOverride"
.Ltag264: .asciz "FieldBGM_TryFadeOut"
.Ltag265: .asciz "FieldBattleDTO_Free"
.Ltag266: .asciz "FieldCamera_Create"
.Ltag267: .asciz "FieldCamera_Delete"
.Ltag268: .asciz "FieldCommManager_GetCurrentOccupiedSecre"
.Ltag269: .asciz "FieldCommManager_IsInMovementState"
.Ltag270: .asciz "FieldCommManager_PauseUndergroundResourc"
.Ltag271: .asciz "FieldCommManager_UnpauseUndergroundResou"
.Ltag272: .asciz "FieldEffectManager_ApplyAnimCopyToModel"
.Ltag273: .asciz "FieldEffectManager_FinishAnimManager"
.Ltag274: .asciz "FieldEffectManager_Free"
.Ltag275: .asciz "FieldEffectManager_GetFieldSystem"
.Ltag276: .asciz "FieldEffectManager_GetRendererContext"
.Ltag277: .asciz "FieldEffectManager_HasRenderer"
.Ltag278: .asciz "FieldEffectManager_HeapAlloc"
.Ltag279: .asciz "FieldEffectManager_HeapAllocInit"
.Ltag280: .asciz "FieldEffectManager_HeapFree"
.Ltag281: .asciz "FieldEffectManager_InitAnimManager"
.Ltag282: .asciz "FieldEffectManager_InitAnimManagerList"
.Ltag283: .asciz "FieldEffectManager_InitRenderer"
.Ltag284: .asciz "FieldEffectManager_InitRenderers"
.Ltag285: .asciz "FieldEffectManager_LoadAnimation"
.Ltag286: .asciz "FieldEffectManager_LoadModel"
.Ltag287: .asciz "FieldEffectManager_New"
.Ltag288: .asciz "FieldEffectManager_Render"
.Ltag289: .asciz "FieldEffect_InitRenderObject"
.Ltag290: .asciz "FieldEvent_GetInteractedBgEventScript"
.Ltag291: .asciz "FieldEvent_GetInteractedWallSignScript"
.Ltag292: .asciz "FieldMapChange_Set3DDisplay"
.Ltag293: .asciz "FieldMapChange_UpdateGameData"
.Ltag294: .asciz "FieldMapChange_UpdateGameDataDistortionW"
.Ltag295: .asciz "FieldMap_FadeScreen"
.Ltag296: .asciz "FieldMenuManager_DeleteMoveTutorCost"
.Ltag297: .asciz "FieldMenuManager_NewMoveTutorCostWindow"
.Ltag298: .asciz "FieldMessage_AddSignpostWindow"
.Ltag299: .asciz "FieldMessage_AddWindow"
.Ltag300: .asciz "FieldMessage_DrawWindow"
.Ltag301: .asciz "FieldMessage_FinishedPrinting"
.Ltag302: .asciz "FieldMessage_LoadAndDrawSignpost"
.Ltag303: .asciz "FieldMessage_LoadTextPalettes"
.Ltag304: .asciz "FieldMessage_Print"
.Ltag305: .asciz "FieldMessage_PrintWithParams"
.Ltag306: .asciz "FieldMotionBlur_Start"
.Ltag307: .asciz "FieldMotionBlur_Stop"
.Ltag308: .asciz "FieldOverworldState_GetCameraType"
.Ltag309: .asciz "FieldOverworldState_GetEntranceLocation"
.Ltag310: .asciz "FieldOverworldState_GetExitLocation"
.Ltag311: .asciz "FieldOverworldState_GetMapHistory"
.Ltag312: .asciz "FieldOverworldState_GetPlayerLocation"
.Ltag313: .asciz "FieldOverworldState_GetPoisonStepCount"
.Ltag314: .asciz "FieldOverworldState_GetSafariBallCount"
.Ltag315: .asciz "FieldOverworldState_GetSafariStepCount"
.Ltag316: .asciz "FieldOverworldState_GetSpecialLocation"
.Ltag317: .asciz "FieldOverworldState_GetWeather"
.Ltag318: .asciz "FieldSystem_CheckForTrainersWantingBattl"
.Ltag319: .asciz "FieldSystem_CreateTask"
.Ltag320: .asciz "FieldSystem_EndBottomScreen"
.Ltag321: .asciz "FieldSystem_EndVsSeekerTask"
.Ltag322: .asciz "FieldSystem_FindCollidingLoadedMapPropBy"
.Ltag323: .asciz "FieldSystem_FindCollidingLoadedMapPropBy"
.Ltag324: .asciz "FieldSystem_GetDayOfMonth"
.Ltag325: .asciz "FieldSystem_GetMonth"
.Ltag326: .asciz "FieldSystem_GetSaveData"
.Ltag327: .asciz "FieldSystem_GetScriptMemberPtr"
.Ltag328: .asciz "FieldSystem_GetVarPointer"
.Ltag329: .asciz "FieldSystem_HasPenalty"
.Ltag330: .asciz "FieldSystem_InitBottomScreen"
.Ltag331: .asciz "FieldSystem_IsBottomScreenDone"
.Ltag332: .asciz "FieldSystem_IsBottomScreenRunningDummy"
.Ltag333: .asciz "FieldSystem_IsInValidLocation"
.Ltag334: .asciz "FieldSystem_IsRunningFieldMapInner"
.Ltag335: .asciz "FieldSystem_IsRunningTask"
.Ltag336: .asciz "FieldSystem_ResumeProcessing"
.Ltag337: .asciz "FieldSystem_RunInitScript"
.Ltag338: .asciz "FieldSystem_SaveObjects"
.Ltag339: .asciz "FieldSystem_SaveTVSegment_WhatsFishing"
.Ltag340: .asciz "FieldSystem_SendPoketchEvent"
.Ltag341: .asciz "FieldSystem_StartMapChangeWarpTask"
.Ltag342: .asciz "FieldSystem_StartVsSeekerTask"
.Ltag343: .asciz "FieldSystem_TryGetVar"
.Ltag344: .asciz "FieldSystem_TrySetVillaFurnitureScript"
.Ltag345: .asciz "FieldTask_GetEnv"
.Ltag346: .asciz "FieldTask_GetFieldSystem"
.Ltag347: .asciz "FieldTask_GetState"
.Ltag348: .asciz "FieldTask_InitCall"
.Ltag349: .asciz "Field_DoPoisonEffect"
.Ltag350: .asciz "FogManager_ApplyColor"
.Ltag351: .asciz "FogManager_ApplyDensityTable"
.Ltag352: .asciz "FogManager_ApplyParameters"
.Ltag353: .asciz "FogManager_Free"
.Ltag354: .asciz "FogManager_GetColor"
.Ltag355: .asciz "FogManager_GetOffset"
.Ltag356: .asciz "FogManager_GetSlope"
.Ltag357: .asciz "FogManager_IsEnabled"
.Ltag358: .asciz "FogManager_New"
.Ltag359: .asciz "Font_CalcStringWidth"
.Ltag360: .asciz "Font_CalcStringWidthWithCursorControl"
.Ltag361: .asciz "Font_GetAttribute"
.Ltag362: .asciz "G2x_SetBGyAffine_"
.Ltag363: .asciz "G2x_SetBlendAlpha_"
.Ltag364: .asciz "G3X_SetEdgeColorTable"
.Ltag365: .asciz "G3X_SetFog"
.Ltag366: .asciz "G3X_SetFogTable"
.Ltag367: .asciz "G3X_SetHOffset"
.Ltag368: .asciz "G3_RequestSwapBuffers"
.Ltag369: .asciz "G3_ResetG3X"
.Ltag370: .asciz "GFXBoxTest_IsModelInView"
.Ltag371: .asciz "GXLayers_EngineAToggleLayers"
.Ltag372: .asciz "GXLayers_SetBanks"
.Ltag373: .asciz "GXLayers_SwapDisplay"
.Ltag374: .asciz "GXLayers_TurnBothDispOn"
.Ltag375: .asciz "GX_GetBankForLCDC"
.Ltag376: .asciz "GX_LoadBGPltt"
.Ltag377: .asciz "GX_LoadOBJPltt"
.Ltag378: .asciz "GX_ResetBankForBG"
.Ltag379: .asciz "GX_SetBankForBG"
.Ltag380: .asciz "GX_SetBankForLCDC"
.Ltag381: .asciz "GX_SetGraphicsMode"
.Ltag382: .asciz "GXx_SetMasterBrightness_"
.Ltag383: .asciz "GameRecords_GetRecordValue"
.Ltag384: .asciz "GameRecords_IncrementRecordValue"
.Ltag385: .asciz "GameRecords_IncrementTrainerScore"
.Ltag386: .asciz "GetSecondsSinceMidnight"
.Ltag387: .asciz "GetTextureDataBufferFromResourceName"
.Ltag388: .asciz "GetTextureDataVRAMBuffer"
.Ltag389: .asciz "GetTimeOfDay"
.Ltag390: .asciz "Good_GetDepth"
.Ltag391: .asciz "Good_GetWidth"
.Ltag392: .asciz "Graphics_GetCharData"
.Ltag393: .asciz "Graphics_GetPlttData"
.Ltag394: .asciz "Graphics_GetScrnDataFromOpenNARC"
.Ltag395: .asciz "Graphics_LoadPaletteFromOpenNARC"
.Ltag396: .asciz "Graphics_LoadTilesToBgLayerFromOpenNARC"
.Ltag397: .asciz "HBlankSystem_Delete"
.Ltag398: .asciz "HBlankSystem_New"
.Ltag399: .asciz "HBlankSystem_Start"
.Ltag400: .asciz "HBlankSystem_StartTask"
.Ltag401: .asciz "HBlankSystem_Stop"
.Ltag402: .asciz "HBlankTask_Delete"
.Ltag403: .asciz "HMCutIn_EndTask"
.Ltag404: .asciz "HMCutIn_IsFinished"
.Ltag405: .asciz "HMCutIn_StartTask"
.Ltag406: .asciz "HeapExp_FndInitAllocator"
.Ltag407: .asciz "Heap_Alloc"
.Ltag408: .asciz "Heap_AllocAtEnd"
.Ltag409: .asciz "Heap_Create"
.Ltag410: .asciz "Heap_Destroy"
.Ltag411: .asciz "Heap_Free"
.Ltag412: .asciz "Heap_FreeExplicit"
.Ltag413: .asciz "Heap_Realloc"
.Ltag414: .asciz "HearthomeGym_CheckIfEnteredIncorrectDoor"
.Ltag415: .asciz "HearthomeGym_CheckIfPlayerSeesTrainer"
.Ltag416: .asciz "HoneyTree_FreeShakeData"
.Ltag417: .asciz "HoneyTree_ShakeDataInit"
.Ltag418: .asciz "HoneyTree_TryInteract"
.Ltag419: .asciz "InitRenderer"
.Ltag420: .asciz "IsScreenFadeDone"
.Ltag421: .asciz "Item_IsMail"
.Ltag422: .asciz "Item_MoveForTMHM"
.Ltag423: .asciz "LCRNG_Next"
.Ltag424: .asciz "LandDataManager_DistortionWorldSetOffset"
.Ltag425: .asciz "LandDataManager_End"
.Ltag426: .asciz "LandDataManager_ForgetTrackedTarget"
.Ltag427: .asciz "LandDataManager_FreeNARCAndLoadedMapBuff"
.Ltag428: .asciz "LandDataManager_GetLoadedMapPropManager"
.Ltag429: .asciz "LandDataManager_GetOffsetTileX"
.Ltag430: .asciz "LandDataManager_GetOffsetTileZ"
.Ltag431: .asciz "LandDataManager_GetTrackedTargetLoadedMa"
.Ltag432: .asciz "LandDataManager_HasEnded"
.Ltag433: .asciz "LandDataManager_InitialLoad"
.Ltag434: .asciz "LandDataManager_New"
.Ltag435: .asciz "LandDataManager_RenderLoadedMaps"
.Ltag436: .asciz "LandDataManager_SetInDistortionWorld"
.Ltag437: .asciz "LandDataManager_SetMapLoadedCallback"
.Ltag438: .asciz "LandDataManager_SetSkipMapProps"
.Ltag439: .asciz "LandDataManager_Tick"
.Ltag440: .asciz "LandDataManager_TrackTarget"
.Ltag441: .asciz "LinearInterpolationTaskFX32_Init"
.Ltag442: .asciz "LinearInterpolationTaskFX32_Update"
.Ltag443: .asciz "LinearInterpolationTaskS32_Init"
.Ltag444: .asciz "LinearInterpolationTaskS32_Update"
.Ltag445: .asciz "ListMenu_CalcTrueCursorPos"
.Ltag446: .asciz "ListMenu_Free"
.Ltag447: .asciz "ListMenu_GetAttribute"
.Ltag448: .asciz "ListMenu_GetListAndCursorPos"
.Ltag449: .asciz "ListMenu_New"
.Ltag450: .asciz "ListMenu_ProcessInput"
.Ltag451: .asciz "ListMenu_SetAltTextColors"
.Ltag452: .asciz "LoadMemberFromNARC"
.Ltag453: .asciz "LoadMemberFromNARC_OutFileSize"
.Ltag454: .asciz "LoadMemberFromOpenNARC"
.Ltag455: .asciz "LoadMessageBoxGraphics"
.Ltag456: .asciz "LoadStandardWindowGraphics"
.Ltag457: .asciz "LoadedMapBuffers_Free"
.Ltag458: .asciz "LoadedMapBuffers_GetBDHCFileBufPtr"
.Ltag459: .asciz "LoadedMapBuffers_GetMapModelFileBufPtr"
.Ltag460: .asciz "LoadedMapBuffers_New"
.Ltag461: .asciz "LoadedMapBuffers_NewWithHeapMapModelFile"
.Ltag462: .asciz "LoadedMapBuffers_SwapMapModelData"
.Ltag463: .asciz "LocalMapObj_CheckAnimationFinished"
.Ltag464: .asciz "LocalMapObj_IsAnimationSet"
.Ltag465: .asciz "LocalMapObj_SetAnimationCode"
.Ltag466: .asciz "MTRNG_Next"
.Ltag467: .asciz "MapHeaderData_GetBgEvents"
.Ltag468: .asciz "MapHeaderData_GetCoordEvents"
.Ltag469: .asciz "MapHeaderData_GetIndexOfWarpEventAtPos"
.Ltag470: .asciz "MapHeaderData_GetNumBgEvents"
.Ltag471: .asciz "MapHeaderData_GetNumCoordEvents"
.Ltag472: .asciz "MapHeaderData_GetNumObjectEvents"
.Ltag473: .asciz "MapHeaderData_GetObjectEvents"
.Ltag474: .asciz "MapHeaderData_GetWarpEventByIndex"
.Ltag475: .asciz "MapHeaderData_Load"
.Ltag476: .asciz "MapHeader_GetAreaDataArchiveID"
.Ltag477: .asciz "MapHeader_GetMapLabelTextID"
.Ltag478: .asciz "MapHeader_GetMapLabelWindowID"
.Ltag479: .asciz "MapHeader_GetPreloadedMapObjectsArchiveI"
.Ltag480: .asciz "MapHeader_HasWildEncounters"
.Ltag481: .asciz "MapHeader_IsBuilding"
.Ltag482: .asciz "MapHeader_IsOnMainMatrix"
.Ltag483: .asciz "MapHeader_LoadString"
.Ltag484: .asciz "MapMatrix_Copy"
.Ltag485: .asciz "MapMatrix_GetAltitudeAtCoords"
.Ltag486: .asciz "MapMatrix_GetHeight"
.Ltag487: .asciz "MapMatrix_GetLandDataIDByIndex"
.Ltag488: .asciz "MapMatrix_GetMapHeaderIDAtCoords"
.Ltag489: .asciz "MapMatrix_GetMapHeaderIDAtIndex"
.Ltag490: .asciz "MapMatrix_GetMatrixID"
.Ltag491: .asciz "MapMatrix_GetWidth"
.Ltag492: .asciz "MapNamePopUp_Create"
.Ltag493: .asciz "MapNamePopUp_Destroy"
.Ltag494: .asciz "MapNamePopUp_Show"
.Ltag495: .asciz "MapObjMan_LocalMapObjByIndex"
.Ltag496: .asciz "MapObjectMan_CheckStatus"
.Ltag497: .asciz "MapObjectMan_FindObjectWithStatus"
.Ltag498: .asciz "MapObjectMan_GetMapObjectConst"
.Ltag499: .asciz "MapObjectMan_GetMaxObjects"
.Ltag500: .asciz "MapObjectMan_GetNARC"
.Ltag501: .asciz "MapObjectMan_GetTaskBasePriority"
.Ltag502: .asciz "MapObjectMan_IsDrawInitialized"
.Ltag503: .asciz "MapObjectMan_PauseAllMovement"
.Ltag504: .asciz "MapObjectMan_SetEndMovement"
.Ltag505: .asciz "MapObjectMan_SetNARC"
.Ltag506: .asciz "MapObjectMan_SetStatusFlagOff"
.Ltag507: .asciz "MapObjectMan_SetStatusFlagOn"
.Ltag508: .asciz "MapObjectMan_StopAllMovement"
.Ltag509: .asciz "MapObjectMan_UnpauseAllMovement"
.Ltag510: .asciz "MapObject_CalculateTaskPriority"
.Ltag511: .asciz "MapObject_CheckManagerStatus"
.Ltag512: .asciz "MapObject_CheckStatus"
.Ltag513: .asciz "MapObject_CheckStatusFlag"
.Ltag514: .asciz "MapObject_Delete"
.Ltag515: .asciz "MapObject_Draw"
.Ltag516: .asciz "MapObject_FieldSystem"
.Ltag517: .asciz "MapObject_FinishAnimation"
.Ltag518: .asciz "MapObject_GetCurrTileBehavior"
.Ltag519: .asciz "MapObject_GetDxFromDir"
.Ltag520: .asciz "MapObject_GetDzFromDir"
.Ltag521: .asciz "MapObject_GetEffectiveGraphicsID"
.Ltag522: .asciz "MapObject_GetFacingDir"
.Ltag523: .asciz "MapObject_GetFieldEffectManager"
.Ltag524: .asciz "MapObject_GetGraphicsID"
.Ltag525: .asciz "MapObject_GetLocalID"
.Ltag526: .asciz "MapObject_GetMapHeaderID"
.Ltag527: .asciz "MapObject_GetMovementType"
.Ltag528: .asciz "MapObject_GetMovingDir"
.Ltag529: .asciz "MapObject_GetPosPtr"
.Ltag530: .asciz "MapObject_GetPosY"
.Ltag531: .asciz "MapObject_GetPrevFacingDir"
.Ltag532: .asciz "MapObject_GetScript"
.Ltag533: .asciz "MapObject_GetSpriteJumpOffset"
.Ltag534: .asciz "MapObject_GetSpritePosOffset"
.Ltag535: .asciz "MapObject_GetSpriteTerrainOffset"
.Ltag536: .asciz "MapObject_GetTaskBasePriority"
.Ltag537: .asciz "MapObject_GetTileBehaviorFromDir"
.Ltag538: .asciz "MapObject_GetTrainerType"
.Ltag539: .asciz "MapObject_GetX"
.Ltag540: .asciz "MapObject_GetXPrev"
.Ltag541: .asciz "MapObject_GetY"
.Ltag542: .asciz "MapObject_GetYPrev"
.Ltag543: .asciz "MapObject_GetZ"
.Ltag544: .asciz "MapObject_GetZPrev"
.Ltag545: .asciz "MapObject_HasAnimationEnded"
.Ltag546: .asciz "MapObject_IsHidden"
.Ltag547: .asciz "MapObject_IsMovementPaused"
.Ltag548: .asciz "MapObject_IsMoving"
.Ltag549: .asciz "MapObject_IsStatusOnElevatedBridge"
.Ltag550: .asciz "MapObject_MapObjectManager"
.Ltag551: .asciz "MapObject_RecalculatePositionHeight"
.Ltag552: .asciz "MapObject_RecalculatePositionHeightEx"
.Ltag553: .asciz "MapObject_SetFlagDoNotSinkIntoTerrain"
.Ltag554: .asciz "MapObject_SetHidden"
.Ltag555: .asciz "MapObject_SetPauseMovementOff"
.Ltag556: .asciz "MapObject_SetPauseMovementOn"
.Ltag557: .asciz "MapObject_SetPos"
.Ltag558: .asciz "MapObject_SetSpriteJumpOffset"
.Ltag559: .asciz "MapObject_SetSpritePosOffset"
.Ltag560: .asciz "MapObject_SetStatusFlagOff"
.Ltag561: .asciz "MapObject_SetStatusFlagOn"
.Ltag562: .asciz "MapObject_SetX"
.Ltag563: .asciz "MapObject_SetY"
.Ltag564: .asciz "MapObject_SetZ"
.Ltag565: .asciz "MapObject_StartAnimation"
.Ltag566: .asciz "MapObject_SwitchMovementType"
.Ltag567: .asciz "MapObject_TryFace"
.Ltag568: .asciz "MapObject_UpdateCoords"
.Ltag569: .asciz "MapPropAnimationManager_AddAllAnimations"
.Ltag570: .asciz "MapPropAnimationManager_AddAnimationToRe"
.Ltag571: .asciz "MapPropAnimationManager_AdvanceAnimation"
.Ltag572: .asciz "MapPropAnimationManager_Free"
.Ltag573: .asciz "MapPropAnimationManager_GetAnimation"
.Ltag574: .asciz "MapPropAnimationManager_GetAnimeListNARC"
.Ltag575: .asciz "MapPropAnimationManager_GetPropAnimation"
.Ltag576: .asciz "MapPropAnimationManager_LoadPropAnimatio"
.Ltag577: .asciz "MapPropAnimationManager_New"
.Ltag578: .asciz "MapPropAnimationManager_RemoveAnimationF"
.Ltag579: .asciz "MapPropAnimationManager_UnloadAllAnimati"
.Ltag580: .asciz "MapPropAnimation_GoToFirstFrame"
.Ltag581: .asciz "MapPropAnimation_SetAnimationPaused"
.Ltag582: .asciz "MapPropAnimation_StartLoop"
.Ltag583: .asciz "MapPropManager_FindLoadedPropByModelID"
.Ltag584: .asciz "MapPropManager_Free"
.Ltag585: .asciz "MapPropManager_Init"
.Ltag586: .asciz "MapPropManager_Load"
.Ltag587: .asciz "MapPropManager_LoadOne"
.Ltag588: .asciz "MapPropManager_New"
.Ltag589: .asciz "MapPropManager_Render"
.Ltag590: .asciz "MapPropManager_Render2"
.Ltag591: .asciz "MapPropMaterialShape_Alloc"
.Ltag592: .asciz "MapPropMaterialShape_Free"
.Ltag593: .asciz "MapPropMaterialShape_GetMaterialShapeIDs"
.Ltag594: .asciz "MapPropMaterialShape_Load"
.Ltag595: .asciz "MapPropOneShotAnimationManager_Free"
.Ltag596: .asciz "MapPropOneShotAnimationManager_GetAnimat"
.Ltag597: .asciz "MapPropOneShotAnimationManager_IsAnimati"
.Ltag598: .asciz "MapPropOneShotAnimationManager_LoadPropA"
.Ltag599: .asciz "MapPropOneShotAnimationManager_New"
.Ltag600: .asciz "MapPropOneShotAnimationManager_PlayAnima"
.Ltag601: .asciz "MapPropOneShotAnimationManager_UnloadAni"
.Ltag602: .asciz "MapProp_GetMaterialShapeIDsCount"
.Ltag603: .asciz "MapProp_GetMaterialShapeIDsLocator"
.Ltag604: .asciz "MapProp_GetModel"
.Ltag605: .asciz "MapProp_GetRenderObj"
.Ltag606: .asciz "Menu_Free"
.Ltag607: .asciz "Menu_MakeYesNoChoice"
.Ltag608: .asciz "Menu_NewSimple"
.Ltag609: .asciz "Menu_ProcessInput"
.Ltag610: .asciz "Menu_ProcessInputAndHandleExit"
.Ltag611: .asciz "MessageLoader_Free"
.Ltag612: .asciz "MessageLoader_GetNewString"
.Ltag613: .asciz "MessageLoader_GetSpeciesName"
.Ltag614: .asciz "MessageLoader_GetString"
.Ltag615: .asciz "MessageLoader_Init"
.Ltag616: .asciz "MessageUtil_ExpandedString"
.Ltag617: .asciz "MessageUtil_SpeciesName"
.Ltag618: .asciz "MiscSaveBlock_GetFavoriteMon"
.Ltag619: .asciz "MiscSaveBlock_GetPersistedMapFeatures"
.Ltag620: .asciz "MiscSaveBlock_SetFavoriteMon"
.Ltag621: .asciz "ModelAttributes_ApplyGlobal"
.Ltag622: .asciz "ModelAttributes_ApplyToModel"
.Ltag623: .asciz "ModelAttributes_Free"
.Ltag624: .asciz "ModelAttributes_New"
.Ltag625: .asciz "ModelAttributes_SetAlpha"
.Ltag626: .asciz "ModelAttributes_SetAmbientReflection"
.Ltag627: .asciz "ModelAttributes_SetCullMode"
.Ltag628: .asciz "ModelAttributes_SetDiffuseReflection"
.Ltag629: .asciz "ModelAttributes_SetEmissionColor"
.Ltag630: .asciz "ModelAttributes_SetLightColor"
.Ltag631: .asciz "ModelAttributes_SetLightVector"
.Ltag632: .asciz "ModelAttributes_SetMiscAttrEnabled"
.Ltag633: .asciz "ModelAttributes_SetPolygonMode"
.Ltag634: .asciz "ModelAttributes_SetSpecularReflection"
.Ltag635: .asciz "MotionBlur_Delete"
.Ltag636: .asciz "MotionBlur_New"
.Ltag637: .asciz "MovementAction_TurnActionTowardsDir"
.Ltag638: .asciz "NARC_AllocAndReadWholeMember"
.Ltag639: .asciz "NARC_AllocAndReadWholeMemberByIndexPair"
.Ltag640: .asciz "NARC_AllocAtEndAndReadFromMemberByIndexP"
.Ltag641: .asciz "NARC_AllocAtEndAndReadWholeMemberByIndex"
.Ltag642: .asciz "NARC_GetFileCount"
.Ltag643: .asciz "NARC_GetMemberSize"
.Ltag644: .asciz "NARC_ReadFile"
.Ltag645: .asciz "NARC_ReadFromMember"
.Ltag646: .asciz "NARC_ReadWholeMember"
.Ltag647: .asciz "NARC_ReadWholeMemberByIndexPair"
.Ltag648: .asciz "NARC_Seek"
.Ltag649: .asciz "NARC_ctor"
.Ltag650: .asciz "NARC_dtor"
.Ltag651: .asciz "NNS_G2dGetImagePaletteLocation"
.Ltag652: .asciz "NNS_G2dGetUnpackedCharacterData"
.Ltag653: .asciz "NNS_G2dGetUnpackedPaletteData"
.Ltag654: .asciz "NNS_G2dGetUnpackedScreenData"
.Ltag655: .asciz "NNS_G2dInitOamManagerModule"
.Ltag656: .asciz "NNS_G3dAllocAnmObj"
.Ltag657: .asciz "NNS_G3dAnmObjInit"
.Ltag658: .asciz "NNS_G3dDraw1Mat1Shp"
.Ltag659: .asciz "NNS_G3dFreeAnmObj"
.Ltag660: .asciz "NNS_G3dGetAnmByIdx"
.Ltag661: .asciz "NNS_G3dGetMdlSet"
.Ltag662: .asciz "NNS_G3dGetTex"
.Ltag663: .asciz "NNS_G3dGlbFlushP"
.Ltag664: .asciz "NNS_G3dGlbGetInvV"
.Ltag665: .asciz "NNS_G3dGlbLightColor"
.Ltag666: .asciz "NNS_G3dGlbLightVector"
.Ltag667: .asciz "NNS_G3dGlbMaterialColorDiffAmb"
.Ltag668: .asciz "NNS_G3dGlbMaterialColorSpecEmi"
.Ltag669: .asciz "NNS_G3dGlbPolygonAttr"
.Ltag670: .asciz "NNS_G3dGlbSetBaseScale"
.Ltag671: .asciz "NNS_G3dGlbSetBaseTrans"
.Ltag672: .asciz "NNS_G3dMdlSetMdl1DotAll"
.Ltag673: .asciz "NNS_G3dMdlSetMdlAlphaAll"
.Ltag674: .asciz "NNS_G3dMdlSetMdlAmbAll"
.Ltag675: .asciz "NNS_G3dMdlSetMdlCullModeAll"
.Ltag676: .asciz "NNS_G3dMdlSetMdlDepthTestCondAll"
.Ltag677: .asciz "NNS_G3dMdlSetMdlDiffAll"
.Ltag678: .asciz "NNS_G3dMdlSetMdlEmiAll"
.Ltag679: .asciz "NNS_G3dMdlSetMdlFarClipAll"
.Ltag680: .asciz "NNS_G3dMdlSetMdlFogEnableFlagAll"
.Ltag681: .asciz "NNS_G3dMdlSetMdlLightEnableFlagAll"
.Ltag682: .asciz "NNS_G3dMdlSetMdlPolygonIDAll"
.Ltag683: .asciz "NNS_G3dMdlSetMdlPolygonModeAll"
.Ltag684: .asciz "NNS_G3dMdlSetMdlSpecAll"
.Ltag685: .asciz "NNS_G3dMdlSetMdlXLDepthUpdateAll"
.Ltag686: .asciz "NNS_G3dRenderObjAddAnmObj"
.Ltag687: .asciz "NNS_G3dRenderObjInit"
.Ltag688: .asciz "NNS_G3dRenderObjRemoveAnmObj"
.Ltag689: .asciz "NNS_GfdResetLnkPlttVramState"
.Ltag690: .asciz "NNS_GfdResetLnkTexVramState"
.Ltag691: .asciz "NNSi_G3dModifyMatFlag"
.Ltag692: .asciz "NNSi_G3dModifyPolygonAttrMask"
.Ltag693: .asciz "Options_Frame"
.Ltag694: .asciz "Options_TextFrameDelay"
.Ltag695: .asciz "Overlay_LoadByID"
.Ltag696: .asciz "Overlay_UnloadByID"
.Ltag697: .asciz "OverworldAnimManagerList_FinishAndFree"
.Ltag698: .asciz "OverworldAnimManagerList_InitManager"
.Ltag699: .asciz "OverworldAnimManagerList_New"
.Ltag700: .asciz "OverworldAnimManagerList_Render"
.Ltag701: .asciz "OverworldAnimManager_CallTickFunc"
.Ltag702: .asciz "OverworldAnimManager_Finish"
.Ltag703: .asciz "OverworldAnimManager_GetFuncsContext"
.Ltag704: .asciz "OverworldAnimManager_GetPosition"
.Ltag705: .asciz "OverworldAnimManager_GetUserData"
.Ltag706: .asciz "OverworldAnimManager_GetUserInt"
.Ltag707: .asciz "OverworldAnimManager_SetPosition"
.Ltag708: .asciz "OverworldMapHistory_Push"
.Ltag709: .asciz "ParticleSystem_CreateEmitter"
.Ltag710: .asciz "ParticleSystem_DrawAll"
.Ltag711: .asciz "ParticleSystem_Free"
.Ltag712: .asciz "ParticleSystem_FreeAll"
.Ltag713: .asciz "ParticleSystem_GetActiveEmitterCount"
.Ltag714: .asciz "ParticleSystem_GetCamera"
.Ltag715: .asciz "ParticleSystem_New"
.Ltag716: .asciz "ParticleSystem_SetCameraProjection"
.Ltag717: .asciz "ParticleSystem_SetResource"
.Ltag718: .asciz "ParticleSystem_UpdateAll"
.Ltag719: .asciz "ParticleSystem_ZeroAll"
.Ltag720: .asciz "Party_AddPokemon"
.Ltag721: .asciz "Party_FindFirstEligibleBattler"
.Ltag722: .asciz "Party_GetCurrentCount"
.Ltag723: .asciz "Party_GetPokemonBySlotIndex"
.Ltag724: .asciz "Party_HasMonWithMove"
.Ltag725: .asciz "Party_HasSpecies"
.Ltag726: .asciz "Party_HasTwoAliveMons"
.Ltag727: .asciz "Party_RemovePokemonBySlotIndex"
.Ltag728: .asciz "Party_ResetMonMoveSlot"
.Ltag729: .asciz "PersistedMapFeatures_GetBuffer"
.Ltag730: .asciz "PersistedMapFeatures_GetID"
.Ltag731: .asciz "PersistedMapFeatures_IsCurrentDynamicMap"
.Ltag732: .asciz "PlayTime_GetHours"
.Ltag733: .asciz "PlayTime_GetMinutes"
.Ltag734: .asciz "PlayerAvatar_CalcFaceDirection"
.Ltag735: .asciz "PlayerAvatar_CanUseRockClimb"
.Ltag736: .asciz "PlayerAvatar_CanUseSurf"
.Ltag737: .asciz "PlayerAvatar_CheckCollision"
.Ltag738: .asciz "PlayerAvatar_CheckDistortionCollision"
.Ltag739: .asciz "PlayerAvatar_CheckEscapedFromDeepMud"
.Ltag740: .asciz "PlayerAvatar_CheckForceStopMovement"
.Ltag741: .asciz "PlayerAvatar_CheckForcedMovement"
.Ltag742: .asciz "PlayerAvatar_CheckStartMove"
.Ltag743: .asciz "PlayerAvatar_ClearMoveState"
.Ltag744: .asciz "PlayerAvatar_ClearSpeed"
.Ltag745: .asciz "PlayerAvatar_DistortionGravityChanged"
.Ltag746: .asciz "PlayerAvatar_DistortionStateOnFloor"
.Ltag747: .asciz "PlayerAvatar_ForceStopMovement"
.Ltag748: .asciz "PlayerAvatar_GetDistortionCurrTileBehavi"
.Ltag749: .asciz "PlayerAvatar_GetDistortionDir"
.Ltag750: .asciz "PlayerAvatar_GetDistortionFacingTileBeha"
.Ltag751: .asciz "PlayerAvatar_GetFacingDir"
.Ltag752: .asciz "PlayerAvatar_GetFacingDistortionWorldPos"
.Ltag753: .asciz "PlayerAvatar_GetGender"
.Ltag754: .asciz "PlayerAvatar_GetMapObject"
.Ltag755: .asciz "PlayerAvatar_GetMoveState"
.Ltag756: .asciz "PlayerAvatar_GetMoveStateFromPlayerAvata"
.Ltag757: .asciz "PlayerAvatar_GetMovingDir"
.Ltag758: .asciz "PlayerAvatar_GetPlayerMoveState"
.Ltag759: .asciz "PlayerAvatar_GetPlayerState"
.Ltag760: .asciz "PlayerAvatar_GetPos"
.Ltag761: .asciz "PlayerAvatar_GetRequestStateFlag"
.Ltag762: .asciz "PlayerAvatar_GetSpeed"
.Ltag763: .asciz "PlayerAvatar_GetStepDistortionWorldPos"
.Ltag764: .asciz "PlayerAvatar_GetSurfMountAnimManager"
.Ltag765: .asciz "PlayerAvatar_GetTileBehaviorFromDir"
.Ltag766: .asciz "PlayerAvatar_GetXPos"
.Ltag767: .asciz "PlayerAvatar_GetZPos"
.Ltag768: .asciz "PlayerAvatar_InitMapFeatures"
.Ltag769: .asciz "PlayerAvatar_IsAnimationSetOrWalkOnSpotS"
.Ltag770: .asciz "PlayerAvatar_IsMapObjectAnimationSet"
.Ltag771: .asciz "PlayerAvatar_MapDistortionState"
.Ltag772: .asciz "PlayerAvatar_RequestChangeState"
.Ltag773: .asciz "PlayerAvatar_SetEscapedFromDeepMud"
.Ltag774: .asciz "PlayerAvatar_SetIgnoreTileBehavior"
.Ltag775: .asciz "PlayerAvatar_SetMapObjMovement"
.Ltag776: .asciz "PlayerAvatar_SetPlayerState"
.Ltag777: .asciz "PlayerAvatar_SetRequestStateFlag"
.Ltag778: .asciz "PlayerAvatar_SetSurfMountAnimManager"
.Ltag779: .asciz "PlayerAvatar_SetTransitionState"
.Ltag780: .asciz "PlayerAvatar_TryFace"
.Ltag781: .asciz "PlayerAvatar_TurnOnRequestStateBit"
.Ltag782: .asciz "Player_ConvertStateToTransition"
.Ltag783: .asciz "Player_GetSpriteFromStateAndGender"
.Ltag784: .asciz "PlttTransfer_Clear"
.Ltag785: .asciz "PlttTransfer_Free"
.Ltag786: .asciz "PlttTransfer_Init"
.Ltag787: .asciz "PokeRadar_ClearIfAllOutOfView"
.Ltag788: .asciz "Pokedex_Capture"
.Ltag789: .asciz "Pokedex_CountSeen"
.Ltag790: .asciz "Pokedex_HasSeenSpecies"
.Ltag791: .asciz "Pokedex_HeightWeightData"
.Ltag792: .asciz "Pokedex_HeightWeightData_Free"
.Ltag793: .asciz "Pokedex_HeightWeightData_Height"
.Ltag794: .asciz "Pokedex_HeightWeightData_Load"
.Ltag795: .asciz "Pokedex_HeightWeightData_Release"
.Ltag796: .asciz "Pokedex_IsObtained"
.Ltag797: .asciz "Pokemon_AddMove"
.Ltag798: .asciz "Pokemon_CalcLevelAndStats"
.Ltag799: .asciz "Pokemon_Copy"
.Ltag800: .asciz "Pokemon_DoPoisonDamage"
.Ltag801: .asciz "Pokemon_FromBoxPokemon"
.Ltag802: .asciz "Pokemon_GetBaseSpeciesFromPersonalData"
.Ltag803: .asciz "Pokemon_GetBoxPokemon"
.Ltag804: .asciz "Pokemon_GetGenderOf"
.Ltag805: .asciz "Pokemon_GetNatureOf"
.Ltag806: .asciz "Pokemon_GetValue"
.Ltag807: .asciz "Pokemon_InitWith"
.Ltag808: .asciz "Pokemon_IsPersonalityShiny"
.Ltag809: .asciz "Pokemon_LevelUpMove"
.Ltag810: .asciz "Pokemon_LoadLevelUpMoveIdsOf"
.Ltag811: .asciz "Pokemon_New"
.Ltag812: .asciz "Pokemon_ReplaceMove"
.Ltag813: .asciz "Pokemon_SetGiratinaFormByHeldItem"
.Ltag814: .asciz "Pokemon_SetRotomForm"
.Ltag815: .asciz "Pokemon_SetShayminForm"
.Ltag816: .asciz "Pokemon_SetValue"
.Ltag817: .asciz "Pokemon_ShouldLevelUp"
.Ltag818: .asciz "Pokemon_UpdateFriendship"
.Ltag819: .asciz "PoketchSystem_Create"
.Ltag820: .asciz "PoketchSystem_IsSystemShutdown"
.Ltag821: .asciz "PoketchSystem_SendEvent"
.Ltag822: .asciz "PoketchSystem_StartShutdown"
.Ltag823: .asciz "PoketchUnavailableScreen_Exit"
.Ltag824: .asciz "PoketchUnavailableScreen_Init"
.Ltag825: .asciz "PoketchUnavailableScreen_IsDone"
.Ltag826: .asciz "Poketch_Enable"
.Ltag827: .asciz "Poketch_IsEnabled"
.Ltag828: .asciz "QuadraticInterpolationTaskFX32_Init"
.Ltag829: .asciz "QuadraticInterpolationTaskFX32_Update"
.Ltag830: .asciz "RadarChain_Clear"
.Ltag831: .asciz "RadarChargeStep"
.Ltag832: .asciz "ReadFileToHeap"
.Ltag833: .asciz "RenderOam_Free"
.Ltag834: .asciz "RenderOam_GetScreenOam"
.Ltag835: .asciz "RenderOam_Init"
.Ltag836: .asciz "RenderOam_InitSurface"
.Ltag837: .asciz "RenderOam_Transfer"
.Ltag838: .asciz "Repel_UpdateSteps"
.Ltag839: .asciz "ResetVisibleHardwareWindows"
.Ltag840: .asciz "ResourceHeap_Free"
.Ltag841: .asciz "ResourceHeap_FreeItem"
.Ltag842: .asciz "ResourceHeap_GetItemData"
.Ltag843: .asciz "ResourceHeap_HasItem"
.Ltag844: .asciz "ResourceHeap_LoadMemberFromNARC"
.Ltag845: .asciz "ResourceHeap_New"
.Ltag846: .asciz "SaveData_GetBag"
.Ltag847: .asciz "SaveData_GetBattleFrontier"
.Ltag848: .asciz "SaveData_GetChatotCry"
.Ltag849: .asciz "SaveData_GetCoins"
.Ltag850: .asciz "SaveData_GetDaycare"
.Ltag851: .asciz "SaveData_GetFieldOverworldState"
.Ltag852: .asciz "SaveData_GetGameRecords"
.Ltag853: .asciz "SaveData_GetOptions"
.Ltag854: .asciz "SaveData_GetParty"
.Ltag855: .asciz "SaveData_GetPlayTime"
.Ltag856: .asciz "SaveData_GetPokedex"
.Ltag857: .asciz "SaveData_GetPoketch"
.Ltag858: .asciz "SaveData_GetSpecialEncounters"
.Ltag859: .asciz "SaveData_GetTrainerInfo"
.Ltag860: .asciz "SaveData_GetVarsFlags"
.Ltag861: .asciz "SaveData_GetWiFiList"
.Ltag862: .asciz "SaveData_GetWifiBattleTowerRecord"
.Ltag863: .asciz "SaveData_MiscSaveBlock"
.Ltag864: .asciz "SaveData_Save"
.Ltag865: .asciz "SaveData_WiFiHistory"
.Ltag866: .asciz "ScreenScrollManager_Delete"
.Ltag867: .asciz "ScreenScrollManager_GetWriteBuffer"
.Ltag868: .asciz "ScreenScrollManager_New"
.Ltag869: .asciz "ScreenScrollManager_RestartDMA"
.Ltag870: .asciz "ScreenScrollManager_ScrollX"
.Ltag871: .asciz "ScreenScrollManager_Stop"
.Ltag872: .asciz "ScreenScrollManager_SwapBuffers"
.Ltag873: .asciz "ScreenSliceEffect_Delete"
.Ltag874: .asciz "ScreenSliceEffect_Modify"
.Ltag875: .asciz "ScreenSliceEffect_New"
.Ltag876: .asciz "ScreenSplitEffect_Delete"
.Ltag877: .asciz "ScreenSplitEffect_New"
.Ltag878: .asciz "ScriptContext_Pause"
.Ltag879: .asciz "ScriptContext_ReadHalfWord"
.Ltag880: .asciz "ScriptManager_Set"
.Ltag881: .asciz "ScriptManager_Start"
.Ltag882: .asciz "Script_GetTrainerID"
.Ltag883: .asciz "Script_IsTrainerDefeated"
.Ltag884: .asciz "Script_IsTrainerDoubleBattle"
.Ltag885: .asciz "SecretBase_GetGoodIDAtIndex"
.Ltag886: .asciz "SecretBase_GetGoodXCoordAtIndex"
.Ltag887: .asciz "SecretBase_GetGoodZCoordAtIndex"
.Ltag888: .asciz "SecretBase_LoadProps"
.Ltag889: .asciz "SetAllGraphicsModes"
.Ltag890: .asciz "SetColorBrightness"
.Ltag891: .asciz "SetHBlankCallback"
.Ltag892: .asciz "SetScreenColorBrightness"
.Ltag893: .asciz "SetVBlankCallback"
.Ltag894: .asciz "Signpost_DoCurrentCommand"
.Ltag895: .asciz "Signpost_Free"
.Ltag896: .asciz "Signpost_Init"
.Ltag897: .asciz "Simple3D_ApplyAnimCopyToModel"
.Ltag898: .asciz "Simple3D_BindAnimToRenderObj"
.Ltag899: .asciz "Simple3D_CreateRenderObject"
.Ltag900: .asciz "Simple3D_CreateRenderObjectWithAnim"
.Ltag901: .asciz "Simple3D_DrawRenderObj"
.Ltag902: .asciz "Simple3D_DrawRenderObjRotationAngles"
.Ltag903: .asciz "Simple3D_DrawRenderObjSimple"
.Ltag904: .asciz "Simple3D_DrawRenderObjWithPos"
.Ltag905: .asciz "Simple3D_DrawRenderObjWithPosAndRotation"
.Ltag906: .asciz "Simple3D_FreeAnimObject"
.Ltag907: .asciz "Simple3D_FreeAnimation"
.Ltag908: .asciz "Simple3D_FreeModel"
.Ltag909: .asciz "Simple3D_HasAnimationReachedEnd"
.Ltag910: .asciz "Simple3D_LoadAnimFromOpenNARC"
.Ltag911: .asciz "Simple3D_LoadModelFromSet"
.Ltag912: .asciz "Simple3D_ScheduleBindModelTexture"
.Ltag913: .asciz "Simple3D_UpdateAnim"
.Ltag914: .asciz "Simple3D_ZeroOutAnimation"
.Ltag915: .asciz "Sound_IsEffectPlaying"
.Ltag916: .asciz "Sound_PlayEffect"
.Ltag917: .asciz "Sound_SetPlayerVolume"
.Ltag918: .asciz "Sound_StopEffect"
.Ltag919: .asciz "SpecialEncounter_GetHoneyTree"
.Ltag920: .asciz "SpecialEncounter_GetLastSlatheredTreeId"
.Ltag921: .asciz "SpecialEncounter_GetPlayerHoneyTreeState"
.Ltag922: .asciz "SpecialEncounter_SetLastSlatheredTreeId"
.Ltag923: .asciz "SpecialMetLoc_GetId"
.Ltag924: .asciz "SpeciesData_GetSpeciesValue"
.Ltag925: .asciz "SpriteList_Add"
.Ltag926: .asciz "SpriteList_AddAffine"
.Ltag927: .asciz "SpriteList_Delete"
.Ltag928: .asciz "SpriteList_InitRendering"
.Ltag929: .asciz "SpriteList_New"
.Ltag930: .asciz "SpriteList_Update"
.Ltag931: .asciz "SpriteResourceCollection_Add"
.Ltag932: .asciz "SpriteResourceCollection_AddFrom"
.Ltag933: .asciz "SpriteResourceCollection_AddPalette"
.Ltag934: .asciz "SpriteResourceCollection_AddPaletteFrom"
.Ltag935: .asciz "SpriteResourceCollection_AddPaletteFromE"
.Ltag936: .asciz "SpriteResourceCollection_AddTiles"
.Ltag937: .asciz "SpriteResourceCollection_AddTilesFrom"
.Ltag938: .asciz "SpriteResourceCollection_AddTilesFromEx"
.Ltag939: .asciz "SpriteResourceCollection_Delete"
.Ltag940: .asciz "SpriteResourceCollection_Extend"
.Ltag941: .asciz "SpriteResourceCollection_IsIDUnused"
.Ltag942: .asciz "SpriteResourceCollection_New"
.Ltag943: .asciz "SpriteResourceCollection_Remove"
.Ltag944: .asciz "SpriteResourceList_Delete"
.Ltag945: .asciz "SpriteResourceList_New"
.Ltag946: .asciz "SpriteResourceTable_Clear"
.Ltag947: .asciz "SpriteResourceTable_GetArrayElement"
.Ltag948: .asciz "SpriteResourceTable_GetCount"
.Ltag949: .asciz "SpriteResourceTable_GetEntryID"
.Ltag950: .asciz "SpriteResourceTable_GetEntryVRAMType"
.Ltag951: .asciz "SpriteResourceTable_GetNARCEntryMemberIn"
.Ltag952: .asciz "SpriteResourceTable_GetPaletteIndex"
.Ltag953: .asciz "SpriteResourceTable_IsNARCEntryCompresse"
.Ltag954: .asciz "SpriteResourceTable_LoadFromBinary"
.Ltag955: .asciz "SpriteResourceTable_Size"
.Ltag956: .asciz "SpriteResource_GetID"
.Ltag957: .asciz "SpriteResource_ReleaseData"
.Ltag958: .asciz "SpriteResourcesHeaderList_Free"
.Ltag959: .asciz "SpriteResourcesHeaderList_NewFromResdat"
.Ltag960: .asciz "SpriteResourcesHeader_Init"
.Ltag961: .asciz "SpriteSystem_SetTrainerClassGraphicsInde"
.Ltag962: .asciz "SpriteTransfer_GetPlttOffset"
.Ltag963: .asciz "SpriteTransfer_RequestCharAtEnd"
.Ltag964: .asciz "SpriteTransfer_RequestCharListAtEnd"
.Ltag965: .asciz "SpriteTransfer_RequestPlttFreeSpace"
.Ltag966: .asciz "SpriteTransfer_RequestPlttFreeSpaceList"
.Ltag967: .asciz "SpriteTransfer_ResetCharTransfer"
.Ltag968: .asciz "SpriteTransfer_ResetCharTransferList"
.Ltag969: .asciz "SpriteTransfer_ResetPlttTransfer"
.Ltag970: .asciz "SpriteTransfer_ResetPlttTransferList"
.Ltag971: .asciz "Sprite_Delete"
.Ltag972: .asciz "Sprite_GetExplicitPalette"
.Ltag973: .asciz "Sprite_GetPaletteProxy"
.Ltag974: .asciz "Sprite_GetPosition"
.Ltag975: .asciz "Sprite_SetAffineOverwriteMode"
.Ltag976: .asciz "Sprite_SetAffineScale"
.Ltag977: .asciz "Sprite_SetAffineScaleEx"
.Ltag978: .asciz "Sprite_SetAffineZRotation"
.Ltag979: .asciz "Sprite_SetAnim"
.Ltag980: .asciz "Sprite_SetAnimFrame"
.Ltag981: .asciz "Sprite_SetAnimSpeed"
.Ltag982: .asciz "Sprite_SetAnimateFlag"
.Ltag983: .asciz "Sprite_SetDrawFlag"
.Ltag984: .asciz "Sprite_SetExplicitOAMMode"
.Ltag985: .asciz "Sprite_SetExplicitPalette"
.Ltag986: .asciz "Sprite_SetExplicitPaletteWithOffset"
.Ltag987: .asciz "Sprite_SetExplicitPriority"
.Ltag988: .asciz "Sprite_SetPosition"
.Ltag989: .asciz "Sprite_SetPriority"
.Ltag990: .asciz "StartMenu_Open"
.Ltag991: .asciz "StartMenu_OpenColosseum"
.Ltag992: .asciz "StartMenu_OpenUnionRoom"
.Ltag993: .asciz "StartScreenFade"
.Ltag994: .asciz "StringList_AddFromMessageBank"
.Ltag995: .asciz "StringList_AddFromString"
.Ltag996: .asciz "StringList_Free"
.Ltag997: .asciz "StringList_New"
.Ltag998: .asciz "StringTemplate_Default"
.Ltag999: .asciz "StringTemplate_Format"
.Ltag1000: .asciz "StringTemplate_Free"
.Ltag1001: .asciz "StringTemplate_New"
.Ltag1002: .asciz "StringTemplate_SetGenderMarker"
.Ltag1003: .asciz "StringTemplate_SetLocationName"
.Ltag1004: .asciz "StringTemplate_SetNickname"
.Ltag1005: .asciz "StringTemplate_SetNumber"
.Ltag1006: .asciz "StringTemplate_SetOTName"
.Ltag1007: .asciz "StringTemplate_SetPlayerName"
.Ltag1008: .asciz "StringTemplate_SetTrainerName"
.Ltag1009: .asciz "String_Copy"
.Ltag1010: .asciz "String_CopyChars"
.Ltag1011: .asciz "String_Free"
.Ltag1012: .asciz "String_Init"
.Ltag1013: .asciz "SurfMountRenderer_Free"
.Ltag1014: .asciz "SurfMountRenderer_HandleSurfBegin"
.Ltag1015: .asciz "SurfMountRenderer_New"
.Ltag1016: .asciz "SurfMountRenderer_SetSyncPos"
.Ltag1017: .asciz "SysTaskManager_AddTask"
.Ltag1018: .asciz "SysTaskManager_ExecuteTasks"
.Ltag1019: .asciz "SysTaskManager_GetRequiredSize"
.Ltag1020: .asciz "SysTaskManager_Init"
.Ltag1021: .asciz "SysTask_Done"
.Ltag1022: .asciz "SysTask_ExecuteAfterVBlank"
.Ltag1023: .asciz "SysTask_ExecuteOnVBlank"
.Ltag1024: .asciz "SysTask_FinishAndFreeParam"
.Ltag1025: .asciz "SysTask_GetParam"
.Ltag1026: .asciz "SysTask_Start"
.Ltag1027: .asciz "SysTask_StartAndAllocateParam"
.Ltag1028: .asciz "SystemFlag_CheckHasPartner"
.Ltag1029: .asciz "SystemFlag_CheckInPalPark"
.Ltag1030: .asciz "SystemFlag_CheckPoketchHidden"
.Ltag1031: .asciz "SystemFlag_CheckSafariGameActive"
.Ltag1032: .asciz "SystemFlag_CheckUnlockedVsSeekerLevel"
.Ltag1033: .asciz "SystemFlag_CheckVsSeekerUsed"
.Ltag1034: .asciz "SystemFlag_ClearStep"
.Ltag1035: .asciz "SystemFlag_HandleOwnsVillaFurniture"
.Ltag1036: .asciz "SystemFlag_HandleStrengthActive"
.Ltag1037: .asciz "SystemFlag_SetVsSeekerUsed"
.Ltag1038: .asciz "SystemVars_GetBattleArcadePrintState"
.Ltag1039: .asciz "SystemVars_GetBattleCastlePrintState"
.Ltag1040: .asciz "SystemVars_GetBattleFactoryPrintState"
.Ltag1041: .asciz "SystemVars_GetBattleHallPrintState"
.Ltag1042: .asciz "SystemVars_GetBattleTowerPrintState"
.Ltag1043: .asciz "SystemVars_GetFriendshipStepCount"
.Ltag1044: .asciz "SystemVars_GetSizeContestRecord"
.Ltag1045: .asciz "SystemVars_GetVsSeekerBattery"
.Ltag1046: .asciz "SystemVars_GetVsSeekerStepCount"
.Ltag1047: .asciz "SystemVars_IncrementAmitySquareStepCount"
.Ltag1048: .asciz "SystemVars_ResetVsSeeker"
.Ltag1049: .asciz "SystemVars_SetFriendshipStepCount"
.Ltag1050: .asciz "SystemVars_SetSizeContestRecord"
.Ltag1051: .asciz "SystemVars_SetVsSeekerBattery"
.Ltag1052: .asciz "SystemVars_SetVsSeekerStepCount"
.Ltag1053: .asciz "TerrainCollisionHitbox_Init"
.Ltag1054: .asciz "TerrainCollisionManager_CheckCollision"
.Ltag1055: .asciz "TerrainCollisionManager_GetTileBehavior"
.Ltag1056: .asciz "Text_AddPrinterWithParams"
.Ltag1057: .asciz "Text_AddPrinterWithParamsAndColor"
.Ltag1058: .asciz "Text_IsPrinterActive"
.Ltag1059: .asciz "TextureResourceManager_AddTextureAndAllo"
.Ltag1060: .asciz "TextureResourceManager_Create"
.Ltag1061: .asciz "TextureResourceManager_Delete"
.Ltag1062: .asciz "TextureResourceManager_Destroy"
.Ltag1063: .asciz "TextureResourceManager_DiscardTextureDat"
.Ltag1064: .asciz "TextureResourceManager_FindTextureResour"
.Ltag1065: .asciz "TextureResourceManager_Free"
.Ltag1066: .asciz "TextureResourceManager_FreeAllSlots"
.Ltag1067: .asciz "TextureResourceManager_IsIDUnused"
.Ltag1068: .asciz "TextureResourceManager_LoadTexture"
.Ltag1069: .asciz "TextureResourceManager_New"
.Ltag1070: .asciz "TextureResourceManager_RemoveTextureWith"
.Ltag1071: .asciz "TextureResourceManager_UploadResourceToV"
.Ltag1072: .asciz "TextureResource_GetPaletteKey"
.Ltag1073: .asciz "TextureResource_GetTex4x4Key"
.Ltag1074: .asciz "TextureResource_GetTexKey"
.Ltag1075: .asciz "TextureResource_GetUnderlyingResource"
.Ltag1076: .asciz "TileBehavior_IsBikeParking"
.Ltag1077: .asciz "TileBehavior_IsBikeSlopeBottom"
.Ltag1078: .asciz "TileBehavior_IsBikeSlopeTop"
.Ltag1079: .asciz "TileBehavior_IsBookshelf1"
.Ltag1080: .asciz "TileBehavior_IsBookshelf2"
.Ltag1081: .asciz "TileBehavior_IsBridge"
.Ltag1082: .asciz "TileBehavior_IsBridgeStart"
.Ltag1083: .asciz "TileBehavior_IsDeepMud"
.Ltag1084: .asciz "TileBehavior_IsDeepMudWithGrass"
.Ltag1085: .asciz "TileBehavior_IsDoor"
.Ltag1086: .asciz "TileBehavior_IsEscalator"
.Ltag1087: .asciz "TileBehavior_IsEscalatorFlipFace"
.Ltag1088: .asciz "TileBehavior_IsMartShelf1"
.Ltag1089: .asciz "TileBehavior_IsMartShelf2"
.Ltag1090: .asciz "TileBehavior_IsMartShelf3"
.Ltag1091: .asciz "TileBehavior_IsPC"
.Ltag1092: .asciz "TileBehavior_IsPuddle"
.Ltag1093: .asciz "TileBehavior_IsReflective"
.Ltag1094: .asciz "TileBehavior_IsRockClimbEastWest"
.Ltag1095: .asciz "TileBehavior_IsRockClimbNorthSouth"
.Ltag1096: .asciz "TileBehavior_IsSlideEastward"
.Ltag1097: .asciz "TileBehavior_IsSlideNorthward"
.Ltag1098: .asciz "TileBehavior_IsSlideSouthward"
.Ltag1099: .asciz "TileBehavior_IsSlideWestward"
.Ltag1100: .asciz "TileBehavior_IsSmallBookshelf1"
.Ltag1101: .asciz "TileBehavior_IsSmallBookshelf2"
.Ltag1102: .asciz "TileBehavior_IsSurfable"
.Ltag1103: .asciz "TileBehavior_IsTV"
.Ltag1104: .asciz "TileBehavior_IsTownMap"
.Ltag1105: .asciz "TileBehavior_IsTrashCan"
.Ltag1106: .asciz "TileBehavior_IsWarpEast"
.Ltag1107: .asciz "TileBehavior_IsWarpEntranceEast"
.Ltag1108: .asciz "TileBehavior_IsWarpEntranceNorth"
.Ltag1109: .asciz "TileBehavior_IsWarpEntranceSouth"
.Ltag1110: .asciz "TileBehavior_IsWarpEntranceWest"
.Ltag1111: .asciz "TileBehavior_IsWarpNorth"
.Ltag1112: .asciz "TileBehavior_IsWarpPanel"
.Ltag1113: .asciz "TileBehavior_IsWarpSouth"
.Ltag1114: .asciz "TileBehavior_IsWarpStairsEast"
.Ltag1115: .asciz "TileBehavior_IsWarpStairsWest"
.Ltag1116: .asciz "TileBehavior_IsWarpWest"
.Ltag1117: .asciz "TileBehavior_IsWaterfall"
.Ltag1118: .asciz "TrainerInfo_BadgeCount"
.Ltag1119: .asciz "TrainerInfo_Gender"
.Ltag1120: .asciz "TrainerInfo_HasBadge"
.Ltag1121: .asciz "TrainerInfo_ID"
.Ltag1122: .asciz "TrainerInfo_Money"
.Ltag1123: .asciz "TrainerInfo_Name"
.Ltag1124: .asciz "TrainerInfo_NameNewString"
.Ltag1125: .asciz "TrainerInfo_New"
.Ltag1126: .asciz "TrainerInfo_SetName"
.Ltag1127: .asciz "UndergroundMan_CheckForTouchInput"
.Ltag1128: .asciz "UndergroundMan_GetSecretBaseXCoordinate"
.Ltag1129: .asciz "UndergroundMan_GetSecretBaseZCoordinate"
.Ltag1130: .asciz "UndergroundMan_OpenMenu"
.Ltag1131: .asciz "UndergroundMan_ProcessInteract"
.Ltag1132: .asciz "UndergroundMan_ProgressInteractCooldown"
.Ltag1133: .asciz "UpdateMonStatusAndTrainerInfo"
.Ltag1134: .asciz "VecFx32_FromXYZ"
.Ltag1135: .asciz "VecFx32_SetPosFromMapCoords"
.Ltag1136: .asciz "VeilstoneGymObjectRenderer_Free"
.Ltag1137: .asciz "VeilstoneGymObjectRenderer_New"
.Ltag1138: .asciz "VeilstoneGym_HitPunchingBag"
.Ltag1139: .asciz "VramTransfer_Free"
.Ltag1140: .asciz "VramTransfer_New"
.Ltag1141: .asciz "VramTransfer_Process"
.Ltag1142: .asciz "VramTransfer_Request"
.Ltag1143: .asciz "VsSeeker_UpdateStepCount"
.Ltag1144: .asciz "WiFiHistory_FlagGeonetLinkInfo"
.Ltag1145: .asciz "WiFiList_GetValidFriendsCount"
.Ltag1146: .asciz "WiFiList_IsValidFriendData"
.Ltag1147: .asciz "WifiBattleTowerRecord_UpdateBattlePoints"
.Ltag1148: .asciz "WildEncounters_TileHasEncounterRate"
.Ltag1149: .asciz "WildEncounters_TryFishingEncounter"
.Ltag1150: .asciz "WildEncounters_TryMudEncounter"
.Ltag1151: .asciz "WildEncounters_TrySweetScentEncounter"
.Ltag1152: .asciz "WildEncounters_TryWildEncounter"
.Ltag1153: .asciz "Window_Add"
.Ltag1154: .asciz "Window_BlitBitmapRect"
.Ltag1155: .asciz "Window_ClearAndCopyToVRAM"
.Ltag1156: .asciz "Window_CopyToVRAM"
.Ltag1157: .asciz "Window_DrawStandardFrame"
.Ltag1158: .asciz "Window_EraseMessageBox"
.Ltag1159: .asciz "Window_EraseStandardFrame"
.Ltag1160: .asciz "Window_FillRectWithColor"
.Ltag1161: .asciz "Window_FillTilemap"
.Ltag1162: .asciz "Window_IsInUse"
.Ltag1163: .asciz "Window_New"
.Ltag1164: .asciz "Window_Remove"
.Ltag1165: .asciz "Window_ScheduleCopyToVRAM"
.Ltag1166: .asciz "Windows_Delete"
.Ltag1167: .asciz "WirelessManager_GetConnectedBitmap"
.Ltag1168: .asciz "free"
.Ltag1169: .asciz "malloc"
.Ltag1170: .asciz "ov56_02256410"
.Ltag1171: .asciz "ov56_02256468"
.Ltag1172: .asciz "ov5_021D1434"
.Ltag1173: .asciz "ov5_021D143C"
.Ltag1174: .asciz "ov5_021D16F4"
.Ltag1175: .asciz "ov5_021D1718"
.Ltag1176: .asciz "ov5_021D1A94"
.Ltag1177: .asciz "ov5_021D1AE4"
.Ltag1178: .asciz "ov5_021D1B18"
.Ltag1179: .asciz "ov5_021D1B6C"
.Ltag1180: .asciz "ov5_021D1BEC"
.Ltag1181: .asciz "ov5_021D1C2C"
.Ltag1182: .asciz "ov5_021D4D78"
.Ltag1183: .asciz "ov5_021D5EB8"
.Ltag1184: .asciz "ov5_021D5EF8"
.Ltag1185: .asciz "ov5_021D5F24"
.Ltag1186: .asciz "ov5_021D5F7C"
.Ltag1187: .asciz "ov5_021D5FE0"
.Ltag1188: .asciz "ov5_021DE3D0"
.Ltag1189: .asciz "ov5_021DE4AC"
.Ltag1190: .asciz "ov5_021DE4CC"
.Ltag1191: .asciz "ov5_021DE5A4"
.Ltag1192: .asciz "ov5_021DE62C"
.Ltag1193: .asciz "ov5_021DE6A4"
.Ltag1194: .asciz "ov5_021DE6BC"
.Ltag1195: .asciz "ov5_021DE6C4"
.Ltag1196: .asciz "ov5_021DE71C"
.Ltag1197: .asciz "ov5_021DE8F8"
.Ltag1198: .asciz "ov5_021DE928"
.Ltag1199: .asciz "ov5_021DE948"
.Ltag1200: .asciz "ov5_021DE988"
.Ltag1201: .asciz "ov5_021DEBEC"
.Ltag1202: .asciz "ov5_021DEC18"
.Ltag1203: .asciz "ov5_021DEC38"
.Ltag1204: .asciz "ov5_021DECB8"
.Ltag1205: .asciz "ov5_021DECEC"
.Ltag1206: .asciz "ov5_021DED04"
.Ltag1207: .asciz "ov5_021DED20"
.Ltag1208: .asciz "ov5_021DEF8C"
.Ltag1209: .asciz "ov5_021DEFA0"
.Ltag1210: .asciz "ov5_021DF038"
.Ltag1211: .asciz "ov5_021DF054"
.Ltag1212: .asciz "ov5_021DF070"
.Ltag1213: .asciz "ov5_021DF084"
.Ltag1214: .asciz "ov5_021DF0CC"
.Ltag1215: .asciz "ov5_021DF17C"
.Ltag1216: .asciz "ov5_021DF1CC"
.Ltag1217: .asciz "ov5_021DF208"
.Ltag1218: .asciz "ov5_021DF224"
.Ltag1219: .asciz "ov5_021DF488"
.Ltag1220: .asciz "ov5_021DF7F8"
.Ltag1221: .asciz "ov5_021DF84C"
.Ltag1222: .asciz "ov5_021DF864"
.Ltag1223: .asciz "ov5_021DF9D4"
.Ltag1224: .asciz "ov5_021DF9E0"
.Ltag1225: .asciz "ov5_021DF9FC"
.Ltag1226: .asciz "ov5_021DFA08"
.Ltag1227: .asciz "ov5_021DFA14"
.Ltag1228: .asciz "ov5_021DFA30"
.Ltag1229: .asciz "ov5_021DFA3C"
.Ltag1230: .asciz "ov5_021DFA7C"
.Ltag1231: .asciz "ov5_021DFDE0"
.Ltag1232: .asciz "ov5_021E1154"
.Ltag1233: .asciz "ov5_021EA728"
.Ltag1234: .asciz "ov5_021EA790"
.Ltag1235: .asciz "ov5_021EA7CC"
.Ltag1236: .asciz "ov5_021EA830"
.Ltag1237: .asciz "ov5_021EA848"
.Ltag1238: .asciz "ov5_021EA854"
.Ltag1239: .asciz "ov5_021EB1A0"
.Ltag1240: .asciz "ov5_021EB2EC"
.Ltag1241: .asciz "ov5_021EB314"
.Ltag1242: .asciz "ov5_021EB328"
.Ltag1243: .asciz "ov5_021EB354"
.Ltag1244: .asciz "ov5_021EB398"
.Ltag1245: .asciz "ov5_021EB40C"
.Ltag1246: .asciz "ov5_021EB438"
.Ltag1247: .asciz "ov5_021EB720"
.Ltag1248: .asciz "ov5_021EB7F8"
.Ltag1249: .asciz "ov5_021EB834"
.Ltag1250: .asciz "ov5_021EB848"
.Ltag1251: .asciz "ov5_021EB874"
.Ltag1252: .asciz "ov5_021EB8B8"
.Ltag1253: .asciz "ov5_021EB944"
.Ltag1254: .asciz "ov5_021EBA0C"
.Ltag1255: .asciz "ov5_021EBA34"
.Ltag1256: .asciz "ov5_021EBA60"
.Ltag1257: .asciz "ov5_021EBA8C"
.Ltag1258: .asciz "ov5_021EBAD0"
.Ltag1259: .asciz "ov5_021EBF50"
.Ltag1260: .asciz "ov5_021EC15C"
.Ltag1261: .asciz "ov5_021EC3F0"
.Ltag1262: .asciz "ov5_021EC454"
.Ltag1263: .asciz "ov5_021EC4BC"
.Ltag1264: .asciz "ov5_021EC554"
.Ltag1265: .asciz "ov5_021EC5C0"
.Ltag1266: .asciz "ov5_021EC734"
.Ltag1267: .asciz "ov5_021EC75C"
.Ltag1268: .asciz "ov5_021EC760"
.Ltag1269: .asciz "ov5_021EC778"
.Ltag1270: .asciz "ov5_021EC790"
.Ltag1271: .asciz "ov5_021EC7B8"
.Ltag1272: .asciz "ov5_021EC7D0"
.Ltag1273: .asciz "ov5_021EC7F0"
.Ltag1274: .asciz "ov5_021EC824"
.Ltag1275: .asciz "ov5_021EC858"
.Ltag1276: .asciz "ov5_021EC86C"
.Ltag1277: .asciz "ov5_021EC898"
.Ltag1278: .asciz "ov5_021EC8EC"
.Ltag1279: .asciz "ov5_021EC938"
.Ltag1280: .asciz "ov5_021EC95C"
.Ltag1281: .asciz "ov5_021EC970"
.Ltag1282: .asciz "ov5_021EC9A0"
.Ltag1283: .asciz "ov5_021EC9E8"
.Ltag1284: .asciz "ov5_021ECA70"
.Ltag1285: .asciz "ov5_021ECA90"
.Ltag1286: .asciz "ov5_021ECABC"
.Ltag1287: .asciz "ov5_021ECAC4"
.Ltag1288: .asciz "ov5_021ECAF0"
.Ltag1289: .asciz "ov5_021ECB34"
.Ltag1290: .asciz "ov5_021ECB58"
.Ltag1291: .asciz "ov5_021ECB78"
.Ltag1292: .asciz "ov5_021ECB80"
.Ltag1293: .asciz "ov5_021ECC20"
.Ltag1294: .asciz "ov5_021ECC78"
.Ltag1295: .asciz "ov5_021ECD04"
.Ltag1296: .asciz "ov5_021ECD38"
.Ltag1297: .asciz "ov5_021ECD68"
.Ltag1298: .asciz "ov5_021ECDA0"
.Ltag1299: .asciz "ov5_021ECE18"
.Ltag1300: .asciz "ov5_021ECE30"
.Ltag1301: .asciz "ov5_021ECE34"
.Ltag1302: .asciz "ov5_021ECE38"
.Ltag1303: .asciz "ov5_021ECE3C"
.Ltag1304: .asciz "ov5_021ECE40"
.Ltag1305: .asciz "ov5_021ECE94"
.Ltag1306: .asciz "ov5_021ECF04"
.Ltag1307: .asciz "ov5_021ECFA4"
.Ltag1308: .asciz "ov5_021ED01C"
.Ltag1309: .asciz "ov5_021ED03C"
.Ltag1310: .asciz "ov5_021ED150"
.Ltag1311: .asciz "ov5_021EDC8C"
.Ltag1312: .asciz "ov5_021EDD94"
.Ltag1313: .asciz "ov5_021EDEB4"
.Ltag1314: .asciz "ov5_021EDF18"
.Ltag1315: .asciz "ov5_021EDF24"
.Ltag1316: .asciz "ov5_021EDF30"
.Ltag1317: .asciz "ov5_021EE3FC"
.Ltag1318: .asciz "ov5_021EE7C0"
.Ltag1319: .asciz "ov5_021EF4BC"
.Ltag1320: .asciz "ov5_021EF4F8"
.Ltag1321: .asciz "ov5_021F0030"
.Ltag1322: .asciz "ov5_021F0484"
.Ltag1323: .asciz "ov5_021F0488"
.Ltag1324: .asciz "ov5_021F10E8"
.Ltag1325: .asciz "ov5_021F10FC"
.Ltag1326: .asciz "ov5_021F121C"
.Ltag1327: .asciz "ov5_021F134C"
.Ltag1328: .asciz "ov5_021F1370"
.Ltag1329: .asciz "ov5_021F17B8"
.Ltag1330: .asciz "ov5_021F17D4"
.Ltag1331: .asciz "ov5_021F1BA4"
.Ltag1332: .asciz "ov5_021F1CC8"
.Ltag1333: .asciz "ov5_021F1CE8"
.Ltag1334: .asciz "ov5_021F23D0"
.Ltag1335: .asciz "ov5_021F23EC"
.Ltag1336: .asciz "ov5_021F2438"
.Ltag1337: .asciz "ov5_021F247C"
.Ltag1338: .asciz "ov5_021F2850"
.Ltag1339: .asciz "ov5_021F2864"
.Ltag1340: .asciz "ov5_021F28F4"
.Ltag1341: .asciz "ov5_021F2974"
.Ltag1342: .asciz "ov5_021F2A7C"
.Ltag1343: .asciz "ov5_021F2A98"
.Ltag1344: .asciz "ov5_021F2BD0"
.Ltag1345: .asciz "ov5_021F2BEC"
.Ltag1346: .asciz "ov5_021F2D20"
.Ltag1347: .asciz "ov5_021F2D3C"
.Ltag1348: .asciz "ov5_021F3284"
.Ltag1349: .asciz "ov5_021F32A0"
.Ltag1350: .asciz "ov5_021F348C"
.Ltag1351: .asciz "ov5_021F34A8"
.Ltag1352: .asciz "ov5_021F37A8"
.Ltag1353: .asciz "ov5_021F37C4"
.Ltag1354: .asciz "ov5_021F3A50"
.Ltag1355: .asciz "ov5_021F3A6C"
.Ltag1356: .asciz "ov5_021F3D00"
.Ltag1357: .asciz "ov5_021F3D1C"
.Ltag1358: .asciz "ov5_021F3E74"
.Ltag1359: .asciz "ov5_021F3E90"
.Ltag1360: .asciz "ov5_021F4018"
.Ltag1361: .asciz "ov5_021F4034"
.Ltag1362: .asciz "ov5_021F47B0"
.Ltag1363: .asciz "ov5_021F47CC"
.Ltag1364: .asciz "ov5_021F4DAC"
.Ltag1365: .asciz "ov5_021F4DC8"
.Ltag1366: .asciz "ov5_021F4E08"
.Ltag1367: .asciz "ov5_021F4E28"
.Ltag1368: .asciz "ov5_021F5060"
.Ltag1369: .asciz "ov5_021F507C"
.Ltag1370: .asciz "ov5_021F50BC"
.Ltag1371: .asciz "ov5_021F50D8"
.Ltag1372: .asciz "ov5_021F5284"
.Ltag1373: .asciz "ov5_021F52A0"
.Ltag1374: .asciz "ov5_021F5428"
.Ltag1375: .asciz "ov5_021F5444"
.Ltag1376: .asciz "ov5_021F55CC"
.Ltag1377: .asciz "ov5_021F55E8"
.Ltag1378: .asciz "ov5_021F575C"
.Ltag1379: .asciz "ov5_021F5778"
.Ltag1380: .asciz "ov5_021F5894"
.Ltag1381: .asciz "ov5_021F58B0"
.Ltag1382: .asciz "ov5_021F5A10"
.Ltag1383: .asciz "ov5_021F5A24"
.Ltag1384: .asciz "ov5_021F5C58"
.Ltag1385: .asciz "ov5_021F5D8C"
.Ltag1386: .asciz "ov5_021F600C"
.Ltag1387: .asciz "ov5_021F6028"
.Ltag1388: .asciz "ov5_021F61BC"
.Ltag1389: .asciz "ov5_021F61D8"
.Ltag1390: .asciz "ov6_022485F4"
.Ltag1391: .asciz "ov6_02248608"
.Ltag1392: .asciz "ov6_0224892C"
.Ltag1393: .asciz "ov6_02248940"
.Ltag1394: .asciz "sprintf"
.Ltag1395: .asciz "sub_0202AED8"
.Ltag1396: .asciz "sub_0202AEF0"
.Ltag1397: .asciz "sub_0202AFD4"
.Ltag1398: .asciz "sub_020363A0"
.Ltag1399: .asciz "sub_020391DC"
.Ltag1400: .asciz "sub_02039298"
.Ltag1401: .asciz "sub_0203A418"
.Ltag1402: .asciz "sub_0203CA40"
.Ltag1403: .asciz "sub_0203CBE0"
.Ltag1404: .asciz "sub_0203CC14"
.Ltag1405: .asciz "sub_020545EC"
.Ltag1406: .asciz "sub_020559DC"
.Ltag1407: .asciz "sub_02056B30"
.Ltag1408: .asciz "sub_02056BDC"
.Ltag1409: .asciz "sub_02056C18"
.Ltag1410: .asciz "sub_0205B2D4"
.Ltag1411: .asciz "sub_0205BEA8"
.Ltag1412: .asciz "sub_0206184C"
.Ltag1413: .asciz "sub_02061AB4"
.Ltag1414: .asciz "sub_02061B48"
.Ltag1415: .asciz "sub_02061BF0"
.Ltag1416: .asciz "sub_02061C48"
.Ltag1417: .asciz "sub_02062764"
.Ltag1418: .asciz "sub_020627B4"
.Ltag1419: .asciz "sub_0206285C"
.Ltag1420: .asciz "sub_02062880"
.Ltag1421: .asciz "sub_02062A0C"
.Ltag1422: .asciz "sub_02062A14"
.Ltag1423: .asciz "sub_02062ACC"
.Ltag1424: .asciz "sub_02062AF0"
.Ltag1425: .asciz "sub_02062B68"
.Ltag1426: .asciz "sub_02062BA4"
.Ltag1427: .asciz "sub_02062C18"
.Ltag1428: .asciz "sub_02062C3C"
.Ltag1429: .asciz "sub_02062CF8"
.Ltag1430: .asciz "sub_02062D4C"
.Ltag1431: .asciz "sub_02062DFC"
.Ltag1432: .asciz "sub_02062E94"
.Ltag1433: .asciz "sub_02062EC8"
.Ltag1434: .asciz "sub_02062F64"
.Ltag1435: .asciz "sub_02062F7C"
.Ltag1436: .asciz "sub_0206326C"
.Ltag1437: .asciz "sub_02063E18"
.Ltag1438: .asciz "sub_02063EBC"
.Ltag1439: .asciz "sub_020656AC"
.Ltag1440: .asciz "sub_02069238"
.Ltag1441: .asciz "sub_0206A218"
.Ltag1442: .asciz "sub_0206DC6C"
.text
.global __wrap_ARNG_Next
.type __wrap_ARNG_Next,%function
__wrap_ARNG_Next:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag0
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ARNG_Next
 .ltorg
.global __wrap_ApplicationManager_Args
.type __wrap_ApplicationManager_Args,%function
__wrap_ApplicationManager_Args:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ApplicationManager_Args
 .ltorg
.global __wrap_AreaDataManager_Alloc
.type __wrap_AreaDataManager_Alloc,%function
__wrap_AreaDataManager_Alloc:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag2
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_AreaDataManager_Alloc
 .ltorg
.global __wrap_AreaDataManager_Free
.type __wrap_AreaDataManager_Free,%function
__wrap_AreaDataManager_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag3
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_AreaDataManager_Free
 .ltorg
.global __wrap_AreaDataManager_GetAreaLightArchiveID
.type __wrap_AreaDataManager_GetAreaLightArchiveID,%function
__wrap_AreaDataManager_GetAreaLightArchiveID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag4
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_AreaDataManager_GetAreaLightArchiveID
 .ltorg
.global __wrap_AreaDataManager_GetMapPropMaterialShape
.type __wrap_AreaDataManager_GetMapPropMaterialShape,%function
__wrap_AreaDataManager_GetMapPropMaterialShape:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag5
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_AreaDataManager_GetMapPropMaterialShape
 .ltorg
.global __wrap_AreaDataManager_GetMapPropModelFile
.type __wrap_AreaDataManager_GetMapPropModelFile,%function
__wrap_AreaDataManager_GetMapPropModelFile:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag6
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_AreaDataManager_GetMapPropModelFile
 .ltorg
.global __wrap_AreaDataManager_GetMapPropModelID
.type __wrap_AreaDataManager_GetMapPropModelID,%function
__wrap_AreaDataManager_GetMapPropModelID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag7
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_AreaDataManager_GetMapPropModelID
 .ltorg
.global __wrap_AreaDataManager_GetMapPropTexture
.type __wrap_AreaDataManager_GetMapPropTexture,%function
__wrap_AreaDataManager_GetMapPropTexture:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag8
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_AreaDataManager_GetMapPropTexture
 .ltorg
.global __wrap_AreaDataManager_GetMapTexture
.type __wrap_AreaDataManager_GetMapTexture,%function
__wrap_AreaDataManager_GetMapTexture:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag9
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_AreaDataManager_GetMapTexture
 .ltorg
.global __wrap_AreaDataManager_HasMapPropModelFile
.type __wrap_AreaDataManager_HasMapPropModelFile,%function
__wrap_AreaDataManager_HasMapPropModelFile:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag10
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_AreaDataManager_HasMapPropModelFile
 .ltorg
.global __wrap_AreaDataManager_IsOutdoorsLighting
.type __wrap_AreaDataManager_IsOutdoorsLighting,%function
__wrap_AreaDataManager_IsOutdoorsLighting:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag11
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_AreaDataManager_IsOutdoorsLighting
 .ltorg
.global __wrap_AreaDataManager_Load
.type __wrap_AreaDataManager_Load,%function
__wrap_AreaDataManager_Load:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag12
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_AreaDataManager_Load
 .ltorg
.global __wrap_AreaLightManager_Free
.type __wrap_AreaLightManager_Free,%function
__wrap_AreaLightManager_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag13
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_AreaLightManager_Free
 .ltorg
.global __wrap_AreaLightManager_New
.type __wrap_AreaLightManager_New,%function
__wrap_AreaLightManager_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag14
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_AreaLightManager_New
 .ltorg
.global __wrap_AreaLightManager_UpdateActiveTemplate
.type __wrap_AreaLightManager_UpdateActiveTemplate,%function
__wrap_AreaLightManager_UpdateActiveTemplate:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag15
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_AreaLightManager_UpdateActiveTemplate
 .ltorg
.global __wrap_AreaLight_UseGlobalModelAttributes
.type __wrap_AreaLight_UseGlobalModelAttributes,%function
__wrap_AreaLight_UseGlobalModelAttributes:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag16
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_AreaLight_UseGlobalModelAttributes
 .ltorg
.global __wrap_Ascii_ConvertToInt
.type __wrap_Ascii_ConvertToInt,%function
__wrap_Ascii_ConvertToInt:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag17
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Ascii_ConvertToInt
 .ltorg
.global __wrap_Ascii_CopyToTerminator
.type __wrap_Ascii_CopyToTerminator,%function
__wrap_Ascii_CopyToTerminator:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag18
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Ascii_CopyToTerminator
 .ltorg
.global __wrap_BDHC_Free
.type __wrap_BDHC_Free,%function
__wrap_BDHC_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag19
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BDHC_Free
 .ltorg
.global __wrap_BDHC_KillLoad
.type __wrap_BDHC_KillLoad,%function
__wrap_BDHC_KillLoad:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag20
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BDHC_KillLoad
 .ltorg
.global __wrap_BDHC_LazyLoad
.type __wrap_BDHC_LazyLoad,%function
__wrap_BDHC_LazyLoad:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag21
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BDHC_LazyLoad
 .ltorg
.global __wrap_BDHC_Load
.type __wrap_BDHC_Load,%function
__wrap_BDHC_Load:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag22
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BDHC_Load
 .ltorg
.global __wrap_BDHC_MarkNotLoaded
.type __wrap_BDHC_MarkNotLoaded,%function
__wrap_BDHC_MarkNotLoaded:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag23
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BDHC_MarkNotLoaded
 .ltorg
.global __wrap_BDHC_New
.type __wrap_BDHC_New,%function
__wrap_BDHC_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag24
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BDHC_New
 .ltorg
.global __wrap_BDHC_Reset
.type __wrap_BDHC_Reset,%function
__wrap_BDHC_Reset:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag25
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BDHC_Reset
 .ltorg
.global __wrap_Bag_CanRemoveItem
.type __wrap_Bag_CanRemoveItem,%function
__wrap_Bag_CanRemoveItem:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag26
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bag_CanRemoveItem
 .ltorg
.global __wrap_Bag_TryAddItem
.type __wrap_Bag_TryAddItem,%function
__wrap_Bag_TryAddItem:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag27
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bag_TryAddItem
 .ltorg
.global __wrap_Bag_TryRemoveItem
.type __wrap_Bag_TryRemoveItem,%function
__wrap_Bag_TryRemoveItem:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag28
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bag_TryRemoveItem
 .ltorg
.global __wrap_BattleFrontierSave_ClearFriendStatsAndShift
.type __wrap_BattleFrontierSave_ClearFriendStatsAndShift,%function
__wrap_BattleFrontierSave_ClearFriendStatsAndShift:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag29
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BattleFrontierSave_ClearFriendStatsAndShift
 .ltorg
.global __wrap_BattleFrontierStats_GetHallRecordStreakIndex
.type __wrap_BattleFrontierStats_GetHallRecordStreakIndex,%function
__wrap_BattleFrontierStats_GetHallRecordStreakIndex:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag30
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BattleFrontierStats_GetHallRecordStreakIndex
 .ltorg
.global __wrap_BattleHallWinRecords_Get
.type __wrap_BattleHallWinRecords_Get,%function
__wrap_BattleHallWinRecords_Get:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag31
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BattleHallWinRecords_Get
 .ltorg
.global __wrap_BattleHallWinRecords_GetRecordForSpecies
.type __wrap_BattleHallWinRecords_GetRecordForSpecies,%function
__wrap_BattleHallWinRecords_GetRecordForSpecies:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag32
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BattleHallWinRecords_GetRecordForSpecies
 .ltorg
.global __wrap_BerryPatchGraphicsManager_Free
.type __wrap_BerryPatchGraphicsManager_Free,%function
__wrap_BerryPatchGraphicsManager_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag33
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BerryPatchGraphicsManager_Free
 .ltorg
.global __wrap_BerryPatchGraphicsManager_FreeEffectCounter
.type __wrap_BerryPatchGraphicsManager_FreeEffectCounter,%function
__wrap_BerryPatchGraphicsManager_FreeEffectCounter:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag34
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BerryPatchGraphicsManager_FreeEffectCounter
 .ltorg
.global __wrap_BerryPatchGraphicsManager_New
.type __wrap_BerryPatchGraphicsManager_New,%function
__wrap_BerryPatchGraphicsManager_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag35
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BerryPatchGraphicsManager_New
 .ltorg
.global __wrap_BerryPatchGraphicsManager_NewEffectCounter
.type __wrap_BerryPatchGraphicsManager_NewEffectCounter,%function
__wrap_BerryPatchGraphicsManager_NewEffectCounter:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag36
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BerryPatchGraphicsManager_NewEffectCounter
 .ltorg
.global __wrap_BerryPatchGraphics_FreeGraphics
.type __wrap_BerryPatchGraphics_FreeGraphics,%function
__wrap_BerryPatchGraphics_FreeGraphics:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag37
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BerryPatchGraphics_FreeGraphics
 .ltorg
.global __wrap_BerryPatchGraphics_GetCurrentGraphicsResourceID
.type __wrap_BerryPatchGraphics_GetCurrentGraphicsResourceID,%function
__wrap_BerryPatchGraphics_GetCurrentGraphicsResourceID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag38
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BerryPatchGraphics_GetCurrentGraphicsResourceID
 .ltorg
.global __wrap_BerryPatchGraphics_GetGraphicsObject
.type __wrap_BerryPatchGraphics_GetGraphicsObject,%function
__wrap_BerryPatchGraphics_GetGraphicsObject:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag39
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BerryPatchGraphics_GetGraphicsObject
 .ltorg
.global __wrap_BerryPatchGraphics_IsBerryPatch
.type __wrap_BerryPatchGraphics_IsBerryPatch,%function
__wrap_BerryPatchGraphics_IsBerryPatch:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag40
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BerryPatchGraphics_IsBerryPatch
 .ltorg
.global __wrap_BerryPatchGraphics_NewGraphics
.type __wrap_BerryPatchGraphics_NewGraphics,%function
__wrap_BerryPatchGraphics_NewGraphics:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag41
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BerryPatchGraphics_NewGraphics
 .ltorg
.global __wrap_BerryPatchGraphics_PauseGraphics
.type __wrap_BerryPatchGraphics_PauseGraphics,%function
__wrap_BerryPatchGraphics_PauseGraphics:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag42
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BerryPatchGraphics_PauseGraphics
 .ltorg
.global __wrap_BerryPatchGraphics_ResumeGraphics
.type __wrap_BerryPatchGraphics_ResumeGraphics,%function
__wrap_BerryPatchGraphics_ResumeGraphics:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag43
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BerryPatchGraphics_ResumeGraphics
 .ltorg
.global __wrap_BerryPatchGraphics_UpdateGraphics
.type __wrap_BerryPatchGraphics_UpdateGraphics,%function
__wrap_BerryPatchGraphics_UpdateGraphics:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag44
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BerryPatchGraphics_UpdateGraphics
 .ltorg
.global __wrap_BerryPatchManager_Free
.type __wrap_BerryPatchManager_Free,%function
__wrap_BerryPatchManager_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag45
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BerryPatchManager_Free
 .ltorg
.global __wrap_BerryPatchManager_New
.type __wrap_BerryPatchManager_New,%function
__wrap_BerryPatchManager_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag46
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BerryPatchManager_New
 .ltorg
.global __wrap_BerryPatches_GetGrowthStage
.type __wrap_BerryPatches_GetGrowthStage,%function
__wrap_BerryPatches_GetGrowthStage:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag47
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BerryPatches_GetGrowthStage
 .ltorg
.global __wrap_BerryPatches_GetMoisture
.type __wrap_BerryPatches_GetMoisture,%function
__wrap_BerryPatches_GetMoisture:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag48
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BerryPatches_GetMoisture
 .ltorg
.global __wrap_BerryPatches_UpdateGrowthStates
.type __wrap_BerryPatches_UpdateGrowthStates,%function
__wrap_BerryPatches_UpdateGrowthStates:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag49
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BerryPatches_UpdateGrowthStates
 .ltorg
.global __wrap_BgConfig_New
.type __wrap_BgConfig_New,%function
__wrap_BgConfig_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag50
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BgConfig_New
 .ltorg
.global __wrap_Bg_ChangeTilemapRectPalette
.type __wrap_Bg_ChangeTilemapRectPalette,%function
__wrap_Bg_ChangeTilemapRectPalette:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag51
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_ChangeTilemapRectPalette
 .ltorg
.global __wrap_Bg_ClearTilemap
.type __wrap_Bg_ClearTilemap,%function
__wrap_Bg_ClearTilemap:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag52
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_ClearTilemap
 .ltorg
.global __wrap_Bg_ClearTilesRange
.type __wrap_Bg_ClearTilesRange,%function
__wrap_Bg_ClearTilesRange:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag53
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_ClearTilesRange
 .ltorg
.global __wrap_Bg_CopyTilemapBufferRangeToVRAM
.type __wrap_Bg_CopyTilemapBufferRangeToVRAM,%function
__wrap_Bg_CopyTilemapBufferRangeToVRAM:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag54
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_CopyTilemapBufferRangeToVRAM
 .ltorg
.global __wrap_Bg_CopyTilemapBufferToVRAM
.type __wrap_Bg_CopyTilemapBufferToVRAM,%function
__wrap_Bg_CopyTilemapBufferToVRAM:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag55
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_CopyTilemapBufferToVRAM
 .ltorg
.global __wrap_Bg_FillTilemap
.type __wrap_Bg_FillTilemap,%function
__wrap_Bg_FillTilemap:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag56
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_FillTilemap
 .ltorg
.global __wrap_Bg_FillTilemapRect
.type __wrap_Bg_FillTilemapRect,%function
__wrap_Bg_FillTilemapRect:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag57
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_FillTilemapRect
 .ltorg
.global __wrap_Bg_FillTilesRange
.type __wrap_Bg_FillTilesRange,%function
__wrap_Bg_FillTilesRange:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag58
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_FillTilesRange
 .ltorg
.global __wrap_Bg_FreeTilemapBuffer
.type __wrap_Bg_FreeTilemapBuffer,%function
__wrap_Bg_FreeTilemapBuffer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag59
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_FreeTilemapBuffer
 .ltorg
.global __wrap_Bg_GetYOffset
.type __wrap_Bg_GetYOffset,%function
__wrap_Bg_GetYOffset:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag60
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_GetYOffset
 .ltorg
.global __wrap_Bg_InitFromTemplate
.type __wrap_Bg_InitFromTemplate,%function
__wrap_Bg_InitFromTemplate:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag61
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_InitFromTemplate
 .ltorg
.global __wrap_Bg_LoadPalette
.type __wrap_Bg_LoadPalette,%function
__wrap_Bg_LoadPalette:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag62
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_LoadPalette
 .ltorg
.global __wrap_Bg_LoadTilemapBuffer
.type __wrap_Bg_LoadTilemapBuffer,%function
__wrap_Bg_LoadTilemapBuffer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag63
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_LoadTilemapBuffer
 .ltorg
.global __wrap_Bg_LoadTiles
.type __wrap_Bg_LoadTiles,%function
__wrap_Bg_LoadTiles:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag64
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_LoadTiles
 .ltorg
.global __wrap_Bg_LoadToTilemapRect
.type __wrap_Bg_LoadToTilemapRect,%function
__wrap_Bg_LoadToTilemapRect:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag65
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_LoadToTilemapRect
 .ltorg
.global __wrap_Bg_MaskPalette
.type __wrap_Bg_MaskPalette,%function
__wrap_Bg_MaskPalette:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag66
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_MaskPalette
 .ltorg
.global __wrap_Bg_RunScheduledUpdates
.type __wrap_Bg_RunScheduledUpdates,%function
__wrap_Bg_RunScheduledUpdates:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag67
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_RunScheduledUpdates
 .ltorg
.global __wrap_Bg_ScheduleScroll
.type __wrap_Bg_ScheduleScroll,%function
__wrap_Bg_ScheduleScroll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag68
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_ScheduleScroll
 .ltorg
.global __wrap_Bg_ScheduleTilemapTransfer
.type __wrap_Bg_ScheduleTilemapTransfer,%function
__wrap_Bg_ScheduleTilemapTransfer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag69
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_ScheduleTilemapTransfer
 .ltorg
.global __wrap_Bg_SetOffset
.type __wrap_Bg_SetOffset,%function
__wrap_Bg_SetOffset:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag70
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_SetOffset
 .ltorg
.global __wrap_Bg_SetPriority
.type __wrap_Bg_SetPriority,%function
__wrap_Bg_SetPriority:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag71
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Bg_SetPriority
 .ltorg
.global __wrap_BillboardGfxSequence_SetData
.type __wrap_BillboardGfxSequence_SetData,%function
__wrap_BillboardGfxSequence_SetData:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag72
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BillboardGfxSequence_SetData
 .ltorg
.global __wrap_BillboardList_Append
.type __wrap_BillboardList_Append,%function
__wrap_BillboardList_Append:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag73
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BillboardList_Append
 .ltorg
.global __wrap_BillboardList_Delete
.type __wrap_BillboardList_Delete,%function
__wrap_BillboardList_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag74
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BillboardList_Delete
 .ltorg
.global __wrap_BillboardList_IsRedrawing
.type __wrap_BillboardList_IsRedrawing,%function
__wrap_BillboardList_IsRedrawing:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag75
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BillboardList_IsRedrawing
 .ltorg
.global __wrap_BillboardList_New
.type __wrap_BillboardList_New,%function
__wrap_BillboardList_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag76
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BillboardList_New
 .ltorg
.global __wrap_BillboardList_ResetRedraw
.type __wrap_BillboardList_ResetRedraw,%function
__wrap_BillboardList_ResetRedraw:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag77
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BillboardList_ResetRedraw
 .ltorg
.global __wrap_BillboardLists_Create
.type __wrap_BillboardLists_Create,%function
__wrap_BillboardLists_Create:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag78
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BillboardLists_Create
 .ltorg
.global __wrap_BillboardLists_Delete
.type __wrap_BillboardLists_Delete,%function
__wrap_BillboardLists_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag79
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BillboardLists_Delete
 .ltorg
.global __wrap_BillboardLists_Draw
.type __wrap_BillboardLists_Draw,%function
__wrap_BillboardLists_Draw:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag80
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BillboardLists_Draw
 .ltorg
.global __wrap_BillboardResources_SetWithoutKeys
.type __wrap_BillboardResources_SetWithoutKeys,%function
__wrap_BillboardResources_SetWithoutKeys:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag81
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BillboardResources_SetWithoutKeys
 .ltorg
.global __wrap_Billboard_AdvanceAnim
.type __wrap_Billboard_AdvanceAnim,%function
__wrap_Billboard_AdvanceAnim:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag82
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_AdvanceAnim
 .ltorg
.global __wrap_Billboard_Delete
.type __wrap_Billboard_Delete,%function
__wrap_Billboard_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag83
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_Delete
 .ltorg
.global __wrap_Billboard_GetAnimFrameNum
.type __wrap_Billboard_GetAnimFrameNum,%function
__wrap_Billboard_GetAnimFrameNum:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag84
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_GetAnimFrameNum
 .ltorg
.global __wrap_Billboard_GetAnimNum
.type __wrap_Billboard_GetAnimNum,%function
__wrap_Billboard_GetAnimNum:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag85
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_GetAnimNum
 .ltorg
.global __wrap_Billboard_GetDrawFlag
.type __wrap_Billboard_GetDrawFlag,%function
__wrap_Billboard_GetDrawFlag:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag86
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_GetDrawFlag
 .ltorg
.global __wrap_Billboard_GetFrameNum
.type __wrap_Billboard_GetFrameNum,%function
__wrap_Billboard_GetFrameNum:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag87
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_GetFrameNum
 .ltorg
.global __wrap_Billboard_GetModel
.type __wrap_Billboard_GetModel,%function
__wrap_Billboard_GetModel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag88
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_GetModel
 .ltorg
.global __wrap_Billboard_GetPos
.type __wrap_Billboard_GetPos,%function
__wrap_Billboard_GetPos:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag89
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_GetPos
 .ltorg
.global __wrap_Billboard_GetRotMatrix
.type __wrap_Billboard_GetRotMatrix,%function
__wrap_Billboard_GetRotMatrix:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag90
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_GetRotMatrix
 .ltorg
.global __wrap_Billboard_GetState
.type __wrap_Billboard_GetState,%function
__wrap_Billboard_GetState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag91
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_GetState
 .ltorg
.global __wrap_Billboard_SetAnimFrameNum
.type __wrap_Billboard_SetAnimFrameNum,%function
__wrap_Billboard_SetAnimFrameNum:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag92
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_SetAnimFrameNum
 .ltorg
.global __wrap_Billboard_SetAnimNum
.type __wrap_Billboard_SetAnimNum,%function
__wrap_Billboard_SetAnimNum:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag93
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_SetAnimNum
 .ltorg
.global __wrap_Billboard_SetCallback
.type __wrap_Billboard_SetCallback,%function
__wrap_Billboard_SetCallback:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag94
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_SetCallback
 .ltorg
.global __wrap_Billboard_SetDrawFlag
.type __wrap_Billboard_SetDrawFlag,%function
__wrap_Billboard_SetDrawFlag:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag95
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_SetDrawFlag
 .ltorg
.global __wrap_Billboard_SetFrameNum
.type __wrap_Billboard_SetFrameNum,%function
__wrap_Billboard_SetFrameNum:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag96
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_SetFrameNum
 .ltorg
.global __wrap_Billboard_SetPos
.type __wrap_Billboard_SetPos,%function
__wrap_Billboard_SetPos:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag97
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_SetPos
 .ltorg
.global __wrap_Billboard_SetRotMatrix
.type __wrap_Billboard_SetRotMatrix,%function
__wrap_Billboard_SetRotMatrix:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag98
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_SetRotMatrix
 .ltorg
.global __wrap_Billboard_SetScale
.type __wrap_Billboard_SetScale,%function
__wrap_Billboard_SetScale:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag99
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_SetScale
 .ltorg
.global __wrap_Billboard_TryRequestVRAMTransfer
.type __wrap_Billboard_TryRequestVRAMTransfer,%function
__wrap_Billboard_TryRequestVRAMTransfer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag100
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Billboard_TryRequestVRAMTransfer
 .ltorg
.global __wrap_BlendPalette
.type __wrap_BlendPalette,%function
__wrap_BlendPalette:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag101
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BlendPalette
 .ltorg
.global __wrap_BlendPalettes
.type __wrap_BlendPalettes,%function
__wrap_BlendPalettes:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag102
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BlendPalettes
 .ltorg
.global __wrap_BoxPokemon_Copy
.type __wrap_BoxPokemon_Copy,%function
__wrap_BoxPokemon_Copy:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag103
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BoxPokemon_Copy
 .ltorg
.global __wrap_BoxPokemon_FromPokemon
.type __wrap_BoxPokemon_FromPokemon,%function
__wrap_BoxPokemon_FromPokemon:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag104
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BoxPokemon_FromPokemon
 .ltorg
.global __wrap_BoxPokemon_GetGender
.type __wrap_BoxPokemon_GetGender,%function
__wrap_BoxPokemon_GetGender:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag105
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BoxPokemon_GetGender
 .ltorg
.global __wrap_BoxPokemon_GetLevel
.type __wrap_BoxPokemon_GetLevel,%function
__wrap_BoxPokemon_GetLevel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag106
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BoxPokemon_GetLevel
 .ltorg
.global __wrap_BoxPokemon_GetValue
.type __wrap_BoxPokemon_GetValue,%function
__wrap_BoxPokemon_GetValue:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag107
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BoxPokemon_GetValue
 .ltorg
.global __wrap_BoxPokemon_Init
.type __wrap_BoxPokemon_Init,%function
__wrap_BoxPokemon_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag108
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BoxPokemon_Init
 .ltorg
.global __wrap_BoxPokemon_SetShayminForm
.type __wrap_BoxPokemon_SetShayminForm,%function
__wrap_BoxPokemon_SetShayminForm:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag109
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BoxPokemon_SetShayminForm
 .ltorg
.global __wrap_BoxPokemon_SetValue
.type __wrap_BoxPokemon_SetValue,%function
__wrap_BoxPokemon_SetValue:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag110
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BoxPokemon_SetValue
 .ltorg
.global __wrap_BrightnessController_IsTransitionComplete
.type __wrap_BrightnessController_IsTransitionComplete,%function
__wrap_BrightnessController_IsTransitionComplete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag111
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BrightnessController_IsTransitionComplete
 .ltorg
.global __wrap_BrightnessController_SetScreenBrightness
.type __wrap_BrightnessController_SetScreenBrightness,%function
__wrap_BrightnessController_SetScreenBrightness:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag112
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BrightnessController_SetScreenBrightness
 .ltorg
.global __wrap_BrightnessController_StartTransition
.type __wrap_BrightnessController_StartTransition,%function
__wrap_BrightnessController_StartTransition:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag113
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BrightnessController_StartTransition
 .ltorg
.global __wrap_BufferManager_Delete
.type __wrap_BufferManager_Delete,%function
__wrap_BufferManager_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag114
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BufferManager_Delete
 .ltorg
.global __wrap_BufferManager_GetReadBuffer
.type __wrap_BufferManager_GetReadBuffer,%function
__wrap_BufferManager_GetReadBuffer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag115
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BufferManager_GetReadBuffer
 .ltorg
.global __wrap_BufferManager_GetWriteBuffer
.type __wrap_BufferManager_GetWriteBuffer,%function
__wrap_BufferManager_GetWriteBuffer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag116
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BufferManager_GetWriteBuffer
 .ltorg
.global __wrap_BufferManager_New
.type __wrap_BufferManager_New,%function
__wrap_BufferManager_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag117
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BufferManager_New
 .ltorg
.global __wrap_BufferManager_SwapBuffers
.type __wrap_BufferManager_SwapBuffers,%function
__wrap_BufferManager_SwapBuffers:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag118
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_BufferManager_SwapBuffers
 .ltorg
.global __wrap_CalcCosineDegrees
.type __wrap_CalcCosineDegrees,%function
__wrap_CalcCosineDegrees:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag119
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CalcCosineDegrees
 .ltorg
.global __wrap_CalcLinearFov
.type __wrap_CalcLinearFov,%function
__wrap_CalcLinearFov:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag120
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CalcLinearFov
 .ltorg
.global __wrap_CalcSineDegrees
.type __wrap_CalcSineDegrees,%function
__wrap_CalcSineDegrees:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag121
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CalcSineDegrees
 .ltorg
.global __wrap_CalcTextureDataSizeFromResourceName
.type __wrap_CalcTextureDataSizeFromResourceName,%function
__wrap_CalcTextureDataSizeFromResourceName:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag122
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CalcTextureDataSizeFromResourceName
 .ltorg
.global __wrap_Camera_AdjustFOV
.type __wrap_Camera_AdjustFOV,%function
__wrap_Camera_AdjustFOV:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag123
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_AdjustFOV
 .ltorg
.global __wrap_Camera_Alloc
.type __wrap_Camera_Alloc,%function
__wrap_Camera_Alloc:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag124
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_Alloc
 .ltorg
.global __wrap_Camera_ClearActive
.type __wrap_Camera_ClearActive,%function
__wrap_Camera_ClearActive:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag125
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_ClearActive
 .ltorg
.global __wrap_Camera_ComputeViewMatrix
.type __wrap_Camera_ComputeViewMatrix,%function
__wrap_Camera_ComputeViewMatrix:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag126
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_ComputeViewMatrix
 .ltorg
.global __wrap_Camera_ComputeViewMatrixWithRoll
.type __wrap_Camera_ComputeViewMatrixWithRoll,%function
__wrap_Camera_ComputeViewMatrixWithRoll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag127
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_ComputeViewMatrixWithRoll
 .ltorg
.global __wrap_Camera_Delete
.type __wrap_Camera_Delete,%function
__wrap_Camera_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag128
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_Delete
 .ltorg
.global __wrap_Camera_DeleteHistory
.type __wrap_Camera_DeleteHistory,%function
__wrap_Camera_DeleteHistory:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag129
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_DeleteHistory
 .ltorg
.global __wrap_Camera_GetAngle
.type __wrap_Camera_GetAngle,%function
__wrap_Camera_GetAngle:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag130
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_GetAngle
 .ltorg
.global __wrap_Camera_GetDistance
.type __wrap_Camera_GetDistance,%function
__wrap_Camera_GetDistance:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag131
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_GetDistance
 .ltorg
.global __wrap_Camera_GetFOV
.type __wrap_Camera_GetFOV,%function
__wrap_Camera_GetFOV:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag132
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_GetFOV
 .ltorg
.global __wrap_Camera_GetPosition
.type __wrap_Camera_GetPosition,%function
__wrap_Camera_GetPosition:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag133
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_GetPosition
 .ltorg
.global __wrap_Camera_GetTarget
.type __wrap_Camera_GetTarget,%function
__wrap_Camera_GetTarget:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag134
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_GetTarget
 .ltorg
.global __wrap_Camera_InitHistory
.type __wrap_Camera_InitHistory,%function
__wrap_Camera_InitHistory:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag135
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_InitHistory
 .ltorg
.global __wrap_Camera_InitWithTarget
.type __wrap_Camera_InitWithTarget,%function
__wrap_Camera_InitWithTarget:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag136
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_InitWithTarget
 .ltorg
.global __wrap_Camera_ReleaseTarget
.type __wrap_Camera_ReleaseTarget,%function
__wrap_Camera_ReleaseTarget:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag137
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_ReleaseTarget
 .ltorg
.global __wrap_Camera_SetAngleAroundTarget
.type __wrap_Camera_SetAngleAroundTarget,%function
__wrap_Camera_SetAngleAroundTarget:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag138
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_SetAngleAroundTarget
 .ltorg
.global __wrap_Camera_SetAsActive
.type __wrap_Camera_SetAsActive,%function
__wrap_Camera_SetAsActive:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag139
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_SetAsActive
 .ltorg
.global __wrap_Camera_SetClipping
.type __wrap_Camera_SetClipping,%function
__wrap_Camera_SetClipping:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag140
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_SetClipping
 .ltorg
.global __wrap_Camera_SetDistance
.type __wrap_Camera_SetDistance,%function
__wrap_Camera_SetDistance:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag141
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_SetDistance
 .ltorg
.global __wrap_Camera_SetFOV
.type __wrap_Camera_SetFOV,%function
__wrap_Camera_SetFOV:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag142
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Camera_SetFOV
 .ltorg
.global __wrap_CanPokemonFormLearnTM
.type __wrap_CanPokemonFormLearnTM,%function
__wrap_CanPokemonFormLearnTM:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag143
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CanPokemonFormLearnTM
 .ltorg
.global __wrap_CanalaveGym_CheckIfPlayerOnPlatform
.type __wrap_CanalaveGym_CheckIfPlayerOnPlatform,%function
__wrap_CanalaveGym_CheckIfPlayerOnPlatform:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag144
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CanalaveGym_CheckIfPlayerOnPlatform
 .ltorg
.global __wrap_CatchingShow_CheckWildEncounter
.type __wrap_CatchingShow_CheckWildEncounter,%function
__wrap_CatchingShow_CheckWildEncounter:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag145
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CatchingShow_CheckWildEncounter
 .ltorg
.global __wrap_CatchingShow_GetBattleDataTransfer
.type __wrap_CatchingShow_GetBattleDataTransfer,%function
__wrap_CatchingShow_GetBattleDataTransfer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag146
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CatchingShow_GetBattleDataTransfer
 .ltorg
.global __wrap_CharTransfer_ClearBuffers
.type __wrap_CharTransfer_ClearBuffers,%function
__wrap_CharTransfer_ClearBuffers:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag147
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CharTransfer_ClearBuffers
 .ltorg
.global __wrap_CharTransfer_Free
.type __wrap_CharTransfer_Free,%function
__wrap_CharTransfer_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag148
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CharTransfer_Free
 .ltorg
.global __wrap_CharTransfer_InitWithVramModes
.type __wrap_CharTransfer_InitWithVramModes,%function
__wrap_CharTransfer_InitWithVramModes:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag149
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CharTransfer_InitWithVramModes
 .ltorg
.global __wrap_ChatotCry_ResetStatus
.type __wrap_ChatotCry_ResetStatus,%function
__wrap_ChatotCry_ResetStatus:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag150
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ChatotCry_ResetStatus
 .ltorg
.global __wrap_ClearUnusedSystemCache
.type __wrap_ClearUnusedSystemCache,%function
__wrap_ClearUnusedSystemCache:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag151
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ClearUnusedSystemCache
 .ltorg
.global __wrap_Coins_GetValue
.type __wrap_Coins_GetValue,%function
__wrap_Coins_GetValue:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag152
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Coins_GetValue
 .ltorg
.global __wrap_CommInfo_TrainerInfo
.type __wrap_CommInfo_TrainerInfo,%function
__wrap_CommInfo_TrainerInfo:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag153
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CommInfo_TrainerInfo
 .ltorg
.global __wrap_CommManager_IsInitializedNotPoketch
.type __wrap_CommManager_IsInitializedNotPoketch,%function
__wrap_CommManager_IsInitializedNotPoketch:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag154
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CommManager_IsInitializedNotPoketch
 .ltorg
.global __wrap_CommManager_PauseUnionClient
.type __wrap_CommManager_PauseUnionClient,%function
__wrap_CommManager_PauseUnionClient:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag155
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CommManager_PauseUnionClient
 .ltorg
.global __wrap_CommManager_PauseUnionServer
.type __wrap_CommManager_PauseUnionServer,%function
__wrap_CommManager_PauseUnionServer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag156
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CommManager_PauseUnionServer
 .ltorg
.global __wrap_CommManager_SetCommError
.type __wrap_CommManager_SetCommError,%function
__wrap_CommManager_SetCommError:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag157
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CommManager_SetCommError
 .ltorg
.global __wrap_CommPlayerMan_ForcePos
.type __wrap_CommPlayerMan_ForcePos,%function
__wrap_CommPlayerMan_ForcePos:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag158
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CommPlayerMan_ForcePos
 .ltorg
.global __wrap_CommPlayer_GetMovementTimer
.type __wrap_CommPlayer_GetMovementTimer,%function
__wrap_CommPlayer_GetMovementTimer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag159
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CommPlayer_GetMovementTimer
 .ltorg
.global __wrap_CommSys_ConnectedCount
.type __wrap_CommSys_ConnectedCount,%function
__wrap_CommSys_ConnectedCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag160
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CommSys_ConnectedCount
 .ltorg
.global __wrap_CommSys_CurNetId
.type __wrap_CommSys_CurNetId,%function
__wrap_CommSys_CurNetId:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag161
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_CommSys_CurNetId
 .ltorg
.global __wrap_DaycareMail_GetMail
.type __wrap_DaycareMail_GetMail,%function
__wrap_DaycareMail_GetMail:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag162
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DaycareMail_GetMail
 .ltorg
.global __wrap_DaycareMon_AddSteps
.type __wrap_DaycareMon_AddSteps,%function
__wrap_DaycareMon_AddSteps:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag163
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DaycareMon_AddSteps
 .ltorg
.global __wrap_DaycareMon_CopyToDaycareMon
.type __wrap_DaycareMon_CopyToDaycareMon,%function
__wrap_DaycareMon_CopyToDaycareMon:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag164
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DaycareMon_CopyToDaycareMon
 .ltorg
.global __wrap_DaycareMon_GetBoxMon
.type __wrap_DaycareMon_GetBoxMon,%function
__wrap_DaycareMon_GetBoxMon:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag165
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DaycareMon_GetBoxMon
 .ltorg
.global __wrap_DaycareMon_GetDaycareMail
.type __wrap_DaycareMon_GetDaycareMail,%function
__wrap_DaycareMon_GetDaycareMail:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag166
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DaycareMon_GetDaycareMail
 .ltorg
.global __wrap_DaycareMon_GetSteps
.type __wrap_DaycareMon_GetSteps,%function
__wrap_DaycareMon_GetSteps:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag167
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DaycareMon_GetSteps
 .ltorg
.global __wrap_DaycareMon_Init
.type __wrap_DaycareMon_Init,%function
__wrap_DaycareMon_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag168
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DaycareMon_Init
 .ltorg
.global __wrap_DaycareMon_SetSteps
.type __wrap_DaycareMon_SetSteps,%function
__wrap_DaycareMon_SetSteps:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag169
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DaycareMon_SetSteps
 .ltorg
.global __wrap_Daycare_AreParentLanguagesDifferent
.type __wrap_Daycare_AreParentLanguagesDifferent,%function
__wrap_Daycare_AreParentLanguagesDifferent:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag170
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Daycare_AreParentLanguagesDifferent
 .ltorg
.global __wrap_Daycare_GetDaycareMon
.type __wrap_Daycare_GetDaycareMon,%function
__wrap_Daycare_GetDaycareMon:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag171
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Daycare_GetDaycareMon
 .ltorg
.global __wrap_Daycare_GetOffspringPersonality
.type __wrap_Daycare_GetOffspringPersonality,%function
__wrap_Daycare_GetOffspringPersonality:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag172
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Daycare_GetOffspringPersonality
 .ltorg
.global __wrap_Daycare_GetStepCounter
.type __wrap_Daycare_GetStepCounter,%function
__wrap_Daycare_GetStepCounter:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag173
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Daycare_GetStepCounter
 .ltorg
.global __wrap_Daycare_HasEgg
.type __wrap_Daycare_HasEgg,%function
__wrap_Daycare_HasEgg:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag174
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Daycare_HasEgg
 .ltorg
.global __wrap_Daycare_SetOffspringPersonality
.type __wrap_Daycare_SetOffspringPersonality,%function
__wrap_Daycare_SetOffspringPersonality:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag175
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Daycare_SetOffspringPersonality
 .ltorg
.global __wrap_Daycare_SetStepCounter
.type __wrap_Daycare_SetStepCounter,%function
__wrap_Daycare_SetStepCounter:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag176
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Daycare_SetStepCounter
 .ltorg
.global __wrap_Daycare_Update
.type __wrap_Daycare_Update,%function
__wrap_Daycare_Update:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag177
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Daycare_Update
 .ltorg
.global __wrap_Direction_GetOpposite
.type __wrap_Direction_GetOpposite,%function
__wrap_Direction_GetOpposite:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag178
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Direction_GetOpposite
 .ltorg
.global __wrap_DisableHBlank
.type __wrap_DisableHBlank,%function
__wrap_DisableHBlank:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag179
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DisableHBlank
 .ltorg
.global __wrap_DistWorldFallingBoulder_New
.type __wrap_DistWorldFallingBoulder_New,%function
__wrap_DistWorldFallingBoulder_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag180
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorldFallingBoulder_New
 .ltorg
.global __wrap_DistWorldFallingBoulder_Tick
.type __wrap_DistWorldFallingBoulder_Tick,%function
__wrap_DistWorldFallingBoulder_Tick:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag181
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorldFallingBoulder_Tick
 .ltorg
.global __wrap_DistWorldSurfMountRenderer_ClearFlags
.type __wrap_DistWorldSurfMountRenderer_ClearFlags,%function
__wrap_DistWorldSurfMountRenderer_ClearFlags:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag182
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorldSurfMountRenderer_ClearFlags
 .ltorg
.global __wrap_DistWorldSurfMountRenderer_Free
.type __wrap_DistWorldSurfMountRenderer_Free,%function
__wrap_DistWorldSurfMountRenderer_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag183
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorldSurfMountRenderer_Free
 .ltorg
.global __wrap_DistWorldSurfMountRenderer_HandleSurfBegin
.type __wrap_DistWorldSurfMountRenderer_HandleSurfBegin,%function
__wrap_DistWorldSurfMountRenderer_HandleSurfBegin:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag184
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorldSurfMountRenderer_HandleSurfBegin
 .ltorg
.global __wrap_DistWorldSurfMountRenderer_New
.type __wrap_DistWorldSurfMountRenderer_New,%function
__wrap_DistWorldSurfMountRenderer_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag185
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorldSurfMountRenderer_New
 .ltorg
.global __wrap_DistWorldSurfMountRenderer_SetFlags
.type __wrap_DistWorldSurfMountRenderer_SetFlags,%function
__wrap_DistWorldSurfMountRenderer_SetFlags:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag186
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorldSurfMountRenderer_SetFlags
 .ltorg
.global __wrap_DistWorld_ApplyGiratinaSpritePalette
.type __wrap_DistWorld_ApplyGiratinaSpritePalette,%function
__wrap_DistWorld_ApplyGiratinaSpritePalette:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag187
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorld_ApplyGiratinaSpritePalette
 .ltorg
.global __wrap_DistWorld_ArePersistedFeaturesInit
.type __wrap_DistWorld_ArePersistedFeaturesInit,%function
__wrap_DistWorld_ArePersistedFeaturesInit:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag188
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorld_ArePersistedFeaturesInit
 .ltorg
.global __wrap_DistWorld_BindMapObjectRotator
.type __wrap_DistWorld_BindMapObjectRotator,%function
__wrap_DistWorld_BindMapObjectRotator:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag189
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorld_BindMapObjectRotator
 .ltorg
.global __wrap_DistWorld_CheckMapTransition
.type __wrap_DistWorld_CheckMapTransition,%function
__wrap_DistWorld_CheckMapTransition:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag190
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorld_CheckMapTransition
 .ltorg
.global __wrap_DistWorld_GetMapObjectRotatorAnimFrame
.type __wrap_DistWorld_GetMapObjectRotatorAnimFrame,%function
__wrap_DistWorld_GetMapObjectRotatorAnimFrame:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag191
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorld_GetMapObjectRotatorAnimFrame
 .ltorg
.global __wrap_DistWorld_HandlePlayerMoved
.type __wrap_DistWorld_HandlePlayerMoved,%function
__wrap_DistWorld_HandlePlayerMoved:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag192
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorld_HandlePlayerMoved
 .ltorg
.global __wrap_DistWorld_HandlePlayerMovementEnd
.type __wrap_DistWorld_HandlePlayerMovementEnd,%function
__wrap_DistWorld_HandlePlayerMovementEnd:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag193
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorld_HandlePlayerMovementEnd
 .ltorg
.global __wrap_DistWorld_HandlePlayerPositionChanged
.type __wrap_DistWorld_HandlePlayerPositionChanged,%function
__wrap_DistWorld_HandlePlayerPositionChanged:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag194
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorld_HandlePlayerPositionChanged
 .ltorg
.global __wrap_DistWorld_IsBoulderAtFallLocation
.type __wrap_DistWorld_IsBoulderAtFallLocation,%function
__wrap_DistWorld_IsBoulderAtFallLocation:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag195
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorld_IsBoulderAtFallLocation
 .ltorg
.global __wrap_DistWorld_LoadFloorOffsets
.type __wrap_DistWorld_LoadFloorOffsets,%function
__wrap_DistWorld_LoadFloorOffsets:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag196
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorld_LoadFloorOffsets
 .ltorg
.global __wrap_DistWorld_RenderInactiveFloor
.type __wrap_DistWorld_RenderInactiveFloor,%function
__wrap_DistWorld_RenderInactiveFloor:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag197
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorld_RenderInactiveFloor
 .ltorg
.global __wrap_DistWorld_TickInactiveFloor
.type __wrap_DistWorld_TickInactiveFloor,%function
__wrap_DistWorld_TickInactiveFloor:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag198
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorld_TickInactiveFloor
 .ltorg
.global __wrap_DistWorld_UnbindMapObjectRotator
.type __wrap_DistWorld_UnbindMapObjectRotator,%function
__wrap_DistWorld_UnbindMapObjectRotator:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag199
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorld_UnbindMapObjectRotator
 .ltorg
.global __wrap_DistWorld_UpdateCameraAngle
.type __wrap_DistWorld_UpdateCameraAngle,%function
__wrap_DistWorld_UpdateCameraAngle:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag200
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorld_UpdateCameraAngle
 .ltorg
.global __wrap_DistWorld_WillBoulderBeAtFallLocation
.type __wrap_DistWorld_WillBoulderBeAtFallLocation,%function
__wrap_DistWorld_WillBoulderBeAtFallLocation:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag201
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DistWorld_WillBoulderBeAtFallLocation
 .ltorg
.global __wrap_DynamicMapFeatures_Free
.type __wrap_DynamicMapFeatures_Free,%function
__wrap_DynamicMapFeatures_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag202
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DynamicMapFeatures_Free
 .ltorg
.global __wrap_DynamicMapFeatures_Init
.type __wrap_DynamicMapFeatures_Init,%function
__wrap_DynamicMapFeatures_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag203
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DynamicMapFeatures_Init
 .ltorg
.global __wrap_DynamicTerrainHeightManager_Free
.type __wrap_DynamicTerrainHeightManager_Free,%function
__wrap_DynamicTerrainHeightManager_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag204
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DynamicTerrainHeightManager_Free
 .ltorg
.global __wrap_DynamicTerrainHeightManager_New
.type __wrap_DynamicTerrainHeightManager_New,%function
__wrap_DynamicTerrainHeightManager_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag205
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_DynamicTerrainHeightManager_New
 .ltorg
.global __wrap_Easy3D_BindTextureToResource
.type __wrap_Easy3D_BindTextureToResource,%function
__wrap_Easy3D_BindTextureToResource:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag206
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Easy3D_BindTextureToResource
 .ltorg
.global __wrap_Easy3D_DrawRenderObj
.type __wrap_Easy3D_DrawRenderObj,%function
__wrap_Easy3D_DrawRenderObj:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag207
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Easy3D_DrawRenderObj
 .ltorg
.global __wrap_Easy3D_Init
.type __wrap_Easy3D_Init,%function
__wrap_Easy3D_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag208
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Easy3D_Init
 .ltorg
.global __wrap_Easy3D_IsTextureUploadedToVRAM
.type __wrap_Easy3D_IsTextureUploadedToVRAM,%function
__wrap_Easy3D_IsTextureUploadedToVRAM:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag209
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Easy3D_IsTextureUploadedToVRAM
 .ltorg
.global __wrap_Easy3D_Shutdown
.type __wrap_Easy3D_Shutdown,%function
__wrap_Easy3D_Shutdown:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag210
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Easy3D_Shutdown
 .ltorg
.global __wrap_EasyChatSentence_Init
.type __wrap_EasyChatSentence_Init,%function
__wrap_EasyChatSentence_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag211
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EasyChatSentence_Init
 .ltorg
.global __wrap_EasyChatSentence_SetTypeAndID
.type __wrap_EasyChatSentence_SetTypeAndID,%function
__wrap_EasyChatSentence_SetTypeAndID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag212
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EasyChatSentence_SetTypeAndID
 .ltorg
.global __wrap_EasyChatSentence_SetWord
.type __wrap_EasyChatSentence_SetWord,%function
__wrap_EasyChatSentence_SetWord:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag213
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EasyChatSentence_SetWord
 .ltorg
.global __wrap_EasyChatSentence_ToString
.type __wrap_EasyChatSentence_ToString,%function
__wrap_EasyChatSentence_ToString:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag214
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EasyChatSentence_ToString
 .ltorg
.global __wrap_EncounterEffect_BlendTrainerSpritePltt
.type __wrap_EncounterEffect_BlendTrainerSpritePltt,%function
__wrap_EncounterEffect_BlendTrainerSpritePltt:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag215
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_BlendTrainerSpritePltt
 .ltorg
.global __wrap_EncounterEffect_Cave_HigherLevel
.type __wrap_EncounterEffect_Cave_HigherLevel,%function
__wrap_EncounterEffect_Cave_HigherLevel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag216
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Cave_HigherLevel
 .ltorg
.global __wrap_EncounterEffect_Cave_LowerLevel
.type __wrap_EncounterEffect_Cave_LowerLevel,%function
__wrap_EncounterEffect_Cave_LowerLevel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag217
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Cave_LowerLevel
 .ltorg
.global __wrap_EncounterEffect_ChampionCynthia
.type __wrap_EncounterEffect_ChampionCynthia,%function
__wrap_EncounterEffect_ChampionCynthia:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag218
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_ChampionCynthia
 .ltorg
.global __wrap_EncounterEffect_Double
.type __wrap_EncounterEffect_Double,%function
__wrap_EncounterEffect_Double:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag219
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Double
 .ltorg
.global __wrap_EncounterEffect_EliteFourAaron
.type __wrap_EncounterEffect_EliteFourAaron,%function
__wrap_EncounterEffect_EliteFourAaron:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag220
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_EliteFourAaron
 .ltorg
.global __wrap_EncounterEffect_EliteFourBertha
.type __wrap_EncounterEffect_EliteFourBertha,%function
__wrap_EncounterEffect_EliteFourBertha:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag221
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_EliteFourBertha
 .ltorg
.global __wrap_EncounterEffect_EliteFourFlint
.type __wrap_EncounterEffect_EliteFourFlint,%function
__wrap_EncounterEffect_EliteFourFlint:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag222
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_EliteFourFlint
 .ltorg
.global __wrap_EncounterEffect_EliteFourLucian
.type __wrap_EncounterEffect_EliteFourLucian,%function
__wrap_EncounterEffect_EliteFourLucian:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag223
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_EliteFourLucian
 .ltorg
.global __wrap_EncounterEffect_Finish
.type __wrap_EncounterEffect_Finish,%function
__wrap_EncounterEffect_Finish:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag224
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Finish
 .ltorg
.global __wrap_EncounterEffect_Flash
.type __wrap_EncounterEffect_Flash,%function
__wrap_EncounterEffect_Flash:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag225
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Flash
 .ltorg
.global __wrap_EncounterEffect_Frontier
.type __wrap_EncounterEffect_Frontier,%function
__wrap_EncounterEffect_Frontier:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag226
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Frontier
 .ltorg
.global __wrap_EncounterEffect_GalacticBoss
.type __wrap_EncounterEffect_GalacticBoss,%function
__wrap_EncounterEffect_GalacticBoss:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag227
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_GalacticBoss
 .ltorg
.global __wrap_EncounterEffect_GalacticGrunt
.type __wrap_EncounterEffect_GalacticGrunt,%function
__wrap_EncounterEffect_GalacticGrunt:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag228
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_GalacticGrunt
 .ltorg
.global __wrap_EncounterEffect_GetHBlankFlag
.type __wrap_EncounterEffect_GetHBlankFlag,%function
__wrap_EncounterEffect_GetHBlankFlag:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag229
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_GetHBlankFlag
 .ltorg
.global __wrap_EncounterEffect_Grass_HigherLevel
.type __wrap_EncounterEffect_Grass_HigherLevel,%function
__wrap_EncounterEffect_Grass_HigherLevel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag230
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Grass_HigherLevel
 .ltorg
.global __wrap_EncounterEffect_Grass_LowerLevel
.type __wrap_EncounterEffect_Grass_LowerLevel,%function
__wrap_EncounterEffect_Grass_LowerLevel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag231
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Grass_LowerLevel
 .ltorg
.global __wrap_EncounterEffect_InitSpriteCollection
.type __wrap_EncounterEffect_InitSpriteCollection,%function
__wrap_EncounterEffect_InitSpriteCollection:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag232
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_InitSpriteCollection
 .ltorg
.global __wrap_EncounterEffect_LeaderByron
.type __wrap_EncounterEffect_LeaderByron,%function
__wrap_EncounterEffect_LeaderByron:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag233
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_LeaderByron
 .ltorg
.global __wrap_EncounterEffect_LeaderCandice
.type __wrap_EncounterEffect_LeaderCandice,%function
__wrap_EncounterEffect_LeaderCandice:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag234
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_LeaderCandice
 .ltorg
.global __wrap_EncounterEffect_LeaderFantina
.type __wrap_EncounterEffect_LeaderFantina,%function
__wrap_EncounterEffect_LeaderFantina:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag235
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_LeaderFantina
 .ltorg
.global __wrap_EncounterEffect_LeaderGardenia
.type __wrap_EncounterEffect_LeaderGardenia,%function
__wrap_EncounterEffect_LeaderGardenia:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag236
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_LeaderGardenia
 .ltorg
.global __wrap_EncounterEffect_LeaderMaylene
.type __wrap_EncounterEffect_LeaderMaylene,%function
__wrap_EncounterEffect_LeaderMaylene:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag237
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_LeaderMaylene
 .ltorg
.global __wrap_EncounterEffect_LeaderRoark
.type __wrap_EncounterEffect_LeaderRoark,%function
__wrap_EncounterEffect_LeaderRoark:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag238
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_LeaderRoark
 .ltorg
.global __wrap_EncounterEffect_LeaderVolkner
.type __wrap_EncounterEffect_LeaderVolkner,%function
__wrap_EncounterEffect_LeaderVolkner:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag239
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_LeaderVolkner
 .ltorg
.global __wrap_EncounterEffect_LeaderWake
.type __wrap_EncounterEffect_LeaderWake,%function
__wrap_EncounterEffect_LeaderWake:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag240
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_LeaderWake
 .ltorg
.global __wrap_EncounterEffect_Legendary
.type __wrap_EncounterEffect_Legendary,%function
__wrap_EncounterEffect_Legendary:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag241
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Legendary
 .ltorg
.global __wrap_EncounterEffect_Mythical
.type __wrap_EncounterEffect_Mythical,%function
__wrap_EncounterEffect_Mythical:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag242
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Mythical
 .ltorg
.global __wrap_EncounterEffect_ScreenSlice
.type __wrap_EncounterEffect_ScreenSlice,%function
__wrap_EncounterEffect_ScreenSlice:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag243
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_ScreenSlice
 .ltorg
.global __wrap_EncounterEffect_ScreenSplit
.type __wrap_EncounterEffect_ScreenSplit,%function
__wrap_EncounterEffect_ScreenSplit:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag244
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_ScreenSplit
 .ltorg
.global __wrap_EncounterEffect_Trainer_Cave_HigherLevel
.type __wrap_EncounterEffect_Trainer_Cave_HigherLevel,%function
__wrap_EncounterEffect_Trainer_Cave_HigherLevel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag245
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Trainer_Cave_HigherLevel
 .ltorg
.global __wrap_EncounterEffect_Trainer_Cave_LowerLevel
.type __wrap_EncounterEffect_Trainer_Cave_LowerLevel,%function
__wrap_EncounterEffect_Trainer_Cave_LowerLevel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag246
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Trainer_Cave_LowerLevel
 .ltorg
.global __wrap_EncounterEffect_Trainer_Grass_HigherLevel
.type __wrap_EncounterEffect_Trainer_Grass_HigherLevel,%function
__wrap_EncounterEffect_Trainer_Grass_HigherLevel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag247
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Trainer_Grass_HigherLevel
 .ltorg
.global __wrap_EncounterEffect_Trainer_Grass_LowerLevel
.type __wrap_EncounterEffect_Trainer_Grass_LowerLevel,%function
__wrap_EncounterEffect_Trainer_Grass_LowerLevel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag248
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Trainer_Grass_LowerLevel
 .ltorg
.global __wrap_EncounterEffect_Trainer_Water_HigherLevel
.type __wrap_EncounterEffect_Trainer_Water_HigherLevel,%function
__wrap_EncounterEffect_Trainer_Water_HigherLevel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag249
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Trainer_Water_HigherLevel
 .ltorg
.global __wrap_EncounterEffect_Trainer_Water_LowerLevel
.type __wrap_EncounterEffect_Trainer_Water_LowerLevel,%function
__wrap_EncounterEffect_Trainer_Water_LowerLevel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag250
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Trainer_Water_LowerLevel
 .ltorg
.global __wrap_EncounterEffect_Water_HigherLevel
.type __wrap_EncounterEffect_Water_HigherLevel,%function
__wrap_EncounterEffect_Water_HigherLevel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag251
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Water_HigherLevel
 .ltorg
.global __wrap_EncounterEffect_Water_LowerLevel
.type __wrap_EncounterEffect_Water_LowerLevel,%function
__wrap_EncounterEffect_Water_LowerLevel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag252
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_EncounterEffect_Water_LowerLevel
 .ltorg
.global __wrap_Encounter_NewVsPalParkTransfer
.type __wrap_Encounter_NewVsPalParkTransfer,%function
__wrap_Encounter_NewVsPalParkTransfer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag253
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Encounter_NewVsPalParkTransfer
 .ltorg
.global __wrap_Encounter_StartVsWild
.type __wrap_Encounter_StartVsWild,%function
__wrap_Encounter_StartVsWild:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag254
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Encounter_StartVsWild
 .ltorg
.global __wrap_ErrorHandling_AssertFail
.type __wrap_ErrorHandling_AssertFail,%function
__wrap_ErrorHandling_AssertFail:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag255
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ErrorHandling_AssertFail
 .ltorg
.global __wrap_FS_CloseFile
.type __wrap_FS_CloseFile,%function
__wrap_FS_CloseFile:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag256
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FS_CloseFile
 .ltorg
.global __wrap_FS_InitFile
.type __wrap_FS_InitFile,%function
__wrap_FS_InitFile:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag257
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FS_InitFile
 .ltorg
.global __wrap_FS_OpenFile
.type __wrap_FS_OpenFile,%function
__wrap_FS_OpenFile:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag258
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FS_OpenFile
 .ltorg
.global __wrap_FS_ReadFile
.type __wrap_FS_ReadFile,%function
__wrap_FS_ReadFile:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag259
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FS_ReadFile
 .ltorg
.global __wrap_FieldBGM_GetEffective
.type __wrap_FieldBGM_GetEffective,%function
__wrap_FieldBGM_GetEffective:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag260
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldBGM_GetEffective
 .ltorg
.global __wrap_FieldBGM_GetForMapHeader
.type __wrap_FieldBGM_GetForMapHeader,%function
__wrap_FieldBGM_GetForMapHeader:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag261
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldBGM_GetForMapHeader
 .ltorg
.global __wrap_FieldBGM_PlayEffectiveForMapHeader
.type __wrap_FieldBGM_PlayEffectiveForMapHeader,%function
__wrap_FieldBGM_PlayEffectiveForMapHeader:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag262
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldBGM_PlayEffectiveForMapHeader
 .ltorg
.global __wrap_FieldBGM_SetOverride
.type __wrap_FieldBGM_SetOverride,%function
__wrap_FieldBGM_SetOverride:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag263
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldBGM_SetOverride
 .ltorg
.global __wrap_FieldBGM_TryFadeOut
.type __wrap_FieldBGM_TryFadeOut,%function
__wrap_FieldBGM_TryFadeOut:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag264
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldBGM_TryFadeOut
 .ltorg
.global __wrap_FieldBattleDTO_Free
.type __wrap_FieldBattleDTO_Free,%function
__wrap_FieldBattleDTO_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag265
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldBattleDTO_Free
 .ltorg
.global __wrap_FieldCamera_Create
.type __wrap_FieldCamera_Create,%function
__wrap_FieldCamera_Create:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag266
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldCamera_Create
 .ltorg
.global __wrap_FieldCamera_Delete
.type __wrap_FieldCamera_Delete,%function
__wrap_FieldCamera_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag267
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldCamera_Delete
 .ltorg
.global __wrap_FieldCommManager_GetCurrentOccupiedSecretBase
.type __wrap_FieldCommManager_GetCurrentOccupiedSecretBase,%function
__wrap_FieldCommManager_GetCurrentOccupiedSecretBase:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag268
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldCommManager_GetCurrentOccupiedSecretBase
 .ltorg
.global __wrap_FieldCommManager_IsInMovementState
.type __wrap_FieldCommManager_IsInMovementState,%function
__wrap_FieldCommManager_IsInMovementState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag269
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldCommManager_IsInMovementState
 .ltorg
.global __wrap_FieldCommManager_PauseUndergroundResources
.type __wrap_FieldCommManager_PauseUndergroundResources,%function
__wrap_FieldCommManager_PauseUndergroundResources:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag270
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldCommManager_PauseUndergroundResources
 .ltorg
.global __wrap_FieldCommManager_UnpauseUndergroundResources
.type __wrap_FieldCommManager_UnpauseUndergroundResources,%function
__wrap_FieldCommManager_UnpauseUndergroundResources:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag271
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldCommManager_UnpauseUndergroundResources
 .ltorg
.global __wrap_FieldEffectManager_ApplyAnimCopyToModel
.type __wrap_FieldEffectManager_ApplyAnimCopyToModel,%function
__wrap_FieldEffectManager_ApplyAnimCopyToModel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag272
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffectManager_ApplyAnimCopyToModel
 .ltorg
.global __wrap_FieldEffectManager_FinishAnimManager
.type __wrap_FieldEffectManager_FinishAnimManager,%function
__wrap_FieldEffectManager_FinishAnimManager:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag273
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffectManager_FinishAnimManager
 .ltorg
.global __wrap_FieldEffectManager_Free
.type __wrap_FieldEffectManager_Free,%function
__wrap_FieldEffectManager_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag274
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffectManager_Free
 .ltorg
.global __wrap_FieldEffectManager_GetFieldSystem
.type __wrap_FieldEffectManager_GetFieldSystem,%function
__wrap_FieldEffectManager_GetFieldSystem:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag275
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffectManager_GetFieldSystem
 .ltorg
.global __wrap_FieldEffectManager_GetRendererContext
.type __wrap_FieldEffectManager_GetRendererContext,%function
__wrap_FieldEffectManager_GetRendererContext:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag276
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffectManager_GetRendererContext
 .ltorg
.global __wrap_FieldEffectManager_HasRenderer
.type __wrap_FieldEffectManager_HasRenderer,%function
__wrap_FieldEffectManager_HasRenderer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag277
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffectManager_HasRenderer
 .ltorg
.global __wrap_FieldEffectManager_HeapAlloc
.type __wrap_FieldEffectManager_HeapAlloc,%function
__wrap_FieldEffectManager_HeapAlloc:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag278
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffectManager_HeapAlloc
 .ltorg
.global __wrap_FieldEffectManager_HeapAllocInit
.type __wrap_FieldEffectManager_HeapAllocInit,%function
__wrap_FieldEffectManager_HeapAllocInit:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag279
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffectManager_HeapAllocInit
 .ltorg
.global __wrap_FieldEffectManager_HeapFree
.type __wrap_FieldEffectManager_HeapFree,%function
__wrap_FieldEffectManager_HeapFree:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag280
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffectManager_HeapFree
 .ltorg
.global __wrap_FieldEffectManager_InitAnimManager
.type __wrap_FieldEffectManager_InitAnimManager,%function
__wrap_FieldEffectManager_InitAnimManager:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag281
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffectManager_InitAnimManager
 .ltorg
.global __wrap_FieldEffectManager_InitAnimManagerList
.type __wrap_FieldEffectManager_InitAnimManagerList,%function
__wrap_FieldEffectManager_InitAnimManagerList:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag282
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffectManager_InitAnimManagerList
 .ltorg
.global __wrap_FieldEffectManager_InitRenderer
.type __wrap_FieldEffectManager_InitRenderer,%function
__wrap_FieldEffectManager_InitRenderer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag283
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffectManager_InitRenderer
 .ltorg
.global __wrap_FieldEffectManager_InitRenderers
.type __wrap_FieldEffectManager_InitRenderers,%function
__wrap_FieldEffectManager_InitRenderers:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag284
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffectManager_InitRenderers
 .ltorg
.global __wrap_FieldEffectManager_LoadAnimation
.type __wrap_FieldEffectManager_LoadAnimation,%function
__wrap_FieldEffectManager_LoadAnimation:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag285
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffectManager_LoadAnimation
 .ltorg
.global __wrap_FieldEffectManager_LoadModel
.type __wrap_FieldEffectManager_LoadModel,%function
__wrap_FieldEffectManager_LoadModel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag286
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffectManager_LoadModel
 .ltorg
.global __wrap_FieldEffectManager_New
.type __wrap_FieldEffectManager_New,%function
__wrap_FieldEffectManager_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag287
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffectManager_New
 .ltorg
.global __wrap_FieldEffectManager_Render
.type __wrap_FieldEffectManager_Render,%function
__wrap_FieldEffectManager_Render:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag288
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffectManager_Render
 .ltorg
.global __wrap_FieldEffect_InitRenderObject
.type __wrap_FieldEffect_InitRenderObject,%function
__wrap_FieldEffect_InitRenderObject:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag289
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEffect_InitRenderObject
 .ltorg
.global __wrap_FieldEvent_GetInteractedBgEventScript
.type __wrap_FieldEvent_GetInteractedBgEventScript,%function
__wrap_FieldEvent_GetInteractedBgEventScript:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag290
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEvent_GetInteractedBgEventScript
 .ltorg
.global __wrap_FieldEvent_GetInteractedWallSignScript
.type __wrap_FieldEvent_GetInteractedWallSignScript,%function
__wrap_FieldEvent_GetInteractedWallSignScript:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag291
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldEvent_GetInteractedWallSignScript
 .ltorg
.global __wrap_FieldMapChange_Set3DDisplay
.type __wrap_FieldMapChange_Set3DDisplay,%function
__wrap_FieldMapChange_Set3DDisplay:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag292
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldMapChange_Set3DDisplay
 .ltorg
.global __wrap_FieldMapChange_UpdateGameData
.type __wrap_FieldMapChange_UpdateGameData,%function
__wrap_FieldMapChange_UpdateGameData:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag293
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldMapChange_UpdateGameData
 .ltorg
.global __wrap_FieldMapChange_UpdateGameDataDistortionWorld
.type __wrap_FieldMapChange_UpdateGameDataDistortionWorld,%function
__wrap_FieldMapChange_UpdateGameDataDistortionWorld:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag294
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldMapChange_UpdateGameDataDistortionWorld
 .ltorg
.global __wrap_FieldMap_FadeScreen
.type __wrap_FieldMap_FadeScreen,%function
__wrap_FieldMap_FadeScreen:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag295
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldMap_FadeScreen
 .ltorg
.global __wrap_FieldMenuManager_DeleteMoveTutorCost
.type __wrap_FieldMenuManager_DeleteMoveTutorCost,%function
__wrap_FieldMenuManager_DeleteMoveTutorCost:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag296
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldMenuManager_DeleteMoveTutorCost
 .ltorg
.global __wrap_FieldMenuManager_NewMoveTutorCostWindow
.type __wrap_FieldMenuManager_NewMoveTutorCostWindow,%function
__wrap_FieldMenuManager_NewMoveTutorCostWindow:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag297
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldMenuManager_NewMoveTutorCostWindow
 .ltorg
.global __wrap_FieldMessage_AddSignpostWindow
.type __wrap_FieldMessage_AddSignpostWindow,%function
__wrap_FieldMessage_AddSignpostWindow:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag298
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldMessage_AddSignpostWindow
 .ltorg
.global __wrap_FieldMessage_AddWindow
.type __wrap_FieldMessage_AddWindow,%function
__wrap_FieldMessage_AddWindow:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag299
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldMessage_AddWindow
 .ltorg
.global __wrap_FieldMessage_DrawWindow
.type __wrap_FieldMessage_DrawWindow,%function
__wrap_FieldMessage_DrawWindow:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag300
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldMessage_DrawWindow
 .ltorg
.global __wrap_FieldMessage_FinishedPrinting
.type __wrap_FieldMessage_FinishedPrinting,%function
__wrap_FieldMessage_FinishedPrinting:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag301
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldMessage_FinishedPrinting
 .ltorg
.global __wrap_FieldMessage_LoadAndDrawSignpost
.type __wrap_FieldMessage_LoadAndDrawSignpost,%function
__wrap_FieldMessage_LoadAndDrawSignpost:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag302
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldMessage_LoadAndDrawSignpost
 .ltorg
.global __wrap_FieldMessage_LoadTextPalettes
.type __wrap_FieldMessage_LoadTextPalettes,%function
__wrap_FieldMessage_LoadTextPalettes:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag303
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldMessage_LoadTextPalettes
 .ltorg
.global __wrap_FieldMessage_Print
.type __wrap_FieldMessage_Print,%function
__wrap_FieldMessage_Print:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag304
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldMessage_Print
 .ltorg
.global __wrap_FieldMessage_PrintWithParams
.type __wrap_FieldMessage_PrintWithParams,%function
__wrap_FieldMessage_PrintWithParams:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag305
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldMessage_PrintWithParams
 .ltorg
.global __wrap_FieldMotionBlur_Start
.type __wrap_FieldMotionBlur_Start,%function
__wrap_FieldMotionBlur_Start:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag306
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldMotionBlur_Start
 .ltorg
.global __wrap_FieldMotionBlur_Stop
.type __wrap_FieldMotionBlur_Stop,%function
__wrap_FieldMotionBlur_Stop:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag307
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldMotionBlur_Stop
 .ltorg
.global __wrap_FieldOverworldState_GetCameraType
.type __wrap_FieldOverworldState_GetCameraType,%function
__wrap_FieldOverworldState_GetCameraType:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag308
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldOverworldState_GetCameraType
 .ltorg
.global __wrap_FieldOverworldState_GetEntranceLocation
.type __wrap_FieldOverworldState_GetEntranceLocation,%function
__wrap_FieldOverworldState_GetEntranceLocation:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag309
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldOverworldState_GetEntranceLocation
 .ltorg
.global __wrap_FieldOverworldState_GetExitLocation
.type __wrap_FieldOverworldState_GetExitLocation,%function
__wrap_FieldOverworldState_GetExitLocation:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag310
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldOverworldState_GetExitLocation
 .ltorg
.global __wrap_FieldOverworldState_GetMapHistory
.type __wrap_FieldOverworldState_GetMapHistory,%function
__wrap_FieldOverworldState_GetMapHistory:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag311
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldOverworldState_GetMapHistory
 .ltorg
.global __wrap_FieldOverworldState_GetPlayerLocation
.type __wrap_FieldOverworldState_GetPlayerLocation,%function
__wrap_FieldOverworldState_GetPlayerLocation:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag312
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldOverworldState_GetPlayerLocation
 .ltorg
.global __wrap_FieldOverworldState_GetPoisonStepCount
.type __wrap_FieldOverworldState_GetPoisonStepCount,%function
__wrap_FieldOverworldState_GetPoisonStepCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag313
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldOverworldState_GetPoisonStepCount
 .ltorg
.global __wrap_FieldOverworldState_GetSafariBallCount
.type __wrap_FieldOverworldState_GetSafariBallCount,%function
__wrap_FieldOverworldState_GetSafariBallCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag314
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldOverworldState_GetSafariBallCount
 .ltorg
.global __wrap_FieldOverworldState_GetSafariStepCount
.type __wrap_FieldOverworldState_GetSafariStepCount,%function
__wrap_FieldOverworldState_GetSafariStepCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag315
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldOverworldState_GetSafariStepCount
 .ltorg
.global __wrap_FieldOverworldState_GetSpecialLocation
.type __wrap_FieldOverworldState_GetSpecialLocation,%function
__wrap_FieldOverworldState_GetSpecialLocation:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag316
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldOverworldState_GetSpecialLocation
 .ltorg
.global __wrap_FieldOverworldState_GetWeather
.type __wrap_FieldOverworldState_GetWeather,%function
__wrap_FieldOverworldState_GetWeather:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag317
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldOverworldState_GetWeather
 .ltorg
.global __wrap_FieldSystem_CheckForTrainersWantingBattle
.type __wrap_FieldSystem_CheckForTrainersWantingBattle,%function
__wrap_FieldSystem_CheckForTrainersWantingBattle:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag318
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_CheckForTrainersWantingBattle
 .ltorg
.global __wrap_FieldSystem_CreateTask
.type __wrap_FieldSystem_CreateTask,%function
__wrap_FieldSystem_CreateTask:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag319
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_CreateTask
 .ltorg
.global __wrap_FieldSystem_EndBottomScreen
.type __wrap_FieldSystem_EndBottomScreen,%function
__wrap_FieldSystem_EndBottomScreen:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag320
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_EndBottomScreen
 .ltorg
.global __wrap_FieldSystem_EndVsSeekerTask
.type __wrap_FieldSystem_EndVsSeekerTask,%function
__wrap_FieldSystem_EndVsSeekerTask:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag321
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_EndVsSeekerTask
 .ltorg
.global __wrap_FieldSystem_FindCollidingLoadedMapPropByModelID
.type __wrap_FieldSystem_FindCollidingLoadedMapPropByModelID,%function
__wrap_FieldSystem_FindCollidingLoadedMapPropByModelID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag322
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_FindCollidingLoadedMapPropByModelID
 .ltorg
.global __wrap_FieldSystem_FindCollidingLoadedMapPropByModelIDs
.type __wrap_FieldSystem_FindCollidingLoadedMapPropByModelIDs,%function
__wrap_FieldSystem_FindCollidingLoadedMapPropByModelIDs:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag323
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_FindCollidingLoadedMapPropByModelIDs
 .ltorg
.global __wrap_FieldSystem_GetDayOfMonth
.type __wrap_FieldSystem_GetDayOfMonth,%function
__wrap_FieldSystem_GetDayOfMonth:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag324
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_GetDayOfMonth
 .ltorg
.global __wrap_FieldSystem_GetMonth
.type __wrap_FieldSystem_GetMonth,%function
__wrap_FieldSystem_GetMonth:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag325
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_GetMonth
 .ltorg
.global __wrap_FieldSystem_GetSaveData
.type __wrap_FieldSystem_GetSaveData,%function
__wrap_FieldSystem_GetSaveData:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag326
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_GetSaveData
 .ltorg
.global __wrap_FieldSystem_GetScriptMemberPtr
.type __wrap_FieldSystem_GetScriptMemberPtr,%function
__wrap_FieldSystem_GetScriptMemberPtr:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag327
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_GetScriptMemberPtr
 .ltorg
.global __wrap_FieldSystem_GetVarPointer
.type __wrap_FieldSystem_GetVarPointer,%function
__wrap_FieldSystem_GetVarPointer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag328
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_GetVarPointer
 .ltorg
.global __wrap_FieldSystem_HasPenalty
.type __wrap_FieldSystem_HasPenalty,%function
__wrap_FieldSystem_HasPenalty:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag329
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_HasPenalty
 .ltorg
.global __wrap_FieldSystem_InitBottomScreen
.type __wrap_FieldSystem_InitBottomScreen,%function
__wrap_FieldSystem_InitBottomScreen:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag330
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_InitBottomScreen
 .ltorg
.global __wrap_FieldSystem_IsBottomScreenDone
.type __wrap_FieldSystem_IsBottomScreenDone,%function
__wrap_FieldSystem_IsBottomScreenDone:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag331
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_IsBottomScreenDone
 .ltorg
.global __wrap_FieldSystem_IsBottomScreenRunningDummy
.type __wrap_FieldSystem_IsBottomScreenRunningDummy,%function
__wrap_FieldSystem_IsBottomScreenRunningDummy:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag332
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_IsBottomScreenRunningDummy
 .ltorg
.global __wrap_FieldSystem_IsInValidLocation
.type __wrap_FieldSystem_IsInValidLocation,%function
__wrap_FieldSystem_IsInValidLocation:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag333
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_IsInValidLocation
 .ltorg
.global __wrap_FieldSystem_IsRunningFieldMapInner
.type __wrap_FieldSystem_IsRunningFieldMapInner,%function
__wrap_FieldSystem_IsRunningFieldMapInner:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag334
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_IsRunningFieldMapInner
 .ltorg
.global __wrap_FieldSystem_IsRunningTask
.type __wrap_FieldSystem_IsRunningTask,%function
__wrap_FieldSystem_IsRunningTask:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag335
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_IsRunningTask
 .ltorg
.global __wrap_FieldSystem_ResumeProcessing
.type __wrap_FieldSystem_ResumeProcessing,%function
__wrap_FieldSystem_ResumeProcessing:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag336
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_ResumeProcessing
 .ltorg
.global __wrap_FieldSystem_RunInitScript
.type __wrap_FieldSystem_RunInitScript,%function
__wrap_FieldSystem_RunInitScript:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag337
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_RunInitScript
 .ltorg
.global __wrap_FieldSystem_SaveObjects
.type __wrap_FieldSystem_SaveObjects,%function
__wrap_FieldSystem_SaveObjects:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag338
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_SaveObjects
 .ltorg
.global __wrap_FieldSystem_SaveTVSegment_WhatsFishing
.type __wrap_FieldSystem_SaveTVSegment_WhatsFishing,%function
__wrap_FieldSystem_SaveTVSegment_WhatsFishing:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag339
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_SaveTVSegment_WhatsFishing
 .ltorg
.global __wrap_FieldSystem_SendPoketchEvent
.type __wrap_FieldSystem_SendPoketchEvent,%function
__wrap_FieldSystem_SendPoketchEvent:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag340
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_SendPoketchEvent
 .ltorg
.global __wrap_FieldSystem_StartMapChangeWarpTask
.type __wrap_FieldSystem_StartMapChangeWarpTask,%function
__wrap_FieldSystem_StartMapChangeWarpTask:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag341
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_StartMapChangeWarpTask
 .ltorg
.global __wrap_FieldSystem_StartVsSeekerTask
.type __wrap_FieldSystem_StartVsSeekerTask,%function
__wrap_FieldSystem_StartVsSeekerTask:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag342
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_StartVsSeekerTask
 .ltorg
.global __wrap_FieldSystem_TryGetVar
.type __wrap_FieldSystem_TryGetVar,%function
__wrap_FieldSystem_TryGetVar:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag343
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_TryGetVar
 .ltorg
.global __wrap_FieldSystem_TrySetVillaFurnitureScript
.type __wrap_FieldSystem_TrySetVillaFurnitureScript,%function
__wrap_FieldSystem_TrySetVillaFurnitureScript:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag344
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldSystem_TrySetVillaFurnitureScript
 .ltorg
.global __wrap_FieldTask_GetEnv
.type __wrap_FieldTask_GetEnv,%function
__wrap_FieldTask_GetEnv:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag345
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldTask_GetEnv
 .ltorg
.global __wrap_FieldTask_GetFieldSystem
.type __wrap_FieldTask_GetFieldSystem,%function
__wrap_FieldTask_GetFieldSystem:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag346
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldTask_GetFieldSystem
 .ltorg
.global __wrap_FieldTask_GetState
.type __wrap_FieldTask_GetState,%function
__wrap_FieldTask_GetState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag347
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldTask_GetState
 .ltorg
.global __wrap_FieldTask_InitCall
.type __wrap_FieldTask_InitCall,%function
__wrap_FieldTask_InitCall:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag348
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FieldTask_InitCall
 .ltorg
.global __wrap_Field_DoPoisonEffect
.type __wrap_Field_DoPoisonEffect,%function
__wrap_Field_DoPoisonEffect:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag349
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Field_DoPoisonEffect
 .ltorg
.global __wrap_FogManager_ApplyColor
.type __wrap_FogManager_ApplyColor,%function
__wrap_FogManager_ApplyColor:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag350
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FogManager_ApplyColor
 .ltorg
.global __wrap_FogManager_ApplyDensityTable
.type __wrap_FogManager_ApplyDensityTable,%function
__wrap_FogManager_ApplyDensityTable:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag351
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FogManager_ApplyDensityTable
 .ltorg
.global __wrap_FogManager_ApplyParameters
.type __wrap_FogManager_ApplyParameters,%function
__wrap_FogManager_ApplyParameters:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag352
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FogManager_ApplyParameters
 .ltorg
.global __wrap_FogManager_Free
.type __wrap_FogManager_Free,%function
__wrap_FogManager_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag353
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FogManager_Free
 .ltorg
.global __wrap_FogManager_GetColor
.type __wrap_FogManager_GetColor,%function
__wrap_FogManager_GetColor:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag354
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FogManager_GetColor
 .ltorg
.global __wrap_FogManager_GetOffset
.type __wrap_FogManager_GetOffset,%function
__wrap_FogManager_GetOffset:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag355
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FogManager_GetOffset
 .ltorg
.global __wrap_FogManager_GetSlope
.type __wrap_FogManager_GetSlope,%function
__wrap_FogManager_GetSlope:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag356
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FogManager_GetSlope
 .ltorg
.global __wrap_FogManager_IsEnabled
.type __wrap_FogManager_IsEnabled,%function
__wrap_FogManager_IsEnabled:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag357
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FogManager_IsEnabled
 .ltorg
.global __wrap_FogManager_New
.type __wrap_FogManager_New,%function
__wrap_FogManager_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag358
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_FogManager_New
 .ltorg
.global __wrap_Font_CalcStringWidth
.type __wrap_Font_CalcStringWidth,%function
__wrap_Font_CalcStringWidth:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag359
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Font_CalcStringWidth
 .ltorg
.global __wrap_Font_CalcStringWidthWithCursorControl
.type __wrap_Font_CalcStringWidthWithCursorControl,%function
__wrap_Font_CalcStringWidthWithCursorControl:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag360
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Font_CalcStringWidthWithCursorControl
 .ltorg
.global __wrap_Font_GetAttribute
.type __wrap_Font_GetAttribute,%function
__wrap_Font_GetAttribute:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag361
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Font_GetAttribute
 .ltorg
.global __wrap_G2x_SetBGyAffine_
.type __wrap_G2x_SetBGyAffine_,%function
__wrap_G2x_SetBGyAffine_:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag362
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_G2x_SetBGyAffine_
 .ltorg
.global __wrap_G2x_SetBlendAlpha_
.type __wrap_G2x_SetBlendAlpha_,%function
__wrap_G2x_SetBlendAlpha_:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag363
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_G2x_SetBlendAlpha_
 .ltorg
.global __wrap_G3X_SetEdgeColorTable
.type __wrap_G3X_SetEdgeColorTable,%function
__wrap_G3X_SetEdgeColorTable:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag364
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_G3X_SetEdgeColorTable
 .ltorg
.global __wrap_G3X_SetFog
.type __wrap_G3X_SetFog,%function
__wrap_G3X_SetFog:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag365
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_G3X_SetFog
 .ltorg
.global __wrap_G3X_SetFogTable
.type __wrap_G3X_SetFogTable,%function
__wrap_G3X_SetFogTable:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag366
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_G3X_SetFogTable
 .ltorg
.global __wrap_G3X_SetHOffset
.type __wrap_G3X_SetHOffset,%function
__wrap_G3X_SetHOffset:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag367
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_G3X_SetHOffset
 .ltorg
.global __wrap_G3_RequestSwapBuffers
.type __wrap_G3_RequestSwapBuffers,%function
__wrap_G3_RequestSwapBuffers:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag368
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_G3_RequestSwapBuffers
 .ltorg
.global __wrap_G3_ResetG3X
.type __wrap_G3_ResetG3X,%function
__wrap_G3_ResetG3X:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag369
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_G3_ResetG3X
 .ltorg
.global __wrap_GFXBoxTest_IsModelInView
.type __wrap_GFXBoxTest_IsModelInView,%function
__wrap_GFXBoxTest_IsModelInView:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag370
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GFXBoxTest_IsModelInView
 .ltorg
.global __wrap_GXLayers_EngineAToggleLayers
.type __wrap_GXLayers_EngineAToggleLayers,%function
__wrap_GXLayers_EngineAToggleLayers:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag371
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GXLayers_EngineAToggleLayers
 .ltorg
.global __wrap_GXLayers_SetBanks
.type __wrap_GXLayers_SetBanks,%function
__wrap_GXLayers_SetBanks:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag372
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GXLayers_SetBanks
 .ltorg
.global __wrap_GXLayers_SwapDisplay
.type __wrap_GXLayers_SwapDisplay,%function
__wrap_GXLayers_SwapDisplay:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag373
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GXLayers_SwapDisplay
 .ltorg
.global __wrap_GXLayers_TurnBothDispOn
.type __wrap_GXLayers_TurnBothDispOn,%function
__wrap_GXLayers_TurnBothDispOn:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag374
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GXLayers_TurnBothDispOn
 .ltorg
.global __wrap_GX_GetBankForLCDC
.type __wrap_GX_GetBankForLCDC,%function
__wrap_GX_GetBankForLCDC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag375
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GX_GetBankForLCDC
 .ltorg
.global __wrap_GX_LoadBGPltt
.type __wrap_GX_LoadBGPltt,%function
__wrap_GX_LoadBGPltt:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag376
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GX_LoadBGPltt
 .ltorg
.global __wrap_GX_LoadOBJPltt
.type __wrap_GX_LoadOBJPltt,%function
__wrap_GX_LoadOBJPltt:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag377
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GX_LoadOBJPltt
 .ltorg
.global __wrap_GX_ResetBankForBG
.type __wrap_GX_ResetBankForBG,%function
__wrap_GX_ResetBankForBG:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag378
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GX_ResetBankForBG
 .ltorg
.global __wrap_GX_SetBankForBG
.type __wrap_GX_SetBankForBG,%function
__wrap_GX_SetBankForBG:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag379
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GX_SetBankForBG
 .ltorg
.global __wrap_GX_SetBankForLCDC
.type __wrap_GX_SetBankForLCDC,%function
__wrap_GX_SetBankForLCDC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag380
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GX_SetBankForLCDC
 .ltorg
.global __wrap_GX_SetGraphicsMode
.type __wrap_GX_SetGraphicsMode,%function
__wrap_GX_SetGraphicsMode:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag381
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GX_SetGraphicsMode
 .ltorg
.global __wrap_GXx_SetMasterBrightness_
.type __wrap_GXx_SetMasterBrightness_,%function
__wrap_GXx_SetMasterBrightness_:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag382
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GXx_SetMasterBrightness_
 .ltorg
.global __wrap_GameRecords_GetRecordValue
.type __wrap_GameRecords_GetRecordValue,%function
__wrap_GameRecords_GetRecordValue:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag383
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GameRecords_GetRecordValue
 .ltorg
.global __wrap_GameRecords_IncrementRecordValue
.type __wrap_GameRecords_IncrementRecordValue,%function
__wrap_GameRecords_IncrementRecordValue:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag384
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GameRecords_IncrementRecordValue
 .ltorg
.global __wrap_GameRecords_IncrementTrainerScore
.type __wrap_GameRecords_IncrementTrainerScore,%function
__wrap_GameRecords_IncrementTrainerScore:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag385
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GameRecords_IncrementTrainerScore
 .ltorg
.global __wrap_GetSecondsSinceMidnight
.type __wrap_GetSecondsSinceMidnight,%function
__wrap_GetSecondsSinceMidnight:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag386
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GetSecondsSinceMidnight
 .ltorg
.global __wrap_GetTextureDataBufferFromResourceName
.type __wrap_GetTextureDataBufferFromResourceName,%function
__wrap_GetTextureDataBufferFromResourceName:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag387
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GetTextureDataBufferFromResourceName
 .ltorg
.global __wrap_GetTextureDataVRAMBuffer
.type __wrap_GetTextureDataVRAMBuffer,%function
__wrap_GetTextureDataVRAMBuffer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag388
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GetTextureDataVRAMBuffer
 .ltorg
.global __wrap_GetTimeOfDay
.type __wrap_GetTimeOfDay,%function
__wrap_GetTimeOfDay:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag389
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_GetTimeOfDay
 .ltorg
.global __wrap_Good_GetDepth
.type __wrap_Good_GetDepth,%function
__wrap_Good_GetDepth:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag390
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Good_GetDepth
 .ltorg
.global __wrap_Good_GetWidth
.type __wrap_Good_GetWidth,%function
__wrap_Good_GetWidth:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag391
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Good_GetWidth
 .ltorg
.global __wrap_Graphics_GetCharData
.type __wrap_Graphics_GetCharData,%function
__wrap_Graphics_GetCharData:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag392
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Graphics_GetCharData
 .ltorg
.global __wrap_Graphics_GetPlttData
.type __wrap_Graphics_GetPlttData,%function
__wrap_Graphics_GetPlttData:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag393
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Graphics_GetPlttData
 .ltorg
.global __wrap_Graphics_GetScrnDataFromOpenNARC
.type __wrap_Graphics_GetScrnDataFromOpenNARC,%function
__wrap_Graphics_GetScrnDataFromOpenNARC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag394
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Graphics_GetScrnDataFromOpenNARC
 .ltorg
.global __wrap_Graphics_LoadPaletteFromOpenNARC
.type __wrap_Graphics_LoadPaletteFromOpenNARC,%function
__wrap_Graphics_LoadPaletteFromOpenNARC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag395
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Graphics_LoadPaletteFromOpenNARC
 .ltorg
.global __wrap_Graphics_LoadTilesToBgLayerFromOpenNARC
.type __wrap_Graphics_LoadTilesToBgLayerFromOpenNARC,%function
__wrap_Graphics_LoadTilesToBgLayerFromOpenNARC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag396
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Graphics_LoadTilesToBgLayerFromOpenNARC
 .ltorg
.global __wrap_HBlankSystem_Delete
.type __wrap_HBlankSystem_Delete,%function
__wrap_HBlankSystem_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag397
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_HBlankSystem_Delete
 .ltorg
.global __wrap_HBlankSystem_New
.type __wrap_HBlankSystem_New,%function
__wrap_HBlankSystem_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag398
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_HBlankSystem_New
 .ltorg
.global __wrap_HBlankSystem_Start
.type __wrap_HBlankSystem_Start,%function
__wrap_HBlankSystem_Start:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag399
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_HBlankSystem_Start
 .ltorg
.global __wrap_HBlankSystem_StartTask
.type __wrap_HBlankSystem_StartTask,%function
__wrap_HBlankSystem_StartTask:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag400
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_HBlankSystem_StartTask
 .ltorg
.global __wrap_HBlankSystem_Stop
.type __wrap_HBlankSystem_Stop,%function
__wrap_HBlankSystem_Stop:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag401
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_HBlankSystem_Stop
 .ltorg
.global __wrap_HBlankTask_Delete
.type __wrap_HBlankTask_Delete,%function
__wrap_HBlankTask_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag402
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_HBlankTask_Delete
 .ltorg
.global __wrap_HMCutIn_EndTask
.type __wrap_HMCutIn_EndTask,%function
__wrap_HMCutIn_EndTask:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag403
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_HMCutIn_EndTask
 .ltorg
.global __wrap_HMCutIn_IsFinished
.type __wrap_HMCutIn_IsFinished,%function
__wrap_HMCutIn_IsFinished:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag404
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_HMCutIn_IsFinished
 .ltorg
.global __wrap_HMCutIn_StartTask
.type __wrap_HMCutIn_StartTask,%function
__wrap_HMCutIn_StartTask:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag405
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_HMCutIn_StartTask
 .ltorg
.global __wrap_HeapExp_FndInitAllocator
.type __wrap_HeapExp_FndInitAllocator,%function
__wrap_HeapExp_FndInitAllocator:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag406
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_HeapExp_FndInitAllocator
 .ltorg
.global __wrap_Heap_Alloc
.type __wrap_Heap_Alloc,%function
__wrap_Heap_Alloc:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag407
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Heap_Alloc
 .ltorg
.global __wrap_Heap_AllocAtEnd
.type __wrap_Heap_AllocAtEnd,%function
__wrap_Heap_AllocAtEnd:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag408
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Heap_AllocAtEnd
 .ltorg
.global __wrap_Heap_Create
.type __wrap_Heap_Create,%function
__wrap_Heap_Create:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag409
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Heap_Create
 .ltorg
.global __wrap_Heap_Destroy
.type __wrap_Heap_Destroy,%function
__wrap_Heap_Destroy:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag410
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Heap_Destroy
 .ltorg
.global __wrap_Heap_Free
.type __wrap_Heap_Free,%function
__wrap_Heap_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag411
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Heap_Free
 .ltorg
.global __wrap_Heap_FreeExplicit
.type __wrap_Heap_FreeExplicit,%function
__wrap_Heap_FreeExplicit:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag412
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Heap_FreeExplicit
 .ltorg
.global __wrap_Heap_Realloc
.type __wrap_Heap_Realloc,%function
__wrap_Heap_Realloc:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag413
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Heap_Realloc
 .ltorg
.global __wrap_HearthomeGym_CheckIfEnteredIncorrectDoor
.type __wrap_HearthomeGym_CheckIfEnteredIncorrectDoor,%function
__wrap_HearthomeGym_CheckIfEnteredIncorrectDoor:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag414
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_HearthomeGym_CheckIfEnteredIncorrectDoor
 .ltorg
.global __wrap_HearthomeGym_CheckIfPlayerSeesTrainer
.type __wrap_HearthomeGym_CheckIfPlayerSeesTrainer,%function
__wrap_HearthomeGym_CheckIfPlayerSeesTrainer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag415
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_HearthomeGym_CheckIfPlayerSeesTrainer
 .ltorg
.global __wrap_HoneyTree_FreeShakeData
.type __wrap_HoneyTree_FreeShakeData,%function
__wrap_HoneyTree_FreeShakeData:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag416
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_HoneyTree_FreeShakeData
 .ltorg
.global __wrap_HoneyTree_ShakeDataInit
.type __wrap_HoneyTree_ShakeDataInit,%function
__wrap_HoneyTree_ShakeDataInit:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag417
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_HoneyTree_ShakeDataInit
 .ltorg
.global __wrap_HoneyTree_TryInteract
.type __wrap_HoneyTree_TryInteract,%function
__wrap_HoneyTree_TryInteract:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag418
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_HoneyTree_TryInteract
 .ltorg
.global __wrap_InitRenderer
.type __wrap_InitRenderer,%function
__wrap_InitRenderer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag419
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_InitRenderer
 .ltorg
.global __wrap_IsScreenFadeDone
.type __wrap_IsScreenFadeDone,%function
__wrap_IsScreenFadeDone:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag420
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_IsScreenFadeDone
 .ltorg
.global __wrap_Item_IsMail
.type __wrap_Item_IsMail,%function
__wrap_Item_IsMail:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag421
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Item_IsMail
 .ltorg
.global __wrap_Item_MoveForTMHM
.type __wrap_Item_MoveForTMHM,%function
__wrap_Item_MoveForTMHM:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag422
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Item_MoveForTMHM
 .ltorg
.global __wrap_LCRNG_Next
.type __wrap_LCRNG_Next,%function
__wrap_LCRNG_Next:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag423
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LCRNG_Next
 .ltorg
.global __wrap_LandDataManager_DistortionWorldSetOffsets
.type __wrap_LandDataManager_DistortionWorldSetOffsets,%function
__wrap_LandDataManager_DistortionWorldSetOffsets:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag424
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LandDataManager_DistortionWorldSetOffsets
 .ltorg
.global __wrap_LandDataManager_End
.type __wrap_LandDataManager_End,%function
__wrap_LandDataManager_End:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag425
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LandDataManager_End
 .ltorg
.global __wrap_LandDataManager_ForgetTrackedTarget
.type __wrap_LandDataManager_ForgetTrackedTarget,%function
__wrap_LandDataManager_ForgetTrackedTarget:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag426
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LandDataManager_ForgetTrackedTarget
 .ltorg
.global __wrap_LandDataManager_FreeNARCAndLoadedMapBuffers
.type __wrap_LandDataManager_FreeNARCAndLoadedMapBuffers,%function
__wrap_LandDataManager_FreeNARCAndLoadedMapBuffers:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag427
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LandDataManager_FreeNARCAndLoadedMapBuffers
 .ltorg
.global __wrap_LandDataManager_GetLoadedMapPropManager
.type __wrap_LandDataManager_GetLoadedMapPropManager,%function
__wrap_LandDataManager_GetLoadedMapPropManager:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag428
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LandDataManager_GetLoadedMapPropManager
 .ltorg
.global __wrap_LandDataManager_GetOffsetTileX
.type __wrap_LandDataManager_GetOffsetTileX,%function
__wrap_LandDataManager_GetOffsetTileX:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag429
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LandDataManager_GetOffsetTileX
 .ltorg
.global __wrap_LandDataManager_GetOffsetTileZ
.type __wrap_LandDataManager_GetOffsetTileZ,%function
__wrap_LandDataManager_GetOffsetTileZ:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag430
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LandDataManager_GetOffsetTileZ
 .ltorg
.global __wrap_LandDataManager_GetTrackedTargetLoadedMapsQuadrant
.type __wrap_LandDataManager_GetTrackedTargetLoadedMapsQuadrant,%function
__wrap_LandDataManager_GetTrackedTargetLoadedMapsQuadrant:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag431
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LandDataManager_GetTrackedTargetLoadedMapsQuadrant
 .ltorg
.global __wrap_LandDataManager_HasEnded
.type __wrap_LandDataManager_HasEnded,%function
__wrap_LandDataManager_HasEnded:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag432
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LandDataManager_HasEnded
 .ltorg
.global __wrap_LandDataManager_InitialLoad
.type __wrap_LandDataManager_InitialLoad,%function
__wrap_LandDataManager_InitialLoad:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag433
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LandDataManager_InitialLoad
 .ltorg
.global __wrap_LandDataManager_New
.type __wrap_LandDataManager_New,%function
__wrap_LandDataManager_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag434
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LandDataManager_New
 .ltorg
.global __wrap_LandDataManager_RenderLoadedMaps
.type __wrap_LandDataManager_RenderLoadedMaps,%function
__wrap_LandDataManager_RenderLoadedMaps:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag435
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LandDataManager_RenderLoadedMaps
 .ltorg
.global __wrap_LandDataManager_SetInDistortionWorld
.type __wrap_LandDataManager_SetInDistortionWorld,%function
__wrap_LandDataManager_SetInDistortionWorld:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag436
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LandDataManager_SetInDistortionWorld
 .ltorg
.global __wrap_LandDataManager_SetMapLoadedCallback
.type __wrap_LandDataManager_SetMapLoadedCallback,%function
__wrap_LandDataManager_SetMapLoadedCallback:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag437
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LandDataManager_SetMapLoadedCallback
 .ltorg
.global __wrap_LandDataManager_SetSkipMapProps
.type __wrap_LandDataManager_SetSkipMapProps,%function
__wrap_LandDataManager_SetSkipMapProps:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag438
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LandDataManager_SetSkipMapProps
 .ltorg
.global __wrap_LandDataManager_Tick
.type __wrap_LandDataManager_Tick,%function
__wrap_LandDataManager_Tick:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag439
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LandDataManager_Tick
 .ltorg
.global __wrap_LandDataManager_TrackTarget
.type __wrap_LandDataManager_TrackTarget,%function
__wrap_LandDataManager_TrackTarget:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag440
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LandDataManager_TrackTarget
 .ltorg
.global __wrap_LinearInterpolationTaskFX32_Init
.type __wrap_LinearInterpolationTaskFX32_Init,%function
__wrap_LinearInterpolationTaskFX32_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag441
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LinearInterpolationTaskFX32_Init
 .ltorg
.global __wrap_LinearInterpolationTaskFX32_Update
.type __wrap_LinearInterpolationTaskFX32_Update,%function
__wrap_LinearInterpolationTaskFX32_Update:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag442
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LinearInterpolationTaskFX32_Update
 .ltorg
.global __wrap_LinearInterpolationTaskS32_Init
.type __wrap_LinearInterpolationTaskS32_Init,%function
__wrap_LinearInterpolationTaskS32_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag443
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LinearInterpolationTaskS32_Init
 .ltorg
.global __wrap_LinearInterpolationTaskS32_Update
.type __wrap_LinearInterpolationTaskS32_Update,%function
__wrap_LinearInterpolationTaskS32_Update:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag444
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LinearInterpolationTaskS32_Update
 .ltorg
.global __wrap_ListMenu_CalcTrueCursorPos
.type __wrap_ListMenu_CalcTrueCursorPos,%function
__wrap_ListMenu_CalcTrueCursorPos:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag445
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ListMenu_CalcTrueCursorPos
 .ltorg
.global __wrap_ListMenu_Free
.type __wrap_ListMenu_Free,%function
__wrap_ListMenu_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag446
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ListMenu_Free
 .ltorg
.global __wrap_ListMenu_GetAttribute
.type __wrap_ListMenu_GetAttribute,%function
__wrap_ListMenu_GetAttribute:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag447
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ListMenu_GetAttribute
 .ltorg
.global __wrap_ListMenu_GetListAndCursorPos
.type __wrap_ListMenu_GetListAndCursorPos,%function
__wrap_ListMenu_GetListAndCursorPos:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag448
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ListMenu_GetListAndCursorPos
 .ltorg
.global __wrap_ListMenu_New
.type __wrap_ListMenu_New,%function
__wrap_ListMenu_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag449
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ListMenu_New
 .ltorg
.global __wrap_ListMenu_ProcessInput
.type __wrap_ListMenu_ProcessInput,%function
__wrap_ListMenu_ProcessInput:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag450
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ListMenu_ProcessInput
 .ltorg
.global __wrap_ListMenu_SetAltTextColors
.type __wrap_ListMenu_SetAltTextColors,%function
__wrap_ListMenu_SetAltTextColors:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag451
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ListMenu_SetAltTextColors
 .ltorg
.global __wrap_LoadMemberFromNARC
.type __wrap_LoadMemberFromNARC,%function
__wrap_LoadMemberFromNARC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag452
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LoadMemberFromNARC
 .ltorg
.global __wrap_LoadMemberFromNARC_OutFileSize
.type __wrap_LoadMemberFromNARC_OutFileSize,%function
__wrap_LoadMemberFromNARC_OutFileSize:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag453
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LoadMemberFromNARC_OutFileSize
 .ltorg
.global __wrap_LoadMemberFromOpenNARC
.type __wrap_LoadMemberFromOpenNARC,%function
__wrap_LoadMemberFromOpenNARC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag454
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LoadMemberFromOpenNARC
 .ltorg
.global __wrap_LoadMessageBoxGraphics
.type __wrap_LoadMessageBoxGraphics,%function
__wrap_LoadMessageBoxGraphics:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag455
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LoadMessageBoxGraphics
 .ltorg
.global __wrap_LoadStandardWindowGraphics
.type __wrap_LoadStandardWindowGraphics,%function
__wrap_LoadStandardWindowGraphics:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag456
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LoadStandardWindowGraphics
 .ltorg
.global __wrap_LoadedMapBuffers_Free
.type __wrap_LoadedMapBuffers_Free,%function
__wrap_LoadedMapBuffers_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag457
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LoadedMapBuffers_Free
 .ltorg
.global __wrap_LoadedMapBuffers_GetBDHCFileBufPtr
.type __wrap_LoadedMapBuffers_GetBDHCFileBufPtr,%function
__wrap_LoadedMapBuffers_GetBDHCFileBufPtr:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag458
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LoadedMapBuffers_GetBDHCFileBufPtr
 .ltorg
.global __wrap_LoadedMapBuffers_GetMapModelFileBufPtr
.type __wrap_LoadedMapBuffers_GetMapModelFileBufPtr,%function
__wrap_LoadedMapBuffers_GetMapModelFileBufPtr:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag459
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LoadedMapBuffers_GetMapModelFileBufPtr
 .ltorg
.global __wrap_LoadedMapBuffers_New
.type __wrap_LoadedMapBuffers_New,%function
__wrap_LoadedMapBuffers_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag460
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LoadedMapBuffers_New
 .ltorg
.global __wrap_LoadedMapBuffers_NewWithHeapMapModelFiles
.type __wrap_LoadedMapBuffers_NewWithHeapMapModelFiles,%function
__wrap_LoadedMapBuffers_NewWithHeapMapModelFiles:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag461
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LoadedMapBuffers_NewWithHeapMapModelFiles
 .ltorg
.global __wrap_LoadedMapBuffers_SwapMapModelData
.type __wrap_LoadedMapBuffers_SwapMapModelData,%function
__wrap_LoadedMapBuffers_SwapMapModelData:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag462
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LoadedMapBuffers_SwapMapModelData
 .ltorg
.global __wrap_LocalMapObj_CheckAnimationFinished
.type __wrap_LocalMapObj_CheckAnimationFinished,%function
__wrap_LocalMapObj_CheckAnimationFinished:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag463
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LocalMapObj_CheckAnimationFinished
 .ltorg
.global __wrap_LocalMapObj_IsAnimationSet
.type __wrap_LocalMapObj_IsAnimationSet,%function
__wrap_LocalMapObj_IsAnimationSet:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag464
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LocalMapObj_IsAnimationSet
 .ltorg
.global __wrap_LocalMapObj_SetAnimationCode
.type __wrap_LocalMapObj_SetAnimationCode,%function
__wrap_LocalMapObj_SetAnimationCode:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag465
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_LocalMapObj_SetAnimationCode
 .ltorg
.global __wrap_MTRNG_Next
.type __wrap_MTRNG_Next,%function
__wrap_MTRNG_Next:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag466
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MTRNG_Next
 .ltorg
.global __wrap_MapHeaderData_GetBgEvents
.type __wrap_MapHeaderData_GetBgEvents,%function
__wrap_MapHeaderData_GetBgEvents:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag467
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapHeaderData_GetBgEvents
 .ltorg
.global __wrap_MapHeaderData_GetCoordEvents
.type __wrap_MapHeaderData_GetCoordEvents,%function
__wrap_MapHeaderData_GetCoordEvents:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag468
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapHeaderData_GetCoordEvents
 .ltorg
.global __wrap_MapHeaderData_GetIndexOfWarpEventAtPos
.type __wrap_MapHeaderData_GetIndexOfWarpEventAtPos,%function
__wrap_MapHeaderData_GetIndexOfWarpEventAtPos:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag469
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapHeaderData_GetIndexOfWarpEventAtPos
 .ltorg
.global __wrap_MapHeaderData_GetNumBgEvents
.type __wrap_MapHeaderData_GetNumBgEvents,%function
__wrap_MapHeaderData_GetNumBgEvents:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag470
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapHeaderData_GetNumBgEvents
 .ltorg
.global __wrap_MapHeaderData_GetNumCoordEvents
.type __wrap_MapHeaderData_GetNumCoordEvents,%function
__wrap_MapHeaderData_GetNumCoordEvents:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag471
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapHeaderData_GetNumCoordEvents
 .ltorg
.global __wrap_MapHeaderData_GetNumObjectEvents
.type __wrap_MapHeaderData_GetNumObjectEvents,%function
__wrap_MapHeaderData_GetNumObjectEvents:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag472
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapHeaderData_GetNumObjectEvents
 .ltorg
.global __wrap_MapHeaderData_GetObjectEvents
.type __wrap_MapHeaderData_GetObjectEvents,%function
__wrap_MapHeaderData_GetObjectEvents:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag473
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapHeaderData_GetObjectEvents
 .ltorg
.global __wrap_MapHeaderData_GetWarpEventByIndex
.type __wrap_MapHeaderData_GetWarpEventByIndex,%function
__wrap_MapHeaderData_GetWarpEventByIndex:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag474
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapHeaderData_GetWarpEventByIndex
 .ltorg
.global __wrap_MapHeaderData_Load
.type __wrap_MapHeaderData_Load,%function
__wrap_MapHeaderData_Load:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag475
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapHeaderData_Load
 .ltorg
.global __wrap_MapHeader_GetAreaDataArchiveID
.type __wrap_MapHeader_GetAreaDataArchiveID,%function
__wrap_MapHeader_GetAreaDataArchiveID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag476
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapHeader_GetAreaDataArchiveID
 .ltorg
.global __wrap_MapHeader_GetMapLabelTextID
.type __wrap_MapHeader_GetMapLabelTextID,%function
__wrap_MapHeader_GetMapLabelTextID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag477
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapHeader_GetMapLabelTextID
 .ltorg
.global __wrap_MapHeader_GetMapLabelWindowID
.type __wrap_MapHeader_GetMapLabelWindowID,%function
__wrap_MapHeader_GetMapLabelWindowID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag478
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapHeader_GetMapLabelWindowID
 .ltorg
.global __wrap_MapHeader_GetPreloadedMapObjectsArchiveID
.type __wrap_MapHeader_GetPreloadedMapObjectsArchiveID,%function
__wrap_MapHeader_GetPreloadedMapObjectsArchiveID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag479
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapHeader_GetPreloadedMapObjectsArchiveID
 .ltorg
.global __wrap_MapHeader_HasWildEncounters
.type __wrap_MapHeader_HasWildEncounters,%function
__wrap_MapHeader_HasWildEncounters:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag480
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapHeader_HasWildEncounters
 .ltorg
.global __wrap_MapHeader_IsBuilding
.type __wrap_MapHeader_IsBuilding,%function
__wrap_MapHeader_IsBuilding:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag481
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapHeader_IsBuilding
 .ltorg
.global __wrap_MapHeader_IsOnMainMatrix
.type __wrap_MapHeader_IsOnMainMatrix,%function
__wrap_MapHeader_IsOnMainMatrix:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag482
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapHeader_IsOnMainMatrix
 .ltorg
.global __wrap_MapHeader_LoadString
.type __wrap_MapHeader_LoadString,%function
__wrap_MapHeader_LoadString:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag483
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapHeader_LoadString
 .ltorg
.global __wrap_MapMatrix_Copy
.type __wrap_MapMatrix_Copy,%function
__wrap_MapMatrix_Copy:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag484
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapMatrix_Copy
 .ltorg
.global __wrap_MapMatrix_GetAltitudeAtCoords
.type __wrap_MapMatrix_GetAltitudeAtCoords,%function
__wrap_MapMatrix_GetAltitudeAtCoords:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag485
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapMatrix_GetAltitudeAtCoords
 .ltorg
.global __wrap_MapMatrix_GetHeight
.type __wrap_MapMatrix_GetHeight,%function
__wrap_MapMatrix_GetHeight:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag486
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapMatrix_GetHeight
 .ltorg
.global __wrap_MapMatrix_GetLandDataIDByIndex
.type __wrap_MapMatrix_GetLandDataIDByIndex,%function
__wrap_MapMatrix_GetLandDataIDByIndex:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag487
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapMatrix_GetLandDataIDByIndex
 .ltorg
.global __wrap_MapMatrix_GetMapHeaderIDAtCoords
.type __wrap_MapMatrix_GetMapHeaderIDAtCoords,%function
__wrap_MapMatrix_GetMapHeaderIDAtCoords:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag488
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapMatrix_GetMapHeaderIDAtCoords
 .ltorg
.global __wrap_MapMatrix_GetMapHeaderIDAtIndex
.type __wrap_MapMatrix_GetMapHeaderIDAtIndex,%function
__wrap_MapMatrix_GetMapHeaderIDAtIndex:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag489
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapMatrix_GetMapHeaderIDAtIndex
 .ltorg
.global __wrap_MapMatrix_GetMatrixID
.type __wrap_MapMatrix_GetMatrixID,%function
__wrap_MapMatrix_GetMatrixID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag490
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapMatrix_GetMatrixID
 .ltorg
.global __wrap_MapMatrix_GetWidth
.type __wrap_MapMatrix_GetWidth,%function
__wrap_MapMatrix_GetWidth:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag491
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapMatrix_GetWidth
 .ltorg
.global __wrap_MapNamePopUp_Create
.type __wrap_MapNamePopUp_Create,%function
__wrap_MapNamePopUp_Create:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag492
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapNamePopUp_Create
 .ltorg
.global __wrap_MapNamePopUp_Destroy
.type __wrap_MapNamePopUp_Destroy,%function
__wrap_MapNamePopUp_Destroy:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag493
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapNamePopUp_Destroy
 .ltorg
.global __wrap_MapNamePopUp_Show
.type __wrap_MapNamePopUp_Show,%function
__wrap_MapNamePopUp_Show:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag494
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapNamePopUp_Show
 .ltorg
.global __wrap_MapObjMan_LocalMapObjByIndex
.type __wrap_MapObjMan_LocalMapObjByIndex,%function
__wrap_MapObjMan_LocalMapObjByIndex:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag495
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObjMan_LocalMapObjByIndex
 .ltorg
.global __wrap_MapObjectMan_CheckStatus
.type __wrap_MapObjectMan_CheckStatus,%function
__wrap_MapObjectMan_CheckStatus:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag496
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObjectMan_CheckStatus
 .ltorg
.global __wrap_MapObjectMan_FindObjectWithStatus
.type __wrap_MapObjectMan_FindObjectWithStatus,%function
__wrap_MapObjectMan_FindObjectWithStatus:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag497
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObjectMan_FindObjectWithStatus
 .ltorg
.global __wrap_MapObjectMan_GetMapObjectConst
.type __wrap_MapObjectMan_GetMapObjectConst,%function
__wrap_MapObjectMan_GetMapObjectConst:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag498
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObjectMan_GetMapObjectConst
 .ltorg
.global __wrap_MapObjectMan_GetMaxObjects
.type __wrap_MapObjectMan_GetMaxObjects,%function
__wrap_MapObjectMan_GetMaxObjects:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag499
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObjectMan_GetMaxObjects
 .ltorg
.global __wrap_MapObjectMan_GetNARC
.type __wrap_MapObjectMan_GetNARC,%function
__wrap_MapObjectMan_GetNARC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag500
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObjectMan_GetNARC
 .ltorg
.global __wrap_MapObjectMan_GetTaskBasePriority
.type __wrap_MapObjectMan_GetTaskBasePriority,%function
__wrap_MapObjectMan_GetTaskBasePriority:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag501
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObjectMan_GetTaskBasePriority
 .ltorg
.global __wrap_MapObjectMan_IsDrawInitialized
.type __wrap_MapObjectMan_IsDrawInitialized,%function
__wrap_MapObjectMan_IsDrawInitialized:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag502
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObjectMan_IsDrawInitialized
 .ltorg
.global __wrap_MapObjectMan_PauseAllMovement
.type __wrap_MapObjectMan_PauseAllMovement,%function
__wrap_MapObjectMan_PauseAllMovement:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag503
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObjectMan_PauseAllMovement
 .ltorg
.global __wrap_MapObjectMan_SetEndMovement
.type __wrap_MapObjectMan_SetEndMovement,%function
__wrap_MapObjectMan_SetEndMovement:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag504
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObjectMan_SetEndMovement
 .ltorg
.global __wrap_MapObjectMan_SetNARC
.type __wrap_MapObjectMan_SetNARC,%function
__wrap_MapObjectMan_SetNARC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag505
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObjectMan_SetNARC
 .ltorg
.global __wrap_MapObjectMan_SetStatusFlagOff
.type __wrap_MapObjectMan_SetStatusFlagOff,%function
__wrap_MapObjectMan_SetStatusFlagOff:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag506
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObjectMan_SetStatusFlagOff
 .ltorg
.global __wrap_MapObjectMan_SetStatusFlagOn
.type __wrap_MapObjectMan_SetStatusFlagOn,%function
__wrap_MapObjectMan_SetStatusFlagOn:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag507
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObjectMan_SetStatusFlagOn
 .ltorg
.global __wrap_MapObjectMan_StopAllMovement
.type __wrap_MapObjectMan_StopAllMovement,%function
__wrap_MapObjectMan_StopAllMovement:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag508
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObjectMan_StopAllMovement
 .ltorg
.global __wrap_MapObjectMan_UnpauseAllMovement
.type __wrap_MapObjectMan_UnpauseAllMovement,%function
__wrap_MapObjectMan_UnpauseAllMovement:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag509
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObjectMan_UnpauseAllMovement
 .ltorg
.global __wrap_MapObject_CalculateTaskPriority
.type __wrap_MapObject_CalculateTaskPriority,%function
__wrap_MapObject_CalculateTaskPriority:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag510
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_CalculateTaskPriority
 .ltorg
.global __wrap_MapObject_CheckManagerStatus
.type __wrap_MapObject_CheckManagerStatus,%function
__wrap_MapObject_CheckManagerStatus:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag511
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_CheckManagerStatus
 .ltorg
.global __wrap_MapObject_CheckStatus
.type __wrap_MapObject_CheckStatus,%function
__wrap_MapObject_CheckStatus:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag512
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_CheckStatus
 .ltorg
.global __wrap_MapObject_CheckStatusFlag
.type __wrap_MapObject_CheckStatusFlag,%function
__wrap_MapObject_CheckStatusFlag:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag513
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_CheckStatusFlag
 .ltorg
.global __wrap_MapObject_Delete
.type __wrap_MapObject_Delete,%function
__wrap_MapObject_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag514
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_Delete
 .ltorg
.global __wrap_MapObject_Draw
.type __wrap_MapObject_Draw,%function
__wrap_MapObject_Draw:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag515
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_Draw
 .ltorg
.global __wrap_MapObject_FieldSystem
.type __wrap_MapObject_FieldSystem,%function
__wrap_MapObject_FieldSystem:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag516
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_FieldSystem
 .ltorg
.global __wrap_MapObject_FinishAnimation
.type __wrap_MapObject_FinishAnimation,%function
__wrap_MapObject_FinishAnimation:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag517
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_FinishAnimation
 .ltorg
.global __wrap_MapObject_GetCurrTileBehavior
.type __wrap_MapObject_GetCurrTileBehavior,%function
__wrap_MapObject_GetCurrTileBehavior:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag518
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetCurrTileBehavior
 .ltorg
.global __wrap_MapObject_GetDxFromDir
.type __wrap_MapObject_GetDxFromDir,%function
__wrap_MapObject_GetDxFromDir:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag519
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetDxFromDir
 .ltorg
.global __wrap_MapObject_GetDzFromDir
.type __wrap_MapObject_GetDzFromDir,%function
__wrap_MapObject_GetDzFromDir:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag520
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetDzFromDir
 .ltorg
.global __wrap_MapObject_GetEffectiveGraphicsID
.type __wrap_MapObject_GetEffectiveGraphicsID,%function
__wrap_MapObject_GetEffectiveGraphicsID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag521
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetEffectiveGraphicsID
 .ltorg
.global __wrap_MapObject_GetFacingDir
.type __wrap_MapObject_GetFacingDir,%function
__wrap_MapObject_GetFacingDir:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag522
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetFacingDir
 .ltorg
.global __wrap_MapObject_GetFieldEffectManager
.type __wrap_MapObject_GetFieldEffectManager,%function
__wrap_MapObject_GetFieldEffectManager:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag523
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetFieldEffectManager
 .ltorg
.global __wrap_MapObject_GetGraphicsID
.type __wrap_MapObject_GetGraphicsID,%function
__wrap_MapObject_GetGraphicsID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag524
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetGraphicsID
 .ltorg
.global __wrap_MapObject_GetLocalID
.type __wrap_MapObject_GetLocalID,%function
__wrap_MapObject_GetLocalID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag525
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetLocalID
 .ltorg
.global __wrap_MapObject_GetMapHeaderID
.type __wrap_MapObject_GetMapHeaderID,%function
__wrap_MapObject_GetMapHeaderID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag526
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetMapHeaderID
 .ltorg
.global __wrap_MapObject_GetMovementType
.type __wrap_MapObject_GetMovementType,%function
__wrap_MapObject_GetMovementType:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag527
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetMovementType
 .ltorg
.global __wrap_MapObject_GetMovingDir
.type __wrap_MapObject_GetMovingDir,%function
__wrap_MapObject_GetMovingDir:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag528
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetMovingDir
 .ltorg
.global __wrap_MapObject_GetPosPtr
.type __wrap_MapObject_GetPosPtr,%function
__wrap_MapObject_GetPosPtr:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag529
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetPosPtr
 .ltorg
.global __wrap_MapObject_GetPosY
.type __wrap_MapObject_GetPosY,%function
__wrap_MapObject_GetPosY:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag530
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetPosY
 .ltorg
.global __wrap_MapObject_GetPrevFacingDir
.type __wrap_MapObject_GetPrevFacingDir,%function
__wrap_MapObject_GetPrevFacingDir:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag531
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetPrevFacingDir
 .ltorg
.global __wrap_MapObject_GetScript
.type __wrap_MapObject_GetScript,%function
__wrap_MapObject_GetScript:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag532
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetScript
 .ltorg
.global __wrap_MapObject_GetSpriteJumpOffset
.type __wrap_MapObject_GetSpriteJumpOffset,%function
__wrap_MapObject_GetSpriteJumpOffset:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag533
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetSpriteJumpOffset
 .ltorg
.global __wrap_MapObject_GetSpritePosOffset
.type __wrap_MapObject_GetSpritePosOffset,%function
__wrap_MapObject_GetSpritePosOffset:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag534
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetSpritePosOffset
 .ltorg
.global __wrap_MapObject_GetSpriteTerrainOffset
.type __wrap_MapObject_GetSpriteTerrainOffset,%function
__wrap_MapObject_GetSpriteTerrainOffset:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag535
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetSpriteTerrainOffset
 .ltorg
.global __wrap_MapObject_GetTaskBasePriority
.type __wrap_MapObject_GetTaskBasePriority,%function
__wrap_MapObject_GetTaskBasePriority:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag536
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetTaskBasePriority
 .ltorg
.global __wrap_MapObject_GetTileBehaviorFromDir
.type __wrap_MapObject_GetTileBehaviorFromDir,%function
__wrap_MapObject_GetTileBehaviorFromDir:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag537
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetTileBehaviorFromDir
 .ltorg
.global __wrap_MapObject_GetTrainerType
.type __wrap_MapObject_GetTrainerType,%function
__wrap_MapObject_GetTrainerType:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag538
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetTrainerType
 .ltorg
.global __wrap_MapObject_GetX
.type __wrap_MapObject_GetX,%function
__wrap_MapObject_GetX:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag539
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetX
 .ltorg
.global __wrap_MapObject_GetXPrev
.type __wrap_MapObject_GetXPrev,%function
__wrap_MapObject_GetXPrev:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag540
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetXPrev
 .ltorg
.global __wrap_MapObject_GetY
.type __wrap_MapObject_GetY,%function
__wrap_MapObject_GetY:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag541
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetY
 .ltorg
.global __wrap_MapObject_GetYPrev
.type __wrap_MapObject_GetYPrev,%function
__wrap_MapObject_GetYPrev:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag542
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetYPrev
 .ltorg
.global __wrap_MapObject_GetZ
.type __wrap_MapObject_GetZ,%function
__wrap_MapObject_GetZ:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag543
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetZ
 .ltorg
.global __wrap_MapObject_GetZPrev
.type __wrap_MapObject_GetZPrev,%function
__wrap_MapObject_GetZPrev:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag544
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_GetZPrev
 .ltorg
.global __wrap_MapObject_HasAnimationEnded
.type __wrap_MapObject_HasAnimationEnded,%function
__wrap_MapObject_HasAnimationEnded:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag545
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_HasAnimationEnded
 .ltorg
.global __wrap_MapObject_IsHidden
.type __wrap_MapObject_IsHidden,%function
__wrap_MapObject_IsHidden:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag546
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_IsHidden
 .ltorg
.global __wrap_MapObject_IsMovementPaused
.type __wrap_MapObject_IsMovementPaused,%function
__wrap_MapObject_IsMovementPaused:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag547
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_IsMovementPaused
 .ltorg
.global __wrap_MapObject_IsMoving
.type __wrap_MapObject_IsMoving,%function
__wrap_MapObject_IsMoving:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag548
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_IsMoving
 .ltorg
.global __wrap_MapObject_IsStatusOnElevatedBridge
.type __wrap_MapObject_IsStatusOnElevatedBridge,%function
__wrap_MapObject_IsStatusOnElevatedBridge:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag549
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_IsStatusOnElevatedBridge
 .ltorg
.global __wrap_MapObject_MapObjectManager
.type __wrap_MapObject_MapObjectManager,%function
__wrap_MapObject_MapObjectManager:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag550
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_MapObjectManager
 .ltorg
.global __wrap_MapObject_RecalculatePositionHeight
.type __wrap_MapObject_RecalculatePositionHeight,%function
__wrap_MapObject_RecalculatePositionHeight:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag551
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_RecalculatePositionHeight
 .ltorg
.global __wrap_MapObject_RecalculatePositionHeightEx
.type __wrap_MapObject_RecalculatePositionHeightEx,%function
__wrap_MapObject_RecalculatePositionHeightEx:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag552
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_RecalculatePositionHeightEx
 .ltorg
.global __wrap_MapObject_SetFlagDoNotSinkIntoTerrain
.type __wrap_MapObject_SetFlagDoNotSinkIntoTerrain,%function
__wrap_MapObject_SetFlagDoNotSinkIntoTerrain:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag553
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_SetFlagDoNotSinkIntoTerrain
 .ltorg
.global __wrap_MapObject_SetHidden
.type __wrap_MapObject_SetHidden,%function
__wrap_MapObject_SetHidden:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag554
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_SetHidden
 .ltorg
.global __wrap_MapObject_SetPauseMovementOff
.type __wrap_MapObject_SetPauseMovementOff,%function
__wrap_MapObject_SetPauseMovementOff:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag555
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_SetPauseMovementOff
 .ltorg
.global __wrap_MapObject_SetPauseMovementOn
.type __wrap_MapObject_SetPauseMovementOn,%function
__wrap_MapObject_SetPauseMovementOn:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag556
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_SetPauseMovementOn
 .ltorg
.global __wrap_MapObject_SetPos
.type __wrap_MapObject_SetPos,%function
__wrap_MapObject_SetPos:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag557
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_SetPos
 .ltorg
.global __wrap_MapObject_SetSpriteJumpOffset
.type __wrap_MapObject_SetSpriteJumpOffset,%function
__wrap_MapObject_SetSpriteJumpOffset:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag558
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_SetSpriteJumpOffset
 .ltorg
.global __wrap_MapObject_SetSpritePosOffset
.type __wrap_MapObject_SetSpritePosOffset,%function
__wrap_MapObject_SetSpritePosOffset:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag559
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_SetSpritePosOffset
 .ltorg
.global __wrap_MapObject_SetStatusFlagOff
.type __wrap_MapObject_SetStatusFlagOff,%function
__wrap_MapObject_SetStatusFlagOff:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag560
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_SetStatusFlagOff
 .ltorg
.global __wrap_MapObject_SetStatusFlagOn
.type __wrap_MapObject_SetStatusFlagOn,%function
__wrap_MapObject_SetStatusFlagOn:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag561
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_SetStatusFlagOn
 .ltorg
.global __wrap_MapObject_SetX
.type __wrap_MapObject_SetX,%function
__wrap_MapObject_SetX:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag562
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_SetX
 .ltorg
.global __wrap_MapObject_SetY
.type __wrap_MapObject_SetY,%function
__wrap_MapObject_SetY:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag563
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_SetY
 .ltorg
.global __wrap_MapObject_SetZ
.type __wrap_MapObject_SetZ,%function
__wrap_MapObject_SetZ:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag564
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_SetZ
 .ltorg
.global __wrap_MapObject_StartAnimation
.type __wrap_MapObject_StartAnimation,%function
__wrap_MapObject_StartAnimation:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag565
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_StartAnimation
 .ltorg
.global __wrap_MapObject_SwitchMovementType
.type __wrap_MapObject_SwitchMovementType,%function
__wrap_MapObject_SwitchMovementType:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag566
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_SwitchMovementType
 .ltorg
.global __wrap_MapObject_TryFace
.type __wrap_MapObject_TryFace,%function
__wrap_MapObject_TryFace:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag567
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_TryFace
 .ltorg
.global __wrap_MapObject_UpdateCoords
.type __wrap_MapObject_UpdateCoords,%function
__wrap_MapObject_UpdateCoords:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag568
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapObject_UpdateCoords
 .ltorg
.global __wrap_MapPropAnimationManager_AddAllAnimationsToRenderObj
.type __wrap_MapPropAnimationManager_AddAllAnimationsToRenderObj,%function
__wrap_MapPropAnimationManager_AddAllAnimationsToRenderObj:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag569
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropAnimationManager_AddAllAnimationsToRenderObj
 .ltorg
.global __wrap_MapPropAnimationManager_AddAnimationToRenderObj
.type __wrap_MapPropAnimationManager_AddAnimationToRenderObj,%function
__wrap_MapPropAnimationManager_AddAnimationToRenderObj:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag570
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropAnimationManager_AddAnimationToRenderObj
 .ltorg
.global __wrap_MapPropAnimationManager_AdvanceAnimations
.type __wrap_MapPropAnimationManager_AdvanceAnimations,%function
__wrap_MapPropAnimationManager_AdvanceAnimations:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag571
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropAnimationManager_AdvanceAnimations
 .ltorg
.global __wrap_MapPropAnimationManager_Free
.type __wrap_MapPropAnimationManager_Free,%function
__wrap_MapPropAnimationManager_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag572
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropAnimationManager_Free
 .ltorg
.global __wrap_MapPropAnimationManager_GetAnimation
.type __wrap_MapPropAnimationManager_GetAnimation,%function
__wrap_MapPropAnimationManager_GetAnimation:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag573
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropAnimationManager_GetAnimation
 .ltorg
.global __wrap_MapPropAnimationManager_GetAnimeListNARCFileCount
.type __wrap_MapPropAnimationManager_GetAnimeListNARCFileCount,%function
__wrap_MapPropAnimationManager_GetAnimeListNARCFileCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag574
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropAnimationManager_GetAnimeListNARCFileCount
 .ltorg
.global __wrap_MapPropAnimationManager_GetPropAnimationCount
.type __wrap_MapPropAnimationManager_GetPropAnimationCount,%function
__wrap_MapPropAnimationManager_GetPropAnimationCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag575
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropAnimationManager_GetPropAnimationCount
 .ltorg
.global __wrap_MapPropAnimationManager_LoadPropAnimations
.type __wrap_MapPropAnimationManager_LoadPropAnimations,%function
__wrap_MapPropAnimationManager_LoadPropAnimations:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag576
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropAnimationManager_LoadPropAnimations
 .ltorg
.global __wrap_MapPropAnimationManager_New
.type __wrap_MapPropAnimationManager_New,%function
__wrap_MapPropAnimationManager_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag577
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropAnimationManager_New
 .ltorg
.global __wrap_MapPropAnimationManager_RemoveAnimationFromRenderObj
.type __wrap_MapPropAnimationManager_RemoveAnimationFromRenderObj,%function
__wrap_MapPropAnimationManager_RemoveAnimationFromRenderObj:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag578
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropAnimationManager_RemoveAnimationFromRenderObj
 .ltorg
.global __wrap_MapPropAnimationManager_UnloadAllAnimations
.type __wrap_MapPropAnimationManager_UnloadAllAnimations,%function
__wrap_MapPropAnimationManager_UnloadAllAnimations:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag579
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropAnimationManager_UnloadAllAnimations
 .ltorg
.global __wrap_MapPropAnimation_GoToFirstFrame
.type __wrap_MapPropAnimation_GoToFirstFrame,%function
__wrap_MapPropAnimation_GoToFirstFrame:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag580
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropAnimation_GoToFirstFrame
 .ltorg
.global __wrap_MapPropAnimation_SetAnimationPaused
.type __wrap_MapPropAnimation_SetAnimationPaused,%function
__wrap_MapPropAnimation_SetAnimationPaused:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag581
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropAnimation_SetAnimationPaused
 .ltorg
.global __wrap_MapPropAnimation_StartLoop
.type __wrap_MapPropAnimation_StartLoop,%function
__wrap_MapPropAnimation_StartLoop:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag582
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropAnimation_StartLoop
 .ltorg
.global __wrap_MapPropManager_FindLoadedPropByModelID
.type __wrap_MapPropManager_FindLoadedPropByModelID,%function
__wrap_MapPropManager_FindLoadedPropByModelID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag583
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropManager_FindLoadedPropByModelID
 .ltorg
.global __wrap_MapPropManager_Free
.type __wrap_MapPropManager_Free,%function
__wrap_MapPropManager_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag584
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropManager_Free
 .ltorg
.global __wrap_MapPropManager_Init
.type __wrap_MapPropManager_Init,%function
__wrap_MapPropManager_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag585
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropManager_Init
 .ltorg
.global __wrap_MapPropManager_Load
.type __wrap_MapPropManager_Load,%function
__wrap_MapPropManager_Load:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag586
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropManager_Load
 .ltorg
.global __wrap_MapPropManager_LoadOne
.type __wrap_MapPropManager_LoadOne,%function
__wrap_MapPropManager_LoadOne:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag587
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropManager_LoadOne
 .ltorg
.global __wrap_MapPropManager_New
.type __wrap_MapPropManager_New,%function
__wrap_MapPropManager_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag588
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropManager_New
 .ltorg
.global __wrap_MapPropManager_Render
.type __wrap_MapPropManager_Render,%function
__wrap_MapPropManager_Render:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag589
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropManager_Render
 .ltorg
.global __wrap_MapPropManager_Render2
.type __wrap_MapPropManager_Render2,%function
__wrap_MapPropManager_Render2:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag590
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropManager_Render2
 .ltorg
.global __wrap_MapPropMaterialShape_Alloc
.type __wrap_MapPropMaterialShape_Alloc,%function
__wrap_MapPropMaterialShape_Alloc:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag591
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropMaterialShape_Alloc
 .ltorg
.global __wrap_MapPropMaterialShape_Free
.type __wrap_MapPropMaterialShape_Free,%function
__wrap_MapPropMaterialShape_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag592
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropMaterialShape_Free
 .ltorg
.global __wrap_MapPropMaterialShape_GetMaterialShapeIDsAt
.type __wrap_MapPropMaterialShape_GetMaterialShapeIDsAt,%function
__wrap_MapPropMaterialShape_GetMaterialShapeIDsAt:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag593
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropMaterialShape_GetMaterialShapeIDsAt
 .ltorg
.global __wrap_MapPropMaterialShape_Load
.type __wrap_MapPropMaterialShape_Load,%function
__wrap_MapPropMaterialShape_Load:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag594
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropMaterialShape_Load
 .ltorg
.global __wrap_MapPropOneShotAnimationManager_Free
.type __wrap_MapPropOneShotAnimationManager_Free,%function
__wrap_MapPropOneShotAnimationManager_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag595
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropOneShotAnimationManager_Free
 .ltorg
.global __wrap_MapPropOneShotAnimationManager_GetAnimationMapPropModelID
.type __wrap_MapPropOneShotAnimationManager_GetAnimationMapPropModelID,%function
__wrap_MapPropOneShotAnimationManager_GetAnimationMapPropModelID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag596
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropOneShotAnimationManager_GetAnimationMapPropModelID
 .ltorg
.global __wrap_MapPropOneShotAnimationManager_IsAnimationLoopFinished
.type __wrap_MapPropOneShotAnimationManager_IsAnimationLoopFinished,%function
__wrap_MapPropOneShotAnimationManager_IsAnimationLoopFinished:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag597
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropOneShotAnimationManager_IsAnimationLoopFinished
 .ltorg
.global __wrap_MapPropOneShotAnimationManager_LoadPropAnimations
.type __wrap_MapPropOneShotAnimationManager_LoadPropAnimations,%function
__wrap_MapPropOneShotAnimationManager_LoadPropAnimations:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag598
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropOneShotAnimationManager_LoadPropAnimations
 .ltorg
.global __wrap_MapPropOneShotAnimationManager_New
.type __wrap_MapPropOneShotAnimationManager_New,%function
__wrap_MapPropOneShotAnimationManager_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag599
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropOneShotAnimationManager_New
 .ltorg
.global __wrap_MapPropOneShotAnimationManager_PlayAnimationWithSoundEffect
.type __wrap_MapPropOneShotAnimationManager_PlayAnimationWithSoundEffect,%function
__wrap_MapPropOneShotAnimationManager_PlayAnimationWithSoundEffect:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag600
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropOneShotAnimationManager_PlayAnimationWithSoundEffect
 .ltorg
.global __wrap_MapPropOneShotAnimationManager_UnloadAnimation
.type __wrap_MapPropOneShotAnimationManager_UnloadAnimation,%function
__wrap_MapPropOneShotAnimationManager_UnloadAnimation:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag601
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapPropOneShotAnimationManager_UnloadAnimation
 .ltorg
.global __wrap_MapProp_GetMaterialShapeIDsCount
.type __wrap_MapProp_GetMaterialShapeIDsCount,%function
__wrap_MapProp_GetMaterialShapeIDsCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag602
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapProp_GetMaterialShapeIDsCount
 .ltorg
.global __wrap_MapProp_GetMaterialShapeIDsLocator
.type __wrap_MapProp_GetMaterialShapeIDsLocator,%function
__wrap_MapProp_GetMaterialShapeIDsLocator:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag603
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapProp_GetMaterialShapeIDsLocator
 .ltorg
.global __wrap_MapProp_GetModel
.type __wrap_MapProp_GetModel,%function
__wrap_MapProp_GetModel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag604
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapProp_GetModel
 .ltorg
.global __wrap_MapProp_GetRenderObj
.type __wrap_MapProp_GetRenderObj,%function
__wrap_MapProp_GetRenderObj:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag605
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MapProp_GetRenderObj
 .ltorg
.global __wrap_Menu_Free
.type __wrap_Menu_Free,%function
__wrap_Menu_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag606
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Menu_Free
 .ltorg
.global __wrap_Menu_MakeYesNoChoice
.type __wrap_Menu_MakeYesNoChoice,%function
__wrap_Menu_MakeYesNoChoice:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag607
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Menu_MakeYesNoChoice
 .ltorg
.global __wrap_Menu_NewSimple
.type __wrap_Menu_NewSimple,%function
__wrap_Menu_NewSimple:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag608
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Menu_NewSimple
 .ltorg
.global __wrap_Menu_ProcessInput
.type __wrap_Menu_ProcessInput,%function
__wrap_Menu_ProcessInput:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag609
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Menu_ProcessInput
 .ltorg
.global __wrap_Menu_ProcessInputAndHandleExit
.type __wrap_Menu_ProcessInputAndHandleExit,%function
__wrap_Menu_ProcessInputAndHandleExit:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag610
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Menu_ProcessInputAndHandleExit
 .ltorg
.global __wrap_MessageLoader_Free
.type __wrap_MessageLoader_Free,%function
__wrap_MessageLoader_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag611
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MessageLoader_Free
 .ltorg
.global __wrap_MessageLoader_GetNewString
.type __wrap_MessageLoader_GetNewString,%function
__wrap_MessageLoader_GetNewString:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag612
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MessageLoader_GetNewString
 .ltorg
.global __wrap_MessageLoader_GetSpeciesName
.type __wrap_MessageLoader_GetSpeciesName,%function
__wrap_MessageLoader_GetSpeciesName:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag613
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MessageLoader_GetSpeciesName
 .ltorg
.global __wrap_MessageLoader_GetString
.type __wrap_MessageLoader_GetString,%function
__wrap_MessageLoader_GetString:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag614
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MessageLoader_GetString
 .ltorg
.global __wrap_MessageLoader_Init
.type __wrap_MessageLoader_Init,%function
__wrap_MessageLoader_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag615
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MessageLoader_Init
 .ltorg
.global __wrap_MessageUtil_ExpandedString
.type __wrap_MessageUtil_ExpandedString,%function
__wrap_MessageUtil_ExpandedString:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag616
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MessageUtil_ExpandedString
 .ltorg
.global __wrap_MessageUtil_SpeciesName
.type __wrap_MessageUtil_SpeciesName,%function
__wrap_MessageUtil_SpeciesName:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag617
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MessageUtil_SpeciesName
 .ltorg
.global __wrap_MiscSaveBlock_GetFavoriteMon
.type __wrap_MiscSaveBlock_GetFavoriteMon,%function
__wrap_MiscSaveBlock_GetFavoriteMon:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag618
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MiscSaveBlock_GetFavoriteMon
 .ltorg
.global __wrap_MiscSaveBlock_GetPersistedMapFeatures
.type __wrap_MiscSaveBlock_GetPersistedMapFeatures,%function
__wrap_MiscSaveBlock_GetPersistedMapFeatures:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag619
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MiscSaveBlock_GetPersistedMapFeatures
 .ltorg
.global __wrap_MiscSaveBlock_SetFavoriteMon
.type __wrap_MiscSaveBlock_SetFavoriteMon,%function
__wrap_MiscSaveBlock_SetFavoriteMon:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag620
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MiscSaveBlock_SetFavoriteMon
 .ltorg
.global __wrap_ModelAttributes_ApplyGlobal
.type __wrap_ModelAttributes_ApplyGlobal,%function
__wrap_ModelAttributes_ApplyGlobal:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag621
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ModelAttributes_ApplyGlobal
 .ltorg
.global __wrap_ModelAttributes_ApplyToModel
.type __wrap_ModelAttributes_ApplyToModel,%function
__wrap_ModelAttributes_ApplyToModel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag622
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ModelAttributes_ApplyToModel
 .ltorg
.global __wrap_ModelAttributes_Free
.type __wrap_ModelAttributes_Free,%function
__wrap_ModelAttributes_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag623
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ModelAttributes_Free
 .ltorg
.global __wrap_ModelAttributes_New
.type __wrap_ModelAttributes_New,%function
__wrap_ModelAttributes_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag624
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ModelAttributes_New
 .ltorg
.global __wrap_ModelAttributes_SetAlpha
.type __wrap_ModelAttributes_SetAlpha,%function
__wrap_ModelAttributes_SetAlpha:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag625
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ModelAttributes_SetAlpha
 .ltorg
.global __wrap_ModelAttributes_SetAmbientReflection
.type __wrap_ModelAttributes_SetAmbientReflection,%function
__wrap_ModelAttributes_SetAmbientReflection:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag626
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ModelAttributes_SetAmbientReflection
 .ltorg
.global __wrap_ModelAttributes_SetCullMode
.type __wrap_ModelAttributes_SetCullMode,%function
__wrap_ModelAttributes_SetCullMode:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag627
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ModelAttributes_SetCullMode
 .ltorg
.global __wrap_ModelAttributes_SetDiffuseReflection
.type __wrap_ModelAttributes_SetDiffuseReflection,%function
__wrap_ModelAttributes_SetDiffuseReflection:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag628
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ModelAttributes_SetDiffuseReflection
 .ltorg
.global __wrap_ModelAttributes_SetEmissionColor
.type __wrap_ModelAttributes_SetEmissionColor,%function
__wrap_ModelAttributes_SetEmissionColor:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag629
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ModelAttributes_SetEmissionColor
 .ltorg
.global __wrap_ModelAttributes_SetLightColor
.type __wrap_ModelAttributes_SetLightColor,%function
__wrap_ModelAttributes_SetLightColor:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag630
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ModelAttributes_SetLightColor
 .ltorg
.global __wrap_ModelAttributes_SetLightVector
.type __wrap_ModelAttributes_SetLightVector,%function
__wrap_ModelAttributes_SetLightVector:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag631
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ModelAttributes_SetLightVector
 .ltorg
.global __wrap_ModelAttributes_SetMiscAttrEnabled
.type __wrap_ModelAttributes_SetMiscAttrEnabled,%function
__wrap_ModelAttributes_SetMiscAttrEnabled:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag632
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ModelAttributes_SetMiscAttrEnabled
 .ltorg
.global __wrap_ModelAttributes_SetPolygonMode
.type __wrap_ModelAttributes_SetPolygonMode,%function
__wrap_ModelAttributes_SetPolygonMode:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag633
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ModelAttributes_SetPolygonMode
 .ltorg
.global __wrap_ModelAttributes_SetSpecularReflection
.type __wrap_ModelAttributes_SetSpecularReflection,%function
__wrap_ModelAttributes_SetSpecularReflection:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag634
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ModelAttributes_SetSpecularReflection
 .ltorg
.global __wrap_MotionBlur_Delete
.type __wrap_MotionBlur_Delete,%function
__wrap_MotionBlur_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag635
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MotionBlur_Delete
 .ltorg
.global __wrap_MotionBlur_New
.type __wrap_MotionBlur_New,%function
__wrap_MotionBlur_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag636
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MotionBlur_New
 .ltorg
.global __wrap_MovementAction_TurnActionTowardsDir
.type __wrap_MovementAction_TurnActionTowardsDir,%function
__wrap_MovementAction_TurnActionTowardsDir:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag637
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_MovementAction_TurnActionTowardsDir
 .ltorg
.global __wrap_NARC_AllocAndReadWholeMember
.type __wrap_NARC_AllocAndReadWholeMember,%function
__wrap_NARC_AllocAndReadWholeMember:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag638
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NARC_AllocAndReadWholeMember
 .ltorg
.global __wrap_NARC_AllocAndReadWholeMemberByIndexPair
.type __wrap_NARC_AllocAndReadWholeMemberByIndexPair,%function
__wrap_NARC_AllocAndReadWholeMemberByIndexPair:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag639
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NARC_AllocAndReadWholeMemberByIndexPair
 .ltorg
.global __wrap_NARC_AllocAtEndAndReadFromMemberByIndexPair
.type __wrap_NARC_AllocAtEndAndReadFromMemberByIndexPair,%function
__wrap_NARC_AllocAtEndAndReadFromMemberByIndexPair:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag640
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NARC_AllocAtEndAndReadFromMemberByIndexPair
 .ltorg
.global __wrap_NARC_AllocAtEndAndReadWholeMemberByIndexPair
.type __wrap_NARC_AllocAtEndAndReadWholeMemberByIndexPair,%function
__wrap_NARC_AllocAtEndAndReadWholeMemberByIndexPair:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag641
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NARC_AllocAtEndAndReadWholeMemberByIndexPair
 .ltorg
.global __wrap_NARC_GetFileCount
.type __wrap_NARC_GetFileCount,%function
__wrap_NARC_GetFileCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag642
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NARC_GetFileCount
 .ltorg
.global __wrap_NARC_GetMemberSize
.type __wrap_NARC_GetMemberSize,%function
__wrap_NARC_GetMemberSize:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag643
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NARC_GetMemberSize
 .ltorg
.global __wrap_NARC_ReadFile
.type __wrap_NARC_ReadFile,%function
__wrap_NARC_ReadFile:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag644
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NARC_ReadFile
 .ltorg
.global __wrap_NARC_ReadFromMember
.type __wrap_NARC_ReadFromMember,%function
__wrap_NARC_ReadFromMember:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag645
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NARC_ReadFromMember
 .ltorg
.global __wrap_NARC_ReadWholeMember
.type __wrap_NARC_ReadWholeMember,%function
__wrap_NARC_ReadWholeMember:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag646
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NARC_ReadWholeMember
 .ltorg
.global __wrap_NARC_ReadWholeMemberByIndexPair
.type __wrap_NARC_ReadWholeMemberByIndexPair,%function
__wrap_NARC_ReadWholeMemberByIndexPair:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag647
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NARC_ReadWholeMemberByIndexPair
 .ltorg
.global __wrap_NARC_Seek
.type __wrap_NARC_Seek,%function
__wrap_NARC_Seek:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag648
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NARC_Seek
 .ltorg
.global __wrap_NARC_ctor
.type __wrap_NARC_ctor,%function
__wrap_NARC_ctor:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag649
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NARC_ctor
 .ltorg
.global __wrap_NARC_dtor
.type __wrap_NARC_dtor,%function
__wrap_NARC_dtor:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag650
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NARC_dtor
 .ltorg
.global __wrap_NNS_G2dGetImagePaletteLocation
.type __wrap_NNS_G2dGetImagePaletteLocation,%function
__wrap_NNS_G2dGetImagePaletteLocation:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag651
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G2dGetImagePaletteLocation
 .ltorg
.global __wrap_NNS_G2dGetUnpackedCharacterData
.type __wrap_NNS_G2dGetUnpackedCharacterData,%function
__wrap_NNS_G2dGetUnpackedCharacterData:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag652
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G2dGetUnpackedCharacterData
 .ltorg
.global __wrap_NNS_G2dGetUnpackedPaletteData
.type __wrap_NNS_G2dGetUnpackedPaletteData,%function
__wrap_NNS_G2dGetUnpackedPaletteData:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag653
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G2dGetUnpackedPaletteData
 .ltorg
.global __wrap_NNS_G2dGetUnpackedScreenData
.type __wrap_NNS_G2dGetUnpackedScreenData,%function
__wrap_NNS_G2dGetUnpackedScreenData:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag654
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G2dGetUnpackedScreenData
 .ltorg
.global __wrap_NNS_G2dInitOamManagerModule
.type __wrap_NNS_G2dInitOamManagerModule,%function
__wrap_NNS_G2dInitOamManagerModule:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag655
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G2dInitOamManagerModule
 .ltorg
.global __wrap_NNS_G3dAllocAnmObj
.type __wrap_NNS_G3dAllocAnmObj,%function
__wrap_NNS_G3dAllocAnmObj:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag656
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dAllocAnmObj
 .ltorg
.global __wrap_NNS_G3dAnmObjInit
.type __wrap_NNS_G3dAnmObjInit,%function
__wrap_NNS_G3dAnmObjInit:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag657
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dAnmObjInit
 .ltorg
.global __wrap_NNS_G3dDraw1Mat1Shp
.type __wrap_NNS_G3dDraw1Mat1Shp,%function
__wrap_NNS_G3dDraw1Mat1Shp:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag658
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dDraw1Mat1Shp
 .ltorg
.global __wrap_NNS_G3dFreeAnmObj
.type __wrap_NNS_G3dFreeAnmObj,%function
__wrap_NNS_G3dFreeAnmObj:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag659
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dFreeAnmObj
 .ltorg
.global __wrap_NNS_G3dGetAnmByIdx
.type __wrap_NNS_G3dGetAnmByIdx,%function
__wrap_NNS_G3dGetAnmByIdx:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag660
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dGetAnmByIdx
 .ltorg
.global __wrap_NNS_G3dGetMdlSet
.type __wrap_NNS_G3dGetMdlSet,%function
__wrap_NNS_G3dGetMdlSet:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag661
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dGetMdlSet
 .ltorg
.global __wrap_NNS_G3dGetTex
.type __wrap_NNS_G3dGetTex,%function
__wrap_NNS_G3dGetTex:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag662
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dGetTex
 .ltorg
.global __wrap_NNS_G3dGlbFlushP
.type __wrap_NNS_G3dGlbFlushP,%function
__wrap_NNS_G3dGlbFlushP:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag663
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dGlbFlushP
 .ltorg
.global __wrap_NNS_G3dGlbGetInvV
.type __wrap_NNS_G3dGlbGetInvV,%function
__wrap_NNS_G3dGlbGetInvV:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag664
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dGlbGetInvV
 .ltorg
.global __wrap_NNS_G3dGlbLightColor
.type __wrap_NNS_G3dGlbLightColor,%function
__wrap_NNS_G3dGlbLightColor:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag665
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dGlbLightColor
 .ltorg
.global __wrap_NNS_G3dGlbLightVector
.type __wrap_NNS_G3dGlbLightVector,%function
__wrap_NNS_G3dGlbLightVector:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag666
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dGlbLightVector
 .ltorg
.global __wrap_NNS_G3dGlbMaterialColorDiffAmb
.type __wrap_NNS_G3dGlbMaterialColorDiffAmb,%function
__wrap_NNS_G3dGlbMaterialColorDiffAmb:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag667
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dGlbMaterialColorDiffAmb
 .ltorg
.global __wrap_NNS_G3dGlbMaterialColorSpecEmi
.type __wrap_NNS_G3dGlbMaterialColorSpecEmi,%function
__wrap_NNS_G3dGlbMaterialColorSpecEmi:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag668
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dGlbMaterialColorSpecEmi
 .ltorg
.global __wrap_NNS_G3dGlbPolygonAttr
.type __wrap_NNS_G3dGlbPolygonAttr,%function
__wrap_NNS_G3dGlbPolygonAttr:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag669
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dGlbPolygonAttr
 .ltorg
.global __wrap_NNS_G3dGlbSetBaseScale
.type __wrap_NNS_G3dGlbSetBaseScale,%function
__wrap_NNS_G3dGlbSetBaseScale:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag670
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dGlbSetBaseScale
 .ltorg
.global __wrap_NNS_G3dGlbSetBaseTrans
.type __wrap_NNS_G3dGlbSetBaseTrans,%function
__wrap_NNS_G3dGlbSetBaseTrans:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag671
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dGlbSetBaseTrans
 .ltorg
.global __wrap_NNS_G3dMdlSetMdl1DotAll
.type __wrap_NNS_G3dMdlSetMdl1DotAll,%function
__wrap_NNS_G3dMdlSetMdl1DotAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag672
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dMdlSetMdl1DotAll
 .ltorg
.global __wrap_NNS_G3dMdlSetMdlAlphaAll
.type __wrap_NNS_G3dMdlSetMdlAlphaAll,%function
__wrap_NNS_G3dMdlSetMdlAlphaAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag673
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dMdlSetMdlAlphaAll
 .ltorg
.global __wrap_NNS_G3dMdlSetMdlAmbAll
.type __wrap_NNS_G3dMdlSetMdlAmbAll,%function
__wrap_NNS_G3dMdlSetMdlAmbAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag674
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dMdlSetMdlAmbAll
 .ltorg
.global __wrap_NNS_G3dMdlSetMdlCullModeAll
.type __wrap_NNS_G3dMdlSetMdlCullModeAll,%function
__wrap_NNS_G3dMdlSetMdlCullModeAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag675
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dMdlSetMdlCullModeAll
 .ltorg
.global __wrap_NNS_G3dMdlSetMdlDepthTestCondAll
.type __wrap_NNS_G3dMdlSetMdlDepthTestCondAll,%function
__wrap_NNS_G3dMdlSetMdlDepthTestCondAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag676
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dMdlSetMdlDepthTestCondAll
 .ltorg
.global __wrap_NNS_G3dMdlSetMdlDiffAll
.type __wrap_NNS_G3dMdlSetMdlDiffAll,%function
__wrap_NNS_G3dMdlSetMdlDiffAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag677
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dMdlSetMdlDiffAll
 .ltorg
.global __wrap_NNS_G3dMdlSetMdlEmiAll
.type __wrap_NNS_G3dMdlSetMdlEmiAll,%function
__wrap_NNS_G3dMdlSetMdlEmiAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag678
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dMdlSetMdlEmiAll
 .ltorg
.global __wrap_NNS_G3dMdlSetMdlFarClipAll
.type __wrap_NNS_G3dMdlSetMdlFarClipAll,%function
__wrap_NNS_G3dMdlSetMdlFarClipAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag679
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dMdlSetMdlFarClipAll
 .ltorg
.global __wrap_NNS_G3dMdlSetMdlFogEnableFlagAll
.type __wrap_NNS_G3dMdlSetMdlFogEnableFlagAll,%function
__wrap_NNS_G3dMdlSetMdlFogEnableFlagAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag680
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dMdlSetMdlFogEnableFlagAll
 .ltorg
.global __wrap_NNS_G3dMdlSetMdlLightEnableFlagAll
.type __wrap_NNS_G3dMdlSetMdlLightEnableFlagAll,%function
__wrap_NNS_G3dMdlSetMdlLightEnableFlagAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag681
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dMdlSetMdlLightEnableFlagAll
 .ltorg
.global __wrap_NNS_G3dMdlSetMdlPolygonIDAll
.type __wrap_NNS_G3dMdlSetMdlPolygonIDAll,%function
__wrap_NNS_G3dMdlSetMdlPolygonIDAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag682
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dMdlSetMdlPolygonIDAll
 .ltorg
.global __wrap_NNS_G3dMdlSetMdlPolygonModeAll
.type __wrap_NNS_G3dMdlSetMdlPolygonModeAll,%function
__wrap_NNS_G3dMdlSetMdlPolygonModeAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag683
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dMdlSetMdlPolygonModeAll
 .ltorg
.global __wrap_NNS_G3dMdlSetMdlSpecAll
.type __wrap_NNS_G3dMdlSetMdlSpecAll,%function
__wrap_NNS_G3dMdlSetMdlSpecAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag684
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dMdlSetMdlSpecAll
 .ltorg
.global __wrap_NNS_G3dMdlSetMdlXLDepthUpdateAll
.type __wrap_NNS_G3dMdlSetMdlXLDepthUpdateAll,%function
__wrap_NNS_G3dMdlSetMdlXLDepthUpdateAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag685
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dMdlSetMdlXLDepthUpdateAll
 .ltorg
.global __wrap_NNS_G3dRenderObjAddAnmObj
.type __wrap_NNS_G3dRenderObjAddAnmObj,%function
__wrap_NNS_G3dRenderObjAddAnmObj:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag686
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dRenderObjAddAnmObj
 .ltorg
.global __wrap_NNS_G3dRenderObjInit
.type __wrap_NNS_G3dRenderObjInit,%function
__wrap_NNS_G3dRenderObjInit:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag687
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dRenderObjInit
 .ltorg
.global __wrap_NNS_G3dRenderObjRemoveAnmObj
.type __wrap_NNS_G3dRenderObjRemoveAnmObj,%function
__wrap_NNS_G3dRenderObjRemoveAnmObj:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag688
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_G3dRenderObjRemoveAnmObj
 .ltorg
.global __wrap_NNS_GfdResetLnkPlttVramState
.type __wrap_NNS_GfdResetLnkPlttVramState,%function
__wrap_NNS_GfdResetLnkPlttVramState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag689
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_GfdResetLnkPlttVramState
 .ltorg
.global __wrap_NNS_GfdResetLnkTexVramState
.type __wrap_NNS_GfdResetLnkTexVramState,%function
__wrap_NNS_GfdResetLnkTexVramState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag690
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNS_GfdResetLnkTexVramState
 .ltorg
.global __wrap_NNSi_G3dModifyMatFlag
.type __wrap_NNSi_G3dModifyMatFlag,%function
__wrap_NNSi_G3dModifyMatFlag:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag691
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNSi_G3dModifyMatFlag
 .ltorg
.global __wrap_NNSi_G3dModifyPolygonAttrMask
.type __wrap_NNSi_G3dModifyPolygonAttrMask,%function
__wrap_NNSi_G3dModifyPolygonAttrMask:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag692
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_NNSi_G3dModifyPolygonAttrMask
 .ltorg
.global __wrap_Options_Frame
.type __wrap_Options_Frame,%function
__wrap_Options_Frame:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag693
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Options_Frame
 .ltorg
.global __wrap_Options_TextFrameDelay
.type __wrap_Options_TextFrameDelay,%function
__wrap_Options_TextFrameDelay:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag694
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Options_TextFrameDelay
 .ltorg
.global __wrap_Overlay_LoadByID
.type __wrap_Overlay_LoadByID,%function
__wrap_Overlay_LoadByID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag695
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Overlay_LoadByID
 .ltorg
.global __wrap_Overlay_UnloadByID
.type __wrap_Overlay_UnloadByID,%function
__wrap_Overlay_UnloadByID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag696
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Overlay_UnloadByID
 .ltorg
.global __wrap_OverworldAnimManagerList_FinishAndFree
.type __wrap_OverworldAnimManagerList_FinishAndFree,%function
__wrap_OverworldAnimManagerList_FinishAndFree:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag697
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_OverworldAnimManagerList_FinishAndFree
 .ltorg
.global __wrap_OverworldAnimManagerList_InitManager
.type __wrap_OverworldAnimManagerList_InitManager,%function
__wrap_OverworldAnimManagerList_InitManager:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag698
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_OverworldAnimManagerList_InitManager
 .ltorg
.global __wrap_OverworldAnimManagerList_New
.type __wrap_OverworldAnimManagerList_New,%function
__wrap_OverworldAnimManagerList_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag699
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_OverworldAnimManagerList_New
 .ltorg
.global __wrap_OverworldAnimManagerList_Render
.type __wrap_OverworldAnimManagerList_Render,%function
__wrap_OverworldAnimManagerList_Render:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag700
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_OverworldAnimManagerList_Render
 .ltorg
.global __wrap_OverworldAnimManager_CallTickFunc
.type __wrap_OverworldAnimManager_CallTickFunc,%function
__wrap_OverworldAnimManager_CallTickFunc:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag701
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_OverworldAnimManager_CallTickFunc
 .ltorg
.global __wrap_OverworldAnimManager_Finish
.type __wrap_OverworldAnimManager_Finish,%function
__wrap_OverworldAnimManager_Finish:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag702
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_OverworldAnimManager_Finish
 .ltorg
.global __wrap_OverworldAnimManager_GetFuncsContext
.type __wrap_OverworldAnimManager_GetFuncsContext,%function
__wrap_OverworldAnimManager_GetFuncsContext:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag703
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_OverworldAnimManager_GetFuncsContext
 .ltorg
.global __wrap_OverworldAnimManager_GetPosition
.type __wrap_OverworldAnimManager_GetPosition,%function
__wrap_OverworldAnimManager_GetPosition:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag704
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_OverworldAnimManager_GetPosition
 .ltorg
.global __wrap_OverworldAnimManager_GetUserData
.type __wrap_OverworldAnimManager_GetUserData,%function
__wrap_OverworldAnimManager_GetUserData:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag705
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_OverworldAnimManager_GetUserData
 .ltorg
.global __wrap_OverworldAnimManager_GetUserInt
.type __wrap_OverworldAnimManager_GetUserInt,%function
__wrap_OverworldAnimManager_GetUserInt:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag706
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_OverworldAnimManager_GetUserInt
 .ltorg
.global __wrap_OverworldAnimManager_SetPosition
.type __wrap_OverworldAnimManager_SetPosition,%function
__wrap_OverworldAnimManager_SetPosition:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag707
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_OverworldAnimManager_SetPosition
 .ltorg
.global __wrap_OverworldMapHistory_Push
.type __wrap_OverworldMapHistory_Push,%function
__wrap_OverworldMapHistory_Push:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag708
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_OverworldMapHistory_Push
 .ltorg
.global __wrap_ParticleSystem_CreateEmitter
.type __wrap_ParticleSystem_CreateEmitter,%function
__wrap_ParticleSystem_CreateEmitter:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag709
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ParticleSystem_CreateEmitter
 .ltorg
.global __wrap_ParticleSystem_DrawAll
.type __wrap_ParticleSystem_DrawAll,%function
__wrap_ParticleSystem_DrawAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag710
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ParticleSystem_DrawAll
 .ltorg
.global __wrap_ParticleSystem_Free
.type __wrap_ParticleSystem_Free,%function
__wrap_ParticleSystem_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag711
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ParticleSystem_Free
 .ltorg
.global __wrap_ParticleSystem_FreeAll
.type __wrap_ParticleSystem_FreeAll,%function
__wrap_ParticleSystem_FreeAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag712
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ParticleSystem_FreeAll
 .ltorg
.global __wrap_ParticleSystem_GetActiveEmitterCount
.type __wrap_ParticleSystem_GetActiveEmitterCount,%function
__wrap_ParticleSystem_GetActiveEmitterCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag713
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ParticleSystem_GetActiveEmitterCount
 .ltorg
.global __wrap_ParticleSystem_GetCamera
.type __wrap_ParticleSystem_GetCamera,%function
__wrap_ParticleSystem_GetCamera:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag714
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ParticleSystem_GetCamera
 .ltorg
.global __wrap_ParticleSystem_New
.type __wrap_ParticleSystem_New,%function
__wrap_ParticleSystem_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag715
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ParticleSystem_New
 .ltorg
.global __wrap_ParticleSystem_SetCameraProjection
.type __wrap_ParticleSystem_SetCameraProjection,%function
__wrap_ParticleSystem_SetCameraProjection:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag716
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ParticleSystem_SetCameraProjection
 .ltorg
.global __wrap_ParticleSystem_SetResource
.type __wrap_ParticleSystem_SetResource,%function
__wrap_ParticleSystem_SetResource:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag717
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ParticleSystem_SetResource
 .ltorg
.global __wrap_ParticleSystem_UpdateAll
.type __wrap_ParticleSystem_UpdateAll,%function
__wrap_ParticleSystem_UpdateAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag718
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ParticleSystem_UpdateAll
 .ltorg
.global __wrap_ParticleSystem_ZeroAll
.type __wrap_ParticleSystem_ZeroAll,%function
__wrap_ParticleSystem_ZeroAll:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag719
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ParticleSystem_ZeroAll
 .ltorg
.global __wrap_Party_AddPokemon
.type __wrap_Party_AddPokemon,%function
__wrap_Party_AddPokemon:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag720
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Party_AddPokemon
 .ltorg
.global __wrap_Party_FindFirstEligibleBattler
.type __wrap_Party_FindFirstEligibleBattler,%function
__wrap_Party_FindFirstEligibleBattler:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag721
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Party_FindFirstEligibleBattler
 .ltorg
.global __wrap_Party_GetCurrentCount
.type __wrap_Party_GetCurrentCount,%function
__wrap_Party_GetCurrentCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag722
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Party_GetCurrentCount
 .ltorg
.global __wrap_Party_GetPokemonBySlotIndex
.type __wrap_Party_GetPokemonBySlotIndex,%function
__wrap_Party_GetPokemonBySlotIndex:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag723
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Party_GetPokemonBySlotIndex
 .ltorg
.global __wrap_Party_HasMonWithMove
.type __wrap_Party_HasMonWithMove,%function
__wrap_Party_HasMonWithMove:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag724
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Party_HasMonWithMove
 .ltorg
.global __wrap_Party_HasSpecies
.type __wrap_Party_HasSpecies,%function
__wrap_Party_HasSpecies:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag725
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Party_HasSpecies
 .ltorg
.global __wrap_Party_HasTwoAliveMons
.type __wrap_Party_HasTwoAliveMons,%function
__wrap_Party_HasTwoAliveMons:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag726
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Party_HasTwoAliveMons
 .ltorg
.global __wrap_Party_RemovePokemonBySlotIndex
.type __wrap_Party_RemovePokemonBySlotIndex,%function
__wrap_Party_RemovePokemonBySlotIndex:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag727
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Party_RemovePokemonBySlotIndex
 .ltorg
.global __wrap_Party_ResetMonMoveSlot
.type __wrap_Party_ResetMonMoveSlot,%function
__wrap_Party_ResetMonMoveSlot:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag728
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Party_ResetMonMoveSlot
 .ltorg
.global __wrap_PersistedMapFeatures_GetBuffer
.type __wrap_PersistedMapFeatures_GetBuffer,%function
__wrap_PersistedMapFeatures_GetBuffer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag729
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PersistedMapFeatures_GetBuffer
 .ltorg
.global __wrap_PersistedMapFeatures_GetID
.type __wrap_PersistedMapFeatures_GetID,%function
__wrap_PersistedMapFeatures_GetID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag730
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PersistedMapFeatures_GetID
 .ltorg
.global __wrap_PersistedMapFeatures_IsCurrentDynamicMap
.type __wrap_PersistedMapFeatures_IsCurrentDynamicMap,%function
__wrap_PersistedMapFeatures_IsCurrentDynamicMap:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag731
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PersistedMapFeatures_IsCurrentDynamicMap
 .ltorg
.global __wrap_PlayTime_GetHours
.type __wrap_PlayTime_GetHours,%function
__wrap_PlayTime_GetHours:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag732
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayTime_GetHours
 .ltorg
.global __wrap_PlayTime_GetMinutes
.type __wrap_PlayTime_GetMinutes,%function
__wrap_PlayTime_GetMinutes:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag733
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayTime_GetMinutes
 .ltorg
.global __wrap_PlayerAvatar_CalcFaceDirection
.type __wrap_PlayerAvatar_CalcFaceDirection,%function
__wrap_PlayerAvatar_CalcFaceDirection:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag734
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_CalcFaceDirection
 .ltorg
.global __wrap_PlayerAvatar_CanUseRockClimb
.type __wrap_PlayerAvatar_CanUseRockClimb,%function
__wrap_PlayerAvatar_CanUseRockClimb:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag735
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_CanUseRockClimb
 .ltorg
.global __wrap_PlayerAvatar_CanUseSurf
.type __wrap_PlayerAvatar_CanUseSurf,%function
__wrap_PlayerAvatar_CanUseSurf:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag736
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_CanUseSurf
 .ltorg
.global __wrap_PlayerAvatar_CheckCollision
.type __wrap_PlayerAvatar_CheckCollision,%function
__wrap_PlayerAvatar_CheckCollision:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag737
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_CheckCollision
 .ltorg
.global __wrap_PlayerAvatar_CheckDistortionCollision
.type __wrap_PlayerAvatar_CheckDistortionCollision,%function
__wrap_PlayerAvatar_CheckDistortionCollision:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag738
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_CheckDistortionCollision
 .ltorg
.global __wrap_PlayerAvatar_CheckEscapedFromDeepMud
.type __wrap_PlayerAvatar_CheckEscapedFromDeepMud,%function
__wrap_PlayerAvatar_CheckEscapedFromDeepMud:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag739
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_CheckEscapedFromDeepMud
 .ltorg
.global __wrap_PlayerAvatar_CheckForceStopMovement
.type __wrap_PlayerAvatar_CheckForceStopMovement,%function
__wrap_PlayerAvatar_CheckForceStopMovement:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag740
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_CheckForceStopMovement
 .ltorg
.global __wrap_PlayerAvatar_CheckForcedMovement
.type __wrap_PlayerAvatar_CheckForcedMovement,%function
__wrap_PlayerAvatar_CheckForcedMovement:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag741
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_CheckForcedMovement
 .ltorg
.global __wrap_PlayerAvatar_CheckStartMove
.type __wrap_PlayerAvatar_CheckStartMove,%function
__wrap_PlayerAvatar_CheckStartMove:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag742
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_CheckStartMove
 .ltorg
.global __wrap_PlayerAvatar_ClearMoveState
.type __wrap_PlayerAvatar_ClearMoveState,%function
__wrap_PlayerAvatar_ClearMoveState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag743
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_ClearMoveState
 .ltorg
.global __wrap_PlayerAvatar_ClearSpeed
.type __wrap_PlayerAvatar_ClearSpeed,%function
__wrap_PlayerAvatar_ClearSpeed:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag744
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_ClearSpeed
 .ltorg
.global __wrap_PlayerAvatar_DistortionGravityChanged
.type __wrap_PlayerAvatar_DistortionGravityChanged,%function
__wrap_PlayerAvatar_DistortionGravityChanged:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag745
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_DistortionGravityChanged
 .ltorg
.global __wrap_PlayerAvatar_DistortionStateOnFloor
.type __wrap_PlayerAvatar_DistortionStateOnFloor,%function
__wrap_PlayerAvatar_DistortionStateOnFloor:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag746
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_DistortionStateOnFloor
 .ltorg
.global __wrap_PlayerAvatar_ForceStopMovement
.type __wrap_PlayerAvatar_ForceStopMovement,%function
__wrap_PlayerAvatar_ForceStopMovement:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag747
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_ForceStopMovement
 .ltorg
.global __wrap_PlayerAvatar_GetDistortionCurrTileBehaviour
.type __wrap_PlayerAvatar_GetDistortionCurrTileBehaviour,%function
__wrap_PlayerAvatar_GetDistortionCurrTileBehaviour:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag748
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetDistortionCurrTileBehaviour
 .ltorg
.global __wrap_PlayerAvatar_GetDistortionDir
.type __wrap_PlayerAvatar_GetDistortionDir,%function
__wrap_PlayerAvatar_GetDistortionDir:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag749
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetDistortionDir
 .ltorg
.global __wrap_PlayerAvatar_GetDistortionFacingTileBehaviour
.type __wrap_PlayerAvatar_GetDistortionFacingTileBehaviour,%function
__wrap_PlayerAvatar_GetDistortionFacingTileBehaviour:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag750
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetDistortionFacingTileBehaviour
 .ltorg
.global __wrap_PlayerAvatar_GetFacingDir
.type __wrap_PlayerAvatar_GetFacingDir,%function
__wrap_PlayerAvatar_GetFacingDir:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag751
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetFacingDir
 .ltorg
.global __wrap_PlayerAvatar_GetFacingDistortionWorldPos
.type __wrap_PlayerAvatar_GetFacingDistortionWorldPos,%function
__wrap_PlayerAvatar_GetFacingDistortionWorldPos:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag752
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetFacingDistortionWorldPos
 .ltorg
.global __wrap_PlayerAvatar_GetGender
.type __wrap_PlayerAvatar_GetGender,%function
__wrap_PlayerAvatar_GetGender:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag753
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetGender
 .ltorg
.global __wrap_PlayerAvatar_GetMapObject
.type __wrap_PlayerAvatar_GetMapObject,%function
__wrap_PlayerAvatar_GetMapObject:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag754
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetMapObject
 .ltorg
.global __wrap_PlayerAvatar_GetMoveState
.type __wrap_PlayerAvatar_GetMoveState,%function
__wrap_PlayerAvatar_GetMoveState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag755
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetMoveState
 .ltorg
.global __wrap_PlayerAvatar_GetMoveStateFromPlayerAvatarState
.type __wrap_PlayerAvatar_GetMoveStateFromPlayerAvatarState,%function
__wrap_PlayerAvatar_GetMoveStateFromPlayerAvatarState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag756
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetMoveStateFromPlayerAvatarState
 .ltorg
.global __wrap_PlayerAvatar_GetMovingDir
.type __wrap_PlayerAvatar_GetMovingDir,%function
__wrap_PlayerAvatar_GetMovingDir:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag757
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetMovingDir
 .ltorg
.global __wrap_PlayerAvatar_GetPlayerMoveState
.type __wrap_PlayerAvatar_GetPlayerMoveState,%function
__wrap_PlayerAvatar_GetPlayerMoveState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag758
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetPlayerMoveState
 .ltorg
.global __wrap_PlayerAvatar_GetPlayerState
.type __wrap_PlayerAvatar_GetPlayerState,%function
__wrap_PlayerAvatar_GetPlayerState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag759
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetPlayerState
 .ltorg
.global __wrap_PlayerAvatar_GetPos
.type __wrap_PlayerAvatar_GetPos,%function
__wrap_PlayerAvatar_GetPos:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag760
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetPos
 .ltorg
.global __wrap_PlayerAvatar_GetRequestStateFlag
.type __wrap_PlayerAvatar_GetRequestStateFlag,%function
__wrap_PlayerAvatar_GetRequestStateFlag:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag761
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetRequestStateFlag
 .ltorg
.global __wrap_PlayerAvatar_GetSpeed
.type __wrap_PlayerAvatar_GetSpeed,%function
__wrap_PlayerAvatar_GetSpeed:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag762
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetSpeed
 .ltorg
.global __wrap_PlayerAvatar_GetStepDistortionWorldPos
.type __wrap_PlayerAvatar_GetStepDistortionWorldPos,%function
__wrap_PlayerAvatar_GetStepDistortionWorldPos:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag763
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetStepDistortionWorldPos
 .ltorg
.global __wrap_PlayerAvatar_GetSurfMountAnimManager
.type __wrap_PlayerAvatar_GetSurfMountAnimManager,%function
__wrap_PlayerAvatar_GetSurfMountAnimManager:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag764
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetSurfMountAnimManager
 .ltorg
.global __wrap_PlayerAvatar_GetTileBehaviorFromDir
.type __wrap_PlayerAvatar_GetTileBehaviorFromDir,%function
__wrap_PlayerAvatar_GetTileBehaviorFromDir:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag765
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetTileBehaviorFromDir
 .ltorg
.global __wrap_PlayerAvatar_GetXPos
.type __wrap_PlayerAvatar_GetXPos,%function
__wrap_PlayerAvatar_GetXPos:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag766
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetXPos
 .ltorg
.global __wrap_PlayerAvatar_GetZPos
.type __wrap_PlayerAvatar_GetZPos,%function
__wrap_PlayerAvatar_GetZPos:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag767
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_GetZPos
 .ltorg
.global __wrap_PlayerAvatar_InitMapFeatures
.type __wrap_PlayerAvatar_InitMapFeatures,%function
__wrap_PlayerAvatar_InitMapFeatures:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag768
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_InitMapFeatures
 .ltorg
.global __wrap_PlayerAvatar_IsAnimationSetOrWalkOnSpotSlow
.type __wrap_PlayerAvatar_IsAnimationSetOrWalkOnSpotSlow,%function
__wrap_PlayerAvatar_IsAnimationSetOrWalkOnSpotSlow:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag769
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_IsAnimationSetOrWalkOnSpotSlow
 .ltorg
.global __wrap_PlayerAvatar_IsMapObjectAnimationSet
.type __wrap_PlayerAvatar_IsMapObjectAnimationSet,%function
__wrap_PlayerAvatar_IsMapObjectAnimationSet:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag770
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_IsMapObjectAnimationSet
 .ltorg
.global __wrap_PlayerAvatar_MapDistortionState
.type __wrap_PlayerAvatar_MapDistortionState,%function
__wrap_PlayerAvatar_MapDistortionState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag771
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_MapDistortionState
 .ltorg
.global __wrap_PlayerAvatar_RequestChangeState
.type __wrap_PlayerAvatar_RequestChangeState,%function
__wrap_PlayerAvatar_RequestChangeState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag772
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_RequestChangeState
 .ltorg
.global __wrap_PlayerAvatar_SetEscapedFromDeepMud
.type __wrap_PlayerAvatar_SetEscapedFromDeepMud,%function
__wrap_PlayerAvatar_SetEscapedFromDeepMud:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag773
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_SetEscapedFromDeepMud
 .ltorg
.global __wrap_PlayerAvatar_SetIgnoreTileBehavior
.type __wrap_PlayerAvatar_SetIgnoreTileBehavior,%function
__wrap_PlayerAvatar_SetIgnoreTileBehavior:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag774
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_SetIgnoreTileBehavior
 .ltorg
.global __wrap_PlayerAvatar_SetMapObjMovement
.type __wrap_PlayerAvatar_SetMapObjMovement,%function
__wrap_PlayerAvatar_SetMapObjMovement:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag775
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_SetMapObjMovement
 .ltorg
.global __wrap_PlayerAvatar_SetPlayerState
.type __wrap_PlayerAvatar_SetPlayerState,%function
__wrap_PlayerAvatar_SetPlayerState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag776
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_SetPlayerState
 .ltorg
.global __wrap_PlayerAvatar_SetRequestStateFlag
.type __wrap_PlayerAvatar_SetRequestStateFlag,%function
__wrap_PlayerAvatar_SetRequestStateFlag:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag777
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_SetRequestStateFlag
 .ltorg
.global __wrap_PlayerAvatar_SetSurfMountAnimManager
.type __wrap_PlayerAvatar_SetSurfMountAnimManager,%function
__wrap_PlayerAvatar_SetSurfMountAnimManager:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag778
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_SetSurfMountAnimManager
 .ltorg
.global __wrap_PlayerAvatar_SetTransitionState
.type __wrap_PlayerAvatar_SetTransitionState,%function
__wrap_PlayerAvatar_SetTransitionState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag779
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_SetTransitionState
 .ltorg
.global __wrap_PlayerAvatar_TryFace
.type __wrap_PlayerAvatar_TryFace,%function
__wrap_PlayerAvatar_TryFace:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag780
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_TryFace
 .ltorg
.global __wrap_PlayerAvatar_TurnOnRequestStateBit
.type __wrap_PlayerAvatar_TurnOnRequestStateBit,%function
__wrap_PlayerAvatar_TurnOnRequestStateBit:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag781
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlayerAvatar_TurnOnRequestStateBit
 .ltorg
.global __wrap_Player_ConvertStateToTransition
.type __wrap_Player_ConvertStateToTransition,%function
__wrap_Player_ConvertStateToTransition:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag782
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Player_ConvertStateToTransition
 .ltorg
.global __wrap_Player_GetSpriteFromStateAndGender
.type __wrap_Player_GetSpriteFromStateAndGender,%function
__wrap_Player_GetSpriteFromStateAndGender:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag783
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Player_GetSpriteFromStateAndGender
 .ltorg
.global __wrap_PlttTransfer_Clear
.type __wrap_PlttTransfer_Clear,%function
__wrap_PlttTransfer_Clear:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag784
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlttTransfer_Clear
 .ltorg
.global __wrap_PlttTransfer_Free
.type __wrap_PlttTransfer_Free,%function
__wrap_PlttTransfer_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag785
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlttTransfer_Free
 .ltorg
.global __wrap_PlttTransfer_Init
.type __wrap_PlttTransfer_Init,%function
__wrap_PlttTransfer_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag786
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PlttTransfer_Init
 .ltorg
.global __wrap_PokeRadar_ClearIfAllOutOfView
.type __wrap_PokeRadar_ClearIfAllOutOfView,%function
__wrap_PokeRadar_ClearIfAllOutOfView:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag787
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PokeRadar_ClearIfAllOutOfView
 .ltorg
.global __wrap_Pokedex_Capture
.type __wrap_Pokedex_Capture,%function
__wrap_Pokedex_Capture:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag788
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokedex_Capture
 .ltorg
.global __wrap_Pokedex_CountSeen
.type __wrap_Pokedex_CountSeen,%function
__wrap_Pokedex_CountSeen:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag789
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokedex_CountSeen
 .ltorg
.global __wrap_Pokedex_HasSeenSpecies
.type __wrap_Pokedex_HasSeenSpecies,%function
__wrap_Pokedex_HasSeenSpecies:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag790
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokedex_HasSeenSpecies
 .ltorg
.global __wrap_Pokedex_HeightWeightData
.type __wrap_Pokedex_HeightWeightData,%function
__wrap_Pokedex_HeightWeightData:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag791
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokedex_HeightWeightData
 .ltorg
.global __wrap_Pokedex_HeightWeightData_Free
.type __wrap_Pokedex_HeightWeightData_Free,%function
__wrap_Pokedex_HeightWeightData_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag792
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokedex_HeightWeightData_Free
 .ltorg
.global __wrap_Pokedex_HeightWeightData_Height
.type __wrap_Pokedex_HeightWeightData_Height,%function
__wrap_Pokedex_HeightWeightData_Height:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag793
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokedex_HeightWeightData_Height
 .ltorg
.global __wrap_Pokedex_HeightWeightData_Load
.type __wrap_Pokedex_HeightWeightData_Load,%function
__wrap_Pokedex_HeightWeightData_Load:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag794
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokedex_HeightWeightData_Load
 .ltorg
.global __wrap_Pokedex_HeightWeightData_Release
.type __wrap_Pokedex_HeightWeightData_Release,%function
__wrap_Pokedex_HeightWeightData_Release:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag795
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokedex_HeightWeightData_Release
 .ltorg
.global __wrap_Pokedex_IsObtained
.type __wrap_Pokedex_IsObtained,%function
__wrap_Pokedex_IsObtained:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag796
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokedex_IsObtained
 .ltorg
.global __wrap_Pokemon_AddMove
.type __wrap_Pokemon_AddMove,%function
__wrap_Pokemon_AddMove:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag797
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_AddMove
 .ltorg
.global __wrap_Pokemon_CalcLevelAndStats
.type __wrap_Pokemon_CalcLevelAndStats,%function
__wrap_Pokemon_CalcLevelAndStats:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag798
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_CalcLevelAndStats
 .ltorg
.global __wrap_Pokemon_Copy
.type __wrap_Pokemon_Copy,%function
__wrap_Pokemon_Copy:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag799
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_Copy
 .ltorg
.global __wrap_Pokemon_DoPoisonDamage
.type __wrap_Pokemon_DoPoisonDamage,%function
__wrap_Pokemon_DoPoisonDamage:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag800
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_DoPoisonDamage
 .ltorg
.global __wrap_Pokemon_FromBoxPokemon
.type __wrap_Pokemon_FromBoxPokemon,%function
__wrap_Pokemon_FromBoxPokemon:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag801
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_FromBoxPokemon
 .ltorg
.global __wrap_Pokemon_GetBaseSpeciesFromPersonalData
.type __wrap_Pokemon_GetBaseSpeciesFromPersonalData,%function
__wrap_Pokemon_GetBaseSpeciesFromPersonalData:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag802
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_GetBaseSpeciesFromPersonalData
 .ltorg
.global __wrap_Pokemon_GetBoxPokemon
.type __wrap_Pokemon_GetBoxPokemon,%function
__wrap_Pokemon_GetBoxPokemon:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag803
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_GetBoxPokemon
 .ltorg
.global __wrap_Pokemon_GetGenderOf
.type __wrap_Pokemon_GetGenderOf,%function
__wrap_Pokemon_GetGenderOf:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag804
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_GetGenderOf
 .ltorg
.global __wrap_Pokemon_GetNatureOf
.type __wrap_Pokemon_GetNatureOf,%function
__wrap_Pokemon_GetNatureOf:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag805
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_GetNatureOf
 .ltorg
.global __wrap_Pokemon_GetValue
.type __wrap_Pokemon_GetValue,%function
__wrap_Pokemon_GetValue:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag806
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_GetValue
 .ltorg
.global __wrap_Pokemon_InitWith
.type __wrap_Pokemon_InitWith,%function
__wrap_Pokemon_InitWith:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag807
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_InitWith
 .ltorg
.global __wrap_Pokemon_IsPersonalityShiny
.type __wrap_Pokemon_IsPersonalityShiny,%function
__wrap_Pokemon_IsPersonalityShiny:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag808
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_IsPersonalityShiny
 .ltorg
.global __wrap_Pokemon_LevelUpMove
.type __wrap_Pokemon_LevelUpMove,%function
__wrap_Pokemon_LevelUpMove:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag809
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_LevelUpMove
 .ltorg
.global __wrap_Pokemon_LoadLevelUpMoveIdsOf
.type __wrap_Pokemon_LoadLevelUpMoveIdsOf,%function
__wrap_Pokemon_LoadLevelUpMoveIdsOf:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag810
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_LoadLevelUpMoveIdsOf
 .ltorg
.global __wrap_Pokemon_New
.type __wrap_Pokemon_New,%function
__wrap_Pokemon_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag811
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_New
 .ltorg
.global __wrap_Pokemon_ReplaceMove
.type __wrap_Pokemon_ReplaceMove,%function
__wrap_Pokemon_ReplaceMove:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag812
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_ReplaceMove
 .ltorg
.global __wrap_Pokemon_SetGiratinaFormByHeldItem
.type __wrap_Pokemon_SetGiratinaFormByHeldItem,%function
__wrap_Pokemon_SetGiratinaFormByHeldItem:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag813
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_SetGiratinaFormByHeldItem
 .ltorg
.global __wrap_Pokemon_SetRotomForm
.type __wrap_Pokemon_SetRotomForm,%function
__wrap_Pokemon_SetRotomForm:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag814
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_SetRotomForm
 .ltorg
.global __wrap_Pokemon_SetShayminForm
.type __wrap_Pokemon_SetShayminForm,%function
__wrap_Pokemon_SetShayminForm:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag815
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_SetShayminForm
 .ltorg
.global __wrap_Pokemon_SetValue
.type __wrap_Pokemon_SetValue,%function
__wrap_Pokemon_SetValue:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag816
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_SetValue
 .ltorg
.global __wrap_Pokemon_ShouldLevelUp
.type __wrap_Pokemon_ShouldLevelUp,%function
__wrap_Pokemon_ShouldLevelUp:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag817
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_ShouldLevelUp
 .ltorg
.global __wrap_Pokemon_UpdateFriendship
.type __wrap_Pokemon_UpdateFriendship,%function
__wrap_Pokemon_UpdateFriendship:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag818
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Pokemon_UpdateFriendship
 .ltorg
.global __wrap_PoketchSystem_Create
.type __wrap_PoketchSystem_Create,%function
__wrap_PoketchSystem_Create:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag819
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PoketchSystem_Create
 .ltorg
.global __wrap_PoketchSystem_IsSystemShutdown
.type __wrap_PoketchSystem_IsSystemShutdown,%function
__wrap_PoketchSystem_IsSystemShutdown:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag820
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PoketchSystem_IsSystemShutdown
 .ltorg
.global __wrap_PoketchSystem_SendEvent
.type __wrap_PoketchSystem_SendEvent,%function
__wrap_PoketchSystem_SendEvent:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag821
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PoketchSystem_SendEvent
 .ltorg
.global __wrap_PoketchSystem_StartShutdown
.type __wrap_PoketchSystem_StartShutdown,%function
__wrap_PoketchSystem_StartShutdown:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag822
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PoketchSystem_StartShutdown
 .ltorg
.global __wrap_PoketchUnavailableScreen_Exit
.type __wrap_PoketchUnavailableScreen_Exit,%function
__wrap_PoketchUnavailableScreen_Exit:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag823
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PoketchUnavailableScreen_Exit
 .ltorg
.global __wrap_PoketchUnavailableScreen_Init
.type __wrap_PoketchUnavailableScreen_Init,%function
__wrap_PoketchUnavailableScreen_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag824
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PoketchUnavailableScreen_Init
 .ltorg
.global __wrap_PoketchUnavailableScreen_IsDone
.type __wrap_PoketchUnavailableScreen_IsDone,%function
__wrap_PoketchUnavailableScreen_IsDone:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag825
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_PoketchUnavailableScreen_IsDone
 .ltorg
.global __wrap_Poketch_Enable
.type __wrap_Poketch_Enable,%function
__wrap_Poketch_Enable:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag826
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Poketch_Enable
 .ltorg
.global __wrap_Poketch_IsEnabled
.type __wrap_Poketch_IsEnabled,%function
__wrap_Poketch_IsEnabled:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag827
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Poketch_IsEnabled
 .ltorg
.global __wrap_QuadraticInterpolationTaskFX32_Init
.type __wrap_QuadraticInterpolationTaskFX32_Init,%function
__wrap_QuadraticInterpolationTaskFX32_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag828
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_QuadraticInterpolationTaskFX32_Init
 .ltorg
.global __wrap_QuadraticInterpolationTaskFX32_Update
.type __wrap_QuadraticInterpolationTaskFX32_Update,%function
__wrap_QuadraticInterpolationTaskFX32_Update:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag829
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_QuadraticInterpolationTaskFX32_Update
 .ltorg
.global __wrap_RadarChain_Clear
.type __wrap_RadarChain_Clear,%function
__wrap_RadarChain_Clear:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag830
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_RadarChain_Clear
 .ltorg
.global __wrap_RadarChargeStep
.type __wrap_RadarChargeStep,%function
__wrap_RadarChargeStep:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag831
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_RadarChargeStep
 .ltorg
.global __wrap_ReadFileToHeap
.type __wrap_ReadFileToHeap,%function
__wrap_ReadFileToHeap:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag832
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ReadFileToHeap
 .ltorg
.global __wrap_RenderOam_Free
.type __wrap_RenderOam_Free,%function
__wrap_RenderOam_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag833
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_RenderOam_Free
 .ltorg
.global __wrap_RenderOam_GetScreenOam
.type __wrap_RenderOam_GetScreenOam,%function
__wrap_RenderOam_GetScreenOam:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag834
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_RenderOam_GetScreenOam
 .ltorg
.global __wrap_RenderOam_Init
.type __wrap_RenderOam_Init,%function
__wrap_RenderOam_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag835
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_RenderOam_Init
 .ltorg
.global __wrap_RenderOam_InitSurface
.type __wrap_RenderOam_InitSurface,%function
__wrap_RenderOam_InitSurface:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag836
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_RenderOam_InitSurface
 .ltorg
.global __wrap_RenderOam_Transfer
.type __wrap_RenderOam_Transfer,%function
__wrap_RenderOam_Transfer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag837
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_RenderOam_Transfer
 .ltorg
.global __wrap_Repel_UpdateSteps
.type __wrap_Repel_UpdateSteps,%function
__wrap_Repel_UpdateSteps:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag838
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Repel_UpdateSteps
 .ltorg
.global __wrap_ResetVisibleHardwareWindows
.type __wrap_ResetVisibleHardwareWindows,%function
__wrap_ResetVisibleHardwareWindows:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag839
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ResetVisibleHardwareWindows
 .ltorg
.global __wrap_ResourceHeap_Free
.type __wrap_ResourceHeap_Free,%function
__wrap_ResourceHeap_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag840
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ResourceHeap_Free
 .ltorg
.global __wrap_ResourceHeap_FreeItem
.type __wrap_ResourceHeap_FreeItem,%function
__wrap_ResourceHeap_FreeItem:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag841
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ResourceHeap_FreeItem
 .ltorg
.global __wrap_ResourceHeap_GetItemData
.type __wrap_ResourceHeap_GetItemData,%function
__wrap_ResourceHeap_GetItemData:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag842
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ResourceHeap_GetItemData
 .ltorg
.global __wrap_ResourceHeap_HasItem
.type __wrap_ResourceHeap_HasItem,%function
__wrap_ResourceHeap_HasItem:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag843
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ResourceHeap_HasItem
 .ltorg
.global __wrap_ResourceHeap_LoadMemberFromNARC
.type __wrap_ResourceHeap_LoadMemberFromNARC,%function
__wrap_ResourceHeap_LoadMemberFromNARC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag844
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ResourceHeap_LoadMemberFromNARC
 .ltorg
.global __wrap_ResourceHeap_New
.type __wrap_ResourceHeap_New,%function
__wrap_ResourceHeap_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag845
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ResourceHeap_New
 .ltorg
.global __wrap_SaveData_GetBag
.type __wrap_SaveData_GetBag,%function
__wrap_SaveData_GetBag:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag846
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_GetBag
 .ltorg
.global __wrap_SaveData_GetBattleFrontier
.type __wrap_SaveData_GetBattleFrontier,%function
__wrap_SaveData_GetBattleFrontier:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag847
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_GetBattleFrontier
 .ltorg
.global __wrap_SaveData_GetChatotCry
.type __wrap_SaveData_GetChatotCry,%function
__wrap_SaveData_GetChatotCry:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag848
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_GetChatotCry
 .ltorg
.global __wrap_SaveData_GetCoins
.type __wrap_SaveData_GetCoins,%function
__wrap_SaveData_GetCoins:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag849
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_GetCoins
 .ltorg
.global __wrap_SaveData_GetDaycare
.type __wrap_SaveData_GetDaycare,%function
__wrap_SaveData_GetDaycare:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag850
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_GetDaycare
 .ltorg
.global __wrap_SaveData_GetFieldOverworldState
.type __wrap_SaveData_GetFieldOverworldState,%function
__wrap_SaveData_GetFieldOverworldState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag851
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_GetFieldOverworldState
 .ltorg
.global __wrap_SaveData_GetGameRecords
.type __wrap_SaveData_GetGameRecords,%function
__wrap_SaveData_GetGameRecords:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag852
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_GetGameRecords
 .ltorg
.global __wrap_SaveData_GetOptions
.type __wrap_SaveData_GetOptions,%function
__wrap_SaveData_GetOptions:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag853
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_GetOptions
 .ltorg
.global __wrap_SaveData_GetParty
.type __wrap_SaveData_GetParty,%function
__wrap_SaveData_GetParty:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag854
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_GetParty
 .ltorg
.global __wrap_SaveData_GetPlayTime
.type __wrap_SaveData_GetPlayTime,%function
__wrap_SaveData_GetPlayTime:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag855
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_GetPlayTime
 .ltorg
.global __wrap_SaveData_GetPokedex
.type __wrap_SaveData_GetPokedex,%function
__wrap_SaveData_GetPokedex:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag856
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_GetPokedex
 .ltorg
.global __wrap_SaveData_GetPoketch
.type __wrap_SaveData_GetPoketch,%function
__wrap_SaveData_GetPoketch:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag857
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_GetPoketch
 .ltorg
.global __wrap_SaveData_GetSpecialEncounters
.type __wrap_SaveData_GetSpecialEncounters,%function
__wrap_SaveData_GetSpecialEncounters:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag858
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_GetSpecialEncounters
 .ltorg
.global __wrap_SaveData_GetTrainerInfo
.type __wrap_SaveData_GetTrainerInfo,%function
__wrap_SaveData_GetTrainerInfo:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag859
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_GetTrainerInfo
 .ltorg
.global __wrap_SaveData_GetVarsFlags
.type __wrap_SaveData_GetVarsFlags,%function
__wrap_SaveData_GetVarsFlags:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag860
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_GetVarsFlags
 .ltorg
.global __wrap_SaveData_GetWiFiList
.type __wrap_SaveData_GetWiFiList,%function
__wrap_SaveData_GetWiFiList:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag861
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_GetWiFiList
 .ltorg
.global __wrap_SaveData_GetWifiBattleTowerRecord
.type __wrap_SaveData_GetWifiBattleTowerRecord,%function
__wrap_SaveData_GetWifiBattleTowerRecord:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag862
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_GetWifiBattleTowerRecord
 .ltorg
.global __wrap_SaveData_MiscSaveBlock
.type __wrap_SaveData_MiscSaveBlock,%function
__wrap_SaveData_MiscSaveBlock:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag863
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_MiscSaveBlock
 .ltorg
.global __wrap_SaveData_Save
.type __wrap_SaveData_Save,%function
__wrap_SaveData_Save:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag864
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_Save
 .ltorg
.global __wrap_SaveData_WiFiHistory
.type __wrap_SaveData_WiFiHistory,%function
__wrap_SaveData_WiFiHistory:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag865
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SaveData_WiFiHistory
 .ltorg
.global __wrap_ScreenScrollManager_Delete
.type __wrap_ScreenScrollManager_Delete,%function
__wrap_ScreenScrollManager_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag866
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ScreenScrollManager_Delete
 .ltorg
.global __wrap_ScreenScrollManager_GetWriteBuffer
.type __wrap_ScreenScrollManager_GetWriteBuffer,%function
__wrap_ScreenScrollManager_GetWriteBuffer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag867
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ScreenScrollManager_GetWriteBuffer
 .ltorg
.global __wrap_ScreenScrollManager_New
.type __wrap_ScreenScrollManager_New,%function
__wrap_ScreenScrollManager_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag868
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ScreenScrollManager_New
 .ltorg
.global __wrap_ScreenScrollManager_RestartDMA
.type __wrap_ScreenScrollManager_RestartDMA,%function
__wrap_ScreenScrollManager_RestartDMA:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag869
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ScreenScrollManager_RestartDMA
 .ltorg
.global __wrap_ScreenScrollManager_ScrollX
.type __wrap_ScreenScrollManager_ScrollX,%function
__wrap_ScreenScrollManager_ScrollX:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag870
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ScreenScrollManager_ScrollX
 .ltorg
.global __wrap_ScreenScrollManager_Stop
.type __wrap_ScreenScrollManager_Stop,%function
__wrap_ScreenScrollManager_Stop:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag871
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ScreenScrollManager_Stop
 .ltorg
.global __wrap_ScreenScrollManager_SwapBuffers
.type __wrap_ScreenScrollManager_SwapBuffers,%function
__wrap_ScreenScrollManager_SwapBuffers:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag872
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ScreenScrollManager_SwapBuffers
 .ltorg
.global __wrap_ScreenSliceEffect_Delete
.type __wrap_ScreenSliceEffect_Delete,%function
__wrap_ScreenSliceEffect_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag873
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ScreenSliceEffect_Delete
 .ltorg
.global __wrap_ScreenSliceEffect_Modify
.type __wrap_ScreenSliceEffect_Modify,%function
__wrap_ScreenSliceEffect_Modify:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag874
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ScreenSliceEffect_Modify
 .ltorg
.global __wrap_ScreenSliceEffect_New
.type __wrap_ScreenSliceEffect_New,%function
__wrap_ScreenSliceEffect_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag875
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ScreenSliceEffect_New
 .ltorg
.global __wrap_ScreenSplitEffect_Delete
.type __wrap_ScreenSplitEffect_Delete,%function
__wrap_ScreenSplitEffect_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag876
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ScreenSplitEffect_Delete
 .ltorg
.global __wrap_ScreenSplitEffect_New
.type __wrap_ScreenSplitEffect_New,%function
__wrap_ScreenSplitEffect_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag877
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ScreenSplitEffect_New
 .ltorg
.global __wrap_ScriptContext_Pause
.type __wrap_ScriptContext_Pause,%function
__wrap_ScriptContext_Pause:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag878
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ScriptContext_Pause
 .ltorg
.global __wrap_ScriptContext_ReadHalfWord
.type __wrap_ScriptContext_ReadHalfWord,%function
__wrap_ScriptContext_ReadHalfWord:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag879
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ScriptContext_ReadHalfWord
 .ltorg
.global __wrap_ScriptManager_Set
.type __wrap_ScriptManager_Set,%function
__wrap_ScriptManager_Set:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag880
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ScriptManager_Set
 .ltorg
.global __wrap_ScriptManager_Start
.type __wrap_ScriptManager_Start,%function
__wrap_ScriptManager_Start:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag881
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ScriptManager_Start
 .ltorg
.global __wrap_Script_GetTrainerID
.type __wrap_Script_GetTrainerID,%function
__wrap_Script_GetTrainerID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag882
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Script_GetTrainerID
 .ltorg
.global __wrap_Script_IsTrainerDefeated
.type __wrap_Script_IsTrainerDefeated,%function
__wrap_Script_IsTrainerDefeated:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag883
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Script_IsTrainerDefeated
 .ltorg
.global __wrap_Script_IsTrainerDoubleBattle
.type __wrap_Script_IsTrainerDoubleBattle,%function
__wrap_Script_IsTrainerDoubleBattle:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag884
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Script_IsTrainerDoubleBattle
 .ltorg
.global __wrap_SecretBase_GetGoodIDAtIndex
.type __wrap_SecretBase_GetGoodIDAtIndex,%function
__wrap_SecretBase_GetGoodIDAtIndex:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag885
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SecretBase_GetGoodIDAtIndex
 .ltorg
.global __wrap_SecretBase_GetGoodXCoordAtIndex
.type __wrap_SecretBase_GetGoodXCoordAtIndex,%function
__wrap_SecretBase_GetGoodXCoordAtIndex:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag886
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SecretBase_GetGoodXCoordAtIndex
 .ltorg
.global __wrap_SecretBase_GetGoodZCoordAtIndex
.type __wrap_SecretBase_GetGoodZCoordAtIndex,%function
__wrap_SecretBase_GetGoodZCoordAtIndex:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag887
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SecretBase_GetGoodZCoordAtIndex
 .ltorg
.global __wrap_SecretBase_LoadProps
.type __wrap_SecretBase_LoadProps,%function
__wrap_SecretBase_LoadProps:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag888
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SecretBase_LoadProps
 .ltorg
.global __wrap_SetAllGraphicsModes
.type __wrap_SetAllGraphicsModes,%function
__wrap_SetAllGraphicsModes:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag889
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SetAllGraphicsModes
 .ltorg
.global __wrap_SetColorBrightness
.type __wrap_SetColorBrightness,%function
__wrap_SetColorBrightness:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag890
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SetColorBrightness
 .ltorg
.global __wrap_SetHBlankCallback
.type __wrap_SetHBlankCallback,%function
__wrap_SetHBlankCallback:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag891
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SetHBlankCallback
 .ltorg
.global __wrap_SetScreenColorBrightness
.type __wrap_SetScreenColorBrightness,%function
__wrap_SetScreenColorBrightness:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag892
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SetScreenColorBrightness
 .ltorg
.global __wrap_SetVBlankCallback
.type __wrap_SetVBlankCallback,%function
__wrap_SetVBlankCallback:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag893
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SetVBlankCallback
 .ltorg
.global __wrap_Signpost_DoCurrentCommand
.type __wrap_Signpost_DoCurrentCommand,%function
__wrap_Signpost_DoCurrentCommand:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag894
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Signpost_DoCurrentCommand
 .ltorg
.global __wrap_Signpost_Free
.type __wrap_Signpost_Free,%function
__wrap_Signpost_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag895
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Signpost_Free
 .ltorg
.global __wrap_Signpost_Init
.type __wrap_Signpost_Init,%function
__wrap_Signpost_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag896
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Signpost_Init
 .ltorg
.global __wrap_Simple3D_ApplyAnimCopyToModel
.type __wrap_Simple3D_ApplyAnimCopyToModel,%function
__wrap_Simple3D_ApplyAnimCopyToModel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag897
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_ApplyAnimCopyToModel
 .ltorg
.global __wrap_Simple3D_BindAnimToRenderObj
.type __wrap_Simple3D_BindAnimToRenderObj,%function
__wrap_Simple3D_BindAnimToRenderObj:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag898
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_BindAnimToRenderObj
 .ltorg
.global __wrap_Simple3D_CreateRenderObject
.type __wrap_Simple3D_CreateRenderObject,%function
__wrap_Simple3D_CreateRenderObject:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag899
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_CreateRenderObject
 .ltorg
.global __wrap_Simple3D_CreateRenderObjectWithAnim
.type __wrap_Simple3D_CreateRenderObjectWithAnim,%function
__wrap_Simple3D_CreateRenderObjectWithAnim:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag900
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_CreateRenderObjectWithAnim
 .ltorg
.global __wrap_Simple3D_DrawRenderObj
.type __wrap_Simple3D_DrawRenderObj,%function
__wrap_Simple3D_DrawRenderObj:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag901
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_DrawRenderObj
 .ltorg
.global __wrap_Simple3D_DrawRenderObjRotationAngles
.type __wrap_Simple3D_DrawRenderObjRotationAngles,%function
__wrap_Simple3D_DrawRenderObjRotationAngles:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag902
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_DrawRenderObjRotationAngles
 .ltorg
.global __wrap_Simple3D_DrawRenderObjSimple
.type __wrap_Simple3D_DrawRenderObjSimple,%function
__wrap_Simple3D_DrawRenderObjSimple:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag903
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_DrawRenderObjSimple
 .ltorg
.global __wrap_Simple3D_DrawRenderObjWithPos
.type __wrap_Simple3D_DrawRenderObjWithPos,%function
__wrap_Simple3D_DrawRenderObjWithPos:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag904
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_DrawRenderObjWithPos
 .ltorg
.global __wrap_Simple3D_DrawRenderObjWithPosAndRotationAngles
.type __wrap_Simple3D_DrawRenderObjWithPosAndRotationAngles,%function
__wrap_Simple3D_DrawRenderObjWithPosAndRotationAngles:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag905
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_DrawRenderObjWithPosAndRotationAngles
 .ltorg
.global __wrap_Simple3D_FreeAnimObject
.type __wrap_Simple3D_FreeAnimObject,%function
__wrap_Simple3D_FreeAnimObject:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag906
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_FreeAnimObject
 .ltorg
.global __wrap_Simple3D_FreeAnimation
.type __wrap_Simple3D_FreeAnimation,%function
__wrap_Simple3D_FreeAnimation:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag907
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_FreeAnimation
 .ltorg
.global __wrap_Simple3D_FreeModel
.type __wrap_Simple3D_FreeModel,%function
__wrap_Simple3D_FreeModel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag908
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_FreeModel
 .ltorg
.global __wrap_Simple3D_HasAnimationReachedEnd
.type __wrap_Simple3D_HasAnimationReachedEnd,%function
__wrap_Simple3D_HasAnimationReachedEnd:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag909
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_HasAnimationReachedEnd
 .ltorg
.global __wrap_Simple3D_LoadAnimFromOpenNARC
.type __wrap_Simple3D_LoadAnimFromOpenNARC,%function
__wrap_Simple3D_LoadAnimFromOpenNARC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag910
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_LoadAnimFromOpenNARC
 .ltorg
.global __wrap_Simple3D_LoadModelFromSet
.type __wrap_Simple3D_LoadModelFromSet,%function
__wrap_Simple3D_LoadModelFromSet:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag911
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_LoadModelFromSet
 .ltorg
.global __wrap_Simple3D_ScheduleBindModelTexture
.type __wrap_Simple3D_ScheduleBindModelTexture,%function
__wrap_Simple3D_ScheduleBindModelTexture:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag912
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_ScheduleBindModelTexture
 .ltorg
.global __wrap_Simple3D_UpdateAnim
.type __wrap_Simple3D_UpdateAnim,%function
__wrap_Simple3D_UpdateAnim:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag913
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_UpdateAnim
 .ltorg
.global __wrap_Simple3D_ZeroOutAnimation
.type __wrap_Simple3D_ZeroOutAnimation,%function
__wrap_Simple3D_ZeroOutAnimation:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag914
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Simple3D_ZeroOutAnimation
 .ltorg
.global __wrap_Sound_IsEffectPlaying
.type __wrap_Sound_IsEffectPlaying,%function
__wrap_Sound_IsEffectPlaying:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag915
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sound_IsEffectPlaying
 .ltorg
.global __wrap_Sound_PlayEffect
.type __wrap_Sound_PlayEffect,%function
__wrap_Sound_PlayEffect:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag916
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sound_PlayEffect
 .ltorg
.global __wrap_Sound_SetPlayerVolume
.type __wrap_Sound_SetPlayerVolume,%function
__wrap_Sound_SetPlayerVolume:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag917
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sound_SetPlayerVolume
 .ltorg
.global __wrap_Sound_StopEffect
.type __wrap_Sound_StopEffect,%function
__wrap_Sound_StopEffect:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag918
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sound_StopEffect
 .ltorg
.global __wrap_SpecialEncounter_GetHoneyTree
.type __wrap_SpecialEncounter_GetHoneyTree,%function
__wrap_SpecialEncounter_GetHoneyTree:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag919
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpecialEncounter_GetHoneyTree
 .ltorg
.global __wrap_SpecialEncounter_GetLastSlatheredTreeId
.type __wrap_SpecialEncounter_GetLastSlatheredTreeId,%function
__wrap_SpecialEncounter_GetLastSlatheredTreeId:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag920
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpecialEncounter_GetLastSlatheredTreeId
 .ltorg
.global __wrap_SpecialEncounter_GetPlayerHoneyTreeStates
.type __wrap_SpecialEncounter_GetPlayerHoneyTreeStates,%function
__wrap_SpecialEncounter_GetPlayerHoneyTreeStates:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag921
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpecialEncounter_GetPlayerHoneyTreeStates
 .ltorg
.global __wrap_SpecialEncounter_SetLastSlatheredTreeId
.type __wrap_SpecialEncounter_SetLastSlatheredTreeId,%function
__wrap_SpecialEncounter_SetLastSlatheredTreeId:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag922
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpecialEncounter_SetLastSlatheredTreeId
 .ltorg
.global __wrap_SpecialMetLoc_GetId
.type __wrap_SpecialMetLoc_GetId,%function
__wrap_SpecialMetLoc_GetId:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag923
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpecialMetLoc_GetId
 .ltorg
.global __wrap_SpeciesData_GetSpeciesValue
.type __wrap_SpeciesData_GetSpeciesValue,%function
__wrap_SpeciesData_GetSpeciesValue:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag924
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpeciesData_GetSpeciesValue
 .ltorg
.global __wrap_SpriteList_Add
.type __wrap_SpriteList_Add,%function
__wrap_SpriteList_Add:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag925
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteList_Add
 .ltorg
.global __wrap_SpriteList_AddAffine
.type __wrap_SpriteList_AddAffine,%function
__wrap_SpriteList_AddAffine:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag926
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteList_AddAffine
 .ltorg
.global __wrap_SpriteList_Delete
.type __wrap_SpriteList_Delete,%function
__wrap_SpriteList_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag927
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteList_Delete
 .ltorg
.global __wrap_SpriteList_InitRendering
.type __wrap_SpriteList_InitRendering,%function
__wrap_SpriteList_InitRendering:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag928
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteList_InitRendering
 .ltorg
.global __wrap_SpriteList_New
.type __wrap_SpriteList_New,%function
__wrap_SpriteList_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag929
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteList_New
 .ltorg
.global __wrap_SpriteList_Update
.type __wrap_SpriteList_Update,%function
__wrap_SpriteList_Update:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag930
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteList_Update
 .ltorg
.global __wrap_SpriteResourceCollection_Add
.type __wrap_SpriteResourceCollection_Add,%function
__wrap_SpriteResourceCollection_Add:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag931
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceCollection_Add
 .ltorg
.global __wrap_SpriteResourceCollection_AddFrom
.type __wrap_SpriteResourceCollection_AddFrom,%function
__wrap_SpriteResourceCollection_AddFrom:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag932
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceCollection_AddFrom
 .ltorg
.global __wrap_SpriteResourceCollection_AddPalette
.type __wrap_SpriteResourceCollection_AddPalette,%function
__wrap_SpriteResourceCollection_AddPalette:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag933
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceCollection_AddPalette
 .ltorg
.global __wrap_SpriteResourceCollection_AddPaletteFrom
.type __wrap_SpriteResourceCollection_AddPaletteFrom,%function
__wrap_SpriteResourceCollection_AddPaletteFrom:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag934
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceCollection_AddPaletteFrom
 .ltorg
.global __wrap_SpriteResourceCollection_AddPaletteFromEx
.type __wrap_SpriteResourceCollection_AddPaletteFromEx,%function
__wrap_SpriteResourceCollection_AddPaletteFromEx:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag935
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceCollection_AddPaletteFromEx
 .ltorg
.global __wrap_SpriteResourceCollection_AddTiles
.type __wrap_SpriteResourceCollection_AddTiles,%function
__wrap_SpriteResourceCollection_AddTiles:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag936
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceCollection_AddTiles
 .ltorg
.global __wrap_SpriteResourceCollection_AddTilesFrom
.type __wrap_SpriteResourceCollection_AddTilesFrom,%function
__wrap_SpriteResourceCollection_AddTilesFrom:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag937
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceCollection_AddTilesFrom
 .ltorg
.global __wrap_SpriteResourceCollection_AddTilesFromEx
.type __wrap_SpriteResourceCollection_AddTilesFromEx,%function
__wrap_SpriteResourceCollection_AddTilesFromEx:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag938
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceCollection_AddTilesFromEx
 .ltorg
.global __wrap_SpriteResourceCollection_Delete
.type __wrap_SpriteResourceCollection_Delete,%function
__wrap_SpriteResourceCollection_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag939
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceCollection_Delete
 .ltorg
.global __wrap_SpriteResourceCollection_Extend
.type __wrap_SpriteResourceCollection_Extend,%function
__wrap_SpriteResourceCollection_Extend:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag940
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceCollection_Extend
 .ltorg
.global __wrap_SpriteResourceCollection_IsIDUnused
.type __wrap_SpriteResourceCollection_IsIDUnused,%function
__wrap_SpriteResourceCollection_IsIDUnused:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag941
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceCollection_IsIDUnused
 .ltorg
.global __wrap_SpriteResourceCollection_New
.type __wrap_SpriteResourceCollection_New,%function
__wrap_SpriteResourceCollection_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag942
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceCollection_New
 .ltorg
.global __wrap_SpriteResourceCollection_Remove
.type __wrap_SpriteResourceCollection_Remove,%function
__wrap_SpriteResourceCollection_Remove:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag943
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceCollection_Remove
 .ltorg
.global __wrap_SpriteResourceList_Delete
.type __wrap_SpriteResourceList_Delete,%function
__wrap_SpriteResourceList_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag944
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceList_Delete
 .ltorg
.global __wrap_SpriteResourceList_New
.type __wrap_SpriteResourceList_New,%function
__wrap_SpriteResourceList_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag945
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceList_New
 .ltorg
.global __wrap_SpriteResourceTable_Clear
.type __wrap_SpriteResourceTable_Clear,%function
__wrap_SpriteResourceTable_Clear:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag946
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceTable_Clear
 .ltorg
.global __wrap_SpriteResourceTable_GetArrayElement
.type __wrap_SpriteResourceTable_GetArrayElement,%function
__wrap_SpriteResourceTable_GetArrayElement:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag947
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceTable_GetArrayElement
 .ltorg
.global __wrap_SpriteResourceTable_GetCount
.type __wrap_SpriteResourceTable_GetCount,%function
__wrap_SpriteResourceTable_GetCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag948
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceTable_GetCount
 .ltorg
.global __wrap_SpriteResourceTable_GetEntryID
.type __wrap_SpriteResourceTable_GetEntryID,%function
__wrap_SpriteResourceTable_GetEntryID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag949
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceTable_GetEntryID
 .ltorg
.global __wrap_SpriteResourceTable_GetEntryVRAMType
.type __wrap_SpriteResourceTable_GetEntryVRAMType,%function
__wrap_SpriteResourceTable_GetEntryVRAMType:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag950
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceTable_GetEntryVRAMType
 .ltorg
.global __wrap_SpriteResourceTable_GetNARCEntryMemberIndex
.type __wrap_SpriteResourceTable_GetNARCEntryMemberIndex,%function
__wrap_SpriteResourceTable_GetNARCEntryMemberIndex:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag951
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceTable_GetNARCEntryMemberIndex
 .ltorg
.global __wrap_SpriteResourceTable_GetPaletteIndex
.type __wrap_SpriteResourceTable_GetPaletteIndex,%function
__wrap_SpriteResourceTable_GetPaletteIndex:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag952
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceTable_GetPaletteIndex
 .ltorg
.global __wrap_SpriteResourceTable_IsNARCEntryCompressed
.type __wrap_SpriteResourceTable_IsNARCEntryCompressed,%function
__wrap_SpriteResourceTable_IsNARCEntryCompressed:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag953
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceTable_IsNARCEntryCompressed
 .ltorg
.global __wrap_SpriteResourceTable_LoadFromBinary
.type __wrap_SpriteResourceTable_LoadFromBinary,%function
__wrap_SpriteResourceTable_LoadFromBinary:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag954
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceTable_LoadFromBinary
 .ltorg
.global __wrap_SpriteResourceTable_Size
.type __wrap_SpriteResourceTable_Size,%function
__wrap_SpriteResourceTable_Size:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag955
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourceTable_Size
 .ltorg
.global __wrap_SpriteResource_GetID
.type __wrap_SpriteResource_GetID,%function
__wrap_SpriteResource_GetID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag956
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResource_GetID
 .ltorg
.global __wrap_SpriteResource_ReleaseData
.type __wrap_SpriteResource_ReleaseData,%function
__wrap_SpriteResource_ReleaseData:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag957
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResource_ReleaseData
 .ltorg
.global __wrap_SpriteResourcesHeaderList_Free
.type __wrap_SpriteResourcesHeaderList_Free,%function
__wrap_SpriteResourcesHeaderList_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag958
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourcesHeaderList_Free
 .ltorg
.global __wrap_SpriteResourcesHeaderList_NewFromResdat
.type __wrap_SpriteResourcesHeaderList_NewFromResdat,%function
__wrap_SpriteResourcesHeaderList_NewFromResdat:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag959
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourcesHeaderList_NewFromResdat
 .ltorg
.global __wrap_SpriteResourcesHeader_Init
.type __wrap_SpriteResourcesHeader_Init,%function
__wrap_SpriteResourcesHeader_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag960
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteResourcesHeader_Init
 .ltorg
.global __wrap_SpriteSystem_SetTrainerClassGraphicsIndex
.type __wrap_SpriteSystem_SetTrainerClassGraphicsIndex,%function
__wrap_SpriteSystem_SetTrainerClassGraphicsIndex:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag961
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteSystem_SetTrainerClassGraphicsIndex
 .ltorg
.global __wrap_SpriteTransfer_GetPlttOffset
.type __wrap_SpriteTransfer_GetPlttOffset,%function
__wrap_SpriteTransfer_GetPlttOffset:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag962
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteTransfer_GetPlttOffset
 .ltorg
.global __wrap_SpriteTransfer_RequestCharAtEnd
.type __wrap_SpriteTransfer_RequestCharAtEnd,%function
__wrap_SpriteTransfer_RequestCharAtEnd:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag963
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteTransfer_RequestCharAtEnd
 .ltorg
.global __wrap_SpriteTransfer_RequestCharListAtEnd
.type __wrap_SpriteTransfer_RequestCharListAtEnd,%function
__wrap_SpriteTransfer_RequestCharListAtEnd:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag964
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteTransfer_RequestCharListAtEnd
 .ltorg
.global __wrap_SpriteTransfer_RequestPlttFreeSpace
.type __wrap_SpriteTransfer_RequestPlttFreeSpace,%function
__wrap_SpriteTransfer_RequestPlttFreeSpace:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag965
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteTransfer_RequestPlttFreeSpace
 .ltorg
.global __wrap_SpriteTransfer_RequestPlttFreeSpaceList
.type __wrap_SpriteTransfer_RequestPlttFreeSpaceList,%function
__wrap_SpriteTransfer_RequestPlttFreeSpaceList:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag966
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteTransfer_RequestPlttFreeSpaceList
 .ltorg
.global __wrap_SpriteTransfer_ResetCharTransfer
.type __wrap_SpriteTransfer_ResetCharTransfer,%function
__wrap_SpriteTransfer_ResetCharTransfer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag967
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteTransfer_ResetCharTransfer
 .ltorg
.global __wrap_SpriteTransfer_ResetCharTransferList
.type __wrap_SpriteTransfer_ResetCharTransferList,%function
__wrap_SpriteTransfer_ResetCharTransferList:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag968
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteTransfer_ResetCharTransferList
 .ltorg
.global __wrap_SpriteTransfer_ResetPlttTransfer
.type __wrap_SpriteTransfer_ResetPlttTransfer,%function
__wrap_SpriteTransfer_ResetPlttTransfer:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag969
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteTransfer_ResetPlttTransfer
 .ltorg
.global __wrap_SpriteTransfer_ResetPlttTransferList
.type __wrap_SpriteTransfer_ResetPlttTransferList,%function
__wrap_SpriteTransfer_ResetPlttTransferList:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag970
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SpriteTransfer_ResetPlttTransferList
 .ltorg
.global __wrap_Sprite_Delete
.type __wrap_Sprite_Delete,%function
__wrap_Sprite_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag971
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_Delete
 .ltorg
.global __wrap_Sprite_GetExplicitPalette
.type __wrap_Sprite_GetExplicitPalette,%function
__wrap_Sprite_GetExplicitPalette:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag972
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_GetExplicitPalette
 .ltorg
.global __wrap_Sprite_GetPaletteProxy
.type __wrap_Sprite_GetPaletteProxy,%function
__wrap_Sprite_GetPaletteProxy:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag973
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_GetPaletteProxy
 .ltorg
.global __wrap_Sprite_GetPosition
.type __wrap_Sprite_GetPosition,%function
__wrap_Sprite_GetPosition:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag974
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_GetPosition
 .ltorg
.global __wrap_Sprite_SetAffineOverwriteMode
.type __wrap_Sprite_SetAffineOverwriteMode,%function
__wrap_Sprite_SetAffineOverwriteMode:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag975
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_SetAffineOverwriteMode
 .ltorg
.global __wrap_Sprite_SetAffineScale
.type __wrap_Sprite_SetAffineScale,%function
__wrap_Sprite_SetAffineScale:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag976
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_SetAffineScale
 .ltorg
.global __wrap_Sprite_SetAffineScaleEx
.type __wrap_Sprite_SetAffineScaleEx,%function
__wrap_Sprite_SetAffineScaleEx:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag977
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_SetAffineScaleEx
 .ltorg
.global __wrap_Sprite_SetAffineZRotation
.type __wrap_Sprite_SetAffineZRotation,%function
__wrap_Sprite_SetAffineZRotation:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag978
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_SetAffineZRotation
 .ltorg
.global __wrap_Sprite_SetAnim
.type __wrap_Sprite_SetAnim,%function
__wrap_Sprite_SetAnim:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag979
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_SetAnim
 .ltorg
.global __wrap_Sprite_SetAnimFrame
.type __wrap_Sprite_SetAnimFrame,%function
__wrap_Sprite_SetAnimFrame:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag980
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_SetAnimFrame
 .ltorg
.global __wrap_Sprite_SetAnimSpeed
.type __wrap_Sprite_SetAnimSpeed,%function
__wrap_Sprite_SetAnimSpeed:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag981
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_SetAnimSpeed
 .ltorg
.global __wrap_Sprite_SetAnimateFlag
.type __wrap_Sprite_SetAnimateFlag,%function
__wrap_Sprite_SetAnimateFlag:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag982
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_SetAnimateFlag
 .ltorg
.global __wrap_Sprite_SetDrawFlag
.type __wrap_Sprite_SetDrawFlag,%function
__wrap_Sprite_SetDrawFlag:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag983
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_SetDrawFlag
 .ltorg
.global __wrap_Sprite_SetExplicitOAMMode
.type __wrap_Sprite_SetExplicitOAMMode,%function
__wrap_Sprite_SetExplicitOAMMode:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag984
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_SetExplicitOAMMode
 .ltorg
.global __wrap_Sprite_SetExplicitPalette
.type __wrap_Sprite_SetExplicitPalette,%function
__wrap_Sprite_SetExplicitPalette:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag985
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_SetExplicitPalette
 .ltorg
.global __wrap_Sprite_SetExplicitPaletteWithOffset
.type __wrap_Sprite_SetExplicitPaletteWithOffset,%function
__wrap_Sprite_SetExplicitPaletteWithOffset:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag986
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_SetExplicitPaletteWithOffset
 .ltorg
.global __wrap_Sprite_SetExplicitPriority
.type __wrap_Sprite_SetExplicitPriority,%function
__wrap_Sprite_SetExplicitPriority:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag987
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_SetExplicitPriority
 .ltorg
.global __wrap_Sprite_SetPosition
.type __wrap_Sprite_SetPosition,%function
__wrap_Sprite_SetPosition:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag988
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_SetPosition
 .ltorg
.global __wrap_Sprite_SetPriority
.type __wrap_Sprite_SetPriority,%function
__wrap_Sprite_SetPriority:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag989
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Sprite_SetPriority
 .ltorg
.global __wrap_StartMenu_Open
.type __wrap_StartMenu_Open,%function
__wrap_StartMenu_Open:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag990
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StartMenu_Open
 .ltorg
.global __wrap_StartMenu_OpenColosseum
.type __wrap_StartMenu_OpenColosseum,%function
__wrap_StartMenu_OpenColosseum:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag991
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StartMenu_OpenColosseum
 .ltorg
.global __wrap_StartMenu_OpenUnionRoom
.type __wrap_StartMenu_OpenUnionRoom,%function
__wrap_StartMenu_OpenUnionRoom:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag992
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StartMenu_OpenUnionRoom
 .ltorg
.global __wrap_StartScreenFade
.type __wrap_StartScreenFade,%function
__wrap_StartScreenFade:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag993
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StartScreenFade
 .ltorg
.global __wrap_StringList_AddFromMessageBank
.type __wrap_StringList_AddFromMessageBank,%function
__wrap_StringList_AddFromMessageBank:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag994
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StringList_AddFromMessageBank
 .ltorg
.global __wrap_StringList_AddFromString
.type __wrap_StringList_AddFromString,%function
__wrap_StringList_AddFromString:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag995
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StringList_AddFromString
 .ltorg
.global __wrap_StringList_Free
.type __wrap_StringList_Free,%function
__wrap_StringList_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag996
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StringList_Free
 .ltorg
.global __wrap_StringList_New
.type __wrap_StringList_New,%function
__wrap_StringList_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag997
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StringList_New
 .ltorg
.global __wrap_StringTemplate_Default
.type __wrap_StringTemplate_Default,%function
__wrap_StringTemplate_Default:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag998
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StringTemplate_Default
 .ltorg
.global __wrap_StringTemplate_Format
.type __wrap_StringTemplate_Format,%function
__wrap_StringTemplate_Format:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag999
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StringTemplate_Format
 .ltorg
.global __wrap_StringTemplate_Free
.type __wrap_StringTemplate_Free,%function
__wrap_StringTemplate_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1000
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StringTemplate_Free
 .ltorg
.global __wrap_StringTemplate_New
.type __wrap_StringTemplate_New,%function
__wrap_StringTemplate_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1001
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StringTemplate_New
 .ltorg
.global __wrap_StringTemplate_SetGenderMarker
.type __wrap_StringTemplate_SetGenderMarker,%function
__wrap_StringTemplate_SetGenderMarker:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1002
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StringTemplate_SetGenderMarker
 .ltorg
.global __wrap_StringTemplate_SetLocationName
.type __wrap_StringTemplate_SetLocationName,%function
__wrap_StringTemplate_SetLocationName:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1003
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StringTemplate_SetLocationName
 .ltorg
.global __wrap_StringTemplate_SetNickname
.type __wrap_StringTemplate_SetNickname,%function
__wrap_StringTemplate_SetNickname:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1004
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StringTemplate_SetNickname
 .ltorg
.global __wrap_StringTemplate_SetNumber
.type __wrap_StringTemplate_SetNumber,%function
__wrap_StringTemplate_SetNumber:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1005
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StringTemplate_SetNumber
 .ltorg
.global __wrap_StringTemplate_SetOTName
.type __wrap_StringTemplate_SetOTName,%function
__wrap_StringTemplate_SetOTName:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1006
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StringTemplate_SetOTName
 .ltorg
.global __wrap_StringTemplate_SetPlayerName
.type __wrap_StringTemplate_SetPlayerName,%function
__wrap_StringTemplate_SetPlayerName:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1007
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StringTemplate_SetPlayerName
 .ltorg
.global __wrap_StringTemplate_SetTrainerName
.type __wrap_StringTemplate_SetTrainerName,%function
__wrap_StringTemplate_SetTrainerName:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1008
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_StringTemplate_SetTrainerName
 .ltorg
.global __wrap_String_Copy
.type __wrap_String_Copy,%function
__wrap_String_Copy:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1009
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_String_Copy
 .ltorg
.global __wrap_String_CopyChars
.type __wrap_String_CopyChars,%function
__wrap_String_CopyChars:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1010
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_String_CopyChars
 .ltorg
.global __wrap_String_Free
.type __wrap_String_Free,%function
__wrap_String_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1011
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_String_Free
 .ltorg
.global __wrap_String_Init
.type __wrap_String_Init,%function
__wrap_String_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1012
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_String_Init
 .ltorg
.global __wrap_SurfMountRenderer_Free
.type __wrap_SurfMountRenderer_Free,%function
__wrap_SurfMountRenderer_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1013
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SurfMountRenderer_Free
 .ltorg
.global __wrap_SurfMountRenderer_HandleSurfBegin
.type __wrap_SurfMountRenderer_HandleSurfBegin,%function
__wrap_SurfMountRenderer_HandleSurfBegin:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1014
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SurfMountRenderer_HandleSurfBegin
 .ltorg
.global __wrap_SurfMountRenderer_New
.type __wrap_SurfMountRenderer_New,%function
__wrap_SurfMountRenderer_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1015
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SurfMountRenderer_New
 .ltorg
.global __wrap_SurfMountRenderer_SetSyncPos
.type __wrap_SurfMountRenderer_SetSyncPos,%function
__wrap_SurfMountRenderer_SetSyncPos:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1016
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SurfMountRenderer_SetSyncPos
 .ltorg
.global __wrap_SysTaskManager_AddTask
.type __wrap_SysTaskManager_AddTask,%function
__wrap_SysTaskManager_AddTask:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1017
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SysTaskManager_AddTask
 .ltorg
.global __wrap_SysTaskManager_ExecuteTasks
.type __wrap_SysTaskManager_ExecuteTasks,%function
__wrap_SysTaskManager_ExecuteTasks:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1018
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SysTaskManager_ExecuteTasks
 .ltorg
.global __wrap_SysTaskManager_GetRequiredSize
.type __wrap_SysTaskManager_GetRequiredSize,%function
__wrap_SysTaskManager_GetRequiredSize:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1019
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SysTaskManager_GetRequiredSize
 .ltorg
.global __wrap_SysTaskManager_Init
.type __wrap_SysTaskManager_Init,%function
__wrap_SysTaskManager_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1020
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SysTaskManager_Init
 .ltorg
.global __wrap_SysTask_Done
.type __wrap_SysTask_Done,%function
__wrap_SysTask_Done:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1021
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SysTask_Done
 .ltorg
.global __wrap_SysTask_ExecuteAfterVBlank
.type __wrap_SysTask_ExecuteAfterVBlank,%function
__wrap_SysTask_ExecuteAfterVBlank:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1022
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SysTask_ExecuteAfterVBlank
 .ltorg
.global __wrap_SysTask_ExecuteOnVBlank
.type __wrap_SysTask_ExecuteOnVBlank,%function
__wrap_SysTask_ExecuteOnVBlank:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1023
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SysTask_ExecuteOnVBlank
 .ltorg
.global __wrap_SysTask_FinishAndFreeParam
.type __wrap_SysTask_FinishAndFreeParam,%function
__wrap_SysTask_FinishAndFreeParam:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1024
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SysTask_FinishAndFreeParam
 .ltorg
.global __wrap_SysTask_GetParam
.type __wrap_SysTask_GetParam,%function
__wrap_SysTask_GetParam:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1025
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SysTask_GetParam
 .ltorg
.global __wrap_SysTask_Start
.type __wrap_SysTask_Start,%function
__wrap_SysTask_Start:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1026
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SysTask_Start
 .ltorg
.global __wrap_SysTask_StartAndAllocateParam
.type __wrap_SysTask_StartAndAllocateParam,%function
__wrap_SysTask_StartAndAllocateParam:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1027
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SysTask_StartAndAllocateParam
 .ltorg
.global __wrap_SystemFlag_CheckHasPartner
.type __wrap_SystemFlag_CheckHasPartner,%function
__wrap_SystemFlag_CheckHasPartner:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1028
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemFlag_CheckHasPartner
 .ltorg
.global __wrap_SystemFlag_CheckInPalPark
.type __wrap_SystemFlag_CheckInPalPark,%function
__wrap_SystemFlag_CheckInPalPark:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1029
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemFlag_CheckInPalPark
 .ltorg
.global __wrap_SystemFlag_CheckPoketchHidden
.type __wrap_SystemFlag_CheckPoketchHidden,%function
__wrap_SystemFlag_CheckPoketchHidden:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1030
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemFlag_CheckPoketchHidden
 .ltorg
.global __wrap_SystemFlag_CheckSafariGameActive
.type __wrap_SystemFlag_CheckSafariGameActive,%function
__wrap_SystemFlag_CheckSafariGameActive:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1031
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemFlag_CheckSafariGameActive
 .ltorg
.global __wrap_SystemFlag_CheckUnlockedVsSeekerLevel
.type __wrap_SystemFlag_CheckUnlockedVsSeekerLevel,%function
__wrap_SystemFlag_CheckUnlockedVsSeekerLevel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1032
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemFlag_CheckUnlockedVsSeekerLevel
 .ltorg
.global __wrap_SystemFlag_CheckVsSeekerUsed
.type __wrap_SystemFlag_CheckVsSeekerUsed,%function
__wrap_SystemFlag_CheckVsSeekerUsed:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1033
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemFlag_CheckVsSeekerUsed
 .ltorg
.global __wrap_SystemFlag_ClearStep
.type __wrap_SystemFlag_ClearStep,%function
__wrap_SystemFlag_ClearStep:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1034
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemFlag_ClearStep
 .ltorg
.global __wrap_SystemFlag_HandleOwnsVillaFurniture
.type __wrap_SystemFlag_HandleOwnsVillaFurniture,%function
__wrap_SystemFlag_HandleOwnsVillaFurniture:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1035
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemFlag_HandleOwnsVillaFurniture
 .ltorg
.global __wrap_SystemFlag_HandleStrengthActive
.type __wrap_SystemFlag_HandleStrengthActive,%function
__wrap_SystemFlag_HandleStrengthActive:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1036
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemFlag_HandleStrengthActive
 .ltorg
.global __wrap_SystemFlag_SetVsSeekerUsed
.type __wrap_SystemFlag_SetVsSeekerUsed,%function
__wrap_SystemFlag_SetVsSeekerUsed:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1037
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemFlag_SetVsSeekerUsed
 .ltorg
.global __wrap_SystemVars_GetBattleArcadePrintState
.type __wrap_SystemVars_GetBattleArcadePrintState,%function
__wrap_SystemVars_GetBattleArcadePrintState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1038
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemVars_GetBattleArcadePrintState
 .ltorg
.global __wrap_SystemVars_GetBattleCastlePrintState
.type __wrap_SystemVars_GetBattleCastlePrintState,%function
__wrap_SystemVars_GetBattleCastlePrintState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1039
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemVars_GetBattleCastlePrintState
 .ltorg
.global __wrap_SystemVars_GetBattleFactoryPrintState
.type __wrap_SystemVars_GetBattleFactoryPrintState,%function
__wrap_SystemVars_GetBattleFactoryPrintState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1040
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemVars_GetBattleFactoryPrintState
 .ltorg
.global __wrap_SystemVars_GetBattleHallPrintState
.type __wrap_SystemVars_GetBattleHallPrintState,%function
__wrap_SystemVars_GetBattleHallPrintState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1041
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemVars_GetBattleHallPrintState
 .ltorg
.global __wrap_SystemVars_GetBattleTowerPrintState
.type __wrap_SystemVars_GetBattleTowerPrintState,%function
__wrap_SystemVars_GetBattleTowerPrintState:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1042
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemVars_GetBattleTowerPrintState
 .ltorg
.global __wrap_SystemVars_GetFriendshipStepCount
.type __wrap_SystemVars_GetFriendshipStepCount,%function
__wrap_SystemVars_GetFriendshipStepCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1043
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemVars_GetFriendshipStepCount
 .ltorg
.global __wrap_SystemVars_GetSizeContestRecord
.type __wrap_SystemVars_GetSizeContestRecord,%function
__wrap_SystemVars_GetSizeContestRecord:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1044
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemVars_GetSizeContestRecord
 .ltorg
.global __wrap_SystemVars_GetVsSeekerBattery
.type __wrap_SystemVars_GetVsSeekerBattery,%function
__wrap_SystemVars_GetVsSeekerBattery:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1045
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemVars_GetVsSeekerBattery
 .ltorg
.global __wrap_SystemVars_GetVsSeekerStepCount
.type __wrap_SystemVars_GetVsSeekerStepCount,%function
__wrap_SystemVars_GetVsSeekerStepCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1046
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemVars_GetVsSeekerStepCount
 .ltorg
.global __wrap_SystemVars_IncrementAmitySquareStepCount
.type __wrap_SystemVars_IncrementAmitySquareStepCount,%function
__wrap_SystemVars_IncrementAmitySquareStepCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1047
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemVars_IncrementAmitySquareStepCount
 .ltorg
.global __wrap_SystemVars_ResetVsSeeker
.type __wrap_SystemVars_ResetVsSeeker,%function
__wrap_SystemVars_ResetVsSeeker:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1048
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemVars_ResetVsSeeker
 .ltorg
.global __wrap_SystemVars_SetFriendshipStepCount
.type __wrap_SystemVars_SetFriendshipStepCount,%function
__wrap_SystemVars_SetFriendshipStepCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1049
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemVars_SetFriendshipStepCount
 .ltorg
.global __wrap_SystemVars_SetSizeContestRecord
.type __wrap_SystemVars_SetSizeContestRecord,%function
__wrap_SystemVars_SetSizeContestRecord:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1050
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemVars_SetSizeContestRecord
 .ltorg
.global __wrap_SystemVars_SetVsSeekerBattery
.type __wrap_SystemVars_SetVsSeekerBattery,%function
__wrap_SystemVars_SetVsSeekerBattery:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1051
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemVars_SetVsSeekerBattery
 .ltorg
.global __wrap_SystemVars_SetVsSeekerStepCount
.type __wrap_SystemVars_SetVsSeekerStepCount,%function
__wrap_SystemVars_SetVsSeekerStepCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1052
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_SystemVars_SetVsSeekerStepCount
 .ltorg
.global __wrap_TerrainCollisionHitbox_Init
.type __wrap_TerrainCollisionHitbox_Init,%function
__wrap_TerrainCollisionHitbox_Init:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1053
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TerrainCollisionHitbox_Init
 .ltorg
.global __wrap_TerrainCollisionManager_CheckCollision
.type __wrap_TerrainCollisionManager_CheckCollision,%function
__wrap_TerrainCollisionManager_CheckCollision:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1054
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TerrainCollisionManager_CheckCollision
 .ltorg
.global __wrap_TerrainCollisionManager_GetTileBehavior
.type __wrap_TerrainCollisionManager_GetTileBehavior,%function
__wrap_TerrainCollisionManager_GetTileBehavior:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1055
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TerrainCollisionManager_GetTileBehavior
 .ltorg
.global __wrap_Text_AddPrinterWithParams
.type __wrap_Text_AddPrinterWithParams,%function
__wrap_Text_AddPrinterWithParams:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1056
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Text_AddPrinterWithParams
 .ltorg
.global __wrap_Text_AddPrinterWithParamsAndColor
.type __wrap_Text_AddPrinterWithParamsAndColor,%function
__wrap_Text_AddPrinterWithParamsAndColor:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1057
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Text_AddPrinterWithParamsAndColor
 .ltorg
.global __wrap_Text_IsPrinterActive
.type __wrap_Text_IsPrinterActive,%function
__wrap_Text_IsPrinterActive:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1058
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Text_IsPrinterActive
 .ltorg
.global __wrap_TextureResourceManager_AddTextureAndAllocVRam
.type __wrap_TextureResourceManager_AddTextureAndAllocVRam,%function
__wrap_TextureResourceManager_AddTextureAndAllocVRam:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1059
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TextureResourceManager_AddTextureAndAllocVRam
 .ltorg
.global __wrap_TextureResourceManager_Create
.type __wrap_TextureResourceManager_Create,%function
__wrap_TextureResourceManager_Create:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1060
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TextureResourceManager_Create
 .ltorg
.global __wrap_TextureResourceManager_Delete
.type __wrap_TextureResourceManager_Delete,%function
__wrap_TextureResourceManager_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1061
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TextureResourceManager_Delete
 .ltorg
.global __wrap_TextureResourceManager_Destroy
.type __wrap_TextureResourceManager_Destroy,%function
__wrap_TextureResourceManager_Destroy:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1062
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TextureResourceManager_Destroy
 .ltorg
.global __wrap_TextureResourceManager_DiscardTextureData
.type __wrap_TextureResourceManager_DiscardTextureData,%function
__wrap_TextureResourceManager_DiscardTextureData:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1063
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TextureResourceManager_DiscardTextureData
 .ltorg
.global __wrap_TextureResourceManager_FindTextureResource
.type __wrap_TextureResourceManager_FindTextureResource,%function
__wrap_TextureResourceManager_FindTextureResource:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1064
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TextureResourceManager_FindTextureResource
 .ltorg
.global __wrap_TextureResourceManager_Free
.type __wrap_TextureResourceManager_Free,%function
__wrap_TextureResourceManager_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1065
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TextureResourceManager_Free
 .ltorg
.global __wrap_TextureResourceManager_FreeAllSlots
.type __wrap_TextureResourceManager_FreeAllSlots,%function
__wrap_TextureResourceManager_FreeAllSlots:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1066
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TextureResourceManager_FreeAllSlots
 .ltorg
.global __wrap_TextureResourceManager_IsIDUnused
.type __wrap_TextureResourceManager_IsIDUnused,%function
__wrap_TextureResourceManager_IsIDUnused:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1067
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TextureResourceManager_IsIDUnused
 .ltorg
.global __wrap_TextureResourceManager_LoadTexture
.type __wrap_TextureResourceManager_LoadTexture,%function
__wrap_TextureResourceManager_LoadTexture:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1068
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TextureResourceManager_LoadTexture
 .ltorg
.global __wrap_TextureResourceManager_New
.type __wrap_TextureResourceManager_New,%function
__wrap_TextureResourceManager_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1069
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TextureResourceManager_New
 .ltorg
.global __wrap_TextureResourceManager_RemoveTextureWithID
.type __wrap_TextureResourceManager_RemoveTextureWithID,%function
__wrap_TextureResourceManager_RemoveTextureWithID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1070
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TextureResourceManager_RemoveTextureWithID
 .ltorg
.global __wrap_TextureResourceManager_UploadResourceToVRam
.type __wrap_TextureResourceManager_UploadResourceToVRam,%function
__wrap_TextureResourceManager_UploadResourceToVRam:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1071
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TextureResourceManager_UploadResourceToVRam
 .ltorg
.global __wrap_TextureResource_GetPaletteKey
.type __wrap_TextureResource_GetPaletteKey,%function
__wrap_TextureResource_GetPaletteKey:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1072
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TextureResource_GetPaletteKey
 .ltorg
.global __wrap_TextureResource_GetTex4x4Key
.type __wrap_TextureResource_GetTex4x4Key,%function
__wrap_TextureResource_GetTex4x4Key:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1073
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TextureResource_GetTex4x4Key
 .ltorg
.global __wrap_TextureResource_GetTexKey
.type __wrap_TextureResource_GetTexKey,%function
__wrap_TextureResource_GetTexKey:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1074
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TextureResource_GetTexKey
 .ltorg
.global __wrap_TextureResource_GetUnderlyingResource
.type __wrap_TextureResource_GetUnderlyingResource,%function
__wrap_TextureResource_GetUnderlyingResource:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1075
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TextureResource_GetUnderlyingResource
 .ltorg
.global __wrap_TileBehavior_IsBikeParking
.type __wrap_TileBehavior_IsBikeParking,%function
__wrap_TileBehavior_IsBikeParking:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1076
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsBikeParking
 .ltorg
.global __wrap_TileBehavior_IsBikeSlopeBottom
.type __wrap_TileBehavior_IsBikeSlopeBottom,%function
__wrap_TileBehavior_IsBikeSlopeBottom:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1077
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsBikeSlopeBottom
 .ltorg
.global __wrap_TileBehavior_IsBikeSlopeTop
.type __wrap_TileBehavior_IsBikeSlopeTop,%function
__wrap_TileBehavior_IsBikeSlopeTop:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1078
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsBikeSlopeTop
 .ltorg
.global __wrap_TileBehavior_IsBookshelf1
.type __wrap_TileBehavior_IsBookshelf1,%function
__wrap_TileBehavior_IsBookshelf1:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1079
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsBookshelf1
 .ltorg
.global __wrap_TileBehavior_IsBookshelf2
.type __wrap_TileBehavior_IsBookshelf2,%function
__wrap_TileBehavior_IsBookshelf2:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1080
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsBookshelf2
 .ltorg
.global __wrap_TileBehavior_IsBridge
.type __wrap_TileBehavior_IsBridge,%function
__wrap_TileBehavior_IsBridge:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1081
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsBridge
 .ltorg
.global __wrap_TileBehavior_IsBridgeStart
.type __wrap_TileBehavior_IsBridgeStart,%function
__wrap_TileBehavior_IsBridgeStart:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1082
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsBridgeStart
 .ltorg
.global __wrap_TileBehavior_IsDeepMud
.type __wrap_TileBehavior_IsDeepMud,%function
__wrap_TileBehavior_IsDeepMud:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1083
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsDeepMud
 .ltorg
.global __wrap_TileBehavior_IsDeepMudWithGrass
.type __wrap_TileBehavior_IsDeepMudWithGrass,%function
__wrap_TileBehavior_IsDeepMudWithGrass:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1084
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsDeepMudWithGrass
 .ltorg
.global __wrap_TileBehavior_IsDoor
.type __wrap_TileBehavior_IsDoor,%function
__wrap_TileBehavior_IsDoor:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1085
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsDoor
 .ltorg
.global __wrap_TileBehavior_IsEscalator
.type __wrap_TileBehavior_IsEscalator,%function
__wrap_TileBehavior_IsEscalator:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1086
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsEscalator
 .ltorg
.global __wrap_TileBehavior_IsEscalatorFlipFace
.type __wrap_TileBehavior_IsEscalatorFlipFace,%function
__wrap_TileBehavior_IsEscalatorFlipFace:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1087
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsEscalatorFlipFace
 .ltorg
.global __wrap_TileBehavior_IsMartShelf1
.type __wrap_TileBehavior_IsMartShelf1,%function
__wrap_TileBehavior_IsMartShelf1:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1088
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsMartShelf1
 .ltorg
.global __wrap_TileBehavior_IsMartShelf2
.type __wrap_TileBehavior_IsMartShelf2,%function
__wrap_TileBehavior_IsMartShelf2:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1089
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsMartShelf2
 .ltorg
.global __wrap_TileBehavior_IsMartShelf3
.type __wrap_TileBehavior_IsMartShelf3,%function
__wrap_TileBehavior_IsMartShelf3:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1090
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsMartShelf3
 .ltorg
.global __wrap_TileBehavior_IsPC
.type __wrap_TileBehavior_IsPC,%function
__wrap_TileBehavior_IsPC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1091
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsPC
 .ltorg
.global __wrap_TileBehavior_IsPuddle
.type __wrap_TileBehavior_IsPuddle,%function
__wrap_TileBehavior_IsPuddle:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1092
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsPuddle
 .ltorg
.global __wrap_TileBehavior_IsReflective
.type __wrap_TileBehavior_IsReflective,%function
__wrap_TileBehavior_IsReflective:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1093
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsReflective
 .ltorg
.global __wrap_TileBehavior_IsRockClimbEastWest
.type __wrap_TileBehavior_IsRockClimbEastWest,%function
__wrap_TileBehavior_IsRockClimbEastWest:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1094
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsRockClimbEastWest
 .ltorg
.global __wrap_TileBehavior_IsRockClimbNorthSouth
.type __wrap_TileBehavior_IsRockClimbNorthSouth,%function
__wrap_TileBehavior_IsRockClimbNorthSouth:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1095
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsRockClimbNorthSouth
 .ltorg
.global __wrap_TileBehavior_IsSlideEastward
.type __wrap_TileBehavior_IsSlideEastward,%function
__wrap_TileBehavior_IsSlideEastward:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1096
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsSlideEastward
 .ltorg
.global __wrap_TileBehavior_IsSlideNorthward
.type __wrap_TileBehavior_IsSlideNorthward,%function
__wrap_TileBehavior_IsSlideNorthward:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1097
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsSlideNorthward
 .ltorg
.global __wrap_TileBehavior_IsSlideSouthward
.type __wrap_TileBehavior_IsSlideSouthward,%function
__wrap_TileBehavior_IsSlideSouthward:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1098
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsSlideSouthward
 .ltorg
.global __wrap_TileBehavior_IsSlideWestward
.type __wrap_TileBehavior_IsSlideWestward,%function
__wrap_TileBehavior_IsSlideWestward:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1099
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsSlideWestward
 .ltorg
.global __wrap_TileBehavior_IsSmallBookshelf1
.type __wrap_TileBehavior_IsSmallBookshelf1,%function
__wrap_TileBehavior_IsSmallBookshelf1:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1100
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsSmallBookshelf1
 .ltorg
.global __wrap_TileBehavior_IsSmallBookshelf2
.type __wrap_TileBehavior_IsSmallBookshelf2,%function
__wrap_TileBehavior_IsSmallBookshelf2:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1101
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsSmallBookshelf2
 .ltorg
.global __wrap_TileBehavior_IsSurfable
.type __wrap_TileBehavior_IsSurfable,%function
__wrap_TileBehavior_IsSurfable:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1102
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsSurfable
 .ltorg
.global __wrap_TileBehavior_IsTV
.type __wrap_TileBehavior_IsTV,%function
__wrap_TileBehavior_IsTV:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1103
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsTV
 .ltorg
.global __wrap_TileBehavior_IsTownMap
.type __wrap_TileBehavior_IsTownMap,%function
__wrap_TileBehavior_IsTownMap:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1104
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsTownMap
 .ltorg
.global __wrap_TileBehavior_IsTrashCan
.type __wrap_TileBehavior_IsTrashCan,%function
__wrap_TileBehavior_IsTrashCan:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1105
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsTrashCan
 .ltorg
.global __wrap_TileBehavior_IsWarpEast
.type __wrap_TileBehavior_IsWarpEast,%function
__wrap_TileBehavior_IsWarpEast:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1106
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsWarpEast
 .ltorg
.global __wrap_TileBehavior_IsWarpEntranceEast
.type __wrap_TileBehavior_IsWarpEntranceEast,%function
__wrap_TileBehavior_IsWarpEntranceEast:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1107
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsWarpEntranceEast
 .ltorg
.global __wrap_TileBehavior_IsWarpEntranceNorth
.type __wrap_TileBehavior_IsWarpEntranceNorth,%function
__wrap_TileBehavior_IsWarpEntranceNorth:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1108
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsWarpEntranceNorth
 .ltorg
.global __wrap_TileBehavior_IsWarpEntranceSouth
.type __wrap_TileBehavior_IsWarpEntranceSouth,%function
__wrap_TileBehavior_IsWarpEntranceSouth:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1109
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsWarpEntranceSouth
 .ltorg
.global __wrap_TileBehavior_IsWarpEntranceWest
.type __wrap_TileBehavior_IsWarpEntranceWest,%function
__wrap_TileBehavior_IsWarpEntranceWest:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1110
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsWarpEntranceWest
 .ltorg
.global __wrap_TileBehavior_IsWarpNorth
.type __wrap_TileBehavior_IsWarpNorth,%function
__wrap_TileBehavior_IsWarpNorth:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1111
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsWarpNorth
 .ltorg
.global __wrap_TileBehavior_IsWarpPanel
.type __wrap_TileBehavior_IsWarpPanel,%function
__wrap_TileBehavior_IsWarpPanel:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1112
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsWarpPanel
 .ltorg
.global __wrap_TileBehavior_IsWarpSouth
.type __wrap_TileBehavior_IsWarpSouth,%function
__wrap_TileBehavior_IsWarpSouth:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1113
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsWarpSouth
 .ltorg
.global __wrap_TileBehavior_IsWarpStairsEast
.type __wrap_TileBehavior_IsWarpStairsEast,%function
__wrap_TileBehavior_IsWarpStairsEast:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1114
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsWarpStairsEast
 .ltorg
.global __wrap_TileBehavior_IsWarpStairsWest
.type __wrap_TileBehavior_IsWarpStairsWest,%function
__wrap_TileBehavior_IsWarpStairsWest:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1115
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsWarpStairsWest
 .ltorg
.global __wrap_TileBehavior_IsWarpWest
.type __wrap_TileBehavior_IsWarpWest,%function
__wrap_TileBehavior_IsWarpWest:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1116
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsWarpWest
 .ltorg
.global __wrap_TileBehavior_IsWaterfall
.type __wrap_TileBehavior_IsWaterfall,%function
__wrap_TileBehavior_IsWaterfall:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1117
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TileBehavior_IsWaterfall
 .ltorg
.global __wrap_TrainerInfo_BadgeCount
.type __wrap_TrainerInfo_BadgeCount,%function
__wrap_TrainerInfo_BadgeCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1118
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TrainerInfo_BadgeCount
 .ltorg
.global __wrap_TrainerInfo_Gender
.type __wrap_TrainerInfo_Gender,%function
__wrap_TrainerInfo_Gender:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1119
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TrainerInfo_Gender
 .ltorg
.global __wrap_TrainerInfo_HasBadge
.type __wrap_TrainerInfo_HasBadge,%function
__wrap_TrainerInfo_HasBadge:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1120
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TrainerInfo_HasBadge
 .ltorg
.global __wrap_TrainerInfo_ID
.type __wrap_TrainerInfo_ID,%function
__wrap_TrainerInfo_ID:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1121
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TrainerInfo_ID
 .ltorg
.global __wrap_TrainerInfo_Money
.type __wrap_TrainerInfo_Money,%function
__wrap_TrainerInfo_Money:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1122
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TrainerInfo_Money
 .ltorg
.global __wrap_TrainerInfo_Name
.type __wrap_TrainerInfo_Name,%function
__wrap_TrainerInfo_Name:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1123
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TrainerInfo_Name
 .ltorg
.global __wrap_TrainerInfo_NameNewString
.type __wrap_TrainerInfo_NameNewString,%function
__wrap_TrainerInfo_NameNewString:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1124
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TrainerInfo_NameNewString
 .ltorg
.global __wrap_TrainerInfo_New
.type __wrap_TrainerInfo_New,%function
__wrap_TrainerInfo_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1125
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TrainerInfo_New
 .ltorg
.global __wrap_TrainerInfo_SetName
.type __wrap_TrainerInfo_SetName,%function
__wrap_TrainerInfo_SetName:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1126
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_TrainerInfo_SetName
 .ltorg
.global __wrap_UndergroundMan_CheckForTouchInput
.type __wrap_UndergroundMan_CheckForTouchInput,%function
__wrap_UndergroundMan_CheckForTouchInput:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1127
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_UndergroundMan_CheckForTouchInput
 .ltorg
.global __wrap_UndergroundMan_GetSecretBaseXCoordinate
.type __wrap_UndergroundMan_GetSecretBaseXCoordinate,%function
__wrap_UndergroundMan_GetSecretBaseXCoordinate:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1128
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_UndergroundMan_GetSecretBaseXCoordinate
 .ltorg
.global __wrap_UndergroundMan_GetSecretBaseZCoordinate
.type __wrap_UndergroundMan_GetSecretBaseZCoordinate,%function
__wrap_UndergroundMan_GetSecretBaseZCoordinate:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1129
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_UndergroundMan_GetSecretBaseZCoordinate
 .ltorg
.global __wrap_UndergroundMan_OpenMenu
.type __wrap_UndergroundMan_OpenMenu,%function
__wrap_UndergroundMan_OpenMenu:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1130
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_UndergroundMan_OpenMenu
 .ltorg
.global __wrap_UndergroundMan_ProcessInteract
.type __wrap_UndergroundMan_ProcessInteract,%function
__wrap_UndergroundMan_ProcessInteract:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1131
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_UndergroundMan_ProcessInteract
 .ltorg
.global __wrap_UndergroundMan_ProgressInteractCooldown
.type __wrap_UndergroundMan_ProgressInteractCooldown,%function
__wrap_UndergroundMan_ProgressInteractCooldown:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1132
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_UndergroundMan_ProgressInteractCooldown
 .ltorg
.global __wrap_UpdateMonStatusAndTrainerInfo
.type __wrap_UpdateMonStatusAndTrainerInfo,%function
__wrap_UpdateMonStatusAndTrainerInfo:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1133
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_UpdateMonStatusAndTrainerInfo
 .ltorg
.global __wrap_VecFx32_FromXYZ
.type __wrap_VecFx32_FromXYZ,%function
__wrap_VecFx32_FromXYZ:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1134
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_VecFx32_FromXYZ
 .ltorg
.global __wrap_VecFx32_SetPosFromMapCoords
.type __wrap_VecFx32_SetPosFromMapCoords,%function
__wrap_VecFx32_SetPosFromMapCoords:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1135
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_VecFx32_SetPosFromMapCoords
 .ltorg
.global __wrap_VeilstoneGymObjectRenderer_Free
.type __wrap_VeilstoneGymObjectRenderer_Free,%function
__wrap_VeilstoneGymObjectRenderer_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1136
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_VeilstoneGymObjectRenderer_Free
 .ltorg
.global __wrap_VeilstoneGymObjectRenderer_New
.type __wrap_VeilstoneGymObjectRenderer_New,%function
__wrap_VeilstoneGymObjectRenderer_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1137
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_VeilstoneGymObjectRenderer_New
 .ltorg
.global __wrap_VeilstoneGym_HitPunchingBag
.type __wrap_VeilstoneGym_HitPunchingBag,%function
__wrap_VeilstoneGym_HitPunchingBag:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1138
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_VeilstoneGym_HitPunchingBag
 .ltorg
.global __wrap_VramTransfer_Free
.type __wrap_VramTransfer_Free,%function
__wrap_VramTransfer_Free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1139
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_VramTransfer_Free
 .ltorg
.global __wrap_VramTransfer_New
.type __wrap_VramTransfer_New,%function
__wrap_VramTransfer_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1140
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_VramTransfer_New
 .ltorg
.global __wrap_VramTransfer_Process
.type __wrap_VramTransfer_Process,%function
__wrap_VramTransfer_Process:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1141
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_VramTransfer_Process
 .ltorg
.global __wrap_VramTransfer_Request
.type __wrap_VramTransfer_Request,%function
__wrap_VramTransfer_Request:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1142
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_VramTransfer_Request
 .ltorg
.global __wrap_VsSeeker_UpdateStepCount
.type __wrap_VsSeeker_UpdateStepCount,%function
__wrap_VsSeeker_UpdateStepCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1143
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_VsSeeker_UpdateStepCount
 .ltorg
.global __wrap_WiFiHistory_FlagGeonetLinkInfo
.type __wrap_WiFiHistory_FlagGeonetLinkInfo,%function
__wrap_WiFiHistory_FlagGeonetLinkInfo:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1144
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_WiFiHistory_FlagGeonetLinkInfo
 .ltorg
.global __wrap_WiFiList_GetValidFriendsCount
.type __wrap_WiFiList_GetValidFriendsCount,%function
__wrap_WiFiList_GetValidFriendsCount:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1145
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_WiFiList_GetValidFriendsCount
 .ltorg
.global __wrap_WiFiList_IsValidFriendData
.type __wrap_WiFiList_IsValidFriendData,%function
__wrap_WiFiList_IsValidFriendData:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1146
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_WiFiList_IsValidFriendData
 .ltorg
.global __wrap_WifiBattleTowerRecord_UpdateBattlePoints
.type __wrap_WifiBattleTowerRecord_UpdateBattlePoints,%function
__wrap_WifiBattleTowerRecord_UpdateBattlePoints:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1147
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_WifiBattleTowerRecord_UpdateBattlePoints
 .ltorg
.global __wrap_WildEncounters_TileHasEncounterRate
.type __wrap_WildEncounters_TileHasEncounterRate,%function
__wrap_WildEncounters_TileHasEncounterRate:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1148
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_WildEncounters_TileHasEncounterRate
 .ltorg
.global __wrap_WildEncounters_TryFishingEncounter
.type __wrap_WildEncounters_TryFishingEncounter,%function
__wrap_WildEncounters_TryFishingEncounter:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1149
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_WildEncounters_TryFishingEncounter
 .ltorg
.global __wrap_WildEncounters_TryMudEncounter
.type __wrap_WildEncounters_TryMudEncounter,%function
__wrap_WildEncounters_TryMudEncounter:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1150
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_WildEncounters_TryMudEncounter
 .ltorg
.global __wrap_WildEncounters_TrySweetScentEncounter
.type __wrap_WildEncounters_TrySweetScentEncounter,%function
__wrap_WildEncounters_TrySweetScentEncounter:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1151
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_WildEncounters_TrySweetScentEncounter
 .ltorg
.global __wrap_WildEncounters_TryWildEncounter
.type __wrap_WildEncounters_TryWildEncounter,%function
__wrap_WildEncounters_TryWildEncounter:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1152
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_WildEncounters_TryWildEncounter
 .ltorg
.global __wrap_Window_Add
.type __wrap_Window_Add,%function
__wrap_Window_Add:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1153
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Window_Add
 .ltorg
.global __wrap_Window_BlitBitmapRect
.type __wrap_Window_BlitBitmapRect,%function
__wrap_Window_BlitBitmapRect:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1154
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Window_BlitBitmapRect
 .ltorg
.global __wrap_Window_ClearAndCopyToVRAM
.type __wrap_Window_ClearAndCopyToVRAM,%function
__wrap_Window_ClearAndCopyToVRAM:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1155
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Window_ClearAndCopyToVRAM
 .ltorg
.global __wrap_Window_CopyToVRAM
.type __wrap_Window_CopyToVRAM,%function
__wrap_Window_CopyToVRAM:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1156
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Window_CopyToVRAM
 .ltorg
.global __wrap_Window_DrawStandardFrame
.type __wrap_Window_DrawStandardFrame,%function
__wrap_Window_DrawStandardFrame:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1157
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Window_DrawStandardFrame
 .ltorg
.global __wrap_Window_EraseMessageBox
.type __wrap_Window_EraseMessageBox,%function
__wrap_Window_EraseMessageBox:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1158
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Window_EraseMessageBox
 .ltorg
.global __wrap_Window_EraseStandardFrame
.type __wrap_Window_EraseStandardFrame,%function
__wrap_Window_EraseStandardFrame:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1159
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Window_EraseStandardFrame
 .ltorg
.global __wrap_Window_FillRectWithColor
.type __wrap_Window_FillRectWithColor,%function
__wrap_Window_FillRectWithColor:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1160
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Window_FillRectWithColor
 .ltorg
.global __wrap_Window_FillTilemap
.type __wrap_Window_FillTilemap,%function
__wrap_Window_FillTilemap:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1161
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Window_FillTilemap
 .ltorg
.global __wrap_Window_IsInUse
.type __wrap_Window_IsInUse,%function
__wrap_Window_IsInUse:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1162
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Window_IsInUse
 .ltorg
.global __wrap_Window_New
.type __wrap_Window_New,%function
__wrap_Window_New:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1163
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Window_New
 .ltorg
.global __wrap_Window_Remove
.type __wrap_Window_Remove,%function
__wrap_Window_Remove:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1164
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Window_Remove
 .ltorg
.global __wrap_Window_ScheduleCopyToVRAM
.type __wrap_Window_ScheduleCopyToVRAM,%function
__wrap_Window_ScheduleCopyToVRAM:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1165
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Window_ScheduleCopyToVRAM
 .ltorg
.global __wrap_Windows_Delete
.type __wrap_Windows_Delete,%function
__wrap_Windows_Delete:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1166
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_Windows_Delete
 .ltorg
.global __wrap_WirelessManager_GetConnectedBitmap
.type __wrap_WirelessManager_GetConnectedBitmap,%function
__wrap_WirelessManager_GetConnectedBitmap:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1167
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_WirelessManager_GetConnectedBitmap
 .ltorg
.global __wrap_free
.type __wrap_free,%function
__wrap_free:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1168
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_free
 .ltorg
.global __wrap_malloc
.type __wrap_malloc,%function
__wrap_malloc:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1169
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_malloc
 .ltorg
.global __wrap_ov56_02256410
.type __wrap_ov56_02256410,%function
__wrap_ov56_02256410:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1170
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov56_02256410
 .ltorg
.global __wrap_ov56_02256468
.type __wrap_ov56_02256468,%function
__wrap_ov56_02256468:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1171
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov56_02256468
 .ltorg
.global __wrap_ov5_021D1434
.type __wrap_ov5_021D1434,%function
__wrap_ov5_021D1434:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1172
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021D1434
 .ltorg
.global __wrap_ov5_021D143C
.type __wrap_ov5_021D143C,%function
__wrap_ov5_021D143C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1173
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021D143C
 .ltorg
.global __wrap_ov5_021D16F4
.type __wrap_ov5_021D16F4,%function
__wrap_ov5_021D16F4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1174
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021D16F4
 .ltorg
.global __wrap_ov5_021D1718
.type __wrap_ov5_021D1718,%function
__wrap_ov5_021D1718:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1175
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021D1718
 .ltorg
.global __wrap_ov5_021D1A94
.type __wrap_ov5_021D1A94,%function
__wrap_ov5_021D1A94:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1176
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021D1A94
 .ltorg
.global __wrap_ov5_021D1AE4
.type __wrap_ov5_021D1AE4,%function
__wrap_ov5_021D1AE4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1177
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021D1AE4
 .ltorg
.global __wrap_ov5_021D1B18
.type __wrap_ov5_021D1B18,%function
__wrap_ov5_021D1B18:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1178
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021D1B18
 .ltorg
.global __wrap_ov5_021D1B6C
.type __wrap_ov5_021D1B6C,%function
__wrap_ov5_021D1B6C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1179
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021D1B6C
 .ltorg
.global __wrap_ov5_021D1BEC
.type __wrap_ov5_021D1BEC,%function
__wrap_ov5_021D1BEC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1180
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021D1BEC
 .ltorg
.global __wrap_ov5_021D1C2C
.type __wrap_ov5_021D1C2C,%function
__wrap_ov5_021D1C2C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1181
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021D1C2C
 .ltorg
.global __wrap_ov5_021D4D78
.type __wrap_ov5_021D4D78,%function
__wrap_ov5_021D4D78:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1182
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021D4D78
 .ltorg
.global __wrap_ov5_021D5EB8
.type __wrap_ov5_021D5EB8,%function
__wrap_ov5_021D5EB8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1183
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021D5EB8
 .ltorg
.global __wrap_ov5_021D5EF8
.type __wrap_ov5_021D5EF8,%function
__wrap_ov5_021D5EF8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1184
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021D5EF8
 .ltorg
.global __wrap_ov5_021D5F24
.type __wrap_ov5_021D5F24,%function
__wrap_ov5_021D5F24:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1185
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021D5F24
 .ltorg
.global __wrap_ov5_021D5F7C
.type __wrap_ov5_021D5F7C,%function
__wrap_ov5_021D5F7C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1186
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021D5F7C
 .ltorg
.global __wrap_ov5_021D5FE0
.type __wrap_ov5_021D5FE0,%function
__wrap_ov5_021D5FE0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1187
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021D5FE0
 .ltorg
.global __wrap_ov5_021DE3D0
.type __wrap_ov5_021DE3D0,%function
__wrap_ov5_021DE3D0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1188
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DE3D0
 .ltorg
.global __wrap_ov5_021DE4AC
.type __wrap_ov5_021DE4AC,%function
__wrap_ov5_021DE4AC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1189
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DE4AC
 .ltorg
.global __wrap_ov5_021DE4CC
.type __wrap_ov5_021DE4CC,%function
__wrap_ov5_021DE4CC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1190
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DE4CC
 .ltorg
.global __wrap_ov5_021DE5A4
.type __wrap_ov5_021DE5A4,%function
__wrap_ov5_021DE5A4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1191
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DE5A4
 .ltorg
.global __wrap_ov5_021DE62C
.type __wrap_ov5_021DE62C,%function
__wrap_ov5_021DE62C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1192
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DE62C
 .ltorg
.global __wrap_ov5_021DE6A4
.type __wrap_ov5_021DE6A4,%function
__wrap_ov5_021DE6A4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1193
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DE6A4
 .ltorg
.global __wrap_ov5_021DE6BC
.type __wrap_ov5_021DE6BC,%function
__wrap_ov5_021DE6BC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1194
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DE6BC
 .ltorg
.global __wrap_ov5_021DE6C4
.type __wrap_ov5_021DE6C4,%function
__wrap_ov5_021DE6C4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1195
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DE6C4
 .ltorg
.global __wrap_ov5_021DE71C
.type __wrap_ov5_021DE71C,%function
__wrap_ov5_021DE71C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1196
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DE71C
 .ltorg
.global __wrap_ov5_021DE8F8
.type __wrap_ov5_021DE8F8,%function
__wrap_ov5_021DE8F8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1197
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DE8F8
 .ltorg
.global __wrap_ov5_021DE928
.type __wrap_ov5_021DE928,%function
__wrap_ov5_021DE928:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1198
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DE928
 .ltorg
.global __wrap_ov5_021DE948
.type __wrap_ov5_021DE948,%function
__wrap_ov5_021DE948:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1199
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DE948
 .ltorg
.global __wrap_ov5_021DE988
.type __wrap_ov5_021DE988,%function
__wrap_ov5_021DE988:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1200
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DE988
 .ltorg
.global __wrap_ov5_021DEBEC
.type __wrap_ov5_021DEBEC,%function
__wrap_ov5_021DEBEC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1201
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DEBEC
 .ltorg
.global __wrap_ov5_021DEC18
.type __wrap_ov5_021DEC18,%function
__wrap_ov5_021DEC18:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1202
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DEC18
 .ltorg
.global __wrap_ov5_021DEC38
.type __wrap_ov5_021DEC38,%function
__wrap_ov5_021DEC38:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1203
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DEC38
 .ltorg
.global __wrap_ov5_021DECB8
.type __wrap_ov5_021DECB8,%function
__wrap_ov5_021DECB8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1204
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DECB8
 .ltorg
.global __wrap_ov5_021DECEC
.type __wrap_ov5_021DECEC,%function
__wrap_ov5_021DECEC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1205
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DECEC
 .ltorg
.global __wrap_ov5_021DED04
.type __wrap_ov5_021DED04,%function
__wrap_ov5_021DED04:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1206
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DED04
 .ltorg
.global __wrap_ov5_021DED20
.type __wrap_ov5_021DED20,%function
__wrap_ov5_021DED20:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1207
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DED20
 .ltorg
.global __wrap_ov5_021DEF8C
.type __wrap_ov5_021DEF8C,%function
__wrap_ov5_021DEF8C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1208
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DEF8C
 .ltorg
.global __wrap_ov5_021DEFA0
.type __wrap_ov5_021DEFA0,%function
__wrap_ov5_021DEFA0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1209
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DEFA0
 .ltorg
.global __wrap_ov5_021DF038
.type __wrap_ov5_021DF038,%function
__wrap_ov5_021DF038:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1210
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DF038
 .ltorg
.global __wrap_ov5_021DF054
.type __wrap_ov5_021DF054,%function
__wrap_ov5_021DF054:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1211
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DF054
 .ltorg
.global __wrap_ov5_021DF070
.type __wrap_ov5_021DF070,%function
__wrap_ov5_021DF070:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1212
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DF070
 .ltorg
.global __wrap_ov5_021DF084
.type __wrap_ov5_021DF084,%function
__wrap_ov5_021DF084:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1213
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DF084
 .ltorg
.global __wrap_ov5_021DF0CC
.type __wrap_ov5_021DF0CC,%function
__wrap_ov5_021DF0CC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1214
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DF0CC
 .ltorg
.global __wrap_ov5_021DF17C
.type __wrap_ov5_021DF17C,%function
__wrap_ov5_021DF17C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1215
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DF17C
 .ltorg
.global __wrap_ov5_021DF1CC
.type __wrap_ov5_021DF1CC,%function
__wrap_ov5_021DF1CC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1216
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DF1CC
 .ltorg
.global __wrap_ov5_021DF208
.type __wrap_ov5_021DF208,%function
__wrap_ov5_021DF208:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1217
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DF208
 .ltorg
.global __wrap_ov5_021DF224
.type __wrap_ov5_021DF224,%function
__wrap_ov5_021DF224:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1218
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DF224
 .ltorg
.global __wrap_ov5_021DF488
.type __wrap_ov5_021DF488,%function
__wrap_ov5_021DF488:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1219
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DF488
 .ltorg
.global __wrap_ov5_021DF7F8
.type __wrap_ov5_021DF7F8,%function
__wrap_ov5_021DF7F8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1220
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DF7F8
 .ltorg
.global __wrap_ov5_021DF84C
.type __wrap_ov5_021DF84C,%function
__wrap_ov5_021DF84C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1221
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DF84C
 .ltorg
.global __wrap_ov5_021DF864
.type __wrap_ov5_021DF864,%function
__wrap_ov5_021DF864:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1222
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DF864
 .ltorg
.global __wrap_ov5_021DF9D4
.type __wrap_ov5_021DF9D4,%function
__wrap_ov5_021DF9D4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1223
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DF9D4
 .ltorg
.global __wrap_ov5_021DF9E0
.type __wrap_ov5_021DF9E0,%function
__wrap_ov5_021DF9E0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1224
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DF9E0
 .ltorg
.global __wrap_ov5_021DF9FC
.type __wrap_ov5_021DF9FC,%function
__wrap_ov5_021DF9FC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1225
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DF9FC
 .ltorg
.global __wrap_ov5_021DFA08
.type __wrap_ov5_021DFA08,%function
__wrap_ov5_021DFA08:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1226
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DFA08
 .ltorg
.global __wrap_ov5_021DFA14
.type __wrap_ov5_021DFA14,%function
__wrap_ov5_021DFA14:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1227
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DFA14
 .ltorg
.global __wrap_ov5_021DFA30
.type __wrap_ov5_021DFA30,%function
__wrap_ov5_021DFA30:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1228
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DFA30
 .ltorg
.global __wrap_ov5_021DFA3C
.type __wrap_ov5_021DFA3C,%function
__wrap_ov5_021DFA3C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1229
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DFA3C
 .ltorg
.global __wrap_ov5_021DFA7C
.type __wrap_ov5_021DFA7C,%function
__wrap_ov5_021DFA7C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1230
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DFA7C
 .ltorg
.global __wrap_ov5_021DFDE0
.type __wrap_ov5_021DFDE0,%function
__wrap_ov5_021DFDE0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1231
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021DFDE0
 .ltorg
.global __wrap_ov5_021E1154
.type __wrap_ov5_021E1154,%function
__wrap_ov5_021E1154:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1232
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021E1154
 .ltorg
.global __wrap_ov5_021EA728
.type __wrap_ov5_021EA728,%function
__wrap_ov5_021EA728:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1233
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EA728
 .ltorg
.global __wrap_ov5_021EA790
.type __wrap_ov5_021EA790,%function
__wrap_ov5_021EA790:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1234
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EA790
 .ltorg
.global __wrap_ov5_021EA7CC
.type __wrap_ov5_021EA7CC,%function
__wrap_ov5_021EA7CC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1235
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EA7CC
 .ltorg
.global __wrap_ov5_021EA830
.type __wrap_ov5_021EA830,%function
__wrap_ov5_021EA830:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1236
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EA830
 .ltorg
.global __wrap_ov5_021EA848
.type __wrap_ov5_021EA848,%function
__wrap_ov5_021EA848:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1237
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EA848
 .ltorg
.global __wrap_ov5_021EA854
.type __wrap_ov5_021EA854,%function
__wrap_ov5_021EA854:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1238
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EA854
 .ltorg
.global __wrap_ov5_021EB1A0
.type __wrap_ov5_021EB1A0,%function
__wrap_ov5_021EB1A0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1239
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EB1A0
 .ltorg
.global __wrap_ov5_021EB2EC
.type __wrap_ov5_021EB2EC,%function
__wrap_ov5_021EB2EC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1240
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EB2EC
 .ltorg
.global __wrap_ov5_021EB314
.type __wrap_ov5_021EB314,%function
__wrap_ov5_021EB314:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1241
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EB314
 .ltorg
.global __wrap_ov5_021EB328
.type __wrap_ov5_021EB328,%function
__wrap_ov5_021EB328:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1242
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EB328
 .ltorg
.global __wrap_ov5_021EB354
.type __wrap_ov5_021EB354,%function
__wrap_ov5_021EB354:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1243
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EB354
 .ltorg
.global __wrap_ov5_021EB398
.type __wrap_ov5_021EB398,%function
__wrap_ov5_021EB398:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1244
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EB398
 .ltorg
.global __wrap_ov5_021EB40C
.type __wrap_ov5_021EB40C,%function
__wrap_ov5_021EB40C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1245
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EB40C
 .ltorg
.global __wrap_ov5_021EB438
.type __wrap_ov5_021EB438,%function
__wrap_ov5_021EB438:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1246
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EB438
 .ltorg
.global __wrap_ov5_021EB720
.type __wrap_ov5_021EB720,%function
__wrap_ov5_021EB720:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1247
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EB720
 .ltorg
.global __wrap_ov5_021EB7F8
.type __wrap_ov5_021EB7F8,%function
__wrap_ov5_021EB7F8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1248
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EB7F8
 .ltorg
.global __wrap_ov5_021EB834
.type __wrap_ov5_021EB834,%function
__wrap_ov5_021EB834:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1249
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EB834
 .ltorg
.global __wrap_ov5_021EB848
.type __wrap_ov5_021EB848,%function
__wrap_ov5_021EB848:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1250
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EB848
 .ltorg
.global __wrap_ov5_021EB874
.type __wrap_ov5_021EB874,%function
__wrap_ov5_021EB874:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1251
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EB874
 .ltorg
.global __wrap_ov5_021EB8B8
.type __wrap_ov5_021EB8B8,%function
__wrap_ov5_021EB8B8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1252
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EB8B8
 .ltorg
.global __wrap_ov5_021EB944
.type __wrap_ov5_021EB944,%function
__wrap_ov5_021EB944:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1253
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EB944
 .ltorg
.global __wrap_ov5_021EBA0C
.type __wrap_ov5_021EBA0C,%function
__wrap_ov5_021EBA0C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1254
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EBA0C
 .ltorg
.global __wrap_ov5_021EBA34
.type __wrap_ov5_021EBA34,%function
__wrap_ov5_021EBA34:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1255
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EBA34
 .ltorg
.global __wrap_ov5_021EBA60
.type __wrap_ov5_021EBA60,%function
__wrap_ov5_021EBA60:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1256
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EBA60
 .ltorg
.global __wrap_ov5_021EBA8C
.type __wrap_ov5_021EBA8C,%function
__wrap_ov5_021EBA8C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1257
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EBA8C
 .ltorg
.global __wrap_ov5_021EBAD0
.type __wrap_ov5_021EBAD0,%function
__wrap_ov5_021EBAD0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1258
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EBAD0
 .ltorg
.global __wrap_ov5_021EBF50
.type __wrap_ov5_021EBF50,%function
__wrap_ov5_021EBF50:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1259
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EBF50
 .ltorg
.global __wrap_ov5_021EC15C
.type __wrap_ov5_021EC15C,%function
__wrap_ov5_021EC15C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1260
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC15C
 .ltorg
.global __wrap_ov5_021EC3F0
.type __wrap_ov5_021EC3F0,%function
__wrap_ov5_021EC3F0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1261
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC3F0
 .ltorg
.global __wrap_ov5_021EC454
.type __wrap_ov5_021EC454,%function
__wrap_ov5_021EC454:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1262
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC454
 .ltorg
.global __wrap_ov5_021EC4BC
.type __wrap_ov5_021EC4BC,%function
__wrap_ov5_021EC4BC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1263
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC4BC
 .ltorg
.global __wrap_ov5_021EC554
.type __wrap_ov5_021EC554,%function
__wrap_ov5_021EC554:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1264
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC554
 .ltorg
.global __wrap_ov5_021EC5C0
.type __wrap_ov5_021EC5C0,%function
__wrap_ov5_021EC5C0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1265
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC5C0
 .ltorg
.global __wrap_ov5_021EC734
.type __wrap_ov5_021EC734,%function
__wrap_ov5_021EC734:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1266
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC734
 .ltorg
.global __wrap_ov5_021EC75C
.type __wrap_ov5_021EC75C,%function
__wrap_ov5_021EC75C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1267
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC75C
 .ltorg
.global __wrap_ov5_021EC760
.type __wrap_ov5_021EC760,%function
__wrap_ov5_021EC760:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1268
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC760
 .ltorg
.global __wrap_ov5_021EC778
.type __wrap_ov5_021EC778,%function
__wrap_ov5_021EC778:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1269
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC778
 .ltorg
.global __wrap_ov5_021EC790
.type __wrap_ov5_021EC790,%function
__wrap_ov5_021EC790:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1270
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC790
 .ltorg
.global __wrap_ov5_021EC7B8
.type __wrap_ov5_021EC7B8,%function
__wrap_ov5_021EC7B8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1271
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC7B8
 .ltorg
.global __wrap_ov5_021EC7D0
.type __wrap_ov5_021EC7D0,%function
__wrap_ov5_021EC7D0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1272
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC7D0
 .ltorg
.global __wrap_ov5_021EC7F0
.type __wrap_ov5_021EC7F0,%function
__wrap_ov5_021EC7F0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1273
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC7F0
 .ltorg
.global __wrap_ov5_021EC824
.type __wrap_ov5_021EC824,%function
__wrap_ov5_021EC824:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1274
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC824
 .ltorg
.global __wrap_ov5_021EC858
.type __wrap_ov5_021EC858,%function
__wrap_ov5_021EC858:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1275
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC858
 .ltorg
.global __wrap_ov5_021EC86C
.type __wrap_ov5_021EC86C,%function
__wrap_ov5_021EC86C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1276
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC86C
 .ltorg
.global __wrap_ov5_021EC898
.type __wrap_ov5_021EC898,%function
__wrap_ov5_021EC898:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1277
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC898
 .ltorg
.global __wrap_ov5_021EC8EC
.type __wrap_ov5_021EC8EC,%function
__wrap_ov5_021EC8EC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1278
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC8EC
 .ltorg
.global __wrap_ov5_021EC938
.type __wrap_ov5_021EC938,%function
__wrap_ov5_021EC938:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1279
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC938
 .ltorg
.global __wrap_ov5_021EC95C
.type __wrap_ov5_021EC95C,%function
__wrap_ov5_021EC95C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1280
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC95C
 .ltorg
.global __wrap_ov5_021EC970
.type __wrap_ov5_021EC970,%function
__wrap_ov5_021EC970:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1281
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC970
 .ltorg
.global __wrap_ov5_021EC9A0
.type __wrap_ov5_021EC9A0,%function
__wrap_ov5_021EC9A0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1282
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC9A0
 .ltorg
.global __wrap_ov5_021EC9E8
.type __wrap_ov5_021EC9E8,%function
__wrap_ov5_021EC9E8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1283
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EC9E8
 .ltorg
.global __wrap_ov5_021ECA70
.type __wrap_ov5_021ECA70,%function
__wrap_ov5_021ECA70:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1284
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECA70
 .ltorg
.global __wrap_ov5_021ECA90
.type __wrap_ov5_021ECA90,%function
__wrap_ov5_021ECA90:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1285
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECA90
 .ltorg
.global __wrap_ov5_021ECABC
.type __wrap_ov5_021ECABC,%function
__wrap_ov5_021ECABC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1286
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECABC
 .ltorg
.global __wrap_ov5_021ECAC4
.type __wrap_ov5_021ECAC4,%function
__wrap_ov5_021ECAC4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1287
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECAC4
 .ltorg
.global __wrap_ov5_021ECAF0
.type __wrap_ov5_021ECAF0,%function
__wrap_ov5_021ECAF0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1288
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECAF0
 .ltorg
.global __wrap_ov5_021ECB34
.type __wrap_ov5_021ECB34,%function
__wrap_ov5_021ECB34:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1289
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECB34
 .ltorg
.global __wrap_ov5_021ECB58
.type __wrap_ov5_021ECB58,%function
__wrap_ov5_021ECB58:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1290
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECB58
 .ltorg
.global __wrap_ov5_021ECB78
.type __wrap_ov5_021ECB78,%function
__wrap_ov5_021ECB78:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1291
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECB78
 .ltorg
.global __wrap_ov5_021ECB80
.type __wrap_ov5_021ECB80,%function
__wrap_ov5_021ECB80:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1292
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECB80
 .ltorg
.global __wrap_ov5_021ECC20
.type __wrap_ov5_021ECC20,%function
__wrap_ov5_021ECC20:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1293
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECC20
 .ltorg
.global __wrap_ov5_021ECC78
.type __wrap_ov5_021ECC78,%function
__wrap_ov5_021ECC78:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1294
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECC78
 .ltorg
.global __wrap_ov5_021ECD04
.type __wrap_ov5_021ECD04,%function
__wrap_ov5_021ECD04:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1295
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECD04
 .ltorg
.global __wrap_ov5_021ECD38
.type __wrap_ov5_021ECD38,%function
__wrap_ov5_021ECD38:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1296
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECD38
 .ltorg
.global __wrap_ov5_021ECD68
.type __wrap_ov5_021ECD68,%function
__wrap_ov5_021ECD68:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1297
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECD68
 .ltorg
.global __wrap_ov5_021ECDA0
.type __wrap_ov5_021ECDA0,%function
__wrap_ov5_021ECDA0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1298
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECDA0
 .ltorg
.global __wrap_ov5_021ECE18
.type __wrap_ov5_021ECE18,%function
__wrap_ov5_021ECE18:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1299
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECE18
 .ltorg
.global __wrap_ov5_021ECE30
.type __wrap_ov5_021ECE30,%function
__wrap_ov5_021ECE30:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1300
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECE30
 .ltorg
.global __wrap_ov5_021ECE34
.type __wrap_ov5_021ECE34,%function
__wrap_ov5_021ECE34:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1301
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECE34
 .ltorg
.global __wrap_ov5_021ECE38
.type __wrap_ov5_021ECE38,%function
__wrap_ov5_021ECE38:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1302
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECE38
 .ltorg
.global __wrap_ov5_021ECE3C
.type __wrap_ov5_021ECE3C,%function
__wrap_ov5_021ECE3C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1303
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECE3C
 .ltorg
.global __wrap_ov5_021ECE40
.type __wrap_ov5_021ECE40,%function
__wrap_ov5_021ECE40:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1304
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECE40
 .ltorg
.global __wrap_ov5_021ECE94
.type __wrap_ov5_021ECE94,%function
__wrap_ov5_021ECE94:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1305
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECE94
 .ltorg
.global __wrap_ov5_021ECF04
.type __wrap_ov5_021ECF04,%function
__wrap_ov5_021ECF04:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1306
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECF04
 .ltorg
.global __wrap_ov5_021ECFA4
.type __wrap_ov5_021ECFA4,%function
__wrap_ov5_021ECFA4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1307
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ECFA4
 .ltorg
.global __wrap_ov5_021ED01C
.type __wrap_ov5_021ED01C,%function
__wrap_ov5_021ED01C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1308
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ED01C
 .ltorg
.global __wrap_ov5_021ED03C
.type __wrap_ov5_021ED03C,%function
__wrap_ov5_021ED03C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1309
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ED03C
 .ltorg
.global __wrap_ov5_021ED150
.type __wrap_ov5_021ED150,%function
__wrap_ov5_021ED150:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1310
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021ED150
 .ltorg
.global __wrap_ov5_021EDC8C
.type __wrap_ov5_021EDC8C,%function
__wrap_ov5_021EDC8C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1311
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EDC8C
 .ltorg
.global __wrap_ov5_021EDD94
.type __wrap_ov5_021EDD94,%function
__wrap_ov5_021EDD94:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1312
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EDD94
 .ltorg
.global __wrap_ov5_021EDEB4
.type __wrap_ov5_021EDEB4,%function
__wrap_ov5_021EDEB4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1313
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EDEB4
 .ltorg
.global __wrap_ov5_021EDF18
.type __wrap_ov5_021EDF18,%function
__wrap_ov5_021EDF18:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1314
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EDF18
 .ltorg
.global __wrap_ov5_021EDF24
.type __wrap_ov5_021EDF24,%function
__wrap_ov5_021EDF24:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1315
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EDF24
 .ltorg
.global __wrap_ov5_021EDF30
.type __wrap_ov5_021EDF30,%function
__wrap_ov5_021EDF30:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1316
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EDF30
 .ltorg
.global __wrap_ov5_021EE3FC
.type __wrap_ov5_021EE3FC,%function
__wrap_ov5_021EE3FC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1317
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EE3FC
 .ltorg
.global __wrap_ov5_021EE7C0
.type __wrap_ov5_021EE7C0,%function
__wrap_ov5_021EE7C0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1318
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EE7C0
 .ltorg
.global __wrap_ov5_021EF4BC
.type __wrap_ov5_021EF4BC,%function
__wrap_ov5_021EF4BC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1319
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EF4BC
 .ltorg
.global __wrap_ov5_021EF4F8
.type __wrap_ov5_021EF4F8,%function
__wrap_ov5_021EF4F8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1320
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021EF4F8
 .ltorg
.global __wrap_ov5_021F0030
.type __wrap_ov5_021F0030,%function
__wrap_ov5_021F0030:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1321
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F0030
 .ltorg
.global __wrap_ov5_021F0484
.type __wrap_ov5_021F0484,%function
__wrap_ov5_021F0484:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1322
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F0484
 .ltorg
.global __wrap_ov5_021F0488
.type __wrap_ov5_021F0488,%function
__wrap_ov5_021F0488:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1323
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F0488
 .ltorg
.global __wrap_ov5_021F10E8
.type __wrap_ov5_021F10E8,%function
__wrap_ov5_021F10E8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1324
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F10E8
 .ltorg
.global __wrap_ov5_021F10FC
.type __wrap_ov5_021F10FC,%function
__wrap_ov5_021F10FC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1325
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F10FC
 .ltorg
.global __wrap_ov5_021F121C
.type __wrap_ov5_021F121C,%function
__wrap_ov5_021F121C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1326
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F121C
 .ltorg
.global __wrap_ov5_021F134C
.type __wrap_ov5_021F134C,%function
__wrap_ov5_021F134C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1327
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F134C
 .ltorg
.global __wrap_ov5_021F1370
.type __wrap_ov5_021F1370,%function
__wrap_ov5_021F1370:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1328
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F1370
 .ltorg
.global __wrap_ov5_021F17B8
.type __wrap_ov5_021F17B8,%function
__wrap_ov5_021F17B8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1329
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F17B8
 .ltorg
.global __wrap_ov5_021F17D4
.type __wrap_ov5_021F17D4,%function
__wrap_ov5_021F17D4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1330
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F17D4
 .ltorg
.global __wrap_ov5_021F1BA4
.type __wrap_ov5_021F1BA4,%function
__wrap_ov5_021F1BA4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1331
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F1BA4
 .ltorg
.global __wrap_ov5_021F1CC8
.type __wrap_ov5_021F1CC8,%function
__wrap_ov5_021F1CC8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1332
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F1CC8
 .ltorg
.global __wrap_ov5_021F1CE8
.type __wrap_ov5_021F1CE8,%function
__wrap_ov5_021F1CE8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1333
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F1CE8
 .ltorg
.global __wrap_ov5_021F23D0
.type __wrap_ov5_021F23D0,%function
__wrap_ov5_021F23D0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1334
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F23D0
 .ltorg
.global __wrap_ov5_021F23EC
.type __wrap_ov5_021F23EC,%function
__wrap_ov5_021F23EC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1335
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F23EC
 .ltorg
.global __wrap_ov5_021F2438
.type __wrap_ov5_021F2438,%function
__wrap_ov5_021F2438:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1336
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F2438
 .ltorg
.global __wrap_ov5_021F247C
.type __wrap_ov5_021F247C,%function
__wrap_ov5_021F247C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1337
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F247C
 .ltorg
.global __wrap_ov5_021F2850
.type __wrap_ov5_021F2850,%function
__wrap_ov5_021F2850:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1338
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F2850
 .ltorg
.global __wrap_ov5_021F2864
.type __wrap_ov5_021F2864,%function
__wrap_ov5_021F2864:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1339
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F2864
 .ltorg
.global __wrap_ov5_021F28F4
.type __wrap_ov5_021F28F4,%function
__wrap_ov5_021F28F4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1340
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F28F4
 .ltorg
.global __wrap_ov5_021F2974
.type __wrap_ov5_021F2974,%function
__wrap_ov5_021F2974:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1341
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F2974
 .ltorg
.global __wrap_ov5_021F2A7C
.type __wrap_ov5_021F2A7C,%function
__wrap_ov5_021F2A7C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1342
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F2A7C
 .ltorg
.global __wrap_ov5_021F2A98
.type __wrap_ov5_021F2A98,%function
__wrap_ov5_021F2A98:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1343
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F2A98
 .ltorg
.global __wrap_ov5_021F2BD0
.type __wrap_ov5_021F2BD0,%function
__wrap_ov5_021F2BD0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1344
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F2BD0
 .ltorg
.global __wrap_ov5_021F2BEC
.type __wrap_ov5_021F2BEC,%function
__wrap_ov5_021F2BEC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1345
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F2BEC
 .ltorg
.global __wrap_ov5_021F2D20
.type __wrap_ov5_021F2D20,%function
__wrap_ov5_021F2D20:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1346
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F2D20
 .ltorg
.global __wrap_ov5_021F2D3C
.type __wrap_ov5_021F2D3C,%function
__wrap_ov5_021F2D3C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1347
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F2D3C
 .ltorg
.global __wrap_ov5_021F3284
.type __wrap_ov5_021F3284,%function
__wrap_ov5_021F3284:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1348
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F3284
 .ltorg
.global __wrap_ov5_021F32A0
.type __wrap_ov5_021F32A0,%function
__wrap_ov5_021F32A0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1349
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F32A0
 .ltorg
.global __wrap_ov5_021F348C
.type __wrap_ov5_021F348C,%function
__wrap_ov5_021F348C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1350
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F348C
 .ltorg
.global __wrap_ov5_021F34A8
.type __wrap_ov5_021F34A8,%function
__wrap_ov5_021F34A8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1351
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F34A8
 .ltorg
.global __wrap_ov5_021F37A8
.type __wrap_ov5_021F37A8,%function
__wrap_ov5_021F37A8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1352
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F37A8
 .ltorg
.global __wrap_ov5_021F37C4
.type __wrap_ov5_021F37C4,%function
__wrap_ov5_021F37C4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1353
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F37C4
 .ltorg
.global __wrap_ov5_021F3A50
.type __wrap_ov5_021F3A50,%function
__wrap_ov5_021F3A50:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1354
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F3A50
 .ltorg
.global __wrap_ov5_021F3A6C
.type __wrap_ov5_021F3A6C,%function
__wrap_ov5_021F3A6C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1355
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F3A6C
 .ltorg
.global __wrap_ov5_021F3D00
.type __wrap_ov5_021F3D00,%function
__wrap_ov5_021F3D00:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1356
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F3D00
 .ltorg
.global __wrap_ov5_021F3D1C
.type __wrap_ov5_021F3D1C,%function
__wrap_ov5_021F3D1C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1357
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F3D1C
 .ltorg
.global __wrap_ov5_021F3E74
.type __wrap_ov5_021F3E74,%function
__wrap_ov5_021F3E74:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1358
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F3E74
 .ltorg
.global __wrap_ov5_021F3E90
.type __wrap_ov5_021F3E90,%function
__wrap_ov5_021F3E90:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1359
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F3E90
 .ltorg
.global __wrap_ov5_021F4018
.type __wrap_ov5_021F4018,%function
__wrap_ov5_021F4018:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1360
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F4018
 .ltorg
.global __wrap_ov5_021F4034
.type __wrap_ov5_021F4034,%function
__wrap_ov5_021F4034:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1361
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F4034
 .ltorg
.global __wrap_ov5_021F47B0
.type __wrap_ov5_021F47B0,%function
__wrap_ov5_021F47B0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1362
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F47B0
 .ltorg
.global __wrap_ov5_021F47CC
.type __wrap_ov5_021F47CC,%function
__wrap_ov5_021F47CC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1363
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F47CC
 .ltorg
.global __wrap_ov5_021F4DAC
.type __wrap_ov5_021F4DAC,%function
__wrap_ov5_021F4DAC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1364
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F4DAC
 .ltorg
.global __wrap_ov5_021F4DC8
.type __wrap_ov5_021F4DC8,%function
__wrap_ov5_021F4DC8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1365
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F4DC8
 .ltorg
.global __wrap_ov5_021F4E08
.type __wrap_ov5_021F4E08,%function
__wrap_ov5_021F4E08:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1366
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F4E08
 .ltorg
.global __wrap_ov5_021F4E28
.type __wrap_ov5_021F4E28,%function
__wrap_ov5_021F4E28:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1367
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F4E28
 .ltorg
.global __wrap_ov5_021F5060
.type __wrap_ov5_021F5060,%function
__wrap_ov5_021F5060:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1368
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F5060
 .ltorg
.global __wrap_ov5_021F507C
.type __wrap_ov5_021F507C,%function
__wrap_ov5_021F507C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1369
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F507C
 .ltorg
.global __wrap_ov5_021F50BC
.type __wrap_ov5_021F50BC,%function
__wrap_ov5_021F50BC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1370
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F50BC
 .ltorg
.global __wrap_ov5_021F50D8
.type __wrap_ov5_021F50D8,%function
__wrap_ov5_021F50D8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1371
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F50D8
 .ltorg
.global __wrap_ov5_021F5284
.type __wrap_ov5_021F5284,%function
__wrap_ov5_021F5284:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1372
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F5284
 .ltorg
.global __wrap_ov5_021F52A0
.type __wrap_ov5_021F52A0,%function
__wrap_ov5_021F52A0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1373
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F52A0
 .ltorg
.global __wrap_ov5_021F5428
.type __wrap_ov5_021F5428,%function
__wrap_ov5_021F5428:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1374
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F5428
 .ltorg
.global __wrap_ov5_021F5444
.type __wrap_ov5_021F5444,%function
__wrap_ov5_021F5444:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1375
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F5444
 .ltorg
.global __wrap_ov5_021F55CC
.type __wrap_ov5_021F55CC,%function
__wrap_ov5_021F55CC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1376
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F55CC
 .ltorg
.global __wrap_ov5_021F55E8
.type __wrap_ov5_021F55E8,%function
__wrap_ov5_021F55E8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1377
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F55E8
 .ltorg
.global __wrap_ov5_021F575C
.type __wrap_ov5_021F575C,%function
__wrap_ov5_021F575C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1378
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F575C
 .ltorg
.global __wrap_ov5_021F5778
.type __wrap_ov5_021F5778,%function
__wrap_ov5_021F5778:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1379
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F5778
 .ltorg
.global __wrap_ov5_021F5894
.type __wrap_ov5_021F5894,%function
__wrap_ov5_021F5894:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1380
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F5894
 .ltorg
.global __wrap_ov5_021F58B0
.type __wrap_ov5_021F58B0,%function
__wrap_ov5_021F58B0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1381
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F58B0
 .ltorg
.global __wrap_ov5_021F5A10
.type __wrap_ov5_021F5A10,%function
__wrap_ov5_021F5A10:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1382
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F5A10
 .ltorg
.global __wrap_ov5_021F5A24
.type __wrap_ov5_021F5A24,%function
__wrap_ov5_021F5A24:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1383
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F5A24
 .ltorg
.global __wrap_ov5_021F5C58
.type __wrap_ov5_021F5C58,%function
__wrap_ov5_021F5C58:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1384
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F5C58
 .ltorg
.global __wrap_ov5_021F5D8C
.type __wrap_ov5_021F5D8C,%function
__wrap_ov5_021F5D8C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1385
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F5D8C
 .ltorg
.global __wrap_ov5_021F600C
.type __wrap_ov5_021F600C,%function
__wrap_ov5_021F600C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1386
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F600C
 .ltorg
.global __wrap_ov5_021F6028
.type __wrap_ov5_021F6028,%function
__wrap_ov5_021F6028:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1387
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F6028
 .ltorg
.global __wrap_ov5_021F61BC
.type __wrap_ov5_021F61BC,%function
__wrap_ov5_021F61BC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1388
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F61BC
 .ltorg
.global __wrap_ov5_021F61D8
.type __wrap_ov5_021F61D8,%function
__wrap_ov5_021F61D8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1389
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov5_021F61D8
 .ltorg
.global __wrap_ov6_022485F4
.type __wrap_ov6_022485F4,%function
__wrap_ov6_022485F4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1390
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov6_022485F4
 .ltorg
.global __wrap_ov6_02248608
.type __wrap_ov6_02248608,%function
__wrap_ov6_02248608:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1391
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov6_02248608
 .ltorg
.global __wrap_ov6_0224892C
.type __wrap_ov6_0224892C,%function
__wrap_ov6_0224892C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1392
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov6_0224892C
 .ltorg
.global __wrap_ov6_02248940
.type __wrap_ov6_02248940,%function
__wrap_ov6_02248940:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1393
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_ov6_02248940
 .ltorg
.global __wrap_sprintf
.type __wrap_sprintf,%function
__wrap_sprintf:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1394
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sprintf
 .ltorg
.global __wrap_sub_0202AED8
.type __wrap_sub_0202AED8,%function
__wrap_sub_0202AED8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1395
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_0202AED8
 .ltorg
.global __wrap_sub_0202AEF0
.type __wrap_sub_0202AEF0,%function
__wrap_sub_0202AEF0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1396
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_0202AEF0
 .ltorg
.global __wrap_sub_0202AFD4
.type __wrap_sub_0202AFD4,%function
__wrap_sub_0202AFD4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1397
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_0202AFD4
 .ltorg
.global __wrap_sub_020363A0
.type __wrap_sub_020363A0,%function
__wrap_sub_020363A0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1398
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_020363A0
 .ltorg
.global __wrap_sub_020391DC
.type __wrap_sub_020391DC,%function
__wrap_sub_020391DC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1399
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_020391DC
 .ltorg
.global __wrap_sub_02039298
.type __wrap_sub_02039298,%function
__wrap_sub_02039298:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1400
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02039298
 .ltorg
.global __wrap_sub_0203A418
.type __wrap_sub_0203A418,%function
__wrap_sub_0203A418:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1401
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_0203A418
 .ltorg
.global __wrap_sub_0203CA40
.type __wrap_sub_0203CA40,%function
__wrap_sub_0203CA40:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1402
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_0203CA40
 .ltorg
.global __wrap_sub_0203CBE0
.type __wrap_sub_0203CBE0,%function
__wrap_sub_0203CBE0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1403
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_0203CBE0
 .ltorg
.global __wrap_sub_0203CC14
.type __wrap_sub_0203CC14,%function
__wrap_sub_0203CC14:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1404
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_0203CC14
 .ltorg
.global __wrap_sub_020545EC
.type __wrap_sub_020545EC,%function
__wrap_sub_020545EC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1405
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_020545EC
 .ltorg
.global __wrap_sub_020559DC
.type __wrap_sub_020559DC,%function
__wrap_sub_020559DC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1406
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_020559DC
 .ltorg
.global __wrap_sub_02056B30
.type __wrap_sub_02056B30,%function
__wrap_sub_02056B30:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1407
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02056B30
 .ltorg
.global __wrap_sub_02056BDC
.type __wrap_sub_02056BDC,%function
__wrap_sub_02056BDC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1408
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02056BDC
 .ltorg
.global __wrap_sub_02056C18
.type __wrap_sub_02056C18,%function
__wrap_sub_02056C18:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1409
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02056C18
 .ltorg
.global __wrap_sub_0205B2D4
.type __wrap_sub_0205B2D4,%function
__wrap_sub_0205B2D4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1410
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_0205B2D4
 .ltorg
.global __wrap_sub_0205BEA8
.type __wrap_sub_0205BEA8,%function
__wrap_sub_0205BEA8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1411
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_0205BEA8
 .ltorg
.global __wrap_sub_0206184C
.type __wrap_sub_0206184C,%function
__wrap_sub_0206184C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1412
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_0206184C
 .ltorg
.global __wrap_sub_02061AB4
.type __wrap_sub_02061AB4,%function
__wrap_sub_02061AB4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1413
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02061AB4
 .ltorg
.global __wrap_sub_02061B48
.type __wrap_sub_02061B48,%function
__wrap_sub_02061B48:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1414
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02061B48
 .ltorg
.global __wrap_sub_02061BF0
.type __wrap_sub_02061BF0,%function
__wrap_sub_02061BF0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1415
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02061BF0
 .ltorg
.global __wrap_sub_02061C48
.type __wrap_sub_02061C48,%function
__wrap_sub_02061C48:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1416
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02061C48
 .ltorg
.global __wrap_sub_02062764
.type __wrap_sub_02062764,%function
__wrap_sub_02062764:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1417
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02062764
 .ltorg
.global __wrap_sub_020627B4
.type __wrap_sub_020627B4,%function
__wrap_sub_020627B4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1418
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_020627B4
 .ltorg
.global __wrap_sub_0206285C
.type __wrap_sub_0206285C,%function
__wrap_sub_0206285C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1419
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_0206285C
 .ltorg
.global __wrap_sub_02062880
.type __wrap_sub_02062880,%function
__wrap_sub_02062880:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1420
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02062880
 .ltorg
.global __wrap_sub_02062A0C
.type __wrap_sub_02062A0C,%function
__wrap_sub_02062A0C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1421
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02062A0C
 .ltorg
.global __wrap_sub_02062A14
.type __wrap_sub_02062A14,%function
__wrap_sub_02062A14:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1422
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02062A14
 .ltorg
.global __wrap_sub_02062ACC
.type __wrap_sub_02062ACC,%function
__wrap_sub_02062ACC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1423
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02062ACC
 .ltorg
.global __wrap_sub_02062AF0
.type __wrap_sub_02062AF0,%function
__wrap_sub_02062AF0:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1424
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02062AF0
 .ltorg
.global __wrap_sub_02062B68
.type __wrap_sub_02062B68,%function
__wrap_sub_02062B68:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1425
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02062B68
 .ltorg
.global __wrap_sub_02062BA4
.type __wrap_sub_02062BA4,%function
__wrap_sub_02062BA4:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1426
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02062BA4
 .ltorg
.global __wrap_sub_02062C18
.type __wrap_sub_02062C18,%function
__wrap_sub_02062C18:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1427
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02062C18
 .ltorg
.global __wrap_sub_02062C3C
.type __wrap_sub_02062C3C,%function
__wrap_sub_02062C3C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1428
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02062C3C
 .ltorg
.global __wrap_sub_02062CF8
.type __wrap_sub_02062CF8,%function
__wrap_sub_02062CF8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1429
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02062CF8
 .ltorg
.global __wrap_sub_02062D4C
.type __wrap_sub_02062D4C,%function
__wrap_sub_02062D4C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1430
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02062D4C
 .ltorg
.global __wrap_sub_02062DFC
.type __wrap_sub_02062DFC,%function
__wrap_sub_02062DFC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1431
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02062DFC
 .ltorg
.global __wrap_sub_02062E94
.type __wrap_sub_02062E94,%function
__wrap_sub_02062E94:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1432
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02062E94
 .ltorg
.global __wrap_sub_02062EC8
.type __wrap_sub_02062EC8,%function
__wrap_sub_02062EC8:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1433
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02062EC8
 .ltorg
.global __wrap_sub_02062F64
.type __wrap_sub_02062F64,%function
__wrap_sub_02062F64:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1434
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02062F64
 .ltorg
.global __wrap_sub_02062F7C
.type __wrap_sub_02062F7C,%function
__wrap_sub_02062F7C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1435
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02062F7C
 .ltorg
.global __wrap_sub_0206326C
.type __wrap_sub_0206326C,%function
__wrap_sub_0206326C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1436
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_0206326C
 .ltorg
.global __wrap_sub_02063E18
.type __wrap_sub_02063E18,%function
__wrap_sub_02063E18:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1437
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02063E18
 .ltorg
.global __wrap_sub_02063EBC
.type __wrap_sub_02063EBC,%function
__wrap_sub_02063EBC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1438
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02063EBC
 .ltorg
.global __wrap_sub_020656AC
.type __wrap_sub_020656AC,%function
__wrap_sub_020656AC:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1439
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_020656AC
 .ltorg
.global __wrap_sub_02069238
.type __wrap_sub_02069238,%function
__wrap_sub_02069238:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1440
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_02069238
 .ltorg
.global __wrap_sub_0206A218
.type __wrap_sub_0206A218,%function
__wrap_sub_0206A218:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1441
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_0206A218
 .ltorg
.global __wrap_sub_0206DC6C
.type __wrap_sub_0206DC6C,%function
__wrap_sub_0206DC6C:
 push {r0-r4,lr}
 vpush {d0-d7}
 ldr r0,=.Ltag1442
 mov r1,lr
 bl P3DS_Crumb
 vpop {d0-d7}
 pop {r0-r4,lr}
 b __real_sub_0206DC6C
 .ltorg
