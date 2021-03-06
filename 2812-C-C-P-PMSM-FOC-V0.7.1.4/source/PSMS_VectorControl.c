//*****************************************************************************************************
//Flash和RAM软件版本切换说明(程序默认为ram版本)
//
//一.切换为Flash烧写版本方法
//1.将主程序中的:MemCopy(&RamfuncsLoadStart, &RamfuncsLoadEnd, &RamfuncsRunStart);
//               InitFlash();
//  两个函数取消注释
//2.将工程中的2812_RAM.cmd从工程中删除，添加CMD文件夹下的F2812_Flash.cmd文件，全编译一次即可烧写。
//
//二.切换为RAM在线仿真版本方法
//1.将主程序中的:MemCopy(&RamfuncsLoadStart, &RamfuncsLoadEnd, &RamfuncsRunStart);
//               InitFlash();
//  两个函数注释掉
//2.将工程中的F2812_Flash.cmd从工程中删除，添加CMD文件夹下的2812_RAM.cmd文件，全编译一次即可。
//
//*****************************************************************************************************
 
//*****************************************************************************************************
//头文件
//*****************************************************************************************************
#include "DSP281x_Device.h" 
#include "DSP281x_GlobalPrototypes.h" 
#include <math.h>

//*****************************************************************************************************
//宏定义
//***************************************************************************************************** 
#define CPU_CLOCK_SPEED      6.6667L   // for a 150MHz CPU clock speed
#define ADC_usDELAY 5000L
#define System_Operation_usDELAY 50000L
//#define DELAY_US(A)  DSP28x_usDelay(((((long double) A * 1000.0L) / (long double)CPU_CLOCK_SPEED) - 9.0L) / 5.0L) 

//*****************************************************************************************************
//子函数声明
//***************************************************************************************************** 
extern void DSP28x_usDelay(unsigned long Count);  
interrupt void MainISR(void);
interrupt void SCIBRX_ISR(void);     // SCI-B
interrupt void ISRTimer2(void);
interrupt void Cap3_ISR(void);
void Init_SiShu(void);

//*****************************************************************************************************
//全局变量定义与初始化
//***************************************************************************************************** 
float32 i=0;
float32 j=0;
float32 k=0;
Uint16 l=0;
Uint16 IsrTicker = 0;
Uint16 BackTicker = 0; //用于次数计数
Uint16 T1Period=0;     // T1定时器周期(Q0)
Uint16 T3Period = 0;   
float32 Modulation=0.25;    // 调制比
int16 MPeriod=0;
int32 Tmp=0;
Uint16 lcd_dis_flag=0;
//:::::::::::::::::::::::::::位置环变量定义:::::::::::::::::::::::::::
long PlaceError=0,Place_now=0, Now_P=0,//圈数
              OutPreSat_Place=0;//位置变量值定义
Uint16 PlaceSetBit=0;  //位置设定标志位

int32 	PosCount = 0;

float32 MfuncF1=0;
float32 MfuncF2=0;
float32 MfuncF3=0;  
//===============转子初始位置定位=============================  
Uint16 LocationFlag=1;
Uint16 LocationEnd=0; 
Uint16 Position=1;
Uint16 PositionPhase60=1;
Uint16 PositionPhase120=2;
Uint16 PositionPhase180=3; 
Uint16 PositionPhase240=4;
Uint16 PositionPhase300=5;
Uint16 PositionPhase360=6;  

//===============DAC模拟===================================== 
_iq DACTemp0=0;
_iq DACTemp1=0;
_iq DACTemp2=0; 

_iq MfuncC1=0;
_iq MfuncC2=0;
_iq MfuncC3=0; 
 
//===============转子速度计算===================================== 
Uint16 SpeedLoopPrescaler = 10;     // 速度环定标
Uint16 SpeedLoopCount = 1;          // 速度环计数  
_iq NewRawTheta=0;
_iq OldRawTheta=0; 
_iq SpeedRpm=0;                     //速度，单位：转/每分钟

_iq RawThetaTmp=0;
float32 SpeedRef=0;
_iq Speed=0;                        //速度，标幺值

//===============转子角度计算===================================
Uint16 DirectionQep=0;               //转子旋转方向
_iq RawTheta=0;
_iq OldRawThetaPos = 0;

_iq TotalPulse=0; 

_iq MechTheta = 0;                   //机械角度，单位：度
_iq ElecTheta = 0;                   //电气角度，单位：度
_iq	AnglePU=0;                       //角度标幺化
_iq	Cosine=0;
_iq	Sine=0;

//===============控制绕组电流计算============================ 
_iq ia=0;
_iq ib=0;
_iq ic=0;
_iq ialfa=0;
_iq ibeta=0; 
_iq id=0;
_iq iq=0; 

