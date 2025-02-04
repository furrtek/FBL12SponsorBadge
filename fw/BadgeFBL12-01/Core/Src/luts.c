// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#include "main.h"
#include "luts.h"

const uint8_t sin_lut[256] = {
	128, 131, 134, 137, 140, 143, 146, 149,
	152, 156, 159, 162, 165, 168, 171, 174,
	176, 179, 182, 185, 188, 191, 193, 196,
	199, 201, 204, 206, 209, 211, 213, 216,
	218, 220, 222, 224, 226, 228, 230, 232,
	234, 235, 237, 239, 240, 242, 243, 244,
	246, 247, 248, 249, 250, 251, 251, 252,
	253, 253, 254, 254, 254, 255, 255, 255,
	255, 255, 255, 255, 254, 254, 253, 253,
	252, 252, 251, 250, 249, 248, 247, 246,
	245, 244, 242, 241, 239, 238, 236, 235,
	233, 231, 229, 227, 225, 223, 221, 219,
	217, 215, 212, 210, 207, 205, 202, 200,
	197, 195, 192, 189, 186, 184, 181, 178,
	175, 172, 169, 166, 163, 160, 157, 154,
	151, 148, 145, 142, 138, 135, 132, 129,
	126, 123, 120, 117, 113, 110, 107, 104,
	101, 98, 95, 92, 89, 86, 83, 80,
	77, 74, 71, 69, 66, 63, 60, 58,
	55, 53, 50, 48, 45, 43, 40, 38,
	36, 34, 32, 30, 28, 26, 24, 22,
	20, 19, 17, 16, 14, 13, 11, 10,
	9, 8, 7, 6, 5, 4, 3, 3,
	2, 2, 1, 1, 0, 0, 0, 0,
	0, 0, 0, 1, 1, 1, 2, 2,
	3, 4, 4, 5, 6, 7, 8, 9,
	11, 12, 13, 15, 16, 18, 20, 21,
	23, 25, 27, 29, 31, 33, 35, 37,
	39, 42, 44, 46, 49, 51, 54, 56,
	59, 62, 64, 67, 70, 73, 76, 79,
	81, 84, 87, 90, 93, 96, 99, 103,
	106, 109, 112, 115, 118, 121, 124
};

