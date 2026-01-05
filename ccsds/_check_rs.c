//#include "ccsds_io.h"

#define SYNCLENGTH      4
#define DATALENGTH      223
#define CODELENGTH      32

#define DataLength_max 239
#define CodeLength_max 32
#define InterLengthMax  8

//------------------------------------------------------------------------------
// Parameters for ECSS Reed-Solomon RS(255,223) encoder
//------------------------------------------------------------------------------

#define DataLengthMax   DATALENGTH*InterLengthMax
#define CodeLengthMax   CODELENGTH*InterLengthMax

// zero element
#define Zero            255

// generator polynomial
/*const*/ int gp [32+1] =        {0x00, 0x45, 0xD6, 0x06, 0xD1, 0x8F, 0x51, 0x2E, 0x20,
                        0xA5, 0x5D, 0xE4, 0xBE, 0x06, 0x55, 0x46, 0xEA, 0x46,
                        0x55, 0x06, 0xBE, 0xE4, 0x5D, 0xA5, 0x20, 0x2E, 0x51,
                        0x8F, 0xD1, 0x06, 0xD6, 0x45, 0x00};

const int gp8 [16+1] =       {0x00, 0xa5, 0xa0, 0x4a, 0xe6, 0xae, 0xd8, 0x92, 0xb2,
			0x92, 0xd8, 0xae, 0xe6, 0x4a, 0xa0, 0xa5 ,0x00};


// galois field
const int gf [255+1] =       {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x5F,
                        0xBE, 0x23, 0x46, 0x8C, 0x47, 0x8E, 0x43, 0x86, 0x53,
                        0xA6, 0x13, 0x26, 0x4C, 0x98, 0x6F, 0xDE, 0xE3, 0x99,
                        0x6D, 0xDA, 0xEB, 0x89, 0x4D, 0x9A, 0x6B, 0xD6, 0xF3,
                        0xB9, 0x2D, 0x5A, 0xB4, 0x37, 0x6E, 0xDC, 0xE7, 0x91,
                        0x7D, 0xFA, 0xAB, 0x09, 0x12, 0x24, 0x48, 0x90, 0x7F,
                        0xFE, 0xA3, 0x19, 0x32, 0x64, 0xC8, 0xCF, 0xC1, 0xDD,
                        0xE5, 0x95, 0x75, 0xEA, 0x8B, 0x49, 0x92, 0x7B, 0xF6,
                        0xB3, 0x39, 0x72, 0xE4, 0x97, 0x71, 0xE2, 0x9B, 0x69,
                        0xD2, 0xFB, 0xA9, 0x0D, 0x1A, 0x34, 0x68, 0xD0, 0xFF,
                        0xA1, 0x1D, 0x3A, 0x74, 0xE8, 0x8F, 0x41, 0x82, 0x5B,
                        0xB6, 0x33, 0x66, 0xCC, 0xC7, 0xD1, 0xFD, 0xA5, 0x15,
                        0x2A, 0x54, 0xA8, 0x0F, 0x1E, 0x3C, 0x78, 0xF0, 0xBF,
                        0x21, 0x42, 0x84, 0x57, 0xAE, 0x03, 0x06, 0x0C, 0x18,
                        0x30, 0x60, 0xC0, 0xDF, 0xE1, 0x9D, 0x65, 0xCA, 0xCB,
                        0xC9, 0xCD, 0xC5, 0xD5, 0xF5, 0xB5, 0x35, 0x6A, 0xD4,
                        0xF7, 0xB1, 0x3D, 0x7A, 0xF4, 0xB7, 0x31, 0x62, 0xC4,
                        0xD7, 0xF1, 0xBD, 0x25, 0x4A, 0x94, 0x77, 0xEE, 0x83,
                        0x59, 0xB2, 0x3B, 0x76, 0xEC, 0x87, 0x51, 0xA2, 0x1B,
                        0x36, 0x6C, 0xD8, 0xEF, 0x81, 0x5D, 0xBA, 0x2B, 0x56,
                        0xAC, 0x07, 0x0E, 0x1C, 0x38, 0x70, 0xE0, 0x9F, 0x61,
                        0xC2, 0xDB, 0xE9, 0x8D, 0x45, 0x8A, 0x4B, 0x96, 0x73,
                        0xE6, 0x93, 0x79, 0xF2, 0xBB, 0x29, 0x52, 0xA4, 0x17,
                        0x2E, 0x5C, 0xB8, 0x2F, 0x5E, 0xBC, 0x27, 0x4E, 0x9C,
                        0x67, 0xCE, 0xC3, 0xD9, 0xED, 0x85, 0x55, 0xAA, 0x0B,
                        0x16, 0x2C, 0x58, 0xB0, 0x3F, 0x7E, 0xFC, 0xA7, 0x11,
                        0x22, 0x44, 0x88, 0x4F, 0x9E, 0x63, 0xC6, 0xD3, 0xF9,
                        0xAD, 0x05, 0x0A, 0x14, 0x28, 0x50, 0xA0, 0x1F, 0x3E,
                        0x7C, 0xF8, 0xAF, 0x00};

