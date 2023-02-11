#include <SoftwareSerial.h>
SoftwareSerial sim(7,8);
#include <EEPROM.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h> // thư viện giao tiếp SPI
LiquidCrystal_I2C lcd(0x27,16,2);
#include <MFRC522.h> // thư viện module RFID
char password[4];//Định dạng mật khẩu có 4 ký tự bao gồm số và chữ
char initial_password[4],new_password[4], password1[4],password2[4];
int i=0;
int lock = 6;//Relay chan so 8
int sai=0;
const String myphone = "0965843904";
String val;
uint8_t successRead;    // biến đọc thẻ thành công
byte storedCard[4];   // lưu trữ UID trong EEPROM
byte readCard[4];   // Biến lưu giá trị UID đọc được
boolean programMode = false;
boolean match = false;          // Biến thẻ hợp lệ
#define Button 5 
#define SS_PIN 10 // khai báo chân SS của RFID
#define RST_PIN 9 // khai báo chân Reset của RFID
MFRC522 mfrc522(SS_PIN, RST_PIN); // khai báo tên module RFID
char key_pressed=0;
char pressed=0;
const byte rows = 4;
const byte columns = 4;
char hexaKeys[rows][columns] = 
{
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte row_pins[rows] = {4,3, 2,0};
byte column_pins[columns] = {A0, A1, A2, A3};
Keypad keypad_key = Keypad(makeKeymap(hexaKeys), row_pins, column_pins, rows, columns);
int wrong_num=0;
void setup() {
  SPI.begin(); // bắt đầu giao tiếp SPI
  mfrc522.PCD_Init(); // xóa dữ liệu MFRC522
  lcd.backlight(); // bật đèn nền LCD
  lcd.init();
  lcd.setCursor(0,0);
  lcd.clear();
  lcd.print("Nhap mat khau");
  pinMode(lock,OUTPUT);
  pinMode(Button,INPUT_PULLUP);
  sim.begin(9600);
  sim.println("AT");// because of SIM800L autobounding mode
  sim.println("AT+CIMI");
  sim.println("AT+CMGF=1\r");
  sim.println("AT+CNMI=1,2,0,0,0");
  for(int j=0;j<4;j++)
//  EEPROM.write(j, j+49);//Ghi chuỗi kí tự 1234 vô EEPROM của arduino làm mật khẩu ban đầu
   for(int j=0;j<4;j++) { //Đọc mật khẩu hiện trong EEPROM của arduino
   initial_password[j]=EEPROM.read(j);
   }  
  Serial.begin(9600);
  Serial.println("DO AN TOT NGHIEP");
  Serial.println("CUA THONG MINH");
}

void loop() {
 successRead = getID();
 if(successRead)
  { 
    rfid();
   }
else {
  banphim();
  sim800l();
  button();
}
}
// Nhấn nút mở cửa khi ở bên trong
void button() {
   if(digitalRead(Button)==LOW){ 
      lcd.clear();
      lcd.print("Cua Dang Mo");
      digitalWrite(lock, HIGH);
      delay(5000);//Relay sẽ bật mở cửa trong 10s
      digitalWrite(lock, LOW);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Nhap mat khau");
     }
}

void banphim(){
  lcd.setCursor(0, 0); // dòng 1
  lcd.print("Nhap mat khau"); // in ra dòng 1 LCD
  digitalWrite(lock, LOW);
  key_pressed = keypad_key.getKey();
if(key_pressed=='#'){// Nút nhấn trên keypad
    change_password();
  }
if(key_pressed == '*') //Chương trình thêm xóa thẻ
  { 
  int j=0;
  lcd.clear();
  lcd.print("Nhap mat khau");
  lcd.setCursor(0,1);
  lcd.print("Them Xoa The");
  delay(2000);
  lcd.clear();
  lcd.print("Nhap mat khau");
  lcd.setCursor(0,1);
  while(j<4){
    char key=keypad_key.getKey();
    if(key){
      new_password[j++]=key;
      lcd.print(key);
      delay(500);
      lcd.setCursor(j-1,1);
      lcd.print("*");
    }
    key=0;
  }
  delay(500);

if(!(strncmp(new_password, initial_password, 4))){//so sánh chuỗi kí tự giữa mật khẩu mới và mật khẩu ban đầu Có khác nhau ko nếu khác thực hiện hàm dưới 
   
    lcd.clear();
    lcd.print("Them, Xoa The");
    lcd.setCursor(0,1);
    sai =0;
    do {
    successRead = getID();
    }
    while(!successRead);{
     //When in program mode check First If master card scanned again to exit program mode
      if ( findID(readCard) ) { // If scanned card is known delete it
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Xoa Thanh Cong");
        deleteID(readCard);
        delay(3000);
        lcd.clear();
        Serial.println("XOA THE THANH CONG");
      }
      
      else {                    // If scanned card is not known add it
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Them Thanh Cong");
        writeID(readCard);
        delay(3000);
        lcd.clear();
        Serial.println("THEM THE THANH CONG");
      }
    }
  }
  else {
     lcd.clear();
     lcd.print("MAT KHAU SAI");
     delay(2000);
     Serial.println("MAT KHAU SAI THEM XOA THE THAT BAI");
     sai++;
  }
  }
    
  // Nhập mật khẩu, nếu mật khẩu đúng mở cửa
  if (key_pressed !='#' && key_pressed != '*'&&key_pressed != NO_KEY) //Hàm chuyển mật khẩu nhập vào hiển thị trên LCD dưới dạng ****
  {
    password[i++]=key_pressed;//Lệnh nhập pass sẽ hiển thị lần lượt từng kí tự mà mình nhập
    lcd.setCursor(i-1,1);
    lcd.print(key_pressed);
    delay(500);
    lcd.setCursor(i-1,1);
    lcd.print("*");
  if(i==4){//Nếu nhập mật khẩu đúng 4 chữ số
    delay(200);//đợi 0.2s
    if(!(strncmp(password, initial_password,4))){//so sánh mật khẩu mình nhập dòng có đúng với mật khẩu ban đầu mặc định dòng code 37 ko
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Cua Dang Mo");
      digitalWrite(lock, HIGH);
      delay(5000);//Relay sẽ bật mở cửa trong 10s
      digitalWrite(lock, LOW);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Nhap mat khau");
      i=0;//Trở về vị trí ban đầu để nhập mật khẩu mới
      sai=0;
      Serial.println("MAT KHAU DUNG CUA DANG MO");
    }
    else{//Ngược lại
      lcd.clear();
      lcd.print("MAT KHAU SAI");
      delay(3000);
      lcd.clear();
      lcd.print("Nhap mat khau");
      lcd.setCursor(0,1);
      i=0;
      Serial.println("MAT KHAU SAI");
      sai++;
    }
  }
 }
}
// Chương trình thay đổi mật khẩu
void change_password(){
  int j=0;
  lcd.clear();
  lcd.print("Nhap mat khau cu");
  lcd.setCursor(0,1);
  while(j<4){
    char key=keypad_key.getKey();
    if(key){
      new_password[j++]=key;
      lcd.print(key);
      delay(500);
      lcd.setCursor(j-1,1);
      lcd.print("*");
    }
    key=0;
  }
  delay(500);

  if((strncmp(new_password, initial_password, 4))){//so sánh chuỗi kí tự giữa mật khẩu mới và mật khẩu ban đầu Có khác nhau ko nếu khác thực hiện hàm dưới 
    lcd.clear();
    lcd.print("MAT KHAU SAI");
    lcd.setCursor(0,1);
    lcd.print("Thu lai sau");
    delay(2000);
    Serial.println("THAY DOI MAT KHAU THAT BAI");
    sai++;
  }
  else{
    int k=0;
    int g=0;
    lcd.clear();
    sai=0;
    lcd.print("Mat khau moi");
    lcd.setCursor(0,1);
   while(k<4){
    char key=keypad_key.getKey();
    if(key !='#'&& key !='*'&& key != NO_KEY ){
      password1[k++]=key;
      lcd.print(key);
      delay(500);
      lcd.setCursor(k-1,1);
      lcd.print("*");
    }
    key=0;
  }
    delay(500);
    lcd.clear();
    lcd.print("Xac nhan mk");
    lcd.setCursor(0,1);
  while(g<4){
    char key=keypad_key.getKey();
    if(key){
      password2[g++]=key;
      lcd.print(key);
      delay(500);
      lcd.setCursor(g-1,1);
      lcd.print("*");
    }
    key=0;
  }
  if ((strncmp(password1, password2, 4))) {
    lcd.clear();

    
    lcd.setCursor(0,0);
    lcd.print("Doi mat khau");
    lcd.setCursor(0,1);
    lcd.print("Khong Thanh Cong");
    delay(1000); 
    Serial.println("DOI MAT KHAU THAT BAI");
  }
  else {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Doi mat khau");
    lcd.setCursor(0,1);
    lcd.print("Thanh Cong");
    delay(2000);
     for(g=0 ; g<4 ; g++){
       initial_password[g]=password2[g];
       EEPROM.write(g,password2[g]);
       delay(100);
  }
  Serial.println("DOI MAT KHAU THANH CONG");
  
    }
  }
  lcd.clear();
  lcd.print("Nhap mat khau");
  lcd.setCursor(0,1);
  key_pressed=0; 
  
  
}
// Chương trình mở khóa cửa bằng thẻ
void rfid() // chương trình đọc UID thẻ RFID
{
      if ( findID(readCard) ) { // Kiểm tra xem có UID trong EEPROM hay không, nếu có thông báo thẻ hợp lệ, mở cửa
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" The Hop Le ");
        lcd.setCursor(0, 1);
        lcd.print("Cua Dang Mo");
        Serial.println("THE HOP LE");
        Serial.println();
        digitalWrite(lock, HIGH);
        delay(5000);
        digitalWrite(lock, LOW);
        lcd.clear();
           
      }
      else {      // nếu k có báo thẻ không hợp lệ
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sai The");
        lcd.setCursor(0, 1);
        lcd.print("Hay Thu Lai");
        Serial.println(" SAI THE THU LAI SAU");
        delay(1000);
        lcd.clear();
       
      }
    }