// For 8kHz samplerate
const uint8_t lpf_lut[256] = {
	112,	// 996Hz
	112,	// 992Hz
	111,	// 989Hz
	111,	// 985Hz
	111,	// 982Hz
	111,	// 978Hz
	111,	// 975Hz
	110,	// 971Hz
	110,	// 968Hz
	110,	// 964Hz
	110,	// 961Hz
	109,	// 957Hz
	109,	// 954Hz
	109,	// 950Hz
	109,	// 947Hz
	108,	// 943Hz
	108,	// 940Hz
	108,	// 936Hz
	108,	// 933Hz
	108,	// 929Hz
	107,	// 926Hz
	107,	// 922Hz
	107,	// 919Hz
	107,	// 915Hz
	106,	// 912Hz
	106,	// 908Hz
	106,	// 905Hz
	106,	// 901Hz
	105,	// 898Hz
	105,	// 894Hz
	105,	// 891Hz
	105,	// 887Hz
	104,	// 883Hz
	104,	// 880Hz
	104,	// 876Hz
	104,	// 873Hz
	103,	// 869Hz
	103,	// 866Hz
	103,	// 862Hz
	103,	// 859Hz
	102,	// 855Hz
	102,	// 852Hz
	102,	// 848Hz
	102,	// 845Hz
	101,	// 841Hz
	101,	// 838Hz
	101,	// 834Hz
	101,	// 831Hz
	100,	// 827Hz
	100,	// 824Hz
	100,	// 820Hz
	100,	// 817Hz
	99,	// 813Hz
	99,	// 810Hz
	99,	// 806Hz
	99,	// 803Hz
	98,	// 799Hz
	98,	// 796Hz
	98,	// 792Hz
	97,	// 789Hz
	97,	// 785Hz
	97,	// 782Hz
	97,	// 778Hz
	96,	// 775Hz
	96,	// 771Hz
	96,	// 767Hz
	96,	// 764Hz
	95,	// 760Hz
	95,	// 757Hz
	95,	// 753Hz
	94,	// 750Hz
	94,	// 746Hz
	94,	// 743Hz
	94,	// 739Hz
	93,	// 736Hz
	93,	// 732Hz
	93,	// 729Hz
	92,	// 725Hz
	92,	// 722Hz
	92,	// 718Hz
	92,	// 715Hz
	91,	// 711Hz
	91,	// 708Hz
	91,	// 704Hz
	90,	// 701Hz
	90,	// 697Hz
	90,	// 694Hz
	90,	// 690Hz
	89,	// 687Hz
	89,	// 683Hz
	89,	// 680Hz
	88,	// 676Hz
	88,	// 673Hz
	88,	// 669Hz
	87,	// 666Hz
	87,	// 662Hz
	87,	// 658Hz
	87,	// 655Hz
	86,	// 651Hz
	86,	// 648Hz
	86,	// 644Hz
	85,	// 641Hz
	85,	// 637Hz
	85,	// 634Hz
	84,	// 630Hz
	84,	// 627Hz
	84,	// 623Hz
	83,	// 620Hz
	83,	// 616Hz
	83,	// 613Hz
	82,	// 609Hz
	82,	// 606Hz
	82,	// 602Hz
	81,	// 599Hz
	81,	// 595Hz
	81,	// 592Hz
	80,	// 588Hz
	80,	// 585Hz
	80,	// 581Hz
	79,	// 578Hz
	79,	// 574Hz
	79,	// 571Hz
	78,	// 567Hz
	78,	// 564Hz
	78,	// 560Hz
	77,	// 557Hz
	77,	// 553Hz
	77,	// 550Hz
	76,	// 546Hz
	76,	// 542Hz
	76,	// 539Hz
	75,	// 535Hz
	75,	// 532Hz
	75,	// 528Hz
	74,	// 525Hz
	74,	// 521Hz
	74,	// 518Hz
	73,	// 514Hz
	73,	// 511Hz
	72,	// 507Hz
	72,	// 504Hz
	72,	// 500Hz
	71,	// 497Hz
	71,	// 493Hz
	71,	// 490Hz
	70,	// 486Hz
	70,	// 483Hz
	70,	// 479Hz
	69,	// 476Hz
	69,	// 472Hz
	68,	// 469Hz
	68,	// 465Hz
	68,	// 462Hz
	67,	// 458Hz
	67,	// 455Hz
	67,	// 451Hz
	66,	// 448Hz
	66,	// 444Hz
	65,	// 441Hz
	65,	// 437Hz
	65,	// 433Hz
	64,	// 430Hz
	64,	// 426Hz
	63,	// 423Hz
	63,	// 419Hz
	63,	// 416Hz
	62,	// 412Hz
	62,	// 409Hz
	61,	// 405Hz
	61,	// 402Hz
	61,	// 398Hz
	60,	// 395Hz
	60,	// 391Hz
	59,	// 388Hz
	59,	// 384Hz
	58,	// 381Hz
	58,	// 377Hz
	58,	// 374Hz
	57,	// 370Hz
	57,	// 367Hz
	56,	// 363Hz
	56,	// 360Hz
	56,	// 356Hz
	55,	// 353Hz
	55,	// 349Hz
	54,	// 346Hz
	54,	// 342Hz
	53,	// 339Hz
	53,	// 335Hz
	52,	// 332Hz
	52,	// 328Hz
	52,	// 325Hz
	51,	// 321Hz
	51,	// 317Hz
	50,	// 314Hz
	50,	// 310Hz
	49,	// 307Hz
	49,	// 303Hz
	48,	// 300Hz
	48,	// 296Hz
	47,	// 293Hz
	47,	// 289Hz
	47,	// 286Hz
	46,	// 282Hz
	46,	// 279Hz
	45,	// 275Hz
	45,	// 272Hz
	44,	// 268Hz
	44,	// 265Hz
	43,	// 261Hz
	43,	// 258Hz
	42,	// 254Hz
	42,	// 251Hz
	41,	// 247Hz
	41,	// 244Hz
	40,	// 240Hz
	40,	// 237Hz
	39,	// 233Hz
	39,	// 230Hz
	38,	// 226Hz
	38,	// 223Hz
	37,	// 219Hz
	37,	// 216Hz
	36,	// 212Hz
	36,	// 208Hz
	35,	// 205Hz
	35,	// 201Hz
	34,	// 198Hz
	33,	// 194Hz
	33,	// 191Hz
	32,	// 187Hz
	32,	// 184Hz
	31,	// 180Hz
	31,	// 177Hz
	30,	// 173Hz
	30,	// 170Hz
	29,	// 166Hz
	29,	// 163Hz
	28,	// 159Hz
	27,	// 156Hz
	27,	// 152Hz
	26,	// 149Hz
	26,	// 145Hz
	25,	// 142Hz
	25,	// 138Hz
	24,	// 135Hz
	23,	// 131Hz
	23,	// 128Hz
	22,	// 124Hz
	22,	// 121Hz
	21,	// 117Hz
	21,	// 114Hz
	20,	// 110Hz
	19,	// 107Hz
	19,	// 103Hz
	18,	// 100Hz
};

