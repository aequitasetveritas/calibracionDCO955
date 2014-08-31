/*
 * calibracion.c
 *
 *  Created on: Aug 31, 2014
 *      Author: escualo
 */

void grabar_ctes(){
	char *Flash_ptrA;
	unsigned char CAL_DATA[8];                  // Temp. storage for constants
	unsigned int j=0;
	CAL_DATA[j++] = 0x7f; 						//16MHz
	CAL_DATA[j++] = 0x8f;
	CAL_DATA[j++] = 0x85; 						//12MHz
	CAL_DATA[j++] = 0x8e;
	CAL_DATA[j++] = 0x75; 						//8MHz
	CAL_DATA[j++] = 0x8d;
	CAL_DATA[j++] = 0x34; 						//1MHz
	CAL_DATA[j++] = 0x87;
	Flash_ptrA = (char *)0x10C0;              // Point to beginning of seg A
	FCTL2 = FWKEY + FSSEL0 + FN1;             // MCLK/3 for Flash Timing Generator
	FCTL1 = FWKEY + ERASE;                    // Set Erase bit
	FCTL3 = FWKEY + LOCKA;                    // Clear LOCK & LOCKA bits
	*Flash_ptrA = 0x00;                       // Dummy write to erase Flash seg A
	FCTL1 = FWKEY + WRT;                      // Set WRT bit for write operation
	Flash_ptrA = (char *)0x10F8;              // Point to beginning of cal consts
	for (j = 0; j < 8; j++)
		*Flash_ptrA++ = CAL_DATA[j];          // re-flash DCO calibration data
	FCTL1 = FWKEY;                            // Clear WRT bit
	FCTL3 = FWKEY + LOCKA + LOCK;             // Set LOCK & LOCKA bit
}
