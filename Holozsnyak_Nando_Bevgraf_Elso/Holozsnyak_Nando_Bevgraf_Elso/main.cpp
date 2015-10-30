#include <GLFW\glfw3.h>
#include "Header.h"
#include <math.h>
#include <time.h> 

#define PI 3.141592653589793

typedef struct vector2d { GLdouble x, y; } VECTOR2D;

typedef struct point2d { GLdouble x, y; } POINT2D;

typedef struct circle2d { GLdouble x, y, r; } CIRCLE2D;

GLFWwindow* window;

typedef struct circle {
	Vector<GLdouble> *M = new Vector<GLdouble>(2);
	GLdouble x, y, r, mass;
	int cS; //circle State - ez mutatja hogy a k�r sz�rke 0 , feh�r 1 vagy fekete 2 
} CIRCLE;

bool gameRunning = true;
int winner = 0;
int circleNum = 15;
int circleRemaining = circleNum-1;
int circleRadius = 16;
int circleMass = 100;
GLsizei winWidth = 800, winHeight = 600;

GLdouble updateFrequency = 0.01, lastUpdate;
CIRCLE *cArr = new CIRCLE[circleNum];
CIRCLE *Players = new CIRCLE[2];
int PlayerPoints[2] = { 0 };

double fRand(double fMin, double fMax)
{
	double f = (double)rand() / RAND_MAX;
	return fMin + f * (fMax - fMin);
}
VECTOR2D initVector2D(GLdouble x, GLdouble y) {
	VECTOR2D P;
	P.x = x;
	P.y = y;
	return P;
}

POINT2D initPoint2D(GLdouble x, GLdouble y) {
	POINT2D P;
	P.x = x;
	P.y = y;
	return P;
}

CIRCLE2D initCircle2D(GLdouble x, GLdouble y, GLdouble r) {
	CIRCLE2D P;
	P.x = x;
	P.y = y;
	P.r = r;
	return P;
}
bool checkCircleSpawn() {
	bool spawnedTogether = false;
	return spawnedTogether;
}
void init()
{
    glClearColor (0.3, 0.3, 0.3, 0.0);	

    glMatrixMode (GL_PROJECTION);	
	glLoadIdentity();               
	glOrtho(0.f, winWidth, 0.f, winHeight, 0.f, 1.f);
}

void circle(CIRCLE O) {

	glBegin(GL_TRIANGLE_FAN);
	for (GLdouble t = 0; t <= 2 * PI; t += 0.01)
		glVertex2f(O.x + O.r * cos(t), O.y + O.r * sin(t));
	glEnd();
}
int szor = 15;
void circleVector(CIRCLE O) {
	/*Vector<GLdouble> a(2);
	a << O.r << O.r;
	double C = (Vector<GLdouble>::innerMultiply(a, *O.M))/(a.Distance()*O.M->Distance());*/
	
	glBegin(GL_LINES);
		glVertex2f(O.x, O.y);
		glVertex2f(O.x+O.M->GetValue(0)*O.r, O.y+ O.M->GetValue(1)*O.r);
	glEnd();

	glBegin(GL_TRIANGLE_FAN);
	for (GLdouble t = 0; t <= 2 * PI; t += 0.01)
		glVertex2f(O.x + O.M->GetValue(0) * cos(t), O.y + O.M->GetValue(1) * sin(t));
	glEnd();
}

void Mirror2X(Vector<GLdouble> *T) {
	Vector<double> normal(2);
	normal.Reset();
	normal << 1 << 0;
	*T = T->Mirror(normal);
}

void Mirror2Y(Vector<GLdouble> *T) {
	Vector<double> normal(2);
	normal.Reset();
	normal << 0 << 1;
	*T = T->Mirror(normal);
}

void MoveBalls() {
	
	for (int id = 0; id < circleNum; id++) {
		if (cArr[id].x + cArr[id].r + cArr[id].M->GetValue(0) > winWidth || cArr[id].x - cArr[id].r + cArr[id].M->GetValue(0) < 0)
		{
			*cArr[id].M = *cArr[id].M * (-1);
			Mirror2X(cArr[id].M);
		}
		else if (cArr[id].y + cArr[id].r + cArr[id].M->GetValue(1) > winHeight || cArr[id].y - cArr[id].r + cArr[id].M->GetValue(1) < 0)
		{
			*cArr[id].M = *cArr[id].M * (-1);
			Mirror2Y(cArr[id].M);			
		}

		cArr[id].x += cArr[id].M->GetValue(0);
		cArr[id].y += cArr[id].M->GetValue(1);
	}
		
}

void MovePlayers() {
	
	for (int id = 0; id < 2; id++) {
		Players[id].x += Players[id].M->GetValue(0);
		Players[id].y += Players[id].M->GetValue(1);
		if (Players[id].x + Players[id].r > 800 || Players[id].x - Players[id].r < 0)
		{
			*Players[id].M = *Players[id].M * (-1);
			Mirror2X(Players[id].M);
		}
		if (Players[id].y + Players[id].r > 600 || Players[id].y - Players[id].r < 0)
		{
			*Players[id].M = *Players[id].M * (-1);
			Mirror2Y(Players[id].M);
		}
				
	}


}

