#include "stdafx.h"
#include <Windows.h>
#include <ntddscsi.h>
#include <iostream>
#include <sstream>
#include <iomanip>

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		std::cout << "\nez_idfy 20151205.4534 by York Lin\n";
		std::cout << "\nusage:\n";
		std::cout << "	ez_idfy.exe \\\\.\\physicaldrive<n> <hex command>\n";
		std::cout << "example:\n";
		std::cout << "	ez_idfy.exe 0 ec\n";
		return 0;
	}
	
	std::string drive_address;
	std::string drive_address_num;

	drive_address = "\\\\.\\PhysicalDrive";
	drive_address_num = drive_address + *argv[1];

	HANDLE handle = ::CreateFileA
	(
		drive_address_num.c_str(),
		GENERIC_READ | GENERIC_WRITE,	//IOCTL_ATA_PASS_THROUGH requires read-write
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		0,								//no security attributes
		OPEN_EXISTING,
		0,								//flags and attributes
		0								//no template file
	);

	if (handle == INVALID_HANDLE_VALUE)
	{
		std::cout << "error: Invalid handle\n";
		return 0;
	}

	// IDENTIFY command requires a 512 byte buffer for data:
	const unsigned int IDENTIFY_buffer_size = 512;
	unsigned char Buffer[IDENTIFY_buffer_size + sizeof(ATA_PASS_THROUGH_EX)] = { 0 };

	ATA_PASS_THROUGH_EX & PTE = *(ATA_PASS_THROUGH_EX *)Buffer;
	PTE.Length = sizeof(PTE);
	PTE.TimeOutValue = 10;
	PTE.DataTransferLength = 512;
	PTE.DataBufferOffset = sizeof(ATA_PASS_THROUGH_EX);

	// Set up the IDE registers as specified in ATA spec.
	IDEREGS * ir = (IDEREGS *)PTE.CurrentTaskFile;

	int command_num;
	sscanf_s(argv[2], "%x", &command_num);
	ir->bCommandReg = command_num;

	ir->bSectorCountReg = 1;

	// IDENTIFY is neither 48-bit nor DMA, it reads from the device:
	PTE.AtaFlags = ATA_FLAGS_DATA_IN | ATA_FLAGS_DRDY_REQUIRED;

	// protocol =0;
	PTE.CurrentTaskFile[0] = 0x01;
	//PTE.CurrentTaskFile[3] = (0x0001 & 0x00ff); // Commid LSB
	//PTE.CurrentTaskFile[4] = ((0x0001 & 0xff00) >> 8); // Commid MSB
	PTE.CurrentTaskFile[3] = 0x01; // for command 5c
								   //PTE.CurrentTaskFile[6] = 0xec;

	std::cout << "\n";

	printf("Features Register      = %X\n", PTE.CurrentTaskFile[0]);
	printf("Sector Count Register  = %X\n", PTE.CurrentTaskFile[1]);
	printf("Sector Number Register = %X\n", PTE.CurrentTaskFile[2]);
	printf("Cylinder Low Register  = %X\n", PTE.CurrentTaskFile[3]);
	printf("Cylinder High Register = %X\n", PTE.CurrentTaskFile[4]);
	printf("Device/Head Register   = %X\n", PTE.CurrentTaskFile[5]);
	printf("Command Register       = %X\n", PTE.CurrentTaskFile[6]);
	printf("Reserved               = %X\n", PTE.CurrentTaskFile[7]);

	DWORD BR = 0;
	BOOL b = ::DeviceIoControl(handle, IOCTL_ATA_PASS_THROUGH, &PTE, sizeof(Buffer), &PTE, sizeof(Buffer), &BR, 0);

	if (b == 0)
	{
		std::cout << "error: Invalid call\n";
		return 0;
	}

	std::string str(Buffer, Buffer + sizeof Buffer / sizeof Buffer[0]);
	str = str.substr(40, IDENTIFY_buffer_size + sizeof(ATA_PASS_THROUGH_EX));
	std::stringstream ss;
	ss << std::hex << str;

	int count = 0;

	unsigned char x;
	ss >> std::noskipws;

	while (ss >> x)
	{
		if (count % 16 == 0)
		{
			std::cout << "\n";
			printf("%04X", count);
		}

		if (count % 4 == 0)
		{
			std::cout << " ";
		}

		std::cout << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)x;
		count++;

		if (count % 16 == 0)
		{
			if (count > 0)
			{
				std::cout << " ";

				for (int strc = 0; strc<16; strc++)
				{
					std::string tempstring;
					char ctemp[2];
					char ctemp2;
					tempstring = str.substr(count - 16 + strc, 1);
					strcpy_s(ctemp, tempstring.c_str());
					ctemp2 = ctemp[0];

					if ((int(ctemp2) > 20) && (int(ctemp2) < 127))
					{
						std::cout << tempstring;
					}
					else
					{
						std::cout << ".";
					}
				}
			}
		}
	}
	std::cout << "\n";

	return 0;
}