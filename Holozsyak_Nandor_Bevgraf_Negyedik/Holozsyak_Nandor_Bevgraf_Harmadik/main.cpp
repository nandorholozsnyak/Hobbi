#include <GLFW\glfw3.h>                           // (or others, depending on the system in use)
#include <cmath>
#include <iostream>
#include <stdio.h>
#include <Torusz.h>
#include <vector>
#include <thread>
#include <chrono>

/*======================================*/

/**
* 3D vektor strukt�ra
*/
typedef struct { GLdouble x, y, z; } VECTOR3;


/**
* visszadja az (x,y,z) vektort
*/
VECTOR3 initVector3(GLdouble x, GLdouble y, GLdouble z) {
	VECTOR3 P;

	P.x = x;
	P.y = y;
	P.z = z;

	return P;
}

/*
* 3D vektor vektorok
*/
std::vector<VECTOR3> rajzolhatoPontok;
std::vector<VECTOR3> transformedPontok;

/*----------------------------------------------------------------------------------------*/

/**
* 4D vektor
*/
typedef struct { GLdouble x, y, z, w; } VECTOR4;

/*
* 4D vektor vektorok
*/
std::vector<VECTOR4> pontok4d;

/**
* visszadja az (x,y,z,w) vektort
*/
VECTOR4 initVector4(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
	VECTOR4 P;

	P.x = x;
	P.y = y;
	P.z = z;
	P.w = w;

	return P;
}

/*----------------------------------------------------------------------------------------*/


/**
* egy laphoz tartoz� cs�csok indexei
*/
typedef struct { GLint v[4]; VECTOR3 faceCenter; } FACE;

std::vector<FACE> faceTorusz;

/**
* visszadja a (v0,v1,v2,v3) indexekhez tartoz� lapot
*/
FACE initFace(GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
	FACE f;
	GLdouble x, y, z;
	f.v[0] = v0;
	f.v[1] = v1;
	f.v[2] = v2;
	f.v[3] = v3;
	x = (pontok4d[v0].x + pontok4d[v1].x + pontok4d[v2].x + pontok4d[v3].x) / 4;
	y = (pontok4d[v0].y + pontok4d[v1].y + pontok4d[v2].y + pontok4d[v3].y) / 4;
	z = (pontok4d[v0].z + pontok4d[v1].z + pontok4d[v2].z + pontok4d[v3].z) / 4;

	f.faceCenter = initVector3(x, y, z);

	return f;
}

/*------------------------------------------------------------------------------------------------------------------*/

/**
* 4x4 m�trix, sorfolytonosan t�rolva
*/
typedef GLdouble MATRIX4[4][4];


/*======================================*/

