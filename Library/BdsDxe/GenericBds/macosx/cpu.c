/* 

 cpu.c
 implementation for cpu

 Remade by Slice 2011 based on Apple's XNU sources
 Portion copyright from Chameleon project. Thanks to all who involved to it.
 */
/*
 * Copyright (c) 2000-2006 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
#include "macosx.h"
#include "Library/IoLib.h"


UINT8							gDefaultType; 
UINT64            TurboMsr = 0;

#define CLKNUM          1193182         /* formerly 1193167 1193182 */
#define CALIBRATE_TIME_MSEC 30 /* 30 msecs */
#define CALIBRATE_LATCH ((CLKNUM * CALIBRATE_TIME_MSEC + 1000/2)/1000)

VOID
enable_PIT2 (
 VOID
 )
{
#if 0
  __asm__ volatile(
                   " inb   $0x61,%%al  \n\t"
                   " and   $0xFC,%%al  \n\t"  
                   " or    $1,%%al     \n\t"
                   " outb  %%al,$0x61  \n\t"
                   : : : "%al" );
#else
  IoAndThenOr8(0x61, 0xFC, 0x01);
#endif
}

VOID
disable_PIT2 (
  VOID
  )
{
#if 0
  __asm__ volatile(
                   " inb   $0x61,%%al  \n\t"
                   " and   $0xFC,%%al  \n\t"
                   " outb  %%al,$0x61  \n\t"
                   : : : "%al" );
#else
  IoAnd8(0x61, 0xFC);
#endif
}

VOID
set_PIT2_mode0 (
  IN   UINT64  Value
  )
{
#if 0
  __asm__ volatile(
                   " movb  $0xB0,%%al  \n\t"
                   " outb	%%al,$0x43	 \n\t"
                   " movb	%%dl,%%al	   \n\t"
                   " outb	%%al,$0x42	 \n\t"
                   " movb	%%dh,%%al	   \n\t"
                   " outb	%%al,$0x42"
                   : : "d"(Value));
#else
  IoWrite8(0x43, 0xB0);
  IoWrite8(0x42, (UINT8)Value);
  IoWrite8(0x42, (UINT8)(((UINT16)Value) >> 8));
#endif
}

UINT32 poll_PIT2_gate(
  VOID
  )
{
  UINT32 count = 0;
#if 0
  CHAR8 nmi_sc_val[1];
  do {
    ++count;
    __asm__ volatile(
                     " inb	$0x61,%0"
                     : "=q"(nmi_sc_val));
  } while( (nmi_sc_val[1] & 0x20) == 0);
#else
  do {
    count++;
  } while((IoRead8(0x61) & 0x20) == 0);
#endif
  return count;
}

UINT64 measure_tsc_frequency(
   VOID
   )
{
  UINT64 tscStart;
  UINT64 tscEnd;
  UINT64 tscDelta = 0xffffffffffffffffULL;
  UINT32 pollCount;
  UINT64 retval = 0;
  UINTN i;
  
  for(i = 0; i < 3; ++i)
  {
    enable_PIT2();
    set_PIT2_mode0(CALIBRATE_LATCH);
    tscStart = AsmReadTsc();
    pollCount = poll_PIT2_gate();
    tscEnd = AsmReadTsc();
    if(pollCount <= 1)
      continue;
    if((tscEnd - tscStart) <= CALIBRATE_TIME_MSEC)
      continue;
    if( (tscEnd - tscStart) < tscDelta )
      tscDelta = tscEnd - tscStart;
  }
  if(tscDelta > (1ULL<<32))
    retval = 0;
  else
  {
    retval = tscDelta * 1000 / 30;
  }
  disable_PIT2();
  return retval;
}
// ----=========================================----
VOID DoCpuid(UINT32 selector, UINT32 *data)
{
	AsmCpuid(selector, data, data+1, data+2, data+3);
}