//===============PI控制器参数计算============================ 
_iq ID_Given=0;
_iq ID_Ref=0;
_iq ID_Fdb=0;
_iq ID_Error=0; 

_iq ID_Up=0;
_iq ID_Up1=0;
_iq ID_Ui=0;
_iq ID_OutPreSat=0;
_iq ID_SatError=0;
_iq ID_OutMax=_IQ(1);
_iq ID_OutMin=_IQ(-1); 
_iq ID_Out=0;

_iq IQ_Given=0;
_iq IQ_Ref=0;
_iq IQ_Fdb=0;
_iq IQ_Error=0; 


_iq IQ_Up=0;
_iq IQ_Up1=0;
_iq IQ_Ui=0;
_iq IQ_OutPreSat=0;
_iq IQ_SatError=0;
_iq IQ_OutMax=_IQ(1);
_iq IQ_OutMin=_IQ(-1); 
_iq IQ_Out=0; 

_iq Speed_Given=_IQ(0.2); //速度给定    标幺值 0.2==>600RPM，最高转速1.0==>3000RPM
_iq Speed_Ref=0;
_iq Speed_Fdb=0;
_iq Speed_Error=0; 

_iq Speed_Up=0;
_iq Speed_Up1=0;
_iq Speed_Ui=0;
_iq Speed_OutPreSat=0;
_iq Speed_SatError=0;
_iq Speed_OutMax=_IQ(0.99999);
_iq Speed_OutMin=_IQ(-0.99999);
_iq Speed_Out=0;   

//===============SVPWM计算==================================== 
Uint16 Sector = 0; 
_iq	Ualfa=0;  		
_iq	Ubeta=0;		
_iq	Ud=0;		
_iq	Uq=0;			
_iq	B0=0;			
_iq	B1=0;
_iq	B2=0;
_iq	X=0;
_iq	Y=0;
_iq	Z=0;
_iq	t1=0;
_iq	t2=0;
_iq	Ta=0;
_iq	Tb=0;
_iq	Tc=0;
_iq	MfuncD1=0;
_iq	MfuncD2=0;
_iq	MfuncD3=0; 
//===================================================================
Uint16 Run_PMSM=0;
float32 TEMP2=0;
Uint16 speed_give=0;
_iq MechScaler=_IQ(0.0);           
_iq SpeedScaler=_IQ(0.00);
Uint16 HallAngle=0;

Uint32 RS232_CNT=0;
Uint16 Hall_Fault=0;
Uint16 Angle_uint16=0;

Uint16 BuChang=0;
int16 TotalCnt=0;
_iq RawCnt1=0;
_iq RawCnt2=0;
Uint16 ShangDian_Err=0;

//========================速度环PI参数=================================
_iq Speed_Kp=_IQ(5);
_iq Speed_Ki=_IQ(0.001);
//=====================================================================

//========================Q轴电流环PI参数==============================
_iq IQ_Kp=_IQ(0.3);
_iq IQ_Ki=_IQ(0.001);
//=====================================================================

//========================D轴电流环PI参数==============================
_iq ID_Kp=_IQ(0.3);
_iq ID_Ki=_IQ(0.001);
//=====================================================================

long PlaceSet=1000000;//位置环指令脉冲数
Uint16 PlaceEnable=0;//位置环使能  1 使能 ;  0 禁止

//=====================参数设置========================================
float32 E_Ding_DianLiu=1.5;         //设置电机的额定电流 单位A 
_iq BaseRpm=_IQ(3000);              //设置电机额定转速
Uint16 BaseSpeed=3000;              //设置电机额定转速与BaseRpm相等
_iq PolePairs=_IQ(4);               //设置电机极对数
_iq LineEncoder=_IQ(2500);          //设置电机的增量式光电编码器线数
_iq Offset_Angle=_IQ(0.48);         //设置电机编码器Z相与电机A相的偏移角度，标么值
Uint16 ZhengFan=1;                  //设置PMSM电机正反转  1 正转 ;  0 反转
Uint16 KEY_Type=1;                  //设置键盘显示板类型  1 LCD  ;  0 数码管  


