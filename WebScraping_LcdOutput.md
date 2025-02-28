
# 経済指標値表示システム

## 1. 目的
経済指標値(ドル円やS&P500その他日経平均等)をLCDに表示しライトを灯す。<br>
昨年１月より新NISAが始まり投資する人が増えた。アメリカ市場は、日本時間の夜から朝が取引時間である。気になる人は、携帯やPCで確認する人がほとんどであるが、寝てる時にいちいち起きてデバイスを起動するのは面倒である。<br>
そこで、Raspberry Pi４を使ってライトを灯し、経済指標値をLCDに表示するIoT機器を開発した。カスタマイズすることで、必要な指標値を必用な時に表示できるため、ストレスが減り、消費電力も抑えられることが期待される。<br>

## 2. 機能
WiFi経由でWebスクレイピングし経済指標値を取得する<br>
取得した指標値をLCDに表示する<br>
ライトを点灯する(LEDで代用)<br>
処理周期は、cron(Linux標準搭載の定期タスク実行機能)を利用することで指定可能<br>
Pythonプログラムの変更で指標値をLCDに表示するタイミングや条件を変更可能<br>

## 3. システム構成 
### 3.1. ハードウェア 
Raspberry Pi４<br>
２行☓８文字のLCD表示機(AQM0802A)：Raspberry Pi４のI2Cポートに接続する<br>
LED(ライトの代わり)：Raspberry Pi４のGPIOポート(20ピン)に接続する<br>
ブレッドボード、電線、抵抗<br>
### 3.2. ソフトウエア(★は今回作成したもの)
#### 3.2.1. アプリ
数分間隔でWebをスクレイピングし、経済指標値(ドル円、S&P500指標値等)を取得する（pythonプログラム★）<br>
システムコールを呼び出してI2Cを制御しLCDに表示する（C言語プログラム★）<br>
ライブラリを呼び出してGPIOを制御しライトを点灯する（C言語プログラム★）<br>
#### 3.2.2. ライブラリ(apt install して使用する) 
Requests(Webページの内容を取得する)<br>
BeautifulSoup(取得したWebデータを解析する)<br>
libgpiod(カーネルが提供するインターフェースを利用してGPIOを制御する)<br>
i2c-tools(i2cdetectでデバイス接続確認をする場合)<br>
#### 3.2.3. OS
ubuntu 24.04 カーネルバージョン：6.8.0-1018-rspi<br>
　
## 4. ソフトウエア仕様
### 4.1. Webスクレイピングし経済指標値を取得する
pythonプログラム(sgets_comm.py)<br> 
　`def get_price(url, selector):`<br>
　　選択項目を指定し目的の値を取得する<br>
　`def get_financial_data():`<br>
　　investing.comから”ドル円、日経平均、S&P500、10年米国債利回り、2年米国債利回り、金価格”を取得する<br>
　`def send_message(message):`<br>
　　pipeを利用してC言語プログラムへ伝達する<br>
　`if __name__ == "__main__":`<br>
　　上記処理を実行する　<br>
### 4.2. 取得した経済指標値をLCDに表示する
C言語プログラム(aqm0802.c)<br>
　`void lcd_init(int fd)`<br>
　　LCD(AQM0802A)を初期化する<br>
　　　機能セット: 8ビットモード、2行表示、 拡張命令セット、内部発振器周波数設定、コントラスト設定、電源/フォロワ制御<br>
　　　機能セット: 基本命令セット、表示制御: 表示ON、カーソルOFF、点滅OFF、画面クリア<br>
　`void lcd_write_byte(int fd, unsigned char data, unsigned char mode)`<br>
　　I2C経由に１バイト出力する<br>
　`void lcd_write_string(int fd, const char *str)`<br>
　　LCDに文字列を表示する。上記１バイト出力関数を呼び出す<br>
　`void lcd_line_select(int fd,int line)`<br>
　　LCDの表示行を指定する<br>
　`int main()`<br>
　　gpioを初期化する<br>
　　i2c用デバイスファイルをオープンする<br>
　　ライトを点灯する<br>
　　スクレイピング処理から伝達された経済指標データを１項目づつLCDに表示する<br>
　　ライトを消灯する<br>
### 4.3. ライトを点灯する 
C言語プログラム(aqm0802.c)<br>
　`int gpio_init()`<br>
　　GPIODライブラリを呼び出しGPIOラインの初期化をする<br>
　`void gpio_cleanup()`<br>
　　GPIOラインを開放する<br>
　`void light_ctrl(char onoff)`<br>
　　ラズパイGPIO(20ピン)にON/OFFを出力する<br>
### 4.4. 一連の処理(WebスクレイピングとLCD表示)を周期的に実行する
cronを利用しシェルスクリプトを周期的に実行する<br>
　crontabの内容(以下の場合3分毎に実施)<br>
　　crontab -e を実行後以下を記述（シェルスクリプトを置く位置に応じて適宜変更する）<br>
　　*/3 * * * * /home/user/lcd_show.sh <br>
　シェルの内容(lcd_show.sh)<br>
　　Webスクレイピングのための仮想環境のアクティベート<br>
　　pythonプログラム実行<br>
　　仮想環境のデアクティベート<br>
### 4.5. python プログラム から C言語プログラムへのLCD表示文字列データの伝達
pipeを用いて実施する<br>

## 5. 動画
ライト(LED)を点灯し、各指標値をLCDに順に表示し、最後にライトを消灯する動作を撮影<br>
https://github.com/muuyan66/display_value_on_lcd/blob/main/README.md

　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　以上