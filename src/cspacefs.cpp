#include <efi.h>
#include "quibbleproto.h"

#define UNUSED(x) (void)(x)
#define min(a, b) ((a) < (b) ? (a) : (b))

typedef struct
{
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL proto;
    EFI_QUIBBLE_PROTOCOL quibble_proto;
    EFI_HANDLE controller;
    EFI_BLOCK_IO_PROTOCOL* block;
    EFI_DISK_IO_PROTOCOL* disk_io;
} volume;

typedef struct
{
	EFI_FILE_PROTOCOL proto;
	volume* vol;
} inode;

static EFI_SYSTEM_TABLE* systable;
static EFI_BOOT_SERVICES* bs;
static EFI_DRIVER_BINDING_PROTOCOL drvbind;

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

static void do_print_error(CHAR16* func, EFI_STATUS Status)
{
    systable->ConOut->OutputString(systable->ConOut, (CHAR16*)L"Error in ");
    systable->ConOut->OutputString(systable->ConOut, func);
    systable->ConOut->OutputString(systable->ConOut, (CHAR16*)L": ");
    systable->ConOut->OutputString(systable->ConOut, (CHAR16*)L"0x");
    systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0xF0000000) >> 28));
    systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x0F000000) >> 24));
    systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x00F00000) >> 20));
    systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x000F0000) >> 16));
    systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x0000F000) >> 12));
    systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x00000F00) >> 8));
    systable->ConOut->OutputString(systable->ConOut, HEX((Status & 0x000000F0) >> 4));
    systable->ConOut->OutputString(systable->ConOut, HEX(Status & 0x0000000F));
    systable->ConOut->OutputString(systable->ConOut, (CHAR16*)L".\r\n");
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

static EFI_STATUS EFIAPI file_open(struct _EFI_FILE_HANDLE* File, struct _EFI_FILE_HANDLE** NewHandle, CHAR16* FileName,
    UINT64 OpenMode, UINT64 Attributes) {
    systable->ConOut->OutputString(systable->ConOut, (CHAR16*)L"file_open\r\n");

    // FIXME

    return EFI_UNSUPPORTED;
}

static EFI_STATUS EFIAPI file_close(struct _EFI_FILE_HANDLE* File) {
    systable->ConOut->OutputString(systable->ConOut, (CHAR16*)L"file_close\r\n");

    // FIXME

    return EFI_UNSUPPORTED;
}

static EFI_STATUS EFIAPI file_delete(struct _EFI_FILE_HANDLE* File) {
    UNUSED(File);

    return EFI_UNSUPPORTED;
}

static EFI_STATUS EFIAPI file_read(struct _EFI_FILE_HANDLE* File, UINTN* BufferSize, VOID* Buffer) {
    systable->ConOut->OutputString(systable->ConOut, (CHAR16*)L"file_read\r\n");

    // FIXME

    return EFI_UNSUPPORTED;
}

static EFI_STATUS EFIAPI file_write(struct _EFI_FILE_HANDLE* File, UINTN* BufferSize, VOID* Buffer) {
    UNUSED(File);
    UNUSED(BufferSize);
    UNUSED(Buffer);

    return EFI_UNSUPPORTED;
}

static EFI_STATUS EFIAPI file_set_position(struct _EFI_FILE_HANDLE* File, UINT64 Position) {
    systable->ConOut->OutputString(systable->ConOut, (CHAR16*)L"file_set_position\r\n");

    // FIXME

    return EFI_UNSUPPORTED;
}

static EFI_STATUS EFIAPI file_get_position(struct _EFI_FILE_HANDLE* File, UINT64* Position) {
    systable->ConOut->OutputString(systable->ConOut, (CHAR16*)L"file_get_position\r\n");

    // FIXME

    return EFI_UNSUPPORTED;
}

static EFI_STATUS EFIAPI file_get_info(struct _EFI_FILE_HANDLE* File, EFI_GUID* InformationType, UINTN* BufferSize, VOID* Buffer) {
    systable->ConOut->OutputString(systable->ConOut, (CHAR16*)L"file_get_info\r\n");

    // FIXME

    return EFI_UNSUPPORTED;
}

