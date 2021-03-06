/* $Id: Console.c $ */
/** @file
 * Console.c - VirtualBox Console control emulation
 */

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

#include "macosx.h"


CHAR8*	DefaultMemEntry = "N/A";
CHAR8*	DefaultSerial = "CT288GT9VT6";
CHAR8*  BiosVendor = "Apple Inc.";
CHAR8*  AppleManufacturer = "Apple Computer, Inc.";
extern  UINT64  TurboMsr;

CHAR8* AppleFirmwareVersion[24] =
{
	"MB11.88Z.0061.B03.0809221748",
	"MB21.88Z.00A5.B07.0706270922",
	"MB41.88Z.0073.B00.0809221748",
	"MB52.88Z.0088.B05.0809221748",
	"MBP51.88Z.007E.B06.0906151647",
    "MBP81.88Z.0047.B27.1102071707",
    "MBP83.88Z.0047.B24.1110261426",
    "MBP91.88Z.00D3.B08.1205101904",
	"MBA31.88Z.0061.B07.0712201139",
    "MBA51.88Z.00EF.B01.1205221442",
	"MM21.88Z.009A.B00.0706281359",
    "MM51.88Z.0077.B10.1102291410",
    "MM61.88Z.0106.B00.1208091121",
	"IM81.88Z.00C1.B00.0803051705",
	"IM101.88Z.00CC.B00.0909031926",
    "IM111.88Z.0034.B02.1003171314",
	"IM112.88Z.0057.B01.0802091538",
    "IM113.88Z.0057.B01.1005031455",
	"IM121.88Z.0047.B1F.1201241648",
    "IM122.88Z.0047.B1F.1223021110",
    "IM131.88Z.010A.B00.1209042338",
	"MP31.88Z.006C.B05.0802291410",
	"MP41.88Z.0081.B04.0903051113",
	"MP51.88Z.007F.B00.1008031144"
    
};

CHAR8* AppleBoardID[24] =
{
	"Mac-F4208CC8", //MB11 - yonah
	"Mac-F4208CA9",  //MB21 - merom 05/07
	"Mac-F22788A9",  //MB41 - penryn
	"Mac-F22788AA",  //MB52
	"Mac-F42D86C8",  //MBP51
    "Mac-94245B3640C91C81",  //MBP81 - i5 SB IntelHD3000
    "Mac-942459F5819B171B",  //MBP83 - i7 SB  ATI
    "Mac-6F01561E16C75D06",  //MBP92 - i5-3210M IvyBridge HD4000
	"Mac-942452F5819B1C1B",  //MBA31
    "Mac-2E6FAB96566FE58C",  //MBA52 - i5-3427U IVY BRIDGE IntelHD4000 did=166
	"Mac-F4208EAA",          //MM21 - merom GMA950 07/07
    "Mac-8ED6AF5B48C039E1",  //MM51 - Sandy + Intel 30000
    "Mac-F65AE981FFA204ED",  //MM62 - Ivy
	"Mac-F227BEC8",  //IM81 - merom 01/09
	"Mac-F2268CC8",  //IM101 - wolfdale? E7600 01/
    "Mac-F2268DAE",  //IM111 - Nehalem
	"Mac-F2238AC8",  //IM112 - Clarkdale
    "Mac-F2238BAE",  //IM113 - lynnfield
    "Mac-942B5BF58194151B",  //IM121 - i5-2500 - sandy
    "Mac-942B59F58194171B",  //IM122 - i7-2600
    "Mac-00BE6ED71E35EB86",  //IM131 - -i5-3470S -IVY
	"Mac-F2268DC8",  //MP31 - xeon quad 02/09 conroe
	"Mac-F4238CC8",  //MP41 - xeon wolfdale
	"Mac-F221BEC8"   //MP51 - Xeon Nehalem 4 cores    
};

CHAR8* AppleReleaseDate[24] =
{
	"09/22/08",  //mb11
	"06/27/07",
	"09/22/08",
	"01/21/09",
	"06/15/09",  //mbp51
    "02/07/11",
    "10/26/11",
    "05/10/2012", //MBP92
	"12/20/07",
    "05/22/2012", //mba52
	"08/07/07",  //mm21
    "02/29/11",  //MM51
    "08/09/2012", //MM62
	"03/05/08",
	"09/03/09",  //im101
	"03/17/10",
    "03/17/10",  //11,2
	"05/03/10",
    "01/24/12",  //121 120124
    "02/23/12",  //122
    "09/04/2012",  //131
	"02/29/08",
	"03/05/09",
	"08/03/10"    
};

CHAR8* AppleProductName[24] =
{
	"MacBook1,1",
	"MacBook2,1",
	"MacBook4,1",
	"MacBook5,2",
	"MacBookPro5,1",
    "MacBookPro8,1",
    "MacBookPro8,3",
    "MacBookPro9,2",
	"MacBookAir3,1",
    "MacBookAir5,2",
	"Macmini2,1",
    "Macmini5,1",
    "Macmini6,2",
	"iMac8,1",
	"iMac10,1",
    "iMac11,1",
	"iMac11,2",
    "iMac11,3",
	"iMac12,1",
    "iMac12,2",
    "iMac13,1",
	"MacPro3,1",
	"MacPro4,1",
	"MacPro5,1"    
};

