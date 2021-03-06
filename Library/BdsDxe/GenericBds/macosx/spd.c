/*
 * spd.c - serial presence detect memory information
 implementation for reading memory spd

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 *
 * Originally restored from pcefi10.5 by netkas
 * Dynamic mem detection original impl. by Rekursor
 * System profiler fix and other fixes by Mozodojo.
 * Slice 2011  remade for UEFI
 */

//
#include "macosx.h"
#include "spd.h"
#include "memvendors.h"
#include "Library/IoLib.h"

#define UNKNOWN_MEM_TYPE 2
#define SMBHSTSTS 0
#define SMBHSTCNT 2
#define SMBHSTCMD 3
#define SMBHSTADD 4
#define SMBHSTDAT 5
#define SBMBLKDAT 7
#define SPD_INDEXES_SIZE (sizeof(spd_indexes) / sizeof(INT8))
#define SPD_TO_SMBIOS_SIZE (sizeof(spd_mem_to_smbios)/sizeof(UINT8))
#define READ_SPD(spd, base, slot, x) spd[x] = smb_read_byte_intel(base, 0x50 + slot, x)
#define SMST(a) ((UINT8)((spd[a] & 0xf0) >> 4))
#define SLST(a) ((UINT8)(spd[a] & 0x0f))
#define PCI_COMMAND_OFFSET                          0x04

CHAR8 *spd_memory_types[] =
{
	"RAM",          /* 00h  Undefined */
	"FPM",          /* 01h  FPM */
	"EDO",          /* 02h  EDO */
	"",				/* 03h  PIPELINE NIBBLE */
	"SDRAM",        /* 04h  SDRAM */
	"",				/* 05h  MULTIPLEXED ROM */
	"DDR SGRAM",	/* 06h  SGRAM DDR */
	"DDR SDRAM",	/* 07h  SDRAM DDR */
	"DDR2 SDRAM",   /* 08h  SDRAM DDR 2 */
	"",				/* 09h  Undefined */
	"",				/* 0Ah  Undefined */
	"DDR3 SDRAM"	/* 0Bh  SDRAM DDR 3 */
};
UINT8 spd_mem_to_smbios[] =
{
	UNKNOWN_MEM_TYPE,		/* 00h  Undefined */
	UNKNOWN_MEM_TYPE,		/* 01h  FPM */
	UNKNOWN_MEM_TYPE,		/* 02h  EDO */
	UNKNOWN_MEM_TYPE,		/* 03h  PIPELINE NIBBLE */
	SMB_MEM_TYPE_SDRAM,		/* 04h  SDRAM */
	SMB_MEM_TYPE_ROM,		/* 05h  MULTIPLEXED ROM */
	SMB_MEM_TYPE_SGRAM,		/* 06h  SGRAM DDR */
	SMB_MEM_TYPE_DDR,		/* 07h  SDRAM DDR */
	SMB_MEM_TYPE_DDR2,		/* 08h  SDRAM DDR 2 */
	UNKNOWN_MEM_TYPE,		/* 09h  Undefined */
	UNKNOWN_MEM_TYPE,		/* 0Ah  Undefined */
	SMB_MEM_TYPE_DDR3		/* 0Bh  SDRAM DDR 3 */
};
UINT8 spd_indexes[] = {
	SPD_MEMORY_TYPE,
	SPD_DDR3_MEMORY_BANK,
	SPD_DDR3_MEMORY_CODE,
	SPD_NUM_ROWS,
	SPD_NUM_COLUMNS,
	SPD_NUM_DIMM_BANKS,
	SPD_NUM_BANKS_PER_SDRAM,
	4,7,8,9,12,64, /* TODO: give names to these values */
	95,96,97,98, 122,123,124,125 /* UIS */
};
INTN mapping []= {0,2,1,3,4,6,5,7,8,10,9,11};

/** Read one byte from the intel i2c, used for reading SPD on intel chipsets only. */

