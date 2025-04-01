/*
2021.01.15 by kyoul 
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <math.h>
#include <windows.h>

#define N_hl1_Units 300
#define N_hl2_Units 100

FILE *imagecus = {nullptr}, *labelcus = {nullptr};

float (*imagebuf)[28 * 28] = new float[60005][28 * 28];
int labelbuf[60005];
float (*testimagebuf)[28 * 28] = new float[10005][28 * 28];
int testlabelbuf[10005];

float inputl[28 * 28];
float hl1[N_hl1_Units], pre_hl1[N_hl1_Units], hl1_w[28 * 28][N_hl1_Units], hl1_b[N_hl1_Units];
float hl2[N_hl2_Units], pre_hl2[N_hl2_Units], hl2_w[N_hl1_Units][N_hl2_Units], hl2_b[N_hl2_Units];
float outputl[10], pre_outputl[10], outputl_w[N_hl2_Units][10], outputl_b[10];
float dropout[N_hl1_Units+N_hl2_Units];

int Errcnt=0;
float costsum=0; 
float dasum[N_hl1_Units + N_hl2_Units + 10];
float dwsum[(28*28*N_hl1_Units) + (N_hl1_Units*N_hl2_Units) + (N_hl2_Units*10)];
float dbsum[N_hl1_Units + N_hl2_Units + 10];

unsigned char pBmpImage[28*28*3];

///////////////////////////////////////////////////////////////////////////////////////

float sig(int x){
	return 1.0/(1.0+pow(M_E,-x));
}

float dsig(int x){
	return sig(x)*(1.0 - sig(x));
}

float gaussianRandom(float average, float stdev) {
	float v1, v2, s, temp;
	do{
		v1 =  2 * ((float) rand() / RAND_MAX) - 1;      // -1.0 ~ 1.0 ������ ��
		v2 =  2 * ((float) rand() / RAND_MAX) - 1;      // -1.0 ~ 1.0 ������ ��
		s = v1 * v1 + v2 * v2;
		
	} while (s >= 1 || s == 0);
	s = sqrt( (-2 * log(s)) / s );
	temp = v1 * s;
	temp = (stdev * temp) + average;
	return temp;
}

///////////////////////////////////////////////////////////////////////////////////////

void LoadBmp24()
{
	BITMAPFILEHEADER bfh; // ��Ʈ�� ��������� �����մϴ�.
	BITMAPINFOHEADER bih; // ��Ʈ�� ��������� �����մϴ�.
	memset(&bfh, 0, sizeof(BITMAPFILEHEADER)); // ����ü�� �ʱ�ȭ�մϴ�.
	memset(&bih, 0, sizeof(BITMAPFILEHEADER));
	unsigned char trash[12]; // ������� �����͸� �о� ���� �迭�Դϴ�.
	int trash_width; // 4bite ������ ���� ���� ������� �������� ũ�⸦ ��Ÿ���ϴ�.
	int temp; // 4�� ���� ������ ���� �����ϴµ� ����մϴ�.
	FILE *fp; // ���� �������Դϴ�.
	fp = fopen("file.bmp", "rb");
	if(fp == NULL)	return;
	
	fread(&bfh, sizeof(BITMAPFILEHEADER), 1, fp);
	
	if(bfh.bfType != 0x4D42)	{
		fclose(fp);
		return;
	}
	
	fread(&bih, sizeof(BITMAPINFOHEADER), 1, fp);
	
	if(bih.biBitCount != 24){
		fclose(fp);
		return;
	}

	if((temp = (bih.biWidth*3) % 4) == 0)
		fread(pBmpImage, bih.biWidth*3, bih.biHeight, fp);
	fclose(fp);
}

void drawimageBmp(unsigned char *buf){
	printf("\n\n");
	for (int i = 27; i > 0; i--){
		for (int j = 0; j < 28; j++){
			if (*(buf + i * 28*3 + j*3) > 200)
				printf("%c ", '@');
			else if (*(buf + i * 28*3 + j*3) > 80)
				printf("%c ", '*');
			else if (*(buf + i * 28*3 + j*3) > 0)
				printf("%c ", '+');
			else
				printf("%c ", '.'); 
		}
//		for(int j=0;j<28;j++) 
//			printf("%.0f ",(pBmpImage[i*28*3+j*3])/255.0);	
		printf("\n");
	}
}

//////////////////////////////////////////////////////////////////////////////////////////

union BYTE4{ //(Big endian���� ��ϵ�)4����Ʈ int �о� little endian 4byte int �� �ٲٴµ� ����� union 
	int v;
	unsigned char uc[4];
};

// big endian int �� ��ϵ� 4����Ʈ�� �о� little endian int �� �ؼ��Ͽ� ����
int read_int_bigendian(FILE *fp){
	unsigned char buf4[4] = { 0 };
 	BYTE4 b4;
 	fread(buf4, 1, 4, fp); 
 	b4.uc[0] = buf4[3];   //����Ʈ ���� ������
	b4.uc[1] = buf4[2];
	b4.uc[2] = buf4[1];
	b4.uc[3] = buf4[0];
	printf("\nv = %5d,    %.2x %.2x %.2x %.2x", b4.v, b4.uc[0], b4.uc[1], b4.uc[2], b4.uc[3]);
	return b4.v;
}

// 28*28 �迭�� �� �̹����� ���÷��� 
void drawimage(unsigned char *buf){
	printf("\n\n");
	for (int i = 0; i < 28; i++){
		for (int j = 0; j < 28; j++){
			if (*(buf + i * 28 + j) > 200)
				printf("%c ", '@');
			else if (*(buf + i * 28 + j) > 80)
				printf("%c ", '*');
			else if (*(buf + i * 28 + j) > 0)
				printf("%c ", '+');
			else
				printf("%c ", '.'); 
			}
		printf("\n");
	}
}

// �̹��� ���ð� ���÷��� 
void drawlawimage(unsigned char *buf){
	printf("\n\n");
	for (int i = 0; i < 28; i++){
		for (int j = 0; j < 28; j++){
			printf("%3d ", *(buf + i * 28 + j));
		}
		printf("\n");
	}
}

void printimagebuf(int n){
	for(int i=0;i<n;i++)
	{
		for (int j = 0; j < 28; j++){
			for (int k = 0; k < 28; k++){
				printf("%.0f ",imagebuf[i][j * 28 + k]);
			}
			printf("\n");
		}
		printf("\n");
	}
}

void read_image(int n,int chk) {
	unsigned char image[28*28];
	unsigned char val_label;
	
	int i = 0;
	while (i<n){   
		fread(image, 28 * 28, 1, imagecus);
		fread(&val_label, 1, 1, labelcus);
		for (int j = 0; j < 28; j++){
			for (int k = 0; k < 28; k++){
				if(chk==0) {
					imagebuf[i][j * 28 + k] = *(image + j * 28 + k)/255.0;
					labelbuf[i] = val_label;
				}
				else {
					testimagebuf[i][j * 28 + k] = *(image + j * 28 + k)/255.0;
					testlabelbuf[i] = val_label;
				}
			}
		}
		//drawimage((unsigned char *)image);
		//drawlawimage((unsigned char *)image);
		//if(chk)printf("\nno.%d ----------------- label = %d ------------------\n", i,val_label);
		i++;
	}
}


int Read_mnist(int n) 
{
	FILE *f_image = NULL, *f_label = NULL;
	long filelen;
	int magicnum, imagenum, imagewidth, imageheight;
	int magicnuml, imagenuml;
	unsigned char image[28*28];
	unsigned char val_label;
	
	//���� ����
	f_label = fopen("train-labels.idx1-ubyte", "rb"); //���� 60,000 �� �̹����� �� ����
	f_image = fopen("train-images.idx3-ubyte", "rb"); //���� 60,000 �� �̹��� ����
	if (f_label == NULL)
		return -1;
	if (f_image == NULL)
		return -1;
	
	//���� ���� Ȯ��
	fseek(f_image, 0L, SEEK_END);
	filelen = ftell(f_image);
	fseek(f_image, 0L, SEEK_SET); 
	printf("image file length = %ld\n", filelen);
	
	//����κ� �б�
	magicnum = read_int_bigendian(f_image);
	imagenum = read_int_bigendian(f_image);
	imagewidth = read_int_bigendian(f_image);
	imageheight = read_int_bigendian(f_image);
	printf("\nmagicnumber = %d,", magicnum); 
	printf("\nimagenum = %d,", imagenum); 
	printf("\nimagewidth = %d,", imagewidth); 
	printf("\nimageheight = %d,\n", imageheight);
	
	magicnuml = read_int_bigendian(f_label);
	imagenuml = read_int_bigendian(f_label);
	printf("\nmagicnumber_label = %d,", magicnuml); 
	printf("\nimagenum_label = %d,\n", imagenuml); 
	
	if(magicnum==2051)	printf("\nmagicnumber : OK");
	if(magicnuml==2049)	printf("\nmagicnumber_label : OK");
	if(imagenum==imagenuml)	printf("\nimagenum : OK");
	
	printf("\n\nnumtoread = %d",n);
	
	puts("\n\npress any key...\n");
	_getch();
	
	//���� �̹��� �����ͺκ� 28*28 �� �б�  
	imagecus = f_image;
	labelcus = f_label;
	read_image(n,0);
	
	//printimagebuf(n);

	//_getch();
	fclose(imagecus);
	fclose(labelcus);
	return 0;
}

int Read_mnist_test(int n) 
{
	FILE *f_image = NULL, *f_label = NULL;
	long filelen;
	int magicnum, imagenum, imagewidth, imageheight;
	int magicnuml, imagenuml;
	unsigned char image[28*28];
	unsigned char val_label;
	
	//���� ����
	f_label = fopen("t10k-labels.idx1-ubyte", "rb"); //���� 10,000 �� �̹����� �� ����
	f_image = fopen("t10k-images.idx3-ubyte", "rb"); //���� 10,000 �� �̹��� ����
	if (f_label == NULL)
		return -1;
	if (f_image == NULL)
		return -1;
	
	//���� ���� Ȯ��
	fseek(f_image, 0L, SEEK_END);
	filelen = ftell(f_image);
	fseek(f_image, 0L, SEEK_SET); 
	printf("test image file length = %ld\n", filelen);
	
	//����κ� �б�
	magicnum = read_int_bigendian(f_image);
	imagenum = read_int_bigendian(f_image);
	imagewidth = read_int_bigendian(f_image);
	imageheight = read_int_bigendian(f_image);
	printf("\nmagicnumber = %d,", magicnum); 
	printf("\nimagenum = %d,", imagenum); 
	printf("\nimagewidth = %d,", imagewidth); 
	printf("\nimageheight = %d,\n", imageheight);
	
	magicnuml = read_int_bigendian(f_label);
	imagenuml = read_int_bigendian(f_label);
	printf("\nmagicnumber_label = %d,", magicnuml); 
	printf("\nimagenum_label = %d,\n", imagenuml); 
	
	if(magicnum==2051)	printf("\nmagicnumber : OK");
	if(magicnuml==2049)	printf("\nmagicnumber_label : OK");
	if(imagenum==imagenuml)	printf("\nimagenum : OK");
	
	printf("\n\nnumtoread = %d",n);
	
	puts("\n\npress any key...");
	_getch();
	
	//���� �̹��� �����ͺκ� 28*28 �� �б�  
	imagecus = f_image;
	labelcus = f_label;
	read_image(n,1);
	
	//printimagebuf(n);

	//_getch();
	fclose(imagecus);
	fclose(labelcus);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////

void in(int i, int chk) {
	float inmulti=2;
	for(int j = 0;j<28 * 28;j++) 
		if(chk==0) inputl[j] = imagebuf[i][j] * inmulti;
		else inputl[j] = testimagebuf[i][j] * inmulti;
}

void init() {
	for(int j = 0;j<N_hl1_Units + N_hl2_Units + 10;j++) dasum[j] = 0;
	for(int j = 0;j<(28*28*N_hl1_Units) + (N_hl1_Units*N_hl2_Units) + (N_hl2_Units*10);j++) dwsum[j] = 0;
	for(int j = 0;j<N_hl1_Units + N_hl2_Units + 10;j++) dbsum[j] = 0;
}

void rand_drop(int chk) {
	for(int i=0;i<N_hl1_Units+N_hl2_Units;i++) {
		if(chk==0)dropout[i] = rand()%2;
		else dropout[i] = 1;
	}
}

void pre_process() {
	for(int j = 0;j<N_hl1_Units+N_hl2_Units + 10;j++) dasum[j] = 0;
	for(int j = 0;j<N_hl1_Units;j++) pre_hl1[j] = 0;
	for(int j = 0;j<N_hl2_Units;j++) pre_hl2[j] = 0;
	for(int j = 0;j<10;j++)	pre_outputl[j] = 0;
}

void process() {
	for(int j = 0;j<N_hl1_Units;j++) {
		for(int k = 0;k<28 * 28;k++) {
			pre_hl1[j] += inputl[k] * hl1_w[k][j];
		}
		hl1[j] = sig(pre_hl1[j]+hl1_b[j]) * dropout[j];
	}
	for(int j = 0;j<N_hl2_Units;j++) {
		for(int k = 0;k<N_hl1_Units;k++) {
			pre_hl2[j] += hl1[k] * hl2_w[k][j];
		}
		hl2[j] = sig(pre_hl2[j]+hl2_b[j]) * dropout[N_hl1_Units+j];
	}
	for(int j = 0;j<10;j++) {
		for(int k = 0;k<N_hl2_Units;k++) {
			pre_outputl[j] += hl2[k] * outputl_w[k][j];
		}
		outputl[j] = sig(pre_outputl[j]+outputl_b[j]);
	}
}

void findmax(int i, int chk) {
	float max = 0, maxj;
	if(chk==2)printf("\noutput : ");
	for(int j = 0;j<10;j++) {
		if(chk==2)printf("%f ",outputl[j]);
		if(max<outputl[j]) {
			max=outputl[j];
			maxj=j;	
		}
	}
	if(chk==2)printf("\noutputNum = %d",(int)maxj);
	if(chk==0) {
		//printf("\nlabelNum = %d",(int)labelbuf[i]);
		if((int)maxj!=(int)labelbuf[i]) Errcnt++;
	}
	else {
		if(chk==2)printf("\nlabelNum = %d",(int)testlabelbuf[i]);
		if((int)maxj!=(int)testlabelbuf[i]) Errcnt++;
	}
}

float calc_cost(int i, int chk) {
	float cost=0;
	
	for(int j = 0;j<10;j++)
		if(chk==0)
			if(j == labelbuf[i]) cost += pow(outputl[j]-1.0,2.0);
			else cost += pow(outputl[j],2.0);
		else
			if(j == testlabelbuf[i]) cost += pow(outputl[j]-1.0,2.0);
			else cost += pow(outputl[j],2.0);
			
	return cost;
}

void calc_d(int i) {
	
	for(int j = 0;j<10;j++) // ��Ÿ ���ϱ�  
		if(j == labelbuf[i]) dasum[j] = dsig(pre_outputl[j]) * (outputl[j]-1.0);
		else dasum[j] = dsig(pre_outputl[j]) * (outputl[j]-0.0);
			
	for(int j = 0;j<N_hl2_Units;j++)
		for(int k = 0;k<10;k++)
			dasum[j+10] += outputl_w[j][k] * dsig(pre_hl2[k]) * dasum[k];
			
	for(int j = 0;j<N_hl1_Units;j++)
		for(int k = 0;k<N_hl2_Units;k++)
			dasum[j+10+N_hl2_Units] += hl2_w[j][k] * dsig(pre_hl1[k]) * dasum[k+10];
	
	
	for(int j = 0;j<N_hl2_Units;j++) //����ġ ��Ÿ ���ϱ� 
		for(int k = 0;k<10;k++) 
			dwsum[j*10 + k] += hl2[j] * dasum[k];

	for(int j = 0;j<N_hl1_Units;j++)
		for(int k = 0;k<N_hl2_Units;k++) 
			dwsum[N_hl2_Units*10 + j*N_hl2_Units + k] += hl1[j] * dasum[k+10];

	for(int j = 0;j<28*28;j++)
		for(int k = 0;k<N_hl1_Units;k++) 
			dwsum[N_hl2_Units*10 + N_hl1_Units*N_hl2_Units + j*N_hl1_Units + k] += inputl[j] * dasum[k+10+N_hl2_Units];


	for(int j = 0;j<10;j++) //���̾ ��Ÿ ���ϱ� 
		dbsum[j] += dasum[j];

	for(int j = 0;j<N_hl2_Units;j++)
		dbsum[j+10] += dasum[j+10];
	
	for(int j = 0;j<N_hl1_Units;j++) 
		dbsum[j+10+N_hl2_Units] += dasum[j+10+N_hl2_Units];
		
}

void backpro(int n) {
	
	float rate = 1;
	float nn=1.0/(float)n;
	
	for(int i=0;i<N_hl2_Units;i++) 
		for(int j=0;j<10;j++) 
			outputl_w[i][j] -= rate * dwsum[i*10 + j]*nn * dropout[i+N_hl1_Units];

	for(int i=0;i<N_hl1_Units;i++) 
		for(int j=0;j<N_hl2_Units;j++) 
			hl2_w[i][j] -= rate * dwsum[N_hl2_Units*10 + i*N_hl2_Units + j]*nn * dropout[i] * dropout[j+N_hl1_Units];

	for(int i=0;i<28*28;i++) 
		for(int j=0;j<N_hl1_Units;j++) 
			hl1_w[i][j] -= rate * dwsum[N_hl2_Units*10 + N_hl1_Units*N_hl2_Units + i*N_hl1_Units + j]*nn * dropout[j]; 


	for(int i = 0;i<10;i++) 
		outputl_b[i] -= rate * dbsum[i]*nn;

	for(int i = 0;i<N_hl2_Units;i++) 
		hl2_b[i] -= rate * dbsum[i+10]*nn * dropout[i+N_hl1_Units];
	
	for(int i = 0;i<N_hl1_Units;i++) 
		hl1_b[i] -= rate * dbsum[i+10+N_hl2_Units]*nn * dropout[i];
	
}


int RUN(int from, int to, int chk)
{
	float cost=0;
	
	init();
	rand_drop(1);
	
	for(int i = from;i<=to;i++) {
		
		if(chk==2) {
			printf("\nno.%d",i);
		}
		in(i,chk);
		
		pre_process();
		process();
		
		if(chk){
			findmax(i,chk);
			cost = calc_cost(i,chk);
			if(chk==2)printf("\ncost = %f\n",cost);
			costsum += cost;	
		}
		
		if(chk==0) calc_d(i);
		
	}
	if(chk==0) backpro(to-from+1);
	
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void rand_set(int avr) {
	
	for(int i=0;i<28*28;i++){
		for(int j=0;j<N_hl1_Units;j++){
			hl1_w[i][j]=gaussianRandom(avr,sqrt(2.0/(28*28+N_hl1_Units)));
		}
	}
	for(int i=0;i<N_hl1_Units;i++){
		for(int j=0;j<N_hl2_Units;j++){
			hl2_w[i][j]=gaussianRandom(avr,sqrt(2.0/(N_hl1_Units+N_hl2_Units)));
		}
	}
	for(int i=0;i<N_hl2_Units;i++){
		for(int j=0;j<10;j++){
			outputl_w[i][j]=gaussianRandom(avr,sqrt(2.0/(N_hl2_Units+10)));
		}
	}
	for(int i = 0;i<10;i++) {
		outputl_b[i] =0;
	}
	for(int i = 0;i<N_hl2_Units;i++) {
		hl2_b[i] =0;
	}
	for(int i = 0;i<N_hl1_Units;i++) {	
		hl1_b[i] =0;
	}
	
} 


int main()
{
	int dataNum =10; 
	int readNum =2000, readNumtest =5000;
	int n=10; //repeat
	//10,6000,10000,5
	
	srand(clock());
	rand_set(0);
	
	Read_mnist(readNum);
	Read_mnist_test(readNumtest);
	
	printf("\n\nTesting...");
	RUN(0,readNumtest-1,1);
	printf("\navrcost = %f",costsum/(float)readNumtest);
	printf("\nErr = %f%%",Errcnt/(float)readNumtest*100.0);
	
	//_getch();
	
	printf("\n\nRunning...");
	
	clock_t t_start = clock();
	int per=0;
	costsum=Errcnt=0;
	for(int i=0;i<n;i++) {
		for(int j=0;j<readNum/dataNum;j++) {
			
			if(per+5<=(float)(i*readNum/dataNum+j)/(n*readNum/dataNum)*100) {
				per=(float)(i*readNum/dataNum+j)/(n*readNum/dataNum)*100;
				printf(" %d%%",per);
			}
			
			RUN(j*dataNum,(j+1)*dataNum-1,0);
		}
	}
	clock_t t_end = clock();
	
	printf("\n\nTesting...");
	RUN(0,readNumtest-1,1);
	printf("\navrcost = %f",costsum/(float)readNumtest);
	printf("\nErr = %f%%",Errcnt/(float)readNumtest*100.0);
	
	printf("\nTime = %.2f sec",(float)(t_end-t_start)/1000.0);
	
	printf("\n\nUSER Test");
	while(1)
	{
		_getch();
		LoadBmp24();
		for (int i = 27; i > 0; i--)
			for(int j=0;j<28;j++)
				testimagebuf[0][(27-i)*28+j] = ((float)pBmpImage[i*28*3+j*3])/255.0;	
		drawimageBmp(pBmpImage);
		scanf("%d",&testlabelbuf[0]);
		if(testlabelbuf[0]==-1)break;
		RUN(0,0,2);
	}
	
	fclose(imagecus);
	fclose(labelcus);
	delete[] imagebuf;
	delete[] testimagebuf;
	return 0;
}