CHAR8* AppleFamilies[24] =
{
	"MacBook",
	"MacBook",
	"MacBook",
	"MacBook",
	"MacBookPro",
    "MacBookPro",
    "MacBookPro",
    "MacBook Pro",
	"MacBookAir",
    "MacBook Air",
	"Macmini",
    "Mac mini",
    "Macmini",
	"iMac",
	"iMac",
	"iMac",
	"iMac",
	"iMac",
	"iMac",
    "iMac",
    "iMac",
	"MacPro",
	"MacPro",
	"MacPro"
    
};

CHAR8* AppleSystemVersion[24] =
{
	"1.1",
	"1.2",
	"1.3",
	"1.3",
	"1.0",
	"1.0",
    "1.0",
    "1.0",
	"1.0",
    "1.0",
    "1.1",
    "1.0", //MM51
    "1.0",
	"1.3",
	"1.0",
    "1.0",
	"1.2",
    "1.0",
	"1.9",
    "1.9",
    "1.0",
	"1.3",
	"1.4",
	"1.2"    
};

CHAR8* AppleSerialNumber[24] =
{
	"W80A041AU9B", //MB11
	"W88A041AWGP", //MB21 - merom 05/07
	"W88A041A0P0", //MB41
	"W88AAAAA9GU", //MB52
	"W88439FE1G0", //MBP51
    "W89F9196DH2G", //MBP81 - i5 SB IntelHD3000
    "W88F9CDEDF93", //MBP83 -i7 SB  ATI
    "C02HA041DTY3", //MBP92 - i5 IvyBridge HD4000
	"W8649476DQX",  //MBA31
    "C02HA041DRVC", //MBA52 - IvyBridge
	"W88A56BYYL2",  //MM21 - merom GMA950 07/07
    "C07GA041DJD0", //MM51 - sandy
    "C07JD041DWYN", //MM62 - IVY
	"W89A00AAX88", //IM81 - merom 01/09
	"W80AA98A5PE", //IM101 - wolfdale? E7600 01/09
    "G8942B1V5PJ", //IM111 - Nehalem
	"W8034342DB7", //IM112 - Clarkdale
    "QP0312PBDNR", //IM113 - lynnfield
	"W80CF65ADHJF", //IM121 - i5-2500 - sandy
    "W88GG136DHJQ", //IM122 -i7-2600
    "C02JA041DNCT", //IM131 -i5-3470S -IVY
	"W88A77AA5J4", //MP31 - xeon quad 02/09
	"CT93051DK9Y", //MP41
	"CG154TB9WU3"  //MP51 C07J50F7F4MC
};

CHAR8* AppleChassisAsset[24] =
{
	"MacBook-White",
	"MacBook-White",
	"MacBook-Black",
	"MacBook-Black",
	"MacBook-Aluminum",
	"MacBook-Aluminum",
	"MacBook-Aluminum",
    "MacBook-Aluminum",
	"Air-Enclosure",
    "Air-Enclosure",
	"Mini-Aluminum",
    "Mini-Aluminum",
    "Mini-Aluminum",
	"iMac-Aluminum",
	"iMac-Aluminum",
	"iMac-Aluminum",
	"iMac-Aluminum",
	"iMac-Aluminum",
	"iMac-Aluminum",
    "iMac-Aluminum",
    "iMac-Aluminum",
	"Pro-Enclosure",
	"Pro-Enclosure",
	"Pro-Enclosure"    
};

CHAR8* AppleBoardSN = "C02032101R5DC771H";
CHAR8* AppleBoardLocation = "Part Component";
CHAR8* RuLang = "ru:0";
//---------------------------------------------------------------------------------
/*VOID *GetDataSetting(IN TagPtr dict, IN CHAR8 *propName, OUT UINTN *dataLen)
{
    TagPtr  prop;
    UINT8   *data = NULL;
    UINT32   len;
    //UINTN   i;
    
    prop = GetProperty(dict, propName);
    if (prop) {
        if (prop->data != NULL && prop->dataLen > 0) {
            // data property
            data = AllocateZeroPool(prop->dataLen);
            CopyMem(data, prop->data, prop->dataLen);
            if (dataLen != NULL) {
                *dataLen = prop->dataLen;
            }
            //DBG("Data: %p, Len: %d = ", data, prop->dataLen);
            //for (i = 0; i < prop->dataLen; i++) DBG("%02x ", data[i]);
            //DBG("\n");
        } else {
            // assume data in hex encoded string property
            len = (UINT32)(AsciiStrLen(prop->string) >> 1); // 2 chars per byte
            data = AllocateZeroPool(len);
            len = hex2bin(prop->string, data, len);
            if (dataLen != NULL) {
                *dataLen = len;
            }
            //DBG("Data(str): %p, Len: %d = ", data, len);
            //for (i = 0; i < len; i++) DBG("%02x ", data[i]);
            //DBG("\n");
        }
    }
    return data;
}

*/