UINT8 smb_read_byte_intel(UINT32 base, UINT8 adr, UINT8 cmd)
{
 //   INTN l1, h1, l2, h2;
    UINT64 t, t1, t2;
	
    IoWrite8(base + SMBHSTSTS, 0x1f);					// reset SMBus Controller
    IoWrite8(base + SMBHSTDAT, 0xff);
	
    t1 = AsmReadTsc(); //rdtsc(l1, h1);
    while ( IoRead8(base + SMBHSTSTS) & 0x01)    // wait until read
    {  
     t2 = AsmReadTsc(); //rdtsc(l2, h2);
     t = DivU64x32((t2 - t1), DivU64x32(gCPUStructure.TSCFrequency, 100));
     if (t > 5)
      return 0xFF;                  // break
    }
	
    IoWrite8(base + SMBHSTCMD, cmd);
    IoWrite8(base + SMBHSTADD, (adr << 1) | 0x01 );
    IoWrite8(base + SMBHSTCNT, 0x48 );
	
    t1 = AsmReadTsc();
	
 	while (!( IoRead8(base + SMBHSTSTS) & 0x02))		// wait til command finished
	{	
		t2 = AsmReadTsc();
		t = DivU64x32(t2 - t1, DivU64x32(gCPUStructure.TSCFrequency, 100));
		if (t > 5)
			break;									// break after 5ms
    }
    return IoRead8(base + SMBHSTDAT);
}

/** Read from spd *used* values only*/
VOID init_spd(UINT8* spd, UINT32 base, UINT8 slot)
{
	INTN i;
	for (i=0; i< SPD_INDEXES_SIZE; i++) 
		READ_SPD(spd, base, slot, spd_indexes[i]);
	
}

/** Get Vendor Name from spd, 2 cases handled DDR3 and DDR2, 
    have different formats, always return a valid ptr.*/
CHAR8* getVendorName(RAM_SLOT_INFO* slot, UINT32 base, UINT8 slot_num)
{
    UINT8 bank = 0;
    UINT8 code = 0;
    INTN  i = 0;
    UINT8 * spd = (UINT8 *) slot->spd;

    if (spd[SPD_MEMORY_TYPE]==SPD_MEMORY_TYPE_SDRAM_DDR3) { // DDR3
        bank = (spd[SPD_DDR3_MEMORY_BANK] & 0x07f); // constructors like Patriot use b7=1
        code = spd[SPD_DDR3_MEMORY_CODE];
        for (i=0; i < VEN_MAP_SIZE; i++)
            if (bank==vendorMap[i].bank && code==vendorMap[i].code)
                return vendorMap[i].name;
    }
    else if (spd[SPD_MEMORY_TYPE]==SPD_MEMORY_TYPE_SDRAM_DDR2) {
        if(spd[64]==0x7f) {
            for (i=64; i<72 && spd[i]==0x7f;i++) {
			  bank++;
			  READ_SPD(spd, base, slot_num,i+1); // prefetch next spd byte to read for next loop
			}
			READ_SPD(spd, base, slot_num,i);
            code = spd[i];
        } else {
            code = spd[64]; 
            bank = 0;
        }
        for (i=0; i < VEN_MAP_SIZE; i++)
            if (bank==vendorMap[i].bank && code==vendorMap[i].code)
                return vendorMap[i].name;
    }
    /* OK there is no vendor id here lets try to match the partnum if it exists */
    if (AsciiStrStr(slot->PartNo,"GU332") == slot->PartNo) // Unifosa fingerprint
        return "Unifosa";
    return "NoName";
}

/** Get Default Memory Module Speed (no overclocking handled) */
UINT16 getDDRspeedMhz(UINT8 * spd)
{
    if (spd[SPD_MEMORY_TYPE]==SPD_MEMORY_TYPE_SDRAM_DDR3) { 
        switch(spd[12])  {
        case 0x0f:
            return 1066;
        case 0x0c:
            return 1333;
        case 0x0a:
            return 1600;
        case 0x14:
        default:
            return 800;
        }
    } 
    else if (spd[SPD_MEMORY_TYPE]==SPD_MEMORY_TYPE_SDRAM_DDR2)  {
        switch(spd[9]) {
        case 0x50:
            return 400;
        case 0x3d:
            return 533;
        case 0x30:
            return 667;
        case 0x25:
        default:
            return 800;
        }
    }
    return  800; // default freq for unknown types //shit! DDR1 = 533
}

