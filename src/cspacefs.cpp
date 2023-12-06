#include <new>
#include <efi.h>
#include "quibbleproto.h"

#define UNUSED(x) (void)(x)
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) < (b) ? (b) : (a))

struct volume
{
	~volume();

	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL proto;
	EFI_QUIBBLE_PROTOCOL quibble_proto;
	EFI_HANDLE controller;
	EFI_BLOCK_IO_PROTOCOL* block;
	EFI_DISK_IO_PROTOCOL* disk_io;
	unsigned long sectorsize;
	unsigned long tablesize;
	unsigned long long extratablesize;
	char* table;
	unsigned long long filenamesend;
	unsigned long long tableend;
	char* tablestr;
	unsigned long long tablestrlen;
	unsigned long long filecount;
};

struct inode
{
	inode(volume& vol) : vol(vol) { }
	~inode();

	EFI_FILE_PROTOCOL proto;
	volume& vol;
	unsigned long long index = 0;
	unsigned long long size = 0;
	unsigned long long pos = 0;
};

static EFI_SYSTEM_TABLE* systable;
static EFI_BOOT_SERVICES* bs;
static EFI_DRIVER_BINDING_PROTOCOL drvbind;

static void populate_file_handle(EFI_FILE_PROTOCOL* proto);

static CHAR16* HEX(unsigned long long i)
{
	switch (i)
	{
	case 0:
		return (CHAR16*)L"0";
	case 1:
		return (CHAR16*)L"1";
	case 2:
		return (CHAR16*)L"2";
	case 3:
		return (CHAR16*)L"3";
	case 4:
		return (CHAR16*)L"4";
	case 5:
		return (CHAR16*)L"5";
	case 6:
		return (CHAR16*)L"6";
	case 7:
		return (CHAR16*)L"7";
	case 8:
		return (CHAR16*)L"8";
	case 9:
		return (CHAR16*)L"9";
	case 10:
		return (CHAR16*)L"A";
	case 11:
		return (CHAR16*)L"B";
	case 12:
		return (CHAR16*)L"C";
	case 13:
		return (CHAR16*)L"D";
	case 14:
		return (CHAR16*)L"E";
	case 15:
		return (CHAR16*)L"F";
	default:
		return (CHAR16*)L"";
	}
}

static unsigned toint(unsigned char c)
{
	switch (c)
	{
	case '0':
		return 0;
	case '1':
		return 1;
	case '2':
		return 2;
	case '3':
		return 3;
	case '4':
		return 4;
	case '5':
		return 5;
	case '6':
		return 6;
	case '7':
		return 7;
	case '8':
		return 8;
	case '9':
		return 9;
	default:
		return 0;
	}
}

static bool incmp(unsigned char a, unsigned char b)
{
	if (a >= 'A' && a <= 'Z')
	{
		a += 32;
	}
	if (b >= 'A' && b <= 'Z')
	{
		b += 32;
	}
	return a == b;
}

static void do_print_error(CHAR16* func, EFI_STATUS Status)
{
	systable->ConOut->OutputString(systable->ConOut, (CHAR16*)L"Error in ");
	systable->ConOut->OutputString(systable->ConOut, func);
	systable->ConOut->OutputString(systable->ConOut, (CHAR16*)L": ");
	systable->ConOut->OutputString(systable->ConOut, (CHAR16*)L"0x");
	systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0xF000000000000000) >> 60));
	systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x0F00000000000000) >> 56));
	systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x00F0000000000000) >> 52));
	systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x000F000000000000) >> 48));
	systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x0000F00000000000) >> 44));
	systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x00000F0000000000) >> 40));
	systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x000000F000000000) >> 36));
	systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x0000000F00000000) >> 32));
	systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x00000000F0000000) >> 28));
	systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x000000000F000000) >> 24));
	systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x0000000000F00000) >> 20));
	systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x00000000000F0000) >> 16));
	systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x000000000000F000) >> 12));
	systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x0000000000000F00) >> 8));
	systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x00000000000000F0) >> 4));
	systable->ConOut->OutputString(systable->ConOut, HEX(Status & 0x000000000000000F));
	systable->ConOut->OutputString(systable->ConOut, (CHAR16*)L".\r\n");
}