// Đọc thẻ trong EEPROM
void readID( uint8_t number ) {
  uint8_t start = (number * 4 ) + 2;    
  for ( uint8_t i = 0; i < 4; i++ ) {     
    storedCard[i] = EEPROM.read(start + i);   
  }
}
void writeID( byte a[] ) {
  if ( !findID( a ) ) {     
    uint8_t num = EEPROM.read(5);     
    uint8_t start = ( num * 4 ) + 6;  
    num++;                
    EEPROM.write( 5, num );     
    for ( uint8_t j = 0; j < 4; j++ ) {   
      EEPROM.write( start + j, a[j] );  
    }
  }
  else {
    lcd.setCursor(0, 0);
    lcd.print("Failed!");
    lcd.setCursor(0, 1);
    lcd.print("wrong ID or bad EEPROM");
    delay(2000);
  }
}
// Hàm xóa thẻ
void deleteID( byte a[] ) {
  if ( !findID( a ) ) {     
    lcd.setCursor(0, 0);
    lcd.print("Failed!");
    lcd.setCursor(0, 1);
    lcd.print("wrong ID or bad EEPROM");
    delay(2000);
  }
  else {
    uint8_t num = EEPROM.read(5);   
    uint8_t slot;       
    uint8_t start;      
    uint8_t looping;    
    uint8_t j;
    uint8_t count = EEPROM.read(5); 
    slot = findIDSLOT( a );   
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      
    EEPROM.write( 5, num );   
    for ( j = 0; j < looping; j++ ) {         
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));  
    }
    for ( uint8_t k = 0; k < 4; k++ ) {         
      EEPROM.write( start + j + k, 0);
    }
  }
}
// Kiểm tra xem thẻ có trùng nhau không
boolean checkTwo ( byte a[], byte b[] ) { 
  if ( a[0] != 0 )      
    match = true;       
  for ( uint8_t k = 0; k < 4; k++ ) {   
    if ( a[k] != b[k] )    
      match = false;
  }
  if ( match ) {     
    return true;      
  }
  else  {
    return false;       
  }
}
// Tìm vị trí trống đầu tiên trong EEPROM
uint8_t findIDSLOT( byte find[] ) {
  uint8_t count = EEPROM.read(5);       
  for ( uint8_t i = 1; i <= count; i++ ) {
    readID(i);              
    if ( checkTwo( find, storedCard ) ) {   
      
      return i;         
      break;         
    }
  }
}

