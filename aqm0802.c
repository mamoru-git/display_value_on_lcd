#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <gpiod.h>
#include <string.h>
#include <time.h>

#define I2C_ADDR 0x3E // AQM0802AのI2Cアドレス
#define GPIO_CHIP "gpiochip0"
#define GPIO_PIN 20
#define ON 1
#define OFF 0
struct gpiod_chip *chip;
struct gpiod_line *line;

//LCDに1byte出力する
void lcd_write_byte(int fd, unsigned char data, unsigned char mode) {
    char buf[2];
    buf[0] = mode;
    buf[1] = data;
    write(fd, buf, 2);
    usleep(1000); // 1msの遅延
}

//LCDに文字列を表示する
void lcd_write_string(int fd, const char *str) {
    while (*str) {
        lcd_write_byte(fd, *str, 0x40); // データ書き込みモード
        str++;
    }
}

//LCDをクリアする
void lcd_clr(int fd) {
  lcd_write_byte(fd, 0x01, 0x00);
  usleep(5000); // 5msの遅延
}

//LCDの表示行を指定する
void lcd_line_select(int fd,int line) {
  unsigned char command[2] = {0x80,0xC0};  
  int i = (line == 0) ? 0 : 1;
  lcd_write_byte(fd, command[i], 0x00);
  usleep(5000); // 5msの遅延
}

//LCDを初期化する
void lcd_init(int fd) {
    lcd_write_byte(fd, 0x38, 0x00); // 機能セット: 8ビットモード、2行表示
    lcd_write_byte(fd, 0x39, 0x00); // 機能セット: 拡張命令セット
    lcd_write_byte(fd, 0x14, 0x00); // 内部発振器周波数設定
    lcd_write_byte(fd, 0x73, 0x00); // コントラスト設定 (例)
    lcd_write_byte(fd, 0x56, 0x00); // 電源/フォロワ制御
    lcd_write_byte(fd, 0x6C, 0x00); // フォロワ制御
    lcd_write_byte(fd, 0x38, 0x00); // 機能セット: 基本命令セット
    lcd_write_byte(fd, 0x0C, 0x00); // 表示制御: 表示ON、カーソルOFF、点滅OFF
    lcd_write_byte(fd, 0x01, 0x00); // 画面クリア
    usleep(5000); // 5msの遅延
}

//GPIOラインを初期化する
int gpio_init() {
  chip = gpiod_chip_open_by_name(GPIO_CHIP);
  if (!chip) {
    perror("gpiod_chip_open_by_name");
    return 1;
  }

  line = gpiod_chip_get_line(chip, GPIO_PIN);
  if (!line) {
    perror("gpiod_chip_get_line");
    gpiod_chip_close(chip);
    return 1;
  }

  if (gpiod_line_request_output(line, "light_ctrl", 0) < 0) {
    perror("gpiod_line_request_output");
    gpiod_line_release(line);
    gpiod_chip_close(chip);
    return 1;
  }
  return 0;
}

//GPIOラインを解放する
void gpio_cleanup() {
  if (line) {
    gpiod_line_release(line);
  }
  if (chip) {
    gpiod_chip_close(chip);
  }
}

// LED(GPIO20に接続) を制御する関数
void light_ctrl(char onoff) {
  int value = (onoff == ON) ? 1 : 0;
  if (gpiod_line_set_value(line, value) < 0) {
    perror("gpiod_line_set_value");
  } else {
//    printf("GPIO%d set to %d\n", GPIO_PIN, value);
  }
}

//LCD表示メイン関数
//pipe経由で受信した指標値を 1行目：種別、2行目:値で表示する
int main() {
    int fd;
    char buffer[200];
    ssize_t len;
    char *token,*t1,*t2;
    char tb[20];
    char *key, *value;
    char *rest = buffer;

    time_t t = time(NULL);
    printf("現在時刻: %s", ctime(&t));

    if (gpio_init() != 0)
        exit(1);
    light_ctrl(ON);
    sleep(2);
    if ((fd = open("/dev/i2c-1", O_RDWR)) < 0) {
        perror("Failed to open i2c bus");
        exit(1);
    }
    if (ioctl(fd, I2C_SLAVE, I2C_ADDR) < 0) {
        perror("Failed to set i2c slave address");
        exit(1);
    }
    while ((len = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
//      printf("Received : %s len=%d\n", buffer ,(int)len);
      lcd_init(fd);
      while ((token = strtok_r(rest, ",", &rest))) {
          key = strtok_r(token, ":", &value);
          printf("%s :\n", key);
          lcd_line_select(fd,0);
          lcd_write_string(fd, key);
          printf("%s\n", value);
          lcd_line_select(fd,1);
          lcd_write_string(fd, value);
          sleep(2);
          lcd_clr(fd);
      }
    }
    lcd_clr(fd);
    light_ctrl(OFF);
    close(fd);
    return 0;
}