const int gf_index [255+1] = {0xFF, 0x00, 0x01, 0x7A, 0x02, 0xF4, 0x7B, 0xB5, 0x03,
                        0x30, 0xF5, 0xE0, 0x7C, 0x54, 0xB6, 0x6F, 0x04, 0xE9,
                        0x31, 0x13, 0xF6, 0x6B, 0xE1, 0xCE, 0x7D, 0x38, 0x55,
                        0xAA, 0xB7, 0x5B, 0x70, 0xFA, 0x05, 0x75, 0xEA, 0x0A,
                        0x32, 0x9C, 0x14, 0xD5, 0xF7, 0xCB, 0x6C, 0xB2, 0xE2,
                        0x25, 0xCF, 0xD2, 0x7E, 0x96, 0x39, 0x64, 0x56, 0x8D,
                        0xAB, 0x28, 0xB8, 0x49, 0x5C, 0xA4, 0x71, 0x92, 0xFB,
                        0xE5, 0x06, 0x60, 0x76, 0x0F, 0xEB, 0xC1, 0x0B, 0x0D,
                        0x33, 0x44, 0x9D, 0xC3, 0x15, 0x1F, 0xD6, 0xED, 0xF8,
                        0xA8, 0xCC, 0x11, 0x6D, 0xDE, 0xB3, 0x78, 0xE3, 0xA2,
                        0x26, 0x62, 0xD0, 0xB0, 0xD3, 0x08, 0x7F, 0xBC, 0x97,
                        0xEF, 0x3A, 0x84, 0x65, 0xD8, 0x57, 0x50, 0x8E, 0x21,
                        0xAC, 0x1B, 0x29, 0x17, 0xB9, 0x4D, 0x4A, 0xC5, 0x5D,
                        0x41, 0xA5, 0x9F, 0x72, 0xC8, 0x93, 0x46, 0xFC, 0x2D,
                        0xE6, 0x35, 0x07, 0xAF, 0x61, 0xA1, 0x77, 0xDD, 0x10,
                        0xA7, 0xEC, 0x1E, 0xC2, 0x43, 0x0C, 0xC0, 0x0E, 0x5F,
                        0x34, 0x2C, 0x45, 0xC7, 0x9E, 0x40, 0xC4, 0x4C, 0x16,
                        0x1A, 0x20, 0x4F, 0xD7, 0x83, 0xEE, 0xBB, 0xF9, 0x5A,
                        0xA9, 0x37, 0xCD, 0x6A, 0x12, 0xE8, 0x6E, 0x53, 0xDF,
                        0x2F, 0xB4, 0xF3, 0x79, 0xFE, 0xE4, 0x91, 0xA3, 0x48,
                        0x27, 0x8C, 0x63, 0x95, 0xD1, 0x24, 0xB1, 0xCA, 0xD4,
                        0x9B, 0x09, 0x74, 0x80, 0x3D, 0xBD, 0xDA, 0x98, 0x89,
                        0xF0, 0x67, 0x3B, 0x87, 0x85, 0x86, 0x66, 0x88, 0xD9,
                        0x3C, 0x58, 0x68, 0x51, 0xF1, 0x8F, 0x8A, 0x22, 0x99,
                        0xAD, 0xDB, 0x1C, 0xBE, 0x2A, 0x3E, 0x18, 0x81, 0xBA,
                        0x82, 0x4E, 0x19, 0x4B, 0x3F, 0xC6, 0x2B, 0x5E, 0xBF,
                        0x42, 0x1D, 0xA6, 0xDC, 0xA0, 0xAE, 0x73, 0x9A, 0xC9,
                        0x23, 0x94, 0x8B, 0x47, 0x90, 0xFD, 0xF2, 0x2E, 0x52,
                        0xE7, 0x69, 0x36, 0x59};