// Inverted one-hot LSB to MSB
const uint16_t dig_lut[9] = {
	0xFE00, 0xFD00, 0xFB00, 0xF700, 0xEF00, 0xDF00, 0xBF00, 0x7F00, 0xFF00
};

const uint8_t char_lut[64] = {
	0,	//
	0,	// !
	0,	// "
	0,	// #
	0,	// $
	0,	// %
	0,	// &
	16,	// '
	0,	// (
	0,	// )
	0,	// *
	0,	// +
	0,	// ,
	0,	// -
	19,	// .
	20,	// /
	245,	// 0
	144,	// 1
	87,	// 2
	209,	// 3
	146,	// 4
	75,	// 5
	92,	// 6
	21,	// 7
	95,	// 8
	23,	// 9
	0,	// :
	0,	// ;
	24,	// <
	0,	// =
	6,	// >
	0,	// ?
	205,	// @
	156,	// A
	121,	// B
	97,	// C
	106,	// D
	113,	// E
	35,	// F
	105,	// G
	44,	// H
	128,	// I
	196,	// J
	56,	// K
	96,	// L
	178,	// M
	170,	// N
	225,	// O
	49,	// P
	233,	// Q
	57,	// R
	75,	// S
	129,	// T
	224,	// U
	52,	// V
	172,	// W
	30,	// X
	210,	// Y
	85,	// Z
	97,	// [
	0,	// Backslash
	193,	// ]
	76,	// ^
	64,	// _
};

// Keyboard: C,C#,D,D#,E,F,F#,G,G#,A,A#,B
// TOUCH11 0
// TOUCH4 1
// TOUCH10 2
// TOUCH3 3
// TOUCH9 4
// TOUCH8 5
// TOUCH2 6
// TOUCH7 7
// TOUCH1 8
// TOUCH6 9
// TOUCH0 10
// TOUCH5 11

const uint16_t key_lut[16] = {
	(1 << 10),
	(1 << 8),
	(1 << 6),
	(1 << 3),
	(1 << 1),
	(1 << 11),
	(1 << 9),
	(1 << 7),
	(1 << 5),
	(1 << 4),
	(1 << 2),
	(1 << 0),
	0, 0, 0, 0
};

const char key_name_lut[12][3] = {
	"C ",
	"C'",
	"D ",
	"D'",
	"E ",
	"F ",
	"F'",
	"G ",
	"G'",
	"A ",
	"A'",
	"B ",
};

// Octaves 2, 3, 4 - Based on 8000Hz samplerate and 16bit accumulator
// 1Hz: 8.192
const uint16_t tone_lut[(3 * 12) + 1] = {
	535, 567, 601, 637, 675, 715, 757, 802, 850, 901, 954, 1011,
	1071, 1135, 1202, 1274, 1350, 1430, 1515, 1605, 1701, 1802, 1909, 2022,
	2143, 2270, 2405, 2548, 2700, 2860, 3030, 3211, 3402, 3604, 3818, 4045,
	0	// Used for beep silence
};
