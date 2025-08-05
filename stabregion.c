/* Plot stability regions for multistep methods
   Elmar Klausmeier, 26-May-2025
   Compile like so:
        cc -Wall stabregion.c -o stabregion -lm -llapack
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <complex.h>

extern double dlamch_(char *cmach);
extern void zggev_(char *jobvl, char *jobvr, int *n, complex *a, int *lda, complex *b,
	int *ldb, complex *alpha, complex *beta, complex *vl, int *ldvl, complex *vr, int *ldvr,
	complex *work, int *lwork, double *rwork, int *info);



#define N	150
#define S	0.0	// Tischer parameter

typedef struct {
	const char *name;
	const int p;	// order (for information only)
	const int k;	// number of start steps, 2*(k+l) = number of rows of a[]
	const int l;	// cycle length, i.e., column count of a[]
	double *a;	// transpose of A_i and B_i matrices in a single matrix
} formula_t;

formula_t F[] = {
	{
		"BDF1", 1, 1, 1,
		(double[]){
		-1, 1,
		0, 1 }
	},
	{
		"BDF2", 2, 2, 1,
		(double[]){
		1, -4, 3,
		0, 0, 2 }
	},
	{
		"BDF3", 3, 3, 1,
		(double[]){
		-2, 9, -18, 11,
		0, 0, 0, 6 }
	},
	{
		"BDF4", 4, 4, 1,
		(double[]){
		3, -16, 36, -48, 25,
		0, 0, 0, 0, 12 }
	},
	{
		"BDF5", 5, 5, 1,
		(double[]){
		-12, 75, -200, 300, -300, 137,
		0, 0, 0, 0, 0, 60 }
	},
	{
		"BDF6", 6, 6, 1,
		(double[]){
		10, -72, 225, -400, 450, -360, 147,
		0, 0, 0, 0, 0, 0, 60 }
	},
	{
		"Tendler3", 3, 3, 3,	// name, p, k, l
		(double[]){
		-2, 0, 0,
		9, -2, 0,
		-18, 9, 0,
		11, -18, 9,
		0, 11, -12,
		0, 0, 3,
		// --------
		0, 0, 0,
		0, 0, 0,
		0, 0, 0,
		6, 0, -4,
		0, 6, -4,
		0, 0, 2 }
	},
	{
		"Tendler4", 4, 4, 3,	// name, p, k, l
		(double[]){
		  3 ,    0 ,    0,
		-16 ,    3 ,    0,
		 36 ,  -16 ,   11,
		-48 ,   36 ,  -48,
		 25 ,  -48 ,  216,
		  0 ,   25 , -272,
		  0 ,    0 ,   93,
		// ---------------
		  0 ,    0 ,    0,
		  0 ,    0 ,    0,
		  0 ,    0 ,    0,
		  0 ,    0 ,    0,
		 12 ,    0 ,  -60,
		  0 ,   12 ,  -48,
		  0 ,    0 ,   48 }
	},
	{
		"Tendler5", 5, 5, 4,	// name, p, k, l
		(double[]){
		 -12 ,     0 ,     0 ,     0,
		  75 ,   -12 ,     0 ,     0,
		-200 ,    75 ,  -118 ,     0,
		 300 ,  -200 ,   735 ,  -133,
		-300 ,   300 , -1940 ,   780,
		 137 ,  -300 ,  2980 , -1680,
		   0 ,   137 , -3030 ,  5470,
		   0 ,     0 ,  1373 , -5595,
		   0 ,     0 ,     0 ,  1158,
		// --------------------------
		   0 ,     0 ,     0 ,     0,
		   0 ,     0 ,     0 ,     0,
		   0 ,     0 ,     0 ,     0,
		   0 ,     0 ,     0 ,     0,
		   0 ,     0 ,     0 ,     0,
		  60 ,     0 ,   -60 ,    30,
		   0 ,    60 ,     0 , -1860,
		   0 ,     0 ,   600 , -1530,
		   0 ,     0 ,     0 ,   600 }
	},
	{
		"Tendler6", 6, 6, 4,	// name, p, k, l
		(double[]){
		  10 ,     0 ,     0 ,     0,
		 -72 ,   202 ,     0 ,     0,
		 225 , -1455 ,   195 ,     0,
		-400 ,  4550 , -1399 ,   285,
		 450 , -8100 ,  4340 , -2039,
		-360 ,  9150 , -7540 ,  6225,
		 147 , -7277 ,  8905 ,-10360,
		   0 ,  2930 , -7445 , 18455,
		   0 ,     0 ,  2944 ,-14865,
		   0 ,     0 ,     0 ,  2299,
		// --------------------------
		   0 ,     0 ,     0 ,     0,
		   0 ,     0 ,     0 ,     0,
		   0 ,     0 ,     0 ,     0,
		   0 ,     0 ,     0 ,     0,
		   0 ,     0 ,     0 ,     0,
		   0 ,     0 ,     0 ,     0,
		  60 ,   -60 ,  -420 ,   180,
		   0 ,  1200 ,   -60 , -4080,
		   0 ,     0 ,  1200 , -4680,
		   0 ,     0 ,     0 ,  1200 }
	},
	{
		"Tendler7", 7, 7, 4,	// name, p, k, l
		(double[]){
		  -60 ,     0 ,     0 ,     0,
		  490 ,   -60 ,     0 ,     0,
		-1764 ,   490 ,  -210 ,     0,
		 3675 , -1764 ,  1722 ,  -774,
		-4900 ,  3675 , -6235 ,  6349,
		 4410 , -4900 , 13100 ,-22988,
		-2940 ,  4410 ,-17650 , 48160,
		 1089 , -2940 , 17710 ,-66290,
		    0 ,  1089 ,-11297 , 68159,
		    0 ,     0 ,  2860 ,-42364,
		    0 ,     0 ,     0 ,  9748,
		// ---------------------------
		    0 ,     0 ,     0 ,     0,
		    0 ,     0 ,     0 ,     0,
		    0 ,     0 ,     0 ,     0,
		    0 ,     0 ,     0 ,     0,
		    0 ,     0 ,     0 ,     0,
		    0 ,     0 ,     0 ,     0,
		    0 ,     0 ,     0 ,     0,
		  420 ,     0 ,  -600 ,   840,
		    0 ,   420 , -1860 , -2100,
		    0 ,     0 ,  1200 , -8400,
		    0 ,     0 ,     0 ,  4200 }
	},
	{
		"Tischer2", 2, 2, 2,	// name, p, k, l
		(double[]){
		0.2713503499466539, S * 0.2713503499466539,
		-1.271350349946654, S * -1.271350349946654,
		1, -1 + S,
		0, 1,
		// --------
		-0.06067517497332695, -0.04894502624599904 + S * (-0.06067517497332695),
		0.2143248250266731, 0.1728900524919981 + S * 0.2143248250266731,
		0.575, 0.3010549737540010 + S * 0.575,
		0, 0.575 }
	},
	{
		"Tischer3", 3, 3, 2,	// name, p, k, l
		(double[]){
		-0.7156809323463300, S * (-0.7156809323463300),
		2.421409496232843, S * 2.421409496232843,
		-2.705728563886513, 0.4195749238763092 + S *(-2.705728563886513),
		1, -1.419574923876309 + S,
		0, 1,
		// --------
		-0.8629888978651531, 1.894838801355351+ S * (-0.8629888978651531),
		3.166634912306544, -6.952885160409694+ S * 3.166634912306544,
		-4.013693645981575, 9.231466454815180 + S * (-4.013693645981575),
		1.72, -5.312995019637147 + S * 1.72,
		0, 1.72 }
	},
	{
		"Tischer4", 4, 4, 2,	// name, p, k, l
		(double[]){
		0.6103461100271559, S * 0.6103461100271559,
		-2.747685823192436, S * (-2.747685823192436),
		4.670005693524193, -0.2716147139534076 + S * 4.670005693524193,
		-3.532665980358913, 1.222768338553090 + S * (-3.532665980358913),
		1, -1.951153624599682 + S,
		0, 1,
		// --------
		0.9215366381763085, -2.649958179621567 + S * 0.9215366381763085,
		-4.136159579898865, 11.89388403771322 + S * (-4.136159579898865),
		7.141520549769378, -20.89280076921304 + S * 7.141520549769378,
		-5.652569985267611, 17.85562740240295 + S * (-5.652569985267611),
		1.72, -7.606291401927840 + S * 1.72,
		0, 1.72 }
	},
	{
		"Tischer5", 5, 5, 2,	// name, p, k, l
		(double[]){
		-0.5473524843400159, S * (-0.5473524843400159),
		3.046981165805116, S * 3.046981165805116,
		-6.842547445085438, 0.2023989966807180 + S * (-6.842547445085438),
		7.741845845969376, -1.126707101014822 + S * 7.741845845969376,
		-4.398927082349038, 2.473981498420040 + S * (-4.398927082349038),
		1, -2.549673394085936 + S,
		0, 1,
		// --------
		-0.8285692507199495, 2.569479392269974 + S * (-0.8285692507199495),
		4.523688780360865, -14.02842923277731 + S * 4.523688780360865,
		-10.05367226203171, 31.45494082222007 + S * (-10.05367226203171),
		11.42213178164391, -36.93603791002108 + S * 11.42213178164391,
		-6.661863565106905, 23.96920129089773 + S * (-6.661863565106905),
		1.59, -8.446918649021926 + S * 1.59,
		0, 1.59 }
	},
	{
		"Tischer6", 6, 6, 2,	// name, p, k, l
		(double[]){
		0.5065949241573893, S * 0.5065949241573893,
		-3.347588901033099, S * (-3.347588901033099),
		9.288264853658161, -0.1622565792899431 + S * 9.288264853658161,
		-13.85423667736953, 1.072194564235031 + S * (-13.85423667736953),
		11.70833137691048, -2.943247343299985+ S * 11.70833137691048,
		-5.301365576323402, 4.228025197181846 + S * (-5.301365576323402),
		1, -3.194715838826949 + S,
		0, 1,
		// --------
		0.6632827080818050, -1.966557927018182 + S * 0.6632827080818050,
		-4.318145132370032, 12.80281007873031 + S * (-4.318145132370032),
		11.87138330994840, -35.39751311755192 + S * 11.87138330994840,
		-17.69684560038281, 53.77255375657074 + S * (-17.69684560038281),
		15.14109874111104, -48.44295713982840 + S * 15.14109874111104,
		-7.068045626188859, 26.08926183188078 + S * (-7.068045626188859),
		1.4, -8.166685368910522 + S * 1.4,
		0, 1.4 }
	},
	{
		"Tischer7", 7, 7, 2,	// name, p, k, l
		(double[]){
		-0.4730377247001383, S * (-0.4730377247001383),
		3.614094099926330, S * 3.614094099926330,
		-11.91361867598596, 0.1354428064536342 + S * (-11.91361867598596),
		21.97700424011387, -1.034807631868773 + S * 21.97700424011387,
		-24.50710155920918, 3.390826065383960 + S * (-24.50710155920918),
		16.51027888261980, -6.137117459840426 + S * 16.51027888261980,
		-6.207619262764723, 6.516126407008639 + S * (-6.207619262764723),
		1, -3.870470187137034 + S,
		0, 1,
		// --------
		-0.5593384312526297, 1.627202025358370+ S * (-0.5593384312526297),
		4.220709442337714, -12.27869670539260 + S * 4.220709442337714,
		-13.79410919873106, 40.28383410923075 + S * (-13.79410919873106),
		25.36722912840438, -74.96406486441271 + S * 25.36722912840438,
		-28.42514847816480, 86.48642725198855 + S * (-28.42514847816480),
		19.46252654187500, -63.47949128834042 + S * 19.46252654187500,
		-7.551726936498641, 29.33643618328776 + S * (-7.551726936498641),
		1.275, -8.238999899992440 + S * 1.275,
		0, 1.275 }
	},
	{
		"Tischer8", 8, 8, 2,	// name, p, k, l
		(double[]){
		0.4453348513468461, S * 0.4453348513468461,
		-3.859180665772141, S * (-3.859180665772141),
		14.71642002045547, -0.1162654110659919 + S * 14.71642002045547,
		-32.27324489323404, 1.007532253824141 + S * (-32.27324489323404),
		44.53945382773937, -3.827901007455769 + S * 44.53945382773937,
		-39.61766259292482, 8.302866483037190 + S * (-39.61766259292482),
		22.16853315999479, -11.16589138289488 + S * 22.16853315999479,
		-7.119653707605472, 9.369883714651345 + S * (-7.119653707605472),
		1, -4.570224650096031 + S,
		0, 1,
		// --------
		0.4822252401396465, -1.384350259109908 + S * 0.4822252401396465,
		-4.136942315524837, 11.87614560523851 + S * (-4.136942315524837),
		15.65983206216709, -45.07917311093609 + S * 15.65983206216709,
		-34.21855612352479, 99.29376589604582 + S * (-34.21855612352479),
		47.30113434219221, -139.7912993045485 + S * 47.30113434219221,
		-42.45139314249457, 130.5230230565463 + S * (-42.45139314249457),
		24.20929805095255, -81.18525412979233 + S * 24.20929805095255,
		-8.028548065406828, 33.00040235056584 + S * (-8.028548065406828),
		1.18, -8.408425274884485 + S * 1.275,
		0, 1.18 }
	},
	{
		"Mihelcic4", 4, 3, 2,	// name, p, k, l
		(double[]){
		0.635, 0,
		4.08, -21299.0 / 63500,
		-5.715, 0.288,
		1, -60489.0 / 63500,
		0, 1,
		// --------
		0, 0,
		-653.0 / 200, 4203.0 / 25400,
		-1.63, 10527.0 / 127000,
		109.0 / 200, 94929.0 / 127000,
		0, 0.387 }
	},
	{
		"Mihelcic5", 5, 3, 3,	// name, p, k, l
		(double[]){
		1.473, 0, 0,
		8.784, -1.369091006228046872693999232562, 0,
		-13.257, 2.544, 0.363,
		3, -2.174908993771953127306000767438, -2.160,
		0, 1, -1.203,
		0, 0, 3,
		// --------
		0, 0, 0,
		-7.347, 0.6040303354093489575646664108540, 0,
		-2.874, 0.2391213416373958302586656434162, -0.086,
		1.491, -0.1299696645906510424353335891459, 0.061,
		0, 0.481, 3.424,
		0, 0, 1.035 }
	},
	{
		"Mihelcic6", 6, 4, 4,	// name, p, k, l
		(double[]){
		-0.9390362339466378776258361005001e-01,                                      0,                                      0,                                      0,
		 0.2307741600964885363988695519087e+00,  0.1313792557365642037980205785897e+01,                                      0,                                      0,
		 0.6362705032057367094700091337420e+00, -0.5218540536524916920611801547836e+01,  0.1394364937701534512038572085087e+00,                                      0,
		-0.1773141039907661458106295075601e+01,  0.7529417145415851180555472647834e+01, -0.1011623223444230192358671688990e+01,  0.1444191297888636803281533656209e+01,
		 1,                                     -0.4624669166256576297923876885895e+01,  0.2533821653452779618403940682161e+01, -0.5492130726047090045832910083724e+01,
		 0,                                      1,                                     -0.2661634923778702877249126201679e+01,  0.7660459268273149438357977410280e+01,
		 0,                                      0,                                      1,                                     -0.4612519840114696195806600982764e+01,
		 0,                                      0,                                      0,                                     1,
		// ------------------------------------------------------------------------------------------------------------------------------------------------------------
		-0.1853058013316520953710176130202e-02,                                      0,                                      0,                                      0,
		 0.2451624610656680441199165441716e+00, -0.6428104226828121021947147465023e+00,                                      0,                                      0,
		-0.9774386604306546531165819680760e+00,  0.1217442120880404165141230674303e+01, -0.1234001277245579249775694769490e+00,                                      0,
		 0.5535769499829904561071401752847e+00,  0.2924405192057778509076307429902e+00,  0.6120300247935214387198741562141e+00, -0.6918619210365557047027968203354e+00,
		 0.3644443541805902548632380173403e+00, -0.1439373462587944161697640697511e+01, -0.8155238085505402874283164469721e+00,  0.1168008238715629118739713863798e+01,
		 0,                                      0.5385875007216307945710071168686e+00, -0.1145523605965158361286500199803e-01,  0.4386358816535462908082534556190e+00,
		 0,                                                                          0,  0.4094644596664487699907078389984e+00, -0.1467449527693897117841289393376e+01,
		 0,                                                                          0,                                      0,  0.5438956185163976564593606928377e+00 }
	},
	{
		"Mihelcic7", 7, 5, 5,	// name, p, k, l
		(double[]){
		-0.8153861905026022737434167615414e+00,                                      0,                                      0,                                      0,                                      0,
		 0.4248895254033088383481894572139e+01, -0.2222441760654655361593826056284e-01,                                      0,                                      0,                                      0,
		-0.8854047951365927037900576200388e+01,  0.2654870121454308294304980610591e+00, -0.1032372883543017869086260071380e+00,                                      0,                                      0,
		 0.9222953551982948533098094387068e+01, -0.1205209745869234199415093321373e+01,  0.8599203816315797312780689738287e+00, -0.4503824855570349203575243869964e+00,                                      0,
		-0.4802414664147507604935996997277e+01,  0.2592699276978907532702377400625e+01, -0.2778518172270275904802809146576e+01,  0.2649630382099203346956766040160e+01,  0.3219958911202349451261006329238e-02,
		 1,                                     -0.2630752125658557609101843879749e+01,  0.4366928200996645755541775423194e+01, -0.6226227266320010195981550682429e+01,  0.1553984660531454990906076870577e-01,
		 0,                                      1,                                     -0.3345093122003647795108409243309e+01,  0.7304031887973224464306216642665e+01, -0.4533196816782826816679836557976e+00,
		 0,                                      0,                                      1,                                     -0.4277052518196382694923907613400e+01,  0.1691723344964800813261126750020e+01,
		 0,                                      0,                                      0,                                      1,                                     -0.2257163468803035030953464869257e+01,
		 0,                                      0,                                      0,                                      0,                                      1,
		// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		 0.4225456071360219714271811811639e+00,                                      0,                                      0,                                      0,                                      0,
		-0.1384364739812333541090764922689e+01,  0.5044748643224129949603468040808e-01,                                      0,                                      0,                                      0,
		 0.1146229672531143866569110401229e+01, -0.3550507135730755767219599244966e+00,  0.1003502539567659420767582244072e+00,                                      0,                                      0,
		 0.6542124190961045586099091344395e+00,  0.9776848659097670987554894143623e+00, -0.6085349671365743688808169611528e+00,  0.2643442532041219470801194385611e+00,                                      0,
		-0.1322701573944066648938040619221e+01, -0.1124773012768567816507929091140e+01,  0.1286689084300843479379774302352e+01, -0.1081991130901289033649461985260e+01,  0.3096249379217892308281396122901e-01,
		 0.4840799656531792806536461685364e+00,  0.1917873380953330674263870885019e+00, -0.9223533615854392440249726038524e+00,  0.1354222521191408219378646232897e+01, -0.2269170368480450921510442892867e+00,
		 0,                                      0.3700608846337565198520459335580e+00, -0.2298469653682757407333327433246e+00, -0.9237493311630675669214627664954e-01,  0.7904598622265127771615281344478e+00,
		 0,                                      0,                                      0.3969920978990539400467297585933e+00, -0.8904512454964667161729702886816e+00, -0.1202094468615905428211278510487e+01,
		 0,                                      0,                                      0,                                      0.4473119757158579081724970288316e+00,  0.4061703817752004671762456256483e+00,
		 0,                                      0,                                      0,                                      0,                                      0.3568354106010698554863523086895e+00 }
	},
	{
		"DonelsonHansen1", 5, 3, 3,	// name, p, k, l
		(double[]){
		0, 0, 0,
		-57, 1083, 0,
		24, -456, 0,
		33, -1347, -57,
		0, 720, 24,
		0, 0, 33,
		// --------
		-1, 0, 0,
		24, -350, 0,
		57, -1347, -1,
		10, 456, 24,
		0, 251, 57,
		0, 0, 10 }
	},
	{
		"DonelsonHansen2", 6, 3, 3,	// name, p, k, l
		(double[]){
		-11, 0, 0,
		-27, 188, 0,
		27, -36, 25,
		11, -252, -63,
		0, 100, -9,
		0, 0, 47,
		// --------
		3, 0, 0,
		27, -60, 0,
		27, -252, -9,
		3, 36, -9,
		0, 36, 63,
		0, 0, 15 }
	},
	{
		"DonelsonHansen3", 6, 3, 3,	// name, p, k, l
		(double[]){
		0, 0, 0,
		-57, 136, 0,
		24, -117, -283,
		33, -144, -306,
		0, 125, 531,
		0, 0, 58,
		// --------
		-1, 0, 0,
		24, -45, 0,
		57, -144, 84,
		10, 117, 531,
		0, 42, 306,
		0, 0, 9 }
	},
	{
		"DonelsonHansen4", 5, 3, 2,	// name, p, k, l
		(double[]){
		0, 0,
		-57, 31,
		24, -12,
		33, -39,
		0, 20,
		// --------
		-1, 0,
		24, -10,
		57, -39,
		10, 12,
		0, 7 }
	},
	{
		"DonelsonHansen5", 7, 4, 4,	// name, p, k, l
		(double[]){
		    0,      0,      0,      0,
		-1360,  13409,      0,      0,
		-1350,  30384,   3653,      0,
		 2160, -55026, -22752,   4550,
		  550,   2224, -45792,  14160,
		    0,   9009,  49888, -14850,
		    0,      0,  15003,  -5360,
		    0,      0,      0,   1500,
		// ---------------------------
		   -9,      0,      0,      0,
		  456,  -3585,      0,      0,
		 2376, -32904,  -1182,      0,
		 1656, -19008,   1440,  -1191,
		  141,  16008,  49032, -12456,
		    0,   2529,  42144, -13176,
		    0,      0,   3906,    744,
		    0,      0,      0,    459 }
	},
	{
		"DonelsonHansen6", 8, 4, 4,	// name, p, k, l
		(double[]){
		    0,      0,         0,         0,
		-1360,   4603,         0,         0,
		-1350,   1008,  -3455657,         0,
		 2160, -28242, -28102272,     59160,
		  550,  15728,  -5942052,    175440,
		    0,   6903,  31623488,   -201690,
		    0,      0,   5876493,    -55920,
		    0,      0,         0,     23010,
		// ---------------------------------
		   -9,      0,         0,         0,
		  456,  -1293,         0,         0,
		 2376,  -8136,    789744,         0,
		 1656,   9936,  15276816,    -15543,
		  141,  16968,  40314888,   -159048,
		    0,   1845,  20558640,   -156168,
		    0,      0,   1449972,     20232,
		    0,      0,         0,      6867 }
	}
};

#define NF	(sizeof(F) / sizeof(F[0]))



void printmat(const char *s, double a[], int n) {
	int i, j;
	puts(s);
	for (i=0; i<n; ++i) {
		printf("\t");
		for (j=0; j<n; ++j)
			printf("%13.5f",a[i+j*n]);
		puts("");
	}
}



void plainOutput(int i, complex lambda, double xmin, double widlw) {
	printf("\t%5d\t%16.8f\t%16.8f,\t%9.5f\t%9.5f\n",i,creal(lambda),cimag(lambda),xmin,widlw);
}



void jsOutput(int i, complex lambda, double xmin, double widlw) {
	printf("\t[%.8f, %.8f],\n",creal(lambda),cimag(lambda));
}



void js3d(int i, complex lambda, double xmin, double widlw) {
	printf("\t[%.8f, %.8f, 1],\n",creal(lambda),cimag(lambda));
}



int main (int argc, char *argv[]) {
	int c, colLen, debug=0, i, j, k, n, nr=20, nsq, l, nrest, rest, outp='p';
	int flag, q, nu, iq, iqm1;
	formula_t *fm = NULL;
	double eps, rho1, phi, phiinc, rlambda, ilambda, xmin, xmax, ymax, widlw, a0[N*N], a1[N*N], b0[N*N], b1[N*N];
	double x, y, xstep, ystep;
	char buf[64];
	// for LAPACK QZ method
	int lda=N, ldb=N, ldvl=N, ldvr=N, lwork=N*N, info=0;
	char jobvl[8], jobvr[8], cmach[8];
	complex eiphi, a[N*N], b[N*N], alpha[N], beta[N], work[8*N], vl[N*N], vr[N*N], lambda;
	double rwork[8*N];
	void (*prtf)(int i, complex lambda, double xmin, double widlw) = plainOutput;

	// Pick formula
	while ((c = getopt(argc,argv,"D:db:hm:o:r:s:t:")) != -1) {
		switch(c) {
		case 'D':	// Donelson & Hansen cyclic multistep formulas
			k = atoi(optarg);
			if (k < 1  ||  k > 6) {
				printf("%s: method-number %d out of range, only 1 to 6 allowed\n",argv[0],k);
				return 1;
			}
			sprintf(buf,"DonelsonHansen%d",k);
			for (i=0; i<NF; ++i)	// linear search for formula
				if (strcmp(buf,F[i].name) == 0) {
					fm = &F[i];
					break;
				}
			break;
		case 'b':	// BDF
			k = atoi(optarg);
			if (k < 1  ||  k > 6) {
				printf("%s: order %d out of range, only 1 to 6 allowed\n",argv[0],k);
				return 1;
			}
			sprintf(buf,"BDF%d",k);
			for (i=0; i<NF; ++i)	// linear search for formula
				if (strcmp(buf,F[i].name) == 0) {
					fm = &F[i];
					break;
				}
			break;
		case 'd':
			debug = 1;
			break;
		case 'h':
			printf("%s: compute stability regions for various formulas.\n"
			"-D: Donelson & Hansen formulas\n"
			"-b: BDF\n"
			"-d: debug\n"
			"-h: this help\n"
			"-m: Mihelcic cyclic formulas\n"
			"-o: output format in either 'p' (plain) or 'j' (Javascript) or '3' (3-dimensional)\n"
			"-r: number of records\n"
			"-s: Tischer / Sacks-Davis cyclic formulas\n"
			"-t: Tendler's cyclic multistep formulas\n", argv[0]);
			return 0;
		case 'm':	// Mihelcic's cyclic formulas
			k = atoi(optarg);
			if (k < 4  ||  k > 7) {
				printf("%s: order %d out of range, only 4 to 7 allowed\n",argv[0],k);
				return 1;
			}
			sprintf(buf,"Mihelcic%d",k);
			for (i=0; i<NF; ++i)	// linear search for formula
				if (strcmp(buf,F[i].name) == 0) {
					fm = &F[i];
					break;
				}
			break;
		case 'o':	// output format
			outp = optarg[0];
			switch (outp) {
			case '3':
				prtf = js3d;
				break;
			case 'j':
				prtf = jsOutput;
				break;
			case 'p':
				prtf = plainOutput;
				break;
			default:
				printf("%s: output format %c unknown, only p (for plain) or j (for JavaScript) or 3 (for 3-dimensional) allowed\n",argv[0],outp);
				return 2;
			}
			break;
		case 'r':	// number of eigenvalue computations
			nr = atoi(optarg);
			if (nr <= 0) {
				nr = 20;
				printf("%s: nr<=0, silently set to %d\n",argv[0],nr);
			}
			break;
		case 's':	// Tischer + Sacks-Davis cyclic formulas
			k = atoi(optarg);
			if (k <= 1  ||  k > 8) {
				printf("%s: order %d out of range, only 2 to 8 allowed\n",argv[0],k);
				return 1;
			}
			sprintf(buf,"Tischer%d",k);
			for (i=0; i<NF; ++i)	// linear search for formula
				if (strcmp(buf,F[i].name) == 0) {
					fm = &F[i];
					break;
				}
			break;
		case 't':	// Tendler's cycles
			k = atoi(optarg);
			if (k < 3  ||  k > 7) {
				printf("%s: order %d out of range, only 3 to 7 allowed\n",argv[0],k);
				return 3;
			}
			sprintf(buf,"Tendler%d",k);
			for (i=0; i<NF; ++i)	// linear search for formula
				if (strcmp(buf,F[i].name) == 0) {
					fm = &F[i];
					break;
				}
			break;
		default:
			printf("%s: illegal option %c\n",argv[0],c);
			return 4;
		}
	}
	if (fm == NULL) fm = &F[0];	// default formula
	//prtf = (outp == 'p') ? plainOutput : jsOutput;

	if (debug) {
		// Print out all formulas
		for (i=0; i<NF; ++i) {
			l = F[i].l;
			printf("%s, p=%d, k=%d, l=%d\n",F[i].name,F[i].p,F[i].k,l);
			n = 2 * (F[i].k + l);
			for (k=0; k<n; ++k) {
				printf("\t");
				for (j=0; j<l; ++j)
					printf("%13.4f",F[i].a[k*l+j]);
				puts("");
			}
			printf("rho_0\t");	// rho(1)=0, that's what it should be
			flag = 0;
			for (j=0; j<l; ++j) {
				rho1 = 0;
				n = F[i].k + F[i].l;
				for (k=0; k<n; ++k)
					rho1 += F[i].a[k*l+j];
				if (rho1 != 0) flag = 1;
				printf("%22.9f",rho1);
			}
			puts(flag ? "\t<-----" : "");
			// Compute consistency check up to the order of the formula
			for (q=1; q<=F[i].p+1; ++q) {
				printf("rho_%d\t",q);	// rho_q = \sum \alpha_i i^q - q \sum \beta_i i^{q-1}, should be zero
				flag = 0;
				n = F[i].k + F[i].l;
				for (j=0; j<l; ++j) {
					rho1 = 0.0;
					for (k=0; k<n; ++k) {
						iq = 1, iqm1 = 1;
						for (nu=1; nu<q; ++nu) iqm1 *= k;
						iq = iqm1 * k;
						rho1 += F[i].a[k*l+j] * iq - q * F[i].a[(n+k)*l+j] * iqm1;
					}
					if (rho1 != 0) flag = 1;
					printf("%22.9f",rho1);
				}
				puts(flag ? "\t<-----" : "");
			}
		}
	}

	// Fill the four sqaure A0, A1 and B1, B2 matrices: A1 y(yn+1) + A0 y(n) = B1 z(n+1) + B0 z(n)
	// Matrices in column order, i.e., Fortran mode
	if (fm->k >= fm->l) {
		rest = fm->k - fm->l,
		n = fm->k,
		nrest = fm->l,
		nsq = fm->k * fm->k;
	} else {
		rest = fm->l - fm->k,
		n = fm->l,
		nrest = fm->k,
		nsq = fm->l * fm->l;
	}
	if (n >= N) {
		printf("%s: n=%d equals or exceeds N=%d\n",argv[0],n,N);
		return 5;
	}

	cmach[0] = 'E';
	eps = 10 * dlamch_(cmach);
	colLen = 2 * (fm->k + fm->l);

	memset(a0,0,sizeof(*a0) * nsq);	// set a0[] to zero matrix
	memset(b0,0,sizeof(*b0) * nsq);	// set b0[] to zero matrix
	memset(a1,0,sizeof(*a1) * nsq);	// set a1[] to zero matrix
	memset(b1,0,sizeof(*b1) * nsq);	// set b1[] to zero matrix
	if (debug) printf("k=%d, l=%d, rest=%d, n=%d, nrest=%d, nsq=%d, colLen=%d, nr=%d, eps=%g\n",fm->k,fm->l,rest,n,nrest,nsq,colLen,nr,eps);
	for (i=0; i<rest; ++i) {
		a1[i+i*n] = 1;	// fill with parts of identity matrix
		if (i + nrest < n) a0[i+(i+nrest)*n] = -1;	// cancel out
	}
	for (i=rest; i<n; ++i) {
		for (j=0; j<n; ++j)
			a0[i+j*n] = fm->a[(i-rest)+j*fm->l],
			b0[i+j*n] = fm->a[(i-rest)+((j+fm->l)+n)*fm->l];
		for (j=rest; j<n; ++j)
			a1[i+j*n] = fm->a[(i-rest)+((j-rest)+n)*fm->l],
			b1[i+j*n] = fm->a[(i-rest)+((j-rest+fm->l)+2*n)*fm->l];
	}

	if (debug) {
		// Print A1, A0, B1, B0
		puts(fm->name);
		printmat("A1",a1,n);
		printmat("A0",a0,n);
		printmat("B1",b1,n);
		printmat("B0",b0,n);
	}

	jobvl[0] = 'N';	// do not compute the left generalized eigenvectors
	jobvr[0] = 'N'; // do not compute the right generalized eigenvectors
	lda = n, ldb = n, ldvl = n, ldvr = n, lwork = n*n;
	if (lwork <= 1) lwork = 8;
	xmin = 0, xmax = 0, ymax = 0, widlw = M_PI_2;
	if (outp == 'j' || outp == '3') printf("var data_%s = [\n",fm->name);

	for (i=0,phiinc=2*M_PI/nr,phi=0; i<nr; phi+=phiinc,++i) {
		eiphi = cexp(I * phi);
		for (j=0; j<nsq; ++j)	// A = A1 e^{i\phi} + A0, same for B
			a[j] = a1[j] * eiphi + a0[j],
			b[j] = b1[j] * eiphi + b0[j];
		info = 0;
		zggev_(jobvl, jobvr, &n, a, &lda, b, &ldb, alpha, beta, vl, &ldvl, vr, &ldvr, work, &lwork, rwork, &info);
		if (info) printf("zggev_.info=%d\n",info);
		for (j=0; j<n; ++j) {
			if (beta[j] == 0) continue;
			lambda = alpha[j] / beta[j];
			rlambda = creal(lambda),
			ilambda = cimag(lambda),
			xmin = fmin(xmin,rlambda),	// Widlund distance delta
			xmax = fmax(xmax,rlambda),
			ymax = fmax(ymax,fabs(ilambda));
			if (rlambda < 0
			&&  fabs(rlambda) > eps && fabs(ilambda) > eps)
					widlw = fmin(widlw,fabs(atan(ilambda/rlambda)));	// Widlund wedge in radians
			prtf(i,lambda,xmin,180/M_PI*widlw);
		}
	}

	if (outp == 'j') printf("];\nvar title_%s = { text: '%s:  p=%d, delta=%.6f, alpha=%.6f', left: `center` };\n"
		"var chartDom_%s = document.getElementById('container_%s');\n"
		"var myChart_%s = echarts.init(chartDom_%s);\n"
		"var option_%s = { xAxis: {}, yAxis: {}, tooltip: { trigger: 'axis' }, title: title_%s, series: ["
			" { data: data_%s, symbolSize: 2, type: 'scatter', } ] };\n"
		"option_%s && myChart_%s.setOption(option_%s);\n\n",
		fm->name,fm->name,fm->p,xmin,180/M_PI*widlw,
		fm->name,fm->name,
		fm->name,fm->name,
		fm->name,fm->name,fm->name,
		fm->name,fm->name,fm->name);
	else if (outp == '3') {	// 3D output, i.e., stability mountain
		printf("];\nvar data3d_%s = [\n",fm->name);
		xmin = -1.4 * fmax(fabs(xmin),xmax),
		xmax = 1.1 * xmax,
		ymax = 1.1 * ymax,
		nr = 4.1 * sqrt(nr);
		xstep = (xmax - xmin) / nr, ystep = 2 * ymax / nr;
		for (x=xmin; x<=xmax; x+=xstep) {
			for (y=-ymax; y<=ymax; y+=ystep) {
				for (j=0; j<nsq; ++j)
					a[j] = a0[j] - (x+I*y) * b0[j],
					b[j] = (x+I*y) * b1[j] - a1[j];
#if 0
				for (i=0; i<n; ++i) {
					printf("\t\t");
					for (j=0; j<n; ++j)
						printf(" (%.4f,%.4f)",creal(a[i+j*n]),cimag(a[i+j*n]));
					printf("\n");
				}
				for (i=0; i<n; ++i) {
					printf("\t\t");
					for (j=0; j<n; ++j)
						printf(" (%.4f,%.4f)",creal(b[i+j*n]),cimag(b[i+j*n]));
					printf("\n");
				}
#endif
				info = 0;
				zggev_(jobvl, jobvr, &n, a, &lda, b, &ldb, alpha, beta, vl, &ldvl, vr, &ldvr, work, &lwork, rwork, &info);
				if (info) printf("zggev_.info=%d\n",info);
				for (j=0; j<n; ++j) {
					//printf("alpha[%d]=(%.6f,%.6f), beta[%d]=(%.6f,%.6f)\n",j,creal(alpha[j]),cimag(alpha[j]),j,creal(beta[j]),cimag(beta[j]));
					if (beta[j] == 0) continue;
					//if (cabs(beta[j]) <= eps) continue;
					rlambda = cabs(alpha[j] / beta[j]);
					if (rlambda > 2.3) rlambda = 2.3;
					printf("\t[%.8f, %.8f, %.8f],\n", x, y, rlambda);
				}
			}
		}
		printf("];\n"
			"var option_%s = {\n"
			"\ttooltip: {},\n"
			"\tvisualMap: {\n"
			"\t\tshow: false,\n"
			"\t\tdimension: 2,\n"
			//"\t\tinRange: { color: [ '#313695', '#4575b4', '#74add1', '#abd9e9', '#e0f3f8', '#ffffbf', '#fee090', '#fdae61', '#f46d43', '#d73027', '#a50026' ] }\n"
			"\t},\n"
			"\txAxis3D: { type: 'value' },\n"
			"\tyAxis3D: { type: 'value' },\n"
			"\tzAxis3D: { type: 'value' },\n"
			"\tgrid3D: { viewControl: { projection:'perspective' } },\n"
			"\tseries: [\n"
			"\t\t{ type:'surface', wireframe: { show:true }, data: data3d_%s },\n"
			"\t\t{ type:'scatter3D', symbolSize:2, data: data_%s }\n"
			"\t]\n"
			"}\n"
			"var chartDom_%s = document.getElementById('container_%s');\n"
			"var myChart_%s = echarts.init(chartDom_%s);\n"
			"option_%s && myChart_%s.setOption(option_%s);\n\n",
			fm->name,
			fm->name,
			fm->name,
			fm->name,fm->name,
			fm->name,fm->name,
			fm->name,fm->name,fm->name);
	}

	return 0;
}

