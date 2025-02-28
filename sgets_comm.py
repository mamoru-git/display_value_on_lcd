import requests
import time
import subprocess
from bs4 import BeautifulSoup

def get_price(url, selector):
    retries = 0
    max_retries = 2
    wait_time = 5
    while retries < max_retries:
        try:
            response = requests.get(url)
            response.raise_for_status()  # 200番台以外は例外を発生させる
            retries = max_retries #抜ける
        except requests.exceptions.HTTPError as e:
            if e.response.status_code == 403:
                retries += 1
                print(f"403エラー: {url} - リトライ {retries}/{max_retries}")
                time.sleep(wait_time)
            else:
                raise  # 403以外のエラーは再送しない
        except requests.exceptions.RequestException as e:
            print(f"リクエストエラー: {e}")
            return None  # リクエストエラーは再送しない

#    print(response)
    soup = BeautifulSoup(response.content, 'html.parser')
    # より具体的なセレクタを指定
    price_element = soup.find('div', {'data-test': 'instrument-price-last'})
    if price_element:
        price_str = price_element.text.strip().replace(',', '') # コンマを取り除く
        num = float(price_str)
        formatted_num = format(num, '.2f') #小数点以下２桁
        return formatted_num
    else:
        return None  # 要素が見つからない場合

def get_financial_data():
    data = []
    # S&P500
    sp500_url = "https://www.investing.com/indices/us-spx-500"
    sp500_price = get_price(sp500_url, "sp500")
    data.append("S&P500:"+sp500_price)

    # ドル円
    usdjpy_url = "https://www.investing.com/currencies/usd-jpy"
    usdjpy_price = get_price(usdjpy_url, "usdjpy")
    data.append("USDJPY:"+usdjpy_price)

    # 日経平均
    nikkei_url = "https://www.investing.com/indices/japan-ni225-historical-data"
    nikkei_price = get_price(nikkei_url, "nikkei")
    data.append("Nikkei:"+nikkei_price)

    # 10年米国債利回り
    us10y_url = "https://www.investing.com/rates-bonds/u.s.-10-year-bond-yield"
    us10y_yield = get_price(us10y_url, "us10y")
    data.append("US10Y:"+us10y_yield)

    # 2年米国債利回り
    us2y_url = "https://www.investing.com/rates-bonds/u.s.-2-year-bond-yield"
    us2y_yield = get_price(us2y_url, "us2y")
    data.append("US02Y:"+us2y_yield)

    # 金
    gold_url = "https://www.investing.com/commodities/gold-historical-data"
    gold_price = get_price(gold_url, "gold")
    data.append("US Gold:"+gold_price)
    return data

def send_message(message):
    p = subprocess.Popen(['./aqm0802'], stdin=subprocess.PIPE)
    p.communicate(input=message.encode())

if __name__ == "__main__":
    financial_data = get_financial_data()
#   print(financial_data)
    str_data = ",".join(str(x) for x in financial_data)
    send_message(str_data)