static void win_time_to_efi(int64_t win, EFI_TIME* efi)
{
	int64_t secs, time, days;

	secs = win / 10000000;
	time = secs % 86400;
	days = secs / 86400;

	unsigned int jd = 2305814 + days; // Julian date

	unsigned int f = jd + 1401 + (((((4 * jd) + 274277) / 146097) * 3) / 4) - 38;
	unsigned int e = (4 * f) + 3;
	unsigned int g = (e % 1461) / 4;
	unsigned int h = (5 * g) + 2;

	efi->Month = (((h / 153) + 2) % 12) + 1;
	efi->Year = (e / 1461) - 4716 + ((14 - efi->Month) / 12);
	efi->Day = ((h % 153) / 5) + 1;
	efi->Hour = time / 3600;
	efi->Minute = (time % 3600) / 60;
	efi->Second = time % 60;
	efi->Pad1 = 0;
	efi->Nanosecond = (win % 10000000) * 100;
	efi->TimeZone = 0;
	efi->Daylight = 0;
	efi->Pad2 = 0;
}

static void ATTRtoattr(unsigned long& ATTR)
{
	unsigned long attr = 0;
	if (ATTR & 32768) attr |= EFI_FILE_HIDDEN;
	if (ATTR & 4096) attr |= EFI_FILE_READ_ONLY;
	if (ATTR & 128) attr |= EFI_FILE_SYSTEM;
	if (ATTR & 2048) attr |= EFI_FILE_ARCHIVE;
	if (ATTR & 8192) attr |= EFI_FILE_DIRECTORY;
	//if (ATTR & 1024) attr |= EFI_FILE_REPARSE_POINT;
	ATTR = attr;
}

extern "C"
void* memcpy(void* dest, const void* src, size_t n) {
	void* orig_dest = dest;

#if __INTPTR_WIDTH__ == 64
	while (n >= sizeof(uint64_t)) {
		*(uint64_t*)dest = *(uint64_t*)src;

		dest = (uint8_t*)dest + sizeof(uint64_t);
		src = (uint8_t*)src + sizeof(uint64_t);

		n -= sizeof(uint64_t);
	}
#endif

	while (n >= sizeof(uint32_t)) {
		*(uint32_t*)dest = *(uint32_t*)src;

		dest = (uint8_t*)dest + sizeof(uint32_t);
		src = (uint8_t*)src + sizeof(uint32_t);

		n -= sizeof(uint32_t);
	}

	while (n >= sizeof(uint16_t)) {
		*(uint16_t*)dest = *(uint16_t*)src;

		dest = (uint8_t*)dest + sizeof(uint16_t);
		src = (uint8_t*)src + sizeof(uint16_t);

		n -= sizeof(uint16_t);
	}

	while (n >= sizeof(uint8_t)) {
		*(uint8_t*)dest = *(uint8_t*)src;

		dest = (uint8_t*)dest + sizeof(uint8_t);
		src = (uint8_t*)src + sizeof(uint8_t);

		n -= sizeof(uint8_t);
	}

	return orig_dest;
}

volume::~volume()
{
	bs->FreePool(table);
	bs->FreePool(tablestr);
}

inode::~inode()
{

}