UINT64 GetCPUProperties ()
{
	UINT8                 index;
	UINT32                reg[4];
	UINT64                msr = 0;
	UINT64                flex_ratio = 0;
//	UINT8		XE = 0;
	EFI_STATUS            Status;
	EFI_HANDLE            *HandleBuffer;
	EFI_GUID              **ProtocolGuidArray;
	EFI_PCI_IO_PROTOCOL   *PciIo;
	PCI_TYPE00            Pci;
	UINTN                 HandleCount;
	UINTN                 ArrayCount;
	UINTN                 HandleIndex;
	UINTN                 ProtocolIndex;
	UINT16                qpibusspeed; //units=MHz
	UINT32                qpimult = 2;
	UINT16                did, vid;
	UINTN                 Segment;
	UINTN                 Bus;
	UINTN                 Device;
	UINTN                 Function;
    CHAR8                 str[128], *s;
	UINT32                multiplier=0;
    
	gCPUStructure.MaxDiv=1;
	gCPUStructure.CurrDiv=1;
	gCPUStructure.MaxRatio=10;
	gCPUStructure.MinRatio=10;
	gCPUStructure.ProcessorInterconnectSpeed=0;
	gCPUStructure.Mobile = FALSE; //not same as gMobile
  
	gCPUStructure.FSBFrequency=gCPUStructure.ExternalClock?gCPUStructure.ExternalClock  * 1000000ull:100000000ull;
	gCPUStructure.TSCFrequency=gCPUStructure.CurrentSpeed?gCPUStructure.CurrentSpeed * 1000000ull:measure_tsc_frequency()*1000;
	gCPUStructure.CPUFrequency=gCPUStructure.TSCFrequency;

/*  
	for(i = 0; i < 4; i++)
	{
		DoCpuid(i, gCPUStructure.CPUID[i]);
	}
*/  
    DoCpuid(0, gCPUStructure.CPUID[CPUID_0]);
    AsmWriteMsr64(MSR_IA32_BIOS_SIGN_ID, 0);

    DoCpuid(1, gCPUStructure.CPUID[CPUID_1]);
    msr = AsmReadMsr64(MSR_IA32_BIOS_SIGN_ID);
    gCPUStructure.MicroCode = msr >> 32;
    gCPUStructure.ProcessorFlag = (AsmReadMsr64(MSR_IA32_PLATFORM_ID) >> 50) & 3;

	DoCpuid(0x80000000, gCPUStructure.CPUID[CPUID_80]);
	if((gCPUStructure.CPUID[CPUID_80][0] & 0x0000000f) >= 1)
    {
		DoCpuid(0x80000001, gCPUStructure.CPUID[CPUID_81]); 
	}
	
	gCPUStructure.Vendor      = gCPUStructure.CPUID[CPUID_0][1];
	gCPUStructure.Signature   = gCPUStructure.CPUID[CPUID_1][0];
	gCPUStructure.Stepping    = (UINT8) bitfield(gCPUStructure.CPUID[CPUID_1][0], 3, 0);
	gCPUStructure.Model       = (UINT8) bitfield(gCPUStructure.CPUID[CPUID_1][0], 7, 4);
	gCPUStructure.Family      = (UINT8) bitfield(gCPUStructure.CPUID[CPUID_1][0], 11, 8);
	gCPUStructure.Type        = (UINT8) bitfield(gCPUStructure.CPUID[CPUID_1][0], 13, 12);
	gCPUStructure.Extmodel    = (UINT8) bitfield(gCPUStructure.CPUID[CPUID_1][0], 19, 16);
	gCPUStructure.Extfamily   = (UINT8) bitfield(gCPUStructure.CPUID[CPUID_1][0], 27, 20);
	gCPUStructure.Features    = quad(gCPUStructure.CPUID[CPUID_1][2], gCPUStructure.CPUID[CPUID_1][3]);
	gCPUStructure.ExtFeatures = quad(gCPUStructure.CPUID[CPUID_81][2], gCPUStructure.CPUID[CPUID_81][3]);

	if (gCPUStructure.Family == 0x0f)
    {
		gCPUStructure.Family += gCPUStructure.Extfamily;
	}
	gCPUStructure.Model += (gCPUStructure.Extmodel << 4);

// Cores & Threads count
	if (gCPUStructure.Features & CPUID_FEATURE_HTT)
    {
		gCPUStructure.LogicalPerPackage	= bitfield(gCPUStructure.CPUID[CPUID_1][EBX], 23, 16); //Atom330 = 4
	} else {
		gCPUStructure.LogicalPerPackage	= 1;
	}

    DoCpuid(4, gCPUStructure.CPUID[CPUID_4]);
	gCPUStructure.CoresPerPackage =  bitfield(gCPUStructure.CPUID[CPUID_4][EAX], 31, 26) + 1; //Atom330 = 2
	if (gCPUStructure.CoresPerPackage == 0)
    {
		gCPUStructure.CoresPerPackage = 1;
	}
  
	if((gCPUStructure.CPUID[CPUID_80][EAX] & 0x0000000f) >= 7)
    {
		DoCpuid(0x80000007, gCPUStructure.CPUID[CPUID_87]); 
        gCPUStructure.ExtFeatures |= gCPUStructure.CPUID[CPUID_87][EDX] & (UINT32)CPUID_EXTFEATURE_TSCI;
	}
    if (gCPUStructure.Vendor == CPU_VENDOR_INTEL) {
        switch (gCPUStructure.Model)
        {
          case CPU_MODEL_NEHALEM: 
          case CPU_MODEL_FIELDS: 
          case CPU_MODEL_CLARKDALE:  
          case CPU_MODEL_NEHALEM_EX:	
          case CPU_MODEL_JAKETOWN:
          case CPU_MODEL_SANDY_BRIDGE:
          case CPU_MODEL_IVY_BRIDGE:
          case CPU_MODEL_IVY_BRIDGE_E5:
            msr = AsmReadMsr64(MSR_CORE_THREAD_COUNT);
            gCPUStructure.Cores   = (UINT8)bitfield((UINT32)msr, 31, 16);
            gCPUStructure.Threads = (UINT8)bitfield((UINT32)msr, 15,  0);
            break;
          case CPU_MODEL_DALES:
          case CPU_MODEL_WESTMERE: 
          case CPU_MODEL_WESTMERE_EX:
            msr = AsmReadMsr64(MSR_CORE_THREAD_COUNT);
            gCPUStructure.Cores   = (UINT8)bitfield((UINT32)msr, 19, 16);
            gCPUStructure.Threads = (UINT8)bitfield((UINT32)msr, 15,  0);
            break;            
          default:		
            gCPUStructure.Cores   = (UINT8)(bitfield(gCPUStructure.CPUID[CPUID_1][EBX], 23, 16));
            gCPUStructure.Threads = (UINT8)(gCPUStructure.LogicalPerPackage & 0xff);
            break;
        }    
    }
	if (gCPUStructure.Cores == 0) {
        gCPUStructure.Cores   = (UINT8)(gCPUStructure.CoresPerPackage & 0xff);
        gCPUStructure.Threads = (UINT8)(gCPUStructure.LogicalPerPackage & 0xff);
	}
    // Brand String
	if(gCPUStructure.CPUID[CPUID_80][0] >= 0x80000004)
    {
		ZeroMem(str, 128);
		DoCpuid(0x80000002, reg); 
		CopyMem(&str[0], (CHAR8 *)reg,  16); 
		DoCpuid(0x80000003, reg); 
		CopyMem(&str[16], (CHAR8 *)reg,  16); 
		DoCpuid(0x80000004, reg); 
		CopyMem(&str[32], (CHAR8 *)reg,  16);
		for (s = str; *s != '\0'; s++)
        {
			if (*s != ' ') break; //remove leading spaces
		}
		AsciiStrnCpy(gCPUStructure.BrandString, s, 48);
		
		if (!AsciiStrnCmp((const CHAR8*)gCPUStructure.BrandString, (const CHAR8*)CPU_STRING_UNKNOWN, iStrLen((gCPUStructure.BrandString) + 1, 48))) 
		{
			gCPUStructure.BrandString[0] = '\0'; 
		}
		gCPUStructure.BrandString[47] = '\0'; 
	}
    s[0]=0;
    for (index=0; index < 46; index++)
    {
        // format is either “x.xxyHz” or “xxxxyHz”, where y=M,G,T and x is digits
        // Search brand string for “yHz” where y is M, G, or T
        // Set multiplier so frequency is in MHz
        if (gCPUStructure.BrandString[index+1] == 'H' && gCPUStructure.BrandString[index+2] == 'z') {
            if (gCPUStructure.BrandString[index] == 'M')
                multiplier = 1;
            else if (gCPUStructure.BrandString[index] == 'G')
                multiplier = 1000;
            else if (gCPUStructure.BrandString[index] == 'T')
                multiplier = 1000000;
        }
        if (multiplier > 0) {
            // Copy 7 characters (length of “x.xxyHz”)
            // index is at position of y in “x.xxyHz”
            AsciiStrnCpy(s, &gCPUStructure.BrandString[index-4], 7);
            s[7] = 0; // null terminate the string
            // Compute frequency (in MHz) from brand string
            if (gCPUStructure.BrandString[index-3] == '.') { // If format is “x.xx”
                gCPUStructure.CurrentSpeed  = (UINT16)(gCPUStructure.BrandString[index-4] - '0') * multiplier;
                gCPUStructure.CurrentSpeed += (UINT16)(gCPUStructure.BrandString[index-2] - '0') * (multiplier / 10);
                gCPUStructure.CurrentSpeed += (UINT16)(gCPUStructure.BrandString[index-1] - '0') * (multiplier / 100);
            } else {                            // If format is xxxx
                gCPUStructure.CurrentSpeed  = (UINT16)(gCPUStructure.BrandString[index-4] - '0') * 1000;
                gCPUStructure.CurrentSpeed += (UINT16)(gCPUStructure.BrandString[index-3] - '0') * 100;
                gCPUStructure.CurrentSpeed += (UINT16)(gCPUStructure.BrandString[index-2] - '0') * 10;
                gCPUStructure.CurrentSpeed += (UINT16)(gCPUStructure.BrandString[index-1] - '0');
                gCPUStructure.CurrentSpeed *= multiplier;
            }
            break;
        }
    }
/*
    for (Index=0; Index < 47; Index++) {
        if (gCPUStructure.BrandString[Index] == '@') {
            for (i=0; i < 10; i++)
            {
                str[i] = gCPUStructure.BrandString[Index + 2 + i];
                if (gCPUStructure.BrandString[Index + 3 + i] == 'G')
                {
                    str[i+1] = '\0';
                    break;
                }
            }
            break;
        }
    }
    AsciiStrnCpy(s, str, i+1);
    for (Index=0; Index < 10; Index++) {
        if (s[Index] != '.') {
            str[Index] = s[Index];
            if (s[Index] == '\0' ) break;
        } else
        {
            for (i=1; i < 10; i++)
            {
                str[Index+i-1] = s[Index+i];
                if (s[Index + 1] == '\0')
                    break;
            }
            break;
        }
    }
    switch (AsciiStrLen(str)) {
        case 0:
            break;
        case 1:
            gCPUStructure.CurrentSpeed = AsciiStrDecimalToUintn(str) * 1000;
            break;
        case 2:
            gCPUStructure.CurrentSpeed = AsciiStrDecimalToUintn(str) * 100;
            break;
        case 3:
            gCPUStructure.CurrentSpeed = AsciiStrDecimalToUintn(str) * 10;
            break;
            
        default:
            gCPUStructure.CurrentSpeed = AsciiStrDecimalToUintn(str);
            break;
    }
*/    
	if(gCPUStructure.Vendor == CPU_VENDOR_INTEL && gCPUStructure.Family == 0x06 && gCPUStructure.Model >= 0x0c)
	{
        switch (gCPUStructure.Model)
        {
            
          case CPU_MODEL_SANDY_BRIDGE:// Sandy Bridge, 32nm
          case CPU_MODEL_JAKETOWN:
          case CPU_MODEL_IVY_BRIDGE:
          case CPU_MODEL_IVY_BRIDGE_E5:
            msr = AsmReadMsr64(MSR_PLATFORM_INFO);
            gCPUStructure.MinRatio = (UINT8)(msr >> 40) & 0xff;
    //        gCPUStructure.MaxRatio = (UINT8)(msr >> 8) & 0xff;
            msr = AsmReadMsr64(MSR_IA32_PERF_STATUS);
            TurboMsr = msr + (1 << 8);
            gCPUStructure.MaxRatio = (UINT8)(msr >> 8) & 0xff;
            msr = AsmReadMsr64(MSR_FLEX_RATIO);
            if ((msr >> 16) & 0x01)
            {
              flex_ratio = (msr >> 8) & 0xff;
              if (flex_ratio == 0) { 
                AsmWriteMsr64(MSR_FLEX_RATIO, (msr & 0xFFFFFFFFFFFEFFFFULL));
                gBS->Stall(10);
                msr = AsmReadMsr64(MSR_FLEX_RATIO);
              } 
            }
            gCPUStructure.MinRatio *= 10;
            gCPUStructure.MaxRatio *= 10;          

            msr = AsmReadMsr64(MSR_TURBO_RATIO_LIMIT);        
            gCPUStructure.Turbo1 = (UINT8)((msr >> 0) & 0xff) * 10;
            gCPUStructure.Turbo2 = (UINT8)((msr >> 8) & 0xff) * 10;
            gCPUStructure.Turbo3 = (UINT8)((msr >> 16) & 0xff) * 10;
            gCPUStructure.Turbo4 = (UINT8)(msr >> 24) & 0xff; //later
            if (gCPUStructure.Cores < 4) {
              gCPUStructure.Turbo4 = gCPUStructure.Turbo1;
            } else {
              gCPUStructure.Turbo4 *= 10;          
            }
            break;

          case CPU_MODEL_NEHALEM:// Core i7 LGA1366, Xeon 5500, "Bloomfield", "Gainstown", 45nm
          case CPU_MODEL_FIELDS:// Core i7, i5 LGA1156, "Clarksfield", "Lynnfield", "Jasper", 45nm
          case CPU_MODEL_DALES:// Core i7, i5, Nehalem
          case CPU_MODEL_CLARKDALE:// Core i7, i5, i3 LGA1156, "Westmere", "Clarkdale", , 32nm
          case CPU_MODEL_WESTMERE:// Core i7 LGA1366, Six-core, "Westmere", "Gulftown", 32nm
          case CPU_MODEL_NEHALEM_EX:// Core i7, Nehalem-Ex Xeon, "Beckton"
          case CPU_MODEL_WESTMERE_EX:// Core i7, Nehalem-Ex Xeon, "Eagleton"
            msr = AsmReadMsr64(MSR_PLATFORM_INFO);            
            gCPUStructure.MinRatio = (UINT8)(msr >> 40) & 0xff;
            msr = AsmReadMsr64(MSR_IA32_PERF_STATUS);
            gCPUStructure.MaxRatio = (UINT8)(msr & 0xff); 
            TurboMsr = msr + 1;
    //        if(gCPUStructure.MaxRatio) {
    //          gCPUStructure.FSBFrequency = DivU64x32(gCPUStructure.TSCFrequency, gCPUStructure.MaxRatio);
    //        }
            
            if ((gCPUStructure.Model != CPU_MODEL_NEHALEM_EX) &&
                (gCPUStructure.Model != CPU_MODEL_WESTMERE_EX) &&
                (gCPUStructure.Model != CPU_MODEL_FIELDS))
            {
              msr = AsmReadMsr64(MSR_TURBO_RATIO_LIMIT);
              gCPUStructure.Turbo1 = (UINT8)((msr >> 0) & 0xff) * 10;
              gCPUStructure.Turbo2 = (UINT8)((msr >> 8) & 0xff) * 10;
              gCPUStructure.Turbo3 = (UINT8)((msr >> 16) & 0xff) * 10;
              gCPUStructure.Turbo4 = (UINT8)(msr >> 24) & 0xff; //later
            } else {
              gCPUStructure.Turbo4 = gCPUStructure.MaxRatio + 1;
            }
            
            
            if (gCPUStructure.Cores < 4) {
              gCPUStructure.Turbo4 = gCPUStructure.Turbo1;
            } else {
              gCPUStructure.Turbo4 *= 10;          
            }
            
            gCPUStructure.MaxRatio *= 10;
            gCPUStructure.MinRatio *= 10;
            
            break;

          case CPU_MODEL_ATOM:// Core i7 & Atom
          case CPU_MODEL_DOTHAN:// Pentium M, Dothan, 90nm
          case CPU_MODEL_YONAH:// Core Duo/Solo, Pentium M DC
          case CPU_MODEL_MEROM:// Core Xeon, Core 2 DC, 65nm
          case CPU_MODEL_PENRYN:// Core 2 Duo/Extreme, Xeon, 45nm
            if(AsmReadMsr64(MSR_IA32_PLATFORM_ID) & (1<<28)){
              gCPUStructure.Mobile = TRUE;
            }
            msr = AsmReadMsr64(MSR_IA32_PERF_STATUS);
            TurboMsr = msr + (1 << 8);
            gCPUStructure.MaxRatio = (UINT32)(msr >> 8) & 0x1f;
            gCPUStructure.SubDivider = (UINT32)(msr >> 14) & 0x1;
    //        gCPUStructure.FSBFrequency = DivU64x32(gCPUStructure.TSCFrequency * 2,
    //                                               gCPUStructure.MaxRatio * 2 + gCPUStructure.SubDivider);
            gCPUStructure.MinRatio = 60;
            gCPUStructure.MaxRatio = gCPUStructure.MaxRatio * 10 + gCPUStructure.SubDivider * 5; 
            gCPUStructure.Turbo4 = gCPUStructure.MaxRatio + 10;            
            break;

          case CPU_MODEL_CELERON:// Celeron, Core 2 SC, 65nm
          case CPU_MODEL_LINCROFT:// Atom Lincroft, 45nm
          case CPU_MODEL_XEON_MP:// Xeon MP MP 7400
          case 0x2b:// SNB Xeon //XXX
          default:
            gCPUStructure.MinRatio = 60;
            gCPUStructure.MaxRatio = DivU64x32(gCPUStructure.TSCFrequency, gCPUStructure.FSBFrequency) * 10;
            break;
        }
        msr = AsmReadMsr64(MSR_IA32_PERF_STATUS);
        gCPUStructure.MaxDiv = (UINT8)(msr >> 46) & 0x01;
        gCPUStructure.CurrDiv = (UINT8)(msr >> 15) & 0x01;
	}
// ----====================================================================----
	if (gCPUStructure.Model >= CPU_MODEL_NEHALEM)
    {
		qpimult = 2; //init
		Status = gBS->LocateHandleBuffer(AllHandles,NULL,NULL,&HandleCount,&HandleBuffer);
		if (!EFI_ERROR(Status))
		{	
			for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++)
			{
				Status = gBS->ProtocolsPerHandle(HandleBuffer[HandleIndex],&ProtocolGuidArray,&ArrayCount);
				if (!EFI_ERROR(Status))
				{			
					for (ProtocolIndex = 0; ProtocolIndex < ArrayCount; ProtocolIndex++)
					{
						if (CompareGuid(&gEfiPciIoProtocolGuid, ProtocolGuidArray[ProtocolIndex]))
						{
							Status = gBS->OpenProtocol(HandleBuffer[HandleIndex],&gEfiPciIoProtocolGuid,(VOID **)&PciIo,gImageHandle,NULL,EFI_OPEN_PROTOCOL_GET_PROTOCOL);
							if (!EFI_ERROR(Status))
							{
								Status = PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Function);
								if ((Bus & 0x3F) != 0x3F)
                                {
									continue;
								}
								Status = PciIo->Pci.Read (
                                          PciIo,
                                          EfiPciIoWidthUint32,
                                          0,
                                          sizeof (Pci) / sizeof (UINT32),
                                          &Pci
                                          );
								vid = Pci.Hdr.VendorId & 0xFFFF;
								did = Pci.Hdr.DeviceId & 0xFF00;
								if ((vid == 0x8086) && (did >= 0x2C00)
                                    && (Device == 2) && (Function == 1))
                                {
									Status = PciIo->Mem.Read (
                                            PciIo,
                                            EfiPciIoWidthUint32,
                                            EFI_PCI_IO_PASS_THROUGH_BAR,           
                                            0x50,
                                            1,
                                            &qpimult
                                            );
									if (EFI_ERROR(Status)) continue;
									qpimult &= 0x7F;
									break;
								}
							}
						}
					}
				}
			}
		}
    
		qpibusspeed = qpimult * 2 * gCPUStructure.ExternalClock*1000;
		if (qpibusspeed%100 != 0) qpibusspeed = ((qpibusspeed+50)/100)*100;
		gCPUStructure.ProcessorInterconnectSpeed = qpibusspeed;    
	} 
	
