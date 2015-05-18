#include"ThreeDObj.h"

#include<gl/glut.h>

#include<cmath>
#include<cstdio>
#include<ctime>

using namespace std;

enum State
{
	CHOOSECTRL,
	SETTRANS,
	DEFORMING
};

#define MAXTAGSIZE 32

State curState=CHOOSECTRL;
char stateTag[3][MAXTAGSIZE]=
{
	{"Choose Control Vertices"}, 
	{"Set Transformation"},
	{"Deforming..."}
};

bool lattice=false;
float mtrBuf[16];

char objFileName[MAXFILENAMESIZE];
char objFilePath[MAXFILEPATHSIZE];

ThreeDObj *mesh;	//人头mesh数据
double longitude=-16.998, latitude=28.86;		//经纬度，用于在球面上变换视点
double zoom=580;	//缩放参数
int wWidth,wHeight;	//窗口尺寸
bool wire=true;
int curGNum=0;
bool isMark=true;
Mat transformMtr(4, 4, CV_32FC1);

void initRender(void)
{
	glClearColor(0.0,0.0,0.0,0.0);	//设置清屏所用的颜色为黑色，完全透明
	glEnable(GL_DEPTH_TEST);		//启动深度测试，否则前后关系将由画的顺序决定
	glEnable(GL_CULL_FACE);			//禁用多边形正面或者背面上的光照、阴影和
	glCullFace(GL_BACK);			//颜色计算及操作，消除不必要的渲染计算
	glEnable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	/*glEnable(GL_LIGHTING);
	GLfloat white[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat light_pos[] = {500,500,500,1};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,white);
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, white);
	glEnable(GL_LIGHT0);*/
}

void setTex(void)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mesh->getTexImg().cols, mesh->getTexImg().rows, 
		0, GL_BGR_EXT, GL_UNSIGNED_BYTE, mesh->getTexImg().data);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

void drawObject(void)
{
	if(wire)
	{
		//draw coordinates
		glBegin(GL_LINES);
		{
			glColor3f(0.0f, 1.0f, 0.0f);
			glVertex3f(0.0f, 0.0f, 0.0f);
			glVertex3f((float)zoom, 0.0f, 0.0f);
			glVertex3f(0.0f, 0.0f, 0.0f);
			glVertex3f(0.0f, (float)zoom, 0.0f);
			glVertex3f(0.0f, 0.0f, 0.0f);
			glVertex3f(0.0f, 0.0f, (float)zoom);

			glColor3f(1.0f, 1.0f, 1.0f);
		}
		glEnd();
	}

	glBegin(GL_TRIANGLES);
	{
		for(int groupI=0;groupI<mesh->getGroupAmt();groupI++)
		{
			if(groupI==curGNum&&isMark)
			{
				if(!wire)
				{
					glTexCoord2f(0.0f, 0.0f);
				}
				else
				{
					glColor3f(1.0f, 0.0f, 0.0f);
				}
				for(int indexI=mesh->getgStartRIndex()[groupI]; 
					indexI<mesh->getgStartRIndex()[groupI+1]; indexI++)
				{
					glVertex3fv((float*)(mesh->getVertex()+mesh->getVRenderIndex()[indexI]));
				}
			}
			else
			{
				for(int indexI=mesh->getgStartRIndex()[groupI]; 
					indexI<mesh->getgStartRIndex()[groupI+1]; indexI++)
				{
					if(!wire)
					{
						glTexCoord2fv((float*)(mesh->getTexCoord()+mesh->getTRenderIndex()[indexI]));
					}
					else
					{
						glColor3f(1.0f, 1.0f, 1.0f);
					}
					glVertex3fv((float*)(mesh->getVertex()+mesh->getVRenderIndex()[indexI]));
				}
			}
		}
	}
	glEnd();
/*
	glBegin(GL_TRIANGLES);
	{
		for(int i=0;i<mesh->getFaceAmt()*3;i++)
		{
			glTexCoord2fv((float *)(mesh->getTexCoord()+mesh->getTRenderIndex()[i]));
			glVertex3fv((float *)(mesh->getVertex()+mesh->getVRenderIndex()[i]));
		}
	}
	glEnd();
*/
}

void drawLattice(void)
{
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_LINES);
	set<int>::iterator graphIter;
	for(int vertexI=mesh->getVertexAmt()+1;vertexI<mesh->getVertexAmt()+1+343+512;vertexI++)
	{
		for(graphIter=mesh->getGraph()[vertexI].begin();graphIter!=mesh->getGraph()[vertexI].end();graphIter++)
		{
			glVertex3fv((float*)(mesh->getVertex()+vertexI));
			glVertex3fv((float*)(mesh->getVertex()+*graphIter));
		}
	}
	glEnd();
}