//*****************************************************************************************************
//主程序
//*****************************************************************************************************
void main(void)
{
//======================================================================================================
//系统初始化
//======================================================================================================
	InitSysCtrl();
    InitGpio();
	DINT;
	
//======================================================================================================
//PIE中断控制器初始化
//====================================================================================================== 
	InitPieCtrl();
    IER = 0x0000;
	IFR = 0x0000;

//======================================================================================================
//PIE中断控制器初始化
//====================================================================================================== 
	InitPieVectTable();

	//====仿真注释后两句
    //====flash烧写则取消注释后两句===
    MemCopy(&RamfuncsLoadStart, &RamfuncsLoadEnd, &RamfuncsRunStart);
    InitFlash();
    
    InitSci();
    //InitCpuTimers();
	//ConfigCpuTimer(&CpuTimer2, 150, 20000);	
 	//StartCpuTimer2();
//======================================================================================================
//EVA初始化
//====================================================================================================== 
    EALLOW;

	//EVA通用定时器全局控制寄存器初始化
    EvaRegs.GPTCONA.all = 0x0;		 /* gp timer control register */

	//定时器1的周期寄存器
    EvaRegs.T1PR =3750;  //开关频率10KHz，((1/10KHz)/13.333334ns)/2=7500=3750
	T1Period=EvaRegs.T1PR;    
    EvaRegs.T1CNT = 0x0000;   	//定时器1的计数寄存器 
    EvaRegs.T1CON.all = 0x0840; //定时器1的控制寄存器
    //0x0840 = 0000 1000 0100 0000 
    //15,14  00  仿真挂起立即停止 
	//13     0   保留
    //12,11  01  连续增/减计数模式
    //10~8  000  输入时钟预定标因子x/1
    //7      0   使用自己使能位(TENABEL)
    //6      1   定时器使能控制位
    //5,4   00   使用内部高速时钟HSPLCK
	//3,2   00   定时器比较寄存器的装载条件--计数器值等于0
    //1     0    禁止定时器比较操作 
    //0     0    定时器2的周期寄存器选择位，使用自己的周期寄存器
    
    //捕获单元寄存器_捕获控制寄存器
    EvaRegs.CAPCONA.all=0x8004;
	//0x8004 = 1000 0000 0000 0100 
	//捕获单元3上升沿检查

	TotalPulse=_IQmpy(_IQ(4),LineEncoder);
   
    
    EvaRegs.T2PR = (Uint16)(TotalPulse>>15);//定时器2的周期寄存器
	EvaRegs.T2CNT = 0; 	 					//定时器2的计数寄存器
	EvaRegs.T2CON.all = 0x1874;				//定时器2的控制寄存器
	//0x1874=0001 1000 0111 0100 
	//15,14   00   仿真挂起立即停止 
 	//12,11   11   定向增/减计数
	//10~8   000   输入时钟预定标因子x/1
	//7       0    使用自己使能位(TENABEL)   
	//6       1    使能定时器操作
 	//5,4    11    时钟由QEP电路产生
 	//3,2    01    定时器比较寄存器的装载条件--计数器值等于0或等于周期寄存器的值
    //1      0     禁止定时器比较操作 
    //0      0     定时器2的周期寄存器选择位，使用自己的周期寄存器

    EvaRegs.CMPR1 = 1000; 
    EvaRegs.CMPR2 = 1000; 
    EvaRegs.CMPR3 = 1000;            //全比较器赋值  
	EvaRegs.GPTCONA.bit.T1TOADC = 1; // 由EVA的下溢中断标志触发,启动ADC
    
    //EvaRegs.DBTCONA.all = 0x0ff0;  /* 死区时间3.2us */ 
    //EvaRegs.DBTCONA.all = 0x0cf4;  /* 死区时间5.1us */ 
    EvaRegs.DBTCONA.all = 0x09f4;  /* 死区时间3.8us */ 
    //0x09f4 = 0000 1001 1111 0100
	//11~8 1001  死区定时器周期 9
	//4~2  101   死区定时器预定标控制位（x为高速外设时钟频率,本项目为75MHz） x/32


    EvaRegs.EVAIMRA.all = 0x0200;   /* 开定时器下溢中断 */
    EvaRegs.COMCONA.all = 0x8200; 
    //比较控制寄存器COMCONA 
    //0 A600=1010 0110 0000 0000 
    //15      1   使能比较单元操作
    //14,13  01   当T1CNT=0或T1PR（CMPRX重载条件为下溢或周期匹配）
    //12      0   禁止空间矢量PWM模式
    //11,10  01   当T1CNT=0或T1RP（比较方式控制寄存器的重载条件为下溢或周期匹配）
    //9       1   使能全比较输出

    EvaRegs.CAPCONA.all = 0x9004;  //检测上升沿
    EvaRegs.EVAIFRC.bit.CAP3INT=1; //清除cap3中断标志
    EvaRegs.EVAIMRC.bit.CAP3INT=1; //使能cap3中断
//======================================================================================================
//EVB初始化,用于模拟DAC输出
//====================================================================================================== 
    EvbRegs.T3PR = 3750;  ////开关频率10KHz 
	T3Period=EvbRegs.T3PR;         
    EvbRegs.T3CNT = 0x0000;
    EvbRegs.T3CON.all = 0x0840;   
    //0x0840=0000 1000 0100 0000 
    //连续增/减计数模式
    //输入时钟预定标x/1
    //禁止定时器比较操作 
    //使用内部高速时钟HSPLCK
    //使用自己使能位
    //仿真挂起立即停止 


    EvbRegs.CMPR4 = 1000; 
    EvbRegs.CMPR5 = 1000; 
    EvbRegs.CMPR6 = 1000;         //全比较器赋值 

    EvbRegs.ACTRB.all = 0x0999;   //PWM1,3,5低有效，PWM2,4,6高有效
    EvbRegs.DBTCONB.all = 0x0AF8; 
    //0x0AF8=0000 1010 1111 1000
    //死区控制 x/32=150MHz/32=4.6875MHz  死区周期为10个周期
    //死区时间=32*6.67*10=2134.4ns=2us
    EvbRegs.COMCONB.all =0xA600;  
    //0xA600=1010 0110 0000 0000 
    //使能比较器操作
    //比较寄存器CMPRX重载条件为下溢或周期匹配
    //禁止硬件空间向量PWM模式
    //全比较器输出使能
    //方式控制寄存器ACTRA重载条件为下溢或周期匹配  

//======================================================================================================
//开中断
//====================================================================================================== 
	EvaRegs.EVAIMRA.bit.T1UFINT = 1;
    EvaRegs.EVAIFRA.bit.T1UFINT = 1;//T1下溢中断
    EDIS;

 	EALLOW;
    PieVectTable.T1UFINT = &MainISR;
    PieVectTable.RXBINT = &SCIBRX_ISR;   //设置串口B接受中断的中断向量
    PieVectTable.CAPINT3=&Cap3_ISR;
    //PieVectTable.TINT2 =  &ISRTimer2;
    EDIS;

    PieCtrlRegs.PIEIER2.bit.INTx6=1;//t1upint PIE中断标志寄存器PIEIFRx在有中断激活时，相应位置1，中断被响应后会自动清零，不需要用户操作。
    PieCtrlRegs.PIEIER9.bit.INTx3=1;//SCIRXB
    PieCtrlRegs.PIEIER3.bit.INTx7=1;//CAP3
   	IER |= M_INT2;	// //CPU中断使能寄存器，写入1到相应位，使能中断INT2。 
   	IER |= M_INT9;    //SCI //允许串口中断
   	IER |= M_INT3;
    //IER |= M_INT14;   //cputimer2
    Init_SiShu();
    eva_close();
    
    Ad_CaiJi(); 
    Ad_CaiJi();  

    if(AD_BUF[4]<100)
    {
 	Pwm_EN_0;//允许PWM使能
    }

    else
    {
    Pwm_EN_1;//禁止PWM使能
	ShangDian_Err=1;
    }
    
    Init_ch454();
    Init_lcd();
    DELAY_US(1000000);

    EINT;  
    ERTM;
     
//======================================================================================================
//主循环
//====================================================================================================== 
	for(;;)
	{ 
   		
        CPU_RUN();
        DC_Link();
        ShowDisp();
        Dis_Show();
        deal_key();
        LCD_DIS();

	}

}