//4D-s vektor inhomog�n koordin�t�kra val� �t�ll�sa azaz az els� h�rom koordin�nt�t x,y,z osztjuk az utols�val w -vel.
VECTOR3 convertToInhomogen(VECTOR4 vector) {
	return initVector3(vector.x / vector.w, vector.y / vector.w, vector.z / vector.w);
}
//3D-b�l 4D-be val� �t�ll�s, utols� koordin�ta 1.
VECTOR4 convertToHomogen(VECTOR3 vector) {
	return initVector4(vector.x, vector.y, vector.z, 1.0);
}
//Vektor hossz�t adja vissza
GLdouble getVectorLength(VECTOR3 vec) {
	return sqrt(pow(vec.x, 2) + pow(vec.y, 2) + pow(vec.z, 2));
}
//Normaliz�s egy vektort, elosztja a vektor komponenseit a vektor hossz�val
VECTOR3 normalizeVector(VECTOR3 vector) {
	GLdouble length = getVectorLength(vector);
	return initVector3(vector.x / length, vector.y / length, vector.z / length);
}
//bels� szorzat, X1*X2+Y1*Y2+Z1*Z2
GLdouble dotProduct(VECTOR3 a, VECTOR3 b) {
	return (a.x * b.x + a.y * b.y + a.z * b.z);
}
//Vektori�lis szorzat, determin�ns k�plet alapj�n kij�n.
VECTOR3 crossProduct(VECTOR3 a, VECTOR3 b) {
	return initVector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
/**
* visszaadja az 'a' vektorb�l a 'b' vektorba mutat� vektort,
* azaz a 'b-a' vektort
*/
VECTOR3 vecSub(VECTOR3 a, VECTOR3 b) {
	return initVector3(
		b.x - a.x,
		b.y - a.y,
		b.z - a.z);
}

/*======================================*/
/**
* felt�lti az A m�trixot az egys�gm�trixszal
*/
void initIdentityMatrix(MATRIX4 A)
{
	int i, j;

	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			A[i][j] = 0.0f;

	for (i = 0; i < 4; i++)
		A[i][i] = 1.0f;
}

/**
* felt�lti az A m�trixot a (0,0,s) k�z�ppont� centr�lis vet�t�s m�trixsz�val
*/
void initPersProjMatrix(MATRIX4 A, GLdouble s)
{
	initIdentityMatrix(A);

	A[2][2] = 0.0f;
	A[3][2] = -1.0f / s;
}

/**
* felt�lti az A m�trixot a (wx,wy):(wx+ww,wy+wh) ablakb�l (vx,vy):(vx+vw,vy+vh) n�zetbe
* t�rt�n� transzform�ci� m�trixsz�val
*/
void initWtvMatrix(MATRIX4 A, GLdouble wx, GLdouble wy, GLdouble ww, GLdouble wh,
	GLdouble vx, GLdouble vy, GLdouble vw, GLdouble vh)
{
	initIdentityMatrix(A);

	A[0][0] = vw / ww;
	A[1][1] = vh / wh;
	A[0][3] = -wx*(vw / ww) + vx;
	A[1][3] = -wy*(vh / wh) + vy;
}
/*
* l�trehozza a view m�trixot
*/
void initViewMatrix(MATRIX4 A, VECTOR3 eye, VECTOR3 center, VECTOR3 up) {
	initIdentityMatrix(A);

	// a kamerabol az iranyt meghatarozo pontba mutato vektor
	// center az a pont,amely fele a kamerat tartjuk, eye a kamera hely�t adja
	VECTOR3 centerMinusEye = initVector3(center.x - eye.x, center.y - eye.y, center.z - eye.z);

	// a fenti vektor -1 szeres�nek egyseg hosszra norm�ltja, ebbol lesz a kamera rendszer�nek z-tengelye
	VECTOR3 f = normalizeVector(initVector3(-centerMinusEye.x, -centerMinusEye.y, -centerMinusEye.z));

	// az up vektor es a leendo z tengelyirany vektorialis szorzata adja a kamera x-tengelyiranyat
	VECTOR3 s = normalizeVector(crossProduct(up, f));

	// a kamera y tengelyiranya a mar elkeszult z irany es x irany vektorialis szorzatak�nt j�n ki
	VECTOR3 u = crossProduct(f, s);

	//Az �j b�zisvektorok, s(x),u(y),f(z)
	//A teljes m�trix:
	//A << s.x << s.y << s.z << -<s,eye>
	//A << u.x << u.y << u.z << -<u,eye>
	//A << f.x << f.y << f.z << -<f,eye>
	//A << 0 << 0 << 0 << 1

	A[0][0] = s.x;
	A[0][1] = s.y;
	A[0][2] = s.z;
	A[1][0] = u.x;
	A[1][1] = u.y;
	A[1][2] = u.z;
	A[2][0] = f.x;
	A[2][1] = f.y;
	A[2][2] = f.z;
	A[0][3] = -dotProduct(s, eye);
	A[1][3] = -dotProduct(u, eye);
	A[2][3] = -dotProduct(f, eye);
}


/**
* visszaadja az A m�trix �s a v vektor szorzat�t, A*v-t
*/
VECTOR4 mulMatrixVector(MATRIX4 A, VECTOR4 v) {
	return initVector4(
		A[0][0] * v.x + A[0][1] * v.y + A[0][2] * v.z + A[0][3] * v.w,
		A[1][0] * v.x + A[1][1] * v.y + A[1][2] * v.z + A[1][3] * v.w,
		A[2][0] * v.x + A[2][1] * v.y + A[2][2] * v.z + A[2][3] * v.w,
		A[3][0] * v.x + A[3][1] * v.y + A[3][2] * v.z + A[3][3] * v.w);
}

/**
* felt�lti a C m�trixot az A �s B m�trixok szorzat�val, A*B-vel
*/
void mulMatrices(MATRIX4 A, MATRIX4 B, MATRIX4 C) {
	int i, j, k;

	GLdouble sum;
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++) {
			sum = 0;
			for (k = 0; k < 4; k++)
				sum = sum + A[i][k] * B[k][j];
			C[i][j] = sum;
		}
}

