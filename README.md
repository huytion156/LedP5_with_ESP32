# LedP5_with_ESP32 

**1. Xác định Input và Output của bảng LED: dựa vào chiều mũi tên, mũi tên chỉ từ input sang output** 

<img src="https://i.imgur.com/cagovdg.png">

**2. Gắn dây 16 Pin vào input**

<img src="https://imgur.com/RkC5GB6">

**3. Nối chân pin bảng LED với ESP32 theo sơ đồ sau:**

|Bảng LED P5 | ESP32|
|---------|------|
|OE|23|
|CLK|22|
|LAT|03|
|A|21|
|B|19|
|C|18|
|D|5|
|R1|17|
|G1|16|
|B1|4|
|R2|13|
|G2|2|
|B2|15|

Hàn mạch theo sơ đồ: 

<img src="https://i.imgur.com/qFTazNE.png"> 

**4. Tải thư viện tại** https://github.com/VGottselig/ESP32-RGB-Matrix-Display.git
