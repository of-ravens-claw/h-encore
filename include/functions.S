/* functions.S -- imported functions
 *
 * Copyright (C) 2018 TheFloW, 2026 of-ravens-claw
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

// SceJpegUser
.equ sceJpegDecodeMJpegYCbCr, 0x810230e8
.equ sceJpegGetOutputInfo, 0x810230f8
.equ sceJpegInitMJpegWithParam, 0x81023108
.equ sceJpegCsc, 0x81023118
.equ sceJpegFinishMJpeg, 0x81023128
.equ sceJpegMJpegCsc, 0x81023138

// SceJpegEncUser
.equ sceJpegEncoderSetOutputAddr, 0x81023148
.equ sceJpegEncoderGetContextSize, 0x81023158
.equ sceJpegEncoderInitWithParam, 0x81023168
.equ sceJpegEncoderCsc, 0x81023178
.equ sceJpegEncoderSetValidRegion, 0x81023188
.equ sceJpegEncoderSetCompressionRatio, 0x81023198
.equ sceJpegEncoderEncode, 0x810231a8
.equ sceJpegEncoderEnd, 0x810231b8

// SceGxm
.equ sceGxmMapFragmentUsseMemory, 0x810231c8
.equ sceGxmShaderPatcherCreate, 0x810231d8
.equ sceGxmFinish, 0x810231e8
.equ sceGxmUnmapVertexUsseMemory, 0x810231f8
.equ sceGxmDestroyRenderTarget, 0x81023208
.equ sceGxmTextureSetVAddrMode, 0x81023218
.equ sceGxmCreateRenderTarget, 0x81023228
.equ sceGxmProgramFindParameterByName, 0x81023238
.equ sceGxmSetFragmentTexture, 0x81023248
.equ sceGxmShaderPatcherRegisterProgram, 0x81023258
.equ sceGxmSetVertexProgram, 0x81023268
.equ sceGxmPadHeartbeat, 0x81023278
.equ sceGxmTextureSetMinFilter, 0x81023288
.equ sceGxmTextureSetUAddrMode, 0x81023298
.equ sceGxmTransferFinish, 0x810232a8
.equ sceGxmTextureInitLinear, 0x810232b8
.equ sceGxmShaderPatcherCreateFragmentProgram, 0x810232c8
.equ sceGxmTextureGetData, 0x810232d8
.equ sceGxmProgramParameterGetResourceIndex, 0x810232e8
.equ sceGxmTransferCopy, 0x810232f8
.equ sceGxmSetUniformDataF, 0x81023308
.equ sceGxmTextureInitLinearStrided, 0x81023318
.equ sceGxmTransferFill, 0x81023328
.equ sceGxmSyncObjectCreate, 0x81023338
.equ sceGxmReserveFragmentDefaultUniformBuffer, 0x81023348
.equ sceGxmUnmapFragmentUsseMemory, 0x81023358
.equ sceGxmUnmapMemory, 0x81023368
.equ sceGxmBeginScene, 0x81023378
.equ sceGxmSyncObjectDestroy, 0x81023388
.equ sceGxmSetVertexStream, 0x81023398
.equ sceGxmShaderPatcherGetProgramFromId, 0x810233a8
.equ sceGxmShaderPatcherReleaseVertexProgram, 0x810233b8
.equ sceGxmSetFragmentProgram, 0x810233c8
.equ sceGxmInitialize, 0x810233d8
.equ sceGxmTerminate, 0x810233e8
.equ sceGxmShaderPatcherCreateVertexProgram, 0x810233f8
.equ sceGxmDisplayQueueFinish, 0x81023408
.equ sceGxmDraw, 0x81023418
.equ sceGxmShaderPatcherReleaseFragmentProgram, 0x81023428
.equ sceGxmMapMemory, 0x81023438
.equ sceGxmDepthStencilSurfaceInit, 0x81023448
.equ sceGxmTransferDownscale, 0x81023458
.equ sceGxmCreateContext, 0x81023468
.equ sceGxmShaderPatcherDestroy, 0x81023478
.equ sceGxmDisplayQueueAddEntry, 0x81023488
.equ sceGxmColorSurfaceInit, 0x81023498
.equ sceGxmDestroyContext, 0x810234a8
.equ sceGxmShaderPatcherUnregisterProgram, 0x810234b8
.equ sceGxmMapVertexUsseMemory, 0x810234c8
.equ sceGxmTextureSetMagFilter, 0x810234d8
.equ sceGxmEndScene, 0x810234e8

// SceDisplayUser
.equ sceDisplaySetFrameBuf, 0x810234f8

// SceDisplay
.equ sceDisplayWaitSetFrameBufMulti, 0x81023508
.equ sceDisplayWaitSetFrameBuf, 0x81023518

// SceCtrl
.equ sceCtrlReadBufferPositive, 0x81023528
.equ sceCtrlSetSamplingMode, 0x81023538

// SceAtrac
.equ sceAtracReleaseHandle, 0x81023548
.equ sceAtracCreateDecoderGroup, 0x81023558
.equ sceAtracSetOutputSamples, 0x81023568
.equ sceAtracQueryDecoderGroupMemSize, 0x81023578
.equ sceAtracSetSubBuffer, 0x81023588
.equ sceAtracDecode, 0x81023598
.equ sceAtracAddStreamData, 0x810235a8
.equ sceAtracSetLoopNum, 0x810235b8
.equ sceAtracGetContentInfo, 0x810235c8
.equ sceAtracDeleteDecoderGroup, 0x810235d8
.equ sceAtracIsSubBufferNeeded, 0x810235e8
.equ sceAtracSetDataAndAcquireHandle, 0x810235f8
.equ sceAtracGetStreamInfo, 0x81023608
.equ sceAtracGetSubBufferInfo, 0x81023618

// SceAudio
.equ sceAudioOutOutput, 0x81023628
.equ sceAudioOutOpenPort, 0x81023638
.equ sceAudioOutSetVolume, 0x81023648
.equ sceAudioOutReleasePort, 0x81023658

// SceSysmodule
.equ sceSysmoduleUnloadModule, 0x81023668
.equ sceSysmoduleLoadModule, 0x81023678

// SceRtcUser
.equ sceRtcGetCurrentClockLocalTime, 0x81023688

// ScePvf
.equ scePvfFindOptimumFont, 0x81023698
.equ scePvfSetSkewValue, 0x810236a8
.equ scePvfSetEmboldenRate, 0x810236b8
.equ scePvfNewLib, 0x810236c8
.equ scePvfGetCharGlyphImage_Clip, 0x810236d8
.equ scePvfGetCharInfo, 0x810236e8
.equ scePvfGetFontInfo, 0x810236f8
.equ scePvfSetResolution, 0x81023708
.equ scePvfClose, 0x81023718
.equ scePvfSetEM, 0x81023728
.equ scePvfDoneLib, 0x81023738
.equ scePvfOpen, 0x81023748
.equ scePvfSetCharSize, 0x81023758

// SceTouch
.equ sceTouchGetPanelInfo, 0x81023768
.equ sceTouchRead, 0x81023778
.equ sceTouchSetSamplingState, 0x81023788
.equ sceTouchEnableTouchForce, 0x81023798

// SceAppUtil
.equ sceAppUtilSystemParamGetInt, 0x810237a8
.equ sceAppUtilSaveDataDataSave, 0x810237b8
.equ sceAppUtilSaveDataSlotGetParam, 0x810237c8
.equ sceAppUtilShutdown, 0x810237d8
.equ sceAppUtilSaveDataDataRemove, 0x810237e8
.equ sceAppUtilInit, 0x810237f8

// SceCommonDialog
.equ sceSaveDataDialogContinue, 0x81023808
.equ sceSaveDataDialogTerm, 0x81023818
.equ sceMsgDialogGetStatus, 0x81023828
.equ sceSaveDataDialogSubClose, 0x81023838
.equ sceSaveDataDialogFinish, 0x81023848
.equ sceSaveDataDialogGetStatus, 0x81023858
.equ sceMsgDialogInit, 0x81023868
.equ sceMsgDialogTerm, 0x81023878
.equ sceCommonDialogUpdate, 0x81023888
.equ sceNpTrophySetupDialogInit, 0x81023898
.equ sceNpTrophySetupDialogTerm, 0x810238a8
.equ sceSaveDataDialogGetResult, 0x810238b8
.equ sceSaveDataDialogGetSubStatus, 0x810238c8
.equ sceMsgDialogGetResult, 0x810238d8
.equ sceCommonDialogSetConfigParam, 0x810238e8
.equ sceSaveDataDialogInit, 0x810238f8
.equ sceMsgDialogClose, 0x81023908
.equ sceNpTrophySetupDialogGetStatus, 0x81023918
.equ sceNpTrophySetupDialogGetResult, 0x81023928

// SceNpTrophy
.equ sceNpTrophyInit, 0x81023938
.equ sceNpTrophyCreateHandle, 0x81023948
.equ sceNpTrophyDestroyContext, 0x81023958
.equ sceNpTrophyUnlockTrophy, 0x81023968
.equ sceNpTrophyGetGameInfo, 0x81023978
.equ sceNpTrophyTerm, 0x81023988
.equ sceNpTrophyCreateContext, 0x81023998
.equ sceNpTrophyGetTrophyUnlockState, 0x810239a8
.equ sceNpTrophyAbortHandle, 0x810239b8
.equ sceNpTrophyDestroyHandle, 0x810239c8

// SceNpManager
.equ sceNpInit, 0x810239d8
.equ sceNpTerm, 0x810239e8

// SceLibc
.equ strstr, 0x810239f8
.equ abort, 0x81023a08
.equ __cxa_atexit, 0x81023a18
.equ _sceLdTlsRegisterModuleInfo, 0x81023a28
.equ free, 0x81023a38
.equ memset, 0x81023a48
.equ memcpy, 0x81023a58
.equ malloc, 0x81023a68
.equ exit, 0x81023a78
.equ fgetwc, 0x81023a88
.equ strlen, 0x81023a98
.equ abs, 0x81023aa8
.equ fgetws, 0x81023ab8
.equ printf, 0x81023ac8
.equ strncpy, 0x81023ad8
.equ snprintf, 0x81023ae8
.equ wcslen, 0x81023af8
.equ memalign, 0x81023b08
.equ __cxa_set_dso_handle_main, 0x81023b18
.equ fclose, 0x81023b28
.equ fopen, 0x81023b38

// SceLibstdcxx
.equ _ZdlPv, 0x81023b48
.equ _Znwj, 0x81023b58

// SceLibKernel
.equ sceKernelUnlockLwMutex, 0x81023b68
.equ sceClibMemcpy, 0x81023b78
.equ sceKernelDeleteLwMutex, 0x81023b88
.equ __sce_aeabi_idiv0, 0x81023b98
.equ sceKernelLockLwMutex, 0x81023ba8
.equ sceKernelGetProcessTime, 0x81023bb8
.equ sceClibMemset, 0x81023bc8
.equ sceIoOpen, 0x81023bd8
.equ sceClibMemmove, 0x81023be8
.equ sceKernelCreateEventFlag, 0x81023bf8
.equ sceIoLseek, 0x81023c08
.equ sceIoGetstat, 0x81023c18
.equ sceKernelCreateThread, 0x81023c28
.equ sceKernelGetThreadExitStatus, 0x81023c38
.equ sceKernelCreateLwMutex, 0x81023c48
.equ sceKernelWaitThreadEnd, 0x81023c58
.equ sceKernelStartThread, 0x81023c68
.equ __sce_aeabi_ldiv0, 0x81023c78

// SceIofilemgr
.equ sceIoLseek32, 0x81023c88
.equ sceIoClose, 0x81023c98
.equ sceIoRead, 0x81023ca8

// SceProcessmgr
.equ sceKernelPowerTick, 0x81023cb8

// SceSysmem
.equ sceKernelFreeMemBlock, 0x81023cc8
.equ sceKernelGetMemBlockBase, 0x81023cd8
.equ sceKernelAllocMemBlock, 0x81023ce8

// SceThreadmgr
.equ sceKernelDeleteThread, 0x81023cf8
.equ sceKernelExitDeleteThread, 0x81023d08
.equ sceKernelDelayThread, 0x81023d18
.equ sceKernelClearEventFlag, 0x81023d28
.equ sceKernelDeleteEventFlag, 0x81023d38
.equ sceKernelSetEventFlag, 0x81023d48

// SceThreadmgrCoredumpTime
.equ sceKernelExitThread, 0x81023d58