// iota to alpha representation
const int iota [256] =      { 0x00, 0x7B, 0x79, 0x02, 0x2B, 0x50, 0x52, 0x29, 0x3F,
                        0x44, 0x46, 0x3D, 0x14, 0x6F, 0x6D, 0x16, 0x09, 0x72,
                        0x70, 0x0B, 0x22, 0x59, 0x5B, 0x20, 0x36, 0x4D, 0x4F,
                        0x34, 0x1D, 0x66, 0x64, 0x1F, 0x87, 0xFC, 0xFE, 0x85,
                        0xAC, 0xD7, 0xD5, 0xAE, 0xB8, 0xC3, 0xC1, 0xBA, 0x93,
                        0xE8, 0xEA, 0x91, 0x8E, 0xF5, 0xF7, 0x8C, 0xA5, 0xDE,
                        0xDC, 0xA7, 0xB1, 0xCA, 0xC8, 0xB3, 0x9A, 0xE1, 0xE3,
                        0x98, 0x5F, 0x24, 0x26, 0x5D, 0x74, 0x0F, 0x0D, 0x76,
                        0x60, 0x1B, 0x19, 0x62, 0x4B, 0x30, 0x32, 0x49, 0x56,
                        0x2D, 0x2F, 0x54, 0x7D, 0x06, 0x04, 0x7F, 0x69, 0x12,
                        0x10, 0x6B, 0x42, 0x39, 0x3B, 0x40, 0xD8, 0xA3, 0xA1,
                        0xDA, 0xF3, 0x88, 0x8A, 0xF1, 0xE7, 0x9C, 0x9E, 0xE5,
                        0xCC, 0xB7, 0xB5, 0xCE, 0xD1, 0xAA, 0xA8, 0xD3, 0xFA,
                        0x81, 0x83, 0xF8, 0xEE, 0x95, 0x97, 0xEC, 0xC5, 0xBE,
                        0xBC, 0xC7, 0x37, 0x4C, 0x4E, 0x35, 0x1C, 0x67, 0x65,
                        0x1E, 0x08, 0x73, 0x71, 0x0A, 0x23, 0x58, 0x5A, 0x21,
                        0x3E, 0x45, 0x47, 0x3C, 0x15, 0x6E, 0x6C, 0x17, 0x01,
                        0x7A, 0x78, 0x03, 0x2A, 0x51, 0x53, 0x28, 0xB0, 0xCB,
                        0xC9, 0xB2, 0x9B, 0xE0, 0xE2, 0x99, 0x8F, 0xF4, 0xF6,
                        0x8D, 0xA4, 0xDF, 0xDD, 0xA6, 0xB9, 0xC2, 0xC0, 0xBB,
                        0x92, 0xE9, 0xEB, 0x90, 0x86, 0xFD, 0xFF, 0x84, 0xAD,
                        0xD6, 0xD4, 0xAF, 0x68, 0x13, 0x11, 0x6A, 0x43, 0x38,
                        0x3A, 0x41, 0x57, 0x2C, 0x2E, 0x55, 0x7C, 0x07, 0x05,
                        0x7E, 0x61, 0x1A, 0x18, 0x63, 0x4A, 0x31, 0x33, 0x48,
                        0x5E, 0x25, 0x27, 0x5C, 0x75, 0x0E, 0x0C, 0x77, 0xEF,
                        0x94, 0x96, 0xED, 0xC4, 0xBF, 0xBD, 0xC6, 0xD0, 0xAB,
                        0xA9, 0xD2, 0xFB, 0x80, 0x82, 0xF9, 0xE6, 0x9D, 0x9F,
                        0xE4, 0xCD, 0xB6, 0xB4, 0xCF, 0xD9, 0xA2, 0xA0, 0xDB,
                        0xF2, 0x89, 0x8B, 0xF0};