//*****************************************************************************************************
//中断服务程序
//***************************************************************************************************** 
interrupt void MainISR(void)
{
    _iq t_01,t_02;
    
     IPM_BaoHu();

    RS232_CNT=RS232_CNT+5;
    Show_time++;
    Show_time2++;
    if(Show_time2==1000)//1秒
    {
        Show_time2=0;
        lcd_dis_flag=1;
    }
    Read_key(); 
    Ad_CaiJi();
    JiSuan_Dl(); 
    
	if(Run_PMSM==1&&IPM_Fault==0)
	{

   		if(LocationFlag!=LocationEnd)
		{ 
    		Modulation=0.95;
        	if(GpioDataRegs.GPBDAT.bit.GPIOB15)//W
        	{
            	HallAngle+=1;
        	}
        	if(GpioDataRegs.GPBDAT.bit.GPIOB14)//V
        	{ 
           		HallAngle+=2;
        	}
        	if(GpioDataRegs.GPBDAT.bit.GPIOB13)//U
        	{
          		HallAngle+=4;
        	}

			switch(HallAngle)
			{
			case 5:
				Position=PositionPhase60;
                LocationFlag=LocationEnd;//定位结束
				EvaRegs.T2CNT =(EvaRegs.T2PR-BuChang*0-BuChang/2);   
				break;
			case 1:
				Position=PositionPhase360;
                LocationFlag=LocationEnd;//定位结束
				EvaRegs.T2CNT =(EvaRegs.T2PR-BuChang*5-BuChang/2);   
				break;
			case 3:
				Position=PositionPhase300;
                LocationFlag=LocationEnd;//定位结束
				EvaRegs.T2CNT =(EvaRegs.T2PR-BuChang*4-BuChang/2);   
				break;
			case 2:
				Position=PositionPhase240;
                LocationFlag=LocationEnd;//定位结束
				EvaRegs.T2CNT =(EvaRegs.T2PR-BuChang*3-BuChang/2);   
				break;
			case 6:
				Position=PositionPhase180;
                LocationFlag=LocationEnd;//定位结束
				EvaRegs.T2CNT =(EvaRegs.T2PR-BuChang*2-BuChang/2);   
				break;
			case 4:
				Position=PositionPhase120;
                LocationFlag=LocationEnd;//定位结束
				EvaRegs.T2CNT =(EvaRegs.T2PR-BuChang*1-BuChang/2);     
				break;
            default:
                Run_PMSM=0;//霍尔信号错误启动停止
                eva_close();
                Hall_Fault=1;
                break;
			}

		}   
//======================================================================================================
//初始位置ㄎ唤崾急栈房刂�
//====================================================================================================== 
	else if(LocationFlag==LocationEnd)
	{  
		l++;   
        //旋转方向判定 
		DirectionQep = 0x4000&EvaRegs.GPTCONA.all;
		DirectionQep = DirectionQep>>14;
        RawTheta = _IQ(EvaRegs.T2CNT);
		RawTheta =TotalPulse - RawTheta;
        
		// 计算机械角度
		if(DirectionQep ==1) //T2STAT=1,递增计数，代表顺时针；
		{			
            if((RawTheta> RawCnt1) && (OldRawThetaPos<_IQ(1000)))
			{
				PosCount += TotalCnt;
			}
			Place_now= _IQtoF(TotalPulse - RawTheta)+PosCount;
			OldRawThetaPos = RawTheta; 
		}
		else if(DirectionQep ==0)//T2STAT=0，递减计数，代表逆时针
		{  
            if((OldRawThetaPos> RawCnt2) && (RawTheta<_IQ(900)))
			{
				PosCount -= TotalCnt;
			}
			Place_now = _IQtoF(TotalPulse - RawTheta)+PosCount;
			OldRawThetaPos = RawTheta;
		} 

		MechTheta = _IQmpy(MechScaler,RawTheta);
        if(MechTheta>_IQ(360))
        {MechTheta=MechTheta-_IQ(360);}
        if(MechTheta<_IQ(-360))
        {MechTheta=MechTheta+_IQ(360);}
		ElecTheta = _IQmpy(PolePairs,MechTheta);    
	
		AnglePU=_IQdiv(ElecTheta,_IQ(360))+Offset_Angle;
	   	Sine = _IQsinPU(AnglePU);
	   	Cosine = _IQcosPU(AnglePU);   
//======================================================================================================
//计算绕组电流ia,ib,ic===>ialfa,ibeta===>id,iq，矢量变换
//======================================================================================================
		
		ialfa=ia;
		ibeta=_IQmpy(ia,_IQ(0.57735026918963))+_IQmpy(ib,_IQ(1.15470053837926));  

		id = -_IQmpy(ialfa,Cosine) -_IQmpy(ibeta,Sine);
		iq = -_IQmpy(ibeta,Cosine)+_IQmpy(ialfa,Sine) ; 
 
	    if (SpeedLoopCount>=SpeedLoopPrescaler)
	    {   
			// 旋转方向判定 
			DirectionQep = 0x4000&EvaRegs.GPTCONA.all;
			DirectionQep = DirectionQep>>14;

 			NewRawTheta =_IQ(EvaRegs.T2CNT);
			// 计算机械角度
			if(DirectionQep ==1) //T2STAT=1,递增计数；
			{
               	RawThetaTmp =  OldRawTheta-NewRawTheta ; 
				if(RawThetaTmp > _IQ(0))
				{
				 	RawThetaTmp = RawThetaTmp - TotalPulse;  
				} 
			}
			else if(DirectionQep ==0) //T2STAT=0，递减计数
			{                
			 	RawThetaTmp =OldRawTheta- NewRawTheta; 
				if(RawThetaTmp < _IQ(0))
				{
					 RawThetaTmp = RawThetaTmp + TotalPulse;
				}
			 }
	 
			Speed = _IQmpy(RawThetaTmp,SpeedScaler);
			SpeedRpm = _IQmpy(BaseRpm,Speed);   				
			OldRawTheta = NewRawTheta;
            if(Speed<0)
			{
				speed_dis=_IQtoF(_IQmpy(Speed, _IQ(-100)));
			}
			else
			{
            	speed_dis=_IQtoF(_IQmpy(Speed, _IQ(100)));
            }
		    SpeedLoopCount=1; 
			RawThetaTmp=0; 


//=================位置环控制===================================
	if(PlaceEnable ==1)
   	    {
        PlaceError = PlaceSet + Place_now;
  
		OutPreSat_Place = PlaceError;
		if((PlaceError<=10000)&&(PlaceError>=-10000))
        { 
           OutPreSat_Place = PlaceError/3; 
		}  
		
        if (OutPreSat_Place> 2000)
        {
          SpeedRef =  0.5;
        }
        else if (OutPreSat_Place< -2000)
        {
          SpeedRef =  -0.5;
        }
        else
        {
          SpeedRef = OutPreSat_Place/(float32)BaseSpeed;
        }
	   
   	}
//=================速度环PI===================================

            Speed_Ref=_IQ(SpeedRef);
			Speed_Fdb=Speed;

			Speed_Error=Speed_Ref - Speed_Fdb;//误差

			Speed_Up=_IQmpy(Speed_Kp,Speed_Error);//kp
			

			Speed_OutPreSat=Speed_Up+Speed_Ui;

			if(Speed_OutPreSat>Speed_OutMax)
				{
                    Speed_Out=Speed_OutMax;

            	}
			else if(Speed_OutPreSat<Speed_OutMin)
	 			{
                    Speed_Out=Speed_OutMin;
				}
			else 
				{
                    Speed_Out=Speed_OutPreSat;  
				}
	
			Speed_SatError=Speed_Out-Speed_OutPreSat;  
            Speed_Ui=Speed_Ui + _IQmpy(Speed_Ki,Speed_Up) + _IQmpy(Speed_Ki,Speed_SatError);

			IQ_Given=Speed_Out; 
		} 

	else 
    {
        SpeedLoopCount++; 
	}

//======================================================================================================
//IQ电流PID调节控制
//======================================================================================================  
		IQ_Ref=IQ_Given;
		IQ_Fdb=iq;

		IQ_Error=IQ_Ref-IQ_Fdb;//误差

		IQ_Up=_IQmpy(IQ_Kp,IQ_Error);//kp
		

		IQ_OutPreSat=IQ_Up+IQ_Ui;

		if(IQ_OutPreSat>IQ_OutMax)
		{
            IQ_Out=IQ_OutMax;
		}
		else if(IQ_OutPreSat<IQ_OutMin)
		{
            IQ_Out=IQ_OutMin;
		}
		else 
		{
            IQ_Out=IQ_OutPreSat;  
        }

		IQ_SatError=IQ_Out-IQ_OutPreSat;  
        IQ_Ui=IQ_Ui + _IQmpy(IQ_Ki,IQ_Up) + _IQmpy(IQ_Ki,IQ_SatError);

		Uq=IQ_Out;

//======================================================================================================
//ID电流PID调节控制
//======================================================================================================  
		ID_Ref=0;
		ID_Fdb=id;

		ID_Error=ID_Ref-ID_Fdb;//误差

		ID_Up=_IQmpy(ID_Kp,ID_Error);    
		  

		ID_OutPreSat=ID_Up+ID_Ui;   

		if(ID_OutPreSat>ID_OutMax)   
		{
        	ID_Out=ID_OutMax;
		}
		else if(ID_OutPreSat<ID_OutMin)
		{
        	ID_Out=ID_OutMin;
        }
		else 
		{
        	ID_Out=ID_OutPreSat; 
		}

		ID_SatError=ID_Out-ID_OutPreSat;  
        ID_Ui=ID_Ui+_IQmpy(ID_Ki,ID_Up)+_IQmpy(ID_Ki,ID_SatError);  

		Ud=ID_Out;

//======================================================================================================
//IPark变换UD,UQ-->UALFA,UBETA获得给定信号
//====================================================================================================== 
		Ualfa = _IQmpy(Ud,Cosine) - _IQmpy(Uq,Sine);
		Ubeta = _IQmpy(Uq,Cosine) + _IQmpy(Ud,Sine); 
    
		B0=Ubeta;
		B1=_IQmpy(_IQ(0.8660254),Ualfa)- _IQmpy(_IQ(0.5),Ubeta);// 0.8660254 = sqrt(3)/2 
		B2=_IQmpy(_IQ(-0.8660254),Ualfa)- _IQmpy(_IQ(0.5),Ubeta); // 0.8660254 = sqrt(3)/2

		Sector=0;
		if(B0>_IQ(0)) Sector =1;
		if(B1>_IQ(0)) Sector =Sector +2;
		if(B2>_IQ(0)) Sector =Sector +4; 

		X=Ubeta;//va
		Y=_IQmpy(_IQ(0.8660254),Ualfa)+ _IQmpy(_IQ(0.5),Ubeta);// 0.8660254 = sqrt(3)/2 vb
		Z=_IQmpy(_IQ(-0.8660254),Ualfa)+ _IQmpy(_IQ(0.5),Ubeta); // 0.8660254 = sqrt(3)/2 vc 

	    if(Sector==1)
	 	{
			t_01=Z;
			t_02=Y;
     	    if((t_01+t_02)>_IQ(1))
      	    {
     		   t1=_IQmpy(_IQdiv(t_01, (t_01+t_02)),_IQ(1));
     		   t2=_IQmpy(_IQdiv(t_02, (t_01+t_02)),_IQ(1));
      	    }
     	    else
       	    { 
               t1=t_01;
       		   t2=t_02;
     	    }
			Tb=_IQmpy(_IQ(0.5),(_IQ(1)-t1-t2));
			Ta=Tb+t1;
			Tc=Ta+t2;
		}
		else if(Sector==2)
		{
			t_01=Y;
			t_02=-X;
            if((t_01+t_02)>_IQ(1))
   		    {
     		   t1=_IQmpy(_IQdiv(t_01, (t_01+t_02)),_IQ(1));
     		   t2=_IQmpy(_IQdiv(t_02, (t_01+t_02)),_IQ(1));
      	    }
       		else
      	    { 
      	       t1=t_01;
     		   t2=t_02;
     	    }
			Ta=_IQmpy(_IQ(0.5),(_IQ(1)-t1-t2));
			Tc=Ta+t1;
			Tb=Tc+t2;
 		} 
	    else if(Sector==3)
 	    {
			t_01=-Z;
			t_02=X;
            if((t_01+t_02)>_IQ(1))
     	    {
				t1=_IQmpy(_IQdiv(t_01, (t_01+t_02)),_IQ(1));
				t2=_IQmpy(_IQdiv(t_02, (t_01+t_02)),_IQ(1));
      	    }
            else
			{ 
				t1=t_01;
       			t2=t_02;
      	    }
			Ta=_IQmpy(_IQ(0.5),(_IQ(1)-t1-t2));
			Tb=Ta+t1;
			Tc=Tb+t2;	
	    } 
	    else if(Sector==4)
	    {
			t_01=-X;
			t_02=Z;
            if((t_01+t_02)>_IQ(1))
      		 {
        	 	t1=_IQmpy(_IQdiv(t_01, (t_01+t_02)),_IQ(1));
     		    t2=_IQmpy(_IQdiv(t_02, (t_01+t_02)),_IQ(1));
      		 }
            else
            {
            	t1=t_01;
            	t2=t_02;
            }
			Tc=_IQmpy(_IQ(0.5),(_IQ(1)-t1-t2));
			Tb=Tc+t1;
			Ta=Tb+t2;
	    } 
	    else if(Sector==5)
	    {
			t_01=X;
			t_02=-Y;
            if((t_01+t_02)>_IQ(1))
      	    {
      	    	t1=_IQmpy(_IQdiv(t_01, (t_01+t_02)),_IQ(1));
      	   	    t2=_IQmpy(_IQdiv(t_02, (t_01+t_02)),_IQ(1));
	        }
      	    else
      	    {
      	   	    t1=t_01;
      	   	    t2=t_02;
            }
			Tb=_IQmpy(_IQ(0.5),(_IQ(1)-t1-t2));
			Tc=Tb+t1;
	 		Ta=Tc+t2;
		}
		else if(Sector==6)
		{
			t_01=-Y;
			t_02=-Z;
            if((t_01+t_02)>_IQ(1))
     	    {
     	    	t1=_IQmpy(_IQdiv(t_01, (t_01+t_02)),_IQ(1));
       			t2=_IQmpy(_IQdiv(t_02, (t_01+t_02)),_IQ(1));
      	    }
       		else
      		{ 
       			t1=t_01;
       			t2=t_02;
       		}
			Tc=_IQmpy(_IQ(0.5),(_IQ(1)-t1-t2));
			Ta=Tc+t1;
			Tb=Ta+t2;
		} 
		MfuncD1=_IQmpy(_IQ(2),(_IQ(0.5)-Ta));
		MfuncD2=_IQmpy(_IQ(2),(_IQ(0.5)-Tb));
		MfuncD3=_IQmpy(_IQ(2),(_IQ(0.5)-Tc));  
		}
//======================================================================================================
//EVA全比较器参数赋值，用于驱动电机
//====================================================================================================== 
	MPeriod = (int16)(T1Period * Modulation);              // Q0 = (Q0 * Q0)

	Tmp = (int32)MPeriod * (int32)MfuncD1;                    // Q15 = Q0*Q15，计算全比较器CMPR1赋值
	EvaRegs.CMPR1 = (int16)(Tmp>>16) + (int16)(T1Period>>1); // Q0 = (Q15->Q0)/2 + (Q0/2)

	Tmp = (int32)MPeriod * (int32)MfuncD2;                    // Q15 = Q0*Q15，计算全比较器CMPR2赋值
	EvaRegs.CMPR2 = (int16)(Tmp>>16) + (int16)(T1Period>>1); // Q0 = (Q15->Q0)/2 + (Q0/2)

	Tmp = (int32)MPeriod * (int32)MfuncD3;                    // Q15 = Q0*Q15，计算全比较器CMPR3赋值
	EvaRegs.CMPR3 = (int16)(Tmp>>16) + (int16)(T1Period>>1); // Q0 = (Q15->Q0)/2 + (Q0/2) 
}

	if(DC_ON_flag==1)
	{
   		 DC_ON_CNT++;
   		 if(DC_ON_CNT==5000)
    	 {
      		 DC_ON_OPEN=1;
		     DC_ON_CNT=0;
   		 }
   	 }

//======================================================================================================
//中断返回
//====================================================================================================== 
PieCtrlRegs.PIEACK.all|=0x0002;// Issue PIE ack
EvaRegs.EVAIFRA.bit.T1UFINT=1;

}