//	gCPUStructure.CurrentSpeed=(UINT16)DivU64x32(gCPUStructure.TSCFrequency, 1000000);
	return gCPUStructure.FSBFrequency;
}

UINT16 GetStandardCpuType()
{
	if (gCPUStructure.Threads >= 4) 
	{	
		return 0x402;   // Quad-Core Xeon
	}
	else if (gCPUStructure.Threads == 1) 
	{	
		return 0x201;   // Core Solo
	};
	return 0x301;   // Core 2 Duo
}

UINT16 GetAdvancedCpuType ()
{	
	if (gCPUStructure.Vendor == CPU_VENDOR_INTEL)
	{	
		switch (gCPUStructure.Family) {	
			case 0x06:	
			{			
				switch (gCPUStructure.Model)
				{
					case CPU_MODEL_DOTHAN:// Dothan
					case CPU_MODEL_YONAH: // Yonah
						return 0x201;
					case CPU_MODEL_MEROM: // Merom
					case CPU_MODEL_PENRYN:// Penryn
					case CPU_MODEL_ATOM:  // Atom (45nm)
						return GetStandardCpuType();
						
					case CPU_MODEL_NEHALEM_EX: //Xeon 5300
						return 0x402;
						
					case CPU_MODEL_NEHALEM: // Intel Core i7 LGA1366 (45nm)
						return 0x701; // Core i7
						
					case CPU_MODEL_FIELDS: // Lynnfield, Clarksfield, Jasper
						if (AsciiStrStr(gCPUStructure.BrandString, "i5"))
							return 0x601; // Core i5
						return 0x701; // Core i7
						
					case CPU_MODEL_DALES: // Intel Core i5, i7 LGA1156 (45nm) (Havendale, Auburndale)
                        if (AsciiStrStr(gCPUStructure.BrandString, "Core(TM) i3"))
							return 0x901; // Core i3 //why not 902? Ask Apple
						if (AsciiStrStr(gCPUStructure.BrandString, "i5") || (gCPUStructure.Cores <= 2))
							return 0x602; // Core i5
						return 0x702; // Core i7
						
					//case CPU_MODEL_ARRANDALE:
					case CPU_MODEL_CLARKDALE: // Intel Core i3, i5, i7 LGA1156 (32nm) (Clarkdale, Arrandale)
						
						if (AsciiStrStr(gCPUStructure.BrandString, "i3"))
							return 0x901; // Core i3
						if (AsciiStrStr(gCPUStructure.BrandString, "i5") || (gCPUStructure.Cores <= 2))
							return 0x601; // Core i5 - (M540 -> 0x0602)
						return 0x701; // Core i7
						
					case CPU_MODEL_WESTMERE: // Intel Core i7 LGA1366 (32nm) 6 Core (Gulftown, Westmere-EP, Westmere-WS)
					case CPU_MODEL_WESTMERE_EX: // Intel Core i7 LGA1366 (45nm) 6 Core ???
						return 0x701; // Core i7
                        
					case CPU_MODEL_SANDY_BRIDGE:
                        if (AsciiStrStr(gCPUStructure.BrandString, "i3"))
							return 0x903; // Core i3
                        if (AsciiStrStr(gCPUStructure.BrandString, "i5") || (gCPUStructure.Cores <= 2))
                            return 0x603; // Core i5
                        return 0x703;
                        
                    case CPU_MODEL_IVY_BRIDGE:
                    case CPU_MODEL_IVY_BRIDGE_E5:
                        if (AsciiStrStr(gCPUStructure.BrandString, "i3"))
							return 0x903; // Core i3 - Apple doesn't use it
                        if (AsciiStrStr(gCPUStructure.BrandString, "i5") || (gCPUStructure.Cores <= 2))
                            return 0x604; // Core i5
						return 0x704;
				}
			}
		}
	}
	return GetStandardCpuType();
}