EFI_STATUS
StrToBuf (
          OUT UINT8    *Buf,
          IN  UINTN    BufferLength,
          IN  CHAR16   *Str
          )
{
    UINTN       Index;
    UINTN       StrLength;
    UINT8       Digit;
    UINT8       Byte;
    
    Digit = 0;
    
    //
    // Two hex char make up one byte
    //
    StrLength = BufferLength * sizeof (CHAR16);
    
    for(Index = 0; Index < StrLength; Index++, Str++) {
        
        if ((*Str >= L'a') && (*Str <= L'f')) {
            Digit = (UINT8) (*Str - L'a' + 0x0A);
        } else if ((*Str >= L'A') && (*Str <= L'F')) {
            Digit = (UINT8) (*Str - L'A' + 0x0A);
        } else if ((*Str >= L'0') && (*Str <= L'9')) {
            Digit = (UINT8) (*Str - L'0');
        } else {
            return EFI_INVALID_PARAMETER;
        }
        
        //
        // For odd characters, write the upper nibble for each buffer byte,
        // and for even characters, the lower nibble.
        //
        if ((Index & 1) == 0) {
            Byte = (UINT8) (Digit << 4);
        } else {
            Byte = Buf[Index / 2];
            Byte &= 0xF0;
            Byte = (UINT8) (Byte | Digit);
        }
        
        Buf[Index / 2] = Byte;
    }
    
    return EFI_SUCCESS;
}

/**
 Converts a string to GUID value.
 Guid Format is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
 
 @param Str              The registry format GUID string that contains the GUID value.
 @param Guid             A pointer to the converted GUID value.
 
 @retval EFI_SUCCESS     The GUID string was successfully converted to the GUID value.
 @retval EFI_UNSUPPORTED The input string is not in registry format.
 @return others          Some error occurred when converting part of GUID value.
 
 **/

EFI_STATUS
StrToGuidLE (
             IN  CHAR16   *Str,
             OUT EFI_GUID *Guid
             )
{
    UINT8 GuidLE[16];
    StrToBuf (&GuidLE[0], 4, Str);
	while (!IS_HYPHEN (*Str) && !IS_NULL (*Str)) {
		Str ++;
	}
	
	if (IS_HYPHEN (*Str)) {
		Str++;
	} else {
		return EFI_UNSUPPORTED;
	}
	
    StrToBuf (&GuidLE[4], 2, Str);
	while (!IS_HYPHEN (*Str) && !IS_NULL (*Str)) {
		Str ++;
	}
	
	if (IS_HYPHEN (*Str)) {
		Str++;
	} else {
		return EFI_UNSUPPORTED;
	}
	
    StrToBuf (&GuidLE[6], 2, Str);
	while (!IS_HYPHEN (*Str) && !IS_NULL (*Str)) {
		Str ++;
	}
	
	if (IS_HYPHEN (*Str)) {
		Str++;
	} else {
		return EFI_UNSUPPORTED;
	}
    
    StrToBuf (&GuidLE[8], 2, Str);
	while (!IS_HYPHEN (*Str) && !IS_NULL (*Str)) {
		Str ++;
	}
	
	if (IS_HYPHEN (*Str)) {
		Str++;
	} else {
		return EFI_UNSUPPORTED;
	}
    
    StrToBuf (&GuidLE[10], 6, Str);
    
    CopyMem((UINT8*)Guid, &GuidLE[0], 16);
    return EFI_SUCCESS;
}

VOID
GetDefaultSettings(
   VOID
   )
{
  MACHINE_TYPES   Model;
  
    Model             = GetDefaultModel();
    gSettings.CpuType	= GetAdvancedCpuType();
    gSettings.PMProfile = 0;
    gSettings.DefaultBoot[0] = 0;
    gSettings.BusSpeed = 0;
    gSettings.CpuFreqMHz = 0; //Hz ->MHz
    gSettings.ProcessorInterconnectSpeed = 0;
    AsciiStrCpy(gSettings.VendorName,             BiosVendor);
    AsciiStrCpy(gSettings.RomVersion,             AppleFirmwareVersion[Model]);
    AsciiStrCpy(gSettings.ReleaseDate,            AppleReleaseDate[Model]);
    AsciiStrCpy(gSettings.ManufactureName,        BiosVendor);
    AsciiStrCpy(gSettings.ProductName,            AppleProductName[Model]);
    AsciiStrCpy(gSettings.VersionNr,              AppleSystemVersion[Model]);
    AsciiStrCpy(gSettings.SerialNr,               AppleSerialNumber[Model]);
    AsciiStrCpy(gSettings.FamilyName,             AppleFamilies[Model]);	
    AsciiStrCpy(gSettings.BoardManufactureName,   BiosVendor);
    AsciiStrCpy(gSettings.BoardSerialNumber,      AppleBoardSN);
    AsciiStrCpy(gSettings.BoardNumber,            AppleBoardID[Model]);
    AsciiStrCpy(gSettings.BoardVersion,           AppleSystemVersion[Model]);
    AsciiStrCpy(gSettings.LocationInChassis,      AppleBoardLocation);
    AsciiStrCpy(gSettings.ChassisManufacturer,    BiosVendor);
    AsciiStrCpy(gSettings.ChassisAssetTag,        AppleChassisAsset[Model]);
  
}