void getFPS(void)
{
	static int frame = 0, time, timebase = 0;
	static char buffer[256];

	frame++;
	time=glutGet(GLUT_ELAPSED_TIME);
	if (time - timebase > 1000)
	{
		sprintf(buffer,"FPS:%4.2f, %s",frame*1000.0/(time-timebase), stateTag[curState]);
		timebase = time;		
		frame = 0;
	}

	glutSetWindowTitle(buffer);
}

void render(void)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity(); 
    
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	//清屏

	if(!lattice)
	{
		drawObject();
	}
	else
	{
		drawLattice();
	}

    glutSwapBuffers();

	//Just for FPS tests
	getFPS();	
}

void updateView(int width, int height)
{
	glViewport(0,0,width,height);	// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);	//指定投影矩阵为当前矩阵
    glLoadIdentity();				//重置当前指定的矩阵为单位矩阵

	//创建一个投影矩阵并且与当前矩阵相乘,得到的矩阵设定为当前变换
    gluPerspective(60,(GLfloat)width/(GLfloat)height,1,10001);	

	gluLookAt(	//眼睛坐标
				zoom*cos(latitude/180*3.1415026)*sin(longitude/180*3.1415026),
				zoom*sin(latitude/180*3.1415026),
				zoom*cos(latitude/180*3.1415026)*cos(longitude/180*3.1415026),
				0,0,0,	//相机镜头对准点坐标
				0,cos(latitude/180*3.1415026),0);	//相机向上的方向的向量坐标
}

void reshape(int width, int height)
{
	if (height==0)	// Prevent A Divide By Zero By
	{
		height=1;	// Making Height Equal One
	}

	wHeight = height;
	wWidth = width;

	updateView(wWidth, wHeight);
}