MACHINE_TYPES GetDefaultModel()
{
  MACHINE_TYPES DefaultType = MacPro31;
	// TODO: Add more CPU models and configure the correct machines per CPU/GFX model
	if(gMobile)
	{
		switch (gCPUStructure.Model)
		{
			case CPU_MODEL_ATOM:
				DefaultType = MacBookAir31; //MacBookAir1,1 doesn't support _PSS for speedstep!
				break;
                
			case CPU_MODEL_DOTHAN:	
				DefaultType = MacBook11;
				break;
                
			case CPU_MODEL_YONAH: 
				DefaultType = MacBook11;
				break;
                
			case CPU_MODEL_MEROM: 
				DefaultType = MacBook21;
				break;
                
			case CPU_MODEL_PENRYN:
				if (gGraphics.Vendor == Nvidia)
				{
					DefaultType = MacBookPro51;
				} else
					DefaultType = MacBook41;
				break;
                
			case CPU_MODEL_JAKETOWN:
			case CPU_MODEL_SANDY_BRIDGE: 
				if((AsciiStrStr(gCPUStructure.BrandString, "i3")) || 
				   (AsciiStrStr(gCPUStructure.BrandString, "i5")))
				{
					DefaultType = MacBookPro81;
					break;
				}
                DefaultType = MacBookPro83;
				break;
                
            case CPU_MODEL_IVY_BRIDGE:
            case CPU_MODEL_IVY_BRIDGE_E5:
                DefaultType = MacBookAir52;
				break;

			default:
				if (gGraphics.Vendor == Nvidia)
				{
					DefaultType = MacBookPro51;
				} else
					DefaultType = MacBook52;
				break;
		}
	}
	else // if(gMobile==FALSE)
	{
		switch (gCPUStructure.Model)
		{
			case CPU_MODEL_CELERON:

				DefaultType = MacMini21;
				break;
	
			case CPU_MODEL_LINCROFT:
				DefaultType = MacMini21;
				break;
			case CPU_MODEL_ATOM:
				DefaultType = MacMini21;
				break;
			case CPU_MODEL_MEROM:
				DefaultType = iMac81;
				break;
			case CPU_MODEL_PENRYN:	
				DefaultType = MacPro31;//speedstep without patching; Hapertown is also a Penryn, according to Wikipedia
				break;
			case CPU_MODEL_NEHALEM:
				DefaultType = MacPro41;
				break;
			case CPU_MODEL_NEHALEM_EX:
				DefaultType = MacPro41;
				break;
			case CPU_MODEL_FIELDS:
				DefaultType = iMac112;
				break;
			case CPU_MODEL_DALES: 
				DefaultType = iMac112;
				break;
			case CPU_MODEL_CLARKDALE:
				DefaultType = iMac112;
				break;
			case CPU_MODEL_WESTMERE:
				DefaultType = MacPro51;
				break;
			case CPU_MODEL_WESTMERE_EX:
				DefaultType = MacPro51;
				break;
			case CPU_MODEL_SANDY_BRIDGE:
                if (gGraphics.Vendor == Intel) DefaultType = MacMini51;
				if((AsciiStrStr(gCPUStructure.BrandString, "i3")) || 
				   (AsciiStrStr(gCPUStructure.BrandString, "i5")))
				{
					DefaultType = iMac112;
					break;
				}
				if(AsciiStrStr(gCPUStructure.BrandString, "i7"))
				{
					DefaultType = iMac121;
					break;
				}
                DefaultType = MacPro51;
                break;

            case CPU_MODEL_IVY_BRIDGE:
            case CPU_MODEL_IVY_BRIDGE_E5:
                DefaultType = iMac122;  //do not make 13,1 by default because of OS 10.8.2 doesn't know it
			case CPU_MODEL_JAKETOWN:
				DefaultType = MacPro41;
				break;
			default:
				DefaultType = MacPro31;
				break;
		}
	}	
	return DefaultType;
    
}