double pointDistance(POINT2D p1, POINT2D p2) {
	double sum = 0;
	sum += (p2.x - p1.x)*(p2.x - p1.x);
	sum += (p2.y - p1.y)*(p2.y - p1.y);
	return (double) std::sqrt(sum);
}

void collidee(CIRCLE & a, CIRCLE & b) {

	Vector<GLdouble> w(2);

	Vector<GLdouble> smaller(2);
	if (a.M->Distance() > b.M->Distance())
	{
		smaller = *b.M;
		GLdouble x = b.x - a.x, y = b.y - a.y;
		w << x << y;
	} else {
		smaller = *a.M;
		GLdouble x = a.x - b.x, y = a.y - b.y;
		w << x << y;
	}
	*a.M = *a.M - smaller;
	*b.M = *b.M - smaller;

	Vector<GLdouble> m_1(2);
	Vector<GLdouble> u1(2);
	Vector<GLdouble> m2(2);
	Vector<GLdouble> a2(2);
	GLdouble r;

	if (a.M->Distance()) //ha nem nulla akkor A mozog 
	{
		//ekkor b az �ll� k�r :)
		r = a.mass / b.mass;
		m_1 = a.M->projectTo(w);
		u1 = *a.M - m_1;
		m2 = (m_1 + u1) * ((r - 1) / (r + 1)); //mozg� k�r �j sebess�gvektora
		a2 = (m_1 * (2 * r)) / (r + 1); // az �ll� k�r �j sebess�gvektora
		m2 = m2 + smaller;
		a2 = a2 + smaller;
		*a.M = m2;
		*b.M = a2;


	}
	else //ford�tva
	{
		r = b.mass / a.mass;
		m_1 = b.M->projectTo(w);
		u1 = *b.M - m_1;
		m2 = (m_1 + u1) * ((r - 1) / (r + 1)); // the moving circle's new speedvector
		a2 = (m_1 * (2 * r)) / (r + 1); // the standing circle's new speedvector
		m2 = m2 + smaller;
		a2 = a2 + smaller;
		*a.M = a2;
		*b.M = m2;

	}
}

void PlayerCollideWithCircle(int id) {

	for (int i = 0; i < circleNum; i++) {
		if (pointDistance(initPoint2D(cArr[i].x, cArr[i].y), initPoint2D(Players[id].x, Players[id].y)) < Players[id].r + cArr[i].r && cArr[i].cS == 0) {
			collidee(cArr[i], Players[id]);
			cArr[i].r = 0;
			cArr[i].x = -100;
			cArr[i].y = -100;
			cArr[i].M->Reset();
			PlayerPoints[id]++;
			circleRemaining--;
			std::cout << "Piros: " << PlayerPoints[0] << "\t K�k:" << PlayerPoints[1] << std::endl;
			
		}
		//feh�r goly�
		if (pointDistance(initPoint2D(cArr[i].x, cArr[i].y), initPoint2D(Players[id].x, Players[id].y)) < Players[id].r + cArr[i].r && cArr[i].cS == 1) {
			collidee(cArr[i], Players[id]);
			cArr[i].r = 0;
			cArr[i].x = -100;
			cArr[i].y = -100;
			cArr[i].M->Reset();
			PlayerPoints[id] += 5;
			circleRemaining--;
			std::cout << "feher goly� kil�ve" << std::endl;
			std::cout << "Piros: " << PlayerPoints[0] << "\t K�k:" << PlayerPoints[1] << std::endl;
			
		}
		//fekete goly�
		if (pointDistance(initPoint2D(cArr[i].x, cArr[i].y), initPoint2D(Players[id].x, Players[id].y)) < Players[id].r + cArr[i].r && cArr[i].cS == 2) {
			collidee(cArr[i], Players[id]);
			cArr[i].r = 0;
			cArr[i].x = -100;
			cArr[i].y = -100;
			cArr[i].M->Reset();
			winner = id == 1 ? 0 : 1;
			gameRunning = false;
			std::cout << "fekete goly� kil�ve" << std::endl;
			
		}


	}
	//std::cout << id << "-edik j�tkos pontjai:" << PlayerPoints[id] << std::endl;
}

void checkCollide() {
	//a = m1
	//b = w
	//ap = m_1
	//am = u1
	//Az a vektor b-vel p�rhuzamos komponense : a_p=((a*b)/|b|^2)*b
	//Az a vektor b - re mer�leges komponense : a_m = a-a_p

	for (int i = 0; i < circleNum; i++) {
		for (int j = i; j < circleNum; j++) {
			if (i != j)
				if (pointDistance(initPoint2D(cArr[i].x + cArr[i].M->GetValue(0), cArr[i].y + cArr[i].M->GetValue(1)), initPoint2D(cArr[j].x + cArr[j].M->GetValue(0), cArr[j].y + cArr[j].M->GetValue(1)))  < cArr[i].r + cArr[j].r) {
					collidee(cArr[i], cArr[j]);
				}
		}
	}

	for (int i = 0; i < 2; i++) {
		PlayerCollideWithCircle(i);
		for (int j = i; j < 2; j++) {
			if (i != j) {
				if (pointDistance(initPoint2D(Players[i].x + Players[i].M->GetValue(0), Players[i].y + Players[i].M->GetValue(1)), initPoint2D(Players[j].x + Players[j].M->GetValue(0), Players[j].y + Players[j].M->GetValue(1)))  < Players[i].r + Players[j].r) {
					collidee(Players[i], Players[j]);
				}
			}
		}
	}
}