/** Get DDR3 or DDR2 serial number, 0 most of the times, always return a valid ptr */
CHAR8* getDDRSerial(UINT8* spd)
{
    CHAR8* asciiSerial; //[16];
    asciiSerial = AllocatePool(16);
    if (spd[SPD_MEMORY_TYPE]==SPD_MEMORY_TYPE_SDRAM_DDR3) // DDR3
    {
	AsciiSPrint(asciiSerial, 16, "%X%X%X%X%X%X%X%X", SMST(122) /*& 0x7*/, SLST(122), SMST(123), SLST(123), SMST(124), SLST(124), SMST(125), SLST(125));
    }
    else if (spd[SPD_MEMORY_TYPE]==SPD_MEMORY_TYPE_SDRAM_DDR2) // DDR2 or DDR
    { 
	AsciiSPrint(asciiSerial, 16, "%X%X%X%X%X%X%X%X", SMST(95) /*& 0x7*/, SLST(95), SMST(96), SLST(96), SMST(97), SLST(97), SMST(98), SLST(98));
    }

    return asciiSerial;
}

/** Get DDR3 or DDR2 Part Number, always return a valid ptr */
CHAR8* getDDRPartNum(UINT8* spd, UINT32 base, INTN slot)
{
	CHAR8* asciiPartNo; //[32];
	INTN i, start=0, index = 0;
	CHAR8 c;

	asciiPartNo = AllocatePool(32);

    if (spd[SPD_MEMORY_TYPE]==SPD_MEMORY_TYPE_SDRAM_DDR3) {
		start = 128;
	}
    else if (spd[SPD_MEMORY_TYPE]==SPD_MEMORY_TYPE_SDRAM_DDR2) {
		start = 73;
	}
	
    // Check that the spd part name is zero terminated and that it is ascii:
    ZeroMem(asciiPartNo, 32);  //sizeof(asciiPartNo));
	for (i=start; i < start + 32; i++) {
		READ_SPD(spd, base, slot, i); // only read once the corresponding model part (ddr3 or ddr2)
		c = spd[i];
		if (IS_ALFA(c) || IS_DIGIT(c) || IS_PUNCT(c)) // It seems that System Profiler likes only letters and digits...
			asciiPartNo[index++] = c;
		else if (c < 0x20)
			break;
	}
	
	return asciiPartNo;
}

/** Read from smbus the SPD content and interpret it for detecting memory attributes */
VOID read_smb_intel(EFI_PCI_IO_PROTOCOL *PciIo)
{ 
	EFI_STATUS		Status;
  INTN			i, speed;
  UINT8			spd_size, spd_type;
  UINT32			base, mmio, hostc;
	UINT16			Command;
  RAM_SLOT_INFO*  slot;
	BOOLEAN			fullBanks;
	UINT8*			spdbuf;
	UINT16			vid, did;
	
	vid = gPci.Hdr.VendorId;
	did = gPci.Hdr.DeviceId;
	Status = PciIo->Pci.Read (
							  PciIo, 
							  EfiPciIoWidthUint16, 
							  PCI_COMMAND_OFFSET, 
							  1, 
							  &Command
							  );
	Command |= 1;	
	Status = PciIo->Pci.Write (
							 PciIo, 
							 EfiPciIoWidthUint16, 
							 PCI_COMMAND_OFFSET, 
							 1, 
							 &Command
							 );
  Status = PciIo->Pci.Read (
							  PciIo,
							  EfiPciIoWidthUint32,
							  0x10,
							  1,
							  &mmio
							  );
	Status = PciIo->Pci.Read (
							  PciIo,
							  EfiPciIoWidthUint32,
							  0x20,
							  1,
							  &base
							  );
  base &= 0xFFFE;	
	Status = PciIo->Pci.Read (
							  PciIo,
							  EfiPciIoWidthUint32,
							  0x40,
							  1,
							  &hostc
							  );
  fullBanks = (gDMI->MemoryModules == gDMI->CntMemorySlots);
	spdbuf = AllocateZeroPool(MAX_SPD_SIZE);
    // Search MAX_RAM_SLOTS slots
    for (i = 0; i <  MAX_RAM_SLOTS; i++){
        slot = &gRAM->DIMM[i];
        spd_size = smb_read_byte_intel(base, 0x50 + i, 0);
        // Check spd is present
        if (spd_size && (spd_size != 0xff))
        {
          slot->spd = spdbuf;
          slot->InUse = TRUE;
          ZeroMem(slot->spd, spd_size);          
          // Copy spd data into buffer
          init_spd(slot->spd, base, i);
          switch (slot->spd[SPD_MEMORY_TYPE])  {
          case SPD_MEMORY_TYPE_SDRAM_DDR2:
              slot->ModuleSize = ((1 << ((slot->spd[SPD_NUM_ROWS] & 0x0f) + (slot->spd[SPD_NUM_COLUMNS] & 0x0f) - 17)) * ((slot->spd[SPD_NUM_DIMM_BANKS] & 0x7) + 1) * slot->spd[SPD_NUM_BANKS_PER_SDRAM]);
              break;
          case SPD_MEMORY_TYPE_SDRAM_DDR3:
              slot->ModuleSize = ((slot->spd[4] & 0x0f) + 28 ) + ((slot->spd[8] & 0x7)  + 3 );
              slot->ModuleSize -= (slot->spd[7] & 0x7) + 25;
              slot->ModuleSize = ((1 << slot->ModuleSize) * (((slot->spd[7] >> 3) & 0x1f) + 1));                
              break;
          }            
          spd_type = (slot->spd[SPD_MEMORY_TYPE] < ((UINT8) 12) ? slot->spd[SPD_MEMORY_TYPE] : 0);
          slot->Type = spd_mem_to_smbios[spd_type];
          slot->PartNo = getDDRPartNum(slot->spd, base, i);
          slot->Vendor = getVendorName(slot, base, i);
          slot->SerialNo = getDDRSerial(slot->spd);
          speed = getDDRspeedMhz(slot->spd);
          if (slot->Frequency<speed) slot->Frequency = speed;
          // pci memory controller if available, is more reliable
          if (gRAM->Frequency > 0) {
            UINT32 freq = (UINT32)DivU64x32(gRAM->Frequency, 500000);
            // now round off special cases
            UINT32 fmod100 = freq %100;
            switch(fmod100) {
              case  1:	freq--;	break;
              case 32:	freq++;	break;
              case 65:	freq++; break;
              case 98:	freq+=2;break;
              case 99:	freq++; break;
            }
            slot->Frequency = freq;
          }
          // laptops sometimes show slot 0 and 2 with slot 1 empty when only 2 slots are presents so:
          gDMI->DIMM[i]= (i>0 && gRAM->DIMM[1].InUse==FALSE && fullBanks && gDMI->CntMemorySlots == 2)?mapping[i] : i; // for laptops case, mapping setup would need to be more generic than this
          slot->spd = NULL;
        }
    } // for
}