VOID DumpCPU()
{
	Print(L"\nCPU:	BrandString - %a\n", gCPUStructure.BrandString);
	Print(L"Vendor/Model/ExtModel: 0x%x/0x%x/0x%x\n", gCPUStructure.Vendor,  gCPUStructure.Model, gCPUStructure.Extmodel);
	Print(L"Family/ExtFamily:      0x%x/0x%x\n", gCPUStructure.Family,  gCPUStructure.Extfamily);
	Print(L"Features: 0x%08x\n", gCPUStructure.Features);
	Print(L"Cores: %d\n", gCPUStructure.Cores);
    Print(L"Threads: %d\n", gCPUStructure.Threads);
    Print(L"ExternalClock: %d MHz\n", gCPUStructure.ExternalClock);
	Print(L"CurrentSpeed:  %d\n", gCPUStructure.CurrentSpeed);
	Print(L"FSB: %lld Hz\n", gCPUStructure.FSBFrequency);
	Print(L"TSC: %lld Hz\n", gCPUStructure.TSCFrequency);
	Print(L"CPU: %lld Hz\n", gCPUStructure.CPUFrequency);
	Print(L"ProcessorInterconnectSpeed: %d MHz\n", gCPUStructure.ProcessorInterconnectSpeed);
	Print(L"TSCFreq: %lld KHz\n", measure_tsc_frequency());
	Pause(NULL);Pause(NULL);
}