void keyboard(int key,int x,int y)
{
	switch(key)
	{
		case GLUT_KEY_RIGHT:
			if(cos(latitude/180*3.1415026)<0)
				longitude-=3.0f;
			else
				longitude+=3.0f;

			if(longitude>=180.0f)
				longitude=-180.0f;
			break;

		case GLUT_KEY_LEFT:
			if(cos(latitude/180*3.1415026)<0)
				longitude+=3.0f;
			else
				longitude-=3.0f;

			if(longitude<=-180.0f)
				longitude=180.0f;
			break;

		case GLUT_KEY_UP:	
			latitude+=3.0f;	
			if(latitude>=180.0f)
				latitude=-180.0f; 	
			break;

		case GLUT_KEY_DOWN:	
			latitude-=3.0f;	
			if(latitude<=-180.0f)
				latitude=180.0f; 	
			break;
		

		//缩放功能：（改成鼠标滚轮缩放会更好！）
		case GLUT_KEY_F1:
			zoom-=10;
			if(zoom<10)
			{
				zoom=10;
			}
			break;

		case GLUT_KEY_F2:
			zoom+=10;
			break;

		case GLUT_KEY_F3:
			if(curState==CHOOSECTRL)
			{
				curGNum++;
				if(curGNum>=mesh->getGroupAmt())
				{
					curGNum=0;
				}
			}
			break;

		case GLUT_KEY_F4:
			curState=SETTRANS;
			render();
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			//glTranslatef(0.0f, 0.0f, 230.0f);
			glRotatef(15.0f, 1.0f, 0.0f, 0.0f);
			//glTranslatef(0.0f, 0.0f, -230.0f);
			glGetFloatv(GL_MODELVIEW_MATRIX, mtrBuf);
			glPopMatrix();
			for(int rowI=0; rowI<4; rowI++)
			{
				for(int colI=0; colI<4; colI++)
				{
					transformMtr.at<float>(rowI, colI)=mtrBuf[colI*4+rowI];
				}
			}
			mesh->setControl(curGNum, transformMtr);
			break;
  
		case GLUT_KEY_F5:
			if(curState==SETTRANS)
			{
				curState=DEFORMING;
				clock_t start=clock();
				mesh->deform(SIGMOID);
				clock_t end=clock();
				float fraction=(float)(end-start)/CLOCKS_PER_SEC;
				int sec=(int)fraction;
				fraction-=(float)sec;
				printf("Time used: %dh %dm %fs\n", sec/3600, sec/60%60, sec%60+fraction);
				mesh->write("../3D Objects/deformed.obj");
				curState=CHOOSECTRL;
			}
			break;

		case GLUT_KEY_F6:
			mesh->remeshing();
			break;

		case GLUT_KEY_F7:
			printf("Saving path:\n../3D Objects/");
			scanf("%s", objFileName);
			sprintf(objFilePath, "../3D Objects/");
			strcat(objFilePath, objFileName);
			mesh->write(objFilePath);
			break;

		case GLUT_KEY_F8:
			if(wire)
			{
				wire=false;
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
			else
			{
				wire=true;
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}
			break;

		case GLUT_KEY_F9:
			{
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadIdentity();
				glRotatef(-15.0f, 0.0f, 1.0f, 0.0f);
				glGetFloatv(GL_MODELVIEW_MATRIX, mtrBuf);
				glPopMatrix();
				for(int rowI=0; rowI<4; rowI++)
				{
					for(int colI=0; colI<4; colI++)
					{
						transformMtr.at<float>(rowI, colI)=mtrBuf[colI*4+rowI];
					}
				}
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->write("../3D Objects/cuboid8l_linear15mul6.obj");
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->write("../3D Objects/cuboid8l_linear15mul12.obj");
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->write("../3D Objects/cuboid8l_linear15mul18.obj");
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->write("../3D Objects/cuboid8l_linear15mul24.obj");
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->write("../3D Objects/cuboid8l_linear15mul30.obj");
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->setControl(curGNum, transformMtr);
				mesh->deform(LINEAR);
				mesh->write("../3D Objects/cuboid8l_linear15mul36.obj");
			}
			break;

		case GLUT_KEY_F10:
			curState=SETTRANS;
			render();
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			glRotatef(-16.875f, 0.0f, 1.0f, 0.0f);
			glGetFloatv(GL_MODELVIEW_MATRIX, mtrBuf);
			glPopMatrix();
			for(int rowI=0; rowI<4; rowI++)
			{
				for(int colI=0; colI<4; colI++)
				{
					transformMtr.at<float>(rowI, colI)=mtrBuf[colI*4+rowI];
				}
			}
			mesh->twist(transformMtr);
			break;

		case GLUT_KEY_F11:
			mesh->buildLattice();
			break;

		case GLUT_KEY_F12:
			delete mesh;
			printf("Wavefront OBJ file path:\n../3D Objects/");
			scanf("%s", objFileName);
			sprintf(objFilePath, "../3D Objects/");
			strcat(objFilePath, objFileName);
			mesh=new ThreeDObj(objFilePath);
			setTex();
			curState=CHOOSECTRL;
			curGNum=0;
			break;
	}

	updateView(wWidth,wHeight);
}

//参数为当前鼠标的位置，该函数会在鼠标键处于按下状态时被不断调用
void action(int x,int y)
{
	static int x0=0,y0=0;	//上次调用时鼠标的位置

	if((x-x0<100&&x0-x<100))
	{
		if(cos(latitude/180*3.1415026)<0)
			longitude+=(x-x0)/3.5;
		else
			longitude-=(x-x0)/3.5;

		if(longitude>=180.0f)
			longitude=-180.0f;
		else if(longitude<=-180.0f)
			longitude=180.0f;
 	}

	if((y-y0<100&&y0-y<100))
	{
		latitude+=(y-y0)/3.5;
		if(latitude>=180.0f)
			latitude=-180.0f;
		else if(latitude<=-180.0f)
			latitude=180.0f;
 	}
	
	updateView(wWidth,wHeight);
	x0=x;
	y0=y;
}

void OnTimer(int timerNum)
{
	isMark=!isMark;

	glutTimerFunc(500, OnTimer, 0);
}

//Just for FPS tests
void idle()
{
	glutPostRedisplay();
}

int main(int argc, char *argv[])
{
	//Init OpenGL
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);

	//Init window
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(640, 480);
	glutCreateWindow("Displayer");

INPUTFILEPATH:
	printf("Wavefront OBJ file path:\n../3D Objects/");
	scanf("%s", objFileName);
	sprintf(objFilePath, "../3D Objects/");
	strcat(objFilePath, objFileName);
	try
	{
		mesh=new ThreeDObj(objFilePath);
	}
	catch(exception e)
	{
		delete mesh;
		puts("File opening failure.");
		goto INPUTFILEPATH;
	}
	
	initRender();		//初始化渲染设置
	setTex();			//设置纹理贴图参数

	glutDisplayFunc(render);	//渲染函数
	glutMotionFunc(action);		//键盘消息处理函数
	glutSpecialFunc(keyboard);	//鼠标消息处理函数
	glutReshapeFunc(reshape);	//窗口变化函数
	glutTimerFunc(500, OnTimer, 0);
		
	//Just for FPS tests
	glutIdleFunc(idle);	

	glutMainLoop();

	return 0;
}