// alpha to iota representation
const int alpha [256] =     { 0x00, 0x98, 0x03, 0x9B, 0x56, 0xCE, 0x55, 0xCD, 0x88,
                        0x10, 0x8B, 0x13, 0xDE, 0x46, 0xDD, 0x45, 0x5A, 0xC2,
                        0x59, 0xC1, 0x0C, 0x94, 0x0F, 0x97, 0xD2, 0x4A, 0xD1,
                        0x49, 0x84, 0x1C, 0x87, 0x1F, 0x17, 0x8F, 0x14, 0x8C,
                        0x41, 0xD9, 0x42, 0xDA, 0x9F, 0x07, 0x9C, 0x04, 0xC9,
                        0x51, 0xCA, 0x52, 0x4D, 0xD5, 0x4E, 0xD6, 0x1B, 0x83,
                        0x18, 0x80, 0xC5, 0x5D, 0xC6, 0x5E, 0x93, 0x0B, 0x90,
                        0x08, 0x5F, 0xC7, 0x5C, 0xC4, 0x09, 0x91, 0x0A, 0x92,
                        0xD7, 0x4F, 0xD4, 0x4C, 0x81, 0x19, 0x82, 0x1A, 0x05,
                        0x9D, 0x06, 0x9E, 0x53, 0xCB, 0x50, 0xC8, 0x8D, 0x15,
                        0x8E, 0x16, 0xDB, 0x43, 0xD8, 0x40, 0x48, 0xD0, 0x4B,
                        0xD3, 0x1E, 0x86, 0x1D, 0x85, 0xC0, 0x58, 0xC3, 0x5B,
                        0x96, 0x0E, 0x95, 0x0D, 0x12, 0x8A, 0x11, 0x89, 0x44,
                        0xDC, 0x47, 0xDF, 0x9A, 0x02, 0x99, 0x01, 0xCC, 0x54,
                        0xCF, 0x57, 0xED, 0x75, 0xEE, 0x76, 0xBB, 0x23, 0xB8,
                        0x20, 0x65, 0xFD, 0x66, 0xFE, 0x33, 0xAB, 0x30, 0xA8,
                        0xB7, 0x2F, 0xB4, 0x2C, 0xE1, 0x79, 0xE2, 0x7A, 0x3F,
                        0xA7, 0x3C, 0xA4, 0x69, 0xF1, 0x6A, 0xF2, 0xFA, 0x62,
                        0xF9, 0x61, 0xAC, 0x34, 0xAF, 0x37, 0x72, 0xEA, 0x71,
                        0xE9, 0x24, 0xBC, 0x27, 0xBF, 0xA0, 0x38, 0xA3, 0x3B,
                        0xF6, 0x6E, 0xF5, 0x6D, 0x28, 0xB0, 0x2B, 0xB3, 0x7E,
                        0xE6, 0x7D, 0xE5, 0xB2, 0x2A, 0xB1, 0x29, 0xE4, 0x7C,
                        0xE7, 0x7F, 0x3A, 0xA2, 0x39, 0xA1, 0x6C, 0xF4, 0x6F,
                        0xF7, 0xE8, 0x70, 0xEB, 0x73, 0xBE, 0x26, 0xBD, 0x25,
                        0x60, 0xF8, 0x63, 0xFB, 0x36, 0xAE, 0x35, 0xAD, 0xA5,
                        0x3D, 0xA6, 0x3E, 0xF3, 0x6B, 0xF0, 0x68, 0x2D, 0xB5,
                        0x2E, 0xB6, 0x7B, 0xE3, 0x78, 0xE0, 0xFF, 0x67, 0xFC,
                        0x64, 0xA9, 0x31, 0xAA, 0x32, 0x77, 0xEF, 0x74, 0xEC,
                        0x21, 0xB9, 0x22, 0xBA};