UINT8 
Checksum8(
  VOID * startPtr, 
  UINT32 len
  )
{
	UINT8	Value = 0;
	UINT8	*ptr=(UINT8 *)startPtr;
	UINT32	i = 0;
	for(i = 0; i < len; i++)
		Value += *ptr++;
  
	return Value;
}

BOOLEAN 
IsHexDigit(
   CHAR8 c
   ) 
{
	return (IS_DIGIT(c) || (c>='A'&&c<='F') || (c>='a'&&c<='f'))?TRUE:FALSE;
}

UINT8 
hexstrtouint8(
    CHAR8* buf
    )
{
	INT8 i;
	if (IS_DIGIT(buf[0]))
		i = buf[0]-'0';
	else if (IS_HEX(buf[0])) 
    i = buf[0]-'a' + 10; 
  else
		i = buf[0]-'A' + 10;
  if (AsciiStrLen(buf) == 1) {
    return i;
  }
	i <<= 4;
	if (IS_DIGIT(buf[1]))
		i += buf[1]-'0';
	else if (IS_HEX(buf[1])) 
    i += buf[1]-'a' + 10; 
	else
		i += buf[1]-'A'+ 10; //no error checking
	return i;
}

BOOLEAN 
hex2bin(
  IN CHAR8 *hex, 
  OUT UINT8 *bin, 
  INT32 len
  )
{
	CHAR8	*p;
	INT32	i;
	CHAR8	buf[3];
	
	if (hex == NULL || bin == NULL || len <= 0 || AsciiStrLen(hex) != len * 2) {
		return FALSE;
	}
	
	buf[2] = '\0';
	p = (CHAR8 *) hex;
	
	for (i = 0; i < len; i++)
	{
		if (!IsHexDigit(p[0]) || !IsHexDigit(p[1])) {
			return -2;
		}
		buf[0] = *p++;
		buf[1] = *p++;
		bin[i] = hexstrtouint8(buf);
	}
	return TRUE;
}

static EFI_FILE_INFO *
EfiLibFileInfo (
  IN EFI_FILE_HANDLE      FHand
  )
{
  EFI_STATUS    Status;
  EFI_FILE_INFO *Buffer;
  UINTN         BufferSize;
  
  //
  // Initialize for GrowBuffer loop
  //
  Buffer      = NULL;
  BufferSize  = SIZE_OF_EFI_FILE_INFO + 200;
  
  //
  // Call the real function
  //
  while (EfiGrowBuffer (&Status, (VOID **) &Buffer, BufferSize)) {
    Status = FHand->GetInfo (
                       FHand,
                       &gEfiFileInfoGuid,
                       &BufferSize,
                       Buffer
                       );
  }
  
  return Buffer;
}

VOID 
Pause(
  IN CHAR16* Message
  )
{
  if (Message) {
    Print(L"%s", Message);
  }
  gBS->Stall(4000000);
}

BOOLEAN 
FileExists(
  IN EFI_FILE *RootFileHandle, 
  IN CHAR16   *RelativePath
  )
{
  EFI_STATUS  Status;
  EFI_FILE    *TestFile;
  
  Status = RootFileHandle->Open(RootFileHandle, &TestFile, RelativePath, EFI_FILE_MODE_READ, 0);
  if (Status == EFI_SUCCESS) {
    TestFile->Close(TestFile);
    return TRUE;
  }
  return FALSE;
}

EFI_STATUS 
egLoadFile(
  IN EFI_FILE_HANDLE BaseDir, 
  IN CHAR16 *FileName,
  OUT UINT8 **FileData, 
  OUT UINTN *FileDataLength
)
{
  EFI_STATUS          Status;
  EFI_FILE_HANDLE     FileHandle;
  EFI_FILE_INFO       *FileInfo;
  UINT64              ReadSize;
  UINTN               BufferSize;
  UINT8               *Buffer;
  
  Status = BaseDir->Open(BaseDir, &FileHandle, FileName, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR(Status))
    return Status;
  
  FileInfo = EfiLibFileInfo(FileHandle);
  if (FileInfo == NULL) {
    FileHandle->Close(FileHandle);
    return EFI_NOT_FOUND;
  }

  ReadSize = FileInfo->FileSize;
  if (ReadSize > MAX_FILE_SIZE) ReadSize = MAX_FILE_SIZE;
  FreePool(FileInfo);
  
  BufferSize = (UINTN)ReadSize;   // was limited to 1 GB above, so this is safe
  Buffer = (UINT8 *) AllocateAlignedPages (EFI_SIZE_TO_PAGES(BufferSize), 16);
  if (Buffer == NULL) {
    FileHandle->Close(FileHandle);
    return EFI_OUT_OF_RESOURCES;
  }
  
  Status = FileHandle->Read(FileHandle, &BufferSize, Buffer);
  FileHandle->Close(FileHandle);
  if (EFI_ERROR(Status)) {
    FreePool(Buffer);
    return Status;
  }
  
  *FileData = Buffer;
  *FileDataLength = BufferSize;
  return EFI_SUCCESS;
}