// Tìm xem có thẻ trong EEPROM hay k
boolean findID( byte find[] ) {
  uint8_t count = EEPROM.read(5);     
  for ( uint8_t i = 1; i <= count; i++ ) {   
    readID(i);          
    if ( checkTwo( find, storedCard ) ) {   
      return true;
      break;  
    }
    else {    
    }
  }
  return false;
}
// Đọc UID của thẻ
uint8_t getID() {
  // Getting ready for Reading Tags
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new Tag placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a Tag placed get Serial and continue
    return 0;
  }
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
  }
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}
// Mở cửa bằng Sim800L
void sim800l() {
if(Serial.available()){
    sim.write(Serial.read());
  }
  if(sim.available()>0){
    val = sim.readStringUntil('\n');
    Serial.println(val);
    if(val =="Open\r"){
    delay(1000);
    digitalWrite(lock, HIGH); 
    lcd.clear();
    lcd.print("Cua Dang Mo");
    Serial.println("CUA DANG MO");
    delay(5000);
    digitalWrite(lock, LOW);
    lcd.clear();
    lcd.print("Nhap mat khau");
    }
  }
  if(sai==3) {
    goidien(myphone);
    sai=0;
  }
}
void goidien(String phone)           
{
  sim.println("ATD" + phone + ";");          // Goi dien 
  delay(15000);                                 // Sau 10s
  sim.println("ATH");                        // Ngat cuoc goi
  delay(2000);
}

  