//------------------------------------------------------------------------------
// ECSS Reed-Solomon RS(255,223) encoder
//------------------------------------------------------------------------------
void RS255_223(unsigned int   datalen,
	       unsigned int   codelen,
	       unsigned char  *data,
	       unsigned char  *code,
	       int dl, int cl, int *_gp) {
	
	unsigned long  i, j;
	register int   ii, jj;
	unsigned long  feedback;
	unsigned long  depth;
	unsigned char  data_alpha[DataLength_max*InterLengthMax];
	unsigned char  code_alpha[CodeLength_max*InterLengthMax];
	unsigned char  data_223_alpha [DataLength_max];
	unsigned char  code_32_alpha [CodeLength_max];
	
	// Caluclate interleave depth 
	depth    = datalen/dl;
	
	// Convert data from iota to alpha representation 
	for (i=0; i < datalen; i++)
		//printf("\n %02X" ,data[i]);
		data_alpha[i] = alpha[data[i]];
	
	for (i=0; i < depth; i++) {
		// De-interleave data 
		for (j=0; j < dl; j++)
			data_223_alpha[j] = data_alpha[j*depth+i];
		
		// Clear codeword*/
		for (ii=0; ii<cl; ii++)
			code_32_alpha[ii] = 0;
		
		// Encode codeword 
		for (ii=dl-1; ii>=0; ii--) {
			feedback = gf_index[data_223_alpha[ii] ^
					    code_32_alpha[cl-1]];
			if (feedback != Zero) {
				for (jj=cl-1; jj>0; jj--)
					if (_gp[jj] != Zero)
						code_32_alpha[jj] = code_32_alpha[jj-1] ^
							gf[(_gp[jj]+feedback)%255];
					else
						code_32_alpha[jj] = code_32_alpha[jj-1];
				code_32_alpha[0] = gf[(_gp[0]+feedback)%255];}
			else {
				for (jj=cl-1; jj>0; jj--)
					code_32_alpha[jj] = code_32_alpha[jj-1];
				code_32_alpha[0] = 0;
			};
		};
		
		// Interleave code symbols 
		for (j=0; j < cl; j++)
			
			code_alpha[j*depth+i] = code_32_alpha[j];
			
	};
	
		
	// Convert from alpha to iota representation 
	for (i=0; i < codelen; i++){
		
		code[i] = iota[code_alpha[i]];
		
	}
};

	
int ccsds_check_rs(unsigned char *m, int len) {

    /*struct ccsds_frame_param *p*/
    int synclength = SYNCLENGTH; /* ASM */
    int framelength = 1115;
    
    /*struct ccsds_state *state*/
    int datashort = 0;
    int scodelen = 0;
    
    int dl = DATALENGTH, cl = CODELENGTH; /* *gp = GP;*/ 
    
    int realdatalen = 0;
    static int framecntr = 0;
    
    static int errbytecntr = 0;
    static int errframecntr = 0;
    int temperrbytecntr = 0;
    
	unsigned char *h = m + synclength; int codelen, datalen = framelength, i;
	unsigned char  encoded[CodeLength_max * InterLengthMax];
	unsigned char  realdata[DataLength_max * InterLengthMax];
	unsigned char sync[4] ={ 0x1A, 0xCF, 0xFC, 0x1D };

	
	if (len < (synclength + framelength)) {
		return 1;
	}

	printf("frame %d %d %d %x\n",framecntr, dl, cl, gp);
	
	if (datashort) {
		codelen  = scodelen;
		for (i=0; i<dl * InterLengthMax; i++)
			realdata[i]=0;
		for (i=framelength; i>0; i--)
			realdata[i+datashort-1] = h[i-1];
		RS255_223(realdatalen, scodelen, realdata, encoded, dl, cl, gp);
		
	} else {
		codelen  = framelength/dl * cl;
		RS255_223(datalen, codelen, h, encoded, dl, cl, gp);
	}

	
	for (i=0; i < synclength; i++){
		if (!(sync[i]==m[i])){
		    
			printf("Sync symbol %d, codeword %d, read/calculated ", i+1, framecntr+1);
			printf("%X%X / ",(m[i]/16)&0xF,   m[i]&0xF);
			printf("%X%X \n",(sync[i]/16)&0xF, sync[i]&0xF);
			
			temperrbytecntr++;
			printf("%d",temperrbytecntr);
			

		};
	};
	
	// compare read code and encoded code l
	for (i=0; i < codelen; i++){
		if (!(h[datalen+i]==encoded[i])){
		    
			printf("Code symbol %d, codeword %d, read/calculated ",
			       i+1, framecntr+1);
			printf("%02X / ", h[datalen+i]);
			printf("%02X \n", encoded[i]);
			temperrbytecntr++;
			printf("%d",temperrbytecntr);
			

		};
		
	};
	
	errbytecntr += temperrbytecntr;
	
	if(temperrbytecntr){
	    
	    errframecntr++;
	}
	
	framecntr++;
	
	if(temperrbytecntr)
        printf("RS Frames - Checked: %06d - FrameError: %06d - ByteError: %06d - ByteErrorLast: %06d \n",
	           framecntr, errframecntr, errbytecntr, temperrbytecntr);
	           
	return errframecntr;
}