/*
static struct smbus_controllers_t smbus_controllers[] = {

	{0x8086, 0x269B, "ESB2",		read_smb_intel },
	{0x8086, 0x25A4, "6300ESB",		read_smb_intel },
	{0x8086, 0x24C3, "ICH4",		read_smb_intel },
	{0x8086, 0x24D3, "ICH5",		read_smb_intel },
	{0x8086, 0x266A, "ICH6",		read_smb_intel },
	{0x8086, 0x27DA, "ICH7",		read_smb_intel },
	{0x8086, 0x283E, "ICH8",		read_smb_intel },
	{0x8086, 0x2930, "ICH9",		read_smb_intel },	
	{0x8086, 0x3A30, "ICH10R",		read_smb_intel },
	{0x8086, 0x3A60, "ICH10B",		read_smb_intel },
	{0x8086, 0x3B30, "5 Series",	read_smb_intel },
	{0x8086, 0x1C22, "6 Series",	read_smb_intel },
	{0x8086, 0x5032, "EP80579",		read_smb_intel }

};
*/

VOID ScanSPD()
{
	EFI_STATUS			Status;
	EFI_HANDLE			*HandleBuffer;
	EFI_GUID			**ProtocolGuidArray;
	EFI_PCI_IO_PROTOCOL *PciIo;
	UINTN				HandleCount;
	UINTN				ArrayCount;
	UINTN				HandleIndex;
	UINTN				ProtocolIndex;

	/* Scan PCI BUS For SmBus controller */
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
							/* Read PCI BUS */
							Status = PciIo->Pci.Read (
													  PciIo,
													  EfiPciIoWidthUint32,
													  0,
													  sizeof (gPci) / sizeof (UINT32),
													  &gPci
													  );
							
							//SmBus controller has class = 0x0c0500
							if ((gPci.Hdr.ClassCode[2] == 0x0c) && (gPci.Hdr.ClassCode[1] == 5) 
								&& (gPci.Hdr.ClassCode[0] == 0) && (gPci.Hdr.VendorId == 0x8086))
							{
								read_smb_intel(PciIo);
							}							
						}
					}
				}
			}
		}
	}
}