interrupt void SCIBRX_ISR(void)     // SCI-B
{
	static unsigned int RxBuf;
	
    RxBuf=ScibRegs.SCIRXBUF.all;
	switch(RxBuf)
	{
	case 0:  //正转
	    ZhengFan=1;
        SpeedRef=speed_give*1.0/100;
		eva_close();
		break;
	case 1:	 //运行
	    Run_PMSM=1;
		eva_open();
	    break;
	case 2:  //增加速度 百分比 10个10个的加
	    if(ZhengFan==1)
	    {
       		 if(speed_give<91)
             {
           		speed_give=10+speed_give;
                SpeedRef=speed_give*1.0/100;
       	     }        
        }
        else
        {
       		 if(speed_give<91)
       		 {
           		 speed_give=10+speed_give;
        		 SpeedRef=speed_give*1.0/-100;
             }
         }		
		break;
	case 3://增加频率 一个一个的加
	    if(ZhengFan==1)
	    {
      		 if(speed_give<100)
       		 {
           		 speed_give=1+speed_give;
        		 SpeedRef=speed_give*1.0/100;
             }
         }
         else
         {
        	 if(speed_give<100)
       		 {
             	 speed_give=1+speed_give;
        		 SpeedRef=speed_give*1.0/-100;
             }
       	 }
		break;
	case 4:	//反转
		ZhengFan=0;
        SpeedRef=speed_give*1.0/-100;
        eva_close();
		break;
	case 5: //停止
		eva_close();
        Run_PMSM=0;
		break;
	case 6: //减小速度百分比 10个10个的减
        if(ZhengFan==1)
	    {
       		 if(speed_give>9)
        	 {
            	 speed_give=speed_give-10;
         		 SpeedRef=speed_give*1.0/100;
             }
        }
        else
        {
       		 if(speed_give>9)
        	 {
           		 speed_give=speed_give-10;
        	 	 SpeedRef=speed_give*1.0/-100;
              }
        }		
		break;
	case 7://减小速度百分比 一个一个的减
		if(ZhengFan==1)
	    {
        	 if(speed_give>0)
        	 {
             		speed_give=speed_give-1;
           	 		SpeedRef=speed_give*1.0/100;
             }
        }
        else
        {
       		 if(speed_give>0)
        	 {
            		speed_give=speed_give-1;
       				SpeedRef=speed_give*1.0/-100;
             }
        }
		break;
	}
    PieCtrlRegs.PIEACK.bit.ACK9 = 1;
}