int ccsds_add_rs(unsigned char *m, int len, unsigned char *rs_code) {

    /*struct ccsds_frame_param *p*/
    int synclength = SYNCLENGTH; /* ASM */
    int framelength = 1115;

    /*struct ccsds_state *state*/
    int datashort = 0;
    int scodelen = 0;

    int dl = DATALENGTH, cl = CODELENGTH; /* *gp = GP;*/

    int realdatalen = 0;
	unsigned char *h = m + synclength; int codelen, datalen = framelength, i;
	unsigned char  realdata[DataLength_max * InterLengthMax];

	if (len < (synclength + framelength)) {
		return 1;
	}

	if (datashort) {
		codelen  = scodelen;
		for (i=0; i<dl * InterLengthMax; i++)
			realdata[i]=0;
		for (i=framelength; i>0; i--)
			realdata[i+datashort-1] = h[i-1];
		RS255_223(realdatalen, scodelen, realdata, rs_code/*encoded*/, dl, cl, gp);
	} else {
		codelen  = framelength/dl * cl;
		RS255_223(datalen, codelen, h, rs_code/*encoded*/, dl, cl, gp);
	}

	return codelen;
}

	
/* RS CORRECTION */
#define tt       16
#define nn       255
#define mm       8
#define kk       223
#define GF_SIZE  256
#define PRIMATIVE_POLY  0x11D
#define alpha 2

int gf_exp[2 *(GF_SIZE-1)];
int gf_log[GF_SIZE];

void calculate_syndrome(unsigned char *received, unsigned char *syndromes, unsigned char *code){
    int i, j;
    unsigned long  feedback;
    int syn_error = 0;

    //gf_init_table();

    for (i = nn-kk -1; i < 0; i--) {
        syndromes[i] = 0;

        for (j = nn-1; j < 0; j--){


            if(received[j] != -1){
            feedback = gf_index[received[j] ^  code[nn-kk-1]];

            //syndromes[i] ^= gf[(gf_index[received[j]] + (i * j)) % nn];
            syndromes[i] ^= syndromes[i + 1] ^ gf[(gp[i]+ feedback)%255];

            //syndromes[i] ^= gf_mod(received[j] * gp[(i * j) % nn]);
            }
            if(syndromes[i] != 0){
                syn_error++;
                //syndromes[i] = gf_index[syndromes[i]];
            }
        }

    }
    LOG_ON_CONSOLE(0, C_YELLOW, "\nError counter : %d\n", syn_error);


}

//------------------------------------------------------------------------------
// STEP 2 : The Error-locator Word Calculation with Berlekamp-Massey Algorithm
//------------------------------------------------------------------------------

void berlekamp_massey(unsigned char *syndrome, unsigned char *error_loc_poly) {
    int elp[tt+1] = {1};
    int discrepancy[tt+1] = {0};
    int l = 0;                            // Polynomial Degree
    int i, j;

    for(i = 1; i <= 2*tt; i++){
        discrepancy[i] = syndrome[i - 1];
        //Discrepancy Calculation

        for(j = 1; j <= l; j++){
            if(elp[j] != 0){
                discrepancy[i] ^= gf[(elp[j] + syndrome[i - j - 1]) % nn];
            }
        }
        // if discrepancy is not equal to 0, Update the polynomials
        if (discrepancy[i] != 0){
            int temp_elp[tt + 1] = {0};

            for(j = 0; j < l; j++){
                    temp_elp[j] = elp[j];
                    //C[i] ^= gf[(gf_index[d] + gf_index[B[i]]) % 255];
            }
            for(j = 0; j < l; j++){
                    elp[j] ^= gf[(gf_index[discrepancy[i]] + gf_index[discrepancy[i - 1]]) % nn];
                    //C[i] ^= gf[(gf_index[d] + gf_index[B[i]]) % 255];
            }
            l = i - l;
        }
    }
    for (i = 0; i <= tt; i ++){
        error_loc_poly[i] = elp[i];
    }
}

//------------------------------------------------------------------------------
// STEP 3 : Finding Error Locations with Chien Search
//------------------------------------------------------------------------------