/*======================================*/

/**
* ablak m�rete
*/
GLdouble winWidth = 800.0f, winHeight = 600.0f;

double PI = 3.14159265358979323846;
MATRIX4 view;

/**
* mer�leges �s centr�lis vet�t�sek m�trixai
*/
MATRIX4 Centralis;

/**
* Wtv m�trixok
*/
MATRIX4 WindowToView;

/**
* seg�dm�trixok
*/
MATRIX4 Tmp, TempR, TempE;

/*
* forgat�si m�trixok
*/
MATRIX4 Rx, Ry, Rz;

/*
* eltol�si m�trix
*/
MATRIX4 Eltolas;

/*
* sk�l�z�s m�trixa
*/
MATRIX4 Skalazas;

MATRIX4 Sik;

/**
* a m�trix szorzat l�ncokb�l l�trej�v� v�geleges m�trixok.
*/
MATRIX4 Tcx, Tcy, Tcz, TC[4];


GLdouble alpha = 3.14f / 2, deltaAlpha = 3.14f / 180.0f;

/**
* n�zet koordin�t�i
*/
//GLdouble cX = (winWidth - winHeight) / 2.0f + 150, cY = 0, cW = winHeight / 2, cH = winHeight / 2 - 100;
GLdouble cX = 100, cY = 0, cW = 600, cH = 500;

/**
* centr�lis vet�t�s k�z�ppontj�nak Z koordin�t�ja
*/
GLdouble center = 6.0f;

//itt inicializ�l�dik az eltol�si m�trix
void initEltolasMatrix(double x, double y, double z) {
	initIdentityMatrix(Eltolas);

	/*
	<<1<< 0 << 0 << x
	<< 0 << 1 << 0 << y
	<< 0 << 0 << 1 << z
	<< 0 << 0 << 0 << 1;
	*/

	Eltolas[0][3] = x;
	Eltolas[1][3] = y;
	Eltolas[2][3] = z;
}
//itt inicializ�l�dik a sk�l�zsi m�trix
void initSkalazasMatrix(double x, double y, double z) {
	//initIdentityMatrix(S);
	Skalazas[0][0] = x;
	Skalazas[1][1] = y;
	Skalazas[2][2] = z;
	Skalazas[3][3] = 1;
}
void initRotationMatrixes(double alpha) {
	initIdentityMatrix(Rx);
	initIdentityMatrix(Ry);
	initIdentityMatrix(Rz);

	/*X:
	<< 1 << 0 << 0 << 0
	<< 0 << std::cos(alpha) << -std::sin(alpha) << 0
	<< 0 << std::sin(alpha) << std::cos(alpha) << 0
	<< 0 << 0 << 0 << 1;
	*/
	Rx[1][1] = std::cos(alpha);
	Rx[1][2] = -std::sin(alpha);
	Rx[2][1] = std::sin(alpha);
	Rx[2][2] = std::cos(alpha);

	/*
	Y:
	<< std::cos(alpha) << 0 << std::sin(alpha) << 0
	<< 0 << 1 << 0 << 0
	<< -std::sin(alpha) << 0 << std::cos(alpha) << 0
	<< 0 << 0 << 0 << 1;
	*/

	Ry[0][0] = std::cos(alpha);
	Ry[0][2] = std::sin(alpha);
	Ry[2][0] = -std::sin(alpha);
	Ry[2][2] = std::cos(alpha);

	/*
	Z:
	<< std::cos(alpha) << -std::sin(alpha) << 0 << 0
	<< std::sin(alpha) << std::cos(alpha) << 0 << 0
	<< 0 << 0 << 1 << 0
	<< 0 << 0 << 0 << 1;
	*/
	Rz[0][0] = std::cos(alpha);
	Rz[0][1] = -std::sin(alpha);
	Rz[1][0] = std::sin(alpha);
	Rz[1][1] = std::cos(alpha);

}