interrupt void Cap3_ISR(void)
{
	EvaRegs.T2CNT = 0;  
 
	vaRegs.EVAIFRC.bit.CAP3INT=1;

}

void Init_SiShu(void)
{
	float32 temp;
    int16 tempcnt;

	temp=(90.0*32768)/LineEncoder;
 	MechScaler=_IQ(temp);

 	temp=(15000*32768)/LineEncoder;
 	temp=temp/BaseSpeed;
 	SpeedScaler=_IQ(temp);

 
 	temp=(((LineEncoder/32768)*4)/360.0)*60;
	temp=(temp*32768)/PolePairs;
	BuChang=temp;

	temp=(Offset_Angle*360)/32768.0;
 	temp=(((LineEncoder/32768)*4)/360.0)*temp;
 	temp=(temp*32768)/PolePairs;

	Angle_uint16=temp;

	TotalCnt=_IQtoF(_IQmpy(LineEncoder, PolePairs));
 
 	tempcnt=TotalCnt-1000;
 	RawCnt1=_IQ(tempcnt);

 	tempcnt=TotalCnt-100;
 	RawCnt2=_IQ(tempcnt);
 	E_Ding_DianLiu=1.414*E_Ding_DianLiu;
 	GuoliuZhi=15*E_Ding_DianLiu;

}


/*interrupt void ISRTimer2(void)
{
 
	
	
}*/
/*====================================================================================================
The end!
=====================================================================================================*/