int chein_search(unsigned char *error_loc_poly, int *error_pos){
    int count = 0;
    int i, j;
    for(i = 0; i < nn; i++){
        int sum = 1;
        for(j = 0; j < tt; j++){
            if(error_loc_poly[j] != 0){
                sum ^= gf[(gf_index[error_loc_poly[j]] + i * j) % nn];
            }
        }
        if (sum == 0){
            error_loc_poly[count++] = nn - i - 1;
        }
    }
    return count;
}

//------------------------------------------------------------------------------
// STEP 4 : Finding Error Values with Forney Algorithm
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// STEP 5 : Correcting Data and Correcting CADUs
//------------------------------------------------------------------------------

int rs_correction(unsigned char *received, unsigned char *code, int datalen, int codelen, int dl, int cl){
    unsigned char syndromes[kk];
    unsigned char locator[kk];
    int error_positions[kk];
    int num_errors;
    int i;

    calculate_syndrome(received, syndromes,code);
    // If syndromes are zero, The Data has no error.
    int error_detected = 0;
    for(i = 0; i < cl; i++){
        if(syndromes[i] != 0){
            error_detected = 1; // There is an error
            break;
        }
    }
    if(!error_detected){
        printf("Data has no error.\n");
    }

    berlekamp_massey(syndromes, locator);

    num_errors = chein_search(locator, error_positions);

    if(num_errors > 0 && num_errors <= cl/2){
        printf("%d data bulundu, düzeltiliyor...\n", num_errors);

        //forney_algorithm(syndromes, locator, error_positions, num_errors, );

        printf("Hatalar başarıyla düzeltildi.\n");

        return 0;
    } else{
        printf("Çok fazla hata var. \n");

        return 1;
    }
}


void decode_rs_(unsigned int   datalen, unsigned int   codelen, unsigned char  *data, unsigned char  *code, int dl, int cl)
{

    register int i, j, u, q;
    int ii, jj;
    cl = nn-kk;

    unsigned char elp[cl + 2], d[cl + 2], l[cl + 2], u_lu[cl + 2], s[cl + 1];
    unsigned char root[tt], loc[tt], z[tt + 1], err[nn], reg[tt + 1];
    int count = 0, syn_error = 0;
    unsigned char  data_alpha[DataLength_max*InterLengthMax];
    unsigned char  data_223_alpha [nn];

    // Caluclate interleave depth
    int depth = datalen/dl;
    //data[15] = 0x50;  // for testing
    // Convert data from iota to alpha representation
    printf("Convert data from iota to alpha representation");
    for (ii=0; ii < datalen; ii++){

        data_alpha[ii] = alpha[&data[ii]];
        //data_alpha[ii] = iota[data[ii]];

    }

    for (ii=0; ii < depth; ii++) {
        // De-interleave data

        for (jj=0; jj < kk; jj++){
            data_223_alpha[jj] = data_alpha[ii*depth+jj];
        }
        calculate_syndrome(data_223_alpha,s,code);

    }
}

/*
 * Sample syndrome codes
 *      for (i = 0; i <= nn-kk; i++) {
            s[i] = 0;
        for (j = 0; j < nn; j++){

            if(data_223_iota[j] != -1){
                s[i] ^= gf[(gf_index[data_223_iota[j]] + i * j) % nn];

            }
        }
        printf("%02X ",data_223_iota[i]);

        LOG_ON_CONSOLE(0, C_RED, "syn_error : %d\n", syn_error );
        printf("%d\n",s[i]);
            if(s[i] != 0){
                syn_error = 1;
                s[i] = gf_index[s[i]];
            }
        }

    if (syn_error){
        printf("There is an error!!!!");
        }
 * */



/*
 * sample forney
 *
 *
void forney_algorithm(int *syndromes, int *error_loc_poly,  int *error_pos, int *error_magnitudes){
    int numerator[tt + 1] = {0}, denominator[tt + 1] = {0};
    int i, j;

    for(i = 0; i < tt; i++){
        //int xi_inv = gf_index[255 - error_positions[i]];
        numerator[i] = syndromes[i];

        for(j = 0; j < tt; j++){
            if(error_loc_poly[j] != 0){
                numerator[i] ^= gf[(gf_index[error_loc_poly[j]] + error_pos[i] * j) % nn];
            }
        }

        denominator[i] = 1;
        for(j = 0; j < num_errors; j++){
            if (i != j){
                denominator ^= gf[(gf_index[255 - error_positions[j]] + xi_inv) % 255];
            }
        }

        error_magnitude[i] = gf[(gf_index[numerator] - gf_index[denominator] + 255) % 255];
    }

    // Correction
    for(i = 0; i < num_errors; i++){
        printf("a\n");

        data_alpha[error_positions[i]] ^= error_magnitude[i];
        printf("%02X \n",data_alpha[error_positions[i]] );
    }
}
 * */