double deltaA = 0.001;

/**
* el��ll�tja a sz�ks�ges m�trixokat
*/
void initTransformations()
{

	// vet�t�si m�trixok
	initPersProjMatrix(Centralis, center);

	// Wtv m�trixok
	initWtvMatrix(WindowToView, -0.5f, -0.5f, 1.0f, 1.0f, cX, cY, cW, cH);

	//T = W�C�K
	mulMatrices(Centralis, view, Tmp);
	mulMatrices(WindowToView, Tmp, Sik);
	//M = W�C�K�L�R, t�rusz m�trixa el�bb forgatom, eltolom, kamera, centr�lis vet�t�s �s w2v
	//T�ruszok
	initRotationMatrixes(deltaA);
	initEltolasMatrix(0, 0, 0);

	// t�rusz - X tengely k�r�li forg�ssal
	mulMatrices(Eltolas, Rx, TempR);
	mulMatrices(view, TempR, TempE);
	mulMatrices(Centralis, TempE, Tmp);
	mulMatrices(WindowToView, Tmp, Tcx);

}

/*======================================*/


VECTOR3 eye, up, centerVec;
double R = 37;

void init()
{
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, winWidth, 0.0f, winHeight, 0.0f, 1.0f);

	eye = initVector3(0, 0, R); //megadja a kamera poz�ci�j�t (Ez legyen most a z tengely pozit�v fel�n)
	centerVec = initVector3(0.0, 0.0, 0.0); //megadja, hogy merre n�z a kamera (Ez legyen most az orig�)
	up = initVector3(0.0, 1.0, 0.0); //megdja, hogy merre van a felfele ir�ny (Ez legyen most az y tengely)

	initViewMatrix(view, eye, centerVec, up);

	initTransformations();
}


//itt fogjuk rendezni a lapokat a k�z�ppontj�nak a z koordin�t�i szerint
int comparePointsbyAtlag(const void *a, const void *b) {

	if (((*(FACE*)a).faceCenter.z < ((*(FACE*)b).faceCenter.z))) return -1;
	if (((*(FACE*)a).faceCenter.z == ((*(FACE*)b).faceCenter.z))) return  0;
	if (((*(FACE*)a).faceCenter.z > ((*(FACE*)b).faceCenter.z))) return  1;

}


double sarok = 12;
void drawGrid(VECTOR3 color, MATRIX4 T)
{
	int i, j, id = 0;
	VECTOR4 ph, pt;
	VECTOR3 pih;
	glLineWidth(1.0f);
	glColor3f(color.x, color.y, color.z);
	glBegin(GL_LINES);
	for (double i = -sarok; i <= sarok; i += sarok / 12) {
		ph = initVector4(-sarok, 0, i, 1.0f);

		// alkalmazzuk a transzform�ci�t
		pt = mulMatrixVector(T, ph);

		// visszat�r�nk inhomog�n alakra
		pih = initVector3(pt.x / pt.w, pt.y / pt.w, pt.z / pt.w);

		// kirajzoljuk a cs�csot
		glVertex2f(pih.x, pih.y);

		ph = initVector4(sarok, 0, i, 1.0f);

		// alkalmazzuk a transzform�ci�t
		pt = mulMatrixVector(T, ph);

		// visszat�r�nk inhomog�n alakra
		pih = initVector3(pt.x / pt.w, pt.y / pt.w, pt.z / pt.w);

		// kirajzoljuk a cs�csot
		glVertex2f(pih.x, pih.y);

	}
	glEnd();
	glBegin(GL_LINES);
	for (double i = -sarok; i <= sarok; i += sarok / 12) {
		ph = initVector4(i, 0, -sarok, 1.0f);

		// alkalmazzuk a transzform�ci�t
		pt = mulMatrixVector(T, ph);

		// visszat�r�nk inhomog�n alakra
		pih = initVector3(pt.x / pt.w, pt.y / pt.w, pt.z / pt.w);

		// kirajzoljuk a cs�csot
		glVertex2f(pih.x, pih.y);

		ph = initVector4(i, 0, sarok, 1.0f);

		// alkalmazzuk a transzform�ci�t
		pt = mulMatrixVector(T, ph);

		// visszat�r�nk inhomog�n alakra
		pih = initVector3(pt.x / pt.w, pt.y / pt.w, pt.z / pt.w);

		// kirajzoljuk a cs�csot
		glVertex2f(pih.x, pih.y);
	}
	glEnd();
}