VOID 
WaitForSts(
  VOID
  ) 
{
	UINT32 inline_timeout = 100000;
	while (AsmReadMsr64(MSR_IA32_PERF_STATUS) & (1 << 21)) { if (!inline_timeout--) break; }
}

EFI_STATUS 
SaveSettings(
  VOID
  )
{
  UINT64  msr;
  gMobile = gSettings.Mobile;
  
  if ((gSettings.BusSpeed != 0) &&
      (gSettings.BusSpeed > 10 * kilo) &&
      (gSettings.BusSpeed < 500 * kilo)) {
        gCPUStructure.ExternalClock = gSettings.BusSpeed;
        gCPUStructure.FSBFrequency = gSettings.BusSpeed * kilo; //kHz -> Hz
  }
  
  if ((gSettings.CpuFreqMHz != 0) &&
      (gSettings.CpuFreqMHz > 100) &&
      (gSettings.CpuFreqMHz < 20000)){
        gCPUStructure.CurrentSpeed = gSettings.CpuFreqMHz;
  }
  
  if (gSettings.Turbo){
    if (gCPUStructure.Turbo4) {
      gCPUStructure.CPUFrequency = DivU64x32(gCPUStructure.Turbo4 * gCPUStructure.FSBFrequency, 10);
    }
    if (TurboMsr != 0) {
      AsmWriteMsr64(MSR_IA32_PERF_CONTROL, TurboMsr);
      gBS->Stall(100);
      WaitForSts();
    }
    msr = AsmReadMsr64(MSR_IA32_PERF_STATUS);
  } 
  return EFI_SUCCESS;
}
// ----============================----

EFI_STATUS
GetBootDefault(
   IN EFI_FILE *RootFileHandle,
   IN CHAR16* ConfigPlistPath
   )
{
    EFI_STATUS	Status = EFI_NOT_FOUND;
    CHAR8*      gConfigPtr = NULL;
    TagPtr      dict;
    TagPtr      dictPointer;
    TagPtr      prop;
    UINTN       size;

  if ((RootFileHandle != NULL) && FileExists(RootFileHandle, ConfigPlistPath)) {
    Status = egLoadFile(RootFileHandle, ConfigPlistPath, (UINT8**)&gConfigPtr, &size);
  } 
	if(EFI_ERROR(Status)) {
		Print(L"Error loading config.plist!\n\r");
		return Status;
	}
  
	if(gConfigPtr)
	{		
		if(ParseXML((const CHAR8*)gConfigPtr, &dict) != EFI_SUCCESS)
		{
			Print(L"config error\n");
			return EFI_UNSUPPORTED;
		}
  }
  dictPointer = GetProperty(dict,"SystemParameters");
  if (dictPointer) {
    prop = GetProperty(dictPointer,"Timeout");
    if(prop)
    {
      if ((prop->string[0] == '0')  && 
          (prop->string[1] == 'x' || prop->string[1] == 'X')) 
        gSettings.BootTimeout = AsciiStrHexToUintn(prop->string);
      else 
        gSettings.BootTimeout = AsciiStrDecimalToUintn(prop->string);
    }
    prop = GetProperty(dictPointer,"DefaultBootVolume");
    if(prop) AsciiStrToUnicodeStr(prop->string, gSettings.DefaultBoot);
  }
	return Status;
}