static EFI_STATUS EFIAPI file_set_info(struct _EFI_FILE_HANDLE* File, EFI_GUID* InformationType, UINTN BufferSize, VOID* Buffer) {
    UNUSED(File);
    UNUSED(InformationType);
    UNUSED(BufferSize);
    UNUSED(Buffer);

    return EFI_UNSUPPORTED;
}

static EFI_STATUS file_flush(struct _EFI_FILE_HANDLE* File) {
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
		do_print_error((CHAR16*)L"AllocatePool", Status);
		return Status;
	}

    populate_file_handle(&ino->proto);

    ino->vol = vol;

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
	
    if (*DriverNameLen < sizeof(name))
    {
		*DriverNameLen = sizeof(name);
		return EFI_BUFFER_TOO_SMALL;
	}

    *DriverNameLen = sizeof(name);

    for (unsigned long long i = 0; i < sizeof(name); i++)
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

    char table[512] = { 0 };
    Status = block->ReadBlocks(block, block->Media->MediaId, 0, 512, table);

    if (EFI_ERROR(Status))
    {
		bs->CloseProtocol(ControllerHandle, &disk_guid, This->DriverBindingHandle, ControllerHandle);
		bs->CloseProtocol(ControllerHandle, &block_guid, This->DriverBindingHandle, ControllerHandle);
		return Status;
	}

    if (table[0] > 23)
    {
        bs->CloseProtocol(ControllerHandle, &disk_guid, This->DriverBindingHandle, ControllerHandle);
        bs->CloseProtocol(ControllerHandle, &block_guid, This->DriverBindingHandle, ControllerHandle);
        return EFI_UNSUPPORTED;
    }

    unsigned long sectorsize = 2 << (9 + table[0]);
    unsigned long tablesize = table[4] + (table[3] << 8) + (table[2] << 16) + (table[1] << 24);
    unsigned long long extratablesize = static_cast<unsigned long long>(sectorsize) * tablesize;

    char* blk = 0;
    Status = bs->AllocatePool(EfiBootServicesData, sectorsize, (void**)&blk);

    if (EFI_ERROR(Status))
    {
		do_print_error((CHAR16*)L"AllocatePool", Status);
        bs->CloseProtocol(ControllerHandle, &disk_guid, This->DriverBindingHandle, ControllerHandle);
        bs->CloseProtocol(ControllerHandle, &block_guid, This->DriverBindingHandle, ControllerHandle);
        return Status;
    }

    Status = block->ReadBlocks(block, block->Media->MediaId, extratablesize, sectorsize, blk);

    if (EFI_ERROR(Status))
    {
        bs->FreePool(blk);
        bs->CloseProtocol(ControllerHandle, &disk_guid, This->DriverBindingHandle, ControllerHandle);
        bs->CloseProtocol(ControllerHandle, &block_guid, This->DriverBindingHandle, ControllerHandle);
        return Status;
    }

    bool found = false;
    unsigned long i = 0;
    long long loc = -1;

    for (; i < sectorsize; i++)
    {
        if ((blk[i] & 0xff) == 255)
        {
            if ((blk[min(i + 1, sectorsize)] & 0xff) == 254)
            {
                found = true;
                break;
            }
            else
            {
                loc = i;
            }
		}
    }

    // if following check is not enough to block the driver from loading unnecessarily,
    // then we can add a check to make sure all bytes between loc and i equal a vaild path.
    // if that is not enough, then nothing will be.

    bs->FreePool(blk);

    if (!found || (loc == -1 && i > 256))
    {
		bs->CloseProtocol(ControllerHandle, &disk_guid, This->DriverBindingHandle, ControllerHandle);
		bs->CloseProtocol(ControllerHandle, &block_guid, This->DriverBindingHandle, ControllerHandle);
		return EFI_UNSUPPORTED;
	}

    // if it is continue.

    Status = bs->AllocatePool(EfiBootServicesData, sizeof(volume), (void**)&vol);

    if (EFI_ERROR(Status))
    {
        do_print_error((CHAR16*)L"AllocatePool", Status);
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

    Status = bs->InstallMultipleProtocolInterfaces(&ControllerHandle, &fs_guid, &vol->proto, &quibble_guid, &vol->quibble_proto, nullptr);

    if (EFI_ERROR(Status))
    {
        do_print_error((CHAR16*)L"InstallMultipleProtocolInterfaces", Status);
        vol->volume::~volume();
        bs->FreePool(vol);
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
