#include <SoftwareSerial.h>
#include <avr/io.h>
#include <avr/interrupt.h>

//SoftwareSerial xbee(5, 4);//XBee UART bağlantısı 5-RX, 4-TX
/*
  Arduino -> XBee Adapter
  5(Rx)  ->   Tx(Dout)
  4(Tx)  ->   Rx(Din)
*/

unsigned int sayac = 0;//Hedefin vurulduğu Toplam sayı

int ilk_siddet = 0;//
int ortam_siddeti = 100;//1,5 saniye ortam gürültü ortalaması, daha sonra 1 önceki analog kanal değeri
bool ortam_dinleme = false;//false değeri için hedef atışa hazırdır
bool yeni_atis = false;//yeni bir dogru atış tespitinde true değerini alır bir sonraki atış bekleme için false yapılır
bool hazir = false;//atış tespitinde atış algılanmasında sonra gerekli olan sürenin geçmesi ve analog kalaın değerlendirmeye hazır olma durumunu tutar(true)

int fark;
int esik = 100;


//senkron bekleme
bool bekleme = true;
byte gelen_istek = 0;
unsigned long atis_sonu = 0;

void setup() {
  Serial.begin(9600);//com port a
  //xbee.begin(9600);//XBee
  pinMode(8, OUTPUT);//8. pin atış tespitinde kullanılacak led pini
  delay(1000);

  ADCSRA = 0x8F;      // Enable the ADC and its interrupt feature
  //and set the ACD clock pre-scalar to clk/128
  //ADMUX = 0xE0;     // Select internal 2.56V as Vref, left justify

  //ADC MODUL AYARLARI
  ADMUX |= (1 << REFS0); //AREF seçimi,REFS1 "0" kalır ve besleme gerilimi olan 5V referans alınır
  ADMUX &= 0xF0;//ADC kanalı seçimi(A0)*/
  ADCSRA |= (1 << ADSC); //ADC dönüşüm başlatma bayragını aktif etme, dönüşüm bittiğinde "0" olur.

  TCNT1 = 63191; // 1 saniye 15625 için >> 65535-15625, 63191 == 150ms
  TCCR1A = 0x00;//Normal mode
  TCCR1B &= (0 << CS11); // 1024 prescler
  TCCR1B |= (1 << CS10); // 1024 prescler
  TCCR1B |= (1 << CS12); // 1024 prescler
  TIMSK1 |= (1 << TOIE1); // Timer1 taşma kesmesi aktif

  sei(); //Global kesmeler devrede

}

void loop() {
  if (millis() < 1500) { // açılışta 1,5 saniye süresince ortamın gürültü ortalmasını hesaplanır

    ortam_siddeti = (ilk_siddet + ortam_siddeti) / 2;
  }
  else
  {
    ortam_dinleme = false;
    ////Serial.println("PUAN 4: ");
  }

  ADCSRA |= (1 << ADSC); //ADC dönüşüm başlatma bayragını aktif etme, dönüşüm bittiğinde "0" olur.
  if (yeni_atis == true) {
    yeni_atis = false;
    //Serial.print("YENİ ATIS GÖNDERİLDİ : ");
    //Serial.println(sayac);
    gonder(sayac, millis());

    //senkron
    atis_sonu = millis();
    bekleme = true;
  }

  while (bekleme) {
    ADCSRA |= (1 << ADSC); //ADC dönüşüm başlatma bayragını aktif etme, dönüşüm bittiğinde "0" olur.
    ////Serial.println("-!00");
    if (Serial.available() > 0) {
      //Serial.println("oke ");
      gelen_istek = Serial.read();

      if (gelen_istek == 101) //reset
      {
        sayac = 0;
        gonder(sayac, millis() - 2000);
        //bekleme=false;
        break;
      }
      else if (gelen_istek == 102)
      {
        //bekleme=false;
        gonder(sayac, millis() - 2000);
        break;
      }
      else if ((gelen_istek >= 141) && (gelen_istek <= 150)) {

        gelen_istek = gelen_istek % 140;
        switch (gelen_istek)
          {
          case 1:
            esik = 100;
            break;

          case 2:
            esik = 200;
            break;

          case 3:
            esik = 300;
            break;

          case 4:
            esik = 400;
            break;

          case 5:
            esik = 500;
            break;

          case 6:
            esik = 600;
            break;

          case 7:
            esik = 700;
            break;

          case 8:
            esik = 800;
            break;

          case 9:
            esik = 900;
            break;

          case 10:
            esik = 1000;
            break;

          default:
            esik = 100;
          }
        break;
      }
    }
    ////Serial.println("PUAN 4: ");
  }
}

void gonder(unsigned int veri, unsigned long start) {

  Serial.write(veri);

  unsigned long ilk = millis();
  unsigned int gelen;

  while ((millis() - start) < 5000) {

    if ((millis() - ilk) < 100) {

      //senkron ledi kapalı

      if (Serial.available()) {

        gelen = Serial.read();
        unsigned int ters = 255 - veri;

        if (gelen == ters) {
          ////Serial.println("onaylandı!!!!!!!!!!");
          //senkron ledi açık
          Serial.write(veri);
          break;
        }
      }
    } else {
      ////Serial.println("onay yok");
      gonder(veri, start);
      break;
    }
    ADCSRA |= (1 << ADSC); //ADC dönüşüm başlatma bayragını aktif etme, dönüşüm bittiğinde "0" olur.
  }
}

int analog_degerr = 0;//analog kanaldan gelen değerin saklandığı değişken 0-1024

ISR(ADC_vect) {
  analog_degerr = ADC;
  ////Serial.println("PUAN 5: ");
  if (ortam_dinleme == true) {
    ilk_siddet = analog_degerr;
    //Serial.println("PUAN 2:S ");
  }

  else {
    ////Serial.println("PUANS ");
    fark = abs(analog_degerr - ortam_siddeti);
    if (fark >= esik)
    {
      //Serial.println("PUAN : ");

      if (hazir == true) {
        //Serial.print("TOPLAM PUAN : ");
        sayac++;
        sayac = sayac % 100;
        //Serial.println(sayac);
        //Serial.println("");

        digitalWrite(8, HIGH);

        TCNT1 = 63191;// 65535-61600(1024 prescaler)=~150 ms
        TIFR1 |= (1 << TOV1) ;//timer1 taşma bayragı sıfırlanır.
        TIMSK1 |= (1 << TOIE1) ;// Timer1 taşma kesmesi aktif

        yeni_atis = true;
        hazir = false;

        //senkron
        bekleme = false;

      }
    }
    ortam_siddeti = analog_degerr;
  }
}

ISR (TIMER1_OVF_vect)    // Timer1 ISR
{
  TIMSK1 &= (0 << TOIE1) ;   // Timer1 taşma kesmesi devredışı (TOIE1)
  digitalWrite(8, LOW);
  hazir = true;  // for 1 sec at 16 MHz
}