static EFI_STATUS drv_supported(EFI_DRIVER_BINDING_PROTOCOL* This, EFI_HANDLE ControllerHandle, EFI_DEVICE_PATH_PROTOCOL* RemainingDevicePath)
{
	EFI_STATUS Status;
	EFI_DISK_IO_PROTOCOL* disk_io;
	EFI_GUID guid_disk = EFI_DISK_IO_PROTOCOL_GUID;
	EFI_GUID guid_block = EFI_BLOCK_IO_PROTOCOL_GUID;

	UNUSED(RemainingDevicePath);

	Status = bs->OpenProtocol(ControllerHandle, &guid_disk, (void**)&disk_io, This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);

	if (EFI_ERROR(Status))
	{
		return Status;
	}

	bs->CloseProtocol(ControllerHandle, &guid_disk, This->DriverBindingHandle, ControllerHandle);

	return bs->OpenProtocol(ControllerHandle, &guid_block, 0, This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
}

static unsigned long long getfilenameindex(CHAR16* FileName, volume& vol)
{
	unsigned long long loc = 0;
	unsigned FileNameLen = 0;

	for (; FileName[FileNameLen] != 0; FileNameLen++);
	if (FileNameLen == 0)
	{
		return 0;
	}

	unsigned j = 0;
	bool found = false;
	bool start = true;
	for (unsigned long long i = 0; i < vol.filecount + 1; i++)
	{
		for (; loc < vol.filenamesend - vol.tableend + 1; loc++)
		{
			if (((vol.table[vol.tableend + loc] & 0xff) == 255) || ((vol.table[vol.tableend + loc] & 0xff) == 42)) // 255 = file, 42 = fuse symlink
			{
				found = (j == FileNameLen);
				j = 0;
				if (found)
				{
					return i - 1;
				}
				start = true;
				if ((vol.table[vol.tableend + loc] & 0xff) == 255)
				{
					loc++;
					break;
				}
			}
			if ((incmp((vol.table[vol.tableend + loc] & 0xff), (FileName[j] & 0xff)) || (((vol.table[vol.tableend + loc] & 0xff) == *"/") && ((FileName[j] & 0xff) == *"\\"))) && start) // case insensitive, / and \ are the same, make sure it is not just an end or middle of filename
			{
				j++;
			}
			else
			{
				if ((vol.table[vol.tableend + loc] & 0xff) != 42)
				{
					start = false;
				}
				j = 0;
			}
		}
	}
	return 0;
}

static EFI_STATUS EFIAPI file_open(struct _EFI_FILE_HANDLE* File, struct _EFI_FILE_HANDLE** NewHandle, CHAR16* FileName, UINT64 OpenMode, UINT64 Attributes)
{
	UNUSED(Attributes);

	if (OpenMode & EFI_FILE_MODE_CREATE)
	{
		return EFI_UNSUPPORTED;
	}

	EFI_STATUS Status;
	inode* ino = _CR(File, inode, proto);
	inode* file;

	Status = bs->AllocatePool(EfiBootServicesData, sizeof(inode), (void**)&file);

	if (EFI_ERROR(Status))
	{
		do_print_error((CHAR16*)L"AllocatePool 0", Status);
		return Status;
	}

	new (file) inode(ino->vol);

	populate_file_handle(&file->proto);

	file->index = getfilenameindex(FileName, file->vol);
	file->size = 0;
	file->pos = 0;

	unsigned long long loc = 0;
	for (unsigned long long i = 0; i < file->vol.tablestrlen; i++)
	{
		if (loc == file->index)
		{
			loc = i;
			break;
		}
		if (file->vol.tablestr[i] == *".")
		{
			loc++;
		}
	}

	bool notzero = false;
	unsigned cur = 0;
	unsigned long long int0 = 0;
	unsigned long long int1 = 0;
	unsigned long long int2 = 0;

	for (unsigned long long i = loc; i < file->vol.tablestrlen; i++)
	{
		if (file->vol.tablestr[i] == *"," || file->vol.tablestr[i] == *".")
		{
			if (notzero)
			{
				switch (cur)
				{
				case 0:
					file->size += file->vol.sectorsize;
					break;
				case 1:
					break;
				case 2:
					file->size += int2 - int1;
					break;
				}
			}
			cur = 0;
			int0 = 0;
			int1 = 0;
			int2 = 0;
			if (file->vol.tablestr[i] == *".")
			{
				break;
			}
		}
		else if (file->vol.tablestr[i] == *";")
		{
			cur++;
		}
		else
		{
			notzero = true;
			if (cur == 0)
			{
				int0 += toint(file->vol.tablestr[i] & 0xff);
				if (file->vol.tablestr[i + 1] != *";" && file->vol.tablestr[i + 1] != *"," && file->vol.tablestr[i + 1] != *".")
				{
					int0 *= 10;
				}
			}
			else if (cur == 1)
			{
				int1 += toint(file->vol.tablestr[i] & 0xff);
				if (file->vol.tablestr[i + 1] != *";" && file->vol.tablestr[i + 1] != *"," && file->vol.tablestr[i + 1] != *".")
				{
					int1 *= 10;
				}
			}
			else if (cur == 2)
			{
				int2 += toint(file->vol.tablestr[i] & 0xff);
				if (file->vol.tablestr[i + 1] != *";" && file->vol.tablestr[i + 1] != *"," && file->vol.tablestr[i + 1] != *".")
				{
					int2 *= 10;
				}
			}
		}
	}

	*NewHandle = &file->proto;

	return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI file_close(struct _EFI_FILE_HANDLE* File)
{
	inode* file = _CR(File, inode, proto);

	file->inode::~inode();

	return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI file_delete(struct _EFI_FILE_HANDLE* File)
{
	UNUSED(File);

	return EFI_UNSUPPORTED;
}

static EFI_STATUS EFIAPI file_read(struct _EFI_FILE_HANDLE* File, UINTN* BufferSize, VOID* Buffer)
{
	inode* file = _CR(File, inode, proto);
	
	do_print_error((CHAR16*)L"file_read", file->index);//

	// FIXME - read from disk

	return EFI_UNSUPPORTED;
}

static EFI_STATUS EFIAPI file_write(struct _EFI_FILE_HANDLE* File, UINTN* BufferSize, VOID* Buffer)
{
	UNUSED(File);
	UNUSED(BufferSize);
	UNUSED(Buffer);

	return EFI_UNSUPPORTED;
}

static EFI_STATUS EFIAPI file_set_position(struct _EFI_FILE_HANDLE* File, UINT64 Position)
{
	inode* file = _CR(File, inode, proto);

	if (Position == 0xFFFFFFFFFFFFFFFF)
	{
		file->pos = file->size;
	}
	else
	{
		file->pos = Position;
	}

	return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI file_get_position(struct _EFI_FILE_HANDLE* File, UINT64* Position)
{
	inode* file = _CR(File, inode, proto);

	*Position = file->pos;

	return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI file_get_info(struct _EFI_FILE_HANDLE* File, EFI_GUID* InformationType, UINTN* BufferSize, VOID* Buffer)
{
	inode* file = _CR(File, inode, proto);
	EFI_FILE_INFO* info = (EFI_FILE_INFO*)Buffer;

	info->Size = file->size;
	info->FileSize = file->size;
	info->PhysicalSize = (file->size + file->vol.sectorsize + 1) / file->vol.sectorsize * file->vol.sectorsize;

	double time = 0;
	char tim[8] = { 0 };
	char ti[8] = { 0 };

	memcpy(tim, file->vol.table + file->vol.filenamesend + 2 + file->index * 24, 8);
	for (unsigned i = 0; i < 8; i++)
	{
		ti[i] = tim[7 - i];
	}
	memcpy(&time, ti, 8);
	win_time_to_efi(time * 10000000 + 116444736000000000, &info->LastAccessTime);

	memcpy(tim, file->vol.table + file->vol.filenamesend + 2 + file->index * 24 + 8, 8);
	for (unsigned i = 0; i < 8; i++)
	{
		ti[i] = tim[7 - i];
	}
	memcpy(&time, ti, 8);
	win_time_to_efi(time * 10000000 + 116444736000000000, &info->ModificationTime);

	memcpy(tim, file->vol.table + file->vol.filenamesend + 2 + file->index * 24 + 16, 8);
	for (unsigned i = 0; i < 8; i++)
	{
		ti[i] = tim[7 - i];
	}
	memcpy(&time, ti, 8);
	win_time_to_efi(time * 10000000 + 116444736000000000, &info->CreateTime);

	unsigned long winattrs = (file->vol.table[file->vol.filenamesend + 2 + file->vol.filecount * 24 + file->index * 11 + 7] & 0xff) << 24 | (file->vol.table[file->vol.filenamesend + 2 + file->vol.filecount * 24 + file->index * 11 + 8] & 0xff) << 16 | (file->vol.table[file->vol.filenamesend + 2 + file->vol.filecount * 24 + file->index * 11 + 9] & 0xff) << 8 | (file->vol.table[file->vol.filenamesend + 2 + file->vol.filecount * 24 + file->index * 11 + 10] & 0xff);
	ATTRtoattr(winattrs);
	info->Attribute = winattrs;

	//info->FileName = ;

	return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI file_set_info(struct _EFI_FILE_HANDLE* File, EFI_GUID* InformationType, UINTN BufferSize, VOID* Buffer)
{
	UNUSED(File);
	UNUSED(InformationType);
	UNUSED(BufferSize);
	UNUSED(Buffer);

	return EFI_UNSUPPORTED;
}

static EFI_STATUS file_flush(struct _EFI_FILE_HANDLE* File)
{
	UNUSED(File);

	// nop

	return EFI_SUCCESS;
}

static void populate_file_handle(EFI_FILE_PROTOCOL* proto)
{
	proto->Revision = EFI_FILE_PROTOCOL_REVISION;
	proto->Open = file_open;
	proto->Close = file_close;
	proto->Delete = file_delete;
	proto->Read = file_read;
	proto->Write = file_write;
	proto->GetPosition = file_get_position;
	proto->SetPosition = file_set_position;
	proto->GetInfo = file_get_info;
	proto->SetInfo = file_set_info;
	proto->Flush = file_flush;
}

static EFI_STATUS EFIAPI open_volume(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* This, EFI_FILE_PROTOCOL** Root)
{
	EFI_STATUS Status;
	volume* vol = _CR(This, volume, proto);
	inode* ino;

	Status = bs->AllocatePool(EfiBootServicesData, sizeof(inode), (void**)&ino);

	if (EFI_ERROR(Status))
	{
		do_print_error((CHAR16*)L"AllocatePool 1", Status);
		return Status;
	}

	new (ino) inode(*vol);

	populate_file_handle(&ino->proto);

	ino->index = getfilenameindex((CHAR16*)L"/", ino->vol);

	*Root = &ino->proto;

	return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI get_arc_name(EFI_QUIBBLE_PROTOCOL* This, char* ArcName, UINTN* ArcnameLen)
{
	UNUSED(This);
	UNUSED(ArcName);
	UNUSED(ArcnameLen);

	return EFI_UNSUPPORTED;
}

static EFI_STATUS get_driver_name(EFI_QUIBBLE_PROTOCOL* This, CHAR16* DriverName, UINTN* DriverNameLen)
{
	static const char16_t name[] = u"cspacefs";

	UNUSED(This);

	if (*DriverNameLen < sizeof(name) / 2)
	{
		*DriverNameLen = sizeof(name) / 2;
		return EFI_BUFFER_TOO_SMALL;
	}

	*DriverNameLen = sizeof(name) / 2;

	for (unsigned long long i = 0; i < sizeof(name) / 2; i++)
	{
		DriverName[i] = name[i];
	}

	return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI drv_start(EFI_DRIVER_BINDING_PROTOCOL* This, EFI_HANDLE ControllerHandle, EFI_DEVICE_PATH_PROTOCOL* RemainingDevicePath)
{
	UNUSED(RemainingDevicePath);

	EFI_STATUS Status;
	EFI_GUID disk_guid = EFI_DISK_IO_PROTOCOL_GUID;
	EFI_GUID block_guid = EFI_BLOCK_IO_PROTOCOL_GUID;
	EFI_GUID fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_GUID quibble_guid = EFI_QUIBBLE_PROTOCOL_GUID;
	EFI_BLOCK_IO_PROTOCOL* block;
	EFI_DISK_IO_PROTOCOL* disk_io;
	volume* vol;

	Status = bs->OpenProtocol(ControllerHandle, &block_guid, (void**)&block, This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);

	if (EFI_ERROR(Status))
	{
		do_print_error((CHAR16*)L"OpenProtocol(block)", Status);
		return Status;
	}

	if (block->Media->BlockSize == 0)
	{
		bs->CloseProtocol(ControllerHandle, &block_guid, This->DriverBindingHandle, ControllerHandle);
		return EFI_UNSUPPORTED;
	}

	Status = bs->OpenProtocol(ControllerHandle, &disk_guid, (void**)&disk_io, This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);

	if (EFI_ERROR(Status))
	{
		bs->CloseProtocol(ControllerHandle, &block_guid, This->DriverBindingHandle, ControllerHandle);
		return Status;
	}

	// FIXME - DISK_IO 2?

	// Detect if CSpaceFS is present or not.

	char form[512] = { 0 };
	Status = block->ReadBlocks(block, block->Media->MediaId, 0, 512, form);

	if (EFI_ERROR(Status))
	{
		bs->CloseProtocol(ControllerHandle, &disk_guid, This->DriverBindingHandle, ControllerHandle);
		bs->CloseProtocol(ControllerHandle, &block_guid, This->DriverBindingHandle, ControllerHandle);
		return Status;
	}

	if ((form[0] & 0xff) > 23)
	{
		bs->CloseProtocol(ControllerHandle, &disk_guid, This->DriverBindingHandle, ControllerHandle);
		bs->CloseProtocol(ControllerHandle, &block_guid, This->DriverBindingHandle, ControllerHandle);
		return EFI_UNSUPPORTED;
	}

	unsigned long sectorsize = 1 << (9 + (form[0] & 0xff));
	unsigned long tablesize = 1 + (form[4] & 0xff) + ((form[3] & 0xff) << 8) + ((form[2] & 0xff) << 16) + ((form[1] & 0xff) << 24);
	unsigned long long extratablesize = static_cast<unsigned long long>(sectorsize) * tablesize;
	char* table = nullptr;
	unsigned long long filenamesend = 5;
	unsigned long long tableend = 0;
	char* tablestr = nullptr;
	unsigned long long tablestrlen = 0;
	unsigned long long filecount = 0;

	Status = bs->AllocatePool(EfiBootServicesData, extratablesize, (void**)&table);

	if (EFI_ERROR(Status))
	{
		do_print_error((CHAR16*)L"AllocatePool 2", Status);
		bs->CloseProtocol(ControllerHandle, &disk_guid, This->DriverBindingHandle, ControllerHandle);
		bs->CloseProtocol(ControllerHandle, &block_guid, This->DriverBindingHandle, ControllerHandle);
		return Status;
	}

	Status = block->ReadBlocks(block, block->Media->MediaId, 0, extratablesize, table);

	if (EFI_ERROR(Status))
	{
		bs->FreePool(table);
		bs->CloseProtocol(ControllerHandle, &disk_guid, This->DriverBindingHandle, ControllerHandle);
		bs->CloseProtocol(ControllerHandle, &block_guid, This->DriverBindingHandle, ControllerHandle);
		return Status;
	}

	bool found = false;
	unsigned long long loc = 0;

	for (filenamesend = 5; filenamesend < extratablesize; filenamesend++)
	{
		if ((table[filenamesend] & 0xff) == 255)
		{
			if ((table[min(filenamesend + 1, extratablesize)] & 0xff) == 254)
			{
				found = true;
				break;
			}
			else
			{
				if (loc == 0)
				{
					tableend = filenamesend;
				}

				// if following check is not enough to block the driver from loading unnecessarily,
				// then we can add a check to make sure all bytes between loc and i equal a vaild path.
				// if that is not enough, then nothing will be.

				loc = filenamesend;
			}
		}
		if (filenamesend - loc > 256 && loc > 0)
		{
			break;
		}
	}

	if (!found)
	{
		bs->FreePool(table);
		bs->CloseProtocol(ControllerHandle, &disk_guid, This->DriverBindingHandle, ControllerHandle);
		bs->CloseProtocol(ControllerHandle, &block_guid, This->DriverBindingHandle, ControllerHandle);
		return EFI_UNSUPPORTED;
	}

	// if it is continue.

	char* charmap = (char*)"0123456789-,.; ";
	unsigned dmap[256] = { 0 };
	unsigned p = 0;

	for (unsigned i = 0; i < 15; i++)
	{
		for (unsigned o = 0; o < 15; o++)
		{
			dmap[p] = charmap[i] << 8 | charmap[o];
			p++;
		}
	}

	tablestrlen = tableend + tableend - 10;

	Status = bs->AllocatePool(EfiBootServicesData, tablestrlen, (void**)&tablestr);

	if (EFI_ERROR(Status))
	{
		do_print_error((CHAR16*)L"AllocatePool 3", Status);
		bs->FreePool(table);
		bs->CloseProtocol(ControllerHandle, &disk_guid, This->DriverBindingHandle, ControllerHandle);
		bs->CloseProtocol(ControllerHandle, &block_guid, This->DriverBindingHandle, ControllerHandle);
		return Status;
	}

	for (unsigned long long i = 0; i < tableend - 5; i++)
	{
		tablestr[i + i] = (dmap[table[i + 5] & 0xff] >> 8) & 0xff;
		tablestr[i + i + 1] = dmap[table[i + 5] & 0xff] & 0xff;
	}

	for (unsigned long long i = 0; i < tablestrlen; i++)
	{
		if (tablestr[i] == *".")
		{
			filecount++;
		}
	}

	// init volume ^^^

	Status = bs->AllocatePool(EfiBootServicesData, sizeof(volume), (void**)&vol);

	if (EFI_ERROR(Status))
	{
		do_print_error((CHAR16*)L"AllocatePool 4", Status);
		bs->FreePool(table);
		bs->FreePool(tablestr);
		bs->CloseProtocol(ControllerHandle, &disk_guid, This->DriverBindingHandle, ControllerHandle);
		bs->CloseProtocol(ControllerHandle, &block_guid, This->DriverBindingHandle, ControllerHandle);
		return Status;
	}

	vol->proto.Revision = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
	vol->proto.OpenVolume = open_volume;
	vol->controller = ControllerHandle;
	vol->block = block;
	vol->disk_io = disk_io;
	vol->quibble_proto.GetArcName = get_arc_name;
	vol->quibble_proto.GetWindowsDriverName = get_driver_name;

	vol->sectorsize = sectorsize;
	vol->tablesize = tablesize;
	vol->extratablesize = extratablesize;
	vol->table = table;
	vol->filenamesend = filenamesend;
	vol->tableend = tableend;
	vol->tablestr = tablestr;
	vol->tablestrlen = tablestrlen;
	vol->filecount = filecount;

	Status = bs->InstallMultipleProtocolInterfaces(&ControllerHandle, &fs_guid, &vol->proto, &quibble_guid, &vol->quibble_proto, nullptr);

	if (EFI_ERROR(Status))
	{
		do_print_error((CHAR16*)L"InstallMultipleProtocolInterfaces", Status);
		vol->volume::~volume();
		bs->FreePool(vol);
		bs->FreePool(table);
		bs->FreePool(tablestr);
		bs->CloseProtocol(ControllerHandle, &disk_guid, This->DriverBindingHandle, ControllerHandle);
		bs->CloseProtocol(ControllerHandle, &block_guid, This->DriverBindingHandle, ControllerHandle);
		return Status;
	}

	return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI drv_stop(EFI_DRIVER_BINDING_PROTOCOL* This, EFI_HANDLE ControllerHandle, UINTN NumberOfChildren, EFI_HANDLE* ChildHandleBuffer)
{
	UNUSED(This);
	UNUSED(ControllerHandle);
	UNUSED(NumberOfChildren);
	UNUSED(ChildHandleBuffer);

	return EFI_INVALID_PARAMETER;
}

extern "C"
EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
	EFI_STATUS Status;
	EFI_GUID guid = EFI_DRIVER_BINDING_PROTOCOL_GUID;

	systable = SystemTable;
	bs = SystemTable->BootServices;

	drvbind.Supported = drv_supported;
	drvbind.Start = drv_start;
	drvbind.Stop = drv_stop;
	drvbind.Version = 0x10;
	drvbind.ImageHandle = ImageHandle;
	drvbind.DriverBindingHandle = ImageHandle;

	Status = bs->InstallProtocolInterface(&drvbind.DriverBindingHandle, &guid, EFI_NATIVE_INTERFACE, &drvbind);
	if (EFI_ERROR(Status))
	{
		do_print_error((CHAR16*)L"InstallProtocolInterface", Status);
		return Status;
	}

	return EFI_SUCCESS;
}