EFI_STATUS
GetUserSettings(
    IN EFI_FILE *RootFileHandle,
    IN CHAR16* ConfigPlistPath
    )
{
    EFI_STATUS	Status = EFI_NOT_FOUND;
    UINTN       size;
    CHAR8*      gConfigPtr = NULL;
    TagPtr      dict;
    TagPtr      dictPointer;
    TagPtr      prop;
    CHAR16      UStr[64];
    CHAR8       *BA;
    CHAR16      SystemID[40];

    if ((RootFileHandle != NULL) && FileExists(RootFileHandle, ConfigPlistPath))
    {
        Status = egLoadFile(RootFileHandle, ConfigPlistPath, (UINT8**)&gConfigPtr, &size);
    } 
	if(EFI_ERROR(Status))
    {
		Print(L"Error loading config.plist!\n\r");
		return Status;
	}

	if(gConfigPtr)
	{		
        if(ParseXML((const CHAR8*)gConfigPtr, &dict) != EFI_SUCCESS)
        {
            Print(L"config error\n");
            return EFI_UNSUPPORTED;
        }
        ZeroMem(gSettings.Language, 10);
        ZeroMem(gSettings.BootArgs, 120);
        dictPointer = GetProperty(dict,"SystemParameters");
        if (dictPointer) {
          prop = GetProperty(dictPointer,"prev-lang");
          if(prop)  AsciiStrCpy(gSettings.Language, prop->string);
            else    AsciiStrCpy(gSettings.Language, RuLang);
          prop = GetProperty(dictPointer,"boot-args");
          if(prop) {
            AsciiStrCpy(gSettings.BootArgs, prop->string);
            BA = &gSettings.BootArgs[119];
            bootArgsLen = 120;
            while ((*BA == ' ') || (*BA == 0)) { BA--; bootArgsLen--; }
          }
          prop = GetProperty(dictPointer,"DefaultBootVolume");
          if(prop) AsciiStrToUnicodeStr(prop->string, gSettings.DefaultBoot);
          prop = GetProperty(dictPointer,"CustomUUID");
          if(prop)
          {
            AsciiStrToUnicodeStr(prop->string, SystemID);
            Status = StrToGuidLE(SystemID, &gUuid);
            //else value from SMBIOS
          }
        }
        ZeroMem(gSettings.SerialNr, 64);
        dictPointer = GetProperty(dict,"Graphics");
        if (dictPointer) {
          prop = GetProperty(dictPointer,"GraphicsInjector");
          gSettings.GraphicsInjector=TRUE;
          if(prop) if ((prop->string[0] == 'n') || (prop->string[0] == 'N'))
            gSettings.GraphicsInjector=FALSE;
          prop = GetProperty(dictPointer,"VRAM");
          if(prop)
          {
            AsciiStrToUnicodeStr(prop->string, (CHAR16*)&UStr[0]);
            gSettings.VRAM = (UINT64)StrDecimalToUintn((CHAR16*)&UStr[0]) << 20;  //bytes
          }
          prop = GetProperty(dictPointer,"LoadVBios");
          gSettings.LoadVBios = FALSE;
          if(prop) if ((prop->string[0] == 'y') || (prop->string[0] == 'Y'))
            gSettings.LoadVBios = TRUE;
          prop = GetProperty(dictPointer,"VideoPorts");
          if(prop)
          {
            AsciiStrToUnicodeStr(prop->string, (CHAR16*)&UStr[0]);
            gSettings.VideoPorts = (UINT16)StrDecimalToUintn((CHAR16*)&UStr[0]);
            
          }
          prop = GetProperty(dictPointer,"FBName");
          if(prop) AsciiStrToUnicodeStr(prop->string, gSettings.FBName);
          prop = GetProperty(dictPointer,"NVCAP");
          if(prop) hex2bin(prop->string, (UINT8*)&gSettings.NVCAP[0], 20);
          prop = GetProperty(dictPointer,"cDevProp");
          if(prop) hex2bin(prop->string, (UINT8*)&gSettings.Dcfg[0], 8);
/*          prop = GetProperty(dictPointer, "CustomEDID");
          if(prop)
          {
            UINTN j = 128;
            gSettings.CustomEDID = GetDataSetting(dictPointer, "CustomEDID", &j);
          } */            
        }
        dictPointer = GetProperty(dict,"PCI");
        if (dictPointer) {
          gSettings.PCIRootUID = 0;
          prop = GetProperty(dictPointer,"PCIRootUID");
          if(prop)
          {
            AsciiStrToUnicodeStr(prop->string, (CHAR16*)&UStr[0]);
            gSettings.PCIRootUID = (UINT16)StrDecimalToUintn((CHAR16*)&UStr[0]);
          }
          prop = GetProperty(dictPointer,"DeviceProperties");
          if(prop)
          {
            cDevProp = AllocateZeroPool(AsciiStrLen(prop->string)+1);
            AsciiStrCpy(cDevProp, prop->string);
          } 
//          gSettings.CustomDevProp = FALSE;
//          prop = GetProperty(dictPointer,"CustomDevProp");
//          if(prop) if ((prop->string[0] == 'y') || (prop->string[0] == 'Y'))
//            gSettings.CustomDevProp = TRUE;
          gSettings.ETHInjection = FALSE;
          prop = GetProperty(dictPointer,"ETHInjection");
          if(prop) if ((prop->string[0] == 'y') || (prop->string[0] == 'Y'))
            gSettings.ETHInjection = TRUE;
          gSettings.USBInjection = FALSE;
          prop = GetProperty(dictPointer,"USBInjection");
          if(prop) if ((prop->string[0] == 'y') || (prop->string[0] == 'Y'))
            gSettings.USBInjection = TRUE;

          gSettings.HDAInjection = FALSE;
          gSettings.HDALayoutId = 0;
          prop = GetProperty(dictPointer,"HDAInjection");
          if(prop && (prop->string[0] != 'n' || prop->string[0] != 'N'))
          {
              gSettings.HDAInjection = TRUE;
              if ((prop->string[0] == '0')  && (prop->string[1] == 'x' || prop->string[1] == 'X'))
                gSettings.HDALayoutId = AsciiStrHexToUintn(prop->string);
              else
                gSettings.HDALayoutId = AsciiStrDecimalToUintn(prop->string);
          }
        }
        dictPointer = GetProperty(dict,"ACPI");
        if (dictPointer) {
          prop = GetProperty(dictPointer,"DropOemSSDT");
          gSettings.DropSSDT = FALSE;
          if(prop) if ((prop->string[0] == 'y') || (prop->string[0] == 'Y'))
            gSettings.DropSSDT = TRUE;
/*          prop = GetProperty(dictPointer,"GeneratePStates");
          gSettings.GeneratePStates = FALSE;
          if(prop) if ((prop->string[0] == 'y') || (prop->string[0] == 'Y'))
            gSettings.GeneratePStates = TRUE;
          prop = GetProperty(dictPointer,"GenerateCStates");
          gSettings.GenerateCStates = FALSE;
          if(prop) if ((prop->string[0] == 'y') || (prop->string[0] == 'Y'))
            gSettings.GenerateCStates = TRUE; */
          prop = GetProperty(dictPointer,"ResetAddress");
          gSettings.ResetAddr  = 0x64;
          if(prop)
          {
            if ((prop->string[0] == '0')  && 
                (prop->string[1] == 'x' || prop->string[1] == 'X')) 
              gSettings.ResetAddr = AsciiStrHexToUintn(prop->string);
            else 
              gSettings.ResetAddr = AsciiStrDecimalToUintn(prop->string);
          }
          prop = GetProperty(dictPointer,"ResetValue");
          gSettings.ResetVal = 0xFE;
          if(prop)
          {
            if ((prop->string[0] == '0')  && 
                (prop->string[1] == 'x' || prop->string[1] == 'X')) 
              gSettings.ResetVal = AsciiStrHexToUintn(prop->string);
            else 
              gSettings.ResetVal = AsciiStrDecimalToUintn(prop->string);
          }
          //other known pair is 0x0[C/2]F9/0x06
/*          prop = GetProperty(dictPointer,"EnableC2");
          gSettings.EnableC2 = FALSE;
          if(prop) if ((prop->string[0] == 'y') || (prop->string[0] == 'Y'))
            gSettings.EnableC2 = TRUE; 
          prop = GetProperty(dictPointer,"EnableC4");
          gSettings.EnableC4 = FALSE;
          if(prop) if ((prop->string[0] == 'y') || (prop->string[0] == 'Y'))
            gSettings.EnableC4 = TRUE;
          prop = GetProperty(dictPointer,"EnableC6");
          gSettings.EnableC6 = FALSE;
          if(prop) if ((prop->string[0] == 'y') || (prop->string[0] == 'Y'))
            gSettings.EnableC6 = TRUE;
          prop = GetProperty(dictPointer,"EnableISS");
          gSettings.EnableISS = FALSE;
          if(prop) if ((prop->string[0] == 'y') || (prop->string[0] == 'Y'))
            gSettings.EnableISS = TRUE; */
          gSettings.PMProfile = 0;
          prop = GetProperty(dictPointer,"PMProfile");
          if(prop)
          {
            if ((prop->string[0] == '0')  && 
                (prop->string[1] == 'x' || prop->string[1] == 'X')) 
              gSettings.PMProfile = AsciiStrHexToUintn(prop->string);
            else 
              gSettings.PMProfile = AsciiStrDecimalToUintn(prop->string);
          }
        }        
        dictPointer = GetProperty(dict,"SMBIOS");
        if (dictPointer) {
          prop = GetProperty(dictPointer,"BiosVendor");
          if(prop) AsciiStrCpy(gSettings.VendorName, prop->string);
          prop = GetProperty(dictPointer,"BiosVersion");
          if(prop) AsciiStrCpy(gSettings.RomVersion, prop->string);
          prop = GetProperty(dictPointer,"BiosReleaseDate");
          if(prop) AsciiStrCpy(gSettings.ReleaseDate, prop->string);
          prop = GetProperty(dictPointer,"Manufacturer");
          if(prop) AsciiStrCpy(gSettings.ManufactureName, prop->string);
          prop = GetProperty(dictPointer,"ProductName");
          if(prop) AsciiStrCpy(gSettings.ProductName, prop->string);
          prop = GetProperty(dictPointer,"Version");
          if(prop) AsciiStrCpy(gSettings.VersionNr, prop->string);
          prop = GetProperty(dictPointer,"Family");
          if(prop) AsciiStrCpy(gSettings.FamilyName, prop->string);
          prop = GetProperty(dictPointer,"SerialNumber");
          if(prop) AsciiStrCpy(gSettings.SerialNr, prop->string);
          prop = GetProperty(dictPointer,"BoardManufacturer");
          if(prop) AsciiStrCpy(gSettings.BoardManufactureName, prop->string);
          prop = GetProperty(dictPointer,"BoardSerialNumber");
          if(prop) AsciiStrCpy(gSettings.BoardSerialNumber, prop->string);
          prop = GetProperty(dictPointer,"Board-ID");
          if(prop) AsciiStrCpy(gSettings.BoardNumber, prop->string);
          prop = GetProperty(dictPointer,"BoardVersion");
          if(prop) AsciiStrCpy(gSettings.BoardVersion, prop->string);
          prop = GetProperty(dictPointer,"Mobile");
          gSettings.Mobile = gMobile;  //default
          if(prop) if ((prop->string[0] == 'y') || (prop->string[0] == 'Y'))
            gSettings.Mobile = TRUE;
          prop = GetProperty(dictPointer,"LocationInChassis");
          if(prop) AsciiStrCpy(gSettings.LocationInChassis, prop->string);
          prop = GetProperty(dictPointer,"ChassisManufacturer");
          if(prop) AsciiStrCpy(gSettings.ChassisManufacturer, prop->string);
          prop = GetProperty(dictPointer,"ChassisAssetTag");
          if(prop) AsciiStrCpy(gSettings.ChassisAssetTag, prop->string);
        }    
        dictPointer = GetProperty(dict,"CPU");
        if (dictPointer) {
          prop = GetProperty(dictPointer,"Turbo");
          gSettings.Turbo = FALSE;
          if(prop) if ((prop->string[0] == 'y') || (prop->string[0] == 'Y'))
            gSettings.Turbo = TRUE;
          prop = GetProperty(dictPointer,"CpuFrequencyMHz");
          if(prop)
          {
            if ((prop->string[0] == '0')  && 
                (prop->string[1] == 'x' || prop->string[1] == 'X')) 
              gSettings.CpuFreqMHz = AsciiStrHexToUintn(prop->string);
            else 
              gSettings.CpuFreqMHz = AsciiStrDecimalToUintn(prop->string);
          }
          prop = GetProperty(dictPointer,"ProcessorType");
          gSettings.CpuType = GetAdvancedCpuType();
          if(prop)
          {
            if ((prop->string[0] == '0')  && 
                (prop->string[1] == 'x' || prop->string[1] == 'X')) 
              gSettings.CpuType = AsciiStrHexToUintn(prop->string);
            else 
              gSettings.CpuType = AsciiStrDecimalToUintn(prop->string);
          }
          prop = GetProperty(dictPointer,"BusSpeedkHz");
          if(prop)
          {
            if ((prop->string[0] == '0')  && 
                (prop->string[1] == 'x' || prop->string[1] == 'X')) 
              gSettings.BusSpeed = AsciiStrHexToUintn(prop->string);
            else 
              gSettings.BusSpeed = AsciiStrDecimalToUintn(prop->string);
          }
          prop = GetProperty(dictPointer,"QPI");      
          if(prop)
          {
            if ((prop->string[0] == '0')  && 
                (prop->string[1] == 'x' || prop->string[1] == 'X')) 
              gSettings.ProcessorInterconnectSpeed = AsciiStrHexToUintn(prop->string);
            else 
              gSettings.ProcessorInterconnectSpeed = AsciiStrDecimalToUintn(prop->string);
          }
        }    
        SaveSettings();
	}	
	return Status;
}	

