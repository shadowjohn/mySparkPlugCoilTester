/*
 * 火星塞、高壓線圈測試機
 * Author: 羽山秋人 (https://3wa.tw)
 * Release Date: 2022-01-16
 * 控制 0~15000rpm
 * PWM Output PIN D1
 * VR (接中腳) Pin A0
 * D1 接 1N4007二極體後，接至 BT151-500R (G腳)
 * D6 CLK (顯示在 TM1637用)
 * D7 DIO (顯示在 TM1637用)
 */
#include <Arduino.h>
#include <TM1637.h> //數位模組
const int PWM_PIN = D1; 
const int VR = A0;  //使用 10KΩ

#define CLK D6
#define DIO D7
TM1637 tm1637(CLK, DIO);

int val = 0;      //類比輸入
int min_val = 100;
int max_val = 1000;
int c = 0;
int HZ = 0; //產生的 HZ，0~250 // 250 * 60 = 15000 rpm
long fullDuty = 0; //一個rpm多長時間
long fireTime = 200; //200us 點火的時間最長就 200us
int RPM = 0;

//為避免眼睛跟不上七段，每經過0.2s 才更新一次七段 0.2s = 200ms = 200000us
volatile unsigned long isShowCount = 0; 
long showTM1637Times = 200000;


/*   
轉速   60 轉 =  每分鐘    60 轉，每秒  1    轉，1轉 = 1          秒 = 1000.000 ms = 1000000us
轉速   100 轉 = 每分鐘   100 轉，每秒  1.67 轉，1轉 = 0.598802   秒 =  598.802 ms =  598802us
轉速   200 轉 = 每分鐘   200 轉，每秒  3.3  轉，1轉 = 0.300003   秒 =  300.003 ms =  300003us
轉速   600 轉 = 每分鐘   600 轉，每秒  10   轉，1轉 = 0.1        秒 =  100.000 ms =  100000us
轉速  1500 轉 = 每分鐘  1500 轉，每秒  25   轉，1轉 = 0.04       秒 =   40.000 ms =   40000us
轉速  6000 轉 = 每分鐘  6000 轉，每秒  60   轉，1轉 = 0.01666... 秒 =   16.667 ms =   16667us
轉速 14000 轉 = 每分鐘 14000 轉，每秒 233.3 轉，1轉 = 0.0042863. 秒 =    4.286 ms =    4286us
轉速 14060 轉 = 每分鐘 14060 轉，每秒 240   轉，1轉 = 0.0041667. 秒 =    4.167 ms =    4167us
轉速 15000 轉 = 每分鐘 15000 轉，每秒 250   轉，1轉 = 0.004      秒 =    4.000 ms =    4000us
轉速 16000 轉 = 每分鐘 16000 轉，每秒 266.6 轉，1轉 = 0.0037500. 秒 =    3.750 ms =    3750us
*/  
void setup() {
  //開啟串列埠 功能，並設定鮑率 9600
  Serial.begin(9600);
  // put your setup code here, to run once:
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN,LOW);
  
  tm1637.init();
  tm1637.set(BRIGHT_TYPICAL); //BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;

  pinMode(VR, INPUT);
  val = analogRead(VR);  //讀取 VR 腳的類比輸入 
  
  //測試一下 七段，跑二次
  playFirstTime(); 
  playFirstTime(); 

  //顯示 0
  diaplayOnLed(0);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  val = analogRead(VR);  //讀取 VR 腳的類比輸入 
  //自動修正可變電阻範圍
  min_val = (val<min_val)?val:min_val;
  max_val = (val>max_val)?val:max_val; 
  
  HZ = map(val,min_val,max_val,1,250); // 顯示 HZ，用來對照三用電表
  fullDuty = 1000000 / HZ; // 60rpm ~ 15000rpm  
  RPM = map(val,min_val,max_val,0,15000);
  
  //正，方波
  digitalWrite(PWM_PIN,HIGH);
  delayMicroseconds( fireTime );

  //負
  digitalWrite(PWM_PIN,LOW);
  delayMicroseconds((fullDuty-fireTime));

  isShowCount+=fullDuty;
  if(isShowCount>=showTM1637Times)
  {    
    //然後顯示在 TM1637 上
    displayRPM();  
    diaplayOnLed(RPM);
    isShowCount = 0; //重來
  }
}


void playFirstTime()
{
  // 七段顯示 0000~9999 
  for (int i = 0; i <= 9; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      tm1637.display(j, i);
    }
    delay(100);
  }
}

void displayRPM() {

  Serial.print("RPM: ");
  Serial.print(RPM);  
  Serial.print("RPM Delay: ");
  Serial.print(fullDuty);  
  Serial.println();
}
void diaplayOnLed(int show_rpm)
{
  //將轉速，變成顯示值  
  //太多數位有點眼花
  //String rpm_str = String(show_rpm/10);
  show_rpm = (show_rpm>9999)?9999:show_rpm;
  
  String rpm_str = String(show_rpm);
  if (rpm_str.length() <= 3)
  {
    rpm_str = lpad(rpm_str, 4, "X"); // 變成如 "XXX0"
  }
  //Serial.print("\nAfter lpad:");
  //Serial.println(rpm_str);
  for (int i = 0; i < 4; i++)
  {
    if (rpm_str[i] == 'X')
    {
      tm1637.display(i, -1); //-1 代表 blank 一片黑
    }
    else
    {
      // Serial.println(rpm_str[i]);
      // 腦包直接轉回 String 再把單字轉 int
      // From : https://www.arduino.cc/en/Tutorial.StringToIntExample
      tm1637.display(i, String(rpm_str[i]).toInt());
    }
  }
}
String lpad(String temp , byte L , String theword) {
  //用來補LED左邊的空白
  byte mylen = temp.length();
  if (mylen > (L - 1))return temp.substring(0, L - 1);
  for (byte i = 0; i < (L - mylen); i++)
    temp = theword + temp;
  return temp;
}