/*
int rs_check_demo(unsigned char *data, int data_len, unsigned char *syndromes, int cl, const int *gf, const int *gf_index){
    int i;
    static int errframecntr = 0;


    calculate_syndrome(data, syndromes, data_len, cl, gf, gf_index);

    // If syndromes are zero, The Data has no error.
    for(i = 0; i < cl; i++){
        if(syndromes[i] != 0){
            errframecntr++; // There is an error
        }
    }
    return errframecntr; // There is no error
}
struct ccsds_frame_param{

    int synclenght;
    int framelenght;

    };

struct ccsds_state{

    int framecntr;
    int dl;
    int cl;
    int realdatalen;
    int datashort;
    int codelen;
    int gp[33];
    };
void RS255_223_decode(unsigned int datalen, unsigned int codelen, unsigned char *data, unsigned char *code, int dl, int cl, int *_gp){

    unsigned int i;
    register int   ii, jj;

    unsigned long feedback;
    unsigned char data_alpha[DataLengthMax];
    unsigned char code_alpha[CodeLengthMax];

    // Veriyi iota'dan alpha temsiline çevir
    for( i = 0; i < datalen; i++)
        data_alpha[i] = alpha[data[i]];

    //Kod char temizle
    for( ii = 0; ii < cl; ii++)
        code_alpha[ii] = 0;

    // Reed-Solomon çözme işlemi
    for(ii = dl - 1; ii >= 0; ii-- ){
        feedback = gf_index[data_alpha[ii] ^ code_alpha[cl-1]];
        if(feedback != Zero){
            for( jj = cl - 1; jj > 0; jj-- ){
                if (_gp[jj] != Zero)
                    code_alpha[jj] = code_alpha[jj - 1] ^ gf[(_gp[jj] + feedback) % 255];
                else
                    code_alpha[jj] = code_alpha[jj - 1];

                }
            code_alpha[0] = gf[(_gp[0] + feedback) % 255];
            }
        else{
            for(jj = cl - 1; jj > 0; jj-- )
                code_alpha[jj] = code_alpha[jj - 1];
            code_alpha[0] = 0;
            }
        }
    for (i = 0; i < codelen; i++)
        code[i] = iota[code_alpha[i]];
    }
 */



/*

void berlekamp_massey(unsigned char *syndromes, unsigned char *locator, int cl, const int *gf, const int *gf_index) {

    unsigned char C[CodeLengthMax] = {1}; // Initial Polynomial
    unsigned char B[CodeLengthMax] = {1}; // Auxiliary Polynomial
    unsigned char T[CodeLengthMax];       // Temp Polynomial
    unsigned char b = 1;                  // Error Mangitude
    int L = 0;                            // Polynomial Degree
    //int m = 1;
    int n, i;

    for(n = 0; n < cl; n++){
        //Discrepancy Calculation
        unsigned char d = syndromes[n];
        for(i = 1; i <= L; i++){
            d ^= gf[(gf_index[C[i]] + syndromes[n - i]) % 255];
            }
        // if discrepancy is not equal to 0, Update the polynomials
        if (d != 0){
            memcpy(T, C, sizeof(C));
            int i;
            for(i = 0; i < cl; i++){
                if(B[i] != 0){
                    C[i] ^= gf[(gf_index[d] + gf_index[B[i]]) % 255];
                }
            }
            if(2 * L <= n){
                L = n + 1 - L;
                memcpy(B, T, sizeof(T));
                b = d;
            }
        }
        // Shift Polynomial B
        int i;
        for(i = (cl - 1); i > 0; i--){
            B[i] = B[i - 1];
        }
        B[0] = 0;
    }
    memcpy(locator, C, CodeLengthMax);
}
*/



	
	
	





