void drawToruszWithPlanes(VECTOR3 color, MATRIX4 T, MATRIX4 CAM) {

	double B[4];
	double u, v;
	double c = 4, a = 2;
	VECTOR4 beforeVector, afterVector;
	VECTOR3 drawVector;
	int i = 0, j = 0;
	glColor3f(color.x, color.y, color.z);

	//2PI / (PI/6) = 12
	//2PI / (Pi/12) = 24;
	//2PI / (Pi/8) = 16;
	//2PI / (Pi/4) = 8;
	double delta = PI / 14;
	//Meghat�rozzuk a lapok sz�m�t.
	int dI = (int)((2 * PI) / (delta)), dJ = (int)((2 * PI) / (delta));
	//m�sodik v�ltoz� szerinti k�ls� ciklus hossz�s�gi k�r�ket gy�rt le
	for (u = 0; u < 2 * PI+delta; u += delta) {
		for (v = 0; v < 2 * PI; v += delta) {
			B[0] = (c + a*cos(v))*cos(u);
			B[1] = a*sin(v);
			B[2] = (c + a*cos(v))*sin(u);
			B[3] = 1.0;
			// homog�n koordin�t�s alakra hozzuk a cs�csot
			beforeVector = initVector4(B[0], B[1], B[2], B[3]);

			//el�bb a pontok4d-be belerakjuk azokat a pontokat amiket el�bb az �j koordin�ta rendszerben megkapunk hogy majd ezek alapj�n tudjunk sz�molgatni, d�nt�seket hozni hogy l�that�-e vagy sem
			afterVector = mulMatrixVector(CAM, beforeVector);
			pontok4d.push_back(afterVector);

			//itt meg vessz�k a m�r vet�tett pontunkat �s azokat is berakjuk
			afterVector = mulMatrixVector(T, beforeVector);
			drawVector = initVector3(afterVector.x / afterVector.w, afterVector.y / afterVector.w, afterVector.z / afterVector.w);
			//a pontokat amiket m�r majd rajzolhatunk azokat elrajkuk a pontok nev� t�mbbe
			rajzolhatoPontok.push_back(drawVector);	
		}
	}


	GLdouble x, y, z;
	//Itt t�rt�nik a lapok elk�sz�t�se, az elv a k�vetkez�, a pontokat legy�rtottuk �gy mint sz�less�gi k�r�k csatlakoz�si pontjai �s ezeket kell �sszepakolni lapokk�nt
	//egy p�lda az els� lapra, vagyi annak indexeire: 0,12,13,1 ha 12 pont k�sz�l egy bels� ciklus fut�sa alatt, de best�lj�k r�la dI k�nt
	//vannak k�l�nb�z� esetek amik le vannak ellen�rizve p�ld�ul minden lapnak dI poligonhalmaznak az utols� lapj�t hozz� kell k�tni az els�h�z
	//illetve az utols� dI poligonhalmazt hozz� kell k�tni az els� dI poligonhalmazhoz
	//a k�vetkez� dupla for ciklus ezeket j�tsza el
	//a sorrend mindig, bal als�, bal fels�, jobb fels�, �s jobb als� �ramutat� j�r�s�val megyez� ir�ny�
	for (i = 0; i < dI; i++) {
		for (int j = 0; j < dJ; j++) {

			FACE f;
			//ha m�r az utols� laphalmazn�l j�runk
			/*if (i == dI - 1) {
				if (j == dJ - 1) {
					f = initFace((GLint)(i * dI + j), (GLint)(j%dI), (GLint)(j%dI + 1 - dI), (GLint)(i * dI + j + 1 - dI));
				} else {
					f = initFace((GLint)(i * dI + j), (GLint)(j%dI), (GLint)(j%dI + 1), (GLint)(i * dI + j + 1));
				}
			}*/
			//ha m�g nem
			/*else {
				//ha a laphalmaz utols� lapj�n�l j�runk k�ss�k be az els�h�z.
				if (j == dJ - 1) {
					f = initFace((GLuint)(i * dI + j), (GLuint)(i * dI + j + dJ), (GLuint)(i * dI + j + dJ + 1 - dI), (GLuint)(i * dI + j + 1 - dI));
				} else {
					//bal als�, bal fels�, jobb fels�, �s jobb als�
					f = initFace((GLint)(i * dI + j), (GLint)(i * dI + j + dJ), (GLint)(i * dI + j + dJ + 1), (GLint)(i * dI + j + 1));
				}
			}*/
			//ha m�r az utols� lapn�l j�runk akkor azt visszak�tj�k az els�h�z
			if (j == dI - 1) {
				f = initFace((GLuint)(i * dI + j), (GLuint)(i * dI + j + dJ), (GLuint)(i * dI + j + dJ + 1 - dI), (GLuint)(i * dI + j + 1 - dI));
			}
			else {
				//bal als�, bal fels�, jobb fels�, �s jobb als�
				f = initFace((GLint)(i * dI + j), (GLint)(i * dI + j + dJ), (GLint)(i * dI + j + dJ + 1), (GLint)(i * dI + j + 1));
			}

			faceTorusz.push_back(f);
			//std::cout << "actual F:" << f.v[0] << " - " << f.v[1] << " - " << f.v[2] << " - " << f.v[3] << " x:" << f.faceCenter.x << " y:" << f.faceCenter.y << " z:" << f.faceCenter.z << std::endl;
		}
	}

	//itt rendezz�k a lapokat tartalmaz� vektorunkat
	qsort(&faceTorusz[0], faceTorusz.size(), sizeof(FACE), comparePointsbyAtlag);

	for (int i = 0; i < faceTorusz.size(); i++) {
		//lap �leinek meghat�roz�sa �s ezek ut�n ezeknek a vektori�lis szorzata adja a lap testb�l kifel� mutat� norm�lvektor�t
		VECTOR3 edges[2] =
		{
				vecSub(faceTorusz[i].faceCenter,
				convertToInhomogen(pontok4d[faceTorusz[i].v[1]])),
				vecSub(faceTorusz[i].faceCenter,
					convertToInhomogen(pontok4d[faceTorusz[i].v[0]]))
		};

		// itt j�n a norm�lvektor sz�m�t�s
		VECTOR3 normalVector = normalizeVector(crossProduct(edges[0], edges[1]));
		// kamer�ba mutat� vektor, amit normaliz�lnunk kell
		VECTOR3 toCamera;
		toCamera = normalizeVector(vecSub(faceTorusz[i].faceCenter, initVector3(0.0f, 0.0f, center)));
		// l�that�s�g eld�nt�se skal�ris szorzat alapj�n
		if (dotProduct(normalVector, toCamera) > 0) {
			//std::cout << i << " lap kirajzol�dik" << std::endl;
			glColor3f(0, 0, 0);
			glLineWidth(1.5);

			glBegin(GL_LINES);
			//els� vonal
			glVertex2d(rajzolhatoPontok[faceTorusz[i].v[0]].x, rajzolhatoPontok[faceTorusz[i].v[0]].y);
			glVertex2d(rajzolhatoPontok[faceTorusz[i].v[1]].x, rajzolhatoPontok[faceTorusz[i].v[1]].y);
			//m�sodik
			glVertex2d(rajzolhatoPontok[faceTorusz[i].v[1]].x, rajzolhatoPontok[faceTorusz[i].v[1]].y);
			glVertex2d(rajzolhatoPontok[faceTorusz[i].v[2]].x, rajzolhatoPontok[faceTorusz[i].v[2]].y);
			//harmadik
			glVertex2d(rajzolhatoPontok[faceTorusz[i].v[2]].x, rajzolhatoPontok[faceTorusz[i].v[2]].y);
			glVertex2d(rajzolhatoPontok[faceTorusz[i].v[3]].x, rajzolhatoPontok[faceTorusz[i].v[3]].y);
			//negyedik
			glVertex2d(rajzolhatoPontok[faceTorusz[i].v[3]].x, rajzolhatoPontok[faceTorusz[i].v[3]].y);
			glVertex2d(rajzolhatoPontok[faceTorusz[i].v[0]].x, rajzolhatoPontok[faceTorusz[i].v[0]].y);
			glEnd();

			//glColor3f((double)i / faceTorusz.size(), 0.05, 0);
			glColor3f((dotProduct(normalVector, toCamera) + 1) / 2, (dotProduct(normalVector, toCamera) + 1) / 2, (dotProduct(normalVector, toCamera) + 1) / 4);
			//glColor3f(color.x, color.y, color.z);
			glBegin(GL_POLYGON);
			glVertex2d(rajzolhatoPontok[faceTorusz[i].v[0]].x, rajzolhatoPontok[faceTorusz[i].v[0]].y);
			glVertex2d(rajzolhatoPontok[faceTorusz[i].v[1]].x, rajzolhatoPontok[faceTorusz[i].v[1]].y);
			glVertex2d(rajzolhatoPontok[faceTorusz[i].v[2]].x, rajzolhatoPontok[faceTorusz[i].v[2]].y);
			glVertex2d(rajzolhatoPontok[faceTorusz[i].v[3]].x, rajzolhatoPontok[faceTorusz[i].v[3]].y);
			glEnd();

		}

	}

	pontok4d.clear();
	rajzolhatoPontok.clear();
	faceTorusz.clear();
}

void draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	//drawGrid(initVector3(0, 0, 0), Sik);
	drawToruszWithPlanes(initVector3(0.0f, 1.0f, 0.0f), Tcx, TempE);
	deltaA += 0.01;
	initTransformations();
	glFlush();
}


void keyPressed(GLFWwindow * windows, GLint key, GLint scanCode, GLint action, GLint mods) {
	if (action == GLFW_PRESS || GLFW_REPEAT) {
		switch (key) {
		case GLFW_KEY_LEFT:
			alpha += deltaAlpha;
			eye.x = R*cos(alpha);
			//eye.y = 0;
			eye.z = R*sin(alpha);
			initViewMatrix(view, eye, centerVec, up);
			initTransformations();
			break;
		case GLFW_KEY_UP:
			eye.x = R * cos(alpha);
			eye.y += 1;
			eye.z = R * sin(alpha);
			initViewMatrix(view, eye, centerVec, up);
			initTransformations();
			break;
		case GLFW_KEY_DOWN:
			eye.x = R* cos(alpha);
			eye.y -= 1;
			eye.z = R * sin(alpha);
			initViewMatrix(view, eye, centerVec, up);
			initTransformations();
			break;
		case GLFW_KEY_RIGHT:
			alpha -= deltaAlpha;
			eye.x = R*cos(alpha);
			//eye.y = 0;
			eye.z = R*sin(alpha);
			initViewMatrix(view, eye, centerVec, up);
			initTransformations();
			break;
		case GLFW_KEY_KP_ADD:
			R -= 1;
			eye.x = R* cos(alpha);
			//eye.y = 0;
			eye.z = R * sin(alpha);
			initViewMatrix(view, eye, centerVec, up);
			initTransformations();
			break;
		case GLFW_KEY_KP_SUBTRACT:
			R += 1;
			eye.x = R* cos(alpha);
			//eye.y = 0;
			eye.z = R * sin(alpha);
			initViewMatrix(view, eye, centerVec, up);
			initTransformations();
			break;
		}
	}


	glfwPollEvents();
}

int main(int argc, char ** argv)
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(winWidth, winHeight, "4. hazi feladat", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, keyPressed);

	init();
	draw();

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		draw();

		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}