void simpleKeyCheck(GLFWwindow * window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_A && action == GLFW_PRESS) {
		Mirror2X(Players[0].M);
	}
	if (key == GLFW_KEY_S && action == GLFW_PRESS) {
		Mirror2Y(Players[0].M);
	}
	if (key == GLFW_KEY_K && action == GLFW_PRESS) {
		Mirror2X(Players[1].M);
	}
	if (key == GLFW_KEY_L && action == GLFW_PRESS) {
		Mirror2Y(Players[1].M);
	}
}
void winnerColor() {
	glClear(GL_COLOR_BUFFER_BIT);
	if (winner == 0) {
		glClearColor(1,0,0,0);
	}
	else if (winner == 1) {
		glClearColor(0, 0, 1, 0);
	}
}
void draw()
{
	glfwSetKeyCallback(window, simpleKeyCheck);
	double now = glfwGetTime();
	if (now - lastUpdate > updateFrequency) {
		MoveBalls();
		MovePlayers();
		checkCollide();

		if (circleRemaining == 0) {
			winner = PlayerPoints[0] > PlayerPoints[1] ? 0 : 1;
			gameRunning = false;
		}
		lastUpdate = now;
	}

	glClear(GL_COLOR_BUFFER_BIT);
	
	for (int i = 0; i < circleNum-2; i++) {
		glColor3f(0.6f, 0.6f, 0.6f);
		circle(cArr[i]);
		glColor3f(1.0, 0.4, 0.2);
		circleVector(cArr[i]);
	}

	glColor3f(0, 0, 0);
	circle(cArr[circleNum - 2]);

	glColor3f(0.0, 0.4, 0.2);
	circleVector(cArr[circleNum - 2]);

	glColor3f(1, 1, 1);
	circle(cArr[circleNum - 1]);

	glColor3f(0.0, 0.4, 0.2);
	circleVector(cArr[circleNum - 1]);

	//red - 0
	glColor3f(1, 0, 0);
	circle(Players[0]);

	glColor3f(0.0, 0.4, 0.2);
	circleVector(Players[0]);

	//blue - 1
	glColor3f(0, 0, 1);
	circle(Players[1]);

	glColor3f(0.0, 0.4, 0.2);
	circleVector(Players[1]);

	glFlush ( );
}

void initCircle(int i, int Code) {
	GLdouble x, y, k, l;
	
	if (Code == 0) {
		x = rand() % (winWidth - circleRadius*2) + circleRadius;
		y = rand() % (winHeight - circleRadius*2) + circleRadius;
		k = fRand(-2, 2);
		l = fRand(-2, 2);
		cArr[i].r = circleRadius;
		cArr[i].x = x;
		cArr[i].y = y;
		cArr[i].cS = 0;
		cArr[i].mass = circleMass;
		cArr[i].M->AddItem(k);
		cArr[i].M->AddItem(l);
		std::cout << *cArr[i].M << std::endl;
	} else if (Code == 1 || Code == 2) { //1 feh�r 2 fekete
		x = rand() % (winWidth - circleRadius * 2) + circleRadius;
		y = rand() % (winHeight - circleRadius * 2) + circleRadius;
		k = fRand(-2, 2);
		l = fRand(-2, 2);
		cArr[i].r = circleRadius;
		cArr[i].x = x;
		cArr[i].y = y;
		cArr[i].cS = Code;
		cArr[i].mass = circleMass;
		cArr[i].M->AddItem(k);
		cArr[i].M->AddItem(l);
		std::cout << *cArr[i].M << std::endl;
	} else if (Code == 3) {
		x = rand() % (winWidth - circleRadius * 2) + circleRadius;
		y = rand() % (winHeight - circleRadius * 2) + circleRadius;
		k = fRand(-2, 2);
		l = fRand(-2, 2);
		Players[i].r = circleRadius;
		Players[i].x = x;
		Players[i].y = y;
		Players[i].cS = 2;
		Players[i].mass = circleMass/2;
		Players[i].M->AddItem(k);
		Players[i].M->AddItem(l);
	}
}


int main (int argc, char** argv)
{
	srand(time(NULL));
	for (int i = 0; i < circleNum-2; i++) {
		initCircle(i,0);
	}
	initCircle(circleNum - 1, 1);
	initCircle(circleNum - 2, 2);
	initCircle(0, 3);
	initCircle(1, 3);
	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(winWidth, winHeight, "Circle", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	init();

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		if (gameRunning) {
			draw(); /* Render here */
		}
		else {
			winnerColor();
		}

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}
	std::cout << "Pontok:" << std::endl;
	glfwTerminate();

    return 0;
}
