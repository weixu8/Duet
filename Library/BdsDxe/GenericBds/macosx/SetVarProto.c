/*
 * Copyright (C) 2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
/*
 Slice 2011, 
 some more adoptations for Apple's OS
 */

/*******************************************************************************
 *   Header Files                                                               *
 *******************************************************************************/
#include "macosx.h"

#define DEVICE_PROPERTIES_SIGNATURE SIGNATURE_64('A','P','P','L','E','D','E','V')
EFI_GUID gDevicePropertiesGuid                  = {0x91BD12FE, 0xF6C3, 0x44FB, {0xA5, 0xB7, 0x51, 0x22, 0xAB, 0x30, 0x3A, 0xE0}};
EFI_GUID gAppleScreenInfoGuid                   = {0xe316e100, 0x0751, 0x4c49, {0x90, 0x56, 0x48, 0x6c, 0x7e, 0x47, 0x29, 0x03}};

typedef struct _APPLE_GETVAR_PROTOCOL APPLE_GETVAR_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *APPLE_GETVAR_PROTOCOL_GET_DEVICE_PROPS) (
  IN     APPLE_GETVAR_PROTOCOL   *This,
  IN     CHAR8                   *Buffer,
  IN OUT UINT32                  *BufferSize
  );

struct _APPLE_GETVAR_PROTOCOL {
  UINT64    Sign;
  EFI_STATUS(EFIAPI *Unknown1)(IN VOID *);
  EFI_STATUS(EFIAPI *Unknown2)(IN VOID *);
  EFI_STATUS(EFIAPI *Unknown3)(IN VOID *);
  APPLE_GETVAR_PROTOCOL_GET_DEVICE_PROPS  GetDevProps;
  APPLE_GETVAR_PROTOCOL_GET_DEVICE_PROPS  GetDevProps1;
};

CHAR8                           *gDevProp = NULL;
CHAR8                           *cDevProp = NULL;


EFI_STATUS EFIAPI
GetDeviceProps(IN     APPLE_GETVAR_PROTOCOL   *This,
               IN     CHAR8                   *Buffer,
               IN OUT UINT32                  *BufferSize)
{ 
    UINT32		cnt = 0;
	UINT8     *binStr = NULL;
 // EFI_STATUS    Status;

  // iCloud
  //Status = gRT->SetVariable(L"ROM", &gEfiAppleNvramGuid, 0, 0, NULL);
  
    if ((cDevProp!=NULL) && AsciiStrLen(cDevProp)>3)
	{
        cnt = (UINT32)AsciiStrLen(cDevProp) / 2;
		binStr = AllocateZeroPool(cnt);
        if(hex2bin(cDevProp, binStr, cnt))
        {
          *BufferSize = cnt;    
          CopyMem(Buffer, binStr,  cnt);
          return EFI_SUCCESS;      
        }
    }
    else if ((gDevProp!=NULL) && AsciiStrLen(gDevProp)>3)
    {
      cnt = (UINT32)AsciiStrLen(gDevProp) / 2;
      binStr = AllocateZeroPool(cnt);
      if(hex2bin(gDevProp, binStr, cnt))
      {
          *BufferSize = cnt;
          CopyMem(Buffer, binStr,  cnt);
          return EFI_SUCCESS;
      }
    }
    *BufferSize = 0;
	return EFI_SUCCESS;
}

APPLE_GETVAR_PROTOCOL mDeviceProperties=
{
	DEVICE_PROPERTIES_SIGNATURE,
	NULL,
	NULL,
	NULL,
	GetDeviceProps,   
    GetDeviceProps,
};

typedef	EFI_STATUS (EFIAPI *EFI_SCREEN_INFO_FUNCTION)(
  VOID* This, 
  UINT64* baseAddress,
  UINT64* frameBufferSize,
  UINT32* byterPerRow,
  UINT32* Width,
  UINT32* Height,
  UINT32* colorDepth
  );

typedef struct {	
	EFI_SCREEN_INFO_FUNCTION GetScreenInfo;	
} EFI_INTERFACE_SCREEN_INFO;

EFI_STATUS GetScreenInfo(VOID* This, UINT64* baseAddress, UINT64* frameBufferSize,
                         UINT32* bpr, UINT32* w, UINT32* h, UINT32* colorDepth)
{
	EFI_GRAPHICS_OUTPUT_PROTOCOL	*GraphicsOutput=NULL;
	EFI_STATUS						Status;
	
	Status = gBS->HandleProtocol (
              gST->ConsoleOutHandle,
              &gEfiGraphicsOutputProtocolGuid,
              (VOID **) &GraphicsOutput);
	if(EFI_ERROR(Status))		return EFI_UNSUPPORTED;
	Print(L"GetScreenInfo called with args: %lx %lx %lx %lx %lx %lx\n",
          baseAddress, frameBufferSize, bpr, w, h, colorDepth);
	*frameBufferSize = (UINT64)GraphicsOutput->Mode->FrameBufferSize;
	*baseAddress = (UINT64)GraphicsOutput->Mode->FrameBufferBase;
	*w = (UINT32)GraphicsOutput->Mode->Info->HorizontalResolution;
	*h = (UINT32)GraphicsOutput->Mode->Info->VerticalResolution;
	*colorDepth = 32;
	*bpr = (UINT32)(GraphicsOutput->Mode->Info->PixelsPerScanLine*32) >> 3;
	return EFI_SUCCESS;
}

EFI_INTERFACE_SCREEN_INFO mScreenInfo=
{
	GetScreenInfo
};

EFI_STATUS EFIAPI
SetPrivateVarProto(VOID)
{
  EFI_STATUS  Status;
	
  Status = gBS->InstallMultipleProtocolInterfaces (
                   &gImageHandle,
                   &gDevicePropertiesGuid,
                   &mDeviceProperties,
                   &gAppleScreenInfoGuid,
                   &mScreenInfo,
                   NULL
                   );
  return Status;
}