/*
EFI_STATUS 
GetOSVersion(
  IN EFI_FILE *RootFileHandle,
  OUT CHAR16  *OsName
  )
{
    EFI_STATUS				Status = EFI_NOT_FOUND;
    CHAR8*						plistBuffer = 0;
    UINTN             plistLen;
    TagPtr						dict=NULL;
    TagPtr						prop = NULL;
    CHAR16*           SystemPlist = L"System\\Library\\CoreServices\\SystemVersion.plist";
    CHAR16*           ServerPlist = L"System\\Library\\CoreServices\\ServerVersion.plist";
  
    if (!RootFileHandle)
    {
        return EFI_NOT_FOUND;
    }
    OsName = NULL;
	// Mac OS X 
	if (FileExists(RootFileHandle, SystemPlist)) 
		Status = egLoadFile(RootFileHandle, SystemPlist, (UINT8 **)&plistBuffer, &plistLen);
    // Mac OS X Server 
    else if (FileExists(RootFileHandle, ServerPlist))
            Status = egLoadFile(RootFileHandle, ServerPlist, (UINT8 **)&plistBuffer, &plistLen);
  
	if(!EFI_ERROR(Status))
	{
		if(ParseXML(plistBuffer, &dict) != EFI_SUCCESS)
		{
			FreePool(plistBuffer);
			return EFI_NOT_FOUND;
		}    
		prop = GetProperty(dict, "ProductVersion");
		if(prop != NULL)
		{
			// Tiger
			if(AsciiStrStr(prop->string, "10.4") != 0){
        OsName = L"tiger";
        Status = EFI_SUCCESS;
      } else
        // Leopard
        if(AsciiStrStr(prop->string, "10.5") != 0){
          OsName = L"leo";
          Status = EFI_SUCCESS;
        } else
          // Snow Leopard
          if(AsciiStrStr(prop->string, "10.6") != 0){
            OsName = L"snow";
            Status = EFI_SUCCESS;
          } else
            // Lion
            if(AsciiStrStr(prop->string, "10.7") != 0){
              OsName = L"lion";
              Status = EFI_SUCCESS;
            } else
              // Mountain Lion
              if(AsciiStrStr(prop->string, "10.8") != 0){
                OsName = L"cougar";
                Status = EFI_SUCCESS;
              }
    } 
	}	
	return Status;
}
*/